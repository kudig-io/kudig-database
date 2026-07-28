---
title: FAQ 文档
description: '## 问题1：Kubernetes 网络从外部到 Pod 的完整链路是什么？'
summary: '该分层的核心价值是：入口治理（Ingress/Gateway）与服务发现/负载均衡（Service）解耦，底层由 CNI 负责跨节点转发与策略能力。'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- prometheus
- envoy
- cilium
- flannel
- calico
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- FAQ 文档 是什么
- 如何 FAQ 文档
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- FAQ
- 文档
- networking
prerequisites:
- kubectl-basics
- networking-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
- kafka-basics
- redis-basics
- mysql-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# FAQ 文档
本文档适用于：产品手册、官网页面、课程测验、现场 Q&A。

<!-- chunk: 问题1：[[kubernetes|Kubernetes]] 网络从外部到 Pod 的完整链路是什么？ -->
## 问题1：Kubernetes 网络从外部到 Pod 的完整链路是什么？
- **答案**：典型南北向链路为：Internet → DNS → CDN/WAF → 云负载均衡（SLB/ALB/NLB）→ Kubernetes [[ingress|Ingress]]（Nginx Ingress / Gateway API / ALB Ingress Controller）→ Kubernetes [[service|Service]]（ClusterIP / NodePort / LoadBalancer / Headless）→ CNI 网络（Flannel/Calico/Cilium/Terway，veth/bridge/ENI）→ Pod 网络（如 10.244.0.0/16）→ 物理网络/VPC（Node Network）。  
  该分层的核心价值是：入口治理（Ingress/Gateway）与服务发现/负载均衡（Service）解耦，底层由 CNI 负责跨节点转发与策略能力。

<!-- chunk: 问题2：ClusterIP 适合什么场景？为什么说它是微服务“唯一标准入口”？ -->
## 问题2：ClusterIP 适合什么场景？为什么说它是微服务“唯一标准入口”？
- **答案**：ClusterIP 用于**集群内部服务发现与负载均衡**，对调用方暴露稳定的虚拟 IP（ClusterIP），屏蔽后端 Pod 的弹性伸缩、重建、滚动升级带来的 IP 变化。  
  生产建议要点：
  - 通过 `selector` 精确选择后端（必要时可按版本控制）。
  - `publishNotReadyAddresses: false` 确保仅路由到 Ready Pod。
  - 需要会话保持时可用 `sessionAffinity: ClientIP`（如 WebSocket、本地缓存）。

<!-- chunk: 问题3：什么时候需要为 ClusterIP 固定 `clusterIP`？ -->
## 问题3：什么时候需要为 ClusterIP 固定 `clusterIP`？
- **答案**：在少数灾备/依赖固定地址的场景（例如外部系统以白名单方式放行某个固定 IP，或迁移过程需要保持地址不变）可固定 `clusterIP`。  
  注意：固定 ClusterIP 会降低灵活性，需确保 Service CIDR 规划充足、且变更有严格流程，否则可能造成 IP 冲突或不可用。

<!-- chunk: 问题4：什么是“拓扑感知路由（Topology Aware Hints）”，能带来什么收益？ -->
## 问题4：什么是“拓扑感知路由（Topology Aware Hints）”，能带来什么收益？
- **答案**：拓扑感知路由用于让 kube-proxy **优先选择同节点/同可用区（AZ）的 Endpoint**，减少跨 AZ 延迟与费用。  
  启用方式示例：为 Service 添加注解 `service.kubernetes.io/topology-aware-hints: "auto"`。  
  文档给出的生产效果参考：延迟降低 40–60%，跨 AZ 流量成本降低约 70%，适用于缓存、数据库等延迟敏感服务的多 AZ 部署。

<!-- chunk: 问题5：NodePort 适合生产吗？有哪些典型用途与风险？ -->
## 问题5：NodePort 适合生产吗？有哪些典型用途与风险？
- **答案**：NodePort 通过在每个节点开放端口（默认范围 30000–32767）把服务“直通”到节点网络，**测试环境快速验证**很方便，但生产通常不推荐作为主要暴露方式。  
  典型用途：配合物理四层 LB、临时联调、非云环境快速暴露。  
  主要风险：
  - 需要管理节点暴露面与端口冲突。
  - 如果策略不当可能丢失源 IP 或导致负载不均。

