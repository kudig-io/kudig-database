---
title: Ingress Controllers
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- prometheus
- istio
- envoy
- cilium
- helm
- ingress
- gateway
tier: core
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Ingress Controllers 是什么
- 如何 Ingress Controllers
trigger_keywords:
- Ingress
- Controllers
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- tls-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Ingress|Ingress]] Controllers

## 概述

Ingress 资源本身只是声明式的路由配置，**必须有 Ingress Controller 在集群中运行**才能将其转化为实际的流量转发规则。Ingress Controller 通常以负载均衡器或反向代理的形式实现，负责监听 Ingress 和 EndpointSlice 的变化，并动态配置底层数据面（如 NGINX、[[Envoy|Envoy]]、云厂商 LB 等）。

## 核心概念/原理

- **控制器与 IngressClass**：每个 Ingress 通过 `ingressClassName` 字段关联一个 IngressClass，IngressClass 则声明了负责实现该类的控制器名称（`spec.controller`）。控制器仅处理匹配其 IngressClass 的 Ingress 资源。
- **多控制器共存**：一个集群中可以同时部署多个 Ingress Controller，只要它们使用不同的 IngressClass 即可。例如，一个用于内部流量（内部 NGINX），一个用于公网流量（云厂商 LB）。
- **默认控制器**：若创建 Ingress 时未指定 `ingressClassName`，且集群中恰好只有一个 IngressClass 被标记为默认，则 [[Kubernetes|Kubernetes]] 会自动将其分配给该 Ingress。

## 关键机制或特性

- **官方维护控制器**：Kubernetes 项目官方维护 AWS 和 GCE 的 Ingress Controller。
- **社区与第三方控制器**：社区提供了大量实现，涵盖不同数据面和云环境，常见的包括：
  - **NGINX Ingress Controller**（基于 NGINX）
  - **Traefik**（Go 编写的反向代理）
  - **HAProxy Ingress**（基于 HAProxy）
  - **Contour / Emissary-Ingress**（基于 Envoy）
  - **Istio Ingress**（基于 Istio）
  - **Kong Ingress Controller**
  - **Cilium Ingress Controller**
  - **云厂商方案**：Azure Application Gateway、Alibaba Cloud API Gateway、OCI Native Ingress Controller 等
- **功能差异**：不同控制器对路径类型、注解、TLS、速率限制、高级路由等功能的支持程度不同。

## 使用场景

- **根据现有技术栈选型**：若团队熟悉 NGINX，可选择 NGINX Ingress Controller；若需要 Service Mesh 能力，可选 Istio 或 Cilium。
- **多云/混合云部署**：在不同云厂商的集群中部署对应控制器，以利用云原生负载均衡能力。
- **安全与 WAF 需求**：选择集成 Web 应用防火墙（WAF）的控制器，如 BunkerWeb、Wallarm。

## 最佳实践/注意事项

- **仔细阅读控制器文档**：不同控制器的行为、注解和限制差异较大，部署前务必查看官方文档中的 caveats。
- **推荐使用 Gateway API**：Ingress API 已冻结，官方建议新项目或重构时评估并迁移到 Gateway API。
- **避免多个默认 IngressClass**：集群中最多只能有一个默认 IngressClass，否则会导致未指定类的 Ingress 创建被阻止。
- **区分 IngressClass 的适用范围**：通过 IngressClass 的 `parameters.scope` 字段，可以将配置参数限定在集群级或命名空间级，便于多团队协作。

## 生产 YAML 示例

### IngressClass 定义

```yaml
# 定义默认 IngressClass
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx
  annotations:
    ingressclass.kubernetes.io/is-default-class: "true"
spec:
  controller: k8s.io/ingress-nginx
---
# 定义第二个 IngressClass（内部流量）
apiVersion: networking.k8s.io/v1
kind: IngressClass
metadata:
  name: nginx-internal
spec:
  controller: k8s.io/ingress-nginx-internal
  parameters:
    apiGroup: k8s.example.net
    kind: IngressParameters
    name: internal-config
    scope: Namespace
```

