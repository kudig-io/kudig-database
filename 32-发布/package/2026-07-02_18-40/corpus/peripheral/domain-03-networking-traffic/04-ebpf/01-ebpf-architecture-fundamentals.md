---
title: eBPF 架构基础与程序类型 (eBPF Architecture Fundamentals and Program Types)
description: '# eBPF 架构基础与程序类型 (eBPF Architecture Fundamentals and Program Types)'
summary: 'eBPF（Extended Berkeley Packet Filter）是 Linux 内核中的一项革命性技术，允许在内核空间安全运行沙箱程序，无需修改内核源码或加载内核模块。它本质上是一个在内核中运行的虚拟机，提供了一种安全、高效的方式来扩展内核功能。'
category: ebpf-technology
tags:
- k8s
- ebpf
- cilium
- networking
- observability
- docker
- ingress
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- SRE
- 网络工程师
- 内核工程师
estimated_read_time: 5min
intent_queries:
- eBPF 架构基础与程序类型 (eBPF Architecture Fundamentals and Program Types) 是什么
- 如何 eBPF 架构基础与程序类型 (eBPF Architecture Fundamentals and Program Types)
- Kubernetes 35 ebpf technology 最佳实践
trigger_keywords:
- eBPF
- 架构基础与程序类型
- eBPF
- Architecture
- Fundamentals
- and
- Program
- Types
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# eBPF 架构基础与程序类型 (eBPF Architecture Fundamentals and Program Types)

> **适用范围**: 内核开发、网络加速、安全监控 | **专家级别**: ⭐⭐⭐⭐⭐ | **更新时间**: 2026-03-03
> **内核要求**: Linux Kernel >= 4.9 (基础) | >= 5.15 (BTF/CO-RE) | >= 6.1 (高级特性)

---

<!-- chunk: 📋 目录 -->## 📋 目录

