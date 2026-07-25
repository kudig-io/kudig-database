---
title: Linux Security Modules for Containers
description: Linux Security Modules for Containers — Kubernetes 生产运维知识库
summary: Linux Security Modules for Containers — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- security
- apparmor
- selinux
- seccomp
- capabilities
- kubelet
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Linux Security Modules for Containers 是什么
- 如何 Linux Security Modules for Containers
trigger_keywords:
- Linux
- Security
- Modules
- for
- Containers
prerequisites:
- kubectl-basics
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Linux Security Modules for Containers

## Three-Layer Security Model

| Module | Type | Granularity | Learning Curve | Default Distro | Runtime Overhead |
|--------|------|-------------|----------------|----------------|-----------------|
| AppArmor | Mandatory Access Control | File/network/capability paths | Low | Ubuntu, Debian | ~1-3% |
| SELinux | Mandatory Access Control | File/process/port/type labels | High | RHEL, CentOS, Fedora | ~1-5% |
| seccomp | System Call Filter | Individual syscall allow/deny | Medium | All (kernel-level) | <1% |

## AppArmor

Profile-based MAC with simple syntax. Profiles define what files, networks, and capabilities a process can access:
- Allow patterns: `/app/** r` (read access to app directory)
- Deny patterns: `deny /etc/** w` (no write to /etc)
- Capabilities: `capability net_bind_service` (allow port binding)

K8s integration via annotation: `container.apparmor.security.beta.kubernetes.io/<container>: localhost/<profile>`

## SELinux

Type enforcement MAC with complex policy language. Every process, file, and port has a security context:
- Container process type: `container_t`
- Container file type: `container_file_t`
- Volume relabeling: `chcon -Rt container_file_t /mnt/data/app`

K8s integration via `securityContext.seLinuxOptions`.

## seccomp

Kernel-level syscall filtering. Whitelist-only approach: default deny all syscalls, explicitly allow needed ones.

K8s integration via `securityContext.seccompProfile`:
- `RuntimeDefault`: K8s built-in profile (recommended baseline)
- `Localhost`: Custom profile at `/var/lib/kubelet/seccomp/<profile>.json`

## Linux Capabilities

Capabilities subdivide root power into fine-grained permissions:

| Capability | Allows | Risk |
|-----------|--------|------|
| CAP_NET_BIND_SERVICE | Bind to ports < 1024 | Low |
| CAP_SYS_ADMIN | Mount, namespace ops | Very High (near-root) |
| CAP_NET_RAW | Raw socket access | Medium |
| CAP_DAC_OVERRIDE | Bypass file permissions | High |

Best practice: `drop: ["ALL"]` then add only needed capabilities.

## K8s SecurityContext Baseline

```yaml
securityContext:
  runAsNonRoot: true
  readOnlyRootFilesystem: true
  allowPrivilegeEscalation: false
  capabilities:
    drop: ["ALL"]
    add: ["NET_BIND_SERVICE"]  # only if needed
  seccompProfile:
    type: RuntimeDefault
```

## 源码实现分析

### seccomp 内核执行机制

```c
// kernel/seccomp.c - BPF 过滤器执行
static int __seccomp_filter(int this_syscall, const struct seccomp_data *sd) {
    // 1. 运行 BPF 程序检查系统调用
    u32 action = seccomp_run_filters(sd);
    
    switch (action) {
    case SECCOMP_RET_ALLOW:
        return 0;  // 允许系统调用
    case SECCOMP_RET_ERRNO:
        // 返回错误码，不执行系统调用
        syscall_set_return_value(current, regs, -errno, 0);
        return -1;
    case SECCOMP_RET_KILL_PROCESS:
        do_exit(SIGSYS);  // 杀死进程
    case SECCOMP_RET_LOG:
        audit_seccomp(this_syscall);  // 记录日志但允许
        return 0;
    }
}
```

### 安全模块层次架构

