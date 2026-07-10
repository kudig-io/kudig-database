---
title: 集群升级异常故障树分析 (skills)
description: NODE_KUBELET_OR --> NODE_KUBELET3[kubelet 配置不兼容]
summary: NODE_KUBELET_OR --> NODE_KUBELET3[kubelet 配置不兼容]
category: skills
tags:
- k8s
- fta
- troubleshooting
- etcd
- kubelet
- scheduler
- pdb
- daemonset
- crd
- webhook
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 集群升级异常故障树分析 是什么
- 如何 集群升级异常故障树分析
trigger_keywords:
- 集群升级异常故障树分析
prerequisites:
- kubectl-basics
- etcd-basics
fta_id: FTA-CLUSTER_UPGRADE-001
component: Cluster Upgrade
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 集群升级异常故障树分析

<!-- condition: kubectl get nodes -o jsonpath='{range .items[?(@.status.conditions[?(@.type=="Ready" && @.status!="True")].name)]} {.}{"\n"}{end}' 显示有 NotReady 节点 -->

# 集群升级异常 FTA 树

## 适用范围与说明
- **目标**：覆盖集群升级失败、版本不兼容与回滚失败的关键成因与路径。
- **范围**：控制面升级、节点升级、API 版本兼容、运行时/插件、回滚与审计。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: 集群升级异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> CP[控制面升级异常]
  OR0 --> NODE[节点升级异常]
  OR0 --> API[API 版本不兼容]
  OR0 --> PLUG[插件/运行时异常]
  OR0 --> ROLLBACK[回滚异常]
  OR0 --> AUDIT[审计与准备缺失]

  %% ========== 1. 控制面升级异常 ==========
  CP_OR{{OR}}
  CP --> CP_OR
  CP_OR --> CP_VER[版本异常]
  CP_OR --> CP_ETCD[etcd 升级异常]
  CP_OR --> CP_COMP[组件升级异常]
  CP_OR --> CP_CERT[证书异常]

  CP_VER_OR{{OR}}
  CP_VER --> CP_VER_OR
  CP_VER_OR --> CP_VER1[跨版本升级]
  CP_VER_OR --> CP_VER2[组件版本不一致]

  CP_ETCD_OR{{OR}}
  CP_ETCD --> CP_ETCD_OR
  CP_ETCD_OR --> CP_ETCD1[etcd 数据迁移失败]
  CP_ETCD_OR --> CP_ETCD2[etcd 集群不健康]
  CP_ETCD_OR --> CP_ETCD3[etcd Schema 变更不兼容]

  CP_COMP_OR{{OR}}
  CP_COMP --> CP_COMP_OR
  CP_COMP_OR --> CP_COMP1[API Server 升级失败]
  CP_COMP_OR --> CP_COMP2[Controller Manager 升级失败]
  CP_COMP_OR --> CP_COMP3[Scheduler 升级失败]

  CP_CERT_OR{{OR}}
  CP_CERT --> CP_CERT_OR
  CP_CERT_OR --> CP_CERT1[证书过期]
  CP_CERT_OR --> CP_CERT2[CA 不匹配]

  %% ========== 2. 节点升级异常 ==========
  NODE_OR{{OR}}
  NODE --> NODE_OR
  NODE_OR --> NODE_KUBELET[kubelet 升级异常]
  NODE_OR --> NODE_DRAIN[节点 Drain 异常]
  NODE_OR --> NODE_JOIN[节点重新加入异常]

  NODE_KUBELET_OR{{OR}}
  NODE_KUBELET --> NODE_KUBELET_OR
  NODE_KUBELET_OR --> NODE_KUBELET1[kubelet 版本不兼容]
  NODE_KUBELET_OR --> NODE_KUBELET2[kubelet 启动失败]
  NODE_KUBELET_OR --> NODE_KUBELET3[kubelet 配置不兼容]

  NODE_DRAIN_OR{{OR}}
  NODE_DRAIN --> NODE_DRAIN_OR
  NODE_DRAIN_OR --> NODE_DRAIN1[PDB 阻塞 Drain]
  NODE_DRAIN_OR --> NODE_DRAIN2[Pod 终止超时]
  NODE_DRAIN_OR --> NODE_DRAIN3[DaemonSet Pod 阻塞]

  NODE_JOIN_OR{{OR}}
  NODE_JOIN --> NODE_JOIN_OR
  NODE_JOIN_OR --> NODE_JOIN1[节点证书问题]
  NODE_JOIN_OR --> NODE_JOIN2[无法连接 API Server]

  %% AND 门：版本差异过大 + 无中间版本
  AND_VER{{"AND: 版本跨度过大"}}
  NODE_KUBELET --> AND_VER
  AND_VER --> AND_VER1[kubelet 与 API Server 版本差 > 2]
  AND_VER --> AND_VER2[未进行中间版本升级]

  %% ========== 3. API 版本不兼容 ==========
  API_OR{{OR}}
  API --> API_OR
  API_OR --> API_DEP[废弃 API 异常]
  API_OR --> API_CRD[CRD 异常]
  API_OR --> API_WEBHOOK[Webhook 异常]

  API_DEP_OR{{OR}}
  API_DEP --> API_DEP_OR
  API_DEP_OR --> API_DEP1[使用已移除 API]
  API_DEP_OR --> API_DEP2[API 迁移不完整]

  API_CRD_OR{{OR}}
  API_CRD --> API_CRD_OR
  API_CRD_OR --> API_CRD1[CRD 版本不兼容]
  API_CRD_OR --> API_CRD2[CRD 转换失败]

  %% AND 门：使用废弃 API + 未迁移
  AND_API{{"AND: 废弃 API 未迁移"}}
  API_DEP --> AND_API
  AND_API --> AND_API1[应用使用已废弃 API]
  AND_API --> AND_API2[升级前未完成迁移]

  %% ========== 4. 插件/运行时异常 ==========
  PLUG_OR{{OR}}
  PLUG --> PLUG_OR
  PLUG_OR --> PLUG_CNI[CNI 异常]
  PLUG_OR --> PLUG_CSI[CSI 异常]
  PLUG_OR --> PLUG_RT[容器运行时异常]

  PLUG_CNI_OR{{OR}}
  PLUG_CNI --> PLUG_CNI_OR
  PLUG_CN

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[skills/ts-storage.md|ts-storage]] — 存储故障排查
- [[skills/skill-19-node-resource-pressure.md|skill-19-node-resource-pressure]] — 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation
- [[certificate-fta]] — 证书异常故障树分析
- [[higress-fta]] — Higress 网关异常故障树分析
- [[skills/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]] — FTA-Driven Runbook Automation

- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/cluster-upgrade-fta.md|集群升级异常故障树分析]]
- [[skills/skill-README.md|topic-skills — 工单智能体 Kubernetes 诊断 Skill 库]] — Cross-reference
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]


<!-- risk-assessed -->
