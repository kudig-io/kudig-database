---
title: Kubernetes Service 与 Ingress 网络面经
summary: 系统梳理 ClusterIP、NodePort、LoadBalancer、Ingress、Ingress Controller、kube-proxy 与 iptables/IPVS 的关系、流量路径、配置示例和排障要点。
category: interview
tags:
- kubernetes
- k8s
- service
- ingress
- networking
- interview
tier: core
created: '2026-08-28'
last_updated: 2026-08
difficulty: intermediate
reading_level: intermediate
audience:
- 后端工程师
- SRE
- 平台工程师
- 云原生面试准备者
estimated_read_time: 15min
intent_queries:
- Kubernetes Service 和 Ingress 面试怎么答
- ClusterIP NodePort LoadBalancer Ingress 有什么区别
- kube-proxy iptables IPVS 如何转发 Service 流量
trigger_keywords:
- ClusterIP
- NodePort
- LoadBalancer
- Ingress
- Ingress Controller
- kube-proxy
- iptables
- IPVS
prerequisites:
- kubectl-basics
- pod-networking-basics
---

# Kubernetes Service 与 Ingress 网络面经

## 一句话总览

Kubernetes Service 解决的是 **Pod IP 不稳定时如何提供稳定访问入口**；ClusterIP、NodePort、LoadBalancer 是 Service 暴露范围逐层扩大的三种形态；Ingress 解决的是 **HTTP/HTTPS 多域名、多路径如何复用一个入口做七层路由**；Ingress Controller 才是真正处理流量的反向代理。

最重要的心智模型：

```text
外部用户
  ↓
Ingress / LoadBalancer / NodePort 等外部入口
  ↓
Service 稳定抽象
  ↓
EndpointSlice / Endpoints 后端列表
  ↓
Pod IP:targetPort
```

如果把 Service 类型看成套娃：

```text
LoadBalancer
  └── NodePort
        └── ClusterIP
              └── EndpointSlice / Pod IP
```

也就是说：

- `ClusterIP` 只提供集群内访问入口；
- `NodePort` 在 ClusterIP 基础上，在每个 Node 上打开一个端口；
- `LoadBalancer` 在 NodePort 基础上，让云厂商创建外部负载均衡器；
- `Ingress` 不是 Service 类型，它是七层路由规则，需要 Ingress Controller 执行。

---

## 高频面试题 1：ClusterIP、NodePort、LoadBalancer、Ingress 分别是什么？

### 标准回答

`ClusterIP` 是 Service 的默认类型，它给一组 Pod 提供一个稳定的集群内虚拟 IP。集群内的 Pod 可以通过 `serviceName.namespace.svc.cluster.local` 或 ClusterIP 访问后端 Pod。

`NodePort` 是在 ClusterIP 的基础上，把同一个 Service 暴露到每个 Node 的固定端口上。外部客户端可以访问 `任意NodeIP:NodePort`，再由 kube-proxy 规则转发到后端 Pod。

`LoadBalancer` 是在 NodePort 的基础上，让云厂商创建一个外部负载均衡器。用户访问云 LB 的公网或内网 IP，LB 再把流量转发到各 Node 的 NodePort。

`Ingress` 是 Kubernetes 的七层 HTTP/HTTPS 路由资源，它描述域名、路径与后端 Service 的映射关系。Ingress 自身不处理流量，必须由 Ingress Controller 监听 Ingress 资源并执行路由。

### 对比表

