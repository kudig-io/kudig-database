# FAQ 文档
本文档适用于：产品手册、官网页面、课程测验、现场 Q&A。

## 问题1：Kubernetes 网络从外部到 Pod 的完整链路是什么？
- **答案**：典型南北向链路为：Internet → DNS → CDN/WAF → 云负载均衡（SLB/ALB/NLB）→ Kubernetes Ingress（Nginx Ingress / Gateway API / ALB Ingress Controller）→ Kubernetes Service（ClusterIP / NodePort / LoadBalancer / Headless）→ CNI 网络（Flannel/Calico/Cilium/Terway，veth/bridge/ENI）→ Pod 网络（如 10.244.0.0/16）→ 物理网络/VPC（Node Network）。  
  该分层的核心价值是：入口治理（Ingress/Gateway）与服务发现/负载均衡（Service）解耦，底层由 CNI 负责跨节点转发与策略能力。

## 问题2：ClusterIP 适合什么场景？为什么说它是微服务“唯一标准入口”？
- **答案**：ClusterIP 用于**集群内部服务发现与负载均衡**，对调用方暴露稳定的虚拟 IP（ClusterIP），屏蔽后端 Pod 的弹性伸缩、重建、滚动升级带来的 IP 变化。  
  生产建议要点：
  - 通过 `selector` 精确选择后端（必要时可按版本控制）。
  - `publishNotReadyAddresses: false` 确保仅路由到 Ready Pod。
  - 需要会话保持时可用 `sessionAffinity: ClientIP`（如 WebSocket、本地缓存）。

## 问题3：什么时候需要为 ClusterIP 固定 `clusterIP`？
- **答案**：在少数灾备/依赖固定地址的场景（例如外部系统以白名单方式放行某个固定 IP，或迁移过程需要保持地址不变）可固定 `clusterIP`。  
  注意：固定 ClusterIP 会降低灵活性，需确保 Service CIDR 规划充足、且变更有严格流程，否则可能造成 IP 冲突或不可用。

## 问题4：什么是“拓扑感知路由（Topology Aware Hints）”，能带来什么收益？
- **答案**：拓扑感知路由用于让 kube-proxy **优先选择同节点/同可用区（AZ）的 Endpoint**，减少跨 AZ 延迟与费用。  
  启用方式示例：为 Service 添加注解 `service.kubernetes.io/topology-aware-hints: "auto"`。  
  文档给出的生产效果参考：延迟降低 40–60%，跨 AZ 流量成本降低约 70%，适用于缓存、数据库等延迟敏感服务的多 AZ 部署。

## 问题5：NodePort 适合生产吗？有哪些典型用途与风险？
- **答案**：NodePort 通过在每个节点开放端口（默认范围 30000–32767）把服务“直通”到节点网络，**测试环境快速验证**很方便，但生产通常不推荐作为主要暴露方式。  
  典型用途：配合物理四层 LB、临时联调、非云环境快速暴露。  
  主要风险：
  - 需要管理节点暴露面与端口冲突。
  - 如果策略不当可能丢失源 IP 或导致负载不均。

## 问题6：`externalTrafficPolicy: Cluster` 与 `Local` 有什么区别？如何选择？
- **答案**：
  - **Cluster（默认）**：可能发生 SNAT，客户端源 IP 可能丢失；可跨节点转发，负载更均衡。
  - **Local**：保留真实源 IP；仅转发到**本节点**的 Pod（避免跨节点跳转），但可能出现负载不均，且通常需要 `healthCheckNodePort` 配合健康检查。  
  选择建议：需要审计、IP 白名单、真实源 IP 的场景选 `Local`；无源 IP 强需求、追求均衡选 `Cluster`。

## 问题7：LoadBalancer 为什么是生产环境“唯一推荐”的外部暴露方式之一？如何做成本优化？
- **答案**：LoadBalancer 由云厂商自动提供高可用 LB、健康检查与流量切换，运维成本低、可靠性高，适合生产对外服务。  
  成本优化策略（文档建议）：
  - **复用已有 SLB/ALB**：通过注解指定现有 LB ID，避免重复创建。
  - **按流量计费**：低流量业务更划算。
  - **共享带宽包**：多个 LB 共享带宽降低费用。
  - **Ingress 替代多 SLB**：使用“单 SLB + Ingress”让多服务共享入口，节省显著。

## 问题8：Headless Service 是什么？它与 StatefulSet 的关系是什么？
- **答案**：Headless Service 通过 `clusterIP: None` 关闭虚拟 IP，DNS 直接解析到 Pod IP 列表，客户端可直连 Pod，不经 kube-proxy 的服务转发路径，性能更优。  
  与 StatefulSet 配合时：
  - StatefulSet 使用 `serviceName` 绑定 Headless Service。
  - 可获得稳定的 Pod DNS：如 `mysql-0.mysql.namespace.svc.cluster.local` 对应固定实例。  
  适用：MySQL/PostgreSQL 主从、Redis Cluster、ES、Kafka、MongoDB 等有状态集群。

