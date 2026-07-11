---
title: KUDIG 内容审计报告 (2026-07-11)
category: audit
tags: [audit, quality, coverage]
tier: supporting
created: 2026-07-11
---

# KUDIG 内容审计报告 (2026-07-11)

> 对 20 个中文知识域的全面审计：量化指标、覆盖缺口、深度抽检、跨域重复。

## 执行摘要

- **全库总页数**: 2332（不含 index/README/MOC）
- **平均 frontmatter 完整度**: 99.9%（2329/2332）
- **陈旧页面数**: 0（所有 last_updated >= 2025-12）
- **主要短板**:
  - Z1: 故障诊断域占全库 15.5%，体量失衡，含大量 QA 语料/脚本等辅助文件
  - Z2: 系统基础域 608 页（占 26%），其中知识字典条目过多，信噪比低
  - Z3: 难度分布严重偏斜——全库 advanced 占 54%，beginner 仅 2.5%
- **重点建议**:
  - R1: 将系统基础/知识字典的 ~400 词条提炼为精简参考，降低页面膨胀感
  - R2: 故障诊断域的 QA 语料、脚本目录应归入数据资产而非内容页面
  - R3: 增加 beginner 级别内容，构建新手学习路径

---

## 域级明细

### 1. 集群基础

| 指标 | 值 |
|---|---|
| 页面数（不含 index/README/MOC） | 97 |
| 二级子目录 | API版本, kubectl, 架构总览, 控制平面, 设计原则, 升级路径, 性能调优 (7) |
| Tier 分布 | core: 8, supporting: 21, peripheral: 78 |
| Difficulty 分布 | beginner: 1, intermediate: 22, advanced: 95, expert: 0 |
| Frontmatter 完整度 | 97/97 = 100% |
| 字数估算 | ~10.2 万行 / ~290 万字符 |

**覆盖缺口**
- 集群多租户隔离策略
- 集群成本分摊模型
- 自定义调度器开发指南
- 集群审计日志配置实践

**深度抽检（3 页）**
- `集群基础/控制平面/01-plane-architecture-overview.md`: deep / 18 代码块 / 有命令 / 无 mermaid / 有 cross_refs
- `集群基础/控制平面/11-etcd-deep-dive.md`: deep / 丰富命令与配置
- `集群基础/kubectl/05-kubectl-commands-reference.md`: medium / 命令参考型

**问题与机会**
- peripheral 占比 74%，核心内容被稀释；建议将部分 peripheral 提升为 supporting

### 2. 工作负载

| 指标 | 值 |
|---|---|
| 页面数 | 42 |
| 二级子目录 | Java-on-K8s, 核心工作负载 (2) |
| Tier 分布 | core: 12, supporting: 18, peripheral: 17 |
| Difficulty 分布 | beginner: 0, intermediate: 29, advanced: 17, expert: 0 |
| Frontmatter 完整度 | 42/42 = 100% |
| 字数估算 | ~3.6 万行 |

**覆盖缺口**
- Go/Rust/Python on K8s 垂直栈（仅 Java）
- Serverless 工作负载（Knative 在专项技术）
- GPU 工作负载调度
- 多集群工作负载分发

**深度抽检**
- `工作负载/核心工作负载/02-deployment-production-patterns.md`: deep / 代码块丰富
- `工作负载/Java-on-K8s/02-spring-boot-kubernetes-production.md`: deep / 完整生产指南
- `工作负载/核心工作负载/04-daemonset-management.md`: medium

**问题与机会**
- 子目录仅 2 个，覆盖面窄；建议增加 Go/Python/Node.js on K8s

### 3. 网络

| 指标 | 值 |
|---|---|
| 页面数 | 113 |
| 二级子目录 | API网关, eBPF, K8s网络核心, Terway, 服务网格, 附件, 网络基础 (7) |
| Tier 分布 | core: 14, supporting: 23, peripheral: 86 |
| Difficulty 分布 | beginner: 0, intermediate: 26, advanced: 88, expert: 10 |
| Frontmatter 完整度 | 113/113 = 100% |
| 字数估算 | ~11.5 万行 |

