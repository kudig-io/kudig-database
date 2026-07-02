---
title: CNCF 边缘计算与 AI/ML 项目全景
description: '## 概述'
summary: '新兴计算范式覆盖 **边缘计算**、**AI/ML 平台**、**Serverless** 和 **裸机/设备管理** 四大领域。'
category: entities
tags:
- k8s
- cncf
- edge
- ai-ml
- serverless
- wasm
- etcd
- crd
- operator
- kubeflow
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- CNCF 边缘计算与 AI/ML 项目全景 是什么
- 如何 CNCF 边缘计算与 AI/ML 项目全景
trigger_keywords:
- CNCF
- 边缘计算与
- AI
- ML
- 项目全景
prerequisites:
- kubectl-basics
- etcd-basics
---



# CNCF 边缘计算与 AI/ML 项目全景

> 聚合页面 | 涵盖 20 个 CNCF 边缘/AI/Serverless 项目

## 概述

新兴计算范式覆盖 **边缘计算**、**AI/ML 平台**、**Serverless** 和 **裸机/设备管理** 四大领域。

---

## 边缘计算

### [[kubeedge]] — 毕业项目

KubeEdge 将 K8s 扩展到边缘。

- 边缘节点离线自治运行
- 边缘设备管理（DeviceModel CRD）
- 云端-边端协同架构

### [[openyurt]] — 孵化项目

OpenYurt 是阿里开源的边缘计算平台。

- 节点池管理
- 边缘网关
- 与标准 K8s API 兼容

### [[k0s]] — 沙箱项目

k0s 是零依赖的轻量级 K8s 发行版。

### [[k3s]] — 沙箱项目

k3s 是 Rancher 的轻量级 K8s（<100MB 二进制）。

- 边缘和 IoT 场景首选
- 单节点部署
- 内置 SQLite（可选 etcd）

### [[akri]] — 沙箱项目

Akri 自动发现边缘设备并暴露为 K8s 资源。

---

## AI/ML 平台

### [[kserve]] — 孵化项目

KServe 提供 K8s 上的无服务器模型推理。

- 自动伸缩（缩容到零）
- 多框架支持（TensorFlow/PyTorch/ONNX）
- 推理图（InferenceGraph）

### [[kubeflow]] — 孵化项目

Kubeflow 是 K8s 上的 ML 工作流平台。

- Jupyter Notebooks
- Pipelines（ML 工作流编排）
- Training Operator（分布式训练）
- KServe 推理

### [[kaito]] — 沙箱项目

KAITO（Kubernetes AI Toolchain Operator）自动化 AI 模型推理部署。

### [[kagent]] — 沙箱项目

KAgent 在 K8s 上部署 AI Agent。

### [[k8sgpt]] — 沙箱项目

K8sGPT 利用 AI 诊断 K8s 集群问题。

### [[vineyard]] — 沙箱项目

Vineyard 是内存数据共享中间件，用于 ML 数据流水线。

---

## Serverless 与 FaaS

### [[openfunction]] — 沙箱项目

OpenFunction 是 K8s 原生 FaaS 平台。

### [[slimfaas]] — 沙箱项目

SlimFaas 是轻量级 FaaS 框架。

### [[serverless-devs]] — 沙箱项目

[[entities/serverless-devs.md|Serverless Devs]] 是 Serverless 应用开发工具。

### [[serverless-workflow]] — 沙箱项目

Serverless Workflow 定义无服务器工作流规范。

### [[Radius]] — 沙箱项目

Radius 是云原生应用平台。

---

## 裸机与设备管理

### [[tinkerbell]] — 沙箱项目

Tinkerbell 提供裸机配置和置备。

### metal3-io — 孵化项目

Metal3 使用 BMO（BareMetal Operator）管理裸机。

### [[flatcar]] — 孵化项目

Flatcar Container Linux 是不可变的容器优化 Linux 发行版。

### [[cozystack]] — 沙箱项目

Cozystack 是 PaaS/云平台构建工具。

### [[interlink]] — 沙箱项目

InterLink 将远程计算资源连接到 K8s。

---

## 架构选型建议

| 场景 | 推荐方案 |
|---|---|
| 边缘 K8s | K3s 或 KubeEdge |
| ML 模型推理 | KServe |
| ML 全流程 | Kubeflow |
| 裸机置备 | Metal3 或 Tinkerbell |
| 轻量 K8s | K3s 或 k0s |

---

## 相关页面

- [[entities/cncf-orchestration.md|cncf-orchestration]] — 编排与应用管理
- [[entities/cncf-runtime.md|cncf-runtime]] — 容器运行时与工具链

## Related

- [[serverless-workflow]] — Serverless Workflow
- [[serverless-devs]] — Serverless Devs
- [[flatcar]] — Flatcar Container Linux
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[entities/kaito.md|KAITO]]
- [[entities/kairos.md|Kairos]]