---
title: Pixie [entities]
description: '## 概述'
summary: 'Pixie 是一个 Kubernetes 原生的可观测性平台，使用 eBPF 自动采集遥测数据，无需代码变更或手动 instrumentation。它提供对服务通信 (HTTP、gRPC、DNS、MySQL、PostgreSQL、Redis、Kafka)、资源使用和应用性能的即时可见性。Pixie 数据在集群内处理，支持 PxL 查询语言进行分析。'
category: entities
tags:
- k8s
- cncf
- observability
- pixie
- prometheus
- grafana
- istio
- redis
- mysql
- postgresql
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Pixie 是什么
- 如何 Pixie
trigger_keywords:
- Pixie
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- kafka-basics
- redis-basics
- mysql-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pixie

> **CNCF 状态**: Sandbox | **类别**: Observability | **主要语言**: C++, Go

## 概述

Pixie 是由 New Relic 开源（现 CNCF Sandbox）的 Kubernetes 原生可观测性平台，使用 eBPF 自动采集遥测数据，无需代码变更或手动 instrumentation。它提供对服务通信（HTTP、gRPC、DNS、MySQL、PostgreSQL、Redis、Kafka）、资源使用和应用性能的即时可见性。Pixie 数据在集群内处理（Edge Processing），支持 PxL 查询语言进行自定义分析。

## 核心特性

- **零 Instrumentation**: 基于 eBPF 自动采集，无需修改应用代码或注入 Sidecar
- **协议自动解析**: HTTP、gRPC、MySQL、PostgreSQL、Redis、DNS、Kafka
- **PxL 查询语言**: Python 风格的查询语言进行数据分析和可视化
- **边缘计算**: 数据在集群内处理，不外传，满足数据驻留合规
- **即时 Service Map**: 安装即可获得服务拓扑图和请求追踪
- **CPU 火焰图**: 自动采集 CPU 性能分析数据

## 架构

Pixie 由 Vizier（集群内数据采集和处理）和 Cloud（管理和可视化）组成。Vizier 以 DaemonSet 部署在每个节点，包含 PEM（Pixie Edge Module——eBPF 采集器）和 Query Broker（PxL 查询执行）。PEM 通过 eBPF 挂载到内核跟踪点（tracepoints、kprobes、uprobes），自动采集系统调用级别的协议数据。数据在节点本地缓冲和处理，通过 PxL 查询时聚合返回。Kelvin（集群级聚合器）处理跨节点数据。Cloud 端提供 Web UI 和管理界面，不存储遥测数据本身。

## Kubernetes 集成

Pixie 通过 DaemonSet 在每个节点部署 PEM。PEM 以特权模式运行以加载 eBPF 程序。通过 Kubernetes API 自动发现 Pod、Service 和命名空间元数据。与 CNI 插件无关——在内核层采集，兼容所有网络方案。Pixie CLI 通过 Pixie Auth 连接集群，支持 PDK（Pixie Development Kit）开发自定义脚本。数据保留在集群内，可通过 Pixie CLI 或 Web UI 查询。

## 生产使用场景

1. **零侵入监控**: 对已有应用无需修改代码即可获得全链路追踪
2. **协议级诊断**: 分析 HTTP 请求延迟、数据库查询性能
3. **安全合规**: 数据不出集群，适合金融/医疗等敏感场景
4. **CPU 分析**: 自动采集 CPU 火焰图，定位性能瓶颈

## 安装与配置

```bash
# 安装 Pixie CLI
brew install pixie
# 部署到集群（交互式）
px deploy
# 非交互式部署（指定集群）
px deploy --cluster_name=prod-cluster --deploy_key=$PIXIE_DEPLOY_KEY
# 验证部署状态
px status
```

```yaml
# Vizier 自定义配置（px deploy --set 参数）
# 限制 PEM 资源使用
pemMemoryLimit: 2Gi
pemMemoryRequest: 1Gi
# 数据保留时间（默认24h）
dataRetention: 12h
# 禁用自动更新
disableAutoUpdate: true
```

```pxl
# PxL 脚本示例：服务延迟分析
import px
df = px.DataFrame(table='http_events', start_time='-5m')
df = df.groupby(['service', 'req_path']).agg(
    latency_p99=('latency', px.percentile(99)),
    req_count=('latency', px.count)
)
df = df[px.column('req_count') > 100]
display(df)
```

## 运维操作

