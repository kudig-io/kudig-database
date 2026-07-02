---
title: eBPF 安全运行时
description: 'Tetragon 安全策略、Falco eBPF 驱动、KRSI 与 eBPF 审计策略实战'
summary: 'Tetragon 安全策略、Falco eBPF 驱动、KRSI 与 eBPF 审计策略实战'
category: specialized-tech
tags:
- ebpf
- tetragon
- falco
- krsi
- security
- runtime-security
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
- Tetragon 安全策略是什么
- 如何使用 Falco eBPF 进行运行时安全
- KRSI 是什么
trigger_keywords:
- tetragon
- falco
- krsi
- ebpf
- runtime-security
- audit
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


# eBPF 安全运行时

## 1. 运行时安全架构

```
内核事件 → eBPF 传感器 → 策略引擎 → 响应动作
    │              │            │          │
    │              │            │          └── 告警/阻断/记录
    │              │            └── TracingPolicy
    │              └── 进程/文件/网络/安全
    └── kprobe/tracepoint/LSM
```

eBPF 安全运行时优势：

| 特性 | 传统方案 | eBPF 方案 |
|------|----------|-----------|
| 内核模块 | 需要 | 不需要 |
| 性能影响 | 高 | 低（<3%） |
| 策略灵活性 | 固定 | 动态可编程 |
| 容器感知 | 有限 | 完全支持 |

## 2. Tetragon 安全策略

### 2.1 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Helm 安装
helm repo add cilium https://helm.cilium.io/
helm repo update

helm install tetragon cilium/tetragon \
  --namespace kube-system \
  --set tetragonOperator.image.repository=cilium/tetragon-operator \
  --set tetragon.image.repository=cilium/tetragon \
  --set tetragon.enableProcessCredScanning=true \
  --set tetragon.enableProcessNsScanning=true

# 查看事件
kubectl logs -n kube-system ds/tetragon -f
```
### 2.2 进程执行监控

```yaml
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: process-exec-monitor
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
                - "/usr/bin/coreutils"
        - matchActions:
            - action: FollowFD
    - call: "security_file_open"
      syscall: false
      args:
        - index: 0
          type: "file"
      selectors:
        - matchArgs:
            - index: 0
              operator: "Prefix"
              values:
                - "/etc/shadow"
                - "/etc/passwd"
                - "/root/.ssh"
```

### 2.3 敏感文件访问控制

```yaml
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: sensitive-file-protection
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
                - "/etc/gshadow"
                - "/etc/sudoers"
                - "/var/run/secrets/kubernetes.io"
        - matchBinaries:
            - operator: "NotIn"
              values:
                - "/usr/bin/sudo"
                - "/usr/bin/passwd"
        - matchNamespaces:
            - operator: "In"
              values:
                - "init_mnt"
        - matchActions:
            - action: Override
              argError: -13    # EACCES
```

### 2.4 网络连接监控

```yaml
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: network-connection-monitor
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
    - call: "tcp_close"
      syscall: false
      args:
        - index: 0
          type: "sock"
      selectors:
        - matchActions:
            - action: FollowFD
```

### 2.5 Signal/Override/FollowFD 动作

```yaml
# Signal - 发送信号终止进程
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: block-malicious-process
spec:
  kprobes:
    - call: "security_bprm_check"
      syscall: false
      args:
        - index: 0
          type: "file"
      selectors:
        - matchBinaries:
            - operator: "In"
              values:
                - "/tmp/*"
                - "/dev/shm/*"
        - matchActions:
            - action: Sigkill
              rateLimit: "10/m"
---
# Override - 覆盖系统调用返回值
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: block-unauthorized-mount
spec:
  kprobes:
    - call: "__x64_sys_mount"
      syscall: true
      args:
        - index: 0
          type: "string"
      selectors:
        - matchArgs:
            - index: 0
              operator: "Prefix"
              values:
                - "/proc"
                - "/sys"
        - matchActions:
            - action: Override
              argError: -1    # EPERM
```

### 2.6 安全策略组合

```yaml
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: comprehensive-security
spec:
  kprobes:
    # 进程执行
    - call: "security_bprm_check"
      syscall: false
      args:
        - index: 0
          type: "file"
      selectors:
        - matchBinaries:
            - operator: "NotIn"
              values:
                - "/usr/bin/*"
                - "/usr/sbin/*"
        - matchActions:
            - action: Sigkill
              rateLimit: "5/m"
    # 敏感文件写入
    - call: "security_file_open"
      syscall: false
      args:
        - index: 0
          type: "file"
        - index: 1
          type: "int"
      selectors:
        - matchArgs:
            - index: 0
              operator: "Prefix"
              values:
                - "/etc"
                - "/root"
            - index: 1
              operator: "Equal"
              values:
                - "2"    # O_WRONLY
        - matchActions:
            - action: Override
              argError: -13