<!-- chunk: 问题6：`externalTrafficPolicy: Cluster` 与 `Local` 有什么区别？如何选择？ -->
## 问题6：`externalTrafficPolicy: Cluster` 与 `Local` 有什么区别？如何选择？
- **答案**：
  - **Cluster（默认）**：可能发生 SNAT，客户端源 IP 可能丢失；可跨节点转发，负载更均衡。
  - **Local**：保留真实源 IP；仅转发到**本节点**的 Pod（避免跨节点跳转），但可能出现负载不均，且通常需要 `healthCheckNodePort` 配合健康检查。  
  选择建议：需要审计、IP 白名单、真实源 IP 的场景选 `Local`；无源 IP 强需求、追求均衡选 `Cluster`。

<!-- chunk: 问题7：LoadBalancer 为什么是生产环境“唯一推荐”的外部暴露方式之一？如何做成本优化？ -->
## 问题7：LoadBalancer 为什么是生产环境“唯一推荐”的外部暴露方式之一？如何做成本优化？
- **答案**：LoadBalancer 由云厂商自动提供高可用 LB、健康检查与流量切换，运维成本低、可靠性高，适合生产对外服务。  
  成本优化策略（文档建议）：
  - **复用已有 SLB/ALB**：通过注解指定现有 LB ID，避免重复创建。
  - **按流量计费**：低流量业务更划算。
  - **共享带宽包**：多个 LB 共享带宽降低费用。
  - **Ingress 替代多 SLB**：使用“单 SLB + Ingress”让多服务共享入口，节省显著。

<!-- chunk: 问题8：Headless Service 是什么？它与 StatefulSet 的关系是什么？ -->
## 问题8：Headless Service 是什么？它与 StatefulSet 的关系是什么？
- **答案**：Headless Service 通过 `clusterIP: None` 关闭虚拟 IP，DNS 直接解析到 Pod IP 列表，客户端可直连 Pod，不经 kube-proxy 的服务转发路径，性能更优。  
  与 StatefulSet 配合时：
  - StatefulSet 使用 `serviceName` 绑定 Headless Service。
  - 可获得稳定的 Pod DNS：如 `mysql-0.mysql.namespace.svc.cluster.local` 对应固定实例。  
  适用：MySQL/PostgreSQL 主从、Redis Cluster、ES、Kafka、MongoDB 等有状态集群。

<!-- chunk: 问题9：Ingress、Gateway API、ALB Ingress Controller 如何选型？ -->
## 问题9：Ingress、Gateway API、ALB Ingress Controller 如何选型？
- **答案**：可按复杂度与环境决策：
  - **自建/多云且简单 HTTP 路由**：Nginx Ingress Controller。
  - **复杂治理、多协议、角色分离**：Gateway API（如 Envoy Gateway）。
  - **云厂商托管且深度集成（ACK）**：ALB Ingress Controller（利用云原生能力、可基于 Header/Cookie 灰度、成本可通过单 ALB 承载多服务优化）。  
  文档给出决策树：需要南北向流量管理 → 云厂商环境优先使用对应的云控制器；否则自建按需求选择 Nginx/Gateway API/Traefik 等。

<!-- chunk: 问题10：如何用 Nginx Ingress 实现金丝雀发布？ -->
## 问题10：如何用 Nginx Ingress 实现金丝雀发布？
- **答案**：常见做法是创建两份 Ingress：
  1. 生产 Ingress（100% 指向 v1）。
  2. Canary Ingress（通过注解开启金丝雀并设置权重，如 `nginx.ingress.kubernetes.io/canary: "true"`、`nginx.ingress.kubernetes.io/canary-weight: "10"`，让 10% 流量进入 v2）。  
  观察监控与指标后逐步提升权重（10% → 30% → 50% → 100%），完成后删除 Canary Ingress 并更新生产 Ingress 指向新版本。

<!-- chunk: 问题11：CNI 插件应该怎么选？Terway / Calico / Cilium 的关键差异是什么？ -->
## 问题11：CNI 插件应该怎么选？Terway / Calico / Cilium 的关键差异是什么？
- **答案**：CNI 决定 Pod 跨节点通信模型、性能上限与策略能力。文档给出选型矩阵要点：
  - **Terway（ACK 生产标准）**：VPC/ENI 原生路由，性能高；支持 ENI 独占与 ENI-IP 共享等模式，适合大规模生产。
  - **Calico**：BGP/IPIP，NetworkPolicy 能力强，适用于中大规模与策略需求强的场景。
  - **Cilium（eBPF）**：高性能、可观测性（Hubble）、支持 L3-L7 策略，适合新集群与未来演进。  
  选择建议：ACK 优先 Terway；追求 L7 策略与可观测性可考虑 Cilium；需要成熟策略体系可考虑 Calico。

