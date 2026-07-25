---
title: 镜像仓库、DNS 与负载均衡集成源码分析
description: 基于 coredns-1.14.6 与 cloud-provider 本地源码的三大基础服务集成剖析：Harbor 镜像拉取链、CoreDNS kubernetes 插件、Service LB 与 Ingress/Gateway API
summary: 剖析集群三大基础依赖与 K8s 的集成点（行号实测）：镜像从 Harbor 到节点的完整拉取与认证链、CoreDNS 的 ServeDNS/kubernetes 插件如何把 Service 对象翻译成 DNS 记录、cloud-provider 的 EnsureLoadBalancer 与 Ingress/Gateway API 南北向体系，给出接入层排障方法。
category: source-analysis
tags:
- k8s
- source-code
- harbor
- coredns
- ingress
- loadbalancer
- gateway-api
- registry
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
- CoreDNS 如何解析 Service 域名
- LoadBalancer 类型 Service 谁来实现
- Harbor 镜像拉取认证链路
- Ingress 与 Gateway API 的关系
trigger_keywords:
- Harbor
- CoreDNS
- ndots
- LoadBalancer
- cloud-provider
- Ingress
- Gateway API
- imagePullSecrets
related_domains:
- 网络
- 安全
- 集群基础
k8s_versions:
- '1.36'
authors:
- name: KUDIG Team
  role: contributor
---

# 镜像仓库、DNS 与负载均衡集成源码分析

> **源码基线**：`33-源码/网络/coredns-1.14.6/` + `33-源码/平台工程/cloud-provider-master/`（行号实测）；Harbor/ingress-nginx 为机制级分析（源码树待入库，见 [[33-源码/README.md|33-源码 待补充清单]]）
> 本篇属 [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 生态集成系列]]。

## 一、Harbor：镜像供应链的上游

镜像拉取链路（衔接 [[10-平台工程/06-代码分析/kubernetes-ecosystem/01-container-runtime-cri.md|01 篇]] ImageService）：

```
Pod spec.image ─▶ kubelet 组装凭证 ─▶ CRI PullImage(:232) ─▶ containerd
                     │                                        │ OCI Distribution API
        imagePullSecrets（Pod 级）                  GET /v2/<name>/manifests/<tag>（Harbor）
        节点 config.json / credential provider              按层拉 blob（含代理缓存/P2P 分发）
```

集成点与生产要点：

- **凭证的三个来源与优先级**：Pod 的 `imagePullSecrets` > ServiceAccount 默认注入的 pull secret > 节点级配置（credential provider 插件可对接云厂商 registry 免密）。ImagePullBackOff 排凭证时先确认**用的是哪一层**
- **Harbor 的增值在仓库侧而非协议侧**：对 K8s 它就是标准 OCI registry；项目级 RBAC、漏洞扫描（Trivy）阻断策略、镜像签名（Cosign/Notation）+ 准入校验（Kyverno/policy webhook 验签）、多仓库复制（异地/跨云）——供应链完整体系见 [[08-安全/05-供应链/index.md|安全域：供应链]]
- **registry 挂了 ≠ 存量业务挂**：已拉取镜像配合 `imagePullPolicy: IfNotPresent` 可继续起容器；但 `Always` 策略/新节点扩容/驱逐重建全部失败——registry 的可用性等级应按「阻断扩容与自愈」评估，proxy-cache 项目 + 关键镜像预热是标准缓解

## 二、CoreDNS：把 Service 对象翻译成 DNS

```go
// coredns-1.14.6（实测行号）
// core/dnsserver/ 插件链入口
func (h Handler) ServeDNS / plugin chain            // handler.go:13   逐插件处理 DNS 请求
// plugin/kubernetes/kubernetes.go
func (k *Kubernetes) Services(ctx, state, exact, opt)  // :100  查 informer 缓存出记录
```

CoreDNS 是插件链架构（`Corefile` 每行一个插件），kubernetes 插件（:100）内嵌 informer watch Service/EndpointSlice/Namespace，**纯内存查表生成 DNS 记录、零外部查询**：

- `<svc>.<ns>.svc.cluster.local` → A 记录（ClusterIP）；headless Service → 全部 Pod IP；SRV 记录带端口
- ExternalName Service → CNAME；集群外域名经 `forward` 插件转上游
- **`ndots:5` 是集群 DNS 性能的第一话题**：kubelet 下发的 resolv.conf 设 `ndots:5`，短名（如 `mysql`、甚至 `example.com`——点数<5）会先按 search 域展开查 4 次（`.<ns>.svc`、`.svc`、`.cluster.local`、原名）——外部域名解析放大 4 倍 QPS。解法：FQDN 加尾点、NodeLocal DNSCache、调 Pod dnsConfig
- CoreDNS 自身以 Deployment 跑在集群内、经 Service VIP（通常 10.96.0.10）暴露——**它的可达性依赖 kube-proxy 规则**（[[10-平台工程/06-代码分析/kubernetes-core/09-kube-proxy-deep-dive.md|09 篇]]），「DNS 间歇失败」的 conntrack/UDP 陷阱正是两者交界处

