---
title: 疑难问题系统性排查方法论：配置优先（Configuration-First） [topic-structural-trouble-shooting]
description: 'title: 疑难问题系统性排查方法论：配置优先（Configuration-First）'
summary: 'title: 疑难问题系统性排查方法论：配置优先（Configuration-First）'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- configuration
- etcd
- kubelet
- prometheus
- coredns
- daemonset
- ingress
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- 疑难问题系统性排查方法论：配置优先（Configuration-First） 是什么
- 如何 疑难问题系统性排查方法论：配置优先（Configuration-First）
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 疑难问题系统性排查方法论：配置优先（Configuration-First） 故障排查
- 疑难问题系统性排查方法论：配置优先（Configuration-First） 排障步骤
trigger_keywords:
- 疑难问题系统性排查方法论：配置优先
- Configuration-First
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 疑难问题系统性排查方法论：配置优先（Configuration-First）
description: '# 疑难问题系统性排查方法论：配置优先（Configuration-First）'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[etcd|etcd]]
- [[kubelet|kubelet]]
- [[Prometheus|prometheus]]
- coredns
- daemonset
- ingress
- gateway
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 10min
intent_queries:
- 疑难问题系统性排查方法论：配置优先（Configuration-First） 是什么
- 如何 疑难问题系统性排查方法论：配置优先（Configuration-First）
- 疑难问题系统性排查方法论：配置优先（Configuration-First） 故障排查
- 疑难问题系统性排查方法论：配置优先（Configuration-First） 排障步骤
trigger_keywords:
- 疑难问题系统性排查方法论：配置优先
- Configuration-First
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 疑难问题系统性排查方法论：配置优先（Configuration-First）

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-04 | **难度**: 高级 | **定位**: 跨组件方法论

---

<!-- chunk: 0. 方法论定位 -->## 0. 方法论定位

本文档定义了面向 Kubernetes **疑难问题**的系统性排查方法论——**配置优先（Configuration-First）**。

在生产环境的日常运维中，绝大多数问题可以通过标准排查流程快速定位。但当遇到**症状模糊、影响范围不清晰、多个组件交叉关联**的疑难问题时，SRE 工程师常常陷入"东查一下、西查一下"的无序排查状态，浪费大量时间在网络链路、内核参数等深层排查上，而忽略了最简单也最高频的根因——**配置错误**。

**核心主张**：遇到复杂疑难问题时，**先检查配置文件**，然后按照特定步骤进行深入分析。

## 与现有排查体系的关系

```
┌─────────────────────────────────────────────────────────────────────────┐
│  本文档：配置优先方法论（排查策略与思维框架）                                │
│  ├── 适用于：疑难问题、复杂问题、多组件交叉问题                              │
│  ├── 解决：排查顺序问题（先查什么？后查什么？）                              │
│  └── 输出：结构化排查路径 + 配置验证清单                                   │
├─────────────────────────────────────────────────────────────────────────┤
│  domain-10-troubleshooting-diagnostics/topic-fta/          → 为什么出问题（故障树因果分析，演绎法）                 │
│  domain-10-troubleshooting-diagnostics/topic-febm/         → 如何从证据推导结论（取证循证，归纳法）                 │
│  topic-structural/   → 具体怎么查（按组件的详细排查步骤）                    │
│  domain-10-troubleshooting-diagnostics/topic-skills/       → Agent 怎么做（自动化诊断-修复闭环）                  │
└─────────────────────────────────────────────────────────────────────────┘
```

| 维度 | 配置优先方法论 | FTA 故障树 | FEBM 取证 | Skills |
|------|-------------|-----------|----------|--------|
| **解决的问题** | 排查顺序与策略 | 根因定位模型 | 证据推导 | 自动化执行 |
| **核心思想** | 先简后繁、先配置后链路 | 演绎分解 | 归纳推理 | 诊断-修复闭环 |
| **使用时机** | 疑难问题排查入口 | 构建因果关系图 | 事后复盘 | Agent 运行时 |
| **互补关系** | 决定排查起点 | 决定排查路径 | 决定证据链 | 决定执行动作 |

---

<!-- chunk: 1. 方法论核心原则 -->## 1. 方法论核心原则

## 1.1 黄金法则：配置优先

