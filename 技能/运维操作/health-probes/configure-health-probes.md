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

## 生产案例

### 案例 1: livenessProbe 过于敏感导致 Pod 频繁重启

| 时间 | 事件 |
|------|------|
| - | Pod 每 5min 重启一次，业务间歇性中断 |
| - | livenessProbe initialDelaySeconds=5，应用启动需 30s |
| - | 🟢 增加 initialDelaySeconds=60 + startupProbe |

**根因**: 应用启动慢但 livenessProbe 过早开始检测，导致启动期间被杀。

### 案例 2: readinessProbe 路径错误导致流量无法到达

**现象**: Pod Running 但 Service 无 Endpoints，流量 503。

**诊断**: readinessProbe path 为 /health，应用实际为 /healthz

**修复**: 🟢 修正 readinessProbe path

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 探针配置导致服务中断 | 修正探针配置 |
| P1 | Pod 频繁重启 | 调整 liveness 参数 |
| P2 | 探针优化 | 添加 startupProbe |

## 面试要点

1. **Q: 三种探针的作用和区别？**
   A: livenessProbe: 检测容器是否死锁，失败则重启；readinessProbe: 检测是否就绪接收流量，失败则从 Service 移除；startupProbe: 检测启动是否完成，完成前禁用其他探针。

2. **Q: 探针的四种检测方式？**
   A: httpGet(HTTP 状态码 200-399)、tcpSocket(端口可连接)、exec(命令退出码 0)、grpc(gRPC Health Check)。生产推荐 httpGet 或 grpc。

3. **Q: 探针参数调优建议？**
   A: initialDelaySeconds: 略大于启动时间；periodSeconds: 10s(不要太短)；timeoutSeconds: 3-5s；failureThreshold: 3-5；successThreshold: 1(liveness 必须为 1)。慢启动应用必配 startupProbe。

## Related

- [[技能/troubleshoot-pod-issues.md|troubleshoot-pod-issues]] — [[技能/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]
- [[pod-lifecycle|[[Pod Lifecycle|Pod Lifecycle]]]]
- [[deployment|Deployment]]
- [[技能/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]


<!-- risk-assessed -->
