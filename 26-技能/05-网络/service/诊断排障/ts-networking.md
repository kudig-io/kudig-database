---
title: 网络故障排查
description: '# 网络故障排查'
summary: '1. **组件存活**：`kubectl get [[Pods|pods]] -n kube-system -l k8s-app=calico-node`/`-l app=flannel`/`-l k8s-app=[[Cilium|cilium]]`，若异常先看对应日志。'
category: skills
tags:
- k8s
- troubleshooting
- structural
- networking
- etcd
- apiserver
- kubelet
- prometheus
- istio
- envoy
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 网络故障排查 是什么
- 如何 网络故障排查
trigger_keywords:
- 网络故障排查
prerequisites:
- kubectl-basics
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
- kafka-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 网络故障排查

### 01 Cni Troubleshootingompt 模板|Troubleshooting]]

#### 0. 10 分钟快速诊断

1. **组件存活**：`kubectl get [[Pods|pods]] -n kube-system -l k8s-app=calico-node`/`-l app=flannel`/`-l k8s-app=[[Cilium|cilium]]`，若异常先看对应日志。
2. **CNI 配置完整性**：节点上检查 `/etc/cni/net.d/` 与 `/opt/cni/bin/` 是否匹配版本、文件未损坏。
3. **Pod IP 分配**：`kubectl get pods -A -o wide | head` 查看是否出现无 IP/重复 IP；CNI 日志搜索 `IPAM`/`no available IPs`。
4. **路由/封装**：`ip route`、`bridge fdb show`、`tcpdump -i eth0 udp port 4789` 验证 VXLAN；BGP 场景检查 `bird`/`calico-node` 路由。
5. **MTU 与分片**：对大包探测 `ping -M do -s 1472 <pod-ip>`，若不通需调小 Pod MTU。
6. **跨节点连通**：在不同节点 Pod 之间 `ping`/`curl`，结合 `ip route get` 确认路径正确。
7. **快速缓解**：
   - IPAM 耗尽：扩展地址池或回收泄露 IP。
   - 组件异常：滚动重启 CNI [[daemonset|DaemonSet]]，避免单节点规则不同步。
   - 网络抖动：先降低变更频率，避免大量 Pod 同时创建/删除导致 FDB/ARP 抖动。
8. **证据留存**：保存 CNI 日志、节点路由/ARP/FDB 快照、失败的连通性测试结果。

---

#### 排查方法与步骤


#### 2.1 排查原理：CNI 架构与数据平面

CNI（Container Network Interface）负责为 Pod 配置网络。深入理解其架构是高效排查的关键：

#### 2.1.1 CNI 标准接口
- **CNI 规范版本**：当前主流 v0.4.0 / v1.0.0，定义了标准化的网络配置接口
- **调用时机**：
  - **ADD**：Pod 创建时，kubelet 调用 CNI 插件创建网络命名空间并配置网络
  - **DEL**：Pod 删除时，kubelet 调用 CNI 插件清理网络资源
  - **CHECK**（v0.4.0+）：检查网络配置是否符合预期
  - **VERSION**：查询插件支持的 CNI 版本
- **调用参数**：
  ```json
  {
    "cniVersion": "1.0.0",
    "name": "k8s-pod-network",
    "type": "calico",
    "ipam": {
      "type": "calico-ipam"
    },
    "containerID": "abc123...",
    "netns": "/var/run/netns/cni-xxx",
    "ifname": "eth0"
  }

  ```
- **返回结果**：包含分配的 IP 地址、路由、DNS 配置等

#### 2.1.2 CNI 插件分类与职责

##### 1. 主插件（Main Plugin）
负责创建网络接口和配置路由：

