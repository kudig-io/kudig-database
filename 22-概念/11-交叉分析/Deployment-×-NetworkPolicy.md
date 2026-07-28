---
title: Deployment × NetworkPolicy
summary: Deployment × NetworkPolicy：Deployment与NetworkPolicy是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- workloads
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

# Deployment × NetworkPolicy

## 概述
Deployment 的 Pod 通过 labels 被 NetworkPolicy 选中并应用网络访问控制。NetworkPolicy 使用 `podSelector` 匹配 Deployment 管理的 Pod，定义入站（Ingress）和出站（Egress）规则。在零信任网络架构中，每个 Deployment 对应一组 NetworkPolicy——仅允许上游 Service 的流量入站，仅允许到下游 Service 和 DNS 的流量出站。这种"Deployment + NetworkPolicy 配对"模式是微服务网络安全的基础。

## 技术关联机制

1. **label 匹配机制**：NetworkPolicy 的 `podSelector` 通过 label 匹配目标 Pod。Deployment 的 `spec.template.metadata.labels` 定义了 Pod 的 labels。因此 NetworkPolicy 实际匹配的是 Deployment 的 Pod template labels，而非 Deployment 对象本身。如果 Deployment 的 labels 被 NetworkPolicy 选中，该 Deployment 管理的所有 Pod 都受策略约束。

2. **滚动更新期间的策略连续性**：Deployment 滚动更新创建的新 Pod 会自动继承 `spec.template.metadata.labels`。只要新 Pod 的 labels 与 NetworkPolicy 的 podSelector 匹配，策略立即对新 Pod 生效。这意味着滚动更新不会产生"未受保护的窗口期"——新 Pod 从创建起就受 NetworkPolicy 约束。

3. **Namespace 隔离与跨 Namespace 通信**：如果多个 Deployment 分布在不同 Namespace，NetworkPolicy 需要使用 `namespaceSelector` 来允许跨 Namespace 通信。例如，前端 Deployment（namespace: frontend）需要访问后端 Deployment（namespace: backend）——backend Namespace 中的 NetworkPolicy 需要允许来自 frontend Namespace 的入站流量。

4. **NetworkPolicy 与 Deployment 扩缩容**：当 Deployment 扩容（新 Pod 创建）或缩容（旧 Pod 删除）时，CNI 插件自动为新 Pod 应用 NetworkPolicy、为删除的 Pod 移除规则。这种动态绑定是自动的——但大规模扩缩容时，CNI 插件需要快速处理大量规则更新。

## 实践场景

- **微服务零信任网络**：为每个微服务 Deployment 创建 NetworkPolicy，仅允许已知上游的入站流量和到已知下游的出站流量
- **数据库访问控制**：数据库 Deployment 的 NetworkPolicy 仅允许应用层 Deployment 的 Pod 访问数据库端口
- **多环境隔离**：dev/staging/prod 环境的 Deployment 在不同 Namespace，NetworkPolicy 阻止跨环境通信
- **Deny-All 基线**：为新 Namespace 自动创建 deny-all NetworkPolicy，然后为每个 Deployment 逐步放行必要通信

## 常见问题

### 问题1：Deployment Pod 间通信被 NetworkPolicy 意外阻断
**症状**：创建 NetworkPolicy 后 Deployment 的 Pod 无法与其他 Pod 通信
**根因**：NetworkPolicy 的 podSelector/namespaceSelector 过于严格，未放行必要的通信路径
**修复**：`kubectl exec` 进入 Pod 测试网络连通性；检查 NetworkPolicy 规则；务必放行 kube-dns（53 端口）

### 问题2：滚动更新后新 Pod 无法被访问
**症状**：Deployment 滚动更新后新版本 Pod 无法接收流量
**根因**：新 Pod 的 labels 与旧 Pod 略有不同（如新增了 version 标签），NetworkPolicy 未匹配新 labels
**修复**：确保 NetworkPolicy 的 podSelector 使用稳定的通用标签（如 `app: xxx`）而非易变标签（如 `version: v2`）

### 问题3：NetworkPolicy 阻断了 readinessProbe/livenessProbe
**症状**：Deployment Pod 的 probe 检查失败导致 Pod 不 Ready
**根因**：kubelet 的 probe 请求被 NetworkPolicy 阻断（kubelet 从节点 IP 发起 probe，可能不在 NetworkPolicy 允许列表中）
**修复**：在 NetworkPolicy 中放行节点 IP 到 Pod 的 probe 端口；或使用 `ingress.from.ipBlock` 放行节点 CIDR

## 关键命令

```bash
# 🟢 查看 Deployment Pod 的 labels（被 NetworkPolicy 匹配）
kubectl get pods -l app=<name> -n <ns> --show-labels

# 🟢 查看 Namespace 中的 NetworkPolicy
kubectl get networkpolicy -n <ns>

# 🟢 查看 NetworkPolicy 匹配的 Pod
kubectl get pods -n <ns> --selector='<networkpolicy-podSelector>'

# 🟢 测试 Pod 间网络连通性
kubectl exec -it <pod-a> -n <ns> -- curl -s -o /dev/null -w "%{http_code}" http://<pod-b>:<port>

# 🟡 创建 Deployment 的默认 NetworkPolicy（仅允许同 Namespace 流量）
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: <name>-policy
  namespace: <ns>
spec:
  podSelector:
    matchLabels:
      app: <name>
  policyTypes: ["Ingress"]
  ingress:
  - from:
    - podSelector: {}
    - namespaceSelector:
        matchLabels:
          name: monitoring
EOF
```

## 权衡取舍

| 维度 | Deployment 倾向 | NetworkPolicy 倾向 | 权衡点 |
|------|----------------|-------------------|--------|
| 标签管理 | 易变标签支持版本管理 | 稳定标签简化策略匹配 | 灵活性 vs 策略稳定性 |
| 网络开放 | 开放通信简化开发 | 严格隔离提升安全 | 开发效率 vs 安全性 |
| 规则复杂度 | 少规则简化部署 | 多规则精细控制 | 运维简单 vs 安全粒度 |
| 扩缩容影响 | 快速扩缩无需关注策略 | CNI 需快速同步规则 | 扩缩速度 vs 策略同步 |

## 最佳实践
1. 为每个 Deployment 使用稳定的通用标签（如 `app: xxx`）作为 NetworkPolicy 的匹配依据，避免使用易变标签
2. 为每个生产 Deployment 创建对应的 NetworkPolicy，遵循"默认拒绝，按需放行"原则
3. 务必在 NetworkPolicy 中放行 kube-dns 53 端口和节点 IP 的 probe 请求
4. 将 NetworkPolicy 纳入 GitOps 管理，与 Deployment 配置在同一 PR 中变更

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[deployment|Deployment]]
- [[networkpolicy|NetworkPolicy]]
## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[23-实体/08-交付与制品/argo.md|Argo Workflows]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[22-概念/11-交叉分析/etcd-×-PVC.md|etcd-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[22-概念/11-交叉分析/apiserver-×-RBAC.md|apiserver-×-RBAC]]
- [[22-概念/11-交叉分析/apiserver-×-NetworkPolicy.md|apiserver-×-NetworkPolicy]]


<!-- risk-assessed -->
