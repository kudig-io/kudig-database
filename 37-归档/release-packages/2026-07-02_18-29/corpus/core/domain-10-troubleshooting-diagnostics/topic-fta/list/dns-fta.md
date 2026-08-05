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
sources: ["domain-10-troubleshooting-diagnostics/topic-fta/list/dns-fta.md"]
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

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[ts-networking|网络故障排查]]

## Related

- [[coredns]] — CoreDNS
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/network-index.md|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/领域索引/dns-index.md|DNS 知识图谱索引]]

---

> 📄 **完整版本**: 本文件为精简版故障树速查。完整的故障树分析（含详细根因推理和决策路径）请查阅：
> [[_archives/troubleshooting-diagnostics/FTA故障树/list/dns-fta.md|Dns FTA 完整版]]


<!-- risk-assessed -->
