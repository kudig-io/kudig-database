---
title: HPA 异常故障树分析
description: ALG_OR --> ALG1[阈值配置不当]
category: fta
tags:
- fta
- troubleshooting
- hpa
- horizontal-pod-autoscaler
- metrics-server
- apiserver
- kubelet
- controller-manager
- prometheus
- pdb
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- HPA 异常故障树分析 是什么
- 如何 HPA 异常故障树分析
- HPA 异常故障树分析 根因分析
- HPA 异常故障树分析 故障树
trigger_keywords:
- HPA
- 异常故障树分析
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
fta_id: FTA-HPA-001
component: Hpa
severity: critical
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-02-workloads-applications/21-hpa-vpa-autoscaling.md
  label: '深度文档: 21-hpa-vpa-autoscaling'
---

<!-- condition: kubectl get hpa -A -o jsonpath='{range .items[?(@.status.currentReplicas != @.status.desiredReplicas)]} {.metadata.namespace}/{.metadata.name}{\"\n\"}{end}' 显示副本数不匹配 -->

# HPA 异常 FTA 树

## 适用范围与说明
- **目标**：覆盖 HPA 扩缩容失效、指标不可用与震荡的关键成因与路径。
- **范围**：指标采集、算法策略、目标对象状态、资源与配额、控制面依赖。
- **符号**：
  - **OR 门**：任一子事件成立即可触发父事件
  - **AND 门**：所有子事件同时成立才触发父事件

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE[顶事件: HPA 扩缩容异常]
  OR0{{OR}}
  TE --> OR0

  OR0 --> MET[指标不可用/不准确]
  OR0 --> ALG[算法与策略异常]
  OR0 --> OBJ[目标对象异常]
  OR0 --> QUO[配额与容量限制]
  OR0 --> CP[控制面依赖异常]

  MET_OR{{OR}}
  MET --> MET_OR
  MET_OR --> MET1[Metrics Server 异常]
  MET_OR --> MET2[自定义指标采集失败]
  MET_OR --> MET3[指标延迟/过期]

  MET1_OR{{OR}}
  MET1 --> MET1_OR
  MET1_OR --> MET1A[Metrics Server [[concepts/pod-lifecycle|pod]] 异常]
  MET1_OR --> MET1B[API 注册失败]
  MET1_OR --> MET1C[kubelet 指标采集失败]

  MET2_OR{{OR}}
  MET2 --> MET2_OR
  MET2_OR --> MET2A[Prometheus Adapter 异常]
  MET2_OR --> MET2B[外部指标源不可达]
  MET2_OR --> MET2C[指标名称/标签不匹配]

  ALG_OR{{OR}}
  ALG --> ALG_OR
  ALG_OR --> ALG1[阈值配置不当]
  ALG_OR --> ALG2[冷却窗口设置不合理]
  ALG_OR --> ALG3[副本震荡]
  ALG_OR --> ALG4[扩容卡住]

  ALG1_OR{{OR}}
  ALG1 --> ALG1_OR
  ALG1_OR --> ALG1A[目标值过高/过低]
  ALG1_OR --> ALG1B[指标类型选择错误]

  AND_OSCILLATION{{AND}}
  ALG3 --> AND_OSCILLATION
  AND_OSCILLATION --> ALG3A[阈值设置过敏感]
  AND_OSCILLATION --> ALG3B[冷却窗口过短]

  AND_STUCK{{AND}}
  ALG4 --> AND_STUCK
  AND_STUCK --> ALG4A[指标持续不可用]
  AND_STUCK --> ALG4B[已达 maxReplicas]

  OBJ_OR{{OR}}
  OBJ --> OBJ_OR
  OBJ_OR --> OBJ1[目标资源不存在]
  OBJ_OR --> OBJ2[副本状态不收敛]
  OBJ_OR --> OBJ3[目标资源 Scale 子资源异常]

  OBJ2_OR{{OR}}
  OBJ2 --> OBJ2_OR
  OBJ2_OR --> OBJ2A[新 Pod 启动失败]
  OBJ2_OR --> OBJ2B[旧 Pod 无法终止]
  OBJ2_OR --> OBJ2C[副本数与期望不一致]

  QUO_OR{{OR}}
  QUO --> QUO_OR
  QUO_OR --> QUO1[资源配额限制]
  QUO_OR --> QUO2[节点资源不足]
  QUO_OR --> QUO3[PDB 阻止缩容]

  QUO1_OR{{OR}}
  QUO1 --> QUO1_OR
  QUO1_OR --> QUO1A[命名空间 CPU/内存配额用尽]
  QUO1_OR --> QUO1B[Pod 数量超过限制]

  QUO2_OR{{OR}}
  QUO2 --> QUO2_OR
  QUO2_OR --> QUO2A[可调度节点资源不足]
  QUO2_OR --> QUO2B[Cluster Autoscaler 未能扩展]

  CP_OR{{OR}}
  CP --> CP_OR
  CP_OR --> CP1[API Server 异常]
  CP_OR --> CP2[HPA 控制器异常]
  CP_OR --> CP3[RBAC 权限不足]

  CP2_OR{{OR}}
  CP2 --> CP2_OR
  CP2_OR --> CP2A[控制器进程异常]
  CP2_OR --> CP2B[控制循环卡死]
  CP2_OR --> CP2C[同步周期过长]
