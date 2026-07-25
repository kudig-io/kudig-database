---
title: "eBPF 运行时安全：Falco/Tetragon/Tracee 部署与威胁检测"
description: "基于 eBPF 的容器运行时安全方案，涵盖 Falco 规则引擎、Tetragon 强制策略、Tracee 取证及事件响应"
summary: "深入对比 Falco、Tetragon、Tracee 三大 eBPF 运行时安全工具的架构与部署，讲解运行时威胁检测规则编写、安全事件响应流程及生产环境最佳实践"
category: 容器运行时
tags:
- ebpf
- falco
- tetragon
- tracee
- runtime-security
- threat-detection
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 安全工程师
estimated_read_time: 20min
intent_queries:
- "如何检测容器运行时安全威胁"
- "Falco 和 Tetragon 怎么选"
- "eBPF 运行时安全如何部署"
trigger_keywords:
- falco
- tetragon
- tracee
- ebpf
- runtime-security
- threat-detection
prerequisites:
- kubectl-basics
- ebpf-fundamentals
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

# eBPF 运行时安全

## 概述

容器运行时安全是云原生安全体系中最关键也最薄弱的环节。传统的静态安全措施（镜像扫描、网络策略、RBAC）无法检测运行时的恶意行为——如容器内反弹 shell、异常进程执行、敏感文件篡改等。eBPF（extended Berkeley Packet Filter）技术使得安全工具能够在内核态实时监控系统调用、进程行为和网络活动，而无需修改应用代码或加载内核模块。

当前三大主流 eBPF 运行时安全工具：
- **Falco**（CNCF 毕业项目）：基于规则引擎的威胁检测，支持 eBPF 和内核模块两种数据源
- **Tetragon**（Cilium/Isovalent）：eBPF 原生，支持检测+强制（enforcement），可在内核态直接阻断恶意行为
- **Tracee**（Aqua Security）：侧重安全取证和事件追踪，提供丰富的开箱即用检测规则

## 核心概念

### eBPF 安全监控原理

```
用户态应用（容器进程）
    ↓ syscall
内核态：
    ├── kprobe/tracepoint（进程事件：execve, fork, exit）
    ├── LSM hooks（安全模块钩子：file_open, bprm_check_security）
    ├── socket filter（网络事件：connect, accept, sendto）
    └── eBPF Map（状态存储：进程树、白名单、计数器）
         ↓
    eBPF Program（过滤、聚合、判定）
         ↓ perf buffer / ring buffer
用户态 Agent（Falco/Tetragon/Tracee）
         ↓
    告警 / 阻断 / 日志
```

### 三大工具架构对比

| 维度 | Falco | Tetragon | Tracee |
|------|-------|----------|--------|
| 数据源 | eBPF / 内核模块 / 插件 | 纯 eBPF | 纯 eBPF |
| 检测方式 | 规则引擎（条件匹配） | eBPF 程序（内核态判定） | 事件流 + 签名 |
| 强制能力 | 无（仅检测+告警） | 有（内核态 kill/override） | 无（检测+取证） |
| 规则语言 | YAML（条件表达式） | Go（eBPF 程序）+ CRD | Go 签名 / Rego |
| 性能开销 | 中（事件量大时高） | 低（内核态过滤） | 中 |
| 社区成熟度 | 高（CNCF 毕业） | 中（CNCF 沙箱） | 中 |
| 适用场景 | 合规审计、威胁检测 | 零信任强制、实时阻断 | 安全取证、事件调查 |
| K8s 集成 | DaemonSet + 规则 ConfigMap | DaemonSet + CRD（TracingPolicy） | DaemonSet |
| 云原生支持 | 插件架构（K8s audit、CloudTrail） | 原生 K8s 感知 | K8s 元数据关联 |

### 检测 vs 强制

- **检测模式（Detection）**：监控并告警，不干预进程执行。适合初期部署和观察期。
- **强制模式（Enforcement）**：在内核态直接阻断恶意行为（kill 进程、拒绝文件访问、重置网络连接）。Tetragon 独有优势。

## 生产部署

### Falco 部署

