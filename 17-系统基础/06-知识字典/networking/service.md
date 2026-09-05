---
title: Service
description: '## 概述'
summary: Service 是 [[kubernetes|Kubernetes]] 中用于将运行在一组 Pod 上的网络应用暴露给集群内外的核心抽象对象。由于
  Pod 是临时的、会被动态创建和销毁的，其 IP 地址也随之变化，Service 通过稳定的虚拟 IP（ClusterIP）和 DNS 名称，解耦了前端客户端与后端
  Pod 的耦合，使现有应用无需改造即可在 Kubernetes 中运行。
category: dictionary
tags:
- k8s
- glossary
- terminology
- statefulset
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
- Service 是什么
- 如何 Service
trigger_keywords:
- Service
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Service

## 概述

Service 是 [[kubernetes|Kubernetes]] 中用于将运行在一组 Pod 上的网络应用暴露给集群内外的核心抽象对象。由于 Pod 是临时的、会被动态创建和销毁的，其 IP 地址也随之变化，Service 通过稳定的虚拟 IP（ClusterIP）和 DNS 名称，解耦了前端客户端与后端 Pod 的耦合，使现有应用无需改造即可在 Kubernetes 中运行。

## 核心概念/原理

- **Selector 与 EndpointSlice**：Service 通过 `selector` 匹配标签相同的 Pod，控制平面自动创建并维护对应的 EndpointSlice，记录所有后端 Pod 的 IP 与端口。无 selector 的 Service 可配合手动创建的 EndpointSlice，将流量转发到集群外部地址或其他命名空间。
- **端口映射**：Service 的 `port` 是暴露的端口，`targetPort` 是 Pod 容器的实际监听端口，支持按名称引用容器端口，便于后端升级时平滑切换。
- **Headless Service**：将 `.spec.clusterIP` 显式设为 `"None"`，不再分配虚拟 IP，DNS 直接返回后端 Pod 的 IP 列表（A/AAAA 记录），适用于需要直接访问特定 Pod 或有状态服务场景。
- **服务发现**：集群内的 Pod 可通过环境变量（创建顺序有要求）或 DNS 发现 Service，推荐使用 DNS 方式以避免依赖启动顺序。

## 关键机制或特性

- **Service 类型**：
  - `ClusterIP`（默认）：集群内部可访问的虚拟 IP。
  - `NodePort`：在每个节点上开放固定端口（默认 30000–32767），将流量代理到 Service。
  - `LoadBalancer`：在云厂商环境中自动创建外部负载均衡器。
  - `ExternalName`：通过 DNS CNAME 将 Service 映射到外部域名，不做任何代理。
- **[[endpointslices|EndpointSlices]]**：自 v1.21 起稳定，是 kube-proxy 路由内部流量的真实来源，默认每个 Slice 最多 100 个端点（最大可配 1000）。旧版 Endpoints API 已弃用。
- **流量策略**：支持 `internalTrafficPolicy` 与 `externalTrafficPolicy`（Cluster/Local），控制流量在集群内部或外部进入时的路由范围。
- **会话保持（Session Affinity）**：可基于客户端 IP 配置会话亲和性，使同一客户端流量始终到达同一 Pod。
- **应用协议（appProtocol）**：自 v1.20 起稳定，用于为端口声明应用层协议（如 `kubernetes.io/h2c`、`kubernetes.io/ws`），供实现方提供更丰富的行为。

## 使用场景

- **微服务间通信**：通过 ClusterIP + DNS 实现服务间稳定调用。
- **外部访问入口**：使用 NodePort 或 LoadBalancer 将 Web 应用暴露到公网。
- **连接集群外服务**：利用无 selector Service + 手动 EndpointSlice 或 ExternalName 对接外部数据库、 legacy 系统。
- **有状态服务发现**：Headless Service 配合 [[statefulset|StatefulSet]]，为每个 Pod 提供独立 DNS 记录。

## 最佳实践/注意事项

