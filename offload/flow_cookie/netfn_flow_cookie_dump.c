/*
 * Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include <linux/debugfs.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <netfn_flow_cookie.h>
#include "netfn_flow_cookie_dump.h"
#include "netfn_flow_cookie_priv.h"

/*
 * Debugfs dentry object.
 */
static struct dentry *netfn_flow_cookie_dump_dentry;

/*
 * Character device stuff - used to communicate status back to user space
 */
static int netfn_flow_cookie_dump_dev_major_id;	/* Major ID of registered char dev from which we can dump out state to userspace */

/*
 * netfn_flow_cookie_dump_write_reset()
 *	Reset the msg buffer, specifying a new initial prefix
 *
 * Returns 0 on success
 */
int netfn_flow_cookie_dump_write_reset(struct netfn_flow_cookie_dump_instance *fdi, char *prefix)
{
	int result;

	fdi->msgp = fdi->msg;
	fdi->msg_len = 0;

	result = snprintf(fdi->prefix, NETFN_FLOW_COOKIE_DUMP_FILE_PREFIX_SIZE, "%s", prefix);
	if ((result < 0) || (result >= NETFN_FLOW_COOKIE_DUMP_FILE_PREFIX_SIZE)) {
		return -1;
	}

	fdi->prefix_level = 0;
	fdi->prefix_levels[fdi->prefix_level] = result;

	return 0;
}

/*
 * netfn_flow_cookie_dump_prefix_add()
 *	Add another level to the prefix
 *
 * Returns 0 on success
 */
int netfn_flow_cookie_dump_prefix_add(struct netfn_flow_cookie_dump_instance *fdi, char *prefix)
{
	int pxsz;
	int pxremain;
	int result;

	pxsz = fdi->prefix_levels[fdi->prefix_level];
	pxremain = NETFN_FLOW_COOKIE_DUMP_FILE_PREFIX_SIZE - pxsz;

	result = snprintf(fdi->prefix + pxsz, pxremain, ".%s", prefix);
	if ((result < 0) || (result >= pxremain)) {
		return -1;
	}

	fdi->prefix_level++;

	netfn_flow_cookie_assert(fdi->prefix_level < NETFN_FLOW_COOKIE_DUMP_FILE_PREFIX_LEVELS_MAX, "Bad prefix handling\n");
	fdi->prefix_levels[fdi->prefix_level] = pxsz + result;

	return 0;
}

/*
 * netfn_flow_cookie_dump_prefix_index_add()
 *	Add another level (numeric) to the prefix
 *
 * Returns 0 on success
 */
int netfn_flow_cookie_dump_prefix_index_add(struct netfn_flow_cookie_dump_instance *fdi, uint32_t index)
{
	int pxsz;
	int pxremain;
	int result;

	pxsz = fdi->prefix_levels[fdi->prefix_level];
	pxremain = NETFN_FLOW_COOKIE_DUMP_FILE_PREFIX_SIZE - pxsz;
	result = snprintf(fdi->prefix + pxsz, pxremain, ".%u", index);
	if ((result < 0) || (result >= pxremain)) {
		return -1;
	}

	fdi->prefix_level++;
	fdi->prefix_levels[fdi->prefix_level] = pxsz + result;

	return 0;
}

/*
 * netfn_flow_cookie_dump_prefix_remove()
 *	Remove level from the prefix
 *
 * Returns 0 on success
 */
int netfn_flow_cookie_dump_prefix_remove(struct netfn_flow_cookie_dump_instance *fdi)
{
	int pxsz;

	fdi->prefix_level--;
	if (fdi->prefix_level < 0) {
		netfn_flow_cookie_warn("Bad prefix handling\n");
		return 0;
	}

	pxsz = fdi->prefix_levels[fdi->prefix_level];
	fdi->prefix[pxsz] = 0;

	return 0;
}

/*
 * netfn_flow_cookie_dump_write()
 *	Write out to the message buffer, prefix is added automatically.
 *
 * Returns 0 on success
 */
