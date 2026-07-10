---
title: etcd 知识图谱索引
description: '## 知识图谱'
summary: '## 知识图谱'
category: index
tags:
- k8s
- index
- catalog
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- istio
- envoy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 30min
intent_queries:
- etcd 知识图谱索引 是什么
- 如何 etcd 知识图谱索引
- etcd 相关文档汇总
trigger_keywords:
- etcd
- 知识图谱索引
- index
- knowledge-graph
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- ebpf-basics
- cilium-basics
- etcd-basics
- tls-basics
- policy-basics
- observability-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# etcd 知识图谱索引

> 知识图谱：按主题 **etcd** 聚合项目内所有相关内容，按关联度分层级组织。

---

## 一、etcd 核心文档 (直接相关)

> 这些文档以 etcd 为主题或直接面向 etcd 运维场景。

### 深度技术

- [[集群基础/控制平面/11-etcd-deep-dive.md|11 etcd deep dive]]
- etcd运维操作

### 故障排查与维护

- etcd 故障排查 (etcd Troubleshooting)
- [[故障诊断/高级排障/01-control-plane/02-etcd-troubleshooting.md|etcd 故障排查指南]]
- [[故障诊断/高级排障/10-etcd-maintenance.md|etcd 维护专项文档]]
- [[故障诊断/FTA故障树/list/etcd-fta.md|etcd 异常故障树分析 (etcd FTA)]]

### CNCF 生态

- etcd (CNCF Graduated)

---

## 二、etcd 关联文档 (K8s 集成)

> 这些文档涉及 etcd 但以其他 K8s 组件为主题。

### 控制平面核心

- 备份与灾难恢复 (含 etcd 备份)
- API Server 深度解析 (etcd 依赖)
- API Server 调优 (含 etcd 分库策略)
- kubeadm 集群生命周期管理
- 控制平面监控可观测性

### 集群创建与证书

- [[平台工程/代码分析/functions-cluster-create/07-etcd.md|etcd 集群初始化细节]]
- [[平台工程/代码分析/functions-cluster-create/13-etcd-advanced.md|etcd 进阶: 数据存储与维护]]
- [[平台工程/代码分析/functions-cluster-cert/04-etcd-cert.md|etcd 证书体系源码分析]]

### 平台运维

- 集群生命周期管理 (Cluster Lifecycle Management)
- Lease 与 Leader 选举机制 (与 etcd 强相关)

---

## 三、架构基础

- Kubernetes 核心组件深度剖析 (Core Components Deep Dive)
- 06 - 集群配置参数完全参考

## 设计原理

- Domain-2 设计原则 — 开源项目索引
- 01 - Kubernetes 设计原则与哲学 (Foundations)
- 02 - 声明式 API 与面向终态设计 (Declarative API)
- 03 - 控制器模式与调谐循环 (Controller Pattern)
- 04 - List-Watch 机制深度解析 (List-Watch)
- 05 - Informer 架构与工作队列 (Informer & Workqueue)
- 06 - 资源版本与并发控制 (Concurrency Control)
- 07 - 分布式共识与 etcd 原理 (etcd & Raft)
- 09 - Kubernetes 源码结构与阅读指南 (Source Code)

---

## 四、扩展故障排查参考

> 以下为 K8s 全域故障排查索引，etcd 问题可参考控制平面、网络、存储等章节。

### 控制平面故障排查

- [[故障诊断/高级排障/01-control-plane/01-apiserver-troubleshooting.md|API Server 故障排查指南]]
- [[故障诊断/高级排障/01-control-plane/03-scheduler-troubleshooting.md|Scheduler 故障排查指南]]
- [[故障诊断/高级排障/01-control-plane/04-controller-manager-troubleshooting.md|Controller Manager 故障排查指南]]
- [[故障诊断/高级排障/01-control-plane/08-control-plane-performance-troubleshooting.md|控制平面性能瓶颈分析与优化指南]]
- [[故障诊断/高级排障/01-control-plane/09-control-plane-ha-troubleshooting.md|控制平面高可用故障处理指南]]

### 网络与存储故障排查

- [[故障诊断/高级排障/03-networking/01-cni-troubleshooting.md|CNI 网络插件故障排查指南]]
- [[故障诊断/高级排障/04-storage/01-pv-pvc-troubleshooting.md|PV/PVC 存储深度排查与持久化治理指南]]

### 技能卡片