- **优先使用 DNS 发现**：相比环境变量，DNS 不依赖 Pod 与 Service 的创建顺序，更灵活可靠。
- **无 selector Service 需手动维护 EndpointSlice**：创建或更新 EndpointSlice 时，避免使用 loopback、link-local 或其他 Service 的 ClusterIP 作为 endpoint 地址。
- **NodePort 端口冲突**：可指定 `nodePort` 使用静态段（默认 30000–30085）以降低冲突概率；动态分配使用 30086–32767。
- **LoadBalancer IP 弃用**：`.spec.loadBalancerIP` 在 v1.24 已弃用，建议改用云厂商特定的注解或迁移到 Gateway API。
- **ExternalName 的协议兼容性**：对 HTTP/HTTPS 等依赖 Host 头的协议，ExternalName 可能导致 TLS 证书不匹配或 Host 头错误，需谨慎使用。

## 生产 YAML 示例

### 各类型 Service 对照

```yaml
# 1. ClusterIP（默认 — 集群内部访问）
apiVersion: v1
kind: Service
metadata:
  name: backend-api
  namespace: production
spec:
  type: ClusterIP
  selector:
    app: backend-api
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
    appProtocol: http             # 声明应用协议
  - name: grpc
    port: 9090
    targetPort: 9090
    protocol: TCP
    appProtocol: kubernetes.io/h2c
  sessionAffinity: ClientIP
  sessionAffinityConfig:
    clientIP:
      timeoutSeconds: 10800       # 会话保持 3 小时
---
# 2. Headless Service（StatefulSet 直接 Pod DNS）
apiVersion: v1
kind: Service
metadata:
  name: cassandra
  namespace: data
spec:
  clusterIP: None                  # Headless
  selector:
    app: cassandra
  ports:
  - port: 9042
# DNS 返回所有 Pod IP：cassandra-0.cassandra.data.svc.cluster.local
---
# 3. NodePort（开发/测试暴露）
apiVersion: v1
kind: Service
metadata:
  name: web-nodeport
  namespace: staging
spec:
  type: NodePort
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 8080
    nodePort: 30080                # 静态端口（可选）
---
# 4. LoadBalancer（云环境生产暴露）
apiVersion: v1
kind: Service
metadata:
  name: web-public
  namespace: production
  annotations:
    service.beta.kubernetes.io/aws-load-balancer-type: "nlb"
    service.beta.kubernetes.io/aws-load-balancer-scheme: "internet-facing"
spec:
  type: LoadBalancer
  selector:
    app: web
  ports:
  - port: 443
    targetPort: 8443
  externalTrafficPolicy: Local     # 保留源 IP
---
# 5. ExternalName（CNAME 映射外部服务）
apiVersion: v1
kind: Service
metadata:
  name: legacy-db
  namespace: production
spec:
  type: ExternalName
  externalName: db.legacy.example.com    # DNS CNAME
---
# 6. 无 Selector Service + 手动 EndpointSlice（对接外部地址）
apiVersion: v1
kind: Service
metadata:
  name: external-payment
  namespace: production
spec:
  ports:
  - port: 443
    targetPort: 443
```

## Service 类型决策树

```
需要暴露到集群外部？
  │
  ├─ 否 → ClusterIP（默认）
  │       └─ 需要直接访问 Pod？ → Headless (clusterIP: None)
  │
  └─ 是 → 有云 LB？
          │
          ├─ 是 → LoadBalancer
          │       └─ 需要保留源 IP？ → externalTrafficPolicy: Local
          │
          └─ 否 → NodePort
                  └─ 仅做 DNS 映射？ → ExternalName

```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| Service ClusterIP 无响应 | 无后端 Pod 或 Pod 未 Ready | `kubectl get endpointslices -l kubernetes.io/service-name=<svc>` |
| LoadBalancer 一直 Pending | 云控制器未安装或配额不足 | `kubectl describe svc <name>` 查看 Events |
| ExternalName 解析失败 | DNS CNAME 链不通或 TLS 证书不匹配 | `kubectl exec -- nslookup <external-name>` |
| NodePort 无法从外部访问 | 防火墙/安全组未开放端口 | 检查节点安全组规则（30000-32767） |
| 会话保持不生效 | `sessionAffinity` 未配置或超时过短 | `kubectl get svc -o yaml` 检查 sessionAffinity 字段 |

