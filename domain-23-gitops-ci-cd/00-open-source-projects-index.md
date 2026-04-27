# Domain-23 GitOps & CI/CD — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: Argo CD v3.3 / Flux v2.5 / Tekton v0.65

---

## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、Argo 生态详解 (CNCF Graduated)](#二argo-生态详解-cncf-graduated)
- [三、Flux 生态详解 (CNCF Graduated)](#三flux-生态详解-cncf-graduated)
- [四、CI/CD 流水线项目](#四cicd-流水线项目)
- [五、版本与发布动态](#五版本与发布动态)
- [六、GitOps 选型指南](#六gitops-选型指南)

---

## 一、核心项目总览

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Argo CD** | 声明式 GitOps CD | Graduated | v3.3.8 | 18k+ | Apache-2.0 |
| **Argo Workflows** | 容器化工作流引擎 | Graduated | v3.6.0 | 15k+ | Apache-2.0 |
| **Argo Rollouts** | 渐进式交付 | Graduated | v1.8.0 | 2.5k+ | Apache-2.0 |
| **Argo Events** | 事件驱动自动化 | Graduated | v1.9.0 | 2k+ | Apache-2.0 |
| **Flux** | GitOps 持续交付 | Graduated | v2.5.0 | 6k+ | Apache-2.0 |
| **Flagger** | 渐进式发布控制器 | Flux 生态 | v1.40.0 | 4.5k+ | Apache-2.0 |
| **Tekton** | 云原生 CI/CD 框架 | CDF | v0.65.0 | 8k+ | Apache-2.0 |
| **Jenkins** | CI/CD 服务器 | CDF | v2.492.3 | 23k+ | MIT |
| **GitLab CI** | 集成 CI/CD | GitLab | v17.10.0 | - | EE/CE |
| **GitHub Actions** | 托管 CI/CD | GitHub | - | - | 商业 |
| **KubeSphere DevOps** | 集成 DevOps 平台 | 非 CNCF | v4.1.0 | 15k+ | Apache-2.0 |
| **Carvel kapp-controller** | K8s 应用交付 | VMware | v0.55.0 | 1k+ | Apache-2.0 |
| **Spinnaker** | 多云持续交付平台 | Netflix/Armory | v1.37.0 | 9k+ | Apache-2.0 |
| **Concourse CI** | 声明式 CI/CD 管道 | VMware | v7.12.0 | 7k+ | Apache-2.0 |
| **Woodpecker CI** | 轻量级 CI/CD (Drone fork) | 社区 | v3.0.0 | 4k+ | Apache-2.0 |
| **Argo CD Image Updater** | Argo CD 镜像自动更新 | Argo | v0.15.0 | 1k+ | Apache-2.0 |
| **Renovate** | 自动化依赖更新 | Mend | v39.0.0 | 18k+ | AGPL-3.0 |
| **SOPS** | YAML/JSON 加密 (GitOps 密钥) | Mozilla | v3.9.0 | 17k+ | MPL-2.0 |
| **Reloader** | ConfigMap/Secret 变更自动重启 | Stakater | v1.3.0 | 7k+ | Apache-2.0 |

---

## 二、Argo 生态详解 (CNCF Graduated)

### 2.1 Argo CD

```yaml
# 核心特性
- 声明式 GitOps 持续交付
- 自动同步与自愈
- 多集群多租户管理
- 支持 Helm/Kustomize/Jsonnet
- RBAC 与 SSO 集成
- 可视化应用拓扑
- 回滚与历史版本管理
```

**版本发布路线**

| 版本 | 发布日期 | 支持状态 | 最新补丁 |
|:---|:---|:---|:---|
| v3.3 | 2026.02 | ✅ 活跃支持 | v3.3.8 (2026.04) |
| v3.2 | 2025.11 | ✅ 活跃支持 | v3.2.10 (2026.04) |
| v3.1 | 2025.08 | ✅ 活跃支持 | v3.1.15 (2026.04) |
| v3.0 | 2025.05 | ❌ 已终止 | v3.0.23 (2026.01) |
| v2.14 | 2025.02 | ❌ 已终止 | v2.14.21 (2025.11) |

**v3.0 重大变更**
- 默认 `resource.exclusions` 配置 (排除高变动资源)
- 移除废弃的 `argocd_app_sync_status` 等指标 (迁移至 `argocd_app_info` labels)
- 低风险的 major 版本升级

**Helm 安装**
```bash
helm repo add argo https://argoproj.github.io/argo-helm
helm install argocd argo/argo-cd --version 7.8.0
```

**GitHub**: https://github.com/argoproj/argo-cd
**文档**: https://argo-cd.readthedocs.io/

### 2.2 Argo Workflows

- 容器原生工作流引擎
- DAG (有向无环图) 与步骤工作流
- 资源模板与入口点 (Entrypoint)
- 事件触发与定时任务 (CronWorkflow)
- 制品管理 (Artifacts)

**GitHub**: https://github.com/argoproj/argo-workflows

### 2.3 Argo Rollouts

- 蓝绿部署 (Blue-Green)
- 金丝雀发布 (Canary) — 支持自动/手动推进
- 实验流量 (Experiment)
- 分析运行 (Analysis) — 集成 Prometheus/Datadog
- 与 Ingress Controller / Service Mesh 集成

**GitHub**: https://github.com/argoproj/argo-rollouts

### 2.4 Argo Events

- 事件驱动架构
- 支持 20+ 事件源 (Webhook, S3, SNS, GitHub, Calendar 等)
- 传感器触发 Argo Workflows / K8s 资源 / Lambda

**GitHub**: https://github.com/argoproj/argo-events

---

## 三、Flux 生态详解 (CNCF Graduated)

> ⚠️ **社区动态**: Weaveworks (Flux 主要赞助公司) 于 2024 年初倒闭，但 Flux 项目已被 CNCF 社区接管，继续维护。

### 3.1 Flux v2

```yaml
# 核心特性
- 基于 GitOps Toolkit 构建
- 源管理 (Git/OCI/Helm/Bucket)
- Kustomize 与 Helm 原生支持
- 镜像自动更新 (Image Automation)
- 多租户与 RBAC
- 与 Terraform 集成 (tf-controller)
```

**核心组件**
| 组件 | 作用 |
|:---|:---|
| source-controller | 管理 Git/Helm/OCI 源 |
| kustomize-controller | 应用 Kustomize 覆盖 |
| helm-controller | 管理 Helm releases |
| image-reflector-controller | 扫描镜像仓库 |
| image-automation-controller | 自动更新 Git 中的镜像标签 |
| notification-controller | 事件通知与告警 |

**GitHub**: https://github.com/fluxcd/flux2
**文档**: https://fluxcd.io/

### 3.2 Flagger

- 与 Flux 紧密集成的渐进式交付工具
- 金丝雀、A/B 测试、蓝绿发布
- 与 Istio/Linkerd/Cilium/NGINX Ingress 集成
- 自动化指标分析驱动的回滚/推进

**GitHub**: https://github.com/fluxcd/flagger

---

## 四、CI/CD 流水线项目

### 4.1 Tekton

```yaml
# 核心概念
- Task: 最小可执行单元
- TaskRun: Task 的一次执行
- Pipeline: 多个 Task 的有序组合
- PipelineRun: Pipeline 的一次执行
- Workspace: Task/Pipeline 间的数据共享
```

**优势**: 完全 K8s 原生、可组合、声明式
**劣势**: 配置较复杂、生态工具链不如 Jenkins 成熟

**GitHub**: https://github.com/tektoncd/pipeline

### 4.2 Jenkins on K8s

- 最成熟的 CI/CD 服务器
- Jenkins Kubernetes Plugin 动态创建 Pod 作为 Agent
- 海量插件生态 (1800+)
- 缺点: 安全性历史包袱、配置漂移

**推荐**: Jenkins X (云原生 Jenkins) 已停止活跃开发，建议评估 **Tekton** 或 **Argo Workflows** 替代。

### 4.3 GitLab CI / GitHub Actions

| 维度 | GitLab CI | GitHub Actions |
|:---|:---|:---|
| 运行器 | 自托管 Runner / Shared Runner | 自托管 Runner / GitHub-hosted |
| K8s 集成 | 原生 K8s Executor | 社区 Action |
| 容器注册表 | 集成 GitLab Registry | GitHub Packages |
| 安全扫描 | 集成 SAST/DAST | 依赖 GitHub Advanced Security |
| 自托管 | GitLab CE/EE | GitHub Enterprise Server |

---

## 五、版本与发布动态

### Argo CD 支持周期
- 每 3 个月一个 minor 版本
- 支持周期: 当前版本 + 前两个版本
- 建议始终使用最新补丁版本

### Helm 4 前瞻
- 开发于 2024.11 KubeCon 正式启动
- 预计 2025.11 KubeCon NA 发布
- 将解决 Helm 3 的架构债务

### Flux 社区接管
- Weaveworks 倒闭后，Flux 由 CNCF 社区维护
- 主要维护者来自 Akuity、ControlPlane、Microsoft 等
- v2.5 为最新稳定版，路线图正常推进

---

## 六、GitOps 选型指南

```
┌─────────────────────────────────────────────────────────────┐
│                    GitOps 工具选型决策树                       │
└─────────────────────────────────────────────────────────────┘

1. 需要 UI 可视化与管理?
   └─ Yes ──► Argo CD (业界最强 UI)
   └─ No  ──► Flux (纯声明式，Git 即 UI)

2. 需要复杂工作流编排?
   └─ Yes ──► Argo Workflows
   └─ No  ──► 纯 CD 工具足够

3. 渐进式交付 (金丝雀/A/B)?
   └─ Yes ──► Argo Rollouts / Flagger
   └─ No  ──► 基础 RollingUpdate

4. 团队规模 > 50 人，多租户?
   └─ Yes ──► Argo CD (RBAC/SSO/项目隔离成熟)
   └─ No  ──► Flux 更简单轻量

5. 已使用 GitLab / GitHub?
   └─ GitLab ──► GitLab CI + Agent (或集成 Argo CD)
   └─ GitHub ──► GitHub Actions + Argo CD/Flux

6. 需要事件驱动自动化?
   └─ Yes ──► Argo Events
   └─ No  ──► 基础 Git Webhook 触发

7. 传统 CI 迁移上 K8s?
   └─ 渐进迁移 ──► Jenkins + K8s Plugin
   └─ 全新构建 ──► Tekton / Argo Workflows
```

---

## 参考链接

- [Argo 官方文档](https://argo-cd.readthedocs.io/)
- [Flux 官方文档](https://fluxcd.io/)
- [Tekton 官方文档](https://tekton.dev/docs/)
- [CNCF CI/CD 白皮书](https://github.com/cncf/tag-app-delivery/blob/main/ci-cd-whitepaper.md)
- [Helm 4 路线图](https://github.com/helm/community/blob/main/hips/hip-0016.md)