<!-- chunk: 问题12：Terway 的 ENI 独占、ENI-IP 共享、VPC 路由模式分别适合什么业务？ -->
## 问题12：Terway 的 ENI 独占、ENI-IP 共享、VPC 路由模式分别适合什么业务？
- **答案**：
  - **ENI 独占**：Pod 独立弹性网卡，性能接近宿主机但 Pod 密度受 ENI 配额限制；适合数据库、缓存等高性能/强隔离业务（可做 Pod 级安全组）。
  - **ENI-IP 共享**：Pod 使用 ENI 辅助 IP，共享网卡；性能损耗小、Pod 密度高；适合 Web/微服务。
  - **VPC 路由**：通过 VPC 路由表/节点转发，性能与隔离折中；适合混合场景。

<!-- chunk: 问题13：为什么要做 CoreDNS 优化？推荐哪些关键配置？ -->
## 问题13：为什么要做 CoreDNS 优化？推荐哪些关键配置？
- **答案**：DNS 是服务发现基础，抖动会放大为全链路问题。文档建议的关键优化：
  - 开启缓存：`cache 30`（减少大量重复查询）。
  - 限制上游并发：`forward` 中设置 `max_concurrent`，防止 DNS 洪水。
  - 启用监控：`prometheus :9153` 并配合告警（如 SERVFAIL）。
  - 需要更低延迟可部署 **NodeLocal DNS Cache**（本地缓存 IP 如 169.254.20.10），降低 CoreDNS 压力并减少 conntrack 风险。

<!-- chunk: 问题14：如何用 NetworkPolicy 落地“零信任网络模型”？ -->
## 问题14：如何用 NetworkPolicy 落地“零信任网络模型”？
- **答案**：典型步骤是“默认拒绝 + 最小放行”：
  1. 在命名空间创建 `default-deny-ingress`，拒绝所有入口。
  2. 为关键链路放行（如仅允许 frontend → backend 的业务端口）。
  3. 放行 backend → 数据库的入口端口（如 3306）。
  4. 别忘了放行 DNS（通常需要允许 Pod 访问 kube-system 的 53/UDP 做解析）。  
  注意：是否生效取决于 CNI 是否支持 NetworkPolicy（如 Flannel 默认不支持，需配合 Calico/Cilium 等）。

<!-- chunk: 问题15：排障时如何快速定位是 Service、DNS 还是 CNI 的问题？ -->
## 问题15：排障时如何快速定位是 Service、DNS 还是 CNI 的问题？
- **答案**：可按“从上到下”排查：
  - **Service/Endpoints**：`kubectl get endpoints -A`，确认是否有可用地址；无 Endpoints 多半是 selector/Pod 就绪/标签问题。
  - **DNS**：在调试 Pod 内 `nslookup <svc>.<ns>.svc.cluster.local`，看是否解析成功/是否 SERVFAIL。
  - **连通性**：`curl -v http://<service>` 或直连 Pod IP 排除服务端问题。
  - **节点侧实现**：必要时在节点上查看 kube-proxy 规则（iptables/ipvs）或抓包（如 DNS 53 端口）。  
  文档也给出常用 netshoot/busybox 的排障命令模板，可直接复用。

---

<!-- chunk: 问题16：Service Mesh 与 Ingress/Gateway 如何分工？什么场景需要引入 Mesh？ -->
## 问题16：Service Mesh 与 Ingress/Gateway 如何分工？什么场景需要引入 Mesh？
- **答案**：Ingress/Gateway 负责**南北向流量**（外部进入集群），Service Mesh 负责**东西向流量**（集群内服务间通信）。  
  引入 Mesh 的典型场景：
  - 需要 mTLS 加密所有服务间通信（零信任网络）。
  - 需要细粒度流量治理（重试、熔断、超时、流量镜像）。
  - 需要分布式追踪与可观测性（无需修改应用代码）。
  - 多语言微服务需要统一的流量策略。  
  注意：Mesh 引入额外延迟（约 1-3ms）与资源开销（每 Pod 约 100m CPU / 128Mi 内存），小规模服务可能不需要。

<!-- chunk: 问题17：Kubernetes 双栈（IPv4/IPv6）网络如何部署？有哪些注意事项？ -->
## 问题17：Kubernetes 双栈（IPv4/IPv6）网络如何部署？有哪些注意事项？
- **答案**：Kubernetes 1.23+ 正式支持双栈。关键配置：
  - kube-apiserver: `--service-cluster-ip-range=10.96.0.0/16,fd00::/108`
  - kube-controller-manager: `--cluster-cidr=10.244.0.0/16,fd00:10:244::/56`
  - CNI 必须支持双栈（Calico/Cilium 已支持，Flannel 部分支持）。  
  注意事项：
  - Service 可指定 `ipFamilyPolicy: PreferDualStack` 或 `RequireDualStack`。
  - 节点必须同时具备 IPv4 和 IPv6 地址。
  - 部分云厂商 LB 对 IPv6 支持有限，需确认。
  - NetworkPolicy 在双栈下需同时匹配 IPv4 和 IPv6 CIDR。

