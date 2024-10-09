/*
 * netfn_tcpst_rx.c
 *	Netfn TCP Speedtest receive file
 *
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
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
#include <linux/version.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/tcp.h>
#include <net/tcp.h>

#include "netfn_tcpst_priv.h"

/*
 * netfn_tcpst_http_get_timeout()
 *	Handles HTTP test timeout and offset
 */
static enum hrtimer_restart netfn_tcpst_http_get_timeout(struct hrtimer *timer)
{
	struct netfn_tcpst_conn *conn = container_of(timer, struct netfn_tcpst_conn, hrtimer);
	struct netfn_tcpst_stats_ext *ext_stats = &conn->ext_stats;
	struct netfn_tcpst_stats *stats = &conn->stats;

	/*
	 * We take the bytes sample at 2 instances:
	 * 1. When the offset timer expired
	 * 2. When the duration timer expired
	 */
	stats->test_bytes.rcvd = (ext_stats->test_rx_bytes - stats->test_bytes.rcvd);

	pr_debug("%px: Timer expired (offset(%d) duration (%d) state(%lxx)\n", conn, conn->offset, conn->duration, conn->timer_state);

	/*
	 * Check if offset timer expired
	 */
	if (test_and_clear_bit(NETFN_TCPST_TIMER_FWD_BIT, &conn->timer_state)) {
		stats->http.eom_time = ktime_get_real_ns();
		set_bit(NETFN_TCPST_TIMER_COMP_BIT, &conn->timer_state);
		return HRTIMER_NORESTART;
	}

	/*
	 * Set the offset timer bit and forward the timer for duration time
	 */
	set_bit(NETFN_TCPST_TIMER_FWD_BIT, &conn->timer_state);
	hrtimer_forward_now(timer, ktime_set(conn->duration, 0));
	return HRTIMER_RESTART;
}

/*
 * netfn_tcpst_rx_conn()
 *	Flush the packets to linux TCP stack
 */
static inline void netfn_tcpst_rx_conn(struct netfn_tcpst_conn *conn, struct sk_buff *skb)
{
	struct inet6_protocol *proto6;
	struct net_protocol *proto4;
	struct dst_entry *dst;
	struct socket *sock;
	struct sock *sk;

	/*
	 * Check if socket exists if not consume skb and return
	 */
	sock = READ_ONCE(conn->sock);
	if (!sock) {
		dev_kfree_skb(skb);
		pr_err("%px: Socket is destroyed already\n", conn);
		return;
	}

	sk = sock->sk;
	dst = rcu_dereference(sk->sk_rx_dst);

	/*
	 * Check if dst cache is obsolete else consume the skb and return
	 */
	dst = dst ? dst_check(dst, sk->sk_rx_dst_cookie) : NULL;
	if (!dst) {
		dev_kfree_skb(skb);
		conn->ext_stats.dst_miss_pkts += !dst;
		return;
	}

	skb_dst_set_noref(skb, dst);

	/*
	 * Hand over to linux
	 */
	if (skb->protocol == htons(ETH_P_IP)) {
		proto4 = rcu_dereference(inet_protos[IPPROTO_TCP]);
		proto4->handler(skb);
		return;
	}

	proto6 = rcu_dereference(inet6_protos[IPPROTO_TCP]);
	proto6->handler(skb);
}

/*
 * netfn_tcpst_http_get_resp()
 *	Parse and verify the response
 */
static enum netfn_tcpst_err netfn_tcpst_http_get_resp(struct netfn_tcpst_conn *conn)
{
	struct msghdr msg = {0};
	char *buf = conn->buf;
	struct kvec iov;
	int rx_bytes;
	char *data;

	/*
	 * Prepare receive message
	 */
	msg.msg_flags |= (MSG_WAITALL | MSG_TRUNC);
	iov.iov_base = buf;
	iov.iov_len = NETFN_TCPST_HTTP_RESPONSE_LEN;

	/*
	 * Wait for HTTP response
	 */
	rx_bytes = kernel_recvmsg(READ_ONCE(conn->sock), &msg, &iov, 1, NETFN_TCPST_HTTP_RESPONSE_LEN, 0);
	if (rx_bytes < 0) {
		pr_warn("%px: Error in kernel_recvmsg\n", conn);
		return NETFN_TCPST_ERR_RX;
	}

	pr_debug("%px: Response received(%s)\n", conn, buf);

	/*
	 * Check if response has "200 OK" in response
	 */
	if (!strnstr(buf, "200 OK", strlen(buf))) {
		pr_warn("%px: Invalid response\n", conn);
		return NETFN_TCPST_ERR_CONNECT;
	}

