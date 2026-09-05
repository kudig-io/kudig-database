---
title: Kubernetes
description: Kubernetes（简称 K8s）是 Google 开源的容器编排平台，现已成为容器编排的事实标准。它自动化了容器的部署、扩缩容、负载均衡和自愈，是云原生技术...
summary: Kubernetes（简称 K8s）是 Google 开源的容器编排平台，现已成为容器编排的事实标准。它自动化了容器的部署、扩缩容、负载均衡和自愈，是云原生技术...
category: dictionary
tags:
- k8s
- glossary
- kubernetes
- k8s
- container-orchestration
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes 是什么
- Kubernetes (K8s) 详解
trigger_keywords:
- Kubernetes
- Kubernetes (K8s)
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes

> **英文名**: Kubernetes (K8s)

## 概述

Kubernetes（简称 K8s）是 Google 开源的容器编排平台，现已成为容器编排的事实标准。它自动化了容器的部署、扩缩容、负载均衡和自愈，是云原生技术栈的核心基础设施。

## 核心概念/原理

### 核心架构

```
┌─────────────────────────────────┐
│         Control Plane           │
│  ┌──────────┐  ┌─────────────┐  │
│  │apiserver │  │  scheduler  │  │
│  └──────────┘  └─────────────┘  │
│  ┌──────────────────────────┐   │
│  │  controller-manager      │   │
│  └──────────────────────────┘   │
│  ┌──────┐                       │
│  │ etcd │                       │
│  └──────┘                       │
└─────────────────────────────────┘
         ↕
┌─────────────────────────────────┐
│         Worker Nodes            │
│  ┌────────┐  ┌───────────────┐  │
│  │kubelet │  │  kube-proxy   │  │
│  └────────┘  └───────────────┘  │
│  ┌─────────────────────────┐    │
│  │  Container Runtime      │    │
│  └─────────────────────────┘    │
└─────────────────────────────────┘
```

### 声明式模型

Kubernetes 采用声明式 API：用户描述「期望状态」（Desired State），控制器持续将「实际状态」推向「期望状态」。

## 关键机制或特性

- **自动调度**：根据资源需求和约束将 Pod 调度到最佳节点。
- **自愈能力**：Pod 崩溃自动重启，节点故障自动迁移。
- **水平扩缩**：通过 HPA/VPA 自动调整资源。
- **服务发现与负载均衡**：通过 Service 和 Ingress 暴露应用。
- **滚动更新与回滚**：零停机部署和快速回滚。
- **声明式配置**：GitOps 友好的基础设施即代码。

## 使用场景与最佳实践

- 使用 kubeadm 初始化生产级集群。
- 遵循最小权限原则配置 RBAC。
- 为所有工作负载设置 resource requests/limits。
- 使用命名空间隔离不同团队或环境的工作负载。
- 启用审计日志（Audit Log）追踪 API 操作。
- 定期升级集群版本，关注弃用 API 迁移。

## 参考链接

