---
title: Linux Sysctl Reference for Kubernetes
description: Linux Sysctl Reference for Kubernetes — Kubernetes 生产运维知识库
category: references
tags:
- linux
- sysctl
- kernel
- tuning
- k8s
- kubelet
- elasticsearch
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Linux Sysctl Reference for Kubernetes 是什么
- 如何 Linux Sysctl Reference for Kubernetes
trigger_keywords:
- Linux
- Sysctl
- Reference
- for
- Kubernetes
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# Linux Sysctl Reference for Kubernetes

## Network Parameters

### Connection Tracking (conntrack)

| Parameter | Default | Recommended | Purpose |
|---|---|---|---|
| net.netfilter.nf_conntrack_max | 65536 | 1000000 | Max tracked connections |
| net.netfilter.nf_conntrack_tcp_timeout_established | 432000 | 86400 | TCP established timeout |
| net.netfilter.nf_conntrack_tcp_timeout_time_wait | 120 | 60 | TIME_WAIT duration |

### TCP Tuning

| Parameter | Default | Recommended | Purpose |
|---|---|---|---|
| net.core.somaxconn | 128 | 32768 | Listen backlog |
| net.core.netdev_max_backlog | 1000 | 32768 | NIC receive queue |
| net.ipv4.tcp_max_syn_backlog | 128 | 32768 | SYN backlog |
| net.ipv4.tcp_tw_reuse | 0 | 1 | Reuse TIME_WAIT sockets |
| net.ipv4.ip_local_port_range | 32768-60999 | 1024-65535 | Ephemeral port range |
| net.ipv4.tcp_fin_timeout | 60 | 10 | FIN_WAIT2 timeout |

### File Descriptors

| Parameter | Default | Recommended | Purpose |
|---|---|---|---|
| fs.file-max | system-dependent | 1048576 | System-wide max FDs |
| fs.inotify.max_user_watches | 8192 | 524288 | File watcher limit |
| fs.nr_open | 1048576 | 1048576 | Per-process max FDs |

### Bridge Networking (CNI)

| Parameter | Default | Recommended | Purpose |
|---|---|---|---|
| net.bridge.bridge-nf-call-iptables | 1 | 1 | Bridge to iptables |
| net.bridge.bridge-nf-call-ip6tables | 1 | 1 | Bridge to ip6tables |
| net.bridge.bridge-nf-call-arptables | 1 | 1 | Bridge to arptables |

## Memory Parameters

| Parameter | Default | Recommended | Purpose |
|---|---|---|---|
| vm.overcommit_memory | 0 | 1 | Allow memory overcommit |
| vm.panic_on_oom | 0 | 0 | Do not panic on OOM |
| vm.swappiness | 60 | 1 | Minimize swap usage |
| vm.max_map_count | 65530 | 262144 | mmap limit (Elasticsearch) |

## Applying Configuration

```bash
# Create sysctl config
cat > /etc/sysctl.d/99-kubernetes.conf << 'EOF'
# Network tuning
net.core.somaxconn = 32768
net.core.netdev_max_backlog = 32768
net.ipv4.tcp_max_syn_backlog = 32768
net.ipv4.tcp_tw_reuse = 1
net.ipv4.ip_local_port_range = 1024 65535
net.ipv4.tcp_fin_timeout = 10

# Conntrack
net.netfilter.nf_conntrack_max = 1000000

# File descriptors
fs.file-max = 1048576
fs.inotify.max_user_watches = 524288

# Bridge
net.bridge.bridge-nf-call-iptables = 1
net.bridge.bridge-nf-call-ip6tables = 1

# Memory
vm.overcommit_memory = 1
vm.swappiness = 1
vm.max_map_count = 262144
EOF

# Apply
sysctl --system
# Or apply single parameter
sysctl -w net.core.somaxconn=32768

# Verify
sysctl net.core.somaxconn
```

## Pod-Level Sysctls

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: tuned-pod
spec:
  securityContext:
    sysctls:
      - name: net.core.somaxconn
        value: "16384"
      - name: net.ipv4.tcp_keepalive_time
        value: "600"
  containers:
    - name: app
      image: nginx
```

**Note**: Only safe sysctls can be set at pod level. Unsafe sysctls require kubelet configuration.

## Related

- [[entities/kubelet.md|kubelet]] — kubelet
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/linux-container-foundation.md|linux-container-foundation]] — Linux Container Foundation
- [[concepts/linux-sysctl-tuning.md|linux-sysctl-tuning]] — Linux Sysctl Tuning for Kubernetes
- [[concepts/linux-sysctl-tuning.md|Linux Sysctl Tuning]]
- [[concepts/linux-container-foundation.md|Linux Container Foundation]]
- [[skills/configure-health-probes.md|Configure Health Probes]]
