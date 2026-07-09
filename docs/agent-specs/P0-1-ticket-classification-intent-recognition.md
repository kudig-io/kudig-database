---
title: 工单分类体系与意图识别语料库
description: '**关联**: P0-2 多技能协同协议, P0-3 会话上下文管理'
summary: '**关联**: P0-2 多技能协同协议, P0-3 会话上下文管理'
category: general
tags:
- k8s
- etcd
- apiserver
- kubelet
- scheduler
- coredns
- containerd
- redis
- mysql
- postgresql
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 工单分类体系与意图识别语料库 是什么
- 如何 工单分类体系与意图识别语料库
trigger_keywords:
- 工单分类体系与意图识别语料库
prerequisites:
- kubectl-basics
- etcd-basics
- kafka-basics
- redis-basics
- mysql-basics
- backup-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单分类体系与意图识别语料库

> **版本**: v1.0
> **创建日期**: 2026-05-18
> **用途**: 智能体 Agent 工单路由引擎的分类与意图识别基础
> **关联**: P0-2 多技能协同协议, P0-3 会话上下文管理

---

## 1. 工单分类体系

### 1.1 工单大类 (Ticket Category)

| 大类 ID | 大类名称 | 说明 | 爆炸半径 | 典型 SLA |
|---------|---------|------|---------|----------|
| TC-INFRA | 基础设施 | 节点、网络、存储、控制平面等底层组件问题 | 全集群/多业务 | P0: 15min |
| TC-APP | 应用层 | Pod、Deployment、Service、ConfigMap 等应用运行时问题 | 单业务/多租户 | P1: 30min |
| TC-SEC | 安全合规 | 认证、授权、证书、审计、合规等安全事件 | 集群/审计影响 | P0: 立即 |
| TC-DATA | 数据层 | 数据库、缓存、消息队列、数据备份等数据相关问题 | 数据丢失风险 | P0: 立即 |

### 1.2 工单子类 (Ticket Subcategory)

#### TC-INFRA 基础设施子类

| 子类 ID | 子类名称 | 覆盖范围 | 典型 Skill |
|---------|---------|---------|-----------|
| TC-INFRA-NODE | 节点问题 | 节点 NotReady/Unknown/压力, kubelet 异常, 容器运行时异常 | SKILL-NODE-001 |
| TC-INFRA-NET | 网络问题 | CNI 异常, DNS 解析失败, 网络策略冲突, 网段冲突 | SKILL-NET-001/002/003 |
| TC-INFRA-STORE | 存储问题 | PVC Pending, CSI 驱动异常, 存储类配置错误, 卷挂载失败 | SKILL-STORE-001 |
| TC-INFRA-CP | 控制平面问题 | etcd 异常, API Server 不可用, Scheduler 调度失败, Controller Manager 异常 | SKILL-CP-001 |
| TC-INFRA-SCALE | 弹性伸缩问题 | HPA/VPA 不触发, Cluster Autoscaler 异常, 节点池问题 | SKILL-SCALE-001 |
| TC-INFRA-UPGRADE | 升级迁移 | 集群升级失败, 节点升级卡住, 版本兼容性问题 | 暂无对应 Skill |

#### TC-APP 应用层子类

| 子类 ID | 子类名称 | 覆盖范围 | 典型 Skill |
|---------|---------|---------|-----------|
| TC-APP-POD | Pod 生命周期 | CrashLoopBackOff, OOMKilled, Pending 调度, 镜像拉取失败 | SKILL-POD-001/002, SKILL-IMAGE-001 |
| TC-APP-WORKLOAD | 工作负载管理 | Deployment 滚动更新卡住, StatefulSet 有序部署, DaemonSet 调度异常 | SKILL-WORK-001 |
| TC-APP-SVC | 服务连通性 | Service 无 Endpoints, Ingress 路由失败, Gateway 配置错误 | SKILL-NET-002, SKILL-NET-003 |
| TC-APP-CONFIG | 配置管理 | ConfigMap/Secret 未生效, 配置热更新失败, 编码问题 | SKILL-CONFIG-001 |
| TC-APP-INGRESS | 入口流量 | Ingress 404/502/503, 证书错误, 路径重写失败 | SKILL-NET-003 |

#### TC-SEC 安全合规子类

