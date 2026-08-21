---
title: Topic Multi Fault Scenarios
description: Topic Multi Fault Scenarios 目录索引
summary: Topic Multi Fault Scenarios 目录索引
category: index
tags:
- index
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
---


# Topic Multi Fault Scenarios

> 本页为 `19-故障诊断/09-多故障场景` 目录的自动索引。  <!-- N3: 旧短路径 topic-multi-fault-scenarios 更新 -->

## 文档

- [[19-故障诊断/09-多故障场景/MULTI-001-节点NotReady-plus-DNS解析失败并发.md|Multi 001 节点Notready Plus Dns解析失败并发]]
- [[19-故障诊断/09-多故障场景/MULTI-002-Pod-CrashLoop-plus-镜像拉取失败并发.md|Multi 002 Pod Crashloop Plus 镜像拉取失败并发]]
- [[19-故障诊断/09-多故障场景/MULTI-003-证书过期-plus-控制平面组件崩溃并发.md|Multi 003 证书过期 Plus 控制平面组件崩溃并发]]
- [[19-故障诊断/09-多故障场景/MULTI-004-HPA不扩容-plus-节点资源压力并发.md|Multi 004 Hpa不扩容 Plus 节点资源压力并发]]
- [[19-故障诊断/09-多故障场景/MULTI-005-Ingress-502-plus-Service-Endpoint为空并发.md|Multi 005 Ingress 502 Plus Service Endpoint为空并发]]
- [[19-故障诊断/09-多故障场景/MULTI-006-RBAC权限不足-plus-镜像拉取失败并发.md|Multi 006 Rbac权限不足 Plus 镜像拉取失败并发]]
- [[19-故障诊断/09-多故障场景/MULTI-007-StatefulSet-PVC未绑定-plus-节点NotReady并发.md|Multi 007 Statefulset Pvc未绑定 Plus 节点Notready并发]]
- [[19-故障诊断/09-多故障场景/MULTI-008-Prometheus-OOM-plus-日志采集中断并发.md|Multi 008 Prometheus Oom Plus 日志采集中断并发]]
- [[19-故障诊断/09-多故障场景/MULTI-009-NetworkPolicy阻断-plus-DNS解析失败并发.md|Multi 009 Networkpolicy阻断 Plus Dns解析失败并发]]
- [[19-故障诊断/09-多故障场景/MULTI-010-集群升级失败-plus-证书过期并发.md|Multi 010 集群升级失败 Plus 证书过期并发]]

---

## 方法论衔接  <!-- M3: 新增 FTA 衔接小节 -->

本目录的并发/级联故障场景与 [FTA 故障树](../06-FTA故障树/README.md) 的以下能力直接对应：

| 多故障场景 | FTA 支撑点 | 关联文档 |
|:---|:---|:---|
| 级联故障（A 故障触发 B 故障） | AND 门 / 传递符号（transfer）表达事件链 | [第三章：FTA 符号体系](../06-FTA故障树/03-fta-symbol-system-and-standards.md)、[第四章：FTA 方法论核心原则](../06-FTA故障树/04-fta-core-principles.md) |
| 共因失效（同一根因多个表现） | 共因失效（Common Cause Failure）建模 | [第四章：FTA 方法论核心原则](../06-FTA故障树/04-fta-core-principles.md)、[glossary/common-cause-failure](../06-FTA故障树/glossary/common-cause-failure.md) |
| 复合问题（多组件同时异常） | 多棵故障树组合 / 割集分析 | [kubernetes-fta-full-analysis-v2](../06-FTA故障树/kubernetes-fta-full-analysis-v2.md)（16 顶事件覆盖）、[fta-index](../06-FTA故障树/fta-index.md) |

> 使用方法：对每个 MULTI-xxx 场景，先按 [配置优先方法论](../04-高级排障/structural-00-configuration-first-methodology.md) 排序排查顺序，再分别沿各故障分支的 FTA 树定位根因。

---

