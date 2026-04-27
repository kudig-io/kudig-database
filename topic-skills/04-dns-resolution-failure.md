---
skill_id: "SKILL-NET-001"
skill_name: "DNS 解析故障诊断与修复 / DNS Resolution Failure Diagnosis & Remediation"
version: "1.0"
category: "network"
severity_range: "P0-P2"
k8s_versions:
  - "1.28.x"
  - "1.29.x"
  - "1.30.x"
  - "1.31.x"
  - "1.32.x"
tested_on:
  - "1.28.15"
  - "1.29.12"
  - "1.30.8"
  - "1.31.4"
  - "1.32.0"
k8s_version_notes:
  - "v1.28+: CoreDNS 1.10+ default, NodeLocal DNSCache stable"
  - "v1.29+: PodDisruptionConditions GA"
  - "v1.30+: CoreDNS 1.11+ with updated plugin config"
  - "v1.31+: AdminNetworkPolicy / BaselineAdminNetworkPolicy (alpha)"
  - "v1.32+: nftables kube-proxy mode (GA)"
last_updated: "2026-04-26"
estimated_resolution_time: "5-20min"
risk_level: "medium"
agent_execution_mode: "L2-semi-auto"
trigger_keywords:
  - "DNS"
  - "NXDOMAIN"
  - "dns resolution"
  - "DNS解析失败"
  - "域名解析"
  - "CoreDNS"
  - "name resolution"
  - "DNS超时"
  - "DNS timeout"
  - "could not resolve"
  - "no such host"
trigger_events:
  - "DNSConfigForming"
  - "NetworkNotReady"
trigger_metrics:
  - 'coredns_dns_responses_total{rcode="SERVFAIL"}'
  - 'coredns_dns_responses_total{rcode="NXDOMAIN"}'
  - 'coredns_dns_request_duration_seconds'
  - 'coredns_panics_total'
related_skills:
  - "SKILL-NET-002"
  - "SKILL-NODE-001"
fta_refs:
  - "topic-fta/list/dns-fta.md"
knowledge_refs:
  - "topic-structural-trouble-shooting/"
  - "domain-5-networking/"
  - "domain-12-troubleshooting/"
---

# DNS 解析故障诊断与修复 / DNS Resolution Failure Diagnosis & Remediation

---

## 1. 概述

DNS 是 Kubernetes 集群中**所有服务发现的基石**。集群内部的 Service 访问（`<service>.<namespace>.svc.cluster.local`）、Headless Service 的 Pod 发现、以及 Pod 对外部域名的访问，全部依赖 DNS 解析。当 DNS 出现故障时，影响呈**级联放大**——几乎所有依赖网络通信的应用组件都将失败，表现为大面积的连接超时、服务不可达和应用报错。DNS 故障的隐蔽性在于：应用层面的错误信息千变万化（HTTP 502/503、connection refused、timeout），但根因往往指向同一个问题——DNS 解析失败。

自 Kubernetes 1.12 起，**CoreDNS** 取代 kube-dns 成为默认的集群 DNS 提供者。CoreDNS 以 Deployment 方式部署在 `kube-system` namespace 中，通过 `kube-dns` Service（ClusterIP）对外提供 DNS 服务。每个 Pod 的 `/etc/resolv.conf` 中的 `nameserver` 指向该 ClusterIP，所有 DNS 查询通过该地址路由到 CoreDNS Pod。

### 典型触发场景

1. **CoreDNS 异常**: CoreDNS Pod 崩溃（CrashLoopBackOff）、资源不足（OOMKilled / CPU throttling）、配置错误（Corefile 语法错误），导致 DNS 服务完全不可用或响应超慢
2. **网络策略阻断**: NetworkPolicy 意外阻断了 Pod 到 kube-dns Service（UDP/TCP 53 端口）的流量，导致 DNS 查询被丢弃
3. **外部 DNS 不可达**: CoreDNS 的 upstream DNS 服务器（通常是节点的 `/etc/resolv.conf` 中配置的 DNS）不可达或响应超慢，导致外部域名解析失败
4. **ndots 配置问题**: 默认 `ndots=5` 导致对外部域名（如 `api.example.com`）的查询先经过 5 次无效的搜索域扩展，产生大量不必要的 DNS 查询，造成严重延迟
5. **conntrack 竞态条件**: Linux 内核 conntrack 表在 UDP DNS 查询时存在已知的竞态条件（race condition），导致间歇性 DNS 解析失败——这是一个著名的 Linux 内核问题

### 覆盖范围

- 集群内部 DNS 解析（Service → ClusterIP、Headless Service → Pod IP）
- 外部域名解析（Pod → 外部域名，如 `google.com`、API endpoints）
- NodeLocal DNSCache 相关故障
- Headless Service DNS 记录异常
- DNS 延迟与性能问题

### 前置条件

- **RBAC 权限**: 至少需要对 pods（kube-system namespace）、services、endpoints、configmaps 的 get/list 权限；exec 权限用于在 Pod 内执行 DNS 测试
- **工具要求**: kubectl (v1.28+), 集群内需有可执行 `nslookup`/`dig` 的 Pod（或可临时部署 dnsutils Pod）
- **SSH 访问**: 部分深度诊断（conntrack 检查、NodeLocal DNSCache 排查）需要节点 SSH 访问
- **监控系统**: Prometheus + CoreDNS metrics（用于 trigger_metrics 匹配和性能分析）

> ⚠️ **重要**: DNS 故障的爆炸半径极大。集群级 DNS 完全不可用属于 P0 事件，需立即响应。诊断过程中应首先判断是全局性还是局部性 DNS 故障，以快速定位根因范围。

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | 应用日志中出现 "could not resolve host" / "NXDOMAIN" / "no such host" / Application logs show DNS resolution failure errors | 检查应用 Pod 日志 `kubectl logs <pod>` 搜索 `resolve`、`NXDOMAIN`、`no such host`、`DNS` 等关键词 | 0.95 | 域名本身拼写错误或不存在（如 typo）；应用硬编码了错误的域名 |
| S2 | 在 Pod 内执行 `nslookup` 或 `dig` 返回 SERVFAIL / `nslookup` or `dig` inside pod returns SERVFAIL | `kubectl exec <pod> -- nslookup kubernetes.default.svc.cluster.local` 返回 `** server can't find ... SERVFAIL` | 0.95 | 目标域名确实不存在（合法的 NXDOMAIN）；Pod 使用了自定义 dnsPolicy 指向了非 CoreDNS 服务器 |
| S3 | CoreDNS Pod 处于 CrashLoopBackOff 状态 / CoreDNS pods in CrashLoopBackOff | `kubectl get pods -n kube-system -l k8s-app=kube-dns` 显示 CrashLoopBackOff | 0.90 | CoreDNS 正在滚动更新过程中短暂出现的重启；新部署的 CoreDNS 配置正在初始化 |
| S4 | DNS 解析延迟严重（>5 秒）/ DNS resolution is abnormally slow (>5s latency) | `kubectl exec <pod> -- time nslookup google.com` 返回超过 5 秒；Prometheus 指标 `coredns_dns_request_duration_seconds` P99 > 5s | 0.80 | 目标域名的权威 DNS 服务器本身响应慢（非集群问题）；Pod 到 CoreDNS 的网络路径拥塞（CNI 问题，应使用 SKILL-NET-002） |
| S5 | 外部域名解析正常但集群内部 Service 解析失败 / External domains resolve but internal services don't | `kubectl exec <pod> -- nslookup google.com` 成功，但 `kubectl exec <pod> -- nslookup <service>.<namespace>.svc.cluster.local` 失败 | 0.85 | Service 确实不存在或被删除；Service 所在的 namespace 拼写错误；Pod 的 search domain 配置异常 |
| S6 | 集群内部 Service 解析正常但外部域名解析失败 / Internal services resolve but external domains don't | `kubectl exec <pod> -- nslookup kubernetes.default` 成功，但 `kubectl exec <pod> -- nslookup google.com` 失败或超时 | 0.80 | Pod 的 dnsPolicy 设置为 `ClusterFirst`，但集群本身就是隔离网络（air-gapped），无法访问外部 DNS——这是正常行为 |
| S7 | DNS 仅在部分 Pod 中失败，其他 Pod 正常 / DNS works on some pods but not others | 同一 Service 的不同 Pod 表现不一致；跨 namespace 的 Pod DNS 行为不一致 | 0.75 | 不同 Pod 使用了不同的 dnsPolicy（如某些 Pod 使用 `Default` 而非 `ClusterFirst`）；NetworkPolicy 仅影响特定 namespace/label 的 Pod |
| S8 | kube-dns Service 没有可用 Endpoints / kube-dns Service has no endpoints | `kubectl get endpoints kube-dns -n kube-system` 显示 `<none>` 或 endpoints 列表为空 | 0.90 | kube-dns Service 的 selector 被误修改，与 CoreDNS Pod 的 label 不匹配（这本身就是根因之一） |
| S9 | DNS 间歇性失败（时好时坏）/ Intermittent DNS failures (race condition / conntrack) | 应用偶尔报 DNS 超时，但重试后成功；DNS 失败没有明确的规律 | 0.70 | 网络链路不稳定（丢包导致的偶尔超时，不特定于 DNS）；应用自身的 DNS 缓存过期导致的周期性解析延迟 |
| S10 | Prometheus 告警 CoreDNS SERVFAIL 率升高 / Prometheus alert: CoreDNS SERVFAIL rate increased | `coredns_dns_responses_total{rcode="SERVFAIL"}` 速率突增；CoreDNS 相关告警触发 | 0.85 | SERVFAIL 仅针对特定外部域名（上游 DNS 问题，非 CoreDNS 自身故障）；短暂的 SERVFAIL 尖刺后自行恢复 |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "应用报错无法解析域名，DNS 解析失败"
- "Pod 内无法访问其他服务，DNS 不通"
- "CoreDNS 挂了，集群 DNS 不工作"
- "域名解析超时，服务调用很慢"
- "nslookup 返回 SERVFAIL"
- "外部域名解析不了，内部服务正常"
- "DNS 间歇性失败，应用偶尔报错"
- "kube-dns 服务没有 endpoints"
- "所有服务都报连接超时，怀疑是 DNS 问题"
- "新建的 Service 域名解析不到"

**English ticket descriptions**:
- "DNS resolution failing inside pods, applications can't reach services"
- "CoreDNS pods are crashing, cluster DNS is down"
- "NXDOMAIN errors for internal service names"
- "DNS timeout when resolving external domains"
- "Intermittent DNS failures causing random service disruptions"
- "nslookup returns SERVFAIL for all queries"
- "kube-dns service has no endpoints"
- "DNS is slow, queries taking more than 5 seconds"
- "Could not resolve host errors in application logs"
- "Services unreachable, suspected DNS issue"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| DNS 解析正常，但 HTTP 请求返回 502/503 | SKILL-NET-002 | Service 网络问题（kube-proxy、Endpoints、后端 Pod 不健康），非 DNS 故障 |
| DNS 解析正常，但 Pod 无法连接到 Service 的 ClusterIP | SKILL-NET-002 | iptables/ipvs 规则问题或 Pod 网络路由异常，不涉及 DNS |
| 仅特定 HTTP 端点访问失败，但 DNS 解析该域名可以返回 IP | 不适用本 Skill | 应用层或负载均衡器问题，DNS 工作正常 |
| Pod 处于 Pending 状态且尚未运行 | SKILL-POD-002 | Pod 调度问题，DNS 测试无法在未运行的 Pod 中执行 |
| 节点处于 NotReady 状态 | SKILL-NODE-001 | 节点级别故障可能间接影响 DNS，但根因在节点层面 |
| 自定义 DNS 服务器（非 CoreDNS）的故障 | 手动处理 | 本 Skill 仅覆盖 CoreDNS 作为集群 DNS 提供者的场景 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断故障爆炸半径：

**Step T1**: 快速验证集群 DNS 是否完全不可用
```bash
# 使用 kube-system 中的任一 Pod 测试 DNS（kube-system 中总有可用的 Pod）
# 测试集群内部 DNS（kubernetes.default 是始终存在的 Service）
kubectl exec -n kube-system deploy/coredns -- nslookup kubernetes.default.svc.cluster.local 2>/dev/null || \
  kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes.default.svc.cluster.local
```
> **判断规则**:
> - 返回正确的 ClusterIP → 集群 DNS 基本可用，问题可能是局部性的，继续 T2
> - 返回 SERVFAIL / 超时 / 连接被拒 → 集群 DNS 可能全局不可用，**初步定级 P0**，继续 T2 确认
> - `kubectl exec` 本身超时 → 可能是 apiserver 或节点级问题，参考 SKILL-NODE-001

**Step T2**: 检查 CoreDNS Pod 状态
```bash
# 获取 CoreDNS Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
# 检查 kube-dns Service 和 Endpoints
kubectl get svc kube-dns -n kube-system
kubectl get endpoints kube-dns -n kube-system
```
> **判断规则**:
> - 所有 CoreDNS Pod 处于 CrashLoopBackOff / Error / 未运行 → CoreDNS 完全不可用，**确认 P0**
> - 部分 CoreDNS Pod 不健康但有健康实例 → DNS 服务降级但未完全中断，**P1**
> - 所有 CoreDNS Pod Running 且 Ready → CoreDNS 本身运行正常，问题在其他层面（配置、网络），继续 T3
> - Endpoints 为空 → kube-dns Service 没有后端，等同于 DNS 不可用，**确认 P0**

