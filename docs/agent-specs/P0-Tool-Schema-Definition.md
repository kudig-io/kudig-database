---
title: Agent Tool Schema 定义文档
description: '**用途**: 定义 Agent 可执行的 Kubernetes 运维工具 Schema，供 Agent 调用'
category: general
tags:
- k8s
- etcd
- kubelet
- coredns
- docker
- opa
- hpa
- pdb
- statefulset
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Agent Tool Schema 定义文档 是什么
- 如何 Agent Tool Schema 定义文档
trigger_keywords:
- Agent
- Tool
- Schema
- 定义文档
prerequisites:
- kubectl-basics
- etcd-basics
- tls-basics
- policy-basics
created: "2026-05-23"
---

# Agent Tool Schema 定义文档

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: 定义 Agent 可执行的 Kubernetes 运维工具 Schema，供 Agent 调用

---

## 1. 概述

本文档定义 Agent 在问题诊断和修复过程中可调用的工具 Schema。每个工具包含：
- 名称和描述
- 输入参数规范
- 输出格式定义
- 风险等级评估
- 回滚能力说明

---

## 2. 工具分类

| Category | 工具数量 | 说明 |
|----------|---------|------|
| DIAGNOSTIC | 15 | 诊断类工具 |
| CONFIGURATION | 10 | 配置查看/修改工具 |
| REMEDIATION | 12 | 修复操作工具 |
| MONITORING | 8 | 监控指标工具 |

---

## 3. 诊断类工具 (DIAGNOSTIC)

### 3.1 kubectl_get_pods

```yaml
tool:
  name: kubectl_get_pods
  description: 获取指定命名空间的 Pod 列表及状态
  category: DIAGNOSTIC
  
  parameters:
    type: object
    properties:
      namespace:
        type: string
        description: 命名空间名称
        required: false
        default: "default"
        enum: ["default", "kube-system", "kube-public", "all"]
      
      all_namespaces:
        type: boolean
        description: 是否查询所有命名空间
        required: false
        default: false
      
      selector:
        type: string
        description: Label selector (e.g., "app=nginx")
        required: false
        example: "app=myapp,tier=frontend"
      
      field_selector:
        type: string
        description: Field selector (e.g., "status.phase=Running")
        required: false
      
      show_watch:
        type: boolean
        description: 是否持续监控变化
        required: false
        default: false

  output:
    format: json
    schema:
      type: object
      properties:
        apiVersion: string
        kind: string
        items: array
          - metadata:
              name: string
              namespace: string
              labels: object
              creationTimestamp: string
            spec:
              nodeName: string
              containers:
                - name: string
                  image: string
                  ports: array
                  env: array
            status:
              phase: enum[Pending|Running|Succeeded|Failed|Unknown]
              conditions: array
              startTime: string
              message: string
              reason: string
    
    display_fields: ["NAME", "READY", "STATUS", "RESTARTS", "AGE", "NODE"]

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  examples:
    - description: "查看 default 命名空间所有 Pod"
      command: kubectl get pods -n default
      
    - description: "查看所有命名空间的 Running 状态 Pod"
      command: kubectl get pods --all-namespaces --field-selector=status.phase=Running
      
    - description: "查看指定标签的 Pod"
      command: kubectl get pods -l app=nginx,tier=frontend

  error_codes:
    "Unauthorized": "RBAC 权限不足，需要 get pods 权限"
    "NotFound": "命名空间不存在"
    "Timeout": "API Server 响应超时"
```

### 3.2 kubectl_describe_pod

```yaml
tool:
  name: kubectl_describe_pod
  description: 获取 Pod 详细信息，包括事件、容器状态、资源使用
  category: DIAGNOSTIC
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: Pod 名称
        required: true
      
      namespace:
        type: string
        description: 命名空间名称
        required: false
        default: "default"
      
      container:
        type: string
        description: 指定容器名称（仅显示该容器日志）
        required: false

  output:
    format: text/table
    sections:
      - Name
      - Namespace
      - Priority
      - Node
      - Start Time
      - Labels
      - Annotations
      - Status
      - Containers
      - Conditions
      - Events

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "Pod 处于非 Running 状态时查看详情"
    - "查看 Pod 调度到的节点"
    - "查看最近的事件和告警原因"
    - "查看容器重启次数和状态"

  error_codes:
    "Unauthorized": "RBAC 权限不足"
    "NotFound": "Pod 不存在"
```

### 3.3 kubectl_get_events

```yaml
tool:
  name: kubectl_get_events
  description: 获取集群事件，用于问题诊断
  category: DIAGNOSTIC
  
  parameters:
    type: object
    properties:
      namespace:
        type: string
        description: 命名空间（默认为 all）
        required: false
      
      all_namespaces:
        type: boolean
        description: 查询所有命名空间
        required: false
        default: false
      
      sort_by:
        type: string
        description: 排序字段
        required: false
        default: "lastTimestamp"
        enum: ["lastTimestamp", "firstTimestamp", "count", "reason"]
      
      since:
        type: string
        description: 只显示最近 N 小时内的事件
        required: false
        example: "2h"
      
      field_selector:
        type: string
        description: Field selector 过滤
        required: false
        example: "reason=Failed"
      
      watch:
        type: boolean
        description: 持续监控新事件
        required: false
        default: false

  output:
    format: table
    columns: ["LAST SEEN", "TYPE", "REASON", "OBJECT", "MESSAGE"]
    
  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看节点 NotReady 前后的事件"
    - "查看 Pod 创建失败的原因"
    - "查看调度失败的详细原因"
    - "持续监控异常事件"

  error_codes:
    "Timeout": "事件过多，API 超时"
```