<!-- chunk: 问题18：Pod 网络性能不达预期时如何调优？ -->
## 问题18：Pod 网络性能不达预期时如何调优？
- **答案**：分层排查与调优：
  1. **基线测试**：`iperf3 -c <target-pod-ip> -t 30 -P 4` 确认实际带宽。
  2. **CNI 层**：确认 MTU 配置正确（通常 1450 for overlay，1500 for ENI/routing）。
  3. **节点层**：`sysctl net.core.rmem_max`、`net.core.wmem_max` 调大缓冲区。
  4. **conntrack**：`sysctl net.netfilter.nf_conntrack_max` 防止表满丢包。
  5. **中断亲和**：确认网卡中断分散到多 CPU（`irqbalance` 或手动设置）。
  6. **eBPF 加速**：Cilium 的 eBPF 模式绕过 iptables，性能提升 20-40%。

```bash
# 🟢 低风险：网络性能基线测试
# Pod 间带宽测试
kubectl run iperf-server --image=networkstatic/iperf3 --restart=Never -- iperf3 -s
kubectl run iperf-client --image=networkstatic/iperf3 --restart=Never --rm -it -- \
  iperf3 -c <server-pod-ip> -t 30 -P 4

# 检查 MTU
kubectl exec -it <pod> -- ip link show eth0 | grep mtu

# 检查 conntrack 表使用率
# 🟢 低风险
kubectl exec -it -n kube-system <calico-node-pod> -- \
  conntrack -C 2>/dev/null || cat /proc/sys/net/netfilter/nf_conntrack_count
```

<!-- chunk: 问题19：如何为多租户集群设计网络隔离？ -->
## 问题19：如何为多租户集群设计网络隔离？
- **答案**：多租户网络隔离分层实施：
  1. **Namespace 级**：每个租户独立 Namespace + 默认拒绝 NetworkPolicy。
  2. **NetworkPolicy**：租户间完全隔离，仅允许共享服务（DNS、监控）访问。
  3. **CNI 高级特性**：Calico GlobalNetworkPolicy / Cilium CiliumNetworkPolicy 支持 L7 策略。
  4. **节点隔离**：关键租户使用专用节点池（nodeSelector + taint）。
  5. **Ingress 隔离**：每租户独立 IngressClass 或 Gateway，避免路由冲突。

```yaml
# 🟡 中风险：会修改网络策略
# 租户隔离 NetworkPolicy 模板
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-isolation
  namespace: tenant-a
spec:
  podSelector: {}  # 匹配所有 Pod
  policyTypes:
    - Ingress
    - Egress
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              tenant: tenant-a  # 仅允许同租户
    - from:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53  # 允许 DNS
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              tenant: tenant-a
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
```

<!-- chunk: 问题20：kube-proxy 的 iptables 与 IPVS 模式如何选择？ -->
## 问题20：kube-proxy 的 iptables 与 IPVS 模式如何选择？
- **答案**：
  - **iptables（默认）**：适合 Service 数量 < 1000 的集群，规则线性增长，大规模时延迟增加。
  - **IPVS**：适合 Service > 1000 的大规模集群，基于哈希表查找，性能恒定；支持多种负载均衡算法（rr/lc/dh/sh/sed/nq）。  
  切换注意事项：
  - IPVS 需要节点加载内核模块：`ip_vs`、`ip_vs_rr`、`ip_vs_wrr`、`ip_vs_sh`、`nf_conntrack`。
  - 切换需滚动重启 kube-proxy DaemonSet。
  - Cilium eBPF 模式可完全替代 kube-proxy（KPR），性能最优。

```bash
# 🟢 低风险：检查当前 kube-proxy 模式
kubectl get configmap kube-proxy -n kube-system -o yaml | grep mode
# 检查 IPVS 模块是否加载
# 🟢 低风险
lsmod | grep ip_vs
```

<!-- chunk: 问题21：DNS 解析延迟高或间歇性失败如何排查？ -->
## 问题21：DNS 解析延迟高或间歇性失败如何排查？
- **答案**：DNS 问题分层排查：
  1. **确认现象**：`kubectl exec -it <pod> -- nslookup kubernetes.default` 多次测试。
  2. **CoreDNS 状态**：`kubectl get pods -n kube-system -l k8s-app=kube-dns` 确认副本数与健康。
  3. **CoreDNS 指标**：`coredns_dns_request_duration_seconds` P99 > 100ms 则异常。
  4. **conntrack 竞争**：UDP 并发查询触发 conntrack 插入冲突（`insert_failed`），解决方案：NodeLocal DNSCache 或 `single-request-reopen`。
  5. **上游 DNS**：检查 `/etc/resolv.conf` 中上游服务器可达性。

