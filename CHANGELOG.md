# 变更历史

### 2026-03-03 Domain-34 CNCF Landscape 全量项目库上线
**CNCF 云原生全景图 218 个开源项目完整收录**:
- ✅ 新增 `domain-34-cncf-landscape/` 目录，包含 219 篇文档（1 README + 218 项目）
- ✅ **Graduated 项目（34 个）**：全部深度增强，平均 11KB/篇
  - Kubernetes、Prometheus、Envoy、Helm、Istio、etcd、containerd、Argo
  - Cilium、Harbor、Falco、Flux、SPIFFE、SPIRE、Knative、Dapr
  - CRI-O、Dragonfly、KubeEdge、TUF、Fluentd、Rook、Linkerd、OPA
  - CloudEvents、Crossplane、KEDA、cert-manager、TiKV、CubeFS、Vitess、in-toto
  - CoreDNS、Jaeger
- ✅ **Incubating 项目（37 个）**：核心项目深度增强（OpenTelemetry、gRPC）
  - Thanos、NATS、Backstage、Kyverno、Longhorn、Chaos Mesh、Contour
  - KubeVirt、Kubeflow、Volcano、Artifact Hub、Buildpacks、CNI、Cortex 等
- ✅ **Sandbox 项目（147 个）**：统一模板格式，基础信息完整
- ✅ 每篇 Graduated 文档包含：架构图、核心概念、安装部署、使用示例、生态集成、参考资源
- ✅ 按成熟度分目录：graduated/、incubating/、sandbox/
- ✅ 每个项目独立文件夹，统一命名规范
- ✅ 更新项目 README.md：文档总数 668 → 886，领域数量 33 → 34

### 2026-03-03 Topic-Deployment Kubernetes 部署方案指南全新上线
**从零到生产的完整部署路径**:
- ✅ 新增 `topic-deployment/` 目录，包含 4 篇渐进式部署方案 + README
- ✅ 01-本地 Demo 部署（kind/minikube）：零成本快速体验，30-60 分钟完成
- ✅ 02-单节点部署（k3s/kubeadm）：真实 Linux 环境，All-in-One 方案
- ✅ 03-研发环境部署：多节点集群 + 监控 + CI/CD + 权限管控 + 日志收集
- ✅ 04-生产环境部署：高可用 + 安全合规 + 灾备恢复 + 成本优化 + 升级策略
- ✅ 提供决策树、工具选型矩阵、硬件需求对照表
- ✅ 每篇包含完整命令、配置文件、预期输出和故障排查指南
- ✅ 新增 `count-stats.sh` 项目统计脚本

### 2026-03-03 Topic-Learn Kubernetes 1 个月学习计划上线
**系统化学习路径与实践项目**:
- ✅ 新增 `topic-learn/` 目录，包含 46 篇学习指南文档
- ✅ Week 1 地基建设期（7 天）：Docker 基础 → Linux 基础 → K8s 架构全貌 → kubectl 实战
- ✅ Week 2 核心技术构建期（7 天）：控制平面精读 → 工作负载深潜 → 网络栈精通 → 存储体系
- ✅ Week 3 运维作战能力期（7 天）：安全合规 → 可观测性构建 → 故障排查方法论 → 平台运维
- ✅ Week 4 企业级进阶期（7 天）：企业监控/日志 → GitOps → FTA/FEBM 专题 → 生产最佳实践
- ✅ 5 个渐进式实践项目：集群搭建 → 应用编排 → 可观测性 + 故障演练 → GitOps 流水线 → 毕业综合项目
- ✅ 补充资源：命令速查表、知识地图、阅读顺序指南、学习海报
- ✅ 每周含检查点（checkpoint），每日含理论阅读 + 实践任务 + 费曼复述

### 2026-03-03 Topic-FTA Node 故障树深度扩展
**Node 故障树分析全面增强**:
- ✅ `node-fta.md` 深度扩展至 105KB，新增 1000+ 行专家级内容
- ✅ 覆盖节点 NotReady、资源压力、kubelet 异常、容器运行时故障等全场景

