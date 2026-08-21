---
title: KUDIG 全局标签字典
description: '| `networking` | 网络技术 | domain-5, domain-15, 相关 |'
summary: '| `networking` | 网络技术 | domain-5, domain-15, 相关 |'
category: general
tags:
- k8s
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
- istio
- envoy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG 全局标签字典 是什么
- 如何 KUDIG 全局标签字典
trigger_keywords:
- KUDIG
- 全局标签字典
prerequisites:
- kubectl-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: KUDIG 全局标签字典
description: KUDIG 全局标签字典
category: docs
tags:
- k8s
- dictionary
- taxonomy
- metadata
relationships:
- target: "[[29-文档/specs/FRONTMATTER-SPEC.md|KUDIG Frontmatter 规范]]"
  type: related_to
- target: "[[29-文档/specs/SCENARIO-TAXONOMY.md|KUDIG 场景分类体系]]"
  type: related_to
- target: "[[29-文档/specs/SYNONYM-DICTIONARY.md|KUDIG 同义词与别名词典]]"
  type: related_to
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- DevOps
estimated_read_time: 10min
last_updated: 2026-05
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'

tier: peripheral
---
# KUDIG 全局标签字典

> 创建时间: 2026-05-20
> 用途: 统一 3,337+ 文档的标签系统，确保 Agent/RAG 检索一致性
> 规则: 所有标签使用小写英文 + 数字，使用连字符连接多词标签

---

## 一级标签（领域分类）

| 标签 | 说明 | 适用文档类型 |
|---|---|---|
| `k8s` | Kubernetes 核心知识 | 所有 domain 文档 |
| `docker` | Docker 容器技术 | domain-13, 相关 |
| `linux` | Linux 系统 | domain-14, 相关 |
| `networking` | 网络技术 | domain-5, domain-15, 相关 |
| `storage` | 存储技术 | domain-6, domain-16, 相关 |
| `security` | 安全技术 | domain-7, domain-25, domain-39, 相关 |
| `observability` | 可观测性 | domain-8, domain-20, domain-21, 相关 |
| `ai` | AI/ML 基础设施 | domain-11, topic-ai-*, 相关 |
| `devops` | DevOps 实践 | domain-9, domain-23, 相关 |
| `platform` | 平台工程 | domain-36, 相关 |
| `mesh` | Service Mesh | domain-26, 相关 |
| `gitops` | GitOps 方法论 | domain-23, 相关 |
| `iac` | 基础设施即代码 | domain-24, 相关 |
| `cncf` | CNCF 生态 | domain-34, 相关 |
| `ebpf` | eBPF 技术 | domain-35, 相关 |
| `edge` | 边缘计算 | domain-37, 相关 |
| `wasm` | WebAssembly | domain-38, 相关 |
| `gateway` | API 网关 | domain-40, 相关 |
| `database` | 数据库中间件 | domain-28, 相关 |
| `cloud` | 多云/混合云 | domain-17, domain-27, 相关 |
| `hardware` | 硬件相关 | domain-31, 相关 |
| `paper` | 学术论文 | domain-19, 相关 |
| `yaml` | YAML 清单 | domain-32, 相关 |
| `events` | Kubernetes 事件 | domain-33, 相关 |
| `quality` | 测试与质量 | domain-29, 相关 |
| `disaster-recovery` | 灾备与容灾 | domain-30, 相关 |
| `cheatsheet` | 速查参考 | topic-cheat-sheet, 相关 |
| `fta` | 问题树分析 | topic-fta, 相关 |
| `skill` | 操作技能 | topic-skills, 相关 |
| `troubleshooting` | 问题排查 | domain-12, topic-structural-trouble-shooting, 相关 |
| `learning` | 学习路径 | topic-learn, 相关 |
| `dictionary` | 运维术语 | topic-dictionary, 相关 |
| `release-notes` | 版本发布说明 | topic-release-notes, 相关 |
| `migration` | 迁移指南 | topic-migration, 相关 |
| `architecture` | 应用架构 | topic-application-architecture, 相关 |
| `deployment` | 部署策略 | topic-deployment, 相关 |
| `java` | Java 生态 | domain-java-kubernetes, 相关 |
| `terway` | Terway CNI | domain-5-networking, 相关 |
| `febm` | FEBM 取证 | topic-febm, 相关 |
| `ai-agent` | AI 智能体 | 02-ai-agents, 相关 |
| `ai-coding` | AI 编程 | topic-ai-coding, 相关 |
| `qa` | QA 评测语料 | topic-qa-corpus, 相关 |
| `moc` | MOC 导航页 | 所有 MOC.md 文件 |

---

## 二级标签（组件/技术）

