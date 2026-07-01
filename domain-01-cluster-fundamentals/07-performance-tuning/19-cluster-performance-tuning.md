---
title: 19-集群性能调优
description: '- 性能优化'
summary: '- 性能优化'
category: general
tags:
- k8s
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- containerd
- docker
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- 集群性能调优 是什么
- 如何 集群性能调优
- Kubernetes 01 cluster fundamentals 最佳实践
trigger_keywords:
- 集群性能调优
- cluster
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- prometheus-basics
- etcd-basics
---



---
tags:
- k8s
- production
- best-practice
- performance
aliases:
- performance
- 性能优化
- 性能调优
intent_queries:
- 如何优化19-cluster-performance-tuning的性能？
- 19-cluster-performance-tuning的性能调优指南
- 19-cluster-performance-tuning的瓶颈在哪里？

tier: peripheral---
title: 19-集群性能调优
description: '<!-- chunk: 📋 概述' -->## 📋 概述'
category: production-operations
tags:
- k8s
- production
- operations
- best-practices
- [[etcd|etcd]]
- apiserver
- [[kubelet|kubelet]]
- scheduler
- controller-manager
- [[Prometheus|prometheus]]
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 5min
intent_queries:
- 集群性能调优 是什么
- 如何 集群性能调优
- [[Kubernetes|Kubernetes]] 18 production operations 最佳实践
trigger_keywords:
- 集群性能调优
- production
- operations
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 19-集群性能调优

> **适用范围**: Kubernetes v1.25-v1.32 | **维护状态**: 🔧 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐

<!-- chunk: 📋 概述 -->## 📋 概述

集群性能调优是保障Kubernetes高效稳定运行的关键。本文档详细介绍内核参数优化、组件调优和性能监控的最佳实践。

<!-- chunk: ⚡ 内核参数优化 -->## ⚡ 内核参数优化

## 系统级性能调优

## 1. 网络性能优化
```bash
#!/bin/bash
# 网络性能优化脚本

# 设置网络参数
cat > /etc/sysctl.d/99-k8s-network.conf << EOF
# TCP优化
net.core.somaxconn = 65535
net.core.netdev_max_backlog = 5000
net.core.rmem_default = 262144
net.core.wmem_default = 262144
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216

# TCP缓冲区
net.ipv4.tcp_rmem = 4096 65536 16777216
net.ipv4.tcp_wmem = 4096 65536 16777216
net.ipv4.tcp_mem = 786432 1048576 26777216

# TCP优化
net.ipv4.tcp_fin_timeout = 30
net.ipv4.tcp_keepalive_time = 1200
net.ipv4.tcp_max_syn_backlog = 8192
net.ipv4.tcp_max_tw_buckets = 6000
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_syncookies = 1
net.ipv4.tcp_timestamps = 1
net.ipv4.tcp_sack = 1
net.ipv4.tcp_window_scaling = 1

# 网络设备优化
net.core.message_cost = 0
net.core.message_burst = 100
EOF

# 应用配置
sysctl -p /etc/sysctl.d/99-k8s-network.conf

# 针对特定网络接口优化
INTERFACE="eth0"
ethtool -G $INTERFACE rx 4096 tx 4096
ethtool -K $INTERFACE gro on
ethtool -K $INTERFACE gso on
ethtool -K $INTERFACE tso on
ethtool -K $INTERFACE lro on
```

## 2. 文件系统优化
```bash
#!/bin/bash
# 文件系统性能优化

# 优化ext4文件系统
tune2fs -o journal_data_writeback /dev/sda1
tune2fs -m 1 /dev/sda1  # 减少保留块百分比

# 挂载选项优化
cat >> /etc/fstab << EOF
/dev/sda1 / ext4 defaults,noatime,data=writeback,barrier=0 0 1
EOF

# 内核文件句柄优化
cat > /etc/sysctl.d/99-k8s-fs.conf << EOF
fs.file-max = 2097152
fs.nr_open = 2097152
fs.aio-max-nr = 1048576
fs.inotify.max_user_watches = 1048576
fs.inotify.max_user_instances = 1024
fs.inotify.max_queued_events = 16384
EOF

sysctl -p /etc/sysctl.d/99-k8s-fs.conf

# 用户级文件句柄限制
cat >> /etc/security/limits.conf << EOF
root soft nofile 1048576
root hard nofile 1048576
* soft nofile 1048576
* hard nofile 1048576
EOF
```

