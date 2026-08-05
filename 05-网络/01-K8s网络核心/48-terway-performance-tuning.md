---
title: Terway 性能调优
description: '## 概述'
summary: 'Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]'
category: entities
tags:
- k8s
- networking
- terway
- cni
- alicloud
- prometheus
- grafana
- cilium
- networkpolicy
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Terway 性能调优 是什么
- 如何 Terway 性能调优
trigger_keywords:
- Terway
- 性能调优
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Terway 性能调优

> **所属项目**: Terway (阿里云 ACK CNI 插件) | **适用版本**: ACK v1.25+

## 概述

title: 06 - Terway 性能调优 (Performance Tuning)

## 技术细节

### 网络模式性能对比

| 模式 | 吞吐量 (相对物理机) | 延迟 | 密度 | 适用场景 |
|-----|------------------|------|------|----------|
| **VPC** | ~70% | 较高 | 低 | 兼容性优先 |
| **ENI 独占** | ~95% | 最低 | 低 | 高性能/数据库 |
| **ENI 多 IP (ENIIP)** | ~90% | 低 | 高 | 通用工作负载 |
| **IPVlan** | ~95% | 最低 | 高 | 高性能 + 高密度 |

### 内核参数调优

#### 网络栈优化

```bash
# 🟡 中风险：内核参数调优 (需重启或 sysctl -p)
cat >> /etc/sysctl.d/99-terway.conf <<EOF
# TCP 优化
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_fin_timeout = 30
net.ipv4.tcp_keepalive_time = 600
net.ipv4.tcp_keepalive_intvl = 30
net.ipv4.tcp_keepalive_probes = 10

# 连接队列
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.core.netdev_max_backlog = 65535

# 缓冲区
net.core.rmem_max = 16777216
net.core.wmem_max = 16777216
net.ipv4.tcp_rmem = 4096 87380 16777216
net.ipv4.tcp_wmem = 4096 65536 16777216

# 连接跟踪
net.netfilter.nf_conntrack_max = 1048576
net.netfilter.nf_conntrack_tcp_timeout_established = 86400

# 文件描述符
fs.file-max = 2097152
fs.nr_open = 2097152
EOF

sysctl -p /etc/sysctl.d/99-terway.conf
```

#### 验证内核参数

```bash
# 🟢 低风险：验证内核参数
sysctl net.ipv4.tcp_tw_reuse
sysctl net.core.somaxconn
sysctl net.netfilter.nf_conntrack_max
```

### ENI 配置优化

#### 多队列配置

```bash
# 🟢 低风险：查看 ENI 队列数
ethtool -l eth0

# 🟡 中风险：设置队列数 (需实例支持)
ethtool -L eth0 combined 8

# 🟢 低风险：查看中断分布
cat /proc/interrupts | grep virtio
```

#### 中断亲和性优化

```bash
#!/bin/bash
# 🟡 中风险：中断亲和性优化脚本
set -euo pipefail

INTERFACE=${1:-eth0}

# 获取 CPU 数量
CPU_COUNT=$(nproc)

# 获取中断号
IRQS=$(grep $INTERFACE /proc/interrupts | awk '{print $1}' | tr -d ':')

# 分布中断到不同 CPU
i=0
for irq in $IRQS; do
  cpu=$((i % CPU_COUNT))
  echo "设置 IRQ $irq 到 CPU $cpu"
  echo $cpu > /proc/irq/$irq/smp_affinity_list
  i=$((i + 1))
done

echo "中断亲和性优化完成"
```

### IP 池调优

#### 调整 IP 池大小

```yaml
# eni-config ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "max_pool_size": 10,       # 增大 IP 池 (默认 5)
      "min_pool_size": 2,        # 最小预热 IP 数
      "vswitches": {
        "cn-hangzhou-h": ["vsw-bp1234567890abcdef"]
      },
      "security_group": "sg-bp1234567890abcdef",
      "enable_trunk": false,
      "enable_ipvlan": true,     # 启用 IPVlan 提升性能
      "enable_ebpf": true
    }
```

```bash
# 🟡 中风险：应用配置
kubectl apply -f eni-config.yaml

# 重启 Terway 使配置生效
kubectl rollout restart ds/terway-eniip -n kube-system
```

### IPVlan 模式启用

#### 前提条件

- 内核版本 >= 4.19
- 实例规格支持 (ecs.g6/ecs.c6 及以上)

#### 启用 IPVlan

```yaml
# eni-config ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "enable_ipvlan": true,     # 启用 IPVlan
      "enable_ebpf": true,       # 配合 eBPF
      "vswitches": {
        "cn-hangzhou-h": ["vsw-bp1234567890abcdef"]
      }
    }
```

```bash
# 🟡 中风险：应用配置并重启
kubectl apply -f eni-config.yaml
kubectl rollout restart ds/terway-eniip -n kube-system

# 🟢 低风险：验证 IPVlan 模式
kubectl exec -it <pod> -- ip link show
# 应看到 ipvlan 类型的接口
```

