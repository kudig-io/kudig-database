---
title: Service 无法访问 深度解析
description: 对 Service 不可达的五类典型场景进行 prose 决策树拆解，覆盖阿里云 SLB/NLB/ALB 集成、Terway 网络注意事项与特殊边界条件
category: Kubernetes-Incident-Response
tags:
- k8s
- service
- unreachable
- endpoint
- kube-proxy
- slb
- nlb
- alb
- terway
- aliyun
- deep-dive
- skills
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 网络工程师
estimated_read_time: 15min
intent_queries:
- 为什么 Service 无法访问
- 阿里云 ACK Service SLB 排障
- Terway 网络下 Service 有什么特殊注意
trigger_keywords:
- Service
- 无法访问
- Endpoint
- kube-proxy
- SLB
- NLB
- ALB
- Terway
prerequisites:
- service-connectivity-skill
k8s_versions:
- 1.28.x
- 1.30.x
- 1.32.x
skill_id: SKILL-SVC-001-DEEP
skill_name: Service 无法访问 深度解析
version: 1.0.0
created: "2026-06-26"
authors:
- name: KUDIG Team
  role: contributor

---

# Service 无法访问 深度解析

> 本文是 [[domain-10-troubleshooting-diagnostics/topic-skills/05-service-connectivity.md|Service 连通性与 Endpoint 异常诊断与修复]] 的深度补充，系统梳理 Endpoints 为空、Endpoints 有但不可达、DNS 失败、kube-proxy 异常、SLB 后端异常五类核心场景的决策树，并重点覆盖阿里云/专有云环境。

## 1. 决策树：Service 为什么不可达

Service 不可达的问题可以抽象为三个层次：**控制平面层**（Endpoints 是否存在且正确）、**数据平面层**（kube-proxy 是否将流量正确转发）、**外部入口层**（云 LB 是否将外部流量正确引入）。诊断时应按这个顺序逐层排除，避免一上来就抓包或重启组件。

### 1.1 Endpoints 为空

Endpoints 为空意味着 Kubernetes 认为当前没有可用的后端 Pod。此时所有通过 Service 的流量都没有目的地，访问结果通常是「connection refused」或「no endpoints available」。

排查这一步的核心命令是 `kubectl get endpoints <svc> -n <ns>`。为什么先看 Endpoints？因为 Endpoints 是 Service 与 Pod 之间的「桥梁」，它由 endpoint-controller 根据 Service selector 和 Pod readiness 状态自动生成。如果 Endpoints 为空，问题一定出在这座桥上，而不是桥后的网络路径。

Endpoints 为空的常见原因有两类：
- **Selector 不匹配**：Service 的 `spec.selector` 与 Pod 的 `metadata.labels` 不一致。这种错误常见于 Helm chart 升级后 label 被改写、或者多个团队分别维护 Deployment 和 Service。
- **后端 Pod 未就绪**：Pod 存在，但 readiness probe 失败，因此 controller 不会将其加入 Endpoints。此时应继续查看 Pod 状态和事件，而不是修改 Service。

### 1.2 Endpoints 有但不可达

Endpoints 有 IP 地址，但通过 Service ClusterIP 访问失败，这说明桥是存在的，但桥后的转发路径有问题。此时应区分两种情况：
- **直接访问 Pod IP 成功，但通过 ClusterIP 失败**：问题在 Service 数据平面，通常是 kube-proxy 规则缺失/错误、端口映射不匹配、或者 NetworkPolicy 阻断。
- **直接访问 Pod IP 也失败**：问题在后端 Pod 本身，可能是应用未监听端口、容器端口配置错误、或者 Pod 网络异常。

排查时先执行 `kubectl exec` 进入测试 Pod，分别用 Pod IP 和 ClusterIP 做对比测试。这个对比是 Service 排障中最关键的步骤，它能快速把问题限定在「Service 层」还是「Pod 层」。

### 1.3 DNS 失败

如果客户端使用 Service 名称访问时提示 `Unknown host` 或 `NXDOMAIN`，问题在 DNS 解析层，而不是 Service 本身。此时即使 Service 和 Endpoints 都正常，客户端也无法获得 ClusterIP。

DNS 失败常见原因包括：CoreDNS Pod 异常、Pod 的 `/etc/resolv.conf` 配置错误（如 search domain 缺失）、或者客户端使用了错误的域名格式。排查时应先用 `nslookup <service>.<namespace>.svc.cluster.local` 验证完整域名解析，再用短名称 `nslookup <service>` 验证 search domain 是否生效。