### 2026-03-02 Topic-FTA 全量故障树分析手册 v2.0 重大更新
**36 个 Kubernetes 组件故障树全面深化**:
- ✅ 全量 36 个 FTA 文档从框架级扩展为生产级深度内容
- ✅ Pod FTA 深度重构至 199KB（1795+ 行新增），覆盖全生命周期故障场景
- ✅ 每个 FTA 新增：根因分析链、诊断命令集、修复方案、预防措施、Prometheus 告警规则
- ✅ 重点增强：证书 FTA（52KB）、集群自动伸缩 FTA（49KB）、集群升级 FTA（52KB）、Webhook/准入控制 FTA（50KB）、PSP/SCC FTA（44KB）
- ✅ 新增 `topic-fta/list/README.md` 故障树索引与导航
- ✅ 总计新增 21,500+ 行故障排查内容

### 2026-03-01 Topic-FTA 故障树分析框架扩展
**Kubernetes 全组件 FTA 框架建立**:
- ✅ 新增 32 个组件级 FTA 文档框架
- ✅ 涵盖：API Server、etcd、Controller Manager、Scheduler、DNS、Ingress、Gateway API、Service Mesh/Istio
- ✅ 涵盖：RBAC、NetworkPolicy、HPA/VPA、PDB、PSP/SCC、Resource Quota
- ✅ 涵盖：Deployment、StatefulSet、DaemonSet、Job/CronJob、Helm、GitOps/ArgoCD
- ✅ 涵盖：GPU、Cluster Autoscaler、Cloud Provider、备份恢复、集群升级、证书管理、Webhook/准入控制、监控告警
- ✅ 新增 `fta_methodology_and_agentic_practices.md` FTA 方法论与 AI Agent 实践文档

### 2026-02-28 Topic-FTA 故障树分析体系创建
**FTA (Fault Tree Analysis) 故障树分析专题上线**:
- ✅ 新增 `topic-fta/` 目录，包含完整 FTA 方法论体系
- ✅ 5 个核心 FTA 文档：Pod FTA（597 行）、Node FTA（163 行）、NodePool FTA（131 行）、CSI FTA（118 行）、Terway FTA（118 行）
- ✅ FTA 方法论理论基础（23 篇）：起源与发展、数学基础、符号体系、核心原则、构建流程、验证与质量、维护与演进
- ✅ AI Agent 智能运维实践：Agent 编排模式、FTA 驱动 Runbook 自动化、AIOps 集成、智能工单处理
- ✅ 附录：术语表、工具与资源、参考文献、模板
- ✅ `kubernetes_fta_full_analysis.md` Kubernetes 全量故障树分析（126KB）

### 2026-02-26 GitBook 离线静态导出与 FEBM/FTA 更新
**跨平台文档浏览系统完善**:
- ✅ 完成 GitBook/mdBook 在 macOS 上的离线静态导出
- ✅ 修复 mdBook 构建脚本兼容性问题
- ✅ 优化 `export-static.sh`、`refresh.sh`、`start.sh`、`generate-summary.sh` 脚本
- ✅ 更新 `gitbook/src/SUMMARY.md` 目录结构
- ✅ 添加 topic-febm 和 topic-fta 符号链接到 GitBook
- ✅ 更新 FEBM（故障事件行为模型）和 FTA 相关内容

### 2026-02-13 修复与维护
- ✅ 项目文件修复与维护性更新

### 2026-02-12 GitBook 文档浏览系统上线
**基于 mdBook 的本地文档浏览系统**:
- ✅ 新增 `gitbook/` 目录，基于 mdBook 构建本地可浏览文档站
- ✅ 提供 macOS 和 Windows 跨平台构建脚本
- ✅ 支持本地服务（`start.sh`）、重新构建（`refresh.sh`）、静态导出（`export-static.sh`）
- ✅ 自动生成 `SUMMARY.md` 目录结构
- ✅ 支持全文搜索、离线浏览

