# Topic: Presentations（技术演示文稿）

> **11 篇 Presentation** | 面向内部培训与技术分享的 Kubernetes 专题演示文稿

## 概述

本目录收录了 Kubernetes 核心技术领域的演示文稿，适用于团队内部培训、技术分享、Workshop 等场景。每篇 Presentation 包含完整的讲解大纲、关键知识点、演示命令和参考资源。

## 文档索引

| # | 文档 | 主题 |
|:---:|:---|:---|
| 1 | [架构基础](./kubernetes-architecture-fundamentals-presentation.md) | K8s 架构概览、核心组件、设计哲学 |
| 2 | [CoreDNS](./kubernetes-coredns-presentation.md) | DNS 服务发现、CoreDNS 配置与调优 |
| 3 | [Ingress](./kubernetes-ingress-presentation.md) | Ingress 控制器、路由规则、TLS 终止 |
| 4 | [可观测性](./kubernetes-observability-presentation.md) | 监控、日志、链路追踪三大支柱 |
| 5 | [调度器](./kubernetes-scheduling-presentation.md) | 调度框架、亲和性、拓扑分布约束 |
| 6 | [安全与 RBAC](./kubernetes-security-rbac-presentation.md) | RBAC 模型、ServiceAccount、安全策略 |
| 7 | [Service](./kubernetes-service-presentation.md) | Service 类型、kube-proxy 模式、流量拓扑 |
| 8 | [存储](./kubernetes-storage-presentation.md) | PV/PVC、StorageClass、CSI 驱动 |
| 9 | [Terway 网络](./kubernetes-terway-presentation.md) | 阿里云 Terway CNI、VPC 网络模式 |
| 10 | [故障排查方法论](./kubernetes-troubleshooting-methodology-presentation.md) | FTA/FEBM、结构化排障流程 |
| 11 | [工作负载](./kubernetes-workload-presentation.md) | Deployment/StatefulSet/DaemonSet/Job |

## 模板

| 文件 | 说明 |
|:---|:---|
| [presentation-template.md](./presentation-template.md) | 新建 Presentation 的标准模板 |

## 使用场景

| 场景 | 推荐 Presentation | 时长建议 |
|:---|:---|:---:|
| 新人入职培训 | 架构基础 → 工作负载 → Service | 各 45min |
| SRE 技术分享 | 故障排查方法论 → 可观测性 | 各 60min |
| 网络专题 Workshop | CoreDNS → Ingress → Terway | 各 30min |
| 安全合规培训 | 安全与 RBAC | 60min |

## 交叉引用

| 相关目录 | 关系 |
|:---|:---|
| [topic-learn/](../topic-learn/) | 系统化学习计划（Presentation 可作为课程配套） |
| [topic-cheat-sheet/](../topic-cheat-sheet/) | 演示中的命令速查 |
| [topic-fta/](../topic-fta/) | 故障排查方法论 Presentation 的深度参考 |