## 问题9：Ingress、Gateway API、ALB Ingress Controller 如何选型？
- **答案**：可按复杂度与环境决策：
  - **自建/多云且简单 HTTP 路由**：Nginx Ingress Controller。
  - **复杂治理、多协议、角色分离**：Gateway API（如 Envoy Gateway）。
  - **云厂商托管且深度集成（ACK）**：ALB Ingress Controller（利用云原生能力、可基于 Header/Cookie 灰度、成本可通过单 ALB 承载多服务优化）。  
  文档给出决策树：需要南北向流量管理 → 云厂商环境优先使用对应的云控制器；否则自建按需求选择 Nginx/Gateway API/Traefik 等。

## 问题10：如何用 Nginx Ingress 实现金丝雀发布？
- **答案**：常见做法是创建两份 Ingress：
  1. 生产 Ingress（100% 指向 v1）。
  2. Canary Ingress（通过注解开启金丝雀并设置权重，如 `nginx.ingress.kubernetes.io/canary: "true"`、`nginx.ingress.kubernetes.io/canary-weight: "10"`，让 10% 流量进入 v2）。  
  观察监控与指标后逐步提升权重（10% → 30% → 50% → 100%），完成后删除 Canary Ingress 并更新生产 Ingress 指向新版本。

## 问题11：CNI 插件应该怎么选？Terway / Calico / Cilium 的关键差异是什么？
- **答案**：CNI 决定 Pod 跨节点通信模型、性能上限与策略能力。文档给出选型矩阵要点：
  - **Terway（ACK 生产标准）**：VPC/ENI 原生路由，性能高；支持 ENI 独占与 ENI-IP 共享等模式，适合大规模生产。
  - **Calico**：BGP/IPIP，NetworkPolicy 能力强，适用于中大规模与策略需求强的场景。
  - **Cilium（eBPF）**：高性能、可观测性（Hubble）、支持 L3-L7 策略，适合新集群与未来演进。  
  选择建议：ACK 优先 Terway；追求 L7 策略与可观测性可考虑 Cilium；需要成熟策略体系可考虑 Calico。

## 问题12：Terway 的 ENI 独占、ENI-IP 共享、VPC 路由模式分别适合什么业务？
- **答案**：
  - **ENI 独占**：Pod 独立弹性网卡，性能接近宿主机但 Pod 密度受 ENI 配额限制；适合数据库、缓存等高性能/强隔离业务（可做 Pod 级安全组）。
  - **ENI-IP 共享**：Pod 使用 ENI 辅助 IP，共享网卡；性能损耗小、Pod 密度高；适合 Web/微服务。
  - **VPC 路由**：通过 VPC 路由表/节点转发，性能与隔离折中；适合混合场景。

## 问题13：为什么要做 CoreDNS 优化？推荐哪些关键配置？
- **答案**：DNS 是服务发现基础，抖动会放大为全链路故障。文档建议的关键优化：
  - 开启缓存：`cache 30`（减少大量重复查询）。
  - 限制上游并发：`forward` 中设置 `max_concurrent`，防止 DNS 洪水。
  - 启用监控：`prometheus :9153` 并配合告警（如 SERVFAIL）。
  - 需要更低延迟可部署 **NodeLocal DNS Cache**（本地缓存 IP 如 169.254.20.10），降低 CoreDNS 压力并减少 conntrack 风险。

## 问题14：如何用 NetworkPolicy 落地“零信任网络模型”？
- **答案**：典型步骤是“默认拒绝 + 最小放行”：
  1. 在命名空间创建 `default-deny-ingress`，拒绝所有入口。
  2. 为关键链路放行（如仅允许 frontend → backend 的业务端口）。
  3. 放行 backend → 数据库的入口端口（如 3306）。
  4. 别忘了放行 DNS（通常需要允许 Pod 访问 kube-system 的 53/UDP 做解析）。  
  注意：是否生效取决于 CNI 是否支持 NetworkPolicy（如 Flannel 默认不支持，需配合 Calico/Cilium 等）。

## 问题15：排障时如何快速定位是 Service、DNS 还是 CNI 的问题？
- **答案**：可按“从上到下”排查：
  - **Service/Endpoints**：`kubectl get endpoints -A`，确认是否有可用地址；无 Endpoints 多半是 selector/Pod 就绪/标签问题。
  - **DNS**：在调试 Pod 内 `nslookup <svc>.<ns>.svc.cluster.local`，看是否解析成功/是否 SERVFAIL。
  - **连通性**：`curl -v http://<service>` 或直连 Pod IP 排除服务端问题。
  - **节点侧实现**：必要时在节点上查看 kube-proxy 规则（iptables/ipvs）或抓包（如 DNS 53 端口）。  
  文档也给出常用 netshoot/busybox 的排障命令模板，可直接复用。