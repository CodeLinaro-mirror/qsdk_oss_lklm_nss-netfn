/*
 * netfn_capwap_enc.c
 *	Network function's CAPWAP offload encapsulation.
 *
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/skbuff.h>
#include <linux/debugfs.h>
#include <linux/ipv6.h>

#include <net/udp.h>

#include "netfn_capwap.h"
#include "netfn_capwap_priv.h"
#include "netfn_capwap_hdr.h"
#include "netfn_capwap_tun.h"
#include "netfn_capwap_enc.h"

/*
 * netfn_capwap_enc_capwap()
 *	Encapsulate the CAPWAP header
 */
static void netfn_capwap_enc_capwap(struct netfn_capwap_tun_enc *enc, struct sk_buff *skb, struct netfn_capwap_enc_mdata *mdata)
{
	struct netfn_capwap_hdr *nch = (struct netfn_capwap_hdr *)__skb_push(skb, sizeof(*nch));
	struct netfn_capwap_prehdr *phdr = &mdata->phdr;
	union netfn_capwap_hdr_info info = {0};
	union netfn_capwap_hdr_frag frag = {0};
	uint8_t hlen = sizeof(*nch);

	/*
	 * Check if this packet has wireless info.
	 */
	if (phdr->type & NETFN_CAPWAP_PKT_TYPE_WINFO) {
		hlen += (phdr->nwireless * NETFN_CAPWAP_WIRELESS_INFO_LEN);
		info.fields.wbit = 1;
	}

	/*
	 * If packet is 802.11 packet.
	 */
	if ((phdr->type & NETFN_CAPWAP_PKT_TYPE_80211) && mdata->first_frag) {
		info.fields.tbit = info.fields.wbid = 1;
	}

	netfn_capwap_hdr_set_radio_id(&info, phdr->rid);
	info.fields.fbit = mdata->frag;
	info.fields.lbit = mdata->frag_end;

	info.fields.kbit = 0;
	info.fields.mbit = 0;
	info.fields.flags = 0;
	frag.fields.id = mdata->frag_id;
	netfn_capwap_hdr_set_hlen(&info, hlen);
	netfn_capwap_hdr_set_frag_offset(&frag, mdata->frag_offset);

	nch->info = htonl(info.word);
	nch->frag = htonl(frag.word);
}

/*
 * netfn_capwap_enc_l4()
 *	Encapsulate the UDP/UDPLite header
 */
static void netfn_capwap_enc_l4(struct netfn_capwap_tun_enc *enc, struct sk_buff *skb)
{
	struct udphdr *udph = (struct udphdr *)__skb_push(skb, sizeof(*udph));
	struct netfn_capwap_tun_enc_hdr *hdr = &enc->hdr;

	skb_reset_transport_header(skb);

	udph->dest = hdr->dest_port;
	udph->source = hdr->src_port;

	/*
	 * udph->len is set to payload len in case protocol is udp,
	 * else indicates checksum coverage for udplite protocol.
	 */
	udph->len = (hdr->proto_next_hdr == IPPROTO_UDP) ? htons(skb->len) : hdr->csum_cov;

	/*
	 * check = 0 means no checksum, however the intent is to have it offloaded to EDMA.
	 * skb->ip_summed should be set to CHECKSUM_PARTIAL
	 */
	udph->check = 0;
}

/*
 * netfn_capwap_enc_l3()
 *	Encapsulate the IP header
 */
