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
tier: supporting
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

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]
- networking.md|网络故障排查]]

## Related

- [[webhook-admission-fta]] — Admission Webhook 异常 FTA 树
- [[calico-fta]] — Calico Fta
- [[skills/ts-gitops-devops.md|ts-gitops-devops]] — GitOps/DevOps 排查
- [[skills/Agent Orchestration Patterns.md|Agent Orchestration Patterns]] — Agent Orchestration Patterns for FTA
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[故障诊断/topic-fta/list/service-fta.md|Service 异常故障树分析]]
- [[生态参考/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/topic-index/network-index.md|Network 网络知识图谱索引]]


<!-- risk-assessed -->