```yaml
# 🟡 中风险：部署 Falco DaemonSet（需要 privileged 权限）
# 使用 Helm 部署 Falco
# helm repo add falcosecurity https://falcosecurity.github.io/charts
# helm install falco falcosecurity/falco -n falco --create-namespace \
#   --set driver.kind=ebpf \
#   --set falcosidekick.enabled=true

apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: falco
  namespace: falco
  labels:
    app: falco
spec:
  selector:
    matchLabels:
      app: falco
  template:
    metadata:
      labels:
        app: falco
    spec:
      serviceAccountName: falco
      containers:
      - name: falco
        image: falcosecurity/falco:0.38.1
        securityContext:
          privileged: true
        env:
        - name: FALCO_DRIVER_LOADER
          value: "ebpf"
        volumeMounts:
        - name: proc
          mountPath: /host/proc
          readOnly: true
        - name: boot
          mountPath: /host/boot
          readOnly: true
        - name: modules
          mountPath: /host/lib/modules
          readOnly: true
        - name: rules
          mountPath: /etc/falco/rules.d
        resources:
          requests:
            cpu: 100m
            memory: 256Mi
          limits:
            cpu: "1"
            memory: 512Mi
      volumes:
      - name: proc
        hostPath:
          path: /proc
      - name: boot
        hostPath:
          path: /boot
      - name: modules
        hostPath:
          path: /lib/modules
      - name: rules
        configMap:
          name: falco-custom-rules
```

### Falco 自定义规则

```yaml
# 🟢 低风险：自定义检测规则 ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-custom-rules
  namespace: falco
data:
  custom_rules.yaml: |
    - rule: Detect Reverse Shell in Container
      desc: Detect any shell process that opens a network connection (potential reverse shell)
      condition: >
        spawned_process and container and
        proc.name in (bash, sh, zsh, ash) and
        (fd.typechar = 4 or fd.typechar = 6) and
        evt.arg.flags contains "O_RDWR"
      output: >
        Reverse shell detected (user=%user.name user_loginuid=%user.loginuid
        command=%proc.cmdline pid=%proc.pid connection=%fd.name
        container_id=%container.id container_name=%container.name
        image=%container.image.repository:%container.image.tag)
      priority: CRITICAL
      tags: [container, shell, network, mitre_execution]

    - rule: Detect Crypto Miner Process
      desc: Detect known cryptocurrency mining processes
      condition: >
        spawned_process and container and
        (proc.name in (xmrig, minerd, cpuminer, stratum) or
         proc.cmdline contains "stratum+tcp://" or
         proc.cmdline contains "--donate-level")
      output: >
        Crypto miner detected (command=%proc.cmdline container=%container.name
        image=%container.image.repository)
      priority: CRITICAL
      tags: [container, crypto, mitre_impact]

    - rule: Sensitive File Modification in Container
      desc: Detect modification of sensitive files inside containers
      condition: >
        write and container and
        fd.name in (/etc/shadow, /etc/passwd, /etc/sudoers,
                    /root/.ssh/authorized_keys, /etc/crontab)
      output: >
        Sensitive file modified (file=%fd.name command=%proc.cmdline
        container=%container.name user=%user.name)
      priority: ERROR
      tags: [container, filesystem, mitre_persistence]
```

### Tetragon 部署

```yaml
# 🟡 中风险：部署 Tetragon（eBPF 安全强制）
# helm install tetragon cilium/tetragon -n kube-system \
#   --set tetragon.enableProcessCred=true \
#   --set tetragon.enableProcessNs=true

apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: security-monitoring
  namespace: kube-system
spec:
  kprobes:
  - call: "fd_install"
    syscall: false
    args:
    - index: 0
      type: "int"
    - index: 1
      type: "file"
    selectors:
    - matchNamespaces:
      - production
      matchArgs:
      - index: 1
        operator: "Equal"
        values:
        - "/etc/shadow"
        - "/etc/passwd"
        - "/root/.ssh"
  tracepoints:
  - event: "sched/sched_process_exec"
    selectors:
    - matchNamespaces:
      - production
      matchBinaries:
      - operator: "In"
        values:
        - "curl"
        - "wget"
        - "nc"
        - "ncat"
        - "python"
        - "perl"
```

### Tetragon 强制策略（Enforcement）