**Calico**：
- **数据平面**：纯三层路由（默认）或 VXLAN/IPIP 封装
- **控制平面**：BGP 协议分发路由（bird）或 kube-apiserver 存储路由
- **网络策略**：通过 iptables 或 eBPF（Calico-eBPF）实现
- **优势**：性能好（无封装）、支持网络策略、大规模集群稳定
- **组件**：
  - `calico-node`（DaemonSet）：运行 BIRD BGP、Felix（路由/策略管理）
  - `calico-kube-controllers`（Deployment）：监听 API Server 同步网络配置
  - `calico-typha`（可选）：缓存 API Server 数据，减少 API 压力

**Flannel**：
- **数据平面**：VXLAN（默认）、Host-GW（纯路由）、UDP（已废弃）
- **控制平面**：etcd 或 Kubernetes API 存储网络配置
- **网络策略**：不支持（需配合 Calico Policy Controller）
- **优势**：简单易部署、社区成熟
- **后端模式**：
  - **VXLAN**：三层网络隧道，兼容性好但有性能开销（5-10%）
  - **Host-GW**：纯路由，要求节点在同一二层网络，性能最优
  - **WireGuard**：加密隧道，安全但性能开销较大

**Cilium**：
- **数据平面**：eBPF 内核加速（绕过 netfilter/iptables）
- **控制平面**：Key-Value Store（etcd）或 CRD
- **网络策略**：L3-L7 策略（HTTP/gRPC/Kafka 协议感知）
- **优势**：高性能、可观测性强、支持服务网格
- **组件**：
  - `cilium-agent`（Daemon
...(截断)

---

### 02 Dns Troubleshooting

#### 0. 10 分钟快速诊断

1. **CoreDNS 存活与 Endpoints**：`kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide`，`kubectl get endpoints -n kube-system kube-dns`。
2. **Pod 内快速验证**：`kubectl run dnsutils --rm -it --image=registry.k8s.io/e2e-test-images/jessie-dnsutils:1.3 -- sh`，执行 `dig kubernetes.default` 与 `dig @<cluster-dns-ip> kubernetes.default.svc.cluster.local`。
3. **resolv.conf 校验**：`kubectl exec <pod> -- cat /etc/resolv.conf`，检查 `nameserver` 是否为集群 DNS，`search` 与 `ndots` 是否合理。
4. **Corefile 检查**：`kubectl get cm -n kube-system coredns -o yaml`，确认 `forward` 上游、`cache`、`loop` 插件配置合理。
5. **上游 DNS 健康**：`kubectl logs -n kube-system -l k8s-app=kube-dns | grep -E "SERVFAIL|timeout|forward" | tail`，排除上游抖动。
6. **网络路径**：`kubectl get svc -n kube-system kube-dns -o wide` 与 `kube-proxy` 规则，确认 Service/Endpoints 正确映射。
7. **快速缓解**：
   - CoreDNS 资源不足：扩副本或提高 CPU/内存 request。
   - 上游慢：启用 `cache` 并调整 `max_concurrent`；必要时引入 NodeLocal DNSCache。
8. **证据留存**：保存 DNS 测试结果、CoreDNS 日志、Corefile 配置与资源使用快照。

---

#### 排查方法与步骤


#### 2.1 排查原理

CoreDNS 是 Kubernetes 集群的 DNS 服务，负责服务发现和外部域名解析。排查需要从以下层面：

1. **服务层面**：CoreDNS Pod 是否正常运行
2. **配置层面**：CoreDNS 配置是否正确
3. **网络层面**：Pod 到 CoreDNS 的网络是否通畅
4. **上游层面**：上游 DNS 是否正常
5. **客户端层面**：Pod 的 DNS 配置是否正确

#### 2.1.1 CoreDNS 架构深度剖析

**核心插件链机制**

CoreDNS 采用插件化架构，每个 DNS 请求按照 Corefile 中定义的插件顺序依次处理：

