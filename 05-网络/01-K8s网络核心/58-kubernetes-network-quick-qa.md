---
title: K8s 网络快问快答与面试模拟对话
summary: 42 道 Kubernetes 网络口述自测题（Service/Ingress/CNI/DNS/NetworkPolicy/排障）与四轮面试官追问模拟脚本，配套理论面经使用。
category: interview
tags:
- kubernetes
- k8s
- networking
- interview
- quick-qa
tier: core
created: '2026-08-31'
last_updated: 2026-08
difficulty: intermediate
audience:
- 后端工程师
- SRE
- 平台工程师
- 云原生面试准备者
estimated_read_time: 12min
intent_queries:
- Kubernetes 网络快问快答
- K8s Service Ingress 口述自测
- K8s 网络面试模拟对话
trigger_keywords:
- 快问快答
- 面试模拟
- ClusterIP
- NodePort
- Ingress
- kube-proxy
prerequisites:
- kubernetes-service-ingress-interview
---

# K8s 网络快问快答与面试模拟对话

理论体系见 [[05-网络/01-K8s网络核心/57-kubernetes-service-ingress-interview.md|Kubernetes Service 与 Ingress 网络面经]]。建议先读理论，再用本文口述自测；两者配合使用，先理解再刷题。

---

## K8s 网络快问快答

建议先遮住答案口述，再对照关键词补全。

### 1. Service 解决什么问题？

Service 为一组动态变化的 Pod 提供稳定的访问入口。Pod 重建、扩缩容时 IP 会变化，但 Service 名称和 ClusterIP 通常保持稳定。

### 2. ClusterIP 是什么？

ClusterIP 是 Service 的集群内虚拟 IP。它通常不属于某块真实网卡，访问它时，节点内核根据 kube-proxy 写入的规则把流量 DNAT 到后端 Pod。

### 3. NodePort 是什么？

NodePort 在 ClusterIP 基础上，在每个 Node 开放一个端口。外部客户端可通过 `NodeIP:NodePort` 访问 Service 后端。

### 4. LoadBalancer 是什么？

LoadBalancer 类型通常让云厂商创建外部负载均衡器，并把外部流量转发到集群节点。传统实现中通常还会分配 NodePort。

### 5. 三种 Service 类型是什么关系？

传统实现可记为：

```text
LoadBalancer
  └── NodePort
        └── ClusterIP
              └── Pod IP
```

### 6. Service 如何找到 Pod？

Service 通过 selector 匹配 Pod label，EndpointSlice Controller 再生成对应的后端地址列表。

### 7. EndpointSlice 是什么？

EndpointSlice 保存 Service 后端的 Pod IP、端口和就绪状态，是传统 Endpoints API 的可扩展替代方案。

### 8. kube-proxy 是流量代理进程吗？

通常不是逐包代理。它负责监听 Service 和 EndpointSlice 变化并维护 iptables/IPVS 规则，真正的数据包转发由 Linux 内核完成。

### 9. iptables 模式如何转发 Service 流量？

请求命中 ClusterIP 和 Service port 后，iptables 规则选择一个后端，并把目的地址 DNAT 为 `PodIP:targetPort`。

### 10. IPVS 相比 iptables 有什么优势？

IPVS 使用内核负载均衡框架和哈希查找，支持更多调度算法，在大量 Service 场景下通常比庞大的 iptables 规则链更高效。

### 11. `port`、`targetPort`、`nodePort` 有什么区别？

- `port`：Service 暴露的端口；
- `targetPort`：后端 Pod 实际监听的端口；
- `nodePort`：每个 Node 对外开放的端口。

```text
ClusterIP:80 / NodeIP:30080 → PodIP:8080
```

### 12. Ingress 是 Service 类型吗？

不是。Ingress 是声明 HTTP/HTTPS 七层路由规则的 Kubernetes API 资源。

### 13. Ingress 和 Ingress Controller 有什么区别？

Ingress 是规则，Ingress Controller 是执行规则的控制器和反向代理。没有匹配的 Controller，Ingress 资源不会自动处理流量。

### 14. Ingress Controller 如何暴露？

它本身通常是一组 Pod，可通过 LoadBalancer、NodePort、hostNetwork、MetalLB 或外部硬件负载均衡器暴露。

