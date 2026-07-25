---
title: 安装插件（Installing Addons）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- prometheus
- cilium
- flannel
- calico
- coredns
- helm
- daemonset
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 安装插件（Installing Addons） 是什么
- 如何 安装插件（Installing Addons）
trigger_keywords:
- 安装插件
- Installing
- Addons
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 安装插件（Installing Addons）

## 概述

插件（Addons）用于扩展 [[Kubernetes|Kubernetes]] 的功能。Kubernetes 本身不提供原生的完整集群功能（如 DNS、网络、仪表板等），而是通过插件生态来补充这些能力。本文档列出了 Kubernetes 官方文档中提到的一些可用插件，并提供其安装说明的链接。

## 核心概念/原理

- **Addons 是第三方项目**：大多数插件不由 Kubernetes 核心项目直接维护，但它们是运行生产级集群所必需或强烈推荐的组件。
- **功能扩展领域**：包括网络与网络策略、服务发现、可视化与控制、基础设施、可观测性等。
- **安装方式多样**：不同插件有不同的安装方法，通常通过 [[DaemonSet|DaemonSet]]、Deployment、[[Helm|Helm]] chart 或运营商（operator）部署。

## 关键机制或特性

### 网络与网络策略

| 插件 | 说明 |
|------|------|
| ACI | Cisco ACI 集成容器网络与网络安全 |
| [[Antrea|Antrea]] | 基于 Open vSwitch 的 L3/4 网络与安全服务（CNCF Sandbox） |
| Calico | 网络与网络策略提供者，支持多种网络模式 |
| Canal | 结合 Flannel 和 Calico，提供网络与网络策略 |
| Cilium | 基于 eBPF 的网络、可观测性和安全解决方案（CNCF Graduated） |
| CNI-Genie | 支持多种 CNI 插件无缝切换（CNCF Sandbox） |
| Contiv | 提供可配置的网络（L3 BGP、VXLAN overlay、L2、Cisco-SDN/ACI） |
| Contrail | 基于 Tungsten Fabric 的多云网络虚拟化与策略管理平台 |
| Flannel | 简单的 overlay 网络提供者 |
| Gateway API | SIG Network 社区管理的开源服务网络 API |
| Knitter | 支持 Kubernetes Pod 多网络接口的插件 |
| kube-router | 基于 BGP、IPVS/nftables 的 Kubernetes 网络一站式方案 |
| Multus | 支持多 CNI 插件的网络方案 |
| OVN-Kubernetes | 基于 OVN 的 Kubernetes 网络提供者 |
| Nodus | 基于 OVN 的 Service Function Chaining (SFC) CNI |
| NSX-T Container Plug-in (NCP) | VMware NSX-T 与 Kubernetes 的集成 |
| Nuage | 提供 Kubernetes Pod 与非 Kubernetes 环境之间的策略驱动网络 |
| Romana | 支持 NetworkPolicy API 的 L3 网络方案 |
| Spiderpool | 面向裸金属、虚拟机、公有云的 underlay 和 RDMA 网络方案 |
| Terway | 基于阿里云 VPC 和 ECS 网络的 CNI 插件套件 |
| Weave Net | 提供网络与网络策略，支持网络分区后继续工作 |

### 服务发现

- **CoreDNS**：灵活、可扩展的 DNS 服务器，通常作为集群内的 Pod DNS 安装。

### 可视化与控制

- **Dashboard**：Kubernetes 的 Web 仪表板界面。
- **Headlamp**：可扩展的 Kubernetes UI，可在集群内部署或作为桌面应用使用。

### 基础设施

- **KubeVirt**：在 Kubernetes 上运行虚拟机，通常用于裸金属集群。
- **Node Problem Detector**：在 Linux 节点上运行，将系统问题报告为 Event 或 Node condition。

### 可观测性

- **kube-state-metrics**：将 Kubernetes 对象状态暴露为 Prometheus 指标。

### 废弃插件

部分插件文档存放在已弃用的 `cluster/addons` 目录中，维护良好的插件应迁移到本文档列表中。

## 使用场景

