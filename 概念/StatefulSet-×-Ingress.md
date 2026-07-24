---
title: StatefulSet × Ingress
summary: StatefulSet × Ingress：StatefulSet与Ingress是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- workloads
- networking
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

# StatefulSet × Ingress

## 概述
StatefulSet 管理有状态应用（如数据库、消息队列），每个副本有稳定的 Pod 名称和网络标识。Ingress 通常用于将 HTTP(S) 流量路由到无状态的 Deployment+Service 组合。StatefulSet 与 Ingress 的交集主要出现在需要为每个 StatefulSet 副本暴露独立 HTTP 端点的场景（如 Elasticsearch 节点管理界面、Kafka UI）。由于 Ingress 的 backend 只能指向 Service 而非单个 Pod，为 StatefulSet 的每个副本创建独立 Ingress 规则需要借助 Headless Service + Pod DNS。

## 技术关联机制

1. **Headless Service 作为桥梁**：StatefulSet 必须关联一个 Headless Service（`clusterIP: None`）。CoreDNS 为每个 Pod 创建独立的 A 记录（`<pod-name>.<service-name>.<namespace>.svc.cluster.local`）。Ingress 的 backend 可以指向一个普通 Service（ClusterIP），该 Service 的 selector 匹配 StatefulSet 的 Pod，将流量负载均衡到所有副本。

2. **单副本路由 vs 多副本路由**：
   - **整体路由**：Ingress → Service（ClusterIP）→ StatefulSet 所有副本。适用于 StatefulSet 内部自行处理 leader 选举和请求分发的场景（如 Elasticsearch）。
   - **逐副本路由**：为每个副本创建独立 Service + Ingress 规则。如 `sts-0` 对应 `svc-sts-0` + Ingress host `node0.app.com`。适用于需要直接访问特定副本的场景（如 Kafka Broker 直接连接）。

3. **StatefulSet 滚动更新对 Ingress 的影响**：StatefulSet 逆序滚动更新时，Pod 被逐个重建。Ingress Controller 通过 Endpoints 感知 Pod IP 变化。由于 StatefulSet 更新是串行的（一个 Pod 更新完才更新下一个），更新期间 Endpoints 中可能同时包含新旧版本的 Pod IP——Ingress Controller 会负载均衡到混合版本的后端。

4. **gRPC/TCP 服务与 Ingress**：StatefulSet 管理的服务（如数据库）通常使用 TCP 协议而非 HTTP。标准 Ingress 只支持 HTTP(S)/L7 路由。如果需要为 StatefulSet 暴露 TCP 服务，使用 Ingress Controller 的 TCP services configMap（NGINX Ingress Controller 支持）或 LoadBalancer/NodePort Service。

## 实践场景

- **Elasticsearch 集群**：StatefulSet 管理 ES data node，Ingress 暴露 ES REST API（通过 ClusterIP Service 负载均衡）
- **Kafka Manager UI**：为 Kafka StatefulSet 的管理界面创建 Ingress，通过域名访问集群管理
- **数据库读写分离**：主库 Ingress 指向 sts-0（写），从库 Ingress 指向 sts-1, sts-2（读），通过独立 Service 实现读写分离
- **多租户独立实例**：每个租户一个 StatefulSet 副本，通过独立 Ingress host 路由到对应副本

## 常见问题

### 问题1：Ingress 无法直接路由到 StatefulSet 的特定副本
**症状**：需要访问 StatefulSet 的某个特定 Pod（如 sts-2），但 Ingress 只能负载均衡到所有副本
**根因**：标准 Ingress backend 指向 Service，Service 将流量负载均衡到所有匹配 Pod
**修复**：创建独立的 ClusterIP Service（selector 包含 `statefulset.kubernetes.io/pod-name: <sts-name>-2`）匹配特定 Pod，Ingress backend 指向该 Service

### 问题2：StatefulSet 滚动更新期间 Ingress 后端版本混合
**症状**：滚动更新期间 Ingress 同时路由到新旧版本 Pod
**根因**：StatefulSet 串行更新期间，已更新和未更新的 Pod 同时存在于 Endpoints 中
**修复**：使用 readinessGate 或应用层版本协商确保兼容性；在低峰期快速完成更新

### 问题3：TCP 服务无法通过 Ingress 暴露
**症状**：StatefulSet 管理的 MySQL/TCP 服务无法通过 Ingress 暴露
**根因**：标准 Kubernetes Ingress 只支持 HTTP(S) L7 路由
**修复**：使用 NGINX Ingress Controller 的 TCP services ConfigMap 暴露 TCP 端口；或使用 LoadBalancer/NodePort Service

## 关键命令

```bash
# 🟢 查看 StatefulSet 和关联的 Headless Service
kubectl get sts,svc -l app=<name> -n <ns>

# 🟢 查看 StatefulSet Pod 的稳定 DNS
kubectl run dns-test --image=busybox --rm -it --restart=Never -- nslookup <sts-name>-0.<headless-svc>.<ns>.svc.cluster.local

# 🟢 查看指向 StatefulSet 的 Ingress
kubectl get ingress -n <ns> -o wide

# 🟢 检查 Endpoints（确认 Ingress 后端可达）
kubectl get endpoints <service-name> -n <ns>

# 🟡 创建指向特定 StatefulSet 副本的 Service
kubectl expose pod <sts-name>-0 --name=<svc-name>-0 --port=<port> -n <ns>
```

## 权衡取舍

| 维度 | StatefulSet 倾向 | Ingress 倾向 | 权衡点 |
|------|-----------------|-------------|--------|
| 路由粒度 | 逐副本独立路由 | Service 级负载均衡 | 精确控制 vs 配置简单 |
| 协议支持 | TCP/自定义协议 | HTTP(S) L7 路由 | 有状态服务 vs Web 路由 |
| 滚动更新 | 串行更新数据安全 | 并行后端版本混合 | 数据安全 vs 路由一致性 |
| DNS 稳定性 | Pod DNS 稳定标识 | Service DNS 负载均衡 | 标识稳定 vs 负载分散 |

## 最佳实践
1. 为 StatefulSet 创建 ClusterIP Service（非 Headless）作为 Ingress backend，让 Ingress Controller 自动负载均衡
2. 如果需要路由到特定副本，创建带 `statefulset.kubernetes.io/pod-name` selector 的独立 Service
3. TCP 服务（如数据库）使用 Ingress Controller 的 TCP passthrough 或 LoadBalancer 暴露，不要使用标准 Ingress
4. StatefulSet 滚动更新期间监控 Ingress 的后端版本一致性，确保应用层兼容混合版本

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[StatefulSet]]
- [[Ingress]]
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/apiserver-×-Service.md|apiserver-×-Service]]
- [[概念/StatefulSet-×-Service.md|StatefulSet-×-Service]]


<!-- risk-assessed -->
