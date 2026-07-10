---
title: Service 连通性与 Endpoint 异常诊断与修复 / Service Connectivity & Endpoint Diagnosis
description: '## 1. 概述'
summary: 'Service 是 [[Kubernetes|Kubernetes]] 中网络连通性的**核心抽象层**。它为一组功能相同的 Pod 提供稳定的虚拟 IP（ClusterIP）和 DNS 名称，解耦了服务消费者与服务提供者。当 Service 连通性出现问题时，表现为集群内部或外部的客户端无法通过 Service 地址访问后端 Pod，'
category: network
tags:
- k8s
- skills
- sop
- runbook
- apiserver
- controller-manager
- prometheus
- istio
- envoy
- cilium
tier: core
created: '2026-05-23'
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 40min
intent_queries:
- Service 连通性与 Endpoint 异常诊断与修复 / Service Connectivity & Endpoint Diagnosis 是什么
- 如何 Service 连通性与 Endpoint 异常诊断与修复 / Service Connectivity & Endpoint Diagnosis
trigger_keywords:
- Service不通
- Service unreachable
- connection refused
- connection timed out
- no endpoints
- Endpoint异常
- ClusterIP不通
- NodePort不通
- LoadBalancer pending
- 服务不可达
- service discovery failure
- kube-proxy
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
skill_id: SKILL-05_SERVICE_CONNECTIVITY-001
skill_name: Service 连通性与 Endpoint 异常诊断与修复 / Service Connectivity & Endpoint Diagnosis
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




<!-- condition: kubectl get endpoints <service-name> -n <ns> -o jsonpath='{.subsets}' | jq 'length == 0 or . == null' 显示 Endpoint 为空 -->

# [[Service|Service]] 连通性与 Endpoint 异常诊断与修复 / Service Connectivity & Endpoint Diagnosis

---

## 1. 概述

Service 是 [[Kubernetes|Kubernetes]] 中网络连通性的**核心抽象层**。它为一组功能相同的 Pod 提供稳定的虚拟 IP（ClusterIP）和 DNS 名称，解耦了服务消费者与服务提供者。当 Service 连通性出现问题时，表现为集群内部或外部的客户端无法通过 Service 地址访问后端 Pod，直接导致微服务间通信断裂、业务功能不可用。

### 典型触发场景

1. **Endpoints 为空**: Service 的 label selector 与后端 Pod 的 labels 不匹配，或者所有后端 Pod 的 readiness probe 均失败，导致 EndpointSlice 中无任何就绪地址
2. **端口映射错误**: Service 的 `targetPort` 与容器实际监听的 `containerPort` 不一致，流量被转发到未监听的端口，返回 connection refused
3. **kube-proxy 规则缺失/过期**: kube-proxy Pod 异常或 iptables/IPVS/nftables 规则未正确同步，导致 ClusterIP 上的流量无法被正确 DNAT 到后端 Pod
4. **[[NetworkPolicy|NetworkPolicy]] 阻断**: 集群中配置了 NetworkPolicy，显式或隐式地阻断了客户端 Pod 到 Service 后端 Pod 的流量
5. **LoadBalancer External IP 未分配**: 云环境中 LoadBalancer 类型 Service 的 External IP 长时间处于 `<pending>`，云控制器（cloud-controller-manager）无法正常工作

### Service 类型覆盖

本 [[SKILL|Skill]] 覆盖以下所有 Service 类型的连通性问题：

| Service 类型 | 说明 | 典型故障模式 |
|-------------|------|-------------|
| **ClusterIP** | 集群内部虚拟 IP，最常用类型 | Endpoints 为空、端口映射错误、kube-proxy 规则缺失 |
| **NodePort** | 在所有节点上暴露固定端口 | externalTrafficPolicy 导致部分节点不可达、NodePort 范围冲突 |
| **LoadBalancer** | 通过云厂商 LB 暴露服务 | External IP pending、LB 健康检查失败、cloud-controller-manager 异常 |
| **ExternalName** | CNAME 别名，不创建 Endpoints | DNS 解析失败、目标域名不可达 |
| **Headless** (ClusterIP: None) | 不分配 ClusterIP，直接返回 Pod IP | Pod DNS 记录未注册、StatefulSet Pod 未就绪 |

### 前置条件

- **RBAC 权限**: 至少需要对 services、endpoints、endpointslices、pods、events、networkpolicies 的 get/list/watch 权限；修复操作需要 update/patch 权限
- **调试能力**: 需要能够 `kubectl exec` 进入测试 Pod 执行网络诊断（curl, wget, nslookup）
- **kube-proxy 访问**: 深度诊断可能需要查看 kube-proxy 的日志和配置（需 kube-system namespace 访问权限）
- **工具要求**: kubectl (v1.28+), jq（推荐）, 集群内有可用的调试 Pod（如 busybox, nicolaka/netshoot）
- **监控系统**: Prometheus + kube-state-metrics（用于 trigger_metrics 匹配）

> **重要**: 如果问题表现为 DNS 解析失败（即 `nslookup <service-name>` 无法返回 IP），应优先使用 SKILL-NET-001（DNS 诊断）。本 Skill 假设 DNS 解析正常，问题在于解析后的 IP 地址（ClusterIP/PodIP）层面的连通性。

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 访问 Service ClusterIP 时返回 `connection refused` / Connection refused when accessing Service ClusterIP | `kubectl exec <test-pod> -- curl -s -o /dev/null -w "%{http_code}" --connect-timeout 5 http://<cluster-ip>:<port>/` 返回错误；或应用日志中出现 connection refused | 0.90 | 应用主动返回拒绝连接（如端口保护、ACL 限制）；Pod 正在启动但尚未开始监听端口 |
| S2 | 访问 Service 时连接超时 / Connection timeout when accessing Service | `kubectl exec <test-pod> -- curl -s --connect-timeout 10 http://<cluster-ip>:<port>/` 超时无响应；客户端日志出现 `dial tcp <ip>:<port>: i/o timeout` | 0.85 | 后端应用本身响应慢（非 Service 层问题）；客户端设置的超时时间过短 |
| S3 | Service 的 Endpoints 列表为空 / Service Endpoints list is empty | `kubectl get endpoints <service> -n <namespace>` 的 ENDPOINTS 列显示 `<none>` | 0.95 | Service 类型为 ExternalName（本身不创建 Endpoints）；Service 刚创建，Endpoints controller 尚未同步（通常 <5s） |
| S4 | EndpointSlice 显示无就绪地址 / EndpointSlice shows no ready addresses | `kubectl get endpointslices -l kubernetes.io/service-name=<service> -n <namespace> -o yaml` 中所有 endpoints 的 `conditions.ready` 均为 `false` | 0.95 | 所有后端 Pod 正在滚动更新中短暂全部未就绪；Pod 刚创建，readiness probe 初始延迟期内 |
| S5 | NodePort 在任何节点上均无响应 / NodePort not responding on any node | `curl -s --connect-timeout 5 http://<node-ip>:<nodeport>/` 在多个节点上均超时或拒绝连接 | 0.85 | 防火墙/安全组阻止了外部到 NodePort 范围（默认 30000-32767）的访问（非 K8s 内部问题）；`--nodeport-addresses` 配置限制了监听的网段 |
| S6 | LoadBalancer 类型 Service 的 External IP 长时间为 `<pending>` / LoadBalancer External IP stuck in pending | `kubectl get svc <service> -n <namespace>` 的 EXTERNAL-IP 列持续显示 `<pending>` 超过 5 分钟 | 0.90 | 使用 MetalLB 等裸金属 LB 方案时，IP 池耗尽属于容量规划问题而非问题；刚创建的 LB Service 在云端分配 IP 通常需要 1-3 分钟 |
| S7 | Service 从部分 Pod 可达但从其他 Pod 不可达 / Service works from some pods but not others | 同一 Service，从不同 namespace 或不同节点上的 Pod 访问结果不一致 | 0.75 | 客户端 Pod 本身网络异常（应先排查客户端 Pod 网络栈）；NetworkPolicy 按 namespace 精细控制（预期行为而非问题） |
| S8 | Service 连通性间歇性失败 / Intermittent connection failures to service | 对同一 Service 的多次请求中，部分成功部分失败；监控显示错误率在 0-100% 之间波动 | 0.70 | 后端应用本身不稳定（如 OOM 重启周期）；负载过高导致部分请求超时（属于容量问题而非连通性问题） |
| S9 | Ingress controller 到后端 Service 的健康检查失败 / Health check from Ingress controller to backend fails | Ingress controller 日志显示 upstream health check failure；`kubectl describe ingress <name>` 显示 backend unhealthy | 0.75 | Ingress controller 自身配置错误（如健康检查路径错误）；Ingress controller Pod 本身网络异常 |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "Service 不通，无法访问后端服务"
- "服务 ClusterIP 连接被拒绝"
- "Service 没有 Endpoint，后端不可达"
- "NodePort 端口访问超时"
- "LoadBalancer 的外部 IP 一直是 pending 状态"
- "部分 Pod 访问某个 Service 正常，部分不行"
- "kube-proxy 好像有问题，Service 转发异常"
- "微服务间调用失败，connection refused"
- "Ingress 后端健康检查报错，upstream 不可达"
- "访问 xxx 服务超时，但 Pod 本身是正常的"

**English ticket descriptions**:
- "Service is unreachable, connection refused to ClusterIP"
- "No endpoints available for service"
- "NodePort not responding from outside the cluster"
- "LoadBalancer external IP stuck in pending"
- "Intermittent service connectivity failures between microservices"
- "kube-proxy rules seem stale, service forwarding broken"
- "Service works from one namespace but not another"
- "Backend health check failing from ingress controller"
- "Service discovery seems broken, connection timed out"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| DNS 解析 Service 名称失败（`nslookup <svc>.<ns>.svc.cluster.local` 无法返回 ClusterIP） | SKILL-NET-001 | DNS 层面问题，需排查 CoreDNS 配置和状态 |
| Pod 本身处于 CrashLoopBackOff 导致所有后端不可用 | SKILL-POD-001 | Pod 应用层问题，需优先排查 Pod 崩溃原因 |
| 节点 NotReady 导致 Pod 无法正常运行，间接影响 Service | SKILL-NODE-001 | 节点级问题，需优先恢复节点状态 |
| Ingress 规则配置错误（path、host 匹配问题），Service 本身可达 | Ingress 配置问题 | 非 Service 层面问题，Service 直接访问正常 |
| Service Mesh（Istio/Linkerd）sidecar 导致的连通性问题 | Service Mesh 诊断 | 超出本 Skill 范围，需排查 sidecar proxy 配置 |
| 集群外部客户端无法访问 ClusterIP（ClusterIP 设计上仅集群内可达） | 预期行为 | 非问题，需使用 NodePort/LoadBalancer/Ingress 暴露服务 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径：

**Step T1**: 确认 Service 类型和基本信息
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get svc <service> -n <namespace> -o wide
```
> **判断规则**:
> - TYPE 为 ClusterIP → 仅影响集群内部通信
> - TYPE 为 NodePort → 可能影响外部流量入口
> - TYPE 为 LoadBalancer → 可能影响生产外部流量入口
> - EXTERNAL-IP 为 `<pending>` → LoadBalancer 未就绪（RC-009）
> - SELECTOR 列为空 → ExternalName 或手动管理 Endpoints 的 Service

**Step T2**: 检查 Endpoint 数量
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get endpoints <service> -n <namespace>
```
> **判断规则**:
> - ENDPOINTS 列显示 IP 地址列表 → Endpoints 存在，问题可能在 kube-proxy 或网络层
> - ENDPOINTS 列显示 `<none>` → Endpoints 为空，核心问题（RC-001 或 RC-002）
> - 显示的 IP 数量远少于预期 Pod 数量 → 部分后端 Pod 未就绪