### 3.4 kubectl_logs

```yaml
tool:
  name: kubectl_logs
  description: 获取 Pod 容器日志
  category: DIAGNOSTIC
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: Pod 名称
        required: true
      
      container:
        type: string
        description: 容器名称（多容器 Pod 必须指定）
        required: false
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"
      
      previous:
        type: boolean
        description: 获取上一个已终止容器的日志
        required: false
        default: false
      
      since:
        type: string
        description: 只显示最近 N 分钟/小时/天的日志
        required: false
        example: "5m"
      
      tail:
        type: integer
        description: 只显示最后 N 行
        required: false
        default: 100
      
      timestamps:
        type: boolean
        description: 显示时间戳
        required: false
        default: false
      
      limit_bytes:
        type: integer
        description: 限制返回的字节数
        required: false

  output:
    format: text
    encoding: utf-8

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "应用启动失败，查看启动日志"
    - "CrashLoopBackOff 查看容器退出日志"
    - "OOMKilled 查看 GC 或内存分配日志"
    - "previous=true 查看崩溃前的日志"

  error_codes:
    "BadRequest": "容器名称错误或多容器 Pod 未指定容器"
    "NotFound": "Pod 或容器不存在"
```

### 3.5 kubectl_get_nodes

```yaml
tool:
  name: kubectl_get_nodes
  description: 获取节点列表及状态
  category: DIAGNOSTIC
  
  parameters:
    type: object
    properties:
      all_namespaces:
        type: boolean
        description: 无视此参数，节点不属于命名空间
        required: false
      
      selector:
        type: string
        description: Label selector
        required: false
      
      show_kind:
        type: boolean
        description: 显示节点角色标签（如 master）
        required: false
        default: true

  output:
    format: table
    columns: ["NAME", "STATUS", "ROLES", "AGE", "VERSION"]
    status_meanings:
      Ready: "节点健康，可调度 Pod"
      NotReady: "节点不健康，kubelet 未上报状态"
      SchedulingDisabled: "节点已被 cordon，不再调度新 Pod"

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看集群所有节点状态"
    - "确认 master 节点数量"
    - "查看节点 Kubernetes 版本分布"

  error_codes:
    "Unauthorized": "RBAC 权限不足"
```

### 3.6 kubectl_describe_node

```yaml
tool:
  name: kubectl_describe_node
  description: 获取节点详细信息，包括资源使用、条件和事件
  category: DIAGNOSTIC
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: 节点名称
        required: true

  output:
    format: text/multi-section
    sections:
      - Name
      - Roles
      - Labels
      - Annotations
      - CreationTimestamp
      - Conditions (MemoryPressure|DiskPressure|PIDPressure|NetworkUnavailable|Ready)
      - Addresses (InternalIP|ExternalIP|Hostname)
      - System Info (OS|Kernel|Container Runtime|CRI version|Kubelet version|Kube-proxy version)
      - Pod List
      - Memory/Torque/Nodefs capacity
      - Events

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "节点 NotReady 时查看详细原因"
    - "排查节点资源不足"
    - "查看节点污点配置"

  error_codes:
    "NotFound": "节点不存在"
```

### 3.7 kubectl_top_node

```yaml
tool:
  name: kubectl_top_node
  description: 获取节点资源使用情况（CPU/内存）
  category: DIAGNOSTIC
  
  parameters:
    type: object
    properties:
      name:
        type: string
        description: 节点名称（不指定则显示所有）
        required: false
      
      no_headers:
        type: boolean
        description: 不显示表头
        required: false
        default: false

  output:
    format: table
    columns: ["NAME", "CPU(cores)", "CPU%", "MEMORY(bytes)", "MEMORY%"]
    
  prerequisites:
    - "metrics-server 已部署"
    - "有 metrics.k8s.io API 访问权限"

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "找出 CPU/Memory 最高的节点"
    - "判断是否需要扩容"
    - "验证资源压缩是否生效"

  error_codes:
    "MetricsNotAvailable": "metrics-server 未部署或不可用"
    "Unauthorized": "无 metrics API 权限"
```

### 3.8 kubectl_top_pod

```yaml
tool:
  name: kubectl_top_pod
  description: 获取 Pod 资源使用情况
  category: DIAGNOSTIC
  
  parameters:
    type: object
    properties:
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"
      
      all_namespaces:
        type: boolean
        description: 查询所有命名空间
        required: false
        default: false
      
      selector:
        type: string
        description: Label selector
        required: false
      
      containers:
        type: boolean
        description: 显示每个容器的资源使用
        required: false
        default: false

  output:
    format: table
    columns: ["NAMESPACE", "NAME", "CPU(cores)", "MEMORY(bytes)"]
    # containers=true 时额外显示每容器

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "找出内存使用最高的 Pod"
    - "诊断 OOMKilled 原因"
    - "验证 resource limits 是否生效"

  error_codes:
    "MetricsNotAvailable": "metrics-server 未部署"
```

