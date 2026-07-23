---
title: scripts/ - 项目工具脚本索引
description: '| 3 | `code-example-validation.sh` | Bash | 文档中 YAML/Bash 代码块语法校验 |
  自动提取所有 Markdown 中的 `yaml` 和 `bash` 代码块，分别用 Python yaml 解析和 `bash -n` 做语法校验 |'
summary: '| 3 | `code-example-validation.sh` | Bash | 文档中 YAML/Bash 代码块语法校验 | 自动提取所有
  Markdown 中的 `yaml` 和 `bash` 代码块，分别用 Python yaml 解析和 `bash -n` 做语法校验 |'
category: general
tags:
- k8s
- agent
- daemonset
- gpu
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- scripts/ - 项目工具脚本索引 是什么
- 如何 scripts/ - 项目工具脚本索引
trigger_keywords:
- scripts
- 项目工具脚本索引
prerequisites:
- kubectl-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# scripts/ - 项目工具脚本索引

> 本目录存放 KUDIG-DATABASE 项目级自动化脚本，用于**统计、质量检查、可视化、语料增强**四类场景。

> **完整脚本说明请参阅** [`../README.md`](../README.md#-脚本工具)。

## 脚本清单

| # | 脚本 | 语言 | 用途 | 保留理由 |
|:---:|:---|:---:|:---|:---|
| 1 | `generate-readme-stats.sh` | Bash | README 数字指标统计 | **核心脚本**。自动计算 README 中引用的所有数字（文档数、字符数、知识域、FTA/FEBM/CNCF 等），支持 JSON/表格/徽章三种输出格式 |
| 2 | `comprehensive-quality-check.sh` | Bash | 知识库全面质量检查 | 检查目录结构完整性、README 链接有效性、文档长度、头部信息完整性、专家级内容标识等 |
| 3 | `code-example-validation.sh` | Bash | 文档中 YAML/Bash 代码块语法校验 | 自动提取所有 Markdown 中的 `yaml` 和 `bash` 代码块，分别用 Python yaml 解析和 `bash -n` 做语法校验 |
| 4 | `fta_tree_visualization.py` | Python | FTA 故障树可视化 | 基于 matplotlib 生成高质量 FTA（故障树分析）示意图 PNG |
| 5 | `enhance-cross-refs.py` | Python | 自动生成 cross_refs 结构化引用 | 扫描 domain/topic 目录，基于标题关键词生成 cross_refs 字段（domain/fta/skill/structural/cheatsheet） |
| 6 | `validate-frontmatter.py` | Python | 批量检查 front matter 完整性 | 检测 intent_queries、trigger_keywords、reading_level、audience 等关键字段是否缺失 |
| 7 | `format-intent-queries.py` | Python | 统一 intent_queries 格式 | 确保英文动词短语和中文疑问句/陈述句格式一致，提升 AI 检索质量 |
| 8 | `gen-doc-stats.py` | Python | 生成文档统计报告 | 按 domain/topic 分类统计文档数量、难度分布、AI 就绪度 |
| 9 | `check-broken-links.sh` | Bash | 验证 cross_refs 路径有效性 | 防止断链导致 AI 检索失败 |
| 10 | `batch-enrich.sh` | Bash | 批量补充缺失字段 | 自动添加 reading_level、audience、estimated_read_time |
| 11 | `export_corpus_for_nas.py` | Python | 导出 llm-wiki 语料到 release/ | 由 `_meta/corpus-config/profiles/*.yaml` 驱动 include/exclude，输出 corpus（按 tier）+ qa + metadata，供 Agent 加载 |
| 12 | `start-web.sh` | Bash | 启动本地预览服务 | Astro 开发服务器（默认）/ 构建预览 / 可视化 HTML 静态伺服 |
| 13 | `video-content-generator.py` | Python | 数字人播报脚本生成 | 基于 FTA/FEBM/Skills 生成视频文案 |
| 14 | `video-generator.py` | Python | 视频生成 API 调用 | 调用腾讯智影/HeyGen/剪映生成视频 |

每个脚本都配有完整的 Unix manpage，可通过以下方式查看：

```bash
# 查看 manpage
man ./man/man1/kudig-stats.1
man ./man/man1/kudig-quality.1
man ./man/man1/kudig-validate.1
man ./man/man1/kudig-fta-viz.1

# 或安装到系统后直接使用
man kudig-stats
man kudig-quality
```

更多安装选项详见 [`man/INSTALL.md`](../man/INSTALL.md) 和 [`man/README.md`](../man/README.md)。

## 其他工具

Domain 级别的专用工具存放在各自目录下：

- `故障诊断/tools/` — K8s 故障排查工具套件

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- 网络 MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[概念/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[元数据/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[AI基础设施/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[AI基础设施/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[技能/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[技能/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[平台工程/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[实体/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[生态参考/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[生态参考/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[生态参考/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
