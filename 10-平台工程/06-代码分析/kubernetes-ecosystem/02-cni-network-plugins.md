---
title: CNI 网络插件集成源码分析
description: 基于 cni/libcni、flannel-0.28.7、terway-1.17.5、cilium-1.19.6 本地源码的 CNI 调用链剖析，Calico 机制级对比与 Pod 网络排障方法
summary: 从 libcni 的 AddNetworkList 出发（行号实测），拆解容器运行时调用 CNI 的完整链路、flannel VXLAN/terway ENI 两种典型数据面实现，结合 cilium-1.19.6 源码剖析 eBPF 数据面与 kube-proxy replacement（行号实测），机制级对比 Calico BGP，给出 Pod 网络分层排障方法。
category: source-analysis
tags:
- k8s
- source-code
- cni
- flannel
- calico
- cilium
- terway
- vxlan
tier: core
created: '2026-07-25'
last_updated: 2026-07
difficulty: expert
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 30min
intent_queries:
- CNI 插件如何被调用
- flannel vxlan 源码工作原理
- Calico 与 Cilium 如何选型
- Pod 拿不到 IP 怎么排查
trigger_keywords:
- CNI
- libcni
- flannel
- calico
- cilium
- terway
- vxlan
- eBPF
- NetworkPolicy
related_domains:
- 网络
- 集群基础
- 容器运行时
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# CNI 网络插件集成源码分析

> **源码基线**：`33-源码/网络/{cni-main,flannel-0.28.7,terway-1.17.5,cilium-1.19.6}/`（行号实测）；Calico 为机制级分析（源码树待入库，见 [[33-源码/README.md|33-源码 待补充清单]]）
> 本篇属 [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 生态集成系列]]。

## 一、CNI 的调用位置：运行时而非 kubelet

最常见的误解是「kubelet 调 CNI」。实际链路（对接 [[10-平台工程/06-代码分析/kubernetes-ecosystem/01-container-runtime-cri.md|01 篇]]）：

```
kubelet ──CRI RunPodSandbox──▶ containerd/CRI-O
                                   │ 创建 pause 容器 + netns
                                   │ 调 libcni（进程内库调用）
                                   ▼
                     读 /etc/cni/net.d/*.conflist（字典序取首个）
                     exec /opt/cni/bin/<plugin>（环境变量+stdin 传参）
                                   ▼
                     链式执行：主插件(flannel/calico/...) → ipam → portmap/bandwidth...
```

```go
// cni-main/libcni/api.go（实测行号）
func (c *CNIConfig) AddNetworkList(ctx, list *NetworkConfigList, rt *RuntimeConf) (types.Result, error)  // :515
```

`AddNetworkList` 按 conflist 中 `plugins` 数组顺序逐个 exec 二进制并把前一个的 Result 作为 `prevResult` 传给下一个——这就是「CNI 插件链」：主插件负责接口与 IP，portmap 补 hostPort 规则，bandwidth 挂 tbf 限速。三个动词 `ADD/DEL/CHECK` 经环境变量 `CNI_COMMAND` 传递，配置经 stdin。

源码级要点：

- **CNI 是一次性 exec 协议，不是常驻服务**：插件二进制执行完即退出。Calico/Cilium 等的常驻 agent（DaemonSet）与 CNI 二进制是两个角色——二进制只做「接线」，agent 负责路由/策略的持续调谐
- **`RunPodSandbox` 返回前 CNI 必须成功**：CNI 失败 = Pod 永远卡 `ContainerCreating`，事件里可见 `failed to setup network for sandbox`
- **DEL 必须幂等**：kubelet/运行时会对已不存在的 sandbox 重放 DEL，插件写错幂等性会导致 IP 泄漏（IPAM 池耗尽是典型后果）

## 二、flannel：最小可用的 overlay 实现

flannel 由两部分组成：CNI 二进制（薄，读 subnet.env 委托给 bridge+host-local 插件）与 flanneld DaemonSet（重，管子网与 backend）：

```go
// flannel-0.28.7（实测行号）
// pkg/subnet/kube/kube.go
func NewSubnetManager(ctx, apiUrl, kubeconfig, prefix, netConfPath, ...)  // :81  用 Node 对象当子网数据库
// pkg/backend/vxlan/vxlan.go
func New(sm subnet.Manager, be *backendConfig) (backend.Backend, error)  // :83  VXLAN backend 注册
```

- **kube subnet manager**（:81）：不再依赖独立 etcd，直接把每节点的 PodCIDR 分配写在 Node 的 `spec.podCIDR` 与 annotation 上——watch Node 即 watch 全网拓扑，这是 flannel 与 K8s 集成的核心落点
- **VXLAN backend**（:83）：每节点建 `flannel.1` VTEP 设备，其余节点的 MAC/IP 映射通过 watch Node 事件下发为 FDB/ARP/路由三张表。跨节点 Pod 流量：`veth → cni0 网桥 → flannel.1 封 UDP:8472 → 对端物理网卡`
- 生产定位：无 NetworkPolicy 能力（需搭配 Calico policy-only 即 Canal）、无加密（wireguard backend 除外）；胜在依赖极少、故障面小

## 三、terway：VPC 原生路线（无 overlay）

```go
// terway-1.17.5/plugin/terway/cni.go（实测行号）
func cmdAdd(args *skel.CmdArgs) error  // :67  CNI ADD 入口
```

terway 的 `cmdAdd` 不建 overlay，而是通过 unix socket 向节点上 terwayd 申请**真实 VPC 资源**（ENI 或 ENI 上的辅助 IP），Pod IP 就是 VPC IP——集群内外同一张网，无封包开销，代价是 IP 配额与 ENI 数受实例规格约束。详细容量规划见 [[05-网络/06-Terway/index.md|网络域：Terway]]。云厂商 CNI（AWS VPC CNI、Azure CNI）同属此路线。

