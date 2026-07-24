---
title: CNI (Container Network Interface)
description: '## 概述'
summary: 'CNI (Container Network Interface) 是一个定义容器网络配置的规范和库，用于在 Linux 容器中配置网络接口。它是 Kubernetes 和其他容器编排平台的网络基础，提供了插件化的网络解决方案。'
category: entities
tags:
- k8s
- cncf
- networking
- cni
- cilium
- calico
- containerd
- cri-o
- networkpolicy
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNI (Container Network Interface) 是什么
- 如何 CNI (Container Network Interface)
trigger_keywords:
- CNI
- Container
- Network
- Interface
prerequisites:
- kubectl-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# CNI (Container Network Interface)

> **CNCF 状态**: Incubating | **类别**: Networking | **主要语言**: Go

## 概述

CNI (Container Network Interface) 是一个定义容器网络配置的规范和库，用于在 Linux 容器中配置网络接口。它是 Kubernetes 和其他容器编排平台的网络基础，提供了插件化的网络解决方案。

## 核心能力

- **简单规范**: 清晰的 JSON 配置和二进制插件接口
- **可组合**: 支持多插件链式调用
- **容器运行时无关**: 支持 containerd、CRI-O、Podman 等
- **丰富插件生态**: 官方和第三方插件覆盖各种网络场景
- **版本兼容**: 规范版本向后兼容

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **版本兼容**: 使用与 Kubernetes 版本匹配的 CNI 规范
- **配置优先级**: 配置文件按字典序加载，用数字前缀控制顺序
- **IPAM 选择**: 生产环境推荐使用 host-local 或 Calico IPAM
- **网络策略**: 结合 NetworkPolicy 实现细粒度访问控制
- **监控**: 监控网络插件健康状态和 IP 池使用情况

## 架构定位

在 CNCF 生态中，cni 属于 **Networking** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 参考链接

- [[cilium]]
- [[containerd]]
- [[概念/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[实体/networkpolicy.md|networkpolicy]]
- [[概念/container-runtime-comparison.md|container-runtime-comparison]]

## Related

- [[slimtoolkit]] — SlimToolkit
- [[实体/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd
- [[cri-o]] — CRI-O

- 39-csi-cni-version-matrix
- 27-cni-troubleshooting-optimization
- 03-cni-plugins-comparison
- CNI 架构与核心原理
- [[故障诊断/核心排障/03-networking-cni-troubleshooting.md|03-networking-cni-troubleshooting]]
- 03-cilium-cni-architecture
- [[故障诊断/高级排障/03-networking/01-cni-troubleshooting.md|01-cni-troubleshooting]]
- cni
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.7
- RELEASE-NOTES-0.6
- RELEASE-NOTES-1.6
- RELEASE-NOTES-0.7
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- Wiki 全量知识库摘要 — 2026-05-21 — Cross-reference
- [[实体/linux-sysctl-reference.md|Linux Sysctl Reference for Kubernetes]] — Cross-reference
- [[实体/networking-terms.md|K8s 网络术语参考]] — Cross-reference
- [[实体/k8s-workloads-domain-guide.md|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[实体/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- [[实体/k8s-structured-troubleshooting.md|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[实体/k8s-architecture-fundamentals.md|K8s 架构基础与核心组件原理]] — Cross-reference
- [[实体/root-terms.md|K8s Root术语参考]] — Cross-reference
- [[实体/k8s-control-plane-deep-dive.md|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[实体/k8s-cloud-provider-comparison.md|云厂商托管 Kubernetes 服务全景对比（13 家）]] — Cross-reference
- [[实体/k8s-networking-ecosystem.md|网络体系：CNI、Service、Ingress、Gateway API 与多集群网络]] — Cross-reference
- [[实体/k8s-cluster-delete.md|Kubernetes 集群删除操作指南]] — Cross-reference
- [[实体/release-notes-cli-tools.md|发布说明索引 — CLI 工具]] — Cross-reference
- [[实体/k8s-node-create.md|Kubernetes 节点管理操作指南]] — Cross-reference
- [[实体/platform-engineering-terms.md|K8s 平台工程术语参考]] — Cross-reference
- [[实体/version-upgrade-guide.md|版本升级指南]] — Cross-reference
- [[实体/k8s-networking-domain-guide.md|Kubernetes Networking Domain Guide]] — Cross-reference
- [[概念/kubeadm-cluster-operations.md|kubeadm 集群运维全景]] — Cross-reference
- [[概念/IaC × 多集群管理.md|基础设施即代码 x 多集群管理]] — Cross-reference
- [[故障诊断/Production Troubleshooting Playbook.md|Production Troubleshooting Playbook]] — Cross-reference
- [[概念/k8s-mttr-benchmark.md|K8s 问题分布与 MTTR 基准]] — Cross-reference
- [[概念/cni-networking-model.md|CNI 网络模型与插件对比]] — Cross-reference
- [[元数据/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[概念/node-lifecycle-management.md|节点生命周期管理]] — Cross-reference
- [[概念/Kubernetes Core Concepts.md|Kubernetes Core Concepts]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[技能/网络/networkpolicy/最佳实践/k8s-network-security-guide.md|Kubernetes 网络安全最佳实践]] — Cross-reference
- [[技能/节点/node/诊断排障/ts-node-components.md|节点组件故障排查]] — Cross-reference
- [[技能/工作负载/daemonset/培训/learn-13-daemonset-basics.md|第13课：DaemonSet 与节点守护]] — Cross-reference
- [[技能/集群运维/kubeadm/kubeadm-cluster-lifecycle.md|kubeadm 集群创建生命周期]] — Cross-reference
- [[技能/网络/networkpolicy/skill-20-networkpolicy-connectivity.md|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[技能/工作负载/statefulset/skill-21-statefulset-failure.md|StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation]] — Cross-reference
- [[技能/网络/networkpolicy/networkpolicy-fta.md|NetworkPolicy 异常故障树分析]] — Cross-reference
- [[技能/集群运维/kubeadm/kubeadm-cluster-deletion.md|kubeadm 集群删除操作]] — Cross-reference
- [[技能/工作负载/pod/方法论/skill-reference-root-cause-catalog.md|Root Cause Catalog]] — Cross-reference
- [[技能/工作负载/deployment/deployment-workload-selection.md|工作负载控制器选型]] — Cross-reference
- [[技能/网络/cni/最佳实践/k8s-network-configuration-guide.md|Kubernetes 网络配置最佳实践]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-04-debug-tools-setup.md|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-oncall-quick-qa.md|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-12-common-problems.md|第十课：常见问题排查]] — Cross-reference
- [[技能/工作负载/pod/培训/learn-analogy-dictionary.md|K8S 概念类比词典]] — Cross-reference
- [[实体/cncf-networking.md|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/flannel-index.md|Flannel 知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]


<!-- risk-assessed -->
