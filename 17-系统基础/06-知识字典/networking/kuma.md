---
title: Kuma 服务网格
description: Kuma 是 Kong 开源的 CNCF Sandbox 服务网格，基于 Envoy Proxy，支持 Kubernetes 和通用 VM
  环境，以易用性和多网...
summary: Kuma 是 Kong 开源的 CNCF Sandbox 服务网格，基于 Envoy Proxy，支持 Kubernetes 和通用 VM 环境，以易用性和多网...
category: dictionary
tags:
- k8s
- glossary
- networking
- service-mesh
- envoy
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kuma 服务网格 是什么
- Kuma 详解
trigger_keywords:
- Kuma 服务网格
- Kuma
- dictionary
prerequisites:
- kubernetes
---



# Kuma 服务网格（Kuma）

## 概述

Kuma 是 Kong 开源的 CNCF Sandbox 服务网格，基于 Envoy Proxy，支持 Kubernetes 和通用 VM 环境，以易用性和多网格（multi-mesh）架构著称。

## 核心概念/原理

- **Envoy 驱动**：基于 Envoy Proxy 的数据面
- **通用平台**：同时支持 Kubernetes 和 VM/裸金属
- **多网格**：原生支持多网格隔离架构
- **CNCF Sandbox**：Kong 主导，社区活跃

## 关键机制或特性

- Mesh CRD 定义网格实例（多网格隔离）
- TrafficPermission / TrafficRoute / TrafficLog 策略
- mTLS 自动管理（内置 CA）
- 速率限制和熔断
- MeshGateway 支持入口流量
- Kong Mesh 商业版提供企业功能
- Kuma GUI 可视化管理

## 使用场景与最佳实践

- 轻量级服务网格部署
- K8s + VM 混合环境的服务治理
- 多团队/多环境的网格隔离
- 需要简单操作体验的服务网格
- Istio 的轻量替代方案

## 参考链接

- https://kuma.io/
- https://github.com/kumahq/kuma

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│              Kuma Control Plane (CP)                │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ Policy      │  │ xDS Server   │  │ Mesh      │  │
│  │ Manager     │  │ (Envoy API)  │  │ Manager   │  │
│  └──────┬──────┘  └──────┬───────┘  └───────────┘  │
│         │                │                         │
│  ┌──────▼────────────────▼─────────────────────┐  │
│  │     Data Plane Proxy (DPP) - Envoy          │  │
│  │  (Sidecar / Gateway / VM Agent)             │  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（kumahq/kuma）

| 模块 | 路径 | 职责 |
|------|------|------|
| CP 主循环 | `app/kuma-cp/` | 控制平面启动与编排 |
| xDS 生成 | `pkg/xds/` | Envoy 配置生成与推送 |
| Policy | `pkg/plugins/policies/` | 流量策略实现（路由/限流/熔断） |
| Mesh | `pkg/core/resources/` | Mesh 资源模型与存储 |
| DPP | `app/kuma-dp/` | 数据平面代理管理 |

### 多网格隔离机制

1. 每个 Mesh 是独立的逻辑网络边界
2. 服务通过 `kuma.io/mesh` 标签归属到特定 Mesh
3. 跨 Mesh 通信需显式声明 MeshTrafficPermission
4. 每个 Mesh 可配置独立的 mTLS、日志、追踪策略

## 生产案例

### 案例 1：mTLS 证书轮转导致服务中断

| 时间 | 事件 |
|------|------|
| 04:00 | 自动证书轮转触发（默认 30 天） |
| 04:01 | 部分旧版 Envoy 不支持热证书更新，连接断开 |
| 04:05 | 确认：Envoy 1.24 以下版本需要重启才能加载新证书 |
| 04:10 | 修复：升级 DPP 到最新版本，启用 `builtin` CA 的渐进式轮转 |

**修复命令**：
```bash
# 检查 Mesh mTLS 状态 🟢 只读
kubectl get mesh default -o yaml | grep -A10 "mtls:"
# 查看 DPP 版本 🟢 只读
kubectl get pods -A -l app=kuma-dp -o jsonpath='{.items[*].spec.containers[*].image}'
# 重启 DPP 加载新证书 🟡 中风险
kubectl rollout restart deploy/my-service
```

### 案例 2：跨 Mesh 流量被默认拒绝

**现象**：服务 A（mesh-a）无法访问服务 B（mesh-b），返回 RBAC 拒绝。

**诊断**：Kuma 默认禁止跨 Mesh 通信，需显式创建 MeshTrafficPermission。

