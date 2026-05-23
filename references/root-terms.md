---
title: K8s Root术语参考
description: '### Topic Dictionary 内容缺口分析（2026 行业最佳实践视角）'
category: references
tags:
- k8s
- dictionary
- root
- hpa
- vpa
- pdb
- rbac
- gpu
- wasm
- kserve
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s Root术语参考 是什么
- 如何 K8s Root术语参考
trigger_keywords:
- K8s
- Root术语参考
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
created: "2026-05-23"
---

本页汇总了 **Root** 领域的 4 个 Kubernetes 术语定义与概念说明。

---

### Topic Dictionary 内容缺口分析（2026 行业最佳实践视角）

## 六、使用建议

1. **定期复盘**：建议每季度根据 CNCF 年度报告、KubeCon 议题和行业白皮书更新本缺口分析
2. **社区贡献**：新增文件应遵循统一格式（7 个固定章节），并在 `README.md` 中更新目录导航
3. **交叉引用**：新文件与现有概念文件之间应通过 Markdown 链接建立关联网络
4. **版本控制**：对于快速演进的技术（如 LLM 推理、Wasm），建议标注"最后更新日期"

---

### topic-dictionary MOC

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 207 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

---

### Topic Dictionary 知识字典

## 使用建议

1. **按角色检索**：
   - **开发者**：重点查阅 `workloads/`、`configuration/`、`networking/`
   - **SRE/运维**：重点查阅 `operations/`、`observability/`、`scheduling/`、`security/`
   - **平台工程师**：重点查阅 `platform-engineering/`、`security/`、`scheduling/`
   - **架构师**：重点查阅 `fundamentals/`、`multi-cloud/`、`platform-engineering/`
   - **AI/ML 工程师**：重点查阅 `specialized-workloads/`、`scheduling/`、`storage/`

2. **按问题检索**：
   - 某个概念不懂 → `fundamentals/`
   - 应用部署失败 → `workloads/` + `operations/production-troubleshooting-playbook.md`
   - 性能问题 → `operations/performance-tuning-expert.md` + `scheduling/`
   - 安全加固 → `security/` + `operations/certificates.md`
   - 容量告警 → `operations/capacity-planning-forecasting.md` + `scheduling/`
   - 成本优化 → `operations/finops-and-cost-optimization.md` + `workloads/spot-and-preemptible-workloads.md`
   - LLM 推理部署 → `specialized-workloads/kserve-model-serving.md` + `specialized-workloads/llm-inference-optimization.md`
   - GPU 调度问题 → `specialized-workloads/gpu-resource-management-and-partitioning.md` + `scheduling/dynamic-resource-allocation.md`

3. **持续演进**：新增内容应按照上述领域边界归入对应目录；若出现跨领域内容，优先归入**最相关的单一领域**，并在文档中通过链接引用其他领域。

---

### K8s 中英术语表（Glossary）

## 8. 常见缩写

| 缩写 | 全称 | 中文 |
|------|------|------|
| K8s | Kubernetes | 容器编排平台 |
| CNCF | Cloud Native Computing Foundation | 云原生计算基金会 |
| CRI | Container Runtime Interface | 容器运行时接口 |
| CSI | Container Storage Interface | 容器存储接口 |
| CNI | Container Network Interface | 容器网络接口 |
| RBAC | Role-Based Access Control | 基于角色的访问控制 |
| HPA | Horizontal Pod Autoscaler | 水平 Pod 自动扩缩容 |
| VPA | Vertical Pod Autoscaler | 垂直 Pod 自动扩缩容 |
| PDB | Pod Disruption Budget | Pod 中断预算 |
| SSA | Server-Side Apply | 服务器端应用 |

---

（代码示例已省略）

## Related

- [[entities/container-runtime|container-runtime]] — Container Runtime
- [[kserve]] — KServe
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/resource-management|resource-management]] — Resource Management (Requests, Limits, QoS)