## 3. 内存管理优化
```bash
#!/bin/bash
# 内存性能优化

cat > /etc/sysctl.d/99-k8s-memory.conf << EOF
# 内存回收优化
vm.swappiness = 1
vm.dirty_ratio = 15
vm.dirty_background_ratio = 5
vm.dirty_expire_centisecs = 2000
vm.dirty_writeback_centisecs = 100
vm.vfs_cache_pressure = 50

# 内存过量使用
vm.overcommit_memory = 1
vm.overcommit_ratio = 100

# 透明大页优化
kernel.transparent_hugepage = madvise

# NUMA优化
kernel.numa_balancing = 0
EOF

sysctl -p /etc/sysctl.d/99-k8s-memory.conf

# 禁用透明大页压缩
echo never > /sys/kernel/mm/transparent_hugepage/defrag
echo never > /sys/kernel/mm/transparent_hugepage/enabled
```

## 容器运行时优化

## 1. Containerd优化配置
```toml
# /etc/containerd/config.toml
version = 2

[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
    stream_server_address = "127.0.0.1"
    stream_server_port = "0"
    enable_selinux = false
    enable_tls_streaming = false
    
    [plugins."io.containerd.grpc.v1.cri".containerd]
      snapshotter = "overlayfs"
      default_runtime_name = "runc"
      
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
          runtime_type = "io.containerd.runc.v2"
          [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
            SystemdCgroup = true
            BinaryName = "/usr/bin/runc"
    
    [plugins."io.containerd.grpc.v1.cri".cni]
      bin_dir = "/opt/cni/bin"
      conf_dir = "/etc/cni/net.d"
      
    [plugins."io.containerd.grpc.v1.cri".registry]
      config_path = "/etc/containerd/certs.d"
      
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
          endpoint = ["https://mirror.gcr.io", "https://registry-1.docker.io"]

[plugins."io.containerd.internal.v1.opt"]
  path = "/opt/containerd"

[plugins."io.containerd.grpc.v1.cri".registry.configs]
  [plugins."io.containerd.grpc.v1.cri".registry.configs."gcr.io".tls]
    insecure_skip_verify = true
```

## 2. Docker性能优化
```json
{
  "registry-mirrors": [
    "https://mirror.gcr.io",
    "https://docker.mirrors.ustc.edu.cn"
  ],
  "insecure-registries": [],
  "debug": false,
  "experimental": false,
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "100m",
    "max-file": "3"
  },
  "storage-driver": "overlay2",
  "storage-opts": [
    "overlay2.override_kernel_check=true"
  ],
  "live-restore": true,
  "default-ulimits": {
    "nofile": {
      "Name": "nofile",
      "Hard": 1048576,
      "Soft": 1048576
    }
  }
}
```

<!-- chunk: 🎯 Kubernetes组件调优 -->## 🎯 Kubernetes组件调优

## API Server优化

## 1. API Server配置优化
```yaml
# API Server优化配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
  - name: kube-apiserver
    image: registry.k8s.io/kube-apiserver:v1.28.2
    command:
    - kube-apiserver
    - --advertise-address=$(NODE_IP)
    - --allow-privileged=true
    - --authorization-mode=Node,RBAC
    - --client-ca-file=/etc/kubernetes/pki/ca.crt
    - --enable-admission-plugins=NodeRestriction
    - --enable-bootstrap-token-auth=true
    - --etcd-cafile=/etc/kubernetes/pki/etcd/ca.crt
    - --etcd-certfile=/etc/kubernetes/pki/apiserver-etcd-client.crt
    - --etcd-keyfile=/etc/kubernetes/pki/apiserver-etcd-client.key
    - --etcd-servers=https://127.0.0.1:2379
    - --kubelet-client-certificate=/etc/kubernetes/pki/apiserver-kubelet-client.crt
    - --kubelet-client-key=/etc/kubernetes/pki/apiserver-kubelet-client.key
    - --kubelet-preferred-address-types=InternalIP,ExternalIP,Hostname
    - --proxy-client-cert-file=/etc/kubernetes/pki/front-proxy-client.crt
    - --proxy-client-key-file=/etc/kubernetes/pki/front-proxy-client.key
    - --requestheader-allowed-names=front-proxy-client
    - --requestheader-client-ca-file=/etc/kubernetes/pki/front-proxy-ca.crt
    - --requestheader-extra-headers-prefix=X-Remote-Extra-
    - --requestheader-group-headers=X-Remote-Group
    - --requestheader-username-headers=X-Remote-User
    - --secure-port=6443
    - --service-account-issuer=https://kubernetes.default.svc.cluster.local
    - --service-account-key-file=/etc/kubernetes/pki/sa.pub
    - --service-account-signing-key-file=/etc/kubernetes/pki/sa.key
    - --service-cluster-ip-range=10.96.0.0/12
    - --tls-cert-file=/etc/kubernetes/pki/apiserver.crt
    - --tls-private-key-file=/etc/kubernetes/pki/apiserver.key
    
    # 性能优化参数
    - --max-requests-inflight=3000
    - --max-mutating-requests-inflight=1000
    - --request-timeout=2m
    - --min-request-timeout=1800
    - --target-ram-mb=0  # 自动检测
    - --kubelet-timeout=10s
    - --enable-aggregator-routing=true
    - --default-not-ready-toleration-seconds=300
    - --default-unreachable-toleration-seconds=300
    - --delete-collection-workers=3
    - --enable-garbage-collector=true
    - --enable-logs-handler=true
    - --event-ttl=1h
    - --goaway-chance=0.001
    - --http2-max-streams-per-connection=1000
    - --kube-api-qps=50
    - --kube-api-burst=100
    - --lease-reuse-duration-seconds=60
    - --livez-grace-period=10s
    - --profiling=false
    - --shutdown-delay-duration=70s
    - --tls-cipher-suites=TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
```

