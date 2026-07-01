---
title: 'Day 9: K8s 审计日志配置与分析实操'
description: '# Day 9: K8s 审计日志配置与分析实操'
summary: '# Day 9: K8s 审计日志配置与分析实操'
category: learning
tags:
- k8s
- training
- hands-on
- apiserver
- prometheus
- statefulset
- daemonset
- rbac
- crd
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 9: K8s 审计日志配置与分析实操 是什么'
- '如何 Day 9: K8s 审计日志配置与分析实操'
trigger_keywords:
- Day
- '9:'
- K8s
- 审计日志配置与分析实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- logging-basics
---



# Day 9: K8s 审计日志配置与分析实操

> **日期**: Week 2 Day 2 | **主题**: 审计日志配置与深度分析 | **版本**: K8s 1.28-1.33

---

## 1. 审计策略概述

### 1.1 审计生命周期

```
请求 → API Server → 认证 → 授权 → 准入控制 → 审计日志记录 → 处理请求
```

### 1.2 审计阶段 (Stage)

| Stage | 时机 | 用途 |
|-------|------|------|
| `RequestReceived` | 收到请求时 | 记录原始请求 |
| `ResponseStarted` | 响应开始发送时 | 记录长时间运行请求的初始响应 |
| `ResponseComplete` | 响应发送完成 | 记录最终响应状态 |
| `Panic` | 服务器 panic 时 | 记录紧急状态（不常用） |

### 1.3 审计级别 (Level)

| Level | 说明 | 适用场景 |
|-------|------|---------|
| `None` | 不记录 | 排除噪音 |
| `Metadata` | 仅记录元数据（user, timestamp, resource） | 大多数操作 |
| `RequestResponse` | 记录元数据 + 请求体 + 响应体 | 变更操作 |
| `Request` | 仅记录请求体（不含响应） | 特殊情况 |

---

## 2. 配置审计策略

### 2.1 创建审计策略文件

```bash
cat > audit-policy.yaml <<'EOF'
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  # 不记录只读请求（get/list/watch）
  - level: Metadata
    verbs: ["get", "list", "watch"]
    resources:
      - group: ""
        resources: ["pods", "services", "configmaps"]

  # 记录所有变更操作
  - level: RequestResponse
    resources:
      - group: "apps"
        resources: ["deployments", "statefulsets", "daemonsets"]
      - group: ""
        resources: ["pods", "services", "configmaps", "secrets"]
      - group: "rbac.authorization.k8s.io"
        resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]

  # 不记录 /healthz 和 /version
  - level: None
    nonResourceURLs:
      - "/healthz"
      - "/version"
      - "/api"

  # 高危操作记录完整信息
  - level: RequestResponse
    userGroups: ["system:masters"]
    resources:
      - group: ""
        resources: ["secrets", "configmaps"]
    verbs: ["create", "update", "patch", "delete"]

  # 记录所有 namespaced 资源变更
  - level: Metadata
    resources:
      - group: "*"
        resources: ["*"]
    namespaces: ["kube-system"]
EOF
```

### 2.2 在 API Server 中启用审计

```bash
# 方式 1: 通过 kube-apiserver 静态配置
# 编辑 /etc/kubernetes/manifests/kube-apiserver.yaml
--audit-policy-file=/etc/kubernetes/audit-policy.yaml
--audit-log-path=/var/log/kubernetes/audit.log
--audit-log-format=json
--audit-log-maxage=30
--audit-log-maxbackup=10
--audit-log-maxsize=100
--audit-log-truncate-enabled=true

# 方式 2: 通过 Dynamic Audit Configuration（K8s 1.20+）
# 创建 AuditSink CRD（需要启用 FeatureGate: DynamicAuditing）
```

### 2.3 验证审计配置

```bash
# 检查 API Server 是否加载审计策略
kubectl get pods -n kube-system -l component=kube-apiserver -o yaml | grep audit

# 检查审计日志是否生成
ls -la /var/log/kubernetes/audit.log
```

---

## 3. 审计日志分析实操

### 3.1 查看审计日志格式

```json
{
  "kind": "Event",
  "apiVersion": "audit.k8s.io/v1",
  "level": "RequestResponse",
  "auditID": "abc-123-def",
  "stage": "ResponseComplete",
  "requestURI": "/api/v1/namespaces/default/pods",
  "verb": "create",
  "user": {
    "username": "alice@example.com",
    "uid": "ldap-12345",
    "groups": ["engineering", "frontend-team"]
  },
  "sourceIPs": ["10.0.0.5"],
  "userAgent": "kubectl/v1.30.0",
  "objectRef": {
    "resource": "pods",
    "namespace": "default",
    "name": "nginx-pod",
    "apiVersion": "v1"
  },
  "responseStatus": {
    "code": 201
  },
  "requestTimestamp": "2026-05-18T10:00:00.000Z",
  "responseTimestamp": "2026-05-18T10:00:01.000Z",
  "requestReceivedTimestamp": "2026-05-18T10:00:00.000Z"
}
```

### 3.2 常用分析查询

