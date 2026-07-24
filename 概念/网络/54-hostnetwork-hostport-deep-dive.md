---
sources:
- "网络/K8s网络核心/54-hostnetwork-hostport-deep-dive.md"
title: hostNetwork 与 hostPort 深度解析
summary: 解析 Kubernetes hostNetwork/hostPort 的网络栈绕过机制、端口冲突调度、DNS 策略与安全权衡。
category: concepts
tags:
- hostnetwork
- hostport
- networking
- dns-policy
- daemonset
- security-context
tier: core
created: 2026-07-23
updated: 2026-07-23
last_updated: 2026-07
status: stable
difficulty: advanced
reading_level: advanced
audience:
- 网络架构师
- SRE
estimated_read_time: 18min
intent_queries:
- hostNetwork 是什么
- hostPort 与 hostNetwork 区别
- hostNetwork Pod DNS 不工作怎么办
- hostNetwork 端口冲突如何排查
trigger_keywords:
- hostNetwork
- hostPort
- 主机网络
- dnsPolicy
- ClusterFirstWithHostNet
k8s_versions:
- '1.28'
- '1.30'
- '1.32'
- '1.33'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令与示例清单。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。`hostNetwork: true` 与 `hostPort` 会绕过 CNI 与 NetworkPolicy 的隔离边界，错误配置可直接暴露节点端口、让 Pod 窥视宿主机流量，甚至导致节点上所有业务 Pod 端口冲突而大面积 CrashLoopBackOff。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# hostNetwork 与 hostPort 深度解析

> **适用版本**: Kubernetes v1.28+（行为在 1.30/1.32/1.33 保持一致）
> **最后更新**: 2026-07

---

## 概述

Kubernetes 的默认网络模型建立在「每个 Pod 拥有独立网络命名空间（network namespace, netns）」这一前提之上：CNI 插件为每个 Pod 创建一对 veth pair，一端留在 Pod netns 内（`eth0`），另一端接入节点的网桥或由 eBPF 程序接管，Pod 因此获得独立的 Pod IP、独立的路由表、独立的 iptables 规则栈。这种隔离是 K8s 网络的可组合性、NetworkPolicy、Service 路由的基础，但也带来了**不可避免的转发开销**：每一个跨 Pod 的包都要穿过两层 netns、两次 conntrack 查询、至少一次 veth pair 的软中断切换。

`hostNetwork` 与 `hostPort` 是两条绕过这条默认路径的「逃生通道」，它们以牺牲隔离性换取性能或特定端口绑定能力：

- **`hostNetwork: true`** 让 Pod **完全**共享宿主机网络命名空间——容器没有自己的 `eth0`、没有自己的 IP、没有自己的路由表，直接读写节点的网络栈。Pod 进程看到的 `0.0.0.0:9100` 就是节点的 `0.0.0.0:9100`。
- **`hostPort`** 则是一种折中：Pod **保留**独立 netns 与独立 Pod IP，但 kubelet 会在宿主机上配置一条 iptables DNAT 规则，把宿主机的某个端口（如 `9100`）转发到该 Pod 的 `containerPort`。外部访问节点 IP:9100 时，包到达节点后经 DNAT 重定向进 Pod netns。

两者都绕过了「Pod 通过 CNI 注册到 Pod 网络」这条主线，因此会带来一系列非直觉的副作用：kube-proxy 不再为这些 Pod 处理 Service 的 ClusterIP 路由、NetworkPolicy 对 hostNetwork Pod 完全失效、DNS 解析行为发生改变、调度器需要额外检查端口唯一性。这些副作用是本篇的核心议题——理解它们的机制，是安全使用这两条通道的前提。

> 本文聚焦 `hostNetwork`/`hostPort` 本身的机制与权衡。Service 的整体模型见 [[网络/K8s网络核心/06-service-concepts-types.md|Service 概念与类型]]，DNS 细节见 [[网络/K8s网络核心/11-dns-service-discovery-coredns.md|DNS 服务发现 CoreDNS]]，DaemonSet 作为最常见载体的实践见 [[概念/daemonset.md|DaemonSet]]。

---

## hostNetwork vs hostPort：核心对比

在深入机制之前，先用一张表把两个特性放在同一个坐标系下对比。这张表是后续所有讨论的锚点——理解每一行的差异，就理解了为什么这两个特性不可互换。

| 维度 | 普通 Pod | `hostNetwork: true` | `hostPort` |
|---|---|---|---|
| **网络命名空间** | 独立（CNI 创建） | 共享宿主机 netns | 独立（仍由 CNI 创建） |
| **Pod IP** | Pod IP（来自 PodCIDR） | 节点 IP（`status.podIP == node IP`） | Pod IP（宿主端口经 DNAT 转入） |
| **端口绑定方式** | CNI 管理，端口仅在 Pod netns 内 | 容器进程直接 `bind` 节点端口 | iptables DNAT：宿主端口 → Pod IP:containerPort |
| **kube-proxy 参与** | 处理 Service（iptables/ipvs/nftables 规则） | **不参与**（Pod 不在 Pod 网络中，无 Endpoint 路由） | 不影响 Service 处理，但该 Pod 本身仍可作 Endpoint |
| **网络路径跳数** | Pod → veth → 节点协议栈 → veth → Pod | Pod 进程直接读写节点协议栈 | 外部 → 节点 iptables DNAT → veth → Pod |
| **调度约束** | 无特殊约束 | 同节点同端口仅一个 Pod（kubelet 启动兜底） | `hostPort` 端口在节点上需唯一（scheduler 部分检查） |
| **NetworkPolicy** | 生效 | **完全失效**（Pod 不在 Pod 网络，CNI 看不到） | 生效（Pod 仍在 Pod 网络） |
| **DNS 默认策略** | `ClusterFirst`（走 CoreDNS） | 默认继承节点 `resolv.conf`，**无法解析集群 Service 名** | `ClusterFirst`（默认正常） |
| **推荐 DNS 策略** | `ClusterFirst` | `ClusterFirstWithHostNet`（必需） | `ClusterFirst` |
| **延迟开销** | veth + conntrack | **最低**（无 veth 切换） | DNAT + veth（每包经 iptables） |
| **典型场景** | 99% 的业务 Pod | CNI agent、node-exporter、高性能平面 | 罕见，偶用于固定节点端口暴露 |