| 概念 | 类型 | 作用 | 访问范围 | 典型用途 |
|---|---|---|---|---|
| Pod IP | CNI 分配的真实 IP | Pod 间直接通信 | 集群网络内 | 调试、Service 后端 |
| ClusterIP | Service 虚拟 IP | 稳定访问一组 Pod | 集群内部 | 微服务内部调用 |
| NodePort | Service 类型 | 暴露每个节点端口 | 集群外部可访问节点时 | 测试、自建 LB 后端 |
| LoadBalancer | Service 类型 | 申请云 LB | 公网或 VPC 内 | 暴露单个 TCP/UDP 服务 |
| Ingress | networking.k8s.io 资源 | 声明 HTTP 路由规则 | 取决于 Controller 暴露方式 | 多域名/多路径统一入口 |
| Ingress Controller | Deployment/DaemonSet | 反向代理与路由执行器 | 通常通过 LB/NodePort 暴露 | Nginx/Envoy/Traefik 网关 |

### 面试加分点

`type: NodePort` 的 Service 仍然有 ClusterIP；`type: LoadBalancer` 的 Service 通常也会分配 NodePort。它们不是互斥关系，而是暴露范围逐层叠加。

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web
spec:
  type: LoadBalancer
  selector:
    app: web
  ports:
  - port: 80        # Service 暴露端口，ClusterIP:80
    targetPort: 8080 # Pod 容器端口
    nodePort: 30080  # NodeIP:30080
```

---

## 高频面试题 2：Service 是怎么把流量转发到 Pod 的？

### 标准回答

Service 本身不是一个长期运行的代理进程。创建 Service 后，控制面会根据 Service 的 selector 找到匹配 Pod，并生成 EndpointSlice；每个节点上的 kube-proxy 监听 Service 和 EndpointSlice 变化，把对应的转发规则写入本机 iptables 或 IPVS。真正转发数据包的是 Linux 内核的 netfilter/IPVS，而不是 kube-proxy 进程本身。

### 控制面路径

```text
kubectl apply Service
  ↓
API Server 保存 Service
  ↓
EndpointSlice Controller 根据 selector 匹配 Ready Pod
  ↓
生成 EndpointSlice: PodIP:targetPort 列表
  ↓
kube-proxy watch Service / EndpointSlice
  ↓
在每个 Node 写入 iptables/IPVS 规则
```

### 数据面路径：Pod 访问 ClusterIP

```text
Pod A
  ↓ 访问 web-svc:80，DNS 解析为 ClusterIP
ClusterIP:80
  ↓ 命中本节点 iptables/IPVS 规则
DNAT 到某个 PodIP:targetPort
  ↓ 经 CNI 网络转发
Pod B
```

以 iptables 模式为例，核心动作是：

```text
目的地址: ClusterIP:port
  ↓
匹配 KUBE-SERVICES 规则
  ↓
进入某个 KUBE-SVC-* 链选择后端
  ↓
进入 KUBE-SEP-* 链做 DNAT
  ↓
目的地址改写为 PodIP:targetPort
```

### 容易混淆点

ClusterIP 通常不是某块网卡真实持有的 IP。它更像一个被 kube-proxy 写进内核规则里的虚拟入口。访问 ClusterIP 时，数据包在节点上被改写成后端 Pod IP，再由 CNI 负责跨节点或本节点转发。

---

## 高频面试题 3：NodePort 的完整流量路径是什么？

### 标准回答

外部客户端访问 `NodeIP:NodePort` 后，流量到达某个节点。该节点上的 kube-proxy 规则会匹配 NodePort，并选择一个后端 Pod，然后把目的地址 DNAT 为 PodIP:targetPort。如果后端 Pod 在本节点，则直接转发；如果在其他节点，则通过集群网络转发到目标节点上的 Pod。

```text
外部客户端
  ↓
NodeIP:NodePort
  ↓
当前 Node 的 iptables/IPVS 规则
  ↓
DNAT 到某个 PodIP:targetPort
  ↓
CNI 网络
  ↓
目标 Pod
```

### 关键补充：源 IP 保留

默认情况下，如果外部流量进入 NodePort 后被转发到其他节点的 Pod，通常会发生 SNAT，Pod 看到的源 IP 可能是节点 IP，而不是真实客户端 IP。

如果希望保留真实客户端 IP，可以设置：

```yaml
spec:
  externalTrafficPolicy: Local
