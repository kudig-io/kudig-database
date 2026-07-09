---
title: Kubewarden 策略引擎
description: Kubewarden 是 SUSE 开源的 CNCF Sandbox 项目，使用 WebAssembly（Wasm）作为策略执行引擎，支持用
  Rust/Go/T...
summary: Kubewarden 是 SUSE 开源的 CNCF Sandbox 项目，使用 WebAssembly（Wasm）作为策略执行引擎，支持用 Rust/Go/T...
category: dictionary
tags:
- k8s
- glossary
- security
- policy
- wasm
tier: core
created: 2026-06
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubewarden 策略引擎 是什么
- Kubewarden 详解
trigger_keywords:
- Kubewarden 策略引擎
- Kubewarden
- dictionary
prerequisites:
- kubernetes
---



# Kubewarden 策略引擎（Kubewarden）

## 概述

Kubewarden 是 SUSE 开源的 CNCF Sandbox 项目，使用 WebAssembly（Wasm）作为策略执行引擎，支持用 Rust/Go/TypeScript/Rego 等多种语言编写 Admission 策略。

## 核心概念/原理

- **Wasm 策略引擎**：使用 WebAssembly 沙箱执行策略
- **多语言**：支持 Rust/Go/TypeScript/Rego/Kubernetes CEL 编写策略
- **CNCF Sandbox**：SUSE 主导
- **安全沙箱**：Wasm 提供强隔离的策略执行环境

## 关键机制或特性

- AdmissionPolicy / ClusterAdmissionPolicy CRD
- Wasm 模块作为策略执行单元
- PolicyServer 管理策略执行
- 策略可从 OCI Registry 分发
- 支持上下文感知（Context Aware）策略
- Kubewarden Inspector 策略审计
- 与 Kyverno/OPA 策略互补

## 使用场景与最佳实践

- Admission 策略的 Wasm 安全执行
- 多语言策略开发
- 策略即代码（Policy as Code）
- 需要强隔离的策略执行环境
- 从 OCI Registry 分发和管理策略

## 参考链接

- https://kubewarden.io/
- https://github.com/kubewarden

## Related

- [[系统基础/topic-dictionary/security/opa.md|OPA]]
- [[系统基础/topic-dictionary/security/kyverno.md|Kyverno]]
- [[系统基础/topic-dictionary/security/gatekeeper.md|Gatekeeper]]