static void netfn_capwap_enc_l3(struct netfn_capwap_tun_enc *enc, struct sk_buff *skb)
{
	struct netfn_capwap_tun_enc_hdr *hdr = &enc->hdr;
	struct ipv6hdr *ip6h;
	struct iphdr *iph;

	skb_reset_network_header(skb);
	skb->ip_summed = CHECKSUM_PARTIAL;

	if (hdr->ip_version == IPVERSION) {
		iph = (struct iphdr *)__skb_push(skb, sizeof(*iph));
		skb->protocol = htons(ETH_P_IP);

		iph->version = 4;
		iph->ihl = sizeof(*iph) >> 2;
		iph->tos = hdr->tos;
		iph->tot_len = htons(skb->len);
		iph->id = htons(enc->ipv4_id++);
		iph->frag_off = 0;
		iph->ttl = hdr->ttl;
		iph->frag_off = htons(IP_DF);
		iph->protocol = hdr->proto_next_hdr;
		iph->saddr = hdr->src_ip[0];
		iph->daddr = hdr->dest_ip[0];
		return;
	}

	/*
	 * IPv6 header encapsulation
	 */
	ip6h = (struct ipv6hdr *)__skb_push(skb, sizeof(*ip6h));
	skb->protocol = htons(ETH_P_IPV6);

	ip6h->version = 6;
	ip6_flow_hdr(ip6h, hdr->tos, 0);	/* Sets dscp and flow label. */
	ip6h->nexthdr = hdr->proto_next_hdr;
	ip6h->payload_len = htons(skb->len - sizeof(*ip6h));
	ip6h->hop_limit = hdr->ttl;
	/*
	 * TODO: Convert to in6_addr
	 */
	memcpy(ip6h->saddr.s6_addr32, hdr->src_ip, sizeof(ip6h->saddr.s6_addr32));
	memcpy(ip6h->daddr.s6_addr32, hdr->dest_ip, sizeof(ip6h->daddr.s6_addr32));
}

/*
 * netfn_capwap_enc_snap_hdr()
 *	Copy snap header with next protocol.
 */
static inline void netfn_capwap_enc_snap_hdr(struct netfn_capwap_tun_enc *enc, struct sk_buff *skb)
{
	uint16_t *next_proto = (uint16_t *)__skb_push(skb, sizeof(skb->protocol));
	uint8_t *snap_hdr = __skb_push(skb, NETFN_CAPWAP_SNAP_HDR_LEN);

	/*
	 * TODO: Try with struct object copy.
	 */
	*next_proto = skb->protocol;
	snap_hdr[0] = enc->snap_hdr[0];
	snap_hdr[1] = enc->snap_hdr[1];
	snap_hdr[3] = enc->snap_hdr[2];
}

/*
 * netfn_capwap_enc_80211()
 *	Converts the inner capwap packet from 802.3 to 802.11
 */
static void netfn_capwap_enc_80211(struct netfn_capwap_tun_enc *enc, struct sk_buff *skb, void *preh)
{
	struct netfn_capwap_prehdr *phdr = preh;
	struct ieee80211_hdr_3addr *wlan_hdr;
	struct netfn_capwap_winfo *winfo;
	uint16_t fc = 0, nwireless;
	uint8_t src_mac[ETH_ALEN];
	uint8_t dst_mac[ETH_ALEN];
	struct ethhdr *eth;

	eth = (struct ethhdr *)__skb_pull(skb, sizeof(*eth));
	nwireless = phdr->nwireless;

	/*
	 * Copy mac addresses.
	 */
	ether_addr_copy(src_mac, eth->h_source);
	ether_addr_copy(dst_mac, eth->h_dest);

	/*
	 * Insert trustsec if enabled.
	 */
	if (enc->features & NETFN_CAPWAP_FEATURES_INNER_SGT) {
		/*
		 * TODO: Add support when trustsec is enabled.
		 */
		BUG_ON(1);
	}

	/*
	 * Encapsulate with snap header.
	 */
	netfn_capwap_enc_snap_hdr(enc, skb);

	/*
	 * Insert QoS header.
	 */
	if (enc->features & NETFN_CAPWAP_FEATURES_WLAN_QOS) {
		uint16_t *qos = (uint16_t *)__skb_push(skb, sizeof(phdr->wlan_qos));

		fc |= IEEE80211_STYPE_QOS_DATA;
		*qos = phdr->wlan_qos;
	}

	/*
	 * Insert the wlan header.
	 */
	wlan_hdr = (struct ieee80211_hdr_3addr *)__skb_push(skb, sizeof(*wlan_hdr));
	skb_reset_inner_mac_header(skb);

	/*
	 * Copy over the MAC addresses.
	 */
	ether_addr_copy(wlan_hdr->addr1, (uint8_t *)&enc->bssid);
	ether_addr_copy(wlan_hdr->addr2, src_mac);
	ether_addr_copy(wlan_hdr->addr3, dst_mac);

	/*
	 * Fill frame control, duration and sequence.
	 */
	fc |= (IEEE80211_FTYPE_DATA | IEEE80211_FCTL_TODS);
	wlan_hdr->frame_control = htons(fc);

	/*
	 * Set duration and sequence.
	 */
	wlan_hdr->duration_id = htons(0);
	wlan_hdr->seq_ctrl = htons(phdr->vapid);

	/*
	 * Insert wlan info.
	 */
	memcpy(__skb_push(skb, sizeof(phdr->winfo)), &phdr->winfo, nwireless * sizeof(phdr->winfo));
}