- [[故障诊断/技能体系/skill-set/k8s-node-notready/assets/escalation-template.md|升级消息模板 / Escalation Message Template]]
- [[故障诊断/技能体系/skill-set/k8s-node-notready/reference/diagnostic-workflow.md|诊断工作流 / Diagnostic Workflow]]
- [[故障诊断/技能体系/skill-set/k8s-node-notready/reference/remediation-playbook.md|修复操作手册 / Remediation Playbook]]
- [[故障诊断/技能体系/skill-set/k8s-node-notready/reference/root-cause-catalog.md|根因分类 / Root Cause Catalog]]
- [[故障诊断/技能体系/skill-set/k8s-node-notready/reference/version-matrix.md|版本兼容矩阵与知识进化 / Version Matrix & Knowledge Evolution]]
- [[故障诊断/技能体系/skill-set/k8s-node-notready/SKILL.md|K8s Node NotReady 诊断与修复]]
- [[故障诊断/技能体系/skill-set/k8s-node-notready/USAGE-GUIDE.md|Skills + FTA 使用指南 — k8s-node-notready & node-fta]]

## YAML 清单参考

- 17 - StorageClass / VolumeSnapshot YAML 配置参考
- 32 - Lease / Event / Node YAML 配置参考

## 术语词典

- [[系统基础/知识字典/configuration/secrets.md|Secrets]]
- [[系统基础/知识字典/fundamentals/cloud-controller-manager.md|Cloud Controller Manager（云控制器管理器）]]
- [[系统基础/知识字典/fundamentals/garbage-collection.md|Garbage Collection（垃圾回收）]]
- [[系统基础/知识字典/fundamentals/kubernetes-components.md|Kubernetes 组件]]
- [[系统基础/知识字典/fundamentals/kubernetes-concepts-reference.md|知识地图]]
- [[系统基础/知识字典/fundamentals/leases.md|Leases（租约）]]
- [[系统基础/知识字典/fundamentals/namespaces.md|命名空间]]
- [[系统基础/知识字典/fundamentals/nodes.md|Nodes（节点）]]
- [[系统基础/知识字典/fundamentals/storage-versions.md|存储版本]]
- [[系统基础/知识字典/fundamentals/the-kubectl-command-line-tool.md|kubectl 命令行工具]]
- [[系统基础/知识字典/networking/ingress-controllers.md|Ingress Controllers]]
- [[系统基础/知识字典/platform-engineering/admission-webhook-good-practices.md|Admission Webhook 最佳实践]]
- [[系统基础/知识字典/platform-engineering/api-priority-and-fairness.md|API 优先级与公平性（API Priority and Fairness）]]
- [[系统基础/知识字典/platform-engineering/cluster-api-and-fleet-management.md|Cluster API 与集群舰队管理]]
- [[系统基础/知识字典/platform-engineering/compatibility-version-for-control-plane.md|Kubernetes 控制平面组件的兼容版本]]
- [[系统基础/知识字典/platform-engineering/coordinated-leader-election.md|协调领导者选举（Coordinated Leader Election）]]
- [[系统基础/知识字典/platform-engineering/custom-resources.md|自定义资源]]
- [[系统基础/知识字典/platform-engineering/device-plugins.md|设备插件]]
- [[系统基础/知识字典/platform-engineering/extending-the-kubernetes-api.md|扩展 Kubernetes API]]
- [[系统基础/知识字典/platform-engineering/gitops-and-continuous-delivery.md|GitOps 与持续交付]]
- [[系统基础/知识字典/platform-engineering/infrastructure-as-code-for-kubernetes.md|Kubernetes 基础设施即代码（IaC）]]
- [[系统基础/知识字典/platform-engineering/operator-pattern.md|Operator 模式]]
- [[系统基础/知识字典/scheduling/gang-scheduling.md|Gang Scheduling]]
- [[系统基础/知识字典/scheduling/kubernetes-scheduler.md|Kubernetes Scheduler]]
- [[系统基础/知识字典/scheduling/pod-topology-spread-constraints.md|Pod Topology Spread Constraints]]
- [[系统基础/知识字典/scheduling/scheduler-performance-tuning.md|Scheduler Performance Tuning]]
- [[系统基础/知识字典/security/cloud-native-security-practices.md|09 - 云原生安全专家指南]]
- [[系统基础/知识字典/security/cloud-native-security.md|云原生安全]]
- [[系统基础/知识字典/security/controlling-access-to-the-kubernetes-api.md|控制对 Kubernetes API 的访问]]
- [[系统基础/知识字典/security/good-practices-for-kubernetes-secrets.md|Kubernetes Secrets 最佳实践]]
- [[系统基础/知识字典/security/kubernetes-api-server-bypass-risks.md|Kubernetes API Server 绕过风险]]
- [[系统基础/知识字典/security/multi-tenancy.md|多租户]]
- [[系统基础/知识字典/security/role-based-access-control-good-practices.md|基于角色的访问控制（RBAC）最佳实践]]
- [[系统基础/知识字典/security/secrets-management-deep-dive.md|密钥管理深度指南]]
- [[系统基础/知识字典/security/security-checklist.md|安全清单]]
- [[系统基础/知识字典/security/service-accounts.md|服务账号]]
- [[系统基础/知识字典/tooling/cli-commands.md|知识地图]]
- [[系统基础/知识字典/tooling/tool-ecosystem.md|Kusheet 工具与开源项目 URL 汇总]]

