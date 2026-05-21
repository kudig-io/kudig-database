---
title: 系统日志（System Logs）
description: '# 系统日志（System Logs）'
category: dictionary
tags:
- k8s
- glossary
- terminology
- apiserver
- kubelet
- scheduler
- controller-manager
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 系统日志（System Logs） 是什么
- 如何 系统日志（System Logs）
trigger_keywords:
- 系统日志
- System
- Logs
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

# 系统日志（System Logs）

## 概述

系统组件日志记录了集群中发生的事件，对于调试和故障排查非常有用。通过配置日志详细程度（verbosity），可以查看从粗粒度的错误信息到细粒度的逐步事件跟踪（如 HTTP 访问日志、Pod 状态变化、控制器操作、调度器决策等）等不同级别的日志内容。

## 核心概念/原理

- **klog**：Kubernetes 的日志库，为 Kubernetes 系统组件生成日志消息。
- **日志输出稳定性**：与命令行标志不同，日志输出的格式和内容**不属于 Kubernetes API 稳定性保证范围**，不同版本之间日志条目和格式可能发生变化。
- **日志输出目标**：输出始终写入 `stderr`，重定向由调用 Kubernetes 组件的外部程序（如 POSIX shell、systemd）处理。
- **kube-log-runner**：在无法使用 shell 重定向的环境（如 distroless 容器、Windows 系统服务）中，可使用 `kube-log-runner` 包装器来重定向日志输出。

## 关键机制或特性

### klog 命令行标志变更

自 Kubernetes v1.23 起，以下 klog 命令行标志已被弃用，并在 v1.26 中移除：

- `--add-dir-header`
- `--alsologtostderr`
- `--log-backtrace-at`
- `--log-dir`
- `--log-file`
- `--log-file-max-size`
- `--logtostderr`
- `--one-output`
- `--skip-headers`
- `--skip-log-headers`
- `--stderrthreshold`

### 结构化日志（Structured Logging）

FEATURE STATE: `Kubernetes v1.23 [beta]`

结构化日志引入了统一的日志消息结构，便于程序化提取信息。默认文本格式与传统 klog 向后兼容：

```
<klog header> "<message>" <key1>="<value1>" <key2>="<value2>" ...
```

示例：

```
I1025 00:15:15.525108       1 controller_utils.go:116] "Pod status updated" pod="kube-system/kubedns" status="ready"
```

注意：向结构化日志的迁移仍在进行中，当前版本中并非所有日志消息都是结构化的。

### 上下文日志（Contextual Logging）

FEATURE STATE: `Kubernetes v1.30 [beta]`（Kubernetes 1.35 默认启用，由 `ContextualLogging` 特性门控控制）

上下文日志在结构化日志之上构建。如果开发者在组件中使用 `WithValues` 或 `WithName` 等函数，日志条目会包含调用者传入的额外信息（如 `logger="example.myname"`、`foo="bar"`）。禁用上下文日志后，这些额外信息将不会出现在输出中。

### JSON 日志格式

FEATURE STATE: `Kubernetes v1.19 [alpha]`

通过 `--logging-format=json` 标志可将日志格式从 klog 原生格式切换为 JSON 格式。关键字段包括：

- `ts`：Unix 时间戳（float，必需）
- `v`：详细程度（int，仅用于 info 消息）
- `err`：错误字符串（string，可选）
- `msg`：消息（string，必需）

当前支持 JSON 格式的组件：kube-controller-manager、kube-apiserver、kube-scheduler、kubelet。

注意：并非所有日志都保证以 JSON 格式输出（如进程启动期间），解析日志时需要能处理非 JSON 行。

### 日志详细程度级别

`-v` 标志控制日志详细程度：

- 值越大，记录的事件越多。
- 值越小，记录的事件越少。
- `0` 仅记录关键事件。

### 系统组件日志位置

- **Linux（systemd）**：kubelet 和容器运行时写入 `journald`；非 systemd 环境写入 `/var/log` 下的 `.log` 文件。
- **Windows**：默认写入 `C:\var\logs`。
- **容器内运行的组件**：直接写入 `/var/log` 下的 `.log` 文件，绕过默认容器日志机制。

### 日志查询（Node Log Query）

FEATURE STATE: `Kubernetes v1.30 [beta]`（默认禁用）

启用 `NodeLogQuery` 特性门控，并将 kubelet 配置中的 `enableSystemLogHandler` 和 `enableSystemLogQuery` 设为 `true` 后，可以通过 API 查询节点上的服务日志：

```bash
kubectl get --raw "/api/v1/nodes/<node-name>/proxy/logs/?query=kubelet"
kubectl get --raw "/api/v1/nodes/<node-name>/proxy/logs/?query=kubelet&pattern=error"
```

支持参数：`boot`、`pattern`、`query`、`sinceTime`、`untilTime`、`tailLines`。

## 使用场景

- **组件故障排查**：查看 kubelet、scheduler、controller-manager 等组件的日志定位问题。
- **审计与安全分析**：收集和分析控制平面组件的访问与操作日志。
- **性能调优**：通过调高 `-v` 级别获取更细粒度的事件跟踪，分析性能瓶颈。
- **自动化日志收集**：结合节点日志代理将系统组件日志统一收集到集中式日志平台。

## 最佳实践/注意事项

- 日志格式和内容可能随版本变化，构建自动化解析工具时需保持兼容性和可更新性。
- 对于 distroless 容器或 Windows 服务，使用 `kube-log-runner` 实现日志重定向。
- 系统组件日志需要配置日志轮转，防止磁盘空间耗尽。
- 使用 JSON 格式时注意并非所有启动阶段的日志都是 JSON，解析器需要具备容错能力。
- 启用 Node Log Query 时，确保只有授权用户才能访问节点日志 API，避免信息泄露。
- 日志详细程度 `-v` 值越高，日志量越大，生产环境中需权衡排查需求与存储/性能开销。

## 参考链接

- [System Logs - Kubernetes 官方文档](https://kubernetes.io/docs/concepts/cluster-administration/system-logs/)

## Related

- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
