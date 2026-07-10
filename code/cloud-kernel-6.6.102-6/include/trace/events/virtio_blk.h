/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM virtio_blk

#if !defined(_TRACE_VIRTIO_BLK_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_VIRTIO_BLK_H

#include <linux/tracepoint.h>

struct request;

TRACE_EVENT(virtblk_request_done,

	TP_PROTO(struct request *req, u8 ret),

	TP_ARGS(req, ret),

	TP_STRUCT__entry(
		__field(struct request *, req)
		__field(u32, tag)
		__field(int, qid)
		__field(void *, end_io_data)
		__field(u8, ret)
	),

	TP_fast_assign(
		__entry->req = req;
		__entry->tag = req->tag;
		__entry->qid = req->q->id;
		__entry->end_io_data = req->end_io_data;
		__entry->ret = ret;
	),

	TP_printk("DONE: req=%p qid=%d tag=%d ret=%d ioucmd=%p",
		  __entry->req, __entry->qid, __entry->tag,
		  __entry->ret, __entry->end_io_data)
);

TRACE_EVENT(virtblk_uring_cmd_io,

	TP_PROTO(struct request *req, u32 type, u64 sector),

	TP_ARGS(req, type, sector),

	TP_STRUCT__entry(
		__field(struct request *, req)
		__field(u32, tag)
		__field(u32, type)
		__field(u64, sector)
	),

	TP_fast_assign(
		__entry->req = req;
		__entry->tag = req->tag;
		__entry->type = type;
		__entry->sector = sector;
	),

	TP_printk("URING: req=%p tag=%d type=%d sector=%llu",
		  __entry->req, __entry->tag, __entry->type,
		  __entry->sector)
);

TRACE_EVENT(virtio_prep_rq,

	TP_PROTO(struct request *req, bool bid, int num),

	TP_ARGS(req, bid, num),

	TP_STRUCT__entry(
		__field(struct request *, req)
		__field(u32, tag)
		__field(bool, bid)
		__field(int, num)
	),

	TP_fast_assign(
		__entry->req = req;
		__entry->tag = req->tag;
		__entry->bid = bid;
		__entry->num = num;
	),

	TP_printk("QUEUE: req=%p tag=%d bid=%d sgs=%d",
		  __entry->req, __entry->tag, __entry->bid, __entry->num)
);

#endif /* _TRACE_VIRTIO_BLK_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
