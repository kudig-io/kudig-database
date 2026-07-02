---
title: eBPF 开发基础
description: 'eBPF 程序类型、libbpf/CO-RE 开发模式、Map 类型与工具链详解'
summary: 'eBPF 程序类型、libbpf/CO-RE 开发模式、Map 类型与工具链详解'
category: specialized-tech
tags:
- ebpf
- libbpf
- xdp
- btf
- co-re
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- eBPF 是什么
- 如何开发 eBPF 程序
- libbpf 和 CO-RE 是什么
trigger_keywords:
- ebpf
- libbpf
- bpf
- xdp
- tracepoint
- kprobe
- btf
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# eBPF 开发基础

## 1. eBPF 概述

eBPF（extended Berkeley Packet Filter）是 Linux 内核中的可编程虚拟机，允许在不修改内核代码的前提下，安全地在内核空间运行自定义程序。

```
用户态程序 → 加载 eBPF 字节码 → 内核验证器(Verifier) → JIT 编译 → 内核中执行
     │                                                              │
     └── 读取 Map 数据 ←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←←
```

核心优势：

| 特性 | 说明 |
|------|------|
| **安全** | 内核验证器确保程序不会崩溃内核 |
| **高性能** | JIT 编译为原生指令，接近内核模块性能 |
| **可编程** | 用户态程序可通过 Map 与内核通信 |
| **无需重启** | 动态加载/卸载，无需重启内核 |

## 2. eBPF 程序类型

### 2.1 网络类

```c
// XDP (eXpress Data Path) - 最早的网络入口点
SEC("xdp")
int xdp_drop_icmp(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;

    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;

    struct iphdr *iph = (void *)(eth + 1);
    if ((void *)(iph + 1) > data_end)
        return XDP_PASS;

    // 丢弃 ICMP 包
    if (iph->protocol == IPPROTO_ICMP)
        return XDP_DROP;

    return XDP_PASS;
}
```

```c
// TC (Traffic Control) - 流量控制层
SEC("tc")
int tc_filter_egress(struct __sk_buff *skb) {
    void *data = (void *)(long)skb->data;
    void *data_end = (void *)(long)skb->data_end;

    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return TC_ACT_OK;

    // 标记特定流量
    if (eth->h_proto == bpf_htons(ETH_P_IP)) {
        struct iphdr *iph = (void *)(eth + 1);
        if ((void *)(iph + 1) > data_end)
            return TC_ACT_OK;

        // 设置 DSCP 标记
        iph->tos = (iph->tos & 0x03) | (0x2E << 2);
    }

    return TC_ACT_OK;
}
```

### 2.2 跟踪类

```c
// kprobe - 动态跟踪内核函数
SEC("kprobe/do_sys_openat2")
int trace_open(struct pt_regs *ctx) {
    u64 pid = bpf_get_current_pid_tgid() >> 32;
    u64 ts = bpf_ktime_get_ns();

    // 记录每次 open 系统调用
    struct event evt = {
        .pid = pid,
        .ts = ts,
    };
    bpf_get_current_comm(&evt.comm, sizeof(evt.comm));

    bpf_map_update_elem(&events, &pid, &evt, BPF_ANY);
    return 0;
}
```

```c
// tracepoint - 静态内核跟踪点
SEC("tracepoint/syscalls/sys_enter_write")
int trace_write(struct trace_event_raw_sys_enter *ctx) {
    u64 pid = bpf_get_current_pid_tgid() >> 32;
    int fd = ctx->args[0];
    size_t count = ctx->args[2];

    struct write_event evt = {
        .pid = pid,
        .fd = fd,
        .count = count,
    };
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU,
                          &evt, sizeof(evt));
    return 0;
}
```

```c
// uprobe - 用户态函数跟踪
SEC("uprobe/libc.so.6:malloc")
int trace_malloc(struct pt_regs *ctx) {
    u64 pid = bpf_get_current_pid_tgid() >> 32;
    size_t size = PT_REGS_PARM1(ctx);

    bpf_printk("malloc(%lu) pid=%lu", size, pid);
    return 0;
}
```

### 2.3 程序类型速查

