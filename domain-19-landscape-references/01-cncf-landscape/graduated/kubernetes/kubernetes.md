---
title: Kubernetes
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- etcd
- kubelet
- scheduler
- prometheus
- grafana
- istio
- envoy
- helm
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 是什么
- 如何 Kubernetes
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Kubernetes
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- gitops-basics
- ebpf-basics
- etcd-basics
- tls-basics
---

title: Kubernetes
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- etcd
- kubelet
- scheduler
- prometheus
- grafana
- istio
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kubernetes 是什么
- 如何 Kubernetes
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kubernetes
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Kubernetes

> **成熟度**: Graduated | **加入时间**: 2016-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kubernetes.io |
| **GitHub** | https://github.com/kubernetes/kubernetes |
| **文档** | https://kubernetes.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Orchestration & Management |

---

## 项目概述

### 简介
Kubernetes（K8s）是一个开源的容器编排平台，用于自动化容器化应用的部署、扩展和管理。

### 核心定位
Kubernetes 解决了大规模容器化应用的编排、调度、服务发现、负载均衡、存储编排、自动恢复等核心问题，是云原生生态系统的基石。

### 发展历程
- **2014-06**: Google 开源 Kubernetes 项目
- **2015-07**: Kubernetes v1.0 发布，CNCF 成立
- **2016-03**: 成为 CNCF 首个托管项目
- **2018-03**: 成为 CNCF 首个毕业项目
- **2024-04**: Kubernetes v1.30 发布

---

## 核心功能

### 主要特性
- **容器编排**: 自动化容器的部署、扩展和运维
- **服务发现**: 内置 DNS 和负载均衡
- **自动恢复**: 自动重启失败容器、替换节点
- **滚动更新**: 零停机部署和回滚
- **配置管理**: ConfigMap 和 Secret 管理
- **存储编排**: 自动挂载存储系统
- **批处理**: Job 和 CronJob 支持

### 功能架构
```
┌─────────────────────────────────────────────────────────────┐
│                      Control Plane                          │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐│
│  │  API Server  │ │   Scheduler  │ │ Controller Manager   ││
│  └──────────────┘ └──────────────┘ └──────────────────────┘│
│  ┌──────────────────────────────────────────────────────── ┐│
│  │                         etcd                            ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
                              │
┌─────────────────────────────────────────────────────────────┐
│                       Worker Nodes                          │
│  ┌──────────────┐ ┌──────────────┐ ┌──────────────────────┐│
│  │    kubelet   │ │  kube-proxy  │ │  Container Runtime   ││
│  └──────────────┘ └──────────────┘ └──────────────────────┘│
│  ┌──────────────────────────────────────────────────────── ┐│
│  │                        Pods                             ││
│  └─────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────┘
```

---

## 技术架构

### 整体架构
Kubernetes 采用主从架构，由控制平面（Control Plane）和工作节点（Worker Nodes）组成。控制平面负责集群的全局决策，工作节点运行实际的应用负载。

### 核心组件
| 组件 | 功能 | 说明 |
|:---|:---|:---|
| API Server | 集群入口 | 所有操作的统一入口，RESTful API |
| etcd | 状态存储 | 分布式键值存储，保存集群状态 |
| Scheduler | 调度器 | 为 Pod 选择合适的节点 |
| Controller Manager | 控制器 | 运行各种控制器，维护期望状态 |
| kubelet | 节点代理 | 管理节点上的容器生命周期 |
| kube-proxy | 网络代理 | 维护节点网络规则，实现 Service |

### 工作原理
1. 用户通过 kubectl 或 API 提交期望状态
2. API Server 将状态存储到 etcd
3. Controller 监听状态变化，执行调谐逻辑
4. Scheduler 为新 Pod 分配节点
5. kubelet 在节点上创建和管理容器
6. kube-proxy 配置网络规则实现服务访问

---

## 使用场景