**Step T3**: 确认后端 Pod 状态
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n <namespace> -l <selector-from-svc> -o wide
```
> **判断规则**:
> - 无 Pod 匹配 → label selector 不匹配（RC-001）
> - Pod 存在但 READY 列非全部就绪（如 0/1）→ readiness probe 失败（RC-002）
> - Pod 全部 Ready → Endpoints 应存在，问题在其他层面

**Step T4**: 评估爆炸半径
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查是否有多个 Service 出现同样问题（排查系统级问题）
kubectl get endpoints --all-namespaces | grep '<none>'
```
> **判断规则**:
> - 仅单个 Service 受影响 → 问题局限于该 Service 的配置或后端 Pod
> - 多个 Service 同时无 Endpoints → 可能是 kube-controller-manager 的 endpoint-controller 异常（系统级问题）
> - 跨多个 namespace 的多个 Service 均不通 → 可能是 kube-proxy 全局问题（P0）

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| 核心基础设施 Service（CoreDNS、Ingress controller、API gateway）不可达 | **P0** | 集群基础服务不可用，影响所有依赖方。CoreDNS 不可达将导致全集群 DNS 解析失败，级联影响极大 | 立即响应，15min 内确认根因 |
| 生产环境面向客户的 Service 不可达 | **P1** | 直接影响终端用户体验和业务收入 | 15min 内响应，30min 内修复 |
| 内部 Service 不可达但有冗余/降级方案 | **P2** | 服务降级但不完全中断，通过重试或备用路径可部分缓解 | 30min 内响应，2h 内修复 |
| 非关键 Service 或仅部分连通性受影响 | **P3** | 非关键服务或影响面有限，不直接影响核心业务流程 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **全集群 Service 不通**: 所有 namespace 中的 Service 均不可达，疑似 kube-proxy 全局问题或控制平面异常
- **CoreDNS Service 不可达**: `kube-dns` / `coredns` Service 无法响应，将导致全集群 DNS 解析中断
- **Ingress controller Service 不可达**: 所有外部流量入口中断
- **kube-controller-manager 异常**: endpoint-controller 无法正常工作，所有新建 Service 均无法自动创建 Endpoints
- **疑似安全事件**: NetworkPolicy 被意外修改或删除，大量 Service 连通性同时发生变化

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集 Service、Endpoints、Pod 的状态信息，定位问题所在层面。所有命令均为只读操作。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 获取 Service 完整信息
- **命令**:
  ```bash
  kubectl get svc <service> -n <namespace> -o wide
  ```
- **超时**: 10s
- **预期输出模式**: 表格输出包含 NAME, TYPE, CLUSTER-IP, EXTERNAL-IP, PORT(S), AGE, SELECTOR
- **判断规则**:
  - TYPE 为 `ClusterIP` → 记录 CLUSTER-IP 和 PORT(S)，用于后续连通性测试
  - TYPE 为 `NodePort` → 记录 NodePort 映射（格式 `<port>:<nodeport>/<protocol>`）
  - TYPE 为 `LoadBalancer` 且 EXTERNAL-IP 为 `<pending>` → 直接标记 RC-009，跳转 D2.9 深度检查
  - TYPE 为 `ExternalName` → 不涉及 Endpoints，问题应在 DNS 层面，考虑转到 SKILL-NET-001
  - SELECTOR 列为空 → 可能是无 selector 的 Service（手动管理 Endpoints），继续 D1.2 确认
  - 命令超时 → apiserver 可能不可达，立即升级（参见 3.3）
- **版本差异**: 无

**Step D1.2**: 检查 Endpoints 和 EndpointSlice
- **命令**:
  ```bash
  # 检查 legacy Endpoints
  kubectl get endpoints <service> -n <namespace>

  # 检查 EndpointSlice（v1.28+ 默认）
  kubectl get endpointslices -l kubernetes.io/service-name=<service> -n <namespace>
  ```
- **超时**: 10s
- **预期输出模式**: Endpoints 资源显示后端 Pod IP 和端口列表
- **判断规则**:
  - Endpoints 显示 IP 地址列表 → Endpoints 存在，问题不在 selector/readiness，继续 D1.4
  - Endpoints 显示 `<none>` → Endpoints 为空，核心问题。继续 D1.3 排查原因
  - EndpointSlice 存在但 Endpoints 为空 → 可能是 endpoints-controller 与 endpointslice-controller 不一致，关注 RC-004（kube-proxy 可能使用 EndpointSlice 而非 legacy Endpoints）
  - 无 EndpointSlice 资源 → endpoint-slice-controller 异常，检查 kube-controller-manager 日志
- **版本差异**:
  - **[v1.28+]**: EndpointSlice 为默认的 endpoint 分发机制，kube-proxy 默认消费 EndpointSlice
  - **[v1.33+]**: legacy Endpoints 可能开始被弃用，优先使用 EndpointSlice 检查

**Step D1.3**: 验证 Service selector 与 Pod labels 匹配
- **命令**:
  ```bash
  # 获取 Service 的 selector
  kubectl get svc <service> -n <namespace> -o jsonpath='{.spec.selector}' | jq .

  # 使用 Service 的 selector 查找匹配的 Pod
  kubectl get pods -n <namespace> -l <key1>=<value1>,<key2>=<value2> -o wide
  ```
- **超时**: 10s
- **预期输出模式**: Pod 列表，应与预期的后端 Pod 一致
- **判断规则**:
  - 无 Pod 匹配 selector → **RC-001（label selector 不匹配）**。可能是 Deployment labels 与 Service selector 不一致，或 Pod template labels 缺失/错误
  - Pod 匹配但 READY 列显示 `0/1` 或 `0/N` → 所有 Pod readiness 失败，继续 D1.4 → **RC-002**
  - Pod 匹配且 READY 为 `1/1` → Pod 就绪但 Endpoints 为空，异常情况。检查 endpoint-controller（kube-controller-manager）
  - Pod 存在但带有 `deletionTimestamp`（正在 Terminating）→ 所有 Pod 正在被删除，Endpoints 将被清空
- **版本差异**: 无

**Step D1.4**: 检查 Pod readiness 详情
- **命令**:
  ```bash
  # 逐 Pod 检查就绪状态和 IP
  kubectl get pods -n <namespace> -l <selector> \
    -o custom-columns=NAME:.metadata.name,READY:.status.containerStatuses[*].ready,IP:.status.podIP,NODE:.spec.nodeName,PHASE:.status.phase
  ```
- **超时**: 10s
- **预期输出模式**: Pod 名称、就绪状态、IP、所在节点
- **判断规则**:
  - 所有 Pod 的 READY 均为 `false` → 全部 readiness probe 失败（RC-002），继续 D2.3 深入检查
  - 部分 Pod READY 为 `true`，部分为 `false` → 部分 Pod 不就绪，Endpoints 中仅包含就绪的 Pod。如果就绪 Pod 数量为 0 → 同 RC-002
  - 所有 Pod READY 为 `true` 但 Endpoints 为空 → endpoint-controller 异常，检查 kube-controller-manager
  - Pod 无 IP（`<none>`）→ Pod 网络未分配，CNI 问题（关联 SKILL-NODE-001）
- **版本差异**: 无

**Step D1.5**: 检查 Service 相关事件
- **命令**:
  ```bash
  kubectl get events -n <namespace> --field-selector involvedObject.name=<service> --sort-by=.lastTimestamp --no-headers | tail -20
  ```
- **超时**: 10s
- **预期输出模式**: Service 相关事件列表
- **判断规则**:
  - 出现 `FailedToUpdateEndpoint` → endpoint-controller 更新 Endpoints 失败
  - 出现 `FailedToUpdateEndpointSlices` → endpointslice-controller 更新失败
  - 出现 `EnsuringLoadBalancer` 持续出现且无 `EnsuredLoadBalancer` → LoadBalancer 创建/更新失败（RC-009）
  - 出现 `CreatingLoadBalancerFailed` → 云端 LB 创建失败，检查 cloud-controller-manager 日志
  - 无事件 → Service 配置可能一直不正确（如创建时就 selector 不匹配）
- **版本差异**: 无

---

### Decision Branch（基于 Phase 1 结果的决策分支）

根据 D1.1-D1.5 的结果，按以下决策树进入不同的深度检查路径：

```
D1.2: Endpoints 是否为空？
├── YES → D1.3: 是否有匹配 selector 的 Pod？
│   ├── NO → *** RC-001: Label Selector 不匹配 *** → 跳转 REM-001
│   └── YES → D1.4: Pod 是否全部就绪？
│       ├── NO → *** RC-002: Readiness Probe 失败 *** → 进入 D2.3
│       └── YES → endpoint-controller 异常 → 检查 kube-controller-manager
├── NO → Service 有 Endpoints
│   ├── D1.1: 访问 ClusterIP 返回 connection refused？
│   │   └── YES → *** 疑似 RC-003: 端口映射不匹配 *** → 进入 D2.2
│   ├── D1.1: 访问 ClusterIP 超时？
│   │   └── YES → *** 疑似 RC-004/RC-005: kube-proxy 或 NetworkPolicy *** → 进入 D2.5/D2.6
│   └── D1.1: LoadBalancer EXTERNAL-IP 为 pending？
│       └── YES → *** RC-009: 云厂商 LB 控制器异常 *** → 进入 D2.9
```

---

### Phase 2: 深度检查（只读，零风险）

> **目标**: 针对 Phase 1 定位的问题方向，进行深度验证和根因确认。所有命令均为只读操作。
> **预计耗时**: 5-15 分钟

**Step D2.1**: Label selector 详细验证
- **命令**:
  ```bash
  # 获取 Service 的完整 selector
  kubectl get svc <service> -n <namespace> -o jsonpath='{.spec.selector}'
  # 示例输出: {"app":"myapp","version":"v1"}

  # 获取疑似目标 Pod 的实际 labels
  kubectl get pods -n <namespace> --show-labels

  # 精确比对：逐 label 验证
  kubectl get pods -n <namespace> -l app=myapp --show-labels
  kubectl get pods -n <namespace> -l app=myapp,version=v1 --show-labels
  ```
- **超时**: 10s
- **预期输出模式**: label 键值对
- **判断规则**:
  - Service selector 的某个 key 在 Pod labels 中不存在 → label key 缺失（RC-001）
  - Service selector 的 key 存在但 value 不匹配（如 selector 为 `app: myapp` 但 Pod label 为 `app: my-app`）→ label value 不匹配（RC-001）
  - 所有 label 完全匹配 → selector 无误，排除 RC-001
  - 常见陷阱：Deployment 的 `spec.selector.matchLabels` 与 `spec.template.metadata.labels` 不一致
- **版本差异**: 无

**Step D2.2**: 端口映射详细验证
- **命令**:
  ```bash
  # 获取 Service 端口定义
  kubectl get svc <service> -n <namespace> -o jsonpath='{range .spec.ports[*]}{"port:"}{.port}{" targetPort:"}{.targetPort}{" protocol:"}{.protocol}{"\n"}{end}'

  # 获取后端 Pod 容器端口定义
  kubectl get pods -n <namespace> -l <selector> -o jsonpath='{range .items[*]}{.metadata.name}{": "}{range .spec.containers[*]}{.name}{"="}{range .ports[*]}{.containerPort}{"/"}{.protocol}{" "}{end}{end}{"\n"}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: Service 端口映射 vs 容器端口
- **判断规则**:
  - Service `targetPort` 为数字且与容器 `containerPort` 不一致 → **RC-003**
  - Service `targetPort` 为命名端口（如 `http`）但容器中无同名端口定义 → **RC-003**
  - Service `protocol` 为 TCP 但容器监听 UDP（或反之）→ **RC-010**
  - 端口映射完全匹配 → 排除 RC-003
  - Service 有多个端口定义，其中某个端口映射错误 → 部分端口工作部分不工作
- **版本差异**: 无

**Step D2.3**: Pod readiness probe 深度检查
- **命令**:
  ```bash
  # 查看 Pod 详情，关注 readiness probe 配置和失败信息
  kubectl describe pod <pod> -n <namespace> | grep -A 15 "Readiness"

  # 查看 Pod 事件中的 readiness 失败记录
  kubectl get events -n <namespace> --field-selector involvedObject.name=<pod> --sort-by=.lastTimestamp | grep -i "unhealthy|probe"

  # 查看 Pod 最近日志（排查应用启动失败）
  kubectl logs <pod> -n <namespace> --tail=50
  ```
- **超时**: 15s
- **预期输出模式**: readiness probe 配置、失败事件、应用日志
- **判断规则**:
  - readiness probe 配置的 `path` 或 `port` 与应用实际不匹配 → readiness 探针配置错误（RC-002 变种）
  - 应用日志显示启动失败或依赖未就绪 → 应用层问题导致 readiness 失败（RC-002）
  - readiness probe 成功间隔内 Pod 频繁在 ready/not-ready 之间切换 → 应用健康状态不稳定
  - 日志无错误但 probe 仍失败 → 可能 probe 超时设置过短（`timeoutSeconds` 默认 1s）
- **版本差异**: 无

**Step D2.4**: 直接 Pod 连通性测试（绕过 Service）
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 从测试 Pod 直接访问后端 Pod IP（绕过 Service ClusterIP）
  kubectl exec <test-pod> -n <test-namespace> -- curl -s -o /dev/null -w "%{http_code}" --connect-timeout 5 http://<pod-ip>:<container-port>/

  # 如果无 curl，使用 wget
  kubectl exec <test-pod> -n <test-namespace> -- wget -q -O /dev/null --timeout=5 http://<pod-ip>:<container-port>/

  # 测试 TCP 连通性
  kubectl exec <test-pod> -n <test-namespace> -- nc -zv <pod-ip> <container-port> -w 5
  ```