有两个关键认知需要从这里建立：

**第一，`hostNetwork` 与 `hostPort` 不是程度上的递进关系，而是两种不同的机制。** `hostNetwork` 是「整个 Pod 搬进节点网络栈」，`hostPort` 是「在保留 Pod 网络的前提下，给宿主机开一个转发口」。一个 hostNetwork Pod 里所有容器的所有端口天然都在节点上；一个 hostPort Pod 只有被显式声明了 `hostPort` 的那个端口暴露在节点上。

**第二，`hostNetwork` 绕过的是整个 CNI 数据平面，不只是「少一跳」。** 这意味着 kube-proxy 的 Service 规则、NetworkPolicy 的放行/拒绝规则、CNI 的 IPAM 分配、流量镜像、eBPF 程序（如 Cilium 的 socket-level 加速）——全部对该 Pod 失效。这不是「性能更好一点的 Pod」，而是一个「寄生在节点网络上的进程，恰好由 kubelet 管理生命周期」。

---

## hostNetwork 工作机制

### 1. kubelet 如何跳过 CNI

普通 Pod 的启动流程中，kubelet 调用 CNI 插件的 `cmdAdd`，插件为 Pod 创建 netns、分配 IP、接入节点网络。但当 Pod spec 中 `hostNetwork: true` 时，kubelet 走的是完全不同的代码路径：

```
kubelet SyncPod
 └── pod.HostNetwork == true ?
      ├── 是 → 不调用 CNI，容器 runtime 直接以 host netns 启动容器
      │        （containerd/CRI-O 使用 ContainerNetworkNamespace = "host"）
      └── 否 → 调用 CNI cmdAdd，创建独立 netns
```

在 CRI 层面，kubelet 向容器运行时传递的 `PodSandboxConfig` 中，`Linux.PodSecurityContext.NamespaceOptions.Network` 被设为 `NODE`（而非 `POD`）。容器运行时据此创建容器时，容器进程直接 `setns(2)` 到节点的网络命名空间——它的 `/proc/$PID/ns/net` 与 kubelet、与 sshd、与节点上所有进程指向同一个 netns。

这一步带来的直接后果可以立即验证：

```bash
# 🟢 低风险：只读，对比普通 Pod 与 hostNetwork Pod 的网络命名空间
# 在节点上执行
readlink /proc/1/ns/net                       # 1 号进程（systemd）的 netns
crictl inspect <hostnetwork-pod-container-id> \
  | grep -A2 networkNamespace                  # hostNetwork Pod 与上面相同

readlink /proc/<normal-pod-pid>/ns/net         # 普通 Pod 是不同的 inode
```

### 2. status.podIP 等于节点 IP

因为 Pod 没有自己的网络接口，`status.podIP` 字段会被填充为节点的主 IP。这一点看似琐碎，却是许多排障误判的根源——监控面板上看到某个 Pod 的 IP 与 Node IP 相同，常被误以为是「IPAM 冲突」或「Pod 抢了节点的 IP」，实际上是 hostNetwork 的预期行为。

```bash
# 🟢 低风险：只读，列出所有 hostNetwork Pod 及其（等于节点 IP 的）podIP
kubectl get pods -A -o jsonpath='{range .items[?(@.spec.hostNetwork)]}{.metadata.namespace}/{.metadata.name}{"\t"}{.status.podIP}{"\t"}{.spec.nodeName}{"\n"}{end}'
```

### 3. 绕过 kube-proxy 与 Service 的语义

这是 hostNetwork 最容易被误解的一点。kube-proxy 监听 Endpoints/EndpointSlice 的变化，在节点上维护「ClusterIP → 后端 Pod IP」的 iptables/ipvs/nftables 规则。但一个 hostNetwork Pod 的「Pod IP」就是节点 IP——它**不在 PodCIDR 内**，因此：

- 如果一个 Service 通过 `selector` 选中了这个 hostNetwork Pod，kube-controller-manager 仍然会把它加入 Endpoints（因为 Pod 有匹配的 label 且 `status.podIP` 存在），其 Endpoint 地址就是节点 IP。
- 当集群内其他 Pod 通过该 Service 的 ClusterIP 访问时，kube-proxy 的 DNAT 规则会把 ClusterIP 改写成节点 IP，包被「路由回节点自身」。
- 从该 hostNetwork Pod **内部**发起的连接访问 Service ClusterIP，因为 Pod 直接用节点协议栈，访问 ClusterIP 会命中本节点 kube-proxy 的规则——这部分是工作的。

**但有一个重要陷阱**：hostNetwork Pod 作为 Service 后端时，`externalTrafficPolicy` 的行为会很诡异。`Cluster` 策略下，流量可能被 SNAT，后端看不到真实客户端 IP；`Local` 策略下，只有与该 hostNetwork Pod 同节点的客户端才能命中——这在跨节点访问时表现为「Service 偶尔通偶尔不通」。**生产实践上，不要把 hostNetwork Pod 作为普通 Service 的后端**，除非你完全清楚客户端 IP 与 SNAT 的链路。

### 4. dnsPolicy 的连锁反应（详见下文专节）

hostNetwork Pod 默认不会使用集群的 CoreDNS。这是因为 kubelet 为 Pod 生成 `/etc/resolv.conf` 的逻辑与 netns 的归属相关：普通 Pod 的 `nameserver` 指向 CoreDNS 的 ClusterIP；而 hostNetwork Pod 默认继承节点的 `/etc/resolv.conf`，那里的 `nameserver` 通常是节点的上游 DNS（如 VPC DNS 或公共 DNS），根本不认识 `kubernetes.default.svc.cluster.local` 这种集群内域名。