### 15. Ingress 的完整流量路径是什么？

```text
Client → DNS → LoadBalancer/NodePort
       → Ingress Controller
       → 匹配 Host/Path
       → Service/EndpointSlice
       → Pod
```

### 16. Ingress 为什么还要引用 Service？

Pod 生命周期短且 IP 不稳定。Service 提供稳定后端抽象，EndpointSlice 则维护实时 Pod 地址。部分 Controller 最终会直接把请求发给 EndpointSlice 中的 Pod IP。

### 17. DNS 和 Service 有什么关系？

CoreDNS 把 Service 名称解析为 ClusterIP。客户端访问 ClusterIP 后，再由 Service 数据面规则把请求转发到后端 Pod。

### 18. Service 的完整 DNS 名是什么？

```text
<service>.<namespace>.svc.cluster.local
```

同一 Namespace 内通常可以只使用 Service 名称。

### 19. Headless Service 是什么？

Headless Service 设置 `clusterIP: None`，不提供 ClusterIP。DNS 通常直接返回后端地址，常用于 StatefulSet 和需要自主服务发现的应用。

### 20. ExternalName Service 是什么？

ExternalName 把集群内 Service 名称映射为外部 DNS 名称，本质上依赖 DNS CNAME，不创建 ClusterIP，也不进行数据面负载均衡。

### 21. Kubernetes 对 Pod 网络有什么基本要求？

每个 Pod 拥有独立 IP；在标准网络模型中，Pod 之间应能直接通信，不需要应用感知的 NAT。具体连通由 CNI 实现。

### 22. CNI 负责什么？

CNI 插件负责创建 Pod 网络接口、分配 IP、配置路由或隧道，并实现跨节点 Pod 网络。常见实现包括 Calico、Cilium、Flannel 和 Terway。

### 23. Service 和 CNI 如何分工？

```text
Service/kube-proxy：选择目标 Pod，并实现服务转发
CNI：负责把数据包真正送到目标 Pod
```

### 24. NetworkPolicy 解决什么问题？

NetworkPolicy 声明允许哪些入站和出站通信，相当于面向 Pod 的网络访问控制策略。

### 25. 创建 NetworkPolicy 就一定会生效吗？

不一定。底层 CNI 必须实现 NetworkPolicy，否则资源可能被 API Server 接受，但不会真正执行策略。

### 26. 没有 NetworkPolicy 时默认允许还是拒绝？

默认通常是允许。Pod 被相应方向的 NetworkPolicy 选中后，该方向才进入白名单式隔离状态。

### 27. `externalTrafficPolicy: Local` 有什么作用？

它只把外部流量转发给入口节点上的本地 Endpoint，通常可以保留客户端源 IP；代价是流量分布可能不均，没有本地 Endpoint 的节点也无法承接请求。

### 28. Service 如何实现会话保持？

可设置：

```yaml
spec:
  sessionAffinity: ClientIP
```

同一客户端 IP 会在一定时间内尽量命中同一后端。

### 29. Ingress 返回 404 怎么排查？

优先检查 Host、Path、`pathType`、IngressClass 是否匹配，以及请求是否携带正确 Host 头。

```bash
kubectl describe ingress <name>
curl -H 'Host: web.example.com' http://<ingress-address>/
```

### 30. Ingress 返回 502/503 怎么排查？

检查 Service 是否有可用 EndpointSlice、Pod 是否 Ready、端口是否匹配，以及 Controller 到后端的网络是否正常。

### 31. ClusterIP 不通时按什么顺序排查？

```text
Pod Running/Ready
  → Service selector
  → EndpointSlice
  → port/targetPort
  → NetworkPolicy
  → kube-proxy/数据面
```

### 32. NodePort 不通时检查什么？

检查 Node IP 可达性、NodePort、安全组和主机防火墙、EndpointSlice、`externalTrafficPolicy` 以及 kube-proxy 规则。

### 33. 为什么 Pod 能通过 Service 名访问服务？

Pod 的 DNS 查询由 CoreDNS 解析，Service 名被解析为 ClusterIP，再由节点 Service 数据面转发到后端 Pod。

