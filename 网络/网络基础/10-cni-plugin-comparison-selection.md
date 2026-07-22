---
title: CNI Plugin Comparison & Selection Guide
description: CNI 插件对比与选型 — Calico/Cilium/Flannel/Weave 架构对比、性能基准、选型决策矩阵
summary: Kubernetes CNI 网络插件全面对比，涵盖架构、性能、安全、运维维度选型指南
category: reference
tags:
- cni
- calico
- cilium
- flannel
- network-plugin
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: networking
---
# CNI 插件对比与选型指南

> Kubernetes 网络插件的架构对比、性能基准与生产选型决策。

## CNI 插件全景

| 插件 | 数据平面 | 网络策略 | 加密 | 维护方 |
|------|----------|----------|------|--------|
| Cilium | eBPF | L3-L7 | 支持(WireGuard) | Isovalent/CNCF |
| Calico | iptables/eBPF | L3-L4 | 支持(WireGuard) | Tigera/CNCF |
| Flannel | VXLAN/host-gw | 无 | 无 | CNCF |
| Weave | 网桥/VXLAN | L3-L4 | 支持 | Weaveworks |
| Antrea | OVS | L3-L4 | 支持 | VMware/CNCF |
| Kube-OVN | OVS | L3-L4 | 支持 | 灵雀云 |

## 架构深度对比

### Cilium（eBPF 数据平面）

```
┌─────────────────────────────────────────┐
│              Node                        │
│  ┌─────┐  ┌─────┐  ┌─────┐            │
│  │Pod A│  │Pod B│  │Pod C│            │
│  └──┬──┘  └──┬──┘  └──┬──┘            │
│     │        │        │                 │
│  ┌──▼────────▼────────▼──────────────┐  │
│  │     eBPF Programs (TC/XDP)        │  │
│  │  ┌────────┐ ┌────────┐ ┌──────┐  │  │
│  │  │Policy  │ │LB/SNAT│ │Conntrk│  │  │
│  │  │Enforce │ │        │ │      │  │  │
│  │  └────────┘ └────────┘ └──────┘  │  │
│  └───────────────────────────────────┘  │
│              │                           │
│  ┌───────────▼───────────────────────┐  │
│  │         Physical NIC              │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

**优势：**
- 无 iptables 规则膨胀（O(1) 查找 vs O(n)）
- 内核级包处理（零拷贝）
- L7 可观测性（HTTP/gRPC/Kafka 协议感知）
- 无 kube-proxy 依赖

### Calico（BGP 路由模式）

```
┌─────────────────────────────────────────┐
│              Node                        │
│  ┌─────┐  ┌─────┐                      │
│  │Pod A│  │Pod B│                      │
│  └──┬──┘  └──┬──┘                      │
│     │        │                          │
│  ┌──▼────────▼──┐                       │
│  │  veth pair   │                       │
│  └──────┬───────┘                       │
│         │                               │
│  ┌──────▼───────────────────────────┐   │
│  │  iptables/nftables (Felix)       │   │
│  │  或 eBPF (Calico eBPF mode)     │   │
│  └──────────────┬───────────────────┘   │
│                 │                        │
│  ┌──────────────▼───────────────────┐   │
│  │  BIRD BGP Agent                  │   │
│  │  (纯 L3 路由，无 Overlay)        │   │
│  └──────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

**优势：**
- BGP 纯路由（无封装开销）
- 成熟稳定（10+ 年生产验证）
- 与物理网络深度集成
- 支持 eBPF 模式（可选）

### Flannel（简单 Overlay）