## 2. API Server资源分配
```yaml
# API Server资源优化
apiVersion: v1
kind: Pod
metadata:
  name: kube-apiserver
  namespace: kube-system
spec:
  containers:
  - name: kube-apiserver
    resources:
      requests:
        cpu: 2
        memory: 8Gi
      limits:
        cpu: 4
        memory: 16Gi
    startupProbe:
      httpGet:
        path: /livez
        port: 6443
        scheme: HTTPS
      initialDelaySeconds: 10
      periodSeconds: 10
      timeoutSeconds: 15
      successThreshold: 1
      failureThreshold: 5
    livenessProbe:
      httpGet:
        path: /livez
        port: 6443
        scheme: HTTPS
      initialDelaySeconds: 10
      periodSeconds: 10
      timeoutSeconds: 15
      successThreshold: 1
      failureThreshold: 8
    readinessProbe:
      httpGet:
        path: /readyz
        port: 6443
        scheme: HTTPS
      initialDelaySeconds: 5
      periodSeconds: 5
      timeoutSeconds: 15
      successThreshold: 1
      failureThreshold: 3
```

## Etcd性能优化

## 1. Etcd配置优化
```yaml
# Etcd优化配置
apiVersion: v1
kind: Pod
metadata:
  name: etcd
  namespace: kube-system
spec:
  containers:
  - name: etcd
    image: registry.k8s.io/etcd:3.5.9-0
    command:
    - etcd
    - --advertise-client-urls=https://$(NODE_IP):2379
    - --cert-file=/etc/kubernetes/pki/etcd/server.crt
    - --client-cert-auth=true
    - --data-dir=/var/lib/etcd
    - --initial-advertise-peer-urls=https://$(NODE_IP):2380
    - --initial-cluster=default=https://$(NODE_IP):2380
    - --key-file=/etc/kubernetes/pki/etcd/server.key
    - --listen-client-urls=https://127.0.0.1:2379,https://$(NODE_IP):2379
    - --listen-metrics-urls=http://127.0.0.1:2381
    - --listen-peer-urls=https://$(NODE_IP):2380
    - --name=$(NODE_NAME)
    - --peer-cert-file=/etc/kubernetes/pki/etcd/peer.crt
    - --peer-client-cert-auth=true
    - --peer-key-file=/etc/kubernetes/pki/etcd/peer.key
    - --peer-trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    - --snapshot-count=10000
    - --trusted-ca-file=/etc/kubernetes/pki/etcd/ca.crt
    
    # 性能优化参数
    - --auto-compaction-mode=revision
    - --auto-compaction-retention=1000
    - --quota-backend-bytes=8589934592  # 8GB
    - --max-request-bytes=1572864  # 1.5MB
    - --max-concurrent-streams=2000
    - --heartbeat-interval=100
    - --election-timeout=1000
    - --grpc-keepalive-min-time=5s
    - --grpc-keepalive-interval=2h
    - --grpc-keepalive-timeout=20s
```