## 四、Calico 与 Cilium：路线对比与 Cilium 源码落点

| | Calico | Cilium |
|---|-------|--------|
| 数据面 | 内核路由表 + iptables/nftables（可选 eBPF 模式） | eBPF（tc/XDP 挂载点） |
| 跨节点 | BGP 直路由（同网段免封装）/ IPIP / VXLAN | VXLAN/Geneve 或直路由 |
| NetworkPolicy | Felix agent 翻译为 iptables 规则 | 翻译为 eBPF map，支持 L7（HTTP/gRPC/Kafka）策略 |
| 身份模型 | 基于 IP/selector | 基于 identity（label 哈希），策略判定不依赖 IP |
| kube-proxy | 依赖 | **可完全替代**（见下） |
| 可观测性 | 常规日志/指标 | Hubble 流级可观测 |

### Cilium 源码落点（cilium-1.19.6，行号实测）

Cilium 同样遵循「CNI 二进制轻、常驻 agent 重」分工，但二进制不直接接线，而是转手给 agent：

```go
// plugins/cilium-cni/cmd/cmd.go
func (cmd *Cmd) Add(args *skel.CmdArgs) (err error)   // :523 CNI ADD 入口
    c.EndpointCreate(ep)                              // :860 向 cilium-agent 申请创建 endpoint（unix socket API）
func (cmd *Cmd) Del(args *skel.CmdArgs) error         // :905 DEL → EndpointDelete（幂等）

// pkg/k8s/watchers/watcher.go（agent 侧的 K8s 集成点）
func (k *K8sWatcher) InitK8sSubsystem(ctx)            // :237 启动全部 K8s watcher
func (k *K8sWatcher) enableK8sWatchers(ctx, ...)      // :266 按资源类型逐个挂 Informer

// pkg/k8s/network_policy.go
func ParseNetworkPolicy(logger, clusterName, np)      // :147 K8s NetworkPolicy → Cilium PolicyEntries

// pkg/endpoint/bpf.go
func (e *Endpoint) regenerateBPF(regenContext)        // :375 策略/配置变更 → 重编译+重载该 endpoint 的 eBPF 程序
```

两个结构性事实：

1. **endpoint 是 Cilium 的基本调谐单元**：CNI Add（:523）只负责建 veth 并注册 endpoint（:860），后续策略变更不重新接线，而是走 regenerateBPF（:375）热替换 eBPF 字节码——这就是「策略生效不断连」的实现基础
2. **NetworkPolicy 的消费链清晰可寻**：Informer（:266）→ ParseNetworkPolicy（:147）→ identity 化策略表 → 受影响 endpoint 逐个 regenerate；策略不生效时沿这条链查 agent 日志即可分段定位

**Cilium kube-proxy replacement**：用 eBPF 在 socket 层（`connect()` 时直接改写目标地址）与 tc 层实现 Service 转发，绕过 [[10-平台工程/06-代码分析/kubernetes-core/09-kube-proxy-deep-dive.md|09 篇]]的 iptables/ipvs 全部规则链——查找 O(1)、无 conntrack DNAT 开销、无 syncProxyRules 收敛延迟。代价：排障工具链从 `iptables-save`/`conntrack` 换成 `cilium bpf lb list`/`hubble observe`，团队技能栈需同步切换。

**NetworkPolicy 的实现者是 CNI 而非 K8s**：apiserver 只存储 NetworkPolicy 对象，无任何内置执行逻辑。flannel 装了策略也不生效（无报错！）是最隐蔽的「假安全」配置，域内详述见 [[08-安全/02-网络安全/index.md|安全域：网络安全]]。

## 五、生产排障速查

| 症状 | 层次定位 | 检查手段 |
|------|---------|---------|
| Pod 卡 ContainerCreating（network 事件） | CNI ADD 失败 | `kubectl describe pod` 事件、`/etc/cni/net.d/` 配置、CNI agent DaemonSet 日志 |
| Pod 有 IP 但跨节点不通 | backend 数据面 | flannel：`bridge fdb show dev flannel.1`、UDP 8472 放行；Calico：BGP peer 状态 `calicoctl node status` |
| IPAM 池耗尽 | DEL 泄漏或 CIDR 过小 | host-local：`/var/lib/cni/networks/` 残留文件；节点 PodCIDR 容量 vs maxPods |
| NetworkPolicy 不生效 | CNI 无策略能力或消费链中断 | 确认插件支持（flannel 不支持）、Felix/Cilium agent 日志（Cilium 沿 ParseNetworkPolicy:147 → regenerateBPF:375 链路查） |
| DNS 间歇失败但网络正常 | conntrack/UDP（09 篇）或 CNI MTU | 封装模式下 MTU 需减去封装头（VXLAN -50），`ip link` 对照 |
| 节点级 Pod 全部不通 | CNI agent 挂/配置漂移 | DaemonSet Pod 状态、conflist 是否被其他插件覆盖 |

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 系列总览]]
- [[10-平台工程/06-代码分析/kubernetes-ecosystem/01-container-runtime-cri.md|01 - 容器运行时与 CRI 集成]]（CNI 的调用方）
- [[10-平台工程/06-代码分析/kubernetes-core/09-kube-proxy-deep-dive.md|kubernetes-core 09 - kube-proxy 源码深度剖析]]（被 Cilium 替代的对象）
- [[05-网络/01-K8s网络核心/index.md|网络域：K8s 网络核心]]
- [[05-网络/05-eBPF/index.md|网络域：eBPF]]
- [[05-网络/06-Terway/index.md|网络域：Terway]]
- [[08-安全/02-网络安全/index.md|安全域：网络安全]]