**Step T3**: 判断故障范围：集群级 / namespace 级 / Pod 级
```bash
# 从不同 namespace 的 Pod 中测试 DNS
# 测试 1: 从 default namespace
kubectl run dns-test-default --image=busybox:1.36 --rm -it --restart=Never -n default -- nslookup kubernetes.default.svc.cluster.local

# 测试 2: 从 kube-system namespace
kubectl exec -n kube-system $(kubectl get pods -n kube-system -l k8s-app=kube-proxy -o jsonpath='{.items[0].metadata.name}') -- nslookup kubernetes.default.svc.cluster.local 2>/dev/null

# 测试 3: 同时测试内部和外部 DNS
kubectl run dns-test-ext --image=busybox:1.36 --rm -it --restart=Never -- sh -c "nslookup kubernetes.default.svc.cluster.local && nslookup google.com"
```
> **判断规则**:
> - 所有 namespace 的 Pod 都无法解析 → 集群级故障，**P0**
> - 仅特定 namespace 的 Pod 无法解析 → namespace 级故障（可能是 NetworkPolicy），**P1-P2**
> - 仅特定 Pod 无法解析 → Pod 级配置问题，**P2**
> - 内部解析失败、外部正常 → CoreDNS zone 数据问题，**P1**
> - 外部解析失败、内部正常 → upstream DNS 问题，**P1-P2**

**Step T4**: 评估业务影响
```bash
# 检查最近的 DNS 相关告警和事件
kubectl get events -A --sort-by=.lastTimestamp | grep -i "dns\|resolve\|coredns" | tail -20
# 检查是否有大量 Pod 报错
kubectl get pods -A --field-selector status.phase!=Running,status.phase!=Succeeded | head -20
```
> **判断规则**:
> - 大量跨 namespace 的 Pod 异常 + DNS 故障确认 → 高业务影响
> - 仅少量 Pod 受影响 → 低业务影响

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| CoreDNS 完全不可用（所有 Pod 异常）**或** 集群级 DNS 解析全部失败 | **P0** | 集群级 DNS 故障，几乎所有服务间通信和外部访问中断。DNS 是最基础的基础设施，故障影响面极广 | 立即响应，10min 内确认根因并开始修复 |
| 多个 namespace 的 DNS 受影响 **或** 部分 CoreDNS Pod 异常导致 DNS 间歇性失败 | **P1** | DNS 服务降级，部分请求成功、部分失败，业务体验严重受损 | 15min 内响应，30min 内修复 |
| 仅特定 namespace 或特定域名的 DNS 解析失败 **或** DNS 延迟升高但未完全中断 | **P2** | 局部 DNS 问题，影响范围有限。可能是 NetworkPolicy、ndots 配置或特定 Service 的 DNS 记录问题 | 30min 内响应，2h 内修复 |
| 偶发性 DNS 超时，业务层面有重试机制可容忍 **或** DNS 延迟轻微升高 | **P3** | 轻微 DNS 性能问题，不影响核心业务功能 | 4h 内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工 SRE / 值班工程师**：

- **CoreDNS 完全宕机**: 所有 CoreDNS Pod 异常且无法自动恢复超过 5 分钟
- **kube-dns Service 被删除或修改**: `kubectl get svc kube-dns -n kube-system` 不存在或 ClusterIP 被更改
- **集群级 DNS 全面中断**: 从任意 Pod 均无法解析 `kubernetes.default.svc.cluster.local`
- **CoreDNS ConfigMap 被误删**: `kubectl get configmap coredns -n kube-system` 不存在
- **DNS 故障导致级联失败**: 因 DNS 不可用导致其他关键组件（如 Ingress Controller、外部 Secret 同步）也出现故障
- **安全事件疑虑**: CoreDNS ConfigMap 被未授权修改，怀疑 DNS 劫持或投毒

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 通过 kubectl 远程收集 DNS 服务和配置信息，快速定位故障层面。所有命令均为只读操作。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 检查 CoreDNS Pod 状态和就绪情况
- **命令**:
  ```bash
  kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
  ```
- **超时**: 10s
- **预期输出模式**: Pod 列表，包含 NAME, READY, STATUS, RESTARTS, AGE, IP, NODE 等列
- **判断规则**:
  - 所有 Pod 状态为 `Running` 且 READY 为 `1/1` → CoreDNS Pod 运行正常，继续 D1.2 检查 Service 层
  - 有 Pod 状态为 `CrashLoopBackOff` → 根因为 RC-001（CoreDNS Pod 异常），跳转 D2.1 查看日志
  - 有 Pod 状态为 `OOMKilled` → 根因为 RC-006（CoreDNS 资源不足），跳转 D2.4 检查资源
  - READY 显示 `0/1`（运行但未就绪）→ CoreDNS 启动但 readiness probe 失败，跳转 D2.1 查看日志
  - Pod 数量为 0 → CoreDNS Deployment 被缩容或删除，紧急情况
  - RESTARTS 数值很高 → CoreDNS 频繁重启（RC-001），跳转 D2.1
- **版本差异**: 无

**Step D1.2**: 检查 kube-dns Service 和 Endpoints
- **命令**:
  ```bash
  # 检查 kube-dns Service
  kubectl get svc kube-dns -n kube-system -o wide
  # 检查 kube-dns Endpoints
  kubectl get endpoints kube-dns -n kube-system
  ```
- **超时**: 10s
- **预期输出模式**: Service 信息（ClusterIP、端口）和 Endpoints 列表（CoreDNS Pod IP:Port）
- **判断规则**:
  - Service 存在且 ClusterIP 正确，Endpoints 包含 CoreDNS Pod IP → Service 层正常，继续 D1.3
  - Service 存在但 Endpoints 为 `<none>` → 根因为 RC-002（kube-dns 无 Endpoints），检查 Service selector 是否匹配 CoreDNS Pod labels
  - Service 不存在 → kube-dns Service 被删除，**立即升级**（参见 3.3）
  - Service 的 ClusterIP 与 Pod 中 `/etc/resolv.conf` 的 nameserver 不匹配 → 配置不一致，可能是手动修改导致
  - Endpoints 中的 IP 与 CoreDNS Pod IP 不一致 → endpoint controller 异常
- **版本差异**: 无

**Step D1.3**: 在受影响 Pod 内执行 DNS 测试
- **命令**:
  ```bash
  # 测试 1: 集群内部 DNS（kubernetes.default 是始终存在的 Service）
  kubectl exec <pod> -- nslookup kubernetes.default.svc.cluster.local

  # 测试 2: 跨 namespace 的 Service DNS
  kubectl exec <pod> -- nslookup <target-service>.<namespace>.svc.cluster.local

  # 测试 3: 外部域名 DNS
  kubectl exec <pod> -- nslookup google.com

  # 测试 4: 直接指定 CoreDNS ClusterIP 测试（绕过 resolv.conf 配置）
  kubectl exec <pod> -- nslookup kubernetes.default.svc.cluster.local <kube-dns-clusterip>
  ```
- **超时**: 30s（DNS 查询可能超时）
- **预期输出模式**: nslookup 返回正确的 IP 地址
- **判断规则**:
  - 测试 1 失败 → CoreDNS 本身有问题或不可达（RC-001/RC-002/RC-004）
  - 测试 1 成功但测试 2 失败 → 特定 Service 不存在或 namespace 错误（非 DNS 系统问题）
  - 测试 1 成功但测试 3 失败 → 外部 DNS 解析问题（RC-003/RC-009），或 ndots 配置问题（RC-005）
  - 测试 4 成功但测试 1 失败 → Pod 的 resolv.conf 配置错误（RC-012）
  - 所有测试成功但应用仍报错 → 可能是应用自身的 DNS 缓存、连接池或特定域名问题
- **版本差异**: 无

**Step D1.4**: 检查受影响 Pod 的 DNS 配置
- **命令**:
  ```bash
  # 查看 Pod 的 resolv.conf
  kubectl exec <pod> -- cat /etc/resolv.conf
  ```
- **超时**: 5s
- **预期输出模式**:
  ```
  nameserver 10.96.0.10
  search <namespace>.svc.cluster.local svc.cluster.local cluster.local
  options ndots:5
  ```
- **判断规则**:
  - `nameserver` 指向 kube-dns ClusterIP（通常 `10.96.0.10`）→ 配置正确
  - `nameserver` 指向其他 IP → Pod 使用了自定义 dnsPolicy（可能是 `Default` 或 `None`），检查 Pod spec 中的 `dnsPolicy` 字段（RC-012）
  - `search` 域缺少 `svc.cluster.local` → 搜索域配置异常（RC-012）
  - `ndots:5`（默认值）→ 记录，可能导致外部域名解析慢（RC-005），继续 D2.5 深入分析
  - `resolv.conf` 为空或内容异常 → kubelet DNS 配置注入失败（RC-012）
- **版本差异**: 无

**Step D1.5**: 检查相关 Kubernetes Events
- **命令**:
  ```bash
  # 检查 kube-system namespace 中与 CoreDNS 相关的事件
  kubectl get events -n kube-system --sort-by=.lastTimestamp | grep -i "coredns\|dns\|kube-dns" | tail -20
  # 检查受影响 Pod 所在 namespace 的网络相关事件
  kubectl get events -n <namespace> --sort-by=.lastTimestamp | grep -i "dns\|network\|resolve" | tail -20
  ```
- **超时**: 10s
- **预期输出模式**: Event 列表
- **判断规则**:
  - 出现 `DNSConfigForming` 事件 → DNS 配置正在生成，可能是暂时性的
  - 出现 `BackOff` + CoreDNS Pod → CoreDNS 持续崩溃（RC-001）
  - 出现 `OOMKilled` → CoreDNS 内存不足（RC-006）
  - 出现 `FailedCreatePodSandBox` + DNS 相关 → CNI 或网络层问题
  - 出现 `NetworkNotReady` → 节点网络未就绪，可能间接影响 DNS
- **版本差异**: 无

---

**⚙️ 决策分支点（Decision Branch）**:

基于 Phase 1 的检查结果，选择诊断方向：

| D1.3 结果 | 初步判断 | 下一步 |
|-----------|---------|--------|
| 内部 DNS 失败 + 外部 DNS 失败 | CoreDNS 本身不可用或不可达 | Phase 2: D2.1 → D2.2 → D2.3 → D2.4 |
| 内部 DNS 失败 + 外部 DNS 正常 | CoreDNS zone 数据问题或 Service 注册问题 | Phase 2: D2.2（Corefile）→ D2.9（Headless Service） |
| 内部 DNS 正常 + 外部 DNS 失败 | 上游 DNS 不可达或 ndots 配置问题 | Phase 2: D2.2（upstream config）→ D2.5（ndots）→ D2.10 |
| 部分 Pod 失败、部分正常 | NetworkPolicy 或 Pod 级 DNS 配置问题 | Phase 2: D2.3（NetworkPolicy）→ 回顾 D1.4（dnsPolicy） |
| DNS 间歇性失败 | conntrack 竞态或 CoreDNS 负载问题 | Phase 2: D2.4（资源）→ D2.7（conntrack）→ D2.6（NodeLocal DNS） |

---

### Phase 2: 深度检查（只读，零风险）

> **目标**: 深入检查 CoreDNS 配置、日志、资源使用以及网络策略等，定位具体根因。所有命令均为只读操作。
> **预计耗时**: 5-10 分钟

**Step D2.1**: 检查 CoreDNS 日志
- **命令**:
  ```bash
  # 查看所有 CoreDNS Pod 的最近日志
  kubectl logs -n kube-system -l k8s-app=kube-dns --tail=200 --timestamps
  # 如果有多个 CoreDNS Pod，逐个查看以发现差异
  kubectl logs -n kube-system <coredns-pod-1> --tail=100
  kubectl logs -n kube-system <coredns-pod-2> --tail=100
  ```
- **超时**: 15s
- **预期输出模式**: CoreDNS 日志条目
- **判断规则**:
  - 日志包含 `SERVFAIL` → CoreDNS 无法解析请求，需进一步确认是内部 zone 还是外部 forward 导致
  - 日志包含 `REFUSED` → CoreDNS 拒绝了 DNS 查询，可能是配置不当
  - 日志包含 `plugin/loop: Loop ... detected` → 根因为 RC-011（DNS 循环检测），CoreDNS 检测到自身在解析循环中
  - 日志包含 `i/o timeout` 或 `dial tcp ... i/o timeout` → 上游 DNS 服务器不可达（RC-009）
  - 日志包含 `plugin/errors` + 具体域名 → 特定域名解析问题
  - 日志包含 `Corefile parse error` 或 `Failed to start server` → Corefile 配置语法错误（RC-003）
  - 日志包含 `OOM` 或被 `killed` → 资源不足（RC-006）
  - 日志包含 `connection refused` → 上游 DNS 或自身端口绑定问题
  - 日志无异常但 DNS 仍失败 → 问题可能在网络层（NetworkPolicy / conntrack），继续 D2.3/D2.7
- **版本差异**:
  - **[v1.28+]**: CoreDNS 1.10.1+ 默认启用了改进的日志格式，包含更详细的查询信息
  - **[v1.30+]**: CoreDNS 1.11.x 新增 `dns64` 插件日志，双栈集群可能看到额外的日志条目

**Step D2.2**: 检查 CoreDNS ConfigMap（Corefile 配置）
- **命令**:
  ```bash
  kubectl get configmap coredns -n kube-system -o yaml
  ```
- **超时**: 10s
- **预期输出模式**: Corefile 内容，通常结构如下：
  ```
  .:53 {
      errors
      health {
          lameduck 5s
      }
      ready
      kubernetes cluster.local in-addr.arpa ip6.arpa {
          pods insecure
          fallthrough in-addr.arpa ip6.arpa
          ttl 30
      }
      prometheus :9153
      forward . /etc/resolv.conf {
          max_concurrent 1000
      }
      cache 30
      loop
      reload
      loadbalance
  }
  ```
- **判断规则**:
  - ConfigMap 不存在 → **立即升级**，CoreDNS 配置丢失
  - `forward` 指令指向 `/etc/resolv.conf` → 上游 DNS 取自 CoreDNS Pod 所在节点的 resolv.conf，检查节点 DNS 配置
  - `forward` 指令指向特定 IP（如 `8.8.8.8`）→ 确认该 IP 是否可达（RC-009）
  - `kubernetes cluster.local` zone 配置缺失或错误 → 根因为 RC-003（Corefile 配置错误）
  - `loop` 插件被移除 → 存在 DNS 循环风险（RC-011 的潜在原因）
  - `forward` 的 `max_concurrent` 设置过低 → 高负载下可能限流导致超时
  - 存在自定义 zone 配置（如 `consul` 或自定义 forward）→ 检查自定义配置的正确性
  - `cache` TTL 设置过低或过高 → 可能影响性能或数据新鲜度