## 2. Etcd存储优化
```bash
#!/bin/bash
# Etcd存储性能优化

# SSD存储挂载优化
cat >> /etc/fstab << EOF
/dev/nvme0n1 /var/lib/etcd ext4 defaults,noatime,data=ordered,barrier=0 0 2
EOF

# Etcd磁盘IO优化
cat > /etc/systemd/system/etcd.service.d/10-performance.conf << EOF
[Service]
IOSchedulingClass=best-effort
IOSchedulingPriority=0
CPUSchedulingPolicy=rr
CPUSchedulingPriority=99
EOF

systemctl daemon-reload

# 文件系统优化
tune2fs -o journal_data_ordered /dev/nvme0n1
tune2fs -m 1 /dev/nvme0n1

# 定期维护脚本
cat > /usr/local/bin/etcd-maintenance.sh << 'EOF'
#!/bin/bash
# Etcd维护脚本

# 检查碎片整理需求
FRAGMENTATION=$(etcdctl --endpoints=https://127.0.0.1:2379 \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  alarm list | grep "NOSPACE" || echo "OK")

if [ "$FRAGMENTATION" != "OK" ]; then
  echo "Performing etcd defragmentation..."
  etcdctl --endpoints=https://127.0.0.1:2379 \
    --cert=/etc/kubernetes/pki/etcd/server.crt \
    --key=/etc/kubernetes/pki/etcd/server.key \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    defrag
fi

# 检查健康状态
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  endpoint health

# 清理旧快照
find /var/lib/etcd/member/snap -name "db-*" -mtime +7 -delete
EOF

chmod +x /usr/local/bin/etcd-maintenance.sh

# 添加定时任务
cat > /etc/cron.d/etcd-maintenance << EOF
0 2 * * * root /usr/local/bin/etcd-maintenance.sh >> /var/log/etcd-maintenance.log 2>&1
EOF
```

## Controller Manager优化

## 1. Controller Manager配置
```yaml
# Controller Manager优化配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-controller-manager
  namespace: kube-system
spec:
  containers:
  - name: kube-controller-manager
    image: registry.k8s.io/kube-controller-manager:v1.28.2
    command:
    - kube-controller-manager
    - --allocate-node-cidrs=true
    - --authentication-kubeconfig=/etc/kubernetes/controller-manager.conf
    - --authorization-kubeconfig=/etc/kubernetes/controller-manager.conf
    - --bind-address=127.0.0.1
    - --client-ca-file=/etc/kubernetes/pki/ca.crt
    - --cluster-cidr=10.244.0.0/16
    - --cluster-name=kubernetes
    - --cluster-signing-cert-file=/etc/kubernetes/pki/ca.crt
    - --cluster-signing-key-file=/etc/kubernetes/pki/ca.key
    - --controllers=*,bootstrapsigner,tokencleaner
    - --kubeconfig=/etc/kubernetes/controller-manager.conf
    - --leader-elect=true
    - --requestheader-client-ca-file=/etc/kubernetes/pki/front-proxy-ca.crt
    - --root-ca-file=/etc/kubernetes/pki/ca.crt
    - --service-account-private-key-file=/etc/kubernetes/pki/sa.key
    - --use-service-account-credentials=true
    
    # 性能优化参数
    - --concurrent-deployment-syncs=10
    - --concurrent-endpoint-syncs=10
    - --concurrent-gc-syncs=30
    - --concurrent-namespace-syncs=20
    - --concurrent-replicaset-syncs=10
    - --concurrent-resource-quota-syncs=5
    - --concurrent-service-syncs=2
    - --concurrent-serviceaccount-token-syncs=5
    - --large-cluster-size-threshold=50
    - --node-eviction-rate=0.1
    - --secondary-node-eviction-rate=0.01
    - --node-monitor-grace-period=40s
    - --node-startup-grace-period=60s
    - --pod-eviction-timeout=5m0s
    - --pv-recycler-pod-template-filepath-nfs=""
    - --pv-recycler-pod-template-filepath-hostpath=""
    - --terminated-pod-gc-threshold=1000
    - --flex-volume-plugin-dir=/usr/libexec/kubernetes/kubelet-plugins/volume/exec/
```

## 2. Scheduler优化配置
```yaml
# Scheduler优化配置
apiVersion: v1
kind: Pod
metadata:
  name: kube-scheduler
  namespace: kube-system
spec:
  containers:
  - name: kube-scheduler
    image: registry.k8s.io/kube-scheduler:v1.28.2
    command:
    - kube-scheduler
    - --authentication-kubeconfig=/etc/kubernetes/scheduler.conf
    - --authorization-kubeconfig=/etc/kubernetes/scheduler.conf
    - --bind-address=127.0.0.1
    - --kubeconfig=/etc/kubernetes/scheduler.conf
    - --leader-elect=true
    
    # 性能优化参数
    - --percentage-of-nodes-to-score=50
    - --pod-max-in-unschedulable-pods-duration=30s
    - --scheduler-name=default-scheduler
    - --disable-preemption=false
    - --enable-priority-and-fairness=true
    - --contention-profiling=false
    - --kube-api-burst=100
    - --kube-api-qps=50
    - --parallelism=16
    - --permit-without-streaming=true
    - --pod-initial-backoff-seconds=1
    - --pod-max-backoff-seconds=10
```