/*
 * netfn_capwap_enc_frag()
 *	CAPWAP fragmentation routine
 */
static bool netfn_capwap_enc_frag(struct netfn_capwap_tun_enc *enc, struct sk_buff *orig_skb, struct sk_buff_head *q_frag)
{
	struct netfn_capwap_enc_stats *stats = &enc->stats;
	uint8_t *next_data = orig_skb->data;
	uint16_t total_len = orig_skb->len;
	uint16_t headroom, tailroom;
	uint16_t mtu = enc->mtu;
	int frag_len;

	headroom = skb_headroom(orig_skb);
	tailroom = skb_tailroom(orig_skb);

	/*
	 * If buffer size is less than next hop MTU no need to fragment.
	 */
	if (total_len <= mtu) {
		NETFN_CAPWAP_TUN_STATS_INC(&stats->num_frags);
		__skb_queue_head(q_frag, orig_skb);
		return true;
	}

	frag_len = mtu;
	total_len -= frag_len;
	skb_trim(orig_skb, frag_len);
	next_data += frag_len;

	NETFN_CAPWAP_TUN_STATS_INC(&stats->num_frags);
	__skb_queue_head(q_frag, orig_skb);

	/*
	 * This is a fragment loop.
	 */
	while (total_len > 0) {
		struct sk_buff *skb;
		ssize_t skb_size;

		frag_len = min(total_len, mtu);
		total_len -= frag_len;
		skb_size = frag_len + enc->outer_hdr_len + headroom + tailroom;

		skb = dev_alloc_skb(skb_size);
		if (!skb) {
			pr_debug("%px: Unable to allocate memory for fragment len(%ld).\n", enc, skb_size);
			NETFN_CAPWAP_TUN_STATS_INC(&stats->drop_mem_alloc);
			return false;
		}

		skb_reserve(skb, enc->outer_hdr_len + headroom);
		memcpy(__skb_put(skb, frag_len), next_data, frag_len);
		NETFN_CAPWAP_TUN_STATS_INC(&stats->num_frags);
		__skb_queue_head(q_frag, skb);
		next_data += frag_len;
	}

	return true;
}

/*
 * netfn_capwap_enc_stats_show()
 *	Prints the encap stats.
 */