**覆盖缺口**
- DNS 高级优化（CoreDNS 已有，但缺少外部 DNS 集成）
- IPv6 双栈生产实践
- 多集群网络联邦
- 网络故障自愈机制

**深度抽检**
- `网络/K8s网络核心/01-network-architecture-overview.md`: deep / 26 代码块 / 35min 阅读
- `网络/eBPF/03-cilium-cni-architecture.md`: deep / 228 行 eBPF 引用
- `网络/服务网格/01-istio-enterprise-service-mesh.md`: deep

**问题与机会**
- Terway 内容重复（K8s网络核心 + Terway 独立子目录 + 附件中均有）
- eBPF 内容深度优秀，但 peripheral 比例过高

### 4. 存储

| 指标 | 值 |
|---|---|
| 页面数 | 39 |
| 二级子目录 | K8s存储, 存储基础, 分布式存储, 有状态应用存储 (4) |
| Tier 分布 | core: 5, supporting: 23, peripheral: 15 |
| Difficulty 分布 | beginner: 0, intermediate: 15, advanced: 29, expert: 0 |
| Frontmatter 完整度 | 39/39 = 100% |
| 字数估算 | ~3.4 万行 |

**覆盖缺口**
- 存储性能基准测试方法论
- 云厂商托管存储对比
- 存储多租户隔离
- 对象存储在 K8s 的最佳实践

**深度抽检**
- `存储/K8s存储/01-storage-architecture-overview.md`: deep / CSI/PV/PVC 全覆盖
- `存储/分布式存储/01-velero-backup-recovery.md`: deep / 104 行备份恢复
- `存储/K8s存储/08-storage-performance-tuning.md`: medium

**问题与机会**
- 体量偏小（39 页），建议补充云存储对比和性能基准

### 5. 安全

| 指标 | 值 |
|---|---|
| 页面数 | 58 |
| 二级子目录 | 策略治理, 供应链, 合规审计, 身份与访问, 网络安全, 运行时安全 (6) |
| Tier 分布 | core: 5, supporting: 61, peripheral: 7 |
| Difficulty 分布 | beginner: 0, intermediate: 15, advanced: 54, expert: 0 |
| Frontmatter 完整度 | 58/58 = 100% |
| 字数估算 | ~6.2 万行 |

**覆盖缺口**
- Secrets 管理最佳实践（已有但分散）
- Pod Security Standards 迁移指南
- 安全左移（DevSecOps）流水线
- 密钥轮换自动化

**深度抽检**
- `安全/身份与访问/01-authentication-authorization-system.md`: deep
- `安全/运行时安全/01-falco-cloud-native-security.md`: deep / Falco 规则示例
- `安全/供应链/01-supply-chain-security-overview.md`: deep

**问题与机会**
- supporting 占 92%，缺少更多 core 级别的锚定页面

### 6. 可观测性

| 指标 | 值 |
|---|---|
| 页面数 | 69 |
| 二级子目录 | SLO-SLI, 告警, 工具, 链路追踪, 日志, 指标, 总览 (7) |
| Tier 分布 | core: 10, supporting: 68, peripheral: 2 |
| Difficulty 分布 | beginner: 0, intermediate: 55, advanced: 15, expert: 0 |
| Frontmatter 完整度 | 69/69 = 100% |
| 字数估算 | ~6.2 万行 |

**覆盖缺口**
- OpenTelemetry Collector 深度配置
- eBPF 可观测性（与网络/eBPF 交叉）
- 可观测性成本优化
- AIOps 智能告警

**深度抽检**
- `可观测性/SLO-SLI/05-slo-implementation-guide.md`: deep / 121 行 SLO 引用
- `可观测性/总览/01-observability-architecture-overview.md`: deep
- `可观测性/工具/05-datadog-enterprise-apm.md`: medium

**问题与机会**
- SLO-SLI 子目录内容极为丰富（7 页 core），但与可靠性域有重叠

### 7. 平台工程

