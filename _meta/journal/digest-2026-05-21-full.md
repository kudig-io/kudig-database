---
title: Wiki 全量知识库摘要 — 2026-05-21
description: 3,653 篇文档 · 175.5 万行 · 68MB · 40 领域 + 21 专题
category: journal
tags:
- digest
- meta/review
- k8s
- cncf
- wiki-ingest
- 全量分析
- etcd
- apiserver
- kubelet
- scheduler
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Wiki 全量知识库摘要 — 2026-05-21 是什么
- 如何 Wiki 全量知识库摘要 — 2026-05-21
trigger_keywords:
- Wiki
- 全量知识库摘要
- '2026-05-21'
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- iac-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- redis-basics
- mysql-basics
- gpu-scheduling-basics
- policy-basics
created: "2026-05-23"
---

# Wiki 全量知识库摘要 — 2026-05-21

> 3,653 篇文档 · 175.5 万行 · 68MB · 40 领域 + 21 专题
> 扫描范围：全库（含源文档 + wiki 页面 + 发布记录 + 报告）

---

## 核心洞察

1. **这是一份企业级 K8s 生产运维知识体系**——不是教程合集，而是面向真实生产场景的结构化知识库。40 个领域从架构基础到灾难恢复全覆盖，96 个行业架构模式从电商到量子计算全覆盖。

2. **FTA（故障树分析）是整个知识库的操作核心**——44 棵故障树覆盖 K8s 全组件，被 144 条跨域链接引用（最高跨域链接密度）。这是区别于所有其他 K8s 文档集的独特资产。

3. **CNCF 生态全景覆盖 236 个项目**——是全库最大单域（domain-34），配合发布记录中的 1,323 条 changelog，构成了一套从项目选型到版本追踪的完整参考体系。

4. **应用架构模式库覆盖 96 个行业**——从电商、金融到脑机接口、量子计算，每个模式都是完整的 K8s 生产架构设计文档。这是面向阿里云视角的行业解决方案知识图谱。

5. **学习路径体系包含 137 篇培训文档**——分基础、内训、公训、快速入门、故障排查 5 个子路径，支持从零到 oncall 的完整培训周期。

---

## 知识库全景

### 文档规模

| 指标 | 数值 |
|---|---|
| 文档总数 | 3,653 篇 |
| 总行数 | 1,755,074 行 |
| 总大小 | 67,928,585 字节 (68MB) |
| 平均大小 | 18,625 字节/篇 |
| 最大文档 | 502KB（K8s CHANGELOG-1.19） |
| Frontmatter 覆盖率 | 98% |
| 交叉引用总数 | 15,306 条 |
| 唯一链接目标 | 2,041 个 |
| 含交叉引用的文档 | 1,292 篇 |

### 文件大小分布

| 区间 | 数量 | 占比 |
|---|---|---|
| <1KB | 212 | 6% |
| 1-5KB | 1,262 | 35% |
| 5-10KB | 663 | 18% |
| 10-20KB | 578 | 16% |
| 20-50KB | 682 | 19% |
| >50KB | 250 | 7% |

---

## 内容类型分布

| 类型 | 文档数 | 说明 |
|---|---|---|
| 专题指南 (topic-*) | 2,277 | 行业架构、FTA、学习路径、发布记录等 |
| 领域知识 (domain-*) | 1,088 | 40 个 K8s 核心领域 |
| Wiki 页面 | 100 | 结构化概念/实体/技能/参考/综合 |
| 报告 (reports/) | 23 | 分析报告、执行报告 |
| 工具/模板 | 22 | 人页面、提示词、模板、脚本 |
| 发布记录 (release-notes/) | 22 | 产品发布材料 |
| 其他 | 115 | 索引、配置、变更日志等 |

---

## 40 个领域知识域

### 核心 K8s 体系（8 域，263 篇）

