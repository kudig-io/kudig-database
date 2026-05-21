---
title: GitOps/DevOps 排查
description: '# GitOps/DevOps 排查'
category: skills
tags:
- k8s
- troubleshooting
- structural
- gitops-devops
- etcd
- helm
- argocd
- flux
- docker
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- GitOps/DevOps 排查 是什么
- 如何 GitOps/DevOps 排查
trigger_keywords:
- GitOps
- DevOps
- 排查
prerequisites:
- kubectl-basics
- helm-basics
- gitops-basics
- etcd-basics
- backup-basics
---

# GitOps/DevOps 排查

### 01 Gitops Devops Troubleshooting

#### 0. 10 分钟快速诊断

1. **控制器存活**：检查 ArgoCD/Flux 控制器 Pod 状态与日志。
2. **同步状态**：`kubectl get applications/helmreleases/kustomizations -A`，定位 OutOfSync/Failed。
3. **仓库连接**：验证 repo secret/SSH key/Token，确认仓库可访问。
4. **渲染检查**：确认 Helm/Kustomize 渲染是否失败或资源冲突。
5. **漂移检测**：对关键应用执行 diff，判断实际与期望偏差。
6. **快速缓解**：
   - 临时暂停自动同步，手动回滚稳定版本。
   - 修复仓库凭证或降低同步频率。
7. **证据留存**：保存控制器日志、同步状态与失败事件。

#### GitOps 核心组件故障现象

| 问题类型 | 典型现象 | 影响程度 | 紧急级别 |
|---------|---------|---------|---------|
| ArgoCD 同步失败 | `ApplicationOutOfSync` 持续存在 | ⭐⭐⭐ 高 | P0 |
| FluxCD reconciliation 失败 | `ReconciliationFailed` 事件频繁 | ⭐⭐⭐ 高 | P0 |
| Git 仓库连接问题 | `failed to clone repository` | ⭐⭐⭐ 高 | P0 |
| Helm Chart 部署失败 | `Helm release failed` | ⭐⭐ 中 | P1 |
| CI/CD 流水线阻塞 | `pipeline stuck in pending` | ⭐⭐⭐ 高 | P0 |
| 配置漂移检测失效 | `drift detected but not corrected` | ⭐⭐ 中 | P1 |
| 多环境同步异常 | `environments out of sync` | ⭐⭐ 中 | P1 |
| Secret 管理失败 | `failed to decrypt secrets` | ⭐⭐⭐ 高 | P0 |

#### GitOps 状态检查命令

```bash
# ArgoCD 状态检查
echo "=== ArgoCD 状态检查 ==="
kubectl get pods -n argocd -l app.kubernetes.io/name=argocd-application-controller
kubectl get applications -A
argocd app list 2>/dev/null || echo "ArgoCD CLI 未配置"

# FluxCD 状态检查
echo "=== FluxCD 状态检查 ==="
kubectl get pods -n flux-system
flux check 2>/dev/null || echo "Flux CLI 未配置"
kubectl get kustomizations,helmreleases -A

# Git 仓库连接检查
echo "=== Git 仓库连接检查 ==="
kubectl get gitrepositories -A
kubectl get secrets -n argocd -l argocd.argoproj.io/secret-type=repository 2>/dev/null

# CI/CD 流水线状态
echo "=== CI/CD 流水线状态 ==="
kubectl get pipelineruns,taskruns -A 2>/dev/null || echo "Tekton 未部署"
# 或者检查 Jenkins/GitLab CI 等其他 CI 系统状态
```

---

### 02 Tekton Troubleshooting

#### 0. 10 分钟快速诊断

1. **PipelineRun 状态**：`tkn pipelinerun list` 或 `kubectl get pipelineruns -A`，查看失败的运行。
2. **TaskRun 详情**：`tkn taskrun logs <taskrun-name>` 查看具体任务日志。
3. **Workspace 状态**：`kubectl get pvc -n <namespace>`，确认 workspace PVC 已绑定。
4. **事件检查**：`kubectl get events --field-selector reason=FailedMount` 或 `FailedPullImage`。
5. **ServiceAccount 权限**：确认 PipelineRun 使用的 ServiceAccount 有创建 Pod 的权限。
6. **快速缓解**：
   - 任务卡住：删除 PipelineRun 后使用 `tkn pipeline start` 重新触发。
   - 镜像拉取失败：检查 `imagePullSecrets` 或切换到公共镜像。
   - Workspace 空间不足：增大 PVC 容量或使用 `emptyDir`。
7. **证据留存**：保存 PipelineRun YAML、TaskRun 日志、Workspace 使用情况和节点事件。

---

#### 2. 排查方法与步骤



#### 2.1 诊断原理说明

Tekton Pipelines 的执行流程：

```
用户创建 PipelineRun
        │
        ▼
┌─────────────────────────────┐
│ Tekton Pipelines Controller │ ──► 解析 PipelineRun，创建 TaskRun
│ (tekton-pipelines-controller)│
└──────────────┬──────────────┘
               │
        ┌──────┴──────┐
        ▼             ▼
┌─────────────┐   ┌─────────────┐
│   TaskRun   │   │   TaskRun   │
│   (串行)    │──►│   (并行)    │
└──────┬──────┘   └──────┬──────┘
       │                 │
       ▼                 ▼
┌─────────────┐   ┌─────────────┐
│    Pod      │   │    Pod      │
│  (步骤容器)  │   │  (步骤容器)  │
└─────────────┘   └─────────────┘
```