### 2026-02-10 Domain-32 YAML 配置清单手册全新上线
**Kubernetes YAML 全资源配置完整参考手册**:
- ✅ 新增 36 篇 YAML 配置参考文档，覆盖 60+ Kubernetes 原生 API 资源类型
- ✅ 覆盖 Kubernetes v1.25-v1.32 全版本，含版本兼容矩阵和 Feature Gates 时间线
- ✅ 每篇包含完整字段规范、最小化示例、生产级示例、源码级内部机制解析
- ✅ 涵盖工作负载（Pod/Deployment/StatefulSet/DaemonSet/Job/CronJob）
- ✅ 涵盖网络（Service 5 种类型/Ingress/Gateway API 核心 + 高级路由）
- ✅ 涵盖存储（PV/PVC/StorageClass/VolumeSnapshot/CSI 驱动资源）
- ✅ 涵盖安全（RBAC/NetworkPolicy/Pod Security Standards/Admission Webhook/ValidatingAdmissionPolicy + CEL）
- ✅ 涵盖调度（PriorityClass/RuntimeClass/HPA v2/PDB）
- ✅ 涵盖扩展（CRD/APIService/API Priority & Fairness）
- ✅ 涵盖集群基础设施（Lease/Event/Node/kubeadm/组件配置 Kubelet + KubeProxy + Scheduler）
- ✅ 涵盖生态工具（Kustomize/Helm/ArgoCD 完整配置参考）
- ✅ 全部中文散文讲解，YAML 使用英文配详细中文注释
- ✅ 更新文档总数：622 → 658 篇，领域数量：32 → 33

### 2026-02-09 Gemini 全域文档质量增强
**Domain 1-3 文档专业深度提升**:
- ✅ Domain-1 架构基础：增强性能调优指南（62 行）、安全架构（43 行）、故障排查指南（79 行）
- ✅ Domain-2 设计原则：18 篇设计原则文档全面增强前言与专业概述
- ✅ Domain-3 控制平面：安全加固（93 行重构）、认证授权深度解析（80 行）、API 扩展（21 行）
- ✅ 新增文档统计分析资产：知识分布分析 Excel、分类饼图、领域饼图
- ✅ 统一文档质量标准，提升专业术语一致性

### 2026-02-08 Domain-31 硬件知识库与企业级测试/灾备增强
**硬件基础知识域全新上线**:
- ✅ 新增 `domain-31-hardware/` 目录，包含 18 篇硬件技术文档
- ✅ 涵盖：云计算硬件架构、服务器架构原理、CPU 技术深度解析、主板芯片组技术
- ✅ 涵盖：内存技术深度解析、HDD 存储技术、SSD 存储技术、网络硬件技术
- ✅ 涵盖：硬件厂商生态、故障排查方法论、CPU/内存/存储/网络/电源散热/BIOS 固件故障排查
- ✅ 涵盖：Kubernetes 硬件故障排查、硬件错误代码参考、硬件故障案例研究
- ✅ Domain-29 自动化测试：新增 Cypress（1406 行）、Playwright（1348 行）企业级自动化
- ✅ Domain-30 灾备恢复：新增 Commvault（1690 行）、Rubrik（348 行）企业级灾备

