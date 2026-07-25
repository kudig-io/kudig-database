---
title: eBPF 可观测工具实战
description: 'bcc/bpftrace 工具集、Pixie 无侵入可观测、Parca 持续性能剖析与 Tetragon 安全观测'
summary: 'bcc/bpftrace 工具集、Pixie 无侵入可观测、Parca 持续性能剖析与 Tetragon 安全观测'
category: specialized-tech
tags:
- ebpf
- bcc
- bpftrace
- pixie
- parca
- tetragon
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
- eBPF 可观测工具是什么
- 如何使用 bpftrace 进行系统分析
- Pixie 是什么
trigger_keywords:
- ebpf
- bcc
- bpftrace
- pixie
- parca
- tetragon
- observability
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


# eBPF 可观测工具实战

## 1. bcc 工具集

### 1.1 安装

```bash
# Ubuntu/Debian
sudo apt-get install bpfcc-tools linux-headers-$(uname -r)

# RHEL/CentOS
sudo yum install bcc-tools bcc-doc

# 二进制路径
/usr/share/bcc/tools/
```

### 1.2 进程与系统调用分析

```bash
# 跟踪所有 execve 调用
sudo /usr/share/bcc/tools/execsnoop

# 跟踪特定进程的系统调用（替代 strace）
sudo /usr/share/bcc/tools/opensnoop -p 1234

# 跟踪文件打开
sudo /usr/share/bcc/tools/opensnoop -n nginx

# 跟踪所有系统调用延迟
sudo /usr/share/bcc/tools/syscount -d 5

# 统计系统调用（替代 strace -c）
sudo /usr/share/bcc/tools/syscount -p 1234
```

### 1.3 CPU 分析

```bash
# CPU 使用火焰图数据采集
sudo /usr/share/bcc/tools/profile -F 99 -f 10 > profile.stacks

# 调度延迟分析
sudo /usr/share/bcc/tools/runqlat

# 每 CPU 调度延迟直方图
sudo /usr/share/bcc/tools/runqlat -C

# CPU 迁移跟踪
sudo /usr/share/bcc/tools/migrate
```

### 1.4 I/O 分析

```bash
# 块 I/O 延迟分析
sudo /usr/share/bcc/tools/biolatency

# 块 I/O 大小分布
sudo /usr/share/bcc/tools/biosize

# 文件系统 I/O 延迟
sudo /usr/share/bcc/tools/fslatency

# 统计每个进程的 I/O
sudo /usr/share/bcc/tools/biotop

# 跟踪 VFS 操作
sudo /usr/share/bcc/tools/vfsstat
```

### 1.5 网络分析

```bash
# TCP 连接跟踪
sudo /usr/share/bcc/tools/tcplife

# TCP 重传跟踪
sudo /usr/share/bcc/tools/tcpretrans

# TCP 接收窗口收缩
sudo /usr/share/bcc/tools/tcprcvbuf

# DNS 查询跟踪
sudo /usr/share/bcc/tools/dnssnoop

# 套接字生命周期
sudo /usr/share/bcc/tools/socklife
```

## 2. bpftrace 工具

### 2.1 安装与基础

```bash
# Ubuntu/Debian
sudo apt-get install bpftrace

# RHEL/CentOS
sudo yum install bpftrace

# 版本检查
bpftrace --version
```

### 2.2 单行脚本

```bash
# 跟踪系统调用入口（类似 strace）
bpftrace -e 'tracepoint:syscalls:sys_enter_* { @[probe] = count(); }'

# 统计每秒系统调用次数
bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @calls = count(); } interval:s:1 { print(@calls); clear(@calls); }'

# 跟踪 open 系统调用
bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'

# 延迟直方图
bpftrace -e 'kprobe:do_sys_open { @start[tid] = nsecs; } kretprobe:do_sys_open /@start[tid]/ { @ns = hist(nsecs - @start[tid]); delete(@start[tid]); }'
```

### 2.3 实用脚本