- **超时**: 15s
- **预期输出模式**: HTTP 状态码或连接成功/失败
- **判断规则**:
  - 直接访问 Pod IP 成功，但通过 ClusterIP 失败 → 问题在 Service 层（kube-proxy/iptables/IPVS），继续 D2.5
  - 直接访问 Pod IP 也失败（connection refused）→ 应用未监听该端口（RC-003）或 Pod 网络异常
  - 直接访问 Pod IP 超时 → 跨节点网络问题（RC-012）或 NetworkPolicy 阻断（RC-005）
  - 直接访问成功且 ClusterIP 也成功 → 问题可能已恢复或是间歇性的
- **版本差异**: 无

**Step D2.5**: kube-proxy 状态和规则检查
- **命令**:
  ```bash
  # 检查 kube-proxy Pod 状态
  kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide

  # 检查 kube-proxy 配置（获取 proxy mode）
  kubectl get configmap kube-proxy -n kube-system -o jsonpath='{.data.config\.conf}' | grep mode

  # 检查 kube-proxy 日志（最近 10 分钟）
  kubectl logs -n kube-system -l k8s-app=kube-proxy --since=10m --tail=50

  # 在节点上检查 iptables 规则（iptables 模式）
  # 需要 SSH 或 kubectl debug node
  # iptables-save | grep <service-cluster-ip>

  # 在节点上检查 IPVS 规则（IPVS 模式）
  # ipvsadm -ln | grep <service-cluster-ip>

  # 在节点上检查 nftables 规则（nftables 模式, v1.29+）
  # nft list ruleset | grep <service-cluster-ip>
  ```
- **超时**: 15s
- **预期输出模式**: kube-proxy Pod 状态、运行模式、日志输出
- **判断规则**:
  - kube-proxy Pod 处于 CrashLoopBackOff → **RC-004**，kube-proxy 异常
  - kube-proxy Pod 全部 Running 但日志中有 `error syncing rules` → 规则同步失败（RC-004）
  - kube-proxy 日志包含 `couldn't get current iptables rules` → iptables 二进制文件问题
  - 节点上找不到 Service ClusterIP 对应的 iptables/IPVS/nftables 规则 → 规则缺失（RC-004）
  - kube-proxy 模式为 `iptables` 但期望 `ipvs` 或反之 → 配置不一致
  - kube-proxy 日志正常且规则存在 → 排除 RC-004
- **版本差异**:
  - **[v1.28]**: kube-proxy 支持 iptables 和 IPVS 模式
  - **[v1.29+]**: nftables 模式作为 alpha 可用
  - **[v1.31+]**: nftables 模式升级为 beta
  - **[v1.32+]**: nftables 模式 GA。使用 nftables 时，`iptables-save` 不会显示 kube-proxy 规则，需用 `nft list ruleset`

**Step D2.6**: NetworkPolicy 分析
- **命令**:
  ```bash
  # 获取目标 namespace 中的所有 NetworkPolicy
  kubectl get networkpolicy -n <namespace>

  # 详细查看每个 NetworkPolicy 的规则
  kubectl describe networkpolicy -n <namespace>

  # 检查客户端 Pod 所在 namespace 的 egress 策略
  kubectl get networkpolicy -n <client-namespace>
  ```
- **超时**: 10s
- **预期输出模式**: NetworkPolicy 列表和详细规则
- **判断规则**:
  - 存在 default-deny ingress policy 且无允许来自客户端的 ingress 规则 → **RC-005**
  - 存在 default-deny egress policy 在客户端 namespace 且无允许到目标 Service 的 egress 规则 → **RC-005**
  - NetworkPolicy 的 `podSelector` 匹配了 Service 后端 Pod，但 `ingress.from` 未包含客户端 Pod 的 labels/namespace → **RC-005**
  - 无 NetworkPolicy → 排除 RC-005
  - NetworkPolicy 的 `ports` 字段未包含 Service 使用的端口 → **RC-005**（端口级别阻断）
- **版本差异**:
  - **[v1.28+]**: NetworkPolicy 核心 API 稳定（v1），所有版本行为一致
  - **[v1.30+]**: AdminNetworkPolicy 和 BaselineAdminNetworkPolicy (beta) 引入了集群级别的网络策略，需额外检查

**Step D2.7**: conntrack 表检查（针对间歇性问题）
- **命令**:
  ```bash
  # 在节点上检查 conntrack 表中与 Service ClusterIP 相关的条目
  # 需要 SSH 到节点或使用 kubectl debug node
  kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- conntrack -L -d <service-cluster-ip> 2>/dev/null | head -20

  # 检查 conntrack 表大小
  kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- sh -c 'cat /proc/sys/net/netfilter/nf_conntrack_count && echo "/" && cat /proc/sys/net/netfilter/nf_conntrack_max'
  ```
- **超时**: 15s
- **预期输出模式**: conntrack 条目列表
- **判断规则**:
  - 存在大量 `UNREPLIED` 或 `TIME_WAIT` 状态的 conntrack 条目 → 可能有 conntrack 竞争条件（RC-006）
  - conntrack 表接近满（count/max > 80%）→ conntrack 表溢出，新连接被丢弃（RC-006）
  - conntrack 条目指向已不存在的 Pod IP → 陈旧条目（RC-006）
  - conntrack 表正常 → 排除 RC-006
- **版本差异**:
  - **[v1.32+]**: nftables 模式使用内核内置的连接跟踪，行为与 iptables 模式略有不同

**Step D2.8**: externalTrafficPolicy 和 topology 检查
- **命令**:
  ```bash
  # 检查 externalTrafficPolicy 设置
  kubectl get svc <service> -n <namespace> -o jsonpath='{.spec.externalTrafficPolicy}'

  # 检查 internalTrafficPolicy 设置
  kubectl get svc <service> -n <namespace> -o jsonpath='{.spec.internalTrafficPolicy}'

  # 如果 externalTrafficPolicy=Local，检查哪些节点有后端 Pod
  kubectl get pods -n <namespace> -l <selector> -o custom-columns=NAME:.metadata.name,NODE:.spec.nodeName,READY:.status.containerStatuses[*].ready

  # 检查 topology aware routing 注解
  kubectl get svc <service> -n <namespace> -o jsonpath='{.metadata.annotations.service\.kubernetes\.io/topology-mode}'
  ```
- **超时**: 10s
- **预期输出模式**: externalTrafficPolicy 值、Pod 分布
- **判断规则**:
  - `externalTrafficPolicy=Local` 且通过 NodePort 访问的节点上没有就绪的后端 Pod → **RC-007**
  - `internalTrafficPolicy=Local` 且客户端 Pod 所在节点上无后端 Pod → internalTrafficPolicy 导致的连通性问题（RC-007 变种）
  - `topology-mode: Auto` 可能导致流量不均或在特定拓扑区域内无后端时连接失败
  - 无特殊 traffic policy 设置 → 排除 RC-007
- **版本差异**:
  - **[v1.28+]**: `internalTrafficPolicy` 稳定
  - **[v1.30+]**: Topology aware routing 改进，引入 `service.kubernetes.io/topology-mode` 注解
  - **[v1.31+]**: Traffic distribution for Services (beta)，新增 `spec.trafficDistribution` 字段

**Step D2.9**: LoadBalancer 深度检查（仅 LB 类型 Service）
- **命令**:
  ```bash
  # 检查 Service 完整 YAML，关注 status.loadBalancer
  kubectl get svc <service> -n <namespace> -o yaml | grep -A 20 "status:"

  # 检查 cloud-controller-manager 日志
  kubectl logs -n kube-system -l component=cloud-controller-manager --tail=50

  # 检查 Service 的 annotations（云厂商特定配置）
  kubectl get svc <service> -n <namespace> -o jsonpath='{.metadata.annotations}' | jq .

  # 检查是否有 LB 相关事件
  kubectl get events -n <namespace> --field-selector involvedObject.name=<service> --sort-by=.lastTimestamp
  ```
- **超时**: 15s
- **预期输出模式**: LB 状态、cloud-controller-manager 日志
- **判断规则**:
  - `status.loadBalancer.ingress` 为空 → LB 尚未创建（RC-009）
  - cloud-controller-manager 日志包含 `failed to ensure load balancer` → 云端 API 调用失败（RC-009）
  - 日志包含权限相关错误（`AccessDenied`、`Forbidden`）→ 云端 IAM 权限不足（RC-009）
  - 日志包含配额相关错误 → 云端 LB 配额耗尽（RC-009）
  - Service annotations 中有错误的子网、安全组配置 → 配置错误（RC-009 变种）
- **版本差异**: 无（取决于云厂商实现）

**Step D2.10**: sessionAffinity 和高级配置检查
- **命令**:
  ```bash
  # 检查 sessionAffinity 设置
  kubectl get svc <service> -n <namespace> -o jsonpath='sessionAffinity={.spec.sessionAffinity} timeoutSeconds={.spec.sessionAffinityConfig.clientIP.timeoutSeconds}'

  # 检查 publishNotReadyAddresses
  kubectl get svc <service> -n <namespace> -o jsonpath='{.spec.publishNotReadyAddresses}'

  # 检查 Service 的 ipFamilies 和 ipFamilyPolicy（双栈场景）
  kubectl get svc <service> -n <namespace> -o jsonpath='ipFamilies={.spec.ipFamilies} ipFamilyPolicy={.spec.ipFamilyPolicy}'
  ```
- **超时**: 10s
- **预期输出模式**: sessionAffinity 配置值
- **判断规则**:
  - `sessionAffinity=ClientIP` 且 `timeoutSeconds` 过长 → 可能导致流量长期粘滞到特定 Pod，当该 Pod 异常时影响体验（RC-011）
  - `publishNotReadyAddresses=true` → Service 会包含未就绪 Pod 的地址，可能导致请求被发送到未就绪的 Pod
  - 双栈配置但集群不支持 IPv6 → 可能导致部分连接失败
  - 配置正常 → 排除 RC-011
- **版本差异**:
  - **[v1.31+]**: `spec.trafficDistribution` 字段（beta），新的流量分发策略可能影响连通性行为

---

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: 完整连通性矩阵测试
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 从多个不同节点上的 Pod 测试 Service 连通性
  # Pod A (节点1) → Service
  kubectl exec <pod-a> -n <ns-a> -- curl -s -o /dev/null -w "from pod-a: %{http_code}\n" --connect-timeout 5 http://<service>.<namespace>.svc.cluster.local:<port>/

  # Pod B (节点2) → Service
  kubectl exec <pod-b> -n <ns-b> -- curl -s -o /dev/null -w "from pod-b: %{http_code}\n" --connect-timeout 5 http://<service>.<namespace>.svc.cluster.local:<port>/

  # 同 namespace 内 Pod → Service
  kubectl exec <same-ns-pod> -n <namespace> -- curl -s -o /dev/null -w "same-ns: %{http_code}\n" --connect-timeout 5 http://<service>:<port>/
  ```
- **超时**: 30s
- **风险级别**: 🟢 低（只读 HTTP GET 请求）
- **预期输出模式**: 各 Pod 访问 Service 的 HTTP 状态码
- **判断规则**:
  - 所有 Pod 均失败 → 全局 Service 连通性问题（kube-proxy 或 Endpoints 问题）
  - 部分 Pod 成功，部分失败 → 节点级别问题（特定节点的 kube-proxy 异常）或 NetworkPolicy（RC-005）
  - 同 namespace 成功，跨 namespace 失败 → NetworkPolicy 按 namespace 限制（RC-005）
  - 全部成功 → 问题可能是间歇性的或已恢复
- **版本差异**: 无

**Step D3.2**: 协议级测试
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # TCP 连通性测试
  kubectl exec <test-pod> -- nc -zv <service-cluster-ip> <port> -w 5

  # UDP 连通性测试（如果 Service 使用 UDP）
  kubectl exec <test-pod> -- nc -zuv <service-cluster-ip> <udp-port> -w 5

  # HTTP 完整请求测试（包含响应头和响应时间）
  kubectl exec <test-pod> -- curl -sv --connect-timeout 5 --max-time 10 http://<service-cluster-ip>:<port>/
  ```
- **超时**: 20s
- **风险级别**: 🟢 低（网络测试请求）
- **预期输出模式**: 连接成功/失败信息、HTTP 响应详情
- **判断规则**:
  - TCP 连接成功但 HTTP 请求返回错误 → 应用层问题（非 Service 层）
  - TCP 连接被拒绝 → 端口未监听（RC-003）或 kube-proxy 规则指向错误后端
  - UDP 测试无响应 → 需确认应用确实监听 UDP；UDP 的 Service 诊断更复杂
- **版本差异**: 无