### 34. kube-proxy 挂掉后 Service 会立刻全部中断吗？

不一定。已有内核规则可能继续工作，但新的 Service、Endpoint 变化和扩缩容无法及时同步，转发状态会逐渐过期。

### 35. Service selector 写错有什么现象？

EndpointSlice 没有目标后端，Service 请求失败，Ingress 访问后端时常表现为 503。

```bash
kubectl get endpointslice -l kubernetes.io/service-name=<svc>
kubectl get pod --show-labels
```

### 36. readinessProbe 和 Service 有什么关系？

Pod 即使处于 Running，只要未 Ready，通常也不会作为可服务 Endpoint 接收正常 Service 流量。

### 37. `hostNetwork: true` 有什么影响？

Pod 与宿主机共享网络命名空间和 Node IP，网络路径更直接，但会降低隔离性，并带来端口冲突风险。

### 38. hostPort 和 NodePort 有什么区别？

hostPort 把某个 Pod 的端口绑定到它所在节点；NodePort 属于 Service，在每个节点开放端口，并可负载均衡到多个后端 Pod。

### 39. Service 能根据 HTTP Path 路由吗？

普通 Service 是四层抽象，主要依据 IP 和端口，不能根据 Host 或 Path 路由。七层路由应使用 Ingress、Gateway API 或服务网格。

### 40. Gateway API 和 Ingress 有什么关系？

Gateway API 是更具扩展性和角色分工能力的新一代流量管理 API，可表达更丰富的四层和七层路由；Ingress 模型更简单，扩展通常依赖注解。

### 41. 如何完整描述外部请求访问 Pod 的过程？

外部用户通过 DNS 访问负载均衡器，流量进入 Ingress Controller 的 Service 和 Controller Pod。Controller 根据 Ingress 的 Host/Path 规则选择后端 Service，再根据 EndpointSlice 把请求发送到某个 Pod。kube-proxy 负责维护 Service 数据面规则，CNI 负责实际 Pod 网络连通。

### 42. 如何一句话说明 Service、Ingress、Ingress Controller 的关系？

Service 是稳定的四层服务入口，Ingress 是七层路由规则，Ingress Controller 是接收外部请求并执行这些规则的代理。

---

## 面试模拟对话脚本

用法：自己分饰两角，先遮住"候选人"部分口述作答，再对照参考回答和点评复盘。每题按真实面试节奏控制在 1-2 分钟内，追问链尽量不跳步。

### 场景一：开场热身——Service 基础

**面试官**：先聊聊 Kubernetes 的 Service 吧，你在项目里用过哪些类型？

**候选人（参考）**：用得最多的是 ClusterIP，做服务间的内部调用，比如订单服务访问支付服务。对外暴露 HTTP 服务我们走 Ingress，由统一入口按域名和路径分流。NodePort 一般用于临时调试，或者作为自建环境下外部负载均衡器的后端；云上需要直接暴露 TCP 服务时会用 LoadBalancer。

**点评**：
- ✅ 亮点：把每种类型和真实使用场景绑定，证明是"用过"而不是"背过"。
- ⚠️ 风险：如果只报类型名词，面试官会立刻转入原理追问，要准备好场景二。
- 🎯 面试官意图：快速定位实际经验层次，决定后面问概念还是问原理。

**面试官（追问）**：那 NodePort 和 ClusterIP 是两种独立的 Service 吗？

**候选人（参考）**：不是独立的，是叠加关系。创建 NodePort 类型的 Service，它同时会有 ClusterIP；LoadBalancer 通常还会带 NodePort。可以理解成套娃：LoadBalancer 包着 NodePort，NodePort 包着 ClusterIP，ClusterIP 后面才是真实的 Pod。

**点评**：
- ✅ 亮点：主动否定"并列关系"这个常见误区，给出清晰的层次结构。
- 🎯 面试官意图：高频送分题，回答犹豫会直接给整场面试定基调。

---

### 场景二：数据面深挖——kube-proxy 与转发原理

**面试官**：你说访问 ClusterIP 会转发到 Pod，那这个 ClusterIP 到底是谁持有的？在哪个网卡上？