| 指标 | 值 |
|---|---|
| 页面数 | 204 |
| 二级子目录 | 代码分析, 构建, 开发体验, 运维, 治理 (5) |
| Tier 分布 | core: 15, supporting: 114, peripheral: 105 |
| Difficulty 分布 | beginner: 7, intermediate: 188, advanced: 126, expert: 14 |
| Frontmatter 完整度 | 204/204 = 100% |
| 字数估算 | ~13.8 万行 |

**覆盖缺口**
- 自助服务门户设计模式
- 平台成熟度评估模型
- 开发者门户与 API 文档

**深度抽检**
- `平台工程/构建/01-platform-engineering-overview.md`: deep
- `平台工程/开发体验/09-developer-experience-metrics.md`: deep / DX 指标体系
- `平台工程/代码分析/cluster-create/` 系列: medium（代码分析型）

**问题与机会**
- 代码分析子目录含大量自动生成文件（每个函数一个目录），拉高页面数
- peripheral 比例 51%，大量代码解析页面可考虑归档

### 8. 发布变更

| 指标 | 值 |
|---|---|
| 页面数 | 50 |
| 二级子目录 | GitOps, IaC, 变更管理, 部署方案, 测试质量, 迁移方案 (6) |
| Tier 分布 | core: 21, supporting: 20, peripheral: 19 |
| Difficulty 分布 | beginner: 0, intermediate: 25, advanced: 41, expert: 0 |
| Frontmatter 完整度 | 50/50 = 100% |
| 字数估算 | ~4.7 万行 |

**覆盖缺口**
- Feature Flag 管理
- 蓝绿/金丝雀自动化（Argo Rollouts 已有）
- 数据库迁移策略
- 回滚决策自动化

**深度抽检**
- `发布变更/GitOps/10-gitops-pipeline-practices.md`: deep / 61 行引用
- `发布变更/GitOps/08-fleet-gitops-operations-guide.md`: deep / 75 行
- `发布变更/变更管理/01-change-window-and-approval.md`: medium

**问题与机会**
- core 比例最高（42%），结构健康

### 9. 可靠性

| 指标 | 值 |
|---|---|
| 页面数 | 50 |
| 二级子目录 | SRE实践, 备份恢复, 混沌工程, 容量规划, 事后复盘, 性能测试, 灾难恢复 (7) |
| Tier 分布 | core: 7, supporting: 26, peripheral: 15 |
| Difficulty 分布 | beginner: 0, intermediate: 21, advanced: 34, expert: 0 |
| Frontmatter 完整度 | 50/50 = 100% |
| 字数估算 | ~3.2 万行 |

**覆盖缺口**
- 弹性模式（circuit breaker, bulkhead, retry）
- 多活架构设计
- 故障演练自动化平台
- 可靠性量化指标体系

**深度抽检**
- `可靠性/混沌工程/01-chaos-engineering-overview.md`: medium / 5 代码块 / 原则文档
- `可靠性/SRE实践/02-release-gate-slo-based.md`: deep / 66 行 SLO 引用
- `可靠性/灾难恢复/12-disaster-recovery-bc-runbook-v1.md`: deep

**问题与机会**
- 体量适中，结构与可观测性/SLO-SLI 有重叠

### 10. 故障诊断

| 指标 | 值 |
|---|---|
| 页面数 | 362 |
| 二级子目录 | FEBM方法论, FTA故障树, JVM调优, QA语料, 多故障场景, 高级排障, 工具, 核心排障, 基础设施排障, 技能体系, 资源排障 (11) |
| Tier 分布 | core: 222, supporting: 228, peripheral: 49 |
| Difficulty 分布 | beginner: 3, intermediate: 84, advanced: 227, expert: 12 |
| Frontmatter 完整度 | 361/363 = 99% |
| 字数估算 | ~38.3 万行 |

**覆盖缺口**
- 存储层故障诊断已有但分散
- AI/ML 工作负载排障（已有但初期）
- 多集群排障