1. [eBPF 概述与历史演进](#1-ebpf-概述与历史演进)
2. [eBPF 虚拟机架构](#2-ebpf-虚拟机架构)
3. [eBPF 验证器工作原理](#3-ebpf-验证器工作原理)
4. [JIT 编译器与性能优化](#4-jit-编译器与性能优化)
5. [eBPF 程序类型详解](#5-ebpf-程序类型详解)
6. [eBPF 程序生命周期管理](#6-ebpf-程序生命周期管理)
7. [BTF 与 CO-RE](#7-btf-与-co-re)
8. [最佳实践与常见问题](#8-最佳实践与常见问题)

---

<!-- chunk: 1. eBPF 概述与历史演进 -->## 1. eBPF 概述与历史演进

## 1.1 什么是 eBPF (What is eBPF)

eBPF（Extended Berkeley Packet Filter）是 Linux 内核中的一项革命性技术，允许在内核空间安全运行沙箱程序，无需修改内核源码或加载内核模块。它本质上是一个在内核中运行的虚拟机，提供了一种安全、高效的方式来扩展内核功能。

**核心价值主张：**

| 特性 | 传统方式 | eBPF 方式 |
|------|---------|----------|
| 内核扩展 | 修改内核源码、编译内核模块 | 加载 eBPF 程序，无需重启 |
| 安全性 | 内核模块可崩溃系统 | 验证器保证程序安全 |
| 可观测性 | 静态探针，有限信息 | 动态插桩，完整上下文 |
| 网络性能 | 用户态网络栈开销大 | XDP 内核最早处理点，接近线速 |
| 安全策略 | SELinux/AppArmor 规则固化 | LSM eBPF 动态策略 |

## 1.2 历史演进：cBPF → eBPF (Historical Evolution)

```
时间线：Berkeley Packet Filter 的发展历程
─────────────────────────────────────────────────────────────────────────────

1992年  ┌─────────────────────────────────────────────────────────────┐
        │ cBPF (Classic BPF) 诞生                                      │
        │ • Steven McCanne & Van Jacobson 在 BSD 系统中提出             │
        │ • 论文: "The BSD Packet Filter: A New Architecture for        │
        │   User-level Packet Capture" (USENIX 1993)                   │
        │ • 2个32位寄存器 (A: Accumulator, X: Index)                   │
        │ • tcpdump 使用 cBPF 进行包过滤                               │
        └─────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
2014年  ┌─────────────────────────────────────────────────────────────┐
        │ eBPF (Extended BPF) 诞生 - Linux 3.18                        │
        │ • Alexei Starovoitov 重新设计 BPF 虚拟机                     │
        │ • 11个64位寄存器                                              │
        │ • 支持任意程序类型 (不仅仅是包过滤)                          │
        │ • JIT 编译器支持 x86-64                                      │
        │ • Maps 数据结构实现内核/用户态通信                           │
        └─────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
2015年  ┌─────────────────────────────────────────────────────────────┐
        │ kprobe 支持 - Linux 4.1                                       │
        │ • eBPF 程序可附加到内核函数探针                              │
        │ • 开始替代 SystemTap/DTrace 的部分功能                       │
        └─────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
2016年  ┌─────────────────────────────────────────────────────────────┐
        │ XDP (eXpress Data Path) - Linux 4.8                           │
        │ • 网络驱动层最早处理点                                        │
        │ • 可实现接近线速的包处理                                      │
        │ • Facebook 使用 XDP 防御 DDoS 攻击                           │
        └─────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
2017年  ┌─────────────────────────────────────────────────────────────┐
        │ Cilium 1.0 发布                                               │
        │ • 基于 eBPF 的容器网络和安全                                 │
        │ • L3/L4/L7 网络策略                                          │
        └─────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
2020年  ┌─────────────────────────────────────────────────────────────┐
        │ BTF + CO-RE - Linux 5.4/5.8                                   │
        │ • BPF Type Format 类型信息随内核发布                          │
        │ • CO-RE: Compile Once - Run Everywhere                        │
        │ • 解决内核版本兼容性问题                                      │
        └─────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
2021年  ┌─────────────────────────────────────────────────────────────┐
        │ LSM BPF - Linux 5.7                                           │
        │ • eBPF 程序可附加到 LSM 钩子                                 │
        │ • 实现灵活的内核安全策略                                      │
        │ • Tetragon 基于此技术构建                                     │
        └─────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
2022年  ┌─────────────────────────────────────────────────────────────┐
        │ BPF Token, 结构体程序 - Linux 5.15+                          │
        │ • bpf_loop() 减少验证器复杂度                                │
        │ • 改进的 CO-RE 重定位                                         │
        │ • eBPF for Windows (Microsoft 项目)                           │
        └─────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
2024年+ ┌─────────────────────────────────────────────────────────────┐
        │ eBPF 生态成熟                                                 │
        │ • Cilium CNCF 毕业项目                                        │
        │ • Linux 基金会 eBPF 基金会成立                               │
        │ • 广泛应用于云计算、安全、可观测性                           │
        └─────────────────────────────────────────────────────────────┘
```

## 1.3 cBPF vs eBPF 技术对比 (Technical Comparison)

```c
/* cBPF 程序示例：过滤 TCP 包 (tcpdump -d 'tcp') */
/* Classic BPF - 仅 2 个寄存器，有限操作集 */
struct sock_filter cBPF_tcp_filter[] = {
    { 0x28, 0, 0, 0x0000000c },  /* ldh [12]  - 加载以太网类型 */
    { 0x15, 0, 5, 0x000086dd },  /* jeq #0x86dd, IPv6 */
    { 0x30, 0, 0, 0x00000014 },  /* ldb [20]  - 加载协议类型 */
    { 0x15, 6, 0, 0x00000006 },  /* jeq #6, TCP */
    { 0x15, 0, 15, 0x0000000800},/* jeq #0x800, IPv4 */
    { 0x30, 0, 0, 0x00000017 },  /* ldb [23]  */
    { 0x15, 3, 14, 0x00000006 }, /* jeq #6, TCP */
    { 0x6, 0, 0, 0x0000ffff },   /* ret #65535 - 通过 */
    { 0x6, 0, 0, 0x00000000 },   /* ret #0 - 丢弃 */
};

/* eBPF 程序示例：XDP 过滤 TCP 包 */
/* Extended BPF - 11 个寄存器，支持函数调用、Maps */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <bpf/bpf_helpers.h>

SEC("xdp")
int xdp_filter_tcp(struct xdp_md *ctx) {
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_DROP;
    
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;
    
    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return XDP_DROP;
    
    /* 完整的 IPv4 头部长度计算 */
    if (ip->protocol != IPPROTO_TCP)
        return XDP_PASS;
    
    /* 更新统计计数器 - 使用 Map */
    __u32 key = 0;
    __u64 *count = bpf_map_lookup_elem(&tcp_packets, &key);
    if (count)
        __sync_fetch_and_add(count, 1);
    
    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";
```

---

<!-- chunk: 2. eBPF 虚拟机架构 -->## 2. eBPF 虚拟机架构

## 2.1 整体架构图 (Overall Architecture)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        eBPF 系统架构                                     │
│                                                                         │
│  用户空间 (User Space)                                                  │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │  应用程序 (libbpf / cilium-ebpf / bcc)                           │  │
│  │  ┌─────────────┐  ┌──────────────┐  ┌──────────────────────┐   │  │
│  │  │ BPF 字节码   │  │   Map 操作   │  │   程序控制/配置      │   │  │
│  │  │ (.o 文件)    │  │ (读/写/删)   │  │   (attach/detach)    │   │  │
│  │  └──────┬──────┘  └──────┬───────┘  └──────────┬───────────┘   │  │
│  └─────────┼────────────────┼─────────────────────┼───────────────┘  │
│             │                │                     │                    │
│  ┌──────────▼────────────────▼─────────────────────▼───────────────┐  │
│  │                   bpf() 系统调用                                  │  │
│  │   BPF_PROG_LOAD | BPF_MAP_CREATE | BPF_PROG_ATTACH | ...        │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                         │
├─────────────────────────────────────────────────────────────────────────┤
│  内核空间 (Kernel Space)                                                │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                    eBPF 子系统核心                                │  │
│  │                                                                   │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌──────────────────────┐ │  │
│  │  │  验证器      │    │  JIT 编译器  │    │   BPF 虚拟机         │ │  │
│  │  │ (Verifier)  │───▶│ (x86/ARM/  │    │  (解释执行/JIT)      │ │  │
│  │  │             │    │  RISC-V..) │    │                      │ │  │
│  │  └─────────────┘    └─────────────┘    └──────────────────────┘ │  │
│  │                                                                   │  │
│  │  ┌─────────────────────────────────────────────────────────────┐ │  │
│  │  │                      eBPF Maps                              │ │  │
│  │  │  Hash | Array | LRU | RingBuf | PerfEvent | StackTrace ...  │ │  │
│  │  └─────────────────────────────────────────────────────────────┘ │  │
│  └──────────────────────────────────────────────────────────────────┘  │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐  │
│  │                    内核挂载点 (Hook Points)                       │  │
│  │                                                                   │  │
│  │  网卡驱动 ──▶ XDP  ──▶ TC ingress ──▶ netfilter ──▶ TC egress  │  │
│  │                                                                   │  │
│  │  kprobe/kretprobe  tracepoint  LSM  cgroup  socket  perf event  │  │
│  └──────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────┘
```

## 2.2 eBPF 寄存器集 (Register Set)

eBPF 虚拟机拥有 11 个 64 位通用寄存器和一个程序计数器：

```
eBPF 寄存器详解
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

寄存器  用途                              映射 (x86-64)   备注
──────  ────────────────────────────────  ──────────────  ──────────────────
r0      函数返回值 / 程序退出代码          rax             程序结束时含返回值
r1      函数调用第1个参数                  rdi             程序上下文 (ctx)
r2      函数调用第2个参数                  rsi
r3      函数调用第3个参数                  rdx
r4      函数调用第4个参数                  rcx
r5      函数调用第5个参数                  r8
r6      被调用者保存 (callee-saved)        rbx             调用 helper 后保持
r7      被调用者保存                       r13
r8      被调用者保存                       r14
r9      被调用者保存                       r15
r10     只读帧指针 (frame pointer)         rbp             指向 eBPF 栈顶
pc      程序计数器                         rip             只读，不可直接修改

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

关键约束：
• r1-r5 在 helper 函数调用后可能被修改，需要保存到 r6-r9
• r10 始终指向栈底 (BPF_STACK_SIZE = 512 字节)
• 子函数调用时，参数通过 r1-r5 传递，返回值在 r0
```

```c
/* 寄存器使用示例 - eBPF 汇编视角 */
/* BPF 字节码 (人类可读形式) */

// r1 = ctx (XDP 程序的 xdp_md 指针)
// r2 = 0
// r0 = bpf_map_lookup_elem(map, &key)
// 等价于:
//   mov r2, r1          ; r2 = ctx
//   mov r1, map_fd      ; r1 = map 文件描述符
//   call bpf_map_lookup_elem
//   ; r0 现在包含查找结果 (指针或 NULL)

/* 在 C 代码中，编译器会自动处理寄存器分配 */
SEC("xdp")
int demo_register_usage(struct xdp_md *ctx) {
    /* r1 = ctx (由内核传入) */
    
    __u32 key = 0;           /* 栈上变量: [r10-4] */
    __u64 *value;
    
    /* 调用 helper - 参数在 r1-r5 */
    /* r1 = &stats_map (通过 BPF_CORE_READ 解析) */
    /* r2 = &key */
    value = bpf_map_lookup_elem(&stats_map, &key);
    /* 调用后: r0 = value (可能为 NULL) */
    /* 注意: r1-r5 已被破坏 */
    
    if (value) {
        /* value 保存在 r6 (callee-saved) 以跨越 helper 调用 */
        (*value)++;
    }
    
    return XDP_PASS; /* r0 = 2 (XDP_PASS) */
}
```

## 2.3 指令集架构 (Instruction Set Architecture)

eBPF 使用 64 位固定长度指令（部分指令为 128 位，用于立即数加载）：

```
eBPF 指令格式 (64 位)
┌───────────┬──────┬──────┬────────────┬────────────────────────────┐
│  opcode   │  dst │  src │   offset   │          imm               │
│  (8 bits) │(4bit)│(4bit)│ (16 bits)  │        (32 bits)           │
└───────────┴──────┴──────┴────────────┴────────────────────────────┘

指令类别 (opcode 的高3位):
┌──────────────────────────────────────────────────────────────────┐
│ BPF_LD    (0x00): 加载指令 (带宽)                                │
│ BPF_LDX   (0x01): 从内存加载到寄存器                            │
│ BPF_ST    (0x02): 将立即数存入内存                              │
│ BPF_STX   (0x03): 将寄存器存入内存                              │
│ BPF_ALU   (0x04): 32位算术/逻辑运算                             │
│ BPF_JMP   (0x05): 跳转指令                                      │
│ BPF_JMP32 (0x06): 32位跳转指令                                  │
│ BPF_ALU64 (0x07): 64位算术/逻辑运算                             │
└──────────────────────────────────────────────────────────────────┘
```

```c
/* 指令集使用示例 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* ALU 操作示例 */
SEC("tracepoint/syscalls/sys_enter_write")
int trace_write(struct trace_event_raw_sys_enter *ctx) {
    __u64 pid_tgid = bpf_get_current_pid_tgid();
    __u32 pid = pid_tgid >> 32;          /* 64位右移: BPF_ALU64 | BPF_RSH */
    __u32 tgid = (__u32)pid_tgid;        /* 截断为32位 */
    
    /* 内存访问 - BPF_LDX */
    int fd = (int)ctx->args[0];          /* 从结构体读取字段 */
    
    /* 条件跳转 - BPF_JMP */
    if (fd < 0)
        return 0;
    
    /* 算术运算 - BPF_ALU64 */
    __u64 write_size = (__u64)ctx->args[2];
    __u64 scaled = write_size * 1024;    /* 乘法 */
    
    bpf_printk("PID %d wrote %llu bytes to fd %d\n", pid, write_size, fd);
    
    return 0;
}
```

## 2.4 eBPF 栈 (Stack)

eBPF 程序拥有 512 字节的栈空间：

```
eBPF 栈布局 (512字节)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

高地址
┌──────────────────────────────────────────┐ r10 (帧指针)
│  [r10 - 8]   本地变量 1                  │
├──────────────────────────────────────────┤
│  [r10 - 16]  本地变量 2                  │
├──────────────────────────────────────────┤
│  [r10 - 24]  Map 查找 key (临时)         │
├──────────────────────────────────────────┤
│  ...                                     │
├──────────────────────────────────────────┤
│  [r10 - 256] 子函数调用栈帧              │
├──────────────────────────────────────────┤
│  ...                                     │
├──────────────────────────────────────────┤
│  [r10 - 512] 栈底 (512字节限制)          │
└──────────────────────────────────────────┘
低地址

关键约束：
• 最大 512 字节栈空间
• 函数调用会共享此栈空间 (非递归)
• 通过 BPF_MAP_TYPE_PERCPU_ARRAY 可扩展存储
• 栈上内容必须初始化后才能传递给 helper 函数
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

```c
/* 栈空间管理示例 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

/* 使用 Per-CPU Array 扩展存储 (超过512字节) */
struct large_event {
    __u64 timestamp;
    char comm[16];
    __u8 data[2048];  /* 超过栈大小限制 */
};

/* 使用 Per-CPU Map 作为临时存储 */
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, struct large_event);
} heap_storage SEC(".maps");

SEC("kprobe/vfs_write")
int kprobe_vfs_write(struct pt_regs *ctx) {
    __u32 key = 0;
    
    /* 从 Per-CPU Map 获取"堆"空间 */
    struct large_event *event = bpf_map_lookup_elem(&heap_storage, &key);
    if (!event)
        return 0;
    
    /* 现在可以使用超过512字节的数据结构 */
    event->timestamp = bpf_ktime_get_ns();
    bpf_get_current_comm(event->comm, sizeof(event->comm));
    
    /* 读取数据... */
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

## 2.5 Helper 函数系统 (Helper Functions)

```c
/* eBPF Helper 函数分类与使用 */

/* 1. 时间相关 */
__u64 time_ns = bpf_ktime_get_ns();        /* 内核启动后纳秒数 */
__u64 time_boot = bpf_ktime_get_boot_ns(); /* 包含挂起时间 */
__u64 time_tai = bpf_ktime_get_tai_ns();   /* TAI 时间 */

/* 2. 进程/任务相关 */
__u64 pid_tgid = bpf_get_current_pid_tgid();
__u32 pid = pid_tgid >> 32;    /* 线程 PID */
__u32 tgid = (__u32)pid_tgid;  /* 进程 TGID */

__u64 uid_gid = bpf_get_current_uid_gid();
__u32 uid = (__u32)uid_gid;
__u32 gid = uid_gid >> 32;

char comm[16];
bpf_get_current_comm(comm, sizeof(comm));  /* 进程名 */

/* 3. Map 操作 */
void *val = bpf_map_lookup_elem(&my_map, &key);
int ret = bpf_map_update_elem(&my_map, &key, &val, BPF_ANY);
int ret = bpf_map_delete_elem(&my_map, &key);
long ret = bpf_for_each_map_elem(&my_map, callback_fn, callback_ctx, 0);

/* 4. 内存操作 */
bpf_probe_read_kernel(&dst, sizeof(dst), src);  /* 安全读内核内存 */
bpf_probe_read_user(&dst, sizeof(dst), src);    /* 安全读用户内存 */
long ret = bpf_probe_read_kernel_str(buf, sizeof(buf), str_ptr);

/* 5. 网络相关 */
bpf_skb_load_bytes(skb, offset, &buf, len);    /* 从 skb 读取数据 */
bpf_skb_store_bytes(skb, offset, from, len, flags);
bpf_l3_csum_replace(skb, offset, from, to, flags);
bpf_l4_csum_replace(skb, offset, from, to, flags);
bpf_redirect(ifindex, flags);                   /* 重定向数据包 */
bpf_clone_redirect(skb, ifindex, flags);

/* 6. 性能事件 */
bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &data, sizeof(data));
bpf_ringbuf_output(&rb, &data, sizeof(data), 0);

/* 7. 尾调用 */
bpf_tail_call(ctx, &prog_array, index);         /* 跳转到另一个 eBPF 程序 */

/* 8. 跟踪输出 (调试) */
bpf_printk("key=%u, value=%llu\n", key, value); /* /sys/kernel/debug/tracing/trace_pipe */

/* Helper 函数按程序类型可用性 */
/*
类型               可用 Helper 示例
────────────────── ─────────────────────────────────────────
XDP                bpf_xdp_adjust_head/tail, bpf_redirect_map
TC (skb)           bpf_skb_*, bpf_redirect, bpf_clone_redirect
kprobe/tracepoint  bpf_probe_read_*, bpf_get_current_*, bpf_perf_event_output
LSM                bpf_ima_inode_hash, bpf_sk_storage_*
cgroup/sock        bpf_setsockopt, bpf_getsockopt, bpf_sock_ops_cb_flags_set
*/
```

---

<!-- chunk: 3. eBPF 验证器工作原理 -->## 3. eBPF 验证器工作原理

## 3.1 验证器架构 (Verifier Architecture)

```
eBPF 验证器工作流程
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

用户空间加载程序
       │
       │ bpf(BPF_PROG_LOAD, ...)
       ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          验证器 (Verifier)                               │
│                                                                         │
│  第一阶段: 基本检查 (Basic Checks)                                      │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ • 指令数量 <= 1,000,000 (内核 5.2+ 从 4096 提升)               │   │
│  │ • 指令格式合法性 (opcode, reg 范围)                             │   │
│  │ • 无非法指令 (特权指令等)                                       │   │
│  │ • 程序类型与 helper 函数权限匹配                                │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                           │                                             │
│                           ▼                                             │
│  第二阶段: 控制流分析 (Control Flow Graph Analysis)                     │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ • 构建有向无环图 (DAG)                                          │   │
│  │ • 检测并禁止循环 (传统 eBPF，5.3+ 支持有界循环)               │   │
│  │ • 确保所有代码路径都能到达 BPF_EXIT                            │   │
│  │ • 检测无法到达的代码 (dead code)                               │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                           │                                             │
│                           ▼                                             │
│  第三阶段: 数据流分析 (Data Flow Analysis) - 模拟执行                  │
│  ┌─────────────────────────────────────────────────────────────────┐   │
│  │ 寄存器状态追踪:                                                 │   │
│  │   NOT_INIT | SCALAR_VALUE | PTR_TO_MAP_VALUE | PTR_TO_CTX |    │   │
│  │   PTR_TO_STACK | PTR_TO_PACKET | PTR_TO_FUNC | ...             │   │
│  │                                                                 │   │
│  │ 内存访问检查:                                                   │   │
│  │   • 指针范围检查 (bounds checking)                             │   │
│  │   • 对齐检查 (alignment)                                       │   │
│  │   • 初始化检查 (uninitialized reads)                           │   │
│  │                                                                 │   │
│  │ 指针运算:                                                       │   │
│  │   • 仅允许有限的指针算术                                       │   │
│  │   • 追踪偏移量和可能值范围                                     │   │
│  │                                                                 │   │
│  │ Helper 调用验证:                                                │   │
│  │   • 参数类型检查                                               │   │
│  │   • 返回值类型追踪                                             │   │
│  └─────────────────────────────────────────────────────────────────┘   │
│                           │                                             │
│                     通过 │ 失败                                         │
│                     ┌────┴────┐                                         │
│                     ▼         ▼                                         │
│               JIT 编译     返回 EPERM/EINVAL                            │
│                             + 详细错误信息                              │
└─────────────────────────────────────────────────────────────────────────┘
```

## 3.2 寄存器类型系统 (Register Type System)

```c
/* 验证器追踪的寄存器状态 (内核源码简化版) */

enum bpf_reg_type {
    NOT_INIT = 0,           /* 未初始化，不可读 */
    SCALAR_VALUE,           /* 标量值 (整数) */
    PTR_TO_CTX,             /* 指向程序上下文 (如 xdp_md) */
    CONST_PTR_TO_MAP,       /* 指向 BPF Map 的常量指针 */
    PTR_TO_MAP_VALUE,       /* 指向 Map 值 */
    PTR_TO_MAP_KEY,         /* 指向 Map key */
    PTR_TO_STACK,           /* 指向 eBPF 栈 */
    PTR_TO_PACKET_META,     /* 指向数据包元数据 */
    PTR_TO_PACKET,          /* 指向数据包数据 */
    PTR_TO_PACKET_END,      /* 数据包结束指针 */
    PTR_TO_FLOW_KEYS,       /* 指向 flow_keys */
    PTR_TO_SOCKET,          /* 指向 socket */
    PTR_TO_SOCK_COMMON,     /* 指向 sock_common */
    PTR_TO_TCP_SOCK,        /* 指向 tcp_sock */
    PTR_TO_TP_BUFFER,       /* 指向 tracepoint 缓冲区 */
    PTR_TO_XDP_SOCK,        /* 指向 xdp_sock (AF_XDP) */
    PTR_TO_BTF_ID,          /* 指向内核 BTF 类型 */
    PTR_TO_MEM,             /* 通用内存指针 */
    PTR_TO_BUF,             /* 通用缓冲区指针 */
    PTR_TO_FUNC,            /* 指向函数 */
    CONST_PTR_TO_DYNPTR,    /* 指向动态指针 */
};
```

## 3.3 常见验证失败原因与修复 (Common Verification Failures)

```c
/* 错误示例 1: 未检查 NULL 指针 */
/* 错误: value is not null pointer; mem_size from mem ptr arithmetic */
SEC("xdp")
int bad_null_check(struct xdp_md *ctx) {
    __u32 key = 0;
    __u64 *value = bpf_map_lookup_elem(&my_map, &key);
    
    /* 错误: 未检查 NULL */
    *value += 1;  /* 验证器拒绝: value 可能为 NULL */
    
    return XDP_PASS;
}

/* 修复: 必须检查 NULL */
SEC("xdp")
int good_null_check(struct xdp_md *ctx) {
    __u32 key = 0;
    __u64 *value = bpf_map_lookup_elem(&my_map, &key);
    
    /* 正确: 检查 NULL 后再访问 */
    if (!value)
        return XDP_PASS;
    
    *value += 1;  /* 安全: 已验证非 NULL */
    
    return XDP_PASS;
}

/* 错误示例 2: 数据包边界检查缺失 */
SEC("xdp")
int bad_packet_access(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    
    struct ethhdr *eth = data;
    /* 错误: 未检查边界就访问 eth->h_proto */
    return eth->h_proto == bpf_htons(ETH_P_IP) ? XDP_PASS : XDP_DROP;
}

/* 修复: 必须进行边界检查 */
SEC("xdp")
int good_packet_access(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    
    struct ethhdr *eth = data;
    /* 正确: 先检查边界 */
    if ((void *)(eth + 1) > data_end)
        return XDP_DROP;
    
    return eth->h_proto == bpf_htons(ETH_P_IP) ? XDP_PASS : XDP_DROP;
}

/* 错误示例 3: 无界循环 (Kernel < 5.3) */
SEC("xdp")
int bad_unbounded_loop(struct xdp_md *ctx) {
    /* 错误 (旧内核): 验证器无法确认循环终止 */
    for (int i = 0; i < some_variable; i++) {
        /* ... */
    }
    return XDP_PASS;
}

/* 修复: 使用有界循环或 pragma unroll */
SEC("xdp")
int good_bounded_loop(struct xdp_md *ctx) {
    /* 方式1: pragma unroll (编译时展开) */
    #pragma unroll
    for (int i = 0; i < 10; i++) {
        /* ... */
    }
    
    /* 方式2: 编译时常量限制 */
    #define MAX_ITERATIONS 100
    for (int i = 0; i < MAX_ITERATIONS; i++) {
        /* ... */
    }
    
    /* 方式3: 使用 bpf_loop() (5.17+) */
    bpf_loop(1024, loop_callback, &ctx_data, 0);
    
    return XDP_PASS;
}

/* 错误示例 4: 栈溢出 */
SEC("kprobe/sys_read")
int bad_stack_usage(struct pt_regs *ctx) {
    /* 错误: 512字节栈限制 */
    char large_buf[1024];  /* 超过 512 字节 */
    bpf_probe_read_user(large_buf, sizeof(large_buf), (void *)PT_REGS_PARM2(ctx));
    return 0;
}

/* 修复: 使用 Per-CPU Map 作为"堆" */
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, char[2048]);
} heap SEC(".maps");

SEC("kprobe/sys_read")
int good_stack_usage(struct pt_regs *ctx) {
    __u32 key = 0;
    char *buf = bpf_map_lookup_elem(&heap, &key);
    if (!buf)
        return 0;
    
    bpf_probe_read_user(buf, 1024, (void *)PT_REGS_PARM2(ctx));
    return 0;
}
```

---

<!-- chunk: 4. JIT 编译器与性能优化 -->## 4. JIT 编译器与性能优化

## 4.1 JIT 编译流程 (JIT Compilation Pipeline)

```
eBPF JIT 编译流程
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

C 源码 (eBPF 程序)
       │
       │ clang -target bpf -O2 -g
       ▼
eBPF 字节码 (.o ELF 文件)
  • BPF 指令集 (RISC 风格)
  • BTF 调试信息
  • Map 重定向信息
       │
       │ bpf(BPF_PROG_LOAD)
       ▼
验证器 (Verifier)
  • 安全性验证
  • 类型检查
       │
       │ 验证通过
       ▼
JIT 编译器 (arch/x86/net/bpf_jit_comp.c)
       │
       ├── x86-64 JIT
       ├── ARM64 JIT
       ├── RISC-V JIT
       ├── s390 JIT
       └── PowerPC JIT
       │
       ▼
原生机器码
  • 寄存器直接映射
  • 无解释器开销
  • 与 C 编译代码几乎相同性能
       │
       ▼
挂载到内核 Hook 点执行

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## 4.2 JIT 编译器配置 (JIT Configuration)

```bash
# 启用 JIT 编译 (现代内核默认启用)
echo 1 > /proc/sys/net/core/bpf_jit_enable

# 启用 JIT 硬化 (Hardening) - 防止 JIT spraying 攻击
# 0: 关闭, 1: 非特权用户启用, 2: 所有用户启用
echo 2 > /proc/sys/net/core/bpf_jit_harden

# 启用 kallsyms 中显示 BPF 程序
echo 1 > /proc/sys/net/core/bpf_jit_kallsyms

# 查看 JIT 编译后的机器码 (调试用)
cat /proc/sys/net/core/bpf_jit_enable
# 设置为2可在内核日志中输出 JIT 代码

# 通过 bpftool 查看 JIT 代码
bpftool prog show id <prog_id>
bpftool prog dump jited id <prog_id>
bpftool prog dump xlated id <prog_id>  # 查看 eBPF 字节码 (BTF 注解)
```

## 4.3 JIT 性能对比 (Performance Comparison)

```
性能基准对比 (以 XDP 处理 64字节包为例)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

执行方式                 吞吐量          延迟         说明
─────────────────────── ─────────────── ─────────── ───────────────────
内核网络栈 (传统)        ~1 Mpps         ~5-10 μs    完整 TCP/IP 处理
iptables/netfilter       ~2-3 Mpps       ~2-5 μs     规则线性匹配
nftables                 ~3-5 Mpps       ~1-3 μs     更高效规则引擎
eBPF 解释器 (无JIT)      ~5 Mpps         ~500 ns     纯解释执行
eBPF + JIT              ~15-25 Mpps      ~100-200 ns JIT 编译原生代码
XDP + JIT (驱动模式)     ~40-60 Mpps      ~50-100 ns  最早处理点
DPDK (对比)              ~80-100 Mpps     ~20-50 ns   用户态，独占 CPU

注: 实际性能受硬件、内核版本、程序复杂度影响
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

<!-- chunk: 5. eBPF 程序类型详解 -->## 5. eBPF 程序类型详解

## 5.1 XDP (eXpress Data Path) 网络加速

## XDP 工作原理

```
XDP 数据包处理路径
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

网卡接收数据包
       │
       ▼
┌─────────────────────────────────────────────────────────────────────┐
│  网卡驱动层 (NIC Driver)                                             │
│                                                                     │
│  XDP_HOOK: ndo_bpf / ndo_xdp_xmit                                  │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  XDP 程序执行 (eBPF)                                          │  │
│  │                                                              │  │
│  │  返回值:                                                     │  │
│  │  XDP_DROP (1)    ──▶ 立即丢弃，不分配 skb                   │  │
│  │  XDP_PASS (2)    ──▶ 继续传入内核网络栈                     │  │
│  │  XDP_TX   (3)    ──▶ 从同一网卡发送出去                     │  │
│  │  XDP_REDIRECT(4) ──▶ 重定向到另一网卡/AF_XDP socket         │  │
│  │  XDP_ABORTED(0)  ──▶ 错误丢弃 (产生 tracepoint)             │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  XDP 模式:                                                          │
│  • Native XDP: 网卡驱动直接支持 (最高性能)                        │
│    支持: mlx4/mlx5, i40e, ixgbe, virtio-net, veth, tun ...        │
│  • Generic XDP: 内核通用实现 (兼容性好，性能较低)                 │
│  • Offloaded XDP: 在网卡硬件中执行 (最高性能，Netronome 等)       │
└─────────────────────────────────────────────────────────────────────┘
       │ XDP_PASS
       ▼
  sk_buff 分配 (内存分配开销)
       │
       ▼
  GRO (Generic Receive Offload)
       │
       ▼
  TC ingress hook
       │
       ▼
  Netfilter (iptables/nftables)
       │
       ▼
  路由决策
       │
       ▼
  套接字接收缓冲区
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## XDP 程序实战示例

```c
/* XDP DDoS 防护程序 - IP 黑名单 */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/in.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

/* IP 黑名单 Map */
struct {
    __uint(type, BPF_MAP_TYPE_LPM_TRIE);      /* 最长前缀匹配 */
    __uint(max_entries, 10000);
    __uint(key_size, sizeof(struct bpf_lpm_trie_key) + 4);  /* IPv4 */
    __uint(value_size, sizeof(__u64));
    __uint(map_flags, BPF_F_NO_PREALLOC);
} blacklist_v4 SEC(".maps");

/* 统计计数器 */
struct xdp_stats {
    __u64 rx_packets;
    __u64 dropped_packets;
    __u64 passed_packets;
};

struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_ARRAY);
    __uint(max_entries, 1);
    __type(key, __u32);
    __type(value, struct xdp_stats);
} stats_map SEC(".maps");

/* LPM key 结构 */
struct ipv4_lpm_key {
    __u32 prefixlen;
    __u32 data;
};

static __always_inline int check_blacklist_v4(__u32 src_ip) {
    struct ipv4_lpm_key key = {
        .prefixlen = 32,
        .data = src_ip,
    };
    return bpf_map_lookup_elem(&blacklist_v4, &key) != NULL;
}

SEC("xdp")
int xdp_ddos_protection(struct xdp_md *ctx) {
    void *data_end = (void *)(long)ctx->data_end;
    void *data = (void *)(long)ctx->data;
    
    __u32 stats_key = 0;
    struct xdp_stats *stats = bpf_map_lookup_elem(&stats_map, &stats_key);
    if (stats)
        __sync_fetch_and_add(&stats->rx_packets, 1);
    
    /* 解析以太网头 */
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        goto drop;
    
    __u16 eth_proto = bpf_ntohs(eth->h_proto);
    
    if (eth_proto == ETH_P_IP) {
        /* IPv4 处理 */
        struct iphdr *ip = (void *)(eth + 1);
        if ((void *)(ip + 1) > data_end)
            goto drop;
        
        /* 检查黑名单 */
        if (check_blacklist_v4(ip->saddr)) {
            if (stats)
                __sync_fetch_and_add(&stats->dropped_packets, 1);
            return XDP_DROP;
        }
    }
    
    if (stats)
        __sync_fetch_and_add(&stats->passed_packets, 1);
    return XDP_PASS;

drop:
    if (stats)
        __sync_fetch_and_add(&stats->dropped_packets, 1);
    return XDP_DROP;
}

char LICENSE[] SEC("license") = "GPL";
```

```yaml
# XDP 程序加载与管理 - 使用 bpftool
# 加载 XDP 程序
apiVersion: v1
kind: ConfigMap
metadata:
  name: xdp-loader-script
data:
  load-xdp.sh: |
    #!/bin/bash
    
    # 编译 XDP 程序
    clang -target bpf -O2 -g \
      -I/usr/include/linux \
      -c xdp_ddos.c \
      -o xdp_ddos.o
    
    # 加载到网卡 (eth0)
    # Generic XDP (兼容模式)
    ip link set dev eth0 xdpgeneric obj xdp_ddos.o sec xdp
    
    # Native XDP (需要驱动支持)
    ip link set dev eth0 xdp obj xdp_ddos.o sec xdp
    
    # 卸载 XDP 程序
    ip link set dev eth0 xdp off
    
    # 查看 XDP 程序
    bpftool net show dev eth0
    
    # 向黑名单添加 IP
    bpftool map update pinned /sys/fs/bpf/blacklist_v4 \
      key 32 0 0 0 192 168 1 100 \
      value 1 0 0 0 0 0 0 0
    
    # 查看统计
    bpftool map dump pinned /sys/fs/bpf/stats_map
```

## 5.2 TC (Traffic Control) 流量控制

## TC Hook 点

```
TC (Traffic Control) eBPF Hook 点
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

入方向 (Ingress):
  网卡 ──▶ XDP ──▶ [sk_buff 分配] ──▶ TC Ingress ──▶ Netfilter ──▶ 路由

出方向 (Egress):
  应用 ──▶ 套接字 ──▶ 路由 ──▶ Netfilter ──▶ TC Egress ──▶ 网卡 ──▶ 发出

TC eBPF 动作:
  TC_ACT_OK       (0): 继续正常处理
  TC_ACT_RECLASSIFY(1): 重新分类
  TC_ACT_SHOT     (2): 丢弃数据包
  TC_ACT_PIPE     (3): 传递给下一个动作
  TC_ACT_STOLEN   (4): 数据包被"偷走" (redirect)
  TC_ACT_QUEUED   (5): 加入队列
  TC_ACT_REPEAT   (6): 重复动作
  TC_ACT_REDIRECT (7): 重定向
  TC_ACT_TRAP     (8): 陷入到用户空间 (tc 程序)

TC vs XDP:
  ┌────────────────┬──────────────┬────────────────────────────────────┐
  │ 特性           │ XDP           │ TC                                 │
  ├────────────────┼──────────────┼────────────────────────────────────┤
  │ 执行时机       │ 驱动层 (最早) │ 内核网络栈 (已有 skb)              │
  │ skb 可用性     │ 无 skb       │ 有 skb，可读写全部字段              │
  │ 出方向支持     │ 仅 TX        │ Ingress + Egress                   │
  │ 隧道支持       │ 有限         │ 完整 (vxlan, geneve, ipip)         │
  │ 性能           │ 最高         │ 较高 (skb 分配后)                  │
  │ 容器网络       │ 常用于入口   │ Cilium 大量使用                    │
  └────────────────┴──────────────┴────────────────────────────────────┘
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

```c
/* TC eBPF 程序示例 - 服务负载均衡 */
#include <linux/bpf.h>
#include <linux/pkt_cls.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

/* 后端服务器 Map */
struct backend {
    __u32 ip;
    __u16 port;
    __u8  weight;
    __u8  active;
};

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 8);
    __type(key, __u32);
    __type(value, struct backend);
} backends SEC(".maps");

/* 连接跟踪 Map */
struct conn_key {
    __u32 src_ip;
    __u16 src_port;
    __u32 dst_ip;
    __u16 dst_port;
};

struct conn_val {
    __u32 backend_ip;
    __u16 backend_port;
    __u64 last_seen;
};

struct {
    __uint(type, BPF_MAP_TYPE_LRU_HASH);
    __uint(max_entries, 65536);
    __type(key, struct conn_key);
    __type(value, struct conn_val);
} conn_track SEC(".maps");

static __always_inline __u16 csum_fold(__u32 csum) {
    csum = (csum & 0xffff) + (csum >> 16);
    csum = (csum & 0xffff) + (csum >> 16);
    return (__u16)~csum;
}

SEC("tc")
int tc_load_balancer(struct __sk_buff *skb) {
    void *data_end = (void *)(long)skb->data_end;
    void *data = (void *)(long)skb->data;
    
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return TC_ACT_SHOT;
    
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return TC_ACT_OK;
    
    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return TC_ACT_SHOT;
    
    if (ip->protocol != IPPROTO_TCP)
        return TC_ACT_OK;
    
    __u32 ip_hdr_len = ip->ihl * 4;
    struct tcphdr *tcp = (void *)ip + ip_hdr_len;
    if ((void *)(tcp + 1) > data_end)
        return TC_ACT_SHOT;
    
    /* 检查是否是到 VIP 的流量 */
    __u32 vip = bpf_htonl(0xC0A80001);  /* 192.168.0.1 */
    if (ip->daddr != vip)
        return TC_ACT_OK;
    
    /* 查找已有连接 */
    struct conn_key ckey = {
        .src_ip = ip->saddr,
        .src_port = tcp->source,
        .dst_ip = ip->daddr,
        .dst_port = tcp->dest,
    };
    
    struct conn_val *cval = bpf_map_lookup_elem(&conn_track, &ckey);
    
    __u32 backend_ip;
    __u16 backend_port;
    
    if (cval) {
        /* 使用已有连接的后端 */
        backend_ip = cval->backend_ip;
        backend_port = cval->backend_port;
    } else {
        /* 选择后端 (简单轮询) */
        __u32 idx = bpf_get_prandom_u32() % 3;
        struct backend *be = bpf_map_lookup_elem(&backends, &idx);
        if (!be || !be->active)
            return TC_ACT_SHOT;
        
        backend_ip = be->ip;
        backend_port = be->port;
        
        /* 记录连接 */
        struct conn_val new_val = {
            .backend_ip = backend_ip,
            .backend_port = backend_port,
            .last_seen = bpf_ktime_get_ns(),
        };
        bpf_map_update_elem(&conn_track, &ckey, &new_val, BPF_ANY);
    }
    
    /* 修改目标 IP 和端口 (DNAT) */
    /* 注意: 实际需要更新校验和 */
    bpf_skb_store_bytes(skb, 
        (void *)&ip->daddr - data,
        &backend_ip, 4, BPF_F_RECOMPUTE_CSUM);
    bpf_skb_store_bytes(skb,
        (void *)&tcp->dest - data,
        &backend_port, 2, BPF_F_RECOMPUTE_CSUM);
    
    return TC_ACT_OK;
}

char LICENSE[] SEC("license") = "GPL";
```

```bash
# TC eBPF 程序加载
# 创建 clsact qdisc (支持 eBPF 的虚拟 qdisc)
tc qdisc add dev eth0 clsact

# 加载 ingress 程序
tc filter add dev eth0 ingress bpf obj tc_lb.o sec tc

# 加载 egress 程序
tc filter add dev eth0 egress bpf obj tc_lb.o sec tc

# 查看已加载的过滤器
tc filter show dev eth0 ingress

# 删除过滤器
tc filter del dev eth0 ingress
tc qdisc del dev eth0 clsact

# 使用 bpftool 管理 TC 程序
bpftool net show
bpftool prog show tag <prog_tag>
```

## 5.3 kprobe/kretprobe 内核跟踪

```c
/* kprobe 跟踪内核函数示例 */
#include <linux/bpf.h>
#include <linux/ptrace.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

/* 文件打开事件结构 */
struct file_open_event {
    __u64 timestamp;
    __u32 pid;
    __u32 uid;
    char comm[16];
    char filename[256];
    int flags;
    int ret;  /* 仅 kretprobe 有效 */
};

/* Ring Buffer 传递事件 */
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);  /* 16 MB */
} events SEC(".maps");

/* kprobe: 在 do_sys_openat2 入口处执行 */
SEC("kprobe/do_sys_openat2")
int BPF_KPROBE(kprobe_openat2, int dfd, const char __user *filename, 
               struct open_how *how) {
    struct file_open_event *event;
    
    /* 分配 ring buffer 空间 */
    event = bpf_ringbuf_reserve(&events, sizeof(*event), 0);
    if (!event)
        return 0;
    
    event->timestamp = bpf_ktime_get_ns();
    event->pid = bpf_get_current_pid_tgid() >> 32;
    event->uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(event->comm, sizeof(event->comm));
    
    /* 安全读取用户态字符串 */
    bpf_probe_read_user_str(event->filename, sizeof(event->filename), filename);
    
    /* 读取 open_how 结构体的 flags 字段 (使用 CO-RE) */
    event->flags = BPF_CORE_READ(how, flags);
    event->ret = 0;
    
    /* 提交事件 */
    bpf_ringbuf_submit(event, 0);
    
    return 0;
}

/* kretprobe: 在 do_sys_openat2 返回时执行 */
SEC("kretprobe/do_sys_openat2")
int BPF_KRETPROBE(kretprobe_openat2, long ret) {
    /* 仅关注失败的 open 调用 */
    if (ret >= 0)
        return 0;
    
    struct file_open_event *event;
    event = bpf_ringbuf_reserve(&events, sizeof(*event), 0);
    if (!event)
        return 0;
    
    event->timestamp = bpf_ktime_get_ns();
    event->pid = bpf_get_current_pid_tgid() >> 32;
    event->uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(event->comm, sizeof(event->comm));
    event->ret = ret;
    
    bpf_ringbuf_submit(event, 0);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

```c
/* 使用 fentry/fexit 替代 kprobe (更高效，5.5+) */
/* fentry/fexit 直接 hook 内核函数，无需 int3 断点，性能更好 */

SEC("fentry/tcp_connect")
int BPF_PROG(fentry_tcp_connect, struct sock *sk) {
    __u64 pid_tgid = bpf_get_current_pid_tgid();
    
    struct {
        __u32 pid;
        __u32 saddr;
        __u32 daddr;
        __u16 dport;
    } event = {
        .pid = pid_tgid >> 32,
        .saddr = BPF_CORE_READ(sk, __sk_common.skc_rcv_saddr),
        .daddr = BPF_CORE_READ(sk, __sk_common.skc_daddr),
        .dport = BPF_CORE_READ(sk, __sk_common.skc_dport),
    };
    
    bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU,
                          &event, sizeof(event));
    return 0;
}

SEC("fexit/tcp_connect")
int BPF_PROG(fexit_tcp_connect, struct sock *sk, int ret) {
    if (ret != 0) {
        bpf_printk("tcp_connect failed: ret=%d\n", ret);
    }
    return 0;
}
```

## 5.4 Tracepoint 静态跟踪点

```c
/* Tracepoint 示例 - 跟踪调度器事件 */
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

/* Tracepoint 参数结构 - 通过 BTF 自动获取 */
/* 路径: /sys/kernel/debug/tracing/events/sched/sched_switch/format */
struct sched_switch_args {
    unsigned long long pad;  /* common fields */
    char prev_comm[16];
    pid_t prev_pid;
    int prev_prio;
    long prev_state;
    char next_comm[16];
    pid_t next_pid;
    int next_prio;
};

/* 进程调度延迟跟踪 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, __u32);    /* pid */
    __type(value, __u64);  /* 进入就绪队列时间 */
} sched_start SEC(".maps");

/* 调度延迟直方图 */
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 64);  /* 64 个时间桶 */
    __type(key, __u32);
    __type(value, __u64);
} lat_hist SEC(".maps");

/* 进程加入就绪队列 */
SEC("tracepoint/sched/sched_wakeup")
int tp_sched_wakeup(struct trace_event_raw_sched_wakeup *ctx) {
    __u32 pid = ctx->pid;
    __u64 ts = bpf_ktime_get_ns();
    bpf_map_update_elem(&sched_start, &pid, &ts, BPF_ANY);
    return 0;
}

/* 进程开始执行 */
SEC("tracepoint/sched/sched_switch")
int tp_sched_switch(struct sched_switch_args *ctx) {
    /* 记录被调度出去的进程 */
    __u32 prev_pid = ctx->prev_pid;
    __u32 next_pid = ctx->next_pid;
    
    /* 计算 next 进程的调度延迟 */
    __u64 *start_ts = bpf_map_lookup_elem(&sched_start, &next_pid);
    if (start_ts) {
        __u64 now = bpf_ktime_get_ns();
        __u64 latency_ns = now - *start_ts;
        
        /* 更新直方图 */
        __u32 slot = 0;
        __u64 lat_us = latency_ns / 1000;
        
        /* log2 近似 */
        if (lat_us >= 1) {
            slot = 1;
            __u64 v = lat_us;
            #pragma unroll
            for (int i = 0; i < 63; i++) {
                v >>= 1;
                if (v == 0) break;
                slot++;
            }
        }
        if (slot >= 64) slot = 63;
        
        __u64 *count = bpf_map_lookup_elem(&lat_hist, &slot);
        if (count)
            __sync_fetch_and_add(count, 1);
        
        bpf_map_delete_elem(&sched_start, &next_pid);
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

## 5.5 LSM (Linux Security Module) 安全钩子

```c
/* LSM eBPF 程序 - 运行时安全策略 */
#include <linux/bpf.h>
#include <linux/lsm_hook_defs.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

/* 禁止执行特定文件的规则 */
struct deny_rule {
    char path[256];
    __u64 deny_count;
};

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, char[256]);
    __type(value, struct deny_rule);
} deny_exec_paths SEC(".maps");