| 子类 ID | 子类名称 | 覆盖范围 | 典型 Skill |
|---------|---------|---------|-----------|
| TC-SEC-CERT | 证书安全 | 证书过期, TLS 握手失败, kubelet 证书轮换失败 | SKILL-SEC-001 |
| TC-SEC-RBAC | 权限安全 | RBAC Forbidden, ServiceAccount 权限不足, 策略冲突 | SKILL-SEC-002 |
| TC-SEC-INCIDENT | 安全事件 | 疑似入侵, 异常访问, 审计告警, 合规违规 | SKILL-SECURITY-001 |
| TC-SEC-PSP | Pod 安全策略 | PSA 拒绝, SecurityContext 冲突, 权限提升 | 暂无对应 Skill |

#### TC-DATA 数据层子类

| 子类 ID | 子类名称 | 覆盖范围 | 典型 Skill |
|---------|---------|---------|-----------|
| TC-DATA-DB | 数据库问题 | MySQL/PostgreSQL 连接异常, 读写失败, 主从复制中断 | 暂无对应 Skill |
| TC-DATA-CACHE | 缓存问题 | Redis 集群异常, 缓存穿透/雪崩, 内存耗尽 | 暂无对应 Skill |
| TC-DATA-MQ | 消息队列问题 | Kafka/RabbitMQ 连接异常, 消费积压, 分区不可用 | 暂无对应 Skill |
| TC-DATA-BACKUP | 数据备份 | 备份失败, 快照损坏, 恢复超时, RPO 超标 | 暂无对应 Skill |

---

## 2. 意图识别语料库 (Intent Recognition Corpus)

### 2.1 语料格式

每条语料为 JSONL 格式，包含：
```json
{
  "text": "工单描述原文",
  "lang": "zh|en|混",
  "category": "TC-INFRA-NODE",
  "skill_id": "SKILL-NODE-001",
  "keywords": ["关键词1", "关键词2"],
  "severity_hint": "P0|P1|P2|P3",
  "confidence": 0.0-1.0
}
```

### 2.2 TC-INFRA-NODE 意图语料

```jsonl
{"text": "节点 NotReady，Pod 被驱逐", "lang": "zh", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["NotReady", "驱逐", "节点"], "severity_hint": "P0", "confidence": 0.95}
{"text": "Node is not ready, kubelet stopped posting status", "lang": "en", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["NotReady", "kubelet", "node"], "severity_hint": "P0", "confidence": 0.95}
{"text": "节点状态显示 Unknown，kubelet 无法连接 apiserver", "lang": "zh", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["Unknown", "kubelet", "apiserver"], "severity_hint": "P0", "confidence": 0.90}
{"text": "Node status flapping between Ready and NotReady", "lang": "en", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["flapping", "Ready", "NotReady"], "severity_hint": "P1", "confidence": 0.85}
{"text": "节点磁盘压力告警，DiskPressure 为 True", "lang": "zh", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["DiskPressure", "磁盘压力", "节点"], "severity_hint": "P1", "confidence": 0.90}
{"text": "节点内存压力，MemoryPressure 导致 Pod 被驱逐", "lang": "zh", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["MemoryPressure", "内存压力", "驱逐"], "severity_hint": "P1", "confidence": 0.90}
{"text": "kubelet 进程崩溃，节点显示 NotReady", "lang": "zh", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["kubelet", "崩溃", "NotReady"], "severity_hint": "P0", "confidence": 0.90}
{"text": "containerd 异常，节点上容器创建失败", "lang": "zh", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["containerd", "容器创建失败", "节点"], "severity_hint": "P1", "confidence": 0.80}
{"text": "Node has disk pressure, pods being evicted", "lang": "en", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["disk pressure", "evicted", "node"], "severity_hint": "P1", "confidence": 0.90}
{"text": "多个工作节点同时 NotReady，集群可用性受影响", "lang": "zh", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["多个节点", "NotReady", "可用性"], "severity_hint": "P0", "confidence": 0.95}
{"text": "控制平面节点 NotReady，集群 API 响应异常", "lang": "zh", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["控制平面", "NotReady", "API响应"], "severity_hint": "P0", "confidence": 0.95}
{"text": "节点 SSH 无法连接，但 kubelet 仍在运行", "lang": "zh", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["SSH", "无法连接", "kubelet"], "severity_hint": "P1", "confidence": 0.75}
{"text": "Node Lease object not renewed for 5 minutes", "lang": "en", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["Lease", "not renewed", "5min"], "severity_hint": "P0", "confidence": 0.90}
{"text": "Kubelet not ready due toPLEG issues", "lang": "en", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["PLEG", "KubeletNotReady"], "severity_hint": "P1", "confidence": 0.85}
{"text": "节点网络分区，与 apiserver 通信中断", "lang": "zh", "category": "TC-INFRA-NODE", "skill_id": "SKILL-NODE-001", "keywords": ["网络分区", "通信中断", "apiserver"], "severity_hint": "P0", "confidence": 0.90}
```

