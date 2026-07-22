---
title: Kmesh (entities)
description: '## 概述'
summary: 'Kmesh 是一个基于 eBPF 和可编程内核的无 Sidecar 服务网格，在内核空间实现流量治理能力。与传统 Sidecar 模式（如 Istio/Envoy）不同，Kmesh 将 L4/L7 流量管理逻辑下沉到操作系统内核，消除了 Sidecar 代理带来的额外延迟和资源开销，同时保持与 Istio 控制平面的兼容性。'
category: entities
tags:
- k8s
- cncf
- networking
- kmesh
- istio
- envoy
- cilium
- crd
- operator
- ebpf
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kmesh 是什么
- 如何 Kmesh
trigger_keywords:
- Kmesh
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kmesh

> **CNCF 状态**: Sandbox | **类别**: Networking | **主要语言**: Go, C

## 概述

Kmesh 是由华为开源的基于 eBPF 的无 Sidecar 服务网格，2023 年加入 CNCF Sandbox。与传统 Sidecar 模式（如 Istio/Envoy）不同，Kmesh 将 L4/L7 流量管理逻辑下沉到操作系统内核空间，利用 eBPF 和可编程内核技术在 socket 层和 cgroup 层实现流量治理，消除了 Sidecar 代理带来的额外延迟和资源开销，同时保持与 Istio 控制平面的兼容性。

## 核心特性

- **无 Sidecar**: eBPF 内核级流量治理，无需注入 Sidecar 容器
- **低延迟**: 内核空间处理，消除用户态代理的上下文切换开销
- **L4 治理**: 基于 sockmap 和 skmsg 实现 TCP 层流量路由
- **L7 治理**: 通过 waypoint 代理实现 HTTP/gRPC 层路由
- **Istio 兼容**: 复用 Istio 控制平面（istiod）和 xDS API
- **混合模式**: 可与 Istio Sidecar 共存，按命名空间选择模式

## 架构

Kmesh 分为数据平面和控制平面。数据平面在节点内核中运行 eBPF 程序：kmesh-cni 在容器创建时设置 cgroup 和 socket 映射；sockmap eBPF 程序在内核 TCP 栈中拦截和路由流量； waypoint 代理（Envoy）处理需要 L7 治理的流量。控制平面 kmesh-daemon 从 istiod 接收 xDS 配置（监听器、集群、路由），编译为 eBPF map 配置注入内核。L4 流量在内核直接处理，L7 流量重定向到 waypoint。无需为每个 Pod 注入 Sidecar，大幅减少资源消耗。

## Kubernetes 集成

Kmesh 通过 DaemonSet 部署在每个节点上，以特权模式运行以加载 eBPF 程序。与 Istio 控制平面（istiod）集成，复用 VirtualService、DestinationRule 等 CRD 定义治理策略。通过命名空间标签（如 `istio.io/dataplane-mode=kmesh`）选择启用 Kmesh 而非 Sidecar 模式。支持标准的 Kubernetes Service 和 NetworkPolicy。

## 生产使用场景

1. **高性能服务网格**: 对延迟敏感的场景（如金融交易），消除 Sidecar 开销
2. **大规模集群**: 减少 Sidecar 带来的内存和 CPU 消耗（每 Pod 节省 ~100MB）
3. **渐进迁移**: 从 Sidecar 模式逐步迁移到无 Sidecar 模式
4. **边缘计算**: 资源受限场景下使用轻量级网格能力

## 安装与配置

```bash
# 前置: 确保 Istio 控制平面已安装 (>= 1.18)
istioctl install --set profile=minimal \
  --set meshConfig.defaultConfig.proxy.privileged=true

# 安装 Kmesh
kubectl apply -f https://raw.githubusercontent.com/kmesh-net/kmesh/main/deploy/yaml/kmesh.yaml

# 验证安装
kubectl get pods -n kube-system -l app=kmesh
kubectl get pods -n istio-system

# 启用命名空间的 Kmesh 模式
kubectl label namespace default istio.io/dataplane-mode=kmesh

# 验证 eBPF 程序加载
kubectl exec -n kube-system <kmesh-pod> -- bpftool prog list
kubectl exec -n kube-system <kmesh-pod> -- bpftool map list
```

