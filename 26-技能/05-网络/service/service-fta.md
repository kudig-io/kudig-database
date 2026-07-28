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




# [[service|Service]] 异常故障树分析

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

### 案例 1: Service Endpoints 为空导致全量 502

| 时间 | 事件 |
|------|------|
| 14:30 | 发布系统更新 Deployment label，selector 与 Service 不匹配 |
| 14:31 | 监控告警: Service 后端 502 率 100% |
| 14:35 | SRE 执行 `kubectl get endpoints svc-name -n prod` 发现 ENDPOINTS 为空 |
| 14:38 | 对比 `kubectl get svc svc-name -o yaml` selector 与 Pod label 发现不一致 |
| 14:40 | 🟡 回滚 Deployment label，Endpoints 恢复，流量恢复 |

**根因**: 发布模板中 `app` label 从 `web` 改为 `web-v2`，但 Service selector 未同步更新。

### 案例 2: kube-proxy iptables 规则膨胀导致 Service 间歇性超时

**现象**: 集群 5000+ Service 时，部分 Service 访问 P99 延迟从 5ms 飙升至 2s。

**诊断路径**: `iptables -t filter -L KUBE-SERVICES | wc -l` → 12000+ 规则 → kube-proxy sync 周期过长

**修复**: 🟡 切换 kube-proxy 模式为 IPVS，`--proxy-mode=ipvs`，重启 kube-proxy DaemonSet

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | Service 完全不可用且影响核心业务 | 立即回滚最近变更 + 通知 On-call |
| P1 | 部分 Endpoints 缺失，流量降级 | 15min 内定位，修复 selector/endpoint |
| P2 | 间歇性超时，非核心服务 | 记录并排入下一迭代 |

## 面试要点

1. **Q: Service 的 ClusterIP 是如何分配的？kube-proxy 三种模式的区别？**
   A: ClusterIP 由 API Server 从 --service-cluster-ip-range 中分配。kube-proxy 模式: userspace(已废弃) → iptables(线性匹配,O(n)) → IPVS(哈希表,O(1))。大规模集群推荐 IPVS。

2. **Q: Endpoints 为空的常见原因有哪些？**
   A: ① selector 不匹配 Pod label ② Pod 未 Ready(readinessProbe 失败) ③ 目标端口名称/编号不一致 ④ Pod 所在节点 NotReady ⑤ EndpointSlice 控制器异常。

3. **Q: Headless Service 与 ClusterIP Service 的 DNS 解析有何不同？**
   A: ClusterIP Service 解析返回虚拟 IP；Headless Service(clusterIP: None) 直接返回后端 Pod IP 列表(A 记录)，用于 StatefulSet 的稳定网络标识。

## 相关链接

- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[26-技能/04-工作负载/pod/方法论/FTA Diagnostic Execution Engine.md|[[19-故障诊断/06-FTA故障树/fta-execution-engine|FTA 诊断执行引擎]]]]
- networking.md|网络故障排查]]

## Related

- [[webhook-admission-fta]] — Admission Webhook 异常 FTA 树
- [[calico-fta]] — Calico Fta
- [[26-技能/01-集群运维/gitops-argocd/诊断排障/ts-gitops-devops.md|ts-gitops-devops]] — GitOps/DevOps 排查
- [[26-技能/04-工作负载/pod/方法论/agent/Agent Orchestration Patterns.md|Agent Orchestration Patterns]] — Agent Orchestration Patterns for FTA
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[19-故障诊断/06-FTA故障树/list/service-fta.md|Service 异常故障树分析]]
- [[21-生态参考/03-领域索引/terway-index.md|Terway 知识图谱索引]]
- [[21-生态参考/03-领域索引/network-index.md|Network 网络知识图谱索引]]


<!-- risk-assessed -->
