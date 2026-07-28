---
title: Deployment × Ingress
summary: Deployment × Ingress：Deployment与Ingress是Kubernetes生产环境中的两个关键维度。理解它们之间的交互关系对于构建稳定、可观测的集群至关重要。
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

# Deployment × Ingress

## 概述
Deployment 管理无状态应用的 Pod 副本，Ingress 定义外部 HTTP(S) 流量到集群内 Service 的路由规则。两者通过 Service 间接关联：Ingress → Service → Deployment 的 Pod。Ingress Controller 通过 Endpoints 感知 Deployment Pod 的 IP 变化，动态更新路由后端。当 Deployment 滚动更新时，新旧 Pod 的 IP 变化通过 Endpoints 传播到 Ingress Controller，实现零停机的流量切换。

## 技术关联机制

1. **三层路由链路**：外部请求 → Ingress Controller（NGINX/Envoy）→ Service（ClusterIP 虚 IP）→ Endpoints（Pod IP 列表）→ Deployment Pod。Ingress Controller 不直接 watch Deployment，而是通过 Service 的 Endpoints 获取后端 Pod IP。当 Deployment 滚动更新创建新 Pod 或删除旧 Pod 时，Endpoints Controller 更新 Endpoints 对象，Ingress Controller watch 到变化后更新 upstream 配置。

2. **滚动更新期间的流量行为**：Deployment 滚动更新时，旧 Pod 被终止、新 Pod 被创建。Ingress Controller 通过 Endpoints 感知到 Pod IP 刘表变化。如果配置了 `preStop` hook 和合理的 `terminationGracePeriodSeconds`，旧 Pod 在收到 SIGTERM 后有足够时间处理完已接收的请求再退出，避免 502。NGINX Ingress Controller 还支持 `nginx.ingress.kubernetes.io/server-snippet` 配置 `proxy_ssl_verify` 和慢启动（slow_start）来平滑过渡。

3. **Ingress 的金丝雀发布**：NGINX Ingress Controller 支持通过 annotation 实现 Deployment 的金丝雀发布——创建第二个 Deployment（canary 版本）和对应的 canary Ingress，通过 `nginx.ingress.kubernetes.io/canary-weight: 10` 将 10% 流量导向 canary Deployment。这种方式不需要修改主 Service 或主 Deployment。

4. **蓝绿发布的 Ingress 实现**：通过 Ingress 的 `nginx.ingress.kubernetes.io/blue-green` 或直接修改 Ingress 的 backend Service 指向，实现蓝绿 Deployment 间的流量切换。相比修改 Service selector，Ingress 层切换更加灵活，支持权重控制和灰度策略。

## 实践场景

- **外部流量暴露**：Deployment + ClusterIP Service + Ingress 三件套，将 Web 应用暴露到互联网
- **滚动更新零停机**：Ingress Controller 通过 Endpoints 实时感知 Pod 变化，配合 preStop hook 实现无损更新
- **金丝雀发布**：通过 canary Ingress annotation 将 10% 流量导到新版本 Deployment，验证后逐步提升比例
- **多版本并存**：通过不同 Ingress 的 path/host 规则将不同路径路由到不同版本的 Deployment

## 常见问题

### 问题1：滚动更新期间出现 502 Bad Gateway
**症状**：Deployment 滚动更新时少量请求返回 502
**根因**：旧 Pod 被终止时仍有 Ingress 转发的请求在途；新 Pod 还未完全 Ready 就接收流量
**修复**：配置 `preStop` hook 延迟 Pod 终止；确保 readinessProbe 正确配置；增大 `terminationGracePeriodSeconds`

### 问题2：Ingress 创建成功但访问返回 503
**症状**：Ingress 存在但后端不可达
**根因**：Ingress 的 backend Service 与 Deployment 的 Pod 不匹配（selector 不一致）；或 Deployment 无 Ready Pod
**修复**：确认 Ingress backend Service 名称和端口正确；确认 Service selector 匹配 Deployment Pod labels；确认 Endpoints 非空

### 问题3：金丝雀流量比例不准确
**症状**：配置 canary-weight: 10 但实际流量分配不均匀
**根因**：NGINX Ingress Controller 的 canary-weight 基于 cookie/请求哈希，少量请求时可能不精确
**修复**：增加测试请求量获得统计显著性；使用 Istio VirtualService 获得更精确的流量控制

## 关键命令

```bash
# 🟢 查看 Deployment、Service、Ingress 的关联
kubectl get deployment,svc,ingress -l app=<name> -n <ns>

# 🟢 查看 Endpoints（确认 Ingress 后端可达）
kubectl get endpoints <service-name> -n <ns>

# 🟢 查看 Ingress 路由规则
kubectl describe ingress <name> -n <ns>

# 🟢 查看 Ingress Controller 日志（排查路由问题）
kubectl logs -n ingress-nginx <controller-pod> | grep <service-name>

# 🟡 配置金丝雀 Ingress
kubectl annotate ingress <canary-name> -n <ns> \
  nginx.ingress.kubernetes.io/canary-weight=10 \
  nginx.ingress.kubernetes.io/canary-by-header=x-canary
```

## 权衡取舍

| 维度 | Deployment 倾向 | Ingress 倾向 | 权衡点 |
|------|----------------|-------------|--------|
| 流量切换 | Service selector 切换 | Ingress backend 切换 | 切换粒度 vs 灵活性 |
| 金丝雀 | 多 Deployment 管理复杂 | Annotation 控制简单 | 管理复杂 vs 操作便利 |
| 滚动更新兼容 | preStop + grace period | Endpoints 实时同步 | Pod 生命周期 vs 流量切换 |
| 多版本路由 | 标签区分版本 | host/path 路由分发 | 标签管理 vs 路由规则 |

## 最佳实践
1. 为 Deployment 配置 `preStop` hook（如 `sleep 5`），确保 Ingress Controller 在 Pod 终止前移除 Endpoints
2. 使用 readinessProbe 确保 Pod 完全就绪后才被 Ingress 接入流量
3. 金丝雀发布通过 Ingress annotation 实现，避免修改主 Deployment 和 Service
4. 在 Ingress 层配置 TLS termination，后端 Deployment 使用 HTTP（简化证书管理）

## 工具推荐
- kubectl：基础诊断
- [[23-实体/08-交付与制品/helm.md|Helm]]/Kustomize：配置管理
- [[23-实体/07-可观测性/prometheus.md|Prometheus]]/Grafana：联合监控
- [[23-实体/08-交付与制品/argocd.md|ArgoCD]]：GitOps同步

## 相关概念
- [[deployment|Deployment]]
- [[ingress|Ingress]]
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