int netfn_flow_cookie_dump_write(struct netfn_flow_cookie_dump_instance *fdi, char *name, char *fmt, ...)
{
	int remain;
	char *ptr;
	int result;
	va_list args;

	remain = NETFN_FLOW_COOKIE_DUMP_FILE_BUFFER_SIZE - fdi->msg_len;
	ptr = fdi->msg + fdi->msg_len;
	result = snprintf(ptr, remain, "%s.%s=", fdi->prefix, name);
	if ((result < 0) || (result >= remain)) {
		return -1;
	}

	fdi->msg_len += result;
	remain -= result;
	ptr += result;

	va_start(args, fmt);
	result = vsnprintf(ptr, remain, fmt, args);
	va_end(args);
	if ((result < 0) || (result >= remain)) {
		return -2;
	}

	fdi->msg_len += result;
	remain -= result;
	ptr += result;

	result = snprintf(ptr, remain, "\n");
	if ((result < 0) || (result >= remain)) {
		return -3;
	}

	fdi->msg_len += result;
	return 0;
}

/*
 * netfn_flow_cookie_dump_info()
 *	Prepare the flow dump message here
 *	for both the ipv4 and ipv6 flows.
 */
int netfn_flow_cookie_dump_info(struct netfn_flow_cookie_dump_instance *fdi)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	int result;
	struct netfn_flow_cookie_db *d;
	int i;
	uint32_t hash_idx;
	struct netfn_flow_cookie_db_entry *flow;
	uint32_t node_idx;
	uint32_t dump_src_addr[4];
	uint32_t dump_dst_addr[4];

	netfn_flow_cookie_info("Inside netfn flow dump get main api start");

	spin_lock_bh(&nfc_ctx->lock);

	for (i = 0; i < NETFN_FLOW_COOKIE_DB_ARRAY_SIZE; i = i + 1) {

		rcu_read_lock_bh();

		d = nfc_ctx->db_array[i];

		if (!d) {
			netfn_flow_cookie_warn("DB handle is NULL at index, i:%d", i);
			rcu_read_unlock_bh();
			continue;
		}
		if (netfn_flow_cookie_dump_prefix_add(fdi, "db")) {
			goto netfn_flow_cookie_dump_write_error;
		}
		if (netfn_flow_cookie_dump_prefix_index_add(fdi, i)) {
			goto netfn_flow_cookie_dump_write_error;
		}

		for (hash_idx = 0; hash_idx < d->hash_size; hash_idx = hash_idx+1) {

			/*
			 * Extract information from the DB Hash table for inclusion into the message.
			 */
			struct hlist_node *node_head_ptr = hlist_first_rcu(&(d->db_table[hash_idx]));
			flow = hlist_entry(node_head_ptr, struct netfn_flow_cookie_db_entry, node);

			if (unlikely(!node_head_ptr) || unlikely(!flow)) {
				continue;
			}

			/*
			 * Perform suitable prefix add to the message.
			 */
			if (netfn_flow_cookie_dump_prefix_add(fdi, "hash_idx")) {
				goto netfn_flow_cookie_dump_write_error;
			}

			/*
			 *  Perform suitable prefix index add to the message.
			 */
			if (netfn_flow_cookie_dump_prefix_index_add(fdi, hash_idx)) {
				goto netfn_flow_cookie_dump_write_error;
			}

			/*
			 * Perform suitable prefix add to the message.
			 */
			if (netfn_flow_cookie_dump_prefix_add(fdi, "db_entry")) {
				goto netfn_flow_cookie_dump_write_error;
			}

			hlist_for_each_entry_rcu(flow, &(d->db_table[hash_idx]), node) {
				if (unlikely(!flow)) {
					break;
				} else {

					if (netfn_flow_cookie_dump_prefix_index_add(fdi, node_idx)) {
						goto netfn_flow_cookie_dump_write_error;
					}

					if (flow->tuple.ip_version == NETFN_FLOWMGR_TUPLE_IP_VERSION_V4) {

					       /*
						* We perform the ntohl operations on these IPV4
						* source and destination IP addresses to ensure that
						* the data is dumped in the correct order on the console.
						*/
						dump_src_addr[0] = flow->tuple.tuples.tuple_5.src_ip.ip4.s_addr;
						dump_dst_addr[0] = flow->tuple.tuples.tuple_5.dest_ip.ip4.s_addr;

					       /*
						* Perform the flow dump write for source ip address.
						*/
						if (netfn_flow_cookie_dump_write(fdi, "sip_address", "%pI4", dump_src_addr)) {
							goto netfn_flow_cookie_dump_write_error;
						}

					       /*
						* Perform the flow dump write for destination ip address.
						*/
						if (netfn_flow_cookie_dump_write(fdi, "dip_address", "%pI4", dump_dst_addr)) {
							goto netfn_flow_cookie_dump_write_error;
						}
					} else if (flow->tuple.ip_version == NETFN_FLOWMGR_TUPLE_IP_VERSION_V6) {

					       /*
						* We perform the htonl operations on these IPV6
						* source and destination IP addresses to ensure that
						* the data is dumped in the correct order on the console.
						*/
						dump_src_addr[0] = flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[0];
						dump_src_addr[1] = flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[1];
						dump_src_addr[2] = flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[2];
						dump_src_addr[3] = flow->tuple.tuples.tuple_5.src_ip.ip6.s6_addr32[3];
						dump_dst_addr[0] = flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[0];
						dump_dst_addr[1] = flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[1];
						dump_dst_addr[2] = flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[2];
						dump_dst_addr[3] = flow->tuple.tuples.tuple_5.dest_ip.ip6.s6_addr32[3];

					       /*
						* Perform the flow dump write for source ip address.
						*/
						if (netfn_flow_cookie_dump_write(fdi, "sip_address", "%pI6", dump_src_addr)) {
							goto netfn_flow_cookie_dump_write_error;
						}

					       /*
						* Perform the flow dump write for destination ip address.
						*/
						if (netfn_flow_cookie_dump_write(fdi, "dip_address", "%pI6", dump_dst_addr)) {
							goto netfn_flow_cookie_dump_write_error;
						}
					}

				       /*
					* Perform the flow dump write for source port.
					*/
					if (netfn_flow_cookie_dump_write(fdi, "src_port", "%u", ntohs(flow->tuple.tuples.tuple_5.l4_src_ident))) {
						goto netfn_flow_cookie_dump_write_error;
					}

				       /*
					* Perform the flow dump write for destination port.
					*/
					if (netfn_flow_cookie_dump_write(fdi, "dst_port", "%u", ntohs(flow->tuple.tuples.tuple_5.l4_dest_ident))) {
						goto netfn_flow_cookie_dump_write_error;
					}

				       /*
					* Perform the flow dump write for protocol.
					*/
					if (netfn_flow_cookie_dump_write(fdi, "protocol", "%u", flow->tuple.tuples.tuple_5.protocol)) {
						goto netfn_flow_cookie_dump_write_error;
					}

				       /*
					* Perform the flow dump write for IP Version.
					*/
					if (netfn_flow_cookie_dump_write(fdi, "ip_ver", "%u", flow->tuple.ip_version)) {
						goto netfn_flow_cookie_dump_write_error;
					}

				       /*
					* Perform the flow dump write for Flow ID.
					*/
					if (netfn_flow_cookie_dump_write(fdi, "flow_id", "%u", (flow->flow_cookie).flow_id)) {
						goto netfn_flow_cookie_dump_write_error;
					}

				       /*
					* Perform the flow dump write for Sawf Handle.
					*/
					if (netfn_flow_cookie_dump_write(fdi, "sawf_handle", "%u", (flow->flow_cookie).sawf_handle.sawf_id)) {
						goto netfn_flow_cookie_dump_write_error;
					}

				       /*
					* Perform the flow dump write for Flow Mark.
					*/
					if (netfn_flow_cookie_dump_write(fdi, "flow_mark", "%u", (flow->flow_cookie).flow_mark)) {
						goto netfn_flow_cookie_dump_write_error;
					}

					/*
					 * Perform the flow dump write for SCS ID.
					 */
					if (netfn_flow_cookie_dump_write(fdi, "scs_id", "%u", (flow->flow_cookie).scs_id)) {
						goto netfn_flow_cookie_dump_write_error;
					}

				       /*
					* Perform the flow dump write for Flow Hit Count.
					*/
					if (netfn_flow_cookie_dump_write(fdi, "flow_hit_count", "%lld", atomic64_read(&flow->Hit_count))) {
						goto netfn_flow_cookie_dump_write_error;
					}

				       /*
					* We keep iterating for the subsequent DB entries
					* if exist in the Hash chain.
					*/
					node_idx++;

				       /*
					* Perform suitable prefix remove to the message.
					*/
					if (netfn_flow_cookie_dump_prefix_remove(fdi)) {
						goto netfn_flow_cookie_dump_write_error;
					}
				}
			}

		       /*
			* Perform suitable prefix remove to the message.
			*/
			if (netfn_flow_cookie_dump_prefix_remove(fdi)) {
				goto netfn_flow_cookie_dump_write_error;
			}

		       /*
			* Perform suitable prefix remove to the message.
			*/
			if (netfn_flow_cookie_dump_prefix_remove(fdi)) {
				goto netfn_flow_cookie_dump_write_error;
			}

		       /*
			* Perform suitable prefix remove to the message.
			*/
			if (netfn_flow_cookie_dump_prefix_remove(fdi)) {
				goto netfn_flow_cookie_dump_write_error;
			}
			node_idx = 0;
		}

		/*
		 * Perform suitable prefix remove to the message.
		 */
		if (netfn_flow_cookie_dump_prefix_remove(fdi)) {
			goto netfn_flow_cookie_dump_write_error;
		}

		/*
		 * Perform suitable prefix remove to the message.
		 */
		if (netfn_flow_cookie_dump_prefix_remove(fdi)) {
			goto netfn_flow_cookie_dump_write_error;
		}

		rcu_read_unlock_bh();
	}

	spin_unlock_bh(&nfc_ctx->lock);

	return 0;