```
┌─────────────┐
│ DNS 请求    │
└──────┬──────┘
       │
       v
┌─────────────┐
│  errors     │ ─── 错误日志记录
└──────┬──────┘
       │
       v
┌─────────────┐
│  cache      │ ─── 缓存查询（命中直接返回）
└──────┬──────┘
       │
       v
┌─────────────┐
│  kubernetes │ ─── 集群内域名解析（*.svc.cluster.local）
└──────┬──────┘
       │
       v
┌─────────────┐
│  forward    │ ─── 上游 DNS 转发（外部域名）
└──────┬──────┘
       │
       v
┌─────────────┐
│ DNS 响应    │
└─────────────┘
```

**关键插件功能详解**

| 插件名称 | 功能 | 关键参数 | 问题影响 |
|---------|------|---------|---------|
| **errors** | 记录错误到日志 | - | 禁用导致错误排查困难 |
| **health** | 健康检查端点 | `lameduck 5s` | 影响滚动更新平滑性 |
| **ready** | 就绪检查端点 `/ready` | - | 影响 Pod 就绪判断 |
| **kubernetes** | K8s 服务发现 | `pods insecure`<br>`fallthrough`<br>`ttl 30` | 集群内域名解析失败 |
| **prometheus** | 暴露指标 | `:9153` | 无监控数据 |
| **forward** | 上游 DNS 转发 | `max_concurrent 1000`<br>`policy sequential/random` | 外部域名解析失败 |
| **cache** | DNS 缓存 | `success 9984 30`<br>`denial 9984 5` | 无缓存导致性能差 |
| **loop** | 检测转发环路 | - | 环路导致无限递归 |
| **reload** | 热加载配置 | - | 需重启 Pod 更新配置 |
| **loadbalance** | 负载均衡 A 记录 | - | 多 IP 返回顺序固定 |

**kubernetes 插件深度解析**

```yaml
kubernetes cluster.local in-addr.arpa ip6.arpa {
    #
...(截断)

---

### 03 Service Ingress Troubleshooting

#### 0. 10 分钟快速诊断

1. **资源链路快照**：`kubectl get svc,ep,endpointslices -n <ns> <svc> -o wide`，确认后端 Ready。
2. **Service 类型核对**：`kubectl get svc <svc> -o jsonpath='{.spec.type}'`，ClusterIP/NodePort/LB 的路径不同。
3. **数据面规则**：
   - iptables：`iptables -t nat -L -n | grep <cluster-ip>`
   - IPVS：`ipvsadm -Ln -t <cluster-ip>:<port>`
4. **Ingress 控制器健康**：`kubectl get pods -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx`，查看日志是否报 404/502/证书错误。
5. **Host/TLS 校验**：`curl -v http://<ingress-ip> -H "Host: <hostname>"`；TLS 证书检查 `kubectl get secret <tls-secret> -o yaml`。
6. **LB 健康检查**：云环境查看 LB 实例状态与安全组放通端口；确保健康检查路径与应用一致。
7. **快速缓解**：
   - Endpoints 为空：修复后端 Pod/Readiness 探针。
   - 规则缺失：重启 kube-proxy 或 Ingress Controller。
   - TLS 失败：更新 Secret 并触发热更新，必要时回滚证书。
8. **证据留存**：保存 Service/Ingress 描述、控制器日志、规则快照与 curl 输出。

---

#### 排查方法与步骤


#### 2.1 排查原理与架构深度剖析

#### 2.1.1 Service 代理模式详解

Kubernetes Service 通过 kube-proxy 实现流量转发，支持三种代理模式：

**模式对比**

| 模式 | 实现方式 | 性能 | 可观测性 | 维护状态 | 适用场景 |
|------|---------|------|---------|---------|---------|
| **userspace** | 用户态代理 | 差（用户态-内核态切换） | 好（日志详细） | 已废弃 | - |
| **iptables** | Netfilter 规则链 | 中（内核态处理） | 差（无连接跟踪日志） | 默认模式 | 中小规模集群 |
| **IPVS** | Linux 虚拟服务器 | 优（LVS 内核模块） | 中（ipvsadm 查询） | 推荐 | 大规模集群（>1000 Service） |

