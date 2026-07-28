---
title: 故障排查实验考核
description: '# 故障排查实验考核'
summary: '用户报告新创建的 Pod 一直处于 `Pending` 状态，无法调度到任何节点。'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- etcd
- kubelet
- ingress
- gateway
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 故障排查实验考核 是什么
- 如何 故障排查实验考核
- 故障排查实验考核 故障排查
- 故障排查实验考核 排障步骤
trigger_keywords:
- 故障排查实验考核
- skills
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
skill_id: SKILL-TROUBLESHOOTING_LAB_EXAM-001
skill_name: 故障排查实验考核
version: 1.0.0
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 故障排查实验考核

> **适用对象**: SRE/Ops 工程师能力考核 | **版本**: [[23-实体/kubernetes.md|[[17-系统基础/05-速查卡/k8s|k8s]]]] 1.28-1.33 | **时间**: 90 分钟

---

## 考核说明

- 每个场景满分 100 分，总分 200 分
- 场景随机抽取，考核过程中可查阅文档
- 需要完整记录排查步骤、命令、修复和验证过程
- 最终需要能复现问题修复

---

## 场景 A：Pod 无法调度（100 分）

### 问题描述

用户报告新创建的 Pod 一直处于 `Pending` 状态，无法调度到任何节点。

### 已知信息

- 命名空间：`production`
- Pod 名称：`payment-api-7d9f8b5c6-x2kqm`
- 镜像：`payment-[[Service|service]]:v1.2`
- 资源请求：`cpu: 2, memory: 4Gi`

### 考核要求

1. **诊断**（40 分）：使用 kubectl 命令收集信息，确定阻塞调度的原因
2. **修复**（40 分）：实施修复措施，使 Pod 能够成功调度
3. **验证**（20 分）：确认 Pod 进入 Running 状态

### 评估标准

- 诊断命令正确（20 分）
- 根因分析准确（20 分）
- 修复方案有效（30 分）
- 验证流程完整（10 分）

---

## 场景 B：Service 无 Endpoints（100 分）

### 问题描述

前端应用无法访问后端 API，所有请求返回 503。运维检查发现后端 Service 的 Endpoints 列表为空。

### 已知信息

- 命名空间：`default`
- Frontend Deployment：`frontend-app`（已 Running）
- Backend Service：`backend-api`（存在但无 Endpoints）
- Backend Deployment：`backend-app`（Pod 处于 Running 状态）

### 考核要求

1. **诊断**（40 分）：分析为什么 Pod Running 但 Service 无 Endpoints
2. **修复**（40 分）：修复 Endpoints 问题
3. **验证**（20 分）：确认前端可以正常访问后端

### 评估标准

- 诊断逻辑清晰（15 分）
- 能识别 selector 不匹配问题（15 分）
- 修复方案有效（25 分）
- 验证流程完整（15 分）
- 能写出关键 YAML（30 分）

---

## 场景 C：etcd 空间告警（100 分）

### 问题描述

收到告警："etcd database space usage exceeds 80% of quota"。API Server 写入开始变慢。

### 已知信息

- 集群：单控制平面节点
- etcd 数据目录：`/var/lib/etcd`
- 当前配额：8GB
- 当前使用：6.8GB

### 考核要求

1. **诊断**（30 分）：确认 etcd 空间使用情况，区分 logical vs physical size
2. **修复**（50 分）：执行 etcd 维护操作（compact + defrag）
3. **验证**（20 分）：确认空间释放，API Server 恢复正常

### 评估标准

- 会使用 etcdctl 命令（15 分）
- 理解 compact + defrag 流程（20 分）
- 操作步骤正确（30 分）
- 验证方法完整（15 分）

---

## 场景 D：节点 NotReady（100 分）

### 问题描述

监控告警节点 `node-2` 变为 `NotReady`，该节点上运行了 5 个业务 Pod。

### 已知信息

- 节点：`node-2`
- 节点上 Pod 数量：5 个
- 问题持续时间：10 分钟
- 业务影响：部分请求失败

### 考核要求

1. **诊断**（30 分）：排查 kubelet 日志，确定 NotReady 原因
2. **决策**（30 分）：决定是否需要驱逐 Pod，如何处理
3. **执行**（30 分）：执行节点维护或恢复操作
4. **验证**（10 分）：确认节点恢复，Pod 正常运行

### 评估标准

- 诊断方法正确（20 分）
- 决策合理（20 分）
- 操作规范（30 分）
- 验证完整（10 分）

---

## 场景 E：Ingress 返回 502（100 分）

### 问题描述

外部用户访问 `api.example.com` 返回 502 Bad Gateway。Ingress 已配置，Service 存在。

### 已知信息

- Ingress：`api-ingress`
- Backend Service：`api-backend-svc`，Port 8080
- Ingress Controller：`nginx-ingress`，状态 Running
- DNS 已正确解析到 Ingress Controller IP

### 考核要求

1. **诊断**（40 分）：确定 502 的根因
2. **修复**（40 分）：修复 Ingress 配置
3. **验证**（20 分）：确认外部可正常访问

### 评估标准

- 知道检查 Ingress 配置（15 分）
- 知道检查 Endpoints（15 分）
- 能识别 backend 配置问题（10 分）
- 修复方案有效（20 分）
- 验证方法正确（10 分）

---

## 考核记录模板

```markdown
## 故障排查实验记录

**考生姓名**：
**考核日期**：
**场景编号**：

### 问题概述
[描述问题现象]

### 诊断过程

#### 步骤 1：[命令]
[输出结果]
[分析]

#### 步骤 2：[命令]
[输出结果]
[分析]

### 根因分析
[总结根因]

### 修复措施

```bash
# 修复命令
[命令1]
[命令2]
```

### 验证

```bash
# 验证命令
[验证命令]
[结果]
```

### 耗时
[开始时间] - [结束时间] = [总耗时]

### 评估
| 项目 | 得分 |
|------|------|
| 诊断 | /40 |
| 修复 | /40 |
| 验证 | /20 |
| 总分 | /100 |
```

---

```yaml
---
id: ASSESSMENT-LAB-001
topic: assessment
type: lab-exam
tags: [assessment, lab-exam, troubleshooting, sre, ops-engineer, k8s-1.28-1.33]
intent_queries:
  - "故障排查实验考核"
  - "场景模拟考试"
difficulty: advanced
target_roles: [sre, ops-engineer]
related:
  - 故障诊断/topic-skills/assessment/k8s-fundamentals-quiz.md
  - 故障诊断/topic-skills/assessment/daily-check-quiz.md
---
```

<!-- risk-assessed -->