| 领域 | 文档数 | 主题 |
|---|---|---|
| domain-01-cluster-fundamentals 架构基础 | 35 | K8s 架构分层、组件关系、集群拓扑 |
| domain-01-cluster-fundamentals 设计原则 | 22 | 声明式 API、控制循环、最终一致性、Watch 机制 |
| domain-01-cluster-fundamentals 控制平面 | 39 | API Server、etcd、Scheduler、Controller Manager、Cloud Controller |
| domain-02-workloads-applications 工作负载 | 30 | Pod、Deployment、StatefulSet、DaemonSet、Job、HPA |
| domain-03-networking-traffic 网络 | 57 | [[cni|cni]]、Service、Ingress、DNS、NetworkPolicy、负载均衡 |
| domain-04-storage-data 存储 | 21 | PV/PVC/StorageClass、CSI、OverlayFS、快照、数据迁移 |
| domain-05-security-compliance 安全 | 24 | RBAC、准入控制、Pod 安全、Secret 管理、审计 |
| domain-06-observability 可观测性 | 35 | Prometheus、Grafana、日志聚合、分布式追踪、告警 |

### 平台运维（6 域，129 篇）

| 领域 | 文档数 | 主题 |
|---|---|---|
| domain-07-platform-engineering 平台运维 | 31 | 集群生命周期、升级策略、多集群管理、成本优化 |
| domain-15-specialized-tech 扩展生态 | 22 | CRD、Operator、Aggregated API、Webhook、调度器扩展 |
| domain-11-production-operations 生产运维 | 34 | 最佳实践、变更管理、容量规划、SLA/SLO/SLI |
| domain-06-observability 企业监控告警 | 15 | 告警规则、值班轮转、事件管理、根因分析 |
| domain-06-observability 日志管理分析 | 12 | EFK/PLG、日志采集、结构化日志、合规审计 |
| domain-07-platform-engineering 平台工程 | 15 | IDP、开发者门户、自助服务、平台即产品 |

### 云原生生态（10 域，405 篇）

| 领域 | 文档数 | 主题 |
|---|---|---|
| domain-13-[[container-runtime]] Docker | 16 | Docker 架构、镜像构建、安全扫描、多阶段构建 |
| domain-17-system-foundation Linux | 13 | 命名空间、cgroup、文件系统、内核调优 |
| domain-03-networking-traffic 镜像管理 | 11 | 镜像仓库、Harbor、签名、SBOM、漏洞扫描 |
| domain-08-release-change-management GitOps/CI-CD | 15 | [[argo|argo]]CD、Flux、Tekton、渐进式交付、Git 工作流 |
| domain-08-release-change-management IaC | 9 | Terraform、Pulumi、Crossplane、配置漂移检测 |
| domain-05-security-compliance 云原生安全 | 18 | 零信任、OPA/Kyverno、运行时安全、密钥管理 |
| domain-03-networking-traffic 服务网格 | 16 | Istio、[[envoy|Envoy]]、mTLS、流量管理、可观测性 |
| domain-19-landscape-references CNCF 全景 | **236** | CNCF 毕业/孵化/沙箱项目全覆盖 |
| domain-05-security-compliance 供应链安全 | 14 | SBOM、签名验证、SLSA、漏洞管理 |
| domain-03-networking-traffic API 网关 | 18 | APISIX、Kong、Envoy Gateway、限流、认证 |

### 高级主题（14 域，263 篇）

| 领域 | 文档数 | 主题 |
|---|---|---|
| domain-14-ai-ml-infra AI 基础设施 | 41 | GPU 调度、模型服务、分布式训练、推理优化 |
| domain-10-troubleshooting-diagnostics 故障排查 | 51 | 系统化排查方法、日志分析、性能调优 |
| domain-03-networking-traffic 网络基础 | 10 | TCP/IP 协议栈、路由、交换、负载均衡 |
| domain-04-storage-data 存储基础 | 9 | 块存储、文件存储、对象存储、存储性能 |
| domain-12-cloud-providers 云厂商指南 | 26 | 阿里云 ACK、AWS EKS、Azure AKS、GCP GKE |
| domain-19-landscape-references 技术论文 | 29 | K8s 高级技术论文、最佳实践白皮书 |
| domain-12-cloud-providers 多云混合云 | 13 | 多云管理、混合云架构、集群联邦 |
| domain-16-database-middleware 数据库中间件 | 12 | MySQL/PostgreSQL on K8s、Redis、消息队列 |
| domain-08-release-change-management 自动化测试 | 8 | E2E 测试、混沌工程、性能测试、合规测试 |
| domain-09-reliability-engineering 灾备连续性 | 12 | 备份恢复、跨区容灾、RPO/RTO、故障切换 |
| domain-17-system-foundation 硬件基础设施 | 21 | 服务器、GPU、网卡、NVMe、DPU/SmartNIC |
| domain-18-manifests-patterns YAML 参考 | 39 | K8s YAML 配置完整参考手册 |
| domain-17-system-foundation Events 事件 | 18 | K8s 全域事件分类与诊断 |
| domain-03-networking-traffic eBPF | 13 | eBPF 编程模型、Cilium、Tetragon |
| domain-15-specialized-tech 边缘计算 | 14 | KubeEdge、边缘节点管理、边缘自治 |
| domain-15-specialized-tech WebAssembly | 14 | Wasm 运行时、Spin、containerd-wasm |