## 三、LoadBalancer：Service 控制器与云 API

`type: LoadBalancer` 的 Service 由两段协作实现——集群内规则（kube-proxy，09 篇）+ 云 LB 建设（cloud-controller-manager）：

```go
// cloud-provider-master（实测行号）
// cloud.go —— 各云厂商实现的接口契约
func EnsureLoadBalancer(ctx, clusterName, service, nodes)     // :158  接口定义
// controllers/service/controller.go —— CCM 内的 Service 控制器
func (c *Controller) syncService(ctx, key)                    // :874  工作队列消费入口
func (c *Controller) processServiceCreateOrUpdate(ctx, ...)   // :325  调 EnsureLoadBalancer→回写 status
```

- 控制器 watch Service/Node，对 LoadBalancer 类型调云 API 建 LB、把结果写回 `status.loadBalancer.ingress`——**`EXTERNAL-IP` 一直 `<pending>` = 这条链路断了**：没装 CCM（自建集群裸用 LoadBalancer 类型）、云配额/权限、或 annotation 参数非法，看 CCM 日志与 Service events 即可定位
- 云 LB 的后端通常是「节点:NodePort」（再经 kube-proxy 二跳）；`externalTrafficPolicy: Local` + healthCheckNodePort 让 LB 只把流量给有端点的节点（保源 IP、省一跳）；部分云支持直连 Pod IP 模式（如 terway ENI，见 [[10-平台工程/06-代码分析/kubernetes-ecosystem/02-cni-network-plugins.md|02 篇]]）
- 裸金属集群的对位实现是 MetalLB：同一控制器模式，用 ARP/BGP 宣告 VIP 替代云 API

## 四、Ingress 与 Gateway API：七层南北向

L4（Service LB）之上的 HTTP 路由层同样是「声明对象 + 集群内控制器翻译」：

- **ingress-nginx**：controller watch Ingress/Service/EndpointSlice/Secret → 渲染 nginx.conf + lua 动态 upstream → reload/热更新。**upstream 直连 Pod IP 而非 Service VIP**（再次绕过 kube-proxy）——Ingress 后端 503 时应检查 EndpointSlice 而非 ClusterIP 连通性
- **Gateway API**：Ingress 的继任者，把「基础设施（Gateway/GatewayClass，平台组管）」与「路由（HTTPRoute，业务组管）」拆成不同对象做角色分权，表达力覆盖 header 路由/流量拆分/跨 ns 引用；Istio/Envoy Gateway/云厂商均以其为统一实现目标，选型对比见 [[05-网络/04-API网关/index.md|网络域：API 网关]]
- TLS 证书自动化（cert-manager）：又一个标准 Operator——watch Certificate CRD → ACME 挑战 → 写 Secret → Ingress 引用

## 五、生产排障速查

| 症状 | 链路定位 | 检查手段 |
|------|---------|---------|
| ImagePullBackOff（401/403） | 凭证链 | 确认生效的凭证层级、`crictl pull` 带凭证复现、Harbor 项目权限 |
| 扩容节点全部拉镜像失败 | registry 可用性/网络 | Harbor 健康、节点到 registry 连通性、proxy-cache |
| DNS 解析慢/外部域名放大 | ndots 展开 | Pod resolv.conf、CoreDNS 指标（请求量按 zone 分解）、NodeLocal DNSCache |
| DNS 间歇性失败 | conntrack/UDP 交界 | 09 篇 conntrack 排查、CoreDNS Pod 分布与副本数 |
| EXTERNAL-IP 一直 pending | CCM 链路 | CCM 是否部署、syncService (:874) 日志、Service events、云配额 |
| Ingress 503 但 Service 正常 | upstream 直连 Pod | EndpointSlice 是否有 ready 端点、Pod readiness、controller 日志 |

---

## 相关文档

- [[10-平台工程/06-代码分析/kubernetes-ecosystem/README.md|kubernetes-ecosystem 系列总览]]
- [[10-平台工程/06-代码分析/kubernetes-ecosystem/01-container-runtime-cri.md|01 - 容器运行时与 CRI 集成]]（镜像拉取执行侧）
- [[10-平台工程/06-代码分析/kubernetes-core/09-kube-proxy-deep-dive.md|kubernetes-core 09 - kube-proxy 源码深度剖析]]（DNS/LB 的数据面依赖）
- [[05-网络/01-K8s网络核心/index.md|网络域：K8s 网络核心]]（DNS 体系）
- [[05-网络/04-API网关/index.md|网络域：API 网关]]
- [[08-安全/05-供应链/index.md|安全域：供应链]]（Harbor 签名/扫描）
- [[18-云厂商/README.md|云厂商域]]（各云 LB 能力对比）