**深度抽检**
- `故障诊断/高级排障/structural-01-control-plane/01-apiserver-troubleshooting.md`: deep
- `故障诊断/资源排障/09-node-comprehensive-troubleshooting.md`: deep / 15 行引用
- `故障诊断/FTA故障树/05-fta-construction-process.md`: deep / FTA 方法论

**问题与机会**
- **体量失衡**：362 页占全库 15.5%，其中 QA 语料和技能体系包含大量辅助文件
- 2 个页面缺少完整 frontmatter
- 建议将 QA语料/脚本 归入数据资产目录

### 11. 生产运维

| 指标 | 值 |
|---|---|
| 页面数 | 92 |
| 二级子目录 | 成本治理, 工单案例, 回复话术, 集群治理, 绿色计算, 事件响应 (6) |
| Tier 分布 | core: 21, supporting: 38, peripheral: 30 |
| Difficulty 分布 | beginner: 6, intermediate: 42, advanced: 45, expert: 0 |
| Frontmatter 完整度 | 92/92 = 100% |
| 字数估算 | ~3.6 万行 |

**覆盖缺口**
- 变更管理自动化
- SLA 管理实践
- 运维知识库构建

**深度抽检**
- `生产运维/事件响应/23-incident-response-handling.md`: deep / 27 行事件引用
- `生产运维/成本治理/03-spot-instance-strategy.md`: deep
- `生产运维/回复话术/01-acknowledgment.md`: medium / 话术模板型

**问题与机会**
- 回复话术子目录内容独特，属运维软技能

### 12. 云厂商

| 指标 | 值 |
|---|---|
| 页面数 | 72 |
| 二级子目录 | AWS-EKS, Azure-AKS, Google-GKE, 阿里云, 多云混合, 华为云CCE, 其他云, 腾讯云TKE (8) |
| Tier 分布 | core: 27, supporting: 47, peripheral: 11 |
| Difficulty 分布 | beginner: 0, intermediate: 26, advanced: 66, expert: 0 |
| Frontmatter 完整度 | 71/72 = 99% |
| 字数估算 | ~5.0 万行 |

**覆盖缺口**
- Oracle OKE 深度（仅概述）
- DigitalOcean DOKS
- 各云成本对比
- 云厂商 SLA 对比

**深度抽检**
- `云厂商/AWS-EKS/aws-eks-overview.md`: deep
- `云厂商/阿里云/ack/99-alicloud-ack-production-runbook.md`: deep / 生产手册
- `云厂商/多云混合/08-multicloud-federation-karmada.md`: deep

**问题与机会**
- 1 页缺少 summary 字段
- 各云厂商结构一致（overview + 生产手册 + 故障手册），利于维护

### 13. 容器运行时

| 指标 | 值 |
|---|---|
| 页面数 | 36 |
| 二级子目录 | containerd-CRI-O, Docker, 镜像构建, 镜像管理, 运行时迁移 (5) |
| Tier 分布 | core: 6, supporting: 18, peripheral: 19 |
| Difficulty 分布 | beginner: 0, intermediate: 24, advanced: 12, expert: 0 |
| Frontmatter 完整度 | 35/36 = 97% |
| 字数估算 | ~2.4 万行 |

**覆盖缺口**
- Wasm 运行时（已移至专项技术/WebAssembly）
- 运行时安全加固
- gVisor/Kata 生产对比
- 镜像签名与验证

**深度抽检**
- `容器运行时/containerd-CRI-O/01-containerd-production-operations.md`: deep
- `容器运行时/镜像管理/01-harbor-enterprise-image-registry.md`: deep
- `容器运行时/Docker/01-docker-architecture-overview.md`: medium

**问题与机会**
- 体量最小域之一，1 页缺 frontmatter
- 文件名含空格："06-runtime-security-hardening 2.md"

### 14. AI基础设施

| 指标 | 值 |
|---|---|
| 页面数 | 144 |
| 二级子目录 | Agent运行时, AI-Agents, AI编码, 基础设施 (4) |
| Tier 分布 | core: 6, supporting: 88, peripheral: 58 |
| Difficulty 分布 | beginner: 0, intermediate: 86, advanced: 121, expert: 0 |
| Frontmatter 完整度 | 144/144 = 100% |
| 字数估算 | ~11.7 万行 |

