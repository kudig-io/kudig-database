# Deployment Create — Kubernetes Deployment 控制器源码分析

本模块基于 Kubernetes 官方源码（`kubernetes/kubernetes`），系统梳理 Deployment 控制器创建、更新、扩缩容、滚动发布的完整逻辑。

---

## 文档索引

| 文档 | 内容 |
|------|------|
| [01-overview](01-overview.md) | Deployment 架构总览：控制器链、 informer 机制、工作队列 |
| [02-deployment-controller](02-deployment-controller.md) | Deployment 控制器入口：syncDeployment、事件处理、编排逻辑 |
| [03-replicaset-controller](03-replicaset-controller.md) | ReplicaSet 控制器：副本管理、Pod 创建/删除、期望状态对齐 |
| [04-rolling-update](04-rolling-update.md) | 滚动更新源码：maxSurge/maxUnavailable、比例缩放、暂停恢复 |
| [05-deployment-status](05-deployment-status.md) | Status 计算逻辑：availableReplicas、progressing、conditions |
| [06-revision-history](06-revision-history.md) | 版本历史与回滚：revision annotation、rollback 实现、清理策略 |

---

## 源码参考

- Deployment 控制器: `pkg/controller/deployment/`
- ReplicaSet 控制器: `pkg/controller/replicaset/`
- 客户端封装: `pkg/controller/deployment/util/`
- 进度追踪: `pkg/controller/deployment/progress.go`
- API 类型定义: `pkg/apis/apps/` / `staging/src/k8s.io/api/apps/v1/`

---

## 版本说明

- 基于 Kubernetes v1.28 - v1.32 源码分析
- Deployment 自 v1.9 起 GA，控制器逻辑已稳定