```
┌─────────────────────────────────────────────────┐
│  Pod SecurityContext (K8s API 层)              │
│  ├── runAsNonRoot / readOnlyRootFilesystem     │
│  ├── capabilities: drop ALL + add specific     │
│  └── seccompProfile: RuntimeDefault            │
└─────────────────┬───────────────────────────────┘
                  │ CRI 传递给运行时
                  ▼
┌─────────────────────────────────────────────────┐
│  containerd / CRI-O (运行时层)                │
│  ├── 生成 OCI spec (config.json)              │
│  ├── 设置 seccomp BPF 程序                    │
│  ├── 设置 AppArmor profile                    │
│  └── 设置 capabilities bounding set           │
└─────────────────┬───────────────────────────────┘
                  │ runc create
                  ▼
┌─────────────────────────────────────────────────┐
│  Linux Kernel (内核层)                        │
│  ├── seccomp: BPF 过滤系统调用              │
│  ├── AppArmor/SELinux: MAC 强制访问控制     │
│  ├── capabilities: 细分 root 权限           │
│  └── namespaces + cgroups: 隔离 + 资源限制  │
└─────────────────────────────────────────────────┘
```

## 使用场景

### 场景一：生产环境安全基线

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 2000
  containers:
  - name: app
    image: myapp:1.0
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop: ["ALL"]
      seccompProfile:
        type: RuntimeDefault
    volumeMounts:
    - name: tmp
      mountPath: /tmp          # 可写临时目录
  volumes:
  - name: tmp
    emptyDir: {}
```

### 场景二：检查安全模块状态

```bash
# 🟢 低风险 - 检查 AppArmor 状态
aa-status                        # Ubuntu/Debian
kubectl get nodes -o jsonpath='{.items[*].status.nodeInfo.osImage}'

# 🟢 低风险 - 检查 SELinux 状态
getenforce                       # RHEL/CentOS
sestatus

# 🟢 低风险 - 检查容器 capabilities
crictl inspect <container-id> | jq '.info.runtimeSpec.process.capabilities'

# 🟢 低风险 - 查看 seccomp 拒绝日志
dmesg | grep -i seccomp
journalctl -k | grep SECCOMP
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 容器默认无 root 权限 | 默认以 root 运行，必须显式设置 runAsNonRoot: true |
| drop ALL 后容器无法运行 | 大多数应用不需要任何 capability，少数需精确添加 |
| seccomp 影响性能 | RuntimeDefault 性能开销 <1%，仅拒绝危险系统调用 |
| AppArmor 和 SELinux 可同时启用 | 只能启用一个 LSM，由内核启动参数决定 |
| privileged 容器只是权限大 | privileged 禁用所有安全限制，等于宿主机 root |
| Pod Security Standards 自动执行 | 需启用 Pod Security Admission 或 Kyverno/OPA 策略 |

## 面试要点

1. **容器安全的四层防线？** — 1) K8s SecurityContext（声明式安全配置）；2) seccomp（系统调用过滤）；3) AppArmor/SELinux（MAC 强制访问控制）；4) Capabilities（细分 root 权限）。层层递进，纵深防御。

2. **seccomp RuntimeDefault 拒绝哪些系统调用？** — 拒绝 ~44 个危险系统调用（如 kexec_load、reboot、mount、unshare CLONE_NEWUSER 等），允许 ~270 个常用系统调用。基于 Docker 默认配置文件，覆盖 99% 应用需求。

3. **Capabilities 最佳实践？** — `drop: ["ALL"]` 先清空，再精确添加需要的。常见需求：NET_BIND_SERVICE（绑定低端口）、CHOWN（修改文件所有者）、NET_RAW（ping）。绝对避免 SYS_ADMIN（近乎 root）。

4. **Pod Security Standards 三个级别？** — Privileged（无限制，系统组件）；Baseline（禁止已知提权，生产默认）；Restricted（最严格，多租户/不可信工作负载）。通过 namespace label 执行：`pod-security.kubernetes.io/enforce: restricted`。

## Related

- [[22-概念/05-安全/secrets-management.md|secrets-management]] — [[Secrets|Secrets]]ts Management|Secrets Management]]
- [[23-实体/kubelet.md|[[kubelet|kubelet]]]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/15-运行时与系统/linux-container-foundation.md|linux-container-foundation]] — Linux Container Foundation
- [[22-概念/05-安全/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[22-概念/15-运行时与系统/linux-container-foundation.md|Linux Container Foundation]]
- [[22-概念/05-安全/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]


<!-- risk-assessed -->
