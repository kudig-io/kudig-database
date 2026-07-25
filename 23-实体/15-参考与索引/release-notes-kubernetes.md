---
title: 发布说明索引 — Kubernetes
description: '| v1.2 → v1.36 | CHANGELOG | 35 | 正式变更日志 |'
summary: '| v1.2 → v1.36 | CHANGELOG | 35 | 正式变更日志 |'
category: references
tags:
- k8s
- release-notes
- kubernetes
- changelog
- core
- docker
- daemonset
- rbac
- crd
- webhook
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 发布说明索引 — Kubernetes 是什么
- 如何 发布说明索引 — Kubernetes
trigger_keywords:
- 发布说明索引
- Kubernetes
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 发布说明索引 — Kubernetes

> 本文档汇总 Kubernetes 核心项目的发布说明索引，共覆盖 **55 篇**发布说明。

---

## 版本覆盖范围

| 范围 | 文件类型 | 数量 | 说明 |
|------|----------|------|------|
| v1.2 → v1.36 | CHANGELOG | 35 | 正式变更日志 |
| v0.19 → v1.1 | RELEASE-NOTES | 20 | 早期发布说明 |

---

## 最新版本

- **最新版本**: v1.1 (RELEASE-NOTES) / v1.36 (CHANGELOG)
- **发布说明目录**: `生态参考/_archived-release-notes/kubernetes/`

---

## Breaking Changes 重点版本

以下版本包含重大破坏性变更，升级时需特别关注：

| 版本 | 关键 Breaking Changes |
|------|----------------------|
| v1.36 | 最新版本，持续关注弃用 API 移除 |
| v1.35 | API 弃用和移除策略更新 |
| v1.34 | 安全上下文默认值变更 |
| v1.32 | Pod 安全准入控制器默认策略收紧 |
| v1.29 | FlowSchema 和 PriorityLevelConfiguration GA |
| v1.26 | CRI v1 移除 dockershim 残留 |
| v1.25 | PodSecurityPolicy 正式移除 |
| v1.22 | 多项 beta API 移除（extensions/v1beta1 等） |

---

## 版本演进里程碑

| 阶段 | 版本范围 | 关键特性 |
|------|----------|----------|
| 早期探索 | v0.19 → v0.21 | 基础功能验证 |
| 初步成型 | v1.0 → v1.5 | Deployment/RS/DaemonSet GA |
| API 稳定 | v1.6 → v1.10 | RBAC GA、CRD 替代 TPR |
| 扩展成熟 | v1.11 → v1.15 | Pod 优先级、IPVS、Admission Webhook |
| 规模优化 | v1.16 → v1.20 | Topology Manager、Server-Side Apply |
| 安全加固 | v1.21 → v1.25 | PSP 移除、SeccompDefault、Pod 安全准入 |
| 云原生深化 | v1.26 → v1.30 | 冻结旧 API、Sidecar 容器、用户命名空间 |
| 最新迭代 | v1.31 → v1.36 | 持续性能优化和安全增强 |

---

## 相关导航

- [[kubernetes|Kubernetes]]
- [[23-实体/15-参考与索引/kubernetes-changelog.md|Kubernetes 变更日志索引]]
- [[22-概念/12-研究/kubernetes-version-evolution.md|Kubernetes 版本演进]]
- [[23-实体/15-参考与索引/version-upgrade-guide.md|版本升级指南]]
- [[21-生态参考/98-merged-indexes/index.md|发布说明阅读指南]]
- [[MOC|发布说明总目录]]

## 升级前检查命令

```bash
# 🟢 检查当前集群版本
kubectl version --short 2>/dev/null || kubectl version

# 🟢 检查已弃用 API 使用情况
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis

# 🟢 使用 kubent 检测弃用 API
kubent  # 扫描集群中使用的弃用 API

# 🟢 使用 pluto 检测弃用 API
pluto detect-all-in-cluster

# 🟢 检查节点版本一致性
kubectl get nodes -o custom-columns=NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion

# 🟢 检查控制平面组件版本
kubectl get pods -n kube-system -o custom-columns=NAME:.metadata.name,IMAGE:.spec.containers[0].image | grep -E 'apiserver|scheduler|controller'

# 🟢 检查 CRD API 版本
kubectl get crd -o custom-columns=NAME:.metadata.name,VERSIONS:.spec.versions[*].name | head -20

# 🟢 检查 PodSecurityPolicy (v1.25 前)
kubectl get psp -A 2>/dev/null || echo "PSP not available (v1.25+)"

# 🟢 检查 Pod Security Admission (v1.25+)
kubectl get namespaces -o custom-columns=NAME:.metadata.name,PSA:.metadata.labels | grep pod-security
```