/* 进程执行审计日志 */
struct exec_audit {
    __u64 timestamp;
    __u32 pid;
    __u32 uid;
    char comm[16];
    char filename[256];
    int denied;
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);
} audit_events SEC(".maps");

/* LSM hook: file_open */
SEC("lsm/file_open")
int BPF_PROG(lsm_file_open, struct file *file) {
    /* 获取文件路径 */
    char path[256] = {};
    
    struct dentry *dentry = BPF_CORE_READ(file, f_path.dentry);
    struct qstr name = BPF_CORE_READ(dentry, d_name);
    
    /* 简化: 仅检查文件名 (实际应检查完整路径) */
    bpf_probe_read_kernel_str(path, sizeof(path), name.name);
    
    /* 查找是否在拒绝列表 */
    struct deny_rule *rule = bpf_map_lookup_elem(&deny_exec_paths, path);
    
    /* 记录审计日志 */
    struct exec_audit *event = bpf_ringbuf_reserve(&audit_events, sizeof(*event), 0);
    if (event) {
        event->timestamp = bpf_ktime_get_ns();
        event->pid = bpf_get_current_pid_tgid() >> 32;
        event->uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
        bpf_get_current_comm(event->comm, sizeof(event->comm));
        __builtin_memcpy(event->filename, path, sizeof(path));
        event->denied = rule ? 1 : 0;
        bpf_ringbuf_submit(event, 0);
    }
    
    if (rule) {
        /* 增加拒绝计数 */
        __sync_fetch_and_add(&rule->deny_count, 1);
        return -EPERM;  /* 拒绝访问 */
    }
    
    return 0;  /* 允许 */
}