结果是：hostNetwork Pod 访问外部域名正常，但访问集群内任何 Service 名（如连接 CoreDNS、连接 kube-api、连接内部其他 Service）都会解析失败。修复方法是显式声明 `dnsPolicy: ClusterFirstWithHostNet`，见下文 DNS 策略专节。

### 5. 端口直接占用宿主端口，无任何隔离

hostNetwork 容器内进程 `bind(0.0.0.0, 9100)`，就是在节点上占用了 9100 端口。这与节点上任何其他进程（sshd、kubelet 自身、另一个 hostNetwork Pod）处于同一竞争域——没有 netns 隔离，没有 CNI 仲裁，操作系统级的端口占用规则直接生效。这就是为什么 hostNetwork 几乎总是和 DaemonSet 搭配：每节点恰好一个，端口规划由部署者手工保证。

```bash
# 🟢 低风险：在节点上查看被 hostNetwork Pod 占用的端口
ss -tlnp | grep -E ':(9100|10250|10257|2379|4194|8443)'
```

---

## hostPort 工作机制

### 1. iptables DNAT：宿主端口到 Pod 端口的桥

`hostPort` 的声明方式是在容器的 `ports` 列表中，给某个 `containerPort` 附加一个 `hostPort` 字段：

```yaml
# 🟡 中风险：hostPort 会在节点上写入 iptables 规则并占用节点端口
apiVersion: v1
kind: Pod
metadata:
  name: app-with-hostport
spec:
  containers:
  - name: app
    image: registry.example.com/app:v1
    ports:
    - containerPort: 8080       # Pod 内监听 8080
      hostPort: 18080           # 节点上监听 18080，转发到 Pod 8080
```

kubelet 在启动该 Pod 后，会在节点的 `nat` 表中写入一条 DNAT 规则。其本质等价于：

```
# 节点上 iptables 规则（简化，实际还含 KUBE-HOSTPORT 链与统计计数）
-A PREROUTING -p tcp --dport 18080 -j DNAT --to-destination <PodIP>:8080
-A OUTPUT     -p tcp --dport 18080 -j DNAT --to-destination <PodIP>:8080
```

可以用以下命令观察这条规则：

```bash
# 🟢 低风险：只读，查看 kubelet 写入的 hostPort DNAT 规则
iptables-save | grep -i hostport
# 或（nftables 后端）
nft list ruleset | grep -i hostport
```

关键区别在于：**Pod 仍然拥有独立 netns 与独立 Pod IP**。外部到 `节点IP:18080` 的包，在 PREROUTING 阶段被改写目的地址为 `<PodIP>:8080`，随后正常按 Pod 网络路由进入 veth pair，进入 Pod netns。这是一条「节点入口 → DNAT → Pod 网络」的转发路径，而 hostNetwork 是「根本没有 Pod 网络这一层」。

### 2. 与 hostNetwork 的本质区别

| 对比点 | hostNetwork | hostPort |
|---|---|---|
| Pod 是否有独立 netns | **否**（共享节点） | 是 |
| Pod 是否有独立 Pod IP | **否**（= 节点 IP） | 是 |
| 暴露的端口范围 | **Pod 内所有端口** | 仅声明了 hostPort 的端口 |
| Service 能否选它作后端 | 能但行为诡异（IP 是节点 IP） | 能且行为正常（IP 是 Pod IP） |
| NetworkPolicy 是否生效 | **否** | 是 |
| DNS 是否需要特殊配置 | 是（需 ClusterFirstWithHostNet） | 否（默认即可） |
| 性能开销 | 最低（无 veth） | 每包额外一次 DNAT 查找 |

一张图概括两者的数据路径差异：

```
[外部客户端]
   │
   │  访问 节点IP:18080
   ▼
┌─────────────────────────────────────────────┐
│  节点 netns                                  │
│   PREROUTING → DNAT → <PodIP>:8080   (hostPort)
│   或                                         │
│   进程直接 bind 0.0.0.0:9100         (hostNetwork)
└─────────────────────────────────────────────┘
   │                              │
   │ hostPort: 进入 veth          │ hostNetwork: 直接是节点进程
   ▼                              ▼
┌──────────────┐           (无独立 Pod netns)
│  Pod netns   │
│  eth0:8080   │
└──────────────┘
```

### 3. 性能开销：每包经 iptables DNAT

hostPort 的代价是**每一个进入包都要经过一次 iptables DNAT 查找**。在大型集群中，节点的 `nat` 表 KUBE-* 链可能有数千条规则（来自 Service、NetworkPolicy、其它 hostPort Pod），DNAT 查找从 O(1) 退化为线性扫描。这意味着：

- 小流量场景下，hostPort 与 NodePort 的性能差异可以忽略。
- 大流量、高 PPS 场景下，hostPort 的 DNAT 会成为 CPU 热点，反而比直接用 ClusterIP + Cluster 模式更慢。
- iptables-legacy 后端尤其明显；切到 ipvs 或 nftables 后端可缓解，但 DNAT 本身的开销仍在。

**因此 hostPort 在生产中极少被推荐**——它既没有 hostNetwork 那样的极致性能（绕过 veth），又比 NodePort 多一层 DNAT，还占用宝贵的节点端口。它的存在更多是历史兼容（早期没有 NodePort 的部署方式），以及某些特殊场景（如固定节点端口暴露给无法用 NodePort 范围的客户端）。

### 4. kubelet 的端口记录

kubelet 会在本节点维护一个「已用 hostPort」的记录集合（保存在 checkpoint 与内存中），新 Pod 启动前会检查其声明的 hostPort 是否已被占用。如果已被占用，kubelet 拒绝启动该 Pod，触发 `CrashLoopBackOff` 或调度失败重试。这套机制见下文「端口冲突与调度」专节。