> **在进行任何深入的系统级排查之前，必须先完成配置文件的检查和验证。**

**为什么配置优先？**

根据生产环境问题统计数据：

| 根因分类 | 占比 | 典型排查时间 | 典型修复时间 |
|---------|------|-------------|-------------|
| **配置错误** | ~45% | 5-15 分钟 | 2-5 分钟 |
| 资源不足 | ~20% | 10-30 分钟 | 5-15 分钟 |
| 版本/兼容性 | ~10% | 15-60 分钟 | 10-30 分钟 |
| 网络链路 | ~10% | 30-120 分钟 | 15-60 分钟 |
| 内核/系统 | ~8% | 60-240 分钟 | 30-120 分钟 |
| 未知/复合 | ~7% | 120+ 分钟 | 60+ 分钟 |

**近一半的问题根因是配置错误**，而配置检查是所有排查手段中成本最低、速度最快的。如果跳过配置检查直接深入网络/内核排查，平均会浪费 30-120 分钟。

## 1.2 排查顺序金字塔

```
                    ┌──────────┐
                    │  内核/系统 │  ← 最后排查（成本最高）
                    │  参数调优  │
                 ┌──┴──────────┴──┐
                 │   网络链路排查   │  ← 第四步
                 │ 抓包/traceroute │
              ┌──┴────────────────┴──┐
              │    资源与运行状态检查    │  ← 第三步
              │  CPU/内存/磁盘/Pod状态  │
           ┌──┴──────────────────────┴──┐
           │       版本与兼容性验证        │  ← 第二步
           │  K8s版本/组件版本/API变更     │
        ┌──┴──────────────────────────┴──┐
        │         配置文件检查与验证         │  ← 第一步（成本最低）
        │  YAML/ConfigMap/Corefile/参数    │
        └──────────────────────────────────┘
```

## 1.3 方法论五步法

```
Step 1: 配置验证 ──→ Step 2: 版本兼容 ──→ Step 3: 运行状态 ──→ Step 4: 网络链路 ──→ Step 5: 系统深层
  (5-15min)           (5-10min)           (10-20min)           (15-60min)          (30-120min)
  
  ↓ 每一步结束时评估：                                                              
  ├── 根因已找到？→ 修复 → 验证 → 结束                                               
  └── 未找到？→ 进入下一步                                                          
```

**关键约束**：
- **禁止跳步**：不允许跳过 Step 1 直接进入 Step 4，除非有明确证据排除配置问题
- **证据驱动**：每一步的排除必须有明确的命令输出或日志证据
- **时间门控**：每一步有建议的时间上限，超时应重新评估方向

---

<!-- chunk: 2. Step 1：配置文件检查与验证 -->## 2. Step 1：配置文件检查与验证

这是整个方法论的核心步骤。对于 Kubernetes 中的任何组件，配置检查应覆盖以下层次：

## 2.1 配置检查四层模型

```
┌──────────────────────────────────────────────────────┐
│  Layer 4: 应用层配置                                    │
│  ConfigMap / Secret / 环境变量 / 命令行参数               │
├──────────────────────────────────────────────────────┤
│  Layer 3: Kubernetes 资源配置                           │
│  Deployment / Service / Ingress / NetworkPolicy YAML   │
├──────────────────────────────────────────────────────┤
│  Layer 2: 集群基础设施配置                               │
│  kubelet 参数 / kube-proxy 配置 / CNI 配置              │
├──────────────────────────────────────────────────────┤
│  Layer 1: 节点/系统配置                                 │
│  /etc/resolv.conf / sysctl / 内核模块                   │
└──────────────────────────────────────────────────────┘
```

## 2.2 通用配置检查清单

每个组件的配置检查应回答以下问题：

| # | 检查项 | 检查内容 | 判定标准 |
|---|--------|---------|---------|
| C1 | **语法正确性** | 配置文件是否有语法错误 | 无解析错误、无 YAML 缩进问题 |
| C2 | **完整性** | 所有必需字段是否存在 | 必填字段均已配置 |
| C3 | **一致性** | 多个配置之间是否矛盾 | selector、port、名称等跨资源引用一致 |
| C4 | **版本适配** | 配置是否适用于当前 K8s 版本 | API version 正确、无已废弃字段 |
| C5 | **变更追溯** | 近期是否有配置变更 | 检查 git log / audit log / ConfigMap 修改时间 |
| C6 | **默认值陷阱** | 隐式默认值是否符合预期 | 确认关键字段未依赖不安全的默认值 |
| C7 | **权限与引用** | 配置引用的资源是否存在且可访问 | Secret/ConfigMap 存在、RBAC 允许访问 |