## 生产检查清单

- [ ] 生产 Service 使用 DNS 服务发现（而非环境变量）
- [ ] LoadBalancer Service 配置 `externalTrafficPolicy` 根据需求
- [ ] 无 selector Service 手动维护 EndpointSlice
- [ ] NodePort 范围由集群管理员统一规划
- [ ] 避免 ExternalName 用于依赖 Host 头的 HTTP/HTTPS 服务
- [ ] 使用 `appProtocol` 声明端口协议便于控制器优化

## 命令快速参考

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Service 列表
kubectl get svc -n production -o wide

# 查看 Service 详情
kubectl describe svc backend-api -n production

# 查看 Service Endpoints
kubectl get endpointslices -l kubernetes.io/service-name=backend-api

# 从集群内测试 Service
kubectl run test --rm -it --image=busybox -- wget -qO- http://backend-api.production.svc:80

# 查看 NodePort 分配
kubectl get svc -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.type}{"\t"}{.spec.ports[*].nodePort}{"\n"}{end}'

# 临时端口转发
kubectl port-forward svc/backend-api 8080:80 -n production
```
## 交叉引用

- [EndpointSlices](endpointslices.md) — Service 后端端点的管理和条件
- [DNS for Services](dns-for-services-and-[[pods|pods]].md) — Service DNS 记录格式
- [Service ClusterIP Allocation](service-clusterip-allocation.md) — ClusterIP 分配策略
- [Service Internal Traffic Policy](service-internal-traffic-policy.md) — 内部流量节点本地路由
- [Ingress](ingress.md) — HTTP/HTTPS 层的 Service 暴露
- [Gateway API](gateway-api.md) — 下一代 Service 暴露方案

## 参考链接

- https://kubernetes.io/docs/concepts/services-networking/service/

## Related

- [[21-生态参考/03-领域索引/dns-index.md|DNS 知识图谱索引]]

```

