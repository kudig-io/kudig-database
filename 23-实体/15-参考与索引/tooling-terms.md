---
title: K8s 工具链术语参考
description: 'description: ''**运维效率提升宝典**: 300+实用命令集合，涵盖日常运维、故障排查、性能调... |'
summary: 'description: ''**运维效率提升宝典**: 300+实用命令集合，涵盖日常运维、故障排查、性能调... |'
category: references
tags:
- k8s
- dictionary
- tooling
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- grafana
- jaeger
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 工具链术语参考 是什么
- 如何 K8s 工具链术语参考
trigger_keywords:
- K8s
- 工具链术语参考
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- cilium-basics
- etcd-basics
- tracing-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 工具链术语参考

本页汇总了 **工具链** 领域的 3 个 Kubernetes 术语定义与概念说明。

> **相关领域**: [[23-实体/15-参考与索引/kubectl-quick-reference.md|kubectl-quick-reference]] | [[23-实体/15-参考与索引/kudig-ecosystem-guide.md|kudig-ecosystem-guide]]

---

## 术语速查表

| 术语 | 英文名 | 说明 |
|------|--------|------|
| **查看所有 Pod 及其详细信息** | Cli Commands | title: 查看所有 Pod 及其详细信息
description: '**运维效率提升宝典**: 300+实用命令集合，涵盖日常运维、故障排查、性能调... |
| **容器镜像优化** | Container Image Optimization | 容器镜像是 Kubernetes 应用部署的基础单元 |
| **Kusheet 工具与开源项目 URL 汇总** | Tool Ecosystem | title: Kusheet 工具与开源项目 URL 汇总
description: '| **适合读者** | 初学者(了解工具选择) → 中级(对比方... |

---

### 查看所有 Pod 及其详细信息

title: 查看所有 Pod 及其详细信息
description: '**运维效率提升宝典**: 300+实用命令集合，涵盖日常运维、故障排查、性能调优等全方位操作'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
- cilium
- flannel
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 查看所有 Pod 及其详细信息 是什么
- 如何 查看所有 Pod 及其详细信息
trigger_keywords:
- 查看所有
- Pod
- 及其详细信息
- dictionary
title_en: Pods
authors:
- name: KUDIG Team
  role: contr...

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/tooling/cli-commands.md`）*

---

### 容器镜像优化

容器镜像是 Kubernetes 应用部署的基础单元。优化镜像不仅可以**缩短启动时间、降低存储和带宽成本**，还能**显著减少安全攻击面**。2026 年的行业最佳实践强调：镜像应尽可能小、只包含应用运行所需的最小依赖、使用不可变基础镜像，并通过 SBOM 和签名确保供应链透明。主流优化手段包括**多阶段构建（Multi-stage Build）、Distroless 镜像、BuildKit 缓存、镜像分层优化和 OCI 标准化**。

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/tooling/container-image-optimization.md`）*

---

### Kusheet 工具与开源项目 URL 汇总

title: Kusheet 工具与开源项目 URL 汇总
description: '| **适合读者** | 初学者(了解工具选择) → 中级(对比方案) → 专家(深度集成) |'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- scheduler
- prometheus
- grafana
- jaeger
- istio
- envoy
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 1h
intent_queries:
- Kusheet 工具与开源项目 URL 汇总 是什么
- 如何 Kusheet 工具与开源项目 URL 汇总
trigger_keywords:
- Kusheet
- 工具与开源项目
- URL
- 汇总
- dictionary
title_en: Tool Ecosystem
authors:
...

> *（内容已精简，完整版请参阅源文件 `系统基础/topic-dictionary/tooling/tool-ecosystem.md`）*

---

## 相关页面

- [[23-实体/15-参考与索引/kubectl-quick-reference.md|kubectl-quick-reference]]
- [[23-实体/15-参考与索引/kudig-ecosystem-guide.md|kudig-ecosystem-guide]]

## 来源文件

- `系统基础/topic-dictionary/tooling/cli-commands.md`
- `系统基础/topic-dictionary/tooling/container-image-optimization.md`
- `系统基础/topic-dictionary/tooling/tool-ecosystem.md`

## Related

- [[jaeger]] — Jaeger
- [[etcd]] — etcd
- [[envoy]] — Envoy
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
