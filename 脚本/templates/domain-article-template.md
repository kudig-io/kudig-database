---
title: Domain Article Template
summary: 'title: "{{主题名称}}" description: "{{一句话摘要：描述本文核心主题、技术定位和生产价值}}" category:
  "{{domain-NAME}}"              # 对应 domain-N-xxx 目录 tags: [k8s, {{component}}, {{tag1}},
  {{tag2}}] k8s_versions:'
category: general
tags:
- domain-article-template
tier: supporting
created: '2026-07-01'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# {{主题名称}}

> **模板版本**: 2.0
> **最后更新**: 2026-05
> **文档类型**: 知识域深度文档
> **适用版本**: [[实体/kubernetes.md|kubernetes]] v1.28 - v1.32

---

## YAML Front Matter

```yaml
---
title: "{{主题名称}}"
description: "{{一句话摘要：描述本文核心主题、技术定位和生产价值}}"
category: "{{domain-NAME}}"              # 对应 domain-N-xxx 目录
tags: [k8s, {{component}}, {{tag1}}, {{tag2}}]
k8s_versions:
  - "1.28"
  - "1.29"
  - "1.30"
  - "1.31"
  - "1.32"
last_updated: "{{YYYY-MM}}"
authors:
  - name: "{{姓名}}"
    role: "{{角色}}"
reviewers: []
difficulty: "intermediate"               # beginner | intermediate | advanced | expert
# ---- New Reading Experience Fields ----
reading_level: "intermediate"            # beginner | intermediate | advanced | expert (同 difficulty)
audience: ["SRE", "Ops Engineer"]        # 目标读者: SRE / DevOps / Developer / Architect
estimated_read_time: "15min"            # 预计阅读时间: "5min" / "30min" / "1h"
prerequisites:                          # 前置知识依赖（文档路径或主题名）
  - "集群基础"
  - "basic-linux-commands"
# ---- Cross-References (统一格式) ----
cross_refs:
  - type: "domain"
    path: "../domain-{{N}}-{{name}}/{{doc}}.md"
    label: "{{说明}}"
  - type: "fta"
    path: "../故障诊断/FTA故障树/list/{{component}}-fta.md"
    label: "{{说明}}"
  - type: "cheatsheet"
    path: "../系统基础/topic-cheat-sheet/{{cheat-sheet}}.md"
    label: "{{说明}}"
related_docs:
  - path: "../domain-{{N}}-{{name}}/{{doc}}.md"
    type: "depth"
    desc: "{{说明}}"
---
```

---

## 1. 概述

{{概述段落：
- 该技术/组件是什么
- 在 Kubernetes 生态中的定位
- 为什么重要（生产环境视角）
- 解决什么核心问题
}}

### 1.1 核心价值

> 本文档面向的人群、需要的前置知识、以及学完后的能力预期。

| 维度 | 说明 |
|:---|:---|
| **目标读者** | {{开发者/运维/SRE/架构师}} |
| **前置知识** | {{需要的背景知识}} |
| **学习成果** | {{学完能做什么}} |
| **难度等级** | 🔰 入门 / 📘 进阶 / ⚡ 高级 / 🏆 专家 |

### 1.2 适用版本

| 组件 | 版本范围 | 兼容性说明 |
|:---|:---|:---|
| Kubernetes | v1.28 - v1.32 | 核心功能稳定，差异见 Section 10 |
| {{相关组件A}} | {{版本范围}} | {{差异说明}} |
| {{相关组件B}} | {{版本范围}} | {{差异说明}} |

### 1.3 与其他知识域的关联

```
本文档关联的知识域:

  上游依赖                     下游应用
  ┌──────────┐               ┌──────────┐
  │ 架构基础  │───►  本文档  ◄──│ 故障排查  │
  │ (集群基础)│               │(故障诊断)│
  └──────────┘               └──────────┘
        │                         ▲
        ▼                         │
  ┌──────────┐               ┌──────────┐
  │ 设计原理  │               │ 平台运维  │
  │ (集群基础)│               │ (平台工程)│
  └──────────┘               └──────────┘

交叉引用: [架构基础](./集群基础/01-kubernetes-architecture-overview.md) |
         [设计原理](./集群基础/01-design-principles-foundations.md)
```

