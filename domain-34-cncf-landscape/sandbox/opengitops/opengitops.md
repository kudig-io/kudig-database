# OpenGitOps

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://opengitops.dev/ |
| **GitHub** | https://github.com/open-gitops |
| **许可证** | Apache-2.0 |
| **CNCF 状态** | Sandbox |

---

## 项目概述

OpenGitOps 是一个 CNCF Sandbox 项目，定义了 GitOps 的标准原则和最佳实践。它并非一个软件工具，而是一组社区驱动的 GitOps 规范和标准，为 GitOps 实践提供厂商中立的定义和指南。

### GitOps 四项原则

1. **声明式 (Declarative)**: 由 GitOps 管理的系统必须以声明式方式表达其期望状态
2. **版本化和不可变 (Versioned and Immutable)**: 期望状态存储在版本控制系统中，作为不可变的真相来源
3. **自动拉取 (Pulled Automatically)**: 软件代理自动从来源拉取期望状态的声明
4. **持续协调 (Continuously Reconciled)**: 软件代理持续观察实际状态并尝试应用期望状态

### 核心内容

- **GitOps 原则规范**: v1.0 标准化 GitOps 的核心定义
- **术语表**: 标准化 GitOps 相关术语（期望状态、实际状态、漂移、协调等）
- **合规标准**: 评估工具是否符合 GitOps 原则的标准

---

## GitOps 实践指南

### 符合 OpenGitOps 原则的工具

| 工具 | 类型 | 说明 |
|:---|:---|:---|
| **Flux CD** | Kubernetes GitOps | CNCF Graduated 项目 |
| **Argo CD** | Kubernetes GitOps | CNCF Graduated 项目 |
| **PipeCD** | 多平台 GitOps | 支持 K8s, Terraform, CloudRun |
| **Terraform** + Git | 基础设施 GitOps | 声明式基础设施管理 |

### GitOps 工作流示例

```
Developer ──► Git Push ──► Git Repository (Source of Truth)
                                    │
                              ┌─────┴─────┐
                              │            │
                              ▼            ▼
                         Flux CD      Argo CD
                         (Pull)       (Pull)
                              │            │
                              ▼            ▼
                         Kubernetes Clusters
                         (Reconcile to desired state)
```

---

## 最佳实践

1. **Git 作为唯一来源**: 所有配置变更通过 Git PR/MR 流程管理
2. **不可变部署**: 使用镜像 digest 而非 mutable tag (如 latest)
3. **自动协调**: 部署工具应持续监控并纠正状态漂移
4. **分离仓库**: 应用代码和部署配置使用独立的 Git 仓库
5. **审计追踪**: 利用 Git 历史提供完整的变更审计日志

---

## 参考资源

- [OpenGitOps 官方网站](https://opengitops.dev/)
- [GitOps 原则 v1.0](https://github.com/open-gitops/documents)
- [OpenGitOps GitHub](https://github.com/open-gitops)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