- **版本差异**:
  - **[v1.29+]**: 默认 Corefile 中可能包含 `dns64` 配置（双栈支持改进）
  - **[v1.30+]**: `forward` 插件新增 `prefer_udp` 选项，影响上游 DNS 查询协议选择

**Step D2.3**: 检查 NetworkPolicy 是否阻断 DNS 流量
- **命令**:
  ```bash
  # 查看所有 namespace 的 NetworkPolicy
  kubectl get networkpolicy -A

  # 查看受影响 namespace 的 NetworkPolicy 详情
  kubectl get networkpolicy -n <affected-namespace> -o yaml

  # 查看 kube-system namespace 的 NetworkPolicy（可能阻断对 CoreDNS 的入站流量）
  kubectl get networkpolicy -n kube-system -o yaml
  ```
- **超时**: 10s
- **预期输出模式**: NetworkPolicy 列表和详情
- **判断规则**:
  - 无 NetworkPolicy → 排除 NetworkPolicy 原因，继续其他诊断
  - 受影响 namespace 存在 egress NetworkPolicy 但未放行 UDP/TCP 53 端口到 kube-system → 根因为 RC-004（NetworkPolicy 阻断 DNS）
  - kube-system 存在 ingress NetworkPolicy 限制了来源 namespace → RC-004
  - NetworkPolicy 使用了 `podSelector` 或 `namespaceSelector` 但规则不匹配 CoreDNS → RC-004
  - 特别检查: 是否有 `deny-all` 类型的默认策略，且未为 DNS 创建例外规则
- **版本差异**:
  - **[v1.29+]**: NetworkPolicy 的 status 字段（beta）可帮助确认策略是否生效
  - **[v1.30+]**: AdminNetworkPolicy (alpha) 可能在更高层面影响 DNS 流量

**Step D2.4**: 检查 CoreDNS 资源使用情况
- **命令**:
  ```bash
  # 检查 CoreDNS Pod 的 CPU 和内存使用
  kubectl top pods -n kube-system -l k8s-app=kube-dns

  # 检查 CoreDNS Deployment 的资源限制配置
  kubectl get deployment coredns -n kube-system -o jsonpath='{.spec.template.spec.containers[0].resources}'
  ```
- **超时**: 10s
- **预期输出模式**: CPU 和内存使用数据，以及资源 limits/requests 配置
- **判断规则**:
  - CPU 使用接近 limits → CoreDNS 被 CPU 限流（throttled），DNS 响应变慢（RC-006）
  - 内存使用接近 limits → CoreDNS 面临 OOMKill 风险（RC-006）
  - 未设置 limits → CoreDNS 可能在资源竞争中被影响
  - 资源使用正常（CPU < 50% limits, Memory < 70% limits）→ 排除资源原因
  - 参考基准: 每 1000 个 Service 约需 100Mi 内存；高 QPS（>5000 qps）需增加 CPU
- **版本差异**: 无

**Step D2.5**: 检查 ndots 配置及其对外部域名解析的影响
- **命令**:
  ```bash
  # 查看 Pod 的 resolv.conf 中的 ndots 设置
  kubectl exec <pod> -- cat /etc/resolv.conf | grep ndots

  # 使用 dig 观察实际 DNS 查询序列（如果 Pod 有 dig 工具）
  kubectl exec <pod> -- dig +search +showsearch google.com

  # 或使用 nslookup verbose 模式
  kubectl exec <pod> -- nslookup -debug google.com 2>&1 | head -50
  ```
- **超时**: 15s
- **预期输出模式**: ndots 值和 DNS 查询扩展序列
- **判断规则**:
  - `ndots:5`（默认值）→ 对于外部域名如 `api.example.com`（包含 2 个 `.`，小于 ndots=5），DNS 客户端会先尝试以下查询:
    1. `api.example.com.<namespace>.svc.cluster.local` → NXDOMAIN
    2. `api.example.com.svc.cluster.local` → NXDOMAIN
    3. `api.example.com.cluster.local` → NXDOMAIN
    4. `api.example.com.` → 最终才做绝对查询
    这意味着每次外部域名查询会产生 4 个无效查询，导致延迟和 CoreDNS 负载增加（RC-005）
  - `ndots:5` 且应用大量访问外部域名 → 这是外部 DNS 延迟的常见原因（RC-005）
  - `ndots:2` 或更低 → 已优化，不太可能是 ndots 问题
  - 使用 FQDN 带尾部点（如 `google.com.`）→ 绕过了 ndots 搜索域扩展
- **版本差异**: 无

**Step D2.6**: 检查 NodeLocal DNSCache（如已部署）
- **命令**:
  ```bash
  # 检查是否部署了 NodeLocal DNSCache DaemonSet
  kubectl get ds -n kube-system | grep -i "node-local\|nodelocaldns"

  # 检查 NodeLocal DNS Pod 状态
  kubectl get pods -n kube-system -l k8s-app=node-local-dns -o wide

  # 如果已部署，检查 NodeLocal DNS 的配置
  kubectl get configmap node-local-dns -n kube-system -o yaml 2>/dev/null
  ```
- **超时**: 10s
- **预期输出模式**: DaemonSet 和 Pod 状态
- **判断规则**:
  - 未部署 NodeLocal DNSCache → 不影响诊断，DNS 查询直接到 CoreDNS
  - NodeLocal DNS Pod 在某些节点上不健康 → 这些节点上的 Pod DNS 查询会 fallback 到 CoreDNS，但 fallback 机制可能存在超时（RC-007）
  - NodeLocal DNS Pod 全部健康 → NodeLocal DNS 工作正常
  - NodeLocal DNS 配置中的 upstream（`__PILLAR__CLUSTER__DNS__`）未正确替换 → 配置问题（RC-007）
  - Pod 的 resolv.conf 中 nameserver 指向 `169.254.20.10`（NodeLocal DNS 链路本地地址）但 NodeLocal DNS Pod 不存在 → DNS 查询将全部失败
- **版本差异**:
  - **[v1.28+]**: NodeLocal DNSCache 支持 IPv6
  - **[v1.30+]**: 改进的 NodeLocal DNS 健康检查机制

**Step D2.7**: 检查 conntrack 表状态（针对间歇性 DNS 失败）
- **命令**:
  ```bash
  # 需要 SSH 到受影响的节点
  # 检查 conntrack 表使用情况
  ssh <node-ip> "cat /proc/sys/net/nf_conntrack_max"
  ssh <node-ip> "cat /proc/sys/net/netfilter/nf_conntrack_count"

  # 检查 conntrack 表是否有溢出记录
  ssh <node-ip> "dmesg | grep -i 'nf_conntrack: table full' | tail -5"

  # 检查 UDP DNS 相关的 conntrack 条目
  ssh <node-ip> "conntrack -L -p udp --dport 53 2>/dev/null | head -20"
  ```
- **超时**: 10s
- **预期输出模式**: conntrack 表最大值、当前使用数、溢出日志
- **判断规则**:
  - `nf_conntrack_count` 接近 `nf_conntrack_max`（>80%）→ conntrack 表即将或已经溢出（RC-008），DNS UDP 包可能被丢弃
  - dmesg 中出现 `nf_conntrack: table full, dropping packet` → 确认 conntrack 溢出正在发生（RC-008）
  - conntrack 表使用率正常（<50%）→ 排除 conntrack 原因
  - **关键背景**: Linux 内核在处理 DNAT（kube-proxy 的 iptables 模式）+ UDP 时存在已知竞态条件。当 Pod 同时发送 A 记录和 AAAA 记录查询（glibc 默认行为）到同一个 kube-dns ClusterIP 时，两个 UDP 包可能竞争同一个 conntrack 条目，导致其中一个查询被丢弃。这是 DNS 间歇性失败的一个著名根因。
- **版本差异**: 与 K8s 版本无关，取决于 Linux 内核版本（kernel 5.0+ 有缓解措施但未完全修复）

**Step D2.8**: 从节点层面测试 DNS（绕过 Pod 网络）
- **命令**:
  ```bash
  # SSH 到节点，直接使用 CoreDNS Pod IP 测试 DNS
  # 先获取 CoreDNS Pod IP
  kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{.items[*].status.podIP}'

  # 在节点上直接向 CoreDNS Pod IP 发送 DNS 查询
  ssh <node-ip> "dig @<coredns-pod-ip> kubernetes.default.svc.cluster.local +short"

  # 测试外部域名
  ssh <node-ip> "dig @<coredns-pod-ip> google.com +short"

  # 使用节点自身的 DNS（非 CoreDNS）测试外部域名
  ssh <node-ip> "dig @$(grep nameserver /etc/resolv.conf | head -1 | awk '{print $2}') google.com +short"
  ```
- **超时**: 15s
- **预期输出模式**: DNS 查询结果（IP 地址）
- **判断规则**:
  - 节点直接查 CoreDNS Pod IP 成功 → CoreDNS 工作正常，问题在 Pod → CoreDNS 的网络路径（kube-proxy 规则或 NetworkPolicy）
  - 节点直接查 CoreDNS Pod IP 失败 → CoreDNS 确实有问题或节点到 CoreDNS Pod 的网络不通
  - 节点自身 DNS 查外部域名失败 → 节点本身的 upstream DNS 不可达（RC-009）
  - 节点自身 DNS 查外部域名成功但通过 CoreDNS 查失败 → CoreDNS 的 forward 配置问题（RC-003）
- **版本差异**: 无

**Step D2.9**: 检查 Headless Service DNS 记录（如适用）
- **命令**:
  ```bash
  # 检查 Headless Service 配置
  kubectl get svc <service-name> -n <namespace> -o yaml | grep -A5 "clusterIP\|selector"

  # 测试 Headless Service DNS 解析
  kubectl exec <pod> -- nslookup <service>.<namespace>.svc.cluster.local

  # 检查 Headless Service 的 Endpoints
  kubectl get endpoints <service-name> -n <namespace>
  ```
- **超时**: 10s
- **预期输出模式**: Headless Service 应返回所有后端 Pod 的 IP 地址（非 ClusterIP）
- **判断规则**:
  - Headless Service（`clusterIP: None`）DNS 查询返回 Pod IP 列表 → 正常
  - Headless Service DNS 查询返回 NXDOMAIN → Endpoints 可能为空（Pod 未就绪或 selector 不匹配）（RC-010）
  - Headless Service DNS 返回的 IP 数量与 Ready Pod 数量不一致 → Endpoints 更新延迟（RC-010）
  - StatefulSet 的 Pod DNS（`<pod-name>.<service>.<namespace>.svc.cluster.local`）解析失败 → Headless Service 或 Pod hostname/subdomain 配置问题
- **版本差异**: 无

**Step D2.10**: 检查上游 DNS 服务器连通性
- **命令**:
  ```bash
  # 从 CoreDNS Pod 中检查上游 DNS
  # 先确认 CoreDNS 使用的上游 DNS
  kubectl exec -n kube-system <coredns-pod> -- cat /etc/resolv.conf

  # 从 CoreDNS Pod 测试上游 DNS 连通性
  kubectl exec -n kube-system <coredns-pod> -- nslookup google.com <upstream-dns-ip>

  # 或从节点层面测试
  ssh <node-ip> "dig @<upstream-dns-ip> google.com +short +time=5"
  ```
- **超时**: 15s
- **预期输出模式**: 上游 DNS 查询结果
- **判断规则**:
  - 上游 DNS 响应正常 → 排除上游 DNS 问题
  - 上游 DNS 超时 → 根因为 RC-009（上游 DNS 不可用），检查节点 DNS 配置和网络出口
  - 上游 DNS 返回 REFUSED → 上游 DNS 服务器拒绝查询（可能是 ACL 限制）
  - CoreDNS Pod 的 `/etc/resolv.conf` 指向了 kube-dns ClusterIP → **DNS 循环！** 根因为 RC-011
- **版本差异**: 无

---

### Phase 3: 主动探测（低风险，可能需审批）

> ⚠️ 以下步骤涉及创建临时资源或主动网络请求。在 L2-semi-auto 模式下，低风险操作可自动执行，中风险需人工确认。
> **预计耗时**: 3-5 分钟

**Step D3.1**: 部署 DNS 调试 Pod 进行系统化测试
- **命令**:
  ```bash
  # 部署 dnsutils Pod（包含 dig、nslookup、host 等工具）
  kubectl run dnsutils --image=registry.k8s.io/e2e-test-images/agnhost:2.39 --restart=Never --command -- sleep 3600

  # 等待 Pod 就绪
  kubectl wait --for=condition=Ready pod/dnsutils --timeout=60s

  # 系统化 DNS 测试
  # 1. 测试集群内部 DNS
  kubectl exec dnsutils -- dig kubernetes.default.svc.cluster.local +short +time=5 +tries=1
  # 2. 测试外部 DNS
  kubectl exec dnsutils -- dig google.com +short +time=5 +tries=1
  # 3. 测试反向 DNS
  kubectl exec dnsutils -- dig -x <kube-dns-clusterip> +short +time=5 +tries=1
  # 4. 测试 SRV 记录（Service port 发现）
  kubectl exec dnsutils -- dig _https._tcp.kubernetes.default.svc.cluster.local SRV +short +time=5
  ```
- **超时**: 90s（包括 Pod 启动时间）
- **风险级别**: 🟢 低（创建临时 Pod，完成后删除）
- **预期输出模式**: 各 DNS 查询返回正确的记录
- **判断规则**:
  - 所有查询成功 → DNS 系统工作正常，原始故障可能已恢复或为应用级问题
  - 内部查询成功但外部失败 → 确认 upstream DNS 问题（RC-003/RC-009）
  - 所有查询失败 → 确认集群 DNS 系统性故障
  - SRV 记录查询失败但 A 记录正常 → CoreDNS kubernetes 插件部分功能异常