---

## 21 个专题目录

### 核心专题

| 专题 | 文档数 | 说明 |
|---|---|---|
| topic-release-notes | **1,323** | CNCF 项目发布记录存档（8 大类） |
| topic-dictionary | 209 | 知识字典：术语定义、概念辨析 |
| topic-learn | 137 | 学习培训体系（5 个子路径） |
| topic-application-architecture | 98 | 96 个行业 K8s 架构设计模式 |
| topic-functions | 89 | 集群操作函数库 |
| topic-fta | **82** | FTA 故障树分析方法论（含 44 棵故障树） |
| topic-structural-trouble-shooting | 73 | 结构化故障排查知识库 |
| topic-ai-agent | 59 | AI Agent 工程专题 |
| topic-skills | 37 | 工单智能体 K8s 诊断 Skill 库 |
| topic-ai-coding | 26 | AI 编程与 LLM 网关专题 |
| topic-index | 25 | 深度研究入口使用指南 |
| topic-scenarios | 22 | 生产场景导航 |
| topic-best-practices | 15 | K8s 最佳实践指南 |
| topic-cheat-sheet | 15 | 速查卡 |
| topic-presentations | 14 | 技术演示文稿 |

### 专项专题

| 专题 | 文档数 | 说明 |
|---|---|---|
| topic-febm | 12 | FEBM 法医鉴定循证方法论 |
| topic-migration | 12 | 自建 K8s 迁移至阿里云 ACK 指南 |
| topic-publish | 12 | 技术论坛全域发布战略方案 |
| topic-deployment | 6 | 部署方案指南 |
| domain-java-[[kubernetes|kubernetes]] | 8 | Java on K8s 实践 |
| topic-qa-corpus | 3 | Agent QA 对语料库 |

### 发布记录详细分类（topic-release-notes）

| 项目类别 | Changelog 数量 |
|---|---|
| 可观测性 (observability) | 374 |
| 安全 (security) | 218 |
| CLI 工具 (cli-tools) | 187 |
| CI/CD & GitOps | 171 |
| 网络 (networking) | 157 |
| 核心依赖 (core-deps) | 83 |
| 存储 (storage) | 76 |
| Kubernetes | 55 |

---

## 交叉引用网络

### 跨域链接 Top 10

| 源域 | 目标域 | 链接数 | 含义 |
|---|---|---|---|
| domain-10-troubleshooting-diagnostics 故障排查 | topic-fta | **144** | 故障排查方法论全面引用 FTA 故障树 |
| topic-scenarios | (多域) | 89 | 场景导航链接到各领域 |
| domain-17-system-foundation 硬件 | topic-fta | 21 | 硬件问题也用 FTA 分析 |
| domain-03-networking-traffic 网络 | topic-fta | 17 | 网络问题用 FTA 定位 |
| domain-06-observability 可观测性 | topic-fta | 6 | 监控告警触发 FTA 诊断 |
| topic-结构化排查 | topic-fta | 10 | 排查框架引用 FTA |
| domain-04-storage-data 存储 | topic-fta | 3 | 存储问题 FTA |
| domain-02-workloads-applications 工作负载 | topic-fta | 3 | 工作负载问题 FTA |
| domain-07-platform-engineering 平台运维 | topic-fta | 3 | 平台运维引用 FTA |
| domain-05-security-compliance 安全 | (多域) | 3 | 安全领域交叉引用 |

**关键发现**：FTA 是整个知识库的跨域枢纽——144 条从 domain-10-troubleshooting-diagnostics 到 topic-fta 的链接说明故障排查方法论完全建立在 FTA 框架之上。