**候选人（参考）**：这是个最容易答错的点。其实没有任何网卡持有 ClusterIP，ping 它通常也是不通的，这是正常现象。它只存在于每个节点上 kube-proxy 写入的转发规则里。数据包到达节点内核后，命中规则被 DNAT 改写成某个后端 Pod 的 IP。真正"存在"的是这条 NAT 规则，而不是一个 IP 地址。

**点评**：
- ✅ 亮点：敢于否定直觉认知，并主动解释"ping 不通是正常的"，这种表述非常加分。
- 🎯 面试官意图：鉴别候选人是背了定义，还是真的理解 Service 是内核 NAT 规则这一本质。

**面试官（追问）**：那 kube-proxy 在数据路径上吗？每个包都经过它吗？

**候选人（参考）**：不在。kube-proxy 是控制面角色，它 watch Service 和 EndpointSlice 的变化，把规则写进本节点的 iptables 或 IPVS，然后就退场了。之后每一个包的转发都是 Linux 内核在数据面完成的。kube-proxy 挂了，已有规则短期内甚至还能继续工作，只是新的 Service 和 Endpoint 变化无法同步。

**点评**：
- ✅ 亮点：区分了控制面（写规则）和数据面（转发），并顺带回答了"kube-proxy 挂了会怎样"。
- ⚠️ 风险：不要说成"流量经过 kube-proxy 代理转发"，这是经典错误答案。

**面试官（追问）**：iptables 模式下，一个请求进来之后具体发生了什么？

**候选人（参考）**：请求的目的地址是 ClusterIP 加 Service 端口。内核 netfilter 里先命中 KUBE-SERVICES 相关规则，然后进入这个 Service 对应的链做后端选择——多个 Pod 时用概率模块做随机负载均衡，最后在 SEP 链里执行 DNAT，目的地址被改写成 PodIP 加 targetPort，之后数据包按普通 Pod 间通信走 CNI 网络送出去。回程包由 conntrack 自动做反向转换，客户端完全无感知。

**点评**：
- ✅ 亮点：说出"概率负载均衡"和 conntrack 回程两个细节，属于超出预期的深度。
- 🎯 面试官意图：检验是否真的看过 iptables 规则，还是只知道"DNAT"一个词。

**面试官（追问）**：集群里 Service 有几千个的时候，iptables 模式会怎么样？

**候选人（参考）**：iptables 规则是线性匹配的，规则数量上去之后，每个包的匹配开销会明显增加，规则同步也会变慢。所以大规模集群一般用 IPVS 模式，它基于哈希表查找，还支持 rr、最小连接数这些成熟算法。再往前走一步，Cilium 这类 eBPF 数据面甚至可以完全替代 kube-proxy。

**点评**：
- ✅ 亮点：性能问题答到"线性匹配对哈希查找"就够面试用了，带上 eBPF 是锦上添花。
- 🎯 面试官意图：考察规模意识，判断有没有维护过大集群。

---

### 场景三：实战排障——Ingress 返回 503

**面试官**：线上反馈你们的一个网站突然 503 了，入口是 Ingress，说说你怎么排查。

**候选人（参考）**：我会从后端往前逐层验证。第一步看 Pod：是不是 Running、readinessProbe 有没有过、容器实际监听端口对不对。第二步看 Service 有没有选中 Pod：查 EndpointSlice，如果为空，大概率是 selector 和 Pod label 不匹配，或者 Pod 没 Ready。第三步在集群内直接 curl Service 名和 ClusterIP 验证数据面：名称不通查 CoreDNS，都不通查 NetworkPolicy 和 kube-proxy。第四步后端全部正常时，才回头看 Ingress 这层：describe 看 IngressClass 对不对、规则引用的 Service 名和端口对不对，再看 Controller 日志。

**点评**：
- ✅ 亮点：展现"每一层独立可验证"的排查心法，而不是漫无目的地乱翻日志。
- 🎯 面试官意图：503 题考的不是命令背诵，而是排查顺序和方法论。

**面试官（追问）**：怎么区分这是 404 问题还是 503 问题？

**候选人（参考）**：404 是规则层的问题：请求没匹配到任何 Ingress 规则，典型原因是 Host 头没带对、path 或 pathType 不匹配，或者规则压根没被 Controller 生效。503 是后端层的问题：规则匹配上了，但后面没有健康的 Endpoint 可以接。简单说，404 找路由，503 找后端。

