---
title: etcd × 蓝绿发布
summary: etcd × 蓝绿发布：etcd与蓝绿发布是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
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
- target: '[[23-实体/08-交付与制品/helm.md]]'
  type: uses
- target: '[[23-实体/07-可观测性/prometheus.md]]'
  type: uses
- target: '[[23-实体/08-交付与制品/argocd.md]]'
  type: related_to
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# etcd × 蓝绿发布

## 概述
蓝绿发布涉及两套完整的 Deployment + Service 资源同时存储在 etcd 中。流量切换（修改 Service selector）本质上是对 etcd 中 Service 对象的一次 PATCH 写入。etcd 的性能决定了流量切换的原子性和延迟——如果 etcd 延迟高，Service selector 变更传播到 Endpoints Controller 和 kube-proxy 的时间增加，蓝绿切换的"瞬间"特性被削弱。

## 技术关联机制

1. **蓝绿发布的 etcd 写入序列**：绿色环境部署 → 创建绿色 Deployment（写入 etcd）→ 创建绿色 ReplicaSet 和 Pod（批量写入 etcd）→ 等待 Pod Ready（轮询 etcd 中 Pod status）→ 修改 Service selector（PATCH Service 对象，一次 etcd 写入）→ Endpoints Controller watch 到 selector 变更（从 etcd 读取新 selector 的 Pod）→ 更新 Endpoints（写入 etcd）→ kube-proxy watch 到 Endpoints 变更 → 更新 iptables/IPVS 规则。

2. **双环境对 etcd 的存储影响**：蓝绿发布期间，蓝色和绿色两套环境的所有资源（Deployment、ReplicaSet、Pod、Service、ConfigMap）同时存在于 etcd 中，占用双倍存储。大规模应用（如 100 副本）的蓝绿发布意味着 etcd 中临时增加数百个对象。对于接近 etcd 存储配额的集群，这可能触发 `database space exceeded` 错误。

3. **etcd 故障对蓝绿发布的影响**：如果 etcd 故障发生在绿色环境部署期间，蓝色环境仍在运行但绿色环境创建未完成——此时无法继续部署也无法通过 kubectl 清理。如果 etcd 故障发生在流量切换后，绿色环境已接管流量但无法回滚（Service selector 无法修改）。这是蓝绿发布中 etcd 故障的最大风险。

4. **回滚的 etcd 操作**：蓝绿回滚只需修改 Service selector 指回蓝色——一次 etcd PATCH 操作。但前提是蓝色环境资源仍在 etcd 中且 Pod 仍在运行。如果蓝色环境已被清理（delete Deployment），回滚需要重新部署蓝色环境，不再是"瞬时切换"。

## 实践场景

- **大规模蓝绿部署的 etcd 写入压力**：部署 100 副本绿色环境时，批量创建 Pod 对 etcd 造成瞬时写入压力
- **流量切换延迟**：etcd 延迟导致 Service selector 变更传播到 kube-proxy 的时间增加，蓝绿切换不再是瞬时
- **etcd 故障期间的蓝绿发布中断**：etcd 不可用时无法创建绿色环境或切换流量，已运行的蓝色环境不受影响（数据面独立）
- **存储配额风险**：双环境部署期间 etcd 存储接近 2GB quota，需要清理旧版本资源释放空间

## 常见问题

### 问题1：蓝绿切换后流量切换延迟
**症状**：修改 Service selector 后数秒内仍有流量到达旧版本
**根因**：etcd 写入延迟导致 selector 变更 → Endpoints 更新 → kube-proxy 规则更新链路变慢
**修复**：检查 etcd 性能；等待 10-30 秒让 iptables 规则完全收敛

### 问题2：etcd 存储配额不足无法部署绿色环境
**症状**：创建绿色 Deployment 时报 `etcdserver: mvcc: database space exceeded`
**根因**：双环境部署增加大量对象，etcd 数据库达到 2GB quota
**修复**：执行 etcd compaction 清理空间；先清理历史 ReplicaSet 释放存储；增加 etcd quota

### 问题3：etcd 故障导致无法回滚
**症状**：绿色环境异常但 etcd 不可用，无法将 Service selector 切回蓝色
**根因**：etcd 故障导致所有 API 操作失败，包括 Service selector 修改
**修复**：优先恢复 etcd；在恢复期间绿色环境 Pod 继续运行（数据面不受 etcd 影响）

## 关键命令

```bash
# 🟢 查看蓝绿两套环境的资源数量
kubectl get deployment,rs,pods -l app=<name> -n <ns>

# 🟢 检查 etcd 存储空间（双环境部署期间监控）
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> endpoint status --write-out=table

# 🟢 查看当前 Service selector
kubectl get svc <name> -n <ns> -o jsonpath='{.spec.selector}'

# 🟢 检查 etcd 性能（影响流量切换速度）
kubectl get --raw /metrics | grep etcd_request_duration_seconds

# 🟡 流量切换（蓝→绿）
kubectl patch svc <name> -n <ns> -p '{"spec":{"selector":{"version":"green"}}}'

# 🟡 回滚（绿→蓝）
kubectl patch svc <name> -n <ns> -p '{"spec":{"selector":{"version":"blue"}}}'
```

## 权衡取舍

| 维度 | etcd 倾向 | 蓝绿发布 倾向 | 权衡点 |
|------|----------|-------------|--------|
| 双环境存储 | 单环境减少 etcd 负载 | 双环境支持快速切换 | 存储成本 vs 切换能力 |
| 切换速度 | 低延迟 etcd 支持瞬时切换 | 延迟增加削弱蓝绿优势 | etcd 性能 vs 发布体验 |
| 回滚依赖 | etcd 可用时回滚即时 | etcd 故障时无法回滚 | etcd 可用性 vs 回滚能力 |
| 资源清理 | 及时清理减少 etcd 存储 | 保留旧版本支持回滚 | 存储效率 vs 安全保障 |

## 最佳实践
1. 蓝绿部署前检查 etcd 存储空间（确保 < 70% 使用率），为双环境预留存储空间
2. 监控 etcd 性能，确保 Service selector 变更在亚秒级完成，保障流量切换的瞬时性
3. 蓝绿切换后保留蓝色环境至少 1-24 小时（视业务风险），确认稳定后清理释放 etcd 存储
4. 制定 etcd 故障期间的应急方案：蓝色环境数据面独立于 etcd 运行，可暂时维持服务直到 etcd 恢复

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[etcd]]
- 蓝绿发布
## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[23-实体/08-交付与制品/argo.md|Argo Workflows]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[22-概念/11-交叉分析/etcd-×-PVC.md|etcd-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[22-概念/11-交叉分析/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]


<!-- risk-assessed -->