### 典型应用
- **微服务架构**: 部署和管理微服务应用
- **CI/CD 平台**: 构建云原生持续交付流水线
- **大数据处理**: 运行 Spark、Flink 等数据处理任务
- **机器学习**: 部署 ML 训练和推理工作负载
- **边缘计算**: 管理边缘节点上的应用

### 适用条件
- 需要自动化容器编排和管理
- 需要高可用和自动恢复能力
- 需要灵活的扩展和升级策略
- 有专业的运维团队支持

### 不适用场景
- 简单的单机应用部署
- 资源极度受限的环境
- 不需要容器化的传统应用

---

## 快速开始

### 安装部署
```bash
# 使用 kind 创建本地集群
kind create cluster --name my-cluster

# 使用 minikube 创建本地集群
minikube start

# 使用 kubeadm 初始化生产集群
kubeadm init --pod-network-cidr=10.244.0.0/16
```

### 基础配置
```yaml
# deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: nginx-deployment
spec:
  replicas: 3
  selector:
    matchLabels:
      app: nginx
  template:
    metadata:
      labels:
        app: nginx
    spec:
      containers:
      - name: nginx
        image: nginx:1.25
        ports:
        - containerPort: 80
```

### 验证测试
```bash
# 查看集群状态
kubectl cluster-info
kubectl get nodes

# 部署应用
kubectl apply -f deployment.yaml

# 查看 Pod 状态
kubectl get pods -w
```

---

## 最佳实践

### 生产环境建议
- 使用高可用控制平面（3+ 节点）
- 配置资源限制（requests/limits）
- 启用 RBAC 和 Pod Security Standards
- 定期备份 etcd 数据
- 使用节点亲和性和反亲和性

### 性能优化
- 合理配置 API Server 限流参数
- 使用 Pod Disruption Budget
- 优化容器镜像大小
- 配置合适的探针参数

### 安全加固
- 启用审计日志
- 使用 NetworkPolicy 隔离网络
- 定期更新 Kubernetes 版本
- 加密 etcd 数据和 Secret

---

## 生态集成

### 相关 CNCF 项目
- **Helm**: Kubernetes 包管理器
- **Prometheus**: 监控和告警
- **Envoy/Istio**: 服务网格
- **Argo**: GitOps 和工作流
- **containerd**: 容器运行时

### 常见集成方案
- Prometheus + Grafana 监控栈
- Istio 服务网格
- ArgoCD GitOps 部署
- Cert-manager 证书管理

---

## 社区与支持

### 社区资源
- Slack: https://slack.k8s.io
- 论坛: https://discuss.kubernetes.io
- Stack Overflow: kubernetes 标签

### 贡献指南
访问 https://www.kubernetes.dev/docs/guide/ 了解如何参与贡献

---

## 参考资源

