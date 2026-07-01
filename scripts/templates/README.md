---
title: "模板标题"
category: templates
tags: ["templates", "visibility/public"]
sources: ["auto-generated"]
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

# KUDIG 文档模板体系

> **模板版本**: 2.0
> **最后更新**: 2026-05
> **维护者**: KUDIG Team

本目录收录项目所有标准化文档模板。新建文档时应从对应模板创建，并严格遵循模板结构。

---

## 模板索引

| 模板文件 | 用途 | 适用目录 | 复杂度 |
|:---|:---|:---|:---:|
| `domain-article-template.md` | 知识域深度文档 | `domain-*/` | ⭐⭐⭐ |
| `fta-template.md` | FTA 故障树分析（融合附录模板） | `domain-10-troubleshooting-diagnostics/topic-fta/list/` | ⭐⭐⭐ |
| `skill-template.md` | Skill 工单技能（融合 Schema 完整规范） | `domain-10-troubleshooting-diagnostics/topic-skills/` | ⭐⭐⭐⭐ |
| `febm-template.md` | FEBM 法医取证分析 | `domain-10-troubleshooting-diagnostics/topic-febm/` | ⭐⭐⭐ |
| `cheat-sheet-template.md` | 技术速查卡 | `domain-17-system-foundation/topic-cheat-sheet/` | ⭐ |
| `presentation-template.md` | 培训课程/演讲稿 | `domain-11-production-operations/topic-presentations/` | ⭐⭐⭐ |

---

## 统一命名规范

所有模板遵循以下规范：

| 元素 | 格式规则 | 示例 |
|:---|:---|:---|
| 文件名 | `{NN}-{kebab-case-name}.md` | `01-node-notready.md` |
| 一级标题 | `{中文名称} {英文名}` | `# Pod 异常故障树分析` |
| 模板变量 | `{{PLACEHOLDER}}` | `{{组件名称}}` |
| 版本标记 | `**[vX.XX+]**` 或 `**[vX.XX-vX.XX]**` | `**[v1.30+]**` |

---

## 通用 Front Matter 规范

推荐在文档开头使用 YAML front matter（可通过 `---` 包裹），包含以下字段：

```yaml
---
title: "{{文档标题}}"
description: "{{一句话摘要}}"
tags: [k8s, troubleshooting, network]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32"]
last_updated: "2026-05"
author: "KUDIG Team"
reviewers: []
related_docs:
  - path: "{{路径}}"
    desc: "{{说明}}"
---
```

---

## 模板使用流程

```bash
# 1. 选择对应模板
cp templates/fta-template.md domain-10-troubleshooting-diagnostics/topic-fta/list/12-new-component-fta.md

# 2. 全局替换占位符
# {{组件名称}} → 实际内容

# 3. 按模板章节结构填充内容

# 4. 提交前检查
# - 所有 {{PLACEHOLDER}} 已替换
# - YAML front matter 已填写
# - 相关文档链接已验证
```

---

## 模板版本历史

| 版本 | 日期 | 变更说明 |
|:---:|:---:|:---|
| 1.0 | 2026-03 | 初始版本，5套模板独立存在 |
| 2.0 | 2026-05 | 合并 skill-schema + skill-template 为完整 Skill 模板；融合 appendix-d-templates 到 FTA 模板；新增 FEBM 模板；presentation-template 纳入体系 |

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/01-ai-infra/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/operate/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/higress-index.md|Higress 知识图谱索引]]