### 1.4 kube-proxy 异常

kube-proxy 负责把 Service 的虚拟 IP 转换为后端 Pod IP。如果 kube-proxy 异常，节点上的 iptables/IPVS/nftables 规则就会缺失或过期，导致 ClusterIP 流量无法被正确 DNAT。

排查 kube-proxy 时，不能只看 Pod 是否 Running。很多 kube-proxy 异常表现为「Pod Running 但规则未同步」，此时需要查看 kube-proxy 日志中是否有 `error syncing rules`，并在节点上检查是否存在对应的 iptables/IPVS/nftables 规则。不同 Kubernetes 版本的 kube-proxy 后端模式不同，1.32 中 nftables 已 GA，使用 `iptables-save` 可能看不到规则，需要改用 `nft list ruleset`。

### 1.5 SLB 后端异常

对于 LoadBalancer 类型 Service，外部流量先经过云厂商负载均衡器，再进入集群节点，最后由 kube-proxy 转发到 Pod。因此即使集群内部通过 ClusterIP 访问正常，外部流量仍可能在 SLB 层失败。

SLB 后端异常通常表现为：Service 的 EXTERNAL-IP 为 `<pending>`、或者 EXTERNAL-IP 已分配但外部访问超时/500。排查时应先看 `status.loadBalancer.ingress`，再检查 cloud-controller-manager 日志和云控制台中的 LB 实例状态。

## 2. 阿里云 SLB / NLB / ALB 在 K8s Service 中的集成与排障

### 2.1 SLB（传统型负载均衡）

ACK 中 `LoadBalancer` 类型 Service 默认会创建阿里云 SLB 实例。SLB 通过四层转发（TCP/UDP）将流量分发到后端 ECS。默认情况下，ACK 的 cloud-controller-manager 会自动为 SLB 添加所有节点作为后端，并在节点上通过 kube-proxy 完成第二次转发到 Pod。

SLB 排障要点：
- **健康检查**：SLB 默认使用 TCP 探测后端节点的 NodePort。如果节点上 kube-proxy 异常，或者 NodePort 对应的 Service 没有后端 Pod，健康检查会失败，SLB 将该节点移除。
- **监听端口**：确认 SLB 监听端口与 Service `spec.ports[].port` 一致，后端端口与 `NodePort` 一致。
- **后端服务器组**：在阿里云控制台检查 SLB 的后端服务器是否包含所有节点，以及是否有节点被标记为异常。
- **安全组**：SLB 访问节点 NodePort 需要节点安全组放行对应端口范围（默认 30000-32767）。

### 2.2 NLB（网络型负载均衡）

NLB 是阿里云新一代四层负载均衡，支持更高的并发和更低的延迟。在 ACK 中，可以通过 Service Annotation `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-type: nlb` 指定创建 NLB。

NLB 与传统 SLB 的关键差异：
- NLB 支持「后端直接挂载 Pod ENI」（Terway 模式下），流量可以不经过 kube-proxy 的二次转发，直接到达 Pod。
- NLB 支持弹性网卡直挂，后端维护由 ACK 自动同步，不需要像 SLB 那样维护节点列表。

NLB 排障要点：
- 确认 ACK 集群已开启 Terway，否则无法使用 ENI 直挂。
- 检查 NLB 后端服务器组中的 Pod IP 是否与当前 Endpoints 一致。
- 如果后端 Pod 跨可用区，检查 NLB 的跨可用区转发开关是否开启。

### 2.3 ALB（应用型负载均衡）

ALB 是七层负载均衡，通常与 ACK 的 Ingress（AlbConfig）配合使用，而不是直接作为 Service 的 LoadBalancer。但在某些场景下，也可以通过 ALB 的 Service 集成暴露七层服务。

ALB 排障要点：
- 七层健康检查默认使用 HTTP/HTTPS，需要确认后端应用提供了对应的健康检查路径。
- ALB 的后端服务器组需要与 ACK Service 的 Endpoints 保持同步，可以通过 AlbConfig 的监听规则检查。
- 如果使用了 HTTPS，需要确认证书已正确挂载，且后端 Pod 的 readiness probe 路径与 ALB 健康检查路径一致。

### 2.4 云厂商控制器日志

无论 SLB、NLB 还是 ALB，出现问题时都应检查 `kube-system` 命名空间下的 cloud-controller-manager 日志：