static ssize_t netfn_capwap_enc_stats_show(struct file *fp, char __user *ubuf, size_t sz, loff_t *ppos)
{
	struct netfn_capwap_tun_enc *enc = fp->private_data;
	struct netfn_capwap_enc_stats *stats = &enc->stats;
	size_t enc_stats_sz, stats_sz;
	ssize_t bytes_read = 0;
	size_t size_wr = 0;
	char *buf;

	enc_stats_sz = sizeof(struct netfn_capwap_enc_stats) / sizeof(uint64_t) * NETFN_CAPWAP_ENC_STATS_STRLEN;
	stats_sz = (NETFN_CAPWAP_ENC_STATS_WIDTH * NETFN_CAPWAP_ENC_STATS_STRLEN) + enc_stats_sz;

	buf = vzalloc(stats_sz);
	if (!buf) {
		pr_warn("%px: Unable to allocate memory for stats.\n", fp);
		return -ENOMEM;
	}

	/*
	 * TODO: Convert to scnpritf engine.
	 * Print encap stats.
	 */
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "\n------------[encap stats start]----------\n");
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "keepalive_pkt", READ_ONCE(stats->keepalive_pkts));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "num_frags", READ_ONCE(stats->num_frags));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "drop_queue_full", READ_ONCE(stats->drop_queue_full));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t\t = %llu\n", "drop_mem_alloc", READ_ONCE(stats->drop_mem_alloc));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "drop_ver_mismatch", READ_ONCE(stats->drop_ver_mis));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "drop_insufficient_hroom", READ_ONCE(stats->drop_insufficient_hroom));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "drop_direct_dtls", READ_ONCE(stats->drop_direct_dtls));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "%s\t = %llu\n", "drop_nwireless_len", READ_ONCE(stats->drop_nwireless_len));
	size_wr += scnprintf(buf + size_wr, stats_sz - size_wr, "------------[encap stats end]-----------\n");

	bytes_read = simple_read_from_buffer(ubuf, sz, ppos, buf, size_wr);
	vfree(buf);
	return bytes_read;
}

/*
 * Capwap file operations.
 */
static const struct file_operations file_ops = {
	.open = simple_open,
	.llseek = default_llseek,
	.read = netfn_capwap_enc_stats_show
};

/*
 * netfn_capwap_enc_stats_read()
 *	Reads encap stats.
 */
void netfn_capwap_enc_stats_read(struct netfn_capwap_tun_enc *enc, struct netfn_capwap_tun_stats *stats)
{
	struct netfn_capwap_enc_stats *dest = &stats->enc;
	struct netfn_capwap_enc_stats *src = &enc->stats;

	/*
	 * Read encap stats.
	 */
	dest->keepalive_pkts = READ_ONCE(src->keepalive_pkts);
	dest->num_frags = READ_ONCE(src->num_frags);
	dest->drop_queue_full = READ_ONCE(src->drop_queue_full);
	dest->drop_mem_alloc = READ_ONCE(src->drop_mem_alloc);
	dest->drop_ver_mis = READ_ONCE(src->drop_ver_mis);
	dest->drop_insufficient_hroom = READ_ONCE(src->drop_insufficient_hroom);
	dest->drop_direct_dtls = READ_ONCE(src->drop_direct_dtls);
	dest->drop_nwireless_len = READ_ONCE(src->drop_nwireless_len);
}

/*
 * netfn_capwap_enc()
 *	Parse, fragment and encapsulate.
 */
