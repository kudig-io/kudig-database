---
title: KUDIG Domain 目录映射表
category: references
tags:
- structure
- taxonomy
- mapping
- domain
- llm-wiki
tier: supporting
created: '2026-07-09'
last_updated: '2026-07-10'
---

# KUDIG Domain 目录映射表

> 本表记录 2026-07-10 目录扁平化重组后的最终结构。
> 原则：一级目录 20 个中文知识域 + 14 个英文支撑域；二级目录一律用中文简称（英文缩写/工具名如 GitOps/IaC/kubectl/Docker/eBPF 保留）；全库原则上只保留两层，专题（知识字典、代码分析、技能体系）因自带分类允许 3 层。
> 旧的 `NN-<english-slug>/` 编号前缀已全部去除；`98-merged-indexes/` 统一归档到 `_archives/domain-indexes/<domain>/`。

## 中文知识域（20）

| # | 一级目录 | Taxonomy Tag | 二级子目录（中文简称） |
|---:|---|---|---|
| 01 | `集群基础/` | `domain/cluster-fundamentals` | `架构总览/`, `设计原则/`, `控制平面/`, `API版本/`, `kubectl/`, `升级路径/` |
| 02 | `工作负载/` | `domain/workloads-applications` | `核心工作负载/`, `Java-on-K8s/` |
| 03 | `网络/` | `domain/networking-traffic` | `K8s网络核心/`, `网络基础/`, `服务网格/`, `API网关/`, `eBPF/`, `附件/`, `Terway/` |
| 04 | `存储/` | `domain/storage-data` | `K8s存储/`, `存储基础/`, `分布式存储/`, `有状态应用存储/` |
| 05 | `安全/` | `domain/security-compliance` | `身份与访问/`, `网络安全/`, `运行时安全/`, `策略治理/`, `供应链/`, `合规审计/` |
| 06 | `可观测性/` | `domain/observability` | `总览/`, `指标/`, `日志/`, `链路追踪/`, `告警/`, `SLO-SLI/`, `工具/` |
| 07 | `平台工程/` | `domain/platform-engineering` | `构建/`, `运维/`, `治理/`, `开发体验/`, `代码分析/` |
| 08 | `发布变更/` | `domain/release-change-management` | `GitOps/`, `IaC/`, `变更管理/`, `测试质量/`, `部署方案/`, `迁移方案/` |
| 09 | `可靠性/` | `domain/reliability-engineering` | `备份恢复/`, `灾难恢复/`, `容量规划/`, `混沌工程/`, `事后复盘/`, `SRE实践/`, `性能测试/` |
| 10 | `故障诊断/` | `domain/troubleshooting-diagnostics` | `资源排障/`, `高级排障/`, `核心排障/`, `基础设施排障/`, `JVM调优/`, `性能调优/`, `工具/`, `FEBM方法论/`, `FTA故障树/`, `技能体系/`, `多故障场景/`, `QA语料/` |
| 11 | `生产运维/` | `domain/production-operations` | `成本治理/`, `集群治理/`, `事件响应/`, `绿色计算/`, `工单案例/`, `回复话术/` |
| 12 | `云厂商/` | `domain/cloud-providers` | `阿里云/`, `AWS-EKS/`, `Google-GKE/`, `Azure-AKS/`, `腾讯云TKE/`, `华为云CCE/`, `多云混合/`, `其他云/` |
| 13 | `容器运行时/` | `domain/container-runtime` | `Docker/`, `镜像管理/`, `containerd-CRI-O/`, `镜像构建/`, `运行时迁移/` |
| 14 | `AI基础设施/` | `domain/ai-ml-infra` | `AI基础设施/`, `AI-Agents/`, `Agent运行时/`, `AI编码/` |
| 15 | `专项技术/` | `domain/specialized-tech` | `边缘计算/`, `WebAssembly/`, `扩展机制/`, `无服务器/` |
| 16 | `数据库中间件/` | `domain/database-middleware` | `数据库/`, `缓存/`, `消息队列/`, `时序数据库/`, `Operator管理/`, `数据流/` |
| 17 | `系统基础/` | `domain/system-foundation` | `Linux/`, `硬件/`, `K8s事件/`, `速查卡/`, `知识字典/` |
| 18 | `清单模式/` | `domain/manifests-patterns` | `YAML参考/`, `Kustomize模式/`, `Helm值模式/` |
| 19 | `生态参考/` | `domain/landscape-references` | `CNCF全景/`, `论文/`, `领域索引/` |
| 20 | `应用模式/` | `domain/application-patterns` | `子模式/`, `行业架构/`, `生产模式/` |

