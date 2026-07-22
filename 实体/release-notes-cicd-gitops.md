---
title: 发布说明索引 — CI/CD 与 GitOps
description: '- **升级要点**: v2.x 全面重构为组件化架构（Source/Kustomize/Helm/Notification 控制器）'
summary: '- **升级要点**: v2.x 全面重构为组件化架构（Source/Kustomize/Helm/Notification 控制器）'
category: references
tags:
- k8s
- release-notes
- cicd
- gitops
- argo-cd
- flux
- tekton
- helm
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布说明索引 — CI/CD 与 GitOps 是什么
- 如何 发布说明索引 — CI/CD 与 GitOps
trigger_keywords:
- 发布说明索引
- CI
- CD
- GitOps
prerequisites:
- kubectl-basics
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 发布说明索引 — CI/CD 与 GitOps

> 本文档汇总 CI/CD 与 GitOps 领域 3 个核心项目的发布说明索引，共覆盖 **171 篇**发布说明。

---

## 项目总览

| 项目 | 文件数 | 最新版本 | 最近 Breaking Changes | 说明 |
|------|--------|----------|----------------------|------|
| Argo CD | 40 | v3.3 | v2.4 | GitOps 持续交付 |
| Flux | 51 | v2.8 | v2.5 | GitOps 工具集 |
| Tekton | 80 | v1.11 | v1.11 | 云原生 CI/CD 引擎 |

---

## 项目详情

### Argo CD

- **最新版本**: v3.3
- **发布说明目录**: `生态参考/_archived-release-notes/cicd-gitops/argo-cd/`
- **版本覆盖**: v0.1 → v3.3（40 个版本）
- **Breaking Changes 提醒**:
  - v2.4: ApplicationSet 控制器合并与 API 变更
- **升级要点**: v2.x 引入 ApplicationSet 和多集群支持增强

### Flux

- **实体页面**: [[flux|Flux]]
- **最新版本**: v2.8
- **发布说明目录**: `生态参考/_archived-release-notes/cicd-gitops/flux/`
- **版本覆盖**: v0.1 → v2.8（51 个版本）
- **Breaking Changes 提醒**:
  - v2.5: Source API 和 Kustomization 控制器行为变更
  - v2.3/v2.4: HelmRelease 和 GitRepository API 调整
- **升级要点**: v2.x 全面重构为组件化架构（Source/Kustomize/Helm/Notification 控制器）

### Tekton

- **最新版本**: v1.11
- **发布说明目录**: `生态参考/_archived-release-notes/cicd-gitops/tekton/`
- **版本覆盖**: v0.1 → v1.11（80 个版本）
- **Breaking Changes 提醒**:
  - v1.11: Task 和 Pipeline API 字段变更
  - v1.0 (里程碑): GA 版本，API 稳定化
- **升级要点**: v1.x 为 GA 版本，API 向后兼容

---

## 跨项目 Breaking Changes 汇总

| 版本 | 项目 | 变更摘要 |
|------|------|----------|
| v2.4 | Argo CD | ApplicationSet 控制器合并 |
| v2.5 | Flux | Source API 和 Kustomization 控制器变更 |
| v1.11 | Tekton | Task/Pipeline API 字段变更 |

---

## 相关导航

- [[概念/gitops-tool-evolution.md|GitOps 工具演进]]
- [[生态参考/98-merged-indexes/index.md|发布说明阅读指南]]
- [[MOC|发布说明总目录]]

## GitOps 组件升级检查

```bash
# 🟢 检查 Argo CD 版本
kubectl get pods -n argocd -o custom-columns=NAME:.metadata.name,IMAGE:.spec.containers[0].image | grep server
argocd version --client
kubectl get cm argocd-cm -n argocd -o yaml | grep server.image

# 🟢 检查 Argo CD 应用同步状态
argocd app list
argocd app get <app-name>

# 🟢 检查 Flux 版本
flux version
kubectl get pods -n flux-system -o custom-columns=NAME:.metadata.name,IMAGE:.spec.containers[0].image

# 🟢 检查 Flux 资源状态
flux get sources git
flux get kustomizations
flux get helmreleases

# 🟢 检查 Tekton 版本
kubectl get pods -n tekton-pipelines -o custom-columns=NAME:.metadata.name,IMAGE:.spec.containers[0].image | grep controller
kubectl get pipelineruns -A --sort-by=.metadata.creationTimestamp | tail -5

# 🟢 检查 Tekton Pipeline 状态
kubectl get pipelines -A
kubectl get tasks -A
```

## GitOps 组件升级路径

| 组件 | 当前版本 | 目标版本 | 关键注意事项 |
|------|----------|----------|----------------|
| Argo CD | v2.3 | v2.4+ | ApplicationSet 控制器合并 |
| Argo CD | v2.x | v3.x | 重大架构变更，需详细评估 |
| Flux | v2.0 | v2.3+ | HelmRelease API 调整 |
| Flux | v2.3 | v2.5+ | Source API 和 Kustomization 变更 |
| Tekton | v0.x | v1.0+ | GA 版本，API 稳定化 |
| Tekton | v1.0 | v1.11 | Task/Pipeline API 字段变更 |

## 升级前检查清单

```bash
# 🟢 检查 Argo CD 应用健康状态
argocd app list --output wide | grep -v Healthy

# 🟢 检查 Flux 资源就绪状态
flux get all -A | grep -v "Applied\|Ready"

# 🟢 检查 Tekton PipelineRun 失败
kubectl get pipelineruns -A --field-selector=status.success=false

# 🟢 备份 Argo CD 配置
kubectl get applications -n argocd -o yaml > argocd-apps-backup.yaml
kubectl get appprojects -n argocd -o yaml > argocd-projects-backup.yaml

# 🟢 备份 Flux 配置
flux export sources git --all-namespaces > flux-sources-backup.yaml
flux export kustomization --all-namespaces > flux-kustomizations-backup.yaml
flux export helmrelease --all-namespaces > flux-helmreleases-backup.yaml
```

## GitOps 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Argo CD OutOfSync | Git 与集群状态不一致 | `argocd app diff <app>` | Sync 或修复漂移 |
| Flux 资源未就绪 | Helm Chart 安装失败 | `flux get helmreleases` | 检查 HelmRelease 事件 |
| Tekton Pipeline 失败 | Task 执行错误 | `kubectl logs <pipelinerun-pod>` | 修复 Task 定义 |
| Argo CD 无法连接 Git | 凭证过期/网络问题 | 检查 repo-server 日志 | 更新 Git 凭证 |
| Flux Source 拉取失败 | Git 仓库不可达 | `flux get sources git` | 检查 URL/凭证 |
| 同步循环 | 资源持续漂移 | `argocd app get <app>` | 检查自引用/Operator 冲突 |

## 检查清单

- [ ] GitOps 组件版本已确认
- [ ] 所有应用/资源状态健康
- [ ] 配置已备份
- [ ] 升级回滚方案已准备
- [ ] Git 仓库凭证有效
- [ ] 监控告警覆盖同步状态
- [ ] 升级窗口已通知相关团队

## Related

- [[实体/k8s-production-operations.md|k8s-production-operations]] — 生产运维：GitOps、FinOps、灾备恢复与变更管理
- [[flux]] — Flux
- [[helm]] — Helm
- [[argo]] — Argo Workflows
- [[实体/argocd.md|argocd]] — ArgoCD

<!-- risk-assessed -->