	data = strnstr(buf, "Content-Length: ", strlen(buf));
	if (!data) {
		pr_warn("%px: Can't find Content-length string\n", conn);
		return NETFN_TCPST_ERR_CONNECT;
	}

	data += sizeof("Content-Length: ");
	conn->http.file_sz = simple_strtol(data, NULL, 10);

	/*
	 * We only count the data bytes, so we subtract strlen(conn->buf)
	 * from total bytes received
	 */
	conn->ext_stats.test_rx_bytes += (rx_bytes - strlen(conn->buf));
	conn->ext_stats.test_rx_pkts += !!rx_bytes;
	return NETFN_TCPST_ERR_NONE;
}

/*
 * netfn_tcpst_http_get()
 *	HTTP GET support
 */
static bool netfn_tcpst_http_get(struct netfn_tcpst_conn *conn)
{
	enum netfn_tcpst_err err = NETFN_TCPST_ERR_NONE;
	struct netfn_tcpst_stats *stats = &conn->stats;
	struct netfn_tcpst_http *http = &conn->http;
	struct msghdr msg = {0};
	struct kvec iov = {0};
	int len;

	/*
	 * Prepare HTTP PUT message
	 */
	msg.msg_flags |= MSG_MORE;
	iov.iov_base = http->hdr;
	iov.iov_len = http->hdr_len;

	/*
	 * Send HTTP GET request to server
	 */
	len = kernel_sendmsg(READ_ONCE(conn->sock), &msg, &iov, 1, http->hdr_len);
	if (len < 0) {
		pr_warn("%px: Error in kernel_sendmsg\n", conn);
		return false;
	}

	/*
	 * Send a GET command to request the contents of the file and
	 * record current timestamp as ROMTime.
	 */
	stats->http.rom_time = ktime_get_real_ns();

	err = netfn_tcpst_http_get_resp(conn);
	if (err != NETFN_TCPST_ERR_NONE) {
		pr_warn("%px: Invalid response received aborting\n", conn);
		return false;
	}

	/*
	 * Increment bom_time for first response only
	 */
	stats->http.bom_time = ktime_get_real_ns();
	return true;
}

/*
 * netfn_tcpst_rx_vp4()
 *	PPE-VP callback for IPv4
 */
bool netfn_tcpst_rx_vp4(struct ppe_vp_cb_info *info, void *cb_data)
{
	struct netfn_tcpst_ctx *ctx = (struct netfn_tcpst_ctx *)cb_data;
	struct sk_buff *skb = info->skb;
	struct netfn_tcpst_conn *conn;

	conn = &ctx->conn[smp_processor_id()];

	/*
	 * Check if connection socket exists,
	 * if not then consume skb and return
	 */
	if (!READ_ONCE(conn->sock)) {
		dev_kfree_skb(skb);
		pr_warn("%px: Connection already destroyed\n", ctx);
		return true;
	}

	conn->ext_stats.vp_rx_pkts++;

	/*
	 * Set skb fields
	 */
	skb->ip_summed = info->ip_summed;
	skb->pkt_type = PACKET_HOST;

	skb_reset_network_header(skb);
	__skb_pull(skb, sizeof(struct iphdr));
	skb_reset_transport_header(skb);

	/*
	 * Hand over to linux
	 */
	rcu_read_lock_bh();
	netfn_tcpst_rx_conn(conn, skb);
	rcu_read_unlock_bh();
	return true;
}

/*
 * netfn_tcpst_rx_vp6()
 *	PPE-VP callback for IPv6
 */
bool netfn_tcpst_rx_vp6(struct ppe_vp_cb_info *info, void *cb_data)
{
	struct netfn_tcpst_ctx *ctx = (struct netfn_tcpst_ctx *)cb_data;
	struct sk_buff *skb = info->skb;
	struct netfn_tcpst_conn *conn;

	conn = &ctx->conn[smp_processor_id()];

	/*
	 * Check if connection socket exists,
	 * if not then consume skb and return
	 */
	if (!READ_ONCE(conn->sock)) {
		dev_kfree_skb(skb);
		pr_warn("%px: Socket already destroyed\n", ctx);
		return true;
	}

	conn->ext_stats.vp_rx_pkts++;

	/*
	 * Set skb fields
	 */
	skb->ip_summed = info->ip_summed;
	skb->pkt_type = PACKET_HOST;

	skb_reset_network_header(skb);
	__skb_pull(skb, sizeof(struct ipv6hdr));
	skb_reset_transport_header(skb);

	/*
	 * Hand over to linux
	 */
	rcu_read_lock_bh();
	netfn_tcpst_rx_conn(conn, skb);
	rcu_read_unlock_bh();
	return true;
}