---

<!-- chunk: 3. 实战案例：CoreDNS 疑难问题的配置优先排查 -->## 3. 实战案例：CoreDNS 疑难问题的配置优先排查

以下以 CoreDNS 问题为完整示例，演示配置优先方法论的具体应用。

## 3.0 场景描述

**问题现象**：集群中部分 Pod 间歇性出现 DNS 解析失败，外部域名解析偶尔超时，应用日志报 `could not resolve host` 和 `i/o timeout`，但 CoreDNS Pod 状态显示 Running。

**疑难点**：CoreDNS 没有明显异常（未 Crash、未 OOM），症状间歇性出现，容易误导排查方向进入网络链路排查。

## 3.1 Step 1：CoreDNS 配置文件检查（首要步骤）

> **核心规定：在进行全面的网络链路排查之前，首先要检查和验证 CoreDNS 的配置文件是否正确。**

## 3.1.1 检查 Corefile（CoreDNS 核心配置）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml
```
**必须验证的配置项**：

| # | 配置项 | 正确示例 | 常见错误 | 影响 |
|---|--------|---------|---------|------|
| CF1 | `kubernetes` 插件域名 | `kubernetes cluster.local in-addr.arpa ip6.arpa` | 域名拼写错误：`cluser.local` | 集群内部 Service 全部解析失败 |
| CF2 | `forward` 上游 DNS | `forward . /etc/resolv.conf` 或 `forward . 8.8.8.8 8.8.4.4` | 指向不可达的 DNS 服务器 | 外部域名解析全部失败 |
| CF3 | `loop` 插件 | 必须存在 `loop` | 缺少 loop 检测 | DNS 转发环路导致 CoreDNS 崩溃 |
| CF4 | `cache` 插件配置 | `cache 30` | 缓存时间过短或缺失 | DNS 查询量激增，性能下降 |
| CF5 | `pods` 参数 | `pods insecure` 或 `pods verified` | `pods disabled` 导致 Pod A/AAAA 记录缺失 | Headless Service 解析异常 |
| CF6 | 插件顺序 | 遵循 CoreDNS 插件链顺序 | 插件顺序错误 | 不可预期的解析行为 |
| CF7 | 自定义 zone 配置 | `example.com { ... }` | zone 块语法错误、花括号不匹配 | CoreDNS 启动失败或部分域名解析异常 |

**标准 Corefile 参考**：

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

## 3.1.2 检查 Pod DNS 配置（resolv.conf）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查目标 Pod 的 DNS 配置
kubectl exec <problem-pod> -- cat /etc/resolv.conf
```
**必须验证的配置项**：

| # | 配置项 | 正确值 | 常见错误 | 影响 |
|---|--------|------|---------|------|
| RC1 | `nameserver` | CoreDNS Service ClusterIP（通常 `10.96.0.10`） | 指向错误 IP 或节点 DNS | 所有 DNS 查询发往错误目标 |
| RC2 | `search` | `<ns>.svc.cluster.local svc.cluster.local cluster.local` | search 域缺失或错误 | 短域名无法解析 |
| RC3 | `ndots` | `ndots:5`（默认）或自定义值 | ndots 设置不当 | 外部域名解析多余查询导致慢 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证 kube-dns Service ClusterIP
kubectl get svc kube-dns -n kube-system -o jsonpath='{.spec.clusterIP}'
```
## 3.1.3 检查 Deployment/Service 配置一致性

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 CoreDNS Deployment 配置
kubectl get deployment coredns -n kube-system -o yaml | grep -A 5 'args|configMap|resources|replicas'

# 检查 CoreDNS Service selector 与 Pod label 是否匹配
kubectl get svc kube-dns -n kube-system -o jsonpath='{.spec.selector}'
kubectl get pods -n kube-system -l k8s-app=kube-dns --show-labels

# 检查 Endpoints 是否正常填充
kubectl get endpoints kube-dns -n kube-system
```
## 3.1.4 检查近期配置变更

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 CoreDNS ConfigMap 最后修改时间
kubectl get configmap coredns -n kube-system -o jsonpath='{.metadata.resourceVersion}'