### 2026-02-08 大规模企业级文档体系扩展
**Domain 18-30 企业级知识域全面上线**:
- ✅ 新增 Docker/Linux 命令完整参考：`99-docker-commands-reference.md`（955 行）、`99-linux-commands-reference.md`（1826 行）
- ✅ **Domain-18 生产运维实践**（24 篇）：生产架构设计原则、多云混合部署、边缘计算部署、企业监控体系、日志分析平台、APM 应用性能监控、零信任安全架构、CIS 基准合规审计、SBOM、GitOps 流水线、IaC、自动化运维工具链、成本治理 FinOps、资源配额管理、绿色计算、备份策略、灾备演练、跨地域灾备、集群性能调优、网络/存储性能优化、变更管理、事件响应、容量规划
- ✅ **Domain-19 技术白皮书**（16 篇）：生产就绪评估、大规模性能优化、零信任安全、多云混合部署、GitOps 实践、FinOps、CSI 存储、网络策略微隔离、Service Mesh/Istio、自动化 SRE、API Server 优化、调度器定制、多租户隔离、事件驱动架构、混沌工程、边缘计算 KubeEdge
- ✅ **Domain-20 企业监控告警**（10 篇）：Prometheus、Grafana、OpenTelemetry、Thanos、Datadog、Elastic Stack、Zabbix、New Relic
- ✅ **Domain-21 日志管理分析**（9 篇）：ELK Stack、Fluentd、Loki、Graylog、Splunk、Loggly、企业日志治理合规、实时分析
- ✅ **Domain-22 容器镜像管理**（7 篇）：Harbor、Docker Registry、JFrog Artifactory、Quay、GitLab Container Registry、Amazon ECR
- ✅ **Domain-23 GitOps CI/CD**（4 篇）：Argo CD、Jenkins、GitLab CI/CD、GitHub Actions
- ✅ **Domain-24 基础设施即代码**（5 篇）：Terraform、Ansible、Pulumi、Azure ARM、Crossplane
- ✅ **Domain-25 云原生安全**（5 篇）：Falco、Sysdig、Aqua Security、Kyverno、Vault
- ✅ **Domain-26 服务网格微服务**（6 篇）：Istio、Linkerd、Consul Connect、Envoy Proxy、Dapr、Traefik Mesh
- ✅ **Domain-27 多云混合**（5 篇）：AWS EKS、Azure AKS、GCP GKE、IBM Cloud、企业多云治理
- ✅ **Domain-28 企业级数据库中间件**（5 篇）：MySQL、PostgreSQL、分布式数据库、MongoDB、Redis
- ✅ **Domain-29 自动化测试质量**（3 篇）：Selenium、JUnit5、AI 测试质量保障
- ✅ **Domain-30 灾备恢复业务连续性**（3 篇）：VMware vSphere、Veeam、混沌工程

### 2026-02-07 README 文档全面更新与统计工具
**根目录 README 与文件结构同步**:
- ✅ 新增 `scripts/count.py` 文档字数统计脚本
- ✅ 更新文档总数统计：606 篇
- ✅ 更新知识体系架构图：添加 topic-structural-trouble-shooting（40 篇）
- ✅ 添加域 18 生产运维实践章节：24 篇完整文档索引（8 个子章节）
- ✅ 添加域 19 技术白皮书章节：16 篇深度技术专题
- ✅ 添加 topic-structural-trouble-shooting 章节：40 篇结构化故障排查文档
- ✅ 修复目录缩进问题，确保层级结构正确
- ✅ 补充 topic-presentations 演示模板文档
- ✅ 优化专题资源章节的完整性

### 2026-02-06 Domain-18 生产运维实践与 Topic-Dictionary 全面扩展
**生产运维实践知识域与运维专家字典重大增强**:
- ✅ Domain-18 生产运维实践完整创建（24 篇），覆盖生产架构设计到容量规划全流程
- ✅ Domain-17 云厂商文档目录重组：标准化为 01-13 数字编号
- ✅ 新增联通云 UK8S（957 行）、移动云 CKE 深度扩展（658 行）
- ✅ Topic-Dictionary 新增 5 篇专家级运维文档：事件管理 Runbook、容量规划预测、变更管理发布、SLI/SLO/SLA 工程、生产故障排查 Playbook
- ✅ Topic-Structural-Trouble-Shooting 新增 4 篇结构化故障排查：控制平面安全/性能/高可用/升级故障排查
- ✅ 新增 AI/ML 工作负载故障排查、GitOps/DevOps 故障排查、监控可观测性故障排查
- ✅ Domain-1 新增生产运维最佳实践（973 行）、升级迁移策略重命名
- ✅ README 大幅更新（425 行变更）

### 2026-02-05 重大更新 v2.1.0 - Domain-4 工作负载管理全面增强与质量提升
**工作负载管理全面增强与全局质量提升**:
- ✅ 新增 06-工作负载监控告警体系（459 行专家级内容）
- ✅ 新增 07-故障排查应急手册（477 行生产级指南）
- ✅ 新增 08-多云混合部署策略（693 行企业级方案）
- ✅ 新增 09-边缘计算部署模式（742 行前沿技术）
- ✅ 完善 02-Deployment 生产实践案例，新增三大行业场景
- ✅ 重新整理文件编号为 01-23 连续序列
- ✅ 修复 README 中所有失效链接（约 50+ 处）
- ✅ 完善变更记录和版本信息，增强术语一致性和专业深度
- ✅ 新增代码示例质量检查脚本，优化现有质量检查工具