```

## 3. Falco eBPF 驱动

### 3.1 安装

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Helm 安装
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm repo update

helm install falco falcosecurity/falco \
  --namespace falco \
  --create-namespace \
  --set driver.kind=ebpf \
  --set falcosidekick.enabled=true \
  --set falcosidekick.config.slack.webhookurl="https://hooks.slack.com/..."
```
### 3.2 Falco 规则

```yaml
# /etc/falco/falco_rules.yaml
- rule: Unauthorized Process in Sensitive Container
  desc: Detect process execution in sensitive containers
  condition: >
    spawned_process and container and
    container.image.repository in (database, redis, etcd) and
    not proc.name in (postgres, mysqld, redis-server, etcd)
  output: >
    Unauthorized process in sensitive container
    (user=%user.name command=%proc.cmdline container=%container.id
     image=%container.image.repository)
  priority: CRITICAL
  tags: [container, process, mitre_execution]

- rule: Sensitive File Access
  desc: Detect access to sensitive files
  condition: >
    open_read and container and
    fd.name in (/etc/shadow, /etc/passwd, /etc/kubernetes) and
    not proc.name in (sshd, sudo)
  output: >
    Sensitive file access detected
    (user=%user.name command=%proc.cmdline file=%fd.name
     container=%container.id image=%container.image.repository)
  priority: WARNING
  tags: [filesystem, mitre_credential_access]

- rule: Network Connection to Known Malicious IP
  desc: Detect outbound connections to known malicious IPs
  condition: >
    outbound and container and
    fd.sip in (malicious_ips)
  output: >
    Connection to known malicious IP
    (command=%proc.cmdline connection=%fd.name
     container=%container.id image=%container.image.repository)
  priority: CRITICAL
  tags: [network, mitre_command_and_control]
```

### 3.3 自定义 Falco 规则

```yaml
# 检测容器逃逸尝试
- rule: Container Escape Attempt
  desc: Detect potential container escape
  condition: >
    spawned_process and container and
    (proc.name in (nsenter, mount, chroot) or
     proc.cmdline contains "/proc/self/exe" or
     proc.cmdline contains "/proc/1/ns")
  output: >
    Container escape attempt detected
    (user=%user.name command=%proc.cmdline container=%container.id
     image=%container.image.repository k8s.pod=%k8s.pod.name)
  priority: CRITICAL
  tags: [container, escape, mitre_privilege_escalation]

# 检测加密挖矿
- rule: Cryptocurrency Mining Detection
  desc: Detect cryptocurrency mining activity
  condition: >
    spawned_process and container and
    (proc.name in (xmrig, minerd, cpuminer) or
     proc.cmdline contains "stratum+tcp" or
     proc.cmdline contains "nicehash")
  output: >
    Cryptocurrency mining detected
    (command=%proc.cmdline container=%container.id
     image=%container.image.repository k8s.pod=%k8s.pod.name)
  priority: CRITICAL
  tags: [container, mining, mitre_execution]
```

## 4. KRSI（Kernel Runtime Security Instrumentation）

### 4.1 概述

KRSI 是 Linux 内核的安全框架，基于 LSM（Linux Security Module）：

```c
// KRSI eBPF 程序挂载点
SEC("lsm/file_open")
int BPF_PROG(file_open_audit, struct file *file, int ret) {
    // 审计文件访问
    return ret;
}

SEC("lsm/bprm_creds_for_exec")
int BPF_PROG(exec_audit, struct linux_binprm *bprm, int ret) {
    // 审计进程执行
    return ret;
}

SEC("lsm/socket_connect")
int BPF_PROG(connect_audit, struct socket *sock, struct sockaddr *address,
             int addrlen, int ret) {
    // 审计网络连接
    return ret;
}
```

### 4.2 与 Tetragon 集成

```yaml
# Tetragon 使用 KRSI LSM hook
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: lsm-file-protection
spec:
  lsmHooks:
    - hook: "file_open"
      args:
        - index: 0
          type: "file"
      selectors:
        - matchArgs:
            - index: 0
              operator: "Prefix"
              values:
                - "/etc/kubernetes"
                - "/var/run/secrets"
        - matchActions:
            - action: Override
              argError: -13
```

## 5. eBPF 审计策略

### 5.1 系统调用审计