**Step D3.3**: 负载测试确认容量问题
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 快速并发测试（10 个并发请求）
  kubectl exec <test-pod> -- sh -c 'for i in $(seq 1 10); do curl -s -o /dev/null -w "%{http_code} %{time_total}s\n" --connect-timeout 5 http://<service-cluster-ip>:<port>/ & done; wait'
  ```
- **超时**: 30s
- **风险级别**: 🟡 中（可能对后端服务产生压力）
- **预期输出模式**: 10 个请求的 HTTP 状态码和响应时间
- **判断规则**:
  - 全部成功 → 非容量问题
  - 部分成功部分超时 → 后端容量不足或负载不均
  - 全部超时 → 确认 Service 层连通性问题
- **版本差异**: 无

---

### Phase 4: Service Mesh 场景诊断

> **目标**: 排查 Service Mesh（Istio/Linkerd）的 sidecar proxy 相关的连通性问题。要求集群已部署 Service Mesh。
> **预计耗时**: 5-10 分钟
> **前置条件**: 已确认 Service 后端 Pod 运行 Service Mesh sidecar（Istio-proxy / Linkerd-proxy）

**Step D4.1**: 检查 Istio sidecar 注入状态
- **命令**:
  ```bash
  # 检查 Pod 是否包含 istio-proxy 容器
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].name}' | grep -o istio-proxy
  
  # 查看 namespace 中所有 Pod 的 sidecar 注入状态
  kubectl get pods -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.containers[*].name}{"\n"}{end}' | grep istio-proxy
  
  # 检查 namespace 的 sidecar 注入标签
  kubectl get namespace <namespace> -o jsonpath='{.metadata.labels.istio-injection}'
  ```
- **超时**: 10s
- **预期输出模式**: Pod 容器列表包含 `istio-proxy`
- **判断规则**:
  - 包含 `istio-proxy` → sidecar 已注入，继续检查 sidecar 状态
  - 不包含 `istio-proxy` → sidecar 未注入，检查 namespace 标签和 Pod annotation
  - namespace 标签 `istio-injection=enabled` 但 Pod 无 sidecar → 注入失败，检查 mutating webhook
  - sidecar 容器状态不是 Running → sidecar 启动失败，检查日志
- **版本差异**: 无（取决于 Istio 版本）

**Step D4.2**: 检查 Istio VirtualService/DestinationRule 路由规则
- **命令**:
  ```bash
  # 获取影响目标 Service 的 VirtualService
  kubectl get virtualservice -n <namespace> -o yaml | grep -A 50 "host: <service-name>"
  
  # 获取 DestinationRule 配置
  kubectl get destinationrule -n <namespace> -o yaml | grep -A 30 "host: <service-name>"
  
  # 检查是否有全局的 VirtualService/DestinationRule
  kubectl get virtualservice -A | grep <service-name>
  kubectl get destinationrule -A | grep <service-name>
  ```
- **超时**: 10s
- **预期输出模式**: VirtualService 和 DestinationRule 的路由规则
- **判断规则**:
  - VirtualService 的 route.destination.host 与 Service 名称不匹配 → 路由配置错误
  - DestinationRule 的 trafficPolicy 配置了不存在的 subset → 路由失败
  - VirtualService match 条件过于严格 → 请求可能被过滤
  - 无 VirtualService/DestinationRule → 使用默认路由，非配置问题
- **版本差异**: 无

**Step D4.3**: 检查 mTLS 模式
- **命令**:
  ```bash
  # 使用 istioctl 检查 mTLS 状态
  istioctl authn tls-check <pod-name>.<namespace> <target-service>.<target-namespace>.svc.cluster.local
  
  # 检查 PeerAuthentication 策略
  kubectl get peerauthentication -n <namespace> -o yaml
  kubectl get peerauthentication -n istio-system -o yaml
  
  # 检查 DestinationRule 中的 TLS 设置
  kubectl get destinationrule -n <namespace> -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.trafficPolicy.tls.mode}{"\n"}{end}'
  ```
- **超时**: 15s
- **预期输出模式**: mTLS 配置和状态
- **判断规则**:
  - `tls-check` 显示 `mTLS` 且状态正常 → mTLS 配置正确
  - 客户端期望 PERMISSIVE 但服务端配置 STRICT → mTLS 不匹配（RC-011）
  - DestinationRule TLS mode 与 PeerAuthentication 不一致 → TLS 配置冲突
  - 跨 namespace 访问时 mTLS 失败 → 检查是否有跨 namespace 的 PeerAuthentication
- **版本差异**: 无

**Step D4.4**: 检查 Envoy sidecar 配置和日志
- **命令**:
  ```bash
  # 查看 Envoy 的 cluster 配置
  istioctl proxy-config cluster <pod-name> -n <namespace> | grep <target-service>
  
  # 查看 Envoy 的 listener 配置
  istioctl proxy-config listener <pod-name> -n <namespace>
  
  # 查看 Envoy 的 route 配置
  istioctl proxy-config route <pod-name> -n <namespace> | grep <target-service>
  
  # 查看 sidecar 日志
  kubectl logs <pod-name> -n <namespace> -c istio-proxy --tail=100 | grep -i "error|upstream|timeout"
  ```
- **超时**: 15s
- **预期输出模式**: Envoy 配置和日志
- **判断规则**:
  - cluster 中不包含目标 Service → 服务发现问题，检查 Istiod 状态
  - listener 未包含目标端口 → listener 配置缺失
  - route 中 match 规则与实际请求不匹配 → 路由无法生效
  - 日志中出现 `upstream connect error` → 后端不可达
  - 日志中出现 `TLS error` → mTLS 握手失败（RC-011）
- **版本差异**: 无

**Step D4.5**: Linkerd 诊断
- **命令**:
  ```bash
  # 检查 Linkerd 控制平面状态
  linkerd check --proxy
  
  # 查看特定 Pod 的 proxy metrics
  linkerd diagnostics proxy-metrics <pod-name> -n <namespace>
  
  # 检查 ServiceProfile 配置
  kubectl get serviceprofile -n <namespace>
  
  # 查看 proxy 日志
  kubectl logs <pod-name> -n <namespace> -c linkerd-proxy --tail=50
  ```
- **超时**: 15s
- **预期输出模式**: Linkerd 状态和 metrics
- **判断规则**:
  - `linkerd check` 显示错误 → Linkerd 控制平面或 proxy 有问题
  - proxy metrics 显示高失败率 → 后端连接问题
  - identity 证书相关错误 → Linkerd identity 证书过期（RC-011）
- **版本差异**: 无（取决于 Linkerd 版本）

---

### Phase 5: Gateway API 路由排查

> **目标**: 排查 Gateway API 配置相关的 Service 连通性问题。适用于使用 Gateway API 替代或补充 Ingress 的集群。
> **预计耗时**: 3-5 分钟
> **前置条件**: 集群已部署 Gateway API CRDs 和 Gateway Controller

**Step D5.1**: 检查 Gateway 状态
- **命令**:
  ```bash
  # 获取所有 Gateway 资源
  kubectl get gateway -A
  
  # 检查 Gateway 详细状态
  kubectl describe gateway <gateway-name> -n <namespace>
  
  # 检查 Gateway 的 conditions
  kubectl get gateway <gateway-name> -n <namespace> -o jsonpath='{range .status.conditions[*]}{.type}{"="}{.status}{" "}{end}'
  
  # 检查 GatewayClass
  kubectl get gatewayclass
  ```
- **超时**: 10s
- **预期输出模式**: Gateway 资源列表和状态
- **判断规则**:
  - Gateway status.conditions 中 `Accepted=True` 和 `Programmed=True` → Gateway 工作正常
  - `Accepted=False` → GatewayClass 不存在或配置错误
  - `Programmed=False` → Gateway Controller 无法配置数据平面
  - listeners 状态显示 `Ready=False` → listener 配置问题（端口、协议、TLS）
- **版本差异**:
  - **[v1.28+]**: Gateway API v0.7.x beta
  - **[v1.30+]**: Gateway API v1.0 GA
  - **[v1.31+]**: Gateway API v1.1 新增 BackendLBPolicy

**Step D5.2**: 检查 HTTPRoute 绑定状态
- **命令**:
  ```bash
  # 获取 HTTPRoute 完整配置
  kubectl get httproute <route-name> -n <namespace> -o yaml
  
  # 检查 parentRefs 绑定状态
  kubectl get httproute <route-name> -n <namespace> -o jsonpath='{.status.parents[*]}' | jq .
  
  # 检查 HTTPRoute 是否被 Gateway 接受
  kubectl get httproute <route-name> -n <namespace> -o jsonpath='{range .status.parents[*]}{.parentRef.name}{" "}{.conditions[*].type}{"="}{.conditions[*].status}{"\n"}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: HTTPRoute 配置和绑定状态
- **判断规则**:
  - status.parents 中 parentRef 对应的 Gateway `Accepted=True` → 路由已被接受
  - `Accepted=False` + reason `NotAllowedByListeners` → listener 不允许此路由
  - `Accepted=False` + reason `RefNotPermitted` → 缺少 ReferenceGrant
  - `ResolvedRefs=False` → backendRef 指向的 Service 不存在或不可达
- **版本差异**: 无

**Step D5.3**: 检查 ReferenceGrant 跨命名空间权限
- **命令**:
  ```bash
  # 获取所有 ReferenceGrant
  kubectl get referencegrant -A
  
  # 检查特定 namespace 的 ReferenceGrant
  kubectl get referencegrant -n <target-namespace> -o yaml
  
  # 检查是否允许从源 namespace 引用
  kubectl get referencegrant -n <target-namespace> -o jsonpath='{range .items[*]}{.metadata.name}{" from: "}{.spec.from[*].namespace}{" to: "}{.spec.to[*].kind}{"\n"}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: ReferenceGrant 配置
- **判断规则**:
  - HTTPRoute 和 backendRef Service 在同一 namespace → 不需要 ReferenceGrant
  - HTTPRoute 在 ns-A，backendRef Service 在 ns-B，无 ReferenceGrant → 缺少授权（RC-015）
  - ReferenceGrant 存在但 `from.namespace` 未包含 HTTPRoute 的 namespace → 授权不足
  - ReferenceGrant `to.kind` 不包含 `Service` → 授权类型不匹配
- **版本差异**: 无

**Step D5.4**: 验证 BackendRef 目标 Service 可达性
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 获取 HTTPRoute 的 backendRefs
  kubectl get httproute <route-name> -n <namespace> -o jsonpath='{.spec.rules[*].backendRefs[*]}' | jq .
  
  # 检查 backendRef 指向的 Service 是否存在
  kubectl get svc <backend-service> -n <backend-namespace>
  
  # 检查 Service 是否有可用 Endpoints
  kubectl get endpoints <backend-service> -n <backend-namespace>
  
  # 从 Gateway Controller Pod 测试到 backend 的连通性
  kubectl exec <gateway-controller-pod> -n <gateway-ns> -- curl -s --connect-timeout 5 http://<backend-service>.<backend-namespace>.svc.cluster.local:<port>/
  ```
- **超时**: 15s
- **预期输出模式**: Service 和 Endpoints 状态
- **判断规则**:
  - backendRef Service 不存在 → 配置错误，需创建 Service
  - Service 存在但 Endpoints 为空 → 后端 Pod 未就绪（回到 D1.3）
  - Gateway Controller 无法连接 backend → 网络或 NetworkPolicy 问题
  - backendRef.port 与 Service.spec.ports 不匹配 → 端口配置错误