### 2026-02-05 项目级文档体系查漏补缺完成
**Domain 1-9 README 标准化**:
- ✅ 补齐核心 Domain README：为 Domain-1 至 Domain-9 创建完整的 README.md 文件
- ✅ 统一文档结构：所有 Domain 目录均具备标准化的目录结构和内容概述
- ✅ 完善学习路径：为每个 Domain 提供清晰的学习建议和路径规划
- ✅ 增强交叉引用：建立 Domain 间的关联关系，形成完整知识体系
- ✅ 质量标准化：确保所有文档遵循统一的质量标准和格式规范

### 2026-02-05 Topic-Dictionary 运维知识中枢全面升级
**从 7 个核心文档扩展到 16 个专业词典文件**:
- ✅ 新增 AI 基础设施专家指南（08）- AI/ML 平台运维专精
- ✅ 新增云原生安全专家指南（09）- 安全防护与合规实践
- ✅ 新增多云混合云运维手册（10）- 跨云部署与成本优化
- ✅ 新增企业级运维最佳实践（11）- 万级节点运维体系
- ✅ 每个新增文档均超过 1000 行专业内容
- ✅ 统一采用 01-11 递增编号体系
- ✅ 专家级内容深度（≥4.8/5 分），生产环境实用性（≥4.9/5 分）

### 2026-02-05 Domain-17 云厂商知识库全面查漏补缺
**14 个云厂商 Kubernetes 服务文档高质量完善**:
- ✅ 完成所有 14 个云厂商 Kubernetes 服务文档的高质量内容完善
- ✅ 新增阿里云专有版 ACK overview 文档，填补内容空白
- ✅ 优化 `domain-17-cloud-provider/README.md` 目录结构和链接引用
- ✅ 完善云厂商服务对比表格，增加特色优势维度
- ✅ 补充所有云厂商的特色功能展示和学习路径

### 2026-02-05 Domain-17 云厂商知识库重点加强
**腾讯云 TKE、华为云 CCE、火山引擎 VEK 三大云厂商深度增强**:
- ✅ 腾讯云 TKE：新增 Gaia 网络优化、大规模集群调优、AI 平台集成等高级内容（1212 行 → 1784 行）
- ✅ 华为云 CCE：全面重构为信创专题，新增鲲鹏 ARM 优化、昇腾 AI 芯片支持、国密安全等特色内容（417 行 → 487 行）
- ✅ 火山引擎 VEK：深度扩展字节级优化、AI/ML 原生支持、大规模调度等核心优势（468 行 → 701 行）
- ✅ 所有文档均达到生产级专家水平，包含详细配置示例和最佳实践

### 2026-02-05 Domain-17 云厂商知识库生产级重构
**13 个云厂商 Kubernetes 服务文档生产级丰富**:
- ✅ 完成所有 13 个云厂商 Kubernetes 服务文档的生产级内容丰富
- ✅ 从运维专家角度提供详细的架构设计、安全加固、监控告警配置
- ✅ 针对不同云厂商特色提供定制化最佳实践方案
- ✅ 统一文档结构，确保从 01 开始递增编号
- ✅ 涵盖阿里云 ACK、AWS EKS、GCP GKE、Azure AKS、腾讯云 TKE、华为云 CCE、天翼云 TKE、移动云 CKE、IBM IKS、Oracle OKE、联通云 UK8S、火山引擎 VEK 等主流云厂商

### 2026-02-05 Domain-12 文档质量优化
**38 篇故障排查文档质量提升**:
- ✅ 完成 38 篇故障排查文档的内容质量检查
- ✅ 统一文档标题层级结构（数字层级标准化）
- ✅ 优化关键文档的目录结构和内容组织
- ✅ 提升文档的生产环境适用性和专家级质量
- ✅ 建立完整的质量检查和优化流程

