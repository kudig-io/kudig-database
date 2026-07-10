---
title: Ingress Controller
summary: Ingress Controller 是 Kubernetes 集群中负责将集群外部流量路由到内部 Service 的关键组件。它与 Ingress
  资源协同工作，但二者职责截然不同：Ingress 资源声明路由规则（WHAT），Ingress Controller 负责执行这些规则（HOW）。
category: concepts
tags:
- core-concept
- domain-03
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Ingress Controller

Ingress Controller 是 Kubernetes 集群中负责将集群外部流量路由到内部 Service 的关键组件。它与 Ingress 资源协同工作，但二者职责截然不同：Ingress 资源声明路由规则（WHAT），Ingress Controller 负责执行这些规则（HOW）。

## Ingress 资源与 Ingress Controller 的区别

- **Ingress 资源**：标准的 Kubernetes API 对象，仅描述 HTTP/HTTPS 路由规则（主机名、路径、后端 Service、TLS 配置）。创建或更新 Ingress 资源本身不会产生任何实际的网络效果。
- **Ingress Controller**：实际运行在 Pod 中的反向代理进程（如 NGINX、Envoy），通过监听 Ingress 资源变化，动态生成并热加载代理配置，将外部请求转发到后端 Pod。

没有 Ingress Controller，Ingress 资源只是一纸空文；没有 Ingress 资源，Controller 无事可做。二者缺一不可。

## 主流控制器对比

| 控制器 | 特点 | 适用场景 |
|---|---|---|
| NGINX Ingress Controller | 社区最成熟、文档丰富、annotations 生态庞大 | 通用场景、中小型集群 |
| Traefik | 原生支持动态配置、Dashboard 友好、云原生设计 | 微服务架构、需要自动服务发现 |
| HAProxy | 高性能、低延迟、企业级稳定性 | 高并发、金融级流量场景 |
| AWS ALB/NLB Ingress | 与云厂商深度集成、免运维代理节点 | AWS EKS 环境 |
| Istio Gateway | 服务网格入口，支持 L7 流量治理 | 已采用 Istio 的集群 |

选型时需权衡性能、可观测性、社区活跃度与团队熟悉度。对于阿里云用户，建议优先评估 ACK 托管方案以降低运维成本。

## Ingress 资源字段详解

- **`rules`**：定义主机名与路径的映射关系，每个 rule 可包含多条 path，支持路径类型 `Exact`、`Prefix`、`ImplementationSpecific`。`Prefix` 是最常用的类型，按前缀匹配请求路径。若路径末尾带 `/`，匹配逻辑会有细微差异。
- **`tls`**：指定 HTTPS 终止使用的 Secret（含证书与私钥），支持通配符证书和多主机 SNI。Controller 通常在此处终止 TLS，后端以明文 HTTP 通信。对于需要端到端加密的场景，可使用 `nginx.ingress.kubernetes.io/backend-protocol: HTTPS`。
- **`annotations`**：控制器特定扩展指令，例如 `nginx.ingress.kubernetes.io/rewrite-target` 用于路径重写、`nginx.ingress.kubernetes.io/rate-limit` 用于限流。不同控制器的 annotations 互不兼容，迁移时需要逐条对照转换。

典型 Ingress 示例：

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  annotations:
    nginx.ingress.kubernetes.io/rewrite-target: /
  name: example-ingress
spec:
  ingressClassName: nginx
  rules:
  - host: app.example.com
    http:
      paths:
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-service
            port:
              number: 8080
```

## 阿里云 ACK 的 Ingress 组件

阿里云 ACK 提供两种托管 Ingress 方案：

- **ALB Ingress**：基于应用型负载均衡（ALB），支持自动弹性、QUIC、基于 Header/Cookie 的高级路由，适合七层流量。ALB 为云原生托管，用户无需关心 Controller Pod 的可用性。
- **Nginx Ingress**：基于社区 NGINX Ingress Controller 的托管版，兼容社区 annotations，适合需要精细化控制的场景。用户需要关注 Controller Pod 的资源配额与版本升级。

选择时需权衡：ALB 免运维但灵活性略低；Nginx 可控性高但需要关注 Controller Pod 资源与版本。对于流量波动大的互联网应用，ALB 的自动弹性更具优势。

## 远程顾问诊断要点

在无法直连集群的远程顾问模式下，排查 Ingress 相关问题的核心思路是分层验证，从规则声明到 Controller 运行再到后端可达性逐层排查：

1. **Ingress Class 匹配问题**：确认 Ingress 资源的 `spec.ingressClassName` 与集群中运行的 Controller 注册的 class 名称一致。若 class 不匹配，Controller 将完全忽略该 Ingress。多个 Controller 共存时尤其容易出错。
2. **控制器未运行**：询问用户执行 `kubectl get pods -n kube-system | grep ingress`（或对应命名空间）查看 Controller Pod 状态。若 Pod 处于 CrashLoopBackOff 或 Pending，需进一步排查镜像拉取、资源配额或节点亲和性。
3. **Backend Service 不存在或端口不匹配**：验证 `spec.rules.http.paths.backend.service.name` 和 `port.number` 指向的 Service 是否存在，且 Service 的 `targetPort` 与后端 Pod 的容器端口一致。若 Service 没有匹配的 Endpoints，Controller 会将该路径标记为不可用。
4. **DNS 与外部可达性**：确认域名解析是否指向 Ingress 的外部端点（如 SLB IP 或 ALB DNS），排除客户端到入口层的网络问题。同时检查安全组或防火墙是否放行了 80/443 端口。
5. **SSL 证书过期或配置错误**：TLS Secret 中证书过期、私钥不匹配或 Secret 名称错误都会导致 HTTPS 访问失败。指导用户检查 Secret 内容以及 Ingress 中 `tls.secretName` 的引用。

更多排查细节可参考 [[故障诊断/技能体系/skill-set/k8s-ingress-gateway/SKILL.md|ingress-gateway-troubleshooting]] 与技能页面 [[故障诊断/技能体系/skill-set/k8s-ingress-gateway/SKILL.md|k8s-ingress-gateway]]。

## 相关概念

- [[service-networking]] — Kubernetes Service 网络模型
- [[cni-networking-model]] — CNI 网络模型与插件对比
- [[service-mesh-architecture]] — 服务网格架构

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
