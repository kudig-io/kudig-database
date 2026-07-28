---
title: etcd × NetworkPolicy
summary: etcd × NetworkPolicy：etcd与NetworkPolicy是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
- security
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

# etcd × NetworkPolicy

## 概述
NetworkPolicy 对象存储在 etcd 中，CNI 插件（Calico/Cilium）通过 apiserver watch etcd 中的 NetworkPolicy 变化来更新数据面的过滤规则。etcd 的性能直接影响 NetworkPolicy 的生效速度——在高延迟 etcd 上创建一条 deny-all 策略后，可能需要数秒钟才能在数据面实际阻断流量，这个窗口期可能带来安全风险。

## 技术关联机制

1. **NetworkPolicy 在 etcd 中的存储与传播**：每个 NetworkPolicy 以 `/registry/networkpolicies/<namespace>/<name>` 为 key 存储在 etcd 中。CNI 插件的策略控制器（如 Calico 的 Typha、Cilium 的 cilium-operator）通过 informer watch 这些对象。当 etcd 写入延迟高时，从策略创建到数据面生效的端到端延迟增加。

2. **大规模 NetworkPolicy 的 etcd 存储压力**：零信任网络架构中，每个微服务可能有入站和出站两套 NetworkPolicy。在 100 个微服务的集群中，可能有 200+ NetworkPolicy 对象，加上 Pod label 变更触发的频繁策略评估，对 etcd 的读写产生持续负载。

3. **etcd 故障期间的策略行为**：当 etcd 不可用时，CNI 插件无法获取新的 NetworkPolicy 变更，但已加载到数据面的策略（iptables/eBPF 规则）继续生效。这意味着 etcd 故障不会导致安全策略失效，但无法新增或修改策略。

4. **Calico Typha 与 etcd watch 分发**：在大规模集群中，每个节点的 Calico felix agent 都通过 apiserver watch etcd 中的 NetworkPolicy 和 Pod label 变化。数百个节点的并发 watch 对 etcd/apiserver 造成巨大压力。Calico Typha 作为中间层，由单个 Typha 实例 watch etcd，再将变更 fan-out 到所有 felix agent，大幅减轻 etcd 负载。

## 实践场景

- **安全策略变更生效延迟**：创建 deny-all NetworkPolicy 后，etcd 延迟导致策略在数秒后才在数据面生效，这个窗口期流量未被过滤
- **大规模策略管理的 etcd 压力**：平台级集群中数百个 NetworkPolicy 的频繁变更对 etcd 产生持续写入压力
- **etcd 恢复后策略同步**：从 etcd 快照恢复后，CNI 插件重新 list 所有 NetworkPolicy 并重建数据面规则，可能产生瞬时高负载
- **多租户隔离**：每个租户 Namespace 的 NetworkPolicy 基线（deny-all + 逐步放行）存储在 etcd 中，租户数量增长导致策略对象膨胀

## 常见问题

### 问题1：创建 NetworkPolicy 后流量未立即被过滤
**症状**：创建了 deny-all 策略但部分 Pod 仍可通信
**根因**：etcd 写入延迟导致 CNI 插件 watch 事件延迟；或 CNI 插件本身处理延迟
**修复**：检查 etcd 性能；确认 CNI 插件 Pod 正常运行；等待策略传播完成（通常 <5s）

### 问题2：大量 NetworkPolicy 导致 CNI 插件 watch 滞后
**症状**：新增 Pod 后 NetworkPolicy 规则未及时应用到该 Pod
**根因**：大规模集群中 CNI 插件的 watch 缓冲区溢出或 etcd relist 风暴导致处理滞后
**修复**：使用 Calico Typha 减少直接 etcd watch 连接数；优化 NetworkPolicy 规则合并减少对象数

### 问题3：etcd 存储满导致 NetworkPolicy 创建失败
**症状**：`kubectl apply` NetworkPolicy 报 `etcdserver: mvcc: database space exceeded`
**根因**：etcd 存储空间达到 2GB quota，无法写入新对象
**修复**：执行 etcd compaction 清理历史 revision；增加 etcd quota；清理不需要的资源

## 关键命令

```bash
# 🟢 查看 NetworkPolicy 数量
kubectl get networkpolicy -A | wc -l

# 🟢 检查 etcd 存储空间使用
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> endpoint status --write-out=table

# 🟢 查看 NetworkPolicy 对象在 etcd 中的存储
kubectl get --raw /metrics | grep apiserver_storage_objects | grep networkpolicy

# 🟢 检查 CNI 插件状态
kubectl get pods -n kube-system | grep -E "calico|cilium"

# 🟢 执行 etcd compaction（释放空间）
ETCDCTL_API=3 etcdctl --endpoints=<endpoints> compact $(ETCDCTL_API=3 etcdctl --endpoints=<endpoints> endpoint status -w json | jq '.[0].Status.header.revision')
```

## 权衡取舍

| 维度 | etcd 倾向 | NetworkPolicy 倾向 | 权衡点 |
|------|----------|-------------------|--------|
| 策略数量 | 少策略减少存储和 watch 压力 | 多策略精细控制 | etcd 负载 vs 安全粒度 |
| 变更频率 | 低频变更减少写入 | 高频变更快速响应 | etcd 压力 vs 安全时效 |
| CNI 架构 | 直连 etcd 简化架构 | Typha 中间层减轻负载 | 架构复杂 vs etcd 性能 |
| 故障影响 | etcd 故障不影已有策略 | 无法新增/修改策略 | 安全持续性 vs 管理能力 |

## 最佳实践
1. 使用 Calico Typha 或 Cilium 的 kvstoreproxy 减少 CNI 插件对 etcd 的直接 watch 压力
2. 合并 NetworkPolicy 规则，减少对象数量（如使用 `namespaceSelector` 替代多个 `podSelector`）
3. 监控 etcd 存储使用量，定期执行 compaction 防止 quota 耗尽
4. 将 NetworkPolicy 纳入 GitOps 管理，控制变更频率并确保策略可追溯

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[etcd]]
- [[networkpolicy|NetworkPolicy]]
## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[23-实体/08-交付与制品/argo.md|Argo Workflows]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[22-概念/11-交叉分析/etcd-×-PVC.md|etcd-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[22-概念/11-交叉分析/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]
- [[22-概念/11-交叉分析/apiserver-×-RBAC.md|apiserver-×-RBAC]]
- [[22-概念/11-交叉分析/apiserver-×-NetworkPolicy.md|apiserver-×-NetworkPolicy]]


<!-- risk-assessed -->
