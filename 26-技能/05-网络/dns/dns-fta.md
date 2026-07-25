---
title: DNS 异常故障树分析 (skills)
description: '### 2. 上游解析诊断'
summary: '### 2. 上游解析诊断'
category: general
tags:
- k8s
- coredns
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- DNS 异常故障树分析 是什么
- 如何 DNS 异常故障树分析
trigger_keywords:
- DNS
- 异常故障树分析
prerequisites:
- kubectl-basics
fta_id: FTA-DNS-001
component: Dns
severity: critical
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "DNS 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get [[Pods|pods]] -n kube-system -l k8s-app=kube-dns -o jsonpath='{range .items[?(@.status.phase!=\'Running\')]} {.metadata.name}{\'\n\'}{end}' 显示 CoreDNS 异常 --> ..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/FTA故障树/list/dns-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# DNS 异常故障树分析

### 诊断命令快速参考表

### 1. CoreDNS 状态诊断

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|----------|--------------|------|
| CORE1A | OOMKilled | `kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{range .items[*]}{.status.containerStatuses[*].lastState.terminated.reason}{"\n"}{end}'` | `OOMKilled` | 确认 CoreDNS 内存溢出 |
| CORE1B | CrashLoopBackOff | `kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{range .items[*]}{.status.containerStatuses[*].state.waiting.reason}{"\n"}{end}'` | `CrashLoopBackOff` | 确认容器反复崩溃 |
| CORE1C | 被节点驱逐 | `kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{range .items[*]}{.status.reason}{"\n"}{end}'` | `Evicted` | 确认被驱逐 |
| CORE2A | kube-dns Service 不存在 | `kubectl get svc kube-dns -n kube-system -o name 2>/dev/null || echo "NOT_FOUND"` | `NOT_FOUND` | 确认 Service 缺失 |
| CORE2B | ClusterIP 不可达 | `kubectl run dns-test --rm -i --restart=Never --image=busybox -- nslookup kubernetes.default 2>&1` | `connection timed out|no servers` | 确认 DNS 服务不可达 |
| CORE2C | DNS 端口被占用 | `kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100 | grep -i "address already in use"` | `address already in use` | 确认端口冲突 |
| CORE3 | 插件加载失败 | `kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100 | grep -iE "plugin.*failed|failed to load"` | `plugin.*failed|failed to load` | 确认插件加载问题 |

### 2. 上游解析诊断

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|----------|--------------|------|
| UP1A | 上游 DNS 服务异常 | `kubectl exec -n kube-system -it $(kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{.items[0].metadata.name}') -- cat /etc/resolv.conf` | 查看上游 DNS 配置 | 获取上游 DNS 地址 |
| UP1B | 防火墙拦截 | `kubectl run dns-test --rm -i --restart=Never --image=busybox -- nc -zv -w 3 ${UPSTREAM_DNS} 53 2>&1` | `Connection timed out|no route` | 确认防火墙阻断 |
| UP1C | forward 配置地址错误 | `kubectl get cm coredns -n kube-system -o jsonpath='{.data.Corefile}' | grep -A2 "forward"` | forward 配置内容 | 检查上游配置 |
| UP2 | 上游超时/丢包 | `kubectl logs
...(截断)

## 生产案例

### 案例 1: CoreDNS Pod OOMKilled 导致集群 DNS 全面不可用

| 时间 | 事件 |
|------|------|
| 09:15 | 业务高峰期 DNS 查询量暴增 3x |
| 09:16 | CoreDNS Pod OOMKilled，剩余 Pod 过载 |
| 09:17 | 全集群服务发现失败，业务 503 |
| 09:20 | 🟡 扩容 CoreDNS replicas 2→6，调高 memory limit 170Mi→512Mi |
| 09:25 | DNS 恢复，业务逐步恢复 |

**根因**: CoreDNS 默认 memory limit 170Mi，业务增长后未同步调整。启用 `autopath` 插件减少无效查询。

### 案例 2: ndots:5 导致外部域名解析超时

**现象**: Pod 内访问 `api.external.com` 延迟 10s+，但 `api.external.com.` 正常。

**诊断**: `kubectl exec pod -- cat /etc/resolv.conf` → ndots:5 → 先尝试 4 次 search 域拼接

**修复**: 🟢 Deployment 中设置 `dnsConfig.options: [{name: ndots, value: "2"}]`，外部域名用 FQDN 加点结尾

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 集群 DNS 完全不可用 | 立即重启 CoreDNS + 扩容 |
| P1 | 部分域名解析失败 | 15min 内检查 Corefile 配置 |
| P2 | 解析延迟偏高 | 优化 ndots/cache 配置 |

## 面试要点

1. **Q: CoreDNS 的 Corefile 配置结构是怎样的？**
   A: Corefile 采用 server-block 结构，每个 block 定义监听域+插件链。典型: `.:53 { errors; health; kubernetes cluster.local; forward . /etc/resolv.conf; cache 30; loop; reload; loadbalance }`。

2. **Q: Pod DNS 策略 ClusterFirst 的解析顺序是什么？**
   A: 先查 search 域拼接(cluster.local→svc.cluster.local→namespace.svc.cluster.local)，ndots 决定何时直接解析原始域名。外部域名建议用 FQDN+点结尾绕过 search。

3. **Q: 如何监控 CoreDNS 性能？**
   A: 启用 prometheus 插件暴露 :9153 指标，关注 coredns_dns_request_duration_seconds、coredns_cache_misses、coredns_forward_request_duration_seconds。

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[ts-networking|网络故障排查]]

## Related

- [[coredns]] — CoreDNS
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[21-生态参考/03-领域索引/terway-index.md|Terway 知识图谱索引]]
- [[21-生态参考/03-领域索引/network-index.md|Network 网络知识图谱索引]]
- [[21-生态参考/03-领域索引/dns-index.md|DNS 知识图谱索引]]


<!-- risk-assessed -->