netfn_flow_cookie_dump_write_error:
	rcu_read_unlock_bh();
	spin_unlock_bh(&nfc_ctx->lock);
	return result;
}

/*
 * netfn_flow_cookie_dump_get()
 *	Prepare the flow dump message
 *	for both the ipv4 and ipv6 flows.
 */
static bool netfn_flow_cookie_dump_get(struct netfn_flow_cookie_dump_instance *fdi)
{
	int result;

	if (netfn_flow_cookie_dump_write_reset(fdi, "netfn_flow")) {
		netfn_flow_cookie_warn("Write reset failed for netfn_flow");
		return result;
	}

	netfn_flow_cookie_info("Prep conn msg for %px\n", fdi);

	return netfn_flow_cookie_dump_info(fdi);
}

/*
 * netfn_flow_cookie_dump_dev_open()
 *	Opens the special char device file which we use to dump our state.
 */
static int netfn_flow_cookie_dump_dev_open(struct inode *inode, struct file *file)
{
	struct netfn_flow_cookie_gbl *nfc_ctx = &netfn_flow_cookie_gbl_ctx;
	struct netfn_flow_cookie_dump_instance *fdi;

	netfn_flow_cookie_info("Performing Netfn Flow Cookie flow_dump open\n");

	/*
	 * Allocate state information for the reading
	 */
	netfn_flow_cookie_assert(file->private_data == NULL, "Unexpected double open: %px?\n", file->private_data);

	fdi = (struct netfn_flow_cookie_dump_instance *)kzalloc(sizeof(struct netfn_flow_cookie_dump_instance), GFP_ATOMIC | __GFP_NOWARN);
	if (!fdi) {
		return -ENOMEM;
	}

	file->private_data = fdi;
	fdi->dump_en = true;

	netfn_flow_cookie_info("Done with netfn flow cookie flow dump device open, db_array:%p", nfc_ctx->db_array);

	return 0;
}

