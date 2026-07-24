---
title: apiserver × Service
summary: apiserver × Service：apiserver与Service是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
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

# apiserver × Service

## 概述
Service 是 `v1` 核心API组的资源，通过 apiserver 定义稳定的虚拟 IP（ClusterIP）和流量转发规则。Service Controller 和 kube-proxy 通过 watch apiserver 上的 Service 和 Endpoints/EndpointSlices 资源来维护数据面的路由规则。apiserver 是 Service 声明面的唯一入口——所有 Service 类型变更（ClusterIP/NodePort/LoadBalancer）、端口映射、selector 更新都通过 apiserver 处理，然后由下游控制器同步到数据面。

## 技术关联机制

1. **Service 生命周期与 Endpoints 联动**：当用户通过 apiserver 创建 Service 时，apiserver 分配 ClusterIP（从 `serviceCIDR` 中分配），并将 Service 对象持久化到 etcd。Endpoints Controller（或 EndpointSlice Controller）通过 informer watch 到新 Service 后，根据 Service 的 `selector` 查找匹配的 Pod，将符合健康条件的 Pod IP+TargetPort 组装成 Endpoints/EndpointSlices 对象写回 apiserver。kube-proxy 通过 watch Endpoints/EndpointSlices 在各节点上更新 iptables/IPVS 规则。

2. **ClusterIP 分配机制**：apiserver 在创建 Service 时从配置的 `serviceCIDR` 范围内分配 ClusterIP。如果指定了固定 ClusterIP 且该 IP 已被占用，apiserver 返回 `ALLOCATED` 错误。在生产环境中，Service 数量增长可能导致 IP 池耗尽——这是一个集群级硬限制。

3. **LoadBalancer 类型的外部集成**：创建 `type: LoadBalancer` 的 Service 时，Service Controller（通常由 Cloud Controller Manager 运行）通过 cloud-provider API 创建外部负载均衡器（如 AWS ELB、GCP GLB），并将外部 IP 回写到 Service 的 `status.loadBalancer.ingress` 字段。这个回写操作依赖 apiserver 可用，如果 apiserver 异常，外部 LB 已创建但 IP 无法回写，用户无法通过 `kubectl get svc` 获取入口地址。

4. **Service 与 DNS 的联动**：CoreDNS 通过 watch apiserver 上的 Service 资源自动创建内部 DNS 记录（`<service>.<namespace>.svc.cluster.local`）。Service 的创建/删除/变更会触发 CoreDNS 的 DNS 记录更新。如果 apiserver 的 watch 机制异常，DNS 记录可能滞后，导致服务发现失败。

## 实践场景

- **微服务间通信**：通过 ClusterIP Service 为每个微服务提供稳定的虚拟 IP，Pod 重启后 IP 变化不影响调用方
- **外部流量接入**：通过 LoadBalancer Service 或 Ingress+ClusterIP Service 将外部流量引入集群
- **Headless Service 用于 StatefulSet**：创建 `clusterIP: None` 的 Headless Service，CoreDNS 返回各 Pod IP，StatefulSet 通过稳定 DNS 名称寻址
- **蓝绿发布流量切换**：通过修改 Service 的 selector 指向不同版本的 Deployment，实现瞬间的流量切换

## 常见问题

### 问题1：Service 创建成功但没有 Endpoints
**症状**：`kubectl get svc` 显示 Service 存在，但 `kubectl get endpoints` 为空
**根因**：Service 的 `selector` 与 Pod 的 `labels` 不匹配；或 Pod 未通过 readinessProbe
**修复**：检查 Service selector 与 Pod labels 的精确匹配；确认 Pod Ready 状态

### 问题2：LoadBalancer Service 的 External IP 长时间 Pending
**症状**：`kubectl get svc` 的 EXTERNAL-IP 列显示 `<pending>`
**根因**：Cloud Provider 的 LB 配额耗尽；或 Cloud Controller Manager 异常
**修复**：检查 Cloud Provider 控制台 LB 配额；确认 CCM Pod 运行正常；检查 CCM 日志

### 问题3：Service 删除后 ClusterIP 未释放导致冲突
**症状**：创建新 Service 报错 `provided IP is already allocated`
**根因**：Service 删除过程中 etcd 的 IP 分配记录未正确清理
**修复**：确认旧 Service 已完全删除（`kubectl get svc --all-namespaces`）；检查 etcd 中的 Service IP 分配 registry

## 关键命令

```bash
# 🟢 查看 Service 和对应 Endpoints
kubectl get svc <name> -n <ns>
kubectl get endpoints <name> -n <ns>

# 🟢 查看 EndpointSlices（大规模集群推荐）
kubectl get endpointslice -n <ns> | grep <service-name>

# 🟢 查看 Service 详细信息
kubectl describe svc <name> -n <ns>

# 🟢 从 Pod 内部测试 Service 连通性
kubectl exec -it <pod> -n <ns> -- curl -s http://<service-name>.<namespace>:<port>

# 🟢 查看节点上的 iptables/IPVS 规则
kubectl -n kube-system exec <kube-proxy-pod> -- iptables-save | grep <service-name>

# 🟡 修改 Service selector（蓝绿切换）
kubectl patch svc <name> -n <ns> -p '{"spec":{"selector":{"version":"v2"}}}'
```

## 权衡取舍

| 维度 | apiserver 倾向 | Service 倾向 | 权衡点 |
|------|---------------|-------------|--------|
| ClusterIP 管理 | 固定 IP 简化排障 | 动态分配避免冲突 | 可预测性 vs 自动化 |
| Service 类型 | ClusterIP 内部简化 | LoadBalancer 外部暴露 | 复杂度 vs 功能完整性 |
| Endpoints 更新 | 批量更新减少写负载 | 逐 Pod 实时反映健康 | etcd 压力 vs 路由精确性 |
| DNS 联动 | 保守更新减少抖动 | 实时同步快速发现 | 稳定性 vs 服务发现速度 |

## 最佳实践
1. 为所有需要服务发现的 Pod 创建对应的 Service，使用 ClusterIP 类型用于内部通信
2. 生产环境使用 IPVS 模式的 kube-proxy 替代 iptables 模式，提升大规模 Service 的路由性能
3. 为每个 Service 配置 readinessProbe 确保 Endpoints 只包含健康的 Pod
4. 使用 EndpointSlices（Kubernetes 1.21+ 默认）替代 Endpoints 以支持更大规模的端点管理

## 工具推荐
- kubectl：基础诊断
- [[实体/helm.md|Helm]]/Kustomize：配置管理
- [[实体/prometheus.md|Prometheus]]/Grafana：联合监控
- [[实体/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- apiserver
- [[Service]]
## Related

- [[实体/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[实体/argo.md|Argo Workflows]]
- [[故障诊断/技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[概念/etcd-×-PVC.md|etcd-×-PVC]]
- [[概念/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[概念/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]
- [[概念/StatefulSet-×-Service.md|StatefulSet-×-Service]]
- [[概念/Deployment-×-Service.md|Deployment-×-Service]]


<!-- risk-assessed -->