```

但它有代价：只有本节点存在后端 Pod 时才会接收流量；如果 LB 把请求打到没有本地 Pod 的节点，请求可能失败。因此通常需要配合云 LB 的健康检查。

---

## 高频面试题 4：iptables 和 IPVS 模式有什么区别？

### 标准回答

iptables 模式通过大量规则链和概率匹配实现 Service 转发，规则规模增大后匹配成本更高。IPVS 模式使用 Linux 内核的 IPVS 负载均衡能力，基于哈希表查找和成熟的调度算法，更适合大规模 Service 场景。

| 维度 | iptables | IPVS |
|---|---|---|
| 实现方式 | netfilter 规则链 | Linux IPVS 内核模块 |
| 匹配复杂度 | 规则链线性匹配 | 哈希查找 |
| 负载均衡 | 随机概率 | rr、lc、sh 等算法 |
| 大规模性能 | 规则多时变差 | 更稳定 |
| 排查命令 | `iptables-save` | `ipvsadm -Ln` |

面试时可以补一句：现在也有 Cilium 等 eBPF 数据面可以替代 kube-proxy，把 Service 转发逻辑放到 eBPF 中实现。

---

## 高频面试题 5：Ingress 和 Ingress Controller 的区别是什么？

### 标准回答

Ingress 是 Kubernetes API 里的声明式路由资源，只描述 HTTP/HTTPS 的路由规则，例如哪个域名、哪个路径转发到哪个 Service。Ingress Controller 是真正执行这些规则的控制器和反向代理，它会 watch Ingress、Service、EndpointSlice 等资源，并动态生成 Nginx、Envoy、Traefik 等代理配置。

一句话：**Ingress 是规则，Ingress Controller 是执行规则的代理进程。**

```text
Ingress YAML
  ↓ 被 Controller watch
Ingress Controller 生成代理配置
  ↓
外部请求进入 Controller
  ↓ 按 Host/Path 匹配规则
转发到后端 Service / Pod
```

如果集群里只有 Ingress 资源，没有安装对应的 Ingress Controller，外部访问不会自动生效。

---

## 高频面试题 6：Ingress Controller 和 Service 如何协同？

### 标准回答

Ingress Controller 自己通常也是一组 Pod，它需要通过一个 Service 暴露出来，常见类型是 `LoadBalancer` 或 `NodePort`。外部请求先进入 Ingress Controller，再由 Controller 按 Ingress 规则转发到后端业务 Service 对应的 Pod。

完整路径通常是：

```text
用户浏览器
  ↓ DNS 解析域名到云 LB
云 LoadBalancer
  ↓
Ingress Controller Service 的 NodePort
  ↓
Ingress Controller Pod
  ↓ 根据 Ingress host/path 匹配规则
后端 Service
  ↓ EndpointSlice 中的 PodIP:targetPort
业务 Pod
```

很多 Ingress Controller 在实际转发时会直接使用 EndpointSlice 里的 Pod IP 作为 upstream，而不是把请求再发给 ClusterIP。原因是 Controller 需要做更细粒度的七层负载均衡、健康检查、会话保持、灰度路由等能力。此时 Service 更像服务发现入口，提供后端 Pod 列表和端口定义。

---

## 配置示例：Deployment + Service + Ingress

### Deployment

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web
  template:
    metadata:
      labels:
        app: web
    spec:
      containers:
      - name: web
        image: nginx:1.27
        ports:
        - containerPort: 80
```

### ClusterIP Service

```yaml
apiVersion: v1
kind: Service
metadata:
  name: web-svc
spec:
  selector:
    app: web
  ports:
  - name: http
    port: 80
    targetPort: 80
```

### Ingress

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: web-ingress
spec:
  ingressClassName: nginx
  rules:
  - host: web.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-svc
            port:
              number: 80