**修复**：创建跨 Mesh 访问策略：
```yaml
apiVersion: kuma.io/v1alpha1
kind: MeshTrafficPermission
metadata:
  name: cross-mesh-access
spec:
  targetRef:
    kind: MeshService
    name: service-b
  from:
    - targetRef:
        kind: Mesh
        name: mesh-a
      default:
        action: Allow
```

## 对比评测

| 维度 | Kuma | Istio | Linkerd |
|------|------|-------|---------|
| 多网格支持 | 原生（多租户 Mesh） | 单网格（多集群） | 单网格 |
| 数据面 | Envoy | Envoy | 自研（Rust） |
| 策略模型 | MeshPolicy 统一 | VirtualService/DR | 注解/CRD |
| 多平台 | K8s + VM + 裸机 | K8s 为主 | K8s |
| 运维复杂度 | 中 | 高 | 低 |

**选型建议**：多团队/多环境需要独立网格治理选 Kuma（多租户 Mesh）；K8s 生态标准化选 Istio；轻量低开销选 Linkerd。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|----------|----------|
| 数据面未注入 | `kubectl get ns -L kuma.io/sidecar-injection` | 命名空间标签缺失 |
| 策略不生效 | `kubectl get meshpolicy`；检查 Mesh 归属 | 策略绑定错误 Mesh、优先级冲突 |
| 多集群同步失败 | 检查 Global/Zone 控制面连接 | Zone 注册失败、网络隔离 |
| 数据面内存高 | 查看 Envoy 配置大小 | 策略过多、Listener 膨胀 |

## 生产部署清单

- [ ] Mesh 资源规划（按团队/环境拆分，多租户隔离）
- [ ] 数据面注入策略（标签/注解）已灰度验证
- [ ] 多集群（Global/Zone）拓扑与证书信任链已配置
- [ ] mTLS 默认开启且证书轮换验证
- [ ] 监控接入（Kuma metrics + 控制面健康）

## 常见误区与设计要点

- **误区 1**：把策略全部放到默认 Mesh——多团队应各自 Mesh 隔离，避免互相影响。
- **误区 2**：忽略 mTLS 默认开启——Kuma 安全模型依赖身份，关闭后策略形同虚设。
- **设计要点**：先设计 Mesh 拓扑（多租户 vs 多集群）；策略分层（Mesh 级 → 服务级）；升级前备份控制面存储（PostgreSQL）。

## 性能参考

- 数据面开销：Envoy 代理额外 5-10% 延迟（HTTP 场景），CPU < 15%。
- 控制面规模：单 Global 支持数千 Zone、数万服务（受 PostgreSQL 性能限制）。
- 同步时效：策略变更秒级推送（xDS 增量）。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 控制平面不可用，所有 DPP 断开 | 恢复 CP，从备份恢复 Mesh 配置 |
| P1 | mTLS 证书过期导致连接失败 | 手动触发证书轮转，重启 DPP |
| P2 | 策略下发延迟 > 10s | 检查 CP 负载，扩容实例 |

## 面试要点

1. **Q：Kuma 与 Istio 的核心差异是什么？**
   A：Kuma 基于 Envoy 但架构更简单：单一控制平面（无 istiod 的复杂组件），原生支持多 Mesh 隔离，同时支持 K8s 和 VM 环境。Istio 功能更丰富（Telemetry、安全策略）但复杂度高。Kuma 适合需要简单操作体验和多环境支持的团队；Istio 适合需要完整服务网格功能的企业。

2. **Q：Kuma 的多 Mesh 隔离如何实现？**
   A：每个 Mesh 是独立的逻辑网络：① 服务通过 `kuma.io/mesh` 标签归属；② xDS 配置按 Mesh 隔离下发；③ mTLS 证书按 Mesh 独立签发；④ 跨 Mesh 通信需 MeshTrafficPermission 显式授权。这比 Istio 的 Namespace 隔离更彻底。

3. **Q：如何在 K8s + VM 混合环境中部署 Kuma？**
   A：① K8s 中部署 CP（Helm）；② K8s Pod 通过 Sidecar Injector 自动注入 DPP；③ VM 上安装 kuma-dp Agent，通过 token 注册到 CP；④ 使用 Mesh 统一管理服务发现和策略；⑤ VM 服务通过 `kuma.io/service` 标签暴露。

## Related

- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
- [[17-系统基础/06-知识字典/networking/linkerd.md|Linkerd]]
- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