```c
// audit_syscalls.bpf.c
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>

struct audit_event {
    u32 pid;
    u32 uid;
    u64 ts;
    char comm[16];
    char syscall[32];
    u64 args[6];
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} audit_events SEC(".maps");

SEC("tracepoint/raw_syscalls/sys_enter")
int audit_syscall(struct trace_event_raw_sys_enter *ctx) {
    struct audit_event *evt;
    u64 pid_tgid = bpf_get_current_pid_tgid();

    evt = bpf_ringbuf_reserve(&audit_events, sizeof(*evt), 0);
    if (!evt)
        return 0;

    evt->pid = pid_tgid >> 32;
    evt->uid = bpf_get_current_uid_gid();
    evt->ts = bpf_ktime_get_ns();
    bpf_get_current_comm(&evt->comm, sizeof(evt->comm));
    evt->syscall[0] = ctx->id;

    bpf_ringbuf_submit(evt, 0);
    return 0;
}
```

### 5.2 Kubernetes 审计集成

```yaml
# 审计策略配置（与 eBPF 互补）
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 审计所有写操作
  - level: RequestResponse
    resources:
      - group: ""
        resources: ["secrets", "configmaps"]
      - group: "apps"
        resources: ["deployments", "statefulsets"]
    verbs: ["create", "update", "patch", "delete"]

  # 审计认证事件
  - level: Metadata
    resources:
      - group: "authentication.k8s.io"
        resources: ["tokenreviews", "subjectaccessreviews"]
```

### 5.3 综合审计架构

```
内核事件 → eBPF 审计 → Tetragon/Falco → SIEM/SOAR
    │              │           │
    │              │           └── 告警/响应
    │              └── 策略过滤
    └── 系统调用/文件/网络/进程

K8s API 审计 → Audit Sink → Webhook → 后端存储
```

## 6. 安全响应与自动化

### 6.1 自动阻断

```yaml
# Tetragon 自动阻断恶意进程
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: auto-block
spec:
  kprobes:
    - call: "security_bprm_check"
      syscall: false
      args:
        - index: 0
          type: "file"
      selectors:
        - matchBinaries:
            - operator: "In"
              values:
                - "/tmp/*"
                - "/dev/shm/*"
                - "/var/tmp/*"
        - matchActions:
            - action: Sigkill
              rateLimit: "5/m"
```

### 6.2 与 Falco Talon 集成

```yaml
# Falco Talon 自动响应
apiVersion: talon.falco.org/v1alpha1
kind: ResponseRule
metadata:
  name: block-crypto-mining
spec:
  match:
    rule: "Cryptocurrency Mining Detection"
    priority: "CRITICAL"
  actions:
    - type: kill
      parameters:
        signal: "SIGKILL"
    - type: label
      parameters:
        labels:
          "security.falco.org/blocked": "true"
```

## 7. 安全策略模板

### 7.1 基线安全策略

```yaml
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: baseline-security
spec:
  kprobes:
    # 禁止特权容器中的危险操作
    - call: "__x64_sys_mount"
      syscall: true
      args:
        - index: 0
          type: "string"
      selectors:
        - matchNamespaces:
            - operator: "In"
              values:
                - "init_mnt"
        - matchActions:
            - action: Override
              argError: -1
    # 监控敏感文件访问
    - call: "security_file_open"
      syscall: false
      args:
        - index: 0
          type: "file"
      selectors:
        - matchArgs:
            - index: 0
              operator: "Prefix"
              values:
                - "/etc/shadow"
                - "/etc/sudoers"
                - "/root/.ssh"
```

### 7.2 容器安全策略

```yaml
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: container-hardening
spec:
  kprobes:
    # 阻止容器内提权
    - call: "__x64_sys_setuid"
      syscall: true
      args:
        - index: 0
          type: "int"
      selectors:
        - matchArgs:
            - index: 0
              operator: "Equal"
              values:
                - "0"
        - matchActions:
            - action: Override
              argError: -1
    # 阻止容器内加载内核模块
    - call: "__x64_sys_finit_module"
      syscall: true
      selectors:
        - matchNamespaces:
            - operator: "In"
              values:
                - "init_mnt"
        - matchActions:
            - action: Override
              argError: -1
```

## 8. 监控与排障

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Tetragon 状态
kubectl get pods -n kube-system -l app.kubernetes.io/name=tetragon

# 查看 Tetragon 事件
kubectl logs -n kube-system ds/tetragon -f | tetra getevents

# Falco 状态
kubectl get pods -n falco

# 查看 Falco 告警
kubectl logs -n falco ds/falco -f

# 查看安全事件
kubectl get events -A --field-selector reason=SecurityViolation
```
---

## Related

- [[eBPF 开发基础|01-ebpf-programming-fundamentals]]
- [[eBPF 可观测工具|02-ebpf-observability-tools]]
- [[eBPF 网络应用|03-ebpf-networking-applications]]

## See Also

- [Tetragon 官方文档](https://tetragon.io/)
- [Falco 官方文档](https://falco.org/docs/)
- [KRSI 文档](https://www.kernel.org/doc/html/latest/bpf/prog_lsm.html)


<!-- risk-assessed -->