- [Kubernetes Official Documentation](https://kubernetes.io/docs/)

## Related

- [[17-系统基础/06-知识字典/workloads/pod.md|Pod]]
- [[17-系统基础/06-知识字典/fundamentals/node.md|Node]]
- [[17-系统基础/06-知识字典/fundamentals/namespace.md|Namespace]]
- [[17-系统基础/06-知识字典/fundamentals/cluster.md|Cluster]]
- [[17-系统基础/06-知识字典/fundamentals/cncf.md|CNCF]]


<!-- risk-assessed -->
- [[01-集群基础/01-架构总览/04-source-code-structure|04 - Kubernetes 源码结构深度解析]]
- [[01-集群基础/01-架构总览/20-kubernetes-v1.29-v1.33-features-guide|Kubernetes v1.29 - v1.33 版本特性深度指南]]
- [[01-集群基础/01-架构总览/18-kubernetes-v1.25-v1.33-feature-comparison-table|Kubernetes v1.25 - v1.33 特性对比总表]]
- [[01-集群基础/01-架构总览/13-observability-architecture|15 - Kubernetes 可观测性架构体系]]
- [[01-集群基础/01-架构总览/12-security-architecture|14 - Kubernetes 安全架构深度分析]]
- [[01-集群基础/01-架构总览/11-performance-tuning-guide|13 - Kubernetes 性能调优专项指南]]
- [[01-集群基础/01-架构总览/14-troubleshooting-guide|16 - Kubernetes 故障排查专家级指南]]
- [[01-集群基础/01-架构总览/10-cluster-deployment-patterns|12 - Kubernetes 集群部署架构模式指南]]
- [[01-集群基础/01-架构总览/16-kubernetes-core-components-v1.29-v1.33-update|Kubernetes 核心组件 v1.29 - v1.33 新特性速查]]
- [[01-集群基础/01-架构总览/02-core-components-deep-dive|Kubernetes 核心组件深度剖析]]
- [[01-集群基础/02-设计原则/17-observability-design-principles|16 - 可观测性设计原则]]
- [[01-集群基础/02-设计原则/03-declarative-api-pattern|02 - 声明式 API 与面向终态设计 (Declarative API)]]
- [[01-集群基础/02-设计原则/18-security-design-patterns|17 - 安全设计模式]]
- [[01-集群基础/02-设计原则/05-watch-list-mechanism|04 - List-Watch 机制深度解析 (List-Watch)]]
- [[01-集群基础/02-设计原则/01-design-principles-foundations|01 - Kubernetes 设计原则与哲学 (Foundations)]]
- [[01-集群基础/03-控制平面/11-etcd-deep-dive|etcd 深度解析]]
- [[01-集群基础/03-控制平面/24-metrics-server-deep-dive|metrics-server 深度解析 (metrics-server Deep Dive)]]
- [[01-集群基础/03-控制平面/20-kube-scheduler-deep-dive|Kubernetes Scheduler 深度解析 (Kube-Scheduler Deep Dive)]]
- [[01-集群基础/03-控制平面/02-plane-components-interaction|控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)]]
- [[01-集群基础/03-控制平面/35-kubeadm-cluster-lifecycle|32 - kubeadm 集群生命周期管理 (Cluster Lifecycle with kubeadm)]]
- [[01-集群基础/03-控制平面/16-kube-proxy-deep-dive|kube-proxy 深度解析 (kube-proxy Deep Dive)]]
- [[01-集群基础/03-控制平面/13-kube-controller-manager-deep-dive|kube-controller-manager 深度解析]]
- [[01-集群基础/03-控制平面/12-apiserver-deep-dive|kube-apiserver 深度解析]]
- [[01-集群基础/06-升级路径/01-cluster-configuration-parameters|06 - 集群配置参数完全参考]]
- [[02-工作负载/00-总览/02-kubernetes-multi-tenant-architecture|Kubernetes 多租户与资源隔离生产架构]]
- [[02-工作负载/01-核心工作负载/21-hpa-vpa-autoscaling|HPA/VPA 自动伸缩配置]]
- [[02-工作负载/01-核心工作负载/18-node-management-operations|27 - 节点与节点池管理 (Node & NodePool Management)]]
- [[02-工作负载/01-核心工作负载/README-old|Domain-4: Kubernetes工作负载]]
- [[02-工作负载/01-核心工作负载/24-kubernetes-v1.33-workloads-guide|Kubernetes v1.29-v1.33 工作负载管理新特性指南]]
- [[02-工作负载/01-核心工作负载/14-sidecar-containers-patterns|Sidecar 容器模式]]
- [[02-工作负载/01-核心工作负载/23-resource-management|16 - 资源管理表]]
- [[02-工作负载/02-Java-on-K8s/05-java-cicd-tekton-argocd|Java CI/CD on Kubernetes: Tekton + ArgoCD 实践指南 [topic-java-kubernetes]]]
- [[03-清单模式/01-YAML参考/32-lease-event-node|32 - Lease / Event / Node YAML 配置参考]]
- [[03-清单模式/01-YAML参考/10-ingress-ingressclass|10 - Ingress / IngressClass YAML 配置参考]]
- [[03-清单模式/01-YAML参考/34-component-configuration|34. Kubernetes 组件配置（Component Configuration）]]
- [[03-清单模式/01-YAML参考/12-gateway-api-advanced-routes|12 - Gateway API 高级路由 YAML 配置参考]]
- [[03-清单模式/01-YAML参考/09-endpoints-endpointslice|09 - Endpoints / EndpointSlice YAML 配置参考]]
- [[04-应用模式/02-行业架构/45-smart-port-shipping|智慧港口与航运架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/36-carbon-esg-management|碳资产管理与 ESG 架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/19-cloudnative-devops-architecture|云原生 DevOps 平台 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/04-im-rtc-architecture|实时通信 (IM / RTC) Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/84-national-park|国家公园架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/83-cultural-digitization|文化数字化架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/94-smart-prison|智慧监狱架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/68-quantum-computing-cloud|量子计算云平台架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/38-supply-chain-finance|供应链金融架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/91-urban-air-mobility|低空经济（eVTOL/UAM）架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/21-cross-border-ecommerce|跨境电商架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/69-6g-core-network|6G 核心网架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/71-smart-tax|智慧税务架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/02-mini-program-architecture|小程序平台 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/53-new-retail-dtc|新零售 DTC 架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/03-cms-architecture|内容管理系统 (CMS) Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/85-hydrogen-energy|氢能源架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/18-data-midplatform-architecture|数据中台 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/16-video-shortform-architecture|音视频与短视频平台 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/41-beauty-ecommerce|美妆电商架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/01-ecommerce-architecture|电商系统 Kubernetes 生产架构设计 (应用模式)]]
- [[04-应用模式/02-行业架构/55-crossborder-dtc|跨境电商独立站架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/27-hospitality-tourism|酒店旅游架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/87-flexible-manufacturing|柔性制造架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/34-sportstech|体育科技架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/28-proptech|房地产科技架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/59-industrial-internet-platform|工业互联网平台架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/54-social-gaming-metaverse|社交游戏与元宇宙社交架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/31-instant-retail|即时零售架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/33-crossborder-warehouse|跨境电商海外仓架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/05-online-education-architecture|在线教育平台 Kubernetes 生产架构设计 (应用模式)]]
- [[04-应用模式/02-行业架构/70-ecny-cbdc|数字人民币架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/62-distributed-energy|分布式能源架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/75-affective-computing|情感计算 AI 架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/50-unmanned-retail|无人零售与智能货柜架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/77-fusion-energy-monitoring|可控核聚变监控架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/79-polar-research|极地科考架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/73-smart-firefighting|智慧消防架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/20-microservice-governance-architecture|微服务治理与 Service Mesh Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/14-smart-healthcare-architecture|智慧医疗 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/96-carbon-capture|碳捕集利用与封存（CCUS）架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/78-deep-sea-exploration|深海探测架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/12-smart-logistics-architecture|智慧物流与供应链 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/58-web3-gamefi|Web3 GameFi 架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/76-synthetic-biology|合成生物学架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/17-saas-multitenant-architecture|SaaS 多租户平台 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/11-smart-retail-architecture|智慧零售与新零售 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/81-smart-customs|智慧海关架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/24-insurtech|保险科技架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/90-neuromorphic-computing|类脑计算架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/46-satellite-internet|卫星互联网架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/52-smart-water|智慧水务架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/82-legaltech|司法科技架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/15-energy-power-architecture|能源电力 Kubernetes 生产架构设计 (应用模式)]]
- [[04-应用模式/02-行业架构/37-pet-economy|宠物经济架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/49-livestream-ecommerce|直播电商架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/66-space-internet|太空互联网架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/06-fintech-architecture|金融科技 (FinTech) Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/88-nanomaterials|纳米材料架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/39-smart-campus|智慧园区架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/13-digital-government-architecture|数字政务 Kubernetes 生产架构设计]]
- [[04-应用模式/02-行业架构/48-vocational-edtech|职业教育培训架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/72-digital-twin-city|数字孪生城市架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/32-smart-restaurant|智慧餐饮架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/89-crispr-gene-editing|基因编辑 CRISPR 架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/56-smart-elderly-care|智慧养老架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/44-martech-adtech|数字营销与广告科技架构设计 — 阿里云视角]]
- [[04-应用模式/02-行业架构/95-industrial-metaverse|工业元宇宙架构设计 — 阿里云视角]]
- [[05-网络/01-K8s网络核心/11-service-advanced-features|Service 高级特性与应用案例 (Service Advanced Features)]]
- [[05-网络/01-K8s网络核心/29-coredns-troubleshooting-optimization|56 - CoreDNS 故障排查与性能优化 (Troubleshooting & Optimization)]]
- [[05-网络/01-K8s网络核心/16-coredns-plugins-reference|55 - CoreDNS 插件完整参考 (Plugins Reference)]]
- [[05-网络/01-K8s网络核心/23-ingress-tls-certificate|130 - Ingress TLS 与证书管理]]
- [[05-网络/01-K8s网络核心/13-dns-service-discovery|33 - 服务发现与 DNS 配置 (Service Discovery & DNS)]]
- [[05-网络/01-K8s网络核心/28-cni-troubleshooting-optimization|144 - CNI 故障排查与优化 (CNI Troubleshooting & Optimization)]]
- [[05-网络/01-K8s网络核心/01-network-architecture-overview-faq|FAQ 文档]]
- [[05-网络/01-K8s网络核心/26-ingress-monitoring-troubleshooting|133 - Ingress 监控与故障排查]]
- [[05-网络/01-K8s网络核心/04b-flannel-ipv6-dual-stack|Flannel IPv6 Dual Stack 支持]]
- [[05-网络/03-服务网格/06-traefik-mesh-enterprise|Traefik Mesh Enterprise Service Mesh 深度实践]]
- [[05-网络/03-服务网格/03-consul-connect-enterprise|Consul Connect 企业级服务网格管理]]
- [[05-网络/03-服务网格/13-spring-cloud-kubernetes-service-mesh-guide|Spring Cloud Kubernetes 与服务网格集成指南]]
- [[05-网络/04-API网关/13-api-gateway-performance-benchmarks|13 - API 网关性能基准测试与调优]]
- [[05-网络/04-API网关/12-api-gateway-observability|12 - API 网关可观测性：指标、日志与链路追踪]]
- [[05-网络/04-API网关/07-envoy-gateway-enterprise|07 - Envoy Gateway 企业级实践]]
- [[05-网络/04-API网关/09-nginx-ingress-migration-guide|09 - 传统 Ingress 控制器向云原生 API 网关迁移]]
- [[05-网络/06-Terway/06-performance|06 - Terway 性能调优 (Performance Tuning)]]
- [[08-安全/05-供应链/11-compliance-automation-audit|合规自动化与审计 (Compliance Automation and Audit)]]
- [[08-安全/05-供应链/09-policy-controller-verification|Policy Controller 镜像验证 (Policy Controller Image Verification)]]
- [[08-安全/06-合规审计/10-compliance-audit-practices|Kubernetes 合规与审计]]
- [[08-安全/06-合规审计/07-certificate-management|证书管理与 TLS 配置]]
- [[08-安全/06-合规审计/11-comprehensive-security-scanning|17 - 安全扫描与漏洞检测工具]]
- [[09-可观测性/00-总览/15-kubernetes-v1.33-observability-guide|Kubernetes v1.29-v1.33 可观测性新特性指南]]
- [[10-平台工程/01-构建/15-backstage-idp-guide|Backstage 内部开发者平台 (IDP) 构建指南]]
- [[10-平台工程/02-运维/19-kubernetes-v1.33-platform-ops-guide|Kubernetes v1.29-v1.33 平台运维新特性指南]]
- [[10-平台工程/02-运维/09-backup-recovery-strategy|Kubernetes 备份与恢复概述 (Backup & Recovery Overview)]]
- [[11-发布变更/01-GitOps/12-gitops-pipeline-practices|10-GitOps流水线实践]]
- [[15-AI基础设施/01-基础设施/29-alibaba-cloud-integration|15 - 阿里云特定集成表]]
- [[15-AI基础设施/02-AI-Agents/15-agent-corpus-gap-analysis|Agent 语料库差距分析：kudig-database 作为 K8s 运维 Agent 语料还缺什么？ [02-ai-agents]]]
- [[15-AI基础设施/02-AI-Agents/openclaw-workspace/SOUL|KuDig Doctor — 角色人格与绝对红线 (02-ai-agents)]]
- [[17-系统基础/02-硬件/16-kubernetes-hardware-troubleshooting|Kubernetes 运维硬件故障排查专题]]
- [[17-系统基础/04-K8s事件/09-job-cronjob-batch-events|09 - Job 与 CronJob 批处理事件]]
- [[17-系统基础/04-K8s事件/11-storage-volume-events|11 - 存储与卷事件]]
- [[17-系统基础/04-K8s事件/12-autoscaling-events|12 - 自动扩缩容事件 (HPA / VPA / Cluster Autoscaler)]]
- [[17-系统基础/04-K8s事件/03-image-pull-events|03 - 镜像拉取事件]]
- [[17-系统基础/04-K8s事件/15-ecosystem-addon-events|15 - 生态系统与插件事件]]
- [[17-系统基础/04-K8s事件/08-statefulset-daemonset-events|08 - StatefulSet 与 DaemonSet 控制器事件]]
- [[17-系统基础/04-K8s事件/02-pod-container-lifecycle-events|02 - Pod 与容器生命周期事件]]
- [[17-系统基础/04-K8s事件/05-scheduling-preemption-events|05 - 调度与抢占事件]]
- [[17-系统基础/04-K8s事件/01-event-system-architecture|01 - Kubernetes 事件系统架构与 API 参考]]
- [[17-系统基础/04-K8s事件/06-node-lifecycle-condition-events|06 - 节点生命周期与状态事件]]
- [[17-系统基础/04-K8s事件/14-namespace-resource-gc-events|14 - Namespace、资源管理与垃圾回收事件]]
- [[17-系统基础/04-K8s事件/10-service-networking-events|10 - Service 与网络事件]]
- [[17-系统基础/05-速查卡/gateway-api|Gateway API]]
- [[17-系统基础/06-知识字典/fundamentals/recommended-labels|推荐标签]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-object-management|Kubernetes 对象管理]]
- [[17-系统基础/06-知识字典/fundamentals/namespaces|命名空间]]
- [[17-系统基础/06-知识字典/fundamentals/nodes|Nodes（节点）]]
- [[17-系统基础/06-知识字典/fundamentals/communication-between-nodes-and-the-control-plane|Communication between Nodes and the Control Plane（节点与控制平面之间的通信）]]
- [[17-系统基础/06-知识字典/fundamentals/the-kubectl-command-line-tool|kubectl 命令行工具]]
- [[17-系统基础/06-知识字典/fundamentals/garbage-collection|Garbage Collection（垃圾回收）]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-self-healing|Kubernetes Self-Healing（Kubernetes 自愈能力）]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-concepts-reference|Kubernetes Concepts Reference]]
- [[17-系统基础/06-知识字典/fundamentals/objects-in-kubernetes|Kubernetes 中的对象]]
- [[17-系统基础/06-知识字典/fundamentals/cloud-controller-manager|Cloud Controller Manager（云控制器管理器）]]
- [[17-系统基础/06-知识字典/fundamentals/storage-versions|存储版本]]
- [[17-系统基础/06-知识字典/multi-cloud/edge-computing-and-k3s|边缘计算与轻量级 Kubernetes]]
- [[17-系统基础/06-知识字典/multi-cloud/multi-cloud-operations|10 - 多云混合云运维手册]]
- [[17-系统基础/06-知识字典/networking/network-policies|Network Policies]]
- [[17-系统基础/06-知识字典/networking/gateway-api|Gateway API]]
- [[17-系统基础/06-知识字典/networking/ingress-controllers|Ingress Controllers]]
- [[17-系统基础/06-知识字典/networking/dns-for-services-and-pods|DNS for Services and Pods]]
- [[17-系统基础/06-知识字典/networking/telco-cloud-and-5g-mec|电信云与 5G 多接入边缘计算（MEC）]]
- [[17-系统基础/06-知识字典/networking/cluster-mesh|多集群网络互联（Cluster Mesh）]]
- [[17-系统基础/06-知识字典/observability/log-aggregation-with-loki|日志聚合与 Loki]]
- [[17-系统基础/06-知识字典/observability/metrics-for-kubernetes-system-components|Kubernetes 系统组件指标]]
- [[17-系统基础/06-知识字典/observability/traces-for-kubernetes-system-components|Kubernetes 系统组件链路追踪]]
- [[17-系统基础/06-知识字典/observability/alerting-and-slo-monitoring|告警与 SLO 监控工程]]
- [[17-系统基础/06-知识字典/observability/opentelemetry-and-distributed-tracing|OpenTelemetry 与分布式链路追踪]]
- [[17-系统基础/06-知识字典/operations/operations-best-practices|01 - Kubernetes 生产环境运维最佳实践字典]]
- [[17-系统基础/06-知识字典/operations/failure-patterns-analysis|02 - Kubernetes 故障模式与根因分析字典]]
- [[17-系统基础/06-知识字典/operations/capacity-planning-forecasting|13 - 容量规划与资源预测]]
- [[17-系统基础/06-知识字典/operations/certificates|Certificates（PKI 证书与要求）]]
- [[17-系统基础/06-知识字典/operations/backup-disaster-recovery|备份与灾难恢复（Backup & Disaster Recovery）]]
- [[17-系统基础/06-知识字典/platform-engineering/kubernetes-api-aggregation-layer|Kubernetes API 聚合层]]
- [[17-系统基础/06-知识字典/platform-engineering/developer-portal-and-platform-metrics|开发者门户与平台工程度量]]
- [[17-系统基础/06-知识字典/platform-engineering/webassembly-wasm-workloads|WebAssembly（Wasm）工作负载]]
- [[17-系统基础/06-知识字典/platform-engineering/proxies-in-kubernetes|Kubernetes 中的代理]]
- [[17-系统基础/06-知识字典/platform-engineering/compatibility-version-for-control-plane|Kubernetes 控制平面组件的兼容版本]]
- [[17-系统基础/06-知识字典/platform-engineering/api-priority-and-fairness|API 优先级与公平性（API Priority and Fairness）]]
- [[17-系统基础/06-知识字典/platform-engineering/infrastructure-as-code-for-kubernetes|Kubernetes 基础设施即代码（IaC）]]
- [[17-系统基础/06-知识字典/platform-engineering/kubevirt-virtual-machines|KubeVirt：在 Kubernetes 上运行虚拟机]]
- [[17-系统基础/06-知识字典/scheduling/node-declared-features|Node Declared Features]]
- [[17-系统基础/06-知识字典/scheduling/assigning-pods-to-nodes|Assigning Pods to Nodes]]
- [[17-系统基础/06-知识字典/scheduling/pod-scheduling-readiness|Pod Scheduling Readiness]]
- [[17-系统基础/06-知识字典/scheduling/pod-priority-and-preemption|Pod Priority and Preemption]]
- [[17-系统基础/06-知识字典/scheduling/api-initiated-eviction|API-initiated Eviction]]
- [[17-系统基础/06-知识字典/security/policy-as-code|策略即代码（Policy as Code）]]
- [[17-系统基础/06-知识字典/security/service-accounts|服务账号]]
- [[17-系统基础/06-知识字典/security/role-based-access-control-good-practices|基于角色的访问控制（RBAC）最佳实践]]
- [[17-系统基础/06-知识字典/security/kubernetes-api-server-bypass-risks|Kubernetes API Server 绕过风险]]
- [[17-系统基础/06-知识字典/security/secrets-management-deep-dive|密钥管理深度指南]]
- [[17-系统基础/06-知识字典/security/security-checklist|安全清单]]
- [[17-系统基础/06-知识字典/security/spiffe-spire-identity|SPIFFE / SPIRE 与工作负载身份]]
- [[17-系统基础/06-知识字典/specialized-workloads/windows-containers-in-kubernetes|Windows 容器在 Kubernetes 中的支持]]
- [[17-系统基础/06-知识字典/specialized-workloads/guide-for-running-windows-containers-in-kubernetes|在 Kubernetes 中运行 Windows 容器指南]]
- [[17-系统基础/06-知识字典/specialized-workloads/ai-infra-specialist|08 - AI/ML基础设施专业词典]]
- [[17-系统基础/06-知识字典/specialized-workloads/vector-databases-and-rag-infrastructure|向量数据库与 RAG 基础设施]]
- [[17-系统基础/06-知识字典/storage/storage-capacity|Storage Capacity（存储容量）]]
- [[17-系统基础/06-知识字典/storage/dynamic-volume-provisioning|Dynamic Volume Provisioning（动态卷供给）]]
- [[17-系统基础/06-知识字典/storage/volumes|Volumes（卷）]]
- [[17-系统基础/06-知识字典/storage/node-specific-volume-limits|Node-specific Volume Limits（节点特定卷限制）]]
- [[17-系统基础/06-知识字典/storage/volume-attributes-classes|Volume Attributes Classes（卷属性类）]]
- [[17-系统基础/06-知识字典/storage/volume-health-monitoring|Volume Health Monitoring（卷健康监控）]]
- [[17-系统基础/06-知识字典/tooling/tool-ecosystem|Kusheet 工具与开源项目 URL 汇总]]
- [[17-系统基础/06-知识字典/tooling/cli-commands|查看所有 Pod 及其详细信息]]
- [[17-系统基础/06-知识字典/workloads/managing-workloads|Managing Workloads]]
- [[17-系统基础/06-知识字典/workloads/workload-management|Workload Management]]
- [[18-云厂商/01-阿里云/专有云-ACK/01-专有云架构概述|阿里云专有云架构概述]]
- [[18-云厂商/07-多云混合/07-huawei-cce-enterprise|华为云 CCE 企业级容器平台深度实践]]
- [[19-故障诊断/01-核心排障/02-control-plane-etcd-troubleshooting|etcd 故障排查]]
- [[19-故障诊断/02-资源排障/05-certificate-troubleshooting|证书故障排查]]
- [[19-故障诊断/04-高级排障/structural-README|Kubernetes 结构化故障排查知识库 [故障诊断]]]
- [[19-故障诊断/04-高级排障/10-kind-k3s-single-node-troubleshooting|Kind / K3s 单机集群故障排查]]
- [[19-故障诊断/08-技能体系/26-namespace-quota-limitrange|Namespace/Quota/LimitRange 故障诊断与修复 / Namespace Quota & LimitRange Failure Diagnosis & Remediation]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-node-notready/reference/diagnostic-workflow|诊断工作流 / Diagnostic Workflow]]
- [[22-概念/01-核心架构/kubernetes-core-concepts|Kubernetes Core Concepts]]
- [[22-概念/01-核心架构/etcd-operational-reference|etcd Operational Reference]]
- [[22-概念/01-核心架构/declarative-api|Declarative API]]
- [[22-概念/01-核心架构/core-dependency-version-matrix|核心依赖版本矩阵]]
- [[22-概念/01-核心架构/watch-mechanism|Watch Mechanism (List-Watch)]]
- [[22-概念/05-安全/linux-security-modules|Linux Security Modules for Containers]]
- [[22-概念/08-可靠性与运维/kubeadm-cluster-operations|kubeadm 集群运维全景]]
- [[22-概念/08-可靠性与运维/K8s-故障分布与-MTTR-基准|K8s 问题分布与 MTTR 基准]]
- [[22-概念/08-可靠性与运维/Structural-Troubleshooting-Framework|Structural Troubleshooting Framework]]
- [[22-概念/10-最佳实践/bp-README|Kubernetes 最佳实践指南]]
- [[22-概念/11-交叉分析/etcd × 高可用模式|etcd × 高可用模式]]
- [[22-概念/11-交叉分析/eBPF × 运行时安全|eBPF x 运行时安全]]
- [[22-概念/12-研究/kubernetes-version-evolution|Kubernetes 版本演进]]
- [[22-概念/12-研究/ai-agent-openclaw-workspace|OpenClaw 工作空间配置]]
- [[22-概念/12-研究/ai-agent-README|AI Agent 工程专题]]
- [[22-概念/12-研究/security-tool-evolution|安全工具演进]]
- [[22-概念/15-运行时与系统/linux-sysctl-tuning|Linux Sysctl Tuning for Kubernetes]]
- [[23-实体/02-K8s核心组件/kube-controller-manager|kube-controller-manager]]
- [[23-实体/02-K8s核心组件/cloud-controller-manager|cloud-controller-manager]]
- [[23-实体/07-可观测性/inspektor-gadget|Inspektor Gadget [entities]]]
- [[23-实体/08-交付与制品/porter|Porter (entities)]]
- [[23-实体/09-编排调度/metal3-io|Metal3]]
- [[23-实体/15-参考与索引/specialized-workloads-terms|K8s 专用工作负载术语参考]]
- [[23-实体/15-参考与索引/linux-sysctl-reference|Linux Sysctl Reference for Kubernetes]]
- [[23-实体/15-参考与索引/networking-terms|K8s 网络术语参考]]
- [[23-实体/15-参考与索引/kubernetes-changelog|Kubernetes 变更日志索引]]
- [[23-实体/15-参考与索引/k8s-design-principles-deep-dive|设计原理：声明式 API、控制器模式与 etcd 共识]]
- [[23-实体/15-参考与索引/workloads-terms|K8s 工作负载术语参考]]
- [[23-实体/15-参考与索引/k8s-glossary-index|K8s 术语表索引]]
- [[23-实体/15-参考与索引/fundamentals-terms|K8s 基础概念术语参考]]
- [[23-实体/15-参考与索引/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]]
- [[23-实体/15-参考与索引/release-notes-kubernetes|发布说明索引 — Kubernetes]]
- [[23-实体/15-参考与索引/k8s-architecture-fundamentals|K8s 架构基础与核心组件原理]]
- [[23-实体/15-参考与索引/root-terms|K8s Root术语参考]]
- [[23-实体/15-参考与索引/scheduling-terms|K8s 调度术语参考]]
- [[23-实体/15-参考与索引/k8s-advanced-ecosystem|硬件知识体系、CNCF 全景生态与 eBPF 平台工程]]
- [[23-实体/15-参考与索引/storage-terms|K8s 存储术语参考]]
- [[23-实体/15-参考与索引/observability-terms|K8s 可观测性术语参考]]
- [[23-实体/15-参考与索引/kubectl Scenario Quick Reference|kubectl Scenario Quick Reference]]
- [[23-实体/15-参考与索引/cncf-security|CNCF 安全与合规项目全景]]
- [[23-实体/15-参考与索引/k8s-deployment-create|Kubernetes Deployment 创建操作指南]]
- [[23-实体/15-参考与索引/k8s-knowledge-map|Kubernetes Knowledge Map]]
- [[23-实体/15-参考与索引/k8s-cluster-delete|Kubernetes 集群删除操作指南]]
- [[23-实体/15-参考与索引/KUDIG Frontmatter Spec|KUDIG Frontmatter Specification]]
- [[23-实体/15-参考与索引/k8s-cluster-create|Kubernetes 集群创建操作指南]]
- [[23-实体/15-参考与索引/configuration-terms|K8s 配置管理术语参考]]
- [[23-实体/15-参考与索引/tooling-terms|K8s 工具链术语参考]]
- [[23-实体/15-参考与索引/k8s-cluster-cert|Kubernetes 集群证书管理操作指南]]
- [[23-实体/15-参考与索引/k8s-node-create|Kubernetes 节点管理操作指南]]
- [[23-实体/15-参考与索引/platform-engineering-terms|K8s 平台工程术语参考]]
- [[23-实体/15-参考与索引/multi-cloud-terms|K8s 多云架构术语参考]]
- [[23-实体/15-参考与索引/version-upgrade-guide|版本升级指南]]
- [[23-实体/15-参考与索引/k8s-difficulty-index|Kubernetes Difficulty Index]]
- [[23-实体/15-参考与索引/operations-terms|K8s 运维运营术语参考]]
- [[26-技能/01-集群运维/cluster-upgrade/reference/skill-reference-version-matrix|Version Matrix]]
- [[26-技能/01-集群运维/cluster-upgrade/最佳实践/k8s-cluster-configuration-guide|Kubernetes 集群配置最佳实践]]
- [[26-技能/01-集群运维/cluster-upgrade/最佳实践/bp-README|Kubernetes 最佳实践指南 (skills)]]
- [[26-技能/01-集群运维/cluster-upgrade/最佳实践/scen-README|生产场景导航]]
- [[26-技能/01-集群运维/gitops-argocd/诊断排障/ts-gitops-devops|GitOps/DevOps 排查]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-cluster-deletion|kubeadm 集群删除操作]]
- [[26-技能/01-集群运维/kubeadm/kubeadm-ha-cluster-setup|kubeadm 高可用集群搭建]]
- [[26-技能/02-控制面/apiserver/诊断排障/ts-control-plane|控制平面故障排查]]
- [[26-技能/02-控制面/scheduler/培训/learn-15-scheduling-basics|第15课：调度与亲和性]]
- [[26-技能/03-节点/skill-19-node-resource-pressure|节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation (skills)]]
- [[26-技能/03-节点/node/skill-notready/skill-k8s-node-notready-USAGE-GUIDE|Usage Guide]]
- [[26-技能/03-节点/node/skill-notready/skill-assets-escalation-template|Escalation Template]]
- [[26-技能/03-节点/node/运维操作/node-drain-and-maintenance|节点驱逐与维护]]
- [[26-技能/04-工作负载/daemonset/培训/learn-13-daemonset-basics|第13课：DaemonSet 与节点守护]]
- [[26-技能/04-工作负载/deployment/deployment-canary-and-bluegreen|金丝雀与蓝绿发布]]
- [[26-技能/04-工作负载/deployment/deployment-workload-selection|工作负载控制器选型]]
- [[26-技能/04-工作负载/deployment/deployment-rolling-update|Deployment 滚动更新策略]]
- [[26-技能/04-工作负载/pod/培训/learn-01-day-one-checklist|Day 1: 新人首日检查清单]]
- [[26-技能/04-工作负载/pod/培训/learn-README|新人上手快速路径（Quick Start）]]
- [[26-技能/04-工作负载/pod/培训/learn-01-what-is-kubernetes|第一课：Kubernetes 入门]]
- [[26-技能/04-工作负载/pod/培训/learn-10-health-check|第八课：健康检查 - Probe 详解]]
- [[26-技能/04-工作负载/pod/培训/learn-03-oncall-handoff|Day 3: 值班交接 SOP]]
- [[26-技能/04-工作负载/pod/培训/learn-lecturer-persona|K8S 讲师角色设定与场景规范]]
- [[26-技能/04-工作负载/pod/培训/learn-inner-training|Kubernetes 培训：Inner Training]]
- [[26-技能/04-工作负载/pod/培训/learn-root|Kubernetes 培训：Root]]
- [[26-技能/04-工作负载/pod/培训/learn-02-pod-basics|第二课：Pod - K8s 的最小调度单元]]
- [[26-技能/04-工作负载/pod/培训/learn-public-training|Kubernetes 培训：Public Training]]
- [[26-技能/04-工作负载/pod/培训/learn-04-debug-tools-setup|Day 4: 调试工具全家桶安装]]
- [[26-技能/04-工作负载/pod/培训/learn-02-first-ticket-guide|Day 2: 第一个工单处理指南]]
- [[26-技能/04-工作负载/pod/培训/learn-oncall-quick-qa|工单数字人快速问答 - On-Call 速查]]
- [[26-技能/04-工作负载/pod/培训/training-public-README|K8s 学习与培训体系]]
- [[26-技能/04-工作负载/pod/培训/learn-12-common-problems|第十课：常见问题排查]]
- [[26-技能/04-工作负载/pod/培训/inner-training/week-1-ack-acr-lifecycle/day-7-cluster-certificate|Day 7: K8S 集群证书]]
- [[26-技能/04-工作负载/pod/培训/lecturer/lecturer-README|Kubernetes 金牌讲师 - 工单数字人场景]]
- [[26-技能/04-工作负载/pod/培训/public-one-month/week-1-foundation/day-5-k8s-architecture|Day 5: Kubernetes 架构全貌]]
- [[26-技能/04-工作负载/pod/培训/测验/assessment-k8s-fundamentals-quiz|K8S Fundamentals Quiz]]
- [[26-技能/04-工作负载/pod/培训/测验/assessment-troubleshooting-lab-exam|Troubleshooting Lab Exam]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes Diagnostic Skills Overview|Kubernetes Diagnostic Skills Overview]]
- [[26-技能/04-工作负载/pod/方法论/skill-reference-diagnostic-workflow|Diagnostic Workflow]]
- [[26-技能/04-工作负载/pod/方法论/skill-reference-root-cause-catalog|Root Cause Catalog]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes FTA Top Events Index|Kubernetes FTA Top Events Index]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles|FTA Methodology and Core Principles]]
- [[26-技能/04-工作负载/pod/清单规范/04-poddisruptionbudget-reference|28 - PodDisruptionBudget YAML 配置参考]]
- [[26-技能/04-工作负载/pod/清单规范/05-advanced-pod-patterns|35 - 高级 Pod 模式与调度策略 YAML 配置参考]]
- [[26-技能/04-工作负载/pod/生命周期与事件/01-pod-container-lifecycle-events|02 - Pod 与容器生命周期事件]]
- [[26-技能/04-工作负载/pod/诊断排障/技能体系-02-pod-crashloop-oomkilled|Pod CrashLoopBackOff & OOMKilled 诊断与修复]]
- [[26-技能/04-工作负载/pod/诊断排障/01-pod-pending-diagnosis|Pod Pending 状态深度诊断]]
- [[26-技能/04-工作负载/pod/诊断排障/技能体系-03-pod-pending|Pod Pending 调度失败诊断与修复]]
- [[26-技能/04-工作负载/pod/调度/assigning-pods-to-nodes|Assigning Pods to Nodes]]
- [[26-技能/04-工作负载/pod/调度/pod-scheduling-readiness|Pod Scheduling Readiness]]
- [[26-技能/04-工作负载/pod/调度/pod-priority-and-preemption|Pod Priority and Preemption]]
- [[26-技能/04-工作负载/pod/调度/字典-pod-overhead|Pod Overhead]]
- [[26-技能/04-工作负载/pod/配置与字典/dns-for-services-and-pods|DNS for Services and Pods]]
- [[26-技能/04-工作负载/statefulset/skill-21-statefulset-failure|StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation (skills)]]
- [[26-技能/05-网络/cni/培训/kubernetes-terway-presentation|Kubernetes Terway (Aliyun) 全栈进阶培训 (从入门到专家) [topic-presentations]]]
- [[26-技能/05-网络/networkpolicy/skill-20-networkpolicy-connectivity|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting (skills)]]
- [[26-技能/05-网络/service/培训/kubernetes-service-presentation|Kubernetes Service 全栈进阶培训 (从入门到专家) [topic-presentations]]]
- [[26-技能/05-网络/service/培训/learn-04-service-basics|第四课：Service - 让应用可以被访问]]
- [[26-技能/06-存储/csi-storage/最佳实践/k8s-storage-configuration-guide|Kubernetes 存储配置最佳实践]]
- [[26-技能/06-存储/csi-storage/诊断排障/ts-storage|存储故障排查]]
- [[26-技能/07-安全/rbac/诊断排障/ts-security-auth|安全认证故障排查]]
- [[26-技能/07-安全/resource-quota/培训/learn-07-namespace-resource-quota|第七课：Namespace 与资源隔离]]
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-logging-management-guide|Kubernetes 日志管理最佳实践]]
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-monitoring-guide|Kubernetes 监控最佳实践]]
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-distributed-tracing-guide|Kubernetes 分布式追踪最佳实践]]