### 3.9 kubectl_get_services

```yaml
tool:
  name: kubectl_get_services
  description: 获取 Service 列表及状态
  category: DIAGNOSTIC
  
  parameters:
    type: object
    properties:
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"
      
      all_namespaces:
        type: boolean
        description: 查询所有命名空间
        required: false
        default: false
      
      selector:
        type: string
        description: Label selector
        required: false

  output:
    format: table
    columns: ["NAME", "TYPE", "CLUSTER-IP", "PORT(S)", "AGE"]
    type_meanings:
      ClusterIP: "内部访问"
      NodePort: "节点端口"
      LoadBalancer: "云厂商负载均衡"
      ExternalName: "外部域名映射"

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看 Service 类型和端口配置"
    - "确认 ClusterIP 是否正确"
    - "检查 NodePort 范围"

  error_codes:
    "Unauthorized": "RBAC 权限不足"
```

### 3.10 kubectl_get_endpoints

```yaml
tool:
  name: kubectl_get_endpoints
  description: 获取 Service 的 Endpoints（后端 Pod IP 列表）
  category: DIAGNOSTIC
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: Service 名称
        required: true
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"

  output:
    format: yaml/json
    fields:
      - name
      - namespace
      - subsets:
          - addresses: [{IP, targetRef}]
          - notReadyAddresses: [{IP, targetRef}]
          - ports: [{port, protocol, name}]

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "Service 无 Endpoints 时排查"
    - "确认 Pod 是否被正确选中"
    - "检查健康/不健康端点数量"

  error_codes:
    "NotFound": "Service 不存在"
```

### 3.11 kubectl_get_pvc

```yaml
tool:
  name: kubectl_get_pvc
  description: 获取 PersistentVolumeClaim 状态
  category: DIAGNOSTIC
  
  parameters:
    type: object
    properties:
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"
      
      all_namespaces:
        type: boolean
        description: 查询所有命名空间
        required: false
        default: false
      
      selector:
        type: string
        description: Label selector
        required: false

  output:
    format: table
    columns: ["NAME", "STATUS", "VOLUME", "CAPACITY", "ACCESS MODES", "STORAGECLASS", "AGE"]
    status_meanings:
      Pending: "正在创建或等待绑定"
      Bound: "已绑定到 PV"
      Lost: "PV 被删除但 PVC 仍存在"

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "PVC 一直 Pending 排查"
    - "确认存储是否足够"
    - "查看 StorageClass 配置"

  error_codes:
    "Unauthorized": "RBAC 权限不足"
```

### 3.12 kubectl_get_pv

```yaml
tool:
  name: kubectl_get_pv
  description: 获取 PersistentVolume 列表及状态
  category: DIAGNOSTIC
  
  parameters:
    type: object
    properties:
      selector:
        type: string
        description: Label selector
        required: false

  output:
    format: table
    columns: ["NAME", "CAPACITY", "ACCESS MODES", "RECLAIM POLICY", "STATUS", "CLAIM", "STORAGECLASS", "AGE"]
    status_meanings:
      Available: "空闲可用"
      Bound: "已绑定到 PVC"
      Released: "PVC 已删除但未回收"
      Failed: "回收失败"

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看所有 PV 状态"
    - "排查 Released 状态 PV"
    - "查看存储类分布"

  error_codes:
    "Unauthorized": "RBAC 权限不足"
```

### 3.13 kubectl_get_configmap

```yaml
tool:
  name: kubectl_get_configmap
  description: 获取 ConfigMap 详情
  category: DIAGNOSTIC
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: ConfigMap 名称
        required: true
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"

  output:
    format: yaml/json
    fields:
      - apiVersion
      - kind
      - metadata (name, namespace, labels, annotations)
      - data: {key: value}
      - binaryData: {key: base64}

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "验证 ConfigMap 内容是否正确"
    - "检查配置热更新是否生效"
    - "排查 ConfigMap 引用错误"

  error_codes:
    "NotFound": "ConfigMap 不存在"
```

### 3.14 kubectl_get_secret

```yaml
tool:
  name: kubectl_get_secret
  description: 获取 Secret 详情（默认不显示值）
  category: DIAGNOSTIC
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: Secret 名称
        required: true
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"
      
      decode:
        type: boolean
        description: 是否解码显示值（敏感操作）
        required: false
        default: false

  output:
    format: yaml/json
    fields:
      - type (Opaque|kubernetes.io/tls|kubernetes.io/dockerconfigjson|etc.)
      - data: {key: base64}  # 未解码
      - stringData: {key: plaintext}  # 仅 decode=true 时

  risk_level: MEDIUM
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "检查 Secret 类型"
    - "验证 dockerconfigjson 格式"
    - "解码查看 TLS 证书"

  error_codes:
    "NotFound": "Secret 不存在"
    "Forbidden": "无 secret 读取权限"
```

### 3.15 kubectl_get_deployment

```yaml
tool:
  name: kubectl_get_deployment
  description: 获取 Deployment 状态和详情
  category: DIAGNOSTIC
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: Deployment 名称
        required: true
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"
      
      show_extended:
        type: boolean
        description: 显示详细状态
        required: false
        default: true

  output:
    format: yaml/json
    key_fields:
      - metadata (name, namespace, labels, annotations)
      - spec (replicas, selector, strategy, template)
      - status:
          replicas
          readyReplicas
          availableReplicas
          updatedReplicas
          conditions

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "检查滚动更新进度"
    - "查看副本数是否匹配"
    - "检查更新策略"

  error_codes:
    "NotFound": "Deployment 不存在"
```