---

## 端口冲突与调度

### 1. hostNetwork 的端口冲突：kubelet 启动兜底

hostNetwork 没有任何「端口唯一性调度」的保证。kube-scheduler 在调度 hostNetwork Pod 时，**默认不知道**该 Pod 会占用节点的哪些端口——因为 Pod spec 里 hostNetwork 只是布尔值，没有端口列表。冲突的发现完全依赖 kubelet 在启动容器时的 `bind()` 系统调用：

```
scheduler 选节点（不考虑端口）
  → Pod 被调度到节点 A
  → kubelet 尝试启动容器
  → 容器进程 bind(节点IP, 9100) 失败：EADDRINUSE
  → 容器启动失败 → CrashLoopBackOff
```

这就是为什么 hostNetwork Pod 几乎总是 DaemonSet：DaemonSet 保证每节点一个，部署者通过「每节点唯一」的隐式契约避免冲突。如果你用 Deployment + hostNetwork，replicas > 节点数时必然冲突——这是一个经典的反模式。

排障时，节点上的 kubelet 日志与容器日志会出现：

```
# 🟢 低风险：查看 kubelet 日志中的端口冲突证据
journalctl -u kubelet --since "10 min ago" | grep -iE 'bind|address already in use|EADDRINUSE'

# 🟢 低风险：在节点上确认端口占用者
ss -tlnp | grep ':9100'
```

### 2. hostPort 的端口冲突：scheduler 部分检查 + kubelet 兜底

hostPort 的情况略好，但远谈不上可靠。kube-scheduler 在调度阶段会读取节点上已声明的 hostPort（通过 Pod 的 `spec.containers[].ports[].hostPort` 字段），并在 `NodePorts`/`NodeAffinity` 等插件中避免把两个声明相同 hostPort 的 Pod 调度到同一节点。这套检查对**声明在 K8s Pod spec 中的 hostPort 是有效的**。

但它有三个盲区：

1. **检测不到非 K8s 进程的端口占用**。如果节点的 18080 已经被某个 systemd 服务或人工 `python -m http.server 18080` 占用，scheduler 完全不知情，Pod 仍会被调度过来，最终 kubelet 写 iptables 时虽然 DNAT 规则可以写入，但真正的 `bind` 冲突要等到流量打过来才暴露（DNAT 后到 PodIP:8080 没问题，但若该 Pod 自己又想监听 18080 则失败）。
2. **不支持固定节点端口的『软约束』**。hostPort 是硬声明，没有 NodePort 那种「自动分配」的回退。
3. **跨命名空间不可见**。如果 ns-a 的 Pod 声明了 hostPort 18080，ns-b 的 Pod 也声明 18080，scheduler 的检查是基于节点全局的，理论上能避免，但若两者使用了不同的调度器扩展或 admission 变更，可能存在时序窗口。

**生产建议**：不要依赖 hostPort 来暴露服务。需要固定节点端口时，用 NodePort（30000-32767）或 LoadBalancer；需要性能时，用 hostNetwork + DaemonSet。

### 3. 经典模式：DaemonSet + hostNetwork

这是 hostNetwork 最经得起考验的搭配。DaemonSet 保证每节点一个 Pod，从而把「同节点同端口只能一个」的约束自动满足：

```yaml
# 🟡 中风险：hostNetwork DaemonSet，部署前确认节点上目标端口未被占用
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: node-exporter
  template:
    metadata:
      labels:
        app: node-exporter
    spec:
      hostNetwork: true              # 关键：直接用节点网络
      dnsPolicy: ClusterFirstWithHostNet   # 关键：让 Pod 仍能解析集群 Service
      hostPID: false                 # 配合：不需要时关闭，减少攻击面
      tolerations:
      - operator: Exists             # DaemonSet 常打在所有节点（含 master）
      containers:
      - name: node-exporter
        image: prom/node-exporter:v1.8.2
        args:
        - --web.listen-address=:9100
        ports:
        - containerPort: 9100
          name: metrics
        resources:
          requests: { cpu: 50m, memory: 64Mi }
          limits:   { cpu: 200m, memory: 128Mi }
        securityContext:
          runAsNonRoot: true
          runAsUser: 65534
          allowPrivilegeEscalation: false
          readOnlyRootFilesystem: true
          capabilities:
            drop: [ALL]
```

要点：`hostNetwork: true` 与 `dnsPolicy: ClusterFirstWithHostNet` 必须同时出现；端口 9100 由 DaemonSet 保证唯一；securityContext 收紧权限以对冲 hostNetwork 带来的隔离损失。DaemonSet 的更多实践见 [[概念/daemonset.md|DaemonSet]]。

---

## DNS 策略：ClusterFirstWithHostNet 详解

DNS 是 hostNetwork 排障的最高频痛点。绝大多数「我起了一个 hostNetwork Pod，结果它连不上 CoreDNS / 连不上数据库 Service / 报 name resolution failed」的工单，根因都是 `dnsPolicy` 没配置正确。

### 1. 四种 dnsPolicy 的行为矩阵

| dnsPolicy | /etc/resolv.conf 的 nameserver | search domain | 适用 Pod |
|---|---|---|---|
| `ClusterFirst`（默认） | CoreDNS 的 ClusterIP | 集群后缀（`<ns>.svc.cluster.local ...`） | 普通 Pod（**hostNetwork 下失效**） |
| `ClusterFirstWithHostNet` | CoreDNS 的 ClusterIP | 集群后缀 | **hostNetwork Pod（必需）** |
| `Default` | 节点 `/etc/resolv.conf` 的 nameserver | 节点的 search domain | 仅需外部 DNS 的 Pod |
| `None` | 由 `dnsConfig` 完全自定义 | 由 `dnsConfig` 指定 | 特殊 DNS 需求 |

### 2. 为什么 ClusterFirst 在 hostNetwork 下不工作

