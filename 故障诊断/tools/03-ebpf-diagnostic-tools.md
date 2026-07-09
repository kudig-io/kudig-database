---
title: eBPF 诊断工具实战指南
description: 面向阿里云/专有云 K8s 的 eBPF 诊断工具指南，涵盖 bcc、bpftrace、Pixie、Inspektor Gadget 的安装、使用与性能排查场景。
summary: 面向阿里云/专有云 K8s 的 eBPF 诊断工具指南，涵盖 bcc、bpftrace、Pixie、Inspektor Gadget 的安装、使用与性能排查场景。
category: troubleshooting
tags:
- k8s
- ebpf
- bcc
- bpftrace
- pixie
- inspektor-gadget
- performance
- diagnostics
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 性能工程师
- 运维工程师
estimated_read_time: 25min
intent_queries:
- eBPF 诊断工具
- bcc bpftrace K8s
- Pixie Inspektor Gadget 使用
trigger_keywords:
- eBPF
- bcc
- bpftrace
- Pixie
- Inspektor Gadget
- 性能诊断
prerequisites:
- kubectl-basics
- linux-basics
- kernel-basics
- performance-basics
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




# eBPF 诊断工具实战指南

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，讲解 eBPF 诊断工具 bcc、bpftrace、Pixie、Inspektor Gadget 的使用方法。

## 目录

