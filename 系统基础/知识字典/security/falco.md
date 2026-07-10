---
title: Falco
description: Falco 是 CNCF 毕业项目，提供云原生运行时安全检测能力。它通过系统调用（syscall）监控容器和主机的异常行为，是 Kubernetes
  运行时安全...
summary: Falco 是 CNCF 毕业项目，提供云原生运行时安全检测能力。它通过系统调用（syscall）监控容器和主机的异常行为，是 Kubernetes
  运行时安全...
category: dictionary
tags:
- k8s
- glossary
- falco
- security
- runtime-security
- cncf
- ebpf
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Falco 是什么
- Falco 详解
trigger_keywords:
- Falco
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Falco

> **英文名**: Falco

## 概述

Falco 是 CNCF 毕业项目，提供云原生运行时安全检测能力。它通过系统调用（syscall）监控容器和主机的异常行为，是 Kubernetes 运行时安全的标准工具。

## 核心概念/原理

### 核心架构

- **Falco Engine**：规则匹配引擎，处理系统调用事件。
- **Falco Drivers**：内核模块或 eBPF 探针，采集 syscall 数据。
- **Falco Rules**：YAML 格式的安全规则定义。
- **Falco Sidekick**：将告警转发到 Slack、Grafana、Kafka 等。

### 规则示例

```yaml
- rule: Terminal shell in container
  desc: A shell was opened in a container
  condition: spawned_process and container and proc.name in (bash, sh, zsh)
  output: >
    Shell opened in container
    (user=%user.name container=%container.name shell=%proc.name)
  priority: WARNING
```

## 关键机制或特性

- **eBPF 探针**：现代部署推荐使用 eBPF 替代内核模块，更安全。
- **规则优先级**：Emergency → Critical → Error → Warning → Notice → Info → Debug。
- **宏和列表**：可组合的 reusable 规则构建块。
- **插件系统**：支持扩展数据源（K8s Audit Log、CloudTrail 等）。
- 与 Kubernetes Audit Log 结合实现 API 级别的安全监控。

## 使用场景与最佳实践

- 部署 Falco 作为 DaemonSet 监控所有节点的运行时行为。
- 使用 Falco Talon 实现自动化响应（如杀死可疑进程）。
- 配合 Falco Sidekick 将告警发送到 Slack/PagerDuty。
- 自定义规则检测特定于业务的异常行为。
- 定期审查 Falco 告警，减少误报。

## 参考链接

- [Falco Official](https://falco.org/docs/)

## Related

- [[系统基础/知识字典/security/trivy.md|Trivy]]
- [[系统基础/知识字典/security/security-context.md|Security Context]]
- [[系统基础/知识字典/security/rbac.md|RBAC]]
- [[系统基础/知识字典/networking/cilium.md|Cilium]]
- [[系统基础/知识字典/observability/prometheus.md|Prometheus]]


<!-- risk-assessed -->