---

## 2. 架构与原理

{{核心架构说明，建议包含：
- 架构图（Mermaid 或文字描述）
- 核心组件及职责
- 工作流程
}}

### 2.1 系统架构图

```mermaid
graph TD
    A[客户端/用户] --> B[{{组件A}}]
    B --> C[{{组件B}}]
    C --> D[{{组件C}}]
    C --> E[{{组件D}}]

    B --> F[控制平面]
    C --> G[数据平面]

    style A fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style B fill:#22c55e,stroke:#166534,color:#fff
    style C fill:#22c55e,stroke:#166534,color:#fff
    style F fill:#f59e0b,stroke:#b45309,color:#fff
    style G fill:#a855f7,stroke:#6b21a8,color:#fff
```

### 2.2 核心组件及职责

| 组件 | 名称 | 职责 | 关键配置 |
|:---|:---|:---|:---|
| {{组件A}} | {{名称}} | {{职责描述}} | `{{key: value}}` |
| {{组件B}} | {{名称}} | {{职责描述}} | `{{key: value}}` |
| {{组件C}} | {{名称}} | {{职责描述}} | `{{key: value}}` |

### 2.3 工作流程

> 描述从请求到完成的完整流程，包含所有关键决策点和状态转换。

```
步骤1: {{操作名称}}
  触发条件: {{条件}}
  执行组件: {{组件}}
  结果: {{结果}}

步骤2: {{操作名称}}
  触发条件: {{条件}}
  执行组件: {{组件}}
  结果: {{结果}}
```

### 2.4 数据结构与存储模型

> 对于需要理解底层数据模型的组件，描述关键数据结构和存储路径。

| 数据结构 | 用途 | 存储位置 | 持久性 |
|:---|:---|:---|:---:|
| {{结构A}} | {{用途}} | {{路径}} | 持久/临时 |
| {{结构B}} | {{用途}} | {{路径}} | 持久/临时 |

---

## 3. 核心概念

### 3.1 {{概念1}}

{{详细说明}}

**关键术语**：
- **{{术语A}}**: {{定义}}
- **{{术语B}}**: {{定义}}

### 3.2 {{概念2}}

{{详细说明}}

### 3.3 {{概念3}}

{{详细说明}}

### 3.4 概念间关系图

```mermaid
graph LR
    A[{{概念A}}] -->|关系| B[{{概念B}}]
    B -->|关系| C[{{概念C}}]
    A -->|关系| C

    style A fill:#3b82f6,stroke:#1d4ed8,color:#fff
    style B fill:#22c55e,stroke:#166534,color:#fff
    style C fill:#a855f7,stroke:#6b21a8,color:#fff
```

---

## 4. 配置与部署

### 4.1 前提条件

> 开始配置前需要满足的环境要求。

| 条件 | 要求 | 验证命令 |
|:---|:---|:---|
| 集群版本 | ≥ v1.28 | `kubectl version --short` |
| 权限 | {{RBAC要求}} | `kubectl auth can-i` |
| 前置组件 | {{组件列表}} | `kubectl get pods -n {{ns}}` |

### 4.2 基本配置

> 最小配置示例，可用于测试环境。

```yaml
# {{配置说明}} - K8s {{版本}}+
apiVersion: {{API版本}}
kind: {{资源类型}}
metadata:
  name: {{名称}}
spec:
  {{spec字段}}:
    {{值}}    # {{中文注释说明}}
```

### 4.3 生产级配置

> 生产环境推荐配置，包含高可用、资源限制、监控注解、安全加固。

```yaml
# 生产环境推荐配置
# 包含：高可用、资源限制、监控注解、安全加固
{{生产级完整配置}}
```

**生产配置要点**：

| 配置项 | 测试环境 | 生产环境 | 原因 |
|:---|:---|:---|:---|
| {{配置A}} | {{值A}} | {{值B}} | {{原因}} |
| {{配置B}} | {{值A}} | {{值B}} | {{原因}} |