| 标签 | 说明 | 关联一级标签 |
|---|---|---|
| `architecture` | 架构设计 | k8s |
| `control-plane` | 控制平面 | k8s |
| `etcd` | etcd 分布式存储 | control-plane |
| `apiserver` | API Server | control-plane |
| `scheduler` | 调度器 | control-plane |
| `controller-manager` | 控制器管理器 | control-plane |
| `workload` | 工作负载 | k8s |
| `pod` | Pod | workload |
| `deployment` | Deployment | workload |
| `statefulset` | StatefulSet | workload |
| `daemonset` | DaemonSet | workload |
| `job` | Job/CronJob | workload |
| `service` | Service 网络 | networking |
| `ingress` | Ingress | networking |
| `cni` | CNI 插件 | networking |
| `network-policy` | 网络策略 | networking, security |
| `dns` | DNS 解析 | networking |
| `pv` | PersistentVolume | storage |
| `pvc` | PersistentVolumeClaim | storage |
| `storage-class` | StorageClass | storage |
| `csi` | Container Storage Interface | storage |
| `rbac` | 基于角色的访问控制 | security |
| `network-policy` | 网络策略 | security |
| `pod-security` | Pod 安全策略 | security |
| `secret` | Secret 管理 | security |
| `certificate` | 证书管理 | security |
| `prometheus` | Prometheus 监控 | observability |
| `grafana` | Grafana 可视化 | observability |
| `alertmanager` | Alertmanager 告警 | observability |
| `logging` | 日志管理 | observability |
| `tracing` | 分布式追踪 | observability |
| `crd` | Custom Resource Definition | k8s |
| `operator` | Operator 模式 | k8s |
| `webhook` | Admission Webhook | k8s |
| `gpu` | GPU 调度 | ai |
| `cuda` | CUDA 计算 | ai |
| `model-serving` | 模型服务 | ai |
| `istio` | Istio Mesh | mesh |
| `envoy` | Envoy 代理 | mesh |
| `argo` | ArgoCD | gitops |
| `flux` | Flux CD | gitops |
| `helm` | Helm 包管理 | k8s |
| `cilium` | Cilium CNI | ebpf |
| `terway` | Terway CNI | networking |
| `kubelet` | Kubelet | control-plane |
| `kube-proxy` | Kube-Proxy | networking |
| `coredns` | CoreDNS | networking |
| `hpa` | Horizontal Pod Autoscaler | workload |
| `vpa` | Vertical Pod Autoscaler | workload |
| `keda` | KEDA 事件驱动伸缩 | workload |

---

## 三级标签（场景/用途）

| 标签 | 说明 |
|---|---|
| `troubleshooting` | 问题排查 |
| `best-practice` | 最佳实践 |
| `performance` | 性能调优 |
| `configuration` | 配置参考 |
| `deployment` | 部署指南 |
| `monitoring` | 监控告警 |
| `security-hardening` | 安全加固 |
| `backup-restore` | 备份恢复 |
| `upgrade` | 升级迁移 |
| `production` | 生产环境 |
| `development` | 开发环境 |
| `testing` | 测试相关 |
| `quick-reference` | 快速参考 |
| `deep-dive` | 深度解析 |
| `tutorial` | 教程 |
| `guide` | 指南 |
| `reference` | 参考资料 |
| `case-study` | 案例分析 |
| `interview` | 面试准备 |
| `exam` | 认证考试 |
| `daily-ops` | 日常运维 |
| `emergency` | 紧急处理 |
| `capacity-planning` | 容量规划 |
| `cost-optimization` | 成本优化 |
| `compliance` | 合规审计 |

---

## 使用规范

### 标签组合规则

每篇文档的 `tags` 应包含：
1. **1-2 个一级标签** — 标识所属领域
2. **1-3 个二级标签** — 标识具体组件
3. **1-2 个三级标签** — 标识场景/用途

**示例**:
```yaml
tags: [k8s, etcd, control-plane, best-practice, production]
```

### 标签命名

- 全部小写
- 多词使用连字符: `network-policy`, `best-practice`
- 不使用下划线、空格、驼峰
- 不使用缩写（除非是广泛认可的缩写如 `rbac`, `cni`, `csi`, `gpu`）

---

*本文档是标签体系的权威定义，新增标签时应在此文件中注册。*

---

## Obsidian 相关文档

- [[29-文档/index.md|KUDIG-DATABASE 首页]]

---

## Related

- [[23-实体/15-参考与索引/KUDIG Frontmatter Spec.md|KUDIG Frontmatter Spec]]
- [[23-实体/15-参考与索引/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]]
- [[29-文档/specs/SYNONYM-DICTIONARY.md|KUDIG 同义词与别名词典]]
- [[29-文档/specs/FRONTMATTER-SPEC.md|KUDIG Frontmatter 规范]]


<!-- risk-assessed -->