```

### 验证命令

```bash
kubectl get deploy web
kubectl get pod -l app=web -o wide
kubectl get svc web-svc
kubectl get endpointslice -l kubernetes.io/service-name=web-svc
kubectl get ingress web-ingress
```

集群内验证 Service：

```bash
kubectl run curl --rm -it --image=curlimages/curl -- curl -s http://web-svc
```

外部验证 Ingress：

```bash
curl -H 'Host: web.example.com' http://<INGRESS_ADDRESS>/
```

---

## 排障思路：从后往前、从内到外

### 1. Pod 是否可用

```bash
kubectl get pod -l app=web -o wide
kubectl describe pod <pod-name>
kubectl logs <pod-name>
```

关注点：

- Pod 是否 Running；
- readinessProbe 是否通过；
- 容器实际监听端口是否等于 Service 的 `targetPort`；
- NetworkPolicy 是否拦截。

### 2. Service 是否选中后端

```bash
kubectl get svc web-svc -o yaml
kubectl get endpointslice -l kubernetes.io/service-name=web-svc -o wide
```

如果 EndpointSlice 为空，优先检查：

```bash
kubectl get pod --show-labels
kubectl describe svc web-svc
```

常见原因是 Service 的 selector 和 Pod label 不匹配，或者 Pod 未 Ready。

### 3. ClusterIP 是否能访问

```bash
kubectl run curl --rm -it --image=curlimages/curl -- curl -v http://web-svc
kubectl run curl --rm -it --image=curlimages/curl -- curl -v http://<ClusterIP>
```

如果 Service 名称不通但 ClusterIP 通，检查 CoreDNS；如果二者都不通，检查 EndpointSlice、targetPort、NetworkPolicy、kube-proxy。

### 4. NodePort 是否能访问

```bash
kubectl get svc <service-name>
curl http://<NodeIP>:<NodePort>
```

常见原因：

- 节点安全组或防火墙未放行 NodePort；
- `externalTrafficPolicy: Local` 但该节点没有本地后端 Pod；
- kube-proxy 异常或节点规则未同步。

### 5. Ingress 是否生效

```bash
kubectl get ingress
kubectl describe ingress web-ingress
kubectl get ingressclass
kubectl logs -n ingress-nginx -l app.kubernetes.io/component=controller
```

常见现象：

| 现象 | 可能原因 |
|---|---|
| Ingress ADDRESS 为空 | Controller 未安装、IngressClass 不匹配、LB 未创建成功 |
| 返回 404 | Host 头或 path 不匹配 |
| 返回 502/503 | 后端 Service 无 Endpoint、Pod 未 Ready、端口不匹配 |
| TLS 报错 | Secret 不存在、证书域名不匹配、TLS 配置错误 |

---

## 面试追问与回答模板

### Q1：为什么 Service 需要 selector？

Service 用 selector 选择一组 Pod，并由 EndpointSlice Controller 生成后端列表。Service 不直接绑定 Pod 名称，因为 Pod 会频繁重建，名字和 IP 都不稳定；label 才是稳定的分组语义。

### Q2：Service 的 `port`、`targetPort`、`nodePort` 有什么区别？

- `port`：Service 对外暴露的端口，例如 `ClusterIP:80`；
- `targetPort`：后端 Pod 容器实际监听的端口，例如 `PodIP:8080`；
- `nodePort`：NodePort 类型在每个 Node 上开放的端口，例如 `NodeIP:30080`。

### Q3：为什么访问任意 Node 的 NodePort 都可能到达任意节点上的 Pod？

因为每个节点都运行 kube-proxy，并且都维护同一份 Service 转发规则。请求进入任意 Node 后，本节点规则都会选择一个后端 Pod。如果目标 Pod 在其他节点，流量再通过 CNI 网络跨节点转发。

### Q4：Ingress 后面为什么还要 Service，不能直接写 Pod 吗？

Pod 是短生命周期对象，IP 不稳定；Service 提供稳定的服务发现抽象，并通过 EndpointSlice 维护当前可用 Pod 列表。Ingress 引用 Service，可以避免把动态 Pod 细节暴露给七层路由规则。

### Q5：Ingress Controller 是否一定通过 LoadBalancer 暴露？

不一定。云上常见用 `type: LoadBalancer`；裸金属或测试环境也可以用 `NodePort`、`hostNetwork`、MetalLB、外部硬件 LB 等方式暴露 Controller。

### Q6：ClusterIP 能不能被集群外访问？

通常不能。ClusterIP 只在集群节点内的 Service 转发规则中有效，外部网络没有到 ClusterIP 网段的路由，也不会命中节点上的 kube-proxy 规则。

---

## 记忆口诀

```text
Pod IP 会变，Service 给稳定入口。
ClusterIP 管集群内，NodePort 开每台节点门。
LoadBalancer 找云厂商要公网入口。
Ingress 只写七层规则，Controller 才真正接流量。
kube-proxy 不搬包，只写规则；真正转发靠内核。
```

更短的版本：

```text
Service 管四层，Ingress 管七层；
Ingress 是配置，Controller 是代理；
ClusterIP 是内网 VIP，NodePort 是节点端口，LoadBalancer 是云上入口。
```

---

## 推荐学习路径

1. 先掌握 Pod 网络：Pod IP 如何由 CNI 分配，跨节点 Pod 如何通信。
2. 再掌握 Service：ClusterIP、NodePort、LoadBalancer、EndpointSlice、kube-proxy。
3. 接着看 DNS：Service 名称如何解析为 ClusterIP。
4. 再看 Ingress：Ingress 资源、IngressClass、Ingress Controller、TLS、七层路由。
5. 最后看生产排障：Endpoint 为空、端口不匹配、Ingress 404/502/503、源 IP 保留、NetworkPolicy 拦截。

本仓库可按以下顺序复习：

- [[05-网络/01-K8s网络核心/00-network-in-nutshell.md|Kubernetes 网络速览]]
- [[05-网络/01-K8s网络核心/01-network-architecture-overview.md|Kubernetes 网络架构总览]]
- [[05-网络/01-K8s网络核心/06-service-concepts-types.md|Service 概念与类型]]
- [[05-网络/01-K8s网络核心/07-service-implementation-details.md|Service 实现细节]]
- [[05-网络/01-K8s网络核心/09-kube-proxy-modes-performance.md|kube-proxy 模式与性能]]
- [[05-网络/01-K8s网络核心/11-dns-service-discovery-coredns.md|DNS 与 Service Discovery]]
- [[05-网络/01-K8s网络核心/19-ingress-fundamentals.md|Ingress 基础]]
- [[05-网络/01-K8s网络核心/20-ingress-controller-deep-dive.md|Ingress Controller 深入]]
- [[05-网络/01-K8s网络核心/25-ingress-monitoring-troubleshooting.md|Ingress 监控与排障]]

---

## 最终面试总结

如果面试官问 Kubernetes Service 和 Ingress 的关系，可以这样收束：

> Service 是 Kubernetes 中面向 Pod 的稳定四层访问抽象。ClusterIP 提供集群内虚拟 IP，NodePort 在每个节点上开放端口，LoadBalancer 借助云厂商暴露外部入口。Service 背后由 EndpointSlice 维护真实 Pod 后端，由 kube-proxy 把规则写入 iptables 或 IPVS，最终由内核完成 DNAT 转发。Ingress 则是七层 HTTP/HTTPS 路由规则，本身不处理流量，必须由 Ingress Controller 执行。Ingress Controller 通常通过 LoadBalancer 或 NodePort 暴露自己，接收外部请求后按域名和路径匹配 Ingress 规则，再转发到后端 Service 对应的 Pod。

