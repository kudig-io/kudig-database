---
title: Service 异常故障树分析 (skills)
description: '| KP2B | 配置错误 | `kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50
  | grep -iE "error|invalid|failed"` | 错误日志 | 检查配置问题'
summary: '| KP2B | 配置错误 | `kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50
  | grep -iE "error|invalid|failed"` | 错误日志 | 检查配置问题'
category: skills
tags:
- k8s
- fta
- troubleshooting
- calico
- webhook
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Service 异常故障树分析 是什么
- 如何 Service 异常故障树分析
trigger_keywords:
- Service
- 异常故障树分析
prerequisites:
- kubectl-basics
- cni-basics
fta_id: FTA-SERVICE-001
component: Service
severity: high
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Service|Service]] 异常故障树分析

### 诊断命令快速参考表

### 1. Endpoint/EndpointSlice 诊断

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|----------|--------------|------|
| EP1 | 无可用 Endpoint | `kubectl get endpoints ${SERVICE_NAME} -n ${NAMESPACE} -o jsonpath='{.subsets[*].addresses[*].ip}'` | IP 地址列表 | 空表示无 Endpoint |
| EP2 | EndpointSlice 不同步 | `kubectl get endpointslice -n ${NAMESPACE} -l kubernetes.io/service-name=${SERVICE_NAME} -o wide` | EndpointSlice 列表 | 检查同步状态 |
| EP3 | Endpoint 地址错误 | `kubectl get endpoints ${SERVICE_NAME} -n ${NAMESPACE} -o yaml | grep -A5 "addresses"` | Pod IP 列表 | 验证 IP 正确性 |
| EP1A | Pod 不健康 | `kubectl get [[Pods|pods]] -n ${NAMESPACE} -l ${SELECTOR} -o jsonpath='{range .items[*]}{.metadata.name}: {.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}'` | `True/False` | 检查 Ready 状态 |
| EP1B | Selector 不匹配 | `kubectl get svc ${SERVICE_NAME} -n ${NAMESPACE} -o jsonpath='{.spec.selector}' && kubectl get pods -n ${NAMESPACE} --show-labels` | 标签匹配情况 | 验证 selector 匹配 |

### 2. kube-proxy 诊断

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|----------|--------------|------|
| KP1A | 规则丢失 | `kubectl exec -n kube-system $(kubectl get pods -n kube-system -l k8s-app=kube-proxy -o jsonpath='{.items[0].metadata.name}') -- iptables -t nat -L KUBE-SERVICES -n | grep ${SERVICE_CLUSTER_IP}` | NAT 规则 | 检查 Service 规则 |
| KP1B | 规则冲突 | `kubectl exec -n kube-system $(kubectl get pods -n kube-system -l k8s-app=kube-proxy -o jsonpath='{.items[0].metadata.name}') -- iptables -t nat -L -n | grep -c ${SERVICE_CLUSTER_IP}` | 规则数量 | >1 表示冲突 |
| KP1C | conntrack 表满 | `kubectl get --raw /api/v1/nodes/${NODE_NAME}/proxy/metrics | grep nf_conntrack` | conntrack 使用率 | 接近 max 表示满 |
| KP2A | 进程崩溃 | `kubectl get pods -n kube-system -l k8s-app=kube-proxy -o jsonpath='{range .items[*]}{.metadata.name}: restarts={.status.containerStatuses[0].restartCount}{"\n"}{end}'` | 重启次数 | >0 表示有重启 |
| KP2B | 配置错误 | `kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50 | grep -iE "error|invalid|failed"` | 错误日志 | 检查配置问题
...(截断)

## 生产案例

### 案例1: Endpoint 不同步导致服务中断

**时间线**:
- 14:02 运维执行 Deployment 滚动更新（v2.1→v2.2）
- 14:03 新 Pod 启动但 readinessProbe 超时（数据库连接池未预热）
- 14:05 旧 Pod 被终止，新 Pod 仍未 Ready → Endpoint 列表为空
- 14:05-14:12 所有流量返回 502，持续 7 分钟
- 14:12 新 Pod Ready，Endpoint 恢复，流量恢复

**根因链**:
```
滚动更新 → 新Pod readinessProbe失败 → 旧Pod被终止(maxSurge=1,maxUnavailable=0配置错误)
→ Endpoint为空 → kube-proxy清除iptables规则 → 502
```

**修复**:
```bash
# 🟡 修正滚动更新策略
kubectl patch deployment ${DEPLOY} -n ${NS} -p '{"spec":{"strategy":{"rollingUpdate":{"maxUnavailable":"25%"}}}}'
# 🟢 验证 Endpoint 恢复
kubectl get endpoints ${SVC} -n ${NS} -w
```

### 案例2: kube-proxy conntrack 表满

**现象**: 间歇性连接超时，`dmesg` 出现 `nf_conntrack: table full, dropping packet`

**根因**: 高并发场景下 conntrack 默认 65536 不够，UDP 超时 30s 导致条目堆积

**修复**:
```bash
# 🔴 调整 conntrack 表大小（节点级）
sysctl -w net.netfilter.nf_conntrack_max=262144
sysctl -w net.netfilter.nf_conntrack_udp_timeout_stream=10
# 🟢 验证
conntrack -C
```

## 预防与监控

### 告警规则

```yaml
# Prometheus 告警
groups:
- name: service-alerts
  rules:
  - alert: ServiceNoEndpoints
    expr: kube_endpoint_address_available == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "Service {{ $labels.namespace }}/{{ $labels.endpoint }} 无可用 Endpoint"
  - alert: KubeProxyConntrackNearFull
    expr: node_nf_conntrack_entries / node_nf_conntrack_entries_limit > 0.8
    for: 5m
    labels:
      severity: warning