/* LSM hook: bprm_check_security - 检查程序执行 */
SEC("lsm/bprm_check_security")
int BPF_PROG(lsm_bprm_check, struct linux_binprm *bprm) {
    char filename[256] = {};
    
    struct file *file = BPF_CORE_READ(bprm, file);
    struct dentry *dentry = BPF_CORE_READ(file, f_path.dentry);
    const unsigned char *name = BPF_CORE_READ(dentry, d_name.name);
    
    bpf_probe_read_kernel_str(filename, sizeof(filename), name);
    
    /* 检查是否允许执行 */
    struct deny_rule *rule = bpf_map_lookup_elem(&deny_exec_paths, filename);
    if (rule)
        return -EACCES;
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

## 5.6 cgroup 程序类型

```c
/* cgroup eBPF 程序 - 容器网络控制 */
#include <linux/bpf.h>
#include <linux/in.h>
#include <bpf/bpf_helpers.h>

/* 允许的端口白名单 */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, __u16);   /* 端口号 */
    __type(value, __u8);  /* 1=允许 */
} allowed_ports SEC(".maps");

/* cgroup/connect4: 控制 IPv4 连接 */
SEC("cgroup/connect4")
int cgroup_connect4(struct bpf_sock_addr *ctx) {
    /* 获取目标端口 */
    __u16 port = bpf_ntohs(ctx->user_port);
    
    /* 检查端口是否在白名单 */
    __u8 *allowed = bpf_map_lookup_elem(&allowed_ports, &port);
    if (!allowed)
        return 0;  /* 拒绝连接 */
    
    return 1;  /* 允许连接 */
}

/* cgroup/sock_create: 控制 socket 创建 */
SEC("cgroup/sock_create")
int cgroup_sock_create(struct bpf_sock *sk) {
    /* 仅允许 TCP 和 UDP */
    if (sk->type != SOCK_STREAM && sk->type != SOCK_DGRAM)
        return 0;  /* 拒绝 */
    return 1;     /* 允许 */
}

/* cgroup/skb: 控制数据包 (与 XDP/TC 类似) */
SEC("cgroup_skb/ingress")
int cgroup_skb_ingress(struct __sk_buff *skb) {
    /* 只允许本地回环流量或已建立连接 */
    __u32 src = skb->remote_ip4;
    
    /* 允许 127.0.0.0/8 */
    if ((bpf_ntohl(src) & 0xFF000000) == 0x7F000000)
        return 1;
    
    return 1;  /* 默认允许 */
}

char LICENSE[] SEC("license") = "GPL";
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# cgroup eBPF 程序挂载
# 找到容器的 cgroup 路径
CONTAINER_ID=$(docker ps -q -f name=myapp)
CGROUP_PATH="/sys/fs/cgroup/unified/docker/${CONTAINER_ID}"

# 使用 bpftool 挂载 cgroup 程序
bpftool prog load cgroup_control.o /sys/fs/bpf/cgroup_prog

bpftool cgroup attach ${CGROUP_PATH} connect4 \
    pinned /sys/fs/bpf/cgroup_prog

# 在 Kubernetes 中使用 (Cilium 方式)
# Cilium 自动为每个 Pod 的 cgroup 挂载策略程序
```
## 5.7 Socket 过滤器

```c
/* Socket Filter 程序 - 捕获特定流量 */
#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

/* BPF_PROG_TYPE_SOCKET_FILTER:
   返回 0 = 丢弃数据包
   返回 > 0 = 保留该数量字节 */

SEC("socket")
int socket_filter_http(struct __sk_buff *skb) {
    /* 仅保留 HTTP 流量 (目标端口 80 或 8080) */
    
    __u8 ip_proto;
    /* 以太网头: 14字节, IP 协议字段偏移 23 */
    if (bpf_skb_load_bytes(skb, 23, &ip_proto, 1) < 0)
        return 0;
    
    if (ip_proto != IPPROTO_TCP)
        return 0;
    
    /* IP 头部长度 (假设标准 20 字节) */
    /* TCP 目标端口: 以太网(14) + IP(20) + TCP_DST_PORT_OFFSET(2) */
    __u16 dst_port;
    if (bpf_skb_load_bytes(skb, 36, &dst_port, 2) < 0)
        return 0;
    
    dst_port = bpf_ntohs(dst_port);
    
    if (dst_port == 80 || dst_port == 8080 || dst_port == 443)
        return skb->len;  /* 保留全部数据 */
    
    return 0;  /* 丢弃 */
}

char LICENSE[] SEC("license") = "GPL";
```

---

<!-- chunk: 6. eBPF 程序生命周期管理 -->## 6. eBPF 程序生命周期管理

## 6.1 程序生命周期 (Program Lifecycle)

```
eBPF 程序生命周期
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

1. 编译阶段
   C 源码 ──[clang/llvm]──▶ eBPF 字节码 (.o ELF)
                                    │
2. 加载阶段                         │
   用户进程 ──[bpf() syscall]──▶ 内核
   • BPF_PROG_LOAD                  │
   • 验证器检查                     │
   • JIT 编译                       │
   • 返回 prog_fd (文件描述符)      │
                                    │
3. 挂载阶段                         │
   prog_fd ──[bpf() / ip / tc]──▶ Hook 点
   • BPF_PROG_ATTACH                │
   • xdp via netlink                │
   • tc via netlink                 │
   • kprobe via perf_event          │
                                    │
4. 运行阶段                         │
   触发条件 ──▶ eBPF 程序执行 ──▶ 返回结果
                    │
                    ▼
              操作 Maps
              调用 Helpers
              输出事件
                                    │
5. 卸载阶段                         │
   close(prog_fd) 或显式 detach     │
   当所有 fd 关闭且无 pinned 引用时  │
   程序被内核释放                   │
                                    │
6. 持久化 (可选)                    │
   bpf_obj_pin(prog_fd, "/sys/fs/bpf/my_prog")
   程序 pin 到 BPF 文件系统，进程退出后仍然存在
   通过 bpf_obj_get("/sys/fs/bpf/my_prog") 重新获取

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## 6.2 使用 libbpf 管理程序生命周期

```c
/* 用户态程序: 使用 libbpf 加载和管理 eBPF 程序 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include "xdp_ddos.skel.h"  /* 由 bpftool gen skeleton 生成 */

static volatile bool running = true;

static void sig_handler(int sig) {
    running = false;
}

int main(int argc, char *argv[]) {
    struct xdp_ddos_bpf *skel;
    int ifindex, err;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <interface>\n", argv[0]);
        return 1;
    }
    
    ifindex = if_nametoindex(argv[1]);
    if (!ifindex) {
        perror("if_nametoindex");
        return 1;
    }
    
    /* 1. 打开 BPF 对象 */
    skel = xdp_ddos_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF object\n");
        return 1;
    }
    
    /* 2. 可选: 在加载前修改 Map 大小等参数 */
    bpf_map__set_max_entries(skel->maps.blacklist_v4, 100000);
    
    /* 3. 加载 eBPF 程序 (验证 + JIT) */
    err = xdp_ddos_bpf__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %d\n", err);
        goto cleanup;
    }
    
    /* 4. 挂载 XDP 程序 */
    err = bpf_xdp_attach(ifindex, 
                          bpf_program__fd(skel->progs.xdp_ddos_protection),
                          XDP_FLAGS_DRV_MODE,  /* Native XDP */
                          NULL);
    if (err) {
        /* 回退到 Generic XDP */
        err = bpf_xdp_attach(ifindex,
                              bpf_program__fd(skel->progs.xdp_ddos_protection),
                              XDP_FLAGS_SKB_MODE,
                              NULL);
        if (err) {
            fprintf(stderr, "Failed to attach XDP: %d\n", err);
            goto cleanup;
        }
    }
    
    printf("XDP program loaded on %s (ifindex=%d)\n", argv[1], ifindex);
    
    /* 5. Pin Maps 到 BPF 文件系统 (可选，用于工具访问) */
    err = bpf_map__pin(skel->maps.stats_map, "/sys/fs/bpf/xdp_stats");
    if (err)
        fprintf(stderr, "Warning: failed to pin stats map: %d\n", err);
    
    /* 6. 运行循环 */
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);
    
    while (running) {
        /* 读取统计信息 */
        __u32 key = 0;
        struct xdp_stats stats[libbpf_num_possible_cpus()];
        
        err = bpf_map__lookup_elem(skel->maps.stats_map, 
                                    &key, sizeof(key),
                                    stats, sizeof(stats), 0);
        if (!err) {
            __u64 total_rx = 0, total_drop = 0;
            for (int i = 0; i < libbpf_num_possible_cpus(); i++) {
                total_rx += stats[i].rx_packets;
                total_drop += stats[i].dropped_packets;
            }
            printf("\rRX: %llu, Dropped: %llu (%.2f%%)",
                   total_rx, total_drop,
                   total_rx ? 100.0 * total_drop / total_rx : 0.0);
            fflush(stdout);
        }
        
        sleep(1);
    }
    
    printf("\nDetaching XDP program...\n");
    
    /* 7. 卸载 XDP 程序 */
    bpf_xdp_detach(ifindex, XDP_FLAGS_DRV_MODE, NULL);
    bpf_xdp_detach(ifindex, XDP_FLAGS_SKB_MODE, NULL);

