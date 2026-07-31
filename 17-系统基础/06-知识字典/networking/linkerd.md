---
title: Linkerd
description: Linkerd 是最早的服务网格项目之一，现为 CNCF 毕业项目。以极简设计和高性能著称，相比 Istio 更轻量、更易运维，适合不需要复杂
  Istio 功能...
summary: Linkerd 是最早的服务网格项目之一，现为 CNCF 毕业项目。以极简设计和高性能著称，相比 Istio 更轻量、更易运维，适合不需要复杂 Istio
  功能...
category: dictionary
tags:
- k8s
- glossary
- linkerd
- service-mesh
- cncf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Linkerd 是什么
- Linkerd 详解
trigger_keywords:
- Linkerd
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Linkerd

> **英文名**: Linkerd

## 概述

Linkerd 是最早的服务网格项目之一，现为 CNCF 毕业项目。以极简设计和高性能著称，相比 Istio 更轻量、更易运维，适合不需要复杂 Istio 功能的中小规模服务网格场景。

## 核心概念/原理

### 核心架构

- **Linkerd Proxy**：Rust 编写的超轻量 sidecar（~10MB 内存）。
- **Linkerd Control Plane**：管理代理配置和证书。
- **Linkerd Viz**：可观测性 Dashboard。

### 与 Istio 对比

| 特性 | Linkerd | Istio |
|------|---------|-------|
| Proxy | Rust (轻量) | Envoy (功能丰富) |
| 复杂度 | 低 | 高 |
| mTLS | 内置 | 内置 |
| L7 策略 | 有限 | 丰富 |
| 资源开销 | 极低 | 较高 |

## 关键机制或特性

- **mTLS**：自动为所有服务间通信启用 mTLS。
- **负载均衡**：P2C（Power of Two Choices）算法。
- **重试和超时**：应用级别的重试策略。
- **流量拆分**：金丝雀发布和 A/B 测试。
- **Multi-cluster**：跨集群服务通信。

## 使用场景与最佳实践

- 需要服务网格但希望最小运维复杂度时选择 Linkerd。
- 使用 Linkerd 的 mTLS 实现零信任网络。
- 启用 Linkerd Viz 监控服务网格指标。
- 配合 Flagger 实现自动化金丝雀发布。
- 使用 `linkerd check` 验证安装和配置。

## 参考链接

- [Linkerd Official](https://linkerd.io/)

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────┐
│              Linkerd Control Plane                  │
├─────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌──────────────┐  ┌───────────┐  │
│  │ destination │  │ identity     │  │ proxy-    │  │
│  │ (服务发现)  │  │ (mTLS CA)    │  │ injector  │  │
│  └──────┬──────┘  └──────┬───────┘  └───────────┘  │
│         │                │                         │
│  ┌──────▼────────────────▼─────────────────────┐  │
│  │   Data Plane: linkerd2-proxy (Rust, per Pod)│  │
│  └──────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

### 源码关键路径（linkerd/linkerd2）

| 模块 | 路径 | 职责 |
|------|------|------|
| destination | `controller/api/destination/` | 服务发现与路由信息 |
| identity | `controller/identity/` | 证书签发与轮转 |
| proxy | `linkerd2-proxy/` (Rust) | 数据平面代理 |
| proxy-injector | `controller/proxy-injector/` | Sidecar 自动注入 |
| viz | `viz/` | 可观测性组件（dashboard/prometheus） |

### 流量拦截机制

1. Pod 创建 → proxy-injector 注入 linkerd2-proxy 容器
2. init container 配置 iptables（REDIRECT 到 proxy）
3. 入站流量 → linkerd2-proxy Inbound → 应用
4. 出站流量 → linkerd2-proxy Outbound → 目标
5. mTLS 自动协商（基于 ServiceAccount Identity）

## 生产案例

### 案例 1：linkerd2-proxy 内存泄漏

| 时间 | 事件 |
|------|------|
| 20:00 | 多个 Pod 的 linkerd-proxy 容器 OOMKilled |
| 20:10 | 确认：proxy 版本 2.12 存在连接池泄漏 bug |
| 20:20 | 临时修复：增加 proxy 内存限制到 512Mi |
| 20:30 | 根因修复：升级 Linkerd 到 2.14+（修复了连接池问题） |

**修复命令**：
```bash
# 检查 proxy 版本 🟢 只读
linkerd version --short
# 查看 proxy 资源使用 🟢 只读
kubectl top pods -n my-ns -c linkerd-proxy
# 升级 Linkerd 🟡 中风险
linkerd upgrade | kubectl apply -f -
kubectl rollout restart deploy/my-service
```

### 案例 2：mTLS 证书轮转失败

**现象**：服务间通信间歇性失败，日志显示 `identity expired`。

**诊断**：identity 组件的证书轮转任务积压，部分 Pod 证书过期。

**修复**：重启 identity Deployment，检查 `linkerd-identity-issuer` Secret 有效性。

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 控制平面全部不可用 | 恢复 control plane，从备份恢复 |
| P1 | mTLS 证书批量过期 | 重启 identity，强制证书轮转 |
| P2 | 单 Pod proxy 异常 | 重启该 Pod，检查 proxy 日志 |

## 面试要点

1. **Q：Linkerd 与 Istio 的核心差异？**
   A：Linkerd 使用自研的 linkerd2-proxy（Rust 实现），专为 Service Mesh 优化，资源占用低（~10MB 内存）；Istio 使用通用 Envoy（C++），功能更丰富但资源占用高（~50MB）。Linkerd 架构更简单（3 个控制平面组件 vs Istio 的 istiod 单体），运维复杂度低。Istio 生态更丰富（Telemetry、多集群）。

2. **Q：Linkerd 的 mTLS 是如何实现的？**
   A：Linkerd 使用 ServiceAccount 作为 Identity：① identity 组件作为 CA 签发证书；② 每个 Pod 的 proxy 获取基于 ServiceAccount 的证书；③ 连接建立时自动 mTLS 协商；④ 证书默认 24h 轮转。实现路径：`controller/identity/` + `linkerd2-proxy/` 的 TLS 模块。

3. **Q：如何评估是否应该采用 Linkerd？**
   A：适合 Linkerd 的场景：① 需要简单的服务网格（mTLS + 可观测性）；② 团队运维能力有限；③ 资源敏感环境（边缘/IoT）；④ 不需要复杂流量管理。不适合：① 需要多集群联邦；② 需要丰富的 L7 策略；③ 已有 Istio 生态投入。

## Related

- [[17-系统基础/06-知识字典/networking/istio.md|Istio]]
- [[17-系统基础/06-知识字典/networking/envoy.md|Envoy]]
- [[17-系统基础/06-知识字典/networking/service.md|Service]]
- [[17-系统基础/06-知识字典/networking/cilium.md|Cilium]]
- [[17-系统基础/06-知识字典/security/certificate.md|Certificate]]


<!-- risk-assessed -->
