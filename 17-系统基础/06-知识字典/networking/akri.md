---
title: Akri 边缘设备发现
description: Akri 是微软开源的 CNCF Sandbox 项目，在 Kubernetes 上自动发现和暴露边缘设备（摄像头/GPU/USB 等），将异构硬件资源抽象为
  ...
summary: Akri 是微软开源的 CNCF Sandbox 项目，在 Kubernetes 上自动发现和暴露边缘设备（摄像头/GPU/USB 等），将异构硬件资源抽象为
  ...
category: dictionary
tags:
- k8s
- glossary
- networking
- edge
- iot
tier: core
created: 2026-06
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

## 架构深度解析

### 组件架构

```
┌─────────────────────────────────────────────────────────┐
│                     Kubernetes 集群                       │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Akri Agent（每节点 DaemonSet）                    │   │
│  │  - 执行 Discovery Handler 发现本地设备            │   │
│  │  - 为设备创建/管理 Kubernetes 资源（Pod）          │   │
│  │  - 上报设备状态（Prometheus 指标）                │   │
│  └──────────────────────────────────────────────────┘   │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Akri Controller（控制面）                         │   │
│  │  - 管理 Configuration CRD（发现配置）              │   │
│  │  - 将设备注册为 Instance CRD                      │   │
│  │  - 协调为设备创建 Pod（Slot 分配）                 │   │
│  └──────────────────────────────────────────────────┘   │
│                                                         │
│  ┌──────────────────────────────────────────────────┐   │
│  │  Discovery Handlers（可插拔）                      │   │
│  │  - ONVIF（摄像头） / OPC UA（工业设备）            │   │
│  │  - udev（USB/PCI 设备） / 自定义协议               │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### 源码关键路径（project-akri/akri）

| 模块 | 路径 | 职责 |
|------|------|------|
| Agent | `agent/` | 设备发现执行、资源（Pod）创建与管理 |
| Controller | `controller/` | Configuration/Instance CRD 协调循环 |
| 发现协议 | `discovery-handlers/` | ONVIF/OPC UA/udev 等协议实现 |
| API 定义 | `crd/` | Configuration/Instance 自定义资源定义 |
| 设备共享 | `shared/` | Pod 间设备共享机制（如视频流复用） |

### 设备发现与接入流程

1. 管理员创建 `Configuration` CRD 声明发现协议与目标（如 ONVIF 摄像头网段）
2. Controller 将 Configuration 分发到各节点 Agent
3. Agent 执行 Discovery Handler，发现设备并上报（设备 ID、协议信息）
4. Controller 汇总设备创建 `Instance` CRD，并调度 Pod（每设备默认一个 Pod）
5. Agent 在本地创建 Pod 并挂载设备；设备消失时回收资源

## 生产案例

### 案例 1：摄像头批量发现失败

| 时间 | 事件 |
|------|------|
| 08:00 | 新增 50 台 ONVIF 摄像头，Akri 仅发现 12 台 |
| 08:10 | Agent 日志显示大量 ONVIF 探测超时 |
| 08:20 | 排查网络：摄像头与节点跨 VLAN，组播被阻断 |
| 08:30 | 调整 ONVIF 配置为单播 IP 列表，全部发现成功 |

**根因**：ONVIF 默认使用组播（WS-Discovery）探测设备，跨 VLAN 场景组播不可达；未配置单播或中间件转发。

**修复命令**：
```bash
# 查看 Agent 发现日志 🟢 只读
kubectl -n akri logs ds/akri-agent --tail=100 | grep -i onvif
# 查看已发现设备 🟢 只读
kubectl get instances.akri.sh -o wide
# 修改 Configuration 为单播探测（YAML）🟡 中风险
# spec.discoveryDetails.ipAddresses: ["192.168.10.11", "192.168.10.12", ...]
kubectl apply -f akri-configuration.yaml
```

### 案例 2：设备 Pod 频繁重建导致推理服务抖动

**现象**：GPU 推理 Pod 周期性重启，业务告警不断。

**诊断**：udev 发现机制对设备热插拔事件敏感，驱动加载瞬间设备短暂消失触发 Instance 删除与重建；Pod 重建耗时导致推理中断。

**修复**：调整 Configuration 的 `discoveryTimeout` 与 Agent 的 `rebounce` 参数（去抖窗口），过滤瞬态消失；为推理工作负载设置 `Deployment` 而非默认单 Pod，配合 PDB 保障可用性。

## 对比评测

| 维度 | Akri | KubeEdge 设备管理 | 手工 Node Feature Discovery |
|------|------|------------------|----------------------------|
| 设备抽象 | Configuration/Instance CRD | DeviceModel/Device CRD | Node Feature Label |
| 发现协议 | ONVIF/OPC UA/udev 可插拔 | Mapper 自定义 | 系统探测为主 |
| Pod 编排 | 自动为设备创建 Pod | 需额外编排 | 仅打标签 |
| 适用场景 | 摄像头/工业设备接入 | 边缘设备管理 | 节点硬件标注 |

**选型建议**：设备即服务（为每设备自动跑 Pod）选 Akri；边缘设备全生命周期管理选 KubeEdge；仅需节点能力标注选 NFD。

## 故障排查速查

| 症状 | 排查命令 | 常见根因 |
|------|---------|---------|
| 设备未发现 | `kubectl logs ds/akri-agent` | 组播被阻断或协议配置错误 |
| Pod 未创建 | `kubectl get instances.akri.sh` | Controller 未调度或配额不足 |
| 设备抖动 | 查看 udev 事件日志 | 热插拔去抖窗口过短 |
| 指标缺失 | `curl node:8080/metrics` | Prometheus 抓取配置缺失 |

## 生产部署清单

- [ ] 发现协议网络规划：跨 VLAN 场景明确单播/IP 列表方案
- [ ] 为设备 Pod 配置资源限制与健康探针，防止资源争抢
- [ ] 配置去抖参数避免瞬态事件触发重建
- [ ] 设备 Pod 使用 Deployment/PDB 保障稳定性
- [ ] 设备容量规划：每设备 Pod 的资源占用评估

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 设备批量失联或 Pod 风暴 | 立即回滚 Configuration 变更并检查网络 |
| P1 | 发现协议版本升级（ONVIF 2.0 → 2.1） | 预发验证协议兼容性再全量 |
| P2 | 设备规模增长超出 Agent 承载 | 评估 Agent 分片与横向扩展 |

## 面试要点

> 以下 Q&A 覆盖 Akri 面试高频考点，按"概念 → 原理 → 实战"递进。

1. **Q：Akri 的"设备即 Kubernetes 资源"如何理解？**
   A：Akri 把物理设备（摄像头、GPU、工业控制器）抽象为 Instance CRD，设备与 Kubernetes 对象一等公民对齐：设备生命周期（发现/消失）驱动资源创建/回收，Pod 可被调度到持有设备的节点，设备能力通过标准 K8s API 暴露，实现"设备即服务"。

2. **Q：Akri 的 Discovery Handler 机制解决了什么问题？**
   A：不同设备协议（ONVIF、OPC UA、udev、MQTT 等）差异巨大，Handler 把协议适配从核心逻辑中解耦为可插拔插件：核心（Agent/Controller）只处理"发现结果 → 资源编排"的统一流程，新设备类型只需新增 Handler，无需改动核心代码。

3. **Q：如何应对设备瞬时消失导致的 Pod 抖动？**
   A：设备消失会触发 Instance 删除与 Pod 回收，需从三方面缓解：配置发现去抖窗口过滤瞬态事件；设备 Pod 采用 Deployment + PDB 保障最小可用；对可重连协议（如 OPC UA）实现断线重连与状态恢复，避免每次断连都触发重建。

## 参考链接

- https://docs.akri.sh/
- https://github.com/project-akri/akri

## Related

- [[17-系统基础/06-知识字典/platform-engineering/kubeedge.md|KubeEdge]]
- [[17-系统基础/06-知识字典/scheduling/hami.md|HAMi]]
- [[17-系统基础/06-知识字典/specialized-workloads/openyurt.md|OpenYurt]]