```bash
#!/usr/bin/env bpftrace
// tcpconnect.bt - 跟踪 TCP 连接
kprobe:tcp_connect
{
    $sk = (struct sock *)arg0;
    $daddr = ntop($sk->__sk_common.skc_daddr);
    printf("PID: %d COMM: %s -> %s:%d\n",
           pid, comm, $daddr,
           $sk->__sk_common.skc_dport);
}
```

```bash
#!/usr/bin/env bpftrace
// runqlat.bt - 调度延迟分布
tracepoint:sched:sched_wakeup
{
    @qtime[args->pid] = nsecs;
}

tracepoint:sched:sched_switch
/args->prev_state == TASK_RUNNING/
{
    $pid = args->prev_pid;
    if (@qtime[$pid]) {
        @usecs = hist((nsecs - @qtime[$pid]) / 1000);
        delete(@qtime[$pid]);
    }
}
```

### 2.4 Kubernetes 容器级分析

```bash
# 跟踪特定容器的系统调用
bpftrace -e '
tracepoint:syscalls:sys_enter_write
/cgroup == 0x100001/    // 容器 cgroup ID
{
    printf("container-pid=%d comm=%s fd=%d\n", pid, comm, args->fd);
}
'

# 按容器统计 CPU 使用
bpftrace -e '
profile:hz:99
{
    @cpu[cgroup] = count();
}
'
```

## 3. Pixie 无侵入可观测

### 3.1 架构

```
Pixie Edge Module (PEM) → Vizier (数据层) → Cloud / 自托管
    │
    └── 自动采集：HTTP/gRPC/MySQL/Postgres/Kafka/DNS/进程
```

### 3.2 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Helm 安装
helm repo add pixie https://pixie-operator-charts.storage.googleapis.com
helm repo update

helm install pixie pixie/pixie-operator-chart \
  --namespace pl \
  --create-namespace \
  --set clusterName=my-cluster \
  --set deployKey=<your-deploy-key>

# 或使用 px CLI
px deploy
```
### 3.3 PxL 查询语言

```python
# 查询 HTTP 请求延迟
import px

df = px.DataFrame(table='http_events', start_time='-5m')
df = df[['time_', 'source_addr', 'destination_addr',
         'req_method', 'req_path', 'resp_status', 'resp_latency_ns']]
df.resp_latency_ms = df.resp_latency_ns / 1e6
px.display(df, 'http_requests')
```

```python
# 按服务统计错误率
import px

df = px.DataFrame(table='http_events', start_time='-15m')
df = df.groupby(['source_service', 'destination_service']).agg(
    total=('resp_status', 'count'),
    errors=('resp_status', lambda x: (x >= 400).sum())
)
df.error_rate = df.errors / df.total * 100
px.display(df[df.error_rate > 1], 'error_services')
```

```python
# 按 Pod 统计 CPU 使用
import px

df = px.DataFrame(table='cpu_cycles', start_time='-5m')
df = df.groupby(['upid']).agg(cycles=('cpu_cycles', 'sum'))
px.display(df, 'pod_cpu')
```

### 3.4 自动采集协议

| 协议 | 采集内容 | 端口检测 |
|------|----------|----------|
| HTTP/1.1 | 请求/响应、延迟、状态码 | 80, 8080, 443 |
| HTTP/2 | gRPC 方法、延迟、状态码 | 动态检测 |
| MySQL | 查询、延迟、错误 | 3306 |
| PostgreSQL | 查询、延迟、错误 | 5432 |
| Kafka | 消息、延迟、Topic | 9092 |
| DNS | 查询、响应、延迟 | 53 |
| Redis | 命令、延迟 | 6379 |

## 4. Parca 持续性能剖析

### 4.1 架构

```
Parca Agent → eBPF 采集（CPU profiling） → Parca Server → Web UI
    │
    └── 支持：Go, Rust, C/C++, Python, Java, Node.js
```

### 4.2 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Helm 安装
helm repo add parca https://parca-dev.github.io/helm-charts
helm repo update

helm install parca parca/parca \
  --namespace parca \
  --create-namespace \
  --set server.enabled=true

# Agent DaemonSet
helm install parca-agent parca/parca-agent \
  --namespace parca \
  --create-namespace
```
### 4.3 使用场景

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 访问 Web UI
kubectl port-forward svc/parca 7070:7070 -n parca