### 3.16 kubectl_get_statefulset

```yaml
tool:
  name: kubectl_get_statefulset
  description: 获取 StatefulSet 状态
  category: DIAGNOSTIC
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: StatefulSet 名称
        required: true
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"

  output:
    format: yaml/json
    key_fields:
      - spec (serviceName, replicas, selector, podManagementPolicy, updateStrategy)
      - status (replicas, readyReplicas, currentReplicas, updatedReplicas, currentRevision, updateRevision)

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "检查有序部署状态"
    - "排查 StatefulSet 启动问题"
    - "查看 ordinal index"

  error_codes:
    "NotFound": "StatefulSet 不存在"
```

---

## 4. 配置查看类工具 (CONFIGURATION)

### 4.1 kubectl_getIngress

```yaml
tool:
  name: kubectl_get_ingress
  description: 获取 Ingress 配置和状态
  category: CONFIGURATION
  
  parameters:
    type: object
    properties:
      name:
        type: string
        description: Ingress 名称（不指定则列出所有）
        required: false
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"
      
      all_namespaces:
        type: boolean
        description: 查询所有命名空间
        required: false
        default: false

  output:
    format: yaml/json
    key_fields:
      - spec (rules, tls)
      - status (loadBalancer)

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看 Ingress 路由规则"
    - "检查 TLS 证书配置"
    - "查看 loadBalancer IP"

  error_codes:
    "NotFound": "Ingress 不存在"
```

### 4.2 kubectl_get_hpa

```yaml
tool:
  name: kubectl_get_hpa
  description: 获取 HorizontalPodAutoscaler 配置
  category: CONFIGURATION
  
  parameters:
    type: object
    properties:
      name:
        type: string
        description: HPA 名称（不指定则列出所有）
        required: false
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"
      
      all_namespaces:
        type: boolean
        description: 查询所有命名空间
        required: false
        default: false

  output:
    format: table
    columns: ["NAME", "REFERENCE", "TARGETS", "MINPODS", "MAXPODS", "REPLICAS", "AGE"]
    # TARGETS 显示 current->desired

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看 HPA 扩展目标"
    - "检查当前/目标指标值"
    - "查看副本数范围"

  error_codes:
    "NotFound": "HPA 不存在"
```

### 4.3 kubectl_get_pdb

```yaml
tool:
  name: kubectl_get_pdb
  description: 获取 PodDisruptionBudget 配置
  category: CONFIGURATION
  
  parameters:
    type: object
    properties:
      name:
        type: string
        description: PDB 名称
        required: false
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"

  output:
    format: yaml/json
    key_fields:
      - spec (minAvailable, maxUnavailable, selector)
      - status (disruptionsAllowed, currentHealthy, desiredHealthy, expectedPods)

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "检查 PDB 是否阻止了驱逐"
    - "查看允许的最大不可用数"

  error_codes:
    "NotFound": "PDB 不存在"
```

### 4.4 kubectl_get_networkpolicy

```yaml
tool:
  name: kubectl_get_networkpolicy
  description: 获取网络策略配置
  category: CONFIGURATION
  
  parameters:
    type: object
    properties:
      name:
        type: string
        description: 网络策略名称
        required: false
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"

  output:
    format: yaml/json
    key_fields:
      - spec (podSelector, policyTypes, ingress, egress)

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看命名空间隔离规则"
    - "检查 Ingress/Egress 白名单"
    - "排查网络不通原因"

  error_codes:
    "NotFound": "NetworkPolicy 不存在"
```

### 4.5 kubectl_get_resourcequota

```yaml
tool:
  name: kubectl_get_resourcequota
  description: 获取资源配额
  category: CONFIGURATION
  
  parameters:
    type: object
    properties:
      name:
        type: string
        description: Quota 名称
        required: false
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"

  output:
    format: yaml/json
    sections:
      - spec (hard limits)
      - status (used vs hard)

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "检查命名空间配额使用"
    - "排查资源创建失败原因"

  error_codes:
    "NotFound": "ResourceQuota 不存在"
```

### 4.6 kubectl_get_limitrange

```yaml
tool:
  name: kubectl_get_limitrange
  description: 获取 LimitRange 配置
  category: CONFIGURATION
  
  parameters:
    type: object
    properties:
      name:
        type: string
        description: LimitRange 名称
        required: false
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"

  output:
    format: yaml/json
    fields:
      - spec (limits: [{type, max, min, default, defaultRequest, limitRatio}])

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看容器默认资源限制"
    - "排查 LimitRange 导致的问题"

  error_codes:
    "NotFound": "LimitRange 不存在"
```

### 4.7 kubectl_get_role

```yaml
tool:
  name: kubectl_get_role
  description: 获取 Role 或 ClusterRole 绑定
  category: CONFIGURATION
  
  parameters:
    type: object
    properties:
      name:
        type: string
        description: Role/ClusterRole 名称
        required: false
      
      namespace:
        type: string
        description: 命名空间（ClusterRole 不需要）
        required: false
      
      kind:
        type: string
        description: 类型
        required: false
        default: "Role"
        enum: ["Role", "ClusterRole", "RoleBinding", "ClusterRoleBinding"]

  output:
    format: yaml/json
    fields:
      - rules (apiGroups, resources, verbs)

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看权限定义"
    - "排查 RBAC 权限问题"

  error_codes:
    "NotFound": "Role 不存在"
```