bool netfn_capwap_enc(struct netfn_capwap_tun_enc *enc, struct sk_buff *orig_skb, struct sk_buff_head *q_frag)
{
	struct netfn_capwap_prehdr *phdr = (struct netfn_capwap_prehdr *)__skb_pull(orig_skb, sizeof(*phdr));
	struct netfn_capwap_enc_mdata mdata = {0};
	uint16_t offset = 0, num_frags;
	struct sk_buff *skb;
	bool is_frag;

	/*
	 * Pull the meta header.
	 */
	mdata.phdr = *phdr;

	/*
	 * If inner is 802.3 then we convert it to 802.11
	 */
	if (phdr->type & NETFN_CAPWAP_PKT_TYPE_8023) {
		netfn_capwap_enc_80211(enc, orig_skb, &mdata.phdr);
		mdata.phdr.type = NETFN_CAPWAP_PKT_TYPE_80211;
	}

	/*
	 * Fragment the skb if needed.
	 */
	if (unlikely(!netfn_capwap_enc_frag(enc, orig_skb, q_frag))) {
		pr_debug("%px: Fragmentation failed\n", enc);
		dev_kfree_skb_list_fast(q_frag);
		return false;
	}

	num_frags = skb_queue_len(q_frag);

	/*
	 * Check if its a fragmented packet.
	 */
	is_frag = !!(num_frags);
	mdata.frag_id = enc->frag_id++;

	/*
	 * Walk the frag list and encapsulate.
	 */
	skb_queue_walk(q_frag, skb) {
		num_frags--;
		if (likely(is_frag)) {
			mdata.frag_end = !num_frags;
			mdata.first_frag = !offset;
			mdata.frag_offset = offset;
			mdata.frag = true;
		}

		offset += skb->len;
		netfn_capwap_enc_capwap(enc, skb, &mdata);

		/*
		 * We don't encapsulate with UDP and IP if dtls is enabled.
		 * DTLS adds these for us.
		 */
		if (enc->features & NETFN_CAPWAP_FEATURES_DTLS) {
			continue;
		}

		netfn_capwap_enc_l4(enc, skb);	/* UDP or UDPLite */
		netfn_capwap_enc_l3(enc, skb);
	}

	return true;
}

/*
 * netfn_capwap_enc_init()
 *	Encapsulation initialization.
 */
bool netfn_capwap_enc_init(struct netfn_capwap_tun_enc *enc, struct netfn_capwap_enc_cfg *cfg, struct dentry *parent)
{
	struct netfn_tuple_5tuple *tuple = &cfg->tuple.tuples.tuple_5;
	struct netfn_capwap_tun_enc_hdr *hdr = &enc->hdr;

	ether_addr_copy(&enc->bssid, cfg->bssid);
	enc->outer_hdr_len = ETH_HLEN;

	/*
	 * TODO: Add vlan and pppoe.
	 */

	if (cfg->tuple.ip_version == IPVERSION) {
		hdr->ip_version = cfg->tuple.ip_version;
		hdr->src_ip[0] = tuple->src_ip[0];
		hdr->dest_ip[0] = tuple->dest_ip[0];

		enc->outer_hdr_len += sizeof(struct iphdr);
	} else {
		hdr->ip_version = cfg->tuple.ip_version;

		/*
		 * TODO: Change to direct assignment once switched to in6_addr
		 */
		memcpy(hdr->src_ip, tuple->src_ip, sizeof(uint32_t) * 4);
		memcpy(hdr->dest_ip, tuple->dest_ip, sizeof(uint32_t) * 4);

		enc->outer_hdr_len += sizeof(struct ipv6hdr);
	}

	enc->frag_id = 1;
	enc->ipv4_id = 1;
	hdr->tos = cfg->tos;
	hdr->csum_cov = cfg->csum_cov;
	hdr->src_port = tuple->l4_src_ident;
	hdr->dest_port = tuple->l4_dest_ident;

	if ((tuple->protocol != IPPROTO_UDP) && (tuple->protocol != IPPROTO_UDPLITE)) {
		pr_warn("%p: Invalid L4 proto(%d).\n", enc, tuple->protocol);
		return false;
	}

	hdr->proto_next_hdr = tuple->protocol;

	enc->outer_hdr_len += sizeof(struct udphdr);
	enc->outer_hdr_len += sizeof(struct netfn_capwap_hdr);
	enc->mtu = ((cfg->mtu - enc->outer_hdr_len) & ~(sizeof(uint64_t) - 1));
	hdr->ttl = cfg->ttl;

	/*
	 * Allocate debugfs entry.
	 */
	if (!debugfs_create_file("encap", S_IRUGO, parent, enc, &file_ops)) {
		pr_warn("%p: Failed to create encap debug entry.\n", enc);
		return false;
	}

	return true;
}
