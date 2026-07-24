---
title: StatefulSet × NetworkPolicy
summary: StatefulSet × NetworkPolicy：StatefulSet与NetworkPolicy是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
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

# StatefulSet × NetworkPolicy

## 概述
StatefulSet 管理有状态应用（如数据库集群），其 Pod 拥有稳定的网络标识和严格的通信需求（如主从复制、raft 选举）。NetworkPolicy 为 StatefulSet Pod 提供网络层隔离，限制只有合法的客户端和集群内部成员可以通信。与 Deployment 不同，StatefulSet 的 Pod 需要逐副本的网络策略——如允许 sts-0 作为主节点接受写请求，允许 sts-1/sts-2 作为从节点发起复制请求。

## 技术关联机制

1. **StatefulSet Pod 的 label 匹配**：NetworkPolicy 的 `podSelector` 通过 label 选中 StatefulSet Pod。StatefulSet 自动为每个 Pod 添加 `statefulset.kubernetes.io/pod-name: <sts-name>-<ordinal>` 标签。利用这个标签，NetworkPolicy 可以精确匹配特定序号的 Pod，为不同副本设置不同网络策略。

2. **数据库集群的网络隔离模式**：以 MySQL 主从集群为例：
   - **主节点（sts-0）**：NetworkPolicy 允许应用层 Pod 的入站写请求（3306 端口）；允许从节点的入站复制请求
   - **从节点（sts-1, sts-2）**：NetworkPolicy 允许应用层 Pod 的入站读请求；允许到主节点的出站复制请求
   - **所有节点**：允许互相之间的心跳检测端口通信

3. **StatefulSet 扩缩容与 NetworkPolicy**：当 StatefulSet 扩容创建新 Pod（如 sts-3）时，新 Pod 自动继承 Pod template labels 并被现有 NetworkPolicy 匹配。CNI 插件为新 Pod 应用策略规则。但如果策略中使用 `podSelector` 精确匹配特定序号的 Pod（如 `sts-0`），新 Pod（`sts-3`）不会被匹配——需要确保策略覆盖所有副本。

4. **Pod-to-Pod 通信的 NetworkPolicy**：有状态应用（如 etcd、Kafka、ZooKeeper）的 Pod 之间需要频繁通信。如果 Namespace 中有 deny-all 默认策略，必须在 NetworkPolicy 中显式放行同 StatefulSet Pod 之间的通信端口。

## 实践场景

- **数据库集群安全隔离**：MySQL/PostgreSQL StatefulSet 通过 NetworkPolicy 仅允许应用层访问数据库端口，阻断非法横向移动
- **消息队列内部通信**：Kafka Broker StatefulSet 的 NetworkPolicy 放行 Broker 间 9092/9093 端口通信，同时仅允许应用 Pod 的生产/消费请求
- **etcd 集群选举隔离**：etcd StatefulSet 的 NetworkPolicy 放行成员间 2380 端口（raft 通信）和客户端 2379 端口
- **读写分离网络策略**：主库 Pod（sts-0）仅接受写请求客户端，从库 Pod（sts-1+）仅接受读请求客户端

## 常见问题

### 问题1：StatefulSet Pod 间通信被 NetworkPolicy 阻断
**症状**：数据库集群的从节点无法连接主节点，导致复制中断
**根因**：deny-all NetworkPolicy 阻断了 Pod 间的复制端口通信
**修复**：在 NetworkPolicy 中添加放行同 StatefulSet Pod 间通信端口的规则（使用 `podSelector` 匹配同 app 标签）

### 问题2：StatefulSet 扩容后新 Pod 无法被客户端访问
**症状**：新扩容的副本（sts-3）无法接收客户端请求
**根因**：NetworkPolicy 使用精确序号匹配（如仅匹配 sts-0 到 sts-2），未覆盖新副本
**修复**：使用通用 app 标签（`app: xxx`）而非序号标签作为 NetworkPolicy podSelector

### 问题3：NetworkPolicy 阻断了数据库的 readinessProbe
**症状**：StatefulSet Pod 的 readinessProbe 失败，Pod 不 Ready
**根因**：kubelet 从节点 IP 发起 probe 请求被 NetworkPolicy 阻断
**修复**：在 NetworkPolicy 中放行节点 IP CIDR 到 Pod probe 端口的入站请求

## 关键命令

```bash
# 🟢 查看 StatefulSet Pod 的 labels（NetworkPolicy 匹配依据）
kubectl get pods -l app=<name> -n <ns> --show-labels

# 🟢 查看 Namespace 中的 NetworkPolicy
kubectl get networkpolicy -n <ns>

# 🟢 测试 StatefulSet Pod 间网络连通性
kubectl exec -it <sts-name>-0 -n <ns> -- curl -s -o /dev/null -w "%{http_code}" http://<sts-name>-1.<svc>:<port>

# 🟢 查看特定 Pod 被 NetworkPolicy 选中
kubectl get networkpolicy -n <ns> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podSelector}{"\n"}{end}'

# 🟡 创建 StatefulSet 内部通信放行策略
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: <sts-name>-internal
  namespace: <ns>
spec:
  podSelector:
    matchLabels:
      app: <name>
  policyTypes: ["Ingress"]
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: <name>
EOF
```

## 权衡取舍

| 维度 | StatefulSet 倾向 | NetworkPolicy 倾向 | 权衡点 |
|------|-----------------|-------------------|--------|
| 副本间通信 | 自由通信简化集群管理 | 严格隔离提升安全 | 管理简单 vs 安全粒度 |
| 扩缩容兼容 | 通用标签覆盖所有副本 | 精确匹配特定副本 | 通用性 vs 精确控制 |
| Probe 放行 | 严格策略阻断 probe | 放行节点 IP 暴露端口 | 安全性 vs 健康检查 |
| 读写分离 | 应用层控制路由 | 网络层隔离读写 | 灵活性 vs 安全隔离 |

## 最佳实践
1. 为 StatefulSet Pod 间的集群内部通信（复制/选举/心跳端口）创建 NetworkPolicy 放行规则
2. 使用通用的 app 标签（而非序号标签）作为 NetworkPolicy podSelector，确保扩缩容后新 Pod 自动受策略保护
3. 在 NetworkPolicy 中放行 kubelet 的 probe 请求（节点 IP CIDR 到 probe 端口）
4. 为数据库 StatefulSet 创建严格的入站策略，仅允许已知应用层 Pod 和集群内部 Pod 访问

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[StatefulSet]]
- [[NetworkPolicy]]
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/apiserver-×-RBAC.md|apiserver-×-RBAC]]
- [[概念/apiserver-×-NetworkPolicy.md|apiserver-×-NetworkPolicy]]


<!-- risk-assessed -->
