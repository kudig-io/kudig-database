---
title: Cilium Mesh 与 Kubernetes 多集群网络实践
description: Cilium ClusterMesh 多集群网络深度指南 — 三大需求（服务发现/连通性/策略）、clustermesh-apiserver 与 etcd KV 同步架构、Global Service、跨集群 NetworkPolicy 与身份、隧道模式选型、与 Istio 多集群及 Karmada 的定位对比、两集群对接完整实操、故障排查与同城双活生产案例
summary: Cilium Mesh 多集群网络实践 — 覆盖 ClusterMesh 架构（clustermesh-apiserver/Global Service/etcd KV 同步/VXLAN-Geneve-原生路由隧道）、跨集群策略与身份、两集群对接完整命令、cilium clustermesh status 与 connectivity test 验证、证书与 KV 同步类故障排查、Istio 多集群对比及同城双活案例
category: service-mesh-microservices
tags:
- cilium
- clustermesh
- service-mesh
- ebpf
- multi-cluster
- network
- network-policy
- kubernetes
- karmada
- hubble
tier: supporting
created: '2026-10-15'
last_updated: '2026-10-15'
difficulty: advanced
audience:
- 网络工程师
- SRE
- 平台工程师
- 架构师
estimated_read_time: 25min
intent_queries:
- Cilium ClusterMesh 是什么，如何工作
- 如何用 Cilium 打通两个 Kubernetes 集群网络
- Global Service 怎么配置，跨集群负载均衡如何实现
- cilium clustermesh status 报证书错误怎么排查
- Cilium Mesh 和 Istio 多集群方案怎么选
- 多集群 NetworkPolicy 如何跨集群生效
trigger_keywords:
- ClusterMesh
- Cilium Mesh
- 多集群网络
- Global Service
- clustermesh-apiserver
- 跨集群
- 服务发现
- 多集群策略
prerequisites:
- kubectl-basics
- networking-basics
- cilium-basics
- ebpf-basics
- helm-basics
k8s_versions:
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（只读/信息收集，无副作用）。

# Cilium Mesh 与 Kubernetes 多集群网络实践

> **适用版本**: Cilium v1.20.x（撰写时最新稳定版为 v1.20.2，经 GitHub Releases 核对；独立的 `cilium/cilium-clustermesh` 仓库已不存在，ClusterMesh 能力全部收敛在主仓库内）
> **难度**: 高级
> **前置阅读**: [[05-网络/02-网络基础/09-cilium-ebpf-network-guide.md|Cilium eBPF 网络与安全实践指南]]

---

## 📋 目录