```bash
kubectl logs -n kube-system -l component=cloud-controller-manager --tail=100
```

这条命令之所以关键，是因为云厂商控制器的报错通常直接指出问题：权限不足（RAM 角色缺少 SLB 操作权限）、配额耗尽（一个账号下的 SLB 实例数达到上限）、子网 IP 不足、或者 Annotation 配置错误。

## 3. Terway 网络下的 Service 特殊注意事项

### 3.1 Pod IP 即 ENI 辅助 IP

在 ACK Terway 模式下，每个 Pod 独占一个 ENI 辅助 IP，Pod IP 就是 VPC 中的真实 IP。这意味着：
- Pod 之间的通信不需要 VXLAN 封装，性能更好。
- Service 的 ClusterIP 仍然由 kube-proxy 维护，但 LoadBalancer 可以直接将后端指向 Pod IP（NLB ENI 直挂）。

Terway 下的 Service 排障需要注意：Pod IP 必须在交换机的网段内，且交换机可用 IP 不能耗尽。如果交换机 IP 耗尽，新 Pod 无法分配 IP，Endpoints 也会为空或缺失。

### 3.2 安全组与 Pod 网络

Terway 模式下，Pod 的安全组规则与节点安全组是分离的。如果 Pod 的安全组未放行入向流量，即使 Service Endpoints 正常、kube-proxy 规则正确，流量到达 Pod 后也会被安全组丢弃。

排查时应检查：
- Pod 所在安全组是否允许来自 SLB/NLB/节点的流量。
- 节点安全组是否允许 Pod 之间的跨节点通信。
- 是否配置了 NetworkPolicy 与云安全组双重限制。

### 3.3 Terway 组件异常

Terway 本身以 DaemonSet 形式运行在每个节点上。如果 Terway Pod 异常，可能导致节点上的 Pod 网络中断，进而影响 Service 转发。排查时应检查：

```bash
kubectl get pods -n kube-system -l app=terway
kubectl logs -n kube-system -l app=terway --tail=100
```

Terway 异常通常与 ENI 配额、交换机 IP 耗尽、或者 RAM 角色权限有关。

## 4. 边界条件

### 4.1 Headless Service

Headless Service 的 `spec.clusterIP` 为 `None`，不分配 ClusterIP，DNS 直接返回后端 Pod 的 IP 列表。它通常用于 StatefulSet 的场景（如数据库主从、分布式协调服务）。

Headless Service 不可达的常见原因：
- Pod 未设置 `hostname` 和 `subdomain`，导致 DNS A 记录未注册。
- Pod 未就绪，DNS 不会返回未就绪 Pod 的地址（除非 `publishNotReadyAddresses=true`）。
- 客户端期望返回单个 IP，但 Headless Service 返回多个 IP，客户端未正确处理。

排查 Headless Service 时，应使用 `nslookup <service>.<namespace>.svc.cluster.local` 检查 DNS 是否返回 Pod IP 列表，而不是检查 ClusterIP。

### 4.2 externalTrafficPolicy

`externalTrafficPolicy` 控制外部流量进入集群后的转发行为：
- `Cluster`（默认）：外部流量到达任意节点后，kube-proxy 可能将其转发到其他节点的 Pod。这种方式会导致一次额外的跨节点跳转，但能确保负载均衡。
- `Local`：外部流量只转发到当前节点上的本地 Pod。如果当前节点没有就绪的后端 Pod，流量会被直接丢弃。

`externalTrafficPolicy=Local` 是最常见的边界问题源。表现为「通过某些节点访问正常，通过其他节点访问失败」。排查时应检查访问请求命中了哪个节点，以及该节点上是否有目标 Pod。

### 4.3 sessionAffinity

`sessionAffinity: ClientIP` 会让同一客户端 IP 的请求固定转发到同一个后端 Pod。超时时间由 `sessionAffinityConfig.clientIP.timeoutSeconds` 控制，默认 10800 秒（3 小时）。

sessionAffinity 的边界问题包括：
- 当某个后端 Pod 异常时，粘滞到该 Pod 的客户端会持续失败，直到 affinity 超时。
- 在 NAT 场景下，大量客户端共用同一个源 IP，导致流量严重不均，甚至压垮单个 Pod。
- 与 `externalTrafficPolicy=Local` 结合使用时，行为会更加复杂。

