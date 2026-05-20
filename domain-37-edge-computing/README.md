---
title: 'Domain 37: 边缘计算 (Edge Computing)'
description: 边缘计算 (Edge Computing) 将计算和数据存储带到更接近数据生成源的位置，以提高响应速度并节省带宽。本领域深入探讨 Kubernetes 在边缘场景的应用，涵盖 KubeEdge、OpenYurt、SuperEdge
  等云原生边缘框架，以及边缘 AI 推理、离线自治、边缘安全等核心技术。
category: edge-computing
tags:
- k8s
- edge
- iot
- kubeedge
- minio
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 边缘计算工程师
- SRE
- IoT 工程师
estimated_read_time: 5min
intent_queries:
- 'Domain 37: 边缘计算 (Edge Computing) 是什么'
- '如何 Domain 37: 边缘计算 (Edge Computing)'
- Kubernetes 37 edge computing 最佳实践
trigger_keywords:
- Domain
- '37:'
- 边缘计算
- Edge
- Computing
- edge
- computing
---

# Domain 37: 边缘计算 (Edge Computing)

> **适用范围**: 边缘 Kubernetes、IoT、边缘 AI | **维护状态**: 持续更新中 | **专家级别**: ⭐⭐⭐⭐⭐ | **更新时间**: 2026-03-04

## 📋 领域概览

边缘计算 (Edge Computing) 将计算和数据存储带到更接近数据生成源的位置，以提高响应速度并节省带宽。本领域深入探讨 Kubernetes 在边缘场景的应用，涵盖 KubeEdge、OpenYurt、SuperEdge 等云原生边缘框架，以及边缘 AI 推理、离线自治、边缘安全等核心技术。

## 📚 文档目录

### 🎯 边缘计算基础 (01-02)
- **[01-边缘计算架构概述](./01-edge-computing-architecture.md)** - 边缘计算定义、架构模式、部署拓扑
- **[02-云边协同设计模式](./02-cloud-edge-collaboration.md)** - 云边通信、数据同步、状态管理

### 🌐 KubeEdge 深度实践 (03-04)
- **[03-KubeEdge架构与部署](./03-kubeedge-architecture-deployment.md)** - CloudCore/EdgeCore 架构、部署配置
- **[04-KubeEdge设备管理与边缘应用](./04-kubeedge-device-edge-apps.md)** - DeviceModel、DeviceTwin、边缘应用部署

### 🔧 其他边缘框架 (05-06)
- **[05-OpenYurt边缘方案](./05-openyurt-architecture.md)** - YurtHub、YurtTunnel、NodePool 设计
- **[06-SuperEdge架构实践](./06-superedge-architecture.md)** - SuperEdge 组件、边缘自治、分布式健康检查

### ⚡ 边缘 AI 与存储 (07-08)
- **[07-边缘AI推理与联邦学习](./07-edge-ai-inference-federated-learning.md)** - ONNX/TFLite、边缘推理、联邦学习架构
- **[08-边缘存储与网络](./08-edge-storage-network.md)** - 边缘存储方案、弱网络优化、断网续传

### 🔒 边缘安全与场景 (09-10)
- **[09-边缘安全架构](./09-edge-security.md)** - 边缘身份、通信安全、设备安全
- **[10-边缘场景案例](./10-edge-use-cases.md)** - 智慧工厂、智慧城市、车联网、零售

## 🎯 学习路径建议

### 🔰 边缘计算入门
1. **01-边缘计算架构** → 理解边缘计算核心概念
2. **02-云边协同** → 掌握云边交互模式
3. **10-边缘场景** → 了解实际应用案例

### ⭐ KubeEdge 工程师
1. **03-KubeEdge架构** → 部署与配置 KubeEdge
2. **04-设备管理** → 边缘设备与应用管理
3. **08-边缘存储网络** → 边缘基础设施优化

### 🤖 边缘 AI 工程师
1. **07-边缘AI推理** → 边缘模型部署与优化
2. **09-边缘安全** → 安全架构设计
3. **10-边缘场景** → 行业最佳实践

## 📊 技术深度对比

| 文档 | 技术深度 | 实践价值 | 适用场景 | 复杂度 |
|------|----------|----------|----------|--------|
| 01-边缘架构 | ⭐⭐⭐⭐ | 很高 | 架构设计 | 中 |
| 02-云边协同 | ⭐⭐⭐⭐ | 高 | 系统设计 | 中高 |
| 03-KubeEdge | ⭐⭐⭐⭐⭐ | 很高 | 边缘部署 | 中高 |
| 04-设备管理 | ⭐⭐⭐⭐ | 很高 | IoT 集成 | 中 |
| 05-OpenYurt | ⭐⭐⭐⭐⭐ | 高 | 边缘方案对比 | 中高 |
| 06-SuperEdge | ⭐⭐⭐⭐ | 高 | 边缘方案对比 | 中高 |
| 07-边缘AI | ⭐⭐⭐⭐⭐ | 很高 | AI 推理 | 高 |
| 08-存储网络 | ⭐⭐⭐⭐ | 高 | 基础设施 | 中高 |
| 09-边缘安全 | ⭐⭐⭐⭐⭐ | 很高 | 安全架构 | 高 |
| 10-边缘场景 | ⭐⭐⭐⭐ | 很高 | 行业实践 | 中 |

## 🔧 核心技术栈

```bash
# 边缘 Kubernetes 框架
KubeEdge (CNCF Incubating)      # 云边协同框架
OpenYurt (CNCF Sandbox)         # 阿里边缘方案
SuperEdge                       # 腾讯边缘方案
K3s                             # 轻量级 K8s

# 边缘 AI
ONNX Runtime                    # 跨平台推理
TensorFlow Lite                 # 移动端推理
OpenVINO                        # Intel 边缘推理

# 边缘存储
EdgeFS                          # 边缘分布式存储
MinIO                           # 对象存储
```

## 📚 相关领域链接

- **[Domain-19: 高级论文](../domain-19-papers)** - 边缘计算深度实践
- **[Domain-5: 网络基础](../domain-5-networking)** - 网络架构基础
- **[Domain-11: AI 基础设施](../domain-11-ai-infra)** - AI 基础设施

---
*本文档由云原生技术专家团队维护，内容基于 2026 年边缘计算最新实践。*
