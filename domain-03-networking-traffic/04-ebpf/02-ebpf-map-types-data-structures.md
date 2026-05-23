---
title: eBPF Map 类型与数据结构 (eBPF Map Types and Data Structures)
description: 1. [eBPF Map 概述与作用](#1-ebpf-map-概述与作用)
category: ebpf-technology
tags:
- k8s
- ebpf
- cilium
- networking
- observability
- daemonset
- rag
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 网络工程师
- 内核工程师
estimated_read_time: 5min
intent_queries:
- eBPF Map 类型与数据结构 (eBPF Map Types and Data Structures) 是什么
- 如何 eBPF Map 类型与数据结构 (eBPF Map Types and Data Structures)
- Kubernetes 35 ebpf technology 最佳实践
trigger_keywords:
- eBPF
- Map
- 类型与数据结构
- eBPF
- Map
- Types
- and
- Data
prerequisites:
- kubectl-basics
- networking-basics
- ebpf-basics
- cilium-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
created: "2026-05-23"
---

# eBPF Map 类型与数据结构 (eBPF Map Types and Data Structures)

> **适用范围**: eBPF 程序开发、内核/用户态通信 | **专家级别**: ⭐⭐⭐⭐⭐ | **更新时间**: 2026-03-03
> **内核要求**: Linux Kernel >= 4.3 (基础 Map) | >= 5.1 (Ring Buffer) | >= 5.8 (新特性)

---

<!-- chunk: 📋 目录 -->## 📋 目录

1. [eBPF Map 概述与作用](#1-ebpf-map-概述与作用)
2. [Hash Map 类型](#2-hash-map-类型)
3. [Array Map 类型](#3-array-map-类型)
4. [Ring Buffer 高性能事件传递](#4-ring-buffer-高性能事件传递)
5. [Perf Event Array](#5-perf-event-array)
6. [Stack 与 Queue Map](#6-stack-与-queue-map)
7. [LPM Trie Map](#7-lpm-trie-map)
8. [Map-in-Map 嵌套结构](#8-map-in-map-嵌套结构)
9. [用户空间与内核空间通信模式](#9-用户空间与内核空间通信模式)
10. [Map 性能优化与调优](#10-map-性能优化与调优)
11. [bpftool Map 操作实践](#11-bpftool-map-操作实践)

---

<!-- chunk: 1. eBPF Map 概述与作用 -->## 1. eBPF Map 概述与作用

#<!-- chunk: 1.1 什么是 eBPF Map (What is eBPF Map) -->## 1.1 什么是 eBPF Map (What is eBPF Map)

eBPF Map 是内核中的键值存储数据结构，是 eBPF 程序间以及 eBPF 程序与用户空间程序通信的核心机制。它提供了持久化存储、状态共享和数据传递等功能。

```
eBPF Map 核心作用
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

┌─────────────────────────────────────────────────────────────────────┐
│                     eBPF Map 使用场景                               │
│                                                                     │
│  1. 状态持久化                                                      │
│     eBPF 程序每次调用都是无状态的，Map 提供跨调用的持久状态        │
│     例: 连接跟踪表、统计计数器、IP 黑名单                          │
│                                                                     │
│  2. eBPF 程序间通信                                                 │
│     多个 eBPF 程序通过共享 Map 交换数据                            │
│     例: XDP 程序将包转发决策写入 Map，TC 程序读取                  │
│                                                                     │
│  3. 用户空间 ↔ 内核空间通信                                        │
│     用户程序读取 eBPF 统计数据                                      │
│     用户程序向 eBPF 程序下发配置                                   │
│                                                                     │
│  4. 尾调用表 (Tail Call)                                           │
│     BPF_MAP_TYPE_PROG_ARRAY: 存储程序引用，实现程序链               │
│                                                                     │
│  5. 事件输出                                                        │
│     Ring Buffer / Perf Event Array: 内核→用户态事件流              │
└─────────────────────────────────────────────────────────────────────┘

eBPF Map 访问接口:

  内核侧 (eBPF 程序):              用户侧 (用户空间程序):
  ┌──────────────────────────┐      ┌──────────────────────────────┐
  │ bpf_map_lookup_elem()    │      │ bpf_map_lookup_elem()        │
  │ bpf_map_update_elem()    │      │ bpf_map_update_elem()        │
  │ bpf_map_delete_elem()    │      │ bpf_map_delete_elem()        │
  │ bpf_map_push_elem()      │      │ bpf_map_get_next_key()       │
  │ bpf_map_pop_elem()       │      │ bpf_map_lookup_batch()       │
  │ bpf_for_each_map_elem()  │      │ bpf_map_update_batch()       │
  └──────────────────────────┘      └──────────────────────────────┘
               │                                 │
               └─────────────┬───────────────────┘
                             ▼
                    ┌──────────────────┐
                    │   eBPF Map       │
                    │  (内核内存)       │
                    └──────────────────┘
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

#<!-- chunk: 1.2 所有 Map 类型总览 (All Map Types Overview) -->## 1.2 所有 Map 类型总览 (All Map Types Overview)

```
eBPF Map 类型分类 (Linux 6.x)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

分类              Map 类型                              引入版本  主要用途
──────────────── ────────────────────────────────────── ──────── ────────────────────
Hash             BPF_MAP_TYPE_HASH                      4.3      通用键值存储
                 BPF_MAP_TYPE_PERCPU_HASH               4.6      无锁每CPU计数
                 BPF_MAP_TYPE_LRU_HASH                  4.10     有限容量缓存
                 BPF_MAP_TYPE_LRU_PERCPU_HASH           4.10     Per-CPU LRU
                 BPF_MAP_TYPE_HASH_OF_MAPS              4.12     Map嵌套

Array            BPF_MAP_TYPE_ARRAY                     3.19     固定数组
                 BPF_MAP_TYPE_PERCPU_ARRAY              4.6      Per-CPU数组
                 BPF_MAP_TYPE_PROG_ARRAY                4.2      尾调用程序表
                 BPF_MAP_TYPE_ARRAY_OF_MAPS             4.12     Map嵌套数组

事件             BPF_MAP_TYPE_PERF_EVENT_ARRAY          4.3      Perf事件输出
                 BPF_MAP_TYPE_RINGBUF                   5.8      高性能环形缓冲

网络             BPF_MAP_TYPE_SOCKMAP                   4.14     Socket重定向
                 BPF_MAP_TYPE_SOCKHASH                  4.18     Socket哈希
                 BPF_MAP_TYPE_DEVMAP                    4.14     设备重定向
                 BPF_MAP_TYPE_DEVMAP_HASH               5.4      哈希设备Map
                 BPF_MAP_TYPE_CPUMAP                    4.15     CPU重定向
                 BPF_MAP_TYPE_XSKMAP                    4.18     AF_XDP socket
                 BPF_MAP_TYPE_REUSEPORT_SOCKARRAY        4.19    端口复用

存储             BPF_MAP_TYPE_SK_STORAGE                5.2      Socket本地存储
                 BPF_MAP_TYPE_INODE_STORAGE             5.10     Inode本地存储
                 BPF_MAP_TYPE_TASK_STORAGE              5.11     Task本地存储
                 BPF_MAP_TYPE_CGROUP_STORAGE            4.19     Cgroup存储
                 BPF_MAP_TYPE_PERCPU_CGROUP_STORAGE      4.20    Per-CPU Cgroup存储

其他             BPF_MAP_TYPE_STACK_TRACE               4.6      调用栈跟踪
                 BPF_MAP_TYPE_CGROUP_ARRAY              4.8      Cgroup路径
                 BPF_MAP_TYPE_LPM_TRIE                  4.11     最长前缀匹配
                 BPF_MAP_TYPE_STACK                     4.20     LIFO队列
                 BPF_MAP_TYPE_QUEUE                     4.20     FIFO队列
                 BPF_MAP_TYPE_STRUCT_OPS                5.6      内核结构体操作
                 BPF_MAP_TYPE_BLOOM_FILTER              5.16     布隆过滤器
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

#<!-- chunk: 1.3 Map 创建与基本操作 (Map Creation) -->## 1.3 Map 创建与基本操作 (Map Creation)

```c
/* eBPF 程序中的 Map 定义方式 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* 方式1: BTF 风格 (推荐，现代方式) */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);          /* Map 类型 */
    __uint(max_entries, 10240);               /* 最大条目数 */
    __type(key, __u32);                       /* key 类型 (自动推断大小) */
    __type(value, __u64);                     /* value 类型 */
    __uint(map_flags, BPF_F_NO_PREALLOC);    /* 可选标志 */
} my_hash_map SEC(".maps");

/* 方式2: 传统方式 (旧代码兼容) */
struct bpf_map_def SEC("maps") my_old_map = {
    .type = BPF_MAP_TYPE_HASH,
    .key_size = sizeof(__u32),
    .value_size = sizeof(__u64),
    .max_entries = 10240,
};

/* 方式3: 用户空间创建 (libbpf) */
/* 用于动态创建，或需要在程序加载前配置的 Map */
```

```c
/* 用户空间 Map 操作 */
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

int main() {
    /* 创建 Hash Map */
    LIBBPF_OPTS(bpf_map_create_opts, opts,
        .map_flags = BPF_F_NO_PREALLOC,
    );
    
    int map_fd = bpf_map_create(
        BPF_MAP_TYPE_HASH,    /* type */
        "my_map",             /* name (可选) */
        sizeof(__u32),        /* key_size */
        sizeof(__u64),        /* value_size */
        10240,                /* max_entries */
        &opts
    );
    
    if (map_fd < 0) {
        perror("bpf_map_create");
        return 1;
    }
    
    /* CRUD 操作 */
    __u32 key = 42;
    __u64 value = 1000;
    
    /* 创建/更新 */
    bpf_map_update_elem(map_fd, &key, &value, BPF_ANY);
    /* BPF_ANY: 不存在则创建，存在则更新 */
    /* BPF_NOEXIST: 仅创建 (不存在时) */
    /* BPF_EXIST: 仅更新 (存在时) */
    
    /* 查询 */
    __u64 result;
    int ret = bpf_map_lookup_elem(map_fd, &key, &result);
    if (ret == 0)
        printf("key=%u, value=%llu\n", key, result);
    
    /* 删除 */
    bpf_map_delete_elem(map_fd, &key);
    
    /* 遍历所有 key */
    __u32 prev_key, next_key;
    memset(&prev_key, 0, sizeof(prev_key));
    while (bpf_map_get_next_key(map_fd, &prev_key, &next_key) == 0) {
        bpf_map_lookup_elem(map_fd, &next_key, &result);
        printf("key=%u, value=%llu\n", next_key, result);
        prev_key = next_key;
    }
    
    /* 批量操作 (5.6+) */
    __u32 keys[100];
    __u64 values[100];
    __u32 count = 100;
    void *in_batch = NULL, *out_batch;
    
    LIBBPF_OPTS(bpf_map_batch_opts, batch_opts,
        .elem_flags = 0,
        .flags = 0,
    );
    
    bpf_map_lookup_batch(map_fd, &in_batch, &out_batch,
                         keys, values, &count, &batch_opts);
    
    close(map_fd);
    return 0;
}
```

---

<!-- chunk: 2. Hash Map 类型 -->## 2. Hash Map 类型

#<!-- chunk: 2.1 BPF_MAP_TYPE_HASH 基础哈希表 -->## 2.1 BPF_MAP_TYPE_HASH 基础哈希表

```
Hash Map 内部结构
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

┌──────────────────────────────────────────────────────────────────┐
│                   BPF_MAP_TYPE_HASH                              │
│                                                                  │
│  实现: 内核 hashtab (kernel/bpf/hashtab.c)                      │
│  锁定: 桶级别的 spinlock (每个 hash bucket 一把锁)              │
│  内存: 预分配 (默认) 或按需分配 (BPF_F_NO_PREALLOC)            │
│                                                                  │
│  Hash 桶数组                                                     │
│  ┌─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┐            │
│  │  0  │  1  │  2  │  3  │  4  │  5  │  6  │  7  │ ...        │
│  └──┬──┴──┬──┴──┬──┴──┬──┴─────┴─────┴─────┴─────┘            │
│     │     │     │     │                                         │
│     ▼     ▼     ▼     ▼                                         │
│  ┌─────┐ NULL ┌─────┐ NULL                                      │
│  │k1:v1│     │k3:v3│                                            │
│  └──┬──┘     └─────┘                                            │
│     │                                                           │
│     ▼                                                           │
│  ┌─────┐                                                        │
│  │k2:v2│  (哈希冲突，链表)                                     │
│  └─────┘                                                        │
│                                                                  │
│  操作复杂度:                                                     │
│  • 查找: O(1) 平均，O(n) 最坏                                   │
│  • 插入: O(1) 平均                                              │
│  • 删除: O(1) 平均                                              │
│  • 遍历: O(capacity) (需要扫描所有桶)                          │
└──────────────────────────────────────────────────────────────────┘
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

```c
/* Hash Map 完整使用示例 - TCP 连接跟踪 */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

/* 连接四元组作为 key */
struct conn_tuple {
    __u32 src_ip;
    __u32 dst_ip;
    __u16 src_port;
    __u16 dst_port;
    __u8  proto;
    __u8  _pad[3];  /* 对齐填充 */
};

/* 连接统计信息作为 value */
struct conn_stats {
    __u64 bytes_rx;
    __u64 bytes_tx;
    __u64 pkts_rx;
    __u64 pkts_tx;
    __u64 last_seen_ns;
    __u8  state;  /* TCP 状态 */
};

/* 连接跟踪 Hash Map */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 65536);
    __type(key, struct conn_tuple);
    __type(value, struct conn_stats);
    __uint(map_flags, BPF_F_NO_PREALLOC);  /* 按需分配节省内存 */
} conn_track SEC(".maps");

static __always_inline void
update_conn_stats(struct conn_tuple *tuple, __u32 pkt_len, bool is_rx) {
    struct conn_stats *stats = bpf_map_lookup_elem(&conn_track, tuple);
    
    if (stats) {
        /* 更新已有连接 */
        if (is_rx) {
            __sync_fetch_and_add(&stats->bytes_rx, pkt_len);
            __sync_fetch_and_add(&stats->pkts_rx, 1);
        } else {
            __sync_fetch_and_add(&stats->bytes_tx, pkt_len);
            __sync_fetch_and_add(&stats->pkts_tx, 1);
        }
        stats->last_seen_ns = bpf_ktime_get_ns();
    } else {
        /* 新建连接 */
        struct conn_stats new_stats = {
            .last_seen_ns = bpf_ktime_get_ns(),
        };
        if (is_rx) {
            new_stats.bytes_rx = pkt_len;
            new_stats.pkts_rx = 1;
        } else {
            new_stats.bytes_tx = pkt_len;
            new_stats.pkts_tx = 1;
        }
        /* BPF_NOEXIST: 避免竞争条件 */
        bpf_map_update_elem(&conn_track, tuple, &new_stats, BPF_NOEXIST);
    }
}

SEC("xdp")
int xdp_conn_track(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    __u32 pkt_len = ctx->data_end - ctx->data;
    
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;
    
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;
    
    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return XDP_PASS;
    
    struct conn_tuple tuple = {
        .src_ip = ip->saddr,
        .dst_ip = ip->daddr,
        .proto = ip->protocol,
    };
    
    if (ip->protocol == IPPROTO_TCP) {
        __u32 ip_hdr_len = ip->ihl * 4;
        struct tcphdr *tcp = (void *)ip + ip_hdr_len;
        if ((void *)(tcp + 1) > data_end)
            return XDP_PASS;
        
        tuple.src_port = tcp->source;
        tuple.dst_port = tcp->dest;
    } else if (ip->protocol == IPPROTO_UDP) {
        /* 类似处理... */
    }
    
    update_conn_stats(&tuple, pkt_len, true);
    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
```

#<!-- chunk: 2.2 BPF_MAP_TYPE_PERCPU_HASH Per-CPU 哈希表 -->## 2.2 BPF_MAP_TYPE_PERCPU_HASH Per-CPU 哈希表

```c
/* Per-CPU Hash Map - 无锁高并发计数 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* Per-CPU Hash: 每个 CPU 核心独立维护一份数据 */
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_HASH);
    __uint(max_entries, 1024);
    __type(key, __u32);    /* 进程 PID */
    __type(value, __u64);  /* 系统调用次数 */
} syscall_counts SEC(".maps");

SEC("tracepoint/raw_syscalls/sys_enter")
int count_syscalls(struct trace_event_raw_sys_enter *ctx) {
    __u32 pid = bpf_get_current_pid_tgid() >> 32;
    
    /* Per-CPU Map 操作无需原子指令，直接访问当前 CPU 的副本 */
    __u64 *count = bpf_map_lookup_elem(&syscall_counts, &pid);
    if (count) {
        (*count)++;  /* 无原子操作，比 PERCPU_ARRAY 更快 */
    } else {
        __u64 init = 1;
        bpf_map_update_elem(&syscall_counts, &pid, &init, BPF_NOEXIST);
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

```c
/* 用户空间读取 Per-CPU Hash Map */
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

void read_percpu_hash(int map_fd) {
    int num_cpus = libbpf_num_possible_cpus();
    __u64 values[num_cpus];
    __u32 key, next_key;
    
    /* 遍历所有 key */
    memset(&key, 0, sizeof(key));
    while (bpf_map_get_next_key(map_fd, &key, &next_key) == 0) {
        /* 每次查找返回所有 CPU 的值数组 */
        if (bpf_map_lookup_elem(map_fd, &next_key, values) == 0) {
            __u64 total = 0;
            for (int i = 0; i < num_cpus; i++) {
                total += values[i];
            }
            printf("PID %u: %llu syscalls (across %d CPUs)\n",
                   next_key, total, num_cpus);
        }
        key = next_key;
    }
}
```

#<!-- chunk: 2.3 BPF_MAP_TYPE_LRU_HASH LRU 哈希表 -->## 2.3 BPF_MAP_TYPE_LRU_HASH LRU 哈希表

```c
/* LRU Hash Map - 自动淘汰最久未使用的条目 */
/* 适用于连接跟踪、缓存等场景 */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* 
 * LRU Hash 特点:
 * • 当 Map 满时，自动淘汰最久未访问的条目
 * • 每个 NUMA 节点维护独立的 LRU 列表
 * • 支持 BPF_F_NO_COMMON_LRU 标志: 每CPU独立LRU (更少竞争)
 */

struct flow_entry {
    __u64 bytes;
    __u64 packets;
    __u64 last_active;
    __u8  tcp_flags;
};

struct {
    __uint(type, BPF_MAP_TYPE_LRU_HASH);
    __uint(max_entries, 1000000);  /* 百万条连接 */
    __type(key, __u64);           /* 流哈希值 */
    __type(value, struct flow_entry);
    /* 注意: LRU Hash 不支持 BPF_F_NO_PREALLOC */
    /* 内存在创建时预分配: max_entries * (key_size + value_size + 头部) */
} flow_table SEC(".maps");

/* LRU 变体: 每CPU独立 LRU，减少锁竞争 */
struct {
    __uint(type, BPF_MAP_TYPE_LRU_PERCPU_HASH);
    __uint(max_entries, 100000);
    __type(key, __u32);
    __type(value, __u64);
} percpu_lru_map SEC(".maps");

SEC("xdp")
int track_flows(struct xdp_md *ctx) {
    /* 简化: 使用包的哈希作为流 ID */
    __u64 flow_id = ctx->rx_queue_index;  /* 实际应计算5元组哈希 */
    
    struct flow_entry *entry = bpf_map_lookup_elem(&flow_table, &flow_id);
    if (entry) {
        /* 更新已有流 - LRU 会自动更新访问时间 */
        __sync_fetch_and_add(&entry->packets, 1);
        entry->last_active = bpf_ktime_get_ns();
    } else {
        /* 新流 - 如果 Map 满了，LRU 会自动淘汰最旧条目 */
        struct flow_entry new_entry = {
            .packets = 1,
            .bytes = ctx->data_end - ctx->data,
            .last_active = bpf_ktime_get_ns(),
        };
        bpf_map_update_elem(&flow_table, &flow_id, &new_entry, BPF_ANY);
    }
    
    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
```

#<!-- chunk: 2.4 Hash Map 对比表 -->## 2.4 Hash Map 对比表

```
Hash Map 类型对比
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

特性                    HASH    PERCPU_HASH  LRU_HASH   LRU_PERCPU_HASH
────────────────────── ─────── ─────────── ─────────── ───────────────
并发安全                桶锁     无锁         桶锁         无锁
内存效率                中       低(N个副本)  高(预分配)   低(预分配*N)
自动淘汰                否       否           是           是
内存分配                按需/预  按需/预       仅预分配     仅预分配
最大条目实际内存        小       大(×CPU数)   固定         固定(×CPU数)
读写性能                中       最高         中           高
遍历支持                是       是           是           是
适用场景                通用     高频无锁计   连接跟踪     高并发缓存
                        键值存储  数、统计      缓存         流量统计

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

<!-- chunk: 3. Array Map 类型 -->## 3. Array Map 类型

#<!-- chunk: 3.1 BPF_MAP_TYPE_ARRAY 基础数组 -->## 3.1 BPF_MAP_TYPE_ARRAY 基础数组

```
Array Map 内部结构
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

┌──────────────────────────────────────────────────────────────────┐
│                   BPF_MAP_TYPE_ARRAY                             │
│                                                                  │
│  实现: 预分配的连续内存数组                                      │
│  Key: 必须是 __u32 类型 (索引)，范围 [0, max_entries-1]         │
│  特点:                                                           │
│  • 创建时预分配所有内存并初始化为 0                             │
│  • 不支持删除操作 (只能清零)                                    │
│  • 原子更新 (atomic)                                            │
│  • 比 Hash Map 快 ~3-5x (无哈希计算，直接索引)                 │
│                                                                  │
│  内存布局:                                                       │
│  index:  [0]      [1]      [2]      [3]    ...   [N-1]         │
│         ┌───────┬────────┬────────┬────────┬────┬────────┐     │
│         │value_0│value_1 │value_2 │value_3 │... │value_N │     │
│         └───────┴────────┴────────┴────────┴────┴────────┘     │
│          ↑ 连续内存，L1/L2 缓存友好                             │
│                                                                  │
│  操作复杂度:                                                     │
│  • 查找: O(1) (直接内存访问)                                    │
│  • 更新: O(1) (直接内存写入)                                    │
│  • 不支持删除 (通过赋0值实现)                                   │
│  • 遍历: O(n) (线性扫描)                                        │
└──────────────────────────────────────────────────────────────────┘
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

```c
/* Array Map 使用示例 - 协议统计 */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <bpf/bpf_helpers.h>

/* 按协议号统计 (0-255，IP协议号) */
struct proto_stats {
    __u64 packets;
    __u64 bytes;
};

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 256);         /* IP 协议号最多 256 个 */
    __type(key, __u32);              /* 协议号 (0-255) */
    __type(value, struct proto_stats);
} proto_counters SEC(".maps");

/* 全局统计 Array */
struct global_stats {
    __u64 total_packets;
    __u64 total_bytes;
    __u64 ipv4_packets;
    __u64 ipv6_packets;
    __u64 other_packets;
};

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);  /* 单个全局统计对象 */
    __type(key, __u32);
    __type(value, struct global_stats);
} global_counter SEC(".maps");

SEC("xdp")
int xdp_count_protos(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    __u32 pkt_len = ctx->data_end - ctx->data;
    
    /* 更新全局统计 */
    __u32 gkey = 0;
    struct global_stats *gs = bpf_map_lookup_elem(&global_counter, &gkey);
    if (gs) {
        __sync_fetch_and_add(&gs->total_packets, 1);
        __sync_fetch_and_add(&gs->total_bytes, pkt_len);
    }
    
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;
    
    __u16 proto = bpf_ntohs(eth->h_proto);
    
    if (proto == ETH_P_IP) {
        struct iphdr *ip = (void *)(eth + 1);
        if ((void *)(ip + 1) > data_end)
            return XDP_PASS;
        
        if (gs)
            __sync_fetch_and_add(&gs->ipv4_packets, 1);
        
        /* 按 IP 协议号统计 */
        __u32 ip_proto = ip->protocol;
        struct proto_stats *ps = bpf_map_lookup_elem(&proto_counters, &ip_proto);
        if (ps) {
            __sync_fetch_and_add(&ps->packets, 1);
            __sync_fetch_and_add(&ps->bytes, pkt_len);
        }
    }
    
    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
```

#<!-- chunk: 3.2 BPF_MAP_TYPE_PERCPU_ARRAY Per-CPU 数组 -->## 3.2 BPF_MAP_TYPE_PERCPU_ARRAY Per-CPU 数组

```c
/* Per-CPU Array - 最高性能计数器 */
/* 每个 CPU 维护独立副本，无任何锁或原子操作 */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/*
 * Per-CPU Array 是 eBPF 中最快的计数器实现:
 * • 完全无锁 (per-CPU独立副本)
 * • 无需原子操作 (每CPU仅单个核访问)
 * • CPU缓存友好 (数据局部性好)
 * • 适合高频更新场景
 */

/* XDP 操作统计 */
struct xdp_action_stats {
    __u64 aborted;
    __u64 drop;
    __u64 pass;
    __u64 tx;
    __u64 redirect;
};

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, struct xdp_action_stats);
} xdp_stats SEC(".maps");

/* 使用 Per-CPU Array 作为"堆"(临时大内存缓冲) */
struct large_scratch {
    char buf[4096];  /* 4KB 临时缓冲区 */
    __u64 timestamp;
    char comm[16];
};

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, struct large_scratch);
} scratch_mem SEC(".maps");

SEC("xdp")
int xdp_with_percpu_stats(struct xdp_md *ctx) {
    __u32 key = 0;
    struct xdp_action_stats *stats = bpf_map_lookup_elem(&xdp_stats, &key);
    
    /* 处理逻辑... */
    int action = XDP_PASS;
    
    if (stats) {
        /* 无锁无原子操作，直接++ */
        switch (action) {
        case XDP_ABORTED: stats->aborted++; break;
        case XDP_DROP:    stats->drop++;    break;
        case XDP_PASS:    stats->pass++;    break;
        case XDP_TX:      stats->tx++;      break;
        case XDP_REDIRECT:stats->redirect++;break;
        }
    }
    
    return action;
}

char LICENSE[] SEC("license") = "GPL";
```

```c
/* 用户空间汇总 Per-CPU Array */
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

struct xdp_action_stats {
    __u64 aborted;
    __u64 drop;
    __u64 pass;
    __u64 tx;
    __u64 redirect;
};

void print_xdp_stats(int map_fd) {
    int ncpus = libbpf_num_possible_cpus();
    struct xdp_action_stats percpu_stats[ncpus];
    struct xdp_action_stats total = {};
    __u32 key = 0;
    
    /* 读取所有 CPU 的数据 */
    if (bpf_map_lookup_elem(map_fd, &key, percpu_stats) != 0) {
        perror("bpf_map_lookup_elem");
        return;
    }
    
    /* 汇总 */
    for (int i = 0; i < ncpus; i++) {
        total.aborted  += percpu_stats[i].aborted;
        total.drop     += percpu_stats[i].drop;
        total.pass     += percpu_stats[i].pass;
        total.tx       += percpu_stats[i].tx;
        total.redirect += percpu_stats[i].redirect;
    }
    
    printf("XDP Stats (Total across %d CPUs):\n", ncpus);
    printf("  ABORTED:  %llu\n", total.aborted);
    printf("  DROP:     %llu\n", total.drop);
    printf("  PASS:     %llu\n", total.pass);
    printf("  TX:       %llu\n", total.tx);
    printf("  REDIRECT: %llu\n", total.redirect);
}
```

#<!-- chunk: 3.3 BPF_MAP_TYPE_PROG_ARRAY 程序数组 (尾调用) -->## 3.3 BPF_MAP_TYPE_PROG_ARRAY 程序数组 (尾调用)

```c
/* Prog Array - 实现 eBPF 程序链 (Tail Calls) */
/*
 * 尾调用 (Tail Call):
 * • 跳转到另一个 eBPF 程序，不返回
 * • 允许将复杂逻辑拆分为多个程序
 * • 最大嵌套深度: 33 层
 * • 类似汇编的 JMP 而非 CALL
 */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* 程序索引定义 */
#define PROG_IDX_TCP    0
#define PROG_IDX_UDP    1
#define PROG_IDX_ICMP   2
#define PROG_IDX_OTHER  3

/* 程序数组 Map */
struct {
    __uint(type, BPF_MAP_TYPE_PROG_ARRAY);
    __uint(max_entries, 8);
    __type(key, __u32);
    __type(value, __u32);  /* 程序 fd */
} prog_array SEC(".maps");

/* 分发器程序 */
SEC("xdp/dispatcher")
int xdp_dispatcher(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_DROP;
    
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;
    
    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return XDP_DROP;
    
    /* 根据协议类型跳转到对应的处理程序 */
    __u32 idx;
    switch (ip->protocol) {
    case IPPROTO_TCP:  idx = PROG_IDX_TCP;  break;
    case IPPROTO_UDP:  idx = PROG_IDX_UDP;  break;
    case IPPROTO_ICMP: idx = PROG_IDX_ICMP; break;
    default:           idx = PROG_IDX_OTHER; break;
    }
    
    /* 尾调用: 跳转到子程序，不返回 */
    bpf_tail_call(ctx, &prog_array, idx);
    
    /* 如果 tail call 失败 (程序未注册)，继续在这里执行 */
    return XDP_PASS;
}

/* TCP 处理程序 */
SEC("xdp/tcp_handler")
int xdp_tcp(struct xdp_md *ctx) {
    /* 处理 TCP 包 */
    bpf_printk("TCP packet received\n");
    return XDP_PASS;
}

/* UDP 处理程序 */
SEC("xdp/udp_handler")
int xdp_udp(struct xdp_md *ctx) {
    /* 处理 UDP 包 */
    bpf_printk("UDP packet received\n");
    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
```

```c
/* 用户空间注册尾调用程序 */
#include <bpf/libbpf.h>

void setup_tail_calls(struct bpf_object *obj) {
    struct bpf_map *prog_map = bpf_object__find_map_by_name(obj, "prog_array");
    int map_fd = bpf_map__fd(prog_map);
    
    /* 获取各子程序的 fd */
    struct bpf_program *tcp_prog = bpf_object__find_program_by_name(obj, "xdp_tcp");
    struct bpf_program *udp_prog = bpf_object__find_program_by_name(obj, "xdp_udp");
    
    int tcp_fd = bpf_program__fd(tcp_prog);
    int udp_fd = bpf_program__fd(udp_prog);
    
    /* 注册到程序数组 */
    __u32 key_tcp = 0, key_udp = 1;
    bpf_map_update_elem(map_fd, &key_tcp, &tcp_fd, BPF_ANY);
    bpf_map_update_elem(map_fd, &key_udp, &udp_fd, BPF_ANY);
    
    printf("Tail call programs registered\n");
}
```

---

<!-- chunk: 4. Ring Buffer 高性能事件传递 -->## 4. Ring Buffer 高性能事件传递

#<!-- chunk: 4.1 Ring Buffer 架构 (Ring Buffer Architecture) -->## 4.1 Ring Buffer 架构 (Ring Buffer Architecture)

```
Ring Buffer vs Perf Event Array 对比
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

BPF_MAP_TYPE_PERF_EVENT_ARRAY (旧方式):
┌─────────────────────────────────────────────────────────────────┐
│  Per-CPU 环形缓冲区                                             │
│                                                                 │
│  CPU0: [ev1][ev3][ev5][    ]  ← 独立缓冲区                    │
│  CPU1: [ev2][ev4][ev6][    ]  ← 独立缓冲区                    │
│  CPU2: [   ][   ][   ][    ]  ← 可能空闲                     │
│  CPU3: [ev7][   ][   ][    ]  ← 独立缓冲区                    │
│                                                                 │
│  问题:                                                          │
│  • 每CPU独立缓冲，总内存 = buffer_size × CPU数                 │
│  • 事件可能乱序 (不同CPU的事件到达顺序不确定)                 │
│  • 用户态需要轮询所有 CPU 的 fd                                │
│  • 数据需要先拷贝到 per-CPU 缓冲，再拷贝到用户态 (两次拷贝)  │
└─────────────────────────────────────────────────────────────────┘

BPF_MAP_TYPE_RINGBUF (5.8+，推荐):
┌─────────────────────────────────────────────────────────────────┐
│  共享环形缓冲区                                                 │
│                                                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │ [事件头][数据1] [事件头][数据2] [事件头][数据3] ...      │  │
│  │  ↑                                               ↑       │  │
│  │ consumer_pos (读指针)                  producer_pos(写)  │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                 │
│  优势:                                                          │
│  • 内存共享: 总内存固定 (不随CPU数增长)                        │
│  • 事件有序: 全局有序                                           │
│  • 零拷贝: 通过 mmap 直接读取，无需拷贝                        │
│  • 单个 epoll fd (而非每CPU一个fd)                             │
│  • 支持预留+提交模式，原子提交                                 │
└─────────────────────────────────────────────────────────────────┘

性能对比:
  场景                  Perf Event Array    Ring Buffer
  ──────────────────── ─────────────────── ──────────────
  内存使用 (100KB×32CPU) 3.2 GB             100 KB
  用户态 CPU 使用        高 (轮询32个fd)    低 (单fd epoll)
  事件顺序               无序               有序
  拷贝次数               2次                0次 (mmap)
  吞吐量 (事件/s)        ~1M                ~5M
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

#<!-- chunk: 4.2 Ring Buffer 完整使用示例 -->## 4.2 Ring Buffer 完整使用示例

```c
/* eBPF 程序侧 - 使用 Ring Buffer */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "vmlinux.h"

/* 安全事件结构 */
struct security_event {
    /* 事件元数据 */
    __u64 timestamp;
    __u32 pid;
    __u32 uid;
    __u32 gid;
    __u32 event_type;
    
    /* 进程信息 */
    char comm[16];
    __u32 ppid;
    
    /* 事件数据 (按类型) */
    union {
        /* 文件操作 */
        struct {
            char filename[256];
            int flags;
            int mode;
        } file;
        
        /* 网络连接 */
        struct {
            __u32 src_ip;
            __u32 dst_ip;
            __u16 src_port;
            __u16 dst_port;
            __u8  proto;
        } net;
        
        /* 进程执行 */
        struct {
            char exe[256];
            char args[512];
        } exec;
    };
};

/* 事件类型 */
#define EVENT_FILE_OPEN     1
#define EVENT_FILE_EXEC     2
#define EVENT_NET_CONNECT   3
#define EVENT_NET_LISTEN    4

/* Ring Buffer Map */
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 26);  /* 64 MB 环形缓冲 */
} security_events SEC(".maps");

/* 辅助函数: 填充通用事件头 */
static __always_inline void
fill_event_header(struct security_event *event, __u32 type) {
    event->timestamp = bpf_ktime_get_boot_ns();
    event->pid = bpf_get_current_pid_tgid() >> 32;
    event->uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    event->gid = bpf_get_current_uid_gid() >> 32;
    event->event_type = type;
    bpf_get_current_comm(event->comm, sizeof(event->comm));
}

/* 方式1: 预留 + 提交 (适合需要动态数据大小) */
SEC("lsm/file_open")
int BPF_PROG(lsm_track_file_open, struct file *file) {
    struct security_event *event;
    
    /* 预留空间 (不会立即发送) */
    event = bpf_ringbuf_reserve(&security_events, sizeof(*event), 0);
    if (!event)
        return 0;  /* 缓冲区满，丢弃 */
    
    /* 填充数据 */
    fill_event_header(event, EVENT_FILE_OPEN);
    
    /* 读取文件名 */
    struct dentry *dentry = BPF_CORE_READ(file, f_path.dentry);
    bpf_probe_read_kernel_str(event->file.filename, 
                               sizeof(event->file.filename),
                               BPF_CORE_READ(dentry, d_name.name));
    
    event->file.flags = BPF_CORE_READ(file, f_flags);
    
    /* 提交事件到用户空间 */
    bpf_ringbuf_submit(event, 0);
    /* 注意: 提交后不能再访问 event */
    
    return 0;
}

/* 方式2: 直接输出 (适合固定大小数据) */
SEC("kprobe/tcp_connect")
int BPF_KPROBE(kprobe_tcp_conn, struct sock *sk) {
    struct security_event event = {};
    
    fill_event_header(&event, EVENT_NET_CONNECT);
    
    event.net.src_ip = BPF_CORE_READ(sk, __sk_common.skc_rcv_saddr);
    event.net.dst_ip = BPF_CORE_READ(sk, __sk_common.skc_daddr);
    event.net.dst_port = BPF_CORE_READ(sk, __sk_common.skc_dport);
    event.net.proto = IPPROTO_TCP;
    
    /* 直接输出，内部预留+提交 */
    bpf_ringbuf_output(&security_events, &event, sizeof(event), 0);
    
    return 0;
}

/* 方式3: 丢弃预留 (条件不满足时) */
SEC("kprobe/vfs_write")
int BPF_KPROBE(kprobe_vfs_write, struct file *file, const char __user *buf,
               size_t count, loff_t *pos) {
    struct security_event *event;
    
    /* 仅跟踪写入量大于 1MB 的操作 */
    if (count < 1024 * 1024)
        return 0;
    
    event = bpf_ringbuf_reserve(&security_events, sizeof(*event), 0);
    if (!event)
        return 0;
    
    /* 检查是否需要继续 */
    __u32 uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    if (uid == 0) {
        /* root 用户，不监控 */
        bpf_ringbuf_discard(event, 0);  /* 丢弃，不发送 */
        return 0;
    }
    
    fill_event_header(event, EVENT_FILE_OPEN);
    bpf_ringbuf_submit(event, 0);
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

```c
/* 用户空间侧 - 消费 Ring Buffer 事件 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <bpf/libbpf.h>
#include "security_monitor.skel.h"

static volatile bool running = true;

/* 事件处理回调 */
static int handle_event(void *ctx, void *data, size_t data_sz) {
    struct security_event *event = data;
    
    /* 格式化时间戳 */
    __u64 ts_ns = event->timestamp;
    __u64 ts_ms = ts_ns / 1000000;
    
    printf("[%llu.%03llu] ", ts_ms / 1000, ts_ms % 1000);
    printf("PID=%-6d UID=%-5d COMM=%-16s ", 
           event->pid, event->uid, event->comm);
    
    switch (event->event_type) {
    case EVENT_FILE_OPEN:
        printf("FILE_OPEN flags=0x%x file=%s\n",
               event->file.flags, event->file.filename);
        break;
        
    case EVENT_NET_CONNECT: {
        char src[16], dst[16];
        uint32_t src_ip = ntohl(event->net.src_ip);
        uint32_t dst_ip = ntohl(event->net.dst_ip);
        snprintf(src, sizeof(src), "%d.%d.%d.%d",
                 (src_ip >> 24) & 0xFF, (src_ip >> 16) & 0xFF,
                 (src_ip >> 8) & 0xFF, src_ip & 0xFF);
        snprintf(dst, sizeof(dst), "%d.%d.%d.%d",
                 (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
                 (dst_ip >> 8) & 0xFF, dst_ip & 0xFF);
        printf("NET_CONNECT %s -> %s:%d\n",
               src, dst, ntohs(event->net.dst_port));
        break;
    }
    
    default:
        printf("UNKNOWN event_type=%d\n", event->event_type);
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    struct security_monitor_bpf *skel;
    struct ring_buffer *rb;
    int err;
    
    /* 加载 eBPF 程序 */
    skel = security_monitor_bpf__open_and_load();
    if (!skel) {
        fprintf(stderr, "Failed to load BPF\n");
        return 1;
    }
    
    /* 挂载 LSM/kprobe 程序 */
    err = security_monitor_bpf__attach(skel);
    if (err) {
        fprintf(stderr, "Failed to attach BPF\n");
        goto cleanup;
    }
    
    /* 创建 Ring Buffer 消费者 */
    rb = ring_buffer__new(
        bpf_map__fd(skel->maps.security_events),
        handle_event,   /* 回调函数 */
        NULL,           /* 回调上下文 */
        NULL            /* 选项 */
    );
    if (!rb) {
        fprintf(stderr, "Failed to create ring buffer\n");
        goto cleanup;
    }
    
    printf("Monitoring security events (Ctrl-C to stop)...\n");
    
    signal(SIGINT, [](int s){ running = false; });
    
    /* 事件循环 */
    while (running) {
        /* 等待并处理事件，超时 100ms */
        err = ring_buffer__poll(rb, 100);
        if (err == -EINTR)
            break;
        if (err < 0) {
            fprintf(stderr, "Ring buffer poll error: %d\n", err);
            break;
        }
        /* err == 0: 超时，无事件
           err > 0: 处理了 err 个事件 */
    }
    
    ring_buffer__free(rb);

cleanup:
    security_monitor_bpf__destroy(skel);
    return 0;
}
```

#<!-- chunk: 4.3 Ring Buffer 高级特性 -->## 4.3 Ring Buffer 高级特性

```c
/* Ring Buffer 高级特性 */

/* 特性1: BPF_RB_NO_WAKEUP / BPF_RB_FORCE_WAKEUP 标志 */
/* 控制何时唤醒用户空间 */
SEC("kprobe/batch_events")
int batch_events(struct pt_regs *ctx) {
    /* 批量提交时，最后一个事件才唤醒用户空间 */
    struct event *e1 = bpf_ringbuf_reserve(&rb, sizeof(*e1), 0);
    if (e1) {
        /* 填充数据... */
        bpf_ringbuf_submit(e1, BPF_RB_NO_WAKEUP);  /* 不唤醒 */
    }
    
    struct event *e2 = bpf_ringbuf_reserve(&rb, sizeof(*e2), 0);
    if (e2) {
        /* 填充数据... */
        bpf_ringbuf_submit(e2, BPF_RB_FORCE_WAKEUP);  /* 强制唤醒 */
    }
    
    return 0;
}

/* 特性2: 查询 Ring Buffer 剩余空间 */
SEC("kprobe/check_space")
int check_ringbuf_space(struct pt_regs *ctx) {
    /* 检查是否有足够空间 */
    __u64 avail = bpf_ringbuf_query(&rb, BPF_RB_AVAIL_DATA);
    __u64 ring_size = bpf_ringbuf_query(&rb, BPF_RB_RING_SIZE);
    __u64 cons_pos = bpf_ringbuf_query(&rb, BPF_RB_CONS_POS);
    __u64 prod_pos = bpf_ringbuf_query(&rb, BPF_RB_PROD_POS);
    
    /* 如果缓冲区超过80%满，减少事件发送 */
    if (avail > ring_size * 80 / 100)
        return 0;  /* 跳过此次事件 */
    
    /* 正常发送... */
    return 0;
}
```

---

<!-- chunk: 5. Perf Event Array -->## 5. Perf Event Array

#<!-- chunk: 5.1 Perf Event Array 使用 -->## 5.1 Perf Event Array 使用

```c
/* BPF_MAP_TYPE_PERF_EVENT_ARRAY 使用示例 */
/* 注意: 5.8+ 推荐使用 Ring Buffer 替代，但旧代码中常见 */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

/* DNS 查询事件 */
struct dns_event {
    __u64 timestamp;
    __u32 pid;
    __u32 uid;
    char  comm[16];
    __u8  query[128];
    __u16 query_type;
    __u32 src_ip;
};

/* Perf Event Array */
struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(__u32));
    __uint(value_size, sizeof(__u32));
} dns_events SEC(".maps");

SEC("tracepoint/net/net_dev_xmit")
int trace_dns_query(struct trace_event_raw_net_dev_xmit *ctx) {
    struct dns_event event = {};
    
    event.timestamp = bpf_ktime_get_ns();
    event.pid = bpf_get_current_pid_tgid() >> 32;
    event.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(event.comm, sizeof(event.comm));
    
    /* 发送到当前 CPU 的 perf 缓冲区 */
    bpf_perf_event_output(ctx, &dns_events, 
                           BPF_F_CURRENT_CPU,  /* 写入当前CPU的缓冲 */
                           &event, sizeof(event));
    return 0;
}

/* 发送动态大小数据 */
SEC("kprobe/sys_read")  
int trace_read(struct pt_regs *ctx) {
    /* 使用 BPF_F_CTXLEN_MASK 发送包含原始上下文数据 */
    struct {
        __u64 pid;
        char data[64];
    } sample = {};
    
    sample.pid = bpf_get_current_pid_tgid() >> 32;
    
    bpf_perf_event_output(ctx, &dns_events,
                           BPF_F_CURRENT_CPU,
                           &sample, sizeof(sample));
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

```c
/* 用户空间侧 - 读取 Perf Event Array */
#include <bpf/libbpf.h>
#include <sys/epoll.h>

struct perf_buffer *pb;

static void handle_dns_event(void *ctx, int cpu, void *data, __u32 size) {
    struct dns_event *event = data;
    printf("DNS query from PID %d: %s\n", event->pid, event->query);
}

static void handle_lost_events(void *ctx, int cpu, __u64 lost_cnt) {
    printf("Lost %llu events on CPU %d!\n", lost_cnt, cpu);
}

int main() {
    /* 创建 Perf Buffer 消费者 */
    LIBBPF_OPTS(perf_buffer_opts, pb_opts,
        .sample_cb = handle_dns_event,
        .lost_cb = handle_lost_events,
    );
    
    pb = perf_buffer__new(
        bpf_map__fd(skel->maps.dns_events),
        64,        /* 每CPU页面数 (64 × 4096 = 256KB per CPU) */
        &pb_opts
    );
    
    /* 事件循环 */
    while (running) {
        perf_buffer__poll(pb, 100);  /* 100ms 超时 */
    }
    
    perf_buffer__free(pb);
    return 0;
}
```

---

<!-- chunk: 6. Stack 与 Queue Map -->## 6. Stack 与 Queue Map

#<!-- chunk: 6.1 BPF_MAP_TYPE_STACK (LIFO) -->## 6.1 BPF_MAP_TYPE_STACK (LIFO)

```c
/* Stack Map - 后进先出 (LIFO) */
/* 用途: 工作队列、任务栈、事件处理等 */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* IP 地址栈 - 用于网络跳转跟踪 */
struct {
    __uint(type, BPF_MAP_TYPE_STACK);
    __uint(max_entries, 1024);
    __uint(value_size, sizeof(__u32));  /* Stack: 无 key */
    /* 注意: Stack/Queue 不使用 key，只有 value */
} ip_stack SEC(".maps");

SEC("xdp")
int xdp_track_hops(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;
    
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;
    
    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return XDP_PASS;
    
    __u32 src_ip = ip->saddr;
    
    /* Push IP 地址到栈 */
    bpf_map_push_elem(&ip_stack, &src_ip, BPF_EXIST);
    /* BPF_EXIST: 如果栈满，覆盖最旧的元素 */
    /* BPF_NOEXIST: 如果栈满，失败返回 */
    
    return XDP_PASS;
}

/* 从栈中读取 (在另一个程序中) */
SEC("kprobe/process_packets")
int process_ips(struct pt_regs *ctx) {
    __u32 ip;
    
    /* Pop: 后进先出 */
    int ret = bpf_map_pop_elem(&ip_stack, &ip);
    if (ret == 0) {
        bpf_printk("Processing IP: %x\n", bpf_ntohl(ip));
    }
    
    /* Peek: 查看顶部但不删除 */
    ret = bpf_map_peek_elem(&ip_stack, &ip);
    if (ret == 0) {
        bpf_printk("Top of stack IP: %x\n", bpf_ntohl(ip));
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

#<!-- chunk: 6.2 BPF_MAP_TYPE_QUEUE (FIFO) -->## 6.2 BPF_MAP_TYPE_QUEUE (FIFO)

```c
/* Queue Map - 先进先出 (FIFO) */
/* 用途: 任务队列、消息传递、流量整形等 */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* 待处理的数据包信息队列 */
struct pkt_info {
    __u32 src_ip;
    __u32 dst_ip;
    __u16 src_port;
    __u16 dst_port;
    __u32 pkt_len;
    __u64 timestamp;
};

struct {
    __uint(type, BPF_MAP_TYPE_QUEUE);
    __uint(max_entries, 4096);
    __uint(value_size, sizeof(struct pkt_info));
} pkt_queue SEC(".maps");

/* 生产者: 将包信息加入队列 */
SEC("xdp")
int xdp_enqueue(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;
    
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;
    
    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return XDP_PASS;
    
    struct pkt_info info = {
        .src_ip = ip->saddr,
        .dst_ip = ip->daddr,
        .pkt_len = ctx->data_end - ctx->data,
        .timestamp = bpf_ktime_get_ns(),
    };
    
    /* 入队 (如果满则失败) */
    bpf_map_push_elem(&pkt_queue, &info, 0);
    
    return XDP_PASS;
}

/* 消费者: 从队列取出包信息处理 */
SEC("kprobe/process_pkt_queue")
int process_queue(struct pt_regs *ctx) {
    struct pkt_info info;
    
    /* 出队: 先进先出 */
    while (bpf_map_pop_elem(&pkt_queue, &info) == 0) {
        bpf_printk("Processing pkt: %x -> %x (%u bytes)\n",
                   bpf_ntohl(info.src_ip),
                   bpf_ntohl(info.dst_ip),
                   info.pkt_len);
        /* 最多处理 10 个，避免循环检测问题 */
        /* 实际应使用 bpf_loop() */
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

#<!-- chunk: 6.3 BPF_MAP_TYPE_STACK_TRACE 调用栈跟踪 -->## 6.3 BPF_MAP_TYPE_STACK_TRACE 调用栈跟踪

```c
/* Stack Trace Map - 存储内核/用户态调用栈 */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

#define MAX_STACK_DEPTH 20

/* 调用栈存储 Map */
struct {
    __uint(type, BPF_MAP_TYPE_STACK_TRACE);
    __uint(max_entries, 10000);
    __uint(key_size, sizeof(__u32));        /* stack_id */
    __uint(value_size, MAX_STACK_DEPTH * sizeof(__u64));  /* 栈帧地址数组 */
} stack_traces SEC(".maps");

/* CPU 性能分析 */
struct profile_key {
    __u32 pid;
    __s32 kernel_stack_id;  /* 内核栈 ID */
    __s32 user_stack_id;    /* 用户栈 ID */
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 100000);
    __type(key, struct profile_key);
    __type(value, __u64);  /* 采样次数 */
} profile_counts SEC(".maps");

/* perf_event 程序 - CPU 采样分析 */
SEC("perf_event")
int profile_cpu(struct bpf_perf_event_data *ctx) {
    __u64 pid_tgid = bpf_get_current_pid_tgid();
    __u32 pid = pid_tgid >> 32;
    
    /* 获取内核态调用栈 */
    __s32 kernel_stack_id = bpf_get_stackid(ctx, &stack_traces, 
                                              0 /* flags */);
    
    /* 获取用户态调用栈 */
    __s32 user_stack_id = bpf_get_stackid(ctx, &stack_traces,
                                           BPF_F_USER_STACK);
    
    struct profile_key key = {
        .pid = pid,
        .kernel_stack_id = kernel_stack_id,
        .user_stack_id = user_stack_id,
    };
    
    /* 增加采样计数 */
    __u64 *count = bpf_map_lookup_elem(&profile_counts, &key);
    if (count) {
        (*count)++;
    } else {
        __u64 init = 1;
        bpf_map_update_elem(&profile_counts, &key, &init, BPF_NOEXIST);
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

```bash
# 用户空间解析调用栈 (使用 bpftrace)
bpftrace -e '
profile:hz:99 {
    @[kstack, ustack, comm] = count();
}
interval:s:10 {
    print(@);
    clear(@);
}'

# 或使用 bcc 的 profile 工具
/usr/share/bcc/tools/profile -F 99 30
```

---

<!-- chunk: 7. LPM Trie Map -->## 7. LPM Trie Map

#<!-- chunk: 7.1 LPM Trie 原理 (LPM Trie Principles) -->## 7.1 LPM Trie 原理 (LPM Trie Principles)

```
LPM Trie (Longest Prefix Match Trie) 结构
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

用途: IP 路由查找、ACL 策略匹配

示例 IP 前缀表:
  10.0.0.0/8     → "Internal Network"
  10.1.0.0/16    → "Dev VLAN"  
  10.1.1.0/24    → "Dev Team A"
  10.1.1.1/32    → "Dev Server 1"
  192.168.0.0/16 → "Lab Network"

查找 10.1.1.1:
  匹配 10.0.0.0/8     (8位匹配)
  匹配 10.1.0.0/16    (16位匹配)
  匹配 10.1.1.0/24    (24位匹配)  
  匹配 10.1.1.1/32    (32位匹配) ← 最长前缀，选择此规则

Trie 树结构:
              root
              /  \
           10.* 192.*
            |      \
         10.1.*   192.168.*
            |
         10.1.1.*
            |
         10.1.1.1

特性:
• 时间复杂度: O(prefix_length) 查找
• 支持 IPv4 (32位) 和 IPv6 (128位)
• 最大 prefix 深度 = key_size * 8 位
• 仅支持 BPF_F_NO_PREALLOC (按需分配)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

```c
/* LPM Trie Map 完整使用示例 - IP 访问控制 */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

/* LPM Trie key 结构: prefixlen + 数据 */
struct ipv4_lpm_key {
    __u32 prefixlen;  /* 前缀长度 (1-32) */
    __u32 ip;         /* IP 地址 (网络字节序) */
};

struct ipv6_lpm_key {
    __u32 prefixlen;   /* 前缀长度 (1-128) */
    __u8  ip[16];      /* IPv6 地址 */
};

/* ACL 规则 */
struct acl_rule {
    __u32 action;       /* 0=DENY, 1=ALLOW */
    __u32 priority;     /* 规则优先级 */
    char  comment[64];  /* 规则说明 */
};

/* IPv4 LPM Trie */
struct {
    __uint(type, BPF_MAP_TYPE_LPM_TRIE);
    __uint(max_entries, 65536);
    __type(key, struct ipv4_lpm_key);    /* key_size = 8 字节 */
    __type(value, struct acl_rule);
    __uint(map_flags, BPF_F_NO_PREALLOC);  /* LPM Trie 必须设置此标志 */
} acl_v4 SEC(".maps");

/* IPv6 LPM Trie */
struct {
    __uint(type, BPF_MAP_TYPE_LPM_TRIE);
    __uint(max_entries, 65536);
    __type(key, struct ipv6_lpm_key);    /* key_size = 20 字节 */
    __type(value, struct acl_rule);
    __uint(map_flags, BPF_F_NO_PREALLOC);
} acl_v6 SEC(".maps");

/* 统计被拒绝的包 */
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, __u64);
} denied_count SEC(".maps");

SEC("xdp")
int xdp_acl(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;
    
    __u16 proto = bpf_ntohs(eth->h_proto);
    
    if (proto == ETH_P_IP) {
        struct iphdr *ip = (void *)(eth + 1);
        if ((void *)(ip + 1) > data_end)
            return XDP_PASS;
        
        /* LPM 查找 - 自动匹配最长前缀 */
        struct ipv4_lpm_key key = {
            .prefixlen = 32,     /* 精确匹配，Trie 会自动找最长匹配 */
            .ip = ip->saddr,
        };
        
        struct acl_rule *rule = bpf_map_lookup_elem(&acl_v4, &key);
        if (rule && rule->action == 0) {
            /* 规则命中且动作为 DENY */
            __u32 cnt_key = 0;
            __u64 *count = bpf_map_lookup_elem(&denied_count, &cnt_key);
            if (count)
                (*count)++;
            return XDP_DROP;
        }
    }
    
    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
```

```c
/* 用户空间管理 LPM Trie */
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <arpa/inet.h>

struct ipv4_lpm_key {
    uint32_t prefixlen;
    uint32_t ip;
};

struct acl_rule {
    uint32_t action;
    uint32_t priority;
    char     comment[64];
};

/* 添加 CIDR 规则 */
int add_acl_rule(int map_fd, const char *cidr, int action, const char *comment) {
    char ip_str[64];
    int prefix_len;
    
    /* 解析 CIDR (如 "10.0.0.0/8") */
    sscanf(cidr, "%[^/]/%d", ip_str, &prefix_len);
    
    struct ipv4_lpm_key key = {
        .prefixlen = prefix_len,
    };
    inet_pton(AF_INET, ip_str, &key.ip);
    
    struct acl_rule rule = {
        .action = action,
        .priority = 100,
    };
    strncpy(rule.comment, comment, sizeof(rule.comment) - 1);
    
    return bpf_map_update_elem(map_fd, &key, &rule, BPF_ANY);
}

/* 删除规则 */
int del_acl_rule(int map_fd, const char *cidr) {
    char ip_str[64];
    int prefix_len;
    sscanf(cidr, "%[^/]/%d", ip_str, &prefix_len);
    
    struct ipv4_lpm_key key = {.prefixlen = prefix_len};
    inet_pton(AF_INET, ip_str, &key.ip);
    
    return bpf_map_delete_elem(map_fd, &key);
}

int main() {
    /* 假设已加载程序并获取 map_fd */
    int map_fd = /* ... */;
    
    /* 添加规则 */
    add_acl_rule(map_fd, "10.0.0.0/8", 1, "Allow internal");
    add_acl_rule(map_fd, "192.168.100.0/24", 0, "Block specific subnet");
    add_acl_rule(map_fd, "10.1.1.1/32", 0, "Block specific host");
    
    /* 查询规则 */
    struct ipv4_lpm_key query = {
        .prefixlen = 32,
        .ip = inet_addr("10.1.1.1"),
    };
    struct acl_rule result;
    if (bpf_map_lookup_elem(map_fd, &query, &result) == 0) {
        printf("Rule for 10.1.1.1: action=%d, comment=%s\n",
               result.action, result.comment);
    }
    
    return 0;
}
```

---

<!-- chunk: 8. Map-in-Map 嵌套结构 -->## 8. Map-in-Map 嵌套结构

#<!-- chunk: 8.1 Map-in-Map 原理与类型 -->## 8.1 Map-in-Map 原理与类型

```
Map-in-Map 嵌套结构
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

类型:
  BPF_MAP_TYPE_ARRAY_OF_MAPS  - 外层是数组，内层是任意 Map
  BPF_MAP_TYPE_HASH_OF_MAPS   - 外层是哈希，内层是任意 Map

使用场景:
┌─────────────────────────────────────────────────────────────────┐
│ 场景1: 按命名空间/租户隔离数据                                  │
│                                                                 │
│  外层 Map (Hash):        内层 Map (Hash):                      │
│  namespace_id → map_fd   conn_tuple → stats                    │
│                                                                 │
│  namespace_1 → [map_fd1] → {conn1: stats1, conn2: stats2}     │
│  namespace_2 → [map_fd2] → {conn1: stats3, conn3: stats4}     │
│                                                                 │
│  优势: 每个命名空间独立 Map，操作隔离，更新一个不影响其他     │
│                                                                 │
│ 场景2: 原子替换策略表                                          │
│                                                                 │
│  外层 Array (index 0):   内层 Map (当前策略):                  │
│  [0] → policy_map_fd    rule1 → action, rule2 → action        │
│                                                                 │
│  更新流程:                                                      │
│  1. 创建新的内层 Map (new_policy_map)                          │
│  2. 用户空间加载新规则到 new_policy_map                        │
│  3. 原子更新外层 Array: array[0] = new_policy_map_fd          │
│  4. eBPF 程序下次访问时自动使用新策略                          │
│  → 实现零停机策略热更新!                                       │
└─────────────────────────────────────────────────────────────────┘
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

```c
/* Map-in-Map 示例 - 多租户连接跟踪 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* 内层 Map 原型 (定义结构模板) */
struct inner_map_type {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, __u64);   /* 连接哈希 */
    __type(value, __u64); /* 字节数 */
} inner_map_proto SEC(".maps");

/* 外层 Map - 按租户 ID 索引，value 是内层 Map 的 fd */
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 256);          /* 最多 256 个租户 */
    __type(key, __u32);               /* 租户 ID */
    __type(value, __u32);             /* 内层 Map fd (由内核管理) */
    /* 引用内层 Map 类型 */
    __array(values, struct inner_map_type);  /* BTF 风格 */
} tenant_maps SEC(".maps");

SEC("xdp")
int multi_tenant_tracking(struct xdp_md *ctx) {
    __u32 tenant_id = 0;  /* 实际应从包头提取 (如 VXLAN VNI) */
    __u64 conn_hash = 0;  /* 实际应计算5元组哈希 */
    __u32 pkt_len = ctx->data_end - ctx->data;
    
    /* 查找租户对应的内层 Map */
    void *inner_map = bpf_map_lookup_elem(&tenant_maps, &tenant_id);
    if (!inner_map)
        return XDP_PASS;  /* 未知租户 */
    
    /* 在内层 Map 中更新连接统计 */
    __u64 *bytes = bpf_map_lookup_elem(inner_map, &conn_hash);
    if (bytes) {
        __sync_fetch_and_add(bytes, pkt_len);
    } else {
        __u64 init = pkt_len;
        bpf_map_update_elem(inner_map, &conn_hash, &init, BPF_NOEXIST);
    }
    
    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
```

```c
/* 策略热更新 - 使用 Map-in-Map 实现零停机更新 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* 内层 Map: 策略规则表 */
struct policy_rule {
    __u32 action;        /* 0=deny, 1=allow */
    __u32 rate_limit;    /* 每秒允许的包数 */
    __u64 hit_count;     /* 命中次数 */
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10000);
    __type(key, __u32);                 /* 源 IP */
    __type(value, struct policy_rule);
} policy_inner_proto SEC(".maps");

/* 外层 Map: 当前活跃策略表 (单元素数组) */
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY_OF_MAPS);
    __uint(max_entries, 1);
    __type(key, __u32);
    __uint(value_size, sizeof(__u32));
    __array(values, struct {
        __uint(type, BPF_MAP_TYPE_HASH);
        __uint(max_entries, 10000);
        __type(key, __u32);
        __type(value, struct policy_rule);
    });
} active_policy SEC(".maps");

SEC("xdp")
int policy_enforcer(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    
    /* 获取当前活跃策略 Map (原子读取) */
    __u32 idx = 0;
    void *policy_map = bpf_map_lookup_elem(&active_policy, &idx);
    if (!policy_map)
        return XDP_PASS;  /* 无策略，全部允许 */
    
    /* 提取源 IP */
    if ((void *)(data + sizeof(struct ethhdr) + sizeof(struct iphdr)) > data_end)
        return XDP_PASS;
    
    struct iphdr *ip = data + sizeof(struct ethhdr);
    __u32 src_ip = ip->saddr;
    
    /* 查询策略 */
    struct policy_rule *rule = bpf_map_lookup_elem(policy_map, &src_ip);
    if (rule) {
        __sync_fetch_and_add(&rule->hit_count, 1);
        if (rule->action == 0)
            return XDP_DROP;
    }
    
    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
```

```c
/* 用户空间热更新流程 */
#include <bpf/libbpf.h>
#include <bpf/bpf.h>

int hot_update_policy(int outer_map_fd, struct policy_entry *new_rules, int count) {
    /* 1. 创建新的内层 Map */
    LIBBPF_OPTS(bpf_map_create_opts, opts);
    int new_inner_fd = bpf_map_create(
        BPF_MAP_TYPE_HASH,
        "policy_new",
        sizeof(__u32),                /* key_size */
        sizeof(struct policy_rule),   /* value_size */
        10000,                        /* max_entries */
        &opts
    );
    
    if (new_inner_fd < 0) {
        perror("bpf_map_create");
        return -1;
    }
    
    /* 2. 加载新规则到新 Map */
    for (int i = 0; i < count; i++) {
        struct policy_rule rule = {
            .action = new_rules[i].action,
            .rate_limit = new_rules[i].rate_limit,
        };
        bpf_map_update_elem(new_inner_fd, &new_rules[i].src_ip, &rule, BPF_ANY);
    }
    
    /* 3. 原子更新外层 Map - eBPF 程序下次执行时自动使用新 Map */
    __u32 idx = 0;
    int ret = bpf_map_update_elem(outer_map_fd, &idx, &new_inner_fd, BPF_ANY);
    
    if (ret < 0) {
        perror("bpf_map_update_elem (outer)");
        close(new_inner_fd);
        return -1;
    }
    
    printf("Policy updated atomically with %d rules\n", count);
    
    /* 4. 关闭新 Map fd (内核仍保持引用) */
    close(new_inner_fd);
    
    return 0;
}
```

---

<!-- chunk: 9. 用户空间与内核空间通信模式 -->## 9. 用户空间与内核空间通信模式

#<!-- chunk: 9.1 通信模式总览 -->## 9.1 通信模式总览

```
eBPF 内核↔用户空间通信模式
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

模式                  数据流向      特点                    适用场景
────────────────────  ───────────── ──────────────────────  ─────────────────────
Array/Hash Map        双向          轮询读取，适合聚合数据  统计、配置下发
Ring Buffer           内→用户       零拷贝，有序，高效      实时事件流
Perf Event Array      内→用户       Per-CPU，高频事件       高频采样 (旧方式)
Socket Map            重定向        透明代理，无拷贝        服务网格、代理
BPF Task/Sk Storage   双向          绑定到内核对象          per-socket状态
BPF Iterator          内→用户       遍历内核对象            批量数据导出

                       ┌─────────────────────────────────────┐
                       │           选择指南                   │
                       │                                     │
                       │  高频实时事件 (>100K/s)?            │
                       │  ┌─ 是 ─▶ Ring Buffer (5.8+)        │
                       │  └─ 否                              │
                       │      │                              │
                       │      ▼                              │
                       │  需要聚合统计?                       │
                       │  ┌─ 是 ─▶ Per-CPU Array/Hash        │
                       │  └─ 否                              │
                       │      │                              │
                       │      ▼                              │
                       │  需要配置下发?                       │
                       │  ┌─ 是 ─▶ Hash/Array Map            │
                       │  └─ 否 ─▶ Ring Buffer (通用)        │
                       └─────────────────────────────────────┘
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

#<!-- chunk: 9.2 BPF Iterator - 高效批量数据导出 -->## 9.2 BPF Iterator - 高效批量数据导出

```c
/* BPF Iterator - 遍历内核对象并导出数据 (5.8+) */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

/* 通过 seq_file 接口导出 Map 中的所有数据 */
/* 挂载到 /sys/fs/bpf/ 后可通过 cat 命令读取 */

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, __u32);
    __type(value, __u64);
} my_data SEC(".maps");

/* BPF Iterator 程序 - 遍历 Map 条目 */
SEC("iter/bpf_map_elem")
int dump_map_elem(struct bpf_iter__bpf_map_elem *ctx) {
    struct seq_file *seq = ctx->meta->seq;
    __u32 *key = ctx->key;
    __u64 *value = ctx->value;
    
    if (!key || !value)
        return 0;
    
    /* 格式化输出到 seq_file */
    BPF_SEQ_PRINTF(seq, "key=%-10u value=%-20llu\n", *key, *value);
    
    return 0;
}

/* 遍历所有 socket */
SEC("iter/tcp4")
int dump_tcp_sockets(struct bpf_iter__tcp *ctx) {
    struct seq_file *seq = ctx->meta->seq;
    struct sock_common *sk_common = ctx->sk_common;
    
    if (!sk_common)
        return 0;
    
    __u32 src_ip = BPF_CORE_READ(sk_common, skc_rcv_saddr);
    __u32 dst_ip = BPF_CORE_READ(sk_common, skc_daddr);
    __u16 src_port = BPF_CORE_READ(sk_common, skc_num);
    __u16 dst_port = BPF_CORE_READ(sk_common, skc_dport);
    
    BPF_SEQ_PRINTF(seq, "%x:%u -> %x:%u\n",
                   src_ip, src_port, dst_ip, bpf_ntohs(dst_port));
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

#<!-- chunk: 9.3 BPF Local Storage - 绑定对象的存储 -->## 9.3 BPF Local Storage - 绑定对象的存储

```c
/* BPF Local Storage - 无需哈希查找的对象绑定存储 */
/* 将数据直接绑定到内核对象 (socket/inode/task) */

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "vmlinux.h"

/* Socket 本地存储 */
struct socket_stats {
    __u64 bytes_sent;
    __u64 bytes_recv;
    __u64 conn_start_ns;
    __u32 pid;
    char  comm[16];
};

/* SK Storage Map 定义 (注意: 不需要 max_entries) */
struct {
    __uint(type, BPF_MAP_TYPE_SK_STORAGE);
    __uint(map_flags, BPF_F_NO_PREALLOC);
    __type(key, int);                      /* sock fd (占位符) */
    __type(value, struct socket_stats);
} sk_stats_map SEC(".maps");

/* Task 本地存储 */
struct task_ctx {
    __u64 syscall_count;
    __u64 last_syscall_ns;
    __u32 suspicious_count;
};

struct {
    __uint(type, BPF_MAP_TYPE_TASK_STORAGE);
    __uint(map_flags, BPF_F_NO_PREALLOC);
    __type(key, int);
    __type(value, struct task_ctx);
} task_ctx_map SEC(".maps");

/* TCP 连接建立时初始化 socket 存储 */
SEC("fentry/tcp_v4_connect")
int BPF_PROG(track_new_conn, struct sock *sk) {
    struct socket_stats *stats;
    
    /* bpf_sk_storage_get: 获取或创建 socket 绑定的存储 */
    stats = bpf_sk_storage_get(&sk_stats_map, sk, NULL, BPF_LOCAL_STORAGE_GET_F_CREATE);
    if (!stats)
        return 0;
    
    stats->conn_start_ns = bpf_ktime_get_ns();
    stats->pid = bpf_get_current_pid_tgid() >> 32;
    bpf_get_current_comm(stats->comm, sizeof(stats->comm));
    
    return 0;
}

/* 跟踪数据发送 */
SEC("fentry/tcp_sendmsg")
int BPF_PROG(track_sendmsg, struct sock *sk, struct msghdr *msg, size_t size) {
    /* 查找此 socket 的统计数据 */
    struct socket_stats *stats = bpf_sk_storage_get(&sk_stats_map, sk, NULL, 0);
    if (!stats)
        return 0;
    
    __sync_fetch_and_add(&stats->bytes_sent, size);
    return 0;
}

/* Socket 关闭时清理 (自动发生，也可手动) */
SEC("fentry/inet_sock_destruct")
int BPF_PROG(cleanup_sk_storage, struct sock *sk) {
    /* 读取最终统计 */
    struct socket_stats *stats = bpf_sk_storage_get(&sk_stats_map, sk, NULL, 0);
    if (stats) {
        bpf_printk("Connection closed: pid=%d sent=%llu recv=%llu\n",
                   stats->pid, stats->bytes_sent, stats->bytes_recv);
    }
    /* 存储会随 socket 自动释放 */
    return 0;
}

/* Task Storage - 追踪进程可疑行为 */
SEC("tracepoint/raw_syscalls/sys_enter")
int track_syscalls(struct trace_event_raw_sys_enter *ctx) {
    struct task_struct *task = (void *)bpf_get_current_task();
    
    /* 获取或创建 task 绑定的上下文 */
    struct task_ctx *tctx = bpf_task_storage_get(&task_ctx_map, task, NULL,
                                                   BPF_LOCAL_STORAGE_GET_F_CREATE);
    if (!tctx)
        return 0;
    
    tctx->syscall_count++;
    tctx->last_syscall_ns = bpf_ktime_get_ns();
    
    /* 检测高频系统调用 (可能的 exploit 特征) */
    if (tctx->syscall_count % 10000 == 0) {
        bpf_printk("High syscall frequency: pid=%d count=%llu\n",
                   bpf_get_current_pid_tgid() >> 32,
                   tctx->syscall_count);
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

---

<!-- chunk: 10. Map 性能优化与调优 -->## 10. Map 性能优化与调优

#<!-- chunk: 10.1 Map 选择决策树 -->## 10.1 Map 选择决策树

```
Map 类型选择决策树
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

需要存储什么数据?
│
├─▶ 事件/消息 (内核→用户态)
│   ├─ 高频 (>50K事件/秒), 内核5.8+ → BPF_MAP_TYPE_RINGBUF
│   └─ 兼容性需求 或 旧内核      → BPF_MAP_TYPE_PERF_EVENT_ARRAY
│
├─▶ 统计计数器 (高并发写)
│   ├─ 整型计数，无锁最优       → BPF_MAP_TYPE_PERCPU_ARRAY (最快)
│   ├─ 动态键 (如PID)，无锁     → BPF_MAP_TYPE_PERCPU_HASH
│   └─ 允许轻微竞争             → BPF_MAP_TYPE_ARRAY + __sync_fetch_and_add
│
├─▶ 键值配置 (用户态下发)
│   ├─ 固定数量，连续键         → BPF_MAP_TYPE_ARRAY (最快)
│   └─ 动态键，任意类型         → BPF_MAP_TYPE_HASH
│
├─▶ 有界缓存 (自动淘汰)
│   ├─ 高并发，分布均匀         → BPF_MAP_TYPE_LRU_PERCPU_HASH
│   └─ 一般场景                 → BPF_MAP_TYPE_LRU_HASH
│
├─▶ IP 路由/前缀匹配            → BPF_MAP_TYPE_LPM_TRIE
│
├─▶ 程序链/动态分发             → BPF_MAP_TYPE_PROG_ARRAY
│
├─▶ Socket 重定向               → BPF_MAP_TYPE_SOCKMAP / SOCKHASH
│
├─▶ 多租户/热更新               → BPF_MAP_TYPE_ARRAY_OF_MAPS
│                                 BPF_MAP_TYPE_HASH_OF_MAPS
│
├─▶ 任务队列 (FIFO)             → BPF_MAP_TYPE_QUEUE
├─▶ 工作栈 (LIFO)               → BPF_MAP_TYPE_STACK
└─▶ 调用栈跟踪                  → BPF_MAP_TYPE_STACK_TRACE
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

#<!-- chunk: 10.2 Map 性能基准 -->## 10.2 Map 性能基准

```
Map 操作性能基准 (Intel Xeon, Linux 5.15, eBPF JIT)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

测试场景: 单核, 32字节key+value, 1M条目

Map 类型                  查找 (ns)   更新 (ns)   说明
─────────────────────── ──────────  ──────────  ─────────────────────────
PERCPU_ARRAY (读)          ~10         ~8        最快，无锁，连续内存
ARRAY (读，无竞争)         ~12         ~10       原子操作但无哈希
PERCPU_HASH (无锁)         ~25         ~30       无锁哈希，需处理冲突
HASH (轻负载)              ~35         ~40       桶锁，较低竞争
LRU_HASH                   ~45         ~50       额外LRU链表维护
LPM_TRIE                   ~80        ~100       树遍历，与前缀数量相关
LRU_PERCPU_HASH            ~30         ~35       Per-CPU LRU，竞争少

注: 实际性能受负载、内存压力、CPU缓存影响显著

高并发场景 (32核, 高冲突):
  HASH (竞争写)           ~200-500 ns  (桶锁竞争)
  PERCPU_HASH (无锁)       ~25-40  ns  (无竞争)
  → PERCPU_HASH 在并发写场景快 5-20x
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

#<!-- chunk: 10.3 内存优化策略 -->## 10.3 内存优化策略

```c
/* 内存优化技巧 */

/* 技巧1: 合理设置 max_entries */
/* 不要过度预留，但要留有余量 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    /* 根据实际连接数设置，不要盲目设置 1M */
    __uint(max_entries, 65536);  /* 64K 通常足够 */
    __type(key, struct conn_key);
    __type(value, struct conn_val);
    __uint(map_flags, BPF_F_NO_PREALLOC);  /* 按需分配 */
} conn_map SEC(".maps");

/* 技巧2: 精简 value 结构，减少对齐浪费 */
/* 不推荐: 大量填充字节 */
struct bad_stats {
    __u64 packets;    /* 8 bytes */
    __u8  proto;      /* 1 byte */
    /* 7 bytes 填充浪费! */
    __u64 bytes;      /* 8 bytes */
    /* 总: 24 bytes (有效: 17 bytes) */
};

/* 推荐: 按大小降序排列字段 */
struct good_stats {
    __u64 packets;    /* 8 bytes */
    __u64 bytes;      /* 8 bytes */
    __u8  proto;      /* 1 byte */
    __u8  _pad[7];    /* 显式填充 (清晰) */
    /* 总: 24 bytes (相同，但意图清晰) */
};

/* 更优: 去掉不必要的填充 */
struct best_stats {
    __u64 packets;    /* 8 bytes */
    __u64 bytes;      /* 8 bytes */
    /* 不需要proto时直接去掉 */
    /* 总: 16 bytes */
} __attribute__((packed));  /* 注意: packed 会影响性能 */

/* 技巧3: 使用 BPF_F_NO_PREALLOC 节省内存 */
/* 预分配 vs 按需分配:
   预分配:  启动时分配所有内存，但内存碎片少，访问快
   按需分配: 内存使用低，但可能有内存分配失败 */

/* 对于稀疏 Map (大 max_entries 但实际条目少) 用 NO_PREALLOC */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1000000);    /* 100万条目上限 */
    __uint(map_flags, BPF_F_NO_PREALLOC);  /* 只分配实际使用的内存 */
    __type(key, __u64);
    __type(value, __u64);
} sparse_map SEC(".maps");

/* 技巧4: 使用 PERCPU Map 避免伪共享 (False Sharing) */
/* 当多个 CPU 核心频繁读写相邻内存时，Cache Line 竞争严重 */
/* Per-CPU Map 每个核心独立 Cache Line，避免竞争 */
```

#<!-- chunk: 10.4 Map 监控与调优 -->## 10.4 Map 监控与调优

```bash
# Map 监控工具

# 1. 查看所有 Map 及内存使用
bpftool map list
# 输出:
# 42: hash  name conn_track  flags 0x1
#         key 16B  value 32B  max_entries 65536  memlock 4194304B
#         btf_id 156

# 2. 查看 Map 详细信息 (JSON)
bpftool map show id 42 -p

# 3. 监控 Map 内存使用
watch -n 1 'bpftool map list | grep memlock'

# 4. 查看 Map 内容 (带 BTF 格式化)
bpftool map dump id 42

# 5. 统计 Map 条目数
bpftool map dump id 42 | grep "key" | wc -l

# 6. 查看内核 BPF 内存统计
cat /proc/net/xdp_diag 2>/dev/null || true
cat /proc/sys/kernel/bpf_stats_enabled

# 7. 使用 bpftrace 监控 Map 操作频率
bpftrace -e '
kprobe:htab_map_update_elem {
    @updates[comm] = count();
}
interval:s:5 {
    print(@updates);
    clear(@updates);
}'

# 8. 检查 Map 内存上限
cat /proc/sys/kernel/bpf_stats_enabled

# 9. 调整进程 memlock 限制 (Map 需要锁定内存)
ulimit -l unlimited  # 或在 /etc/security/limits.conf 中设置
# 或在 systemd 服务中:
# LimitMEMLOCK=infinity

# Kubernetes 中设置 memlock
# 在 DaemonSet 的 securityContext 中:
# resources:
#   limits:
#     memory: 2Gi
```

---

<!-- chunk: 11. bpftool Map 操作实践 -->## 11. bpftool Map 操作实践

#<!-- chunk: 11.1 bpftool 基础操作 (Basic Operations) -->## 11.1 bpftool 基础操作 (Basic Operations)

```bash
# ========================================================
# bpftool Map 操作完整指南
# ========================================================

# 1. 列出所有 Map
bpftool map list
bpftool map list --json | jq '.'  # JSON 格式

# 输出示例:
# 5: percpu_array  name xdp_stats  flags 0x0
#     key 4B  value 48B  max_entries 1  memlock 364544B
# 6: hash  name conn_track  flags 0x1
#     key 16B  value 40B  max_entries 65536  memlock 6291456B
#     btf_id 89

# 2. 显示 Map 详情
bpftool map show id 6
bpftool map show name conn_track  # 按名称查找

# 3. 转储 Map 全部内容
bpftool map dump id 6
# 带 BTF 类型信息的格式化输出 (需要程序使用 BTF)

# 4. 查询特定 key
# 16字节 key (IPv4 5元组), hex 格式
bpftool map lookup id 6 key hex 0a 00 00 01 0a 00 00 02 1f 90 00 50 06 00 00 00

# 5. 更新/添加条目
bpftool map update id 6 \
    key hex 0a 00 00 01 0a 00 00 02 1f 90 00 50 06 00 00 00 \
    value hex 00 00 00 00 00 00 00 64 00 00 00 00 00 00 00 01 ...

# 6. 删除条目
bpftool map delete id 6 key hex 0a 00 00 01 ...

# 7. Pin Map 到 BPF 文件系统
bpftool map pin id 6 /sys/fs/bpf/conn_track_map

# 8. 从 Pin 加载 Map
bpftool map show pinned /sys/fs/bpf/conn_track_map

# 9. 批量操作 (遍历大 Map 效率更高)
bpftool map dump id 6 | head -100  # 前100条

# 10. 查看 Map 的 BTF 类型信息
bpftool btf show id <btf_id>
bpftool btf dump id <btf_id>
```

#<!-- chunk: 11.2 实战脚本示例 -->## 11.2 实战脚本示例

```bash
#!/bin/bash
# ebpf-map-monitor.sh - eBPF Map 监控脚本

set -euo pipefail

PROG_NAME=${1:-""}

echo "=== eBPF Map 监控报告 ==="
echo "时间: $(date)"
echo ""

# 获取所有 Map
MAPS=$(bpftool map list --json 2>/dev/null || echo "[]")

if [ "$MAPS" = "[]" ]; then
    echo "未发现 eBPF Map"
    exit 0
fi

echo "=== Map 列表 ==="
bpftool map list

echo ""
echo "=== 内存使用统计 ==="
total_mem=0
while IFS= read -r line; do
    memlock=$(echo "$line" | grep -oP 'memlock \K\d+' || echo "0")
    total_mem=$((total_mem + memlock))
done < <(bpftool map list)

echo "总 Map 内存锁定: $((total_mem / 1024 / 1024)) MB"

echo ""
echo "=== XDP 统计 (如果存在) ==="
XDP_MAP_ID=$(bpftool map list 2>/dev/null | grep "xdp_stats" | awk '{print $1}' | tr -d ':')
if [ -n "$XDP_MAP_ID" ]; then
    echo "XDP Stats Map (ID: $XDP_MAP_ID):"
    bpftool map dump id "$XDP_MAP_ID" 2>/dev/null || echo "无法读取"
fi

echo ""
echo "=== 连接跟踪 Map (如果存在) ==="
CONN_MAP_ID=$(bpftool map list 2>/dev/null | grep "conn_track" | awk '{print $1}' | tr -d ':')
if [ -n "$CONN_MAP_ID" ]; then
    ENTRY_COUNT=$(bpftool map dump id "$CONN_MAP_ID" 2>/dev/null | grep -c "key" || echo "0")
    echo "连接跟踪条目数: $ENTRY_COUNT"
fi
```

```bash
#!/bin/bash
# lpm-trie-manager.sh - LPM Trie 管理脚本

# 添加 IP/CIDR 到 LPM Trie
add_prefix() {
    local map_id=$1
    local cidr=$2      # 格式: "10.0.0.0/8"
    local action=$3    # 0=deny, 1=allow

    local ip prefix_len
    IFS='/' read -r ip prefix_len <<< "$cidr"

    # 将 IP 转换为小端字节序的 hex
    local ip_hex
    ip_hex=$(python3 -c "
import socket, struct
ip = socket.inet_aton('$ip')
print(' '.join(f'{b:02x}' for b in ip))
")

    # prefixlen 占4字节
    local plen_hex
    plen_hex=$(python3 -c "print(f'{$prefix_len:08x}' | sed 's/../& /g'")

    # key = prefixlen(4B) + ip(4B)
    local key_hex="$(printf '%02x %02x %02x %02x' \
        $(($prefix_len & 0xFF)) 0 0 0) $ip_hex"

    # value = action(4B) + padding
    local val_hex="$(printf '%02x 00 00 00' $action) 00 00 00 00 ..."

    echo "Adding $cidr (action=$action) to map $map_id"
    bpftool map update id "$map_id" key hex $key_hex value hex $val_hex
}

# 使用示例
# MAP_ID=$(bpftool map list | grep "acl_v4" | awk '{print $1}' | tr -d ':')
# add_prefix "$MAP_ID" "192.168.100.0/24" 0  # 拒绝此子网
# add_prefix "$MAP_ID" "10.0.0.0/8" 1        # 允许内网
```

#<!-- chunk: 11.3 使用 libbpf skeleton 的完整示例 -->## 11.3 使用 libbpf skeleton 的完整示例

```c
/* 完整的 eBPF 程序骨架 - 展示 Map 综合使用 */

/* ---- kernel/map_demo.bpf.c ---- */
#include <vmlinux.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

/* 配置 Map (用户态写入，内核态读取) */
struct config {
    __u32 sampling_rate;     /* 采样率 1/N */
    __u32 min_latency_us;    /* 最小延迟阈值 (微秒) */
    __u8  enabled;           /* 开关 */
};

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, struct config);
} config_map SEC(".maps");

/* 统计 Map (Per-CPU，内核高频写，用户态定期读) */
struct stats {
    __u64 total_calls;
    __u64 slow_calls;
    __u64 total_latency_ns;
};

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, struct stats);
} stats_map SEC(".maps");

/* 事件 Map (内核写，用户态读) */
struct slow_event {
    __u64 timestamp;
    __u64 latency_ns;
    __u32 pid;
    char  comm[16];
    char  func[32];
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 23);  /* 8MB */
} events SEC(".maps");

/* 跟踪开始时间 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, __u64);   /* tid */
    __type(value, __u64); /* start_ns */
    __uint(map_flags, BPF_F_NO_PREALLOC);
} start_times SEC(".maps");

SEC("fentry/vfs_read")
int BPF_PROG(fentry_vfs_read, struct file *file, char __user *buf,
             size_t count, loff_t *pos) {
    /* 检查配置是否启用 */
    __u32 cfg_key = 0;
    struct config *cfg = bpf_map_lookup_elem(&config_map, &cfg_key);
    if (!cfg || !cfg->enabled)
        return 0;

    /* 采样: 每 sampling_rate 次记录一次 */
    __u64 tid = bpf_get_current_pid_tgid();
    if (cfg->sampling_rate > 1) {
        __u32 rand = bpf_get_prandom_u32();
        if (rand % cfg->sampling_rate != 0)
            return 0;
    }

    __u64 start = bpf_ktime_get_ns();
    bpf_map_update_elem(&start_times, &tid, &start, BPF_ANY);
    return 0;
}

SEC("fexit/vfs_read")
int BPF_PROG(fexit_vfs_read, struct file *file, char __user *buf,
             size_t count, loff_t *pos, ssize_t ret) {
    __u64 tid = bpf_get_current_pid_tgid();

    __u64 *start = bpf_map_lookup_elem(&start_times, &tid);
    if (!start)
        return 0;

    __u64 latency = bpf_ktime_get_ns() - *start;
    bpf_map_delete_elem(&start_times, &tid);

    /* 更新统计 */
    __u32 stats_key = 0;
    struct stats *s = bpf_map_lookup_elem(&stats_map, &stats_key);
    if (s) {
        s->total_calls++;
        s->total_latency_ns += latency;
    }

    /* 读取配置阈值 */
    __u32 cfg_key = 0;
    struct config *cfg = bpf_map_lookup_elem(&config_map, &cfg_key);
    if (!cfg)
        return 0;

    /* 仅记录超过阈值的慢调用 */
    if (latency < (__u64)cfg->min_latency_us * 1000)
        return 0;

    if (s)
        s->slow_calls++;

    /* 发送慢调用事件 */
    struct slow_event *event = bpf_ringbuf_reserve(&events, sizeof(*event), 0);
    if (!event)
        return 0;

    event->timestamp = bpf_ktime_get_boot_ns();
    event->latency_ns = latency;
    event->pid = tid >> 32;
    bpf_get_current_comm(event->comm, sizeof(event->comm));
    __builtin_memcpy(event->func, "vfs_read", 8);

    bpf_ringbuf_submit(event, 0);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

```yaml
# Kubernetes ConfigMap - eBPF 监控配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: ebpf-monitor-config
  namespace: monitoring
data:
  config.json: |
    {
      "sampling_rate": 10,
      "min_latency_us": 1000,
      "enabled": true,
      "map_sizes": {
        "conn_track": 65536,
        "events_ringbuf_mb": 64,
        "stats_array": 1
      }
    }
  
  # 部署脚本
  deploy.sh: |
    #!/bin/bash
    # 加载 eBPF 程序
    ./map_demo &
    
    # 配置参数
    MAP_ID=$(bpftool map list | grep "config_map" | awk '{print $1}' | tr -d ':')
    
    # 设置采样率为 10 (每10次采样1次)
    # key=0, value: sampling_rate=10, min_latency_us=1000, enabled=1
    bpftool map update id $MAP_ID key hex 00 00 00 00 \
        value hex 0a 00 00 00 e8 03 00 00 01 00 00 00
    
    echo "eBPF monitor configured"
```

---

<!-- chunk: 📊 Map 类型速查表 -->## 📊 Map 类型速查表

| Map 类型 | 引入版本 | Key 类型 | 并发安全 | 自动淘汰 | 主要用途 |
|----------|----------|----------|----------|----------|----------|
| HASH | 3.19 | 任意 | 桶锁 | 否 | 通用键值存储 |
| PERCPU_HASH | 4.6 | 任意 | 无锁 | 否 | 高频计数 |
| LRU_HASH | 4.10 | 任意 | 桶锁 | 是 | 连接跟踪缓存 |
| LRU_PERCPU_HASH | 4.10 | 任意 | 无锁 | 是 | 高并发缓存 |
| ARRAY | 3.19 | u32 | 原子 | 否 | 配置、固定统计 |
| PERCPU_ARRAY | 4.6 | u32 | 无锁 | 否 | 最高性能计数 |
| PROG_ARRAY | 4.2 | u32 | - | - | 尾调用程序链 |
| RINGBUF | 5.8 | - | 无锁 | - | 高性能事件流 |
| PERF_EVENT_ARRAY | 4.3 | u32(cpu) | Per-CPU | - | 事件输出(旧) |
| STACK_TRACE | 4.6 | u32 | 桶锁 | 否 | 调用栈分析 |
| LPM_TRIE | 4.11 | prefix+ip | 桶锁 | 否 | IP前缀匹配 |
| STACK | 4.20 | 无 | 锁 | 否 | LIFO队列 |
| QUEUE | 4.20 | 无 | 锁 | 否 | FIFO队列 |
| ARRAY_OF_MAPS | 4.12 | u32 | 原子 | - | Map嵌套/热更新 |
| HASH_OF_MAPS | 4.12 | 任意 | 桶锁 | - | 多租户Map |
| SK_STORAGE | 5.2 | sock | 无锁 | 随sock | Per-socket状态 |
| TASK_STORAGE | 5.11 | task | 无锁 | 随task | Per-task状态 |
| INODE_STORAGE | 5.10 | inode | 无锁 | 随inode | Per-file状态 |
| BLOOM_FILTER | 5.16 | - | 无锁 | 否 | 快速存在性检查 |
| SOCKMAP | 4.14 | u32 | 锁 | - | Socket重定向 |
| DEVMAP | 4.14 | u32 | - | - | XDP设备重定向 |
| CPUMAP | 4.15 | u32 | - | - | XDP CPU重定向 |

---

<!-- chunk: 🔗 相关资源 -->## 🔗 相关资源

- **内核文档**: [kernel.org/doc/html/latest/bpf/maps.html](https://www.kernel.org/doc/html/latest/bpf/maps.html)
- **libbpf API**: [libbpf.readthedocs.io](https://libbpf.readthedocs.io/)
- **bpftool**: `man bpftool-map`
- **BCC Map 文档**: [github.com/iovisor/bcc/blob/master/docs/reference_guide.md](https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md)
- **eBPF 示例程序**: [github.com/torvalds/linux/tree/master/samples/bpf](https://github.com/torvalds/linux/tree/master/samples/bpf)
- **[[Cilium|Cilium]] eBPF Go 库**: [github.com/cilium/ebpf](https://github.com/cilium/ebpf)

---

<!-- chunk: 📝 相关文档 -->## 📝 相关文档

- **[01-eBPF架构基础](./01-ebpf-architecture-fundamentals.md)** - eBPF 虚拟机、程序类型、验证器
- **[03-Cilium CNI架构](./03-cilium-cni-architecture.md)** - Cilium 中 Map 的实际应用
- **[07-Hubble网络可观测性](./07-hubble-network-observability.md)** - Ring Buffer 在可观测性中的应用
- **[08-bcc与bpftrace工具链](./08-bcc-bpftrace-tools.md)** - Map 调试与分析工具

---
*本文档由云原生技术专家团队维护，内容基于 2026 年 eBPF 生态最新实践。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-35-ebpf-technology MOC
- [[domain-03-networking-traffic/README.md|Domain 35: eBPF 技术体系 (eBPF Technology Stack)]]
- Domain-35 eBPF 技术 — 开源项目索引
- eBPF 架构基础与程序类型 (eBPF Architecture Fundamentals and Program T...
- Cilium CNI 架构与部署 (Cilium CNI Architecture and Deployment)
- Cilium 网络策略 L3/L4/L7 (Cilium Network Policy L3/L4/L7)
- Cilium Service Mesh 无 Sidecar 架构 (Cilium Service Mesh Sideca...
- Tetragon 运行时安全 (Tetragon Runtime Security)
- Hubble 网络可观测性 (Hubble Network Observability)
- bcc 与 bpftrace 工具链 (bcc and bpftrace Tools)
- eBPF 性能优化实践 (eBPF Performance Optimization Practice)
- eBPF 安全应用案例 (eBPF Security Applications and Use Cases)

## See Also

- 10-ebpf-security-applications
- 01-ebpf-architecture-fundamentals
- 03-cilium-cni-architecture
- 04-cilium-network-policy