# 查看 kube-system namespace 近期 events
kubectl get events -n kube-system --sort-by='.lastTimestamp' | grep -i dns

# 如果有 Git 管理的集群配置，检查近期变更
# git log --since="24 hours ago" -- '**/coredns*' '**/dns*'
```
## 3.1.5 Step 1 检查结论模板

完成 Step 1 后，填写以下结论：

```
【Step 1 配置检查结论】
- Corefile 语法: ✅ 正确 / ❌ 发现问题：___
- Corefile 插件配置: ✅ 正确 / ❌ 发现问题：___
- forward 上游 DNS: ✅ 可达 / ❌ 发现问题：___
- resolv.conf: ✅ 正确 / ❌ 发现问题：___
- Service/Endpoints 一致性: ✅ 正确 / ❌ 发现问题：___
- 近期配置变更: ✅ 无变更 / ⚠️ 发现变更：___

→ 根因已定位？[是：修复并验证] / [否：进入 Step 2]
```

## 3.2 Step 2：版本与兼容性验证

仅当 Step 1 未发现配置问题时，进入此步骤。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 CoreDNS 版本
kubectl get deployment coredns -n kube-system -o jsonpath='{.spec.template.spec.containers[0].image}'

# 检查 Kubernetes 版本
kubectl version --short
```
**版本兼容矩阵**：

| K8s 版本 | 推荐 CoreDNS 版本 | 关键变更 |
|---------|------------------|---------|
| v1.28 | v1.10.1 | 支持 DNS-over-TLS |
| v1.29 | v1.11.1 | 改进的 cache 插件 |
| v1.30 | v1.11.1+ | 支持 structured logging |
| v1.31 | v1.11.3 | 性能优化 |
| v1.32 | v1.11.3+ | 稳定性增强 |

**检查要点**：
- CoreDNS 版本是否与当前 K8s 版本兼容
- 是否使用了当前版本已废弃的插件或参数
- 近期是否进行过 K8s 或 CoreDNS 升级

## 3.3 Step 3：运行状态与资源检查

仅当 Step 1-2 未发现问题时，进入此步骤。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# CoreDNS Pod 资源使用情况
kubectl top pods -n kube-system -l k8s-app=kube-dns

# CoreDNS Pod 详细状态
kubectl describe pods -n kube-system -l k8s-app=kube-dns

# CoreDNS 日志检查（关注 error/warning）
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=500 | grep -iE "error|warn|fail|timeout|refused"

# CoreDNS Prometheus 指标（如果已配置）
# 检查 SERVFAIL 率、延迟、请求量
kubectl exec -n kube-system <coredns-pod> -- wget -qO- http://localhost:9153/metrics 2>/dev/null | grep -E "coredns_dns_responses_total|coredns_dns_request_duration"
```
**检查要点**：
- CoreDNS 是否 CPU/内存资源不足（throttling / OOM 风险）
- CoreDNS 副本数是否足够（建议至少 2 副本）
- CoreDNS 日志中是否有持续性错误
- SERVFAIL 比率是否异常偏高

## 3.4 Step 4：网络链路排查

**仅当 Step 1-3 均未发现问题时，才进入网络链路排查。**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 从问题 Pod 直接测试到 CoreDNS Pod IP 的连通性
COREDNS_POD_IP=$(kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{.items[0].status.podIP}')
kubectl exec <problem-pod> -- nslookup kubernetes.default $COREDNS_POD_IP

# 检查 kube-proxy 规则（iptables 模式）
# 需要节点 SSH 访问
iptables-save | grep kube-dns

# 检查是否有 NetworkPolicy 阻断 DNS 流量
kubectl get networkpolicy -A -o yaml | grep -A 10 "port: 53"

# 抓包分析 DNS 流量（需要节点访问）
# tcpdump -i any port 53 -nn -c 100

# 检查 conntrack 表（间歇性 DNS 失败的常见根因）
# conntrack -S | grep drop
# sysctl net.netfilter.nf_conntrack_count net.netfilter.nf_conntrack_max
```
## 3.5 Step 5：系统深层排查