## Docker

- Docker 架构概述与核心概念
- Docker 镜像管理详解
- Docker 容器生命周期管理
- Docker 日志管理与分析
- Docker 自动化运维与CI/CD集成
- Java 应用容器化最佳实践指南
- Docker 命令大全参考

## Linux 基础

- 01 - Linux 系统架构与内核深度解析：生产环境运维专家指南
- 03 - Linux 文件系统深度解析：生产环境存储管理专家指南
- 05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南
- 06 - Linux 性能调优与瓶颈分析：生产环境性能优化专家指南
- 09 - Linux 运维基础与应急响应：生产环境运维专家实践指南
- Linux 命令大全参考

## 网络基础

- 网络安全基础
- Cilium eBPF 网络与安全实践指南

## 云服务商

- [[云厂商/AWS-EKS/aws-eks-overview.md|AWS EKS (Elastic Kubernetes Service) 概述]]
- [[云厂商/Google-GKE/google-cloud-gke-overview.md|Google Cloud GKE (Google Kubernetes Engine) 概述]]
- [[云厂商/Azure-AKS/azure-aks-overview.md|Azure AKS (Azure Kubernetes Service) 概述]]
- [[云厂商/阿里云/ack/245-ack-ebs-storage.md|ACK 关联产品 - EBS 云盘存储 (Elastic Block Storage)]]
- [[云厂商/阿里云/ack/alicloud-ack-overview.md|阿里云 ACK (Alibaba Cloud Container Service for Kubernetes) 概述]]
- [[云厂商/腾讯云TKE/tencent-tke-overview.md|腾讯云 TKE (Tencent Kubernetes Engine) 概述]]
- [[云厂商/华为云CCE/huawei-cce-overview.md|华为云 CCE (Cloud Container Engine) 企业级深度实战指南]]
- [[云厂商/其他云/UCloud-UK8S/ucloud-uk8s-overview.md|UCloud UK8S (UCloud Kubernetes Service) 高性价比企业级实战指南]]
- [[云厂商/其他云/IBM-IKS/ibm-iks-overview.md|IBM IKS (IBM Cloud Kubernetes Service) 概述]]
- [[云厂商/其他云/Oracle-OKE/oracle-oke-overview.md|Oracle OKE (Oracle Container Engine for Kubernetes) 企业级深度解析]]
- [[云厂商/其他云/火山引擎-VEK/volcengine-vek-overview.md|火山引擎 VEK (Volcengine Kubernetes) 字节级深度实战指南]]
- [[云厂商/其他云/天翼云-TKE/ctyun-tke-overview.md|天翼云 TKE (Tianyi Cloud Kubernetes Engine) 概述]]
- [[云厂商/其他云/移动云-CKE/ecloud-cke-overview.md|移动云 CKE (China Mobile Cloud Kubernetes Engine) 企业级深度实战指南]]
- [[云厂商/阿里云/apsara/alicloud-apsara-ack-overview.md|阿里云专有版 ACK (Apsara Stack ACK) 金融级深度解析]]

## 生产运维

- 01-生产架构设计原则
- 02-多云混合部署策略
- 03-边缘计算生产部署
- 04-企业级监控体系
- 06-APM应用性能监控
- 07-零信任安全架构
- 08-CIS基准合规检查
- 10-GitOps流水线实践
- 11-基础设施即代码
- 15-绿色计算可持续发展
- 16-企业级备份策略
- 17-灾难恢复演练
- 19-集群性能调优
- 20-网络性能优化
- 21-存储性能优化
- 22-变更管理流程
- 23. 事件响应处理 (Incident Response Handling)
- KEDA 事件驱动自动缩放实践指南
- Kubernetes 生产环境部署模式架构详解
- Kubernetes 多租户与资源隔离生产架构
- Kubernetes 生产环境完整架构蓝图

## 技术论文

