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
last_updated: '2026-08-25'
---

# KUDIG Domain 目录映射表

> 本表记录 2026-07-23 数字前缀有序化重组后的最终结构。
> 原则：全库 37 个一级目录统一加 `NN-` 两位数字前缀（01-37，按领域重要性/依赖关系排序）；二级目录在知识域（20 域 + 最佳实践 + 生态参考）与 wiki 分类目录（概念/实体/技能）范围内同样加 `NN-` 前缀；目录主体名保持中文简称（英文缩写/工具名如 GitOps/IaC/kubectl/Docker/eBPF 保留）；全库原则上只保留两层，专题（知识字典、代码分析、技能体系）因自带分类允许 3 层。
> 排序逻辑：核心基础设施（01）→ 工作负载与应用（02-04）→ 网络存储数据（05-07）→ 安全与可观测性（08-09）→ 平台运维（10-13）→ 运行时与专项技术（14-17）→ 云厂商（18）→ 故障诊断与实践参考（19-21）→ wiki 分类（22-27）→ 支撑域（28-37）。

## 中文知识域（01-21）

| # | 一级目录 | Taxonomy Tag | 二级子目录（NN-中文简称） |
|---:|---|---|---|
| 01 | `01-集群基础/` | `domain/cluster-fundamentals` | `01-架构总览/`, `02-设计原则/`, `03-控制平面/`, `04-API版本/`, `05-kubectl/`, `06-升级路径/`, `07-性能调优/` |
| 02 | `02-工作负载/` | `domain/workloads-applications` | `01-核心工作负载/`, `02-Java-on-K8s/`, `03-Node-js-on-K8s/`, `04-多语言运行时/` |
| 03 | `03-清单模式/` | `domain/manifests-patterns` | `01-YAML参考/`, `02-Kustomize模式/`, `03-Helm值模式/`, `04-Operator模式/`, `05-GitOps模式/`, `06-安全模式/`, `07-AI-ML模式/`, `08-韧性模式/`, `09-平台模式/` |
| 04 | `04-应用模式/` | `domain/application-patterns` | `01-子模式/`, `02-行业架构/`, `03-生产模式/` |
| 05 | `05-网络/` | `domain/networking-traffic` | `01-K8s网络核心/`, `02-网络基础/`, `03-服务网格/`, `04-API网关/`, `05-eBPF/`, `06-Terway/`, `07-附件/` |
| 06 | `06-存储/` | `domain/storage-data` | `01-K8s存储/`, `02-存储基础/`, `03-分布式存储/`, `04-有状态应用存储/`, `05-存储网络/`, `06-云存储对比/`, `07-AI存储与高级/` |
| 07 | `07-数据库中间件/` | `domain/database-middleware` | `01-数据库/`, `02-缓存/`, `03-消息队列/`, `04-时序数据库/`, `05-Operator管理/`, `06-数据流/`, `07-搜索引擎/`, `08-新型数据库/` |
| 08 | `08-安全/` | `domain/security-compliance` | `01-身份与访问/`, `02-网络安全/`, `03-运行时安全/`, `04-策略治理/`, `05-供应链/`, `06-合规审计/`, `07-零信任架构/` |
| 09 | `09-可观测性/` | `domain/observability` | `00-总览/`, `01-指标/`, `02-日志/`, `03-链路追踪/`, `04-告警/`, `05-SLO-SLI/`, `06-工具/` |
| 10 | `10-平台工程/` | `domain/platform-engineering` | `00-总览/`, `01-构建/`, `02-运维/`, `03-治理/`, `04-开发体验/`, `05-内部开发者平台/`, `06-代码分析/` |
| 11 | `11-发布变更/` | `domain/release-change-management` | `01-GitOps/`, `02-IaC/`, `03-Progressive-Delivery/`, `04-变更管理/`, `05-测试质量/`, `06-部署方案/`, `07-迁移方案/` |
| 12 | `12-可靠性/` | `domain/reliability-engineering` | `01-备份恢复/`, `02-灾难恢复/`, `03-容量规划/`, `04-混沌工程/`, `05-事后复盘/`, `06-SRE实践/`, `07-性能测试/` |
| 13 | `13-生产运维/` | `domain/production-operations` | `01-成本治理/`, `02-集群治理/`, `03-事件响应/`, `04-绿色计算/`, `05-工单案例/`, `06-回复话术/` |
| 14 | `14-容器运行时/` | `domain/container-runtime` | `01-Docker/`, `02-镜像管理/`, `03-containerd-CRI-O/`, `04-镜像构建/`, `05-运行时迁移/` |
| 15 | `15-AI基础设施/` | `domain/ai-ml-infra` | `01-基础设施/`, `02-AI-Agents/`, `03-Agent运行时/`, `04-AI编码/`, `05-K8s-AI基础设施/` |
| 16 | `16-专项技术/` | `domain/specialized-tech` | `01-边缘计算/`, `02-WebAssembly/`, `03-扩展机制/`, `04-无服务器/` |
| 17 | `17-系统基础/` | `domain/system-foundation` | `01-Linux/`, `02-硬件/`, `03-网络基础/`, `04-K8s事件/`, `05-速查卡/`, `06-知识字典/` |
| 18 | `18-云厂商/` | `domain/cloud-providers` | `01-阿里云/`, `02-AWS-EKS/`, `03-Google-GKE/`, `04-Azure-AKS/`, `05-腾讯云TKE/`, `06-华为云CCE/`, `07-多云混合/`, `08-其他云/` |
| 19 | `19-故障诊断/` | `domain/troubleshooting-diagnostics` | `01-核心排障/`, `02-资源排障/`, `03-基础设施排障/`, `04-高级排障/`, `05-JVM调优/`, `06-FTA故障树/`, `07-FEBM方法论/`, `08-技能体系/`, `09-多故障场景/`, `10-QA语料/`, `11-工具/` |
| 20 | `20-最佳实践/` | `domain/best-practices` | `01-best-practices/`, `02-deployment/`, `03-infrastructure/`, `04-migration/`, `05-observability/`, `06-operations/`, `07-scenarios/`, `08-security/` |
| 21 | `21-生态参考/` | `domain/landscape-references` | `01-CNCF全景/`, `02-论文/`, `03-领域索引/` |