```

### 预防措施

| 措施 | 说明 | 优先级 |
|------|------|--------|
| 配置合理的滚动更新策略 | maxUnavailable≥1 避免全部不可用 | P0 |
| readinessProbe 预热 | 应用启动后预热连接池再标记 Ready | P0 |
| conntrack 容量规划 | 按峰值 QPS × 平均连接时长估算 | P1 |
| EndpointSlice 监控 | 监控 Endpoint 变化事件 | P1 |

## 面试要点

1. **Q: Service 无 Endpoint 的排查路径？**
   A: 检查 Pod 是否 Ready → 验证 selector 匹配 → 查看 EndpointSlice 同步状态 → 检查 kube-proxy 日志 → 确认 RBAC 权限

2. **Q: kube-proxy iptables 模式和 IPVS 模式的区别？**
   A: iptables 线性匹配 O(n)，IPVS 哈希表 O(1)；IPVS 支持更多负载均衡算法；大规模集群(>1000 Service)建议 IPVS

3. **Q: ClusterIP 不通的完整排查链？**
   A: Pod内→Service ClusterIP→kube-proxy规则→后端Pod→NetworkPolicy→CNI路由→节点iptables

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]
- networking.md|网络故障排查]]

## Related

- [[webhook-admission-fta]] — Admission Webhook 异常 FTA 树
- [[calico-fta]] — Calico Fta
- [[技能/ts-gitops-devops.md|ts-gitops-devops]] — GitOps/DevOps 排查
- [[技能/Agent Orchestration Patterns.md|Agent Orchestration Patterns]] — Agent Orchestration Patterns for FTA
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[故障诊断/FTA故障树/list/service-fta.md|Service 异常故障树分析]]
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]

---

> 📄 **完整版本**: 本文件为精简版故障树速查。完整的故障树分析（含详细根因推理和决策路径）请查阅：
> [[归档/troubleshooting-diagnostics/FTA故障树/list/service-fta.md|Service FTA 完整版]]


<!-- risk-assessed -->