### 4.4 安装与卸载

```bash
# 安装
{{安装命令}}

# 验证
{{验证命令}}

# 卸载
{{卸载命令}}
```

### 4.5 多云厂商差异说明

> 不同云厂商托管 K8s 中的实现差异。

| 云厂商 | 产品 | 差异说明 | 特殊配置 |
|:---|:---|:---|:---|
| AWS EKS | EKS | {{差异}} | {{配置}} |
| GCP GKE | GKE | {{差异}} | {{配置}} |
| Azure AKS | AKS | {{差异}} | {{配置}} |
| 阿里云 ACK | ACK | {{差异}} | {{配置}} |
| 腾讯云 TKE | TKE | {{差异}} | {{配置}} |
| 华为云 CCE | CCE | {{差异}} | {{配置}} |

---

## 5. 最佳实践

### 5.1 必须项（🔴 生产环境必须遵守）

| 实践 | 说明 | 违反后果 |
|:---|:---|:---|
| {{实践1}} | {{说明}} | {{后果}} |
| {{实践2}} | {{说明}} | {{后果}} |

### 5.2 推荐项（🟡 强烈建议遵循）

| 实践 | 说明 | 收益 |
|:---|:---|:---|
| {{实践1}} | {{说明}} | {{收益}} |
| {{实践2}} | {{说明}} | {{收益}} |

### 5.3 可选项（🟢 根据实际情况选用）

| 实践 | 说明 | 适用场景 |
|:---|:---|:---|
| {{实践1}} | {{说明}} | {{场景}} |
| {{实践2}} | {{说明}} | {{场景}} |

### 5.4 反模式（❌ 避免踩坑）

| 反模式 | 问题 | 正确做法 |
|:---|:---|:---|
| {{反模式A}} | {{问题描述}} | {{正确做法}} |
| {{反模式B}} | {{问题描述}} | {{正确做法}} |

---

## 6. 监控与告警

### 6.1 关键指标体系

> 覆盖金色信号（Google SRE）四要素：延迟、流量、错误、饱和度。

| 指标名称 | 类型 | 说明 | 单位 | 正常范围 | 告警阈值 |
|:---|:---|:---|:---|:---|:---|
| {{metric_name}} | Latency/Traffic/Errors/Saturation | {{说明}} | {{单位}} | {{范围}} | {{阈值}} |

### 6.2 Prometheus 采集配置

```yaml
# Prometheus Operator ServiceMonitor 示例
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: {{component}}
  labels:
    release: prometheus
spec:
  selector:
    matchLabels:
      app: {{component}}
  endpoints:
    - port: metrics
      interval: 15s
      path: /metrics
```

### 6.3 Prometheus 告警规则

> 完整的生产级告警规则，包含多个告警级别和告警抑制。

```yaml
groups:
- name: {{component}}-alerts
  interval: 30s
  rules:
  # === 关键告警 (Critical) ===
  - alert: {{ComponentName}}Down
    expr: up{job="{{component}}"} == 0
    for: 1m
    labels:
      severity: critical
      team: {{team}}
    annotations:
      summary: "{{组件}}实例宕机"
      description: "{{组件}} {{$labels.instance}} 已宕机超过 1 分钟"
      runbook_url: "../故障诊断/FTA故障树/list/{{component}}-fta.md"
      grafana_dashboard: "/d/{{dashboard-id}}/{{component}}-overview"

  - alert: {{ComponentName}}HighErrorRate
    expr: |
      (
        sum(rate({{metric_name}}_errors_total[5m])) by (instance)
        /
        sum(rate({{metric_name}}_requests_total[5m])) by (instance)
      ) > 0.05
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "{{组件}}错误率超过 5%"
      description: "实例 {{$labels.instance }} 错误率: {{$value | humanizePercentage}}"

  # === 警告告警 (Warning) ===
  - alert: {{ComponentName}}HighLatency
    expr: |
      histogram_quantile(0.99,
        sum(rate({{metric_name}}_latency_bucket[5m])) by (le, instance)
      ) > 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "{{组件}} P99 延迟超过 1 秒"
      description: "P99 延迟: {{$value | humanizeDuration}}"

  # === 信息告警 (Info) ===
  - alert: {{ComponentName}}HighMemoryUsage
    expr: |
      (node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes)
      / node_memory_MemTotal_bytes > 0.85
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "节点内存使用率超过 85%"
```

