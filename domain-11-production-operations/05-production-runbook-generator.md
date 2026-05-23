---
title: "生产 Runbook 编写规范与高频操作清单"
category: domain-11
tags: ["domain-11", "runbook", "操作手册", "SRE", "运维", "ACK", "visibility/public"]
sources: ["KUDIG Gap Analysis 2026-05-21"]
created: 2026-05-21
updated: 2026-05-21
status: reviewed
---

# 生产 Runbook 编写规范与高频操作清单

## 概述

Runbook（操作手册）是值班工程师在高压环境下的「救命稻草」。一份好的 Runbook 能够显著降低 MTTR，避免人为失误。本文档提供编写规范和高频操作清单，帮助远程顾问指导客户建立可执行、可维护的操作文档。

## Runbook 编写规范

### 一页一操作

每个 Runbook 只描述一个场景（如「重启 Pod」、「排空节点」），避免混排多个不相关操作。

### 命令可复制

所有命令必须可直接复制执行：

- ❌ `kubectl delete pod <pod-name>`
- ✅ `kubectl delete pod my-pod -n my-ns`

变量使用显式占位符：

```bash
NAMESPACE="default"
POD_NAME="my-app-xxx"
kubectl delete pod "$POD_NAME" -n "$NAMESPACE"
```

### 含验证步骤

每个操作后必须有验证命令和预期结果：

```bash
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='{.status.phase}'
# 预期输出：Running
```

### 前置条件与风险提示

| 要素 | 说明 |
|---|---|
| 前置条件 | 执行前需确认的状态 |
| 影响范围 | 会影响哪些服务或用户 |
| 回滚步骤 | 操作失败如何恢复 |
| 审批要求 | 是否需双人复核 |

## 高频操作清单

### Pod 重启

```bash
kubectl rollout restart deployment/my-app -n my-ns
kubectl rollout status deployment/my-app -n my-ns
```

### 节点排空

```bash
kubectl drain node-01 --ignore-daemonsets --delete-emptydir-data
kubectl get nodes  # 验证 SchedulingDisabled
kubectl uncordon node-01  # 恢复调度
```

### 证书更新

```bash
kubeadm certs check-expiration
kubeadm certs renew all
systemctl restart kubelet
```

### 配置回滚

```bash
kubectl rollout history deployment/my-app -n my-ns
kubectl rollout undo deployment/my-app -n my-ns
# 或回滚到指定版本
kubectl rollout undo deployment/my-app --to-revision=3 -n my-ns
```

## Runbook 维护机制

### 定期演练

- 每季度至少演练一次关键 Runbook
- 记录实际耗时与文档预期的差异
- 演练后更新命令和参数

### 版本控制

- Runbook 纳入 Git 管理，变更需 PR 审核
- 发布版本号，值班手册引用固定版本

### 过期检查

- 每月检查命令是否因集群升级失效
- 标记超过 6 个月未更新的为「待审阅」
- 删除已废弃组件的相关 Runbook

## 阿里云 ACK 常用操作

### 节点池扩容

```bash
ack-node-pool scale --cluster-id $CLUSTER_ID \
  --nodepool-id $NP_ID --count 5
kubectl get nodes -l alibabacloud.com/nodepool-id=$NP_ID
```

### 组件升级

- 在 ACK 控制台查看待升级组件列表
- 先在测试集群验证兼容性
- 升级后参考 [[cluster-upgrade-paths]] 确认版本一致

### 集群备份

- etcd 备份：ACK 托管版自动备份，专有版需配置定时备份
- 应用配置备份：使用 Velero 定期备份命名空间
- 每季度执行一次恢复演练

## 远程顾问指导要点

远程顾问审核客户 Runbook 时，重点关注以下维度：

1. **可执行性**：要求工程师按 Runbook 实操一次，观察是否有歧义或遗漏
2. **完整性**：是否每个操作都有前置条件、执行步骤、验证步骤、回滚步骤
3. **时效性**：命令是否与当前集群版本匹配，参数是否过时
4. **权限合理性**：操作权限是否最小化，是否存在过度授权
5. **命名规范**：文件名是否按「系统_场景_操作」格式统一命名

> 建议为客户建立 Runbook 评分卡，从 5 个维度打分，持续优化。

## 相关链接

- [[production-sre-daily-ops]] — 日常巡检与值班手册
- [[on-call-playbook]] — 值班手册与告警响应规范
- [[incident-response-template]] — 事故响应模板
- [[cluster-upgrade-paths]] — 集群升级路径与版本兼容性