### 2026-02 Topic-Dictionary 运维知识中枢专家级内容深化
**生产环境运维专家级知识库全面丰富**:
- ✅ 为 16 个核心文档添加大量生产环境实战经验和专家级最佳实践
- ✅ 01-运维最佳实践：新增生产环境故障应急响应机制、真实故障案例和处理流程
- ✅ 02-故障模式分析：补充经典故障案例集锦、故障处理经验总结和预防性运维建议
- ✅ 03-性能调优专家：增加大规模集群性能优化案例、性能监控最佳实践和优化检查清单
- ✅ 04-SRE 成熟度模型：新增企业级 SRE 转型路线图、团队建设最佳实践和 SLO 管理实战指南
- ✅ 05-概念参考手册：扩展前沿技术概念，新增 WebAssembly、eBPF、GitOps 等新兴技术详解
- ✅ 06-命令行清单：丰富运维效率提升命令集，添加批量操作、高级调试和自动化脚本
- ✅ 07-工具生态系统：补充前沿技术创新工具和边缘计算 5G 工具，扩展工具覆盖面
- ✅ 保持文件编号 01-11 连续性，结构清晰易维护
- ✅ 更新根目录 README，详细反映 topic-dictionary 内容增强和专家级特色

### 2026-02 Topic-Dictionary 运维知识中枢全面查漏补缺
**高质量专家级内容体系完善**:
- ✅ 深度审计完成：全面审查 16 个核心文档，识别并填补所有内容缺口
- ✅ 高级故障诊断：新增分布式系统故障定位方法论、智能化故障预测与自愈技术
- ✅ 性能调优强化：补充内核级调优参数、容器运行时优化、微服务性能模式
- ✅ 安全防护升级：完善零信任架构实施、高级威胁检测、安全工具链集成
- ✅ 多云管理深化：扩展混合云架构模式、跨云成本优化、统一治理框架
- ✅ AI 运维增强：丰富 GPU 调度策略、模型生命周期管理、AI 成本治理实践
- ✅ 企业级实践：补充万级节点运维经验、大规模集群管理、组织效能提升
- ✅ 质量一致性保证：统一所有文档格式标准，确保专家级质量（≥4.9/5 分）
- ✅ 前沿技术覆盖：全面涵盖 WebAssembly、eBPF、GitOps、Service Mesh 等新兴技术

### 2026-02 Topic-Dictionary 运维知识中枢全面升级
**生产环境运维专家级知识库重构完成**:
- ✅ 新增 4 个专业运维文档：运维最佳实践（01）、故障模式分析（02）、性能调优专家（03）、SRE 成熟度模型（04）
- ✅ 现有文档重新编号：概念参考手册（05）、命令行清单（06）、工具生态系统（07）
- ✅ 所有文档按 01-07 连续编号，确保结构清晰和易维护性
- ✅ 丰富运维最佳实践内容：生产环境配置标准、高可用架构、安全加固、监控告警、灾备恢复
- ✅ 完善故障分析体系：故障模式分类、根因分析方法论、MTTR 优化策略、预防措施体系
- ✅ 强化性能调优能力：瓶颈识别、资源优化、调度器调优、网络存储优化等专家级指导
- ✅ 建立 SRE 成熟度模型：评估标准、自动化分级、监控体系建设、团队能力建设路径
- ✅ 更新根目录 README，添加详细的 topic-dictionary 介绍和导航

### 2026-02 Domain-17 云厂商 Kubernetes 服务全面升级
**云厂商 Kubernetes 服务文档体系重构完成**:
- ✅ 重新组织 `domain-17-cloud-provider` 目录结构，采用数字编号（01-13）标准化命名
- ✅ 丰富核心云厂商文档内容，增加生产环境运维专家级详细配置
- ✅ 补充阿里云 ACK、AWS EKS、GCP GKE、Azure AKS、腾讯云 TKE、天翼云 TKE、IBM IKS 等主要厂商的深度技术文档
- ✅ 完善安全加固、监控告警、成本优化、故障排查等生产实践内容
- ✅ 更新 README 中 Domain-17 章节结构，重新分类国际云厂商和国内云厂商
- ✅ 整合 ACK 关联产品文档（240-245）到新的目录结构中

