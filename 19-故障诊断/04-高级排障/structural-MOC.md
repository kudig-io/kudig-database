---
title: topic-structural-trouble-shooting MOC
description: topic-structural-trouble-shooting 专题导航页，覆盖 71 篇文档
summary: topic-structural-trouble-shooting 专题导航页，覆盖 71 篇文档
category: moc
tags:
- k8s
- moc
- troubleshooting
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- istio
- flannel
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- topic-structural-trouble-shooting MOC 是什么
- 如何 topic-structural-trouble-shooting MOC
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- topic-structural-trouble-shooting MOC 故障排查
- topic-structural-trouble-shooting MOC 排障步骤
trigger_keywords:
- topic-structural-trouble-shooting
- MOC
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- service-mesh-basics
- etcd-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# topic-structural-trouble-shooting MOC.md|MOC]]

> **MOC 版本**: 1.0
> **专题**: topic-structural-trouble-shooting
> **文档数量**: 71 篇
> **最后更新**: 2026-05-21
> **用途**: 本专题的导航入口，汇总所有相关文档

---

## 专题概述

结构化故障排查 — 系统性排障方法论

### 专题定位

| 维度 | 说明 |
|---|---|
| **专题** | topic-structural-trouble-shooting |
| **文档数量** | 71 篇（展示前 50 篇） |
| **难度分布** | 入门 0 / 进阶 0 / 高级 1 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | [[19-故障诊断/04-高级排障/00-configuration-first-methodology.md|疑难问题系统性排查方法论：配置优先（Configuration-First）]] |  | troubleshooting, guide, configuration |  |
| 2 | [[19-故障诊断/04-高级排障/01-control-plane/01-apiserver-troubleshooting.md|[[API Server 故障排查指南|API Server 故障排查指南]]]] |  | troubleshooting, guide |  |
| 3 | [[19-故障诊断/04-高级排障/01-control-plane/02-etcd-troubleshooting.md|[[etcd|etcd]]cd 故障排查指南|etcd 故障排查指南]]]] |  | troubleshooting, guide |  |
| 4 | [[19-故障诊断/04-高级排障/01-control-plane/03-scheduler-troubleshooting.md|Scheduler 故障排查指南]] |  | troubleshooting, guide |  |
| 5 | [[19-故障诊断/04-高级排障/01-control-plane/04-controller-manager-troubleshooting.md|Controller Manager 故障排查指南]] |  | troubleshooting, guide |  |
| 6 | [[19-故障诊断/04-高级排障/01-control-plane/05-webhook-admission-troubleshooting.md|Webhook 与准入控制故障排查指南]] |  | troubleshooting, guide |  |
| 7 | [[19-故障诊断/04-高级排障/01-control-plane/06-apf-troubleshooting.md|API 优先级与公平性 (APF) 故障排查指南]] |  | troubleshooting, guide |  |
| 8 | [[19-故障诊断/04-高级排障/01-control-plane/07-control-plane-security-troubleshooting.md|控制平面安全加固故障排查指南]] |  | troubleshooting, guide, security |  |
| 9 | [[19-故障诊断/04-高级排障/01-control-plane/08-control-plane-performance-troubleshooting.md|控制平面性能瓶颈分析与优化指南]] |  | troubleshooting, guide, performance |  |
| 10 | [[19-故障诊断/04-高级排障/01-control-plane/09-control-plane-ha-troubleshooting.md|控制平面高可用故障处理指南]] |  | troubleshooting, guide |  |
| 11 | [[19-故障诊断/04-高级排障/01-control-plane/10-control-plane-upgrade-troubleshooting.md|控制平面升级迁移问题处理指南]] |  | troubleshooting, guide, upgrade |  |
| 12 | [[19-故障诊断/04-高级排障/02-node-components/01-kubelet-troubleshooting.md|kubelet 故障排查指南]] |  | troubleshooting, guide |  |
| 13 | [[19-故障诊断/04-高级排障/02-node-components/02-kube-proxy-troubleshooting.md|kube-proxy 故障排查指南]] |  | troubleshooting, guide |  |
| 14 | [[19-故障诊断/04-高级排障/02-node-components/03-container-runtime-troubleshooting.md|容器运行时故障排查指南]] |  | troubleshooting, guide |  |
| 15 | [[19-故障诊断/04-高级排障/02-node-components/04-node-troubleshooting.md|节点问题专项排查指南]] |  | troubleshooting, guide |  |
| 16 | [[19-故障诊断/04-高级排障/02-node-components/05-image-registry-troubleshooting.md|镜像与镜像仓库故障排查指南]] |  | troubleshooting, guide |  |
| 17 | [[19-故障诊断/04-高级排障/02-node-components/06-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查指南]] |  | troubleshooting, guide |  |
| 18 | [[19-故障诊断/04-高级排障/03-networking/01-cni-troubleshooting.md|CNI 网络插件故障排查指南]] |  | troubleshooting, guide |  |
| 19 | [[19-故障诊断/04-高级排障/03-networking/02-dns-troubleshooting.md|CoreDNS/DNS 故障排查指南]] |  | troubleshooting, guide |  |
| 20 | [[19-故障诊断/04-高级排障/03-networking/03-service-ingress-troubleshooting.md|Service 与 Ingress 故障排查指南]] |  | troubleshooting, guide |  |
| 21 | [[19-故障诊断/04-高级排障/03-networking/04-networkpolicy-troubleshooting.md|NetworkPolicy 深度排查与零信任安全治理指南]] |  | troubleshooting, guide, networking |  |
| 22 | [[19-故障诊断/04-高级排障/03-networking/05-service-mesh-istio-troubleshooting.md|Service Mesh (Istio) 深度排查与性能调优指南]] |  | troubleshooting, guide |  |
| 23 | [[19-故障诊断/04-高级排障/03-networking/06-gateway-api-troubleshooting.md|Gateway API 深度排查与下一代流量治理指南]] |  | troubleshooting, guide |  |
| 24 | [[19-故障诊断/04-高级排障/03-networking/07-terway-troubleshooting.md|Terway（阿里云 CNI）[[ts-networking]]指南]] |  | troubleshooting, guide |  |
| 25 | [[19-故障诊断/04-高级排障/03-networking/08-flannel-troubleshooting.md|Flannel 网络故障排查指南]] |  | troubleshooting, guide |  |
| 26 | [[19-故障诊断/04-高级排障/03-networking/09-higress-troubleshooting.md|Higress 网关故障排查指南]] |  | troubleshooting, guide |  |
| 27 | [[19-故障诊断/04-高级排障/03-networking/09-nginx-ingress-troubleshooting.md|nginx-ingress-controller 故障排查指南]] |  | troubleshooting, guide |  |
| 28 | [[19-故障诊断/04-高级排障/04-storage/01-pv-pvc-troubleshooting.md|PV/PVC 存储深度排查与持久化治理指南]] |  | troubleshooting, guide |  |
| 29 | [[19-故障诊断/04-高级排障/04-storage/02-csi-troubleshooting.md|CSI 存储驱动深度排查与架构优化指南]] |  | troubleshooting, guide |  |
| 30 | [[19-故障诊断/04-高级排障/04-storage/03-snapshot-backup-troubleshooting.md|CSI 快照与卷备份故障排查指南]] |  | troubleshooting, guide, backup-restore |  |
| 31 | [[19-故障诊断/04-高级排障/04-storage/04-storage-performance-troubleshooting.md|存储 I/O 性能故障排查指南]] |  | troubleshooting, guide, storage |  |
| 32 | [[19-故障诊断/04-高级排障/04-storage/05-storageclass-troubleshooting.md|StorageClass 配置与动态供给故障排查指南]] |  | troubleshooting, guide, storage |  |
| 33 | [[19-故障诊断/04-高级排障/05-workloads/01-pod-troubleshooting.md|Pod 故障排查与运行机制深度指南]] |  | troubleshooting, guide |  |
| 34 | [[19-故障诊断/04-高级排障/05-workloads/02-deployment-troubleshooting.md|Deployment 故障排查指南]] |  | troubleshooting, guide, deployment |  |
| 35 | [[19-故障诊断/04-高级排障/05-workloads/03-statefulset-troubleshooting.md|StatefulSet 故障排查指南]] |  | troubleshooting, guide |  |
| 36 | [[19-故障诊断/04-高级排障/05-workloads/04-daemonset-troubleshooting.md|DaemonSet 故障排查指南]] |  | troubleshooting, guide |  |
| 37 | [[19-故障诊断/04-高级排障/05-workloads/05-job-cronjob-troubleshooting.md|Job 与 CronJob 故障排查指南]] |  | troubleshooting, guide |  |
| 38 | [[19-故障诊断/04-高级排障/05-workloads/06-configmap-secret-troubleshooting.md|ConfigMap 与 Secret 故障排查指南]] |  | troubleshooting, guide, configuration |  |
| 39 | [[19-故障诊断/04-高级排障/06-security-auth/01-rbac-troubleshooting.md|RBAC 与认证故障排查指南]] |  | troubleshooting, guide, rbac |  |
| 40 | [[19-故障诊断/04-高级排障/06-security-auth/02-certificate-troubleshooting.md|Kubernetes 证书故障排查指南]] |  | troubleshooting, guide |  |
| 41 | [[19-故障诊断/04-高级排障/06-security-auth/03-pod-security-troubleshooting.md|Pod 安全与 SecurityContext 故障排查指南]] |  | troubleshooting, guide, security |  |
| 42 | [[19-故障诊断/04-高级排障/06-security-auth/04-audit-logging-troubleshooting.md|审计日志故障排查指南]] |  | troubleshooting, guide, compliance |  |
| 43 | [[19-故障诊断/04-高级排障/07-resources-scheduling/01-resources-quota-troubleshooting.md|资源与调度故障排查指南]] |  | troubleshooting, guide |  |
| 44 | [[19-故障诊断/04-高级排障/07-resources-scheduling/02-autoscaling-troubleshooting.md|HPA 与 VPA 自动扩缩容故障排查指南]] |  | troubleshooting, guide |  |
| 45 | [[19-故障诊断/04-高级排障/07-resources-scheduling/03-cluster-autoscaler-troubleshooting.md|Cluster Autoscaler 节点自动扩缩容故障排查指南]] |  | troubleshooting, guide |  |
| 46 | [[19-故障诊断/04-高级排障/07-resources-scheduling/04-pdb-troubleshooting.md|PodDisruptionBudget (PDB) 故障排查指南]] |  | troubleshooting, guide |  |
| 47 | [[19-故障诊断/04-高级排障/08-cluster-operations/01-cluster-maintenance-troubleshooting.md|集群运维与升级故障排查指南]] |  | troubleshooting, guide |  |
| 48 | [[19-故障诊断/04-高级排障/08-cluster-operations/02-logging-monitoring-troubleshooting.md|日志与监控故障排查指南]] |  | troubleshooting, guide, monitoring |  |
| 49 | [[19-故障诊断/04-高级排障/08-cluster-operations/03-helm-troubleshooting.md|Helm 部署故障排查指南]] |  | troubleshooting, guide |  |
| 50 | [[19-故障诊断/04-高级排障/08-cluster-operations/04-ha-disaster-recovery-troubleshooting.md|集群高可用与灾备故障排查指南]] |  | troubleshooting, guide |  |
| ... | 共 71 篇文档 | | | |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 71 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*

## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|kubernetes]]
- [[23-实体/02-K8s核心组件/cni.md|cni]]
- [[23-实体/02-K8s核心组件/coredns.md|coredns]]
- Wiki Lint Report — 2026-05-21 — Cross-reference
- [[23-实体/15-参考与索引/release-notes-storage.md|发布说明索引 — 存储]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-observability.md|发布说明索引 — 可观测性]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-kubernetes.md|发布说明索引 — Kubernetes]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-knowledge-map.md|Kubernetes Knowledge Map]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-cicd-gitops.md|发布说明索引 — CI/CD 与 GitOps]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[23-实体/15-参考与索引/release-notes-core-deps.md|发布说明索引 — 核心依赖]] — Cross-reference
- [[23-实体/15-参考与索引/k8s-difficulty-index.md|Kubernetes Difficulty Index]] — Cross-reference
- 网络 MOC — Cross-reference
- [[05-网络/01-K8s网络核心/02-cni-architecture-fundamentals.md|CNI 架构与核心原理]] — Cross-reference
- [[09-可观测性/01-总览/01-observability-architecture-overview.md|Kubernetes 可观测性架构体系]] — Cross-reference
- [[15-AI基础设施/01-基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[15-AI基础设施/01-基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- 发布变更 MOC — Cross-reference
- [[01-集群基础/05-kubectl/05-kubectl-commands-reference.md|kubectl 命令完整参考]] — Cross-reference
- [[01-集群基础/01-架构总览/02-core-components-deep-dive.md|Kubernetes 核心组件深度剖析]] — Cross-reference
- [[06-存储/01-K8s存储/02-pv-architecture-fundamentals.md|PV/PVC 核心概念与企业级实践]] — Cross-reference
- [[06-存储/01-K8s存储/01-storage-architecture-overview.md|存储架构概览与核心组件]] — Cross-reference

## 高级排障场景分类

| 分类 | 典型问题 | 难度 | 关键工具 |
|------|----------|------|----------|
| 控制平面 | apiserver 崩溃、etcd 数据损坏、scheduler 异常 | L3-L4 | etcdctl、kube-apiserver 日志 |
| 证书与认证 | 证书过期、RBAC 配置错误、Webhook 拒绝 | L2-L3 | openssl、kubectl auth can-i |
| 网络深层 | CNI 插件崩溃、iptables 规则泄漏、MTU 不匹配 | L3 | tcpdump、iptables-save、cilium status |
| 存储故障 | PV 挂载失败、CSI 驱动异常、数据卷损坏 | L2-L3 | csi-provisioner 日志、fsck |
| 升级失败 | 集群升级中断、API 废弃、组件不兼容 | L3 | kubeadm upgrade plan、API deprecation check |

## 排障优先级矩阵

```
影响范围
│  高 │ P0-控制平面  │ P1-网络深层  │
│     │ (etcd/apiserver) │ (CNI崩溃)     │
│  中 │ P1-证书过期  │ P2-存储异常  │
│     │ (全局影响)   │ (部分Pod)    │
│  低 │ P2-Webhook  │ P3-升级规划  │
│     │ (单个准入)   │ (计划内)     │
└─────┴────────────┴────────────┘
      高紧迫度        低紧迫度
```


<!-- risk-assessed -->