# API 查询
curl "http://localhost:7070/query?query=cpu%3Asamples%3Acount%3Acpu%3Ananoseconds%3Arate%3A5m&time=$(date +%s)"
```
持续 Profiling 优势：

| 传统 Profiling | Parca 持续 Profiling |
|----------------|---------------------|
| 手动触发 | 自动持续采集 |
| 单一时间点 | 全时间线覆盖 |
| 需要应用配合 | 无侵入（eBPF） |
| 生产环境风险 | 生产安全 |

## 5. Tetragon 安全观测

### 5.1 架构

```
Tetragon Agent → eBPF 内核传感器 → TracingPolicy → 事件/动作
    │
    └── 支持：进程、文件、网络、安全事件
```

### 5.2 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Helm 安装
helm repo add cilium https://helm.cilium.io/
helm repo update

helm install tetragon cilium/tetragon \
  --namespace kube-system \
  --set tetragonOperator.image.repository=cilium/tetragon-operator \
  --set tetragon.image.repository=cilium/tetragon

# 查看事件
kubectl logs -n kube-system ds/tetragon -f
```
### 5.3 TracingPolicy 示例

```yaml
# 跟踪敏感文件访问
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: sensitive-file-access
spec:
  kprobes:
    - call: "fd_install"
      syscall: false
      args:
        - index: 0
          type: int
        - index: 1
          type: "file"
      selectors:
        - matchArgs:
            - index: 1
              operator: "Prefix"
              values:
                - "/etc/shadow"
                - "/etc/passwd"
                - "/var/run/secrets"
```

```yaml
# 跟踪进程执行
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: process-execution
spec:
  kprobes:
    - call: "security_bprm_check"
      syscall: false
      args:
        - index: 0
          type: "file"
      selectors:
        - matchBinaries:
            - operator: "NotIn"
              values:
                - "/usr/bin/kubectl"
                - "/bin/bash"
```

```yaml
# 跟踪网络连接
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: network-connections
spec:
  kprobes:
    - call: "tcp_connect"
      syscall: false
      args:
        - index: 0
          type: "sock"
      selectors:
        - matchActions:
            - action: FollowFD
```

### 5.4 安全响应动作

```yaml
# 自动生成安全事件告警
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: security-alerts
spec:
  kprobes:
    - call: "security_bprm_check"
      syscall: false
      args:
        - index: 0
          type: "file"
      selectors:
        - matchActions:
            - action: Sigkill    # 终止进程
              rateLimit: "1/m"
            - action: Override   # 覆盖返回值
              argError: -1       # 返回 EPERM
```

## 6. 工具对比与选型

| 工具 | 主要用途 | 侵入性 | 性能影响 | 适用场景 |
|------|----------|--------|----------|----------|
| **bcc** | 系统调用分析 | 低 | <1% | 开发调试、深度分析 |
| **bpftrace** | 快速原型 | 低 | <1% | 临时排查、快速验证 |
| **Pixie** | 应用可观测 | 无 | 2-5% | K8s 全栈可观测 |
| **Parca** | 性能剖析 | 无 | <0.5% | 持续 CPU 分析 |
| **Tetragon** | 安全观测 | 低 | 1-3% | 运行时安全 |

---

## Related

- [[05-网络/05-eBPF/11-ebpf-programming-fundamentals|eBPF 开发基础]]
- [[05-网络/05-eBPF/13-ebpf-networking-applications|eBPF 网络应用]]
- [[05-网络/05-eBPF/14-ebpf-security-runtime|eBPF 安全运行时]]

## See Also

- [bcc 工具集](https://github.com/iovisor/bcc)
- [bpftrace 文档](https://github.com/bpftrace/bpftrace)
- [Pixie](https://px.dev/)
- [Parca](https://www.parca.dev/)
- [Tetragon](https://tetragon.io/)


<!-- risk-assessed -->