/*
 * netfn_flow_cookie_dump_dev_release()
 *	Called when a process closes the device file.
 */
static int netfn_flow_cookie_dump_dev_release(struct inode *inode, struct file *file)
{
	struct netfn_flow_cookie_dump_instance *fdi;

	fdi = (struct netfn_flow_cookie_dump_instance *)file->private_data;

	if (fdi) {
		kfree(fdi);
	}

	return 0;
}

/*
 * netfn_flow_cookie_dump_dev_read()
 *	Called to read the state
 */
static ssize_t netfn_flow_cookie_dump_dev_read(struct file *file,	/* see include/linux/fs.h   */
		char *buffer,				/* buffer to fill with data */
		size_t length,				/* length of the buffer     */
		loff_t *offset)				/* Doesn't apply - this is a char file */
{
	struct netfn_flow_cookie_dump_instance *fdi;
	int bytes_read = 0;

	fdi = (struct netfn_flow_cookie_dump_instance *)file->private_data;
	if (!fdi) {
		return -ENOMEM;
	}

	/*
	 * If there is still some message remaining to be output then complete that first
	 */
	if (fdi->msg_len) {
		goto char_device_read_output;
	}

	if (fdi->dump_en) {
		if (netfn_flow_cookie_dump_get(fdi)) {
			netfn_flow_cookie_warn("Failed to create netfn flow cookie db state message\n");
			return -EIO;
		}

		netfn_flow_cookie_info("Done with netfn flow dump read");

		/*
		 * We assign the DB array pointer as associated with
		 * Flow Dump Instance to NULL so as to avoid
		 * data flooding onto the flow dump file.
		 */
		fdi->dump_en = false;
		goto char_device_read_output;
	}

	return 0;

char_device_read_output:
	/*
	 * If supplied buffer is small we limit what we output
	 */
	bytes_read = fdi->msg_len;
	if (bytes_read > length) {
		bytes_read = length;
	}

	if (copy_to_user(buffer, fdi->msgp, bytes_read)) {
		return -EIO;
	}

	fdi->msg_len -= bytes_read;
	fdi->msgp += bytes_read;

	netfn_flow_cookie_trace("flow dump read, bytes_read %u bytes\n", bytes_read);

	/*
	 * Most read functions return the number of bytes put into the buffer
	 */
	return bytes_read;
}