### 2.3 TC-APP-POD 意图语料

```jsonl
{"text": "Pod 处于 CrashLoopBackOff 状态，无法正常运行", "lang": "zh", "category": "TC-APP-POD", "skill_id": "SKILL-POD-001", "keywords": ["CrashLoopBackOff", "无法运行", "Pod"], "severity_hint": "P1", "confidence": 0.95}
{"text": "Pod OOMKilled，内存限制不足导致被 kill", "lang": "zh", "category": "TC-APP-POD", "skill_id": "SKILL-POD-001", "keywords": ["OOMKilled", "内存限制", "OOM"], "severity_hint": "P1", "confidence": 0.95}
{"text": "Pod 一直处于 Pending 状态，调度失败", "lang": "zh", "category": "TC-APP-POD", "skill_id": "SKILL-POD-002", "keywords": ["Pending", "调度失败", "Pod"], "severity_hint": "P1", "confidence": 0.95}
{"text": "Pod image pull failure, ErrImagePull", "lang": "en", "category": "TC-APP-POD", "skill_id": "SKILL-IMAGE-001", "keywords": ["ImagePullBackOff", "ErrImagePull", "镜像"], "severity_hint": "P1", "confidence": 0.95}
{"text": "容器启动后立即退出，exit code 137", "lang": "zh", "category": "TC-APP-POD", "skill_id": "SKILL-POD-001", "keywords": ["exit code 137", "立即退出", "OOM"], "severity_hint": "P1", "confidence": 0.90}
{"text": "Pod 不断重启，每次运行几秒就崩溃", "lang": "zh", "category": "TC-APP-POD", "skill_id": "SKILL-POD-001", "keywords": ["不断重启", "崩溃", "CrashLoop"], "severity_hint": "P1", "confidence": 0.90}
{"text": "application container repeatedly crashing", "lang": "en", "category": "TC-APP-POD", "skill_id": "SKILL-POD-001", "keywords": ["repeatedly crashing", "container", "CrashLoop"], "severity_hint": "P1", "confidence": 0.90}
{"text": "Pod 被 OOMKilled (exit code 137)", "lang": "zh", "category": "TC-APP-POD", "skill_id": "SKILL-POD-001", "keywords": ["OOMKilled", "exit code 137", "内存"], "severity_hint": "P1", "confidence": 0.95}
{"text": "多个 Pod 同时 Pending，资源不足", "lang": "zh", "category": "TC-APP-POD", "skill_id": "SKILL-POD-002", "keywords": ["Pending", "资源不足", "多个Pod"], "severity_hint": "P2", "confidence": 0.85}
{"text": "Failed to schedule pod, no nodes matching affinity rules", "lang": "en", "category": "TC-APP-POD", "skill_id": "SKILL-POD-002", "keywords": ["FailedScheduling", "affinity", "no nodes"], "severity_hint": "P2", "confidence": 0.85}
```

### 2.4 TC-INFRA-NET 意图语料