```
┌─────────────────────────────────────────┐
│              Node                        │
│  ┌─────┐  ┌─────┐                      │
│  │Pod A│  │Pod B│                      │
│  └──┬──┘  └──┬──┘                      │
│     │        │                          │
│  ┌──▼────────▼──┐                       │
│  │  cni0 bridge │                       │
│  └──────┬───────┘                       │
│         │                               │
│  ┌──────▼───────────────────────────┐   │
│  │  flanneld (VXLAN/host-gw)       │   │
│  └──────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

**优势：**
- 极简部署（单二进制）
- 适合开发/测试/小规模
- 无策略引擎（轻量）

## 性能基准对比

### 网络延迟（Pod-to-Pod，同节点）

| 插件 | 模式 | P50 延迟 | P99 延迟 |
|------|------|----------|----------|
| Cilium | eBPF | ~25μs | ~50μs |
| Calico | BGP | ~30μs | ~60μs |
| Calico | iptables | ~40μs | ~100μs |
| Flannel | VXLAN | ~50μs | ~120μs |
| Flannel | host-gw | ~30μs | ~60μs |

### 吞吐量（iperf3，10Gbps NIC）

| 插件 | 模式 | 吞吐量 | CPU 开销 |
|------|------|--------|----------|
| Cilium | eBPF | ~9.5 Gbps | 低 |
| Calico | BGP | ~9.2 Gbps | 低 |
| Calico | iptables | ~8.5 Gbps | 中 |
| Flannel | VXLAN | ~7.5 Gbps | 中-高 |

### Service 扩展性（iptables vs eBPF）

| Service 数量 | iptables 延迟 | eBPF 延迟 |
|-------------|--------------|-----------|
| 100 | ~1ms | ~0.1ms |
| 1,000 | ~10ms | ~0.1ms |
| 10,000 | ~100ms | ~0.1ms |
| 50,000 | ~500ms（不可用） | ~0.2ms |

## 选型决策矩阵

### 按场景推荐

| 场景 | 推荐 | 理由 |
|------|------|------|
| 大规模生产（>1000 Pod） | Cilium | eBPF 性能、可观测性 |
| 企业级安全合规 | Calico | 成熟、BGP 集成、企业支持 |
| 开发/测试/小规模 | Flannel | 简单、轻量 |
| 需要 L7 策略 | Cilium | HTTP/gRPC 级别策略 |
| 裸金属/物理网络 | Calico BGP | 无 Overlay、路由集成 |
| 多云/混合云 | Cilium ClusterMesh | 跨集群网络 |
| 国内私有云 | Kube-OVN | 中文社区、OVS 功能丰富 |

### 决策流程图

```
需要 L7 策略或深度可观测？
├── 是 → Cilium
└── 否 → 需要 BGP 与物理网络集成？
         ├── 是 → Calico (BGP mode)
         └── 否 → 集群规模 > 500 节点？
                  ├── 是 → Cilium 或 Calico (eBPF)
                  └── 否 → 需要网络策略？
                           ├── 是 → Calico
                           └── 否 → Flannel (最简)
```

## 迁移指南

### Flannel → Cilium 迁移

```bash
# 1. 预检
cilium status --wait
cilium connectivity test

# 2. 安装 Cilium（不删除 Flannel）
helm install cilium cilium/cilium \
  --namespace kube-system \
  --set cni.chainingMode=generic-veth \
  --set cni.customConf=true

# 3. 验证连通性
cilium connectivity test --multi-cluster

# 4. 移除 Flannel
kubectl delete -n kube-system ds kube-flannel-ds
kubectl delete -n kube-system cm kube-flannel-cfg

# 5. 切换到 Cilium 独立模式
helm upgrade cilium cilium/cilium \
  --namespace kube-system \
  --set cni.chainingMode=none
```

## 生产配置最佳实践

### Cilium 生产配置

```yaml
# cilium-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: cilium-config
  namespace: kube-system
data:
  # 启用 eBPF 替代 kube-proxy
  kube-proxy-replacement: "true"
  # 启用 Hubble 可观测
  enable-hubble: "true"
  hubble-listen-address: ":4244"
  # 启用 Bandwidth Manager
  enable-bandwidth-manager: "true"
  # BPF Map 大小（大规模集群）
  bpf-ct-global-tcp-max: "524288"
  bpf-ct-global-any-max: "262144"
  # 启用 WireGuard 加密
  enable-encryption: "true"
  encryption: "wireguard"
