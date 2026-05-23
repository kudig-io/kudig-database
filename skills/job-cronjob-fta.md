---
title: Job/CronJob 异常故障树分析 (skills)
description: '- **范围**：调度触发、并发与重试策略、镜像与探针、资源与配额、控制器依赖。'
category: skills
tags:
- k8s
- fta
- troubleshooting
- etcd
- job
- cronjob
- webhook
- gpu
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Job/CronJob 异常故障树分析 是什么
- 如何 Job/CronJob 异常故障树分析
trigger_keywords:
- Job
- CronJob
- 异常故障树分析
prerequisites:
- kubectl-basics
- etcd-basics
- gpu-scheduling-basics
fta_id: FTA-JOB_CRONJOB-001
component: Job Cronjob
severity: critical
created: "2026-05-23"
---

# Job/CronJob 异常故障树分析

<!-- condition: kubectl get [[Jobs|jobs]] -A -o jsonpath='{range .items[?(@.status.failed>0)]}{.metadata.name}{"\t"}{.status.failed}{"\n"}{end}' 显示有 Failed Job -->

# Job/CronJob 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 Job/CronJob 执行失败、未触发与重复执行的关键成因与路径。
- **范围**：调度触发、并发与重试策略、镜像与探针、资源与配额、控制器依赖。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: Job/CronJob 异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> TRIG[触发/调度异常]
  OR0 --> POD[Pod 运行异常]
  OR0 --> RETRY[重试与并发异常]
  OR0 --> RES[资源与配额异常]
  OR0 --> CP[控制面依赖异常]

  %% 触发/调度异常分支 - 扩展到3-4层
  TRIG_OR{{OR}}
  TRIG --> TRIG_OR
  TRIG_OR --> TRIG1[CronJob 未触发]
  TRIG_OR --> TRIG2[调度时间问题]
  TRIG_OR --> TRIG3[Job 创建失败]

  TRIG1_OR{{OR}}
  TRIG1 --> TRIG1_OR
  TRIG1_OR --> TRIG1A[suspend 设置为 true]
  TRIG1_OR --> TRIG1B[schedule 表达式错误]
  TRIG1_OR --> TRIG1C[startingDeadlineSeconds 过期]

  TRIG2_OR{{OR}}
  TRIG2 --> TRIG2_OR
  TRIG2_OR --> TRIG2A[时区配置错误]
  TRIG2_OR --> TRIG2B[节点时间漂移]
  TRIG2_OR --> TRIG2C[控制器时钟不同步]

  TRIG3_OR{{OR}}
  TRIG3 --> TRIG3_OR
  TRIG3_OR --> TRIG3A[Job 模板配置错误]
  TRIG3_OR --> TRIG3B[Webhook 拦截失败]

  %% Pod 运行异常分支 - 扩展到3-4层
  POD_OR{{OR}}
  POD --> POD_OR
  POD_OR --> POD1[镜像拉取失败]
  POD_OR --> POD2[容器启动失败]
  POD_OR --> POD3[运行时错误]

  POD1_OR{{OR}}
  POD1 --> POD1_OR
  POD1_OR --> POD1A[ImagePullBackOff]
  POD1_OR --> POD1B[私有仓库认证失败]
  POD1_OR --> POD1C[镜像不存在]

  POD2_OR{{OR}}
  POD2 --> POD2_OR
  POD2_OR --> POD2A[CrashLoopBackOff]
  POD2_OR --> POD2B[配置/Secret 缺失]
  POD2_OR --> POD2C[权限不足]

  POD3_OR{{OR}}
  POD3 --> POD3_OR
  POD3_OR --> POD3A[任务逻辑错误退出]
  POD3_OR --> POD3B[OOMKilled]
  POD3_OR --> POD3C[超时被终止]

  %% 重试与并发异常分支 - 扩展到3-4层 + AND 门
  RETRY_OR{{OR}}
  RETRY --> RETRY_OR
  RETRY_OR --> RETRY1[重试策略问题]
  RETRY_OR --> RETRY2[并发策略问题]
  RETRY_OR --> RETRY3[历史 Job 积压]

  RETRY1_AND{{AND}}
  RETRY1 --> RETRY1_AND
  RETRY1_AND --> RETRY1A[任务持续失败]
  RETRY1_AND --> RETRY1B[backoffLimit 已达到]

  RETRY2_OR{{OR}}
  RETRY2 --> RETRY2_OR
  RETRY2_OR --> RETRY2A[Allow 导致重复运行]
  RETRY2_OR --> RETRY2B[Forbid 阻塞新任务]
  RETRY2_OR --> RETRY2C[Replace 导致任务丢失]

  RETRY3_OR{{OR}}
  RETRY3 --> RETRY3_OR
  RETRY3_OR --> RETRY3A[successfulJobsHistoryLimit 过大]
  RETRY3_OR --> RETRY3B[failedJobsHistoryLimit 过大]

  %% 资源与配额异常分支 - 扩展到3-4层
  RES_OR{{OR}}
  RES --> RES_OR
  RES_OR --> RES1[调度资源不足]
  RES_OR --> RES2[配额限制]
  RES_OR --> RES3[节点选择失败]

  RES1_OR{{OR}}
  RES1 --> RES1_OR
  RES1_OR --> RES1A[CPU/内存不足]
  RES1_OR --> RES1B[GPU 资源不足]
  RES1_OR --> RES1C[本地存储不足]

  RES2_OR{{OR}}
  RES2 --> RES2_OR
  RES2_OR --> RES2A[namespace 配额耗尽]
  RES2_OR --> RES2B[Pod 数量限制]

  RES3_OR{{OR}}
  RES3 --> RES3_OR
  RES3_OR --> RES3A[nodeSelector 不匹配]
  RES3_OR --> RES3B[污点容忍缺失]

  %% 控制面依赖异常分支 - 扩展到3-4层 + AND 门
  CP_OR{{OR}}
  CP --> CP_OR
  CP_OR --> CP1[API Server 问题]
  CP_OR --> CP2[控制器问题]
  CP_OR --> CP3[etcd 问题]

  CP1_OR{{OR}}
  CP1 --> CP1_OR
  CP1_OR --> CP1A[API Server 不可用]
  CP1_OR --> CP1B[请求被限流]

  CP2_AND{{AND}}
  CP2 --> CP2_AND
  CP2_AND --> CP2A[Job 控制器异

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[skills/skill-19-node-resource-pressure.md|skill-19-node-resource-pressure]] — 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation
- [[certificate-fta]] — 证书异常故障树分析
- [[higress-fta]] — Higress 网关异常故障树分析
- [[skills/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]] — FTA-Driven Runbook Automation
- [[cluster-upgrade-fta]] — 集群升级异常故障树分析

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/job-cronjob-fta.md|Job/CronJob 异常故障树分析]]
- [[skills/skill-README|topic-skills — 工单智能体 Kubernetes 诊断 Skill 库]] — Cross-reference