<!-- chunk: 📊 性能监控和分析 -->## 📊 性能监控和分析

## 性能指标收集

## 1. Prometheus性能监控
```yaml
# Kubernetes性能监控规则
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: k8s-performance-rules
  namespace: monitoring
spec:
  groups:
  - name: k8s.performance.rules
    rules:
    # API Server性能指标
    - record: apiserver:request_duration_seconds:p99
      expr: histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m]))
      
    - record: apiserver:request_rate:per_second
      expr: sum(rate(apiserver_request_total[5m])) by (verb, resource)
      
    - record: apiserver:error_rate:percentage
      expr: sum(rate(apiserver_request_total{code=~"5.."}[5m])) / sum(rate(apiserver_request_total[5m])) * 100
      
    # Etcd性能指标
    - record: etcd:disk_backend_commit_duration_seconds:p99
      expr: histogram_quantile(0.99, rate(etcd_disk_backend_commit_duration_seconds_bucket[5m]))
      
    - record: etcd:network_peer_round_trip_time_seconds:p99
      expr: histogram_quantile(0.99, rate(etcd_network_peer_round_trip_time_seconds_bucket[5m]))
      
    - record: etcd:mvcc_db_total_size_in_bytes
      expr: etcd_mvcc_db_total_size_in_bytes
      
    # 节点性能指标
    - record: node:cpu_utilization:ratio
      expr: 1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance)
      
    - record: node:memory_utilization:ratio
      expr: (node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes) / node_memory_MemTotal_bytes
      
    - record: node:disk_io_utilization:ratio
      expr: rate(node_disk_io_time_seconds_total[5m])
      
    # 容器性能指标
    - record: container:cpu_usage:cores
      expr: rate(container_cpu_usage_seconds_total[5m])
      
    - record: container:memory_usage:bytes
      expr: container_memory_working_set_bytes
      
    - record: container:network_receive_bytes:rate
      expr: rate(container_network_receive_bytes_total[5m])
```

## 2. 性能分析仪表板
```json
{
  "dashboard": {
    "title": "Kubernetes Performance Analysis",
    "panels": [
      {
        "title": "API Server Performance",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, rate(apiserver_request_duration_seconds_bucket[5m]))",
            "legendFormat": "{{verb}} {{resource}} p95"
          },
          {
            "expr": "histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m]))",
            "legendFormat": "{{verb}} {{resource}} p99"
          }
        ]
      },
      {
        "title": "Etcd Performance",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.99, rate(etcd_disk_backend_commit_duration_seconds_bucket[5m]))",
            "legendFormat": "Backend Commit p99"
          },
          {
            "expr": "histogram_quantile(0.99, rate(etcd_network_peer_round_trip_time_seconds_bucket[5m]))",
            "legendFormat": "Peer RTT p99"
          }
        ]
      },
      {
        "title": "Node Resource Utilization",
        "type": "graph",
        "targets": [
          {
            "expr": "1 - avg(rate(node_cpu_seconds_total{mode=\"idle\"}[5m])) by (instance)",
            "legendFormat": "CPU {{instance}}"
          },
          {
            "expr": "(node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes) / node_memory_MemTotal_bytes",
            "legendFormat": "Memory {{instance}}"
          }
        ]
      },
      {
        "title": "Container Performance",
        "type": "heatmap",
        "targets": [
          {
            "expr": "rate(container_cpu_usage_seconds_total[5m])",
            "legendFormat": "{{pod}} CPU"
          }
        ]
      }
    ]
  }
}
```

## 性能瓶颈诊断

