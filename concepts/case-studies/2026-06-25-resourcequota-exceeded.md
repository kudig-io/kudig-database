---
title: "[2026-06-25] [P1] ResourceQuota 超限导致新 Pod 无法创建"
category: case-study
tags: [production, incident, cluster-fundamentals, resource-quota, scheduling]
date: "2026-06-25"
severity: P1
mttr: "18min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
---

# [2026-06-25] ResourceQuota CPU 超限导致 CI/CD 部署全部失败

## 工单信息
- **工单编号**: INC-2026-0625-014
- **发现时间**: 2026-06-25 13:10 UTC
- **恢复时间**: 2026-06-25 13:28 UTC
- **影响范围**: `dev-team-a` namespace
- **业务影响**: 开发团队无法部署新功能，紧急 Bug 修复无法上线

## 问题现象
13:10，开发团队在 Slack 报告所有 CI/CD Pipeline 失败：
```
Error from server (Forbidden): error when creating "deployment.yaml": 
  pods "api-7d9f4b8c5a-abc12" is forbidden: exceeded quota: team-a-quota, 
  requested: cpu=2, used: cpu=18, limited: cpu=20
```

所有新 Pod 无法创建，已有 Deployment 的滚动更新也卡住。

## 诊断过程

**13:12** — 检查 ResourceQuota：
```bash
kubectl get resourcequota team-a-quota -n dev-team-a -o yaml
# spec:
#   hard:
#     cpu: "20"
#     memory: 40Gi
#     pods: "50"
# status:
#   used:
#     cpu: "18"
#     memory: 28Gi
#     pods: "42"
```

**13:14** — 检查近期变更：
```bash
# 13:05，开发团队为压测环境部署了 10 个临时 Pod
kubectl get pods -n dev-team-a | grep load-test
# load-test-xxx   1/1   Running   0   10m
# ...（共 10 个）

# 每个 Pod 请求 2 CPU
kubectl get pod load-test-xxx -n dev-team-a -o jsonpath='{.spec.containers[0].resources.requests.cpu}'
# 2
```

**13:16** — 计算：
- 原有 Pod CPU 请求：18
- load-test Pod CPU 请求：10 × 2 = 20
- 总计：18 + 20 = 38 > quota 20

等等，used 显示 18，说明 load-test 的部分 Pod 可能还没被统计，或者有些已失败。实际计算：
- 开发团队部署了 10 个 load-test Pod（每个 request 2 CPU）
- 但 ResourceQuota `cpu: 20` 限制下，只能创建 `(20 - 18) / 2 = 1` 个 Pod
- 因此 1 个成功，其余 9 个被 Quota 拒绝
- 但原有 18 CPU 的使用量中，有些 Deployment 正在滚动更新，需要同时运行新旧 Pod
- 滚动更新时，新 Pod 创建需要额外 CPU，但 Quota 已满，导致更新卡住

## 根因
开发团队在 13:05 为压测部署了 10 个临时 Pod，每个请求 2 CPU。`dev-team-a` namespace 的 ResourceQuota CPU 限制为 20，原有工作负载已使用 18 CPU。临时 Pod 占用了剩余配额，导致：
1. 临时 Pod 中 9 个创建失败
2. 原有 Deployment 的滚动更新因无法创建新 Pod 而卡住
3. 紧急 Bug 修复部署失败

## 修复动作

**13:18** — 删除临时 Pod：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
kubectl delete pods -n dev-team-a -l app=load-test
kubectl get resourcequota team-a-quota -n dev-team-a
# NAME          AGE
# team-a-quota  cpu: 18/20, memory: 28Gi/40Gi, pods: 32/50
```

**13:20** — 触发卡住的 Deployment 继续滚动更新：
```bash
kubectl rollout resume deployment api -n dev-team-a
kubectl get deployment api -n dev-team-a
# NAME   READY   UP-TO-DATE   AVAILABLE
# api    5/5     5            5
```

**13:22** — 部署紧急修复：
```bash
# CI/CD Pipeline 恢复，新部署成功
kubectl get pods -n dev-team-a -l app=api
# NAME                    READY   STATUS
# api-7d9f4b8c5a-new12   1/1     Running
```

**13:25** — 调整临时环境的 ResourceQuota：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch resourcequota team-a-quota -n dev-team-a --type='merge' -p '
{
  "spec": {
    "hard": {
      "cpu": "40",
      "memory": "80Gi",
      "pods": "100"
    }
  }
}'
```

## 验证
- 13:26 — CI/CD Pipeline 全部恢复
- 13:28 — 紧急 Bug 修复部署成功，业务验证通过

## 复盘
- **直接原因**: 临时压测 Pod 占用 ResourceQuota → 配额耗尽 → 新 Pod 无法创建 → 滚动更新卡住 → 部署失败
- **根本原因**: 开发团队未评估临时 Pod 对 ResourceQuota 的影响，缺少临时环境隔离
- **改进措施**:
  1. 临时压测使用独立的 `dev-team-a-loadtest` namespace，不共享生产 ResourceQuota
  2. CI/CD 部署前自动检查 ResourceQuota：`kubectl get resourcequota -n $NAMESPACE -o json | jq '.items[].status.used.cpu'`
  3. 添加告警：`resourcequota_used_cpu / resourcequota_hard_cpu > 0.85`
  4. 为开发 namespace 设置 LimitRange，防止单个 Pod 请求过大资源
- **相关 Skill**: [[ts-resources-scheduling]]
- **相关 FTA**: [[resource-quota-fta]]