<!-- risk-assessed -->
- [[01-集群基础/01-架构总览/12-security-architecture|14 - Kubernetes 安全架构深度分析]]
- [[01-集群基础/01-架构总览/11-performance-tuning-guide|13 - Kubernetes 性能调优专项指南]]
- [[01-集群基础/03-控制平面/16-kube-proxy-deep-dive|kube-proxy 深度解析 (kube-proxy Deep Dive)]]
- [[01-集群基础/03-控制平面/13-kube-controller-manager-deep-dive|kube-controller-manager 深度解析]]
- [[03-清单模式/01-YAML参考/10-ingress-ingressclass|10 - Ingress / IngressClass YAML 配置参考]]
- [[03-清单模式/01-YAML参考/09-endpoints-endpointslice|09 - Endpoints / EndpointSlice YAML 配置参考]]
- [[04-应用模式/02-行业架构/20-microservice-governance-architecture|微服务治理与 Service Mesh Kubernetes 生产架构设计]]
- [[05-网络/01-K8s网络核心/11-service-advanced-features|Service 高级特性与应用案例 (Service Advanced Features)]]
- [[05-网络/01-K8s网络核心/13-dns-service-discovery|33 - 服务发现与 DNS 配置 (Service Discovery & DNS)]]
- [[05-网络/01-K8s网络核心/01-network-architecture-overview-faq|FAQ 文档]]
- [[05-网络/03-服务网格/06-traefik-mesh-enterprise|Traefik Mesh Enterprise Service Mesh 深度实践]]
- [[15-AI基础设施/01-基础设施/29-alibaba-cloud-integration|15 - 阿里云特定集成表]]
- [[16-专项技术/03-扩展机制/20-serverless-faas-guide|K8s Serverless / FaaS 实践指南]]
- [[17-系统基础/04-K8s事件/06-node-lifecycle-condition-events|06 - 节点生命周期与状态事件]]
- [[17-系统基础/04-K8s事件/10-service-networking-events|10 - Service 与网络事件]]
- [[17-系统基础/05-速查卡/gateway-api|Gateway API]]
- [[17-系统基础/06-知识字典/configuration/liveness-readiness-and-startup-probes|Liveness, Readiness, and Startup Probes]]
- [[17-系统基础/06-知识字典/fundamentals/field-selectors|字段选择器]]
- [[17-系统基础/06-知识字典/fundamentals/mixed-version-proxy|Mixed Version Proxy（混合版本代理）]]
- [[17-系统基础/06-知识字典/fundamentals/kubernetes-object-management|Kubernetes 对象管理]]
- [[17-系统基础/06-知识字典/fundamentals/namespaces|命名空间]]
- [[17-系统基础/06-知识字典/fundamentals/the-kubectl-command-line-tool|kubectl 命令行工具]]
- [[17-系统基础/06-知识字典/fundamentals/garbage-collection|Garbage Collection（垃圾回收）]]
- [[17-系统基础/06-知识字典/fundamentals/object-names-and-ids|对象名称和 ID]]
- [[17-系统基础/06-知识字典/fundamentals/objects-in-kubernetes|Kubernetes 中的对象]]
- [[17-系统基础/06-知识字典/fundamentals/cloud-controller-manager|Cloud Controller Manager（云控制器管理器）]]
- [[17-系统基础/06-知识字典/networking/gateway-api|Gateway API]]
- [[17-系统基础/06-知识字典/networking/cluster-networking|集群网络（Cluster Networking）]]
- [[17-系统基础/06-知识字典/networking/networking-on-windows|Networking on Windows]]
- [[17-系统基础/06-知识字典/networking/service-internal-traffic-policy|Service Internal Traffic Policy]]
- [[17-系统基础/06-知识字典/networking/dns-for-services-and-pods|DNS for Services and Pods]]
- [[17-系统基础/06-知识字典/networking/service-mesh|服务网格（Service Mesh）]]
- [[17-系统基础/06-知识字典/networking/cluster-mesh|多集群网络互联（Cluster Mesh）]]
- [[17-系统基础/06-知识字典/networking/ipv4-ipv6-dual-stack|IPv4/IPv6 dual-stack]]
- [[17-系统基础/06-知识字典/networking/topology-aware-routing|Topology Aware Routing]]
- [[17-系统基础/06-知识字典/networking/service-clusterip-allocation|Service ClusterIP allocation]]
- [[17-系统基础/06-知识字典/observability/alerting-and-slo-monitoring|告警与 SLO 监控工程]]
- [[17-系统基础/06-知识字典/operations/node-shutdowns|节点关闭（Node Shutdowns）]]
- [[17-系统基础/06-知识字典/operations/operations-best-practices|01 - Kubernetes 生产环境运维最佳实践字典]]
- [[17-系统基础/06-知识字典/operations/failure-patterns-analysis|02 - Kubernetes 故障模式与根因分析字典]]
- [[17-系统基础/06-知识字典/platform-engineering/developer-portal-and-platform-metrics|开发者门户与平台工程度量]]
- [[17-系统基础/06-知识字典/platform-engineering/proxies-in-kubernetes|Kubernetes 中的代理]]
- [[17-系统基础/06-知识字典/security/security-for-windows-nodes|Windows 节点安全]]
- [[17-系统基础/06-知识字典/security/spiffe-spire-identity|SPIFFE / SPIRE 与工作负载身份]]
- [[17-系统基础/06-知识字典/workloads/managing-workloads|Managing Workloads]]
- [[17-系统基础/06-知识字典/workloads/pod-quality-of-service-classes|Pod Quality of Service Classes]]
- [[17-系统基础/06-知识字典/workloads/container-environment|容器环境（Container Environment）]]
- [[18-云厂商/07-多云混合/07-huawei-cce-enterprise|华为云 CCE 企业级容器平台深度实践]]
- [[19-故障诊断/04-高级排障/10-kind-k3s-single-node-troubleshooting|Kind / K3s 单机集群故障排查]]
- [[19-故障诊断/10-QA语料/command-output-diagnosis|命令输出解读语料 — Agent 诊断推理核心数据 [故障诊断]]]
- [[21-生态参考/01-CNCF全景/03-cncf-selection-guide|CNCF 项目选型指南]]
- [[22-概念/03-网络/tcp-udp-protocol-stack|TCP/UDP Protocol Stack]]
- [[22-概念/08-可靠性与运维/Symptom-SOP-RootCause-Mapping|Symptom-SOP-RootCause Mapping]]
- [[22-概念/08-可靠性与运维/microservice-resilience-patterns|Microservice Resilience Patterns]]
- [[22-概念/10-最佳实践/bp-README|Kubernetes 最佳实践指南]]
- [[22-概念/11-交叉分析/apiserver-×-Service|apiserver × Service]]
- [[22-概念/11-交叉分析/StatefulSet-×-Service|StatefulSet × Service]]
- [[22-概念/11-交叉分析/Deployment-×-Service|Deployment × Service]]
- [[22-概念/11-交叉分析/etcd-×-Service|etcd × Service]]
- [[23-实体/13-云厂商与发行版/007-apsara-stack-ess-scaling|专有云 (Apsara Stack) - ESS 弹性伸缩]]
- [[26-技能/01-集群运维/cloud-provider/诊断排障/ts-cloud-provider|云服务商集成排查]]
- [[26-技能/01-集群运维/cluster-upgrade/最佳实践/bp-README|Kubernetes 最佳实践指南 (skills)]]
- [[26-技能/04-工作负载/deployment/deployment-canary-and-bluegreen|金丝雀与蓝绿发布]]
- [[26-技能/04-工作负载/pod/培训/learn-README|新人上手快速路径（Quick Start）]]
- [[26-技能/04-工作负载/pod/培训/learn-02-pod-basics|第二课：Pod - K8s 的最小调度单元]]
- [[26-技能/04-工作负载/pod/培训/learn-02-first-ticket-guide|Day 2: 第一个工单处理指南]]
- [[26-技能/04-工作负载/pod/培训/learn-oncall-quick-qa|工单数字人快速问答 - On-Call 速查]]
- [[26-技能/04-工作负载/pod/培训/public-one-month/week-1-foundation/day-5-k8s-architecture|Day 5: Kubernetes 架构全貌]]
- [[26-技能/04-工作负载/pod/培训/测验/assessment-troubleshooting-lab-exam|Troubleshooting Lab Exam]]
- [[26-技能/04-工作负载/pod/资源与自动扩缩/pod-quality-of-service-classes|Pod Quality of Service Classes]]
- [[26-技能/04-工作负载/pod/配置与字典/dns-for-services-and-pods|DNS for Services and Pods]]
- [[26-技能/05-网络/cni/培训/kubernetes-terway-presentation|Kubernetes Terway (Aliyun) 全栈进阶培训 (从入门到专家) [topic-presentations]]]
- [[26-技能/05-网络/service/培训/kubernetes-service-presentation|Kubernetes Service 全栈进阶培训 (从入门到专家) [topic-presentations]]]
- [[26-技能/05-网络/service/培训/learn-04-service-basics|第四课：Service - 让应用可以被访问]]
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-logging-management-guide|Kubernetes 日志管理最佳实践]]
- [[26-技能/08-可观测性/monitoring/最佳实践/k8s-monitoring-guide|Kubernetes 监控最佳实践]]