## 版本升级路径建议

| 当前版本 | 目标版本 | 升级策略 | 关键注意事项 |
|----------|----------|----------|----------------|
| v1.24 | v1.25 | 滚动升级 | PSP 移除，迁移到 PSA |
| v1.25 | v1.26 | 滚动升级 | CRI v1alpha2 移除 |
| v1.26 | v1.27 | 滚动升级 | SeccompDefault 默认启用 |
| v1.27 | v1.28 | 滚动升级 | 无重大 Breaking |
| v1.28 | v1.29 | 滚动升级 | FlowSchema GA |
| v1.29 | v1.30 | 滚动升级 | Sidecar 容器 alpha |
| v1.30+ | 最新 | 滚动升级 | 检查弃用 API |

## 升级操作流程

```
Kubernetes 版本升级
├── 1. 升级前评估
│   ├── 阅读目标版本 CHANGELOG
│   ├── 运行 kubent/pluto 检测弃用 API
│   ├── 检查插件兼容性 (CNI/CSI/Ingress)
│   └── 备份 etcd 数据
├── 2. 测试环境验证
│   ├── 在测试集群执行升级
│   ├── 运行全量回归测试
│   └── 验证关键业务功能
├── 3. 生产环境升级
│   ├── 升级控制平面 (Master)
│   ├── 逐批升级 Worker 节点
│   ├── 每批验证业务正常
│   └── 更新 kubectl 客户端
└── 4. 升级后验证
    ├── kubectl get nodes → 所有节点 Ready
    ├── kubectl get pods -A → 无异常 Pod
    ├── 业务功能验证
    └── 监控指标正常
```

## 弃用 API 迁移指南

| 弃用 API | 替代 API | 移除版本 | 迁移命令 |
|----------|----------|----------|----------|
| extensions/v1beta1 Ingress | networking.k8s.io/v1 | v1.22 | `kubectl convert` |
| policy/v1beta1 PSP | Pod Security Admission | v1.25 | 迁移到 PSA 标签 |
| batch/v1beta1 CronJob | batch/v1 | v1.25 | 更新 apiVersion |
| discovery.k8s.io/v1beta1 | discovery.k8s.io/v1 | v1.25 | 更新 apiVersion |
| flowcontrol/v1beta1 | flowcontrol/v1 | v1.29 | 更新 apiVersion |

## 版本兼容性矩阵

| K8s 版本 | kubectl 兼容 | etcd 版本 | containerd | CNI 版本 |
|----------|-------------|----------|------------|----------|
| v1.28 | ±1 版本 | 3.5.x | 1.7.x | 1.0 |
| v1.29 | ±1 版本 | 3.5.x | 1.7.x | 1.0 |
| v1.30 | ±1 版本 | 3.5.x | 1.7.x+ | 1.0 |
| v1.31 | ±1 版本 | 3.5.x | 2.0.x | 1.0 |
| v1.32+ | ±1 版本 | 3.5.x+ | 2.0.x | 1.0+ |

## 检查清单

- [ ] 已阅读目标版本 CHANGELOG 和 Breaking Changes
- [ ] 已运行 kubent/pluto 检测弃用 API
- [ ] 已验证插件兼容性 (CNI/CSI/Ingress/Monitoring)
- [ ] 已备份 etcd 数据
- [ ] 已在测试环境验证升级
- [ ] 升级窗口已通知相关团队
- [ ] 回滚方案已准备
- [ ] 监控告警已配置

## Related

- [[21-生态参考/98-merged-indexes/index.md|release-notes-reading-guide]] — 发布说明阅读指南
- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[23-实体/15-参考与索引/kubernetes-changelog.md|kubernetes-changelog]] — Kubernetes 变更日志索引

<!-- risk-assessed -->