**覆盖缺口**
- GPU 调度与资源管理
- 分布式训练框架对比
- MLOps 流水线
- 模型服务网格
- AI 成本优化

**深度抽检**
- `AI基础设施/AI-Agents/01-ai-agent-fundamentals.md`: deep / 38 代码块
- `AI基础设施/Agent运行时/01-langchain-langgraph-deep-dive.md`: deep
- `AI基础设施/AI编码/01-openrouter-overview-architecture.md`: medium

**问题与机会**
- AI-Agents 子目录 45+ 页，内容增长迅速
- 部分页面 frontmatter 出现重复 title/description 块（格式异常）

### 15. 专项技术

| 指标 | 值 |
|---|---|
| 页面数 | 48 |
| 二级子目录 | WebAssembly, 边缘计算, 扩展机制, 无服务器 (4) |
| Tier 分布 | core: 4, supporting: 11, peripheral: 39 |
| Difficulty 分布 | beginner: 0, intermediate: 10, advanced: 46, expert: 0 |
| Frontmatter 完整度 | 48/48 = 100% |
| 字数估算 | ~7.3 万行 |

**覆盖缺口**
- Service Mesh（与网络域重叠）
- Virtual Kubelet
- 混合云扩展技术

**深度抽检**
- `专项技术/扩展机制/02-operator-development-patterns.md`: deep
- `专项技术/WebAssembly/01-wasm-fundamentals-cloud-native.md`: deep
- `专项技术/边缘计算/01-edge-computing-architecture.md`: deep

**问题与机会**
- peripheral 占 81%，核心内容少；扩展机制子目录是最大价值区

### 16. 数据库中间件

| 指标 | 值 |
|---|---|
| 页面数 | 40 |
| 二级子目录 | Operator管理, 缓存, 时序数据库, 数据库, 数据流, 消息队列 (6) |
| Tier 分布 | core: 8, supporting: 33, peripheral: 7 |
| Difficulty 分布 | beginner: 0, intermediate: 10, advanced: 30, expert: 0 |
| Frontmatter 完整度 | 40/40 = 100% |
| 字数估算 | ~2.1 万行 |

**覆盖缺口**
- 数据库连接池管理
- 分库分表策略
- 时序数据库与监控集成
- CDC 最佳实践

**深度抽检**
- `数据库中间件/数据库/01-mysql-enterprise-database.md`: deep
- `数据库中间件/消息队列/01-nats-deep-dive.md`: deep
- `数据库中间件/数据流/04-debezium-cdc-kubernetes.md`: deep

**问题与机会**
- 结构清晰，6 子目录覆盖全面
- 体量偏小，建议补充连接池和分库分表

### 17. 系统基础

| 指标 | 值 |
|---|---|
| 页面数 | 608 |
| 二级子目录 | K8s事件, Linux, 速查卡, 硬件, 知识字典 (5) |
| Tier 分布 | core: 334, supporting: 185, peripheral: 114 |
| Difficulty 分布 | beginner: 552, intermediate: 44, advanced: 20, expert: 0 |
| Frontmatter 完整度 | 608/608 = 100% |
| 字数估算 | ~17.9 万行 |

**覆盖缺口**
- Windows 容器基础
- ARM 架构支持
- 内核参数调优参考

**深度抽检**
- `系统基础/知识字典/fundamentals/kubernetes.md`: medium / 词条型
- `系统基础/硬件/01-cloud-hardware-architecture.md`: medium
- `系统基础/Linux/06-linux-performance-tuning.md`: deep

**问题与机会**
- **体量膨胀**：608 页占全库 26%，知识字典/fundamentals 含 50+ 短词条
- beginner 占 91%（552/608），全库 beginner 的主要来源
- 建议知识字典词条合并或标记为参考数据

### 18. 清单模式