## wiki 分类与支撑域（22-37）

| 目录 | 原名 | 二级前缀 | 用途 |
|---|---|---|---|
| `22-概念/` | `concepts/` | 有（`00-总览/` … `15-运行时与系统/`） | 概念页（obsidian-wiki 分类） |
| `23-实体/` | `entities/` | 有（`01-research/`） | 实体页：工具/产品/公司（obsidian-wiki 分类） |
| `24-综合/` | `synthesis/` | 无 | 跨域综合文章（obsidian-wiki 分类） |
| `25-研究/` | `research/` | 无 | 研究性笔记 |
| `26-技能/` | `skills/` | 有（`01-集群运维/` … `08-可观测性/`） | Skill 定义（obsidian-wiki 分类） |
| `27-标签/` | `tags/` | 无 | 标签索引页 |
| `28-资产/` | `assets/` | 无 | 图片、PDF 等静态资产 |
| `29-文档/` | `docs/` | 无 | 项目级说明文档 |
| `30-站点/` | `web/` | 无 | Astro 站点源文件（.gitignore 忽略） |
| `31-脚本/` | `scripts/` | 无 | 维护脚本（含 `maintenance/rename-prefix-20260723.py`、`rewrite-prefix-links-20260723.py`） |
| `32-发布/` | `release/` | 无 | 发布包与打包产物（冻结） |
| `33-源码/` | `code/` | 无 | vendor 源码树（Kubernetes release、terway 等，.gitignore 忽略） |
| `34-源码分析/` | — | 无 | 源码分析笔记 |
| `35-元数据/` | `_meta/` | 无 | 元数据（本表、索引、dashboard），本表所在位置 |
| `36-报告/` | `_reports/` | 无 | 冻结的报告文件 |
| `37-归档/` | `_archives/` | 无 | 归档内容（含 `domain-indexes/`、`release-notes/`） |