### 4.8 kubectl_get_certificate

```yaml
tool:
  name: kubectl_get_certificate
  description: 获取 Certificate 状态（[[domain-19-landscape-references/01-cncf-landscape/graduated/cert-manager/cert-manager|cert-manager]]）
  category: CONFIGURATION
  
  parameters:
    type: object
    properties:
      name:
        type: string
        description: Certificate 名称
        required: false
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"

  output:
    format: yaml/json
    key_fields:
      - spec (secretName, issuerRef, dnsNames, duration, renewBefore)
      - status (conditions, notBefore, notAfter, renewalTime, lastTransitionTime)

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "检查证书状态"
    - "查看证书到期时间"
    - "排查证书续期失败"

  error_codes:
    "NotFound": "Certificate 不存在（cert-manager CRD 未安装）"
```

### 4.9 kubectl_get_storageclass

```yaml
tool:
  name: kubectl_get_storageclass
  description: 获取 StorageClass 配置
  category: CONFIGURATION
  
  parameters:
    type: object
    properties:
      name:
        type: string
        description: StorageClass 名称
        required: false

  output:
    format: table
    columns: ["NAME", "PROVISIONER", "AGE", "PARAMETERS", "POLICY"]
    
    # 或完整 YAML：
    # provisioner (ebs.csi.aws.com|pd.csi.storage.gke.io|etc.)
    # parameters (type, filesystem, replication)
    # reclaimPolicy (Delete|Retain)
    # volumeBindingMode (Immediate|WaitForFirstConsumer)

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看可用的存储类型"
    - "检查默认 StorageClass"
    - "查看存储卷绑定模式"

  error_codes:
    "Unauthorized": "RBAC 权限不足"
```

### 4.10 kubectl_get_csi_driver

```yaml
tool:
  name: kubectl_get_csi_driver
  description: 获取 CSI Driver 注册信息
  category: CONFIGURATION
  
  parameters:
    type: object
    properties:
      name:
        type: string
        description: CSI Driver 名称
        required: false

  output:
    format: yaml/json
    key_fields:
      - spec (attachRequired, podInfoOnMount, volumeLifecycleModes)

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看集群支持的 CSI 特性"
    - "检查 CSI driver 是否注册"

  error_codes:
    "NotFound": "CSIDriver 不存在（未安装）"
```

---

## 5. 修复类工具 (REMEDIATION)

### 5.1 kubectl_rollout_restart

```yaml
tool:
  name: kubectl_rollout_restart
  description: 重启 Deployment（触发滚动更新）
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: Deployment 名称
        required: true
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"

  action: "在 Pod template spec 添加注解，触发 rollout"
  side_effects:
    - "Pod 会滚动重启"
    - "服务可能短暂中断"
    - "产生新的 ReplicaSet"

  risk_level: MEDIUM
  rollback:
    enabled: true
    method: "kubectl rollout undo deployment/<name>"
    previous_revision: "可指定 --to-revision=N"

  idempotent: true
  # 幂等：重复执行会追加注解时间戳

  use_cases:
    - "ConfigMap 更新后重启应用"
    - "修复内存泄漏（临时）"
    - "强制重新加载配置"

  prerequisites:
    - "Deployment 存在"
    - "有 deployment update 权限"

  error_codes:
    "NotFound": "Deployment 不存在"
    "Forbidden": "RBAC 权限不足"
```

### 5.2 kubectl_scale

```yaml
tool:
  name: kubectl_scale
  description: 修改 Deployment/ReplicaSet 副本数
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["name", "replicas"]
    properties:
      name:
        type: string
        description: 资源名称
        required: true
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"
      
      replicas:
        type: integer
        description: 目标副本数
        required: true
        minimum: 0
      
      kind:
        type: string
        description: 资源类型
        required: false
        default: "deployment"
        enum: ["deployment", "replicaset", "statefulset"]

  action: "修改 spec.replicas"
  
  risk_level: MEDIUM
  rollback:
    enabled: true
    method: "kubectl scale <kind>/<name> --replicas=<previous>"
    # 需要记录操作前的副本数

  idempotent: true

  use_cases:
    - "紧急扩容应对流量高峰"
    - "缩容节省资源"
    - "副本数归零暂停服务"

  error_codes:
    "NotFound": "资源不存在"
    "Invalid": "副本数格式错误"
```

### 5.3 kubectl_cordon

```yaml
tool:
  name: kubectl_cordon
  description: 标记节点为不可调度
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: 节点名称
        required: true

  action: "添加 node.kubernetes.io/not-ready:NoSchedule 污点"
  # 等效于 kubectl drain --ignore-daemonsets --delete-emptydir-data=false --force <node>
  
  risk_level: MEDIUM
  rollback:
    enabled: true
    method: "kubectl uncordon <name>"

  idempotent: true
  # 幂等：已 cordon 的节点重复执行无影响

  use_cases:
    - "节点维护前标记"
    - "问题节点隔离"
    - "阻止新 Pod 调度到问题节点"

  error_codes:
    "NotFound": "节点不存在"
```