| 类型 | Hook 点 | 典型用途 |
|------|---------|----------|
| `XDP` | 网卡驱动层 | DDoS 防护、负载均衡 |
| `TC` | 流量控制层 | 流量整形、标记 |
| `kprobe` | 内核函数入口 | 动态跟踪 |
| `kretprobe` | 内核函数返回 | 返回值跟踪 |
| `tracepoint` | 静态跟踪点 | 系统事件监控 |
| `uprobe` | 用户态函数 | 应用级跟踪 |
| `LSM` | Linux Security Module | 安全策略 |
| `cgroup` | cgroup 网络 | 容器网络策略 |

## 3. eBPF Map 类型

### 3.1 Hash Map

```c
// 定义
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);           // PID
    __type(value, struct event); // 事件数据
    __uint(pinning, LIBBPF_PIN_BY_NAME);
} events SEC(".maps");

// 使用
SEC("kprobe/tcp_connect")
int trace_tcp_connect(struct pt_regs *ctx) {
    u32 pid = bpf_get_current_pid_tgid() >> 32;
    struct event evt = {};

    bpf_get_current_comm(&evt.comm, sizeof(evt.comm));
    evt.ts = bpf_ktime_get_ns();

    bpf_map_update_elem(&events, &pid, &evt, BPF_ANY);
    return 0;
}
```

### 3.2 Array Map

```c
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 256);
    __type(key, u32);
    __type(value, u64);
} counters SEC(".maps");

// 计数器递增
static __always_inline void increment_counter(u32 idx) {
    u64 *val = bpf_map_lookup_elem(&counters, &idx);
    if (val)
        __sync_fetch_and_add(val, 1);
}
```

### 3.3 Ring Buffer

```c
// 替代 perf event 的高效输出机制
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);  // 256KB
} rb SEC(".maps");

SEC("kprobe/tcp_sendmsg")
int trace_tcp_send(struct pt_regs *ctx) {
    struct event *evt;
    evt = bpf_ringbuf_reserve(&rb, sizeof(*evt), 0);
    if (!evt)
        return 0;

    evt->pid = bpf_get_current_pid_tgid() >> 32;
    evt->ts = bpf_ktime_get_ns();
    bpf_get_current_comm(&evt->comm, sizeof(evt->comm));

    bpf_ringbuf_submit(evt, 0);
    return 0;
}
```

### 3.4 Map 类型速查

| 类型 | 特点 | 适用场景 |
|------|------|----------|
| `HASH` | 键值对，O(1) 查找 | 状态跟踪、缓存 |
| `ARRAY` | 固定大小，索引访问 | 计数器、统计 |
| `RINGBUF` | 环形缓冲区，高效输出 | 事件流 |
| `PERF_EVENT` | 每 CPU 环形缓冲区 | 事件输出（旧） |
| `LRU_HASH` | LRU 淘汰 | 大规模缓存 |
| `LPM_TRIE` | 最长前缀匹配 | IP 路由查找 |
| `PERCPU_HASH` | 每 CPU 哈希表 | 无锁统计 |
| `STACK` | 栈结构 | 函数调用栈 |

## 4. libbpf 与 CO-RE

### 4.1 libbpf 开发模式

```c
// minimal.bpf.c - 内核态程序
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

char LICENSE[] SEC("license") = "GPL";

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} rb SEC(".maps");

SEC("tp/syscalls/sys_enter_execve")
int handle_execve(struct trace_event_raw_sys_enter *ctx)
{
    struct event *e;
    u64 pid_tgid = bpf_get_current_pid_tgid();

    e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
    if (!e)
        return 0;

    e->pid = pid_tgid >> 32;
    e->tgid = (u32)pid_tgid;
    bpf_get_current_comm(&e->comm, sizeof(e->comm));

    // CO-RE: 读取内核结构体字段
    struct task_struct *task = (struct task_struct *)bpf_get_current_task();
    e->ppid = BPF_CORE_READ(task, real_parent, tgid);

    bpf_ringbuf_submit(e, 0);
    return 0;
}
```

### 4.2 用户态加载程序