/*
 * netfn_tcpst_http_get_data()
 *	HTTP TCP receive data
 */
void netfn_tcpst_http_get_data(struct work_struct *work)
{
	struct netfn_tcpst_conn *conn = container_of(work, struct netfn_tcpst_conn, work);
	struct netfn_tcpst_stats_ext *ext_stats = &conn->ext_stats;
	struct netfn_tcpst_stats *stats = &conn->stats;
	bool resched, eom, timeout, abort;
	struct msghdr msg = {0};
	struct kvec iov = {0};
	int rx_bytes;

	/*
	 * Initialize the msg
	 */
	msg.msg_flags |= (MSG_WAITALL | MSG_TRUNC);
	iov.iov_base = conn->buf;
	iov.iov_len = NETFN_TCPST_BUF_LEN;

	/*
	 * Read the data from socket
	 */
	rx_bytes = kernel_recvmsg(READ_ONCE(conn->sock), &msg, &iov, 1, NETFN_TCPST_BUF_LEN, msg.msg_flags);
	if (rx_bytes < 0) {
		pr_warn("%px: Error in kernel_recvmsg aborting on cpu(%d)\n", conn, smp_processor_id());
		netfn_tcpst_state_set(conn->ctx, NETFN_TCPST_STATE_ABORTED, netfn_tcpst_state_get(conn->ctx));
		return;
	}

	/*
	 * Increment the bytes and pkt counter
	 */
	ext_stats->test_rx_bytes += rx_bytes;
	ext_stats->test_rx_pkts += !!rx_bytes;

	/*
	 * Reschedule work if we haven't received all data or no timeout received
	 */
	eom = (ext_stats->test_rx_bytes >= conn->http.file_sz);
	timeout = test_bit(NETFN_TCPST_TIMER_COMP_BIT, &conn->timer_state);
	abort = (netfn_tcpst_state_get(conn->ctx) == NETFN_TCPST_STATE_ABORTED);
	resched = (!eom && !timeout && !abort);
	if (likely(resched) && likely(schedule_work_on(smp_processor_id(), work))) {
		return;
	}

	pr_debug("%px: Not scheduling eom(%d) timeout(%d) abort(%d)\n", conn, eom, timeout, abort);

	/*
	 * Check if duration timer has expired. For timebound test
	 * the stats are captured in timer callback.
	 */
	if (test_bit(NETFN_TCPST_TIMER_COMP_BIT, &conn->timer_state)) {
		netfn_tcpst_state_set(conn->ctx, NETFN_TCPST_STATE_COMPLETED, NETFN_TCPST_STATE_INPROGRESS);
		pr_debug("%px: Timeout occured, marking test completed\n", conn);
		return;
	}

	/*
	 * If EOM is received and its timebound test then we abort.
	 */
	if (eom && conn->duration) {
		netfn_tcpst_state_set(conn->ctx, NETFN_TCPST_STATE_ABORTED, netfn_tcpst_state_get(conn->ctx));
		pr_debug("%px: EOM received before timeout, marking test aborted\n", conn);
		return;
	}

	/*
	 * Update the stats
	 */
	stats->test_bytes.rcvd = ext_stats->test_rx_bytes;
	stats->http.eom_time = ktime_get_real_ns();
	netfn_tcpst_state_set(conn->ctx, NETFN_TCPST_STATE_COMPLETED, NETFN_TCPST_STATE_INPROGRESS);
	pr_debug("%px: EOM received, marking test completed\n", conn);
	return;
}

/*
 * netfn_tcpst_http_get_work()
 *	TCP download support
 */
void netfn_tcpst_http_get_work(struct work_struct *work)
{
	struct netfn_tcpst_conn *conn = container_of(work, struct netfn_tcpst_conn, work);
        enum netfn_tcpst_err err;

        if (!netfn_tcpst_http_get(conn)) {
                pr_warn("%px: Error(%d) in connecting to HTTP server\n", work, err);
		netfn_tcpst_state_set(conn->ctx, NETFN_TCPST_STATE_ABORTED, netfn_tcpst_state_get(conn->ctx));
                return;
        }

	/*
	 * Start the timer only if its time based test
	 */
	if (conn->duration) {
		conn->hrtimer.function = netfn_tcpst_http_get_timeout;
		hrtimer_start(&conn->hrtimer, ktime_set(conn->offset, 0), HRTIMER_MODE_REL);
	}

	/*
	 * Override work function for HTTP data path
	 */
        INIT_WORK(work, netfn_tcpst_http_get_data);

        /*
	 * Schedule the work to receive the HTTP Data
         */
        schedule_work_on(smp_processor_id(), work);
}
