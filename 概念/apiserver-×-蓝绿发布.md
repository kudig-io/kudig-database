---
title: apiserver × 蓝绿发布
summary: apiserver × 蓝绿发布：apiserver与蓝绿发布是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
- release
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
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# apiserver × 蓝绿发布

## 概述
蓝绿发布是一种通过维护两套完整环境（蓝色=当前版本，绿色=新版本）实现零停机切换的发布策略。在 Kubernetes 中，蓝绿发布完全通过 apiserver 上的资源操作实现：创建绿色 Deployment → 等待绿色 Pod 就绪 → 修改 Service selector 将流量切换到绿色 → 确认稳定后删除蓝色 Deployment。apiserver 是这条操作链路的唯一控制入口，Service selector 的原子切换是流量瞬时迁移的关键。

## 技术关联机制

1. **基于 Service Selector 的流量切换**：蓝绿发布的核心是 Service 的 `spec.selector` 字段。蓝色环境运行 `version: blue` 标签的 Pod，Service 的 selector 指向 `version: blue`。绿色环境部署 `version: green` 标签的 Pod 并全部 Ready 后，通过 apiserver 执行 `kubectl patch svc <name> -p '{"spec":{"selector":{"version":"green"}}}'`。这个 PATCH 操作在 apiserver 上是原子的，Endpoints Controller 几乎实时更新 Endpoints，kube-proxy 随即更新 iptables/IPVS 规则，流量在秒级完成切换。

2. **两套 Deployment 并行运行**：蓝色和绿色分别是一个独立的 Deployment（如 `app-blue` 和 `app-green`），各自管理自己的 ReplicaSet 和 Pod。两套 Deployment 同时存在于 apiserver 上，占用双倍计算资源。这是蓝绿发布的主要成本——始终保持 2N 的资源冗余。

3. **回滚的原子性**：蓝绿发布的回滚非常简单——只需将 Service selector 切回 `version: blue`。因为蓝色 Deployment 仍然存在且 Pod 仍在运行（只要资源未回收），流量切换是即时的。这是蓝绿发布相比滚动更新的最大优势：回滚速度等于一次 apiserver PATCH 操作的延迟（毫秒级）。

4. **数据库迁移的挑战**：蓝绿发布在无状态应用上表现完美，但在有状态应用（数据库 schema 变更）场景中需要额外策略。蓝色和绿色 Pod 共享同一 PV/PVC 时，新版本代码可能与旧版本数据不兼容。生产环境通常要求 schema 变更向前兼容（先加列不加约束，新旧版本并行运行后再清理）。

## 实践场景

- **关键业务零停机发布**：金融/电商核心交易系统不允许任何停机窗口，蓝绿发布的秒级切换满足这一需求
- **大版本升级**：应用框架大版本升级（如 Spring Boot 2→3），功能差异大，需要完整环境验证后再切换
- **A/B 测试流量切分**：通过 Service selector 配合权重（或使用 Istio VirtualService）实现 10% 绿色 + 90% 蓝色的流量分配
- **快速回滚能力保障**：发布高风险变更时保留蓝色环境 24 小时，随时可一键回滚

## 常见问题

### 问题1：蓝绿切换后部分请求仍打到旧版本
**症状**：Service selector 已切换为 green，但少量请求仍到达 blue Pod
**根因**：kube-proxy 的 iptables/IPVS 规则更新有延迟；或客户端保持了长连接（keep-alive）
**修复**：等待 30-60 秒让 iptables 规则完全刷新；终止旧版本 Pod 的长连接；使用 Istio 等服务网格获得更精确的流量控制

### 问题2：绿色环境 Pod 无法 Ready 导致切换延迟
**症状**：绿色 Deployment 部署后部分 Pod 持续未 Ready
**根因**：新版本应用配置错误/依赖服务不可达/readinessProbe 配置过严
**修复**：排查 Pod 日志和 Events；修复配置后触发 rollout restart；确认全量 Ready 后再切换 Service

### 问题3：资源不足导致蓝绿无法并行
**症状**：部署绿色 Deployment 时节点资源不足，Pod 处于 Pending
**根因**：蓝绿方案需要 2N 资源，集群容量不足以同时运行两套环境
**修复**：临时扩容节点；或改用滚动更新策略（只需 1.x N 资源）；或缩减蓝色副本数腾出空间

## 关键命令

```bash
# 🟢 查看蓝色和绿色 Deployment
kubectl get deployment -l app=<name> -n <ns>

# 🟢 查看当前 Service selector
kubectl get svc <name> -n <ns> -o jsonpath='{.spec.selector}'

# 🟢 查看 Endpoints 确认流量目标
kubectl get endpoints <name> -n <ns>

# 🟡 部署绿色环境
kubectl apply -f deployment-green.yaml -n <ns>

# 🟡 切换流量到绿色（关键操作）
kubectl patch svc <name> -n <ns> -p '{"spec":{"selector":{"version":"green"}}}'

# 🟡 回滚到蓝色
kubectl patch svc <name> -n <ns> -p '{"spec":{"selector":{"version":"blue"}}}'

# 🔴 确认稳定后清理蓝色环境
kubectl delete deployment <name>-blue -n <ns>
```

## 权衡取舍

| 维度 | apiserver 倾向 | 蓝绿发布 倾向 | 权衡点 |
|------|---------------|-------------|--------|
| 资源占用 | 单环境节省资源 | 双环境快速切换 | 资源成本 vs 切换速度 |
| 切换原子性 | PATCH 操作毫秒级 | Endpoints 秒级收敛 | API 速度 vs 数据面收敛 |
| 回滚速度 | 切回 selector 即时回滚 | 需保留蓝色环境资源 | 资源占用 vs 回滚能力 |
| 数据兼容性 | 无状态应用无冲突 | 有状态应用需兼容方案 | 架构限制 vs 发布灵活 |

## 最佳实践
1. 为蓝色和绿色 Deployment 使用不同的命名（如 `app-blue`/`app-green`）和标签（`version: blue/green`），避免冲突
2. 切换前确保绿色环境所有 Pod 都 Ready 且通过了冒烟测试
3. 切换后保留蓝色环境至少 1-24 小时（视业务风险而定），确认无异常后再清理
4. 对于数据库变更，采用向前兼容的 schema 演进策略，确保蓝绿版本可并行运行

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- apiserver
- 蓝绿发布
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