## 合并与归档记录（2026-07-10 重组，路径为当时旧名）

| 重组动作 | 来源 | 去向 |
|---|---|---|
| 阿里云 3 归 1 | `云厂商/01-alibaba-cloud`、`05-alicloud-ack`、`15-alicloud-apsara-ack` | `云厂商/阿里云/{ack,apsara,通用}` |
| SLO/SLI 归一 | `可靠性/04-slo-sli`（4 篇） | `可观测性/SLO-SLI/` |
| 事件响应归一 | `安全/07-incident-response`（5 篇） | `生产运维/事件响应/` |
| eBPF 归一 | `专项技术/05-ebpf-programming`（4 篇） | `网络/eBPF/编程实践/` |
| 灾后演练并入 DR | `可靠性/09-disaster-recovery-playbooks`（11 篇） | `可靠性/灾难恢复/` |
| 结构化排障拆分 | `19-故障诊断/04-高级排障/structural-` | `故障诊断/资源排障/` + `19-故障诊断/04-高级排障/` |
| 其他云聚合 | `云厂商/09..14` 六个单文件目录 | `云厂商/其他云/` |
| 空壳 topic 删除 | `工作负载/topic-functions`（内容转 `平台工程/代码分析/`）、`生态参考/topic-release-notes` | — |
| 发布笔记归档 | `生态参考/_archived-release-notes` | `_archives/release-notes/`（扁平化） |
| 索引归档 | `*/98-merged-indexes/` × 20 | `_archives/domain-indexes/<domain>/` |

## 命名约定（2026-07-23 前缀有序化后）

1. **数字前缀**：一级目录统一 `NN-`（01-37）前缀；知识域与 wiki 分类目录（概念/实体/技能）的二级目录同样使用 `NN-` 前缀，编号即域内排序。
2. **中文简称主体**：前缀后的目录主体名保持中文简称；英文缩写/工具名保留：`GitOps`、`IaC`、`kubectl`、`Docker`、`eBPF`、`SLO-SLI`、`K8s*`、`API*`、`JVM`、`CNCF`、`Linux`、`YAML`、`Kustomize`、`Helm`、`Operator`、`Terway`、`WebAssembly`、`AWS-EKS`、`Google-GKE`、`Azure-AKS`、`腾讯云TKE`、`华为云CCE`、`containerd-CRI-O`、`Java-on-K8s`、`AI-Agents` 等保持英文或中英混合。
3. **专题例外**：`06-知识字典/`、`06-代码分析/`、`08-技能体系/` 因自带分类体系，允许保留 3 级及更深结构；三级及以下目录不加前缀。
4. **冻结目录**：`37-归档/`、`36-报告/`、`32-发布/` 内容只增不改，仅目录自身改名，内部旧路径引用保留原样。
5. **入口文件**：每个知识域一级目录根有 `README.md`，作为域入口与导航；旧 `index.md` 如仍存在则仅做域内纯文本说明。
6. **构建/忽略目录**：`30-站点/`、`33-源码/` 被 .gitignore 忽略，不在主库追踪范围；其内部结构不受本约定约束。

## 使用方式

- **新页面归档**：按上表第四列选择目标二级目录，直接放入；新增二级目录时顺延编号（域内下一个可用 `NN-`）。
- **wikilink 引用**：优先 `[[NN-中文域/NN-中文子目录/文件名]]`，例如 `[[19-故障诊断/02-资源排障/pod-pending-scheduling-failure]]`。
- **跨域引用**：使用 `[[NN-中文域/README]]` 链接到域入口。
- **语料配置**：`35-元数据/corpus-config/profiles/*` 的 `include.path` 已同步为带前缀的新路径。
- **历史追溯**：所有重组均使用 `git mv`，可通过 `git log --follow` 追溯文件历史；2026-07-23 前缀映射权威来源见 `31-脚本/maintenance/rename-prefix-20260723.py`。
