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

## Related

- [[概念/secrets-management.md|secrets-management]] — [[Secrets|Secrets]]ts Management|Secrets Management]]
- [[实体/kubelet.md|[[kubelet|kubelet]]]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/linux-container-foundation.md|linux-container-foundation]] — Linux Container Foundation
- [[概念/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[概念/linux-container-foundation.md|Linux Container Foundation]]
- [[概念/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]


<!-- risk-assessed -->