---

**iptables 模式架构**

```
┌────────────────────────────────────────────────────────────┐
│                         Client Pod                          │
│                     (10.244.1.5)                            │
└───────────────────────────┬────────────────────────────────┘
                            │ 访问 ClusterIP:Port
                            │ (10.96.100.10:80)
                            v
┌────────────────────────────────────────────────────────────┐
│                      iptables 规则链                        │
│                                                              │
│  PREROUTING (nat)                                           │
│    └─> KUBE-SERVICES                                        │
│         └─> KUBE-SVC-XXXXX  (匹配 10.96.100.10:80)         │
│              ├─> KUBE-SEP-AAAAA (33% 概率) ──> 10.244.2.10:8080
│              ├─> KUBE-SEP-BBBBB (50% 概率) ──> 10.244.3.15:8080
│              └─> KUBE-SEP-CCCCC (100% 概率)──> 10.244.1.20:8080
│                                                              │
│  POSTROUTING (nat)                                 
...(截断)

---

### 04 Networkpolicy Troubleshooting

#### 0.5 10 分钟快速诊断

1. **CNI 是否支持**：确认使用的 CNI 支持 NetworkPolicy（如 Calico/Cilium）；纯 Flannel 无效。
2. **命中策略确认**：`kubectl get netpol -A`，定位目标 Pod 是否被任何策略选中（被选中即进入隔离）。
3. **连通性快速测试**：使用 `netshoot` 在源/目标 Pod 互测，区分 `timeout` 与 `refused`。
4. **命名空间标签**：`kubectl get ns --show-labels`，确认 `namespaceSelector` 依赖的标签是否存在。
5. **DNS 放行**：检查 Egress 是否允许到 `kube-system` 53/UDP/TCP，避免“域名解析失败”。
6. **HostNetwork 排查**：`kubectl get pod -o jsonpath='{.spec.hostNetwork}'`，hostNetwork 不受策略影响。
7. **快速缓解**：临时缩小策略范围或添加显式放行规则（DNS/健康检查/监控）。
8. **证据留存**：保存策略 YAML、连通性测试结果、CNI 日志与规则快照。

---

#### 排查方法与步骤

1. **确认策略是否命中 Pod**：`kubectl get netpol -A`，核对 `podSelector` 与目标 Pod 标签匹配情况。
2. **检查命名空间标签**：`kubectl get ns --show-labels`，确认 `namespaceSelector` 所需标签存在。
3. **验证连通性与错误类型**：用 `netshoot` 进行 `curl`/`nc` 测试，区分超时（Drop）与拒绝（Reject）。
4. **确认 CNI 下发规则**：查看 CNI Agent 日志与规则快照（iptables/eBPF），确认策略已生效。
5. **核对 DNS 放行**：确认 53/UDP 与 53/TCP 到 `kube-system` 放行策略存在。
6. **验证修复结果**：回归测试关键路径与监控抓取，确认告警恢复。

#### 常见修复策略

- **策略未生效**：切换到支持 NetworkPolicy 的 CNI（如 Calico/Cilium）或修复 CNI Agent 异常。
- **误拦截**：补齐 DNS/监控/健康检查放行规则，缩小 `podSelector` 范围。
- **跨命名空间问题**：补充 `ReferenceGrant`/命名空间标签，确保选择器命中。

---

### 05 Service Mesh Istio Troubleshooting

#### 0.5 10 分钟快速诊断

1. **控制面与代理同步**：`istioctl proxy-status`，确认所有代理 `SYNCED`；`kubectl get pods -n istio-system` 检查 istiod。
2. **Sidecar 注入**：`kubectl get pod <pod> -o jsonpath='{.spec.containers[*].name}'`，确认 `istio-proxy` 存在；必要时检查 `istio-injection=enabled` 标签。
3. **访问路径验证**：在源 Pod 内 `curl` 目标服务，结合 Envoy 访问日志解析 `response_flags`。
4. **xDS 配置核对**：`istioctl proxy-config route/cluster/endpoint <pod>`，确认 VirtualService/DR 是否已下发。
5. **mTLS 模式**：`kubectl get peerauthentication -A`，确认是否 `STRICT` 导致非 mTLS 客户端失败。
6. **Gateway 健康**：`kubectl get pods -l app=istio-ingressgateway -n istio-system`，检查 502/503 日志。
7. **快速缓解**：
   - 灰度回退：对关键命名空间先设为 `PERMISSIVE`。
   - 资源加固：提高 istiod 与 gateway 资源并扩副本。
8. **证据留存**：保存 `istioctl analyze` 输出、proxy-status、关键 Envoy 日志。

---

#### 排查方法与步骤

1. **控制面健康检查**：`kubectl get pods -n istio-system`，查看 `istiod` 与 Gateway 状态。
2. **代理同步状态**：`istioctl proxy-status`，确认配置是否 `SYNCED`。
3. **路由与端点核对**：`istioctl proxy-config route/endpoint <pod>`，确认 VS/DR 是否下发。
4. **mTLS 模式核对**：`kubectl get peerauthentication -A`，排查模式切换导致的失败。
5. **日志与响应标记**：解析 Envoy 日志中的 `response_flags`，定位上游失败类型。
6. **修复验证**：回归关键路径，验证流量恢复与告警下降。

#### 常见修复策略

- **注入失败**：修复命名空间标签或 Webhook 健康，必要时临时手动注入验证。
- **503/502**：检查 Service/Endpoints 与 Gateway 资源，必要时扩容 gateway/istiod。
- **mTLS 失败**：先临时回退到 `PERMISSIVE`，确认依赖方全部支持 mTLS 后再收敛。

---

### 06 Gateway Api Troubleshooting

#### 0.5 10 分钟快速诊断

1. **GatewayClass/Gateway 状态**：`kubectl get gatewayclass,gateway -A`，确认 `Accepted/Programmed` 为 True。
2. **Route 绑定**：`kubectl get httproute -A -o yaml | grep -A3 "parents"`，确认 `Accepted` 条件与 ParentRef 正确。
3. **跨 NS 引用**：`kubectl get referencegrant -A`，缺失时会出现 `ResolvedRefs=False`。
4. **后端健康**：检查 Service/Endpoints/探针，排除 503/502 来自后端不可用。
5. **TLS/证书**：确认 Listener 绑定的 Secret 存在、证书链正确；gRPC 场景核对 H2。
6. **控制器日志**：查看 Gateway 控制器日志（如 Envoy Gateway / Nginx Gateway）定位 reconcile 失败原因。
7. **快速缓解**：
   - 回滚最近 Route/Listener 变更。
   - 临时放宽 Route 绑定限制（AllowedRoutes）以恢复流量，再逐步收敛。
8. **证据留存**：保存 Gateway/Route 状态、ReferenceGrant、控制器日志与 curl/openssl 输出。

---

#### 排查方法与步骤

1. **确认 GatewayClass/Gateway 状态**：检查 `Accepted/Programmed` 条件是否为 True。
2. **核对 Route 绑定状态**：查看 `parents` 条件，确认 ParentRef 指向正确。
3. **检查跨命名空间授权**：确认 `ReferenceGrant` 是否存在且匹配。
4. **验证后端健康**：核对 Service/Endpoints/探针与健康检查结果。
5. **检查 TLS/HTTP2**：确认证书 Secret 与 Listener 协议配置一致。
6. **修复验证**：回归访问测试与控制器日志，确认状态恢复。

#### 常见修复策略

- **Programmed 失败**：排查控制器与底层 LB 资源，必要时扩容控制器。
- **Route 未绑定**：修正 ParentRef 与 AllowedRoutes，补齐 `ReferenceGrant`。
- **TLS 失败**：更新证书 Secret，确保证书链和主机名一致。

---

### 07 Terway Troubleshooting

#### 0. 10 分钟快速诊断

1. **Terway Pod 状态**：`kubectl get pods -n kube-system -l app=terway`，确认 terway 和 terway-eniip/terway-eni DaemonSet Pod 均为 Running。
2. **节点弹性网卡信息**：`kubectl describe node <node-name> | grep aliyun.com`，查看已分配/剩余 ENI 和 IP 数量。
3. **Pod IP 归属**：`kubectl get pod <pod-name> -o yaml | grep k8s.aliyun.com`，确认 Pod 使用的是 ENI 模式还是 Veth 模式。
4. **VPC 路由检查**：登录阿里云控制台，确认 VPC 路由表是否包含 Pod CIDR 指向各节点 ECS 实例的路由条目。
5. **安全组规则**：确认节点安全组是否放通 Pod 间通信所需端口（尤其是自定义安全组场景）。
6. **快速缓解**：
   - Pod 无法分配 IP：检查节点 ENI 配额和 IP 池是否耗尽，必要时升级实例规格或释放闲置 Pod。
   - 跨节点通信失败：检查 VPC 路由表和安全组规则，确认无自定义路由冲突。
   - 网络策略不生效：确认是否启用了 Calico 策略引擎且版本兼容。
7. **证据留存**：保存节点 Annotation、terway Pod 日志、弹性网卡控制台截图、VPC 路由表配置。

---

#### 2. 排查方法与步骤


#### 2.2 Pod IP 分配失败排查

#### 2.2.1 排查逻辑决策树

```
Pod 处于 ContainerCreating，事件显示 IP 分配失败
    │
    ├─ 1. 检查 terway Pod 状态
    │       ├─ terway Pod 未 Running → 排查 DaemonSet / 节点资源
    │       └─ terway Pod Running → 进入 2
    │
    ├─ 2. 查看 terway 日志
    │       ├─ "exceeded eni quota" → ENI 配额不足（2.2.2）
    │       ├─ "no available IP" → IP 池耗尽（2.2.3）
    │       ├─ "fixed IP already in use" → 固定 IP 冲突（2.2.4）
    │       └─ 其他错误 → 阿里云 OpenAPI 调用失败（2.2.5）
    │
    └─ 3. 检查节点资源
            ├─ ENI 数量达到实例规格上限 → 升级实例规格或释放 ENI
            └─ 辅助 IP 达到 ENI 上限 → 申请更多 ENI 或调整单 ENI IP 数