```jsonl
{"text": "DNS 解析失败，服务无法发现", "lang": "zh", "category": "TC-INFRA-NET", "skill_id": "SKILL-NET-001", "keywords": ["DNS解析失败", "服务发现", "NXDOMAIN"], "severity_hint": "P1", "confidence": 0.95}
{"text": "CoreDNS Pod 不健康，DNS 解析异常", "lang": "zh", "category": "TC-INFRA-NET", "skill_id": "SKILL-NET-001", "keywords": ["CoreDNS", "不健康", "DNS异常"], "severity_hint": "P1", "confidence": 0.85}
{"text": "DNS resolution timeout, pods cannot reach external services", "lang": "en", "category": "TC-INFRA-NET", "skill_id": "SKILL-NET-001", "keywords": ["DNS timeout", "resolution", "external"], "severity_hint": "P1", "confidence": 0.90}
{"text": "Service 没有 Endpoints，Pod selector 不匹配", "lang": "zh", "category": "TC-INFRA-NET", "skill_id": "SKILL-NET-002", "keywords": ["无Endpoints", "selector", "不匹配"], "severity_hint": "P2", "confidence": 0.90}
{"text": "Service cluster IP cannot be accessed", "lang": "en", "category": "TC-INFRA-NET", "skill_id": "SKILL-NET-002", "keywords": ["ClusterIP", "cannot access", "Service"], "severity_hint": "P2", "confidence": 0.85}
{"text": "Ingress 返回 404，路由配置错误", "lang": "zh", "category": "TC-APP-INGRESS", "skill_id": "SKILL-NET-003", "keywords": ["404", "Ingress", "路由错误"], "severity_hint": "P2", "confidence": 0.90}
{"text": "Ingress gateway 503 error, backend service unavailable", "lang": "en", "category": "TC-APP-INGRESS", "skill_id": "SKILL-NET-003", "keywords": ["503", "gateway", "unavailable"], "severity_hint": "P1", "confidence": 0.85}
{"text": "NetworkPolicy 导致 Pod 之间无法通信", "lang": "zh", "category": "TC-INFRA-NET", "skill_id": "SKILL-NET-004", "keywords": ["NetworkPolicy", "无法通信", "隔离"], "severity_hint": "P2", "confidence": 0.80}
{"text": "跨节点的 Pod 网络不通", "lang": "zh", "category": "TC-INFRA-NET", "skill_id": "SKILL-NET-001", "keywords": ["跨节点", "网络不通", "Pod"], "severity_hint": "P1", "confidence": 0.85}
{"text": "Pod cannot resolve internal cluster domain", "lang": "en", "category": "TC-INFRA-NET", "skill_id": "SKILL-NET-001", "keywords": ["cannot resolve", "cluster domain", "DNS"], "severity_hint": "P1", "confidence": 0.90}
```

### 2.5 TC-SEC 意图语料

```jsonl
{"text": "证书过期，TLS 握手失败", "lang": "zh", "category": "TC-SEC-CERT", "skill_id": "SKILL-SEC-001", "keywords": ["证书过期", "TLS握手失败", "x509"], "severity_hint": "P0", "confidence": 0.95}
{"text": "x509: certificate has expired", "lang": "en", "category": "TC-SEC-CERT", "skill_id": "SKILL-SEC-001", "keywords": ["expired", "certificate", "x509"], "severity_hint": "P0", "confidence": 0.95}
{"text": "RBAC 权限不足，Forbidden 错误", "lang": "zh", "category": "TC-SEC-RBAC", "skill_id": "SKILL-SEC-002", "keywords": ["Forbidden", "RBAC", "权限不足"], "severity_hint": "P2", "confidence": 0.90}
{"text": "ServiceAccount 没有足够权限执行操作", "lang": "zh", "category": "TC-SEC-RBAC", "skill_id": "SKILL-SEC-002", "keywords": ["ServiceAccount", "权限", "Forbidden"], "severity_hint": "P2", "confidence": 0.85}
{"text": "ResourceQuota exceeded，资源配额超限", "lang": "zh", "category": "TC-SEC-RBAC", "skill_id": "SKILL-SEC-002", "keywords": ["ResourceQuota", "exceeded", "配额"], "severity_hint": "P2", "confidence": 0.90}
{"text": "疑似入侵行为，异常 Pod 创建", "lang": "zh", "category": "TC-SEC-INCIDENT", "skill_id": "SKILL-SECURITY-001", "keywords": ["入侵", "异常Pod", "异常创建"], "severity_hint": "P0", "confidence": 0.85}
{"text": "Security event: unauthorized access attempt detected", "lang": "en", "category": "TC-SEC-INCIDENT", "skill_id": "SKILL-SECURITY-001", "keywords": ["unauthorized", "access", "security"], "severity_hint": "P0", "confidence": 0.90}
{"text": "Audit log shows suspicious API calls", "lang": "en", "category": "TC-SEC-INCIDENT", "skill_id": "SKILL-SECURITY-001", "keywords": ["audit", "suspicious", "API calls"], "severity_hint": "P1", "confidence": 0.85}
```

