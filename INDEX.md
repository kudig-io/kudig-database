# KUDIG-DATABASE 知识库全局索引

> 按逻辑分组的完整知识体系导航 | 950+ 文档 | 41 知识域

---

## 核心知识域 (Core Kubernetes)

基础架构、设计原理到故障排查的完整 Kubernetes 技术栈。

| # | 知识域 | 文档数 | 说明 |
|:---:|:---|:---:|:---|
| 1 | [架构基础](./domain-1-architecture-fundamentals/) | 18 | K8s 架构、核心组件、升级策略、性能调优、安全架构 |
| 2 | [设计原理](./domain-2-design-principles/) | 18 | 声明式 API、控制器模式、etcd 共识、Operator 开发 |
| 3 | [控制平面](./domain-3-control-plane/) | 30 | etcd、API Server、Scheduler、KCM、CRI/CSI/CNI |
| 4 | [工作负载](./domain-4-workloads/) | 25 | Pod 生命周期、调度器、HPA/VPA、资源管理 |
| 5 | [网络](./domain-5-networking/) | 41 | CNI、Service、DNS、Ingress、Gateway API |
| 6 | [存储](./domain-6-storage/) | 17 | PV/PVC、StorageClass、CSI 驱动、备份恢复 |
| 7 | [安全合规](./domain-7-security/) | 21 | RBAC、网络安全、运行时安全、审计合规 |
| 8 | [可观测性](./domain-8-observability/) | 30 | 监控指标、日志审计、链路追踪、混沌工程 |
| 9 | [平台运维](./domain-9-platform-ops/) | 25 | 集群管理、GitOps、成本优化、灾备恢复 |
| 10 | [扩展生态](./domain-10-extensions/) | 16 | CRD/Operator、Helm、CI/CD、服务网格 |
| 11 | [AI 基础设施](./domain-11-ai-infra/) | 36 | GPU 调度、分布式训练、LLM 推理、成本优化 |
| 12 | [故障排查](./domain-12-troubleshooting/) | 42+ | 全组件故障排查、结构化排障 |

---

## 底层基础 (Infrastructure)

Docker、Linux、网络存储底层原理和硬件知识。

| # | 知识域 | 文档数 | 说明 |
|:---:|:---|:---:|:---|
| 13 | [Docker](./domain-13-docker/) | 12 | 架构、镜像、容器、网络、存储、安全、排障 |
| 14 | [Linux](./domain-14-linux/) | 10 | 系统架构、进程、文件系统、网络、安全、容器基础 |
| 15 | [网络基础](./domain-15-network-fundamentals/) | 6 | OSI/TCP-IP、DNS、负载均衡、SDN |
| 16 | [存储基础](./domain-16-storage-fundamentals/) | 6 | 存储架构、RAID、分布式系统 |
| 17 | [云厂商](./domain-17-cloud-provider/) | 13家 | 阿里云 ACK、AWS EKS、GCP GKE、Azure AKS 等 |
| 31 | [硬件](./domain-31-hardware/) | 18 | CPU、内存、存储、网络硬件、故障排查 |

---

## 企业级专题 (Enterprise)

面向生产环境的运维实践、监控日志、安全合规等。

| # | 知识域 | 文档数 | 说明 |
|:---:|:---|:---:|:---|
| 18 | [生产运维](./domain-18-production-operations/) | 24 | 架构设计、零信任、GitOps、FinOps、灾备 |
| 19 | [技术白皮书](./domain-19-papers/) | 26 | 深度技术专题、最佳实践白皮书 |
| 20 | [企业监控告警](./domain-20-enterprise-monitoring-alerting/) | 10 | Prometheus、Grafana、Datadog、Elastic |
| 21 | [日志管理](./domain-21-logging-management-analytics/) | 9 | ELK、Fluentd、Loki、Graylog |
| 22 | [容器镜像管理](./domain-22-container-image-management/) | 7 | Harbor、JFrog、Quay |
| 23 | [GitOps CI/CD](./domain-23-gitops-ci-cd/) | 4 | Argo CD、Jenkins、GitHub Actions |
| 24 | [基础设施即代码](./domain-24-infrastructure-as-code/) | 5 | Terraform、Ansible、Pulumi |
| 25 | [云原生安全](./domain-25-cloud-native-security/) | 5 | Falco、Sysdig、Kyverno、Vault |
| 26 | [服务网格](./domain-26-service-mesh-microservices/) | 6 | Istio、Linkerd、Envoy、Dapr |
| 27 | [多云混合](./domain-27-multi-cloud-hybrid/) | 5 | AWS/Azure/GCP/IBM 多云治理 |
| 28 | [数据库中间件](./domain-28-enterprise-database-middleware/) | 5 | MySQL、PostgreSQL、Redis、MongoDB |
| 29 | [自动化测试](./domain-29-automated-testing-quality/) | 5 | Selenium、Playwright、AI 测试 |
| 30 | [灾备恢复](./domain-30-disaster-recovery-business-continuity/) | 5 | VMware、Veeam、混沌工程 |