### 5.4 kubectl_uncordon

```yaml
tool:
  name: kubectl_uncordon
  description: 解除节点不可调度标记
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: 节点名称
        required: true

  action: "移除 node.kubernetes.io/not-ready:NoSchedule 污点"
  
  risk_level: MEDIUM
  rollback:
    enabled: true
    method: "kubectl cordon <name>"  # 回退到 cordon 状态

  idempotent: true

  use_cases:
    - "节点维护完成后恢复调度"
    - "问题节点恢复后重新启用"

  error_codes:
    "NotFound": "节点不存在"
```

### 5.5 kubectl_drain

```yaml
tool:
  name: kubectl_drain
  description: 驱逐节点上的 Pod
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: 节点名称
        required: true
      
      delete_options:
        type: object
        properties:
          grace_period:
            type: integer
            description: 优雅终止时间（秒）
            required: false
            default: -1  # 使用 Pod 指定的值
          
          timeout:
            type: integer
            description: 等待驱逐的超时时间（秒）
            required: false
          
          ignore_daemonsets:
            type: boolean
            description: 忽略 DaemonSet
            required: false
            default: true
          
          delete_empty_dir:
            type: boolean
            description: 删除 emptyDir 数据
            required: false
            default: true
          
          force:
            type: boolean
            description: 强制删除非 DaemonSet Pod
            required: false
            default: false
          
          skip_wait_for_delete_timeout:
            type: integer
            description: 跳过等待 Pod 删除的超时
            required: false

  action: "逐个终止并驱逐 Pod"
  
  risk_level: HIGH
  rollback:
    enabled: false  # 驱逐不可逆，Pod 会在其他节点重建

  use_cases:
    - "节点下线前驱逐所有 Pod"
    - "节点问题时转移负载"

  prerequisites:
    - "节点已被 cordon（建议）"
    - "PDBs 允许驱逐"

  error_codes:
    "NotFound": "节点不存在"
    "Timeout": "驱逐超时"
    "Forbidden": "RBAC 权限不足"
```

### 5.6 kubectl_delete_pod

```yaml
tool:
  name: kubectl_delete_pod
  description: 删除 Pod（会触发重建）
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: Pod 名称
        required: true
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"
      
      wait_for_recreate:
        type: boolean
        description: 等待新 Pod 创建完成
        required: false
        default: false

  action: "删除 Pod，ReplicaSet/Deployment 会创建新的"
  
  risk_level: MEDIUM
  rollback:
    enabled: false  # 只能等新 Pod 运行后检查是否正常

  use_cases:
    - "Pod 处于异常状态无法恢复"
    - "强制重新调度 Pod"
    - "触发配置重新加载"

  error_codes:
    "NotFound": "Pod 不存在"
    "Forbidden": "RBAC 权限不足"
```

### 5.7 kubectl_label

```yaml
tool:
  name: kubectl_label
  description: 添加或更新资源标签
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["name", "labels"]
    properties:
      name:
        type: string
        description: 资源名称
        required: true
      
      resource_type:
        type: string
        description: 资源类型
        required: true
        enum: ["pod", "node", "service", "deployment", "namespace", "configmap", "secret"]
      
      namespace:
        type: string
        description: 命名空间（节点不需要）
        required: false
      
      labels:
        type: object
        description: 标签 key:value
        required: true
        additionalProperties:
          type: string
      
      overwrite:
        type: boolean
        description: 是否覆盖已存在的标签
        required: false
        default: true

  action: "更新资源的 labels"
  
  risk_level: LOW
  rollback:
    enabled: true
    method: "kubectl label <type>/<name> <original-label>=<original-value>"

  idempotent: true

  use_cases:
    - "标记环境类型（env=prod）"
    - "添加版本标签"
    - "修改污点标签"

  error_codes:
    "NotFound": "资源不存在"
    "Invalid": "标签格式错误"
```

### 5.8 kubectl_patch

```yaml
tool:
  name: kubectl_patch
  description: 补丁修改资源
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["name", "resource_type", "patch"]
    properties:
      name:
        type: string
        description: 资源名称
        required: true
      
      resource_type:
        type: string
        description: 资源类型
        required: true
      
      namespace:
        type: string
        description: 命名空间
        required: false
      
      patch:
        type: string|object
        description: 补丁内容
        required: true
        # 支持 JSON merge patch 或 strategic merge patch
      
      patch_type:
        type: string
        description: 补丁类型
        required: false
        default: "strategic"
        enum: ["json", "merge", "strategic"]

  action: "部分更新资源"
  
  risk_level: MEDIUM
  rollback:
    enabled: true
    method: "记录原值，手动 patch 回滚"

  use_cases:
    - "修改容器镜像版本"
    - "更新副本数"
    - "调整 resource limits"

  error_codes:
    "NotFound": "资源不存在"
    "Invalid": "补丁格式错误"
```

### 5.9 kubectl_apply

