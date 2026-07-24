---
title: Deployment × RBAC
summary: Deployment × RBAC：Deployment与RBAC是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- workloads
- security
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-07
relationships:
- target: '[[实体/helm.md]]'
  type: uses
- target: '[[实体/prometheus.md]]'
  type: uses
- target: '[[实体/argocd.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可轮滚）、🟢 低风险/只读（信息收集，无副作用）。

# Deployment × RBAC

## 概述
Deployment 本身是一个被 RBAC 管理的资源——谁有权创建/修改/删除 Deployment 由 RBAC 规则控制。同时，Deployment 管理的 Pod 可能需要通过自身的 ServiceAccount 访问 Kubernetes API（如执行 CRUD 操作），这也由 RBAC 控制。两层 RBAC 常被混淆：一层控制"谁能管理 Deployment"（运维人员权限），另一层控制"Deployment 的 Pod 能做什么"（应用权限）。

## 技术关联机制

1. **管理 Deployment 的 RBAC**：运维人员/CI/CD 系统需要对 `deployments` 资源（`apps` API 组）拥有 `create/get/list/watch/update/patch/delete` 权限。这些权限通过 Role/ClusterRole + RoleBinding/ClusterRoleBinding 授予。例如，ArgoCD 的 SA 需要 ClusterRole 包含 `apps/deployments` 的所有操作权限。开发团队可能只需要 `get/list/watch` 只读权限。

2. **Deployment Pod 的 ServiceAccount RBAC**：Deployment 的 Pod template 中可以指定 `serviceAccountName`。这个 SA 的 RBAC 权限决定了 Pod 内应用能调用的 Kubernetes API。例如，一个需要动态发现 Service 的应用 Pod 需要其 SA 拥有 `services` 的 `get/list/watch` 权限。如果 SA 权限不足，应用调用 API 返回 403。

3. **Deployment 操作的最小权限集**：
   - **创建 Deployment**：`apps/deployments: create`
   - **查看 Deployment**：`apps/deployments: get, list, watch`
   - **更新 Deployment**（如 scale/rollout）：`apps/deployments: update, patch`
   - **删除 Deployment**：`apps/deployments: delete`
   - **级联资源**：`apps/replicasets: *`、`pods: *`（通常由 Controller 自动管理，不需要直接授权）

4. **GitOps 场景下的 Deployment RBAC**：GitOps Controller（ArgoCD/Flux）的 SA 需要管理 Deployment 的完整权限。在多租户环境中，通常通过 ApplicationSet 或 Project 限制每个租户只能操作自己 Namespace 的 Deployment，实现租户隔离。

## 实践场景

- **CI/CD 部署权限**：为 CI pipeline 的 SA 配置 `apps/deployments: create,update,patch,get` 权限，允许自动化部署
- **开发团队只读**：为开发团队配置 `apps/deployments: get,list,watch` 权限，允许查看但禁止修改
- **应用 Pod API 访问**：为需要调用 Kubernetes API 的应用 Pod 配置 ServiceAccount 和 Role（如服务注册/配置读取）
- **多租户隔离**：通过 Namespace 级 RoleBinding 将 Deployment 管理权限限制在租户 Namespace 内

## 常见问题

### 问题1：CI/CD 部署时报 403 Forbidden
**症状**：CI pipeline 执行 `kubectl apply -f deployment.yaml` 报 `deployments.apps is forbidden`
**根因**：CI SA 缺少对 `deployments` 资源的 `create` 权限
**修复**：创建 Role/RoleBinding 授予 CI SA 在目标 Namespace 的 `apps/deployments: create,update,patch` 权限

### 问题2：Deployment Pod 调用 Kubernetes API 返回 403
**症状**：应用 Pod 内代码调用 API 返回 `forbidden: cannot list resource "pods"`
**根因**：Pod 使用的 ServiceAccount 缺少必要权限
**修复**：为 Pod 的 SA 创建 Role + RoleBinding 授予所需资源的最小权限

### 问题3：ArgoCD 无法同步 Deployment
**症状**：ArgoCD Application 同步失败，日志显示 403 错误
**根因**：ArgoCD SA 的 ClusterRole/Role 缺少对目标 Namespace Deployment 的操作权限
**修复**：检查 ArgoCD SA 的 RBAC 配置；确保 ClusterRoleBinding 或 Namespace 级 RoleBinding 覆盖目标 Namespace

## 关键命令

```bash
# 🟢 检查当前用户对 Deployment 的权限
kubectl auth can-i create deployments -n <ns>
kubectl auth can-i update deployments -n <ns>

# 🟢 检查某 SA 对 Deployment 的权限
kubectl auth can-i create deployments --as=system:serviceaccount:<ns>:<sa> -n <ns>

# 🟢 查看 Deployment Pod 使用的 ServiceAccount
kubectl get deployment <name> -n <ns> -o jsonpath='{.spec.template.spec.serviceAccountName}'

# 🟢 查看 SA 的权限列表
kubectl auth can-i --list --as=system:serviceaccount:<ns>:<sa> -n <ns>

# 🟡 创建 Deployment 管理权限的 Role 和 RoleBinding
kubectl create role deployment-manager --verb=create,update,patch,get,list,watch --resource=deployments -n <ns>
kubectl create rolebinding ci-deployment-manager --role=deployment-manager --serviceaccount=<ns>:<ci-sa> -n <ns>
```

## 权衡取舍

| 维度 | Deployment 倾向 | RBAC 倾向 | 权衡点 |
|------|----------------|---------|--------|
| 管理权限 | 宽松权限简化部署 | 严格权限提升安全 | 部署效率 vs 安全性 |
| Pod SA 权限 | 宽泛权限简化应用 | 最小权限降低风险 | 开发便利 vs 安全隔离 |
| 多租户管理 | 集中管理简化运维 | Namespace 隔离提升安全 | 运维效率 vs 租户隔离 |
| GitOps 权限 | ClusterRole 简化配置 | Namespace Role 精细控制 | 配置简单 vs 安全粒度 |

## 最佳实践
1. 区分"管理 Deployment 的人/系统的权限"和"Deployment Pod 的应用权限"两层 RBAC
2. 为 CI/CD pipeline 配置专用的 SA 和最小权限 Role（仅 `deployments: create,update,patch`）
3. 应用 Pod 的 ServiceAccount 遵循最小权限原则，仅授予应用功能所需的精确资源操作权限
4. 多租户环境通过 Namespace 级 RoleBinding 将 Deployment 管理权限限制在租户 Namespace 内

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[Deployment]]
- RBAC
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/apiserver-×-RBAC.md|apiserver-×-RBAC]]
- [[概念/apiserver-×-NetworkPolicy.md|apiserver-×-NetworkPolicy]]


<!-- risk-assessed -->
