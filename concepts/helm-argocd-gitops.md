---
title: "Helm 与 ArgoCD GitOps 工作流"
category: synthesis
tags: [synthesis, helm, argocd, gitops]
sources: []
created: 2026-05-24
updated: 2026-05-24
last_updated: 2026-05-24
---

# Helm 与 ArgoCD GitOps 工作流

> Helm 作为 Kubernetes 包管理器与 ArgoCD 作为声明式 GitOps 持续部署工具的协同工作模式。

## 核心模式

Helm 负责模板化和打包，ArgoCD 负责声明式同步和漂移检测。两者结合形成完整的 GitOps 工作流。

## 工作流

```
代码变更 → Helm Chart 更新 → Git 推送 → ArgoCD 检测 → 自动/手动同步 → 集群更新
```

## 集成方式

1. **Helm 作为 ArgoCD 应用源**: ArgoCD 原生支持 Helm chart 作为应用源
2. **Values 覆盖**: 支持多环境 values 文件（dev/staging/prod）
3. **同步策略**: 自动同步、手动确认、同步窗口

## 最佳实践

- 使用 Helm umbrella chart 管理多组件部署
- 通过 ArgoCD ApplicationSet 实现多集群部署
- 配置 Sync Windows 控制生产环境变更窗口

## 相关页面

- [[helm]] — Helm Chart 管理
- [[argocd]] — ArgoCD 持续部署
- [[deployment]] — Deployment 策略
- [[kubernetes]] — 集群架构