**仅当 Step 1-4 均未发现问题时，才进入系统深层排查。**

```bash
# 内核 conntrack 竞态条件检查
# sysctl -a | grep conntrack

# IPVS/iptables 规则完整性
# ipvsadm -ln | grep <kube-dns-clusterip>

# 节点 /etc/resolv.conf（影响 CoreDNS forward 上游）
# cat /etc/resolv.conf

# 系统级 DNS 缓存（systemd-resolved 等）
# systemd-resolve --status

# 网络命名空间隔离问题
# nsenter --net=<target-ns> -- dig kubernetes.default.svc.cluster.local
```

---

<!-- chunk: 4. 配置优先清单：其他常见组件 -->## 4. 配置优先清单：其他常见组件

配置优先方法论不仅适用于 CoreDNS，以下是其他组件的 Step 1 配置检查要点：

## 4.1 Ingress/Gateway 疑难排查

| 配置检查项 | 检查命令 | 常见配置错误 |
|-----------|---------|------------|
| Ingress 规则 | `kubectl get ingress -o yaml` | host/path 配置错误、TLS secret 引用不存在 |
| IngressClass | `kubectl get ingressclass` | 缺少默认 IngressClass 或指定了错误的 class |
| Backend Service | `kubectl get svc <backend>` | Service 端口与 Ingress 配置不匹配 |
| TLS 证书 | `kubectl get secret <tls-secret> -o yaml` | 证书过期、域名不匹配、格式错误 |

## 4.2 Service 连通性疑难排查

| 配置检查项 | 检查命令 | 常见配置错误 |
|-----------|---------|------------|
| Selector 匹配 | `kubectl get svc <svc> -o jsonpath='{.spec.selector}'` | Selector 与 Pod label 不匹配 |
| 端口映射 | `kubectl get svc <svc> -o yaml` | targetPort 与容器端口不一致 |
| Endpoints 填充 | `kubectl get endpoints <svc>` | Endpoints 为空（Selector 错误或 Pod 未就绪） |
| SessionAffinity | `kubectl get svc <svc> -o jsonpath='{.spec.sessionAffinity}'` | 意外的会话亲和性配置 |

## 4.3 Pod 启动失败疑难排查

| 配置检查项 | 检查命令 | 常见配置错误 |
|-----------|---------|------------|
| 镜像名称与标签 | `kubectl get pod <pod> -o jsonpath='{.spec.containers[*].image}'` | 镜像名拼写错误、标签不存在 |
| 资源请求/限制 | `kubectl get pod <pod> -o yaml | grep -A 5 resources` | requests 超出节点容量、limits 过低 |
| Volume 挂载 | `kubectl describe pod <pod> | grep -A 10 Volumes` | ConfigMap/Secret/PVC 不存在或名称错误 |
| 环境变量引用 | `kubectl get pod <pod> -o yaml | grep -A 3 envFrom` | 引用的 ConfigMap/Secret 不存在 |
| SecurityContext | `kubectl get pod <pod> -o yaml | grep -A 10 securityContext` | 与 PSA 策略冲突 |

## 4.4 etcd 疑难排查

| 配置检查项 | 检查命令 | 常见配置错误 |
|-----------|---------|------------|
| 集群成员配置 | `etcdctl member list` | 成员 URL 不一致或指向已下线节点 |
| 证书配置 | 检查 etcd 启动参数中的证书路径 | 证书过期、CA 不匹配 |
| 数据目录 | 检查 `--data-dir` 参数 | 磁盘满、权限错误 |
| 快照与压缩 | 检查 `--auto-compaction-*` 参数 | 未启用自动压缩导致数据库膨胀 |

---

<!-- chunk: 5. Agent 集成指南 -->## 5. Agent 集成指南

## 5.1 方法论在 Agent 工作流中的位置