```c
// minimal.c - 用户态程序
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <bpf/libbpf.h>
#include "minimal.skel.h"    // 由 bpftool 生成

static volatile bool exiting = false;

static void sig_handler(int sig) {
    exiting = true;
}

static int handle_event(void *ctx, void *data, size_t data_sz) {
    const struct event *e = data;
    printf("exec: pid=%d ppid=%d comm=%s\n", e->pid, e->ppid, e->comm);
    return 0;
}

int main() {
    struct minimal_bpf *skel;
    struct ring_buffer *rb;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // 打开并加载 BPF 程序
    skel = minimal_bpf__open_and_load();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        return 1;
    }

    // 附加到 hook 点
    if (minimal_bpf__attach(skel)) {
        fprintf(stderr, "Failed to attach BPF skeleton\n");
        goto cleanup;
    }

    // 设置 ring buffer 回调
    rb = ring_buffer__new(bpf_map__fd(skel->maps.rb),
                          handle_event, NULL, NULL);
    if (!rb) {
        fprintf(stderr, "Failed to create ring buffer\n");
        goto cleanup;
    }

    printf("Tracing execve... Ctrl+C to exit\n");
    while (!exiting) {
        ring_buffer__poll(rb, 100);
    }

cleanup:
    ring_buffer__free(rb);
    minimal_bpf__destroy(skel);
    return 0;
}
```

### 4.3 CO-RE（Compile Once - Run Everywhere）

```c
// CO-RE: 访问内核结构体，无需内核头文件
struct task_struct *task = (struct task_struct *)bpf_get_current_task();

// BPF_CORE_READ 自动处理字节偏移
int pid = BPF_CORE_READ(task, pid);
int tgid = BPF_CORE_READ(task, tgid);
const char *comm = BPF_CORE_READ(task, comm);

// BPF_CORE_READ_INTO 读取到目标变量
struct task_struct *parent;
BPF_CORE_READ_INTO(&parent, task, real_parent);
int ppid = BPF_CORE_READ(parent, tgid);
```

CO-RE 工作原理：

```
编译时：记录字段重定位信息（BTF）
加载时：根据目标内核 BTF 调整偏移量
运行时：直接访问内核结构体字段
```

## 5. 开发工具链

### 5.1 bpftool

```bash
# 列出已加载的 BPF 程序
bpftool prog list

# 查看程序详情
bpftool prog show id 42

# 反汇编 BPF 程序
bpftool prog dump xlated id 42

# 列出所有 Map
bpftool map list

# 查看 Map 内容
bpftool map dump id 123

# 导出 BTF 信息
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
```

### 5.2 BTF 生成

```bash
# 从当前内核生成 vmlinux.h
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

# 检查内核是否支持 BTF
ls -la /sys/kernel/btf/vmlinux

# 从特定内核头文件生成
bpftool btf dump file /boot/vmlinux-$(uname -r) format c > vmlinux.h
```

### 5.3 Makefile 模板

```makefile
# Makefile for eBPF programs
CLANG ?= clang
BPFTOOL ?= bpftool
ARCH := $(shell uname -m | sed 's/x86_64/x86/' | sed 's/aarch64/arm64/')

BPF_CFLAGS := -g -O2 -target bpf -D__TARGET_ARCH_$(ARCH) \
              -I$(OUTPUT) -Wall

.PHONY: all clean

all: minimal.skel.h minimal

# 生成 vmlinux.h
$(OUTPUT)/vmlinux.h:
	$(BPFTOOL) btf dump file /sys/kernel/btf/vmlinux format c > $@

# 编译 BPF 程序
$(OUTPUT)/minimal.bpf.o: minimal.bpf.c $(OUTPUT)/vmlinux.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

# 生成 skeleton
$(OUTPUT)/minimal.skel.h: $(OUTPUT)/minimal.bpf.o
	$(BPFTOOL) gen skeleton $< > $@

# 编译用户态程序
minimal: minimal.c $(OUTPUT)/minimal.skel.h
	$(CC) -Wall -I$(OUTPUT) -o $@ $< -lbpf -lelf -lz

clean:
	rm -rf $(OUTPUT) minimal
```

### 5.4 libbpf-bootstrap

```bash
# 使用 libbpf-bootstrap 快速开始
git clone https://github.com/libbpf/libbpf-bootstrap.git
cd libbpf-bootstrap/examples/c

# 创建新项目
make minimal    # 编译示例
sudo ./minimal  # 运行
```

---

## Related

- [[domain-15-specialized-tech/05-ebpf-programming/02-ebpf-observability-tools|eBPF 可观测工具]]
- [[domain-15-specialized-tech/05-ebpf-programming/03-ebpf-networking-applications|eBPF 网络应用]]
- [[domain-15-specialized-tech/05-ebpf-programming/04-ebpf-security-runtime|eBPF 安全运行时]]

## See Also

- [eBPF 官方网站](https://ebpf.io/)
- [libbpf 文档](https://libbpf.readthedocs.io/)
- [eBPF Map 参考](https://ebpf.io/ebpf-map/)


<!-- risk-assessed -->
