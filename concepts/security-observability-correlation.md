---
title: 安全事件与可观测性的关联分析
description: '# 安全事件与可观测性的关联分析'
summary: 'Falco → Alertmanager → Loki (日志) + Prometheus (指标)'
category: synthesis
tags:
- security
- observability
- threat-detection
- falco
- eBPF
- prometheus
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 安全事件与可观测性的关联分析 是什么
- 如何 安全事件与可观测性的关联分析
trigger_keywords:
- 安全事件与可观测性的关联分析
prerequisites:
- kubectl-basics
- prometheus-basics
- logging-basics
relationships:
- target: '[[entities/deployment.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 安全事件与可观测性的关联分析

## 关联价值

```
纯安全告警: "检测到容器异常执行"
        ↓
关联可观测性: "检测到容器异常执行 (Pod: web-7d9f4, 
               节点: node-03, 时间: 14:32,
               该 Pod CPU 使用率从 20% 突增至 95%,
               网络出流量增加 10 倍)"
        ↓
→ 更准确的威胁判断
→ 更快的根因定位
```

## 技术实现

```
Falco → Alertmanager → Loki (日志) + Prometheus (指标)
          ↓
    统一事件平台
          ↓
    关联分析引擎
```

## 典型场景

```
场景 1: 加密货币挖矿
  - Falco: 检测到 execve(/usr/bin/xmrig)
  - Prometheus: CPU 突增，网络连接到矿池 IP
  → 确认攻击，自动隔离

场景 2: 数据泄露
  - Falco: 大量读取 /etc/passwd
  - Prometheus: 网络出流量激增
  → 确认异常，阻断网络
```

## 相关 Domain

- 安全/04-runtime-security/01-falco-[[entities/deployment.md|deployment]]
- 可观测性/03-logging/01-logging-collection-analysis
## Related

- [[entities/falco.md|Falco (entities)]]
- [[专项技术/03-edge-computing-production-deployment.md|03-边缘计算生产部署]]
- [[entities/03-prometheus-ha-deployment.md|Prometheus 高可用部署 (entities)]]
- [[log|Wiki Log]]


<!-- risk-assessed -->