```yaml
# 🔴 高风险：强制策略会直接终止进程，可能导致服务中断
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: enforce-no-shell-in-container
  namespace: kube-system
spec:
  kprobes:
  - call: "__x64_sys_execve"
    syscall: false
    args:
    - index: 0
      type: "string"
    selectors:
    - matchNamespaces:
      - production
      matchArgs:
      - index: 0
        operator: "In"
        values:
        - "/bin/bash"
        - "/bin/sh"
        - "/bin/ash"
        - "/bin/zsh"
    actions:
    - action: Sigkill
      argError: -1
      # 仅对非 init 容器进程生效
      argIndex: 0
```

### Tracee 部署

```yaml
# 🟡 中风险：部署 Tracee DaemonSet
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: tracee
  namespace: tracee-system
spec:
  selector:
    matchLabels:
      app: tracee
  template:
    metadata:
      labels:
        app: tracee
    spec:
      containers:
      - name: tracee
        image: aquasec/tracee:0.21.0
        args:
        - --output
        - json
        - --events
        - stdio_over_socket,anti_debugging,ptrace_code_injection,
          container_device_conflict,hidden_kernel_module,
          cgroup_notify_on_release,sched_process_exec
        securityContext:
          privileged: true
        env:
        - name: LIBBPFGO_OSRELEASE_FILE
          value: /etc/os-release-host
        volumeMounts:
        - name: os-release
          mountPath: /etc/os-release-host
          readOnly: true
        - name: tracee-events
          mountPath: /var/log/tracee
        resources:
          requests:
            cpu: 200m
            memory: 512Mi
          limits:
            cpu: "2"
            memory: 1Gi
      volumes:
      - name: os-release
        hostPath:
          path: /etc/os-release
      - name: tracee-events
        hostPath:
          path: /var/log/tracee
```

## 运维操作

### Falco 告警查看与管理

```bash
# 🟢 低风险：查看 Falco 告警
# 实时查看告警
kubectl logs -n falco -l app=falco -f --tail=50

# 查看特定严重级别告警
kubectl logs -n falco -l app=falco --since=1h | grep "CRITICAL"

# 检查 Falco 状态
kubectl get pods -n falco -o wide
kubectl exec -n falco -it ds/falco -- falco --version

# 验证规则加载
kubectl exec -n falco -it ds/falco -- falco --validate /etc/falco/rules.d/custom_rules.yaml

# 查看 Falco 事件统计
kubectl exec -n falco -it ds/falco -- curl -s http://localhost:8765/healthz
```

### Tetragon 事件查看

```bash
# 🟢 低风险：查看 Tetragon 事件
# 实时查看安全事件
kubectl logs -n kube-system -l app.kubernetes.io/name=tetragon -f --tail=20

# 使用 tetra CLI 查看结构化事件
kubectl exec -n kube-system ds/tetragon -- tetra getevents

# 查看特定 Pod 的安全事件
kubectl exec -n kube-system ds/tetragon -- tetra getevents -n production -p my-app-pod

# 检查 TracingPolicy 状态
kubectl get tracingpolicy
kubectl describe tracingpolicy security-monitoring
```

### 安全事件响应流程

```bash
# 🟡 中风险：安全事件响应
# 1. 确认告警（以 Falco 反弹 shell 告警为例）
kubectl logs -n falco -l app=falco --since=5m | grep "Reverse shell"

# 2. 定位受影响 Pod
kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A5 "status:"

# 3. 检查 Pod 内进程
kubectl exec -n <namespace> <pod-name> -- ps aux

# 4. 检查网络连接
kubectl exec -n <namespace> <pod-name> -- ss -tnp

# 5. 隔离 Pod（如果确认恶意）
# 🔴 高风险：删除 Pod 会中断服务
kubectl delete pod <pod-name> -n <namespace> --grace-period=0

# 6. 检查是否有横向移动
kubectl logs -n falco -l app=falco --since=30m | grep "container.id=<container-id>"
```

## 故障排查

### eBPF 加载失败