```

#### 2.2.2 ENI 配额不足

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看节点已分配的 ENI 和 IP 数量
kubectl describe node <node-name> | grep -E "aliyun.com/allocated-eni|aliyun.com/allocated-ip|aliyun.com/eni-max|aliyun.com/ip-max"

# 进入 terway Pod 查看资源池状态
kubectl exec -n kube-system <terway-pod> -- terway-cli show

# 查看实例规格支持的 ENI 和辅助 IP 上限
# 登录阿里云控制台：ECS -> 实例详情 -> 本实例弹性网卡
# 或通过 API
curl "https://ecs.aliyuncs.com/?Action=DescribeInstanceTypes&InstanceTypes.1=<instance-type>"
```
**关键指标**：
- `aliyun.com/allocated-eni`：已分配 ENI 数量
- `aliyun.com/eni-max`：实例规格支持的最大 ENI 数量
- `aliyun.com/allocated-ip`：已分配辅助 IP 数量

**解决方案**：
- 释放不再使用的 Pod（尤其是使用独占 ENI 的 Pod）
- 升级 ECS 实例规格以支持更多 ENI
- 调整 Terway 配置，使用 `eniip` 模式（共享辅助 IP）替代 `eni` 模式（独占 ENI）

#### 2.2.3 IP 资源池耗尽

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 terway 资源池详情
kubectl exec -n kube-system <terway-pod> -- terway-cli show

