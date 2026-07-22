---
title: 系统基础知识域
description: 云原生系统基础知识域总索引，覆盖 Linux、硬件、网络基础、K8s 事件、速查卡、知识字典 6 大子领域
summary: 系统基础知识域总索引，覆盖 Linux/硬件/网络基础/K8s事件/速查卡/知识字典
category: index
tags:
- index
- system-foundation
- linux
- hardware
- networking
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: beginner
audience:
- SRE
- 平台工程师
- 开发工程师
---

# 系统基础知识域

> 系统基础是云原生知识体系的根基，覆盖 Linux 操作系统、硬件架构、网络基础、Kubernetes 事件、速查卡、知识字典等核心领域。

## 领域概述

系统基础知识域是理解 Kubernetes 和云原生技术的基石，包括：

- **Linux 操作系统**：内核架构、进程管理、文件系统、网络、存储、性能调优、安全加固
- **硬件架构**：服务器、CPU、内存、存储、网络硬件、故障排查
- **网络基础**：TCP/IP 协议栈、DNS、iptables/nftables、HTTP/HTTPS、负载均衡
- **K8s 事件**：事件架构、Pod/节点/工作负载事件、故障排查
- **速查卡**：kubectl、Docker、Helm、Git、GitOps、网络、PromQL 等命令速查
- **知识字典**：13 个子领域的术语定义、概念解析、实践指南

## 子目录索引

| 子目录 | 内容 | 文件数 | 难度 |
|--------|------|--------|------|
| [[系统基础/Linux/index.md\|Linux/]] | Linux 系统知识体系 | 15 | 中级-高级 |
| [[系统基础/硬件/index.md\|硬件/]] | 硬件知识体系 | 19 | 中级 |
| [[系统基础/网络基础/index.md\|网络基础/]] | TCP/IP、DNS、iptables、HTTP | 7 | 中级-高级 |
| [[系统基础/K8s事件/index.md\|K8s事件/]] | Kubernetes 事件系统 | 16 | 中级 |
| [[系统基础/速查卡/index.md\|速查卡/]] | 命令速查卡集合 | 17 | 初级-中级 |
| [[系统基础/知识字典/index.md\|知识字典/]] | 术语定义与概念解析 | 16子目录 | 全级别 |

## 学习路径

### 新手入门

1. [[系统基础/Linux/index.md|Linux 基础]] → 掌握操作系统核心概念
2. [[系统基础/速查卡/index.md|速查卡]] → 熟悉常用命令
3. [[系统基础/知识字典/index.md|知识字典]] → 建立术语体系

### 中级进阶

4. [[系统基础/网络基础/index.md|网络基础]] → 深入网络协议
5. [[系统基础/K8s事件/index.md|K8s 事件]] → 掌握故障排查
6. [[系统基础/硬件/index.md|硬件知识]] → 理解底层架构

### 高级精通

7. Linux 性能调优 + 内核参数
8. 网络抓包 + eBPF 诊断
9. 硬件故障排查 + 生产案例

## 核心文档

- [[系统基础/02-linux-kernel-container-fundamentals.md|Linux 内核与容器基础]]
- [[系统基础/99-production-readiness-operations-guide.md|生产就绪运维指南]]
- [[系统基础/README.md|Readme]]

## 跨域关联

| 关联域 | 关联内容 |
|--------|----------|
| 集群基础 | Linux 内核参数影响 etcd/apiserver 性能 |
| 网络 | CNI 实现依赖 iptables/eBPF |
| 存储 | Linux 文件系统与 CSI 驱动 |
| 可观测性 | 系统指标采集依赖 Linux 工具 |
| 故障诊断 | 系统级故障排查方法论 |

## 统计概览

| 指标 | 数值 |
|------|------|
| 子目录数 | 6 |
| 总文件数 | 90+ |
| 覆盖主题 | Linux/硬件/网络/事件/速查/字典 |
| 难度范围 | 初级 → 专家 |
| 目标受众 | SRE/平台工程师/开发/网络工程师 |

## 维护规范

- 每个文件 >= 500 行，包含概念原理、实践示例、故障排查、最佳实践
- frontmatter 必须包含 title, tags, domain, difficulty, audience
- 使用 Obsidian Wiki 链接格式 `[[path|name]]`
- 命令标注风险等级：🔴 高风险、🟡 中风险、🟢 低风险