- **版本差异**: 无

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | **Service selector 与 Pod labels 不匹配** — Service 的 `spec.selector` 中定义的 label 键值对与后端 Pod 的 `metadata.labels` 不完全匹配，导致 endpoint-controller 无法将 Pod 注册到 Endpoints 中。常见于 Deployment/Service 独立修改 labels 时、Helm chart 值覆盖后不一致、或 label 拼写错误 | 高 | D1.2 Endpoints 为空；D1.3 使用 selector 查找 Pod 无结果；D2.1 label key/value 不匹配 | service-fta: BE-selector-mismatch |
| RC-002 | **Pod 就绪探针失败导致 Endpoints 为空** — 所有后端 Pod 的 readiness probe 持续失败，endpoint-controller 不将未就绪 Pod 的地址加入 Endpoints（除非 `publishNotReadyAddresses=true`）。根本原因可能是应用启动失败、依赖服务不可用、探针配置错误（错误的 path/port/timeout） | 高 | D1.2 Endpoints 为空；D1.3 Pod 存在但 READY=false；D2.3 readiness probe 失败事件；应用日志显示错误 | service-fta: BE-readiness-failure |
| RC-003 | **Service targetPort 与容器实际监听端口不匹配** — Service `spec.ports[].targetPort` 指向的端口号（或命名端口）与容器 `spec.containers[].ports[].containerPort` 不一致，导致 kube-proxy 将流量 DNAT 到 Pod 的未监听端口，返回 connection refused | 高 | D1.2 Endpoints 存在；D2.2 targetPort 与 containerPort 不匹配；D2.4 直接访问正确端口成功但通过 Service 失败 | service-fta: BE-port-mismatch |
| RC-004 | **kube-proxy 异常导致 iptables/IPVS/nftables 规则缺失** — kube-proxy Pod 崩溃、配置错误或无法连接 apiserver，未能将 Service/Endpoints 信息同步为节点上的数据平面规则，导致节点无法将 Service ClusterIP 的流量正确转发到后端 Pod | 中 | D2.4 直接访问 Pod IP 成功但 ClusterIP 失败；D2.5 kube-proxy Pod 异常或规则缺失；kube-proxy 日志有错误 | service-fta: BE-kube-proxy-failure |
| RC-005 | **NetworkPolicy 阻断了 Service 流量** — 集群中配置了限制性的 NetworkPolicy，显式地拒绝了客户端 Pod 到 Service 后端 Pod 的 ingress 流量，或拒绝了客户端 Pod 的 egress 流量到 Service 端口 | 中 | D2.6 存在 default-deny 策略或限制性策略；D3.1 从不同 namespace 访问结果不一致；去掉 NetworkPolicy 后恢复正常 | service-fta: BE-networkpolicy-block |
| RC-006 | **conntrack 表项过期或冲突** — 节点上 conntrack 表中存在指向已不存在 Pod 的陈旧条目，或 conntrack 表满导致新连接被丢弃。常见于 Pod 频繁重建（如滚动更新期间）且 conntrack 超时未释放的场景 | 低 | D2.7 conntrack 表接近满或存在大量 UNREPLIED 条目；间歇性连接失败与 Pod 重建时间相关；清除 conntrack 后恢复 | service-fta: BE-conntrack-stale |
| RC-007 | **externalTrafficPolicy=Local 但本地无 Pod** — NodePort 或 LoadBalancer 类型 Service 设置了 `externalTrafficPolicy: Local`，流量到达的节点上没有就绪的后端 Pod，kube-proxy 不会将流量转发到其他节点，直接丢弃。表现为"部分节点可达、部分不可达" | 中 | D2.8 `externalTrafficPolicy=Local`；仅部分节点的 NodePort 可达；不可达节点上确实没有后端 Pod | service-fta: BE-local-traffic-policy |
| RC-008 | **Headless Service 但 Pod 未注册 DNS** — Headless Service（`ClusterIP: None`）依赖 DNS 返回 Pod IP 列表，但后端 Pod 未正确注册 DNS A/AAAA 记录。可能因为 Pod 未设置 `hostname`/`subdomain`、Pod 未就绪、或 CoreDNS 同步延迟 | 低 | Headless Service 的 DNS 查询返回空结果；Pod 存在且就绪但 DNS 记录缺失；CoreDNS 日志中无错误 | service-fta: BE-headless-dns |
| RC-009 | **LoadBalancer 类型 Service 的云厂商控制器异常** — cloud-controller-manager 无法正常工作，未能在云端创建/更新 Load Balancer 资源，导致 `status.loadBalancer.ingress` 为空（External IP 持续 `<pending>`）。可能原因包括云端 API 权限不足、配额耗尽、Service annotations 配置错误 | 中 | D1.1 External IP 为 pending；D1.5 事件显示 LB 创建失败；D2.9 cloud-controller-manager 日志有错误 | service-fta: BE-cloud-lb-failure |
| RC-010 | **Service 协议（TCP/UDP）与应用不匹配** — Service 定义的协议（如 TCP）与应用实际监听的协议（如 UDP）不一致，导致流量无法被正确处理。常见于 DNS 服务（需同时暴露 TCP 和 UDP）或游戏服务器（使用 UDP） | 低 | D2.2 Service protocol 与容器实际协议不一致；D3.2 协议测试失败 | service-fta: BE-protocol-mismatch |
| RC-011 | **sessionAffinity 配置导致流量不均或粘滞问题** — `sessionAffinity: ClientIP` 配置导致特定客户端的所有请求被固定路由到同一后端 Pod，当该 Pod 异常时客户端持续失败直到 affinity 超时。或 `timeoutSeconds` 配置过大导致负载严重不均 | 低 | D2.10 sessionAffinity 为 ClientIP 且 timeout 过长；特定客户端持续失败但其他客户端正常；更换客户端 IP 后恢复 | service-fta: BE-session-affinity |
| RC-012 | **跨节点网络（CNI）问题导致部分连通性问题** — CNI 插件（Calico/Cilium/Flannel 等）在某些节点上出现异常，导致跨节点的 Pod 间通信失败。表现为同一节点上的 Pod 互通，但跨节点访问 Service 失败 | 中 | D2.4 同节点 Pod 可直接通信但跨节点失败；D3.1 连通性矩阵显示特定节点模式的失败；CNI Pod 日志有错误 | service-fta: BE-cni-cross-node |
| RC-013 | **EndpointSlice 与 Endpoints 不一致** — v1.28+ 默认使用 EndpointSlice 作为 endpoint 分发机制，但某些旧版控制器或自定义组件可能仍依赖 legacy Endpoints。两者不一致时可能导致部分流量路由异常或 Service 不可达 | 低 | D1.2 EndpointSlice 与 Endpoints 数据不一致；`kubectl get endpointslices` 与 `kubectl get endpoints` 对比显示差异；kube-proxy 日志显示使用 EndpointSlice 但其他组件使用 Endpoints | service-fta: BE-endpointslice-inconsistent |
| RC-014 | **Service Mesh sidecar 异常** — Istio/Linkerd 等 Service Mesh 的 sidecar proxy 出现问题，包括：sidecar 未注入、注入失败、mTLS 握手错误、VirtualService/DestinationRule 路由不匹配、identity 证书过期等。表现为 mesh 内部 Service 通信失败 | 中 | D4.1 Pod 不包含 istio-proxy/linkerd-proxy 容器；D4.3 mTLS tls-check 显示配置不一致；D4.4 Envoy 日志出现 upstream connect error 或 TLS error；D4.5 `linkerd check` 显示异常 | service-fta: BE-mesh-sidecar-failure |
| RC-015 | **多集群 Service (MCS API) 连通性问题** — 使用 Multi-Cluster Service API (ServiceExport/ServiceImport) 或 Submariner 等方案时，跨集群 Service 发现或路由失败。表现为 `clusterset.local` 域名解析失败或跨集群流量无法路由 | 低 | ServiceExport/ServiceImport 状态不同步；跨集群 DNS 解析失败（`nslookup <service>.<namespace>.svc.clusterset.local`）；网络隧道/VPN 连接中断；MCS controller 日志有同步错误 | service-fta: BE-mcs-connectivity |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: 修复 Service label selector 与 Pod labels 匹配
- **适用根因**: RC-001
- **前置检查**:
  ```bash
  # 确认 Service selector 和 Pod labels 的具体差异
  echo "=== Service Selector ===" && \
  kubectl get svc <service> -n <namespace> -o jsonpath='{.spec.selector}' && echo "" && \
  echo "=== Pod Labels (from Deployment template) ===" && \
  kubectl get deployment <deployment> -n <namespace> -o jsonpath='{.spec.template.metadata.labels}' && echo "" && \
  echo "=== Actual Pod Labels ===" && \
  kubectl get pods -n <namespace> -l <any-known-label> --show-labels
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案A: 修复 Service 的 selector（推荐，不影响已运行 Pod）
  kubectl patch svc <service> -n <namespace> -p '{"spec":{"selector":{"app":"<correct-value>","version":"<correct-value>"}}}'

  # 方案B: 如果是 Pod labels 缺失，修复 Deployment 的 Pod template labels
  # 注意: 修改 Deployment template 会触发滚动更新
  kubectl patch deployment <deployment> -n <namespace> -p '{"spec":{"template":{"metadata":{"labels":{"<missing-key>":"<value>"}}}}}'
  ```
- **后置验证**:
  ```bash
  # 等待 Endpoints 更新（通常 <10s）
  sleep 10
  kubectl get endpoints <service> -n <namespace>
  # 预期: ENDPOINTS 列显示后端 Pod IP 列表

  kubectl get endpointslices -l kubernetes.io/service-name=<service> -n <namespace>
  # 预期: EndpointSlice 中包含就绪地址
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 恢复原始 selector
  kubectl patch svc <service> -n <namespace> -p '{"spec":{"selector":{"app":"<original-value>"}}}'
  ```

#### REM-002: 修复 Service targetPort/port 映射
- **适用根因**: RC-003, RC-010
- **前置检查**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 确认容器实际监听的端口
  kubectl exec <pod> -n <namespace> -- ss -tlnp 2>/dev/null || \
  kubectl exec <pod> -n <namespace> -- netstat -tlnp 2>/dev/null
  # 记录实际监听的端口号和协议
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 修复 targetPort
  kubectl patch svc <service> -n <namespace> --type='json' \
    -p='[{"op":"replace","path":"/spec/ports/0/targetPort","value":<correct-port>}]'

  # 如果协议也需要修复
  kubectl patch svc <service> -n <namespace> --type='json' \
    -p='[{"op":"replace","path":"/spec/ports/0/protocol","value":"<TCP-or-UDP>"}]'
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 测试连通性
  kubectl exec <test-pod> -- curl -s -o /dev/null -w "%{http_code}" --connect-timeout 5 http://<service-cluster-ip>:<port>/
  # 预期: 200 或其他正常 HTTP 状态码

  # 确认端口映射已更新
  kubectl get svc <service> -n <namespace> -o jsonpath='{range .spec.ports[*]}{"port:"}{.port}{" targetPort:"}{.targetPort}{" protocol:"}{.protocol}{"\n"}{end}'
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 恢复原始 targetPort
  kubectl patch svc <service> -n <namespace> --type='json' \
    -p='[{"op":"replace","path":"/spec/ports/0/targetPort","value":<original-port>}]'
  ```

#### REM-003: 强制刷新 Endpoints（删除并等待重建）
- **适用根因**: RC-001, RC-002（在修复 selector/readiness 之后 Endpoints 未自动更新时）
- **前置检查**:
  ```bash
  # 确认 Service 有 selector（有 selector 的 Service 的 Endpoints 由 controller 自动管理）
  kubectl get svc <service> -n <namespace> -o jsonpath='{.spec.selector}'
  # 预期: 非空（如果为空，说明是手动管理的 Endpoints，不适用此操作）

  # 确认后端 Pod 已就绪
  kubectl get pods -n <namespace> -l <selector> -o custom-columns=NAME:.metadata.name,READY:.status.containerStatuses[*].ready
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 删除 Endpoints 对象（controller 会自动重建）
  kubectl delete endpoints <service> -n <namespace>

  # 等待自动重建
  sleep 5
  ```
- **后置验证**:
  ```bash
  kubectl get endpoints <service> -n <namespace>
  # 预期: ENDPOINTS 列显示后端 Pod IP

  kubectl get endpointslices -l kubernetes.io/service-name=<service> -n <namespace>
  # 预期: EndpointSlice 已重建
  ```
- **回滚命令**:
  ```bash
  # Endpoints 由 controller 自动管理，删除后会自动重建，无需手动回滚
  # 如果删除后未自动重建，检查 kube-controller-manager 状态
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-004: 调整 readiness probe 配置
- **适用根因**: RC-002
- **影响说明**: 修改 Deployment/StatefulSet 的 readiness probe 配置将触发 Pod 滚动更新，短暂期间可用 Pod 数量减少。如果修改不当（如完全移除 readiness probe），可能导致未就绪 Pod 接收流量。
- **审批提示**: "建议修改 Deployment `<deployment>` 的 readiness probe 配置（调整 `initialDelaySeconds`/`timeoutSeconds`/`path`/`port`）。此操作将触发 Pod 滚动更新，在更新期间可用后端数量可能短暂减少。是否批准？"
- **前置检查**:
  ```bash
  # 确认当前 readiness probe 配置
  kubectl get deployment <deployment> -n <namespace> -o jsonpath='{.spec.template.spec.containers[0].readinessProbe}' | jq .

  # 确认滚动更新策略
  kubectl get deployment <deployment> -n <namespace> -o jsonpath='{.spec.strategy}'
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 示例: 增加 initialDelaySeconds 和 timeoutSeconds
  kubectl patch deployment <deployment> -n <namespace> --type='json' \
    -p='[{"op":"replace","path":"/spec/template/spec/containers/0/readinessProbe/initialDelaySeconds","value":30},
         {"op":"replace","path":"/spec/template/spec/containers/0/readinessProbe/timeoutSeconds","value":5},
         {"op":"replace","path":"/spec/template/spec/containers/0/readinessProbe/periodSeconds","value":10}]'
  ```