- **清理**:
  ```bash
  kubectl delete pod dnsutils --force --grace-period=0
  ```
- **版本差异**: 无

**Step D3.2**: 使用 `dig +trace` 追踪 DNS 解析链
- **命令**:
  ```bash
  # 追踪外部域名解析的完整路径
  kubectl exec dnsutils -- dig +trace google.com

  # 追踪内部域名解析
  kubectl exec dnsutils -- dig +trace kubernetes.default.svc.cluster.local
  ```
- **超时**: 30s
- **风险级别**: 🟢 低（只读 DNS 查询）
- **预期输出模式**: DNS 解析链路，从根服务器到最终结果
- **判断规则**:
  - 追踪在某一层级中断 → 定位到具体的故障 DNS 服务器
  - 追踪显示查询被发送到错误的服务器 → forward 配置问题（RC-003）
  - 追踪显示循环（查询被发回 CoreDNS）→ DNS 循环（RC-011）
- **版本差异**: 无

**Step D3.3**: DNS 延迟性能测试
- **命令**:
  ```bash
  # 批量 DNS 查询，测量延迟
  kubectl exec dnsutils -- sh -c '
    for i in $(seq 1 20); do
      start=$(date +%s%N)
      dig kubernetes.default.svc.cluster.local +short +time=5 +tries=1 > /dev/null 2>&1
      end=$(date +%s%N)
      echo "Query $i: $(( (end - start) / 1000000 ))ms"
    done
  '

  # 外部域名延迟测试
  kubectl exec dnsutils -- sh -c '
    for i in $(seq 1 10); do
      start=$(date +%s%N)
      dig google.com +short +time=5 +tries=1 > /dev/null 2>&1
      end=$(date +%s%N)
      echo "External query $i: $(( (end - start) / 1000000 ))ms"
    done
  '
  ```
- **超时**: 120s
- **风险级别**: 🟢 低（只读 DNS 查询，轻量负载）
- **预期输出模式**: 每次查询的延迟毫秒数
- **判断规则**:
  - 内部查询延迟 < 10ms → 正常
  - 内部查询延迟 10-100ms → 轻微偏高，可能 CoreDNS 负载较高
  - 内部查询延迟 > 100ms → 异常，CoreDNS 性能问题（RC-006）
  - 外部查询延迟 < 100ms → 正常
  - 外部查询延迟 > 1000ms → 异常，upstream DNS 慢（RC-009）或 ndots 问题（RC-005）
  - 延迟分布不均（大部分快但偶尔很慢）→ 间歇性问题，可能是 conntrack（RC-008）
- **版本差异**: 无

---

### Phase 4: NodeLocal DNSCache 完整排查（如已部署）

> **目标**: 深入排查 NodeLocal DNSCache 的运行状态、上游连接和缓存行为，确认是否为 DNS 故障根因。
> **预计耗时**: 3-5 分钟
> **前置条件**: 已确认集群部署了 NodeLocal DNSCache（D2.6 显示 DaemonSet 存在）

**Step D4.1**: 检查 NodeLocal DNSCache DaemonSet 状态
- **命令**:
  ```bash
  # 获取 NodeLocal DNSCache DaemonSet 状态
  kubectl -n kube-system get ds node-local-dns -o wide
  
  # 检查各节点上的 NodeLocal DNS Pod 状态
  kubectl -n kube-system get pods -l k8s-app=node-local-dns -o wide --sort-by='{.spec.nodeName}'
  
  # 查看 DaemonSet 的滚动更新状态
  kubectl -n kube-system rollout status ds/node-local-dns
  ```
- **超时**: 10s
- **预期输出模式**: DaemonSet 显示 DESIRED = CURRENT = READY
- **判断规则**:
  - READY 数量等于 DESIRED → NodeLocal DNS 在所有目标节点上运行正常
  - READY 数量少于 DESIRED → 部分节点上的 NodeLocal DNS Pod 不健康，检查具体 Pod 状态和日志
  - READY = 0 → NodeLocal DNS 完全不可用，所有 DNS 查询将 fallback 到 CoreDNS（如果配置正确）
  - Pod 状态为 `CrashLoopBackOff` → 检查 Pod 日志确认崩溃原因
- **版本差异**:
  - **[v1.28+]**: NodeLocal DNSCache 支持 IPv6 和双栈配置
  - **[v1.30+]**: 改进的健康检查机制，支持更细粒度的就绪探针

**Step D4.2**: 检查本地 DNS 缓存是否生效
- **命令**:
  ```bash
  # 从 Pod 中测试 NodeLocal DNS 链路本地地址 169.254.20.10
  kubectl exec -it <pod> -- nslookup kubernetes.default 169.254.20.10
  
  # 测试外部域名解析
  kubectl exec -it <pod> -- nslookup google.com 169.254.20.10
  
  # 检查 Pod 的 resolv.conf 是否指向 NodeLocal DNS
  kubectl exec -it <pod> -- cat /etc/resolv.conf | head -3
  ```
- **超时**: 15s
- **预期输出模式**: nslookup 返回正确的 IP 地址；resolv.conf 中 nameserver 为 169.254.20.10
- **判断规则**:
  - 通过 169.254.20.10 解析成功 → NodeLocal DNS 工作正常
  - 通过 169.254.20.10 解析失败但直接访问 CoreDNS ClusterIP 成功 → NodeLocal DNS 本身有问题（RC-007）
  - resolv.conf 指向 169.254.20.10 但 NodeLocal DNS Pod 不存在 → DNS 将完全失败
  - resolv.conf 未指向 169.254.20.10 → kubelet 配置未更新，Pod 仍使用 CoreDNS
- **版本差异**: 无

**Step D4.3**: 检查 NodeLocal DNSCache 与 CoreDNS 的 upstream 连接
- **命令**:
  ```bash
  # 检查 NodeLocal DNS ConfigMap 中的 upstream 配置
  kubectl -n kube-system get cm node-local-dns -o yaml | grep -A 20 "Corefile"
  
  # 查看 NodeLocal DNS Pod 日志，关注 upstream 连接错误
  kubectl -n kube-system logs -l k8s-app=node-local-dns --tail=100 | grep -i "upstream\|forward\|error\|timeout"
  
  # 从 NodeLocal DNS Pod 中测试到 CoreDNS 的连通性
  NODE_LOCAL_POD=$(kubectl -n kube-system get pods -l k8s-app=node-local-dns -o jsonpath='{.items[0].metadata.name}')
  kubectl -n kube-system exec $NODE_LOCAL_POD -- nslookup kubernetes.default <kube-dns-clusterip>
  ```
- **超时**: 15s
- **预期输出模式**: Corefile 配置和日志输出
- **判断规则**:
  - 日志包含 `i/o timeout` 指向 CoreDNS IP → NodeLocal DNS 无法连接 CoreDNS（RC-007）
  - ConfigMap 中 `__PILLAR__CLUSTER__DNS__` 未被替换 → 部署配置错误
  - 日志包含 `no upstream` → upstream 配置缺失
  - 无错误日志且 upstream 测试成功 → NodeLocal DNS 到 CoreDNS 链路正常
- **版本差异**: 无

**Step D4.4**: 验证 iptables/ipvs 劫持规则（169.254.20.10 链路完整性）
- **命令**:
  ```bash
  # SSH 到节点检查 iptables 规则（iptables 模式）
  ssh <node-ip> "iptables-save | grep 169.254.20.10"
  
  # 检查 ipvs 规则（ipvs 模式）
  ssh <node-ip> "ipvsadm -ln | grep 169.254.20.10"
  
  # 验证链路本地地址是否在节点上存在
  ssh <node-ip> "ip addr show | grep 169.254.20.10"
  ```
- **超时**: 10s
- **预期输出模式**: iptables/ipvs 规则和 IP 地址配置
- **判断规则**:
  - 169.254.20.10 地址存在于节点 dummy 接口 → 链路本地地址配置正确
  - 169.254.20.10 地址不存在 → NodeLocal DNS 未正确设置链路本地地址（RC-007）
  - iptables/ipvs 规则将 169.254.20.10:53 流量正确路由 → 劫持规则正常
  - 无相关规则 → kube-proxy 或 NodeLocal DNS 部署异常
- **版本差异**:
  - **[v1.29+]**: nftables 模式下需使用 `nft list ruleset | grep 169.254.20.10` 检查规则
  - **[v1.32+]**: nftables 模式 GA，iptables 命令可能无法显示规则

**Step D4.5**: 检查缓存命中率与 TTL 配置优化
- **命令**:
  ```bash
  # 获取 NodeLocal DNS metrics（如果启用了 Prometheus metrics）
  NODE_LOCAL_POD=$(kubectl -n kube-system get pods -l k8s-app=node-local-dns -o jsonpath='{.items[0].metadata.name}')
  kubectl -n kube-system exec $NODE_LOCAL_POD -- wget -qO- http://localhost:9253/metrics 2>/dev/null | grep -E "coredns_cache_hits_total|coredns_cache_misses_total"
  
  # 检查 NodeLocal DNS 的 cache 配置
  kubectl -n kube-system get cm node-local-dns -o yaml | grep -A 5 "cache"
  ```
- **超时**: 10s
- **预期输出模式**: 缓存命中/未命中计数和 cache 配置
- **判断规则**:
  - cache_hits_total >> cache_misses_total → 缓存有效，减轻了 CoreDNS 压力
  - cache_hits_total ≈ 0 → 缓存未生效，检查 cache 配置
  - cache TTL 设置过低（<10s）→ 可能导致频繁缓存失效，增加 CoreDNS 负载
  - cache TTL 设置过高（>3600s）→ 可能导致 DNS 记录更新延迟
- **版本差异**: 无

---

### Phase 5: 自定义 DNS 策略排查

> **目标**: 排查 Pod 的 dnsPolicy 和 dnsConfig 配置，确认是否为自定义配置导致的 DNS 解析问题。
> **预计耗时**: 2-3 分钟

**Step D5.1**: 检查 Pod dnsPolicy 设置
- **命令**:
  ```bash
  # 获取受影响 Pod 的 dnsPolicy
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.dnsPolicy}'
  
  # 批量检查 namespace 中所有 Pod 的 dnsPolicy
  kubectl get pods -n <namespace> -o custom-columns=NAME:.metadata.name,DNS_POLICY:.spec.dnsPolicy,HOST_NETWORK:.spec.hostNetwork
  
  # 检查 Deployment 模板中的 dnsPolicy 配置
  kubectl get deployment <deployment> -n <namespace> -o jsonpath='{.spec.template.spec.dnsPolicy}'
  ```
- **超时**: 10s
- **预期输出模式**: dnsPolicy 值（ClusterFirst, Default, None, ClusterFirstWithHostNet）
- **判断规则**:
  - `ClusterFirst`（默认）→ 使用集群 DNS（CoreDNS），这是标准配置
  - `Default` → 使用节点的 DNS 配置（/etc/resolv.conf），不使用集群 DNS（RC-012）
  - `None` → 必须配合 dnsConfig 使用，否则 Pod 无 DNS 配置
  - `ClusterFirstWithHostNet` → hostNetwork=true 的 Pod 使用集群 DNS
  - hostNetwork=true 但 dnsPolicy=ClusterFirst → 错误配置，应使用 ClusterFirstWithHostNet（RC-012）
- **版本差异**: 无

**Step D5.2**: 分析 dnsPolicy 行为差异
- **命令**:
  ```bash
  # 创建测试 Pod 比较不同 dnsPolicy 的行为
  # ClusterFirst Pod
  kubectl run dns-test-cf --image=busybox:1.36 --restart=Never --dry-run=client -o yaml -- sleep 3600 | \
    kubectl apply -f - && sleep 5 && kubectl exec dns-test-cf -- cat /etc/resolv.conf
  
  # Default Policy Pod (需要手动指定 dnsPolicy: Default)
  # 对比两者的 resolv.conf 差异
  ```
- **超时**: 30s
- **预期输出模式**: 不同 dnsPolicy 下的 resolv.conf 内容对比
- **判断规则**:
  - **ClusterFirst**: nameserver 指向 kube-dns ClusterIP，search 包含 `svc.cluster.local`
  - **Default**: nameserver 指向节点 DNS（如 10.0.0.2 或云提供商 DNS），无 svc.cluster.local search 域
  - **None**: resolv.conf 完全由 dnsConfig 定义，如果 dnsConfig 为空则无 DNS 配置
  - **ClusterFirstWithHostNet**: 与 ClusterFirst 相同，但用于 hostNetwork Pod
- **版本差异**: 无
- **清理**:
  ```bash
  kubectl delete pod dns-test-cf --force --grace-period=0
  ```

**Step D5.3**: 检查自定义 dnsConfig 配置
- **命令**:
  ```bash
  # 获取 Pod 的完整 dnsConfig
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.dnsConfig}' | jq .
  
  # 检查 dnsConfig 中的自定义 nameservers
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.dnsConfig.nameservers[*]}'
  
  # 检查 dnsConfig 中的自定义 searches
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.dnsConfig.searches[*]}'
  
  # 检查 dnsConfig 中的 options
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.dnsConfig.options}' | jq .
  ```
- **超时**: 10s
- **预期输出模式**: dnsConfig 的 nameservers、searches、options 配置
- **判断规则**:
  - dnsConfig.nameservers 指向不可达的 DNS → 自定义 DNS 不可用（RC-012）
  - dnsConfig.searches 缺少必要的搜索域 → 短域名无法解析
  - dnsConfig.options 中 ndots 值过高或过低 → 影响解析行为（RC-005）
  - dnsConfig 为空且 dnsPolicy=None → Pod 无 DNS 配置（RC-012）