**查找高危操作（Secret 修改/删除）**

```bash
# 查找 Secret 变更操作
cat /var/log/kubernetes/audit.log | jq -r 'select(
  .objectRef.resource == "secrets" and
  (.verb == "create" or .verb == "update" or .verb == "delete")
) | "\(.requestTimestamp) \(.user.username) \(.objectRef.namespace) \(.objectRef.name) \(.verb)"'

# 查找所有删除操作
jq -r 'select(.verb == "delete") | "\(.requestTimestamp) \(.user.username) \(.objectRef.resource) \(.objectRef.namespace)/\(.objectRef.name)"' /var/log/kubernetes/audit.log
```

**查找异常登录**

```bash
# 查找非白名单 IP 的管理操作
jq -r 'select(
  .sourceIPs[] | contains("10.0.0.0/8") | not
) and .responseStatus.code >= 200 and .responseStatus.code < 300
| "\(.requestTimestamp) \(.user.username) \(.sourceIPs[0]) \(.requestURI)"' /var/log/kubernetes/audit.log
```

**查找权限变更**

```bash
# 查找 RBAC 相关变更
jq -r 'select(.objectRef.resource | startswith("role") or startswith("clusterrole")) |
  "\(.requestTimestamp) \(.user.username) \(.verb) \(.objectRef.resource) \(.objectRef.name)"' /var/log/kubernetes/audit.log
```

### 3.3 使用工具分析（audit2rbac）

```bash
# 安装 audit2rbac
go install github.com/liggitt/audit2rbac@latest

# 从审计日志生成最小权限 RBAC
audit2rbac -f /var/log/kubernetes/audit.log --user alice@example.com > alice-rbac.yaml

# 审查生成的 RBAC 配置
cat alice-rbac.yaml
```

---

## 4. 审计日志告警配置

### 4.1 高危操作告警规则

```yaml
# 使用 [[fluentd|Fluentd]]/Prometheus 规则
groups:
  - name: audit-alerts
    rules:
      # 大量 Secret 访问
      - alert: AuditSecretAccessSpike
        expr: |
          rate(audit_event_count{resource="secrets",verb=~"get|list"}[5m]) > 50
        for: 2m
        labels:
          severity: warning
        annotations:
          description: "大量 Secret 访问超过阈值"

      # 高危用户组操作
      - alert: AuditPrivilegedUserAction
        expr: |
          audit_event_count{userGroups=~"system:masters",verb=~"delete"}[5m] > 0
        for: 1m
        labels:
          severity: critical
        annotations:
          description: "管理员组执行了删除操作"
```

---

## 5. 审计日志保留与合规

| 要求 | 配置 |
|------|------|
| 保留周期 | 90 天（生产环境建议 1 年） |
| 存储 | 对象存储（OSS/S3）+ 日志服务 |
| 加密 | 静态加密（服务端加密） |
| 访问控制 | 仅安全团队可读，运维团队仅告警 |

---

## 6. 审计故障排查

| 问题 | 排查 |
|------|------|
| 审计日志无输出 | 检查 API Server 是否指定 `--audit-policy-file`，检查 `/var/log/kubernetes/` 目录权限 |
| 日志文件过大 | 配置日志轮转（`--audit-log-maxsize`），启用 gzip 压缩 |
| 审计日志丢失 | 检查 API Server Pod 的 `volumemounts` 是否正确映射 |

---

## 7. 实战练习

**练习 1**: 配置审计策略，记录所有 `secrets` 的变更操作（RequestResponse 级别）

**练习 2**: 从审计日志提取 `alice@example.com` 最近 24 小时的所有操作记录

**练习 3**: 配置告警规则，检测 5 分钟内超过 10 次 Secret 读取操作

**练习 4**: 使用 `audit2rbac` 从审计日志生成最小权限配置，并与当前 Role 对比差异

---

```yaml
---
id: LEARN-WEEK2-DAY9
title: Day 9 - K8s 审计日志配置与分析实操
topic: security-monitoring
type: hands-on-guide
tags: [audit, logging, security, policy, compliance, hands-on, k8s-1.28-1.33]
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - "K8s 审计日志怎么配置"
  - "审计策略怎么写"
  - "审计日志分析方法"
  - "audit2rbac 怎么用"
  - "审计告警规则怎么配"
trigger_keywords:
  - Audit
  - 审计日志
  - 审计策略
  - Policy
  - RequestReceived
  - ResponseComplete
  - Metadata
  - RequestResponse
  - audit2rbac
  - jq
  - 合规
  - 审计告警
reading_level: intermediate
audience:
  - sre
  - ops-engineer
  - security-engineer
estimated_read_time: 45min
related_domains:
  - domain-05-security-compliance
  - domain-10-troubleshooting-diagnostics
related_topics:
  - security
  - audit
  - logging
  - compliance
related:
  - domain-11-production-operations/topic-learn/public-training/week-2-security-monitoring/day-08-rbac/01-rbac-hands-on.md
  - domain-10-troubleshooting-diagnostics/12-audit-log-analysis.md
---
```