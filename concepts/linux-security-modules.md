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

- [[concepts/secrets-management.md|secrets-management]] — [[Secrets|Secrets]]ts Management|Secrets Management]]
- [[entities/kubelet.md|[[kubelet|kubelet]]]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/linux-container-foundation.md|linux-container-foundation]] — Linux Container Foundation
- [[concepts/cloud-native-defense-in-depth.md|cloud-native-defense-in-depth]] — Cloud Native Defense in Depth
- [[concepts/linux-container-foundation.md|Linux Container Foundation]]
- [[concepts/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
