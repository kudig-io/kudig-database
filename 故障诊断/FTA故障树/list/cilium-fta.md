---
title: Cilium Fta
description: 'description: ''TE["顶事件: Cilium/EBPF 网络异常<br/>Pod 无法通信 / 访问延迟高"]'''
summary: 'description: ''TE["顶事件: Cilium/EBPF 网络异常<br/>Pod 无法通信 / 访问延迟高"]'''
category: skills
tags:
- k8s
- fta
- troubleshooting
- kubelet
- cilium
- coredns
- helm
- daemonset
- gateway
- ebpf
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Cilium Fta 是什么
- 如何 Cilium Fta
trigger_keywords:
- Cilium
- Fta
prerequisites:
- kubectl-basics
- helm-basics
- ebpf-basics
- cilium-basics
fta_id: FTA-CILIUM-001
component: Cilium
severity: critical
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Cilium|Cilium]] Fta

title: cilium FTA 树：eBPF/Cilium CNI 故障诊断
description: 'TE["顶事件: Cilium/EBPF 网络异常<br/>Pod 无法通信 / 访问延迟高"]'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- [[kubelet|kubelet]]
- cilium
- coredns
- helm
- daemonset
- gateway
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- cilium FTA 树：eBPF/Cilium CNI 故障诊断 是什么
- 如何 cilium FTA 树：eBPF/Cilium CNI 故障诊断
- cilium FTA 树：eBPF/Cilium CNI 故障诊断 根因分析
- cilium FTA 树：eBPF/Cilium CNI 故障诊断 故障树
trigger_keywords:
- cilium
- FTA
- 树：eBPF
- Cilium
- CNI
- 故障诊断
- fta
cross_refs:
- type: domain
  path: ../网络/
  label: '知识域: 网络'
fta_metadata:
  fta_id: FTA-CILIUM-001
  top_event: Cilium/eBPF 网络异常 (Pod 无法通信/访问延迟高)
  top_event_id: TE-CILIUM-001
  bottom_events_count: 18
  gate_types:
  - OR
  - AND
  entry_conditions:
  - kubectl get pods -n kube-system -l k8s-app=cilium 显示异常
  - Pod 之间网络不通或延迟高
  - kubectl exec <pod> -n <ns> -- cilium status 显示异常
agent_notes:
  decision_tree_entry: kubectl get pods -n kube-system -l k8s-app=cilium检查 Cilium
    Pod 状态
  critical_commands:
  - kubectl get pods -n kube-system -l k8s-app=cilium -o wide
  - kubectl exec -n kube-system <pod> -- cilium status
  - kubectl exec -n kube-system <pod> -- cilium endpoint list
  - kubectl logs -n kube-system -l k8s-app=cilium --tail=100
  danger_operations:
  - action: kubectl delete pod -n kube-system -l k8s-app=cilium --force
    risk: 强制删除会导致 Cilium 重启，该节点网络可能中断
    requires_confirmation: true
  - action: cilium config DropNotification=true
    risk: 修改 Cilium 配置可能导致网络策略变更，影响流量
    requires_confirmation: true
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
<!-- condition: kubectl get pods -n kube-system -l k8s-app=cilium -o jsonpath='{range .items[?(@.status.phase!="Running")]} {.metadata.name}{\"\n\"}{end}' 显示 Cilium 异常 -->

# cilium FTA 树：eBPF/Cilium CNI 故障诊断

> **fta_id**: FTA-CILIUM-001
> **component**: cni / cilium
> **severity**: P0-P2
> **k8s_versions**: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
> **top_event_id**: TE-CILIUM-001
> **last_updated**: 2026-05
> **authors**: KUDIG Team

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE["顶事件: Cilium/EBPF 网络异常<br/>Pod 无法通信 / 访问延迟高"]
  OR0{{OR}}
  TE --> OR0

  %% ======== 一级分类 ========
  OR0 --> CAT_INIT["A. Cilium Agent 初始化失败"]
  OR0 --> CAT_HEALTH["B. Cilium 健康检查失败"]
  OR0 --> CAT_BPF["C. eBPF Map/Program 异常"]
  OR0 --> CAT_NET["D. 网络连通性问题"]
  OR0 --> CAT_HUBBLE["E. Hubble 流量观测不可用"]
  OR0 --> CAT_SVC["F. Service/LoadBalancer 异常"]
  OR0 --> CAT_BGP["G. BGP Peering 异常"]

  %% ======== A. Agent 初始化 ========
  A_OR{{OR}}
  CAT_INIT --> A_OR
  A_OR --> A1["A1. Cilium Agent 无法启动<br/>config файлов не найден"]
  A_OR --> A2["A2. Kubernetes Mode 初始化失败<br/>API Server 连接异常"]
  A_OR --> A3["A3. 身份认证 (Cilium Identity) 失败<br/>下层 CNI 不兼容"]

  A1_S["Agent Pod 或 DaemonSet 不在 Running 状态"]
  A1 --> A1_S

  A2_S

## 相关链接

- [[技能/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[技能/ts-networking.md|网络故障排查]]

## Related

- [[实体/kubelet.md|kubelet]] — kubelet
- [[helm]] — Helm
- [[coredns]] — CoreDNS
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[故障诊断/FTA故障树/list/cilium-fta.md|cilium-fta]]
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-1.18
- RELEASE-NOTES-1.19
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.9
- RELEASE-NOTES-1.16
- RELEASE-NOTES-1.3
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-1.17
- RELEASE-NOTES-1.2
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.11

<!-- risk-assessed -->
