---
title: Deployment × Service
summary: Deployment × Service：Deployment与Service是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
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

# Deployment × Service

## 概述
Deployment 管理无状态应用的 Pod 副本，Service 为这些 Pod 提供稳定的网络入口。两者通过 label selector 关联——Service 的 `selector` 匹配 Deployment Pod template 的 `labels`。Endpoints Controller 自动维护 Service 与匹配 Pod 的映射关系。Deployment + Service 是 Kubernetes 中最常见的"无状态应用暴露"模式，理解它们之间的 label 匹配、滚动更新期间的行为和流量路由机制是生产运维的基础。

## 技术关联机制

1. **Label Selector 关联机制**：Service 的 `spec.selector` 是一组 key-value 标签匹配条件。Deployment 的 `spec.template.metadata.labels` 定义了 Pod 的标签。Endpoints Controller 持续扫描所有 Pod，将 label 匹配 Service selector 的 Pod IP + TargetPort 加入 Endpoints 列表。当 Deployment 扩缩容时，新 Pod 自动加入 Endpoints、旧 Pod 自动移除——这个动态关联是 Service 的核心能力。

2. **滚动更新期间的 Endpoints 变更**：Deployment 滚动更新时，旧 Pod 被终止、新 Pod 被创建。Endpoints Controller 通过 watch Pod 的 readiness condition 判断是否将 Pod 加入 Endpoints：
   - 新 Pod 创建 → readinessProbe 通过 → Pod Ready → Endpoints Controller 加入 Endpoints → kube-proxy 更新 iptables → 新 Pod 开始接收流量
   - 旧 Pod 收到 SIGTERM → Endpoints Controller 从 Endpoints 移除 → kube-proxy 更新 iptables → 旧 Pod 不再接收新流量 → Pod 处理完已接收请求后退出

3. **readinessProbe 与 Endpoints 的关键关系**：只有 readinessProbe 通过的 Pod 才被加入 Endpoints。如果 readinessProbe 失败，Pod 虽然在运行但不接收流量——这是滚动更新期间保障可用性的关键机制。生产环境必须为 Deployment 配置准确的 readinessProbe。

4. **Session Affinity 与 Deployment**：Service 可以配置 `sessionAffinity: ClientIP`，将来自同一客户端 IP 的请求路由到同一 Pod。但在 Deployment 滚动更新时，目标 Pod 可能被终止——session affinity 不保证跨 Pod 重建的会话保持。

## 实践场景

- **无状态微服务暴露**：Deployment（3 副本）+ ClusterIP Service，集群内其他服务通过 Service 名称访问
- **外部流量接入**：Deployment + NodePort/LoadBalancer Service，将服务暴露到集群外部
- **蓝绿流量切换**：部署蓝色和绿色 Deployment，修改 Service selector 在两者间切换流量
- **金丝雀流量切分**：通过调节蓝色和绿色 Deployment 的副本数比例，间接控制流量分配（不如 Istio 精确）

## 常见问题

### 问题1：Service Endpoints 为空
**症状**：`kubectl get endpoints <service>` 显示为空或 `<none>`
**根因**：Service selector 与 Deployment Pod labels 不匹配；或 Pod 未通过 readinessProbe
**修复**：对比 Service selector 和 Pod labels 确保精确匹配；检查 Pod 的 Ready condition

### 问题2：滚动更新期间短暂的服务不可用
**症状**：Deployment 滚动更新时少量请求失败
**根因**：旧 Pod 被终止时仍有请求在路由到它；或新 Pod 未完全 Ready 就开始接收流量
**修复**：配置 `preStop` hook 延迟 Pod 终止（如 `sleep 5` 等待 Endpoints 更新）；确保 readinessProbe 准确

### 问题3：Service selector 修改后流量未切换
**症状**：修改 Service selector 指向新版本 Deployment 后流量仍到旧版本
**根因**：kube-proxy 的 iptables/IPVS 规则更新有延迟；或 Endpoints 未正确更新
**修复**：等待 10-30 秒让规则收敛；检查 Endpoints 列表是否已更新为新版本 Pod IP

## 关键命令

```bash
# 🟢 查看 Deployment、Service、Endpoints 的关联
kubectl get deployment,svc,endpoints -l app=<name> -n <ns>

# 🟢 验证 label 匹配
kubectl get pods -n <ns> --selector='<service-selector>' --show-labels

# 🟢 查看 Service 详细信息和 Endpoints
kubectl describe svc <name> -n <ns>
kubectl get endpoints <name> -n <ns>

# 🟢 从集群内测试 Service 连通性
kubectl exec -it <pod> -n <ns> -- curl http://<service-name>.<namespace>:<port>

# 🟡 修改 Service selector（蓝绿切换）
kubectl patch svc <name> -n <ns> -p '{"spec":{"selector":{"version":"v2"}}}'

# 🟢 查看 Service 的 Session Affinity 配置
kubectl get svc <name> -n <ns> -o jsonpath='{.spec.sessionAffinity}'
```

## 权衡取舍

| 维度 | Deployment 倾向 | Service 倾向 | 权衡点 |
|------|----------------|-------------|--------|
| Label 设计 | 灵活标签支持版本管理 | 稳定标签简化 selector | 灵活性 vs 路由稳定性 |
| 副本数 vs 流量 | 多副本分摊负载 | 单 Endpoints 列表简化路由 | 负载分散 vs 路由简单 |
| 滚动更新 | preStop 优雅终止 | Endpoints 实时同步 | Pod 生命周期 vs 流量切换 |
| 服务发现 | 应用内编码地址 | DNS 自动发现 | 灵活性 vs 自动化 |

## 最佳实践
1. 使用稳定通用的 label（如 `app: xxx`）作为 Service selector，避免使用易变标签（如 `version`）
2. 为 Deployment 配置准确的 readinessProbe，确保 Pod 真正就绪后才加入 Endpoints
3. 配置 `preStop` hook（如 `sleep 5`）和合理的 `terminationGracePeriodSeconds`，确保滚动更新零停机
4. 使用 ClusterIP Service 用于内部通信，需要外部访问时使用 Ingress 而非直接暴露 NodePort/LoadBalancer

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[Deployment]]
- [[Service]]
## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes (CNCF Graduated)]]
- [[23-实体/08-交付与制品/argo.md|Argo Workflows]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[19-故障诊断/08-技能体系/skill-set/k8s-pvc-storage/DIALOGUE.md|DIALOGUE]]
- [[22-概念/11-交叉分析/etcd-×-PVC.md|etcd-×-PVC]]
- [[22-概念/11-交叉分析/apiserver-×-Pod诊断.md|apiserver-×-Pod诊断]]
- [[22-概念/11-交叉分析/apiserver-×-Service.md|apiserver-×-Service]]
- [[22-概念/11-交叉分析/StatefulSet-×-Service.md|StatefulSet-×-Service]]


<!-- risk-assessed -->