- **版本差异**: 无

**Step D5.4**: 分析 ndots 配置对解析性能的影响
- **命令**:
  ```bash
  # 获取当前 ndots 配置
  kubectl exec <pod> -- cat /etc/resolv.conf | grep ndots
  
  # 使用 dig 观察不同 ndots 值下的查询行为
  # ndots=5 (默认): 域名中 "." 少于 5 个时，先搜索 search 域
  kubectl exec <pod> -- dig +search +showsearch api.example.com 2>&1 | head -30
  
  # 使用 FQDN（尾部带点）绕过 ndots
  kubectl exec <pod> -- dig api.example.com. +short
  ```
- **超时**: 15s
- **预期输出模式**: ndots 值和 DNS 查询序列
- **判断规则**:
  - ndots=5（默认）且应用大量访问外部域名 → 导致 4-5 次无效查询（RC-005），建议降低 ndots
  - ndots=1 或 ndots=2 → 外部域名查询效率高，但短域名（如 `svc-name`）可能解析失败
  - **优化建议**:
    - ndots=2 + 应用使用 FQDN（如 `api.example.com.`）是最佳实践
    - 或在 dnsConfig 中设置 ndots=2 + single-request-reopen
  - 计算无效查询数: `(5 - 域名中的点数)` 次无效查询（对于 ndots=5）
- **版本差异**: 无

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | **CoreDNS Pod 异常（崩溃/未就绪）** — CoreDNS Pod 因 Corefile 语法错误、插件 panic、OOMKilled 等原因崩溃或处于 CrashLoopBackOff，导致 DNS 服务不可用 | 高 | D1.1 显示 CrashLoopBackOff/Error；D2.1 日志包含 panic/fatal/parse error；D1.5 事件显示 BackOff | dns-fta: BE-coredns-crash |
| RC-002 | **kube-dns Service 无 Endpoints** — kube-dns Service 的 selector 与 CoreDNS Pod 的 label 不匹配，或 CoreDNS Pod 全部不健康导致 endpoint controller 移除所有 Endpoints | 高 | D1.2 Endpoints 为空；D1.1 CoreDNS Pod 全部不 Ready；Service selector 与 Pod label 不匹配 | dns-fta: BE-no-endpoints |
| RC-003 | **Corefile 配置错误（upstream DNS 不可达或语法错误）** — CoreDNS 的 Corefile 配置中 forward 指令指向的 upstream DNS 不可达，或配置语法错误导致 CoreDNS 无法正确处理查询 | 中 | D2.2 Corefile 中 forward 配置异常；D2.10 upstream DNS 不可达；D2.1 日志包含 "i/o timeout" | dns-fta: BE-corefile-error |
| RC-004 | **NetworkPolicy 阻断 DNS 端口（53/UDP/TCP）** — 集群中的 NetworkPolicy 意外阻断了 Pod 到 kube-dns Service 的 UDP/TCP 53 端口流量，导致 DNS 查询被丢弃 | 中 | D2.3 存在影响 DNS 端口的 NetworkPolicy；D1.3 部分 namespace 的 Pod DNS 失败而其他正常 | dns-fta: BE-netpol-block |
| RC-005 | **ndots 配置导致外部域名解析超慢** — Pod 默认的 `ndots:5` 设置导致对外部域名的每次查询先产生 4-5 次无效的搜索域扩展查询，大量增加 DNS 延迟和 CoreDNS 负载 | 中 | D1.4 resolv.conf 显示 ndots:5；D2.5 dig 显示大量搜索域扩展查询；D3.3 外部域名延迟 >1s 而内部正常 | dns-fta: BE-ndots-slow |
| RC-006 | **CoreDNS 资源不足（CPU/内存限制过低）** — CoreDNS Pod 的 CPU limits 过低导致 throttling，或 memory limits 过低导致 OOMKilled，在高 DNS 查询负载下 CoreDNS 无法及时响应 | 中 | D2.4 CPU/内存接近 limits；D1.1 显示 OOMKilled；D2.1 日志显示处理延迟 | dns-fta: BE-resource-exhaustion |
| RC-007 | **NodeLocal DNSCache 故障** — NodeLocal DNSCache DaemonSet Pod 在某些节点上不健康，但 Pod 的 resolv.conf 仍指向 NodeLocal DNS 的链路本地地址（169.254.20.10），导致 DNS 查询失败或超时后才 fallback | 中 | D2.6 NodeLocal DNS Pod 不健康；D1.4 nameserver 指向 169.254.20.10；D1.3 仅特定节点上的 Pod DNS 失败 | dns-fta: BE-nodelocal-failure |
| RC-008 | **conntrack 表满导致 DNS 间歇性失败** — Linux 内核 conntrack 表溢出，或 UDP DNS 查询的 conntrack 竞态条件（race condition），导致 DNS 包被随机丢弃 | 低 | D2.7 conntrack 表接近满或有溢出日志；S9 症状（间歇性失败）；问题与高 DNS 查询量相关 | dns-fta: BE-conntrack-race |
| RC-009 | **上游 DNS 服务器不可用** — CoreDNS 配置的上游 DNS 服务器（通常是节点的 `/etc/resolv.conf` 中指定的）不可达或响应超慢，导致所有需要 forward 到上游的查询失败 | 中 | D2.10 上游 DNS 查询超时或失败；D2.1 日志包含 "i/o timeout"；D1.3 外部 DNS 失败但内部正常 | dns-fta: BE-upstream-down |
| RC-010 | **Headless Service DNS 记录未更新** — Headless Service 的 DNS 记录未反映当前的 Ready Pod 列表，可能由于 Endpoints 更新延迟或 CoreDNS 的 kubernetes 插件缓存问题 | 低 | D2.9 Headless Service DNS 返回的 IP 与实际 Ready Pod 不一致；Endpoints 数量与 Pod 数量不匹配 | dns-fta: BE-headless-stale |
| RC-011 | **CoreDNS 循环检测（loop plugin 触发）** — CoreDNS 的 `loop` 插件检测到 DNS 查询循环（通常因为 upstream DNS 指回了 CoreDNS 自身），触发 CoreDNS 崩溃以防止无限循环 | 低 | D2.1 日志包含 "Loop detected"；D2.2 Corefile 中 forward 指向的地址最终解析回 CoreDNS；D2.10 CoreDNS Pod 的 resolv.conf 指向 kube-dns ClusterIP | dns-fta: BE-dns-loop |
| RC-012 | **Pod 的 dnsPolicy 设置错误** — Pod spec 中的 `dnsPolicy` 配置不正确（如 `Default` 替代了 `ClusterFirst`，或 `None` 未配套 `dnsConfig`），导致 Pod 无法使用集群 DNS | 低 | D1.4 resolv.conf 未指向 kube-dns ClusterIP；Pod spec 中 dnsPolicy 为 Default/None；同一 namespace 其他 Pod DNS 正常 | dns-fta: BE-dnspolicy-wrong |
| RC-013 | **CoreDNS 插件链配置异常** — CoreDNS 的 Corefile 中插件配置错误，包括 forward 插件目标不可达、cache 过期配置不当、loop 检测误触发、或自定义插件加载失败，导致 DNS 解析异常 | ~6% | D2.2 Corefile 中插件配置异常；CoreDNS 日志中出现 `plugin/` 相关错误；`kubectl -n kube-system get cm coredns -o yaml` 显示配置错误；修正配置后问题恢复 | dns-fta: BE-plugin-chain-error |
| RC-014 | **大规模集群 DNS QPS 压力** — 集群规模较大（>1000 Pod）或 DNS 查询负载峰值时，CoreDNS 资源不足导致 DNS 响应过慢或超时。症状包括 CoreDNS Pod CPU 持续 >80%、DNS 延迟 >100ms | ~5% | D2.4 CoreDNS CPU/内存使用接近 limits；CoreDNS metrics (`coredns_dns_requests_total`, `coredns_dns_response_rcode_count_total`) 显示 QPS 峰值；DNS 延迟与集群负载相关；扩容或启用 NodeLocal DNSCache 后缓解 | dns-fta: BE-dns-qps-overload |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: 滚动重启 CoreDNS Pod
- **适用根因**: RC-001（CoreDNS 崩溃但 Corefile 配置正确时）
- **前置检查**:
  ```bash
  # 确认 CoreDNS ConfigMap（Corefile）配置正确
  kubectl get configmap coredns -n kube-system -o yaml
  # 确认问题不是 Corefile 语法错误导致的（如果是，需先修复 Corefile）
  # 确认 CoreDNS Deployment 存在
  kubectl get deployment coredns -n kube-system
  ```
- **执行命令**:
  ```bash
  # 滚动重启 CoreDNS（不会导致所有 Pod 同时重启）
  kubectl rollout restart deployment/coredns -n kube-system
  # 等待滚动重启完成
  kubectl rollout status deployment/coredns -n kube-system --timeout=120s
  ```
- **后置验证**:
  ```bash
  # 检查 CoreDNS Pod 状态
  kubectl get pods -n kube-system -l k8s-app=kube-dns
  # 预期: 所有 Pod Running 且 Ready 1/1
  # 快速 DNS 测试
  kubectl run dns-verify --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes.default.svc.cluster.local
  # 预期: 返回正确的 ClusterIP
  ```
- **回滚命令**:
  ```bash
  # 如果新 Pod 仍然崩溃，可回滚到上一个 revision（如果之前有配置变更）
  kubectl rollout undo deployment/coredns -n kube-system
  # 滚动重启本身是幂等操作，通常不需要回滚
  ```

#### REM-002: 修复 Pod dnsPolicy 配置
- **适用根因**: RC-012
- **前置检查**:
  ```bash
  # 确认 Pod 的 dnsPolicy 设置
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.dnsPolicy}'
  # 检查当前设置：如果返回 "Default" 或 "None"，且 Pod 需要访问集群 DNS，则需修复
  # 确认 Pod 的 Deployment/StatefulSet 等控制器
  kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.metadata.ownerReferences[0].kind}/{.metadata.ownerReferences[0].name}'
  ```
- **执行命令**:
  ```bash
  # 修改 Deployment 的 dnsPolicy 为 ClusterFirst（默认值）
  kubectl patch deployment <deployment-name> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/dnsPolicy", "value": "ClusterFirst"}
  ]'
  # 如果 Pod 使用 hostNetwork=true，应设为 ClusterFirstWithHostNet
  # kubectl patch deployment <deployment-name> -n <namespace> --type='json' -p='[
  #   {"op": "replace", "path": "/spec/template/spec/dnsPolicy", "value": "ClusterFirstWithHostNet"}
  # ]'
  ```
- **后置验证**:
  ```bash
  # 等待新 Pod 创建
  kubectl rollout status deployment/<deployment-name> -n <namespace> --timeout=120s
  # 检查新 Pod 的 resolv.conf
  kubectl exec <new-pod-name> -n <namespace> -- cat /etc/resolv.conf
  # 预期: nameserver 指向 kube-dns ClusterIP
  # 测试 DNS
  kubectl exec <new-pod-name> -n <namespace> -- nslookup kubernetes.default.svc.cluster.local
  # 预期: 解析成功
  ```
- **回滚命令**:
  ```bash
  # 回滚到原来的 dnsPolicy
  kubectl patch deployment <deployment-name> -n <namespace> --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/dnsPolicy", "value": "<original-value>"}
  ]'
  ```

#### REM-003: 优化 ndots 设置
- **适用根因**: RC-005
- **前置检查**:
  ```bash
  # 确认当前 ndots 设置
  kubectl exec <pod> -- cat /etc/resolv.conf | grep ndots
  # 预期: ndots:5（默认值，需要优化）
  # 确认应用确实大量访问外部域名（通过日志或流量分析）
  ```
- **执行命令**:
  ```bash
  # 在 Deployment 中添加 dnsConfig 降低 ndots
  kubectl patch deployment <deployment-name> -n <namespace> --type='merge' -p='
  {
    "spec": {
      "template": {
        "spec": {
          "dnsConfig": {
            "options": [
              {"name": "ndots", "value": "2"},
              {"name": "single-request-reopen"},
              {"name": "timeout", "value": "2"},
              {"name": "attempts", "value": "3"}
            ]
          }
        }
      }
    }
  }'
  ```
  > **说明**:
  > - `ndots: 2` — 域名中 `.` 数量 ≥ 2 时直接做绝对查询，避免搜索域扩展（对 `api.example.com` 生效）
  > - `single-request-reopen` — 缓解 conntrack 竞态条件，A 和 AAAA 查询使用不同 socket
  > - `timeout: 2` — DNS 查询超时 2 秒（默认 5 秒）
  > - `attempts: 3` — 重试 3 次（默认 2 次）
- **后置验证**:
  ```bash
  # 等待新 Pod 创建
  kubectl rollout status deployment/<deployment-name> -n <namespace> --timeout=120s
  # 检查新 Pod 的 resolv.conf
  kubectl exec <new-pod-name> -n <namespace> -- cat /etc/resolv.conf
  # 预期: options ndots:2 single-request-reopen timeout:2 attempts:3
  # 测试外部域名解析延迟
  kubectl exec <new-pod-name> -n <namespace> -- time nslookup google.com
  # 预期: 延迟显著降低（从 >5s 降至 <1s）
  ```
- **回滚命令**:
  ```bash
  # 移除 dnsConfig（恢复默认 ndots:5）
  kubectl patch deployment <deployment-name> -n <namespace> --type='json' -p='[
    {"op": "remove", "path": "/spec/template/spec/dnsConfig"}
  ]'
  ```

#### REM-011: CoreDNS 性能调优
- **适用根因**: RC-006, RC-013, RC-014
- **前置检查**:
  ```bash
  # 检查 CoreDNS 当前配置和资源使用
  kubectl get deployment coredns -n kube-system -o jsonpath='{.spec.template.spec.containers[0].resources}'
  kubectl top pods -n kube-system -l k8s-app=kube-dns
  
  # 检查当前 Corefile 配置
  kubectl get configmap coredns -n kube-system -o jsonpath='{.data.Corefile}'
  
  # 检查 CoreDNS 副本数
  kubectl get deployment coredns -n kube-system -o jsonpath='{.spec.replicas}'
  ```