# 查看节点上所有 Pod 使用的 IP
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> -o wide

# 检查是否存在已删除但 IP 未释放的 Pod（孤儿 IP
...(截断)

---

### 08 Flannel Troubleshooting

#### 0. 10 分钟快速诊断

1. **Flannel Pod 状态**：`kubectl get pods -n kube-system -l app=flannel`，确认所有 flannel Pod 为 Running 且运行时间较长（非频繁重启）。
2. **子网分配**：在节点上 `cat /run/flannel/subnet.env`，确认 `FLANNEL_SUBNET` 已正确分配。
3. **CNI 配置**：检查 `/etc/cni/net.d/10-flannel.conflist` 存在且格式正确。
4. **跨节点连通**：在不同节点 Pod 之间执行 `ping` 和 `curl`，确认 overlay 网络正常。
5. **VXLAN 检查**：`ip -d link show flannel.1`，确认 VTEP 和 MAC 地址正确；`bridge fdb show dev flannel.1` 查看远端节点学习状态。
6. **快速缓解**：
   - 子网未分配：删除 `/run/flannel/subnet.env` 并重启 flannel Pod，强制重新注册。
   - 跨节点不通：检查 UDP 4789（VXLAN）是否被防火墙阻断，或尝试切换为 host-gw 模式。
   - MTU 问题：将 Pod MTU 降至 1450（VXLAN 场景）。
