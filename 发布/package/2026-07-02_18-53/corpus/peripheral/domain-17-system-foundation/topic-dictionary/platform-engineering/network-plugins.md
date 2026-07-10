---
title: 网络插件
description: '# 网络插件'
summary: '# 网络插件'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- cilium
- flannel
- calico
- containerd
- cri-o
- ingress
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 网络插件 是什么
- 如何 网络插件
trigger_keywords:
- 网络插件
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- cilium-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 网络插件

## 概述

[[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 允许使用 Container Network Interface（CNI）插件来实现集群网络。CNI 插件是实现 Kubernetes 网络模型的必要组件，负责为 Pod 分配 IP、建立网络连通性，并支持网络策略、端口映射等高级功能。

## 核心概念/原理

- **CNI 插件**：Kubernetes 从 1.3 到最新的 1.35 均支持 CNI 插件。必须使用与集群兼容且满足需求的 CNI 插件。
- **兼容性要求**：CNI 插件需兼容 CNI 规范 v0.4.0 或更高版本。Kubernetes 项目推荐使用兼容 v1.0.0 规范的插件。
- **容器运行时职责**：容器运行时（如 [[containerd|containerd]]、CRI-O）负责加载 CNI 插件。自 Kubernetes 1.24 起，[[kubelet|kubelet]] 不再直接管理 CNI（`cni-bin-dir` 和 `network-plugin` 参数已移除）。

## 关键机制或特性

- **Loopback CNI**：除主网络插件外，容器运行时还需为每个沙箱（Pod）提供 `lo` 回环接口，可通过 CNI loopback 插件或自定义代码实现。
- **hostPort 支持**：可通过官方 `portmap` 插件或自定义端口映射插件实现 `hostPort`。需在 CNI 配置中声明 `portMappings` 能力。
- **流量整形（实验性）**：通过 `bandwidth` 插件支持 Pod 的入站和出站带宽限制。在 Pod 中可通过 `kubernetes.io/ingress-bandwidth` 和 `kubernetes.io/egress-bandwidth` 注解设置带宽。

## 使用场景

- 集群需要实现 Overlay 网络、Underlay 网络或混合网络拓扑时，部署对应的 CNI 插件（如 Calico、[[Cilium|Cilium]]、Flannel）。
- 需要将容器端口暴露到宿主机端口时，启用 `hostPort` 支持。
- 需要对特定 Pod 的网络流量进行限速时，启用 bandwidth 流量整形插件。

## 最佳实践/注意事项

- 选择经过广泛验证、与集群版本兼容的 CNI 插件。
- 自 Kubernetes 1.24 起，CNI 管理职责完全移交给容器运行时，升级集群时需注意相关配置迁移。
- 启用流量整形功能前，确认 CNI 二进制和配置文件均已正确放置（默认 `/opt/cni/bin` 和 `/etc/cni/net.d`）。
- 遇到网络问题时，可参考 Troubleshooting CNI plugin-related errors 进行排查。

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 节点 NotReady | CNI 插件未安装或配置错误 | `kubectl describe node`；检查 `/etc/cni/net.d/` |
| Pod 无法获取 IP | CNI IPAM 地址池耗尽 | 检查 CNI 日志；查看 IPAM 配置 |
| Pod 间网络不通 | CNI 路由配置错误 | `kubectl exec` 测试 ping；检查节点路由表 |

## 生产检查清单

- [ ] CNI 二进制文件部署在 `/opt/cni/bin/`
- [ ] CNI 配置文件在 `/etc/cni/net.d/`
- [ ] IPAM 地址池规划充足
- [ ] kubelet 配置正确的 `--cni-bin-dir` 和 `--cni-conf-dir`

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CNI 配置
ls /etc/cni/net.d/

# 查看 CNI 二进制
ls /opt/cni/bin/

# 查看 Pod CIDR 分配
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}: {.spec.podCIDR}{"\n"}{end}'
```
## 交叉引用

- [计算、存储和网络扩展](./compute-storage-and-networking-extensions.md) — 扩展总览
- [Device Plugins](./device-plugins.md) — 设备级扩展

## 参考链接

- https://kubernetes.io/docs/concepts/extend-kubernetes/compute-storage-net/network-plugins/

## Related

- [[domain-17-system-foundation/知识字典/platform-engineering/admission-webhook-good-practices.md|Admission Webhook 最佳实践]]
- [[domain-17-system-foundation/知识字典/platform-engineering/api-group.md|API 组]]
- [[domain-17-system-foundation/知识字典/platform-engineering/api-priority-and-fairness.md|API 优先级与公平性（API Priority and Fairness）]]


<!-- risk-assessed -->