- **后置验证**:
  ```bash
  # 等待滚动更新完成
  kubectl rollout status deployment/<deployment> -n <namespace> --timeout=300s

  # 检查新 Pod readiness
  kubectl get pods -n <namespace> -l <selector> -o custom-columns=NAME:.metadata.name,READY:.status.containerStatuses[*].ready

  # 检查 Endpoints
  kubectl get endpoints <service> -n <namespace>
  # 预期: ENDPOINTS 中包含新 Pod IP
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 回滚 Deployment 到上一版本
  kubectl rollout undo deployment/<deployment> -n <namespace>
  kubectl rollout status deployment/<deployment> -n <namespace> --timeout=300s
  ```

#### REM-005: 修复或移除阻断性 NetworkPolicy
- **适用根因**: RC-005
- **影响说明**: 修改 NetworkPolicy 将立即影响网络策略的执行。移除过于严格的策略可能暂时暴露未预期的网络路径。应确保修改后的策略符合安全要求。
- **审批提示**: "发现 NetworkPolicy `<policy-name>` 阻断了从 `<source>` 到 Service `<service>` 后端的流量。建议 [添加允许规则/修改策略/临时移除策略]。此操作将立即生效。是否批准？"
- **前置检查**:
  ```bash
  # 完整列出可能影响的 NetworkPolicy
  kubectl get networkpolicy -n <namespace> -o yaml

  # 确认是哪条策略阻断了流量
  kubectl describe networkpolicy <policy-name> -n <namespace>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 方案A: 为现有 default-deny 策略添加允许规则
  kubectl apply -f - <<EOF
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: allow-to-<service>
    namespace: <namespace>
  spec:
    podSelector:
      matchLabels:
        <service-pod-label-key>: <service-pod-label-value>
    ingress:
    - from:
      - namespaceSelector:
          matchLabels:
            kubernetes.io/metadata.name: <client-namespace>
        podSelector:
          matchLabels:
            <client-pod-label-key>: <client-pod-label-value>
      ports:
      - port: <service-target-port>
        protocol: TCP
    policyTypes:
    - Ingress
  EOF

  # 方案B: 临时删除阻断性策略（高紧急场景）
  # kubectl delete networkpolicy <blocking-policy-name> -n <namespace>
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 测试连通性
  kubectl exec <client-pod> -n <client-namespace> -- curl -s -o /dev/null -w "%{http_code}" --connect-timeout 5 http://<service>.<namespace>.svc.cluster.local:<port>/
  # 预期: 连接成功

  # 确认 NetworkPolicy 已生效
  kubectl get networkpolicy -n <namespace>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 方案A 回滚: 删除新添加的允许策略
  kubectl delete networkpolicy allow-to-<service> -n <namespace>

  # 方案B 回滚: 重新应用被删除的策略
  kubectl apply -f <backed-up-policy.yaml>
  ```

#### REM-006: 重启 kube-proxy Pod
- **适用根因**: RC-004
- **影响说明**: 重启 kube-proxy DaemonSet 中的特定节点 Pod 会导致该节点上的 Service 转发规则短暂中断（通常 <30s）。kube-proxy 重启后会重新从 apiserver 同步所有 Service/Endpoints 信息并重建规则。
- **审批提示**: "建议重启节点 `<node-name>` 上的 kube-proxy Pod 以重新同步 iptables/IPVS 规则。该节点上的 Service 转发将短暂中断（约 10-30s）。是否批准？"
- **前置检查**:
  ```bash
  # 确认 kube-proxy Pod 状态
  kubectl get pods -n kube-system -l k8s-app=kube-proxy --field-selector spec.nodeName=<node-name> -o wide

  # 记录当前 kube-proxy 日志中的错误（用于对比）
  kubectl logs -n kube-system -l k8s-app=kube-proxy --field-selector spec.nodeName=<node-name> --tail=20
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 删除特定节点上的 kube-proxy Pod（DaemonSet controller 会自动重建）
  KUBE_PROXY_POD=$(kubectl get pods -n kube-system -l k8s-app=kube-proxy --field-selector spec.nodeName=<node-name> -o jsonpath='{.items[0].metadata.name}')
  kubectl delete pod $KUBE_PROXY_POD -n kube-system
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 等待新 Pod 启动
  sleep 30

  # 检查新 Pod 状态
  kubectl get pods -n kube-system -l k8s-app=kube-proxy --field-selector spec.nodeName=<node-name> -o wide
  # 预期: Running, READY 1/1

  # 检查日志确认规则已同步
  kubectl logs -n kube-system -l k8s-app=kube-proxy --field-selector spec.nodeName=<node-name> --tail=20
  # 预期: 无 error 日志，出现 "Syncing iptables rules" 或类似同步成功信息

  # 测试 Service 连通性
  kubectl exec <test-pod-on-node> -- curl -s -o /dev/null -w "%{http_code}" --connect-timeout 5 http://<service-cluster-ip>:<port>/
  ```
- **回滚命令**:
  ```bash
  # kube-proxy Pod 重建为幂等操作，无需回滚
  # 如果重建后问题未解决，不要反复重启，应升级处理
  ```

#### REM-007: 修改 externalTrafficPolicy 从 Local 改为 Cluster
- **适用根因**: RC-007
- **影响说明**: 将 `externalTrafficPolicy` 从 `Local` 改为 `Cluster` 后，所有节点都将接受并转发外部流量到后端 Pod（即使 Pod 不在本节点上）。这将解决"部分节点不可达"的问题，但代价是：(1) 丢失客户端源 IP（被 SNAT），(2) 增加一跳网络延迟。
- **审批提示**: "建议将 Service `<service>` 的 `externalTrafficPolicy` 从 `Local` 改为 `Cluster`。这将使所有节点均可转发流量，但**会丢失客户端真实源 IP**。是否批准？"
- **前置检查**:
  ```bash
  # 确认当前配置
  kubectl get svc <service> -n <namespace> -o jsonpath='{.spec.externalTrafficPolicy}'
  # 预期: Local

  # 检查是否有依赖客户端源 IP 的逻辑（如 IP 白名单、审计日志）
  # 需人工确认
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch svc <service> -n <namespace> -p '{"spec":{"externalTrafficPolicy":"Cluster"}}'
  ```
- **后置验证**:
  ```bash
  # 测试 NodePort 在之前不可达的节点上是否可达
  curl -s --connect-timeout 5 http://<previously-unreachable-node-ip>:<nodeport>/
  # 预期: 连接成功

  # 确认配置已更新
  kubectl get svc <service> -n <namespace> -o jsonpath='{.spec.externalTrafficPolicy}'
  # 预期: Cluster
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  kubectl patch svc <service> -n <namespace> -p '{"spec":{"externalTrafficPolicy":"Local"}}'
  ```

#### REM-012: Service Mesh sidecar 修复
- **适用根因**: RC-014
- **影响说明**: 修复 Service Mesh sidecar 问题可能涉及重启 Pod、修改注入配置或更新 mTLS 设置。重启 Pod 会导致其上运行的工作负载短暂中断。
- **审批提示**: "发现 Pod `<pod-name>` 的 Service Mesh sidecar 异常。建议 [**重启 Pod**/**修复注入标签**/**更新 mTLS 配置**]。操作期间 Pod 将短暂不可用。是否批准？"
- **前置检查**:
  ```bash
  # 确认 mesh 控制平面健康
  # Istio:
  istioctl version
  kubectl get pods -n istio-system
  istioctl analyze -n <namespace>
  
  # Linkerd:
  linkerd check
  linkerd check --proxy -n <namespace>
  
  # 检查目标 Pod sidecar 状态
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].name}'
  kubectl logs <pod-name> -n <namespace> -c istio-proxy --tail=50 2>/dev/null || \
  kubectl logs <pod-name> -n <namespace> -c linkerd-proxy --tail=50 2>/dev/null
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 方案A: 重启 Pod 以触发 sidecar 重新注入（最常用）
  kubectl delete pod <pod-name> -n <namespace>
  # 等待新 Pod 启动
  kubectl wait --for=condition=Ready pod -l <selector> -n <namespace> --timeout=120s
  
  # 方案B: 修复 namespace 注入标签（如果注入未启用）
  # Istio:
  kubectl label namespace <namespace> istio-injection=enabled --overwrite
  # 然后重启 Deployment 所有 Pod:
  kubectl rollout restart deployment/<deployment-name> -n <namespace>
  
  # 方案C: 修复 mTLS 配置（Istio STRICT -> PERMISSIVE）
  kubectl apply -f - <<EOF
  apiVersion: security.istio.io/v1beta1
  kind: PeerAuthentication
  metadata:
    name: default
    namespace: <namespace>
  spec:
    mtls:
      mode: PERMISSIVE
  EOF
  
  # 方案D: 更新 DestinationRule TLS 配置
  kubectl apply -f - <<EOF
  apiVersion: networking.istio.io/v1beta1
  kind: DestinationRule
  metadata:
    name: <service-name>-mtls
    namespace: <namespace>
  spec:
    host: <service-name>.<namespace>.svc.cluster.local
    trafficPolicy:
      tls:
        mode: ISTIO_MUTUAL
  EOF
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 确认 sidecar 注入成功
  kubectl get pod <new-pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].name}' | grep -E 'istio-proxy|linkerd-proxy'
  # 预期: 输出包含 sidecar 容器名
  
  # 确认 sidecar 状态正常
  kubectl get pod <new-pod-name> -n <namespace>
  # 预期: READY 列显示 2/2 (或包含 sidecar 的正确数量)
  
  # Istio: 检查 mTLS 状态
  istioctl authn tls-check <new-pod-name>.<namespace>
  # 预期: 显示正确的 mTLS 状态
  
  # 测试 Service 连通性
  kubectl exec <test-pod> -n <namespace> -- curl -s -o /dev/null -w "%{http_code}" --connect-timeout 5 http://<service>:<port>/
  # 预期: 200 或其他正常 HTTP 状态码
  
  # Linkerd: 检查 proxy 状态
  linkerd diagnostics proxy-metrics <new-pod-name> -n <namespace> | head -20
  # 预期: 无大量错误指标
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 方案A 回滚: Pod 重启为幂等操作，无需回滚
  
  # 方案B 回滚: 移除注入标签
  kubectl label namespace <namespace> istio-injection-
  kubectl rollout restart deployment/<deployment-name> -n <namespace>
  
  # 方案C/D 回滚: 删除新增的配置
  kubectl delete peerauthentication default -n <namespace>
  kubectl delete destinationrule <service-name>-mtls -n <namespace>
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-008: 清空节点 conntrack 表项
- **适用根因**: RC-006
- **影响说明**: 清空特定 Service ClusterIP 相关的 conntrack 条目会导致与该 Service 相关的所有现有 TCP 连接被重置。对于有状态长连接（如 WebSocket、gRPC streaming）的服务，这将导致连接中断。清空操作后，新连接将正常建立。
- **操作步骤**:
  1. **确认影响范围**:
     ```bash
     # 计算受影响的连接数
     kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- conntrack -L -d <service-cluster-ip> 2>/dev/null | wc -l
     ```
  2. **清空特定 Service 的 conntrack 条目**:
     ```bash
     kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- conntrack -D -d <service-cluster-ip>
     ```
  3. **验证清理结果**:
     ```bash
     kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- conntrack -L -d <service-cluster-ip> 2>/dev/null | wc -l
     # 预期: 0 或极少数量（新连接）
     ```
- **安全检查**:
  - 确认无关键的长连接服务（WebSocket, gRPC streaming, 数据库连接池）通过该 Service
  - 确认业务侧有连接重试机制
- **回滚方案**:
  ```bash
  # conntrack 清空为不可逆操作，但影响有限
  # 被清空的连接将由应用的重试机制自动恢复
  # 如果大量连接断开导致业务雪崩，需紧急扩容后端 Pod 承接重连流量
  ```

#### REM-009: 切换 kube-proxy 运行模式（iptables ↔ IPVS ↔ nftables）
- **适用根因**: RC-004（当前模式存在已知 bug 或不兼容问题时）
- **影响说明**: 修改 kube-proxy 运行模式需要更新 ConfigMap 并重启所有 kube-proxy Pod。切换期间，所有节点上的 Service 转发规则将被重建，可能导致全集群范围内 Service 连通性短暂中断（30s-2min）。
- **操作步骤**:
  1. **备份当前配置**:
     ```bash
     kubectl get configmap kube-proxy -n kube-system -o yaml > kube-proxy-configmap-backup.yaml
     ```
  2. **修改 kube-proxy ConfigMap**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

     ```bash
     kubectl edit configmap kube-proxy -n kube-system
     # 修改 mode 字段:
     #   mode: "ipvs"  (或 "iptables" 或 "nftables")
     # 如果切换到 IPVS，确保节点内核加载了 ip_vs 模块
     ```
  3. **滚动重启 kube-proxy DaemonSet**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

     ```bash
     kubectl rollout restart daemonset kube-proxy -n kube-system
     kubectl rollout status daemonset kube-proxy -n kube-system --timeout=300s
     ```
  4. **清理旧模式的规则**（如从 iptables 切换到 IPVS）:
     ```bash
     # 在各节点上清理旧 iptables 规则（kube-proxy 通常会自动清理）
     # 如果未自动清理:
     # iptables-save | grep -v KUBE | iptables-restore
     ```
- **安全检查**:
  - 确认目标模式的内核模块已加载（IPVS 需要 `ip_vs`, `ip_vs_rr`, `ip_vs_wrr`, `ip_vs_sh`）
  - 确认集群内无依赖特定 kube-proxy 模式行为的组件
  - 建议在维护窗口内执行
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 恢复原始 ConfigMap
  kubectl apply -f kube-proxy-configmap-backup.yaml
  # 再次滚动重启
  kubectl rollout restart daemonset kube-proxy -n kube-system
  kubectl rollout status daemonset kube-proxy -n kube-system --timeout=300s
  ```

