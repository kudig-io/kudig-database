---
title: Akri 边缘设备发现
description: 'Akri 是微软开源的 CNCF Sandbox 项目，在 Kubernetes 上自动发现和暴露边缘设备（摄像头/GPU/USB 等），将异构硬件资源抽象为 ...'
category: dictionary
tags:
- k8s
- glossary
- networking
- edge
- iot
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Akri 边缘设备发现 是什么
- Akri 详解
trigger_keywords:
- Akri 边缘设备发现
- Akri
- dictionary
prerequisites:
- kubernetes
created: 2026-06
---

# Akri 边缘设备发现（Akri）

## 概述

Akri 是微软开源的 CNCF Sandbox 项目，在 Kubernetes 上自动发现和暴露边缘设备（摄像头/GPU/USB 等），将异构硬件资源抽象为 K8s 可调度的资源。

## 核心概念/原理

- **设备发现**：自动发现连接到节点的边缘设备
- **CNCF Sandbox**：微软主导
- **K8s 资源**：将设备暴露为 K8s 扩展资源
- **边缘优化**：专为 IoT/边缘计算设计

## 关键机制或特性

- Configuration CRD 定义设备发现规则
- Instance CRD 表示发现的设备实例
- Discovery Handler（ONVIF/OPC-UA/uDev 等）
- 设备自动调度和绑定 Pod
- 设备健康检查
- Prometheus 指标
- 自定义 Discovery Handler

## 使用场景与最佳实践

- IoT 设备的 K8s 管理
- 边缘节点的硬件资源发现
- 智能摄像头的 AI 推理
- GPU/加速器的自动分配
- 工业设备的容器化接入

## 参考链接

- https://docs.akri.sh/
- https://github.com/project-akri/akri

## Related

- [[domain-17-system-foundation/topic-dictionary/platform-engineering/kubeedge.md|KubeEdge]]
- [[domain-17-system-foundation/topic-dictionary/scheduling/hami.md|HAMi]]
- [[domain-17-system-foundation/topic-dictionary/specialized-workloads/openyurt.md|OpenYurt]]