- [官方文档](https://kubernetes.io/docs)
- [GitHub Repo](https://github.com/kubernetes/kubernetes)
- [CNCF 项目页面](https://www.cncf.io/projects/kubernetes/)
- [Kubernetes Blog](https://kubernetes.io/blog/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[log.md|log]]
- [[CONTRIBUTING.md|CONTRIBUTING]]
- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[journal/digest-2026-05-21-full|Wiki 全量知识库摘要 — 2026-05-21]] — Cross-reference
- [[_reports/WIKI-LINT-REPORT-2026-05-21|Wiki Lint Report — 2026-05-21]] — Cross-reference
- [[references/KUDIG Cheat Sheet Index|KUDIG Cheat Sheet Index]] — Cross-reference
- [[references/specialized-workloads-terms|K8s 专用工作负载术语参考]] — Cross-reference
- [[references/linux-sysctl-reference|Linux Sysctl Reference for Kubernetes]] — Cross-reference
- [[references/networking-terms|K8s 网络术语参考]] — Cross-reference
- [[references/k8s-workloads-domain-guide|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[references/k8s-design-principles-deep-dive|设计原理：声明式 API、控制器模式与 etcd 共识]] — Cross-reference
- [[references/kubernetes-port-reference|Kubernetes Port Reference]] — Cross-reference
- [[references/workloads-terms|K8s 工作负载术语参考]] — Cross-reference
- [[references/k8s-glossary-index|K8s 术语表索引]] — Cross-reference
- [[references/fundamentals-terms|K8s 基础概念术语参考]] — Cross-reference
- [[references/release-notes-kubernetes|发布说明索引 — Kubernetes]] — Cross-reference
- [[references/k8s-architecture-fundamentals|K8s 架构基础与核心组件原理]] — Cross-reference
- [[references/root-terms|K8s Root术语参考]] — Cross-reference
- [[references/scheduling-terms|K8s 调度术语参考]] — Cross-reference
- [[references/kudig-contribution-guide|贡献指南、项目概览与版本发布说明]] — Cross-reference
- [[references/release-notes-reading-guide|发布说明阅读指南]] — Cross-reference
- [[references/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]] — Cross-reference
- [[references/storage-terms|K8s 存储术语参考]] — Cross-reference
- [[references/observability-terms|K8s 可观测性术语参考]] — Cross-reference
- [[references/kubectl Scenario Quick Reference|kubectl Scenario Quick Reference]] — Cross-reference
- [[references/kubectl-quick-reference|Kubectl Quick Reference]] — Cross-reference
- [[references/k8s-deployment-create|Kubernetes Deployment 创建操作指南]] — Cross-reference
- [[references/k8s-knowledge-map|Kubernetes Knowledge Map]] — Cross-reference
- [[references/k8s-cluster-delete|Kubernetes 集群删除操作指南]] — Cross-reference
- [[references/release-notes-cli-tools|发布说明索引 — CLI 工具]] — Cross-reference
- [[references/KUDIG Frontmatter Spec|KUDIG Frontmatter Specification]] — Cross-reference
- [[references/k8s-cluster-create|Kubernetes 集群创建操作指南]] — Cross-reference
- [[references/configuration-terms|K8s 配置管理术语参考]] — Cross-reference
- [[references/release-notes-core-deps|发布说明索引 — 核心依赖]] — Cross-reference
- [[references/k8s-ai-infra-domain-guide|AI Infrastructure on Kubernetes Domain Guide]] — Cross-reference
- [[references/tooling-terms|K8s 工具链术语参考]] — Cross-reference
- [[references/k8s-cluster-cert|Kubernetes 集群证书管理操作指南]] — Cross-reference
- [[references/k8s-node-create|Kubernetes 节点管理操作指南]] — Cross-reference
- [[references/platform-engineering-terms|K8s 平台工程术语参考]] — Cross-reference
- [[references/multi-cloud-terms|K8s 多云架构术语参考]] — Cross-reference
- [[references/kudig-man-pages-index|KUDIG Man Pages Index]] — Cross-reference
- [[references/version-upgrade-guide|版本升级指南]] — Cross-reference
- [[references/k8s-difficulty-index|Kubernetes Difficulty Index]] — Cross-reference
- [[references/k8s-networking-domain-guide|Kubernetes Networking Domain Guide]] — Cross-reference
- [[references/operations-terms|K8s 运维运营术语参考]] — Cross-reference
- [[references/kubernetes-api-versions-reference|Kubernetes API Versions Reference]] — Cross-reference
- [[synthesis/kubeadm-cluster-operations|kubeadm 集群运维全景]] — Cross-reference
- [[synthesis/etcd x 高可用模式|etcd × 高可用模式]] — Cross-reference
- [[synthesis/K8s 故障分布与 MTTR 基准|K8s 故障分布与 MTTR 基准]] — Cross-reference
- [[synthesis/Structural Troubleshooting Framework|Structural Troubleshooting Framework]] — Cross-reference
- [[synthesis/声明式 API × 控制器模式|声明式 API × 控制器模式]] — Cross-reference
- [[synthesis/eBPF x 运行时安全|eBPF x 运行时安全]] — Cross-reference
- [[concepts/deployment-controller-architecture|Deployment 控制器架构]] — Cross-reference
- [[concepts/bp-common-best-practices|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/kubernetes-pki-certificate-system|Kubernetes PKI 证书体系]] — Cross-reference
- [[concepts/bp-infrastructure|最佳实践：Infrastructure]] — Cross-reference
- [[concepts/bp-observability|最佳实践：Observability]] — Cross-reference
- [[concepts/bp-operations|最佳实践：Operations]] — Cross-reference
- [[concepts/declarative-api|Declarative API]] — Cross-reference
- [[concepts/core-dependency-version-matrix|核心依赖版本矩阵]] — Cross-reference
- [[concepts/kubernetes-version-evolution|Kubernetes 版本演进]] — Cross-reference
- [[concepts/multi-tenancy-isolation|Multi-Tenancy Isolation]] — Cross-reference
- [[concepts/cli-tools-evolution|CLI 工具演进]] — Cross-reference
- [[concepts/etcd Operational Reference|etcd Operational Reference]] — Cross-reference
- [[concepts/ai-agent-openclaw-workspace|OpenClaw 工作空间配置]] — Cross-reference
- [[concepts/ai-agent-README|AI Agent 工程专题]] — Cross-reference
- [[concepts/cni-networking-model|CNI 网络模型与插件对比]] — Cross-reference
- [[concepts/linux-sysctl-tuning|Linux Sysctl Tuning for Kubernetes]] — Cross-reference
- [[concepts/storage-tool-evolution|存储工具演进]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[concepts/bp-README|Kubernetes 最佳实践指南]] — Cross-reference
- [[concepts/eventual-consistency|Eventual Consistency in Kubernetes]] — Cross-reference
- [[concepts/k8s-production-best-practices|Kubernetes 生产环境最佳实践]] — Cross-reference
- [[concepts/node-lifecycle-management|节点生命周期管理]] — Cross-reference
- [[concepts/production-operations-best-practices|Production Operations Best Practices]] — Cross-reference
- [[concepts/bp-security|最佳实践：Security]] — Cross-reference
- [[concepts/observability-stack-evolution|可观测性栈演进]] — Cross-reference
- [[concepts/security-tool-evolution|安全工具演进]] — Cross-reference
- [[concepts/watch-mechanism|Watch Mechanism (List-Watch)]] — Cross-reference
- [[concepts/gitops-tool-evolution|GitOps 工具演进]] — Cross-reference
- [[concepts/linux-security-modules|Linux Security Modules for Containers]] — Cross-reference
- [[skills/learn-05-ingress-basics|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference
- [[skills/learn-01-day-one-checklist|Day 1: 新人首日检查清单]] — Cross-reference
- [[skills/k8s-storage-configuration-guide|Kubernetes 存储配置最佳实践]] — Cross-reference
- [[skills/k8s-scaling-guide|Kubernetes 扩缩容最佳实践]] — Cross-reference
- [[skills/k8s-disaster-recovery-guide|Kubernetes 灾难恢复最佳实践]] — Cross-reference
- [[skills/ts-ai-ml-workloads|AI/ML 工作负载排查]] — Cross-reference
- [[skills/dns-fta|DNS 异常故障树分析]] — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/node-fta|Node 异常故障树分析]] — Cross-reference
- [[skills/kubelet-certificate-rotation|kubelet 证书轮换机制]] — Cross-reference
- [[skills/learn-README|新人上手快速路径（Quick Start）]] — Cross-reference
- [[skills/assessment-k8s-fundamentals-quiz-answers|K8S Fundamentals Quiz Answers]] — Cross-reference
- [[skills/k8s-network-security-guide|Kubernetes 网络安全最佳实践]] — Cross-reference
- [[skills/ts-node-components|节点组件故障排查]] — Cross-reference
- [[skills/learn-13-daemonset-basics|第13课：DaemonSet 与节点守护]] — Cross-reference
- [[skills/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]] — Cross-reference
- [[skills/k8s-logging-management-guide|Kubernetes 日志管理最佳实践]] — Cross-reference
- [[skills/skill-20-networkpolicy-connectivity|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[skills/assessment-troubleshooting-lab-exam|Troubleshooting Lab Exam]] — Cross-reference
- [[skills/k8s-monitoring-guide|Kubernetes 监控最佳实践]] — Cross-reference
- [[skills/deployment-canary-and-bluegreen|金丝雀与蓝绿发布]] — Cross-reference
- [[skills/skill-k8s-node-notready-USAGE-GUIDE|Usage Guide]] — Cross-reference
- [[skills/learn-01-what-is-kubernetes|第一课：Kubernetes 入门]] — Cross-reference
- [[skills/ts-security-auth|安全认证故障排查]] — Cross-reference
- [[skills/skill-reference-version-matrix|Version Matrix]] — Cross-reference
- [[skills/develop-crd-operator|Develop CRD Operator]] — Cross-reference
- [[skills/skill-23-job-cronjob-failure|Job/CronJob 故障诊断与修复 / Job & CronJob Failure Diagnosis & Remediation]] — Cross-reference
- [[skills/node-drain-and-maintenance|节点驱逐与维护]] — Cross-reference
- [[skills/k8s-distributed-tracing-guide|Kubernetes 分布式追踪最佳实践]] — Cross-reference
- [[skills/skill-21-statefulset-failure|StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation]] — Cross-reference
- [[skills/Kubernetes Diagnostic Skills Overview|Kubernetes Diagnostic Skills Overview]] — Cross-reference
- [[skills/kubeadm-cluster-deletion|kubeadm 集群删除操作]] — Cross-reference
- [[skills/kubeadm-ha-cluster-setup|kubeadm 高可用集群搭建]] — Cross-reference
- [[skills/k8s-deployment-strategies-guide|Kubernetes 部署策略最佳实践]] — Cross-reference
- [[skills/k8s-cluster-configuration-guide|Kubernetes 集群配置最佳实践]] — Cross-reference
- [[skills/skill-reference-diagnostic-workflow|Diagnostic Workflow]] — Cross-reference
- [[skills/skill-22-daemonset-failure|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[skills/ts-control-plane|控制平面故障排查]] — Cross-reference
- [[skills/skill-reference-remediation-playbook|Remediation Playbook]] — Cross-reference
- [[skills/learn-lecturer-persona|K8S 讲师角色设定与场景规范]] — Cross-reference
- [[skills/learn-inner-training|Kubernetes 培训：Inner Training]] — Cross-reference
- [[skills/learn-15-scheduling-basics|第15课：调度与亲和性]] — Cross-reference
- [[skills/assessment-daily-check-quiz|Daily Check Quiz]] — Cross-reference
- [[skills/skill-reference-root-cause-catalog|Root Cause Catalog]] — Cross-reference
- [[skills/learn-root|Kubernetes 培训：Root]] — Cross-reference
- [[skills/skills-run-README|Skills Demo — 本地运行工单诊断技能]] — Cross-reference
- [[skills/deployment-workload-selection|工作负载控制器选型]] — Cross-reference
- [[skills/k8s-network-configuration-guide|Kubernetes 网络配置最佳实践]] — Cross-reference
- [[skills/monitor-kubernetes-metrics|Monitor Kubernetes Metrics]] — Cross-reference
- [[skills/learn-02-pod-basics|第二课：Pod - K8s 的最小调度单元]] — Cross-reference
- [[skills/learn-04-service-basics|第四课：Service - 让应用可以被访问]] — Cross-reference
- [[skills/learn-public-training|Kubernetes 培训：Public Training]] — Cross-reference
- [[skills/ts-gitops-devops|GitOps/DevOps 排查]] — Cross-reference
- [[skills/learn-04-debug-tools-setup|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[skills/learn-02-first-ticket-guide|Day 2: 第一个工单处理指南]] — Cross-reference
- [[skills/learn-oncall-quick-qa|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[skills/skill-MOC|topic-skills MOC]] — Cross-reference
- [[skills/skill-README|topic-skills — 工单智能体 Kubernetes 诊断 Skill 库]] — Cross-reference
- [[skills/learn-12-common-problems|第十课：常见问题排查]] — Cross-reference
- [[skills/skill-19-node-resource-pressure|节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation]] — Cross-reference
- [[skills/ts-storage|存储故障排查]] — Cross-reference
- [[skills/ts-cluster-operations|集群运维故障排查]] — Cross-reference
- [[skills/skill-assets-escalation-template|Escalation Template]] — Cross-reference
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[entities/argocd|ArgoCD]] — Cross-reference
- [[entities/kubernetes-changelog|Kubernetes 变更日志索引]] — Cross-reference
- [[entities/kube-apiserver|kube-apiserver]] — Cross-reference
- [[entities/inspektor-gadget|Inspektor Gadget]] — Cross-reference
- [[entities/metal3-io|Metal3]] — Cross-reference
- [[entities/core-deps-changelog|核心依赖变更日志索引]] — Cross-reference
- [[entities/cncf-edge-ai|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[entities/container-runtime|Container Runtime]] — Cross-reference
- [[entities/clusterpedia|Clusterpedia]] — Cross-reference
- [[entities/cncf-observability|CNCF 可观测性项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/openkruise-index|OpenKruise 全局索引]]
- [[domain-19-landscape-references/topic-index/helm-index|Helm 全局索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/topic-index/csi-index|CSI (Container Storage Interface) 知识图谱索引]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.12|Kubernetes v0.12 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.8|Kubernetes v0.8 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.16|Kubernetes v0.16 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.17|Kubernetes v0.17 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.13|Kubernetes v0.13 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.9|Kubernetes v0.9 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.18|Kubernetes v0.18 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.6|Kubernetes v0.6 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.7|Kubernetes v0.7 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.19|Kubernetes v0.19 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.4|Kubernetes v0.4 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-1.1|Kubernetes v1.1 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-1.0|Kubernetes v1.0 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.5|Kubernetes v0.5 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.20|Kubernetes v0.20 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.14|Kubernetes v0.14 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.10|Kubernetes v0.10 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.11|Kubernetes v0.11 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.21|Kubernetes v0.21 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/RELEASE-NOTES-0.15|Kubernetes v0.15 Release Notes]]
- [[domain-19-landscape-references/topic-release-notes/cli-tools/minikube/RELEASE-NOTES-1.38|RELEASE-NOTES-1.38]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.8|CHANGELOG-1.8]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.9|CHANGELOG-1.9]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.6|CHANGELOG-1.6]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.10|CHANGELOG-1.10]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.24|CHANGELOG-1.24]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.34|CHANGELOG-1.34]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.20|CHANGELOG-1.20]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.14|CHANGELOG-1.14]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.30|CHANGELOG-1.30]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.15|CHANGELOG-1.15]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.31|CHANGELOG-1.31]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.7|CHANGELOG-1.7]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.11|CHANGELOG-1.11]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.35|CHANGELOG-1.35]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.22|CHANGELOG-1.22]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.16|CHANGELOG-1.16]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.32|CHANGELOG-1.32]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.4|CHANGELOG-1.4]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.12|CHANGELOG-1.12]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.26|CHANGELOG-1.26]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.36|CHANGELOG-1.36]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.5|CHANGELOG-1.5]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.13|CHANGELOG-1.13]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.27|CHANGELOG-1.27]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.17|CHANGELOG-1.17]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.33|CHANGELOG-1.33]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.28|CHANGELOG-1.28]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.18|CHANGELOG-1.18]]
- [[domain-19-landscape-references/topic-release-notes/kubernetes/CHANGELOG-1.29|CHANGELOG-1.29]]