```bash
# 🟢 查看 Vizier 状态
px status
kubectl get pods -n px

# 🟢 运行预置 PxL 脚本
px script run px/service_stats
px script run px/http_data
px script run px/dns_data

# 🟢 查看集群资源使用
px script run px/cluster_stats

# 🟡 重启 Vizier 组件
kubectl rollout restart daemonset/vizier-pem -n px
kubectl rollout restart deployment/vizier-query-broker -n px

# 🟡 更新 Pixie 版本
px deploy --redeploy

# 🔴 卸载 Pixie（清除所有采集数据）
px delete
kubectl delete namespace px
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| PEM Pod CrashLoopBackOff | 内核版本不兼容/权限不足 | `kubectl logs -n px ds/vizier-pem` | 确认内核≥4.14，检查 privileged 权限 |
| 无 HTTP 数据 | 应用使用 HTTPS/非标准端口 | `px script run px/http_data` | 配置 TLS 证书或指定端口 |
| 查询超时 | Kelvin 资源不足 | `kubectl top pod -n px` | 增加 Kelvin 副本或资源限制 |
| DNS 数据缺失 | CoreDNS 使用非标准端口 | `px script run px/dns_data` | 配置 DNS 追踪端口 |
| eBPF 程序加载失败 | 内核配置缺少 BPF 支持 | `dmesg \| grep bpf` | 启用 CONFIG_BPF_SYSCALL |

```
排查流程：
├─ px status 检查组件健康
│  ├─ PEM 异常 → kubectl logs ds/vizier-pem
│  │  ├─ 内核不兼容 → 升级内核或切换节点
│  │  └─ OOM → 增加 pemMemoryLimit
│  └─ Query Broker 异常 → 检查资源使用
├─ 数据缺失
│  ├─ 特定协议 → 确认协议版本/端口配置
│  └─ 全部缺失 → 检查 eBPF 挂载点
└─ 性能问题 → 调整 dataRetention 和采样率
```

## 生产案例

### 案例 1：金融系统零侵入全链路监控

- **场景**: 某银行核心交易系统无法修改代码注入 SDK，需要全链路可观测性
- **排查**: 部署 Pixie 后通过 HTTP/gRPC 自动追踪发现支付网关 P99 延迟异常
- **方案**: 使用 PxL 脚本定位到数据库连接池耗尽，数据全程不出集群满足金融合规
- **效果**: 零代码变更实现全链路监控，满足银保监会数据驻留要求

### 案例 2：微服务 DNS 解析延迟定位

- **场景**: 多个微服务间歇性超时，传统 APM 无法定位
- **排查**: 通过 `px/dns_data` 脚本发现 CoreDNS 响应延迟 P99 达 500ms
- **方案**: 定位到 NodeLocal DNSCache 未启用，启用后 DNS 延迟降至 1ms
- **效果**: 服务超时率从 2% 降至 0.01%

## 替代方案对比

| 维度 | Pixie | Jaeger | Cilium Hubble | OpenTelemetry |
|------|-------|--------|---------------|---------------|
| 侵入性 | 零侵入(eBPF) | 需 SDK | 零侵入 | 需 SDK |
| 数据位置 | 集群内 | 外部存储 | 集群内 | 外部存储 |
| 协议解析 | HTTP/gRPC/DB/DNS | HTTP/gRPC | L3-L7 | 自定义 |
| 查询语言 | PxL | Jaeger UI | Hubble UI | 各后端 |
| 适用场景 | 即时诊断/合规 | 长期追踪 | 网络可观测 | 标准化遥测 |

## 架构定位

在 CNCF 生态中，Pixie 属于 **Observability** 类别，是 eBPF 驱动的零侵入可观测性代表。它与 Prometheus（指标）、Jaeger（追踪）互补。

## 参考链接

- [[实体/prometheus-grafana.md|prometheus-grafana]]
- [[istio]]
- [[operator-pattern]]
- [[概念/service-mesh-architecture.md|service-mesh-architecture]]
- [[概念/observability-pillars.md|observability-pillars]]

## Related

- [[02-istio-advanced-traffic-management]] — Istio 高级流量管理
- [[vscode-kubernetes-tools]] — VS Code Kubernetes Tools
- [[litmus]] — LitmusChaos
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[grpc]] — gRPC

- pixie
- [[实体/cncf-observability.md|CNCF 可观测性项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
