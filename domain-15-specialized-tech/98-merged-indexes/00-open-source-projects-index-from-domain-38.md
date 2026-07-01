---
title: Domain-38 WebAssembly 云原生 — 开源项目索引
description: Domain-38 WebAssembly 云原生 — 开源项目索引 — Kubernetes 生产运维知识库
summary: Domain-38 WebAssembly 云原生 — 开源项目索引 — Kubernetes 生产运维知识库
category: webassembly-cloud-native
tags:
- k8s
- wasm
- webassembly
- cloud-native
- kubelet
- envoy
- containerd
- gateway
- operator
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- 开发工程师
- SRE
estimated_read_time: 5min
intent_queries:
- Domain-38 WebAssembly 云原生 — 开源项目索引 是什么
- 如何 Domain-38 WebAssembly 云原生 — 开源项目索引
- Kubernetes 38 webassembly cloud native 最佳实践
trigger_keywords:
- Domain-38
- WebAssembly
- 云原生
- 开源项目索引
- webassembly
- cloud
- native
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---



# Domain-38 WebAssembly 云原生 — 开源项目索引

> **最后更新**: 2026-04-24

---

## 核心项目

| 项目 | 作用 | 归属 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **WasmEdge** | 轻量级 Wasm 运行时 | CNCF Sandbox | v0.14.0 | 8k+ | Apache-2.0 |
| **wasmCloud** | 分布式 Wasm 应用平台 | CNCF Incubating | v1.5.0 | 4k+ | Apache-2.0 |
| **Spin** | 开发者友好的 Wasm 框架 | Fermyon | v3.2.0 | 6k+ | Apache-2.0 |
| **SpinKube** | K8s 上的 Spin 运行时 | SpinKube | v0.6.0 | 500+ | Apache-2.0 |
| **containerd/runwasi** | containerd Wasm  shim | containerd | v0.8.0 | 1k+ | Apache-2.0 |
| **crun-wasm** | crun Wasm 支持 | Red Hat | v1.20.0 | - | GPL-2.0+ |
| **WASI** | Wasm 系统接口标准 | Bytecode Alliance | v0.2.0 | - | 标准 |
| **WAMR** | 轻量 Wasm 微运行时 (Intel) | Bytecode Alliance | v2.2.0 | 5k+ | Apache-2.0 |
| **Envoy WASM Filter** | Envoy Wasm 扩展 | Envoy | v1.33.0 | - | Apache-2.0 |
| **Open Policy Agent (WASM)** | Rego 编译为 Wasm | CNCF Graduated | v1.3.0 | 9.5k+ | Apache-2.0 |
| **Krustlet** | Kubelet 的 Wasm 实现 (已归档) | Deis/Microsoft | 归档 | 3k+ | Apache-2.0 |
| **Kwasm** | K8s Wasm 运行时 Operator | 社区 | v0.5.0 | 1k+ | Apache-2.0 |
| **Slight** | SpiderLightning Wasm 框架 | Deis | v0.5.0 | 1k+ | MIT |
| **WebAssembly Gateway (Envoy)** | Wasm 网关扩展 | Envoy | v1.33.0 | - | Apache-2.0 |
| **Fermyon Cloud** | Wasm PaaS | Fermyon | SaaS | - | 商业 |
| **Cosmonic** | wasmCloud PaaS | Cosmonic | SaaS | - | 商业 |
| **Slight** | SpiderLightning Wasm 框架 | Deis | v0.5.0 | 1k+ | MIT |
| **Kwasm** | K8s Wasm 运行时 Operator | 社区 | v0.5.0 | 1k+ | Apache-2.0 |

---

## 参考链接

- [WasmEdge 文档](https://wasmedge.org/docs/)
- [wasmCloud 文档](https://wasmcloud.com/docs/)
- [Spin 文档](https://developer.fermyon.com/spin/)
- [W3C WebAssembly](https://webassembly.org/)
- [Bytecode Alliance](https://bytecodealliance.org/)

---

## Obsidian 相关文档

- domain-38-webassembly-cloud-native MOC
- [[domain-15-specialized-tech/README.md|Domain 15: WebAssembly 云原生 (WebAssembly Cloud Native)]]
- WebAssembly 云原生基础
- containerd Wasm 运行时
- SpinKube 框架实践
- wasmCloud 平台
- WasmEdge 运行时
- Wasm 组件模型 (Wasm Component Model)
- Wasm 插件系统 (Wasm Plugin System)
- Wasm AI 推理 (Wasm AI Inference)
- Wasm Serverless (Wasm Serverless)
- Wasm 安全与沙箱 (Wasm Security and Sandbox)