排查 sessionAffinity 问题时，可以尝试从不同的客户端 IP 访问，或者临时将 sessionAffinity 改为 None 进行对比。

## 5. 版本差异

- **K8s 1.28**：EndpointSlice 已经是默认的 endpoint 分发机制，kube-proxy 默认消费 EndpointSlice 而非 legacy Endpoints。排查时要注意两者可能不同步。
- **K8s 1.30**：`internalTrafficPolicy` 稳定，拓扑感知路由增强。在多可用区部署中，流量可能被限制在特定拓扑区域，导致区域内容量不足时 Service 不可达。
- **K8s 1.32**：kube-proxy 的 nftables 后端 GA。使用 nftables 时，传统的 `iptables-save` 看不到规则，需要使用 `nft list ruleset`。同时引入了 `spec.trafficDistribution` 字段，用于更细粒度的流量分发控制。

## 6. 常见错误与禁忌操作

### 6.1 常见误诊

- 把 DNS 失败当成 Service 失败：先确认 `nslookup` 能返回 ClusterIP。
- 把 `externalTrafficPolicy=Local` 的节点差异当成随机故障：这是预期行为。
- 看到 Endpoints 为空就重启 kube-controller-manager：应先检查 selector 和 Pod readiness。
- 忽略 Pod 安全组（Terway）：即使集群内部通，云安全组也可能阻断外部流量。

### 6.2 禁忌操作

- **不要随意修改 Service 的 `clusterIP`**：`clusterIP` 通常由系统分配，手动指定可能引发冲突。
- **不要在未确认影响面的情况下删除 kube-proxy Pod**：这会导致节点上 Service 规则短暂中断。
- **不要忽视 LB 健康检查路径**：七层 LB 的健康检查路径与应用 readiness 路径不一致时，会导致 LB 反复移除后端。
- **不要在没有 VPC 路由知识的情况下手动修改 Terway 相关安全组**：可能误伤所有 Pod 网络。

## 7. 推荐的诊断顺序

1. 确认 Service 类型（ClusterIP/NodePort/LoadBalancer/Headless/ExternalName）。
2. 检查 Endpoints 和 EndpointSlice 是否为空。
3. 如果 Endpoints 为空，检查 selector 和 Pod readiness。
4. 如果 Endpoints 有，对比直接访问 Pod IP 与访问 ClusterIP。
5. 检查 DNS 解析是否正常。
6. 检查 kube-proxy 状态、日志和节点规则。
7. 如果是 LoadBalancer，检查云厂商控制器日志和云控制台 LB 状态。
8. 在 Terway 环境下，额外检查交换机 IP、ENI 配额和 Pod 安全组。

## 8. 关键诊断命令示例（含「为什么」）

以下命令用于快速区分 Service 不可达的三个层次：Endpoints 层、Service 数据平面层、外部 LB 层。

**检查 Endpoints 是否为空**。Endpoints 是 Service 与 Pod 之间的桥梁，先确认桥是否存在，再排查桥后的路径：
```bash
kubectl get endpoints <svc> -n <ns>
kubectl get endpointslice -n <ns> -l kubernetes.io/service-name=<svc>
```

**对比 Pod IP 与 ClusterIP 访问**。如果 Pod IP 通但 ClusterIP 不通，问题在 kube-proxy/端口映射/NetworkPolicy；如果 Pod IP 也不通，问题在后端 Pod：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec <test-pod> -- curl -s -o /dev/null -w "pod-ip: %{http_code}\n" --connect-timeout 5 http://<pod-ip>:<port>/
kubectl exec <test-pod> -- curl -s -o /dev/null -w "cluster-ip: %{http_code}\n" --connect-timeout 5 http://<cluster-ip>:<port>/
```

**检查 kube-proxy 是否同步规则**。kube-proxy Pod Running 不代表规则已同步，需要直接查看规则或日志：
```bash
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50 | grep -iE "error|failed|sync"
```

**检查云厂商控制器日志**。对于 LoadBalancer Service，这是判断 SLB/NLB/ALB 为什么没有正确创建或同步后端的直接证据：
```bash
kubectl logs -n kube-system -l component=cloud-controller-manager --tail=100
```

## 9. 相关链接

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/service-fta.md|Service 异常 FTA 树]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/05-service-connectivity.md|Service 连通性与 Endpoint 异常诊断与修复 Skill]]
- Terway 网络专题
- [[domain-19-landscape-references/topic-index/network-index.md|Network 网络知识图谱索引]]