## 1. 性能分析脚本
```python
#!/usr/bin/env python3
# Kubernetes性能分析工具

import asyncio
import aiohttp
import json
from datetime import datetime, timedelta
from kubernetes import client, config
import numpy as np

class PerformanceAnalyzer:
    def __init__(self):
        config.load_kube_config()
        self.core_v1 = client.CoreV1Api()
        self.apps_v1 = client.AppsV1Api()
        self.metrics_client = client.CustomObjectsApi()
        
        self.performance_thresholds = {
            'api_server_p99_latency': 1.0,  # 秒
            'etcd_commit_latency': 0.1,     # 秒
            'node_cpu_utilization': 0.85,   # 85%
            'node_memory_utilization': 0.90, # 90%
            'pod_startup_time': 30,         # 秒
            'container_restart_rate': 0.1    # 每小时重启次数
        }
    
    async def analyze_cluster_performance(self):
        """分析集群性能"""
        analysis_report = {
            'timestamp': datetime.now().isoformat(),
            'cluster_info': await self.get_cluster_info(),
            'performance_metrics': {},
            'bottlenecks': [],
            'recommendations': []
        }
        
        # 并行收集各项性能指标
        tasks = [
            self.analyze_api_server_performance(),
            self.analyze_etcd_performance(),
            self.analyze_node_performance(),
            self.analyze_pod_performance(),
            self.analyze_network_performance()
        ]
        
        results = await asyncio.gather(*tasks, return_exceptions=True)
        
        # 处理分析结果
        metric_names = [
            'api_server', 'etcd', 'node', 'pod', 'network'
        ]
        
        for i, result in enumerate(results):
            if isinstance(result, Exception):
                analysis_report['bottlenecks'].append({
                    'component': metric_names[i],
                    'error': str(result),
                    'severity': 'critical'
                })
            else:
                analysis_report['performance_metrics'][metric_names[i]] = result
        
        # 识别性能瓶颈
        analysis_report['bottlenecks'].extend(
            await self.identify_performance_bottlenecks(analysis_report['performance_metrics'])
        )
        
        # 生成优化建议
        analysis_report['recommendations'] = await self.generate_recommendations(
            analysis_report['bottlenecks']
        )
        
        return analysis_report
    
    async def get_cluster_info(self):
        """获取集群基本信息"""
        try:
            nodes = self.core_v1.list_node()
            namespaces = self.core_v1.list_namespace()
            
            return {
                'node_count': len(nodes.items),
                'namespace_count': len(namespaces.items),
                'kubernetes_version': nodes.items[0].status.node_info.kubelet_version if nodes.items else 'unknown',
                'cluster_age': self.calculate_cluster_age()
            }
        except Exception as e:
            return {'error': str(e)}
    
    def calculate_cluster_age(self):
        """计算集群年龄"""
        # 简化实现
        return "30 days"
    
    async def analyze_api_server_performance(self):
        """分析API Server性能"""
        try:
            # 查询Prometheus获取API Server指标
            metrics = await self.query_prometheus_metrics([
                'apiserver_request_duration_seconds',
                'apiserver_request_total',
                'apiserver_current_inflight_requests'
            ])
            
            analysis = {
                'p95_latency': self.calculate_percentile(metrics.get('apiserver_request_duration_seconds', []), 95),
                'p99_latency': self.calculate_percentile(metrics.get('apiserver_request_duration_seconds', []), 99),
                'request_rate': self.calculate_rate(metrics.get('apiserver_request_total', [])),
                'error_rate': self.calculate_error_rate(metrics.get('apiserver_request_total', [])),
                'inflight_requests': metrics.get('apiserver_current_inflight_requests', 0)
            }
            
            # 评估性能
            if analysis['p99_latency'] > self.performance_thresholds['api_server_p99_latency']:
                analysis['status'] = 'degraded'
                analysis['issues'] = ['High API server latency']
            else:
                analysis['status'] = 'healthy'
            
            return analysis
            
        except Exception as e:
            return {'status': 'error', 'error': str(e)}
    
    async def analyze_etcd_performance(self):
        """分析Etcd性能"""
        try:
            metrics = await self.query_prometheus_metrics([
                'etcd_disk_backend_commit_duration_seconds',
                'etcd_network_peer_round_trip_time_seconds',
                'etcd_mvcc_db_total_size_in_bytes'
            ])
            
            analysis = {
                'commit_latency_p99': self.calculate_percentile(
                    metrics.get('etcd_disk_backend_commit_duration_seconds', []), 99
                ),
                'peer_rtt_p99': self.calculate_percentile(
                    metrics.get('etcd_network_peer_round_trip_time_seconds', []), 99
                ),
                'db_size_bytes': metrics.get('etcd_mvcc_db_total_size_in_bytes', 0),
                'fragmentation_ratio': await self.calculate_etcd_fragmentation()
            }
            
            # 评估性能
            if analysis['commit_latency_p99'] > self.performance_thresholds['etcd_commit_latency']:
                analysis['status'] = 'degraded'
                analysis['issues'] = ['High etcd commit latency']
            else:
                analysis['status'] = 'healthy'
            
            return analysis
            
        except Exception as e:
            return {'status': 'error', 'error': str(e)}
    
    async def analyze_node_performance(self):
        """分析节点性能"""
        try:
            nodes = self.core_v1.list_node()
            node_metrics = []
            
            for node in nodes.items:
                node_name = node.metadata.name
                metrics = await self.get_node_metrics(node_name)
                node_metrics.append(metrics)
            
            analysis = {
                'avg_cpu_utilization': np.mean([m['cpu_utilization'] for m in node_metrics]),
                'avg_memory_utilization': np.mean([m['memory_utilization'] for m in node_metrics]),
                'high_utilization_nodes': [
                    m for m in node_metrics 
                    if (m['cpu_utilization'] > self.performance_thresholds['node_cpu_utilization'] or
                        m['memory_utilization'] > self.performance_thresholds['node_memory_utilization'])
                ]
            }
            
            # 评估性能
            if analysis['high_utilization_nodes']:
                analysis['status'] = 'warning'
                analysis['issues'] = [f"{len(analysis['high_utilization_nodes'])} nodes with high resource utilization"]
            else:
                analysis['status'] = 'healthy'
            
            return analysis
            
        except Exception as e:
            return {'status': 'error', 'error': str(e)}
    
    async def analyze_pod_performance(self):
        """分析Pod性能"""
        try:
            # 获取Pod启动时间和重启率
            pods = self.core_v1.list_pod_for_all_namespaces()
            
            startup_times = []
            restart_rates = []
            
            for pod in pods.items:
                if pod.status.container_statuses:
                    for container_status in pod.status.container_statuses:
                        # 计算启动时间
                        if container_status.state.running and container_status.state.running.started_at:
                            start_time = container_status.state.running.started_at
                            creation_time = pod.metadata.creation_timestamp
                            startup_time = (start_time - creation_time).total_seconds()
                            startup_times.append(startup_time)
                        
                        # 计算重启率
                        restart_rates.append(container_status.restart_count)
            
            analysis = {
                'avg_startup_time': np.mean(startup_times) if startup_times else 0,
                'p95_startup_time': np.percentile(startup_times, 95) if startup_times else 0,
                'total_restarts': sum(restart_rates),
                'avg_restart_rate': np.mean(restart_rates) if restart_rates else 0
            }
            
            # 评估性能
            issues = []
            if analysis['p95_startup_time'] > self.performance_thresholds['pod_startup_time']:
                issues.append('Slow pod startup times')
            
            if analysis['avg_restart_rate'] > self.performance_thresholds['container_restart_rate']:
                issues.append('High container restart rates')
            
            analysis['status'] = 'degraded' if issues else 'healthy'
            analysis['issues'] = issues
            
            return analysis
            
        except Exception as e:
            return {'status': 'error', 'error': str(e)}
    
    async def analyze_network_performance(self):
        """分析网络性能"""
        try:
            # 查询网络相关指标
            metrics = await self.query_prometheus_metrics([
                'container_network_receive_bytes_total',
                'container_network_transmit_bytes_total',
                'container_network_receive_packets_dropped_total',
                'container_network_transmit_packets_dropped_total'
            ])
            
            analysis = {
                'total_bandwidth': self.calculate_network_bandwidth(metrics),
                'packet_drop_rate': self.calculate_packet_drop_rate(metrics),
                'network_errors': await self.check_network_errors()
            }
            
            # 评估性能
            if analysis['packet_drop_rate'] > 0.01:  # 1%丢包率阈值
                analysis['status'] = 'degraded'
                analysis['issues'] = ['High packet drop rate']
            else:
                analysis['status'] = 'healthy'
            
            return analysis
            
        except Exception as e:
            return {'status': 'error', 'error': str(e)}
    
    async def identify_performance_bottlenecks(self, metrics):
        """识别性能瓶颈"""
        bottlenecks = []
        
        # API Server瓶颈
        api_metrics = metrics.get('api_server', {})
        if api_metrics.get('status') == 'degraded':
            bottlenecks.append({
                'component': 'api_server',
                'issue': 'latency',
                'severity': 'high',
                'impact': 'cluster-wide'
            })
        
        # Etcd瓶颈
        etcd_metrics = metrics.get('etcd', {})
        if etcd_metrics.get('status') == 'degraded':
            bottlenecks.append({
                'component': 'etcd',
                'issue': 'commit_latency',
                'severity': 'critical',
                'impact': 'data_consistency'
            })
        
        # 节点瓶颈
        node_metrics = metrics.get('node', {})
        if node_metrics.get('high_utilization_nodes'):
            bottlenecks.append({
                'component': 'nodes',
                'issue': 'resource_exhaustion',
                'severity': 'medium',
                'impact': 'scheduling_performance'
            })
        
        return bottlenecks
    
    async def generate_recommendations(self, bottlenecks):
        """生成优化建议"""
        recommendations = []
        
        for bottleneck in bottlenecks:
            if bottleneck['component'] == 'api_server':
                recommendations.append({
                    'priority': 'high',
                    'category': 'api_server_scaling',
                    'description': 'Scale up API server instances',
                    'actions': [
                        'Increase API server replicas',
                        'Optimize request handling',
                        'Implement request caching'
                    ]
                })
            
            elif bottleneck['component'] == 'etcd':
                recommendations.append({
                    'priority': 'critical',
                    'category': 'etcd_optimization',
                    'description': 'Optimize etcd performance',
                    'actions': [
                        'Upgrade to faster storage',
                        'Tune etcd parameters',
                        'Implement etcd defragmentation'
                    ]
                })
            
            elif bottleneck['component'] == 'nodes':
                recommendations.append({
                    'priority': 'medium',
                    'category': 'resource_management',
                    'description': 'Address node resource constraints',
                    'actions': [
                        'Add more worker nodes',
                        'Implement resource quotas',
                        'Optimize pod resource requests'
                    ]
                })
        
        return recommendations
    
    # 辅助方法（简化实现）
    async def query_prometheus_metrics(self, metric_names):
        """查询Prometheus指标"""
        # 简化实现，返回模拟数据
        return {name: [] for name in metric_names}
    
    def calculate_percentile(self, data, percentile):
        """计算百分位数"""
        return np.percentile(data, percentile) if data else 0
    
    def calculate_rate(self, data):
        """计算速率"""
        return len(data) / 300 if data else 0  # 假设5分钟窗口
    
    def calculate_error_rate(self, data):
        """计算错误率"""
        return 0.05  # 模拟值
    
    async def calculate_etcd_fragmentation(self):
        """计算etcd碎片率"""
        return 0.15  # 模拟值
    
    async def get_node_metrics(self, node_name):
        """获取节点指标"""
        return {
            'node_name': node_name,
            'cpu_utilization': np.random.uniform(0.3, 0.9),
            'memory_utilization': np.random.uniform(0.4, 0.85)
        }
    
    def calculate_network_bandwidth(self, metrics):
        """计算网络带宽"""
        return 1000000000  # 模拟值 1Gbps
    
    def calculate_packet_drop_rate(self, metrics):
        """计算丢包率"""
        return 0.005  # 模拟值 0.5%
    
    async def check_network_errors(self):
        """检查网络错误"""
        return 0  # 模拟值

# 使用示例
async def main():
    analyzer = PerformanceAnalyzer()
    report = await analyzer.analyze_cluster_performance()
    
    print("Performance Analysis Report:")
    print(json.dumps(report, indent=2, ensure_ascii=False))

if __name__ == "__main__":
    asyncio.run(main())
```

