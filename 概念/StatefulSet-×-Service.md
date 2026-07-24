---
title: StatefulSet × Service
summary: StatefulSet × Service：StatefulSet与Service是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- workloads
- networking
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
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# StatefulSet × Service

## 概述
StatefulSet 必须关联一个 Headless Service（`clusterIP: None`）来为每个 Pod 提供稳定的 DNS 名称。与 Deployment 使用的 ClusterIP Service（负载均衡到所有 Pod）不同，Headless Service 直接返回每个 Pod 的 IP 地址，客户端通过 `<pod-name>.<service-name>.<namespace>.svc.cluster.local` 精确寻址特定副本。这种稳定的网络标识是有状态应用（如数据库主从集群、分布式共识系统）正常运行的必要条件。

## 技术关联机制

1. **Headless Service 的 DNS 行为**：当 Service 的 `clusterIP` 设为 `None` 时，CoreDNS 不返回单个虚拟 IP，而是返回所有匹配 Pod 的 A 记录（每个 Pod 一个 IP）。更关键的是，CoreDNS 为每个 Pod 创建独立的 A 记录：`<sts-name>-0.<headless-svc>.<ns>.svc.cluster.local` → Pod-0 的 IP。Pod 重建后 IP 变化但 DNS 名称不变——CoreDNS 自动更新 A 记录指向新 IP。

2. **StatefulSet 对 Service 的依赖**：StatefulSet 的 `spec.serviceName` 必须指向一个已存在的 Headless Service。这个 Service 为 StatefulSet 的 Pod 提供稳定的网络标识。StatefulSet Controller 在创建 Pod 时依赖该 Service 的 DNS 名称来保证 Pod 的有序启动和网络可达性。如果 `serviceName` 指向的 Service 不存在或不是 Headless 类型，StatefulSet 仍能创建 Pod 但网络标识不稳定。

3. **Headless Service + ClusterIP Service 并存**：生产环境中 StatefulSet 通常同时关联两个 Service：
   - **Headless Service**：用于 Pod 间直接通信（如数据库主从复制、Kafka Broker 间通信）。客户端通过 Pod DNS 名称直接连接特定副本。
   - **ClusterIP Service**：用于客户端负载均衡访问（如应用层连接数据库集群的读请求）。Service 自动将请求负载均衡到所有 Ready 的 Pod。

4. **发布期间的 Endpoints 行为**：StatefulSet 逆序滚动更新时，Pod 被逐个重建。Headless Service 的 DNS 记录随 Pod 重建实时更新（旧 IP 移除，新 IP 加入）。ClusterIP Service 的 Endpoints 也同步更新。但由于更新是串行的，更新期间 Endpoints 中可能同时包含新旧版本的 Pod。

## 实践场景

- **数据库主从集群**：MySQL 主从通过 Headless Service 实现主节点寻址（`mysql-0.mysql-h.ns.svc.cluster.local`），应用直接连主节点写、连从节点读
- **Kafka 集群**：Kafka Broker 通过 Headless Service 互相发现，客户端 bootstrap 通过 ClusterIP Service 负载均衡
- **etcd 集群**：etcd 成员通过 `<etcd-N>.<svc>` 互相寻址，实现稳定的成员发现和 Raft 选举
- **读写分离 Service**：创建指向 sts-0 的独立 Service（selector 含 pod-name 标签）作为写入口，ClusterIP Service 负载均衡所有副本作为读入口

## 常见问题

### 问题1：StatefulSet Pod 的 DNS 名称无法解析
**症状**：`nslookup <sts-name>-0.<svc>.<ns>.svc.cluster.local` 返回 NXDOMAIN
**根因**：Headless Service 未创建或 `clusterIP` 不为 None；或 CoreDNS 异常
**修复**：确认 `spec.serviceName` 指向的 Service 存在且 `clusterIP: None`；检查 CoreDNS Pod 状态

### 问题2：Headless Service DNS 返回的 IP 列表不完整
**症状**：Headless Service DNS 查询只返回部分 Pod IP
**根因**：部分 Pod 未 Ready（Headless Service 默认只返回 Ready Pod 的 A 记录）；或 CoreDNS 的 cache 过期
**修复**：检查所有 Pod 的 Ready 状态；确认 `publishNotReadyAddresses` 配置（如需要在 Pod 未 Ready 时也返回 DNS）

### 问题3：StatefulSet 的 serviceName 配置错误
**症状**：StatefulSet Pod 创建成功但 DNS 名称不工作
**根因**：`spec.serviceName` 指向的 Service 不存在或不是 Headless 类型（clusterIP 不为 None）
**修复**：创建匹配的 Headless Service（`clusterIP: None`）；确保 serviceName 与 Service 名称完全一致

## 关键命令

```bash
# 🟢 查看 StatefulSet 和关联的 Service
kubectl get sts,svc -l app=<name> -n <ns>

# 🟢 确认 Service 是否为 Headless
kubectl get svc <name> -n <ns> -o jsonpath='{.spec.clusterIP}'

# 🟢 查看 StatefulSet 的 serviceName
kubectl get sts <name> -n <ns> -o jsonpath='{.spec.serviceName}'

# 🟢 测试 Pod 的稳定 DNS 解析
kubectl run dns-test --image=busybox --rm -it --restart=Never -- nslookup <sts-name>-0.<svc>.<ns>.svc.cluster.local

# 🟢 查看 Headless Service 的 Endpoints
kubectl get endpoints <headless-svc> -n <ns>

# 🟡 创建 Headless Service
kubectl apply -f - <<EOF
apiVersion: v1
kind: Service
metadata:
  name: <sts-name>-headless
  namespace: <ns>
spec:
  clusterIP: None
  selector:
    app: <name>
  ports:
  - port: <port>
    name: <port-name>
EOF
```

## 权衡取舍

| 维度 | StatefulSet 倾向 | Service 倾向 | 权衡点 |
|------|-----------------|-------------|--------|
| Service 类型 | Headless 提供稳定标识 | ClusterIP 负载均衡 | 标识稳定 vs 负载分散 |
| DNS 返回 | 逐 Pod 返回精确寻址 | 返回虚 IP 自动负载均衡 | 精确控制 vs 自动分发 |
| Ready 依赖 | 需 Ready 才有 DNS | 非 Ready Pod 不在 Endpoints | 就绪检测 vs 可达性 |
| 读写分离 | 逐副本 Service 精确路由 | ClusterIP 自动负载均衡 | 精确控制 vs 配置简单 |

## 最佳实践
1. 为每个 StatefulSet 创建 Headless Service（`clusterIP: None`）提供稳定的 Pod DNS 标识
2. 同时创建 ClusterIP Service 用于客户端负载均衡访问（不需要逐副本寻址的场景）
3. 为需要读写分离的场景创建独立 Service（selector 含 `statefulset.kubernetes.io/pod-name` 标签）指向特定副本
4. 在 Pod 未 Ready 但需要 DNS 可达时（如集群初始化阶段）设置 `publishNotReadyAddresses: true`

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[StatefulSet]]
- [[Service]]
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/apiserver-×-Service.md|apiserver-×-Service]]
- [[概念/Deployment-×-Service.md|Deployment-×-Service]]


<!-- risk-assessed -->