**点评**：
- ✅ 亮点：一句话给出"404 找路由，503 找后端"的判断口诀，简洁且准确。

**面试官（追问）**：`kubectl get ingress` 的 ADDRESS 列是空的，说明什么？

**候选人（参考）**：说明没有任何 Ingress Controller 认领这条 Ingress。常见原因有三个：Controller 没装；`spec.ingressClassName` 写错或与 Controller 声明的 IngressClass 不匹配；Controller 自己的 LoadBalancer 还没创建成功。只有 Ingress 资源、没有 Controller，是什么都不会发生的——Ingress 是菜谱，Controller 才是厨师。

**点评**：
- ✅ 亮点：给出三个可操作的检查方向，并用菜谱/厨师类比收尾，表达有记忆点。

---

### 场景四：压力追问——反直觉细节

**面试官**：Pod 里 ping ClusterIP 能通吗？

**候选人（参考）**：iptables 模式下通常不通。kube-proxy 写的规则只针对 TCP/UDP 这类协议按端口匹配做 DNAT，ICMP 请求没有端口概念，命不中规则，也就没人应答。这不是故障，是正常现象。验证 Service 连通性应该用 curl 或 wget 这类基于端口的方式。

**点评**：
- ✅ 亮点：不仅答出结论，还解释了为什么，并给出替代验证手段。
- ⚠️ 注意：IPVS 模式下 VIP 行为有差异，不确定细节就明确限定"iptables 模式下"，不要把话说死。

**面试官**：外部流量经过 NodePort 转到另一个节点的 Pod 后，Pod 看到的源 IP 是谁？

**候选人（参考）**：默认情况下看到的是节点 IP，不是真实客户端 IP。因为跨节点转发时内核做了一次 SNAT，把源地址改成入口节点的地址，以保证回程包能原路回来。想保留客户端源 IP，要把 Service 的 `externalTrafficPolicy` 设成 Local，只把流量交给入口节点上的本地 Pod。代价是负载可能不均，没有本地 Pod 的节点接不了流量，云 LB 的健康检查也要跟着配合。

**点评**：
- ✅ 亮点：先答默认行为，再解释 SNAT 的"为什么"，最后给方案及其代价，结构完整。
- 🎯 面试官意图：检验生产经验的经典题，纯背文档的人往往答不出代价那一半。

**面试官（收尾）**：最后，把从浏览器输入网址到 Pod 收到请求的完整链路讲一遍。

**候选人（参考）**：浏览器发起请求，DNS 把域名解析到云负载均衡器的公网 IP。LB 把流量转到后端节点，打的是 Ingress Controller 的 Service 端口。请求进入 Controller Pod，它根据 Ingress 资源里的 Host 和 Path 规则选中后端 Service，然后从 EndpointSlice 拿到健康的 Pod 列表，把请求发给其中一个 Pod 的 targetPort。这一路上，kube-proxy 负责把 Service 的转发规则维护在每个节点的内核里，CNI 负责让包真正到达目标 Pod。整条链路里，Ingress 只是一份声明式规则，真正搬包的是 Controller、内核和 CNI。

**点评**：
- ✅ 亮点：一镜到底、层次清楚，最后一句点出"规则与执行分离"的本质，是很好的收束。
- 🎯 面试官意图：收尾题看全局串联能力，答好基本可以锁定网络这轮。

---

### 自练检查清单

- 每个回答能否在 60 秒内说完不卡壳？
- 是否至少有一处"踩过坑"式的细节（如 ping 不通 ClusterIP、源 IP 变节点 IP）？
- 追问被问倒时，能否说清"我知道边界在哪，回去会怎么查"？
- 收尾题能否不看稿一镜到底？

---

## 相关链接

- [[05-网络/01-K8s网络核心/57-kubernetes-service-ingress-interview.md|Kubernetes Service 与 Ingress 网络面经（理论体系）]]
- [[05-网络/01-K8s网络核心/00-network-in-nutshell.md|Kubernetes 网络速览]]
- [[05-网络/01-K8s网络核心/19-ingress-fundamentals.md|Ingress 基础]]