关键在于 kubelet 生成 Pod `/etc/resolv.conf` 的逻辑分支：

```
if pod.Spec.HostNetwork {
    if pod.Spec.DNSPolicy == ClusterFirstWithHostNet {
        生成走 CoreDNS 的 resolv.conf
    } else {
        // 包括 DNSPolicy == ClusterFirst（默认）的情况
        继承节点的 /etc/resolv.conf
    }
} else {
    // 普通 Pod
    按 DNSPolicy 生成（ClusterFirst → CoreDNS）
}
```

也就是说，**`ClusterFirst`（默认值）在 hostNetwork Pod 上不会触发「走 CoreDNS」的逻辑分支**——kubelet 看到 `HostNetwork==true` 且 `DNSPolicy != ClusterFirstWithHostNet`，就把它当成「该用节点 DNS」处理。Pod 拿到的是节点的 `/etc/resolv.conf`，里面的 nameserver 是 VPC DNS 或公共 DNS，search domain 也不是集群后缀，因此：

- `kubernetes.default.svc.cluster.local` → NXDOMAIN（VPC DNS 不认识）
- `my-db.production.svc.cluster.local` → NXDOMAIN
- `api.example.com`（外部）→ 正常解析

这是一个非常隐蔽的陷阱：**Pod 访问外部一切正常，唯独访问集群内服务全失败**。新人很容易误判为「网络不通」而去做 conntrack/tcpdump 排查，浪费数小时。

### 3. 正确配置：ClusterFirstWithHostNet

修复方法只有一行：

```yaml
# 🟡 中风险：dnsPolicy 影响 Pod 解析行为，修改后需重启 Pod 生效
spec:
  hostNetwork: true
  dnsPolicy: ClusterFirstWithHostNet   # 让 hostNetwork Pod 仍用 CoreDNS
```

设置后，kubelet 会为该 Pod 生成与普通 Pod 几乎一致的 `/etc/resolv.conf`（nameserver 指向 CoreDNS ClusterIP，search domain 含集群后缀），DNS 流量从节点网络栈出发，经 kube-proxy 的 Service 规则到达 CoreDNS。

验证：

```bash
# 🟢 低风险：只读，检查 hostNetwork Pod 的 resolv.conf
kubectl -n monitoring exec ds/node-exporter -- cat /etc/resolv.conf
# 期望看到 nameserver <kube-dns ClusterIP> 与 search <ns>.svc.cluster.local ...

# 🟢 低风险：在 hostNetwork Pod 内测试集群域名解析
kubectl -n monitoring exec ds/node-exporter -- \
  nslookup kubernetes.default.svc.cluster.local
```

### 4. 何时用 Default 或 None

- **`Default`**：当 hostNetwork Pod 确实只需要访问外部域名（如一个仅上报指标到外部 SaaS 的 agent），可以用 Default 绕过 CoreDNS，减少 CoreDNS 负载。但务必确认它**不需要**访问任何集群内 Service。
- **`None`**：需要完全自定义 DNS（如指定特定的上游 DNS、特殊的 search domain、自定义 ndots）时，配合 `dnsConfig` 字段：

```yaml
# 🟡 中风险：自定义 DNS，需确保配置正确
spec:
  hostNetwork: true
  dnsPolicy: None
  dnsConfig:
    nameservers: ["169.254.25.10"]    # 阿里云 VPC DNS 或自定义
    searches: ["prod.svc.cluster.local", "svc.cluster.local"]
    options:
    - name: ndots
      value: "2"
    - name: single-request-reopen      # 规避 conntrack 竞态
```

更多 DNS 调优细节（NDots 放大、autopath、NodeLocal DNSCache）见 [[网络/K8s网络核心/50-dns-advanced-external-integration.md|DNS 高级与外部集成]]。

---

## 典型场景与选型

### 1. node-exporter / 节点级监控 agent

Prometheus node-exporter 是 hostNetwork 的教科书用例。它需要采集**节点本身**的指标（CPU、磁盘、网络接口），如果放进普通 Pod netns，它看到的 `eth0` 是 veth pair、看到的网络统计是 Pod 的而非节点的，指标语义就错了。hostNetwork 让它直接读到节点的 `/proc/net/dev`、节点的网络接口列表。

同理适用于：kube-state-metrics（如需采集节点级 cgroup）、各类节点安全 agent（Falco、Tracee、Tetragon 的 sensor 组件）。

### 2. CNI 自身：Calico / Cilium / Flannel 的节点 agent

CNI 的节点 agent（Calico 的 `calico-node`、Cilium 的 `cilium-agent`、Flannel 的 `flanneld`）几乎清一色用 hostNetwork。原因很根本：**这些 agent 的工作就是配置节点网络本身**——它们要写节点的路由表、节点的 iptables、挂载 eBPF 程序到节点的网络接口。把它们关在独立 netns 里等于让它们无法触碰要管理的对象。

Cilium 的 Pod 还会使用 `hostNetwork: true` + `privileged: true`（或一组非常宽的 capabilities），因为 eBPF 加载、TC hook 都需要 `CAP_BPF`、`CAP_NET_ADMIN` 等高权限。这是 hostNetwork 与特权容器并存的少数合理场景。

### 3. Ingress Controller（争议场景）

NGINX Ingress Controller、Traefik 等支持 hostNetwork 模式。其吸引力在于：默认 Ingress 通过 Service（Type=LoadBalancer 或 NodePort）进入，再经 kube-proxy 转发到 Ingress Controller Pod，多了一跳；hostNetwork 让 Ingress 直接监听节点的 80/443，外部流量直达 Pod，延迟更低、kube-proxy 的 NAT 开销消失。

**但这是一个有重大代价的优化**：

- Ingress Controller Pod 不再受 Deployment 的灵活调度约束——它绑死在特定节点（因为占用了 80/443），节点故障时需要外部 LB 感知并切换。
- NetworkPolicy 对它失效，攻击面暴露在节点 80/443。
- HPA 与滚动更新变复杂，因为每节点只能一个。
- 失去了 Service 的负载均衡，多副本必须手工保证端口与节点对应。

