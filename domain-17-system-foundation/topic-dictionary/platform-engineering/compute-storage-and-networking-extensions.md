---
title: 计算、存储与网络扩展
description: '# 计算、存储与网络扩展'
summary: '# 计算、存储与网络扩展'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- daemonset
- operator
- gpu
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 计算、存储与网络扩展 是什么
- 如何 计算、存储与网络扩展
trigger_keywords:
- 计算
- 存储与网络扩展
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
---



# 计算、存储与网络扩展

## 概述

[[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] 提供了多种扩展机制，用于增强集群中节点的能力，或提供连接 Pod 的网络 fabric。这些扩展并非 Kubernetes 核心自带的组件，但能够灵活地支持新硬件、新存储类型以及不同的网络拓扑。

## 核心概念/原理

- **存储插件（CSI / FlexVolume）**：Container Storage Interface（CSI）插件是扩展 Kubernetes 存储能力的标准方式，支持持久化外部存储、临时存储或只读信息接口。FlexVolume 插件自 Kubernetes v1.23 起已弃用，推荐迁移至 CSI。
- **设备插件（Device Plugins）**：允许节点发现除内置资源（如 `cpu`、`memory`）之外的新节点本地设施，并将这些自定义资源暴露给请求的 Pod。
- **网络插件（Network Plugins）**：使 Kubernetes 能够与不同的网络拓扑和技术协同工作。Kubernetes 1.35 兼容 CNI（Container Network Interface）网络插件，必须安装网络插件才能拥有可用的 Pod 网络。

## 关键机制或特性

- **CSI 标准化**：通过统一的接口让存储厂商无需修改 Kubernetes 核心代码即可接入新存储后端。
- **FlexVolume 弃用**：通过 kubelet 调用二进制插件挂载卷，但已被 CSI 取代。
- **CNI 兼容性**：网络插件必须实现 Kubernetes 网络模型，才能为 Pod 提供网络连通性。

## 使用场景

- 需要接入新型存储硬件或云存储服务时，使用 CSI 插件扩展存储能力。
- 需要在集群中使用 GPU、FPGA、高性能网卡等专用硬件时，使用设备插件。
- 需要支持 Overlay 网络、VPC 网络或其他特定网络方案时，使用 CNI 网络插件。

## 最佳实践/注意事项

- 新部署的存储扩展优先选择 CSI，避免使用已弃用的 FlexVolume。
- 确保所选 CNI 插件与集群版本兼容，并满足 Kubernetes 网络模型要求。
- 设备插件和网络插件通常需要特权访问节点资源，部署时需注意安全配置。

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Device Plugin 注册失败 | gRPC socket 路径不正确 | 检查 `/var/lib/kubelet/device-plugins/` 目录下的 socket 文件 |
| CSI 驱动未被识别 | CSIDriver 对象未创建 | `kubectl get csidrivers` 确认驱动已注册 |
| CNI 插件初始化失败 | CNI 二进制文件缺失 | 检查 `/opt/cni/bin/` 和 `/etc/cni/net.d/` |

## 生产检查清单

- [ ] Device Plugin 注册正常，`kubectl describe node` 可见扩展资源
- [ ] CSI 驱动 Pod 运行正常（controller + node DaemonSet）
- [ ] CNI 插件正确配置且所有节点 Ready
- [ ] 使用 CSI 替代 in-tree 存储插件

## 命令快速参考

```bash
# 查看节点扩展资源
kubectl describe node <node> | grep -A 10 "Allocatable"

# 查看 CSI 驱动
kubectl get csidrivers

# 查看 Device Plugin socket
ls /var/lib/kubelet/device-plugins/
```

## 交叉引用

- [Device Plugins](./device-plugins.md) — 设备插件详解
- [Network Plugins](./network-plugins.md) — CNI 插件详解
- [Operator 模式](./operator-pattern.md) — 扩展 Kubernetes 的另一种方式

## 参考链接

- https://kubernetes.io/docs/concepts/extend-kubernetes/compute-storage-net/

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/admission-webhook-good-practices.md|Admission Webhook 最佳实践]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/api-group.md|API 组]]
- [[domain-17-system-foundation/topic-dictionary/platform-engineering/api-priority-and-fairness.md|API 优先级与公平性（API Priority and Fairness）]]
