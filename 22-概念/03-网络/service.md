---
title: Service
summary: Kubernetes Service 是一种抽象，定义了一组 Pod 的逻辑集合和一个访问它们的策略。
category: concepts
tags:
- core-concept
- k8s
- networking
- visibility/public
tier: supporting
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---


# Service

## 概述

Kubernetes Service 是一种将一组逻辑相关的 Pod 暴露为网络服务的抽象。由于 Pod 是易变的（IP 随重建变化、因伸缩增减），直接依赖 Pod IP 既不现实也不稳定。Service 为一组（通过 label selector 匹配的）Pod 提供一个稳定的 ClusterIP、DNS 名称和负载均衡，使调用方无需关心后端 Pod 的变化。

## 架构与工作原理

```
  客户端 (Pod/外部)
        │
        ▼
  ┌───────────────────────┐
  │ Service (ClusterIP)   │   稳定 VIP + DNS: webapp.ns.svc.cluster.local
  │ selector: app=webapp  │
  └──────────┬────────────┘
             │ kube-proxy 维护转发规则
             ▼
  ┌──────── Endpoints / EndpointSlice ───────────┐
  │  10.244.1.5:8080   10.244.2.9:8080   10.244.3.4:8080 │
  └──────────────────────────────────────────────┘
             │
             ▼  （iptables / IPVS / eBPF）
        Pod 副本们
```

**工作流**：
1. Service 通过 `selector` 匹配到对应 Pod，Endpoints Controller 持续维护 Endpoints / EndpointSlice 列表（仅健康且 readiness 通过的 Pod）。
2. kube-proxy 在每个节点上监听 Service/Endpoint 变化，把 VIP 流量通过 **iptables**（默认）或 **IPVS** 规则 DNAT 到后端 Pod。
3. 集群内 DNS（CoreDNS）自动为每个 Service 生成 A/AAAA/SRV 记录，形如 `<service>.<namespace>.svc.cluster.local`。

**Service 类型（spec.type）**：

| 类型 | 暴露范围 | 特点 |
|------|----------|------|
| `ClusterIP`（默认） | 集群内 | 分配内部 VIP，仅集群可达 |
| `NodePort` | 集群外（节点 IP:Port） | 在所有节点开 30000-32767 端口 |
| `LoadBalancer` | 公网 | 由云厂商创建负载均衡器并回调 Service |
| `ExternalName` | CNAME 别名 | 将集群内 DNS 指向外部域名，无代理 |

## 关键组件与特性

| 特性 | 说明 |
|------|------|
| label selector | 通过标签匹配后端 Pod，支持 `matchLabels` / `matchExpressions` |
| ClusterIP | 集群内稳定虚拟 IP（可设 `None` 创建 Headless Service） |
| Endpoints / EndpointSlice | 实际后端 Pod 列表，EndpointSlice 支持大规模扩展 |
| sessionAffinity | ClientIP 会话保持，默认 None |
| externalTrafficPolicy | Cluster（默认）/ Local，Local 保留源 IP 但需逐节点有 Pod |
| multi-port | 一个 Service 暴露多端口，每端口独立 name + targetPort |
| topology aware hints | 按拓扑（区域/节点）优先路由同地域后端 |

## 配置示例

```yaml
---
apiVersion: v1
kind: Service
metadata:
  name: webapp
  namespace: production
  labels:
    app: webapp
spec:
  type: ClusterIP
  selector:
    app: webapp
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
  - name: metrics
    port: 9090
    targetPort: metrics
  sessionAffinity: None
---
# Headless Service：StatefulSet 常用，每个 Pod 有独立 DNS
apiVersion: v1
kind: Service
metadata:
  name: db-headless
  namespace: production
spec:
  clusterIP: None
  selector:
    app: postgres
  ports:
  - port: 5432
---
# LoadBalancer：对外暴露
apiVersion: v1
kind: Service
metadata:
  name: webapp-lb
  annotations:
    service.beta.kubernetes.io/azure-load-balancer-internal: "false"
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local
  selector:
    app: webapp
  ports:
  - port: 443
    targetPort: 8443
```

## 常用操作与命令

```bash
# 查看 Service 及其后端
kubectl get svc -n production
kubectl describe svc webapp
kubectl get endpoints webapp -n production
kubectl get endpointslices -n production -l kubernetes.io/service-name=webapp

# DNS 解析
kubectl run dns-test --image=busybox:1.36 -it --rm --restart=Never -- \
  nslookup webapp.production.svc.cluster.local

# 临时从集群内访问
kubectl run curl --image=curlimages/curl -it --rm --restart=Never -- \
  curl http://webapp.production:80/healthz

# 端口转发到本地
kubectl port-forward svc/webapp 8080:80 -n production
```

## 最佳实践

1. **优先使用 ClusterIP + Ingress**：对外暴露交给 Ingress/网关，NodePort/LoadBalancer 只在必要时使用。
2. **使用 readinessProbe 保证 Endpoints 准确**：只有就绪的 Pod 才进 Endpoints，避免流量打到未就绪实例。
3. **多端口务必命名**：`port.name` 便于协议推断（Prometheus 抓 metrics、Istion 做 mTLS）。
4. **保留源 IP 用 externalTrafficPolicy: Local**：代价是流量只在本节点后端间转发，需保证每节点有 Pod。
5. **Headless + StatefulSet**：有状态服务用 `clusterIP: None` 获取每副本稳定 DNS（`pod-0.svc.ns.svc`）。