### 2.6 TC-INFRA-STORE 意图语料

```jsonl
{"text": "PVC 一直处于 Pending 状态", "lang": "zh", "category": "TC-INFRA-STORE", "skill_id": "SKILL-STORE-001", "keywords": ["Pending", "PVC", "存储"], "severity_hint": "P1", "confidence": 0.95}
{"text": "PersistentVolumeClaim not bound, storage class missing", "lang": "en", "category": "TC-INFRA-STORE", "skill_id": "SKILL-STORE-001", "keywords": ["not bound", "StorageClass", "PVC"], "severity_hint": "P2", "confidence": 0.90}
{"text": "CSI driver 异常，卷挂载失败", "lang": "zh", "category": "TC-INFRA-STORE", "skill_id": "SKILL-STORE-001", "keywords": ["CSI", "挂载失败", "卷"], "severity_hint": "P1", "confidence": 0.85}
{"text": "存储卷只读，无法写入数据", "lang": "zh", "category": "TC-INFRA-STORE", "skill_id": "SKILL-STORE-001", "keywords": ["只读", "存储卷", "写入"], "severity_hint": "P0", "confidence": 0.90}
{"text": "Volume mount failed, device busy", "lang": "en", "category": "TC-INFRA-STORE", "skill_id": "SKILL-STORE-001", "keywords": ["mount failed", "device busy", "volume"], "severity_hint": "P1", "confidence": 0.85}
```

### 2.7 TC-DATA 意图语料

```jsonl
{"text": "MySQL 数据库连接失败，应用无法读写", "lang": "zh", "category": "TC-DATA-DB", "skill_id": null, "keywords": ["MySQL", "连接失败", "数据库"], "severity_hint": "P0", "confidence": 0.90}
{"text": "Redis 集群节点宕机，缓存服务不可用", "lang": "zh", "category": "TC-DATA-CACHE", "skill_id": null, "keywords": ["Redis", "宕机", "缓存"], "severity_hint": "P1", "confidence": 0.90}
{"text": "Kafka 消费者积压严重，消息处理延迟", "lang": "zh", "category": "TC-DATA-MQ", "skill_id": null, "keywords": ["Kafka", "积压", "消息"], "severity_hint": "P1", "confidence": 0.85}
{"text": "备份任务失败，Velero 无法完成快照", "lang": "zh", "category": "TC-DATA-BACKUP", "skill_id": null, "keywords": ["备份失败", "Velero", "快照"], "severity_hint": "P2", "confidence": 0.85}
{"text": "PostgreSQL 主从复制中断，数据一致性受损", "lang": "zh", "category": "TC-DATA-DB", "skill_id": null, "keywords": ["PostgreSQL", "主从复制", "中断"], "severity_hint": "P0", "confidence": 0.90}
{"text": "MongoDB 分片集群某个分片不可用", "lang": "zh", "category": "TC-DATA-DB", "skill_id": null, "keywords": ["MongoDB", "分片", "不可用"], "severity_hint": "P0", "confidence": 0.85}
```

---

## 3. 路由决策算法

### 3.1 多级路由流程

```
工单输入
    │
    ▼
┌─────────────────┐
│ L1: 关键词匹配    │ ← trigger_keywords 精确匹配
│ 置信度 ≥ 0.9    │   直接路由到对应 Skill
└────────┬────────┘
         │ 未匹配
         ▼
┌─────────────────┐
│ L2: 语义相似度   │ ← 计算与语料库的向量相似度
│ 置信度 ≥ 0.85   │   路由到最高相似度 category
└────────┬────────┘
         │ 未匹配
         ▼
┌─────────────────┐
│ L3: 规则引擎     │ ← 基于规则的启发式判断
│ (Entity + Pattern) │   提取关键实体+模式匹配
└────────┬────────┘
         │ 仍未匹配
         ▼
┌─────────────────┐
│ L4: 升级人工     │ ← 无法分类时转人工处理
│ 标记: UNCATEGORIZED │   并记录到反馈语料库
└─────────────────┘
```

### 3.2 关键词优先级表

