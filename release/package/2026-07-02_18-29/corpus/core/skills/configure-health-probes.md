---
title: Configure Health Probes
description: Configure Health Probes — Kubernetes 生产运维知识库
summary: Configure Health Probes — Kubernetes 生产运维知识库
category: skills
tags:
- k8s
- probes
- liveness
- readiness
- startup
- health-check
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Configure Health Probes 是什么
- 如何 Configure Health Probes
trigger_keywords:
- Configure
- Health
- Probes
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Configure Health Probes

## When to Use Each Probe

| Probe | Use When | Effect on Failure |
|-------|----------|-------------------|
| **startupProbe** | Application has slow or variable startup time | Disables liveness/readiness until first success |
| **livenessProbe** | Application can deadlock or become unresponsive | Container is restarted |
| **readinessProbe** | Application needs warmup time or can become temporarily unavailable | Removed from [[Service|Service]] endpoints |

## Probe Types

| Type | Implementation | When to Use |
|------|---------------|-------------|
| **httpGet** | HTTP GET request to path:port | Web servers, APIs with health endpoints |
| **tcpSocket** | TCP connection to port | TCP services without HTTP endpoints |
| **exec** | Execute command in container | Custom health logic, database connections |

## Configuration Parameters

| Parameter | Purpose | Guidance |
|-----------|---------|----------|
| `initialDelaySeconds` | Wait before first probe | Account for startup time |
| `periodSeconds` | Interval between probes | 5-10s for most workloads |
| `timeoutSeconds` | Probe response timeout | 1-5s; too short causes false failures |
| `successThreshold` | Consecutive successes to mark healthy | 1 for most; >1 for critical services |
| `failureThreshold` | Consecutive failures to trigger action | 3 for liveness; 3-5 for readiness |

## Production Template

For a typical web application with 30s startup time:
1. `startupProbe`: `initialDelaySeconds: 10, periodSeconds: 5, failureThreshold: 30` (allows 150s)
2. `livenessProbe`: `initialDelaySeconds: 0, periodSeconds: 10, timeoutSeconds: 5, failureThreshold: 3` (triggered after startupProbe succeeds)
3. `readinessProbe`: `initialDelaySeconds: 5, periodSeconds: 5, timeoutSeconds: 3, failureThreshold: 3`

## Common Pitfalls

- **Too aggressive**: Low failureThreshold or short period causes unnecessary restarts during load spikes
- **No startupProbe**: livenessProbe kills slow-starting apps before they finish initializing
- **liveness == readiness**: Use separate endpoints; readiness should check dependencies (database, cache), liveness should check process health
- **exec probes with heavy commands**: exec probes that take too long can delay startup or cause false failures

## Related

- [[skills/troubleshoot-pod-issues.md|troubleshoot-pod-issues]] — [[skills/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]
- [[pod-lifecycle|[[Pod Lifecycle|Pod Lifecycle]]]]
- [[deployment|Deployment]]
- [[skills/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]


<!-- risk-assessed -->