**现代实践**：优先用 Type=LoadBalancer + Ingress Controller 作为 Service 后端；只有在对延迟极致敏感、且 LB 成本不可接受的小规模集群，才评估 hostNetwork 模式。Cilium 的 L4 LB-by-eBPF 甚至能在保留 Service 抽象的同时获得接近 hostNetwork 的性能，是更好的折中。

### 4. 高性能数据平面

对延迟极度敏感的工作负载（HFT、实时推理、低延迟 RPC）会用 hostNetwork 绕过 veth pair。veth 的开销主要在两次上下文切换（发送端的软中断 → 接收端的软中断）与 conntrack 查找；hostNetwork 把通信双方放进同一 netns（节点），可以走 loopback，延迟从几十微秒降到几微秒。

但这条路代价高昂：失去 NetworkPolicy、失去 Service 抽象、失去 CNI 的可观测性。通常只在「Pod 与本节点的另一 Pod 通信」这一狭窄场景下值得。跨节点通信时 hostNetwork 反而失去意义（仍要走物理网卡）。

### 5. 选型决策表

| 需求 | 推荐方案 | 不推荐 |
|---|---|---|
| 暴露 HTTP 服务到集群外 | LoadBalancer / Ingress | hostPort |
| 暴露固定 TCP 端口到节点 | NodePort | hostPort（除非端口在 NodePort 范围外） |
| 节点级监控 agent | hostNetwork + DaemonSet | hostPort |
| CNI / 网络 agent | hostNetwork + 特权 + DaemonSet | 普通 Pod |
| 低延迟 Pod 间通信（同节点） | hostNetwork（评估 Cilium 替代） | hostPort |
| 临时调试端口转发 | `kubectl port-forward` | hostPort |
| 固定节点端口暴露单个 Pod | hostPort（次选） | hostNetwork（过度） |

---

## 安全权衡

hostNetwork 的安全代价是结构性的，不能用「配置得当就安全」来回避。它打破了 K8s 网络模型的三层隔离：

### 1. 流量可见性

hostNetwork Pod 与节点上所有进程共享网络栈，它能：

- 用 `tcpdump` 抓取**节点上所有其他 Pod** 的流量（普通 Pod 只能看到自己的 veth 流量）。
- 监听节点上其他进程的 loopback 通信。
- 读取节点的路由表、ARP 表、conntrack 表，获取集群拓扑信息。

这对一个被攻陷的 Pod 而言是巨大的信息泄露面。

### 2. NetworkPolicy 完全失效

NetworkPolicy 由 CNI 实现，其工作前提是「Pod 在 Pod 网络中、流量经过 CNI 的 hook 点」。hostNetwork Pod 不在 Pod 网络，CNI 看不到它的流量，因此：

```yaml
# 🟡 这个 NetworkPolicy 对 hostNetwork Pod 完全无效
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-all
  namespace: monitoring
spec:
  podSelector: {}    # 选中所有 Pod，包括 hostNetwork 的
  policyTypes: [Ingress, Egress]
```

上面的策略会让所有**普通 Pod** 的进出流量被拒绝，但 hostNetwork Pod 的流量完全不受影响——它直接走节点协议栈，根本不经过 CNI 的策略执行点。这意味着「我用 NetworkPolicy 隔离了命名空间」这种安全假设在存在 hostNetwork Pod 时**直接被打破**。

Cilium 在较新版本中通过 eBPF 的 host-scope policy 部分覆盖了这一盲区，但仍非标准行为，不能跨 CNI 假设。

### 3. 特权端口与攻击面

hostNetwork 让容器内进程可以 `bind` 节点的特权端口（< 1024，若 Pod 又有 `NET_BIND_SERVICE` capability 或节点 sysctl `net.ipv4.ip_unprivileged_port_start` 调整过）。这给了恶意或被攻陷的进程冒充节点关键服务（如 80/443/53）的能力，进行 MITM 或 DoS。

### 4. 强制对冲措施

如果业务确实需要 hostNetwork，必须同时部署以下对冲：

```yaml
# 🟡 中风险：hostNetwork Pod 的安全基线，缺一不可
spec:
  hostNetwork: true
  dnsPolicy: ClusterFirstWithHostNet
  securityContext:
    runAsNonRoot: true
    runAsUser: 65534
    fsGroup: 65534
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop: [ALL]            # 必须显式 drop ALL，再按需 add
      privileged: false
  # 建议配合 admission 强制 restricted PSS
```

并在集群层面：

- 用 Pod Security Admission（PSS）的 `restricted` 策略约束带 `hostNetwork: true` 的 Pod，至少在非系统命名空间拒绝。
- 用 OPA Gatekeeper / Kyverno 写策略，要求 `hostNetwork: true` 必须同时声明 `dnsPolicy: ClusterFirstWithHostNet` 且 `securityContext.capabilities.drop` 含 `ALL`。
- 把 hostNetwork Pod 限制在专用命名空间（如 `kube-system`、`monitoring`），用 RBAC 阻止业务团队在应用命名空间使用。

网络安全策略的更广议题见 [[安全/网络安全/02-network-security-policies.md|网络安全策略]]。

### 5. 红线

生产环境的红线很清晰：**hostNetwork 只用于无法替代的系统组件**（CNI agent、节点监控、节点安全 agent）。业务应用使用 hostNetwork 几乎总是错误的——它要么是为了绕过对 NetworkPolicy 的设计（这本身就是安全缺陷），要么是为了「省一跳性能」而牺牲了隔离性（性能收益通常远小于维护成本）。如果非要为业务用 hostNetwork，先评估 Cilium 的 socket-LB / XDP 是否能在不破坏隔离的前提下达到同等性能。

---

## 生产实践

### 1. 优先级倒置：hostNetwork 是最后选择