cleanup:
    /* 8. 释放资源 */
    xdp_ddos_bpf__destroy(skel);
    return err;
}
```

## 6.3 BPF 文件系统 (BPF Filesystem)

```bash
# BPF 文件系统操作
# 挂载 BPF 文件系统
mount -t bpf bpf /sys/fs/bpf

# 查看 BPF 文件系统内容
ls -la /sys/fs/bpf/

# Pin 程序
bpftool prog pin id <prog_id> /sys/fs/bpf/my_prog

# Pin Map
bpftool map pin id <map_id> /sys/fs/bpf/my_map

# 从 Pin 重新加载
bpf_prog_fd = bpf_obj_get("/sys/fs/bpf/my_prog")
bpf_map_fd = bpf_obj_get("/sys/fs/bpf/my_map")

# 查看所有 BPF 程序
bpftool prog list

# 查看程序详情
bpftool prog show id <id> -p

# 查看所有 BPF Map
bpftool map list

# 转储 Map 内容
bpftool map dump id <id>

# 更新 Map 内容
bpftool map update id <id> key 0x01 0x00 0x00 0x00 value 0x01

# 查看内核 BPF 统计
cat /proc/sys/kernel/bpf_stats_enabled
echo 1 > /proc/sys/kernel/bpf_stats_enabled
bpftool prog show  # 现在显示运行时间统计
```

---

<!-- chunk: 7. BTF 与 CO-RE -->## 7. BTF 与 CO-RE

## 7.1 BTF (BPF Type Format) 概述

```
BTF 架构与作用
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

