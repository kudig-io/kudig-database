---
title: Spin (entities)
description: '## 概述'
summary: 'Spin 是由 Fermyon 开发的 WebAssembly (Wasm) 微服务框架，用于构建和运行基于事件驱动的 Wasm 应用。它提供极快的冷启动时间（亚毫秒级），支持多种编程语言（Rust、Go、Python、JavaScript、C#等），并内置 HTTP 触发器、Redis 触发器、键值存储、SQL 数据库等能力。'
category: entities
tags:
- k8s
- cncf
- runtime
- spin
- argocd
- containerd
- redis
- crd
- operator
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Spin 是什么
- 如何 Spin
trigger_keywords:
- Spin
prerequisites:
- kubectl-basics
- gitops-basics
- redis-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Spin

> **CNCF 状态**: Sandbox | **类别**: Runtime | **主要语言**: Rust

## 概述

Spin 是由 Fermyon 于 2021 年开发的开源 WebAssembly（Wasm）应用框架，2023 年进入 CNCF Sandbox。它专注于构建和运行基于 Wasm 组件的微服务和边缘应用。与传统容器不同，Spin 应用编译为 Wasm 模块，冷启动时间在亚毫秒级（通常 < 1ms），内存占用仅几 MB。

Spin 使用 `spin.toml` 清单文件定义应用，包含多个组件（Component），每个组件绑定一个触发器（Trigger，如 HTTP、Redis、定时器）和一个 Wasm 模块。内置的 Key-Value 存储、SQLite 数据库和 Outbound HTTP 能力让开发者无需额外配置即可构建完整应用。Spin 支持通过 OCI Registry 分发应用，也支持通过 SpinKube 项目在 Kubernetes 中原生运行 Wasm 工作负载。

## Key Features

- **亚毫秒冷启动**：Wasm 模块加载速度极快，适合 Serverless 和边缘场景
- **多语言 SDK**：提供 Rust、Go、Python、JavaScript、TypeScript、C# 的 SDK 和模板
- **内置存储**：无需外部依赖即可使用 KV Store、SQLite、Redis 等存储能力
- **OCI 分发**：支持将 Spin 应用打包为 OCI 镜像推送到标准 Registry
- **触发器模型**：通过 `[[trigger]]` 声明式绑定 HTTP、Redis、定时器等事件源
- **安全沙箱**：基于 Wasm 的 capability-based 安全模型，必须显式声明 `allowed_outbound_hosts`

## Architecture

Spin 运行时基于 `wasmtime` 引擎，通过 `spin.toml` 清单文件解析应用定义。每个组件在独立的 Wasm 实例中运行，通过 Spin Host Interface 访问宿主提供的 API（KV、SQL、HTTP、时钟）。触发器（Trigger）监听外部事件并在匹配时实例化对应的 Wasm 组件执行业务逻辑。组件之间通过共享的 KV Store 或外部消息系统通信。

## K8s 集成

Spin 通过 **SpinKube** 项目（CNCF Sandbox）与 Kubernetes 深度集成。SpinKube 提供 `SpinAppExecutor` 和 `SpinApp` CRD，通过 containerd-shim-spin 在节点上原生运行 Wasm 工作负载，无需传统容器镜像。也可通过 Spin Operator（Helm Chart）在标准 Kubernetes 集群上部署 Spin 运行时。

## 生产部署要点

- **组件粒度**：每个路由前缀使用独立组件，实现最小权限和独立部署
- **最小权限**：通过 `allowed_outbound_hosts` 限制组件的外部访问范围
- **存储选择**：简单 KV 用内置 KV Store，关系数据用 SQLite 或外部 DB
- **OCI 分发**：使用 OCI Registry 管理 Spin 应用版本
- **Wasm 优化**：使用 `wasm-opt` 优化 Wasm 二进制大小

## 生产场景

1. **边缘 IoT 数据处理**：在资源受限设备上运行轻量数据处理逻辑
2. **Serverless API**：按需加载的 Wasm 微服务，零冷启动延迟
3. **实时数据管道**：Redis 触发器驱动的事件处理组件
4. **插件化扩展**：安全沙箱中运行第三方代码，宿主系统不受影响

## 安装

```bash
# 安装 Spin CLI
curl -fsSL https://developer.fermyon.com/downloads/install.sh | bash
sudo mv spin /usr/local/bin/
# 创建新应用
spin templates install --git https://github.com/fermyon/spin
spin new http-rust myapp
cd myapp && spin build && spin up
```

## 对比

| 特性 | Spin | wasmCloud | WasmEdge |
|------|------|-----------|---------|
| 开发模型 | 应用框架 | 分布式平台 | 嵌入式运行时 |
| 触发器 | HTTP/Redis/Timer | NATS | 命令行/API |
| Kubernetes | SpinKube | wasmCloud Operator | Kubernetes |
| 冷启动 | < 1ms | < 1ms | < 1ms |

## 参考链接

- [[containerd]]
- [[实体/argocd.md|[[ArgoCD|argocd]]]]
- [[operator-pattern]]

## Related

- [[spinkube]] — SpinKube
- [[wasmedge]] — WasmEdge
- [[实体/cncf-runtime.md|cncf-runtime]] — CNCF 容器运行时与工具链项目全景
- [[04-containerd-upgrade-migration]] — containerd 升级迁移
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 03-spinkube-framework
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