```
工单/告警输入
    │
    ▼
┌──────────────────────┐
│ Skill 路由（症状匹配）  │  ← domain-10-troubleshooting-diagnostics/topic-skills/ YAML front matter
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ 配置优先方法论           │  ← 本文档（排查策略决策）
│ Step 1: 配置检查        │     Agent 在 Skill 的 Phase 1 诊断中
│ Step 2: 版本兼容        │     优先执行配置相关检查
│ ...                     │
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ FTA 故障树遍历         │  ← domain-10-troubleshooting-diagnostics/topic-fta/ 因果关系模型
└──────────┬───────────┘
           │
           ▼
┌──────────────────────┐
│ 修复 → 验证 → 闭环    │  ← domain-10-troubleshooting-diagnostics/topic-skills/ Section 6-7
└──────────────────────┘
```

## 5.2 Agent 执行时的配置检查优先级

当 Agent 执行任何 Skill 的 Phase 1（快速诊断）时，应遵循以下优先级：

```yaml
phase_1_priority:
  - step: "配置文件完整性检查"
    priority: 1
    description: "检查核心配置文件语法、完整性、一致性"
    time_budget: "3min"
  - step: "配置变更追溯"
    priority: 2
    description: "检查近期配置变更（ConfigMap、Deployment、DaemonSet）"
    time_budget: "2min"
  - step: "运行状态快速检查"
    priority: 3
    description: "Pod 状态、资源使用、日志错误"
    time_budget: "3min"
  - step: "连通性基础验证"
    priority: 4
    description: "基本的网络连通性测试"
    time_budget: "2min"
```

## 5.3 配置检查自动化模板

Agent 在执行配置检查时可使用以下结构化输出：

```json
{
  "methodology": "configuration-first",
  "component": "coredns",
  "step": 1,
  "checks": [
    {
      "id": "CF1",
      "name": "Corefile 语法验证",
      "command": "kubectl get configmap coredns -n kube-system -o yaml",
      "result": "pass|fail|warning",
      "evidence": "...",
      "finding": "..."
    }
  ],
  "conclusion": {
    "root_cause_found": true,
    "root_cause_id": "CF2",
    "confidence": 0.90,
    "next_step": "fix|step2"
  }
}
```

---

<!-- chunk: 6. 反模式与陷阱 -->## 6. 反模式与陷阱

## 6.1 常见反模式

| # | 反模式 | 描述 | 后果 | 正确做法 |
|---|--------|------|------|---------|
| A1 | **跳过配置直接抓包** | 看到网络相关现象就立即 tcpdump | 浪费 30-120 分钟，可能根因只是一个 typo | 先检查配置，排除配置问题后再抓包 |
| A2 | **症状驱动而非系统性** | 根据症状猜测根因，东查一下西查一下 | 遗漏真正的根因，延长问题时间 | 按五步法顺序排查，每步有明确的检查清单 |
| A3 | **不记录排除证据** | 检查了但没记录结果 | 重复排查、交接困难、事后复盘无据可查 | 每步填写检查结论模板 |
| A4 | **忽略近期变更** | 不查变更历史就开始排查 | 70% 的问题与近期变更相关 | Step 1 必须包含变更追溯 |
| A5 | **默认值盲区** | 假设默认配置没问题 | Kubernetes 默认值不一定适合所有场景 | 明确检查关键参数的默认值 |

## 6.2 CoreDNS 特有陷阱

| 陷阱 | 现象 | 根因 | 排查捷径 |
|------|------|------|---------|
| `ndots:5` 性能陷阱 | 外部域名解析慢（5-10 秒） | 默认 ndots=5 导致短域名先经过 5 次搜索域扩展 | 检查 resolv.conf 的 ndots，对频繁访问外域的 Pod 设置 ndots=2 |
| `loop` 插件缺失 | CoreDNS 反复 CrashLoopBackOff | 节点 /etc/resolv.conf 指向 127.0.0.1 导致转发环路 | 检查 Corefile 是否包含 `loop` 插件 |
| `forward` 指向自身 | CoreDNS CPU 100%、超时 | forward 上游配置指向了 kube-dns ClusterIP | 检查 forward 目标不是 kube-dns 自身 |
| ConfigMap 未生效 | 修改配置后问题依旧 | CoreDNS 需要 `reload` 插件才能热加载，或需手动重启 | 确认 Corefile 包含 `reload` 插件，或手动 rollout restart |

---

<!-- chunk: 7. 配置优先排查检查表（Checklist） -->## 7. 配置优先排查检查表（Checklist）