```yaml
# Kmesh 配置示例 (kmesh-config ConfigMap)
apiVersion: v1
kind: ConfigMap
metadata:
  name: kmesh-config
  namespace: kube-system
data:
  kmesh.json: |
    {
      "serviceCluster": "kmesh",
      "concurrency": 2,
      "enableL7": true,
      "waypointPort": 15019,
      "bpfLogLevel": "info"
    }
```

```yaml
# 使用 Istio VirtualService 配置流量治理 (Kmesh 兼容)
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: reviews-route
  namespace: default
spec:
  hosts:
    - reviews.default.svc.cluster.local
  http:
    - match:
        - headers:
            x-canary:
              exact: "true"
      route:
        - destination:
            host: reviews
            subset: v2
    - route:
        - destination:
            host: reviews
            subset: v1
          weight: 90
        - destination:
            host: reviews
            subset: v2
          weight: 10
---
apiVersion: networking.istio.io/v1beta1
kind: DestinationRule
metadata:
  name: reviews
  namespace: default
spec:
  host: reviews.default.svc.cluster.local
  subsets:
    - name: v1
      labels:
        version: v1
    - name: v2
      labels:
        version: v2
```

## 运维操作

```bash
# 🟢 检查 Kmesh DaemonSet 状态
kubectl get pods -n kube-system -l app=kmesh -o wide
kubectl logs -n kube-system -l app=kmesh --tail=50

# 🟢 检查 eBPF 程序状态
kubectl exec -n kube-system <kmesh-pod> -- bpftool prog list
kubectl exec -n kube-system <kmesh-pod> -- bpftool map list | grep kmesh

# 🟢 检查 xDS 配置同步
kubectl exec -n kube-system <kmesh-pod> -- cat /var/run/kmesh/xds_status.json
kubectl logs -n kube-system <kmesh-pod> | grep -i "xds\|listener\|cluster"

# 🟢 检查命名空间 Kmesh 模式
kubectl get namespaces -l istio.io/dataplane-mode=kmesh

# 🟢 检查 Waypoint 代理状态
kubectl get pods -n istio-system -l app=waypoint
kubectl get gateway -A  # Gateway API waypoint

# 🟢 检查流量统计
kubectl exec -n kube-system <kmesh-pod> -- cat /var/run/kmesh/stats.json

# 🟡 重启 Kmesh DaemonSet (短暂网络中断)
kubectl rollout restart daemonset/kmesh -n kube-system

# 🟡 禁用命名空间的 Kmesh 模式
kubectl label namespace default istio.io/dataplane-mode-
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Kmesh Pod CrashLoop | 内核版本不支持 | `uname -r` (需 >= 5.10) | 升级内核或禁用 Kmesh |
| eBPF 程序加载失败 | 缺少 BPF 文件系统 | `mount \| grep bpf` | `mount -t bpf bpf /sys/fs/bpf` |
| 流量未被拦截 | cgroup 未关联 | 检查 kmesh-cni 日志 | 重启 Kmesh DaemonSet |
| L7 路由不生效 | Waypoint 未部署 | `kubectl get gateway` | 部署 Waypoint 代理 |
| xDS 配置不同步 | istiod 连接失败 | 检查 kmesh 日志 | 检查 istiod Service 可达性 |
| 延迟反而增加 | eBPF map 过大 | 检查 map 大小 | 减少 Service/Endpoint 数量 |

### 排查流程

```
Kmesh 流量治理异常
├── Kmesh Pod 未运行
│   ├── 检查内核版本 (>= 5.10)
│   ├── 检查 BPF 文件系统挂载
│   ├── 检查特权模式权限
│   └── 查看 Pod Events 和日志
├── Pod 运行但流量未治理
│   ├── 确认命名空间标签 istio.io/dataplane-mode=kmesh
│   ├── 检查 eBPF 程序是否加载 (bpftool prog list)
│   ├── 检查 cgroup 关联 (kmesh-cni 日志)
│   └── 检查 xDS 配置同步状态
└── L7 治理不生效
    ├── 确认 Waypoint 代理已部署
    ├── 检查 VirtualService/DestinationRule 配置
    ├── 检查 Waypoint 日志
    └── 确认流量被重定向到 Waypoint 端口