在选择「如何把 Pod 暴露出去」时，遵循从弱破坏到强破坏的顺序：

```
ClusterIP → Headless Service → NodePort → LoadBalancer → hostPort → hostNetwork
   ↑                                                              ↑
 最推荐                                                        最后手段
```

只有在前面所有方案都不可行时，才考虑 hostNetwork。hostPort 几乎不在推荐列表内——它既不安全也不快，是 NodePort 的劣化版本。

### 2. DaemonSet + hostNetwork 的标准清单

任何 hostNetwork DaemonSet 都应满足：

- [x] `hostNetwork: true`
- [x] `dnsPolicy: ClusterFirstWithHostNet`
- [x] 端口已规划且在节点上唯一（用 `ss -tlnp` 核对）
- [x] `tolerations` 覆盖目标节点（含 master，如需）
- [x] `securityContext` 收紧（drop ALL capabilities、非 root、只读根文件系统）
- [x] `resources` 有 requests/limits（hostNetwork Pod 不受限会抢占业务 Pod 资源）
- [x] `priorityClass` 设为系统级（确保资源紧张时不被驱逐）
- [x] 健康检查（livenessProbe/readinessProbe）配置
- [x] 文档标注该端口占用，避免后续冲突

### 3. 排查 hostNetwork Pod 是否存在

集群巡检时，列出所有 hostNetwork Pod，确认它们都是预期的系统组件：

```bash
# 🟢 低风险：只读，列出全集群 hostNetwork Pod
kubectl get pods -A -o jsonpath='{range .items[?(@.spec.hostNetwork)]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'

# 🟢 低风险：只读，列出全集群使用 hostPort 的 Pod
kubectl get pods -A -o jsonpath='{range .items[*]}{range .spec.containers[*].ports[*]}{.hostPort}{"\t"}{@}{"\n"}{end}{end}' | grep -v '^$' | sort -u
```

期望结果只包含 `kube-system`（CNI、kube-proxy）、`monitoring`（exporter）、安全 agent 命名空间。任何出现在业务命名空间的 hostNetwork Pod 都应视为可疑并审查。

### 4. 端口规划表

hostNetwork/hostPort 的端口需要手工规划。以下是一个常见端口的分配范例：

| 端口 | 典型占用者 | 备注 |
|---|---|---|
| 53 | CoreDNS（普通 Pod，非 hostNetwork） | 不要与节点 DNS 冲突 |
| 80 / 443 | Ingress（若 hostNetwork） | 仅在专用 Ingress 节点 |
| 4789 | VXLAN（Flannel/Calico） | CNI 数据面，勿占用 |
| 6443 | kube-apiserver（节点进程） | 勿占用 |
| 9090 | Prometheus / Alertmanager | hostNetwork 时需规划 |
| 9100 | node-exporter | 经典 hostNetwork 端口 |
| 10250 / 10257 / 10259 | kubelet / kube-controller-manager / kube-scheduler | 节点组件，勿占用 |
| 2379 / 2380 | etcd | 节点组件，勿占用 |

### 5. 监控与告警

应对节点端口占用建立监控，及时发现未授权的 hostNetwork Pod：

```yaml
# 🟢 低风险：Prometheus 告警规则示例
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: hostnetwork-alerts
spec:
  groups:
  - name: hostnetwork
    rules:
    - alert: UnexpectedHostNetworkPod
      # 配合一个 exporter 暴露 hostNetwork Pod 列表，或在 Grafana 手工巡检
      expr: kube_pod_spec_host_network{namespace!~"kube-system|monitoring|security"} == 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "非系统命名空间出现 hostNetwork Pod {{ $labels pod }}"
```

---

## 排障

### 症状 1：hostNetwork Pod DNS 解析失败

**现象**：hostNetwork Pod 启动正常，但访问 `kubernetes.default.svc.cluster.local` 或任何集群内 Service 名报 `name resolution failed` / `NXDOMAIN`；访问外部域名（如 `api.github.com`）正常。

**根因**：`dnsPolicy` 未设为 `ClusterFirstWithHostNet`，Pod 继承了节点 `/etc/resolv.conf`，nameserver 不是 CoreDNS。

**处置**：

```bash
# 🟢 低风险：只读，确认 Pod 的 resolv.conf
kubectl -n <ns> exec <pod> -- cat /etc/resolv.conf
# 若 nameserver 不是 kube-dns ClusterIP，即为该问题

# 🟢 低风险：只读，确认 Pod 的 dnsPolicy
kubectl get pod <pod> -o jsonpath='{.spec.dnsPolicy}{"\n"}'
```

修复：在 Pod/DaemonSet spec 中显式设置 `dnsPolicy: ClusterFirstWithHostNet`，删除并重建 Pod。

### 症状 2：Pod 启动失败 `bind: address already in use`

**现象**：hostNetwork 或 hostPort Pod 反复 `CrashLoopBackOff`，容器日志含 `bind: address already in use` / `EADDRINUSE`。

**根因**：目标节点上该端口已被占用——可能是另一个 hostNetwork/hostPort Pod，也可能是节点上的非 K8s 进程（systemd 服务、人工启动的进程），还可能是上一代 Pod 未完全退出（容器进程已死但 socket 处于 TIME_WAIT）。

**处置**：

```bash
# 🟢 低风险：在节点上定位端口占用者
ssh <node>
ss -tlnp | grep ':<port>'

# 🟢 低风险：若是 TIME_WAIT 堆积，查看连接状态
ss -tan state time-wait | wc -l
```

- 若是另一 K8s Pod 占用：检查是否存在两个声明同端口的 Pod（用上面「列出 hostNetwork Pod」的命令）。
- 若是非 K8s 进程：迁移该进程或换端口。
- 若是 TIME_WAIT：等待其释放（通常 60s），或在应用层启用 `SO_REUSEADDR`。

### 症状 3：Service 不路由到 hostNetwork Pod

