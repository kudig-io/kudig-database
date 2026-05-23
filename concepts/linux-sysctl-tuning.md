---
title: Linux Sysctl Tuning for Kubernetes
description: Linux Sysctl Tuning for Kubernetes — Kubernetes 生产运维知识库
category: concepts
tags:
- k8s
- linux
- sysctl
- performance
- kernel
- tuning
- kubelet
- containerd
- docker
- redis
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Linux Sysctl Tuning for Kubernetes 是什么
- 如何 Linux Sysctl Tuning for Kubernetes
trigger_keywords:
- Linux
- Sysctl
- Tuning
- for
- Kubernetes
prerequisites:
- kubectl-basics
- redis-basics
created: "2026-05-23"
---

# Linux Sysctl Tuning for Kubernetes

## Critical Network Parameters

| Parameter | Recommended | K8s Impact |
|-----------|-------------|-----------|
| net.ipv4.ip_forward | 1 | Required for Pod cross-node communication |
| net.bridge.bridge-nf-call-iptables | 1 | Required for kube-proxy [[Service|Service]] routing |
| net.bridge.bridge-nf-call-ip6tables | 1 | Required for IPv6 dual-stack |
| net.netfilter.nf_conntrack_max | 1048576 | Conntrack table size for large clusters |
| net.core.somaxconn | 32768 | Socket listen queue for high-concurrency Services |
| net.ipv4.tcp_max_syn_backlog | 8096 | SYN queue for high-concurrency short connections |
| net.ipv4.tcp_tw_reuse | 1 | Reuse TIME_WAIT sockets for connection recycling |
| net.ipv4.tcp_keepalive_time | 600 | TCP keepalive interval (seconds) |

## Critical Filesystem Parameters

| Parameter | Recommended | K8s Impact |
|-----------|-------------|-----------|
| fs.inotify.max_user_watches | 524288 | [[kubelet|kubelet]] file watcher, must be increased |
| fs.inotify.max_user_instances | 8192 | kubelet + [[Container Runtime|container runtime]] |
| fs.file-max | 2097152 | System-wide file descriptor limit |
| fs.nr_open | 1048576 | Per-process file descriptor limit |
| fs.may_detach_mounts | 1 | Required for containerd/docker container deletion |

## Critical Memory Parameters

| Parameter | Recommended | K8s Impact |
|-----------|-------------|-----------|
| vm.swappiness | 0-10 | K8s recommends 0 to avoid swap performance jitter |
| vm.overcommit_memory | 1 | Always allow overcommit (Redis recommends 1) |
| vm.max_map_count | 262144 | Elasticsearch requires > default 65530 |
| vm.panic_on_oom | 0 | Let K8s OOM Killer handle, not kernel panic |

## Critical Kernel Parameters

| Parameter | Recommended | K8s Impact |
|-----------|-------------|-----------|
| kernel.panic | 10 | Auto-reboot delay after kernel panic (seconds) |
| kernel.panic_on_oops | 1 | Kernel oops triggers panic for node auto-recovery |
| user.max_user_namespaces | 28633 | Required for rootless containers |

## Required Kernel Modules

```bash
# Load required modules
modprobe br_netfilter   # Bridge netfilter for kube-proxy
modprobe overlay        # OverlayFS for container images

# Persist module loading
echo "br_netfilter" >> /etc/modules-load.d/k8s.conf
echo "overlay" >> /etc/modules-load.d/k8s.conf
```

## Application

```bash
# Write configuration
cat > /etc/sysctl.d/99-kubernetes.conf << 'EOF'
# Network
net.ipv4.ip_forward = 1
net.bridge.bridge-nf-call-iptables = 1
net.netfilter.nf_conntrack_max = 1048576
# Filesystem
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 8192
# Memory
vm.swappiness = 0
vm.max_map_count = 262144
EOF

# Apply
sysctl --system
```

## Related

- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/tcp-udp-protocol-stack.md|tcp-udp-protocol-stack]] — TCP/UDP Protocol Stack
- [[concepts/block-file-object-storage.md|block-file-object-storage]] — Block, File, and Object Storage
- [[concepts/linux-container-foundation.md|linux-container-foundation]] — Linux Container Foundation
- [[concepts/linux-container-foundation.md|Linux Container Foundation]]
- [[concepts/tcp-udp-protocol-stack.md|TCP/UDP Protocol Stack]]
- [[concepts/block-file-object-storage.md|Block, File, and Object Storage]]