| 优先级 | 关键词 | 映射 Category | 覆盖场景 |
|--------|--------|-------------|---------|
| P0 | "NotReady", "Unknown", "节点不可用" | TC-INFRA-NODE | 节点级问题 |
| P0 | "OOMKilled", "exit code 137", "内存溢出" | TC-APP-POD | Pod 内存问题 |
| P0 | "certificate expired", "TLS handshake" | TC-SEC-CERT | 证书过期 |
| P1 | "CrashLoopBackOff" | TC-APP-POD | Pod 崩溃重启 |
| P1 | "Pending", "调度失败", "FailedScheduling" | TC-APP-POD | Pod 调度问题 |
| P1 | "DNS", "NXDOMAIN", "解析失败" | TC-INFRA-NET | DNS 问题 |
| P1 | "Ingress", "404", "502", "503" | TC-APP-INGRESS | 入口流量问题 |
| P2 | "Forbidden", "RBAC", "权限" | TC-SEC-RBAC | 权限问题 |
| P2 | "PVC", "Pending", "存储" | TC-INFRA-STORE | 存储问题 |
| P2 | "ResourceQuota", "exceeded" | TC-SEC-RBAC | 配额超限 |
| P2 | "HPA", "不触发", "扩容" | TC-INFRA-SCALE | 弹性伸缩问题 |
| P2 | "Deployment", "滚动更新", "卡住" | TC-APP-WORKLOAD | 部署问题 |
| P3 | "CoreDNS", "coredns" | TC-INFRA-NET | DNS 服务问题 |
| P3 | "Secret", "ConfigMap", "配置" | TC-APP-CONFIG | 配置问题 |

### 3.3 置信度阈值

| 路由级别 | 阈值 | 动作 |
|----------|------|------|
| 高置信度 | ≥ 0.9 | 自动路由到对应 Skill |
| 中置信度 | 0.7 - 0.9 | 路由到 category，启动多技能协同 |
| 低置信度 | 0.5 - 0.7 | 路由到 category，降级为 L1-advisory 模式 |
| 未知 | < 0.5 | 升级人工处理 |

---

## 4. 工单状态机

### 4.1 工单状态定义

```
                    ┌─────────────┐
                    │  CREATED    │ ← 初始状态
                    └──────┬──────┘
                           │ Agent 接收
                           ▼
                    ┌─────────────┐
              ┌─────│  ROUTING    │ ← 路由决策中
              │     └──────┬──────┘
              │            │ 路由完成
              │            ▼
              │     ┌─────────────┐
              │     │ DIAGNOSING  │ ← 诊断执行中
              │     └──────┬──────┘
              │            │
    ┌──────────┴──────────┐│
    │                     ││
    ▼                     ▼│
┌─────────────┐    ┌─────────────┐
│  WAITING    │    │  RESOLVING  │ ← 修复执行中
│  APPROVAL   │    └──────┬──────┘
└─────────────┘           │
                          │ 修复完成
                          ▼
                   ┌─────────────┐
                   │ VERIFYING   │ ← 验证中
                   └──────┬──────┘
                          │ 验证通过
                          ▼
                   ┌─────────────┐
                   │  RESOLVED   │ ← 已解决
                   └─────────────┘

         ┌────────────────────┐
         │                    │
         ▼                    ▼
   ┌─────────────┐     ┌─────────────┐
   │  ESCALATED  │     │   CLOSED    │ ← 最终状态
   └─────────────┘     └─────────────┘
```

### 4.2 状态转换规则

| 当前状态 | 事件 | 下一状态 | 动作 |
|---------|------|---------|------|
| CREATED | Agent 接收工单 | ROUTING | 启动路由决策 |
| ROUTING | 路由成功 | DIAGNOSING | 激活对应 Skill |
| ROUTING | 路由失败 | ESCALATED | 升级人工处理 |
| DIAGNOSING | 根因确认 | RESOLVING | 执行修复操作 |
| DIAGNOSING | 需要审批 | WAITING_APPROVAL | 暂停等待 |
| DIAGNOSING | 诊断超时 | ESCALATED | 升级人工处理 |
| DIAGNOSING | 难度升级 | ESCALATED | 升级人工处理 |
| WAITING_APPROVAL | 审批通过 | RESOLVING | 继续修复 |
| WAITING_APPROVAL | 审批拒绝 | ESCALATED | 升级人工处理 |
| RESOLVING | 修复完成 | VERIFYING | 执行验证 |
| RESOLVING | 修复失败 | ESCALATED | 升级人工处理 |
| VERIFYING | 验证通过 | RESOLVED | 完成处理 |
| VERIFYING | 验证失败 | RESOLVING | 重新修复 |
| RESOLVED | 归档超时 | CLOSED | 结束工单 |
| ESCALATED | 人工处理完成 | CLOSED | 结束工单 |