- **集群网络搭建**：根据环境（云、裸金属、混合云）和需求（overlay、underlay、BGP、eBPF）选择合适的 CNI 插件。
- **服务发现**：所有生产集群几乎都需要安装 CoreDNS 或兼容的 DNS 方案。
- **可视化运维**：安装 Dashboard 或 Headlamp 提供图形化管理界面。
- **虚拟化工作负载**：在需要同时运行容器和虚拟机的场景中使用 KubeVirt。
- **节点健康监控**：使用 Node Problem Detector 提前发现硬件和系统级问题。
- **基于状态的监控告警**：配合 Prometheus 使用 kube-state-metrics 监控对象状态。

## 最佳实践/注意事项

- 插件的选择应基于集群规模、性能需求、安全策略和运维团队的熟悉程度。
- 大多数插件是第三方项目，Kubernetes 官方不对这些项目负责，选择时应评估其社区活跃度、安全更新和支持情况。
- 安装插件前，仔细阅读官方文档和安装指南，确保与当前 Kubernetes 版本兼容。
- 对于网络插件（CNI），一旦选定并在生产环境中大规模部署后，切换成本通常很高，建议在早期充分测试。
- 定期检查已安装插件的版本，及时应用安全补丁和功能更新。
- 在提议向 Kubernetes 官方文档添加新的第三方插件链接时，需先阅读内容指南（content guide）。

## 故障排查

| 症状 | 可能原因 | 排查命令 | 解决方案 |
|------|----------|----------|----------|
| CNI 插件 Pod CrashLoopBackOff | 网络配置冲突或权限不足 | `kubectl logs -n kube-system <cni-pod>` | 检查 CNI 配置文件和 RBAC 权限 |
| CoreDNS 解析失败 | CoreDNS Pod 未就绪或 ConfigMap 错误 | `kubectl get pods -n kube-system -l k8s-app=kube-dns` | 检查 Corefile 配置和上游 DNS |
| Dashboard 无法访问 | Service 类型或 Ingress 未正确配置 | `kubectl get svc -n kubernetes-dashboard` | 确认 NodePort/LoadBalancer/Ingress 配置 |
| 插件版本与 K8s 不兼容 | 安装了与当前 K8s 版本不兼容的插件 | `kubectl version && helm list -A` | 查阅插件兼容性矩阵并升级 |
| Node Problem Detector 无事件 | NPD DaemonSet 未部署或 journald 配置问题 | `kubectl get ds -n kube-system node-problem-detector` | 确认 NPD 有 host 日志访问权限 |
| kube-state-metrics 指标缺失 | RBAC ClusterRole 权限不足 | `kubectl logs -n monitoring kube-state-metrics-*` | 检查 ClusterRole 是否覆盖所有需要的资源类型 |

## 生产检查清单

- [ ] CNI 插件已部署并通过 Pod 网络连通性测试
- [ ] CoreDNS 已部署且 `nslookup kubernetes.default` 正常解析
- [ ] 所有插件版本与当前 Kubernetes 版本兼容
- [ ] 插件的 RBAC 权限遵循最小权限原则
- [ ] 关键插件（CNI、DNS）配置了资源 requests/limits
- [ ] 插件安装使用 Helm Chart 或 GitOps 管理，可重复部署
- [ ] 已评估插件社区活跃度和安全更新频率

## 命令快速参考

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看所有 kube-system 组件状态
kubectl get pods -n kube-system -o wide

# 检查 CNI 插件配置
ls /etc/cni/net.d/

# 测试集群 DNS 解析
kubectl run dns-test --image=busybox:1.36 --rm -it -- nslookup kubernetes.default

# 查看已安装的 Helm releases
helm list -A

# 查看 CoreDNS 配置
kubectl get configmap -n kube-system coredns -o yaml

# 检查 Node Problem Detector 事件
kubectl get events --field-selector source=node-problem-detector
```
## 交叉引用

- [Installing Addons - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/addons/)
- 相关主题：[Network Plugins](../platform-engineering/network-plugins.md) · [Cluster Networking](../networking/cluster-networking.md) · [DNS for Services and Pods](../networking/dns-for-services-and-pods.md)

## 参考链接

- [Installing Addons]()

## Related

- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