| 指标 | 值 |
|---|---|
| 页面数 | 43 |
| 二级子目录 | Helm值模式, Kustomize模式, YAML参考 (3) |
| Tier 分布 | core: 2, supporting: 10, peripheral: 36 |
| Difficulty 分布 | beginner: 0, intermediate: 37, advanced: 7, expert: 0 |
| Frontmatter 完整度 | 43/43 = 100% |
| 字数估算 | ~7.8 万行 |

**覆盖缺口**
- Jsonnet 模式
- Cue 语言配置
- Crossplane 组合模式

**深度抽检**
- `清单模式/YAML参考/07-job-cronjob-reference.md`: deep / 90 行引用
- `清单模式/Kustomize模式/03-kustomize-remote-build-gitops.md`: deep / 25 行
- `清单模式/Helm值模式/01-helm-values-best-practices.md`: deep

**问题与机会**
- peripheral 占 84%，YAML参考以模板为主
- 结构简洁但内容完整

### 19. 生态参考

| 指标 | 值 |
|---|---|
| 页面数 | 54 |
| 二级子目录 | CNCF全景, 领域索引, 论文 (3) |
| Tier 分布 | core: 27, supporting: 14, peripheral: 23 |
| Difficulty 分布 | beginner: 25, intermediate: 11, advanced: 3, expert: 26 |
| Frontmatter 完整度 | 54/54 = 100% |
| 字数估算 | ~4.2 万行 |

**覆盖缺口**
- 开源许可证对比
- 社区参与度评估
- 技术雷达方法论

**深度抽检**
- `生态参考/论文/01-kubernetes-production-readiness-assessment.md`: deep
- `生态参考/CNCF全景/01-cncf-integration-guide.md`: deep
- `生态参考/领域索引/cluster-index.md`: medium / 索引型

**问题与机会**
- 难度分布独特：expert 占 48%（论文类），beginner 占 46%（索引类）
- CNCF全景的 graduated/incubating/sandbox 子目录为空（仅 index.md）

### 20. 应用模式

| 指标 | 值 |
|---|---|
| 页面数 | 111 |
| 二级子目录 | 生产模式, 行业架构, 子模式 (3) |
| Tier 分布 | core: 22, supporting: 95, peripheral: 0 |
| Difficulty 分布 | beginner: 1, intermediate: 97, advanced: 65, expert: 30 |
| Frontmatter 完整度 | 111/111 = 100% |
| 字数估算 | ~6.2 万行 |

**覆盖缺口**
- 微服务拆分模式
- 事件溯源/CQRS 模式
- 多租户 SaaS 模式

**深度抽检**
- `应用模式/生产模式/cost-optimization-finops.md`: deep / 18 代码块 / FinOps 四杠杆
- `应用模式/行业架构/01-ecommerce-architecture.md`: medium / 行业参考
- `应用模式/生产模式/progressive-delivery-patterns.md`: deep

**问题与机会**
- 行业架构 90+ 页覆盖广泛行业
- expert 占 27%，在应用模式中合理
- 无 peripheral 页面，全部为 supporting+

---

## 跨域重复与合并机会

| 主题 | 涉及域 | 建议 |
|---|---|---|
| SLO/SLI/错误预算 | 可观测性(SLO-SLI), 可靠性(SRE实践), 生产运维(成本治理), 系统基础(知识字典) | **需合并**：可观测性/SLO-SLI 为主，可靠性引用即可；系统基础知识字典有 253 行 SLO 词条，应精简 |
| 事件响应/事故处理 | 生产运维(事件响应), 可靠性(事后复盘), 安全(运行时安全), 故障诊断 | **需协调**：生产运维聚焦流程，可靠性聚焦复盘，安全聚焦威胁——当前分工合理但应增加交叉链接 |
| eBPF/Cilium | 网络(eBPF), 专项技术(扩展机制), 故障诊断(工具), 安全(运行时安全) | **需合并**：网络/eBPF 为主体（14 页深度内容），其他域应链接而非重复 |
| 备份/灾难恢复 | 可靠性(备份恢复/灾难恢复), 存储(K8s存储), 故障诊断(高级排障) | **需协调**：可靠性域为主，存储域聚焦 CSI 快照，故障诊断聚焦排障 |
| GitOps/CI/CD | 发布变更(GitOps), 专项技术(扩展机制), 故障诊断(高级排障) | **需合并**：发布变更/GitOps 为主（10+ 页），专项技术有重复的 GitOps 工作流 |
| 性能调优 | 集群基础(性能调优), 平台工程(治理), 网络(性能调优), 应用模式(生产模式) | **需协调**：各领域聚焦自身层面，但应增加统一链接页 |
| 安全合规 | 安全(合规审计), 专项技术(扩展机制), 平台工程(治理) | **需合并**：安全域为主体，其他域引用 |
| 成本优化/FinOps | 生产运维(成本治理), 平台工程(治理), AI基础设施(基础设施), 应用模式(生产模式) | **需合并**：生产运维和应用模式/生产模式各有 FinOps 内容，建议统一到应用模式 |