- [一、概述与核心概念：多集群网络的三大需求](#一概述与核心概念多集群网络的三大需求)
- [二、架构设计：ClusterMesh 组件与数据面](#二架构设计clustermesh-组件与数据面)
- [三、生产级配置与操作指南：两集群对接实操](#三生产级配置与操作指南两集群对接实操)
- [四、定位对比：Cilium Mesh、Istio 多集群与 Karmada/OCM](#四定位对比cilium-meshistio-多集群与karmadaocm)
- [五、性能开销与监控指标](#五性能开销与监控指标)
- [六、故障排查与诊断流程](#六故障排查与诊断流程)
- [七、最佳实践与反模式](#七最佳实践与反模式)
- [八、生产案例：同城双活集群统一服务暴露](#八生产案例同城双活集群统一服务暴露)
- [九、版本兼容性与参考链接](#九版本兼容性与参考链接)

---

## 一、概述与核心概念：多集群网络的三大需求

多集群（Multi-Cluster）是生产 Kubernetes 的常态：按环境拆分（生产/预发）、按可用区拆分（同城双活）、按业务域拆分（多租户隔离）、或混合云分布（自建 IDC + 公有云托管集群）。无论拆分动机是什么，一旦服务需要跨集群调用，网络层就必须同时满足三个需求，缺一不可：

| 需求 | 含义 | 缺失后果 |
|------|------|----------|
| **服务发现** | 集群 A 的 Pod 能"看见"集群 B 的后端端点（IP/身份），Service 解析结果包含远端端点 | 只能靠外部 LB 或手工 Endpoint 拼接，故障转移慢、配置漂移 |
| **连通性** | 集群 A 的 Pod IP 可以直接路由/隧道到集群 B 的 Pod IP，无需经过节点端口或网关 NAT | 每次跨集群调用都要过 Ingress/NodePort，损耗性能且丢失源 IP |
| **策略** | NetworkPolicy、身份（Identity）与加密可以跨集群一致生效 | 跨集群流量成为策略盲区，安全边界出现裂缝 |

传统解法各有残缺：东西向网关 + 外部 DNS 满足"发现"但不满足"连通性与策略"；Submariner/VXLAN 打通 L3 满足"连通性"但 Service 层是空白的。Cilium ClusterMesh 的核心主张是：**用 eBPF 在内核层同时满足三个需求**——服务发现通过 etcd KV 同步实现（Global Service），连通性通过集群间隧道或原生路由实现，策略通过全局安全身份实现。

三个关键术语先行定义：**ClusterMesh** 是 Cilium 的多集群互联能力集，包含控制面同步与数据面转发两部分；**Global Service** 是带 `service.cilium.io/global: "true"` 注解的 Service，其后端端点会被合并到所有互联集群，实现跨集群负载均衡与故障转移；**全局身份（Global Identity）** 则让 Cilium 基于 label 计算的安全身份在所有集群间共享，跨集群流量因此可沿用同一套 NetworkPolicy 语义，无需重新鉴权。

---

## 二、架构设计：ClusterMesh 组件与数据面

### 2.1 clustermesh-apiserver：KV 同步中枢

每个集群在 `kube-system` 内运行一组 `clustermesh-apiserver` Deployment（默认 2 副本，etcd 与 apiserver 同容器或分体，取决于版本），其职责：

```
cluster1                                    cluster2
┌────────────────────────────┐              ┌────────────────────────────┐
│ cilium-agent (每节点)       │              │ cilium-agent (每节点)       │
│   │ watch                   │              │   │ watch                   │
│   ▼                         │   TLS/mTLS   │   ▼                         │
│ clustermesh-apiserver       │◄────────────►│ clustermesh-apiserver       │
│  ├─ etcd (本集群 KV)         │  agent 直连   │  ├─ etcd (本集群 KV)         │
│  └─ cilium-apiserver 聚合   │  对端 etcd    │  └─ cilium-apiserver 聚合   │
└────────────────────────────┘              └────────────────────────────┘
```

关键机制：**不是 etcd 之间的 raft 复制，而是每个集群的 cilium-agent 直接 watch 对方集群的 clustermesh-apiserver etcd**。同步对象是序列化后的 KV：CiliumEndpoint（Pod 端点）、CiliumIdentity（安全身份）、CiliumNode（节点与地址信息）。apiserver 本身不转发任何业务数据包，它是纯控制面——数据包永远走节点间直连隧道。访问方式：默认经 LoadBalancer/NodePort Service 暴露 `2379`（etcd）与 `443`（聚合 API）；agent 到远端 etcd 使用每集群独立的客户端证书（`clustermesh-client-<cluster>` Secret），对等信任由 connect 时自动分发的证书链建立。容量上默认最多互联 **255 个集群**（身份号段按 `cluster.id × 256` 切分），可通过 `clustermesh.maxConnectedClusters=511`（Helm）扩到 511，代价是身份号段更紧张。

### 2.2 集群间数据面：四种连通模式

控制面同步只解决"知道对方在哪"，包怎么过去取决于 `tunnel`/路由配置：

| 模式 | 配置要点 | 适用场景 | 代价 |
|------|----------|----------|------|
| **VXLAN 隧道** | `tunnel=vxlan`，跨集群包再封一层外层 UDP/4789 | 最通用默认；底层网络不可控 | 每包约 +50 字节，MTU 缩水，封解包耗 CPU |
| **Geneve 隧道** | `tunnel=geneve`，UDP/6081，可携带可选字段 | 需要隧道元数据扩展的场景 | 约 +58 字节，其余同 VXLAN |
| **原生路由（native routing）** | `tunnel=disabled` + 底层路由器/BGP 拦截 Pod CIDR 互通 | 云 VPC 互通、自建 BGP 骨干 | 要求 Pod CIDR 在底层网络可路由，规划成本高 |
| **静态/全局 Pod IP** | 集群间分配互不重叠的全局 CIDR，IP 天然唯一 | 多集群统一地址规划（类 Akamek/全球 IP 池思路） | 地址管理集中化，扩展受地址空间约束 |

两条硬性约束（任何模式下都必须满足）：

1. **各集群 Pod CIDR 不得重叠**。否则远端端点 IP 会与本地 IP 冲突，路由表无法区分。
2. **各集群 `cluster.id` 唯一**（1–255）且 `cluster.name` 唯一。身份号段与策略标签都依赖它。

加密：跨集群隧道可叠加 IPsec 或 WireGuard（`encryption.type`），启用后所有互联集群的加密配置必须一致，否则握手失败。

### 2.3 Global Service 与流量调度

带全局注解的 Service，其 Endpoint 会在所有集群合并。调度语义由注解控制：

| 注解 | 作用 |
|------|------|
| `service.cilium.io/global: "true"` | 声明为全局服务，跨集群合并后端 |
| `service.cilium.io/shared: "false"` | 本集群后端不对外共享（单向：本地可调远端，远端调不到本地） |
| `service.cilium.io/affinity: "local"` | 优先本地后端，本地全部不可用才 fallback 到远端（异地容灾推荐） |
| `service.cilium.io/affinity: "remote"` | 优先远端后端（蓝绿集群切换场景） |

底层实现：cilium-agent 把远端 CiliumEndpoint 注入本地 Service 的负载均衡 map（eBPF 侧的 backend 列表），对 kube-proxy 与应用完全透明——应用照常 `curl web.prod.svc.cluster.local`，内核里的 eBPF 程序已把远端端点纳入一致性哈希。

### 2.4 跨集群 NetworkPolicy 与身份

身份标签是全局命名空间里的资源。每个端点自动携带保留标签：

```
k8s:io.cilium.k8s.policy.cluster   = <cluster-name>   # 端点所属集群
k8s:io.kubernetes.pod.namespace    = <namespace>
k8s:io.cilium.k8s.namespace.labels.<k> = <v>          # 命名空间标签（含跨集群）
```

因此"只允许来自 cluster1 的 Pod 访问"这类策略天然可表达（见 3.4 节 YAML），且**策略在两端集群独立求值**——入口方向的策略在本集群 agent 上执行，出口方向在源集群执行。两个集群都应下发对称策略，避免出现"一端放行、另一端默认拒绝"造成的疑难。

---

## 三、生产级配置与操作指南：两集群对接实操

以下用 `cilium` CLI 完成 cluster1/cluster2 两集群对接（等价的 Helm values 在注释中给出，便于 GitOps 化）。

### 3.1 前置检查

```bash
# 🟢 确认两集群 Node 名与 Pod CIDR 不冲突（重点看 podCIDR 列）
kubectl --context cluster1 get nodes -o custom-columns=NAME:.metadata.name,PODCIDR:.spec.podCIDR
kubectl --context cluster2 get nodes -o custom-columns=NAME:.metadata.name,PODCIDR:.spec.podCIDR

# 🟢 确认 cilium 版本一致（跨大版本互联属于非支持组合）
for c in cluster1 cluster2; do cilium version --context $c | head -1; done

# 🟢 测试节点间连通性：从 cluster1 节点 ping cluster2 任一节点 IP
kubectl --context cluster1 run netcheck --image=cilium/netcheck -it --rm --restart=Never -- ping -c3 <cluster2-node-ip>
```

### 3.2 安装 Cilium（指定 cluster id/name 与互斥 CIDR）

```bash
# 🟡 cluster1：隧道模式 + 跨集群加密
cilium install --context cluster1 \
  --cluster-name cluster1 --cluster-id 1 \
  --set clusterPoolIPv4PodCIDRList="10.0.0.0/16" \
  --set tunnelProtocol=vxlan \
  --set encryption.type=wireguard
# 等价 Helm: cluster.name=cluster1, cluster.id=1, ipam.mode=cluster-pool ...

# 🟡 cluster2：注意 Pod CIDR 必须互斥
cilium install --context cluster2 \
  --cluster-name cluster2 --cluster-id 2 \
  --set clusterPoolIPv4PodCIDRList="10.1.0.0/16" \
  --set tunnelProtocol=vxlan \
  --set encryption.type=wireguard

# 🟢 两集群等待就绪
cilium status --context cluster1 --wait
cilium status --context cluster2 --wait
```

原生路由模式替换 `tunnelProtocol` 为 `routingMode=native`，并确保底层路由（BGP/VPC 路由表）已发布两边的 Pod CIDR。

### 3.3 开启 ClusterMesh 并互联

```bash
# 🟡 各集群启用 clustermesh-apiserver（LoadBalancer 不可用时用 NodePort）
cilium clustermesh enable --context cluster1 --service-type LoadBalancer
cilium clustermesh enable --context cluster2 --service-type LoadBalancer

# 🟡 建立对等连接（双向各执行一次）
cilium clustermesh connect --context cluster1 --destination-context cluster2
cilium clustermesh connect --context cluster2 --destination-context cluster1

# 🟢 确认状态：看到每个远端集群 configured 且 agents 达到期望数量
cilium clustermesh status --context cluster1 --wait
cilium clustermesh status --context cluster2 --wait
```

`status` 输出中三列都要绿：`Configured`（Secret 注入成功）、`Connected`（agent 与远端 etcd 建连）、`Endpoints Synced`（KV 同步达成一致）。

### 3.4 Global Service 与跨集群策略

```yaml
# cluster1 与 cluster2 各部署一份同名的后端 Deployment，再创建全局 Service
apiVersion: v1
kind: Service
metadata:
  name: web
  namespace: prod
  annotations:
    service.cilium.io/global: "true"      # 全局合并后端
    service.cilium.io/affinity: "local"   # 本地优先，故障时跨集群兜底
spec:
  type: ClusterIP
  selector:
    app: web
  ports:
  - port: 80
    targetPort: 8080
---
# 跨集群 NetworkPolicy：仅允许 cluster1 的 prod 命名空间调用本服务
apiVersion: cilium.io/v2
kind: CiliumNetworkPolicy
metadata:
  name: web-allow-from-cluster1
  namespace: prod
spec:
  endpointSelector:
    matchLabels:
      app: web
  ingress:
  - fromEndpoints:
    - matchLabels:
        k8s:io.kubernetes.pod.namespace: prod
        k8s:io.cilium.k8s.policy.cluster: cluster1   # 按来源集群收敛
```

```yaml
# 标准 NetworkPolicy 写法（K8s 原生，按集群标签 matchExpressions）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: api-deny-cross-cluster
  namespace: prod
spec:
  podSelector:
    matchLabels: {app: api}
  policyTypes: [Ingress]
  ingress:
  - from:
    - podSelector:
        matchExpressions:
        - key: k8s:io.cilium.k8s.policy.cluster
          operator: In
          values: ["cluster1"]     # 只放行本集群来源
```

### 3.5 端到端验证

```bash
# 🟢 官方跨集群连通性测试（含 Global Service、策略、加密等用例）
cilium connectivity test --context cluster1 --multi-cluster cluster2

# 🟢 手动验证跨集群端点已注入本地 Service
kubectl --context cluster1 -n prod get endpointslice -l kubernetes.io/service-name=web -o wide
#   远端端点 IP 属于 10.1.0.0/16 即为 cluster2 的 Pod

# 🟢 从 cluster1 发起请求，观察是否命中远端后端（hubble 按 cluster 过滤）
hubble --context cluster1 observe -n prod --from-cluster cluster1 --to-cluster cluster2 --protocol http -f

# 🟢 在节点上检查 clustermesh KV 状态
kubectl --context cluster1 -n kube-system exec ds/cilium -c cilium-agent -- cilium-dbg clustermesh status
```

预期：`connectivity test` 全绿；断掉 cluster2 全部后端后，cluster1 的本地优先流量仍可由本地端点承接，反之亦然——这就是 Global Service 的故障转移语义。

---

## 四、定位对比：Cilium Mesh、Istio 多集群与 Karmada/OCM

### 4.1 Cilium Mesh vs Istio 多集群

两者解决的问题层次不同，常常**组合使用而非互斥**：

| 维度 | Cilium ClusterMesh | Istio 多集群（primary-remote） | Istio 多主（multi-primary） |
|------|--------------------|-------------------------------|------------------------------|
| 工作层 | L3/L4：Pod IP 连通 + 身份 + L4 策略 | L7：mTLS、路由、遥测、故障注入 | 同左 |
| 控制面 | 每集群独立 agent，KV 对等同步 | 单一 primary 控制面，remote 只有 istiod 代理或无 | 每集群独立 istiod，靠根 CA 互信 |
| 服务发现 | Global Service 合并后端 | east-west gateway + ServiceEntry/自动发现 | 同左 |
| 数据面开销 | eBPF 内核态，无 sidecar | sidecar（或 ambient ztunnel） | 同左 |
| 典型痛点 | L7 能力弱（需搭配） | 跨集群依赖共享根 CA、网络须打通（常用 Cilium 承担） | 配置复杂度高、一致性难保证 |

实践结论：**Cilium 打通 L3/L4 底座，Istio（尤其 ambient 模式）专注 L7** 的组合在企业里最常见。若只需要"跨集群调用 + 网络策略 + 可观测"，ClusterMesh 单独就够，不必引入完整 mesh（参见 [[05-网络/03-服务网格/07-service-mesh-comparison-selection.md|服务网格选型对比]]、[[05-网络/03-服务网格/01-istio-enterprise-service-mesh.md|Istio 企业级服务网格]]）。

### 4.2 与 Karmada/OCM 的边界

[[23-实体/09-编排调度/karmada.md|Karmada]] 与 OCM（Open Cluster Management）是**管理面**：多集群调度（副本按集群分发）、策略下发、应用生命周期。它们**不提供任何数据面连通能力**——Karmada 把 Deployment 摊到 N 个集群后，服务之间怎么互相访问，Karmada 完全不管。清晰的分层结论：**应用分发/调度层**（Karmada、OCM、Argo CD ApplicationSet）回答"在哪跑"；**网络连通/策略层**（Cilium ClusterMesh、Submariner）回答"怎么通、谁许谁"；**L7 治理层**（Istio ambient、Linkerd）回答"怎么路由、怎么观测"。

典型生产栈即三层叠加：Karmada 负责把 `web` 按集群权重调度，ClusterMesh 保证任一集群的 `api` 都能调到全网 `web` 后端，Istio 负责金丝雀与 mTLS。

---

## 五、性能开销与监控指标

### 5.1 带宽与延迟的量级讨论

> 以下为量级参考，实际数值取决于网卡、CPU、MTU、拓扑与负载模型，务必以自身压测为准。

| 项目 | 隧道模式（VXLAN） | 原生路由 |
|------|------------------|----------|
| 每包额外字节 | ~50 字节（外层 ETH+IP+UDP+VXLAN） | 0 |
| 有效 MTU（物理 1500） | 建议设 1450，否则依赖分片/PMTUD | 1500 全额 |
| 单核吞吐影响 | 封解包 + 哈希消耗 CPU；万兆线速下通常多占约 1 个核的量级 | 显著低于隧道 |
| 跨集群 RTT 额外开销 | 同城 DC 内通常在零点几毫秒量级 | 接近裸网络 |
| 加密（WireGuard/IPsec）叠加 | 再增加每包处理成本，跨机房建议开启 | 同左 |

容量规划要点：跨集群带宽按"故障切换最坏比例"估算——本地优先模式下，一旦对端集群整体故障，全部流量回落本地，本集群出口/入口带宽与后端容量必须能独立承接 100% 负载。

### 5.2 关键监控指标

cilium-agent 与 clustermesh-apiserver 均暴露 Prometheus 指标（`cilium-metrics` Service，默认 9962/9963 端口）：

```yaml
# Prometheus 抓取片段（cilium-agent 与 clustermesh-apiserver）
- job_name: cilium-clustermesh
  kubernetes_sd_configs:
  - role: pod
    namespaces: {names: [kube-system]}
  # 再按 pod label（k8s-app=cilium / clustermesh-apiserver）做 keep 过滤
```

重点指标：

| 指标 | 含义 | 告警建议 |
|------|------|----------|
| `cilium_clustermesh_apiserver_*` | apiserver 侧同步与连接状态 | 连续 5 分钟异常即告警 |
| `cilium_kvstore_events_queue_seconds_*` | KV 事件入队延迟（含 clustermesh scope） | p99 持续 > 1s 关注 |
| `cilium_kvstore_sync_errors` | KV 同步错误计数 | 任何非零增长告警 |
| `hubble_drop_total`（按 cluster label） | 跨集群丢包（策略拒绝/路由黑洞） | 按业务 SLO |
| `cilium_identity_count` | 全局身份数量 | 逼近号段上限时预警 |

---

## 六、故障排查与诊断流程

### 6.1 排查决策树

```
跨集群不通
 ├─ cilium clustermesh status 是否全绿？
 │   ├─ 否 → 控制面问题（证书/连接/同步）→ 6.2
 │   └─ 是 → 数据面问题 → 6.3
 └─ 策略可疑 → hubble observe 看 DROP 原因 → 6.4
```

### 6.2 控制面三类高频故障

**故障 1：证书错误（`x509: certificate signed by unknown authority`）**

```bash
# 🟢 查看具体报错（status 不加 --wait 立即返回）
cilium clustermesh status --context cluster1

# 🟢 查看 agent 连接远端 etcd 的日志
kubectl --context cluster1 -n kube-system logs deploy/cilium --tail=200 | grep -i clustermesh

# 🟡 根因多为重装集群后 cilium-ca 变化。重置对等关系后重建：
cilium clustermesh reset --context cluster1
cilium clustermesh enable  --context cluster1 --service-type LoadBalancer
cilium clustermesh connect --context cluster1 --destination-context cluster2
```

预防：多集群共用同一 CA（Helm `tls.ca` 显式指定），或重装任一集群后固定执行 reset+connect 流程并纳入变更手册。

**故障 2：connection refused / timeout**

逐层排查网络路径：NodePort/LB 端口是否可达（`2379`/`443`）、防火墙/安全组是否放行、LB 健康检查是否把 apiserver 后端摘除、云厂商 LB 是否启用了 proxy protocol 破坏 TLS 握手。

```bash
# 🟢 从 cluster1 节点探测 cluster2 的 clustermesh Service
kubectl --context cluster2 -n kube-system get svc clustermesh-apiserver -o wide
kubectl --context cluster1 run netcheck --image=cilium/netcheck -it --rm -- \
  curl -sk --connect-timeout 3 https://<clustermesh-svc-ip>:443/readyz
```

**故障 3：KV 同步延迟/不收敛**

现象：`status` 显示 `Endpoints Synced: X/Y` 长期不齐。常见原因：端点规模大导致初始全量同步慢（万级端点分钟级）、远端集群身份 churn（频繁滚动重建）、跨机房 etcd watch 被中间设备限速。检查 `cilium_kvstore_events_queue_seconds` 与 `cilium_kvstore_sync_errors`；必要时评估将 clustermesh-apiserver 扩为 2 副本以上并检查其 etcd 磁盘 IO。

### 6.3 数据面：全局服务不路由

```bash
# 🟢 远端端点是否进入本地 endpointslice
kubectl --context cluster1 -n prod get endpointslice -l kubernetes.io/service-name=web -o yaml

# 🟢 本地 Service 后端 map 视角（节点级）
kubectl --context cluster1 -n kube-system exec ds/cilium -c cilium-agent -- \
  cilium-dbg bpf lb list | grep -E '10\.(0|1)\.'
```

高频根因清单：两边 Service 注解不一致（一边 global 一边没加）；隧道模式不一致（一边 vxlan 一边 native）；Pod CIDR 重叠导致端点被丢弃；加密配置不对称；远端后端所在节点 NotReady。

### 6.4 策略类疑难

跨集群流量被拒但单集群正常时，优先怀疑**两端策略不对称**：入口端按 `k8s:io.cilium.k8s.policy.cluster` 收敛了来源，出口端却缺少对应放行。用 Hubble 一次性看清：

```bash
# 🟢 观察跨集群 DROP 的 verdict 与策略 ID
hubble --context cluster2 observe --from-cluster cluster1 --verdict DROPPED -f
```

---

## 七、最佳实践与反模式

**最佳实践**

1. **CIDR 与身份规划先行**：多集群 Pod CIDR、`cluster.id`、Service CIDR 全部登记到全局 IPAM 表；新集群加入前先跑重叠检查脚本。
2. **GitOps 化 mesh 配置**：用 Helm values 而非 CLI 临时参数管理 ClusterMesh（enable/connect 本质也是写 Helm values），保证可重建。
3. **本地优先（affinity: local）为默认**：只有明确需要全局负载均衡（无状态全局缓存池类服务）才用纯 global。
4. **对称策略**：跨集群策略在两端同时下发，CI 中校验两集群策略 diff。
5. **加密默认开启**：跨机房/跨 VPC 一律叠加 WireGuard，同机房按合规要求决定。
6. **演练故障转移**：定期演练"整集群摘除"，验证回落路径与容量假设。

**反模式**

- ❌ 用 ClusterMesh 替代备份/容灾——它能转移流量，不能恢复数据。
- ❌ 跨大版本互联（v1.15 对 v1.20）——仅同主版本组合受支持。
- ❌ 集群数量无上限扩张——每增一个对等集群，agent 多一条 watch 链路与端点集，收敛时间非线性增长。
- ❌ 把跨集群流量当免费——延迟、带宽与排障成本都是真实成本，能收敛的调用域不要盲目打散。
- ❌ 只在一边配置 NetworkPolicy 就以为安全——两端独立求值，缺一边就是裸奔。

---

## 八、生产案例：同城双活集群统一服务暴露

**背景**：某支付平台两个同城 IDC 各一个 K8s 集群（cluster-a / cluster-b，RTT ~1ms），要求：任一 IDC 整体故障时核心链路 5 分钟内恢复；日常流量本 IDC 优先；统一安全策略。

**方案拓扑**：外部流量经 BGP Anycast/DNS 进入两 IDC 各自的入口 LB 层；cluster-a 与 cluster-b 之间以 WireGuard 加密的 ClusterMesh 隧道互联（同城 RTT ~1ms）；所有 Global Service 设 `affinity: local`，核心链路（支付、账务）在两集群等量部署，非核心服务单集群部署但共享全局 Service 名，故障时靠远端后端兜底；跨集群策略以 `io.cilium.k8s.policy.cluster` 标签收敛，审计域覆盖两集群。

**关键配置**：Global Service 全部 `affinity: local`，核心链路（支付、账务）在两集群等量部署；非核心服务仅单集群部署但共享全局 Service 名，故障时靠远端后端兜底。跨集群策略以 `io.cilium.k8s.policy.cluster` 标签收敛，审计域覆盖两集群。

**落地结果与经验**（%%inferred%% 量级化描述）：

- 故障演练：模拟 cluster-a 整体下电，cluster-b 的本地端点承接全部流量，恢复时间主要由入口 LB 摘除决定，网络层自身收敛在秒级。
- 容量教训：初期按 60/40 分流规划带宽，演练发现本地优先意味着最坏情况单集群吃满 100%，出口带宽与后端副本数按单集群全量重规划。
- 运维教训：clustermesh 证书 3 年期，跨集群 CA 轮换被遗漏导致一次 mesh 断连，后续将证书到期时间纳入统一巡检（见 6.2 证书故障）。

---

## 九、版本兼容性与参考链接

### 9.1 版本要点

- 撰写时 Cilium 最新稳定版 **v1.20.2**（`gh api repos/cilium/cilium/releases/latest` 核对，2026-10）；`cilium/cilium-clustermesh` 仓库已 404，多集群相关工具全部并入主仓库与 cilium CLI。
- 升级原则：**互联集群同步升级**，滚动期间跨大版本互联属非支持组合；先升控制面组件（clustermesh-apiserver）再升 agent。
- v1.14+：最大互联集群数可用 `clustermesh.maxConnectedClusters` 提升到 511。
- v1.16+：Helm 的 `tunnel` 拆分为 `routingMode` 与 `tunnelProtocol`，迁移时注意 values 改名。
- Global Service 的 `service.cilium.io/affinity` 注解取代了早期仅靠 `shared: "false"` 表达单向语义的做法，老集群升级后建议统一迁移。

### 9.2 参考链接

- Cilium 官方文档 — ClusterMesh：<https://docs.cilium.io/en/stable/network/clustermesh/>
- Cilium 官方文档 — Global Service 与注解语义：<https://docs.cilium.io/en/stable/network/clustermesh/global-svc/>
- Cilium 官方博客 — Multi-Cluster Networking Deep Dive：<https://cilium.io/blog/>
- Istio 多集群部署模型（primary-remote / multi-primary）：<https://istio.io/latest/docs/ops/deployment/deployment-models/>
- Karmada 官方文档：<https://karmada.io/docs/>
- 相关内部文档：[[05-网络/02-网络基础/09-cilium-ebpf-network-guide.md|Cilium eBPF 网络与安全实践指南]]、[[05-网络/02-网络基础/07-cni-plugin-comparison-selection.md|CNI 插件对比选型]]、[[05-网络/02-网络基础/06-sdn-network-virtualization.md|SDN 网络虚拟化]]、[[23-实体/04-网络/cilium.md|Cilium]]、[[23-实体/07-可观测性/prometheus.md|Prometheus]]

---

*本文由 KUDIG 团队维护，覆盖 Cilium v1.20.x 的 ClusterMesh 能力面。多集群方案选型请结合组织规模与团队运维能力综合评估。*
