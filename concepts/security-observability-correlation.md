---
title: 安全事件与可观测性的关联分析
description: '# 安全事件与可观测性的关联分析'
summary: '# 安全事件与可观测性的关联分析'
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

- domain-05-security-compliance/04-runtime-security/01-falco-[[entities/deployment.md|deployment]]
- domain-06-observability/03-logging/01-logging-collection-analysis
## Related

- [[entities/falco.md|Falco (entities)]]
- [[domain-15-specialized-tech/03-edge-computing-production-deployment.md|03-边缘计算生产部署]]
- [[entities/03-prometheus-ha-deployment.md|Prometheus 高可用部署 (entities)]]
- [[log|Wiki Log]]