#### REM-010: 修复 CNI 网络（重建跨节点通信）
- **适用根因**: RC-012
- **影响说明**: CNI 插件的修复操作因插件类型而异。可能涉及重启 CNI DaemonSet Pod、清理 CNI 状态、或重新配置 CNI。操作不当可能导致节点级别的网络中断。
- **操作步骤**:
  1. **确认 CNI 插件类型**:
     ```bash
     kubectl get pods -n kube-system | grep -E "calico|cilium|flannel|weave|canal"
     ls /etc/cni/net.d/  # 通过 kubectl debug node 执行
     ```
  2. **重启问题节点上的 CNI Pod**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

     ```bash
     # Calico 示例
     kubectl delete pod -n kube-system -l k8s-app=calico-node --field-selector spec.nodeName=<problem-node>

     # Cilium 示例
     kubectl delete pod -n kube-system -l k8s-app=cilium --field-selector spec.nodeName=<problem-node>
     ```
  3. **检查 CNI 配置一致性**:
     ```bash
     # 对比问题节点和正常节点的 CNI 配置
     kubectl debug node/<problem-node> -it --image=busybox -- cat /host/etc/cni/net.d/10-calico.conflist
     kubectl debug node/<healthy-node> -it --image=busybox -- cat /host/etc/cni/net.d/10-calico.conflist
     ```
  4. **验证跨节点连通性恢复**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

     ```bash
     kubectl exec <pod-on-node-a> -- ping -c 3 <pod-ip-on-node-b>
     kubectl exec <pod-on-node-a> -- curl -s --connect-timeout 5 http://<pod-ip-on-node-b>:<port>/
     ```
- **安全检查**:
  - 确认修改不影响其他正常节点的网络
  - 如需修改 CNI 配置，先在非生产节点验证
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 恢复原始 CNI 配置文件（需提前备份）
  # 重启 CNI DaemonSet Pod
  kubectl rollout restart daemonset <cni-daemonset> -n kube-system
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-011: 全集群 kube-proxy DaemonSet 滚动重启
- **适用根因**: RC-004（全集群范围的 kube-proxy 规则异常）
- **审批要求**: 需要高级 SRE + 网络 Team Lead 审批。建议在维护窗口内执行。
- **数据备份**: 备份 kube-proxy ConfigMap 和当前 iptables/IPVS 规则
- **操作步骤**:
  1. **备份当前状态**:
     ```bash
     kubectl get configmap kube-proxy -n kube-system -o yaml > /tmp/kube-proxy-cm-backup.yaml
     kubectl get daemonset kube-proxy -n kube-system -o yaml > /tmp/kube-proxy-ds-backup.yaml
     ```
  2. **设置 DaemonSet 更新策略**（确保 maxUnavailable 合理）:
     ```bash
     kubectl get daemonset kube-proxy -n kube-system -o jsonpath='{.spec.updateStrategy}'
     # 确认 maxUnavailable 不超过节点总数的 25%
     ```
  3. **执行滚动重启**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

     ```bash
     kubectl rollout restart daemonset kube-proxy -n kube-system
     ```
  4. **监控重启进度**:
     ```bash
     kubectl rollout status daemonset kube-proxy -n kube-system --timeout=600s
     ```
  5. **验证全集群 Service 连通性**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

     ```bash
     # 在多个节点上的 Pod 中测试 Service 访问
     for pod in <pod1> <pod2> <pod3>; do
       kubectl exec $pod -- curl -s -o /dev/null -w "$pod: %{http_code}\n" --connect-timeout 5 http://kubernetes.default.svc.cluster.local/healthz
     done
     ```
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 如果重启导致大规模 Service 中断
  # 恢复 DaemonSet 配置
  kubectl apply -f /tmp/kube-proxy-ds-backup.yaml
  # DaemonSet controller 会自动回滚到旧 Pod 模板
  ```

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# V1: 确认 Endpoints 已填充后端 Pod IP
kubectl get endpoints <service> -n <namespace>
# 预期: ENDPOINTS 列显示后端 Pod IP 列表（非 <none>）

# V2: 确认 EndpointSlice 包含就绪地址
kubectl get endpointslices -l kubernetes.io/service-name=<service> -n <namespace> -o jsonpath='{range .items[*].endpoints[*]}{.addresses}{" ready="}{.conditions.ready}{"\n"}{end}'
# 预期: 每个地址的 ready=true

# V3: 从测试 Pod 访问 Service ClusterIP
kubectl exec <test-pod> -- curl -s -o /dev/null -w "%{http_code}" --connect-timeout 5 http://<service-cluster-ip>:<port>/
# 预期: 200 或预期的 HTTP 状态码

# V4: 如果是 NodePort Service，从外部测试
curl -s --connect-timeout 5 http://<node-ip>:<nodeport>/
# 预期: 连接成功

# V5: 如果是 LoadBalancer Service，确认 External IP 已分配且可达
kubectl get svc <service> -n <namespace> -o jsonpath='{.status.loadBalancer.ingress[0].ip}'
# 预期: 有效 IP 地址（非空）
curl -s --connect-timeout 10 http://<external-ip>:<port>/
# 预期: 连接成功

# V6: 确认 Service 相关事件无新错误
kubectl get events -n <namespace> --field-selector involvedObject.name=<service> --sort-by=.lastTimestamp | tail -5
# 预期: 无新的 Warning 事件
```
### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| Endpoint 就绪数量 | `kube_endpoint_address_available{endpoint="<service>"}` | 稳定在预期 Pod 数量 | 下降到 0 或持续波动 |
| Endpoint 未就绪数量 | `kube_endpoint_address_not_ready{endpoint="<service>"}` | 稳定为 0 | 大于 0 且持续 |
| Service 连接成功率 | 应用侧监控或 Ingress controller 的 upstream_response_code | 成功率 >99.9% | 错误率 >1% 持续 5 分钟 |
| 请求延迟 | P99 响应时间 | 恢复到基线水平 | P99 >10x 基线值 |
| kube-proxy 同步状态 | `kubeproxy_sync_proxy_rules_last_timestamp_seconds` | 持续更新 | 长时间不更新（>60s） |
| kube-proxy 规则同步延迟 | `kubeproxy_sync_proxy_rules_duration_seconds` | P99 < 1s | P99 > 5s |
| Pod readiness 状态 | `kube_pod_status_ready{pod=~"<service-pods>"}` | 全部为 1 | 任何 Pod 变为 0 |
| conntrack 表使用率 | `node_nf_conntrack_entries / node_nf_conntrack_entries_limit` | <50% | >80% |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] Service 的 Endpoints 包含预期数量的后端 Pod IP 地址
- [ ] 从集群内不同节点上的 Pod 均能成功访问 Service（ClusterIP 层面）
- [ ] 如果是 NodePort/LoadBalancer 类型，从外部也能正常访问
- [ ] 连续 5 分钟内无新的 Service 相关 Warning 事件
- [ ] 后端 Pod 的 readiness probe 持续通过
- [ ] 应用侧的错误率/延迟指标恢复到正常基线
- [ ] kube-proxy 日志中无新的错误信息
- [ ] 根因已明确记录并采取了预防措施

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| Endpoint 数量稳定性 | `kube_endpoint_address_available` 监控 | 持续 | 数量下降 → 重新进入本 Skill 诊断 |
| Service 错误率 | Ingress controller / 应用 metrics | 持续 | 错误率上升 → 检查后端 Pod 健康状况 |
| Pod readiness 变化 | `kube_pod_status_ready` 监控 | 每 15 分钟 | Pod 频繁在 ready/not-ready 间切换 → 检查 readiness probe 配置和应用稳定性 |
| kube-proxy 状态 | kube-proxy Pod 日志和重启次数 | 每小时 | kube-proxy Pod 重启 → 排查 kube-proxy 崩溃原因 |
| NetworkPolicy 变更 | Audit log 中 NetworkPolicy 的 create/update/delete 事件 | 每 4 小时 | 新的 NetworkPolicy 可能再次阻断流量 |
| conntrack 表使用趋势 | `node_nf_conntrack_entries` 趋势图 | 每小时 | 线性增长 → 可能有连接泄漏或 conntrack 超时过长 |
| 滚动更新后 Endpoints 变化 | Deployment rollout 事件 + Endpoint 变化 | 持续 | 新版本 Pod readiness 失败导致 Endpoints 为空 → 检查新版本应用健康 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后 V1-V6 验证失败 |
| **严重性升级** | 初始分级为 P2 但影响面扩大（如更多 Service 出现同样问题） | 诊断过程中受影响 Service 数量增加 |
| **未知根因** | 完成 Phase 1-3 所有诊断步骤但无法匹配任何已知根因（RC-001 至 RC-012） | 所有诊断步骤均无明确异常发现 |
| **全局问题** | 多个不相关的 Service 同时出现连通性问题 | T4 阶段发现多个 Service 受影响 |
| **控制平面组件异常** | kube-controller-manager 的 endpoint-controller 或 cloud-controller-manager 异常 | 所有新建 Service 的 Endpoints 均无法自动创建 |

### 8.2 升级消息模板

```
【{severity}】Service 连通性与 Endpoint 异常 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: Service {service_name} ({namespace}/{service_type}) 连通性异常，持续 {duration}
- 影响范围:
  - 受影响 Service: {service_name} (type: {service_type})
  - Endpoint 状态: {endpoint_count} 个就绪 / {total_pods} 个后端 Pod
  - 受影响客户端: {affected_clients}
  - 是否影响外部流量: {external_traffic_affected}
- 已完成诊断:
  - Phase 1 快速检查: {phase1_summary}
  - Phase 2 深度检查: {phase2_summary}
  - Phase 3 主动探测: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-NET-002 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤（D1.1 → D1.2 → ... → D3.3）及每步输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
   - 例: "RC-001 已排除 — D2.1 显示 Service selector 与 Pod labels 完全匹配"
   - 例: "RC-005 已排除 — D2.6 显示 namespace 中无 NetworkPolicy"
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
   - 例: "疑似 RC-004（kube-proxy 异常）— D2.4 直接 Pod IP 可达但 ClusterIP 不可达，D2.5 kube-proxy 日志有 sync error"
4. **关键资源快照**:
   ```bash
   # Service 描述
   kubectl describe svc <service> -n <namespace> > svc-describe.txt
   # Endpoints 详情
   kubectl get endpoints <service> -n <namespace> -o yaml > endpoints.yaml
   # EndpointSlice 详情
   kubectl get endpointslices -l kubernetes.io/service-name=<service> -n <namespace> -o yaml > endpointslices.yaml
   # 后端 Pod 状态
   kubectl get pods -n <namespace> -l <selector> -o wide > backend-pods.txt
   # NetworkPolicy（如有）
   kubectl get networkpolicy -n <namespace> -o yaml > networkpolicies.yaml
   # kube-proxy 日志
   kubectl logs -n kube-system -l k8s-app=kube-proxy --since=30m > kube-proxy-logs.txt
   # 相关事件
   kubectl get events -n <namespace> --sort-by=.lastTimestamp > events.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件按时间排列
   - `HH:MM:SS` - 首次检测到 Service 连通性异常
   - `HH:MM:SS` - 开始诊断
   - `HH:MM:SS` - 发现异常 [描述]
   - `HH:MM:SS` - 尝试修复 [操作]
   - `HH:MM:SS` - 修复结果 [成功/失败]
   - `HH:MM:SS` - 决定升级

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| EndpointSlice (discovery.k8s.io/v1) | GA（默认） | GA | GA | GA | GA |
| kube-proxy iptables 模式 | GA | GA | GA | GA | GA |
| kube-proxy IPVS 模式 | GA | GA | GA | GA | GA |
| kube-proxy nftables 模式 | 不可用 | alpha | alpha | beta | GA |
| Topology Aware Routing | beta | beta | GA | GA | GA |
| `internalTrafficPolicy` | GA | GA | GA | GA | GA |
| `externalTrafficPolicy` | GA | GA | GA | GA | GA |
| Traffic Distribution (`spec.trafficDistribution`) | 不可用 | 不可用 | 不可用 | beta | beta |
| Multiple Service CIDRs | 不可用 | alpha | alpha | beta | beta |
| Service `ipFamilyPolicy` (dual-stack) | GA | GA | GA | GA | GA |
| EndpointSlice Mirroring | GA | GA | GA | GA | GA |
| `publishNotReadyAddresses` | GA | GA | GA | GA | GA |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl get endpointslices` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get endpoints` (legacy) | 支持 | 支持 | 支持 | 支持 | 支持 |
| `iptables-save | grep <clusterip>` | 有效 | 有效 | 有效 | 有效（非 nftables 模式） | 仅 iptables 模式 |
| `ipvsadm -ln | grep <clusterip>` | 有效（IPVS 模式） | 同左 | 同左 | 同左 | 同左 |
| `nft list ruleset | grep <clusterip>` | N/A | alpha 模式可用 | alpha 模式可用 | beta 模式可用 | GA，nftables 模式下必用 |
| `kubectl debug node/` 检查 conntrack | 支持 | 支持 | 支持 | 支持 | 支持 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Service | v1 (core) | v1 | v1 | v1 | v1 |
| Endpoints | v1 (core) | v1 | v1 | v1 | v1 |
| EndpointSlice | discovery.k8s.io/v1 | v1 | v1 | v1 | v1 |
| NetworkPolicy | networking.k8s.io/v1 | v1 | v1 | v1 | v1 |
| AdminNetworkPolicy | 不可用 | policy.networking.k8s.io/v1alpha1 | v1alpha1 | v1beta1 | v1beta1 |

