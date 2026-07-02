---
title: API Server 异常故障树分析 (skills)
description: '- **范围**：APIServer 进程与配置、认证鉴权、请求排队与限流、依赖组件、证书与时间、网络与基础设施。'
summary: '- **范围**：APIServer 进程与配置、认证鉴权、请求排队与限流、依赖组件、证书与时间、网络与基础设施。'
category: skills
tags:
- k8s
- fta
- troubleshooting
- etcd
- apiserver
- ingress
- rbac
- webhook
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- API Server 异常故障树分析 是什么
- 如何 API Server 异常故障树分析
trigger_keywords:
- API
- Server
- 异常故障树分析
prerequisites:
- kubectl-basics
- etcd-basics
fta_id: FTA-APISERVER-001
component: Apiserver
severity: high
---



# API Server 异常故障树分析

<!-- condition: kubectl get --raw /healthz 返回非 200 或 kubectl get [[Pods|pods]] -n kube-system -l component=kube-apiserver 显示非 Running -->

# API Server 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 [[domain-17-system-foundation/topic-dictionary/fundamentals/the-kubernetes-api.md|Kubernetes API]] Server 不可用/性能劣化的关键成因与路径，支撑生产环境快速定位与自动化处置。
- **范围**：APIServer 进程与配置、认证鉴权、请求排队与限流、依赖组件、证书与时间、网络与基础设施。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: API Server 不可用/性能劣化]
  OR0{{OR}}
  TE --> OR0

  OR0 --> PROC[进程与资源异常]
  OR0 --> AUTH[认证与鉴权异常]
  OR0 --> RATE[请求排队/限流异常]
  OR0 --> DEP[依赖与存储异常]
  OR0 --> NET[网络与连通性异常]
  OR0 --> CERT[证书与时间异常]
  OR0 --> CFG[配置与发布异常]

  %% 进程与资源异常分支 - 扩展到3-4层
  PROC_OR{{OR}}
  PROC --> PROC_OR
  PROC_OR --> PROC1[进程崩溃/反复重启]
  PROC_OR --> PROC2[CPU/内存资源耗尽]
  PROC_OR --> PROC3[GC/长尾阻塞]

  PROC1_OR{{OR}}
  PROC1 --> PROC1_OR
  PROC1_OR --> PROC1A[OOMKilled]
  PROC1_OR --> PROC1B[探针失败重启]
  PROC1_OR --> PROC1C[panic 崩溃]

  PROC2_OR{{OR}}
  PROC2 --> PROC2_OR
  PROC2_OR --> PROC2A[CPU 限流]
  PROC2_OR --> PROC2B[内存接近 limits]
  PROC2_OR --> PROC2C[控制面节点资源不足]

  PROC3_OR{{OR}}
  PROC3 --> PROC3_OR
  PROC3_OR --> PROC3A[GC STW 过长]
  PROC3_OR --> PROC3B[goroutine 泄漏]
  PROC3_OR --> PROC3C[大请求阻塞]

  %% 认证与鉴权异常分支 - 扩展到3-4层 + AND 门
  AUTH_OR{{OR}}
  AUTH --> AUTH_OR
  AUTH_OR --> AUTH1[身份认证失败]
  AUTH_OR --> AUTH2[授权/鉴权失败]
  AUTH_OR --> AUTH3[准入控制失败]

  AUTH1_OR{{OR}}
  AUTH1 --> AUTH1_OR
  AUTH1_OR --> AUTH1A[OIDC Provider 不可用]
  AUTH1_OR --> AUTH1B[Token 过期/无效]
  AUTH1_OR --> AUTH1C[ServiceAccount Token 问题]

  AUTH2_OR{{OR}}
  AUTH2 --> AUTH2_OR
  AUTH2_OR --> AUTH2A[RBAC 策略拒绝]
  AUTH2_OR --> AUTH2B[Webhook 鉴权超时]

  AUTH3_AND{{AND}}
  AUTH3 --> AUTH3_AND
  AUTH3_AND --> AUTH3A[Webhook 准入不可用]
  AUTH3_AND --> AUTH3B[failurePolicy 为 Fail]

  %% 请求排队/限流异常分支 - 扩展到3-4层 + AND 门
  RATE_OR{{OR}}
  RATE --> RATE_OR
  RATE_OR --> RATE1[APF 限流触发]
  RATE_OR --> RATE2[请求队列积压]
  RATE_OR --> RATE3[特定请求类型问题]

  RATE1_AND{{AND}}
  RATE1 --> RATE1_AND
  RATE1_AND --> RATE1A[请求量突增]
  RATE1_AND --> RATE1B[FlowSchema 配置过严]

  RATE2_OR{{OR}}
  RATE2 --> RATE2_OR
  RATE2_OR --> RATE2A[max-requests-inflight 达限]
  RATE2_OR --> RATE2B[etcd 响应慢导致积压]

  RATE3_OR{{OR}}
  RATE3 --> RATE3_OR
  RATE3_OR --> RATE3A[大量 list 请求]
  RATE3_OR --> RATE3B[watch 风暴]
  RATE3_OR --> RATE3C[高频 create/update]

  %% 依赖与存储异常分支 - 扩展到3-4层
  DEP_OR{{OR}}
  DEP --> DEP_OR
  DEP_OR --> DEP1[etcd 异常]
  DEP_OR --> DEP2[聚合 API 异常]
  DEP_OR --> DEP3[控制面基础设施异常]

  DEP1_OR{{OR}}
  DEP1 --> DEP1_OR
  DEP1_OR --> DEP1A[etcd 不可用]
  DEP1_OR --> DEP1B[etcd 延迟高]
  DEP1_OR --> DEP1C[etcd 空间不足]

  DEP2_OR{{OR}}
  DEP2 --> DEP2_OR
  DEP2_OR --> DEP2A[APIService 不可用]
  DEP2_OR --> DEP2B[聚合后端超时]

  DEP3_OR{{OR}}
  DEP3 --> DEP3_OR
  DEP3_OR --> DEP3A[控制面节点宕机]
  DEP3_OR --> DEP3B[控制面资源竞争]

  %% 网络与连通性异常分支 - 扩展到3-4层
  NET_OR{{OR}}
  NET --> NET_OR
  NET_OR --> NET1[LB/入口问题]
  NET_OR --> NET2[网络链路问题]
  NET_OR --> NET3[DNS 问题]

  NET1_OR{{OR}}
  NET1 --> NET1_OR
  NET1_OR --> NET1A[LB 健康检查失败]
  NET1_OR --> NET1B[LB 后端权重异常]
  NET1_OR --> NET1C

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[skills/ts-control-plane.md|控制平面故障排查]]

## Related

- [[skills/assessment-k8s-fundamentals-quiz.md|assessment-k8s-fundamentals-quiz]] — K8S Fundamentals Quiz
- [[skills/ts-cloud-provider.md|ts-cloud-provider]] — 云服务商集成排查
- [[skills/ts-node-components.md|ts-node-components]] — 节点组件故障排查
- [[entities/kube-apiserver.md|kube-apiserver]] — kube-apiserver
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[nginx-ingress-fta]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[skills/assessment-k8s-fundamentals-quiz-answers.md|K8S Fundamentals Quiz Answers]] — Cross-reference