### 6.4 Grafana 仪表盘关键面板

| 面板名称 | 查询表达式 | 用途 |
|:---|:---|:---|
| 请求率 | `sum(rate({{metric}}[5m])) by (method)` | 流量监控 |
| P99 延迟 | `histogram_quantile(0.99, sum(rate({{metric}}[5m])) by (le))` | 延迟监控 |
| 错误率 | `sum(rate({{metric}}_errors[5m])) / sum(rate({{metric}}_total[5m]))` | 错误监控 |
| 饱和度 | `{{saturation_metric}}` | 资源饱和度 |

### 6.5 SLO 关联

> 将监控指标关联到具体的 SLO/SLI。

| SLO 名称 | SLI 指标 | 目标值 | 当前值 | 状态 |
|:---|:---|:---|:---|:---|
| {{SLO名称}} | {{SLI}} | {{目标}} | {{当前}} | ✅/⚠️/❌ |

---

## 7. 故障排查

### 7.1 常见问题速查

| 现象 | 可能原因 | 置信度 | 排查命令 | 解决方案 |
|:---|:---|:---:|:---|:---|
| {{现象1}} | {{原因1}} | 高 | `{{命令}}` | {{方案}} |
| {{现象1}} | {{原因2}} | 中 | `{{命令}}` | {{方案}} |
| {{现象2}} | {{原因1}} | 高 | `{{命令}}` | {{方案}} |

### 7.2 诊断命令集

> 按诊断阶段组织，从浅到深逐步排查。

**阶段一：快速检查（1 分钟内）**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 资源状态检查
kubectl get {{resource}} -n {{namespace}} -o wide
kubectl describe {{resource}} {{name}} -n {{namespace}}

# 健康检查
kubectl rollout status {{resource}} {{name}} -n {{namespace}}

# 快速日志
kubectl logs -n {{namespace}} {{pod-name}} --tail=50
```
**阶段二：深度诊断（5 分钟内）**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 详细事件分析
kubectl get events -n {{namespace}} --sort-by='.lastTimestamp' | tail -30

# 资源详情
kubectl get {{resource}} {{name}} -n {{namespace}} -o yaml

# 关联资源检查
kubectl get all -n {{namespace}} -l {{label}}
```
**阶段三：专家级诊断**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 网络诊断（如适用）
kubectl exec -it {{pod}} -n {{namespace}} -- /bin/sh -c "netstat -tlnp"

# 特定组件日志
kubectl logs -n {{namespace}} {{pod-name}} --tail=200 --all-containers | grep -E '{{pattern}}'

# 指标验证
curl -s localhost:{{port}}/metrics | grep {{metric}}
```
### 7.3 FTA 故障树入口

> 指向对应的 FTA 故障树文档，系统化排查。

| 问题场景 | FTA 文档 | 关键底事件 |
|:---|:---|:---|
| {{场景1}} | [{{component}}-fta.md](../故障诊断/FTA故障树/list/{{component}}-fta.md) | BE-1 / BE-2 / BE-3 |
| {{场景2}} | [{{component}}-fta.md](../故障诊断/FTA故障树/list/{{component}}-fta.md) | BE-4 / BE-5 |

### 7.4 降级方案

> 当主要方案不可用时的备选降级方案。

| 问题场景 | 主要方案 | 降级方案 | 降级后影响 |
|:---|:---|:---|:---|
| {{场景}} | {{主方案}} | {{降级}} | {{影响}} |

---

## 8. 安全加固

### 8.1 RBAC 配置

```yaml
# 最小权限原则：只授予完成任务所需的最小权限
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: {{component}}-operator
  namespace: {{namespace}}
rules:
- apiGroups: [""]
  resources: ["pods", "services"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "watch", "update"]