- **执行命令**:
  ```bash
  # 优化 1: 调整 cache TTL（延长缓存时间减少上游查询）
  # 编辑 CoreDNS ConfigMap，将 cache 30 调整为 cache 60 或更高
  kubectl edit configmap coredns -n kube-system
  # 在 Corefile 中找到 "cache 30" 并修改为 "cache 60"
  
  # 优化 2: 调整 forward 插件配置
  # 确保 forward 插件的 max_concurrent 设置合理（建议 1000-3000）
  # forward . 8.8.8.8 8.8.4.4 {
  #     max_concurrent 2000
  #     prefer_udp
  # }
  
  # 优化 3: 增加 CoreDNS 副本数（大规模集群）
  kubectl scale deployment coredns -n kube-system --replicas=3
  # 建议: 每 500-1000 Pod 增加 1 个 CoreDNS 副本
  ```
- **后置验证**:
  ```bash
  # 等待 CoreDNS reload（默认 30 秒 reload 周期）
  sleep 45
  
  # 检查 CoreDNS Pod 状态
  kubectl get pods -n kube-system -l k8s-app=kube-dns
  
  # 测试 DNS 延迟
  kubectl run dns-perf-test --image=busybox:1.36 --rm -it --restart=Never -- sh -c '
    for i in 1 2 3 4 5; do
      start=$(date +%s%N)
      nslookup kubernetes.default.svc.cluster.local > /dev/null 2>&1
      end=$(date +%s%N)
      echo "Query $i: $(( (end - start) / 1000000 ))ms"
    done
  '
  # 预期: 延迟显著降低
  ```
- **回滚命令**:
  ```bash
  # 恢复原始 Corefile 配置
  kubectl apply -f /tmp/coredns-configmap-backup.yaml
  
  # 恢复原始副本数
  kubectl scale deployment coredns -n kube-system --replicas=<original-count>
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-004: 修复 Corefile upstream DNS 配置
- **适用根因**: RC-003, RC-009, RC-011
- **影响说明**: 修改 CoreDNS 的 Corefile ConfigMap 将触发 CoreDNS 自动 reload（如果启用了 `reload` 插件），影响所有集群 DNS 查询。配置错误将导致全集群 DNS 中断。
- **审批提示**: "建议修改 CoreDNS Corefile 中的 upstream DNS 配置。CoreDNS 将自动 reload 新配置。配置错误可能导致集群 DNS 全面中断。是否批准？"
- **前置检查**:
  ```bash
  # 备份当前 Corefile
  kubectl get configmap coredns -n kube-system -o yaml > /tmp/coredns-configmap-backup.yaml
  # 确认问题
  kubectl get configmap coredns -n kube-system -o jsonpath='{.data.Corefile}'
  ```
- **执行命令**:
  ```bash
  # 场景 1: 修复 upstream DNS 指向（替换为可靠的 DNS 服务器）
  kubectl edit configmap coredns -n kube-system
  # 在 Corefile 中将 forward 指令修改为:
  #   forward . 8.8.8.8 8.8.4.4 {
  #       max_concurrent 1000
  #   }
  # 或者对于内网环境，指向公司内部 DNS

  # 场景 2: 修复 DNS 循环（RC-011）
  # 确保 forward 不指向 /etc/resolv.conf（如果其中包含 kube-dns ClusterIP）
  # 改为直接指向上游 DNS IP

  # 场景 3: 使用 kubectl 直接 patch（非交互式）
  kubectl create configmap coredns -n kube-system --from-literal=Corefile='
  .:53 {
      errors
      health {
          lameduck 5s
      }
      ready
      kubernetes cluster.local in-addr.arpa ip6.arpa {
          pods insecure
          fallthrough in-addr.arpa ip6.arpa
          ttl 30
      }
      prometheus :9153
      forward . 8.8.8.8 8.8.4.4 {
          max_concurrent 1000
      }
      cache 30
      loop
      reload
      loadbalance
  }
  ' --dry-run=client -o yaml | kubectl apply -f -
  ```
- **后置验证**:
  ```bash
  # 等待 CoreDNS reload（默认 30 秒 reload 周期）
  sleep 45
  # 检查 CoreDNS 日志是否有 reload 成功的记录
  kubectl logs -n kube-system -l k8s-app=kube-dns --tail=20 | grep -i "reload"
  # 测试 DNS
  kubectl run dns-verify --image=busybox:1.36 --rm -it --restart=Never -- sh -c "nslookup kubernetes.default.svc.cluster.local && nslookup google.com"
  # 预期: 内部和外部 DNS 均解析成功
  ```
- **回滚命令**:
  ```bash
  # 从备份恢复
  kubectl apply -f /tmp/coredns-configmap-backup.yaml
  # 强制重启 CoreDNS 加载旧配置
  kubectl rollout restart deployment/coredns -n kube-system
  ```

#### REM-005: 调整 CoreDNS 资源限制
- **适用根因**: RC-006
- **影响说明**: 修改 CoreDNS Deployment 的资源 limits/requests，将触发 Pod 滚动更新。在滚动更新过程中，DNS 服务可能短暂降级（取决于 replicas 数量和 PDB 配置）。
- **审批提示**: "建议提升 CoreDNS 的 CPU/内存资源限制。将触发 CoreDNS Pod 滚动更新，更新期间 DNS 服务可能短暂降级。是否批准？"
- **前置检查**:
  ```bash
  # 查看当前资源配置
  kubectl get deployment coredns -n kube-system -o jsonpath='{.spec.template.spec.containers[0].resources}'
  # 查看当前实际使用
  kubectl top pods -n kube-system -l k8s-app=kube-dns
  # 确认 PodDisruptionBudget
  kubectl get pdb -n kube-system | grep coredns
  ```
- **执行命令**:
  ```bash
  # 提升 CoreDNS 资源限制
  kubectl patch deployment coredns -n kube-system --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources", "value": {
      "requests": {"cpu": "200m", "memory": "256Mi"},
      "limits": {"cpu": "500m", "memory": "512Mi"}
    }}
  ]'
  # 等待滚动更新完成
  kubectl rollout status deployment/coredns -n kube-system --timeout=180s
  ```
- **后置验证**:
  ```bash
  # 确认新 Pod 使用了更新的资源配置
  kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{range .items[*]}{.metadata.name}: cpu_limit={.spec.containers[0].resources.limits.cpu} mem_limit={.spec.containers[0].resources.limits.memory}{"\n"}{end}'
  # 预期: cpu_limit=500m mem_limit=512Mi
  # 测试 DNS
  kubectl run dns-verify --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes.default.svc.cluster.local
  ```
- **回滚命令**:
  ```bash
  # 恢复原始资源配置
  kubectl patch deployment coredns -n kube-system --type='json' -p='[
    {"op": "replace", "path": "/spec/template/spec/containers/0/resources", "value": {
      "requests": {"cpu": "100m", "memory": "70Mi"},
      "limits": {"cpu": "100m", "memory": "170Mi"}
    }}
  ]'
  ```

#### REM-006: 扩容 CoreDNS 副本数
- **适用根因**: RC-006（负载过高导致的性能问题）
- **影响说明**: 增加 CoreDNS Deployment 的 replicas 数量。新 Pod 将被调度到可用节点上。需确保节点有足够资源容纳额外的 CoreDNS Pod。
- **审批提示**: "建议将 CoreDNS 副本数从 {current} 增加到 {target}，以分散 DNS 查询负载。是否批准？"
- **前置检查**:
  ```bash
  # 查看当前副本数
  kubectl get deployment coredns -n kube-system -o jsonpath='{.spec.replicas}'
  # 查看节点可用资源
  kubectl top nodes
  # 建议: 生产环境 CoreDNS 副本数 ≥ 2，大规模集群（>100 nodes）建议 3-5 个副本
  ```
- **执行命令**:
  ```bash
  # 扩容 CoreDNS
  kubectl scale deployment coredns -n kube-system --replicas=3
  # 等待新 Pod 就绪
  kubectl rollout status deployment/coredns -n kube-system --timeout=120s
  ```
- **后置验证**:
  ```bash
  # 确认所有 Pod 就绪
  kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
  # 预期: 3 个 Pod 全部 Running 且 Ready
  # 确认 Endpoints 更新
  kubectl get endpoints kube-dns -n kube-system
  # 预期: 包含 3 个 Pod IP
  ```
- **回滚命令**:
  ```bash
  # 恢复原始副本数
  kubectl scale deployment coredns -n kube-system --replicas=<original-count>
  ```

#### REM-007: 修复 NetworkPolicy 以允许 DNS 流量
- **适用根因**: RC-004
- **影响说明**: 修改 NetworkPolicy 以放行 DNS 流量（UDP/TCP 53）。NetworkPolicy 变更立即生效，可能影响该 namespace 的网络安全策略。
- **审批提示**: "建议修改 namespace `{namespace}` 的 NetworkPolicy 以放行到 kube-dns 的 DNS 流量（UDP/TCP 53）。是否批准？"
- **前置检查**:
  ```bash
  # 确认阻断 DNS 的 NetworkPolicy
  kubectl get networkpolicy -n <namespace> -o yaml
  # 确认 kube-dns 的 ClusterIP 和端口
  kubectl get svc kube-dns -n kube-system
  ```
- **执行命令**:
  ```bash
  # 方案 1: 在现有的 egress NetworkPolicy 中添加 DNS 例外
  # 如果 namespace 有 default-deny egress 策略，需要创建一个允许 DNS 的策略
  cat <<EOF | kubectl apply -f -
  apiVersion: networking.k8s.io/v1
  kind: NetworkPolicy
  metadata:
    name: allow-dns-egress
    namespace: <namespace>
  spec:
    podSelector: {}
    policyTypes:
    - Egress
    egress:
    - to:
      - namespaceSelector:
          matchLabels:
            kubernetes.io/metadata.name: kube-system
        podSelector:
          matchLabels:
            k8s-app: kube-dns
      ports:
      - protocol: UDP
        port: 53
      - protocol: TCP
        port: 53
  EOF
  ```
- **后置验证**:
  ```bash
  # 确认 NetworkPolicy 已创建
  kubectl get networkpolicy allow-dns-egress -n <namespace>
  # 测试 DNS
  kubectl exec <pod-in-namespace> -n <namespace> -- nslookup kubernetes.default.svc.cluster.local
  # 预期: 解析成功
  ```
- **回滚命令**:
  ```bash
  # 删除新创建的 NetworkPolicy
  kubectl delete networkpolicy allow-dns-egress -n <namespace>
  ```

#### REM-012: NodeLocal DNSCache 部署与修复
- **适用根因**: RC-007, RC-008, RC-014
- **影响说明**: 部署或修复 NodeLocal DNSCache 涉及多个组件的配置变更，包括 DaemonSet、ConfigMap 和可能的 kubelet 配置。配置错误可能导致 DNS 完全不可用。
- **审批提示**: "建议部署/修复 NodeLocal DNSCache 以缓解 DNS QPS 压力和 conntrack 竞态条件。此操作会影响所有新创建 Pod 的 DNS 解析路径。是否批准？"
- **前置检查**:
  ```bash
  # 确认集群 DNS 模式（iptables vs ipvs）
  kubectl get configmap kube-proxy -n kube-system -o jsonpath='{.data.config\.conf}' | grep mode
  
  # 检查 NodeLocal DNSCache 是否已部署
  kubectl get ds -n kube-system | grep node-local-dns
  
  # 获取 kube-dns Service ClusterIP
  KUBE_DNS_IP=$(kubectl get svc kube-dns -n kube-system -o jsonpath='{.spec.clusterIP}')
  echo "kube-dns ClusterIP: $KUBE_DNS_IP"
  ```
- **执行命令**:
  ```bash
  # 场景 1: 已部署但故障，修复 NodeLocal DNS Pod
  # 滚动重启 DaemonSet
  kubectl rollout restart ds/node-local-dns -n kube-system
  kubectl rollout status ds/node-local-dns -n kube-system --timeout=300s
  
  # 场景 2: 修复 NodeLocal DNS ConfigMap 配置
  # 确保 upstream DNS 地址正确
  kubectl edit configmap node-local-dns -n kube-system
  # 确认 __PILLAR__CLUSTER__DNS__ 已被替换为实际的 kube-dns ClusterIP
  # 确认 __PILLAR__LOCAL__DNS__ 已被替换为 169.254.20.10
  
  # 场景 3: 验证 169.254.20.10 可达
  kubectl run dns-local-test --image=busybox:1.36 --rm -it --restart=Never -- sh -c '
    # 测试 NodeLocal DNS
    nslookup kubernetes.default 169.254.20.10
    echo "---"
    # 测试外部域名
    nslookup google.com 169.254.20.10
  '
  ```
- **后置验证**:
  ```bash
  # 确认所有 NodeLocal DNS Pod 运行正常
  kubectl get pods -n kube-system -l k8s-app=node-local-dns -o wide
  # 预期: 所有 Pod Running 且 Ready
  
  # 检查 DNS 解析延迟是否下降
  kubectl run dns-perf --image=busybox:1.36 --rm -it --restart=Never -- sh -c '
    for i in 1 2 3 4 5; do
      start=$(date +%s%N)
      nslookup kubernetes.default > /dev/null
      end=$(date +%s%N)
      echo "$i: $(( (end - start) / 1000000 ))ms"
    done
  '
  
  # 检查 CoreDNS QPS 是否降低（通过 metrics）
  kubectl top pods -n kube-system -l k8s-app=kube-dns
  ```
- **回滚命令**:
  ```bash
  # 如果 NodeLocal DNSCache 导致问题，可以禁用
  # 步骤 1: 删除 NodeLocal DNS DaemonSet
  kubectl delete ds node-local-dns -n kube-system
  kubectl delete configmap node-local-dns -n kube-system
  
  # 步骤 2: 恢复 kubelet --cluster-dns 为原始 kube-dns ClusterIP
  # 需要在每个节点上执行
  ssh <node-ip> "systemctl restart kubelet"
  
  # 步骤 3: 新创建的 Pod 将使用原始 DNS 配置
  kubectl run dns-verify --image=busybox:1.36 --rm -it --restart=Never -- cat /etc/resolv.conf
  # 预期: nameserver 指向 kube-dns ClusterIP（而非 169.254.20.10）
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-008: 部署或修复 NodeLocal DNSCache
- **适用根因**: RC-007, RC-008（通过 NodeLocal DNS 缓解 conntrack 竞态）
- **影响说明**: 部署 NodeLocal DNSCache DaemonSet 会修改每个节点上 Pod 的 DNS 解析路径。所有新创建的 Pod 将使用 NodeLocal DNS（169.254.20.10）作为 nameserver。部署过程中如果配置错误，可能导致**全集群 DNS 中断**。
- **操作步骤**:
  1. **评估是否需要 NodeLocal DNSCache**:
     ```bash
     # 检查当前 DNS 查询量
     # 如果 CoreDNS 负载持续较高，或存在 conntrack 竞态导致的间歇性失败，建议部署
     kubectl top pods -n kube-system -l k8s-app=kube-dns
     ```
  2. **获取并自定义 NodeLocal DNS 部署清单**:
     ```bash
     # 下载官方 NodeLocal DNS 部署清单
     # 需要替换以下变量:
     # __PILLAR__DNS__DOMAIN__ → cluster.local
     # __PILLAR__DNS__SERVER__ → kube-dns ClusterIP (如 10.96.0.10)
     # __PILLAR__LOCAL__DNS__ → 169.254.20.10
     ```
  3. **部署 NodeLocal DNS DaemonSet**:
     ```bash
     kubectl apply -f nodelocaldns.yaml
     # 等待所有节点上的 Pod 就绪
     kubectl get ds -n kube-system node-local-dns
     kubectl rollout status ds/node-local-dns -n kube-system --timeout=300s
     ```
  4. **修改 kubelet 配置以使用 NodeLocal DNS**:
     ```bash
     # 在每个节点上将 kubelet 的 --cluster-dns 参数改为 169.254.20.10
     # 或通过 kubelet config 修改 clusterDNS
     # 此步骤需要逐节点滚动执行，每次仅操作一个节点
     ```
  5. **验证**:
     ```bash
     # 在新创建的 Pod 中检查 resolv.conf
     kubectl run dns-verify --image=busybox:1.36 --rm -it --restart=Never -- cat /etc/resolv.conf
     # 预期: nameserver 169.254.20.10
     # 测试 DNS
     kubectl run dns-verify --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes.default.svc.cluster.local
     ```