### NGINX Ingress Controller 部署要点

```yaml
# Helm 安装示例
# helm repo add ingress-nginx https://kubernetes.github.io/ingress-nginx
# helm install ingress-nginx ingress-nginx/ingress-nginx \
#   --namespace ingress-nginx --create-namespace \
#   --set controller.replicaCount=3 \
#   --set controller.resources.requests.cpu=500m \
#   --set controller.resources.requests.memory=512Mi \
#   --set controller.metrics.enabled=true \
#   --set controller.podAntiAffinity=hard

# 验证安装
# kubectl get pods -n ingress-nginx
# kubectl get svc -n ingress-nginx
# kubectl get ingressclass
```

## 主流控制器对比矩阵

| 控制器 | 数据面 | Gateway API | L7 特性 | 许可证 | 适用场景 |
|--------|--------|-------------|---------|--------|----------|
| NGINX Ingress | NGINX | 支持 | 丰富注解 | Apache 2.0 | 通用 Web 服务 |
| Traefik | Go 原生 | 原生支持 | 中间件链 | MIT | 轻量级/边缘 |
| Contour | Envoy | 原生支持 | HTTPProxy CRD | Apache 2.0 | Envoy 生态 |
| Istio Ingress | Envoy | 原生支持 | VirtualService | Apache 2.0 | Service Mesh |
| Cilium Ingress | eBPF+Envoy | 原生支持 | CiliumEnvoyConfig | Apache 2.0 | 高性能/eBPF |
| Kong | NGINX/Go | 支持 | 插件生态 | Apache 2.0 | API 网关 |
| AWS ALB IC | AWS ALB | 有限 | AWS 特性 | Apache 2.0 | AWS 云原生 |

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Ingress 创建成功但无 ADDRESS | 控制器未安装或 IngressClass 不匹配 | `kubectl get ingressclass`；`kubectl get pods -n ingress-nginx` |
| 多个控制器争抢 Ingress | IngressClass 配置错误或多个默认类 | 确保每个 Ingress 明确指定 `ingressClassName` |
| 404 响应所有请求 | 后端 Service/EndpointSlice 为空 | `kubectl describe ingress <name>`；检查后端 Service 端点 |
| TLS 证书错误 | Secret 不存在或证书与域名不匹配 | `kubectl get secret <tls-secret> -o yaml`；检查 CN/SAN |
| 注解不生效 | 注解语法错误或控制器不支持该注解 | 查阅控制器文档确认注解格式和支持情况 |

## 生产检查清单

- [ ] Ingress Controller 至少 2 副本 + podAntiAffinity 分散
- [ ] 设置 resource requests/limits 防止 OOM
- [ ] 配置 Prometheus 指标暴露和告警
- [ ] 集群中仅有一个默认 IngressClass
- [ ] 使用 [[cert-manager|cert-manager]] 自动管理 TLS 证书
- [ ] 定期更新控制器版本修复安全漏洞
- [ ] 评估是否迁移到 Gateway API

## 命令快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有 IngressClass
kubectl get ingressclass

# 查看 Ingress Controller Pod 状态
kubectl get pods -n ingress-nginx

# 查看 Ingress Controller 日志
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=50

# 检查 Ingress 状态和 ADDRESS
kubectl get ingress -A

# 查看 Ingress 详情和 Events
kubectl describe ingress <name> -n <ns>

# 测试 Ingress 路由
curl -H "Host: example.com" http://<ingress-ip>/path
```
## 交叉引用

- [Ingress](ingress.md) — Ingress 资源的规则、路径类型和 TLS 配置
- [Gateway API](gateway-api.md) — 推荐的 Ingress 继任方案
- [Service](service.md) — Ingress 后端指向的 Service 类型
- [Network Policies](network-policies.md) — 控制 Ingress Controller Pod 的网络访问

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/ingress-controllers/

## Related
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
