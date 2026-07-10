---
title: 变更回滚操作手册
description: 面向阿里云/专有云 K8s 的变更回滚操作手册，涵盖 Deployment、StatefulSet、ConfigMap、CRD、数据库变更的回滚步骤与验证。
summary: 面向阿里云/专有云 K8s 的变更回滚操作手册，涵盖 Deployment、StatefulSet、ConfigMap、CRD、数据库变更的回滚步骤与验证。
category: release-management
tags:
- k8s
- rollback
- change-management
- deployment
- statefulset
- database
- alicloud
- apsara-stack
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 值班长
estimated_read_time: 20min
intent_queries:
- K8s 变更回滚手册
- Deployment StatefulSet 回滚
- 配置变更数据库回滚
trigger_keywords:
- 回滚
- rollback
- 变更失败
- undo
- 恢复
prerequisites:
- kubectl-basics
- gitops-basics
- statefulset-basics
- database-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 变更回滚操作手册

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，提供常见变更类型的标准化回滚操作与验证方法。

## 目录

1. [回滚原则](#回滚原则)
2. [Deployment 回滚](#deployment-回滚)
3. [StatefulSet 回滚](#statefulset-回滚)
4. [ConfigMap/Secret 回滚](#configmapsecret-回滚)
5. [CRD/Operator 回滚](#crdoperator-回滚)
6. [数据库变更回滚](#数据库变更回滚)
7. [回滚后验证](#回滚后验证)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 回滚原则

### 1.1 何时回滚

| 场景 | 建议动作 |
|:---|:---|
| 新版本错误率持续上升 | 立即回滚 |
| 核心功能不可用 | 立即回滚 |
| 性能严重下降 | 立即回滚 |
| 监控告警无法解释 | 先观察，必要时回滚 |
| 仅个别非关键 Pod 异常 | 先排查，不立即回滚 |

### 1.2 回滚决策流程

```
发现异常
    │
    ▼
评估影响范围与严重程度
    │
    ├─ 影响小 → 继续观察/热修复
    │
    └─ 影响大 → 立即回滚
              │
              ▼
        执行回滚操作
              │
              ▼
        验证业务恢复
              │
              ▼
        记录事件并复盘
```

---

## 2. Deployment 回滚

### 2.1 查看修订历史

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Deployment 的 rollout 历史
kubectl rollout history deployment/order-service -n production
```
### 2.2 回滚到上一个版本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 回滚到上一个稳定版本
kubectl rollout undo deployment/order-service -n production
```
### 2.3 回滚到指定版本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 回滚到指定 revision
kubectl rollout undo deployment/order-service -n production --to-revision=3
```
### 2.4 验证回滚

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看回滚进度
kubectl rollout status deployment/order-service -n production

# 确认镜像版本
kubectl get deployment order-service -n production -o jsonpath='{.spec.template.spec.containers[0].image}'
```
---

## 3. StatefulSet 回滚

### 3.1 StatefulSet 回滚限制

StatefulSet 默认不保留所有 revision，需先开启 `revisionHistoryLimit`。

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
spec:
  revisionHistoryLimit: 10
```

### 3.2 回滚 StatefulSet

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看历史
kubectl rollout history statefulset/mysql -n production

# 回滚
kubectl rollout undo statefulset/mysql -n production --to-revision=2

# 查看状态
kubectl rollout status statefulset/mysql -n production
```
---

## 4. ConfigMap/Secret 回滚

### 4.1 使用版本控制回滚

ConfigMap 和 Secret 不直接支持 `rollout undo`，需通过 Git 或备份恢复。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 从 Git 恢复上一个版本
kubectl apply -f configmap/order-service-config-v1.yaml

# 触发 Deployment 滚动更新以生效
kubectl rollout restart deployment/order-service -n production
```
### 4.2 使用 immutable ConfigMap

生产建议将 ConfigMap 标记为 immutable，变更时创建新版本：

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: order-service-config-v2
  namespace: production
immutable: true
data:
  DB_HOST: mysql-primary.production.svc.cluster.local
```

---

## 5. CRD/Operator 回滚

### 5.1 备份当前 CRD

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 变更前备份 CRD
kubectl get crd myresources.example.com -o yaml > /backup/myresources.crd.yaml
```
### 5.2 回滚 Operator

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 卸载新版本 Operator
kubectl delete -f operator-v2.yaml

# 2. 安装旧版本 Operator
kubectl apply -f operator-v1.yaml

# 3. 恢复 CRD（如需要）
kubectl apply -f /backup/myresources.crd.yaml
```
### 5.3 回滚 CR 实例

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 从 Git 恢复 CR 配置
kubectl apply -f cr/myresource-production-v1.yaml
```
---

## 6. 数据库变更回滚

### 6.1 无破坏性变更回滚

对于新增表、新增索引等：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 直接回滚应用版本即可
kubectl rollout undo deployment/order-service -n production
```
### 6.2 破坏性变更回滚

对于删除列、修改类型等：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 从备份恢复数据库
mysql -uroot -p < /backup/order-service-20260629.sql

# 2. 回滚应用版本
kubectl rollout undo deployment/order-service -n production
```
### 6.3 使用 Flyway/Liquibase

```bash
# 回滚到上一个 migration 版本
flyway undo -url=jdbc:mysql://mysql-primary.production.svc.cluster.local:3306/order_service -user=root -password=$PWD
```

---

## 7. 回滚后验证

### 7.1 验证清单

| 检查项 | 命令/方法 |
|:---|:---|
| Pod 状态 | `kubectl get pods -n production` |
| 服务入口健康 | `curl https://api.example.com/health` |
| 错误率恢复 | Prometheus 仪表盘 |
| 业务指标恢复 | 业务监控 |
| 日志无异常 | `kubectl logs -l app=order-service -n production --tail=100` |

### 7.2 回滚报告模板

```markdown
# 变更回滚报告

- **变更编号**: RFC-2026-0701-001
- **回滚时间**: 2026-07-01 22:45
- **回滚人**: 张三
- **回滚原因**: 新版本 P95 延迟超过 1s
- **回滚操作**: kubectl rollout undo deployment/order-service -n production
- **验证结果**: 业务恢复，错误率 < 0.1%
- **后续措施**: 排查延迟根因，修复后重新发布
```

---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| 变更前备份 | 关键配置/数据已备份 | 备份记录 |
| 回滚方案 | 已写入 RFC | RFC 文档 |
| 一键回滚 | 命令已验证 | 演练记录 |
| 回滚后验证 | 业务与监控均正常 | 验证清单 |
| 事件记录 | 回滚报告已提交 | 变更系统 |
| 复盘会议 | 24 小时内完成 | 会议记录 |

---

## 回滚后业务验证

回滚不是终点，必须验证业务已恢复，并确保相关指标回归基线。

| 验证项 | 命令 / 检查点 | 通过标准 |
|:---|:---|:---|
| 工作负载版本 | `kubectl describe deployment` | 镜像为回滚版本 |
| Pod 健康 | `kubectl get pods` | Running 且 Ready |
| 服务连通性 | 拨测 / curl | 返回 200 |
| 错误率 | Prometheus | 低于 SLO 阈值 |
| 业务指标 | 业务监控 | 恢复正常 |
| 告警状态 | 告警平台 | 相关告警消除 |

### 变更复盘模板

```markdown
# 变更复盘：{{rfc_id}}

## 变更信息
- 变更内容：{{change_content}}
- 实际窗口：{{actual_window}}
- 是否回滚：是 / 否
- 回滚原因：{{rollback_reason}}

## 时间线
{{timeline}}

## 根本原因
{{root_cause}}

## 改进措施
- {{action_1}}
- {{action_2}}
```

## 回滚权限与审计

所有回滚操作应通过工单或变更系统记录，涉及写操作需保留命令与执行人信息，便于审计与复盘。

## 回滚决策与权限

不是所有变更失败都需要回滚。以下情况建议优先回滚：

1. 错误率超过 SLO 阈值且持续上升。
2. 核心业务流程中断或严重降级。
3. 无法在短时间内定位根因。
4. 存在数据损坏或安全风险。

### 回滚权限矩阵

| 变更类型 | 回滚执行人 | 通知对象 |
|:---|:---|:---|
| 应用 Deployment | 值班 SRE | 应用负责人 |
| 配置 / ConfigMap | 值班 SRE | 配置管理员 |
| 控制平面组件 | 高级 SRE | 平台负责人 |
| 网络 / 存储组件 | 平台工程师 | 架构师、SRE 负责人 |

## 典型工单场景与处理

**场景**：升级后部分用户无法登录，报错 500。

处理步骤：
1. 确认错误率是否超过阈值，影响范围多大。
2. 如无法快速定位，执行 Deployment 回滚。
3. 验证回滚后登录功能恢复正常。
4. 在测试环境复现问题并修复后重新上线。

## 回滚后业务验证清单

回滚完成后，必须按以下清单验证业务恢复：

| 验证项 | 检查命令 / 方法 | 通过标准 |
|:---|:---|:---|
| 版本回退 | `kubectl get deploy -o yaml | grep image` | 镜像为旧版本 |
| Pod 状态 | `kubectl get pods` | Running 且 Ready |
| 服务可用性 | 拨测 / curl | HTTP 200 |
| 错误率 | Prometheus | 低于 SLO |
| 业务指标 | 业务监控 | 恢复至基线 |
| 告警状态 | 告警平台 | 相关告警关闭 |

### 回滚后复盘模板

```markdown
# 回滚复盘
- 变更编号：
- 回滚原因：
- 回滚耗时：
- 业务影响：
- 根本原因：
- 改进措施：
```

### 常见回滚问题

| 问题 | 原因 | 处理 |
|:---|:---|:---|
| 回滚后仍异常 | 数据已被破坏 | 从备份恢复 |
| StatefulSet 无法回滚 | 无历史版本 | 手动修改镜像 |
| Helm 回滚失败 | release 历史被清理 | 重新部署旧 chart |

## 回滚与变更审计

所有回滚操作应记录到变更系统，包含以下信息：

- 变更编号
- 回滚触发原因
- 回滚执行人与时间
- 回滚前后版本
- 业务验证结果
- 后续改进措施

### 典型回滚案例

**场景**：升级 ingress-nginx-controller 后，部分路由返回 502。

处理过程：
1. 查看 ingress-nginx Pod 日志，发现与后端连接超时。
2. 确认新版本默认超时配置变更导致兼容问题。
3. 执行 `helm rollback ingress-nginx` 回滚到旧版本。
4. 验证路由恢复正常。
5. 在新版本测试中修复超时配置后重新发布。

## Related

- [[发布变更/变更管理/22-change-management-process.md|变更管理流程]]
- [[发布变更/变更管理/02-canary-release-strategy.md|金丝雀发布策略与回滚]]

## See Also

- [[可靠性/备份恢复/16-enterprise-backup-strategy.md|企业级备份策略]]
- [[生产运维/03-on-call-playbook.md|值班手册]]

```

<!-- risk-assessed -->