BTF 是一种描述 BPF 程序和 Map 使用的类型信息的元数据格式。
它是 DWARF 调试信息的轻量级替代品，专为 eBPF 设计。

BTF 的作用:
┌─────────────────────────────────────────────────────────────────┐
│ 1. 验证器增强                                                   │
│    • 更精确的类型检查                                          │
│    • 更好的错误信息                                            │
│                                                                 │
│ 2. 调试信息                                                    │
│    • bpftool prog dump xlated 显示 C 源码注解                  │
│    • BTF 使 eBPF 程序可调试                                    │
│                                                                 │
│ 3. Map Pretty-Print                                            │
│    • bpftool map dump 显示结构化数据                           │
│    • 无需手动解析二进制数据                                   │
│                                                                 │
│ 4. CO-RE 基础                                                  │
│    • 内核类型信息通过 BTF 暴露                                │
│    • eBPF 程序可访问内核结构体字段                            │
│    • 无需内核头文件                                            │
│                                                                 │
│ 5. Ring Buffer 类型安全                                        │
│    • BPF_MAP_TYPE_RINGBUF 使用 BTF 进行类型注解               │
└─────────────────────────────────────────────────────────────────┘

BTF 信息来源:
  /sys/kernel/btf/vmlinux    <- 内核 BTF
  程序 .o 文件中的 .BTF section
  程序 .o 文件中的 .BTF.ext section (行号信息)

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## 7.2 CO-RE (Compile Once - Run Everywhere)