以下检查表可在实际排查中直接使用，按顺序逐项完成：

## 7.1 通用配置检查表

- [ ] **C1** 核心配置文件获取并审查（语法、完整性）
- [ ] **C2** 配置文件中所有引用的资源存在且可访问（Secret、ConfigMap、Service）
- [ ] **C3** 跨资源配置一致性验证（selector、port、name 匹配）
- [ ] **C4** API 版本与 K8s 版本兼容性确认
- [ ] **C5** 近期配置变更追溯（24 小时内）
- [ ] **C6** 关键参数默认值确认（非依赖隐式默认值）
- [ ] **C7** 多副本/多实例配置一致性

## 7.2 CoreDNS 专项检查表

- [ ] **CF1** Corefile 语法正确、插件链顺序正确
- [ ] **CF2** `kubernetes` 插件域名 `cluster.local` 拼写正确
- [ ] **CF3** `forward` 上游 DNS 可达且响应正常
- [ ] **CF4** `loop` 插件存在（防止转发环路）
- [ ] **CF5** `cache` 插件配置合理
- [ ] **CF6** `reload` 插件存在（支持热加载）
- [ ] **CF7** Pod resolv.conf 中 nameserver 指向正确的 kube-dns ClusterIP
- [ ] **CF8** Pod resolv.conf 中 search 域和 ndots 配置合理
- [ ] **CF9** kube-dns Service selector 与 CoreDNS Pod label 匹配
- [ ] **CF10** kube-dns Endpoints 正常填充
- [ ] **CF11** CoreDNS Deployment 副本数 ≥ 2
- [ ] **CF12** CoreDNS 资源 requests/limits 配置合理

---

<!-- chunk: 8. 关联资源 -->## 8. 关联资源

| 资源 | 路径 | 关系 |
|------|------|------|
| **DNS 故障树分析** | [domain-10-troubleshooting-diagnostics/topic-fta/list/dns-fta.md](../domain-10-troubleshooting-diagnostics/topic-fta/list/dns-fta.md) | FTA 因果分析模型 |
| **DNS 结构化排查指南** | [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/03-networking/02-dns-troubleshooting.md|02-dns-troubleshooting]].md](./03-networking/02-dns-troubleshooting.md) | 详细排查步骤 |
| **DNS 故障排查（domain-12）** | [[domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/26-dns-troubleshooting.md|26-dns-troubleshooting]].md](../domain-10-troubleshooting-diagnostics/26-dns-troubleshooting.md) | 按组件分类的完整指南 |
| **DNS Skill（Agent 可执行）** | [domain-10-troubleshooting-diagnostics/topic-skills/04-dns-resolution-failure.md](../domain-10-troubleshooting-diagnostics/topic-skills/04-dns-resolution-failure.md) | Agent 运行时 Runbook |
| **FEBM 取证方法论** | [domain-10-troubleshooting-diagnostics/topic-febm/](../domain-10-troubleshooting-diagnostics/topic-febm/) | 事后复盘取证分析 |
| **FTA 方法论合集** | [domain-10-troubleshooting-diagnostics/topic-fta/fta-methodology-and-agentic-practices.md](../domain-10-troubleshooting-diagnostics/topic-fta/fta-methodology-and-agentic-practices.md) | 故障树分析完整方法论 |

---

<!-- chunk: 9. 版本历史 -->## 9. 版本历史

| 版本 | 日期 | 变更内容 |
|------|------|---------|
| 1.0 | 2026-04 | 初始版本：配置优先方法论、CoreDNS 完整示例、Agent 集成指南 |

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/MOC.md|topic-structural-trouble-shooting MOC]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/README.md|Kubernetes 结构化故障排查知识库]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/09-dra-troubleshooting.md|DRA（动态资源分配）故障排查指南]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-etcd-maintenance.md|etcd 维护专项文档]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/symptom-mapping-layer.md|症状快速映射层 (Symptom-SOP-RootCause Mapping)]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-etcd-maintenance.md|10-etcd-maintenance]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/symptom-mapping-layer.md|symptom-mapping-layer]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/09-dra-troubleshooting.md|09-dra-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-etcd-maintenance.md|10-etcd-maintenance]]

```

<!-- risk-assessed -->