---

## 5. Skill 路由表

### 5.1 Category → Skill 映射

| Category | Primary Skill | Fallback Skill | 多技能协同场景 |
|----------|--------------|----------------|--------------|
| TC-INFRA-NODE | SKILL-NODE-001 | SKILL-CP-001 (若控制平面节点) | NODE + SEC-CERT (证书相关) |
| TC-INFRA-NET | SKILL-NET-001 (DNS) | SKILL-NET-002 (Service), SKILL-NET-003 (Ingress) | NET-001 → NET-002 (DNS 影响连通性) |
| TC-INFRA-STORE | SKILL-STORE-001 | SKILL-CP-001 (若涉及 CSI) | STORE + NODE (节点压力导致) |
| TC-INFRA-CP | SKILL-CP-001 | SKILL-SEC-001 (证书相关) | CP + SEC-CERT + STORE (etcd 存储) |
| TC-INFRA-SCALE | SKILL-SCALE-001 | SKILL-POD-001 (调度相关) | SCALE + POD (调度失败导致) |
| TC-APP-POD | SKILL-POD-001/002 | SKILL-IMAGE-001 (镜像相关) | POD + IMAGE (镜像拉取导致) |
| TC-APP-WORKLOAD | SKILL-WORK-001 | SKILL-POD-002 (调度) | WORK + POD (Pod 创建失败) |
| TC-APP-SVC | SKILL-NET-002 | SKILL-NET-001 (DNS) | SVC + NET-001 (服务发现依赖 DNS) |
| TC-APP-CONFIG | SKILL-CONFIG-001 | SKILL-POD-001 (Pod 异常) | CONFIG + POD (配置注入失败) |
| TC-APP-INGRESS | SKILL-NET-003 | SKILL-SEC-001 (证书) | INGRESS + SEC-CERT (TLS 问题) |
| TC-SEC-CERT | SKILL-SEC-001 | SKILL-NODE-001 (kubelet 相关) | SEC-CERT + NODE (证书影响节点) |
| TC-SEC-RBAC | SKILL-SEC-002 | SKILL-POD-001 (权限导致 Pod 异常) | RBAC + POD (权限不足) |
| TC-SEC-INCIDENT | SKILL-SECURITY-001 | SKILL-NODE-001 (节点异常) | SECURITY + NODE + SEC-CERT |
| TC-DATA-DB | 无 (待开发) | - | - |
| TC-DATA-CACHE | 无 (待开发) | - | - |
| TC-DATA-MQ | 无 (待开发) | - | - |
| TC-DATA-BACKUP | 无 (待开发) | - | - |

---

## 6. 反馈闭环

### 6.1 语料反馈机制

当 Agent 遇到无法分类的工单时：
1. 记录工单原文到 `feedback-uncategorized.jsonl`
2. 人工分类后，将映射关系加入训练语料
3. 定期重新训练意图识别模型

### 6.2 路由准确率监控

| 指标 | 计算方式 | 目标值 |
|------|---------|--------|
| 路由成功率 | 成功路由数 / 总工单数 | ≥ 95% |
| 路由准确率 | 路由后 Skill 正确 / 成功路由数 | ≥ 90% |
| 首次解决率 | 一次修复解决 / 总工单数 | ≥ 75% |

---

**关联文档**:
- [P0-2: 多技能协同协议设计](./P0-2-multi-skill-coordination-protocol.md)
- [P0-3: 会话上下文管理机制](./P0-3-session-context-management.md)
- [故障诊断/[[存储/README.md|README]].md](../故障诊断/topic-skills/README.md)
- [故障诊断/topic-fta/list/](../故障诊断/topic-fta/list/) — FTA 问题树参考

<!-- risk-assessed -->
