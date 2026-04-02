# GitOps 与持续交付

## 概述

**GitOps** 是一种以 Git 为唯一事实来源（Single Source of Truth）的运营模型，将基础设施和应用配置的声明式定义存储在 Git 仓库中，通过自动化控制器持续同步集群状态与 Git 中的期望状态。2026 年，GitOps 已成为 Kubernetes 平台工程和多云交付的事实标准，主流实现包括 **Argo CD** 和 **Flux**。

## 核心概念/原理

### 1. GitOps 四大原则

1. **声明式系统**：所有基础设施和应用都以声明式 YAML 描述
2. **版本化与不可变**：Git 仓库作为唯一的配置来源和变更历史记录
3. **自动拉取同步**：控制器自动检测 Git 变更并同步到目标集群
4. **持续协调（Reconciliation）**：控制器持续监控集群状态，自动修复漂移（Drift）

### 2. Argo CD

**Argo CD** 是 CNCF 毕业项目，专为 Kubernetes 设计的声明式 GitOps 持续交付工具：
- **Application 抽象**：将 Git 仓库路径与目标集群/Namespace 绑定
- **多源支持**：可同时从 Helm Chart、Kustomize、Plain YAML 同步
- **可视化 UI**：提供应用拓扑图、资源健康状态、同步历史
- **高级部署策略**：支持蓝绿、金丝雀、滚动升级（通过 Argo Rollouts 扩展）

### 3. Flux

**Flux** 是 CNCF 毕业项目，GitOps 工具的另一主流选择：
- **原生控制器架构**：完全基于 Kubernetes Controller 构建，无外部依赖
- **Source Controller**：支持 Git、Helm、S3、OCI 等多种源类型
- **Kustomize 原生集成**：原生支持 Kustomize 覆盖和补丁管理
- **Flux 生态**：包含 Image Automation（镜像自动更新）、Notification Controller（事件通知）

### 4. GitOps vs 传统 CI/CD

| 维度 | 传统 CI/CD（Push 模式） | GitOps（Pull 模式） |
|------|------------------------|---------------------|
| 凭证管理 | CI 系统需集群 admin 凭证 | 集群只需读 Git 仓库凭证 |
| 安全性 | 外部系统直接操作集群 | 集群自治拉取，最小权限 |
| 可观测性 | 依赖 CI Pipeline 日志 | Git 提交即审计日志 |
| 故障恢复 | 手动重跑 Pipeline | 控制器自动修复漂移 |

## 关键机制或特性

### 多集群/多环境交付

GitOps 通过 **Repository 分支策略** 或 **Overlay 目录结构** 管理多环境：

```
repo/
├── base/                  # 基础资源定义
│   ├── deployment.yaml
│   └── service.yaml
├── overlays/
│   ├── dev/               # 开发环境覆盖
│   ├── staging/           # 预发环境覆盖
│   └── production/        # 生产环境覆盖
```

### 渐进式交付（Progressive Delivery）

结合 **Argo Rollouts** 实现：
- **金丝雀分析（Canary Analysis）**：自动基于 Prometheus 指标判断是否继续推广
- **蓝绿部署**：一键切换活跃服务版本
- **A/B 测试**：按 Header 或权重分配流量

### 镜像自动更新

Flux 的 Image Automation Controller 可：
- 扫描容器镜像仓库发现新版本
- 自动更新 Git 仓库中的镜像标签
- 提交 PR 或直接推送变更，触发同步

## 使用场景

1. **Kubernetes 配置管理**：将所有 Deployment、Service、ConfigMap 纳入 GitOps 管理，任何配置变更都通过 Git PR 完成
2. **多集群一致性交付**：使用同一 Git 仓库为分布于多个云区域的集群同步相同的基线配置
3. **基础设施即代码（IaC）**：通过 GitOps 管理 Terraform/Cluster API 生成的 Kubernetes 资源
4. **自动化回滚**：当新版本出现异常时，通过 Git Revert 快速将集群状态回滚到上一个已知良好版本

## 最佳实践/注意事项

- **分离 Source Repo 与 Config Repo**：应用代码和部署配置分开管理，减少耦合和误操作
- **启用同步前 Dry-Run**：在关键环境启用 Argo CD 的 `Sync Windows` 和 `Resource Hooks`，避免高峰时段意外变更
- **最小权限原则**：GitOps 控制器仅需要目标 Namespace 的读写权限，无需集群 admin 权限
- **漂移告警配置**：当集群状态与 Git 定义不一致时，应及时通过 Slack/PagerDuty 通知运维团队
- **Secret 不存 Git**：使用 External Secrets Operator 或 Sealed Secrets 将敏感信息外部化管理
- **多租户隔离**：为不同团队创建独立的 Application/Namespace，配置 ResourceQuota 和 NetworkPolicy
- **Git 分支策略**：生产环境仅接受 main 分支的同步，所有变更必须经过 PR Review 和自动化检查

## 参考链接

- [Argo CD Documentation](https://argo-cd.readthedocs.io/)
- [Flux Documentation](https://fluxcd.io/)
- [GitOps Working Group - Principles](https://opengitops.dev/)
- [CNCF GitOps Landscape](https://landscape.cncf.io/card-mode?category=continuous-delivery&grouping=category)
