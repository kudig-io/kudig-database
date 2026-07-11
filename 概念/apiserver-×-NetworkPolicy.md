---
title: apiserver × NetworkPolicy
summary: apiserver × NetworkPolicy：apiserver与NetworkPolicy是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
- security
tier: core
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# apiserver × NetworkPolicy

## 概述
NetworkPolicy 是 `networking.k8s.io/v1` API 组下的资源，通过 apiserver 声明 Pod 间的网络访问控制规则（L3/L4 层）。apiserver 负责 NetworkPolicy 的 schema 校验和持久化，但实际的流量过滤由 CNI 插件（Calico/Cilium/Weave）在数据平面执行。理解 apiserver（声明面）和 CNI（执行面）的分离对于排查网络隔离故障至关重要。

## 技术关联机制

1. **NetworkPolicy 的声明与执行分离**：当用户通过 apiserver 创建一个 NetworkPolicy 时，apiserver 将其存入 etcd，但不会对任何流量产生影响。真正执行的是 CNI 插件的策略控制器——它通过 watch apiserver 上的 NetworkPolicy 资源，将规则翻译成底层的 iptables 规则、eBPF 程序或 ipset。如果 CNI 插件未安装或异常，NetworkPolicy 就只是一条存储在 etcd 中的声明，不产生任何隔离效果。

2. **默认允许 vs 默认拒绝**：Kubernetes 的 NetworkPolicy 语义是"默认允许"——如果某个 Namespace 中没有任何 NetworkPolicy 选中某个 Pod，该 Pod 的所有入站和出站流量都是允许的。一旦有 NetworkPolicy 选中该 Pod，只有规则明确允许的流量才被放行。这种行为变化是生产环境最常见的网络中断原因之一。

3. **apiserver 自身的 NetworkPolicy 隔离风险**：如果 NetworkPolicy 规则意外选中了 kube-system 中的关键 Pod（如 coredns、kube-proxy），会破坏集群内部通信。更危险的是，如果 NetworkPolicy 隔离了 apiserver 与 etcd 之间的通信（虽然两者通常在同一节点上通过 localhost 通信），会导致整个集群不可用。

4. **RBAC 与 NetworkPolicy 的多层防御**：apiserver 的 RBAC 控制"谁能创建/修改 NetworkPolicy"（声明面权限），NetworkPolicy 本身控制"Pod 之间能否通信"（数据面权限）。生产环境应在这两个层面都实施最小权限原则。

## 实践场景

- **微服务零信任网络**：为每个微服务的 Pod 创建 NetworkPolicy，仅允许来自上游 Service 的流量和到下游 Service 的流量，实现东西向流量隔离
- **数据库访问控制**：通过 NetworkPolicy 限制只有应用 Namespace 的 Pod 可以访问 MySQL/Redis Pod 的端口，阻断非法横向移动
- **合规性隔离**：PCI-DSS/HIPAA 场景中，将处理敏感数据的 Pod 通过 NetworkPolicy 完全隔离，仅允许审计系统和特定应用访问
- **Deny-All 默认策略**：为每个新 Namespace 自动创建 deny-all 的 NetworkPolicy 作为安全基线，强制每个服务显式声明出入站规则

## 常见问题

### 问题1：创建 NetworkPolicy 后服务间通信中断
**症状**：应用 NetworkPolicy 后，Pod 间请求超时或连接拒绝
**根因**：NetworkPolicy 的 `podSelector` 或 `namespaceSelector` 过于严格，未覆盖必要的通信路径（如 DNS、metrics scrape）
**修复**：临时删除 NetworkPolicy 恢复通信，逐步添加规则；务必放行 kube-dns 的 53 端口 UDP/TCP

### 问题2：NetworkPolicy 已创建但流量未被过滤
**症状**：NetworkPolicy 存在但 Pod 仍然可以从外部访问
**根因**：CNI 插件不支持 NetworkPolicy（如 flannel 默认不支持）；或 CNI 策略控制器异常
**修复**：确认使用的 CNI 插件支持 NetworkPolicy（Calico/Cilium/Weave 支持，flannel 需配合 Calico for policy）；检查 CNI Pod 状态

### 问题3：大规模 NetworkPolicy 导致网络性能下降
**症状**：集群中 NetworkPolicy 数量增多后，Pod 间延迟显著上升
**根因**：iptables 模式的 CNI（如 Calico 在 iptables data plane 下）规则数量与 NetworkPolicy 数量成正比，大量规则导致内核 iptables 性能下降
**修复**：迁移到 eBPF data plane（Cilium 或 Calico eBPF 模式）减少规则匹配开销；合并规则减少条目数

## 关键命令

```bash
# 🟢 查看 Namespace 中的所有 NetworkPolicy
kubectl get networkpolicy -n <ns>

# 🟢 查看 NetworkPolicy 详细规则
kubectl describe networkpolicy <name> -n <ns>

# 🟢 查看 Pod 被哪些 NetworkPolicy 选中
kubectl get pod <name> -n <ns> -o jsonpath='{.metadata.labels}'

# 🟢 检查 CNI 插件状态
kubectl get pods -n kube-system | grep -E "calico|cilium|weave"

# 🟡 创建默认 deny-all 策略（需谨慎）
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: <ns>
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
EOF
```

## 权衡取舍

| 维度 | apiserver 倾向 | NetworkPolicy 倾向 | 权衡点 |
|------|---------------|-------------------|--------|
| 规则数量 | 少规则降低 apiserver 存储 | 多规则精细控制 | 管理复杂度 vs 安全粒度 |
| 策略范围 | 全局策略简化管理 | 命名空间级策略隔离控制 | 统一管控 vs 灵活隔离 |
| CNI 依赖 | 声明与执行解耦 | 实际效果依赖 CNI 能力 | 可移植性 vs 执行确定性 |
| 默认行为 | 默认允许简化入门 | 默认拒绝提升安全 | 易用性 vs 安全基线 |

## 最佳实践
1. 为每个生产 Namespace 创建 deny-all 默认策略作为安全基线，然后按需放行
2. 务必在 NetworkPolicy 中放行 DNS（kube-dns 53 端口）和监控（Prometheus）流量
3. 使用 Calico 或 Cilium 的 eBPF data plane 替代 iptables 以提升大规模策略性能
4. 将 NetworkPolicy 纳入 GitOps 管理，所有策略变更通过 PR review 控制风险

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- apiserver
- [[NetworkPolicy]]
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]
- [[概念/apiserver-×-RBAC.md|apiserver-×-RBAC]]
- [[概念/StatefulSet-×-NetworkPolicy.md|StatefulSet-×-NetworkPolicy]]


<!-- risk-assessed -->