```

### 8.2 网络策略

```yaml
# 限制 {{component}} 只与必要的组件通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: {{component}}-netpol
  namespace: {{namespace}}
spec:
  podSelector:
    matchLabels:
      app: {{component}}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: {{client-app}}
    ports:
    - protocol: TCP
      port: {{port}}
  egress:
  - to:
    - podSelector:
        matchLabels:
          app: {{server-app}}
    ports:
    - protocol: TCP
      port: {{port}}
```

### 8.3 安全上下文

```yaml
securityContext:
  runAsNonRoot: true
  runAsUser: 10000
  runAsGroup: 10000
  fsGroup: 10000
  capabilities:
    drop:
    - ALL
    add:
    - NET_BIND_SERVICE
```

---

## 9. 性能调优

### 9.1 关键参数调优

| 参数 | 默认值 | 推荐值 | 适用场景 | 调整原因 |
|:---|:---|:---|:---|:---|
| {{paramA}} | {{default}} | {{recommended}} | {{场景}} | {{原因}} |
| {{paramB}} | {{default}} | {{recommended}} | {{场景}} | {{原因}} |

### 9.2 瓶颈识别

> 常见的性能瓶颈及排查方法。

| 瓶颈类型 | 症状 | 诊断方法 | 解决方案 |
|:---|:---|:---|:---|
| {{类型}} | {{症状}} | {{方法}} | {{方案}} |

### 9.3 基准测试

```bash
# {{测试名称}}
{{测试命令}}

# 预期结果
{{预期}}
```

---

## 10. 版本差异与兼容性

### 10.1 功能差异表

| 功能 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | 备注 |
|:---|:---:|:---:|:---:|:---:|:---:|:---|
| {{功能A}} | ✅ | ✅ | ✅ | ✅ | ✅ | 稳定功能 |
| {{功能B}} | ⚠️ | ✅ | ✅ | ✅ | ✅ | v1.29+ 稳定 |
| {{功能C}} | ❌ | ❌ | 🔶 | ✅ | ✅ | v1.30+ Alpha |

### 10.2 API 版本差异

| 资源类型 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|:---|:---|:---|:---|:---|:---|
| {{Resource}} | {{api}} | {{api}} | {{api}} | {{api}} | {{api}} |

### 10.3 配置语法变化

```yaml
# v1.28-v1.30 语法（即将废弃）
{{旧语法示例}}

# v1.31+ 推荐语法
{{新语法示例}}
```

---

## 11. 参考资料

### 官方文档
- [Kubernetes 官方文档](https://kubernetes.io/docs/)
- [{{组件}}官方文档]({{URL}})

### 深度阅读
- [设计原理 - {{相关文档}}](../集群基础/{{doc}}.md)
- [架构基础 - {{相关文档}}](../集群基础/{{doc}}.md)

### 相关博客/论文
- [{{标题}}]({{URL}}) - {{说明}}

---

## 12. 相关文档

| 类型 | 文档 | 说明 |
|:---|:---|:---|
| 前置阅读 | [{{文档名}}](../domain-{{N}}-{{name}}/{{doc}}.md) | {{说明}} |
| 深入阅读 | [{{文档名}}](../domain-{{N}}-{{name}}/{{doc}}.md) | {{说明}} |
| 速查参考 | [{{速查卡}}](../系统基础/速查卡/{{cheat-sheet}}.md) | {{说明}} |
| 故障排查 | [{{排障文档}}](../故障诊断/高级排障/{{doc}}.md) | {{说明}} |
| FTA | [{{故障树}}](../故障诊断/FTA故障树/list/{{component}}-fta.md) | {{说明}} |
| Skill | [{{技能}}](../故障诊断/技能体系/{{NN}}-{{scenario}}.md) | {{说明}} |

---

## 版本历史

| 日期 | 版本 | 变更 | 作者 |
|:---:|:---:|:---|:---:|
| YYYY-MM | v1.0 | 初始版本 | {{姓名}} |
| YYYY-MM | v2.0 | {{变更描述}} | {{姓名}} |

<!-- risk-assessed -->