### 9.4 版本相关的诊断注意事项

- **[v1.28+]**: EndpointSlice 是默认的 endpoint 分发机制。kube-proxy 默认消费 EndpointSlice 而非 legacy Endpoints。诊断时应优先检查 EndpointSlice（`kubectl get endpointslices`），legacy Endpoints 作为辅助参考。两者应保持一致。

- **[v1.29+]**: kube-proxy nftables 模式作为 alpha 可用。如果启用了 nftables 模式：
  - `iptables-save` 将**不会**显示 kube-proxy 创建的规则
  - 需使用 `nft list ruleset` 检查 Service 转发规则
  - 部分依赖 iptables 的监控/调试工具可能失效

- **[v1.30+]**: Topology Aware Routing GA。当 Service 配置了 `service.kubernetes.io/topology-mode: Auto` 时：
  - 流量优先路由到同一拓扑区域（zone）的后端 Pod
  - 如果同 zone 无就绪后端，可能出现连通性问题（非 bug，是预期行为）
  - 诊断时需检查 Pod 在不同 zone 的分布和就绪状态

- **[v1.31+]**: Traffic Distribution for Services (beta) 引入 `spec.trafficDistribution` 字段：
  - `PreferClose` 值优先将流量路由到网络拓扑上靠近的后端
  - 新字段可能与旧的 topology annotation 行为不完全一致
  - Multiple Service CIDRs (beta) 允许集群使用多个 Service CIDR 范围，诊断时需确认 Service ClusterIP 属于哪个 CIDR

- **[v1.32+]**: kube-proxy nftables 模式 GA：
  - 新集群可能默认使用 nftables 模式
  - 所有基于 `iptables-save` 的诊断命令需替换为 `nft list ruleset`
  - nftables 模式的规则格式与 iptables 完全不同，需要熟悉 nftables 语法
  - conntrack 行为在 nftables 模式下与 iptables 模式基本一致

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **DNS 问题误判为 Service 连通性问题** | 应用日志显示 "connection refused" 或 "no such host" 访问 Service 名称失败 | CoreDNS 异常导致 Service 名称无法解析为 ClusterIP，而非 Service 本身不可达 | 先用 `nslookup <service>.<namespace>.svc.cluster.local` 验证 DNS 解析。如果 DNS 解析失败，转到 SKILL-NET-001。只有 DNS 解析成功但 IP 不可达才属于本 Skill 范围 |
| **externalTrafficPolicy=Local 导致的"随机"失败** | NodePort/LoadBalancer Service 有时可达有时不可达，表现随机 | `externalTrafficPolicy: Local` 仅在有本地后端 Pod 的节点上接受外部流量，客户端访问不同节点时结果不同 | D2.8 中优先检查 externalTrafficPolicy 设置。如果为 Local，检查每个节点是否有就绪后端 Pod。"随机"的本质是客户端（或 LB 健康检查）轮询到不同节点 |
| **readiness probe 配置错误误判为 selector 不匹配** | Endpoints 为空，初步判断为 label selector 不匹配 | 实际 selector 匹配，但所有 Pod 的 readiness probe 失败（如 probe 的 path/port 与应用不一致），导致 endpoint-controller 不注册这些 Pod | D1.3 中区分"无 Pod 匹配 selector"和"Pod 匹配但未就绪"两种情况。前者是 RC-001，后者是 RC-002 |
| **conntrack 竞争条件导致间歇性断连** | Service 在滚动更新期间出现间歇性连接失败，更新完成后恢复 | 旧 Pod 被删除时其 conntrack 条目未及时清理，新连接被错误地路由到已不存在的旧 Pod IP | 如果问题仅在滚动更新期间出现，检查 conntrack 表中是否有指向旧 Pod IP 的条目。可通过增加 `terminationGracePeriodSeconds` 或使用 preStop hook 给 conntrack 留出清理时间 |
| **NetworkPolicy 端口遗漏导致的"部分"连通性** | Service 有多个端口（如 HTTP:80 和 gRPC:9090），其中一个可达另一个不可达 | NetworkPolicy 的 ingress 规则中只允许了部分端口，遗漏了 Service 使用的其他端口 | D2.6 中检查 NetworkPolicy 的 `ports` 字段是否覆盖了 Service 的所有端口。逐端口测试连通性 |
| **Service 类型为 Headless 但期望有 ClusterIP** | `kubectl get svc` 显示 CLUSTER-IP 为 None，无法通过 ClusterIP 访问 | Service 被创建为 Headless（`clusterIP: None`），设计上不分配 ClusterIP，需通过 DNS 查询 Pod IP 列表 | D1.1 中检查 Service 类型。如果 ClusterIP 为 None，确认这是否是预期行为。如果需要 ClusterIP，需重新创建 Service（ClusterIP 创建后不可修改） |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| Kubernetes 网络模型与 Service 实现 | `网络/` | 理解 Service 的底层实现（iptables/IPVS/nftables 规则生成机制） |
| Service 故障树分析 | `故障诊断/topic-fta/list/service-fta.md` | 理解 Service 连通性问题的完整因果链和概率模型 |
| Ingress 故障树分析 | `故障诊断/topic-fta/[[技能/ingress-fta.md|ingress-fta]].md` | 当问题涉及 Ingress → Service 链路时的参考 |
| 网络故障排查深度指南 | `故障诊断/topic-structural-trouble-shooting/` | 超出本 Skill 覆盖范围的深度网络排查方法 |
| Kubernetes 故障排查方法论 | `故障诊断/` | 系统化故障排查的理论基础和方法论 |
| DNS 诊断 | `SKILL-NET-001` | 当问题根因在 DNS 层面时的关联 Skill |
| Pod 崩溃诊断 | `SKILL-POD-001` | 当后端 Pod CrashLoopBackOff 导致 Endpoints 为空时的关联 Skill |
| 节点 NotReady 诊断 | `SKILL-NODE-001` | 当节点问题间接影响 Service 连通性时的关联 Skill |
| kube-proxy 架构与实现 | `网络/` | 理解 kube-proxy 的三种模式（iptables/IPVS/nftables）及其规则同步机制 |
| NetworkPolicy 原理与实现 | `网络/` | 理解不同 CNI 插件对 NetworkPolicy 的实现差异 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-03 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 12 个根因、11 个修复操作 | 基于 Service 连通性相关工单分析，建立完整诊断与修复流程 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **Service Mesh 场景**: Istio/Linkerd 等 Service Mesh 环境下的 Service 连通性诊断差异（sidecar proxy 注入后的流量路径变化、mTLS 证书问题）
2. **多集群 Service**: 使用 Multi-Cluster Service API 或 Submariner 等方案时的跨集群 Service 连通性诊断
3. **IPv6 / 双栈 Service**: 纯 IPv6 或 IPv4/IPv6 双栈 Service 的特定故障模式
4. **Windows 节点**: Windows 容器节点上的 kube-proxy（使用 userspace 或 kernelspace 模式）和 Service 连通性差异
5. **大规模集群**: 超大规模集群（>5000 节点）中 EndpointSlice 分片、kube-proxy 同步延迟等特有问题
6. **Gateway API**: Gateway API 替代 Ingress 后的 Service 后端连通性诊断差异
7. **eBPF 数据平面**: Cilium 等使用 eBPF 替代 kube-proxy 的场景下的诊断命令和方法差异

## 修复动作

> **本章定位**: 基于 Section 6 修复操作的快速决策摘要，供 Agent 在 QA 语料和运行时直接引用。

### 修复动作速查表

| 根因 | 修复动作 | 风险 | 验证命令 |
|------|---------|------|---------|
| RC-001 selector 不匹配 | `kubectl patch svc <svc> -n <ns> -p '{"spec":{"selector":{"app":"<correct-label>"}}}'` | 🟢 低风险 | `kubectl get endpoints <svc> -n <ns>` |
| RC-003 targetPort 不匹配 | `kubectl patch svc <svc> -n <ns> --type='json' -p='[{"op":"replace","path":"/spec/ports/0/targetPort","value":<correct-port>}]'` | 🟢 低风险 | `kubectl exec <test-pod> -- curl -s -o /dev/null -w "%{http_code}" http://<svc>:<port>/` |
| RC-003 协议不匹配 | `kubectl patch svc <svc> -n <ns> --type='json' -p='[{"op":"replace","path":"/spec/ports/0/protocol","value":"TCP"}]'` | 🟢 低风险 | `kubectl exec <test-pod> -- nc -zv <svc> <port>` |
| RC-002 readiness 失败 | 修复应用或调整 readiness probe 配置 | 🟡 中风险（触发 Pod 重建） | `kubectl get pods -n <ns> -l <selector>` |
| RC-004 kube-proxy 异常 | `kubectl delete pod -n kube-system -l k8s-app=kube-proxy`（DaemonSet 自动重建） | 🟡 中风险（iptables/IPVS 规则短暂不同步） | `kubectl get pods -n kube-system -l k8s-app=kube-proxy` |
| RC-005 NetworkPolicy 阻断 | 修改 NetworkPolicy 放行对应 namespace/pod 和端口 | 🟡 中风险（可能意外扩大攻击面） | `kubectl exec <test-pod> -- curl -s -o /dev/null -w "%{http_code}" http://<svc>:<port>/` |
| RC-007 externalTrafficPolicy=Local | 改为 `externalTrafficPolicy: Cluster` 或确保本地有后端 Pod | 🟡 中风险（改变流量转发行为，可能丢失客户端源 IP） | `curl -s -o /dev/null -w "%{http_code}" http://<node-ip>:<nodeport>/` |
| RC-009 LoadBalancer pending | 检查 cloud-controller-manager 日志，修正 Service annotations | 🟡 中风险（可能重新创建 LB，IP 可能变更） | `kubectl get svc <svc> -n <ns>` |

### danger_operations 高风险操作标注

```yaml
danger_operations:
  - operation: "修改 NetworkPolicy 规则"
    risk: "错误的 NetworkPolicy 可能意外放行攻击流量或阻断合法流量"
    prerequisite:
      - "修改前备份现有策略: kubectl get networkpolicy -n <ns> -o yaml > netpol-backup.yaml"
      - "在测试 namespace 验证策略效果"
    rollback: "kubectl apply -f netpol-backup.yaml"

  - operation: "删除 kube-proxy Pod 强制重建"
    risk: "重建期间该节点上的 Service 转发规则可能不完整，导致部分连接失败"
    mitigation: "逐节点删除，等待新 Pod Ready 后再操作下一节点"
```

### 通用验证步骤

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 确认 Service 有 Endpoints
kubectl get endpoints <svc> -n <ns>

# 2. 确认后端 Pod 就绪
kubectl get pods -n <ns> -l <selector>

# 3. 从测试 Pod 访问 Service
kubectl exec <test-pod> -n <test-ns> -- curl -s -o /dev/null -w "%{http_code}" --connect-timeout 5 http://<svc>.<ns>.svc.cluster.local:<port>/

# 4. 直接访问后端 Pod IP 排除 Service 层问题
kubectl exec <test-pod> -n <test-ns> -- curl -s -o /dev/null -w "%{http_code}" --connect-timeout 5 http://<pod-ip>:<container-port>/
```
## Related

- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]

```

<!-- risk-assessed -->