### 2026-02 Kubernetes 扩展生态体系完善
**Domain-10 扩展生态文档体系重构完成**:
- ✅ 补充完整的扩展开发生态文档（01-04）：CRD 开发指南、Operator 开发模式、准入控制器配置、API 聚合扩展
- ✅ 重构扩展生态文档结构：运维基础技能（05）+ CI/CD 与 GitOps（06-07）+ 包管理与构建（08-11）+ 服务网格（12-13）+ 扩展开发（01-04）
- ✅ 重新编号所有扩展生态文档：124-130 → 01-13
- ✅ 更新 README 中 Domain-10 扩展生态章节结构和链接

### 2026-02 平台运维体系完善
**Domain-9 平台运维文档体系重构完成**:
- ✅ 新增核心运维体系文档（01-08）：运维概览、集群管理、监控告警、GitOps、自动化工具链、成本优化、安全合规、灾备连续性
- ✅ 重构平台运维文档结构：运维基础体系（01-08）+ 控制平面扩展（09-15）+ 备份容灾（16-18）+ 多集群管理（19-21）
- ✅ 重新编号所有平台运维文档：111-123 → 01-21
- ✅ 更新 README 中 Domain-9 平台运维章节结构和链接

### 2026-02 安全合规体系增强
**Domain-7 安全文档体系重构完成**:
- ✅ 新增核心安全体系文档（01-04）：认证授权、网络安全、运行时安全、审计合规
- ✅ 重构安全文档结构：核心安全体系（01-04）+ 安全实践工具（05-16）
- ✅ 重新编号所有安全文档：81-92 → 01-16
- ✅ 更新 README 中 Domain-7 安全合规章节结构和链接

### 2026-02 目录结构优化
**项目结构重组完成**:
- ✅ 创建 `domain-17-cloud-provider` 统一管理所有云厂商文档
- ✅ 将所有 `cloud-*` 目录移动到 `domain-17-cloud-provider/` 下
- ✅ 重命名 `presentations` → `topic-presentations`
- ✅ 重命名 `trouble-shooting` → `topic-trouble-shooting`
- ✅ 更新 README 中所有相关链接，验证所有链接有效性

### 2026-02 域名数字化改造
**域名命名标准化完成**:
- ✅ 将所有域名从字母格式（`domain-a-`）转换为数字格式（`domain-1-`）
- ✅ 更新 README 中所有 219 个文件链接指向正确的数字域名目录
- ✅ 验证所有链接有效性，确保文档可正常访问
- ✅ 更新域统计信息和表格数量统计

### 2026-02 扩展生态文档体系优化
**Domain-10 扩展生态文档体系重构完成**:
- ✅ 重新排序所有扩展生态文档，按开发流程逻辑顺序排列：扩展开发 → 包管理 → CI/CD → 服务网格 → 运维基础
- ✅ 重新编号所有扩展生态文档：01-13，保持连续性
- ✅ 更新 README 中 Domain-10 扩展生态章节结构和链接

### 2026-02 AI 基础设施文档体系优化
**Domain-11 AI 基础设施文档体系重构完成**:
- ✅ 重新排序所有 AI/LLM 文档，按知识体系逻辑顺序排列：AI 基础 → 模型训练 → LLM 专题 → 运维监控 → 成本优化
- ✅ 重新编号所有 AI/LLM 文档：01-30，保持连续性
- ✅ 更新 README 中 Domain-11 AI 基础设施章节结构和链接

### 2026-02 故障排查文档体系优化
**Domain-12 故障排查文档体系完善完成**:
- ✅ 修正所有故障排查文档的标题编号，使其与文件名保持一致
- ✅ 验证所有 38 篇故障排查文档的完整性和一致性
- ✅ 更新 README 中 Domain-12 故障排查章节结构和链接

### 2026-02 根目录结构优化
**项目结构重组完成**:
- ✅ 根目录精简至仅保留 README.md
- ✅ `validate-links.ps1` 脚本移至 `topic-dictionary/` 目录
- ✅ 完善的分类目录结构：topic-dictionary、presentations、updates 等
- ✅ 提升项目专业性和维护便利性