```

## 故障排查表

| 问题现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|----------|
| Pod 无法获取 IP | CNI 插件未就绪 | `kubectl get pods -n kube-system -l k8s-app=cilium` | 重启 CNI DaemonSet |
| 跨节点 Pod 不通 | 路由/隧道配置错误 | `ip route show` / `cilium status` | 检查节点间路由 |
| Service 无法访问 | kube-proxy/eBPF 异常 | `iptables -t nat -L -n` / `cilium service list` | 重启 kube-proxy 或 Cilium |
| 网络策略不生效 | 策略引擎未启用 | `kubectl get networkpolicy -A` | 确认 CNI 支持 NetworkPolicy |
| DNS 解析失败 | CoreDNS 网络不通 | `kubectl exec -it <pod> -- nslookup kubernetes.default` | 检查 Pod 到 CoreDNS 连通性 |
| 高延迟/丢包 | MTU 不匹配 | `ping -s 1472 -M do <target>` | 调整 CNI MTU 配置 |

## 监控指标

### Cilium 关键指标

```yaml
# PrometheusRule - Cilium 告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cilium-alerts
  namespace: monitoring
spec:
  groups:
  - name: cilium.rules
    rules:
    - alert: CiliumAgentDown
      expr: up{job="cilium"} == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Cilium Agent 不可用"
        description: "节点 {{ $labels.instance }} 的 Cilium Agent 已停止 5 分钟"
    - alert: CiliumEndpointNotReady
      expr: cilium_endpoint_state{state="ready"} == 0
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Cilium Endpoint 未就绪"
    - alert: CiliumPolicyDropsHigh
      expr: rate(cilium_drop_count_total{reason="POLICY_DENIED"}[5m]) > 100
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "策略拒绝流量过高"
```

### Calico 关键指标

```bash
# 🟢 低风险：查看 Calico 节点状态
calicoctl node status

# 🟢 低风险：查看 BGP Peer 状态
calicoctl get bgppeer -o wide

# 🟢 低风险：查看 Felix 指标
curl -s http://localhost:9091/metrics | grep felix

# 🟢 低风险：查看 iptables 规则数量
iptables -t filter -L -n | wc -l
```

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| **版本管理** | 使用 Helm 管理 CNI | 便于升级和回滚 |
| **资源预留** | 为 CNI Pod 设置 requests/limits | 避免资源竞争 |
| **日志级别** | 生产环境使用 INFO | DEBUG 仅用于排障 |
| **健康检查** | 配置 liveness/readiness probe | 自动恢复异常节点 |
| **监控告警** | 部署 Prometheus + Grafana | 实时掌握网络状态 |
| **备份配置** | 定期备份 CNI ConfigMap | 便于灾难恢复 |
| **升级策略** | 滚动升级，先测试环境 | 避免全集群故障 |

## 升级与回滚

### Cilium 升级

```bash
# 🟡 中风险：升级 Cilium
# 1. 备份当前配置
kubectl get cm -n kube-system cilium-config -o yaml > cilium-config-backup.yaml

# 2. 升级（使用 Helm）
helm upgrade cilium cilium/cilium \
  --namespace kube-system \
  --version 1.15.0 \
  --reuse-values

# 3. 验证升级
kubectl rollout status ds/cilium -n kube-system
cilium status --wait

# 4. 连通性测试
cilium connectivity test
```

### 回滚操作

```bash
# 🔴 高风险：回滚 CNI（可能导致短暂网络中断）
# 1. 回滚 Helm Release
helm rollback cilium <revision> -n kube-system

# 2. 重启所有 CNI Pod
kubectl rollout restart ds/cilium -n kube-system

# 3. 验证回滚
kubectl get pods -n kube-system -l k8s-app=cilium
cilium status
```

## 容量规划

### 节点规模与 CNI 选择

| 集群规模 | 推荐 CNI | 配置要点 |
|---------|---------|----------|
| < 50 节点 | Flannel/Calico | 默认配置即可 |
| 50-500 节点 | Calico/Cilium | 调整 BPF Map 大小 |
| 500-2000 节点 | Cilium | 启用 eBPF，禁用 kube-proxy |
| > 2000 节点 | Cilium | 集群分片，ClusterMesh 联邦 |

### IP 地址规划

```yaml
# 示例：大规模集群 IP 规划
# Pod CIDR: 10.0.0.0/8 (16M IPs)
# 每节点 /24 (254 Pods)
# 最多支持 65536 节点

# Service CIDR: 172.16.0.0/12 (1M IPs)
# 每 Service 1 IP

# 节点 CIDR: 192.168.0.0/16 (65K IPs)
# 每节点 1 IP
```

## Related

- [[网络/eBPF/index.md|eBPF 网络]]
- [[网络/网络基础/index.md|网络基础]]
- [[网络/服务网格/index.md|Service Mesh]]