## 英文支撑域（14，结构保持不动）

| 目录 | 用途 |
|---|---|
| `assets/` | 图片、PDF 等静态资源 |
| `code/` | 示例代码片段与脚本 |
| `docs/` | 项目级说明文档 |
| `entities/` | 实体页（工具/产品/公司） |
| `release/` | 发布包与打包产物 |
| `research/` | 研究性笔记 |
| `scripts/` | 维护脚本（含 `maintenance/rewrite-links-after-rename.py`） |
| `skills/` | Skill 定义 |
| `synthesis/` | 跨域综合文章 |
| `tags/` | 标签索引页 |
| `web/` | Astro 站点源文件 |
| `concepts/` | 概念页 |
| `_archives/` | 归档内容（含 `domain-indexes/`、`release-notes/`） |
| `_meta/` | 元数据（本表、索引、dashboard） |
| `_reports/` | 冻结的报告文件 |

## 合并与归档记录（2026-07-10 重组）

| 重组动作 | 来源 | 去向 |
|---|---|---|
| 阿里云 3 归 1 | `云厂商/01-alibaba-cloud`、`05-alicloud-ack`、`15-alicloud-apsara-ack` | `云厂商/阿里云/{ack,apsara,通用}` |
| SLO/SLI 归一 | `可靠性/04-slo-sli`（4 篇） | `可观测性/SLO-SLI/` |
| 事件响应归一 | `安全/07-incident-response`（5 篇） | `生产运维/事件响应/` |
| eBPF 归一 | `专项技术/05-ebpf-programming`（4 篇） | `网络/eBPF/编程实践/` |
| 灾后演练并入 DR | `可靠性/09-disaster-recovery-playbooks`（11 篇） | `可靠性/灾难恢复/` |
| 结构化排障拆分 | `故障诊断/topic-structural-trouble-shooting/` | `故障诊断/资源排障/` + `故障诊断/高级排障/` |
| 其他云聚合 | `云厂商/09..14` 六个单文件目录 | `云厂商/其他云/` |
| 空壳 topic 删除 | `工作负载/topic-functions`（内容转 `平台工程/代码分析/`）、`生态参考/topic-release-notes` | — |
| 发布笔记归档 | `生态参考/_archived-release-notes` | `_archives/release-notes/`（扁平化） |
| 索引归档 | `*/98-merged-indexes/` × 20 | `_archives/domain-indexes/<domain>/` |

## 第二层命名约定（重组后）

1. **中文简称**：所有二级目录首选中文简称，无前缀编号。
2. **英文缩写/工具名保留**：`GitOps`、`IaC`、`kubectl`、`Docker`、`eBPF`、`SLO-SLI`、`K8s*`、`API*`、`JVM`、`CNCF`、`Linux`、`YAML`、`Kustomize`、`Helm`、`Operator`、`Terway`、`WebAssembly`、`AWS-EKS`、`Google-GKE`、`Azure-AKS`、`腾讯云TKE`、`华为云CCE`、`containerd-CRI-O`、`Java-on-K8s`、`AI-Agents` 等保持英文或中英混合。
3. **专题例外**：`知识字典/`、`代码分析/`、`技能体系/` 因自带分类体系，允许保留 3 级及更深结构。
4. **冻结目录**：`_archives/`、`_reports/`、`release/` 内容只增不改，旧路径引用保留原样。
5. **入口文件**：每个中文一级目录根有 `README.md`，作为域入口与导航；旧 `index.md` 如仍存在则仅做域内纯文本说明。

## 使用方式

- **新页面归档**：按上表第二列选择目标二级目录，直接放入；不再使用 `NN-` 编号。
- **wikilink 引用**：优先 `[[中文域/中文子目录/文件名]]`，例如 `[[故障诊断/资源排障/pod-pending-scheduling-failure]]`。
- **跨域引用**：使用 `[[中文域/README]]` 链接到域入口。
- **语料配置**：`corpus-config/profiles/*/include.path` 仍以旧 `domain-XX-<slug>/` 为准；迁移时再统一同步到中文路径。
- **历史追溯**：所有重组均使用 `git mv`，可通过 `git log --follow` 追溯文件历史。