### Wiki 页面链接网络（Top Hub 页面）

| Hub 页面 | 入链数 | 角色 |
|---|---|---|
| kubernetes-architecture-overview | **59** | 主枢纽——所有控制平面组件的汇聚点 |
| pod-lifecycle | 17 | 工作负载生命周期的核心参考 |
| security-defense-depth | 15 | 安全分层防御的核心参考 |
| production-operations-best-practices | 15 | 生产运维的核心参考 |
| controller-pattern | 14 | 理解所有控制器行为的基础 |
| resource-management | 13 | 资源管理的核心参考 |
| linux-container-foundation | 13 | 容器技术的基础 |
| gitops-principles | 12 | GitOps 的核心参考 |

---

## FTA 故障树体系

44 棵故障树覆盖 K8s 全组件，是知识库最核心的操作资产：

| 故障树 | 覆盖组件 |
|---|---|
| apiserver-fta | API Server 异常 |
| etcd-fta | etcd 异常 |
| scheduler-fta | 调度器异常 |
| [[domain-10-troubleshooting-diagnostics/topic-fta/list/controller-manager-fta.md|controller-manager-fta]] | 控制器管理器异常 |
| kubelet-fta | Kubelet 异常 |
| calico-fta | Calico CNI 问题 |
| cilium-fta | Cilium eBPF 问题 |
| dns-fta | DNS 问题 |
| [[domain-10-troubleshooting-diagnostics/topic-fta/list/ingress-fta.md|ingress-fta]] | Ingress 问题 |
| storage-fta | 存储问题 |
| certificate-fta | 证书异常 |
| backup-restore-fta | 备份恢复异常 |
| cluster-upgrade-fta | 集群升级异常 |
| [[domain-10-troubleshooting-diagnostics/topic-fta/list/cluster-autoscaler-fta.md|cluster-autoscaler-fta]] | 自动伸缩异常 |
| [[skills/cloud-provider-fta.md|cloud-provider-fta]] | 云平台集成异常 |
| ... | （共 44 棵） |

---

## 96 个行业架构模式

topic-application-architecture 包含 96 个完整的 K8s 生产架构设计文档，覆盖：

**传统行业（01-20）**：电商、小程序、CMS、IM/RTC、在线教育、金融科技、IoT、AI/ML 推理、游戏后端、社交媒体、智慧零售、智慧物流、数字政务、智慧医疗、能源电力、音视频/短视频、SaaS 多租户、数据中台、DevOps 平台、微服务治理

**新兴行业（21-50）**：跨境电商、新能源车联网、信创替代、保险科技、量化交易、航空出行、酒店旅游、地产科技、农业物联网、HR SaaS、即时零售、智慧餐饮、海外仓、体育科技、元宇宙数字孪生、碳资产/ESG、宠物经济、供应链金融、智慧园区、云游戏、美妆电商、二手循环经济、企业 IM、数字营销、智慧港口、卫星互联网、智慧矿山、职业教育、直播电商、无人零售

**前沿科技（51-96）**：智能制造 MES、智慧水务、新零售 DTC、社交游戏元宇宙、跨境独立站、智慧养老、数字疗法、Web3 GameFi、工业互联网、自动驾驶、智慧电网、分布式能源、工业视觉检测、AI 制药、自动驾驶仿真、太空互联网、脑机接口、量子计算、6G 核心网、数字人民币、智慧税务、数字孪生城市、智慧消防、XR 沉浸式、情感计算、合成生物学、可控核聚变、深海探测、极地科考、TSN 网络、智慧海关、司法科技、文化数字化、国家公园、氢能源、固态电池、柔性制造、纳米材料、CRISPR 基因编辑、类脑计算、低空经济/UAM、智慧体育场馆、数字孪生工厂、智慧监狱、工业元宇宙、CCUS 碳捕集

---

## 学习路径体系（topic-learn）

| 子路径 | 文档数 | 说明 |
|---|---|---|
| public-training | 65 | 公开培训课程体系 |
| inner-training | 46 | 内部培训材料 |
| fundamentals | 15 | K8s 基础知识 |
| quick-start | 5 | 快速入门指南 |
| resources | 2 | 学习资源汇总 |
| troubleshooting | 1 | 故障排查培训 |
| oncall-qa | 1 | OnCall 问答 |

---

