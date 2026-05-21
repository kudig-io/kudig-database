---
title: CNI (Container Network Interface)
description: '## 概述'
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

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[concepts/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

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
- [[concepts/cilium-ebpf-networking.md|cilium-ebpf-networking]]
- [[entities/networkpolicy.md|networkpolicy]]
- [[concepts/container-runtime-comparison.md|container-runtime-comparison]]

## Related

- [[slimtoolkit]] — SlimToolkit
- [[entities/networkpolicy.md|networkpolicy]] — NetworkPolicy
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[containerd]] — containerd
- [[cri-o]] — CRI-O

- [[domain-03-networking-traffic/39-csi-cni-version-matrix.md|39-csi-cni-version-matrix]]
- [[domain-03-networking-traffic/27-cni-troubleshooting-optimization.md|27-cni-troubleshooting-optimization]]
- [[domain-03-networking-traffic/03-cni-plugins-comparison.md|03-cni-plugins-comparison]]
- [[domain-03-networking-traffic/02-cni-architecture-fundamentals.md|CNI 架构与核心原理]]
- [[domain-10-troubleshooting-diagnostics/03-networking-cni-troubleshooting.md|03-networking-cni-troubleshooting]]
- [[domain-03-networking-traffic/03-cilium-cni-architecture.md|03-cilium-cni-architecture]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/01-cni-troubleshooting.md|01-cni-troubleshooting]]
- [[domain-19-landscape-references/incubating/cni/cni.md|cni]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.9.md|RELEASE-NOTES-1.9]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-0.8.md|RELEASE-NOTES-0.8]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.8.md|RELEASE-NOTES-1.8]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-0.9.md|RELEASE-NOTES-0.9]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.3.md|RELEASE-NOTES-1.3]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.7.md|RELEASE-NOTES-1.7]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-0.6.md|RELEASE-NOTES-0.6]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.6.md|RELEASE-NOTES-1.6]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-0.7.md|RELEASE-NOTES-0.7]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.2.md|RELEASE-NOTES-1.2]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.5.md|RELEASE-NOTES-1.5]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.1.md|RELEASE-NOTES-1.1]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.0.md|RELEASE-NOTES-1.0]]
- [[domain-19-landscape-references/topic-release-notes/networking/cni-plugins/RELEASE-NOTES-1.4.md|RELEASE-NOTES-1.4]]
- [[journal/digest-2026-05-21-full|Wiki 全量知识库摘要 — 2026-05-21]] — Cross-reference
- [[references/linux-sysctl-reference|Linux Sysctl Reference for Kubernetes]] — Cross-reference
- [[references/networking-terms|K8s 网络术语参考]] — Cross-reference
- [[references/k8s-workloads-domain-guide|Kubernetes Workloads Domain Guide]] — Cross-reference
- [[references/release-notes-networking|发布说明索引 — 网络]] — Cross-reference
- [[references/k8s-structured-troubleshooting|结构化排障方法论：配置优先、全组件排障指南]] — Cross-reference
- [[references/k8s-architecture-fundamentals|K8s 架构基础与核心组件原理]] — Cross-reference
- [[references/root-terms|K8s Root术语参考]] — Cross-reference
- [[references/k8s-control-plane-deep-dive|控制平面深度剖析：API Server、Scheduler、KCM 与 CRI/CSI/CNI]] — Cross-reference
- [[references/k8s-cloud-provider-comparison|云厂商托管 Kubernetes 服务全景对比（13 家）]] — Cross-reference
- [[references/k8s-networking-ecosystem|网络体系：CNI、Service、Ingress、Gateway API 与多集群网络]] — Cross-reference
- [[references/k8s-cluster-delete|Kubernetes 集群删除操作指南]] — Cross-reference
- [[references/release-notes-cli-tools|发布说明索引 — CLI 工具]] — Cross-reference
- [[references/k8s-node-create|Kubernetes 节点管理操作指南]] — Cross-reference
- [[references/platform-engineering-terms|K8s 平台工程术语参考]] — Cross-reference
- [[references/version-upgrade-guide|版本升级指南]] — Cross-reference
- [[references/k8s-networking-domain-guide|Kubernetes Networking Domain Guide]] — Cross-reference
- [[synthesis/kubeadm-cluster-operations|kubeadm 集群运维全景]] — Cross-reference
- [[synthesis/IaC x 多集群管理|基础设施即代码 x 多集群管理]] — Cross-reference
- [[synthesis/Production Troubleshooting Playbook|Production Troubleshooting Playbook]] — Cross-reference
- [[synthesis/K8s 故障分布与 MTTR 基准|K8s 故障分布与 MTTR 基准]] — Cross-reference
- [[concepts/cni-networking-model|CNI 网络模型与插件对比]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[concepts/node-lifecycle-management|节点生命周期管理]] — Cross-reference
- [[concepts/Kubernetes Core Concepts|Kubernetes Core Concepts]] — Cross-reference
- [[skills/learn-decision-tree-mermaid|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/k8s-network-security-guide|Kubernetes 网络安全最佳实践]] — Cross-reference
- [[skills/ts-node-components|节点组件故障排查]] — Cross-reference
- [[skills/learn-13-daemonset-basics|第13课：DaemonSet 与节点守护]] — Cross-reference
- [[skills/kubeadm-cluster-lifecycle|kubeadm 集群创建生命周期]] — Cross-reference
- [[skills/skill-20-networkpolicy-connectivity|NetworkPolicy 连通性故障诊断 / NetworkPolicy Connectivity Troubleshooting]] — Cross-reference
- [[skills/skill-21-statefulset-failure|StatefulSet 故障诊断与修复 / StatefulSet Failure Diagnosis & Remediation]] — Cross-reference
- [[skills/networkpolicy-fta|NetworkPolicy 异常故障树分析]] — Cross-reference
- [[skills/kubeadm-cluster-deletion|kubeadm 集群删除操作]] — Cross-reference
- [[skills/skill-reference-root-cause-catalog|Root Cause Catalog]] — Cross-reference
- [[skills/deployment-workload-selection|工作负载控制器选型]] — Cross-reference
- [[skills/k8s-network-configuration-guide|Kubernetes 网络配置最佳实践]] — Cross-reference
- [[skills/learn-04-debug-tools-setup|Day 4: 调试工具全家桶安装]] — Cross-reference
- [[skills/learn-oncall-quick-qa|工单数字人快速问答 - On-Call 速查]] — Cross-reference
- [[skills/learn-12-common-problems|第十课：常见问题排查]] — Cross-reference
- [[skills/learn-analogy-dictionary|K8S 概念类比词典]] — Cross-reference
- [[entities/cncf-networking|CNCF 网络与服务网格项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/flannel-index|Flannel 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index|Network 网络知识图谱索引]]