<!-- chunk: 🔧 实施检查清单 -->## 🔧 实施检查清单

## 基础性能优化
- [ ] 优化操作系统内核参数
- [ ] 配置网络性能参数
- [ ] 优化文件系统和存储
- [ ] 调整内存管理参数
- [ ] 优化容器运行时配置
- [ ] 实施资源限制和QoS策略

## 组件性能调优
- [ ] 优化API Server性能参数
- [ ] 调整Etcd存储和网络配置
- [ ] 优化Controller Manager并发设置
- [ ] 配置Scheduler调度算法
- [ ] 实施组件资源配额
- [ ] 配置健康检查和探针

## 监控和诊断
- [ ] 部署性能监控系统
- [ ] 配置关键性能指标收集
- [ ] 建立性能基线和阈值
- [ ] 实施自动化性能分析
- [ ] 建立性能瓶颈诊断流程
- [ ] 定期进行性能评估和优化

---

*本文档为企业级Kubernetes集群性能调优提供完整的优化方案和实施指导*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-11-production-operations MOC
- [[domain-11-production-operations/README.md|Domain 11: 生产环境运维最佳实践 (Production Operations Best Practices)]]
- Domain-18 生产运维 — 开源项目索引
- [[domain-01-cluster-fundamentals/01-production-architecture-design-principles.md|01-生产架构设计原则]]
- 02-多云混合部署策略
- 03-边缘计算生产部署
- 04-企业级监控体系
- 05-日志收集分析平台
- 06-APM应用性能监控
- 07-零信任安全架构
- 08-CIS基准合规检查
- 09-软件物料清单

## See Also

- 17-disaster-recovery-drills
- 18-cross-region-disaster-recovery
- 20-network-performance-optimization
- 21-storage-performance-optimization

## Related

- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/openkruise-index.md|OpenKruise 全局索引]]
- [[domain-19-landscape-references/topic-index/node-index.md|Node 知识图谱索引]]