**现象**：把 hostNetwork Pod 加入某 Service 的 selector，从集群内其他 Pod 访问该 Service ClusterIP，时通时不通，或完全不通。

**根因**：kube-proxy 的 DNAT 把 ClusterIP 改写为 hostNetwork Pod 的「Pod IP」，而该 IP 就是节点 IP。流量被路由到该节点后，进入节点的 host netns，但节点的入站连接处理与 Pod 的 Service 后端语义不匹配——尤其是 `externalTrafficPolicy: Local` 时，只有同节点客户端能命中。

**处置**：**不要把 hostNetwork Pod 作为普通 Service 的后端**。若必须暴露 hostNetwork Pod，用 `DaemonSet + hostNetwork + 直接节点IP:port` 访问，或用一个独立的 Proxy Pod（普通 Pod）做转发层。

### 症状 4：NetworkPolicy 对 hostNetwork Pod 不生效

**现象**：声明了 `deny-all` NetworkPolicy，但 hostNetwork Pod 仍能自由进出流量；或反向，期望某 NetworkPolicy 放行 hostNetwork Pod 却无法命中。

**根因**：NetworkPolicy 由 CNI 在 Pod 网络的 hook 点执行，hostNetwork Pod 不经过这些 hook 点，策略天然失效。**这是预期行为，非 bug**。

**处置**：

- 对 hostNetwork Pod，不能用 NetworkPolicy 隔离，只能依赖节点防火墙（如 iptables 的 INPUT 链）、云厂商的安全组、或 Cilium 的 host-scope policy（若部署 Cilium）。
- 在安全文档中明确标注：hostNetwork Pod 是 NetworkPolicy 的盲区，必须用最小权限 securityContext + 节点级防火墙对冲。

### 症状 5：hostPort Pod 调度到节点后不通

**现象**：Pod 已 Running，但访问 `节点IP:hostPort` 不通；Pod 内 `containerPort` 直连（经 ClusterIP）正常。

**根因**：kubelet 写入的 DNAT 规则可能未生效（iptables 后端异常、规则被其他工具覆盖），或节点防火墙（如云厂商安全组）拦截了 hostPort 端口。

**处置**：

```bash
# 🟢 低风险：在节点上检查 DNAT 规则是否存在
iptables-save | grep <hostPort>
nft list ruleset 2>/dev/null | grep <hostPort>

# 🟢 低风险：从节点本地测试
curl -v 127.0.0.1:<hostPort>

# 🟢 低风险：确认云安全组放行该端口（在云控制台）
```

### 排查决策树

```
hostNetwork/hostPort 异常
├── DNS 解析失败?
│   └── 检查 dnsPolicy 是否为 ClusterFirstWithHostNet
├── Pod 启动 bind 失败?
│   └── 节点端口冲突 → ss -tlnp 定位占用者
├── Service 访问 hostNetwork Pod 不通?
│   └── 预期行为 → 改用节点IP直连，不把 hostNetwork Pod 作 Service 后端
├── NetworkPolicy 不生效?
│   └── 预期行为 → 用节点防火墙/安全组替代
└── hostPort 不通?
    └── 检查 DNAT 规则 + 云安全组
```

---

## 常见误区

**误区一：「hostPort 是 hostNetwork 的弱化版」。** 不是。hostPort 保留 Pod netns 与 Pod IP，hostNetwork 完全取消 Pod netns。两者的隔离性、Service 行为、NetworkPolicy 有效性、DNS 行为都不同。

**误区二：「hostNetwork Pod 不需要配 dnsPolicy，它会自动用集群 DNS」。** 错。hostNetwork Pod 默认继承节点 resolv.conf，必须显式 `ClusterFirstWithHostNet` 才能用 CoreDNS。这是最常见的工单根因。

**误区三：「NetworkPolicy 能限制 hostNetwork Pod」。** 不能。NetworkPolicy 在 CNI 数据平面执行，hostNetwork Pod 绕过该平面，策略对它无效。

**误区四：「Deployment + hostNetwork 可以水平扩展」。** 不行。每个节点最多一个同端口 hostNetwork Pod，Deployment replicas > 节点数必然冲突。hostNetwork 几乎只与 DaemonSet 搭配。

**误区五：「hostPort 比 NodePort 性能好」。** 一般不成立。hostPort 每包经 DNAT，NodePort 也是 DNAT，开销相当；而 NodePort 有 scheduler 的端口管理、Service 抽象、健康检查后端，远优于 hostPort。

**误区六：「CNI 的 hostNetwork Pod 也受 NetworkPolicy 管控」。** 不受。即使是 Cilium 自己的 agent Pod，作为 hostNetwork Pod 也不受标准 NetworkPolicy 约束（Cilium 通过 host policy 单独管理）。

---

## 相关文档

- [[网络/K8s网络核心/01-network-architecture-overview.md|K8s 网络架构总览]] — Pod 网络命名空间、CNI 与 kube-proxy 的整体模型，是理解 hostNetwork「绕过了什么」的基础
- [[网络/K8s网络核心/06-service-concepts-types.md|Service 概念与类型]] — Service 的 ClusterIP/NodePort/LoadBalancer 模型，hostNetwork 与之的交互细节
- [[网络/K8s网络核心/11-dns-service-discovery-coredns.md|DNS 服务发现 CoreDNS]] — CoreDNS 工作机制，理解 dnsPolicy 各取值背后的解析路径
- [[网络/K8s网络核心/50-dns-advanced-external-integration.md|DNS 高级与外部集成]] — NDots、autopath、NodeLocal DNSCache，hostNetwork Pod 同样适用
- [[概念/daemonset.md|DaemonSet]] — hostNetwork 最常见的载体，DaemonSet + hostNetwork 的部署模式
- [[安全/网络安全/02-network-security-policies.md|网络安全策略]] — NetworkPolicy 的能力边界，为何对 hostNetwork Pod 失效

<!-- risk-assessed -->