```c
/* CO-RE 使用示例 */
/* 解决不同内核版本间结构体布局差异问题 */

#include <vmlinux.h>        /* 从内核 BTF 生成的所有内核类型 */
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>  /* CO-RE 宏 */
#include <bpf/bpf_tracing.h>

/* 方式1: 使用 BPF_CORE_READ 宏 (推荐) */
SEC("kprobe/tcp_sendmsg")
int kprobe_tcp_sendmsg(struct pt_regs *ctx) {
    struct sock *sk = (struct sock *)PT_REGS_PARM1(ctx);
    
    /* CO-RE 安全读取: 自动处理不同内核版本的字段偏移 */
    __u32 src_ip = BPF_CORE_READ(sk, __sk_common.skc_rcv_saddr);
    __u32 dst_ip = BPF_CORE_READ(sk, __sk_common.skc_daddr);
    __u16 dst_port = BPF_CORE_READ(sk, __sk_common.skc_dport);
    
    /* 嵌套读取 */
    __u32 netns_ino = BPF_CORE_READ(sk, __sk_common.skc_net.net, 
                                     ns.inum);
    
    bpf_printk("TCP: %x -> %x:%d (ns=%u)\n", 
               src_ip, dst_ip, bpf_ntohs(dst_port), netns_ino);
    return 0;
}

/* 方式2: 使用 bpf_core_read() 函数 */
SEC("kprobe/vfs_read")
int kprobe_vfs_read(struct pt_regs *ctx) {
    struct file *file = (struct file *)PT_REGS_PARM1(ctx);
    
    /* 读取文件路径 */
    struct dentry *dentry;
    bpf_core_read(&dentry, sizeof(dentry), &file->f_path.dentry);
    
    struct qstr d_name;
    bpf_core_read(&d_name, sizeof(d_name), &dentry->d_name);
    
    char filename[256];
    bpf_probe_read_kernel_str(filename, sizeof(filename), d_name.name);
    
    bpf_printk("vfs_read: %s\n", filename);
    return 0;
}

/* 方式3: CO-RE 枚举值访问 */
SEC("kprobe/do_exit")
int kprobe_do_exit(struct pt_regs *ctx) {
    struct task_struct *task = (struct task_struct *)bpf_get_current_task();
    
    /* 读取进程退出代码 */
    int exit_code = BPF_CORE_READ(task, exit_code);
    
    /* 读取进程标志 */
    unsigned int flags = BPF_CORE_READ(task, flags);
    
    /* 检查是否是内核线程 */
    bool is_kthread = (flags & PF_KTHREAD) != 0;
    
    if (!is_kthread) {
        bpf_printk("Process exiting: pid=%d, exit_code=%d\n",
                   BPF_CORE_READ(task, pid), exit_code);
    }
    
    return 0;
}

/* 方式4: 条件编译兼容不同内核版本 */
struct task_struct___old {
    int pid;
    /* 旧内核中的字段 */
} __attribute__((preserve_access_index));

struct task_struct___new {
    int pid;
    __u64 random_seed;  /* 新内核新增字段 */
} __attribute__((preserve_access_index));

SEC("kprobe/sys_fork")
int handle_fork(struct pt_regs *ctx) {
    struct task_struct *task = (void *)bpf_get_current_task();
    
    /* CO-RE: 检查字段是否存在 */
    if (bpf_core_field_exists(((struct task_struct___new *)0)->random_seed)) {
        /* 新内核: 访问新字段 */
        __u64 seed = BPF_CORE_READ((struct task_struct___new *)task, random_seed);
        bpf_printk("New kernel, seed=%llu\n", seed);
    } else {
        /* 旧内核: 使用兼容方式 */
        bpf_printk("Old kernel, no random_seed field\n");
    }
    
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
```

```bash
# BTF/CO-RE 工具操作

# 检查内核是否支持 BTF
ls /sys/kernel/btf/vmlinux

# 生成 vmlinux.h (包含所有内核类型)
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h

# 查看程序的 BTF 信息
bpftool prog dump xlated id <id> linum  # 显示源码行号
bpftool map dump id <id>                # 显示结构化数据

# 验证 CO-RE 重定向信息
llvm-readelf -S my_prog.o | grep BTF   # 查看 BTF section
bpftool btf dump file my_prog.o        # 查看程序 BTF

# 使用 pahole 生成 BTF 信息 (旧内核)
pahole -J --btf_encode_detached external.btf vmlinux
```

## 7.3 vmlinux.h 的使用

```c
/* 使用 vmlinux.h 无需内核头文件 */

/* 传统方式: 需要大量内核头文件 */
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/socket.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/tcp.h>
// ... 可能需要数十个头文件，且与内核版本绑定

/* CO-RE 方式: 只需 vmlinux.h */
#include "vmlinux.h"  /* 包含所有内核类型 */
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>

/* 现在可以直接使用所有内核结构体 */
SEC("fentry/tcp_v4_connect")
int BPF_PROG(fentry_tcp_v4_connect, struct sock *sk) {
    /* 直接访问 task_struct、sock、file 等所有内核结构体 */
    struct task_struct *task = (void *)bpf_get_current_task_btf();
    
    pid_t pid = BPF_CORE_READ(task, pid);
    uid_t uid = BPF_CORE_READ(task, cred, uid.val);
    
    /* 读取 socket 信息 */
    __u32 daddr = BPF_CORE_READ(sk, __sk_common.skc_daddr);
    __u16 dport = BPF_CORE_READ(sk, __sk_common.skc_dport);
    
    bpf_printk("PID %d (uid=%d) connecting to %x:%d\n",
               pid, uid, bpf_ntohl(daddr), bpf_ntohs(dport));
    return 0;
}
```

---

<!-- chunk: 8. 最佳实践与常见问题 -->## 8. 最佳实践与常见问题

## 8.1 性能最佳实践 (Performance Best Practices)

```c
/* 最佳实践 1: 使用 Per-CPU Map 避免锁竞争 */

/* 不推荐: 全局 Hash Map (需要原子操作) */
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key, __u32);
    __type(value, __u64);
} global_counters SEC(".maps");

/* 推荐: Per-CPU Hash Map (无锁) */
struct {
    __uint(type, BPF_MAP_TYPE_PERCPU_HASH);
    __type(key, __u32);
    __type(value, __u64);
} percpu_counters SEC(".maps");

/* 最佳实践 2: 使用 Ring Buffer 替代 Perf Event Array (5.8+) */

/* 旧方式: Perf Event Array */
struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(__u32));
    __uint(value_size, sizeof(__u32));
} old_events SEC(".maps");

/* 推荐方式: Ring Buffer (内存效率更高) */
struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);
} ringbuf_events SEC(".maps");

/* 最佳实践 3: 使用 __always_inline 减少函数调用开销 */
static __always_inline int parse_ethhdr(struct xdp_md *ctx,
                                         struct ethhdr **eth,
                                         __u16 *proto) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    
    *eth = data;
    if ((void *)(*eth + 1) > data_end)
        return -1;
    
    *proto = bpf_ntohs((*eth)->h_proto);
    return 0;
}

/* 最佳实践 4: 早期返回，减少不必要的处理 */
SEC("xdp")
int fast_filter(struct xdp_md *ctx) {
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;
    
    struct ethhdr *eth = data;
    if ((void *)(eth + 1) > data_end)
        return XDP_DROP;  /* 早期返回 */
    
    /* 仅处理 IPv4 */
    if (eth->h_proto != bpf_htons(ETH_P_IP))
        return XDP_PASS;  /* 早期返回 */
    
    struct iphdr *ip = (void *)(eth + 1);
    if ((void *)(ip + 1) > data_end)
        return XDP_DROP;
    
    /* ... 继续处理 */
    return XDP_PASS;
}

/* 最佳实践 5: 使用 BPF_MAP_TYPE_ARRAY 替代 HASH (固定大小数据) */
/* Array 比 Hash 快 ~3-5倍，因为直接索引无需哈希计算 */
struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);  /* 而非 BPF_MAP_TYPE_HASH */
    __uint(max_entries, 256);          /* 按协议号索引 */
    __type(key, __u32);
    __type(value, __u64);
} proto_stats SEC(".maps");
```