## Wiki 页面状态

| 类别 | 数量 | 生命周期 |
|---|---|---|
| 概念 (concepts/) | 39 | 全部 draft |
| 实体 (entities/) | 24 | 全部 draft |
| 技能 (skills/) | 15 | 全部 draft |
| 参考 (references/) | 19 | 全部 draft |
| 综合 (synthesis/) | 3 | 全部 draft |
| 项目 (projects/) | 1 | — |
| 日志 (journal/) | 1 | 本次新增 |

**所有 100 个 wiki 页面均处于 lifecycle:draft 状态**——尚未发布为稳定版本。

---

## 标签体系

全库 Top 20 标签：

| 标签 | 文档数 | 说明 |
|---|---|---|
| k8s | 122 | K8s 相关文档 |
| MOC|MOC]] | 63 | 知识导航地图 |
| scenario | 21 | 生产场景 |
| k8s-1.28-1.33 | 19 | K8s 版本特定内容 |
| hands-on | 15 | 动手实践 |
| week-2 | 14 | 培训第 2 周 |
| networking | 12 | 网络 |
| security | 9 | 安全 |
| week-1 / week-4 | 各 9 | 培训进度 |
| deployment | 7 | 部署 |
| storage | 7 | 存储 |
| troubleshooting | 6 | 故障排查 |
| docker | 6 | Docker |
| service | 6 | 服务 |
| kubernetes | 6 | K8s 通用 |
| linux / training | 各 5 | Linux、培训 |
| ingress / namespace | 各 4 | 网络、隔离 |

**注意**：尚无 `_meta/taxonomy.md`——标签词汇表未正式建立。

---

## 待办事项（Open Threads）

| 类型 | 数量 | 说明 |
|---|---|---|
| Draft 页面 | 100 | 所有 wiki 页面均为 draft，需审核发布 |
| 未编排标签 | — | `_meta/taxonomy.md` 缺失，标签体系未规范化 |
| _insights.md | 缺失 | 尚未生成洞察报告（hub 页面、桥接模式等） |
| _raw/ 未暂存 | 0 | 干净状态，无待处理原始文件 |
| Frontmatter 缺失 | 2% | 约 68 篇源文档缺少 frontmatter |

---

## 推荐重读

| 页面 | 原因 |
|---|---|
| domain-10-troubleshooting-diagnostics | FTA 跨域链接的核心枢纽（144 条引用），确保方法论准确 |
| domain-10-troubleshooting-diagnostics/topic-fta/list/ | 44 棵故障树是整个知识库的操作核心，需逐一验证 |
| domain-19-landscape-references | 236 个 CNCF 项目是最大单域，确保覆盖度和准确性 |
| domain-20-application-patterns/topic-application-architecture/ | 96 个行业模式需检查是否与最新阿里云产品对齐 |
| concepts/kubernetes-architecture-overview | Wiki 主枢纽（59 入链），所有页面依赖它 |

---

## 知识库独特价值

1. **FTA 故障树体系**——44 棵结构化故障树，144 条跨域引用，是区别于所有其他 K8s 文档集的核心资产
2. **96 行业架构模式**——从电商到量子计算的完整 K8s 生产架构设计，行业覆盖度极高
3. **1,323 条发布记录**——CNCF 8 大类项目的版本变更存档，支持版本追踪和升级决策
4. **AI Agent 诊断能力**——59 篇 AI Agent 专题 + 37 篇诊断 Skill + 82 篇 FTA 方法论，构成智能运维知识基座
5. **学习路径完整**——137 篇培训文档，从快速入门到 OnCall 值班全覆盖

---

*由 wiki-digest 生成 · 2026-05-21 · 3,653 篇文档扫描*
*存储库：/Users/allengaller/Documents/GitHub/kudig-io/kudig-database*

## Related

- [[concepts/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[pod-lifecycle]] — Pod Lifecycle
- [[concepts/ai-agent-README.md|ai-agent-README]] — AI Agent 工程专题
- [[concepts/gitops-principles.md|gitops-principles]] — GitOps Principles and Practice
- [[concepts/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security

- [[entities/tinkerbell.md|Tinkerbell]]
- [[entities/krkn.md|Krkn]]
- [[entities/chaosblade.md|ChaosBlade]]
- [[entities/vineyard.md|Vineyard]]