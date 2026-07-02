---
title: 证书异常故障树分析 (skills)
description: ROT_AUTO_OR --> ROT_AUTO2[轮换触发阈值配置错误]
summary: ROT_AUTO_OR --> ROT_AUTO2[轮换触发阈值配置错误]
category: skills
tags:
- k8s
- fta
- troubleshooting
- etcd
- kubelet
- scheduler
- webhook
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 证书异常故障树分析 是什么
- 如何 证书异常故障树分析
trigger_keywords:
- 证书异常故障树分析
prerequisites:
- kubectl-basics
- etcd-basics
- tls-basics
fta_id: FTA-CERTIFICATE-001
component: Certificate
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 证书异常故障树分析

<!-- condition: kubeadm certs check-expiration | grep -E 'EXPIRES|expired' 显示证书即将过期或已过期 -->

# 证书异常 FTA 树

## 适用范围与说明
- **目标**：覆盖证书过期、链不完整与轮换失败的关键成因与路径。
- **范围**：控制面证书、节点证书、Webhook 证书、时间同步、更新流程。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: 证书异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> EXP[证书过期]
  OR0 --> ROT[轮换失败]
  OR0 --> CHAIN[证书链异常]
  OR0 --> TIME[时间同步异常]
  OR0 --> DEP[依赖组件异常]
  OR0 --> AUDIT[审计与回滚缺失]

  %% ========== 1. 证书过期 ==========
  EXP_OR{{OR}}
  EXP --> EXP_OR
  EXP_OR --> EXP_CP[控制面证书过期]
  EXP_OR --> EXP_NODE[节点证书过期]
  EXP_OR --> EXP_WH[Webhook/扩展证书过期]

  %% 1.1 控制面证书过期
  EXP_CP_OR{{OR}}
  EXP_CP --> EXP_CP_OR
  EXP_CP_OR --> EXP_CP1[API Server 证书过期]
  EXP_CP_OR --> EXP_CP2[etcd 证书过期]
  EXP_CP_OR --> EXP_CP3[Controller Manager 证书过期]
  EXP_CP_OR --> EXP_CP4[Scheduler 证书过期]

  %% 1.2 节点证书过期
  EXP_NODE_OR{{OR}}
  EXP_NODE --> EXP_NODE_OR
  EXP_NODE_OR --> EXP_NODE1[kubelet 客户端证书过期]
  EXP_NODE_OR --> EXP_NODE2[kubelet 服务端证书过期]
  EXP_NODE_OR --> EXP_NODE3[kube-proxy 证书过期]

  %% 1.3 Webhook/扩展证书过期
  EXP_WH_OR{{OR}}
  EXP_WH --> EXP_WH_OR
  EXP_WH_OR --> EXP_WH1[Admission Webhook 证书过期]
  EXP_WH_OR --> EXP_WH2[API 聚合层证书过期]
  EXP_WH_OR --> EXP_WH3[cert-manager 自签证书过期]

  %% ========== 2. 轮换失败 ==========
  ROT_OR{{OR}}
  ROT --> ROT_OR
  ROT_OR --> ROT_AUTO[自动轮换异常]
  ROT_OR --> ROT_MANUAL[人工轮换异常]
  ROT_OR --> ROT_CM[cert-manager 异常]

  %% 2.1 自动轮换异常
  ROT_AUTO_OR{{OR}}
  ROT_AUTO --> ROT_AUTO_OR
  ROT_AUTO_OR --> ROT_AUTO1[kubelet 轮换未启用]
  ROT_AUTO_OR --> ROT_AUTO2[轮换触发阈值配置错误]
  ROT_AUTO_OR --> ROT_AUTO3[CSR 审批失败]

  %% AND 门：kubelet 轮换未启用 + 证书有效期短
  AND_KUBELET{{"AND: 轮换未启用 + 有效期短"}}
  ROT_AUTO1 --> AND_KUBELET
  AND_KUBELET --> AND_KUBELET1[rotateCertificates 未开启]
  AND_KUBELET --> AND_KUBELET2[证书有效期 < 30 天]

  %% 2.2 人工轮换异常
  ROT_MANUAL_OR{{OR}}
  ROT_MANUAL --> ROT_MANUAL_OR
  ROT_MANUAL_OR --> ROT_MANUAL1[kubeadm 轮换命令失败]
  ROT_MANUAL_OR --> ROT_MANUAL2[证书分发不完整]
  ROT_MANUAL_OR --> ROT_MANUAL3[组件未重启加载新证书]

  %% 2.3 cert-manager 异常
  ROT_CM_OR{{OR}}
  ROT_CM --> ROT_CM_OR
  ROT_CM_OR --> ROT_CM1[cert-manager Pod 不可用]
  ROT_CM_OR --> ROT_CM2[Issuer/ClusterIssuer 配置错误]
  ROT_CM_OR --> ROT_CM3[Certificate CR 状态异常]

  %% ========== 3. 证书链异常 ==========
  CHAIN_OR{{OR}}
  CHAIN --> CHAIN_OR
  CHAIN_OR --> CHAIN_INTER[中间证书异常]
  CHAIN_OR --> CHAIN_ROOT[根证书异常]
  CHAIN_OR --> CHAIN_MISMATCH[证书链不匹配]

  %% 3.1 中间证书异常
  CHAIN_INTER_OR{{OR}}
  CHAIN_INTER --> CHAIN_INTER_OR
  CHAIN_INTER_OR --> CHAIN_INTER1[中间证书缺失]
  CHAIN_INTER_OR --> CHAIN_INTER2[中间证书过期]
  CHAIN_INTER_OR --> CHAIN_INTER3[中间证书顺序错误]

  %% 3.2 根证书异常
  CHAIN_ROOT_OR{{OR}}
  CHAIN_ROOT --> CHAIN_ROOT_OR
  CHAIN_ROOT_OR --> CHAIN_ROOT1[根证书变更未同步]
  CHAIN_ROOT_OR --> CHAIN_ROOT2[根证书不受信任]
  CHAIN_ROOT_OR --> CHAIN_ROOT3[CA 证书过期]

  %% 3.3 证书链不匹配
  CHAIN_MISMATCH_OR{{OR}}
  CHAIN_MISMATCH --> CHAIN_MISMATCH_OR
  CHAIN_MISMATCH_OR --> CHAIN_MISMATCH1[私钥与证书不匹配]
  CHAIN_MISMATCH_OR --> CHAIN_MISMATCH2[证书 SAN 不包含当前域名]
  CHA

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[skills/ts-security-auth.md|安全认证排查]]

## Related

- [[skills/ts-cluster-operations.md|ts-cluster-operations]] — 集群运维故障排查
- [[skills/ts-storage.md|ts-storage]] — 存储故障排查
- [[skills/skill-19-node-resource-pressure.md|skill-19-node-resource-pressure]] — 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation
- [[entities/kubelet.md|kubelet]] — kubelet
- [[cert-manager]] — cert-manager

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/certificate-fta.md|证书异常故障树分析]]

<!-- risk-assessed -->