## 8.2 调试技巧 (Debugging Tips)

```bash
# 调试技巧 1: 查看验证器日志
# 在 libbpf 中启用详细日志
cat > debug_load.c << 'EOF'
#include <bpf/libbpf.h>

int main() {
    LIBBPF_OPTS(bpf_object_open_opts, opts,
        .kernel_log_level = 1 | 2,  /* 详细验证器日志 */
    );
    
    struct bpf_object *obj = bpf_object__open_opts("my_prog.o", &opts);
    // ...
}
EOF

# 调试技巧 2: bpf_printk 输出
# eBPF 程序中使用 bpf_printk 打印调试信息
# 查看输出:
cat /sys/kernel/debug/tracing/trace_pipe
# 或
sudo cat /sys/kernel/tracing/trace_pipe

# 调试技巧 3: 使用 bpftrace 快速验证
# 一行式跟踪
bpftrace -e 'kprobe:do_sys_openat2 { printf("%s %s\n", comm, str(arg1)); }'
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { printf("%s -> %s\n", comm, str(args->filename)); }'
bpftrace -e 'xdp:* { @[probe] = count(); }'

# 调试技巧 4: 使用 bpftool 查看程序状态
bpftool prog list                     # 列出所有程序
bpftool prog show id <id> -p          # 详细信息 (JSON格式)
bpftool prog dump xlated id <id>      # 查看字节码 (带BTF注解)
bpftool prog dump jited id <id>       # 查看JIT代码
bpftool prog tracelog                  # 查看 bpf_printk 输出
bpftool prog profile id <id> duration 5 cycles instructions  # 性能分析

# 调试技巧 5: 检查程序运行统计
echo 1 > /proc/sys/kernel/bpf_stats_enabled
bpftool prog show id <id>
# 输出包含: run_cnt, run_time_ns

# 调试技巧 6: 使用 strace 跟踪 bpf() 系统调用
strace -e bpf ./my_bpf_loader

# 调试技巧 7: 检查 eBPF 错误日志
dmesg | grep -i bpf
journalctl -k | grep -i bpf
```

## 8.3 常见问题与解决方案 (Common Issues)

```
常见问题速查表
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

问题                              原因                   解决方案
────────────────────────────────  ────────────────────── ──────────────────────────────
验证失败: "R1 !read_ok"          使用了未初始化寄存器   初始化所有变量
验证失败: "invalid mem access"   指针边界检查失败       添加边界检查后再访问
验证失败: "back-edge from..."    存在循环               使用有界循环/pragma unroll
验证失败: "combined stack size"  栈使用超过 512 字节    使用 Per-CPU Map 作为堆
加载失败: EPERM                  权限不足               需要 CAP_BPF 或 CAP_SYS_ADMIN
加载失败: EINVAL                 程序类型不匹配          检查 SEC() 声明
程序被拒绝: "unknown func"       调用了不支持的 helper  检查内核版本和程序类型
Map 查找返回 NULL               key 不存在 (正常)      检查返回值，初始化默认值
XDP DROP 率高                   程序逻辑问题            使用 bpftrace 调试数据流
kprobe 无法附加                 函数被内联              使用 tracepoint 或 raw_tracepoint
BTF 信息缺失                    内核未编译 BTF          使用 CONFIG_DEBUG_INFO_BTF=y

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Kubernetes 中 eBPF 相关配置检查
# 检查节点内核版本
kubectl get nodes -o wide
kubectl debug node/<node-name> -it --image=ubuntu -- uname -r

# 检查 eBPF 特性支持
kubectl debug node/<node-name> -it --image=ubuntu -- bash -c "
  echo '=== Kernel Version ==='
  uname -r
  
  echo '=== BPF JIT ===' 
  cat /proc/sys/net/core/bpf_jit_enable
  
  echo '=== BTF Support ==='
  ls /sys/kernel/btf/vmlinux && echo 'BTF: YES' || echo 'BTF: NO'
  
  echo '=== eBPF Loaded Programs ==='
  bpftool prog list 2>/dev/null || echo 'bpftool not available'
  
  echo '=== Cilium eBPF Status ==='
  cilium status 2>/dev/null || echo 'cilium not available'
"

# 检查 Cilium eBPF 程序
kubectl -n kube-system exec -it ds/cilium -- cilium bpf endpoint list
kubectl -n kube-system exec -it ds/cilium -- cilium bpf policy list
kubectl -n kube-system exec -it ds/cilium -- cilium bpf nat list
```
## 8.4 安全注意事项 (Security Considerations)

```
eBPF 安全最佳实践
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

1. 权限控制
   • 使用 CAP_BPF (5.8+) 而非 CAP_SYS_ADMIN
   • 限制非特权 BPF: /proc/sys/kernel/unprivileged_bpf_disabled
   • 在容器中使用 securityContext 限制 BPF 权限

2. 内核版本
   • 维护内核安全更新，eBPF 验证器有已知漏洞
   • 使用 JIT hardening: /proc/sys/net/core/bpf_jit_harden = 2
   • 启用 CONFIG_BPF_JIT_ALWAYS_ON

3. 程序审计
   • 使用 bpftool prog show 审计已加载的 eBPF 程序
   • 实施 eBPF 程序签名验证
   • 使用 LSM BPF 钩子限制 bpf() 系统调用

4. Map 访问控制
   • 使用 BPF Token 控制 Map 创建权限
   • 限制 Map 大小防止 DoS
   • 定期审计 Map 内容

5. 供应链安全
   • 验证 eBPF 程序来源
   • 使用构建时签名
   • Cilium 使用镜像签名保证程序完整性
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

```yaml
# Kubernetes Pod 安全配置 - 允许 eBPF (仅必要权限)
apiVersion: v1
kind: Pod
metadata:
  name: ebpf-monitor
spec:
  containers:
  - name: monitor
    image: my-ebpf-monitor:latest
    securityContext:
      capabilities:
        add:
        - BPF           # CAP_BPF (Linux 5.8+)
        - PERFMON       # CAP_PERFMON (perf events)
        - SYS_RESOURCE  # 调整 rlimit (MAP_LOCKED)
        drop:
        - ALL
      readOnlyRootFilesystem: true
      runAsNonRoot: false  # eBPF 通常需要 root
      privileged: false    # 不需要完全特权
    volumeMounts:
    - name: bpf-fs
      mountPath: /sys/fs/bpf
    - name: debugfs
      mountPath: /sys/kernel/debug
      readOnly: true
  volumes:
  - name: bpf-fs
    hostPath:
      path: /sys/fs/bpf
      type: Directory
  - name: debugfs
    hostPath:
      path: /sys/kernel/debug
      type: Directory
  hostPID: true  # 访问所有进程信息 (仅监控场景)
  hostNetwork: true  # XDP/TC 程序可能需要
```

---

<!-- chunk: 📊 eBPF 程序类型速查表 -->## 📊 eBPF 程序类型速查表

| 程序类型 | 触发时机 | 上下文 | 返回值 | 主要用途 |
|----------|----------|--------|--------|----------|
| XDP | 网卡驱动收包 | `xdp_md` | XDP_DROP/PASS/TX/REDIRECT | DDoS防护、负载均衡 |
| TC (cls_bpf) | TC 入/出方向 | `__sk_buff` | TC_ACT_OK/SHOT/REDIRECT | 流量控制、NAT |
| kprobe | 内核函数入口 | `pt_regs` | 0 | 内核函数跟踪 |
| kretprobe | 内核函数返回 | `pt_regs` | 0 | 返回值跟踪 |
| fentry/fexit | 内核函数 (BTF) | 函数参数 | 0 | 高性能跟踪 (推荐) |
| tracepoint | 静态跟踪点 | 事件参数结构 | 0 | 内核事件跟踪 |
| raw_tracepoint | 原始跟踪点 | `bpf_raw_tracepoint_args` | 0 | 高性能事件跟踪 |
| LSM | 安全策略点 | 安全函数参数 | 0/错误码 | 运行时安全策略 |
| cgroup_skb | cgroup 数据包 | `__sk_buff` | 0/1 (丢弃/允许) | 容器网络控制 |
| cgroup_sock | socket 操作 | `bpf_sock` | 0/1 | Socket 控制 |
| socket_filter | Socket 过滤 | `__sk_buff` | 数据包长度/0 | 流量分析 |
| sk_msg | sendmsg 路径 | `sk_msg_md` | SK_PASS/DROP | 消息过滤/重定向 |
| sk_skb | recv skb | `__sk_buff` | SK_PASS/DROP | Socket Map 重定向 |
| perf_event | 性能事件 | `bpf_perf_event_data` | 0 | CPU分析、采样 |
| uprobe/uretprobe | 用户态函数 | `pt_regs` | 0 | 用户态跟踪 |
| USDT | 用户态静态探针 | 探针参数 | 0 | 应用观测 |

---

<!-- chunk: 🔗 相关资源 -->## 🔗 相关资源

- **内核文档**: [kernel.org/doc/html/latest/bpf](https://www.kernel.org/doc/html

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-35-ebpf-technology KUDIG Database — Global MOC
- [[domain-03-networking-traffic/README.md|[[Domain 35: eBPF 技术体系 (eBPF Technology Stack)|Domain 35: eBPF 技术体系 (eBPF Technology Stack)]]]]
- Domain-35 eBPF 技术 — 开源项目索引
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-03-networking-traffic/04-ebpf/01-ebpf-map-types-data-structures|02 ebpf map types data structures]]
- Cilium CNI 架构与部署 (Cilium CNI Architecture and Deployment)
- Cilium 网络策略 L3/L4/L7 (Cilium Network Policy L3/L4/L7)
- Cilium Service Mesh 无 Sidecar 架构 (Cilium Service Mesh Sideca...
- Tetragon 运行时安全 (Tetragon Runtime Security)
- Hubble 网络可观测性 (Hubble Network Observability)
- bcc 与 bpftrace 工具链 (bcc and bpftrace Tools)
- eBPF 性能优化实践 (eBPF Performance Optimization Practice)
- eBPF 安全应用案例 (eBPF Security Applications and Use Cases)

## See Also

- 09-ebpf-performance-optimization
- 10-ebpf-security-applications
- 02-ebpf-map-types-data-structures
- 03-cilium-cni-architecture


<!-- risk-assessed -->
