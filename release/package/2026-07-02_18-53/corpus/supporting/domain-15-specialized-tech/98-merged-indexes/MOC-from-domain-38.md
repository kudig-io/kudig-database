---
title: domain-38-webassembly-cloud-native MOC
description: domain-38-webassembly-cloud-native 知识域导航页，覆盖 12 篇文档
summary: domain-38-webassembly-cloud-native 知识域导航页，覆盖 12 篇文档
category: moc
tags:
- k8s
- moc
- wasm
- containerd
- serverless
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-21'
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- domain-38-webassembly-cloud-native MOC 是什么
- 如何 domain-38-webassembly-cloud-native MOC
- Kubernetes 15 specialized tech 最佳实践
trigger_keywords:
- domain-38-webassembly-cloud-native
- MOC
- specialized
- tech
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# domain-38-webassembly-cloud-native MOC

> **MOC 版本**: 1.0
> **知识域**: domain-38-webassembly-cloud-native
> **文档数量**: 12 篇
> **最后更新**: 2026-05-21
> **用途**: 本知识域的导航入口，汇总所有相关文档、关联领域、和场景入口

---

## 领域概述

WebAssembly 云原生 — Wasm、WASI、WasmEdge

### 知识域定位

| 维度 | 说明 |
|---|---|
| **知识域** | domain-38-webassembly-cloud-native |
| **文档数量** | 12 篇 |
| **难度分布** | 入门 0 / 进阶 0 / 高级 0 / 专家 0 |

---

## 文档清单

| # | 文档 | 难度 | 标签 | 估计阅读时间 |
|---|---|---|---|---|
| 1 | Domain-38 WebAssembly 云原生 — 开源项目索引 |  | wasm, cloud-native |  |
| 2 | WebAssembly 云原生基础 |  | wasm, cloud-native |  |
| 3 | containerd Wasm 运行时 |  | wasm, cloud-native |  |
| 4 | SpinKube 框架实践 |  | wasm, cloud-native |  |
| 5 | wasmCloud 平台 |  | wasm, cloud-native |  |
| 6 | WasmEdge 运行时 |  | wasm, cloud-native |  |
| 7 | Wasm 组件模型 (Wasm Component Model) |  | wasm, cloud-native |  |
| 8 | Wasm 插件系统 (Wasm Plugin System) |  | wasm, cloud-native |  |
| 9 | Wasm AI 推理 (Wasm AI Inference) |  | wasm, cloud-native |  |
| 10 | Wasm Serverless (Wasm Serverless) |  | wasm, cloud-native |  |
| 11 | Wasm 安全与沙箱 (Wasm Security and Sandbox) |  | wasm, cloud-native, security |  |
| 12 | WebAssembly (Wasm) 云原生实践指南 |  | wasm, cloud-native, guide |  |

---

## 知识图谱

```mermaid
graph TD
    subgraph domain-38-webassembly-cloud-native
        A["Domain-38 WebAssembly 云原生 — 开源项目索引"]
    B["WebAssembly 云原生基础"]
    C["containerd Wasm 运行时"]
    D["SpinKube 框架实践"]
    E["wasmCloud 平台"]
    F["WasmEdge 运行时"]
    end

    A --> B
    A --> C
    A --> D
    A --> E
    A --> F

    style A fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style B fill:#22c55e,stroke:#166534,color:#fff
```

---

## 关联入口

| 入口 | 说明 |
|---|---|
| FTA 故障树 | domain-38-webassembly-cloud-native 相关故障树分析 |
| Skills 技能 | domain-38-webassembly-cloud-native 相关操作技能 |
| 深度研究入口 | 语料库索引与向量检索 |

---

## 统计信息

| 指标 | 数值 |
|---|---|
| 文档总数 | 12 |
| 覆盖 K8s 版本 | v1.25 - v1.32 |

---

*本文档由 scripts/generate-mocs.py 自动生成，最后更新 2026-05-21。*


<!-- risk-assessed -->