/*
 * netfn_flow_cookie_dump_dev_write()
 * 	write file operation handler
 */
static ssize_t netfn_flow_cookie_dump_dev_write(struct file *filp, const char *buff, size_t len, loff_t *off)
{
	/*
	 * Not supported.
	 */

	return -EINVAL;
}

/*
 * File operations used in the char device
 *	NOTE: The char device is a simple file that allows us to dump our connection tracking state
 */
static struct file_operations netfn_flow_cookie_dump_fops = {
	.read = netfn_flow_cookie_dump_dev_read,
	.write = netfn_flow_cookie_dump_dev_write,
	.open = netfn_flow_cookie_dump_dev_open,
	.release = netfn_flow_cookie_dump_dev_release
};

/*
 * netfn_flow_cookie_dump_exit()
 * 	unregister character device for flow dump.
 */
void netfn_flow_cookie_dump_exit(void)
{
	unregister_chrdev(netfn_flow_cookie_dump_dev_major_id, "netfn_flow_dump");

	/*
	 * Remove the debugfs files recursively.
	 */
	if (netfn_flow_cookie_dump_dentry) {
		debugfs_remove_recursive(netfn_flow_cookie_dump_dentry);
	}

}

/*
 * netfn_flow_cookie_dump_init()
 * 	register a character device for flow dump.
 */
int netfn_flow_cookie_dump_init(void)
{
	int dev_id = -1;

	netfn_flow_cookie_dump_dentry = debugfs_create_dir("netfn_flow_cookie_flow_dump", NULL);
	if (!netfn_flow_cookie_dump_dentry) {
		netfn_flow_cookie_warn("Failed to create netfn flow cookie flow dump directory in debugfs\n");
		return -1;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0))
	if (!debugfs_create_u32("netfn_flow_dump", S_IRUGO, netfn_flow_cookie_dump_dentry,
				(u32 *)&netfn_flow_cookie_dump_dev_major_id)) {
		netfn_flow_cookie_warn("Failed to create netfn flow cookie flow dump dev major file in debugfs\n");
		goto init_cleanup;
	}
#else
	debugfs_create_u32("netfn_flow_dump", S_IRUGO, netfn_flow_cookie_dump_dentry,
				(u32 *)&netfn_flow_cookie_dump_dev_major_id);
#endif

	/*
	 * Register a char device that we will use to provide a dump of our state
	 */
	dev_id = register_chrdev(0, "netfn_flow_cookie_flow_dump", &netfn_flow_cookie_dump_fops);
	if (dev_id < 0) {
		netfn_flow_cookie_trace("Failed to register chrdev %u\n", dev_id);
		goto init_cleanup;
	}

	netfn_flow_cookie_dump_dev_major_id = dev_id;
	netfn_flow_cookie_trace("registered chr dev major id assigned %u\n", netfn_flow_cookie_dump_dev_major_id);
	return 0;

init_cleanup:
	debugfs_remove_recursive(netfn_flow_cookie_dump_dentry);
	return dev_id;
}