- **安全检查**:
  - 确保 NodeLocal DNS ConfigMap 中的 upstream DNS 地址正确
  - 逐节点部署，每次验证一个节点后再继续下一个
  - 保留原始 kubelet 配置备份
- **回滚方案**:
  ```bash
  # 1. 将 kubelet --cluster-dns 恢复为原始 kube-dns ClusterIP
  # 2. 删除 NodeLocal DNS DaemonSet
  kubectl delete ds node-local-dns -n kube-system
  kubectl delete configmap node-local-dns -n kube-system
  # 3. 重启 kubelet
  ssh <node-ip> "systemctl restart kubelet"
  ```

#### REM-009: 调整 conntrack 表设置
- **适用根因**: RC-008
- **影响说明**: 修改节点的内核 conntrack 参数需要在每个受影响节点上操作。参数修改立即生效但不持久化（需写入 sysctl.conf）。设置不当可能影响节点网络性能。
- **操作步骤**:
  1. **评估当前 conntrack 状态**:
     ```bash
     ssh <node-ip> "cat /proc/sys/net/nf_conntrack_max"
     ssh <node-ip> "cat /proc/sys/net/netfilter/nf_conntrack_count"
     ```
  2. **增大 conntrack 表上限**:
     ```bash
     # 临时修改（重启后失效）
     ssh <node-ip> "sysctl -w net.nf_conntrack_max=524288"
     # 减少 conntrack 条目超时时间（加速回收）
     ssh <node-ip> "sysctl -w net.netfilter.nf_conntrack_udp_timeout=30"
     ssh <node-ip> "sysctl -w net.netfilter.nf_conntrack_udp_timeout_stream=120"
     ```
  3. **持久化配置**:
     ```bash
     ssh <node-ip> "cat >> /etc/sysctl.d/99-conntrack.conf << 'EOF'
     net.nf_conntrack_max = 524288
     net.netfilter.nf_conntrack_udp_timeout = 30
     net.netfilter.nf_conntrack_udp_timeout_stream = 120
     EOF"
     ssh <node-ip> "sysctl --system"
     ```
  4. **验证**:
     ```bash
     ssh <node-ip> "sysctl net.nf_conntrack_max"
     # 预期: net.nf_conntrack_max = 524288
     ```
- **安全检查**:
  - 确认修改的参数值合理（不宜过大导致内存占用过高，每个 conntrack 条目约 300 bytes）
  - 524288 条目约占 150MB 内存
  - 在一个节点上验证后再推广到其他节点
- **回滚方案**:
  ```bash
  # 恢复默认值
  ssh <node-ip> "sysctl -w net.nf_conntrack_max=131072"
  ssh <node-ip> "rm -f /etc/sysctl.d/99-conntrack.conf"
  ssh <node-ip> "sysctl --system"
  ```

---

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-010: 完整重写 CoreDNS ConfigMap
- **适用根因**: RC-003（严重的 Corefile 配置错误或 ConfigMap 损坏）, RC-011（复杂的 DNS 循环）
- **审批要求**: 需要高级 SRE 审批。Corefile 错误将导致**全集群 DNS 立即中断**。
- **数据备份**:
  ```bash
  # 完整备份当前 ConfigMap
  kubectl get configmap coredns -n kube-system -o yaml > /tmp/coredns-configmap-full-backup-$(date +%Y%m%d%H%M%S).yaml
  # 备份 CoreDNS Deployment
  kubectl get deployment coredns -n kube-system -o yaml > /tmp/coredns-deployment-backup-$(date +%Y%m%d%H%M%S).yaml
  ```
- **操作步骤**:
  1. **准备标准 Corefile**:
     ```bash
     # 确认集群 DNS 域名（通常为 cluster.local）
     kubectl get configmap kubeadm-config -n kube-system -o jsonpath='{.data.ClusterConfiguration}' 2>/dev/null | grep dnsDomain
     # 确认节点的上游 DNS
     ssh <any-node-ip> "cat /etc/resolv.conf"
     ```
  2. **创建新的 ConfigMap**:
     ```bash
     cat <<'EOF' > /tmp/coredns-corefile.yaml
     apiVersion: v1
     kind: ConfigMap
     metadata:
       name: coredns
       namespace: kube-system
     data:
       Corefile: |
         .:53 {
             errors
             health {
                 lameduck 5s
             }
             ready
             kubernetes cluster.local in-addr.arpa ip6.arpa {
                 pods insecure
                 fallthrough in-addr.arpa ip6.arpa
                 ttl 30
             }
             prometheus :9153
             forward . <upstream-dns-ip-1> <upstream-dns-ip-2> {
                 max_concurrent 1000
             }
             cache 30
             loop
             reload
             loadbalance
         }
     EOF
     ```
  3. **应用新的 ConfigMap**:
     ```bash
     kubectl apply -f /tmp/coredns-corefile.yaml
     ```
  4. **强制重启 CoreDNS 确保加载新配置**:
     ```bash
     kubectl rollout restart deployment/coredns -n kube-system
     kubectl rollout status deployment/coredns -n kube-system --timeout=120s
     ```
  5. **全面验证**:
     ```bash
     # 内部 DNS
     kubectl run dns-verify --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes.default.svc.cluster.local
     # 外部 DNS
     kubectl run dns-verify-ext --image=busybox:1.36 --rm -it --restart=Never -- nslookup google.com
     ```
- **回滚方案**:
  ```bash
  # 从备份恢复
  kubectl apply -f /tmp/coredns-configmap-full-backup-<timestamp>.yaml
  kubectl rollout restart deployment/coredns -n kube-system
  ```

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

```bash
# V1: 确认 CoreDNS Pod 全部健康
kubectl get pods -n kube-system -l k8s-app=kube-dns
# 预期: 所有 Pod Running 且 Ready 1/1，RESTARTS 不再增加

# V2: 确认 kube-dns Service 有正确的 Endpoints
kubectl get endpoints kube-dns -n kube-system
# 预期: Endpoints 包含所有健康 CoreDNS Pod 的 IP

# V3: 测试集群内部 DNS 解析
kubectl run dns-v3 --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes.default.svc.cluster.local
# 预期: 返回正确的 ClusterIP (如 10.96.0.1)

# V4: 测试外部 DNS 解析
kubectl run dns-v4 --image=busybox:1.36 --rm -it --restart=Never -- nslookup google.com
# 预期: 返回 Google 的 IP 地址

# V5: 测试跨 namespace Service 解析（使用受影响的 Service）
kubectl run dns-v5 --image=busybox:1.36 --rm -it --restart=Never -- nslookup <target-service>.<target-namespace>.svc.cluster.local
# 预期: 返回 Service 的 ClusterIP

# V6: 确认 DNS 延迟在正常范围内
kubectl run dns-v6 --image=busybox:1.36 --rm -it --restart=Never -- sh -c "time nslookup kubernetes.default.svc.cluster.local"
# 预期: 延迟 < 100ms (内部 DNS)
```

### 7.2 短期监控（5-15 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| CoreDNS SERVFAIL 率 | `rate(coredns_dns_responses_total{rcode="SERVFAIL"}[5m])` | 下降至 0 或极低值 | SERVFAIL 率持续 > 1 qps |
| CoreDNS 请求延迟 | `histogram_quantile(0.99, rate(coredns_dns_request_duration_seconds_bucket[5m]))` | P99 延迟 < 100ms (内部) | P99 > 500ms 持续 5 分钟 |
| CoreDNS 请求 QPS | `rate(coredns_dns_requests_total[5m])` | 稳定或恢复到正常水平 | QPS 突然降至 0（CoreDNS 再次宕机）或异常飙升 |
| CoreDNS CPU 使用率 | `kubectl top pods -n kube-system -l k8s-app=kube-dns` | CPU 使用率低于 limits 的 80% | CPU 使用率接近 100% limits |
| CoreDNS 内存使用率 | `kubectl top pods -n kube-system -l k8s-app=kube-dns` | 内存使用率低于 limits 的 80% | 内存持续增长且接近 limits |
| CoreDNS Pod 重启次数 | `kubectl get pods -n kube-system -l k8s-app=kube-dns` RESTARTS 列 | 不再增加 | 修复后仍有新的重启 |
| 应用 DNS 错误日志 | `kubectl logs <app-pod> \| grep -c "resolve\|NXDOMAIN\|no such host"` | 不再出现新的 DNS 错误 | 持续出现 DNS 相关错误 |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认故障已解决：

- [ ] CoreDNS 所有 Pod 处于 Running/Ready 状态，无新的重启
- [ ] kube-dns Service Endpoints 完整，包含所有健康 CoreDNS Pod
- [ ] 集群内部 DNS 解析正常（`nslookup kubernetes.default.svc.cluster.local` 成功）
- [ ] 外部 DNS 解析正常（`nslookup google.com` 成功）
- [ ] DNS 延迟在正常范围内（内部 <100ms，外部 <500ms）
- [ ] 原始报错的应用/Pod 不再出现 DNS 相关错误
- [ ] CoreDNS SERVFAIL 率恢复到正常水平（接近 0）
- [ ] 如果使用 NodeLocal DNSCache，所有节点上的 NodeLocal DNS Pod 健康
- [ ] 根因已明确记录并采取了预防措施

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| CoreDNS SERVFAIL 率 | Prometheus: `coredns_dns_responses_total{rcode="SERVFAIL"}` | 持续监控 | 率再次升高 → 重新进入本 Skill 诊断流程 |
| CoreDNS Pod 稳定性 | `kubectl get pods -n kube-system -l k8s-app=kube-dns` RESTARTS | 每小时 | 新增重启 → 检查 CoreDNS 日志确认崩溃原因 |
| DNS 延迟趋势 | `coredns_dns_request_duration_seconds` P99 趋势 | 每小时 | P99 持续上升 → 检查 CoreDNS 负载和资源使用 |
| conntrack 表使用率 | `ssh <node> "cat /proc/sys/net/netfilter/nf_conntrack_count"` | 每 4 小时 | 使用率 >80% → 考虑增大 conntrack_max |
| 应用 DNS 错误日志 | 应用监控系统或日志聚合 | 持续 | 新的 DNS 错误 → 确认是否同一根因复发 |
| CoreDNS ConfigMap 变更 | `kubectl get configmap coredns -n kube-system -o jsonpath='{.metadata.resourceVersion}'` | 每 4 小时 | resourceVersion 意外变更 → 检查是否有未授权修改 |
| 上游 DNS 可用性 | 从节点 `dig @<upstream-dns> google.com` | 每小时 | 上游 DNS 不可达 → 联系网络团队 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **10 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证（V1-V6） | REM-xxx 执行后验证持续失败 |
| **严重性升级** | 初始分级为 P2 但影响面扩大（更多 namespace 或 Pod 受影响） | 诊断过程中 DNS 故障范围扩大 |
| **未知根因** | 完成 Phase 1-3 所有诊断步骤但无法匹配任何已知根因（RC-001 至 RC-012） | 所有诊断步骤均无明确异常发现 |
| **CoreDNS 完全不可恢复** | CoreDNS Pod 重启后仍持续崩溃，所有已知修复手段无效 | 尝试 REM-001 + REM-004/REM-010 后问题持续 |
| **安全疑虑** | 发现 CoreDNS ConfigMap 被未授权修改，或 DNS 查询被劫持到异常 IP | 任何诊断步骤中发现安全异常 |
| **级联故障** | DNS 故障导致其他关键基础设施组件（Ingress Controller、Cert Manager、External Secrets 等）也出现故障 | 诊断过程中发现更多受影响组件 |