```yaml
tool:
  name: kubectl_apply
  description: 应用 YAML 配置（创建或更新）
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["manifest"]
    properties:
      manifest:
        type: string|object
        description: YAML 配置内容
        required: true
      
      namespace:
        type: string
        description: 命名空间（可从 manifest 读取）
        required: false
      
      dry_run:
        type: string
        description: 试运行模式
        required: false
        enum: ["none", "server", "client"]
        default: "none"
      
      validate:
        type: boolean
        description: 是否校验 schema
        required: false
        default: true
      
      force:
        type: boolean
        description: 强制替换（可能导致服务中断）
        required: false
        default: false

  action: "create 或 update 资源"
  
  risk_level: MEDIUM
  rollback:
    enabled: true
    method: "kubectl delete -f <manifest> 或 kubectl rollout undo"

  prerequisites:
    - "YAML 格式正确"
    - "资源定义完整"

  error_codes:
    "Invalid": "YAML 格式错误"
    "Conflict": "force=false 时资源已存在"
    "Forbidden": "RBAC 权限不足"
```

### 5.10 kubectl_debug

```yaml
tool:
  name: kubectl_debug
  description: 创建调试容器
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["pod", "image"]
    properties:
      pod:
        type: string
        description: 目标 Pod 名称
        required: true
      
      namespace:
        type: string
        description: 命名空间
        required: false
        default: "default"
      
      image:
        type: string
        description: 调试镜像
        required: true
        default: "busybox:1.36"
      
      container:
        type: string
        description: 调试容器名称
        required: false
      
      command:
        type: array
        description: 启动命令
        required: false
      
      attach:
        type: boolean
        description: 是否 attach 到容器
        required: false
        default: false

  action: "在 Pod 所在节点创建调试容器"
  
  risk_level: MEDIUM
  rollback:
    enabled: true
    method: "kubectl delete pod <debug-pod>"

  use_cases:
    - "网络诊断（curl, ping, nslookup）"
    - "文件系统检查"
    - "进程分析"

  error_codes:
    "NotFound": "Pod 不存在"
    "NodeResources": "节点资源不足"
```

### 5.11 kubectl_certificate_approve

```yaml
tool:
  name: kubectl_certificate_approve
  description: 批准 CertificateSigningRequest
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: CSR 名称
        required: true

  action: "批准 CSR，使其生效"
  
  risk_level: HIGH
  rollback:
    enabled: false  # 批准后无法撤回，只能删除证书

  use_cases:
    - "批准 kubelet bootstrap CSR"
    - "批准 TLS bootstrapping 请求"

  prerequisites:
    - "CSR 存在且状态为 Pending"
    - "有 certificates.k8s.io/sign 权限"

  error_codes:
    "NotFound": "CSR 不存在"
    "AlreadyExists": "CSR 已被批准"
```

### 5.12 kubectl_certificate_deny

```yaml
tool:
  name: kubectl_certificate_deny
  description: 拒绝 CertificateSigningRequest
  category: REMEDIATION
  
  parameters:
    type: object
    required: ["name"]
    properties:
      name:
        type: string
        description: CSR 名称
        required: true

  action: "拒绝 CSR，阻止证书签发"
  
  risk_level: HIGH
  rollback:
    enabled: false  # 拒绝后无法撤回

  use_cases:
    - "拒绝未授权的证书请求"
    - "阻止可疑的 kubelet 请求"

  error_codes:
    "NotFound": "CSR 不存在"
    "AlreadyExists": "CSR 已被处理"
```

---

## 6. 监控指标类工具 (MONITORING)

### 6.1 kubectl_proxy

```yaml
tool:
  name: kubectl_proxy
  description: 启动 API Server 代理（用于访问 Kubernetes API）
  category: MONITORING
  
  parameters:
    type: object
    properties:
      port:
        type: integer
        description: 本地监听端口
        required: false
        default: 8001
      
      address:
        type: string
        description: 监听地址
        required: false
        default: "127.0.0.1"

  action: "在本地启动代理，访问集群 API"
  
  risk_level: LOW
  rollback:
    enabled: true
    method: "Ctrl+C 停止或 kill <pid>"

  use_cases:
    - "本地访问 Kubernetes API"
    - "测试 API endpoints"
    - "绕过集群网络限制"

  error_codes:
    "PortInUse": "端口已被占用"
```

### 6.2 kubectl_auth_can_i

```yaml
tool:
  name: kubectl_auth_can_i
  description: 检查当前用户的 RBAC 权限
  category: MONITORING
  
  parameters:
    type: object
    required: ["verb", "resource"]
    properties:
      verb:
        type: string
        description: 操作动词
        required: true
        enum: ["get", "list", "create", "update", "delete", "deletecollection", "patch", "watch", "exec", "approve", "deny"]
      
      resource:
        type: string
        description: 资源类型
        required: true
      
      namespace:
        type: string
        description: 命名空间
        required: false
      
      subresource:
        type: string
        description: 子资源
        required: false

  output:
    format: text
    values: ["yes", "no"]

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "排查 RBAC 权限问题"
    - "验证 ServiceAccount 权限"
    - "调试 Forbidden 错误"

  error_codes: []
```

### 6.3 kubectl_api_versions

```yaml
tool:
  name: kubectl_api_versions
  description: 列出集群支持的 API 版本
  category: MONITORING
  
  parameters:
    type: object

  output:
    format: text/list
    groups:
      - "admissionregistration.k8s.io/v1"
      - "apiextensions.k8s.io/v1"
      - "apiregistration.k8s.io/v1"
      - "apps/v1"
      - "batch/v1"
      - "certificates.k8s.io/v1"
      - ...

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "确认集群支持的 CRD 版本"
    - "排查 API 版本不兼容"

  error_codes: []
```

