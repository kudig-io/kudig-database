---
title: StatefulSet 异常故障树分析 (skills)
description: '- **范围**：有序部署、PVC 绑定、存储与网络、镜像与探针、控制器状态。'
category: general
tags:
- k8s
- etcd
- kubelet
- controller-manager
- statefulset
- rbac
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- StatefulSet 异常故障树分析 是什么
- 如何 StatefulSet 异常故障树分析
trigger_keywords:
- StatefulSet
- 异常故障树分析
prerequisites:
- kubectl-basics
- etcd-basics
fta_id: FTA-STATEFULSET-001
component: Statefulset
severity: critical
created: "2026-05-23"
---

---
title: "StatefulSet 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get [[Pods|pods]] -n <ns> -l app=<name> -o jsonpath='{range .items[?(@.status.phase!=\'Running\')]} {.metadata.name}{\'\n\'}{end}' 显示 StatefulSet Pod 非 Running --..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["domain-10-troubleshooting-diagnostics/topic-fta/list/statefulset-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# StatefulSet 异常故障树分析

<!-- condition: kubectl get pods -n <ns> -l app=<name> -o jsonpath='{range .items[?(@.status.phase!=\"Running\")]} {.metadata.name}{\"\n\"}{end}' 显示 StatefulSet Pod 非 Running -->

# StatefulSet 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 StatefulSet Pod 启动失败、序号错乱与持久化异常的关键成因与路径。
- **范围**：有序部署、PVC 绑定、存储与网络、镜像与探针、控制器状态。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: StatefulSet 异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> PVC[PVC/存储异常]
  OR0 --> POD[Pod 启动异常]
  OR0 --> ORD[有序部署异常]
  OR0 --> NET[网络/服务依赖异常]
  OR0 --> CTRL[控制器状态异常]

  PVC_OR{{OR}}
  PVC --> PVC_OR
  PVC_OR --> PVC1[PVC 绑定失败]
  PVC_OR --> PVC2[卷挂载失败/只读]
  PVC_OR --> PVC3[存储扩容失败]

  PVC1_OR{{OR}}
  PVC1 --> PVC1_OR
  PVC1_OR --> PVC1A[StorageClass 不存在]
  PVC1_OR --> PVC1B[PV 容量不足]
  PVC1_OR --> PVC1C[拓扑约束冲突]

  AND_PVC_BIND{{AND}}
  PVC1C --> AND_PVC_BIND
  AND_PVC_BIND --> PVC1C1[存储类不支持拓扑]
  AND_PVC_BIND --> PVC1C2[Pod 调度到错误可用区]

  PVC2_OR{{OR}}
  PVC2 --> PVC2_OR
  PVC2_OR --> PVC2A[CSI 驱动异常]
  PVC2_OR --> PVC2B[挂载权限错误]
  PVC2_OR --> PVC2C[卷损坏/只读]

  POD_OR{{OR}}
  POD --> POD_OR
  POD_OR --> POD1[镜像拉取失败]
  POD_OR --> POD2[探针失败]
  POD_OR --> POD3[CrashLoopBackOff]
  POD_OR --> POD4[Init 容器失败]

  POD1_OR{{OR}}
  POD1 --> POD1_OR
  POD1_OR --> POD1A[镜像不存在]
  POD1_OR --> POD1B[仓库认证失败]

  ORD_OR{{OR}}
  ORD --> ORD_OR
  ORD_OR --> ORD1[有序部署卡住]
  ORD_OR --> ORD2[RollingUpdate 分区策略异常]
  ORD_OR --> ORD3[Pod 管理策略错误]

  AND_ORDER{{AND}}
  ORD1 --> AND_ORDER
  AND_ORDER --> ORD1A[前序 Pod 未就绪]
  AND_ORDER --> ORD1B[OrderedReady 策略生效]

  ORD2_OR{{OR}}
  ORD2 --> ORD2_OR
  ORD2_OR --> ORD2A[partition 设置错误]
  ORD2_OR --> ORD2B[更新停滞在 partition]

  NET_OR{{OR}}
  NET --> NET_OR
  NET_OR --> NET1[Headless Service 配置错误]
  NET_OR --> NET2[DNS 解析异常]
  NET_OR --> NET3[Pod 间通信失败]

  NET1_OR{{OR}}
  NET1 --> NET1_OR
  NET1_OR --> NET1A[ClusterIP 设置非 None]
  NET1_OR --> NET1B[Selector 不匹配]

  CTRL_OR{{OR}}
  CTRL --> CTRL_OR
  CTRL_OR --> CTRL1[StatefulSet 控制器异常]
  CTRL_OR --> CTRL2[API Server 异常]
  CTRL_OR --> CTRL3[RBAC 权限不足]
```

---

## 生产级观测与证据
- **事件**：`FailedCreate`、`FailedMount`、`FailedScheduling`、`Unhealthy`、`ProvisioningFailed`。
- **关键指标**：`kube_statefulset_status_replicas`、`kube_statefulset_status_replicas_ready`、`kube_statefulset_status_replicas_current`、`kube_persistentvolumeclaim_status_phase`。
- **关键日志**：`kube-controller-manager`、`kubelet`、CSI 日志、etcd 日志（如果是 etcd StatefulSet）。
- **配置核对**：`volumeClaimTemplates`、滚动策略、Headless Service、资源请求、podManagementPolicy。

---

## JSON 工作流（含与/或门控件）

```json
{
  "flow_steps": [
    { "name": "开始", "action": "start", "step": "start_sts_fta", "next_step": "event_sts_abnormal" },
    { "name": "顶事件: StatefulSet 异常", "action": "event", "step": "event_sts_abnormal", "description": "Pod 未就绪/有序部署卡住/PVC 异常", "next_step": "gate_root_or" },
    { "name": "根因 OR 门", "action": "gate_or", "step": "gate_root_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["cat_pvc", "cat_pod", "cat_order", "cat_net",

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[ts-workloads|工作负载故障排查]]

## Related

- [[statefulset]] — StatefulSet
- [[kubelet]] — kubelet
- [[etcd]] — etcd