```bash
# 🟢 低风险：诊断 eBPF 加载问题
# 检查内核版本（eBPF 需要 4.15+，推荐 5.8+）
uname -r

# 检查 BPF 文件系统是否挂载
mount | grep bpf
# 应有：bpffs on /sys/fs/bpf type bpf

# 检查内核配置
grep CONFIG_BPF /boot/config-$(uname -r)
# 需要：CONFIG_BPF=y, CONFIG_BPF_SYSCALL=y, CONFIG_BPF_JIT=y

# 检查 Falco eBPF probe 是否加载
ls /root/.falco/
# 应有：falco-bpf.o 或对应内核版本的 probe

# 手动加载 eBPF probe
kubectl exec -n falco -it ds/falco -- falco --modern-bpf --dry-run

# 检查 Tetragon eBPF 程序
bpftool prog list | grep tetragon
```

### 性能问题

```bash
# 🟢 低风险：性能诊断
# 检查 Falco 事件丢弃率
kubectl logs -n falco -l app=falco --since=1h | grep "dropped"

# 调整 Falco 缓冲区大小
# 在 falco.yaml 中：
# engine:
#   ebpf:
#     buf_size_preset: 4  # 增大缓冲区

# 检查 Tetragon 事件速率
kubectl exec -n kube-system ds/tetragon -- tetra getevents --output compact | wc -l

# 检查节点 CPU 开销
kubectl top pods -n falco
kubectl top pods -n kube-system -l app.kubernetes.io/name=tetragon

# 如果开销过高，缩小监控范围
# Falco：通过 rules 的 condition 添加 namespace 过滤
# Tetragon：通过 TracingPolicy 的 matchNamespaces 限制
```

### 误报处理

```bash
# 🟢 低风险：处理误报
# Falco：添加例外规则
# 在 custom_rules.yaml 中：
# - rule: Detect Reverse Shell in Container
#   exceptions:
#   - name: known_admin_tools
#     values:
#     - proc.cmdline contains "kubectl exec"
#     - container.image.repository = "registry.example.com/admin-toolkit"

# Tetragon：调整 TracingPolicy selector
# 添加 matchBinaries 排除合法工具

# 验证规则修改后无语法错误
kubectl exec -n falco -it ds/falco -- falco --validate /etc/falco/rules.d/
```

## 最佳实践

### 部署策略

1. **分阶段部署**：先检测模式运行 2-4 周，收集基线，再逐步启用强制策略
2. **命名空间隔离**：生产环境使用更严格的规则，开发环境放宽限制
3. **资源预留**：eBPF 安全工具 DaemonSet 设置合理的 resource limits，避免与业务争抢
4. **内核版本要求**：生产节点内核 ≥ 5.8（BTF 支持），推荐 5.15+（完整 eBPF 特性）

### 规则管理

1. **规则版本控制**：自定义规则存储在 Git 仓库，通过 [[03-清单模式/09-平台模式/02-cue-language-configuration|CUE]] 或 Helm values 管理
2. **白名单机制**：为合法运维操作（kubectl exec、CI/CD 工具）添加例外
3. **规则测试**：使用 `falco --validate` 和 `tetra getevents --dry-run` 验证规则
4. **告警路由**：Falco 告警通过 Falcosidekick 路由到 Slack/PagerDuty/SIEM

### 与现有安全体系集成

- **镜像安全**：运行时检测是 [[14-容器运行时/03-containerd-CRI-O/06-runtime-security-hardening|运行时安全加固]] 的补充，不能替代镜像扫描
- **网络策略**：eBPF 网络监控与 [[23-实体/04-网络/cilium|Cilium NetworkPolicy]] 互补
- **SIEM 集成**：告警事件导出到 ELK/Splunk，与 [[23-实体/07-可观测性/prometheus|Prometheus]] 指标关联
- **RBAC 审计**：K8s Audit Log + Falco 双重审计，覆盖 API 层和运行时层

## Related

- [[14-容器运行时/03-containerd-CRI-O/06-runtime-security-hardening|运行时安全加固]]
- [[14-容器运行时/03-containerd-CRI-O/04-kata-containers-secure-container|Kata Containers 安全容器]]
- [[17-系统基础/01-Linux/07-linux-security-hardening|Linux 安全加固]]
- [[24-综合/05-可观测性/ebpf-observability|eBPF 可观测性]]
- [[10-平台工程/03-治理/10-security-compliance|安全合规]]
- [[24-综合/05-可观测性/ebpf-observability|eBPF 可观测性综合]]