1. [eBPF 概述](#ebpf-概述)
2. [bcc 工具集](#bcc-工具集)
3. [bpftrace](#bpftrace)
4. [Pixie](#pixie)
5. [Inspektor Gadget](#inspektor-gadget)
6. [典型诊断场景](#典型诊断场景)
7. [性能与安全注意事项](#性能与安全注意事项)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. eBPF 概述

### 1.1 什么是 eBPF

eBPF（Extended Berkeley Packet Filter）允许在内核中安全地运行沙箱程序，无需修改内核源码或加载内核模块。

### 1.2 eBPF 在 K8s 诊断中的优势

| 优势 | 说明 |
|:---|:---|
| 低开销 | 内核态执行，减少数据拷贝 |
| 全栈可见 | 从系统调用到网络包均可观测 |
| 安全 | 经过 verifier 验证 |
| 动态 | 无需重启即可加载程序 |

---

## 2. bcc 工具集

### 2.1 安装 bcc

```bash
# Ubuntu
apt-get install bpfcc-tools linux-headers-$(uname -r)

# CentOS
yum install bcc-tools kernel-devel-$(uname -r)
```

### 2.2 常用 bcc 工具

| 工具 | 用途 |
|:---|:---|
| execsnoop | 追踪进程执行 |
| opensnoop | 追踪文件打开 |
| biolatency | 块设备 IO 延迟 |
| biosnoop | 块设备 IO 详情 |
| tcpconnect | 追踪 TCP 连接 |
| tcpaccept | 追踪 TCP 监听 |
| runqlat | CPU 调度延迟 |
| profile | CPU 火焰图 |

### 2.3 在 K8s 节点上使用

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 进入节点执行
kubectl node-shell <node-name>

# 追踪新启动的进程
execsnoop

# 查看磁盘 IO 延迟分布
biolatency

# 抓取 30 秒 CPU 火焰图
profile -af 30 > /tmp/profile.out
```
---

## 3. bpftrace

### 3.1 安装 bpftrace

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Ubuntu
apt-get install bpftrace

# 容器方式
kubectl run bpftrace --rm -it --privileged --image=quay.io/iovisor/bpftrace -- bpftrace -l
```
### 3.2 常用脚本

```bash
# 追踪文件打开
bpftrace -e 'kprobe:do_sys_open { printf("%s: %s\n", comm, str(arg1)); }'

# 统计 TCP 重传
bpftrace -e 'kprobe:tcp_retransmit { @[saddr, daddr] = count(); }'

# 查看进程启动
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { printf("%s: %s\n", comm, str(args->filename)); }'
```

### 3.3 结合 kubectl-trace

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在指定 Pod 上运行 bpftrace 脚本
kubectl trace run <pod-name> -n <namespace> \
  -e 'kprobe:do_sys_open { printf("%s: %s\n", comm, str(arg1)); }'
```
---

## 4. Pixie

### 4.1 部署 Pixie

```bash
# 安装 Pixie CLI
bash -c "$(curl -fsSL https://withpixie.ai/install.sh)"

# 部署到集群
px deploy
```

### 4.2 常用 Pixie 脚本

```bash
# 查看 HTTP 流量
px scripts list | grep http
px run px/http_data

# 查看 MySQL 查询
px run px/mysql_data

# 查看 Pod CPU 火焰图
px run px/perf_flamegraph
```

### 4.3 Pixie 优势

- 自动协议解析（HTTP、gRPC、MySQL、Redis 等）
- 无需手动编写 eBPF 程序
- 提供可视化界面

---

## 5. Inspektor Gadget

### 5.1 部署 Inspektor Gadget

```bash
# 安装 CLI
curl -sL https://github.com/inspektor-gadget/inspektor-gadget/releases/latest/download/ig-linux-amd64.tar.gz | \
  tar xvz -C /usr/local/bin

# 部署到集群
ig deploy
```

### 5.2 常用 Gadget

```bash
# 追踪 exec 事件
ig trace exec -n production

# 追踪网络
ig trace dns -n production

# 追踪 TCP 连接
ig trace tcp -n production

# 查看 OOM 事件
ig snapshot process -n production
```

### 5.3 与 kubectl 集成

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 作为 kubectl 插件使用
kubectl gadget trace exec -n production
```
---

## 6. 典型诊断场景

### 6.1 高 CPU 排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 使用 bcc profile 抓取火焰图
kubectl node-shell <node-name>
profile -af 60 > /tmp/cpu.flamegraph

# 2. 使用 Pixie 查看 CPU 火焰图
px run px/perf_flamegraph --start_time -5m
```
### 6.2 高延迟排查

```bash
# 1. 使用 bpftrace 追踪系统调用延迟
bpftrace -e 'kprobe:do_sys_open { @start[tid] = nsecs; }
             kretprobe:do_sys_open /@start[tid]/ {
               @latency_us = hist((nsecs - @start[tid]) / 1000);
               delete(@start[tid]);
             }'

# 2. 使用 Pixie 查看服务间延迟
px run px/http_latency
```

### 6.3 网络问题排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 Inspektor Gadget 追踪 TCP 重传
ig trace tcp -n production --drop

# 使用 bcc tcpretrans
kubectl node-shell <node-name>
tcpretrans
```
---

## 7. 性能与安全注意事项

### 7.1 性能开销

| 工具 | 开销 | 建议 |
|:---|:---|:---|
| bcc | 中 | 短时运行，生产谨慎 |
| bpftrace | 中 | 脚本优化后再上生产 |
| Pixie | 低-中 | 长期运行需关注资源 |
| Inspektor Gadget | 低 | 适合持续监控 |

### 7.2 安全注意

- eBPF 程序需要 CAP_BPF 或 root 权限
- 生产环境使用需严格控制权限
- 阿里云/专有云需确认内核版本支持 eBPF

---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| 内核版本支持 | 4.19+ | `uname -r` |
| eBPF 工具安装 | 至少一种工具可用 | 命令测试 |
| 权限控制 | 限制 eBPF 权限 | RBAC/CAP |
| 性能基线 | 开销 < 5% | 监控 |
| 脚本审核 | 生产脚本需评审 | 代码仓库 |

---

## eBPF 安全风险与限制

eBPF 程序运行在内核态，具有较高权限，必须控制加载来源与范围。

| 风险 | 说明 | 缓解措施 |
|:---|:---|:---|
| 内核崩溃 | 错误的 eBPF 程序可能导致 panic | 使用成熟工具，测试后再上生产 |
| 数据泄露 | eBPF 可读取任意进程内存 | 限制执行权限与审计 |
| 性能开销 | 高频探针可能影响吞吐 | 采样与按需启用 |
| 内核版本依赖 | 部分特性需较新内核 | 确认内核 >= 4.19 |

### 性能基线测试

在启用 eBPF 监控前，先在测试环境测量基线性能：

```bash
# 使用 bpftrace 跟踪 CPU 调度，评估开销
bpftrace -e 'kprobe:finish_task_switch { @ = count(); } interval:s:1 { print(@); clear(@); }'
```

### eBPF 与可观测性平台集成

将 eBPF 采集的指标统一接入 Prometheus/Grafana，形成网络、安全、性能一体化视图。

| 数据源 | 指标示例 |
|:---|:---|
| Pixie | HTTP 延迟、数据库查询、资源使用 |
| Tetragon | 进程启动、网络连接、文件访问 |
| bcc | TCP 重传、IO 延迟、CPU 剖析 |

## eBPF 工具选型建议

| 场景 | 首选工具 | 说明 |
|:---|:---|:---|
| 快速查看集群事件 | inspektor-gadget | K8s 原生，按 Pod/Namespace 过滤 |
| 自动可观测性 | Pixie | 无需埋点，自动采集 |
| 深度性能剖析 | bcc profile / offcputime | 定位 CPU/IO 热点 |
| 安全事件检测 | Tetragon / bpftrace | 实时阻断与取证 |
| 教学与脚本化 | bpftrace | 语法简单，便于定制 |

### 生产使用注意事项

1. 先在测试环境验证 eBPF 程序稳定性。
2. 限制 eBPF 工具的部署范围，避免全集群同时启用。
3. 对高频率探针进行采样，降低性能开销。
4. 记录所有 eBPF 程序的来源与用途，便于审计。

## 典型工单场景与处理

**场景**：某服务 CPU 突然升高，但应用日志无异常。

处理步骤：
1. 使用 ktop 定位高 CPU Pod。
2. 使用 bcc profile 抓取 CPU 火焰图。
3. 分析火焰图找出热点函数。
4. 结合代码或配置定位根因并修复。

## eBPF 性能基线与测试

在启用 eBPF 监控前，建议建立性能基线并评估开销。

### 基线测试步骤

1. 在未启用 eBPF 时记录应用的 QPS、延迟、CPU 使用率。
2. 启用 eBPF 探针后再次测量相同指标。
3. 计算性能偏差，确认是否在可接受范围（通常 < 5%）。
4. 逐步增加探针数量，观察边际开销。

### 常用 bcc 性能工具

| 工具 | 用途 |
|:---|:---|
| `profile` | CPU 火焰图 |
| `offcputime` |  off-CPU 时间分析 |
| `biolatency` | 块设备 IO 延迟 |
| `tcpconnect` | TCP 连接跟踪 |
| `tcpretrans` | TCP 重传分析 |
| `runqlat` | CPU 调度延迟 |

### eBPF 安全审计

- 记录所有加载的 eBPF 程序及其来源。
- 限制 eBPF 程序加载权限，仅允许可信镜像。
- 定期审查 eBPF 程序是否被篡改。

## eBPF 与现有监控互补

eBPF 不是用来替代 Prometheus/日志/追踪，而是作为补充，提供更低开销、更细粒度的洞察。

| 现有监控 | eBPF 补充能力 |
|:---|:---|
| Prometheus 指标 | 内核级 CPU/IO/网络事件 |
| 日志 | 无需修改应用即可采集系统调用 |
| Tracing | 无侵入地追踪请求在内核中的路径 |
| 安全审计 | 实时监控异常进程与网络行为 |

### eBPF 实施路线图

1. 阶段 1：部署 Pixie 或 inspektor-gadget，获取开箱即用能力。
2. 阶段 2：针对高频问题编写 bpftrace 脚本。
3. 阶段 3：将 eBPF 事件接入 Prometheus/Grafana。
4. 阶段 4：建立 eBPF 安全规则与自动化响应。

## eBPF 诊断实战：高延迟根因分析

以某服务 P99 延迟突增为例：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 使用 bcc 抓取 CPU 火焰图
kubectl run bcc-profile --rm -it --restart=Never   --image=quay.io/iovisor/bcc:latest   --overrides='{"spec":{"nodeName":"node-1","hostPID":true}}'   -- /usr/share/bcc/tools/profile -F 99 -p <pid> 30

# 2. 使用 inspektor-gadget 查看 TCP 连接延迟
kubectl gadget trace tcpconnect -n production -p <pod-name>

# 3. 使用 bpftrace 跟踪慢系统调用
bpftrace -e 'kprobe:tcp_sendmsg /pid == <pid>/ { @start[tid] = nsecs; } kretprobe:tcp_sendmsg /@start[tid]/ { @latency = hist((nsecs - @start[tid]) / 1000); delete(@start[tid]); }'
```
### eBPF 资源限制

- 单节点同时加载的 eBPF 程序数量有限。
- 内核 map 大小受内存限制。
- 部分旧内核不支持高级 eBPF 特性。
- 生产环境启用前务必进行兼容性测试。

## Related

- [[故障诊断/tools/README.md|Domain-12 故障排查工具套件使用说明]]
- [[故障诊断/02-infrastructure-troubleshooting/33-performance-bottleneck-troubleshooting.md|性能瓶颈故障诊断]]

## See Also

- [[故障诊断/tools/01-kubectl-plugins-guide.md|kubectl 插件指南]]
- [[故障诊断/tools/02-network-diagnostic-tools.md|网络诊断工具]]


<!-- risk-assessed -->