### eBPF 加速

#### 启用 eBPF

```yaml
# eni-config ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "enable_ebpf": true,       # 启用 eBPF
      "enable_ipvlan": true
    }
```

#### eBPF 优势

| 特性 | 传统 iptables | eBPF |
|-----|--------------|------|
| 性能 | 规则数线性下降 | 恒定高性能 |
| NetworkPolicy | 支持 | 支持 (更高效) |
| 连接跟踪 | 内核 conntrack | eBPF map |
| 可观测性 | 有限 | 丰富 (bpftrace) |

### 性能基准测试

#### 测试脚本

```bash
#!/bin/bash
# 🟢 低风险：Terway 性能基准测试
set -euo pipefail

NAMESPACE=${1:-default}
SERVER_IP=${2:-}

echo "=== Terway 性能基准测试 ==="

if [ -z "$SERVER_IP" ]; then
  echo "创建 iperf3 服务端..."
  kubectl run iperf-server --image=networkstatic/iperf3 -n $NAMESPACE --restart=Never -- -s
  kubectl wait --for=condition=Ready pod/iperf-server -n $NAMESPACE --timeout=60s
  SERVER_IP=$(kubectl get pod iperf-server -n $NAMESPACE -o jsonpath='{.status.podIP}')
fi

echo "服务端 IP: $SERVER_IP"

# 1. TCP 带宽测试
echo "[1] TCP 带宽测试..."
kubectl run iperf-client-tcp --image=networkstatic/iperf3 -n $NAMESPACE --rm -it --restart=Never -- \
  -c $SERVER_IP -t 30 -P 4

# 2. UDP 带宽测试
echo "[2] UDP 带宽测试..."
kubectl run iperf-client-udp --image=networkstatic/iperf3 -n $NAMESPACE --rm -it --restart=Never -- \
  -c $SERVER_IP -u -b 10G -t 30

# 3. 延迟测试
echo "[3] 延迟测试..."
kubectl run ping-test --image=nicolaka/netshoot -n $NAMESPACE --rm -it --restart=Never -- \
  ping -c 100 $SERVER_IP

# 清理
kubectl delete pod iperf-server -n $NAMESPACE 2>/dev/null || true

echo "=== 测试完成 ==="
```

#### 预期性能指标

| 测试项 | ENI 独占 | ENIIP | IPVlan | 单位 |
|-------|---------|-------|--------|------|
| TCP 吞吐 | 25+ | 20+ | 25+ | Gbps |
| UDP 吞吐 | 25+ | 20+ | 25+ | Gbps |
| 延迟 (同 AZ) | < 0.5 | < 0.8 | < 0.5 | ms |
| PPS | 10M+ | 8M+ | 10M+ | packets/s |

### 调优检查清单

| 序号 | 检查项 | 命令 | 优化建议 |
|-----|--------|------|----------|
| 1 | 内核参数 | `sysctl -a` | 按上文调优 |
| 2 | ENI 队列数 | `ethtool -l eth0` | 设置为 CPU 核数 |
| 3 | 中断亲和性 | `cat /proc/interrupts` | 均匀分布 |
| 4 | IP 池大小 | 检查 eni-config | 根据 Pod 密度调整 |
| 5 | IPVlan 模式 | 检查 eni-config | 内核支持则启用 |
| 6 | eBPF 加速 | 检查 eni-config | 启用 eBPF |
| 7 | MTU 设置 | `ip link show` | 1500 或 9000 (Jumbo) |

## 参考链接

Terway 作为 CNI 插件实现了 Kubernetes 网络模型，通过 ENI 将 Pod 直接接入 VPC 网络，提供与 [[cilium|Cilium]] 类似的高性能网络方案。与 networking.md|eBPF 网络]] 技术结合，可实现更高效的网络策略和流量管理。^[inferred]

## 生产部署建议

- 建议在生产环境中使用 ENI 多 IP 模式以提高 IP 利用率 ^[inferred]
- 密切监控 ENI 资源使用情况，避免 IP 耗尽 ^[inferred]
- 配合 [[networkpolicy|NetworkPolicy]] 实现 Pod 间访问控制 ^[inferred]

## 参考链接

- [[23-实体/07-可观测性/prometheus-grafana.md|prometheus-grafana]]
- [[cilium]]
- [[23-实体/02-K8s核心组件/cni-plugins.md|cni-plugins]]
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]]

## Related

- [[47-terway-testing-validation]] — Terway 测试验证
- [[telepresence]] — Telepresence
- [[23-实体/02-K8s核心组件/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[43-terway-architecture-deep-dive]]
- [[45-terway-crd-operations]]
- [[46-terway-operations-manual]]
- [[42-terway-product-overview]]
- [[44-terway-usage-guide]]
- [[49-terway-troubleshooting-fta]]
- 46-terway-performance-tuning

<!-- risk-assessed -->