- Domain-19 论文与参考 — 开源项目索引
- Kubernetes 大规模集群性能优化深度实践 (Large-Scale Cluster Performance Optimization)
- Kubernetes 安全零信任架构实施指南 (Zero Trust Security Architecture Implementation)
- Kubernetes GitOps 完整实践指南 (GitOps Complete Practice Guide)
- [[生态参考/论文/07-kubernetes-csi-storage-deep-practice.md|07 kubernetes csi storage deep practice]]
- Kubernetes 自动化运维与SRE实践 (Automation and SRE Practices)
- [[生态参考/论文/11-kubernetes-api-server-deep-optimization-extension.md|11 kubernetes api server deep optimization extension]]
- [[生态参考/论文/12-kubernetes-scheduler-deep-optimization-custom-scheduling.md|12 kubernetes scheduler deep optimization custom scheduling]]
- Kubernetes 多租户安全隔离与资源配额管理 (Multi-Tenancy Security Isolation and Resource Quota Management)
- Kubernetes 事件驱动架构与异步处理 (Event-Driven Architecture and Asynchronous Processing)
- Kubernetes 混沌工程与故障注入测试 (Chaos Engineering and Fault Injection Testing)
- Kubernetes 边缘计算与KubeEdge实践 (Edge Computing and KubeEdge Practice)
- Kubernetes Gateway API 与现代流量管理实践
- Kubernetes 供应链安全实践 (Supply Chain Security: SBOM, SLSA, and Sigstore)
- [[生态参考/论文/21-kubernetes-platform-engineering-internal-developer-platform.md|21 kubernetes platform engineering internal developer platform]]
- Kubernetes WebAssembly (Wasm) 工作负载实践 (WebAssembly Workloads on Kubernetes)
- Kubernetes OpenTelemetry 原生可观测性 (OpenTelemetry Native Observability)
- Kubernetes 策略即代码与治理自动化 (Policy-as-Code and Governance Automation)
- [[生态参考/论文/25-gke-autopilot-google-cloud-ai-infrastructure.md|25 gke autopilot google cloud ai infrastructure]]
- Kubernetes vCluster 与虚拟集群多租户 (vCluster and Virtual Cluster Multi-Tenancy)

## CNCF 生态

- Argo
- cert-manager
- containerd
- CoreDNS
- Crossplane
- Envoy
- etcd
- Flux
- Harbor
- Helm
- Knative
- KubeEdge
- Kubernetes
- SPIFFE
- TiKV
- The Update Framework (TUF)
- Vitess
- Buildpacks
- Cloud Custodian
- CNI (Container Network Interface)
- Cortex
- Flatcar Container Linux
- gRPC
- Karmada
- KServe
- KubeVirt
- Lima
- Notary Project
- OpenFeature
- OpenYurt
- Operator Framework
- Strimzi
- Antrea
- Armada
- Atlantis
- Bank-Vaults
- bpfman
- Carvel
- cdk8s
- ChaosBlade
- CloudNativePG
- Clusternet
- CoHDI (Composable Hyperconverged Disaggregated Infrastructure)
- Confidential Containers
- container2wasm
- ContainerSSH
- Copa (Copacetic)
- Cozystack
- Dalec (Declarative Application Linux Environment Creator)
- DevSpace
- Easegress
- Headlamp
- Hyperlight
- Inclavare Containers
- Inspektor Gadget
- K0s
- k3s
- K8sGPT
- K8up
- Kagent (Kubernetes AI Agent)
- Kanister
- kcp (Kubernetes-like Control Plane)
- kpt
- Krkn (Kraken)
- Kuadrant
- Kube-burner
- Kube-OVN
- kube-rs
- kube-vip
- Kubean
- KubeClipper
- KubeStellar
- Kubewarden
- KUDO (Kubernetes Universal Declarative Operator)
- Kured
- Logging Operator
- MetalLB
- Network Service Mesh (NSM)
- Open Cluster Management (OCM)
- OpenEBS
- OpenFunction
- openGemini
- ORAS
- OVN-Kubernetes
- Oxia
- Parsec (Platform AbstRaction for SECurity)
- Piraeus Datastore
- Porter
- Sermant
- Serverless Devs
- Shipwright
- SOPS
- Spiderpool
- Spin
- SpinKube
- Stacker
- Telepresence
- Tokenetes
- urunc (Unikernel Container Runtime)
- VS Code Kubernetes Tools
- WasmEdge
- xRegistry
- youki
- zot

---

## 五、培训学习

- Day 7: K8S 集群证书


<!-- risk-assessed -->