**关键概念**：
- **Workspace**：任务间共享数据的机制，可以是 PVC、`emptyDir`、ConfigMap、Secret 或 CSI 卷
- **Step**：Task 中的最小执行单元，每个 Step 对应一个容器，按顺序执行
- **Sidecar**：与 Step 容器并行运行的辅助容器（如 Docker daemon、数据库）
- **Result**：Task 向 Pipeline 传递的小型输出（限制 4KB）

---

### 03 Flux Image Automation Troubleshooting

#### 0. 10 分钟快速诊断

1. **ImageRepository 状态**：`flux get image repositories`，确认 `READY` 列状态。
2. **ImagePolicy 状态**：`flux get image policies`，确认策略是否正确匹配标签。
3. **ImageUpdateAutomation 状态**：`flux get image update`，确认自动化是否启用。
4. **Git 写权限**：检查 ImageUpdateAutomation 引用的 GitRepository 是否有写入权限。
5. **最近扫描日志**：查看 image-reflector-controller 日志中的扫描和策略评估结果。
6. **快速缓解**：
   - 镜像扫描失败：检查仓库认证 Secret 和 registry 可达性。
   - 策略不匹配：验证 ImagePolicy 的 semver/regex 规则。
   - 自动提交失败：确认 GitRepository 的 `secretRef` 包含写权限的凭据。
7. **证据留存**：保存 ImageRepository、ImagePolicy、ImageUpdateAutomation 的 YAML、控制器日志、Git commit 历史。

---

#### 2. 排查方法与步骤



#### 2.1 诊断原理说明

Flux 镜像自动化由两个控制器协同工作：

```
ImageRepository
        │
        ▼ 定期扫描
┌─────────────────────────────┐
│ image-reflector-controller  │ ──► 扫描镜像仓库，获取所有标签
└──────────────┬──────────────┘
               │
               ▼
ImagePolicy ──► 根据策略过滤标签，选出最新镜像
               │
               ▼
ImageUpdateAutomation
               │
               ▼
┌─────────────────────────────┐
│ image-automation-controller │ ──► 读取 Git 仓库，应用镜像更新
│                             │     生成 commit 并 push
└─────────────────────────────┘
```

**关键概念**：
- **ImageRepository**：定义要扫描的镜像仓库和扫描间隔
- **ImagePolicy**：定义如何从扫描到的标签中选择最新版本（semver、alphabetical、numerical）
- **ImageUpdateAutomation**：定义如何自动更新 Git 仓库中的镜像引用
- **Policy 标记**：在 Git 仓库的 YAML 中通过注释 `# {"$imagepolicy": "policy-name"}` 标记需要自动更新的字段

---

### 备份恢复故障排查指南

#### 0. 快速诊断

1. **Velero 状态检查**：`kubectl get pods -n velero`，确认 velero DaemonSet 和 Deployment 均为 Running。
2. **备份任务状态**：`kubectl get backup -n velero`，查看 Recent Backup 的 Phase（New/InProgress/Completed/Failed）。
3. **etcd 快照状态**：`kubectl get pods -n kube-system -l app=etcd-operator`，确认 etcd backup operator 正常运行。
4. **日志快速排查**：
   - Velero：`kubectl logs -n velero deployment/velero --tail=50 | grep -i error`
   - etcd-snapshot：`kubectl logs -n kube-system -l app=etcd-operator --tail=30`
5. **RTO/RPO 检查**：核对最近一次成功备份的时间戳，计算是否在 SLA 范围内。

---

#### 2. 排查方法与步骤



#### 2.2 Velero Backup 失败排查

#### Step 1: 检查 Velero 状态

```bash
# 检查 Velero Pod 状态
kubectl get pods -n velero

# 检查备份任务详情
kubectl describe backup -n velero {backup-name}

# 查看 Velero Pod 日志
kubectl logs -n velero deployment/velero --tail=100 | grep -i error
```

#### Step 2: 检查存储后端

```bash
# 检查 BackupStorageLocation 状态
kubectl get backupstoragelocation -n velero

# 描述 BackupStorageLocation 获取详细错误
kubectl describe backupstoragelocation -n velero default

# 检查凭据 Secret
kubectl get secret -n velero velero-backup-creds
```

#### Step 3: 检查 Volume 快照

```bash
# 检查 VolumeSnapshotClass
kubectl get volumesnapshotclass

# 检查 VolumeSnapshot
kubectl get volumesnapshot -n {namespace}

# 检查 CSI Driver 状态
kubectl get csidriver
kubectl get pods -n kube-system | grep csi
```

## 相关链接

- [[skills/develop-crd-operator.md|CRD/Operator 开发]]

## Related

- [[flux]] — Flux
- [[helm]] — Helm
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[entities/argocd.md|argocd]] — ArgoCD