### 8.2 升级消息模板

```
【{severity}】DNS 解析故障诊断与修复 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 故障概述: 集群 DNS 解析故障，持续 {duration}
- 故障范围:
  - 影响类型: {全局/namespace级/Pod级/间歇性}
  - 内部 DNS: {正常/失败/间歇性失败}
  - 外部 DNS: {正常/失败/间歇性失败}
  - 受影响 namespace: {affected_namespaces}
  - 受影响 Pod 数量: {affected_pod_count}
- CoreDNS 状态:
  - Pod 状态: {coredns_pod_status}
  - Endpoints: {endpoint_status}
  - 最近重启次数: {restart_count}
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
- Skill 版本: SKILL-NET-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤（D1.1 → D1.2 → ... → D3.3）及每步输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
   - 例: "RC-004 已排除 — D2.3 显示受影响 namespace 无 NetworkPolicy"
   - 例: "RC-008 已排除 — D2.7 显示 conntrack 表使用率仅 23%"
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
   - 例: "疑似 RC-003（Corefile 配置错误）— D2.2 显示 forward 指向的 IP 10.0.0.2 在 D2.10 测试中超时"
4. **关键资源快照**:
   ```bash
   # CoreDNS Pod 状态和日志
   kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide > coredns-pods.txt
   kubectl logs -n kube-system -l k8s-app=kube-dns --tail=500 > coredns-logs.txt
   # CoreDNS ConfigMap
   kubectl get configmap coredns -n kube-system -o yaml > coredns-configmap.txt
   # kube-dns Service 和 Endpoints
   kubectl get svc,endpoints kube-dns -n kube-system -o yaml > kube-dns-svc-ep.txt
   # NetworkPolicy（如有）
   kubectl get networkpolicy -A -o yaml > all-networkpolicies.txt
   # 受影响 Pod 的 resolv.conf
   kubectl exec <affected-pod> -- cat /etc/resolv.conf > pod-resolv-conf.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件按时间排列
   - `HH:MM:SS` - 首次检测到 DNS 故障
   - `HH:MM:SS` - 开始诊断
   - `HH:MM:SS` - 确认故障范围（全局/局部）
   - `HH:MM:SS` - 发现异常 [描述]
   - `HH:MM:SS` - 尝试修复 [操作]
   - `HH:MM:SS` - 修复结果 [成功/失败]
   - `HH:MM:SS` - 决定升级

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| 默认 CoreDNS 版本 | 1.10.1 | 1.11.1 | 1.11.1 | 1.11.3 | 1.11.3 |
| CoreDNS `loop` 插件 | 默认启用 | 默认启用 | 默认启用 | 默认启用 | 默认启用 |
| CoreDNS `dns64` 插件 | 可用 | 可用 | 默认包含（双栈增强） | 默认包含 | 默认包含 |
| NodeLocal DNSCache | GA | GA | GA（改进健康检查） | GA | GA |
| Pod DNS 配置 (dnsConfig) | GA | GA | GA | GA | GA |
| Dual-stack DNS 支持 | beta | GA | GA（改进） | GA | GA |
| DNS Policy `ClusterFirstWithHostNet` | 支持 | 支持 | 支持 | 支持 | 支持 |
| CoreDNS Prometheus Metrics | 全量 | 全量 | 全量（新增指标） | 全量 | 全量 |
| Topology Aware Hints (DNS) | beta | beta | GA | GA | GA |
| CoreDNS `forward` prefer_udp | 可用 | 可用 | 增强 | 增强 | 增强 |
| AdminNetworkPolicy (DNS 影响) | N/A | alpha | alpha | beta | beta |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubectl get endpoints kube-dns` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubectl get endpointslice -n kube-system` | 支持（推荐） | 支持（推荐） | 支持（推荐） | 支持（推荐） | 支持（推荐） |
| `kubectl debug --image=dnsutils` | 支持 | 支持 | 支持 | 支持 | 支持 |
| CoreDNS `/ready` 端点 | 支持 | 支持 | 支持 | 支持 | 支持 |
| CoreDNS `/health` 端点 | 支持 | 支持 | 支持 | 支持 | 支持 |
| CoreDNS `/metrics` (Prometheus) | :9153 | :9153 | :9153 | :9153 | :9153 |
| `kubectl top pods` (CoreDNS) | 需要 metrics-server | 同左 | 同左 | 同左 | 同左 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Service (kube-dns) | v1 (core) | v1 | v1 | v1 | v1 |
| Endpoints (kube-dns) | v1 (core) | v1 | v1 | v1 | v1 |
| EndpointSlice | discovery.k8s.io/v1 | v1 | v1 | v1 | v1 |
| ConfigMap (coredns) | v1 (core) | v1 | v1 | v1 | v1 |
| NetworkPolicy | networking.k8s.io/v1 | v1 | v1 | v1 | v1 |
| AdminNetworkPolicy | N/A | policy.networking.k8s.io/v1alpha1 | v1alpha1 | v1beta1 | v1beta1 |
| DaemonSet (NodeLocal DNS) | apps/v1 | v1 | v1 | v1 | v1 |

### 9.4 版本相关的诊断注意事项

- **[v1.28+]**: CoreDNS 1.10.1+ 改进了日志输出格式，`errors` 插件提供更详细的错误信息。诊断时 D2.1 中的日志解析更加直观。

- **[v1.29+]**: 双栈（Dual-stack）DNS 支持 GA。在双栈集群中：
  - CoreDNS 同时监听 IPv4 和 IPv6
  - Pod 的 DNS 查询可能同时产生 A 记录和 AAAA 记录查询
  - 这增加了 DNS 查询量（每次查询实际产生两个 UDP 包），可能加剧 conntrack 竞态问题（RC-008）
  - 诊断时需同时检查 IPv4 和 IPv6 的 DNS 功能

- **[v1.30+]**: CoreDNS 1.11.x 系列增强：
  - `dns64` 插件默认包含在 Corefile 模板中（如果集群启用了 IPv6）
  - `forward` 插件新增 `prefer_udp` 选项，可减少 TCP fallback 的延迟
  - 改进的 NodeLocal DNSCache 健康检查，减少了 RC-007 的发生概率
  - Topology Aware Hints GA，CoreDNS 可感知拓扑进行流量路由
  - **默认配置变更**: Corefile 中可能新增 `lameduck` 持续时间配置，影响优雅关闭行为

- **[v1.31+]**: DNS 相关改进：
  - CoreDNS 1.11.3 修复了若干稳定性问题
  - AdminNetworkPolicy (beta) 可能在全局层面影响 DNS 流量，诊断 RC-004 时需额外检查 `kubectl get adminnetworkpolicy`
  - **DNS policy 与 Gateway API 的交互行为**: Gateway API 的 HTTPRoute 可能影响 DNS 解析行为，特别是当使用 parentRef 指向 Gateway 时

- **[v1.32+]**: 稳定性和性能改进：
  - CoreDNS 的 `kubernetes` 插件性能优化，处理大量 Service（>5000）时内存使用更低
  - 改进的 EndpointSlice 支持减少了 RC-010（Headless Service DNS 记录延迟）的发生
  - **DNS policy 与 Gateway API 的交互行为**: Gateway API GA 后，可通过 GatewayClass 的 parametersRef 配置 DNS 行为
  - kube-proxy nftables 模式 GA，NodeLocal DNSCache 的 iptables 规则需调整为 nftables

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **ndots 导致的延迟被误判为 DNS 故障** | 应用报告 DNS 超时或极慢，但 CoreDNS 本身运行正常，日志无错误 | Pod 默认 `ndots:5` 导致每次外部域名查询先执行 4-5 次搜索域扩展查询，累计延迟 >5s 触发应用超时。CoreDNS 实际正常处理了所有查询，只是查询数量被放大了 5 倍 | 在 D1.4 中检查 ndots 值；在 D2.5 中用 `dig +search` 观察实际查询序列。如果 CoreDNS 日志显示大量 NXDOMAIN 且域名带有 `.svc.cluster.local` 后缀，几乎确定是 ndots 问题 |
| **NetworkPolicy 阻断 DNS 未被怀疑** | DNS 仅在特定 namespace 的 Pod 中失败，其他 namespace 正常。初步判断为 CoreDNS 配置问题 | namespace 中存在 `default-deny-egress` NetworkPolicy 但未为 DNS（UDP 53）创建例外规则 | 在 D2.3 中**始终**检查受影响 namespace 的 NetworkPolicy，特别是 egress 策略。关键线索是"部分 namespace 失败、部分正常"——这几乎肯定是 NetworkPolicy 而非 CoreDNS 问题 |
| **conntrack 竞态导致的间歇性失败被误诊为应用 bug** | 应用偶尔报 DNS 超时，重试后成功。开发团队认为是应用代码问题（未正确处理 DNS 超时） | Linux 内核 conntrack 在处理 UDP DNS 查询时的竞态条件——当 glibc 同时发送 A 和 AAAA 查询到同一个 kube-dns ClusterIP 时，两个 UDP 包可能竞争同一个 conntrack 条目，导致其中一个被丢弃 | 如果 DNS 失败是间歇性的、随机的、且集中在高负载时段，优先检查 D2.7（conntrack）。解决方案：REM-003（ndots + single-request-reopen）或 REM-008（NodeLocal DNSCache）从根本上避免竞态 |
| **CoreDNS 循环（loop）被误诊为上游 DNS 不可达** | CoreDNS 持续崩溃重启，日志中有超时信息。初步判断为上游 DNS 服务器不可达 | CoreDNS Pod 的 `/etc/resolv.conf` 中的 nameserver 指向了 kube-dns ClusterIP（即 CoreDNS 自身），形成 DNS 查询循环。`loop` 插件检测到循环后触发 CoreDNS 退出 | 在 D2.10 中检查 CoreDNS Pod 自身的 `/etc/resolv.conf`。如果 nameserver 指向 kube-dns ClusterIP，这就是循环的根因。常见于使用 systemd-resolved 的节点（/etc/resolv.conf 指向 127.0.0.53，而 127.0.0.53 又 forward 到 kube-dns） |
| **Headless Service DNS 问题被误诊为 CoreDNS 故障** | 应用无法通过 DNS 发现 StatefulSet 的特定 Pod | Headless Service 的 Endpoints 为空（Pod 未就绪），或 Pod 的 hostname/subdomain 未正确配置 | 在 D2.9 中检查 Headless Service 的 Endpoints。如果 Endpoints 为空但 CoreDNS 对其他 Service 正常，问题在 Service/Pod 配置层面而非 DNS 系统 |
| **应用 DNS 缓存导致的"DNS 失败"** | 应用报告无法连接到某个 Service，但手动 nslookup 正常 | 应用自身的 DNS 缓存（如 JVM、Node.js 等）缓存了旧的 DNS 记录（旧的 ClusterIP 或旧的 Pod IP），未随 Service 变更更新 | 确认是否最近有 Service 重建或迁移。如果手动 DNS 查询正常但应用仍失败，检查应用的 DNS 缓存配置。JVM 默认缓存 30s（`networkaddress.cache.ttl`），Node.js 的 `dns.lookup` 不缓存但 HTTP Agent 可能保持连接 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| Kubernetes DNS 架构与 CoreDNS 原理 | `domain-5-networking/` | 理解 CoreDNS 的工作原理、Corefile 配置语法、插件链机制 |
| DNS 故障树分析 | `topic-fta/list/dns-fta.md` | 理解 DNS 故障的完整因果链和概率模型 |
| 网络故障深度排查 | `topic-structural-trouble-shooting/` | 超出本 Skill 覆盖范围的深度网络排查方法 |
| Kubernetes 故障排查方法论 | `domain-12-troubleshooting/` | 系统化故障排查的理论基础和方法论 |
| Service 网络问题 | `SKILL-NET-002` | DNS 正常但 Service 连接失败的场景 |
| 节点 NotReady 影响 DNS | `SKILL-NODE-001` (01-node-notready.md) | 节点级故障导致的间接 DNS 影响 |
| conntrack 竞态条件详解 | `domain-5-networking/` | Linux 内核 conntrack 在 UDP/DNAT 场景下的竞态条件技术细节 |
| NetworkPolicy 最佳实践 | `domain-5-networking/` | 如何正确配置 NetworkPolicy 以避免意外阻断 DNS 流量 |

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-03 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 12 个根因、10 个修复操作、6 个常见误诊模式 | 首批 Skill 库建设，基于工单分析 DNS 解析故障为网络类最高频问题 |

### 10.4 待补充的知识空白

以下领域在当前版本中覆盖有限，后续版本将增强：

1. **ExternalDNS 集成问题**: 使用 ExternalDNS 控制器将 Kubernetes Service/Ingress 同步到外部 DNS 提供商（Route53、CloudDNS 等）时的故障诊断
2. **Service Mesh DNS 行为**: Istio / Linkerd 等 Service Mesh 对 DNS 行为的影响（如 Istio 的 DNS 代理、智能 DNS 解析）
3. **自定义 CoreDNS 插件**: 使用自定义 CoreDNS 插件（如 `etcd`、`file`、`secondary`）时的特定故障模式
4. **多集群 DNS**: 跨集群 DNS 解析（如 Submariner、Liqo 或 CoreDNS multicluster 插件）的故障诊断
5. **Windows 节点 DNS**: Windows 容器节点的 DNS 配置和故障差异（kube-proxy for Windows、HNS DNS 策略）
6. **DNS 安全**: DNSSEC 验证、DNS 投毒防护、CoreDNS 安全加固相关的诊断能力