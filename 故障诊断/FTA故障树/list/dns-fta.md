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
sources: ["故障诊断/topic-fta/list/dns-fta.md"]
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

### 案例1: CoreDNS OOMKilled 导致全集群 DNS 解析失败

**时间线**:
- 10:30 业务上线大量 Service，DNS 查询量激增 3x
- 10:35 CoreDNS Pod 内存超过 170Mi limit，触发 OOMKilled
- 10:35-10:38 所有 Pod 内 DNS 解析超时，服务间调用失败
- 10:38 CoreDNS 重启，但缓存冷启动导致延迟高
- 10:45 完全恢复

**根因链**:
```
Service数量激增 → DNS查询量超过缓存容量 → 内存持续增长
→ 超过memory limit → OOMKilled → 全集群DNS中断
```

**修复**:
```bash
# 🟡 调高 CoreDNS 内存限制
kubectl patch deployment coredns -n kube-system -p '{"spec":{"template":{"spec":{"containers":[{"name":"coredns","resources":{"limits":{"memory":"512Mi"},"requests":{"memory":"256Mi"}}}]}}}}'
# 🟢 验证 CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
```

### 案例2: ndots 配置导致外部域名解析慢

**现象**: Pod 内访问外部 API 延迟 5-10s，`nslookup api.external.com` 正常但应用内 HTTP 调用慢

**根因**: `/etc/resolv.conf` 中 `ndots:5`，外部域名先尝试 5 个 search domain 后缀才回退到绝对解析

**修复**:
```yaml
# 🟡 Pod DNS 策略优化
spec:
  dnsConfig:
    options:
    - name: ndots
      value: "2"
    - name: single-request-reopen
```

## 预防与监控

### 告警规则

```yaml
groups:
- name: dns-alerts
  rules:
  - alert: CoreDNSDown
    expr: up{job="coredns"} == 0
    for: 1m
    labels:
      severity: critical
  - alert: CoreDNSHighLatency
    expr: histogram_quantile(0.99, rate(coredns_dns_request_duration_seconds_bucket[5m])) > 0.5
    for: 5m
    labels:
      severity: warning
  - alert: CoreDNSMemoryHigh
    expr: container_memory_working_set_bytes{container="coredns"} / container_spec_memory_limit_bytes{container="coredns"} > 0.85
    for: 5m
    labels:
      severity: warning
```

### 预防措施

| 措施 | 说明 | 优先级 |
|------|------|--------|
| CoreDNS 多副本 + 反亲和 | 至少 2 副本分布在不同节点 | P0 |
| 内存限制充足 | 根据 Service 数量调整，建议 512Mi+ | P0 |
| NodeLocal DNSCache | 节点级缓存减少 CoreDNS 压力 | P1 |
| ndots 优化 | 外部调用多的 Pod 设置 ndots:2 | P1 |

## 面试要点

1. **Q: K8s 集群 DNS 解析的完整链路？**
   A: Pod /etc/resolv.conf → NodeLocal DNS(如有) → CoreDNS Service(ClusterIP) → CoreDNS Pod → 集群内直接解析 / 外部 forward 到上游 DNS

2. **Q: CoreDNS 性能优化方案？**
   A: 启用 NodeLocal DNSCache → 调整副本数(autoscaler) → 优化 forward 策略 → 启用 autopath 插件 → 合理设置缓存 TTL

3. **Q: DNS 解析慢的排查步骤？**
   A: 确认 ndots/search domain 配置 → 检查 CoreDNS 负载 → 查看上游 DNS 延迟 → 确认网络连通性(UDP 53) → 检查 conntrack 表

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[ts-networking|网络故障排查]]

## Related

- [[coredns]] — CoreDNS
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]
- [[生态参考/领域索引/dns-index.md|DNS 知识图谱索引]]

---

> 📄 **完整版本**: 本文件为精简版故障树速查。完整的故障树分析（含详细根因推理和决策路径）请查阅：
> [[归档/troubleshooting-diagnostics/FTA故障树/list/dns-fta.md|Dns FTA 完整版]]


<!-- risk-assessed -->