### 6.4 kubectl_api_resources

```yaml
tool:
  name: kubectl_api_resources
  description: 列出集群支持的资源类型
  category: MONITORING
  
  parameters:
    type: object
    properties:
      namespaced:
        type: boolean
        description: 仅显示命名空间级别的资源
        required: false
      
      api_group:
        type: string
        description: API 组过滤
        required: false
      
      wide:
        type: boolean
        description: 显示更多信息
        required: false
        default: false

  output:
    format: table
    columns: ["NAME", "APIVERSION", "NAMESPACED", "KIND", "VERBS"]

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "查看集群支持的所有资源"
    - "确认 CRD 是否注册"

  error_codes: []
```

### 6.5 kubectl_cluster_info

```yaml
tool:
  name: kubectl_cluster_info
  description: 获取集群信息
  category: MONITORING
  
  parameters:
    type: object

  output:
    format: text
    fields:
      - Kubernetes control plane
      - CoreDNS endpoints
      - etcd server

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "确认集群是否正常运行"
    - "查看 API Server 地址"

  error_codes: []
```

### 6.6 kubectl_version

```yaml
tool:
  name: kubectl_version
  description: 获取客户端和服务器版本
  category: MONITORING
  
  parameters:
    type: object
    properties:
      output:
        type: string
        description: 输出格式
        required: false
        enum: ["yaml", "json", "short"]
        default: "short"

  output:
    format: text/yaml/json
    clientVersion: {major, minor, gitVersion, gitCommit}
    serverVersion: {major, minor, gitVersion, gitCommit}

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "确认 kubectl 版本"
    - "确认集群版本"
    - "版本兼容性检查"

  error_codes:
    "ConnectionRefused": "无法连接到 API Server"
```

### 6.7 kubectl_get --watch

```yaml
tool:
  name: kubectl_watch
  description: 持续监控资源变化
  category: MONITORING
  
  parameters:
    type: object
    required: ["resource"]
    properties:
      resource:
        type: string
        description: 资源类型
        required: true
      
      name:
        type: string
        description: 资源名称（可选，监控特定资源）
        required: false
      
      namespace:
        type: string
        description: 命名空间
        required: false
      
      timeout:
        type: integer
        description: 监控超时时间（秒）
        required: false

  output:
    format: streaming text
    events: ["ADDED", "MODIFIED", "DELETED"]

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "实时观察 Pod 创建过程"
    - "监控事件流"
    - "观察滚动更新进度"

  error_codes:
    "Timeout": "监控超时"
```

### 6.8 kubectl_wait

```yaml
tool:
  name: kubectl_wait
  description: 等待资源满足条件
  category: MONITORING
  
  parameters:
    type: object
    required: ["resource", "condition"]
    properties:
      resource:
        type: string
        description: 资源类型.名称
        required: true
        example: "pod/my-pod"
      
      namespace:
        type: string
        description: 命名空间
        required: false
      
      condition:
        type: string
        description: 等待的条件
        required: true
        enum: ["delete", "exists", "jsonpath", "custom"]
      
      timeout:
        type: integer
        description: 超时时间（秒）
        required: false
        default: 600

  output:
    format: text
    success: "resource <type>/<name> condition met"
    timeout: "error: timed out waiting for condition"

  risk_level: LOW
  side_effects: []
  rollback: false
  idempotent: true

  use_cases:
    - "等待 Pod Running"
    - "等待 Job 完成"
    - "等待 PVC Bound"

  error_codes:
    "Timeout": "条件未在超时内满足"
```

---

## 7. 工具调用约束

### 7.1 风险等级定义

| 等级 | 说明 | 需要确认 | 回滚支持 |
|------|------|---------|----------|
| LOW | 只读操作，无副作用 | 否 | N/A |
| MEDIUM | 修改状态但可恢复 | 建议 | 支持 |
| HIGH | 破坏性操作，不可逆 | 必须 | 不支持 |

### 7.2 调用频率限制

- 诊断类工具：无限制
- 配置类工具：每分钟不超过 10 次
- 修复类工具：每分钟不超过 3 次
- 监控类工具：无限制

### 7.3 必需权限

Agent 调用任何工具前必须验证：
1. ServiceAccount 有足够的 RBAC 权限
2. 不违反 PodSecurityPolicy/OPA 策略
3. 不超出 ResourceQuota 限制

---

## 8. 工具选择决策树

```
用户报告问题
    │
    ▼
┌─────────────────────────────────────────┐
│ 症状分类                                 │
├─────────────────────────────────────────┤
│ │                                        │
│ ▼                                        │
│ ┌─────────┐ ┌─────────┐ ┌─────────┐     │
│ │ Pod异常  │ │ 节点异常  │ │ 网络异常  │     │
│ └────┬────┘ └────┬────┘ └────┬────┘     │
│      │            │            │           │
│      ▼            ▼            ▼           │
│  get pods     get nodes   get svc        │
│  describe   describe    describe         │
│  logs       top node   endpoints         │
│  events     events     networkpolicy     │
└─────────────────────────────────────────┘
```

---

**下一步行动**: 继续完善 Tool Schema，增加更多云厂商特定工具，补充错误处理场景。