## 常见陷阱

- **Service 无 Endpoints**：selector 拼写与 Pod label 不一致，或 Pod readiness 持续失败。
- **DNS 解析失败**：CoreDNS Pod 异常或 ndots 配置导致超长解析，检查 `/etc/resolv.conf`。
- **NodePort 源 IP 被 SNAT**：默认 externalTrafficPolicy=Cluster 会做 SNAT，丢失真实客户端 IP。
- **会话保持导致负载不均**：sessionAffinity=ClientIP 在 NAT 后地址集中时流量倾斜。
- **Service 与 Mesh 冲突**：启用 Istio/Linkerd 时注意协议探测，建议显式声明端口 name（http/tcp）。

## 源码实现分析

### EndpointSlice Controller

```go
// k8s.io/kubernetes/pkg/controller/endpointslice/endpointslice_controller.go
// EndpointSlice Controller 监听 Pod 变化，更新 EndpointSlice
func (c *Controller) syncPod(ctx context.Context, key string) error {
    // 1. 获取 Pod 信息
    pod := c.getPod(key)
    // 2. 查找匹配的 Service（通过 selector）
    services := c.getMatchingServices(pod)
    for _, svc := range services {
        // 3. 检查 Pod 是否 Ready
        ready := isPodReady(pod)
        // 4. 更新 EndpointSlice
        slice := c.getOrCreateEndpointSlice(svc)
        if ready {
            slice.Endpoints = append(slice.Endpoints, v1.Endpoint{
                Addresses: []string{pod.Status.PodIP},
                Conditions: v1.EndpointConditions{Ready: &ready},
                NodeName:   &pod.Spec.NodeName,
                Zone:       &pod.Labels["topology.kubernetes.io/zone"],
            })
        }
        c.client.Update(ctx, slice)
    }
}
```

### Service 类型与数据路径

```
┌───────────────────────────────────────────────────────────┐
│          Service 类型与数据路径                        │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  ClusterIP (默认):                                       │
│  Pod → ClusterIP:port → kube-proxy → Pod IP:targetPort │
│                                                           │
│  NodePort:                                               │
│  外部 → NodeIP:30000-32767 → kube-proxy → Pod IP     │
│                                                           │
│  LoadBalancer:                                           │
│  外部 → 云 LB → NodePort → kube-proxy → Pod IP       │
│                                                           │
│  ExternalName:                                           │
│  Pod → DNS CNAME → 外部域名 (无 kube-proxy)          │
│                                                           │
│  Headless (clusterIP: None):                             │
│  Pod → DNS A记录 → 直接返回所有 Pod IP              │
│                                                           │
│  关键组件:                                               │
│  • EndpointSlice Controller: Pod → EndpointSlice       │
│  • kube-proxy: Service → iptables/IPVS 规则          │
│  • CoreDNS: Service 名 → ClusterIP 解析             │
└───────────────────────────────────────────────────────────┘
```

### 生产配置示例（🟡 部署到集群）

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web-app
  annotations:
    # 云 LB 注解（AWS 示例）
    service.beta.kubernetes.io/aws-load-balancer-type: nlb
    service.beta.kubernetes.io/aws-load-balancer-cross-zone-load-balancing-enabled: "true"
spec:
  type: LoadBalancer
  externalTrafficPolicy: Local  # 保留源 IP
  selector:
    app: web-app
  ports:
  - name: http
    port: 80
    targetPort: 8080
    protocol: TCP
  - name: metrics
    port: 9090
    targetPort: 9090
  sessionAffinity: None
```

## 面试要点

1. **Service 的四种类型及适用场景？**
   - ClusterIP：内部服务间通信（默认）
   - NodePort：无云 LB 的外部访问（开发/测试）
   - LoadBalancer：生产环境外部流量
   - ExternalName：外部服务 DNS 别名

2. **Service 如何发现后端 Pod？**
   - EndpointSlice Controller 监听 Pod 变化
   - 通过 Service selector 匹配 Pod labels
   - 只有 Ready 的 Pod 才加入 Endpoints

3. **externalTrafficPolicy Local vs Cluster？**
   - Cluster：流量可跨节点转发，会 SNAT 丢失源 IP
   - Local：只转发到本节点 Pod，保留源 IP
   - Local 需保证每节点有 Pod，否则流量丢失

4. **Service 与 Ingress 的区别？**
   - Service：L4（TCP/UDP）负载均衡
   - Ingress：L7（HTTP）路由、TLS 终止、路径匹配
   - 生产：Service + Ingress 组合使用

## 相关概念

- [[22-概念/01-核心架构/kubernetes.md|Kubernetes]]
- [[22-概念/02-工作负载/pods.md|Pod]] — Service 的后端
- [[22-概念/03-网络/ingress.md|Ingress]] — 七层入口
- [[22-概念/03-网络/network-policy.md|NetworkPolicy]] — 流量控制
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构概览]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
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
