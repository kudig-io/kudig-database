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
last_updated: 2026-07
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
- target: '[[23-实体/02-K8s核心组件/deployment.md]]'
  type: uses
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 安全事件与可观测性的关联分析

## 概述

安全事件与可观测性的关联分析，是将安全工具（Falco、Cilium Tetragon）产生的告警与可观测性数据（Prometheus 指标、Loki 日志、Jaeger 追踪）进行上下文关联，从而实现更准确的威胁判断和更快的根因定位。纯粹的安全告警往往缺乏上下文，而关联可观测性数据后，安全团队能获得完整的事件全景。

## 关联价值

### 告警上下文增强

```
纯安全告警: "检测到容器异常执行"
         ↓（信息不足，难以判断严重性）

关联可观测性: "检测到容器异常执行
               Pod: web-7d9f4
               节点: node-03
               时间: 14:32
               该 Pod CPU 使用率从 20% 突增至 95%
               网络出流量增加 10 倍
               目标 IP: 已知矿池地址
               关联镜像: 最近从未经验证仓库拉取"
         ↓
→ 高置信度威胁判断
→ 立即自动隔离
→ 完整事件链供取证分析
```

## 技术实现

### 数据流架构

```
数据采集层:
  ├── Falco / Tetragon → 内核级安全事件（eBPF 驱动）
  ├── Prometheus → 基础设施指标（CPU/内存/网络/磁盘）
  ├── Loki → 应用和容器日志
  └── Jaeger → 分布式追踪

           ↓ 标准化与关联

关联分析层:
  └── 统一事件平台（如 Elasticsearch / Splunk / OpenSearch）
      → 时间窗口关联（同一 Pod/节点在 ±5 分钟内的事件）
      → 实体关联（Pod → Deployment → 镜像 → CVE 库）
      → 行为基线对比（偏离正常模式的异常检测）

           ↓

响应层:
  ├── Alertmanager → 分级告警（紧急 → PagerDuty / 普通 → Slack）
  ├── SOAR 平台 → 自动化响应（隔离 Pod / 阻断网络）
  └── SIEM → 合规审计和取证
```

### Falco + Prometheus 关联

```yaml
# Falco 规则: 检测加密货币挖矿
- rule: Cryptocurrency Miner Detected
  desc: Detect binary commonly associated with cryptocurrency miners
  condition: >
    spawned_process and (
      proc.name in (xmrig, ethminer, minerd, kdevtmpfsi) or
      proc.cmdline contains "stratum+tcp" or
      proc.cmdline contains "stratum+ssl"
    )
  output: >
    Cryptocurrency miner detected
    (user=%user.name pod=%k8s.ns.name/%k8s.pod.name
     container=%container.id proc=%proc.name
     cmd=%proc.cmdline)
  priority: CRITICAL
  tags: [container, cryptocurrency, mitre_execution]
```

### 关联查询示例

```sql
-- 关联查询: 当 Falco 告警触发时，查询同期 Prometheus 指标
SELECT 
    falco.event_type,
    falco.pod_name,
    falco.timestamp,
    prom.cpu_usage,
    prom.network_out_bytes,
    log.container_logs
FROM falco_alerts falco
LEFT JOIN prometheus_metrics prom 
    ON falco.pod_name = prom.pod_name 
    AND falco.timestamp BETWEEN prom.timestamp - INTERVAL '5 min' 
                            AND prom.timestamp + INTERVAL '5 min'
LEFT JOIN loki_logs log
    ON falco.pod_name = log.pod_name
    AND falco.timestamp BETWEEN log.timestamp - INTERVAL '5 min'
                            AND log.timestamp + INTERVAL '5 min'
WHERE falco.priority IN ('CRITICAL', 'WARNING');
```

## 典型场景

### 场景 1：加密货币挖矿攻击

```
攻击链路:
  1. Falco: 检测到 execve(/usr/bin/xmrig)
  2. Prometheus: CPU 突增（20% → 95%）
  3. Prometheus: 网络连接到已知矿池 IP（DNS 查询异常）
  4. Loki: 容器日志显示 "stratum+tcp" 连接
  5. Tetragon: 进程由 /tmp 目录执行（非标准路径）

关联结果: → 确认挖矿攻击 → 自动隔离 Pod → 阻断出口流量 → 安全事件升级
```

### 场景 2：数据泄露检测

```
攻击链路:
  1. Falco: 大量读取 /etc/passwd 或 Secret 挂载点
  2. Prometheus: 网络出流量激增（正常 10MB/s → 500MB/s）
  3. Jaeger: 追踪显示异常的数据库查询模式
  4. Tetragon: 检测到数据导出到外部 S3

关联结果: → 确认数据泄露 → 阻断网络 → 审计数据访问 → 合规报告
```

### 场景 3：容器逃逸

```
攻击链路:
  1. Tetragon: 检测到容器内进程尝试访问宿主机文件系统
  2. Falco: 检测到 /proc/1/root 异常访问
  3. Prometheus: 节点上异常进程数增长
  4. Cilium: NetworkPolicy 绕过尝试

关联结果: → 确认逃逸尝试 → 立即驱逐节点 → 隔离并取证
```

## 最佳实践

- **统一时间戳和实体标识**：确保安全工具和可观测性工具使用相同的 Pod/Node/Container 标识符，这是关联分析的基础
- **建立行为基线**：利用机器学习或统计方法建立正常的 CPU/网络/进程行为基线，只有偏离基线的异常才触发告警——减少误报
- **自动化响应（SOAR）**：高置信度威胁（如挖矿检测）应触发自动隔离，而非等待人工响应
- **保留完整事件链**：安全事件相关的指标、日志、追踪数据应保留至少 90 天，满足取证和合规要求
- **定期验证关联规则**：通过红蓝对抗演练验证关联检测规则是否有效覆盖真实攻击路径

## 常见陷阱

- **告警风暴淹没真实威胁**：Falco 规则过于敏感会产生大量低优先级告警——需要持续调优规则并设置合理的静默策略
- **关联查询性能瓶颈**：在海量数据中做时间窗口关联查询性能很差——需要在数据摄入时就打好关联标签，而非查询时关联
- **eBPF 程序开销被忽视**：Falco 和 Tetragon 的 eBPF 探针对高负载节点有 2-5% 的 CPU 开销——需要在关键节点上验证性能影响

## 相关 Domain

- 安全/04-runtime-security/01-falco-[[23-实体/02-K8s核心组件/deployment.md|deployment]]
- 可观测性/03-logging/01-logging-collection-analysis

## 相关页面

- [[22-概念/05-安全/service-mesh-security-governance.md|服务网格安全治理]] — mTLS 与 L7 授权审计
- [[22-概念/05-安全/multi-cluster-security.md|多集群安全架构]] — 跨集群安全事件关联

## Related

- [[23-实体/06-安全/falco.md|Falco (entities)]]
- [[16-专项技术/01-边缘计算/03-edge-computing-production-deployment.md|03-边缘计算生产部署]]
- [[23-实体/07-可观测性/03-prometheus-ha-deployment.md|Prometheus 高可用部署 (entities)]]
- [[log|Wiki Log]]


<!-- risk-assessed -->