---

## 下一步行动（Top 10 优先级）

1. **精简系统基础/知识字典**：将 50+ 短词条合并为分类索引页，降低页面膨胀
2. **故障诊断域 QA 语料归档**：将 QA语料/脚本 移入数据资产目录，减少内容页面数
3. **统一 SLO/SLI 内容**：以可观测性/SLO-SLI 为主，消除系统基础知识字典中的 253 行重复
4. **增加 beginner 内容**：全库 beginner 仅 2.5%（含系统基础后），构建新手学习路径
5. **修复 frontmatter 异常**：容器运行时 1 页、云厂商 1 页、故障诊断 2 页缺完整 frontmatter
6. **消除文件名异常**：`容器运行时/containerd-CRI-O/06-runtime-security-hardening 2.md` 含空格
7. **平衡工作负载域**：增加 Go/Python/Node.js on K8s，目前仅 Java
8. **合并 FinOps 内容**：生产运维和应用模式/生产模式的成本优化内容去重
9. **充实存储域**：仅 39 页，补充云存储对比和性能基准
10. **生态参考/CNCF 子目录**：graduated/incubating/sandbox 目录仅有空 index.md，需填充或移除

---

## 附录：量化指标原始数据

```json
{
  "audit_date": "2026-07-11",
  "total_pages": 2332,
  "total_fm_complete": 2329,
  "total_fm_total": 2332,
  "fm_completion_rate": 0.999,
  "stale_pages": 0,
  "domains": [
    {"name": "集群基础", "pages": 97, "subdirs": 7, "tier": {"core": 8, "supporting": 21, "peripheral": 78}, "difficulty": {"beginner": 1, "intermediate": 22, "advanced": 95, "expert": 0}, "fm": "97/97", "lines": 102343},
    {"name": "工作负载", "pages": 42, "subdirs": 2, "tier": {"core": 12, "supporting": 18, "peripheral": 17}, "difficulty": {"beginner": 0, "intermediate": 29, "advanced": 17, "expert": 0}, "fm": "42/42", "lines": 36126},
    {"name": "网络", "pages": 113, "subdirs": 7, "tier": {"core": 14, "supporting": 23, "peripheral": 86}, "difficulty": {"beginner": 0, "intermediate": 26, "advanced": 88, "expert": 10}, "fm": "113/113", "lines": 115270},
    {"name": "存储", "pages": 39, "subdirs": 4, "tier": {"core": 5, "supporting": 23, "peripheral": 15}, "difficulty": {"beginner": 0, "intermediate": 15, "advanced": 29, "expert": 0}, "fm": "39/39", "lines": 34101},
    {"name": "安全", "pages": 58, "subdirs": 6, "tier": {"core": 5, "supporting": 61, "peripheral": 7}, "difficulty": {"beginner": 0, "intermediate": 15, "advanced": 54, "expert": 0}, "fm": "58/58", "lines": 62343},
    {"name": "可观测性", "pages": 69, "subdirs": 7, "tier": {"core": 10, "supporting": 68, "peripheral": 2}, "difficulty": {"beginner": 0, "intermediate": 55, "advanced": 15, "expert": 0}, "fm": "69/69", "lines": 61839},
    {"name": "平台工程", "pages": 204, "subdirs": 5, "tier": {"core": 15, "supporting": 114, "peripheral": 105}, "difficulty": {"beginner": 7, "intermediate": 188, "advanced": 126, "expert": 14}, "fm": "204/204", "lines": 138322},
    {"name": "发布变更", "pages": 50, "subdirs": 6, "tier": {"core": 21, "supporting": 20, "peripheral": 19}, "difficulty": {"beginner": 0, "intermediate": 25, "advanced": 41, "expert": 0}, "fm": "50/50", "lines": 47008},
    {"name": "可靠性", "pages": 50, "subdirs": 7, "tier": {"core": 7, "supporting": 26, "peripheral": 15}, "difficulty": {"beginner": 0, "intermediate": 21, "advanced": 34, "expert": 0}, "fm": "50/50", "lines": 32020},
    {"name": "故障诊断", "pages": 362, "subdirs": 11, "tier": {"core": 222, "supporting": 228, "peripheral": 49}, "difficulty": {"beginner": 3, "intermediate": 84, "advanced": 227, "expert": 12}, "fm": "361/363", "lines": 382857},
    {"name": "生产运维", "pages": 92, "subdirs": 6, "tier": {"core": 21, "supporting": 38, "peripheral": 30}, "difficulty": {"beginner": 6, "intermediate": 42, "advanced": 45, "expert": 0}, "fm": "92/92", "lines": 35677},
    {"name": "云厂商", "pages": 72, "subdirs": 8, "tier": {"core": 27, "supporting": 47, "peripheral": 11}, "difficulty": {"beginner": 0, "intermediate": 26, "advanced": 66, "expert": 0}, "fm": "71/72", "lines": 49927},
    {"name": "容器运行时", "pages": 36, "subdirs": 5, "tier": {"core": 6, "supporting": 18, "peripheral": 19}, "difficulty": {"beginner": 0, "intermediate": 24, "advanced": 12, "expert": 0}, "fm": "35/36", "lines": 24296},
    {"name": "AI基础设施", "pages": 144, "subdirs": 4, "tier": {"core": 6, "supporting": 88, "peripheral": 58}, "difficulty": {"beginner": 0, "intermediate": 86, "advanced": 121, "expert": 0}, "fm": "144/144", "lines": 117158},
    {"name": "专项技术", "pages": 48, "subdirs": 4, "tier": {"core": 4, "supporting": 11, "peripheral": 39}, "difficulty": {"beginner": 0, "intermediate": 10, "advanced": 46, "expert": 0}, "fm": "48/48", "lines": 72756},
    {"name": "数据库中间件", "pages": 40, "subdirs": 6, "tier": {"core": 8, "supporting": 33, "peripheral": 7}, "difficulty": {"beginner": 0, "intermediate": 10, "advanced": 30, "expert": 0}, "fm": "40/40", "lines": 21320},
    {"name": "系统基础", "pages": 608, "subdirs": 5, "tier": {"core": 334, "supporting": 185, "peripheral": 114}, "difficulty": {"beginner": 552, "intermediate": 44, "advanced": 20, "expert": 0}, "fm": "608/608", "lines": 178553},
    {"name": "清单模式", "pages": 43, "subdirs": 3, "tier": {"core": 2, "supporting": 10, "peripheral": 36}, "difficulty": {"beginner": 0, "intermediate": 37, "advanced": 7, "expert": 0}, "fm": "43/43", "lines": 77644},
    {"name": "生态参考", "pages": 54, "subdirs": 3, "tier": {"core": 27, "supporting": 14, "peripheral": 23}, "difficulty": {"beginner": 25, "intermediate": 11, "advanced": 3, "expert": 26}, "fm": "54/54", "lines": 42061},
    {"name": "应用模式", "pages": 111, "subdirs": 3, "tier": {"core": 22, "supporting": 95, "peripheral": 0}, "difficulty": {"beginner": 1, "intermediate": 97, "advanced": 65, "expert": 30}, "fm": "111/111", "lines": 61887}
  ]
}
```