### 2026-01 增强更新
**底层基础知识域与核心组件深度解析系列**:
- ✅ 域 13 Docker 基础（8 篇）：架构概述、镜像管理、容器生命周期、网络详解、存储卷、Compose 编排、安全最佳实践、故障排查
- ✅ 域 14 Linux 基础（8 篇）：系统架构、进程管理、文件系统、网络配置、存储管理、性能调优、安全加固、容器技术（Namespaces/Cgroups）
- ✅ 域 15 网络基础（6 篇）：协议栈（OSI/TCP-IP）、TCP/UDP 详解、DNS 原理配置、负载均衡技术、网络安全、SDN 与网络虚拟化
- ✅ 域 16 存储基础（6 篇）：存储架构、类型详解、RAID 配置、分布式系统、性能调优和企业级运维实践
- ✅ 阿里云 ACK 关联产品增强（240-245）：ECS 计算资源、SLB/NLB/ALB 负载均衡、VPC 网络规划、RAM 权限与 RRSA、ROS 资源编排、EBS 云盘存储
- ✅ 专有云 Apsara Stack 专题（250-252）：ESS 弹性伸缩、SLS 日志服务、POP 平台运维（ASOP）
- ✅ etcd 深度解析：Raft 共识、MVCC 存储、集群配置、备份恢复、监控调优
- ✅ kube-apiserver 深度解析：认证授权、准入控制、APF 限流、审计日志、高可用
- ✅ kube-controller-manager 深度解析：40+ 控制器详解、Leader 选举、监控指标
- ✅ cloud-controller-manager 深度解析（v2.0 全面重构）：12 章节资深专家级内容，架构演进、核心控制器工作流、Cloud Provider Interface、阿里云 CCM 生产级配置（CLB/NLB/ALB 注解速查 60+ 条）、AWS/Azure/GCP CCM 完整配置、RBAC 权限矩阵、15+ 关键指标与 Prometheus 告警规则
- ✅ kubelet 深度解析：Pod 生命周期、PLEG、健康探测、cgroup 管理、CRI 接口
- ✅ kube-proxy 深度解析：iptables/IPVS/nftables 模式、负载均衡、性能调优
- ✅ kube-scheduler 深度解析：调度框架、插件系统、评分策略、抢占机制、高级调度
- ✅ CRI 容器运行时深度解析：Docker 演进、containerd/CRI-O 架构、runc/crun/youki、gVisor/Kata 安全容器
- ✅ CSI 容器存储深度解析：CSI 规范、Sidecar 组件、AWS EBS/阿里云/Ceph 驱动、快照/克隆/扩展
- ✅ CNI 容器网络深度解析：CNI 规范、Calico BGP/eBPF、Cilium eBPF、NetworkPolicy 实现
- ✅ LLM 系列增强：数据管道、LoRA/QLoRA 微调、vLLM/TGI 推理部署、GPTQ/AWQ 量化、Milvus/Qdrant 向量数据库与 RAG
- ✅ 工具类增强：Vault/ESO 密钥管理、Trivy/Falco 安全扫描、kubectl debug 故障排查、pprof 性能分析、Helm/Kustomize 包管理、Buildah/Kaniko 镜像构建
- ✅ kubectl 命令完整参考（v3.0）：14 章节资深专家级内容，架构原理、版本兼容性矩阵、资源查看/创建/编辑、Pod 调试、部署管理、集群管理、插件扩展、生产运维脚本
- ✅ Service 完整深度解析（v3.0）：12 章节，架构图、字段参考表、多云 LB 配置、kube-proxy 三模式详解、EndpointSlice、DNS 集成、拓扑感知路由
- ✅ Ingress 完整深度解析（v3.0）：12 章节，API 结构详解、IngressClass 多控制器、TLS/cert-manager、金丝雀发布、Gateway API 迁移路径
- ✅ 中等文件增强（5-10KB → 40-60KB）：Sidecar 容器模式、Egress 流量管理、证书管理、LLM 隐私安全、LLM 成本监控、Kubecost 成本管理
- ✅ 故障排查全攻略（v3.0）：15 章节，四步方法论、Pod/Node/Service/存储/控制平面/调度器故障深度排查、v1.25-v1.32 已知问题矩阵、生产级综合诊断脚本
- ✅ 集群配置参数完全参考（v3.0 全面重构）：10 章节，kube-apiserver/etcd/kube-scheduler/kube-controller-manager/kubelet/kube-proxy 完整参数、Feature Gates 版本演进表、多云厂商特定配置
