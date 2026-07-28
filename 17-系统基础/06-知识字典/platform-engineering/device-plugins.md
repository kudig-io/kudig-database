---
title: 设备插件
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- daemonset
- gpu
- nvidia
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 设备插件 是什么
- 如何 设备插件
trigger_keywords:
- 设备插件
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 设备插件

## 概述

设备插件（Device Plugins）是 [[kubernetes|Kubernetes]] 提供的一种扩展机制，允许集群支持需要厂商特定设置的设备或资源，例如 GPU、高性能网卡（NIC）、FPGA 或非易失性主内存。该特性自 Kubernetes v1.26 起进入稳定（Stable）状态。

## 核心概念/原理

- **设备插件框架**：Kubernetes 提供统一的设备插件框架，供硬件厂商向 [[kubelet|kubelet]] 发布系统硬件资源，而无需修改 Kubernetes 核心代码。
- **[[grpc|gRPC]] 注册**：设备插件通过 Unix socket 向 kubelet 的 `Registration` gRPC 服务注册，提供资源名称（遵循 `vendor-domain/resourcetype` 扩展资源命名规范，如 `nvidia.com/gpu`）。
- **资源上报**：注册成功后，设备插件将其管理的设备列表发送给 kubelet，kubelet 在节点状态更新中将这些资源通告给 API server。

## 关键机制或特性

- **DevicePlugin 接口**：设备插件需实现 `GetDevicePluginOptions`、`ListAndWatch`、`Allocate`、`GetPreferredAllocation`、`PreStartContainer` 等 gRPC 方法。
- **Allocate 流程**：在创建容器时调用，设备插件执行设备特定的准备工作（如 GPU 清理、QRNG 初始化），并返回容器运行时配置（Annotations、设备节点、环境变量、挂载、CDI 设备名等）。
- **健康监控**：设备插件通过 `ListAndWatch` 持续监控设备健康状态；设备变为不健康时，kubelet 会下调该资源的可分配数量（allocatable），但不会更改容量（capacity）。
- **PodResources API**：kubelet 提供 `PodResourcesLister` gRPC 服务，支持监控代理发现节点上正在使用的设备及元数据（Pod 名、命名空间、容器名等）。
- **Topology Manager 集成**：自 v1.27 起稳定支持，设备插件可通过 `TopologyInfo` 结构体上报设备的 NUMA 亲和性，帮助 Topology Manager 做出拓扑对齐的资源分配决策。
- **ResourceHealthStatus**：自 v1.31（Alpha，默认关闭）起，启用 `ResourceHealthStatus` 特性门控后，Pod 状态中会出现 `allocatedResourcesStatus` 字段，报告容器分配设备的健康信息。

## 使用场景

- 在集群中调度并使用 GPU 进行深度学习训练或推理。
- 使用 FPGA 加速特定计算任务。
- 使用 RDMA/InfiniBand 网卡实现高性能网络通信。
- 暴露其他需要厂商初始化的专用硬件资源。

## 最佳实践/注意事项

- 建议将设备插件以 DaemonSet 方式部署，确保节点扩展的自动化和故障恢复。
- 设备插件和监控代理均需要特权访问 `/var/lib/kubelet/device-plugins` 和 `/var/lib/kubelet/pod-resources`，部署时需提供相应的安全上下文。
- 设备插件应能检测 kubelet 重启事件（Unix socket 被删除）并自动重新注册。
- 扩展资源仅支持整型请求，不能被超额分配（overcommit），且设备不能在容器间共享。
- 升级 Kubernetes 前，建议设备插件同时支持新旧两个 API 版本，以确保升级期间设备分配不中断。

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| 节点无扩展资源 | Device Plugin 未注册 | `kubectl describe node` 查看 Capacity；检查 device plugin Pod 日志 |
| Pod Pending 请求设备资源 | 设备数量不足 | `kubectl describe node` 查看 Allocatable 中的设备数量 |
| 设备分配后容器启动失败 | 设备挂载或权限问题 | 检查容器日志；确认 /dev 下设备文件存在 |
| kubelet 重启后设备丢失 | Device Plugin 未实现 GetPreferredAllocation | 确认插件支持 kubelet 重启恢复 |

## 生产检查清单

- [ ] Device Plugin DaemonSet 运行正常
- [ ] 节点 Capacity/Allocatable 中可见扩展资源
- [ ] 设备健康检查正确报告
- [ ] kubelet 重启后设备自动恢复

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点设备资源
kubectl describe node <node> | grep -E "nvidia|rdma|fpga"

# 查看 device plugin Pod
kubectl get pods -n kube-system -l app=nvidia-device-plugin
```
## 交叉引用

- [计算、存储和网络扩展](./compute-storage-and-networking-extensions.md) — 扩展总览
- [Network Plugins](./network-plugins.md) — CNI 网络插件

## 参考链接

- https://kubernetes.io/docs/concepts/extend-kubernetes/compute-storage-net/device-plugins/

## Related
- [[21-生态参考/03-领域索引/scheduler-index.md|Scheduler 调度与弹性伸缩知识图谱索引]]


<!-- risk-assessed -->