7. **证据留存**：保存 flannel Pod 日志、`subnet.env`、节点路由表、FDB/ARP 表、`/etc/cni/net.d/` 目录内容。

---

#### 2. 排查方法与步骤


#### 2.2 Pod IP 分配失败排查

#### 2.2.1 排查逻辑决策树

```
Pod 处于 ContainerCreating，无 IP 地址
    │
    ├─ 1. 检查 flannel Pod 状态
    │       ├─ flannel Pod 未 Running → 排查 DaemonSet
    │       └─ flannel Pod Running → 进入 2
    │
    ├─ 2. 检查 CNI 配置
    │       ├─ /etc/cni/net.d/ 无 flannel 配置 → CNI 初始化失败
    │       ├─ /opt/cni/bin/ 无 flannel 二进制 → 插件缺失
    │       └─ 配置正常 → 进入 3
    │
    ├─ 3. 检查子网分配
    │       ├─ /run/flannel/subnet.env 不存在 → flanneld 未正确注册子网
    │       ├─ 子网与其他节点冲突 → 子网分配冲突
    │       └─ 子网正常 → 进入 4
    │
    └─ 4. 检查 IPAM
            ├─ host-local IPAM 池耗尽 → 大量 Pod 创建/删除
            └─ 其他错误 → 查看 CNI 日志
```
# 🟢 低风险：只读/信息收集，通常无副作用
#### 2.2.2 子网分配失败

```bash
# 查看所有节点的子网分配
kubectl get nodes -o json | jq -r '.items[] | 
  "\(.metadata.name): \(.spec.podCIDR // "未分配")"'

# 查看 flannel 子网分配记录
kubectl logs -n kube-system -l app=flannel | grep -i "subnet|lease"

# 检查节点上的子网环境文件
cat /run/flannel/subnet.env
# 预期输

---
(内容截断，完整内容见源文件) ---

## 相关链接

- [[26-技能/05-网络/service/诊断排障/ts-networking.md|网络故障排查总览]]
- [[26-技能/04-工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Pod 故障排查]]

## Related

- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[envoy]] — Envoy
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[26-技能/05-网络/dns/dns-fta.md|DNS 异常故障树分析]] — Cross-reference
- [[26-技能/05-网络/service-mesh/service-mesh-istio-fta.md|Service Mesh(Istio) 异常故障树分析]] — Cross-reference

```

<!-- risk-assessed -->