```

---

## 生产级观测与证据
- **事件**：`FailedGetResourceMetric`、`FailedComputeMetricsReplicas`、`FailedRescale`、`SuccessfulRescale`。
- **关键指标**：`kube_hpa_status_current_replicas`、`kube_hpa_status_desired_replicas`、`kube_hpa_spec_min_replicas`、`kube_hpa_spec_max_replicas`、`kube_hpa_status_condition`。
- **关键日志**：`kube-controller-manager`、`metrics-server`、`prometheus-adapter`、自定义指标适配器日志。
- **配置核对**：目标资源、`min/maxReplicas`、指标阈值、`stabilizationWindowSeconds`、`behavior` 策略。

---

## JSON 工作流（含与/或门控件）

```json
{
  "flow_steps": [
    { "name": "开始", "action": "start", "step": "start_hpa_fta", "next_step": "event_hpa_abnormal" },
    { "name": "顶事件: HPA 扩缩容异常", "action": "event", "step": "event_hpa_abnormal", "description": "扩缩容停滞/震荡/失败", "next_step": "gate_root_or" },
    { "name": "根因 OR 门", "action": "gate_or", "step": "gate_root_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["cat_metrics", "cat_alg", "cat_obj", "cat_quota", "cat_cp"] },

    { "name": "指标不可用/不准确", "action": "event", "step": "cat_metrics", "description": "HPA 无法获取指标", "next_step": "gate_metrics_or" },
    { "name": "指标 OR 门", "action": "gate_or", "step": "gate_metrics_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_metrics_server", "evt_custom_metrics", "evt_metrics_delay"] },

    { "name": "Metrics Server 异常", "action": "event", "step": "evt_metrics_server", "description": "核心指标不可用", "next_step": "gate_metrics_server_or" },
    { "name": "Metrics Server OR 门", "action": "gate_or", "step": "gate_metrics_server_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_ms_pod_fail", "evt_ms_api_fail", "evt_kubelet_fail"] },
    {
      "name": "Metrics Server Pod 异常",
      "action": "event",
      "step": "evt_ms_pod_fail",
      "severity": "critical",
      "probability": "medium",
      "mttr_minutes": 15,
      "detection": {
        "events": ["OOMKilled", "CrashLoopBackOff"],
        "metrics": ["kube_pod_container_status_ready{pod=~'metrics-server.*'}"],
        "logs": ["metrics-server: error"]
      },
      "remediation": {
        "manual_steps": ["检查 metrics-server Pod 状态", "验证资源配置"],
        "auto_actions": ["重启 metrics-server Deployment"]
      }
    },
    {
      "name": "API 注册失败",
      "action": "event",
      "step": "evt_ms_api_fail",
      "severity": "critical",
      "probability": "medium",
      "mttr_minutes": 20,
      "detection": {
        "events": ["FailedDiscoveryCheck"],
        "metrics": [],
        "logs": ["failed to discover metrics API"]
      },
      "remediation": {
        "manual_steps": ["检查 APIService 状态", "验证 metrics-server 证书"],
        "auto_actions": ["重建 APIService"]
      }
    },
    {
      "name": "kubelet 指标采集失败",
      "action": "event",
      "step": "evt_kubelet_fail",
      "severity": "high",
      "probability": "medium",
      "mttr_minutes": 15,
      "detection": {
        "events": [],
        "metrics": [],
        "logs": ["metrics-server: unable to fetch metrics from kubelet"]
      },
      "remediation": {
        "manual_steps": ["检查 kubelet 服务状态", "验证网络连通性"],
        "auto_actions": ["重启 kubelet"]
      }
    },

    { "name": "自定义指标采集失败", "action": "event", "step": "evt_custom_metrics", "description": "自定义/外部指标不可用", "next_step": "gate_custom_metrics_or" },
    { "name": "自定义指标 OR 门", "action": "gate_or", "step": "gate_custom_metrics_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_adapter_fail", "evt_external_fail", "evt_metric_mismatch"] },
    {
      "name": "Prometheus Adapter 异常",
      "action": "event",
      "step": "evt_adapter_fail",
      "severity": "high",
      "probability": "medium",
      "mttr_minutes": 15,
      "detection": {
        "events": [],
        "metrics": ["kube_pod_container_status_ready{pod=~'prometheus-adapter.*'}"],
        "logs": ["prometheus-adapter: error"]
      },
      "remediation": {
        "manual_steps": ["检查 prometheus-adapter Pod 状态", "验证配置规则"],
        "auto_actions": ["重启 prometheus-adapter"]
      }
    },
    {
      "name": "外部指标源不可达",
      "action": "event",
      "step": "evt_external_fail",
      "severity": "high",
      "probability": "medium",
      "mttr_minutes": 20,
      "detection": {
        "events": ["FailedGetExternalMetric"],
        "metrics": [],
        "logs": ["unable to get external metric"]
      },
      "remediation": {
        "manual_steps": ["检查外部指标服务状态", "验证网络连通性"],
        "auto_actions": ["检查外部服务健康"]
      }
    },
    {
      "name": "指标名称/标签不匹配",
      "action": "event",
      "step": "evt_metric_mismatch",
      "severity": "medium",
      "probability": "common",
      "mttr_minutes": 10,
      "detection": {
        "events": ["FailedGetResourceMetric"],
        "metrics": [],
        "logs": ["no metrics returned matching"]
      },
      "remediation": {
        "manual_steps": ["检查 HPA 指标配置", "验证指标名称和选择器"],
        "auto_actions": ["修正指标配置"]
      }
    },

    {
      "name": "指标延迟/过期",
      "action": "event",
      "step": "evt_metrics_delay",
      "severity": "medium",
      "probability": "common",
      "mttr_minutes": 15,
      "detection": {
        "events": [],
        "metrics": ["scrape_duration_seconds"],
        "logs": ["metrics are stale"]
      },
      "remediation": {
        "manual_steps": ["检查指标采集延迟", "优化采集配置"],
        "auto_actions": ["调整采集间隔"]
      }
    },

    { "name": "算法与策略异常", "action": "event", "step": "cat_alg", "description": "扩缩容策略配置问题", "next_step": "gate_alg_or" },
    { "name": "算法 OR 门", "action": "gate_or", "step": "gate_alg_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_threshold_bad", "evt_window_bad", "evt_oscillation", "evt_stuck"] },

    { "name": "阈值配置不当", "action": "event", "step": "evt_threshold_bad", "description": "目标值设置不合理", "next_step": "gate_threshold_or" },
    { "name": "阈值 OR 门", "action": "gate_or", "step": "gate_threshold_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_target_value_bad", "evt_metric_type_bad"] },
    {
      "name": "目标值过高/过低",
      "action": "event",
      "step": "evt_target_value_bad",
      "severity": "medium",
      "probability": "common",
      "mttr_minutes": 10,
      "detection": {
        "events": [],
        "metrics": ["kube_hpa_spec_target_metric"],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["分析实际负载模式", "调整目标利用率"],
        "auto_actions": ["根据历史数据优化阈值"]
      }
    },
    {
      "name": "指标类型选择错误",
      "action": "event",
      "step": "evt_metric_type_bad",
      "severity": "medium",
      "probability": "medium",
      "mttr_minutes": 15,
      "detection": {
        "events": [],
        "metrics": [],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["检查 Resource/Pods/Object/External 指标类型选择", "验证指标语义"],
        "auto_actions": ["修正指标类型"]
      }
    },

    {
      "name": "冷却窗口设置不合理",
      "action": "event",
      "step": "evt_window_bad",
      "severity": "medium",
      "probability": "common",
      "mttr_minutes": 10,
      "detection": {
        "events": [],
        "metrics": [],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["检查 stabilizationWindowSeconds 配置", "验证 behavior 策略"],
        "auto_actions": ["调整冷却窗口"]
      }
    },

    {
      "name": "副本震荡",
      "action": "event",
      "step": "evt_oscillation",
      "severity": "high",
      "probability": "common",
      "mttr_minutes": 15,
      "detection": {
        "events": ["SuccessfulRescale"],
        "metrics": ["changes(kube_hpa_status_desired_replicas[10m])>5"],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["分析扩缩容频率", "调整冷却窗口和阈值"],
        "auto_actions": ["增加 stabilizationWindowSeconds"]
      },
      "next_step": "gate_oscillation_and"
    },
    { "name": "震荡 AND 门", "action": "gate_and", "step": "gate_oscillation_and", "control": "and_gate", "gate_type": "AND", "next_steps": ["evt_threshold_sensitive", "evt_window_short"] },
    {
      "name": "阈值设置过敏感",
      "action": "event",
      "step": "evt_threshold_sensitive",
      "severity": "medium",
      "probability": "common",
      "mttr_minutes": 10,
      "detection": {
        "events": [],
        "metrics": [],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["增加阈值容差", "使用平均值而非峰值"],
        "auto_actions": ["调整目标值"]
      }
    },
    {
      "name": "冷却窗口过短",
      "action": "event",
      "step": "evt_window_short",
      "severity": "medium",
      "probability": "common",
      "mttr_minutes": 10,
      "detection": {
        "events": [],
        "metrics": [],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["增加 stabilizationWindowSeconds (建议 300s 以上)", "配置 behavior 策略"],
        "auto_actions": ["延长冷却窗口"]
      }
    },

    {
      "name": "扩容卡住",
      "action": "event",
      "step": "evt_stuck",
      "severity": "high",
      "probability": "medium",
      "mttr_minutes": 20,
      "detection": {
        "events": ["FailedComputeMetricsReplicas"],
        "metrics": ["kube_hpa_status_current_replicas == kube_hpa_spec_max_replicas"],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["检查是否达到 maxReplicas", "验证指标可用性"],
        "auto_actions": ["提升 maxReplicas", "修复指标采集"]
      },
      "next_step": "gate_stuck_and"
    },
    { "name": "扩容卡住 AND 门", "action": "gate_and", "step": "gate_stuck_and", "control": "and_gate", "gate_type": "AND", "next_steps": ["evt_metrics_unavailable", "evt_max_reached"] },
    {
      "name": "指标持续不可用",
      "action": "event",
      "step": "evt_metrics_unavailable",
      "severity": "high",
      "probability": "medium",
      "mttr_minutes": 15,
      "detection": {
        "events": ["FailedGetResourceMetric"],
        "metrics": [],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["检查指标服务状态"],
        "auto_actions": ["重启指标服务"]
      }
    },
    {
      "name": "已达 maxReplicas",
      "action": "event",
      "step": "evt_max_reached",
      "severity": "medium",
      "probability": "common",
      "mttr_minutes": 10,
      "detection": {
        "events": [],
        "metrics": ["kube_hpa_status_current_replicas == kube_hpa_spec_max_replicas"],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["评估是否需要提升 maxReplicas", "检查资源配额"],
        "auto_actions": ["提升 maxReplicas"]
      }
    },

    { "name": "目标对象异常", "action": "event", "step": "cat_obj", "description": "HPA 目标资源问题", "next_step": "gate_obj_or" },
    { "name": "目标对象 OR 门", "action": "gate_or", "step": "gate_obj_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_target_missing", "evt_replica_diverge", "evt_scale_subresource"] },
    {
      "name": "目标资源不存在",
      "action": "event",
      "step": "evt_target_missing",
      "severity": "critical",
      "probability": "medium",
      "mttr_minutes": 10,
      "detection": {
        "events": ["FailedGetScale"],
        "metrics": ["kube_hpa_status_condition{condition='ScalingActive',status='False'}"],
        "logs": ["unable to get scale for"]
      },
      "remediation": {
        "manual_steps": ["检查 HPA scaleTargetRef 配置", "验证目标 Deployment/StatefulSet 是否存在"],
        "auto_actions": ["修正 scaleTargetRef"]
      }
    },

    { "name": "副本状态不收敛", "action": "event", "step": "evt_replica_diverge", "description": "期望副本数无法达成", "next_step": "gate_replica_or" },
    { "name": "副本 OR 门", "action": "gate_or", "step": "gate_replica_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_pod_start_fail", "evt_pod_terminate_fail", "evt_replica_mismatch"] },
    {
      "name": "新 Pod 启动失败",
      "action": "event",
      "step": "evt_pod_start_fail",
      "severity": "high",
      "probability": "common",
      "mttr_minutes": 15,
      "detection": {
        "events": ["FailedCreate", "FailedScheduling"],
        "metrics": ["kube_deployment_status_replicas_unavailable"],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["检查 Pod 创建失败原因", "参考 pod-fta.md 诊断"],
        "auto_actions": ["修复 Pod 启动问题"]
      }
    },
    {
      "name": "旧 Pod 无法终止",
      "action": "event",
      "step": "evt_pod_terminate_fail",
      "severity": "medium",
      "probability": "medium",
      "mttr_minutes": 15,
      "detection": {
        "events": [],
        "metrics": ["kube_pod_status_phase{phase='Terminating'}"],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["检查 Pod 终止过程", "验证 preStop 钩子"],
        "auto_actions": ["强制删除卡住的 Pod"]
      }
    },
    {
      "name": "副本数与期望不一致",
      "action": "event",
      "step": "evt_replica_mismatch",
      "severity": "high",
      "probability": "medium",
      "mttr_minutes": 15,
      "detection": {
        "events": [],
        "metrics": ["kube_deployment_status_replicas != kube_deployment_spec_replicas"],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["检查 Deployment 状态", "验证滚动更新配置"],
        "auto_actions": ["触发 Deployment 重新调谐"]
      }
    },

    {
      "name": "目标资源 Scale 子资源异常",
      "action": "event",
      "step": "evt_scale_subresource",
      "severity": "high",
      "probability": "rare",
      "mttr_minutes": 15,
      "detection": {
        "events": ["FailedGetScale", "FailedUpdateScale"],
        "metrics": [],
        "logs": ["unable to get/update scale"]
      },
      "remediation": {
        "manual_steps": ["检查目标资源的 scale 子资源", "验证 CRD 定义"],
        "auto_actions": ["修复 CRD 配置"]
      }
    },

    { "name": "配额与容量限制", "action": "event", "step": "cat_quota", "description": "资源限制阻止扩容", "next_step": "gate_quota_or" },
    { "name": "配额 OR 门", "action": "gate_or", "step": "gate_quota_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_resource_quota", "evt_node_capacity", "evt_pdb_block"] },

    { "name": "资源配额限制", "action": "event", "step": "evt_resource_quota", "description": "命名空间配额不足", "next_step": "gate_quota_detail_or" },
    { "name": "配额详情 OR 门", "action": "gate_or", "step": "gate_quota_detail_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_cpu_mem_quota", "evt_pod_count_quota"] },
    {
      "name": "命名空间 CPU/内存配额用尽",
      "action": "event",
      "step": "evt_cpu_mem_quota",
      "severity": "high",
      "probability": "common",
      "mttr_minutes": 15,
      "detection": {
        "events": ["FailedCreate", "exceeded quota"],
        "metrics": ["kube_resourcequota{type='used'} / kube_resourcequota{type='hard'}"],
        "logs": ["exceeded quota"]
      },
      "remediation": {
        "manual_steps": ["检查 ResourceQuota 使用情况", "申请配额提升"],
        "auto_actions": ["提升配额限制"]
      }
    },
    {
      "name": "Pod 数量超过限制",
      "action": "event",
      "step": "evt_pod_count_quota",
      "severity": "high",
      "probability": "medium",
      "mttr_minutes": 15,
      "detection": {
        "events": ["exceeded quota"],
        "metrics": ["kube_resourcequota{resource='pods'}"],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["检查 Pod 数量配额", "清理无用 Pod"],
        "auto_actions": ["提升 Pod 数量限制"]
      }
    },

    { "name": "节点资源不足", "action": "event", "step": "evt_node_capacity", "description": "集群容量不足", "next_step": "gate_node_capacity_or" },
    { "name": "节点容量 OR 门", "action": "gate_or", "step": "gate_node_capacity_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_node_full", "evt_ca_fail"] },
    {
      "name": "可调度节点资源不足",
      "action": "event",
      "step": "evt_node_full",
      "severity": "high",
      "probability": "common",
      "mttr_minutes": 20,
      "detection": {
        "events": ["FailedScheduling"],
        "metrics": ["kube_node_status_allocatable_cpu_cores", "kube_node_status_allocatable_memory_bytes"],
        "logs": ["Insufficient cpu", "Insufficient memory"]
      },
      "remediation": {
        "manual_steps": ["检查节点可分配资源", "扩展集群节点"],
        "auto_actions": ["触发 Cluster Autoscaler"]
      }
    },
    {
      "name": "Cluster Autoscaler 未能扩展",
      "action": "event",
      "step": "evt_ca_fail",
      "severity": "high",
      "probability": "medium",
      "mttr_minutes": 30,
      "detection": {
        "events": ["ScaleUpFailed", "NotTriggerScaleUp"],
        "metrics": ["cluster_autoscaler_scaled_up_nodes_total"],
        "logs": ["cluster-autoscaler: couldn't scale up"]
      },
      "remediation": {
        "manual_steps": ["检查 Cluster Autoscaler 状态", "验证节点池配置"],
        "auto_actions": ["修复 CA 配置"]
      }
    },

    {
      "name": "PDB 阻止缩容",
      "action": "event",
      "step": "evt_pdb_block",
      "severity": "medium",
      "probability": "common",
      "mttr_minutes": 15,
      "detection": {
        "events": ["EvictionBlocked"],
        "metrics": ["kube_poddisruptionbudget_status_pod_disruptions_allowed"],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["检查 PodDisruptionBudget 配置", "验证 minAvailable/maxUnavailable"],
        "auto_actions": ["调整 PDB 配置"]
      }
    },

    { "name": "控制面依赖异常", "action": "event", "step": "cat_cp", "description": "控制面组件问题", "next_step": "gate_cp_or" },
    { "name": "控制面 OR 门", "action": "gate_or", "step": "gate_cp_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_api_fail", "evt_hpa_controller", "evt_rbac_deny"] },
    {
      "name": "API Server 异常",
      "action": "event",
      "step": "evt_api_fail",
      "severity": "critical",
      "probability": "medium",
      "mttr_minutes": 20,
      "detection": {
        "events": [],
        "metrics": ["apiserver_request_total{code=~'5..'}"],
        "logs": ["connection refused to apiserver"]
      },
      "remediation": {
        "manual_steps": ["检查 API Server 状态", "参考 apiserver-fta.md 诊断"],
        "auto_actions": ["检查控制面健康"]
      }
    },

    { "name": "HPA 控制器异常", "action": "event", "step": "evt_hpa_controller", "description": "kube-controller-manager HPA 控制器问题", "next_step": "gate_hpa_controller_or" },
    { "name": "HPA 控制器 OR 门", "action": "gate_or", "step": "gate_hpa_controller_or", "control": "or_gate", "gate_type": "OR", "next_steps": ["evt_controller_crash", "evt_controller_stuck", "evt_sync_slow"] },
    {
      "name": "控制器进程异常",
      "action": "event",
      "step": "evt_controller_crash",
      "severity": "critical",
      "probability": "rare",
      "mttr_minutes": 15,
      "detection": {
        "events": [],
        "metrics": ["kube_pod_container_status_ready{pod=~'kube-controller-manager.*'}"],
        "logs": ["kube-controller-manager: error"]
      },
      "remediation": {
        "manual_steps": ["检查 kube-controller-manager 状态"],
        "auto_actions": ["重启 kube-controller-manager"]
      }
    },
    {
      "name": "控制循环卡死",
      "action": "event",
      "step": "evt_controller_stuck",
      "severity": "high",
      "probability": "rare",
      "mttr_minutes": 20,
      "detection": {
        "events": [],
        "metrics": [],
        "logs": ["work queue is full"]
      },
      "remediation": {
        "manual_steps": ["检查控制器日志", "分析队列积压原因"],
        "auto_actions": ["重启控制器"]
      }
    },
    {
      "name": "同步周期过长",
      "action": "event",
      "step": "evt_sync_slow",
      "severity": "medium",
      "probability": "medium",
      "mttr_minutes": 15,
      "detection": {
        "events": [],
        "metrics": [],
        "logs": []
      },
      "remediation": {
        "manual_steps": ["检查 --horizontal-pod-autoscaler-sync-period 配置"],
        "auto_actions": ["调整同步周期"]
      }
    },

    {
      "name": "RBAC 权限不足",
      "action": "event",
      "step": "evt_rbac_deny",
      "severity": "high",
      "probability": "medium",
      "mttr_minutes": 10,
      "detection": {
        "events": ["Forbidden"],
        "metrics": [],
        "logs": ["cannot get/update scale"]
      },
      "remediation": {
        "manual_steps": ["检查 HPA ServiceAccount 权限", "验证 ClusterRole/RoleBinding"],
        "auto_actions": ["修复 RBAC 配置"]
      }
    },

    { "name": "结束", "action": "end", "step": "end_hpa_fta" }
  ]
}
```

---

## 版本适配（1.19–1.30）
- **1.19–1.23**：HPA v2beta2 API 与指标适配需核对；旧版 metrics-server 兼容性需关注；behavior 字段可能不可用。
- **1.24–1.27**：HPA v2 GA；自定义指标适配器与 API 版本对齐，避免指标读取失败；behavior 策略成为标准配置。
- **1.28–1.30**：稳定 API 为主，需确保指标链路与审计一致性；ContainerResource 指标类型可用。
- **共性**：遵循 `fta-methodology-and-agentic-practices.md` 中的"版本适配基线"。
