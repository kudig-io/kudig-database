---
title: 生产 Runbook 编写规范与高频操作清单
summary: 生产 Runbook 编写规范与高频操作清单：Runbook（操作手册）是值班工程师在高压环境下的「救命稻草」。一份好的 Runbook 能够显著降低
  MTTR，避免人为失误。本文档提供编写规范和高频操作清单，帮助远程顾问指导客户建立可执行、可维护的操作文档。
category: domain-11
tags:
- domain-11
- runbook
- 操作手册
- SRE
- 运维
- ACK
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
NAMESPACE="default"
POD_NAME="my-app-xxx"
kubectl delete pod "$POD_NAME" -n "$NAMESPACE"
```
### 含验证步骤

每个操作后必须有验证命令和预期结果：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment/my-app -n my-ns
kubectl rollout status deployment/my-app -n my-ns
```
### 节点排空

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
kubectl drain node-01 --ignore-daemonsets --delete-emptydir-data
kubectl get nodes  # 验证 SchedulingDisabled
kubectl uncordon node-01  # 恢复调度
```
### 证书更新

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
kubeadm certs check-expiration
kubeadm certs renew all
systemctl restart kubelet
```
### 配置回滚

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
ack-node-pool scale --cluster-id $CLUSTER_ID \
  --nodepool-id $NP_ID --count 5
kubectl get nodes -l alibabacloud.com/nodepool-id=$NP_ID
```
### 组件升级

- 在 ACK 控制台查看待升级组件列表
- 先在测试集群验证兼容性
- 升级后参考 [[concepts/cluster-upgrade-paths.md|cluster-upgrade-paths]] 确认版本一致

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

- [[domain-11-production-operations/01-production-sre-daily-ops.md|production-sre-daily-ops]] — 日常巡检与值班手册
- [[domain-11-production-operations/03-on-call-playbook.md|on-call-playbook]] — 值班手册与告警响应规范
- [[domain-11-production-operations/04-incident-response-template.md|incident-response-template]] — 事故响应模板
- [[concepts/cluster-upgrade-paths.md|cluster-upgrade-paths]] — 集群升级路径与版本兼容性

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