---

## 前沿技术 (Advanced)

CNCF 生态、eBPF、平台工程、边缘计算等前沿领域。

| # | 知识域 | 文档数 | 说明 |
|:---:|:---|:---:|:---|
| 32 | [YAML 配置清单](./domain-32-yaml-manifests/) | 36 | K8s 全资源 YAML 参考手册 |
| 33 | [Kubernetes Events](./domain-33-kubernetes-events/) | 15 | 事件体系完整解析 |
| 34 | [CNCF Landscape](./domain-34-cncf-landscape/) | 218 | Graduated/Incubating/Sandbox 全量项目 |
| 35 | [eBPF 技术](./domain-35-ebpf-technology/) | 10 | eBPF 原理、Cilium、可观测性 |
| 36 | [平台工程](./domain-36-platform-engineering/) | 11 | IDP 内部开发者平台 |
| 37 | [边缘计算](./domain-37-edge-computing/) | 10 | KubeEdge、边缘部署 |
| 38 | [WebAssembly](./domain-38-webassembly-cloud-native/) | 10 | Wasm 云原生工作负载 |
| 39 | [供应链安全](./domain-39-supply-chain-security/) | 10 | SBOM、SLSA、Sigstore |
| 40 | [云原生 API 网关](./domain-40-cloud-native-api-gateway/) | 14 | Gateway API、Higress、APISIX |

---

## 方法论与实践 (Methodology)

FTA 故障树、FEBM 取证、Skills 技能库等独创方法论。

| 专题 | 文档数 | 说明 |
|:---|:---:|:---|
| [FTA 故障树分析](./topic-fta/) | 29+36 | 方法论 23 篇 + 36 个组件故障树 |
| [FEBM 取证循证](./topic-febm/) | 9 | 从证据到结论的归纳式方法论 |
| [Skills 运维技能库](./topic-skills/) | 18 | 生产级诊断-修复闭环 |
| [结构化故障排查](./topic-structural-trouble-shooting/) | 49 | 12 个分类 × 结构化流程 + 配置优先方法论 |
| [配置优先排查方法论](./topic-structural-trouble-shooting/00-configuration-first-methodology.md) | 1 | 疑难问题系统性排查：先配置后链路，CoreDNS 完整示例 |
| [运维词典](./topic-dictionary/) | 200+ | 13 个分类的运维知识条目 |

---

## 学习与参考 (Reference)

速查卡、学习计划、部署方案等参考资料。

| 专题 | 文档数 | 说明 |
|:---|:---:|:---|
| [速查卡](./topic-cheat-sheet/) | 9 | K8s/Linux/Docker/PromQL/Git/SQL 等 |
| [学习计划](./topic-learn/) | 46 | 1 个月系统化学习路径 |
| [部署方案](./topic-deployment/) | 4 | 从 Demo 到生产的渐进式部署 |
| [集群迁移](./topic-migration/) | 10 | 10 步完整迁移指南 |
| [Release Notes](./topic-release-notes/) | 1300+ | K8s 及生态组件版本说明 |
| [Manpage](./man/) | 14 | Unix manpage 参考手册 |

---

## AI 工程 (AI Engineering)

AI Agent、AI Coding 工具相关知识体系。

| 专题 | 文档数 | 说明 |
|:---|:---:|:---|
| [AI Agent 工程](./topic-ai-agent/) | 50 | Agent 基础→Harness 工程→OpenClaw |
| [AI Coding 工具](./topic-ai-coding/) | 24 | OpenRouter、OpenCode |

---

## 工具与发布 (Tooling)

项目工具、脚本、GitBook 和发布计划。

| 目录 | 说明 |
|:---|:---|
| [scripts/](./scripts/) | 统计、质量检查、FTA 可视化等脚本 |
| [gitbook/](./gitbook/) | mdBook 本地文档浏览系统 |
| [reports/](./reports/) | 质量报告、统计数据 |
| [templates/](./templates/) | 文档模板（Domain/FTA/Skill/速查卡） |
| [topic-publish/](./topic-publish/) | 内容发布计划和路线图 |
