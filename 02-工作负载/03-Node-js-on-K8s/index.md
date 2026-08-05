---
title: Node.js on K8s 索引
description: Node.js/Deno/Bun 在 Kubernetes 上的生产级部署与运维实践
summary: Node.js 运行时在 K8s 上的生产实践，涵盖性能调优、内存管理、健康检查、优雅关闭、可观测性集成
category: index
tags:
- index
- nodejs
- workloads
- javascript
tier: supporting
created: '2026-07-21'
last_updated: '2026-07-21'
---

# Node.js on K8s

> Node.js / Deno / Bun 运行时在 Kubernetes 上的生产级部署与运维实践。

## 文档

| 文件 | 内容 |
|------|------|
| [[02-工作负载/03-Node-js-on-K8s/01-nodejs-production-kubernetes.md\|Node.js 生产部署]] | 生产级配置、内存管理、优雅关闭、健康检查 |
| [[02-工作负载/03-Node-js-on-K8s/02-nodejs-observability-performance.md\|Node.js 可观测性与性能]] | 性能调优、APM 集成、事件循环监控 |

## 核心关注点

| 关注点 | 说明 |
|--------|------|
| 内存管理 | V8 堆限制 vs 容器 memory limit 对齐 |
| 优雅关闭 | SIGTERM 处理、连接排空、preStop hook |
| 健康检查 | liveness/readiness/startup probe 设计 |
| 事件循环 | Event Loop Lag 监控与告警 |
| 集群模式 | node:cluster vs 单进程 + HPA |
| 可观测性 | OpenTelemetry SDK、Prometheus 指标暴露 |

## Related

- [[02-工作负载/01-核心工作负载/index.md|核心工作负载]] — K8s 原生工作负载控制器
- [[32-发布/package/2026-07-02_18-29/corpus/supporting/skills/training-lecturer/11-workloads/index|多语言运行时]] — Go/Python/Rust 实践
- [[27-标签/05-交付与运维/production|production 标签枢纽]] — 生产实践
