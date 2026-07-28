---
title: etcd × Ingress
summary: etcd × Ingress：etcd与Ingress是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
category: synthesis
tags:
- cross-domain
- cluster
- networking
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

# etcd × Ingress

## 概述
每一个 Ingress 资源（路由规则、TLS 证书引用、后端 Service 映射）都以序列化对象存储在 etcd 中。Ingress Controller 通过 watch etcd 中（经由 apiserver）的 Ingress 资源变化来实时更新代理配置。当 etcd 性能下降时，Ingress 规则的变更（新增路由、修改权重、更新证书）会延迟传播到 Controller，导致流量路由配置滞后于用户期望。

## 技术关联机制

1. **Ingress 对象在 etcd 中的存储**：每个 Ingress 对象以 `/registry/ingresses/<namespace>/<name>` 为 key 存储在 etcd 中。大规模集群中可能有数千个 Ingress 对象，每个对象包含 host 列表、path 规则、TLS 配置和 backend 映射。Ingress Controller（如 NGINX Ingress Controller）通过 informer watch 这些对象，将变更翻译为 nginx.conf 配置。

2. **watch 延迟传播**：当用户修改 Ingress 规则后，apiserver 将变更写入 etcd，etcd 的 watch notification 经由 apiserver 传播到 Ingress Controller 的 informer。如果 etcd 写入延迟高（如磁盘 I/O 瓶颈），从 `kubectl apply` 到 Ingress Controller 感知变更的端到端延迟会从亚秒级增加到数秒。

3. **Ingress status 回写**：Ingress Controller 在处理完路由配置后，将 External IP/域名回写到 Ingress 的 `status.loadBalancer.ingress` 字段。这个回写操作需要 apiserver 向 etcd 发起 PATCH 请求。如果 etcd 性能差，status 更新会延迟，用户通过 `kubectl get ingress` 看到的地址可能过时。

4. **etcd 存储压力与 Ingress 数量**：在多租户平台中，每个租户可能有数十个 Ingress 规则。万级 Ingress 对象加上关联的 Events 和 EndpointSlices，对 etcd 存储和 relist 性能构成挑战。

## 实践场景

- **路由变更延迟**：修改 Ingress 的 canary-weight annotation 后，etcd 延迟导致 NGINX reload 延迟，金丝雀流量切换不及时
- **证书更新传播**：cert-manager 签发新证书后更新 Secret，Ingress Controller watch 到 Secret 变化需要经过 etcd → apiserver → Controller 链路，延迟高时可能出现证书过期窗口
- **大规模 Ingress 管理的 etcd 压力**：平台级集群中数千个 Ingress 对象的频繁变更对 etcd 产生持续写负载
- **etcd 故障期间的路由行为**：etcd 不可用时已有 Ingress 规则仍生效（Controller 缓存了配置），但无法新增或修改路由

## 常见问题

### 问题1：Ingress 规则修改后 Controller 未及时更新
**症状**：修改 Ingress annotation 后路由行为数秒甚至数十秒未变化
**根因**：etcd 写入延迟导致 informer watch 事件延迟传播
**修复**：检查 etcd 磁盘性能；监控 `etcd_request_duration_seconds`；必要时重启 Ingress Controller 强制 relist

### 问题2：Ingress status 未显示 External IP
**症状**：`kubectl get ingress` 的 ADDRESS 列长时间为空
**根因**：etcd 性能问题导致 Ingress Controller 的 status 回写 PATCH 操作超时
**修复**：检查 etcd 健康；确认 Ingress Controller 的 SA 有 update ingress/status 权限

### 问题3：大规模 Ingress 导致 Controller relist 风暴
**症状**：etcd compaction 后 Ingress Controller 触发全量 relist，数千 Ingress 对象的 list 操作对 etcd 造成瞬时高负载
**根因**：Controller 的 informer watch revision 落后于 etcd compaction 阈值
**修复**：调整 etcd auto-compaction-retention；确保 etcd 内存充足以支持 watch cache

## 关键命令

```bash
# 🟢 查看 Ingress 资源
kubectl get ingress -A

# 🟢 检查 Ingress Controller 日志中的处理延迟
kubectl logs -n ingress-nginx <controller-pod> | grep -E "reload|sync"

# 🟢 查看 etcd 性能指标
kubectl get --raw /metrics | grep etcd_request_duration_seconds

# 🟢 查看 Ingress 对象在 etcd 中的数量
kubectl get --raw /metrics | grep apiserver_storage_objects | grep ingress

# 🟢 检查 etcd 存储 status 的写入延迟
kubectl get --raw /metrics | grep etcd_server_slow_apply_total
```

## 权衡取舍

| 维度 | etcd 倾向 | Ingress 倾向 | 权衡点 |
|------|----------|-------------|--------|
| 路由变更频率 | 低频减少写入压力 | 高频快速响应路由需求 | etcd 负载 vs 路由实时性 |
| Controller 缓存 | Controller 缓存减少读压力 | 实时 watch 保证配置最新 | etcd 读负载 vs 配置时效 |
| status 回写 | 延迟回写减少写入 | 实时回写快速反馈 | etcd 写负载 vs 可观测性 |
| 对象数量 | 少 Ingress 减少存储 | 多 Ingress 支撑业务 | 存储成本 vs 功能需求 |

## 最佳实践
1. 监控 etcd 性能指标，在延迟升高时优先排查 Ingress 变更延迟
2. 使用 NGINX Ingress Controller 的动态配置模式减少全量 reload 对 etcd watch 的冲击
3. 将 Ingress 规则纳入 GitOps 管理，控制变更频率避免频繁写入 etcd
4. 大规模集群定期检查 etcd 中 Ingress 对象数量，评估存储和 relist 性能影响

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[etcd]]
- [[ingress|Ingress]]
## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[23-实体/08-交付与制品/argo.md|Argo Workflows]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[22-概念/11-交叉分析/etcd-×-PVC.md|etcd-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[22-概念/11-交叉分析/etcd-×-灾难恢复.md|etcd-×-灾难恢复]]
- [[22-概念/11-交叉分析/apiserver-×-Service.md|apiserver-×-Service]]
- [[22-概念/11-交叉分析/StatefulSet-×-Service.md|StatefulSet-×-Service]]


<!-- risk-assessed -->