```

## 生产案例

### 案例 1: 金融交易系统消除 Sidecar 延迟

- **场景**: 微服务交易系统使用 Istio Sidecar，P99 延迟增加 3ms，不满足 SLA
- **排查**: 火焰图显示 Sidecar 上下文切换占用 2ms+；每 Pod Envoy 占用 128MB 内存
- **方案**: 迁移到 Kmesh 无 Sidecar 模式；L4 流量内核直接处理；保留 Istio 控制平面管理策略
- **效果**: P99 延迟降低 2.5ms；每 Pod 节省 128MB 内存；集群总内存节省 40%

### 案例 2: 大规模集群 Sidecar 资源优化

- **场景**: 2000+ Pod 集群，Sidecar 总资源占用 256 vCPU + 256GB 内存
- **排查**: 监控显示 Sidecar CPU 利用率平均 <5%，但资源已预留
- **方案**: 按命名空间渐进迁移到 Kmesh；先迁移无 L7 需求的服务；L7 服务使用共享 Waypoint
- **效果**: 资源占用降低 80%；节点可调度资源增加；无需修改业务代码

## 内核要求与兼容性

| 内核版本 | 支持状态 | 功能 |
|----------|----------|------|
| < 5.4 | ❌ 不支持 | - |
| 5.4 - 5.9 | ⚠️ 部分 | 基础 L4 (无 sockmap) |
| 5.10 - 5.15 | ✅ 完整 | L4 + cgroup + sockmap |
| >= 6.1 | ✅ 推荐 | 全部功能 + 性能优化 |

## 对比与替代方案

| 维度 | Kmesh | Cilium Mesh | Istio Ambient | Sidecar (Envoy) |
|------|-------|-------------|---------------|------------------|
| 数据面 | eBPF 内核 | eBPF 内核 | ztunnel+waypoint | 用户态代理 |
| L4 治理 | ✅ 内核级 | ✅ 内核级 | ✅ ztunnel | ✅ |
| L7 治理 | ✅ Waypoint | ⚠️ 有限 | ✅ Waypoint | ✅ 完整 |
| 资源开销 | 极低 | 低 | 中 | 高 |
| 延迟增加 | <0.5ms | <0.5ms | ~1ms | 2-5ms |
| 内核要求 | >= 5.10 | >= 4.19 | 无特殊 | 无特殊 |
| 成熟度 | 早期 | 成熟 | 中期 | 最成熟 |
| Istio 兼容 | ✅ 完整 | 部分 | ✅ 原生 | ✅ 原生 |

## 检查清单

- [ ] 节点内核版本 >= 5.10
- [ ] BPF 文件系统已挂载 (/sys/fs/bpf)
- [ ] Kmesh DaemonSet 所有 Pod Running
- [ ] eBPF 程序成功加载 (bpftool prog list)
- [ ] istiod 控制平面正常运行
- [ ] xDS 配置同步正常
- [ ] 目标命名空间已标记 istio.io/dataplane-mode=kmesh
- [ ] Waypoint 代理已部署 (L7 治理)
- [ ] 流量治理策略验证通过
- [ ] 监控覆盖 Kmesh 组件状态

## 参考链接

- [[istio]]
- [[cilium]]
- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[pod-lifecycle]]

## Related

- [[kured]] — Kured (KUbernetes REboot Daemon)
- [[istio]] — Istio
- [[envoy]] — Envoy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference

<!-- risk-assessed -->