```bash
# 🟢 低风险：DNS 诊断命令集
# 测试 DNS 解析延迟
kubectl exec -it <pod> -- sh -c '
  for i in $(seq 1 10); do
    time nslookup kubernetes.default.svc.cluster.local
  done
'

# CoreDNS 日志检查
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50 | grep -i "SERVFAIL\|timeout\|error"

# 检查 ndots 配置影响
kubectl exec -it <pod> -- cat /etc/resolv.conf
# ndots:5 意味着域名中 < 5 个点会先尝试 search domain 拼接
```

<!-- chunk: 问题22：如何设计生产级 Ingress 高可用架构？ -->
## 问题22：如何设计生产级 Ingress 高可用架构？
- **答案**：生产 Ingress 架构要点：
  1. **多副本 + 反亲和**：Ingress Controller ≥ 2 副本，跨 AZ 分布。
  2. **LB 健康检查**：云 LB 配置主动健康检查（HTTP /healthz）。
  3. **优雅关闭**：`terminationGracePeriodSeconds: 300` + preStop hook 等待连接排干。
  4. **资源预留**：根据 QPS 设置 requests/limits，避免 OOM。
  5. **限流与熔断**：`nginx.ingress.kubernetes.io/limit-rps` 防止后端过载。

```yaml
# 生产级 Nginx Ingress Controller 配置片段
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ingress-nginx-controller
  namespace: ingress-nginx
spec:
  replicas: 3
  template:
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            - labelSelector:
                matchLabels:
                  app.kubernetes.io/name: ingress-nginx
              topologyKey: topology.kubernetes.io/zone
      terminationGracePeriodSeconds: 300
      containers:
        - name: controller
          lifecycle:
            preStop:
              exec:
                command: ["/wait-shutdown"]
          resources:
            requests:
              cpu: 500m
              memory: 512Mi
            limits:
              cpu: "2"
              memory: 2Gi
```

---

## 网络架构决策树

```
服务暴露需求
├── 集群内部访问
│   ├── 无状态服务 → ClusterIP + Deployment
│   ├── 有状态服务 → Headless Service + StatefulSet
│   └── 需要固定 Pod DNS → Headless Service + StatefulSet
├── 外部访问
│   ├── HTTP/HTTPS 七层 → Ingress / Gateway API
│   ├── TCP/UDP 四层 → LoadBalancer / NodePort
│   └── gRPC → Ingress (gRPC 支持) / Gateway API
├── 多租户隔离
│   ├── 网络层 → NetworkPolicy (default-deny + 白名单)
│   ├── 入口层 → 独立 IngressClass / Gateway
│   └── 服务间 → Service Mesh mTLS + AuthorizationPolicy
└── 性能优化
    ├── 同 AZ 优先 → Topology Aware Hints
    ├── 绕过 iptables → Cilium eBPF / IPVS
    └── DNS 加速 → NodeLocal DNSCache
```

## 生产网络配置检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| CoreDNS 副本数 | `kubectl get deploy coredns -n kube-system` | ≥ 2 |
| CoreDNS 资源限制 | `kubectl get deploy coredns -n kube-system -o yaml` | 有 requests/limits |
| kube-proxy 模式 | `kubectl get cm kube-proxy -n kube-system -o yaml` | IPVS (大规模) |
| conntrack 表大小 | `cat /proc/sys/net/netfilter/nf_conntrack_max` | ≥ 1048576 |
| MTU 一致性 | `ip link show` 各节点 | 统一值 |
| NetworkPolicy 覆盖 | `kubectl get networkpolicy -A` | 所有 ns 有 default-deny |
| Ingress 副本数 | `kubectl get deploy -n ingress-nginx` | ≥ 2 |
| DNS ndots 配置 | Pod /etc/resolv.conf | 生产建议 ndots:2 |

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 网络 MOC
- [[05-网络/README.md|Domain 03: Networking 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持
- Flannel 多集群场景与子网冲突处理

## See Also

- 47-terway-troubleshooting-fta
- 00-network-in-nutshell
- 01-network-architecture-overview
- 02-cni-architecture-fundamentals

## Related

- [[21-生态参考/03-领域索引/terway-index.md|Terway 知识图谱索引]]


<!-- risk-assessed -->
