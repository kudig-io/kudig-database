# Day 12: K8S 集群审计

> **学习时间**: 4-5 小时 | **主题**: 审计日志配置与分析方法

---

## 今日目标

- [ ] 理解 K8S 审计日志的作用和工作机制
- [ ] 掌握 ACK 审计日志与 SLS 的集成配置
- [ ] 能够查询和分析审计日志
- [ ] 了解审计策略配置

---

## 理论学习 (2h)

### 必读文档

1. **可观测性架构**
   - 文件: `../../../domain-8-observability/01-observability-architecture-overview.md`
   - 重点: 日志在可观测性中的角色

2. **日志架构**
   - 文件: `../../../domain-8-observability/03-logging-architecture.md`
   - 重点: 审计日志采集与存储架构

---

## 实践任务 (2.5h)

### 任务 1: 审计日志配置 (45min)

```bash
# ACK 审计日志默认投递到 SLS (日志服务)
# 控制台 -> ACK -> 集群详情 -> 集群审计

# 查看审计日志 Project
# 命名格式: k8s-log-<cluster_id>
# Logstore: apiserver-audit-log

# 通过 API 查看审计配置
aliyun cs GET /clusters/<cluster_id> | jq '.meta_data' | jq -r . | jq '.AuditProjectName'

# 审计日志级别:
# None: 不记录
# Metadata: 记录请求元数据 (用户、时间、资源、操作)
# Request: 记录元数据 + 请求体
# RequestResponse: 记录元数据 + 请求体 + 响应体
```

### 任务 2: 审计日志查询 (45min)

```bash
# 在 SLS 控制台查询审计日志

# 常用查询语句:

# 1. 查看所有删除操作
# verb: delete

# 2. 查看特定用户的操作
# user.username: "ram-user@<account_id>"

# 3. 查看 Secret 相关操作
# objectRef.resource: secrets

# 4. 查看失败的请求
# responseStatus.code >= 400

# 5. 查看 RBAC 相关操作
# objectRef.resource: roles OR objectRef.resource: clusterroles

# 使用 kubectl 查看最近的事件作为补充
kubectl get events -A --sort-by='.lastTimestamp' | tail -20
```

### 任务 3: 审计告警配置 (30min)

```
# 在 SLS 中配置审计告警

# 告警场景 1: 高权限操作
# 查询: verb: create AND objectRef.resource: clusterrolebindings
# 告警: 有人创建了 ClusterRoleBinding

# 告警场景 2: 异常删除操作
# 查询: verb: delete AND objectRef.resource: deployments
# 告警: 有 Deployment 被删除

# 告警场景 3: 密钥访问
# 查询: verb: get AND objectRef.resource: secrets AND objectRef.namespace != kube-system
# 告警: 非系统 Namespace 的 Secret 被访问

# 告警场景 4: 权限提升尝试
# 查询: responseStatus.code: 403
# 告警: 有未授权的操作尝试
```

### 任务 4: 审计日志分析实践 (30min)

```bash
# 场景: 排查"谁删除了某个 Deployment"

# 1. 在 SLS 中查询:
# verb: delete AND objectRef.resource: deployments AND objectRef.name: <deployment-name>

# 2. 关注以下字段:
# - user.username: 操作者
# - sourceIPs: 来源 IP
# - requestReceivedTimestamp: 操作时间
# - objectRef.namespace: Namespace
# - responseStatus.code: 是否成功

# 3. 形成审计报告:
# - 操作者: xxx
# - 操作时间: xxx
# - 操作内容: 删除了 namespace/deployment
# - 来源 IP: xxx
# - 操作结果: 成功/失败
```

---

## 费曼复述 (0.5h)

1. **K8S 审计日志记录了什么信息？有什么价值？**
2. **ACK 审计日志如何与 SLS 集成？**
3. **如何通过审计日志追溯"谁做了什么操作"？**

---

## 今日检验

- [ ] 理解审计日志的四个级别
- [ ] 能在 SLS 中查询审计日志
- [ ] 能配置审计告警规则
- [ ] 能通过审计日志追溯操作记录

---

## 核心概念总结

| 审计级别 | 记录内容 | 存储开销 |
|----------|---------|---------|
| None | 不记录 | 无 |
| Metadata | 用户、时间、资源、操作 | 低 |
| Request | 元数据 + 请求体 | 中 |
| RequestResponse | 元数据 + 请求体 + 响应体 | 高 |

---

## 明日预告

Day 13 将学习集群监控体系搭建与告警配置。
