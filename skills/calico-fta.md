---
title: Calico Fta
description: 'description: ''TE["顶事件: Calico 网络异常<br/>Pod 无法通信 / 网络策略不生效 / BGP 会话断开"]'''
category: skills
tags:
- k8s
- fta
- troubleshooting
- kubelet
- flannel
- calico
- daemonset
- networkpolicy
- webhook
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- Calico Fta 是什么
- 如何 Calico Fta
trigger_keywords:
- Calico
- Fta
prerequisites:
- kubectl-basics
- cni-basics
fta_id: FTA-CALICO-001
component: Calico
severity: critical
created: "2026-05-23"
---

# Calico Fta

title: calico FTA 树：Calico CNI 故障诊断
description: 'TE["顶事件: Calico 网络异常<br/>Pod 无法通信 / 网络策略不生效 / BGP 会话断开"]'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- [[kubelet|kubelet]]
- flannel
- calico
- [[DaemonSet|daemonset]]
- networkpolicy
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- calico FTA 树：Calico CNI 故障诊断 是什么
- 如何 calico FTA 树：Calico CNI 故障诊断
- calico FTA 树：Calico CNI 故障诊断 根因分析
- calico FTA 树：Calico CNI 故障诊断 故障树
trigger_keywords:
- calico
- FTA
- 树：Calico
- CNI
- 故障诊断
- fta
cross_refs:
- type: domain
  path: ../domain-03-networking-traffic/
  label: '知识域: domain-03-networking-traffic'
fta_metadata:
  fta_id: FTA-CALICO-001
  top_event: Calico 网络异常 (Pod 无法通信 / 网络策略不生效 / BGP 会话断开)
  top_event_id: TE-CALICO-001
  bottom_events_count: 18
  gate_types:
  - OR
  - AND
  entry_conditions:
  - kubectl get pods -A | grep -E 'calico|felix' 显示异常
  - Pod 之间网络不通
  - kubectl get networkpolicy -A 显示策略未生效
agent_notes:
  decision_tree_entry: kubectl get pods -n kube-system -l k8s-app=calico-node 检查 Calico
    Pod 状态
  critical_commands:
  - kubectl get pods -n kube-system -l k8s-app=calico-node -o wide
  - kubectl exec -n kube-system <pod> -- birdc show protocols
  - kubectl logs -n kube-system -l k8s-app=calico-node --tail=50
  - kubectl get nodes -o wide | grep -E 'calico|tunnel'
  danger_operations:
  - action: kubectl delete pod -n kube-system -l k8s-app=calico-node --force
    risk: 强制删除会导致 Calico 网络重建，可能短暂中断网络通信
    requires_confirmation: true
  - action: kubectl exec -n kube-system <pod> -- calicoctl delete node <node>
    risk: 删除节点可能断开 BGP 会话，导致该节点网络中断
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
<!-- condition: kubectl get pods -A -o json | jq '.items[] | select(.metadata.labels.app == "calico-node" or .metadata.labels.k8s-app == "calico-node") | {name: .metadata.name, status: .status.phase}' 显示 Calico Pod 异常 -->

# calico FTA 树：Calico CNI 故障诊断

> **fta_id**: FTA-CALICO-001
> **component**: cni / calico
> **severity**: P0-P2
> **k8s_versions**: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
> **top_event_id**: TE-CALICO-001
> **last_updated**: 2026-05
> **authors**: KUDIG Team

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE["顶事件: Calico 网络异常<br/>Pod 无法通信 / 网络策略不生效 / BGP 会话断开"]
  OR0{{OR}}
  TE --> OR0

  %% ======== 一级分类 ========
  OR0 --> CAT_INSTALL["A. Calico 安装/初始化失败"]
  OR0 --> CAT_CNI["B. Calico CNI 配置加载失败"]
  OR0 --> CAT_FELIX["C. Felix/Bird BGP 会话异常"]
  OR0 --> CAT_NETPOL["D. NetworkPolicy 不生效"]
  OR0 --> CAT_TUNNEL["E. IPIP/VXLAN 隧道问题"]
  OR0 --> CAT_IPAM["F. IPAM 地址耗尽/冲突"]
  OR0 --> CAT_TYPHA["G. Felix-Typha 通信异常"]

  %% ======== A. 安装 ========
  A_OR{{OR}}
  CAT_INSTALL --> A_OR
  A_OR --> A1["A1. Typha/Confd 配置错误<br/>Calico manifest 不兼容 K8s 版本"]
  A_OR --> A2["A2. CNI 二进制文件未找到<br/>/opt/cni/bin/ 缺少 calico"]
  A_OR --> A3["A3. 容器镜像拉取失败<br/>quay.io/calico 访问超时/认

## 相关链接

- [[skills/FTA Methodology and Core Principles|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine|FTA 诊断执行引擎]]
- [[skills/ts-networking|网络故障排查]]

## Related

- [[skills/skill-MOC|skill-MOC]] — topic-skills MOC
- [[webhook-admission-fta]] — Admission Webhook 异常 FTA 树
- [[entities/networkpolicy|networkpolicy]] — NetworkPolicy
- [[entities/kubelet|kubelet]] — kubelet
- [[cni]] — CNI (Container Network Interface)

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/calico-fta|calico-fta]]
- RELEASE-NOTES-3.18
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.28|RELEASE-NOTES-3.28]]
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.29|RELEASE-NOTES-3.29]]
- RELEASE-NOTES-3.19
- RELEASE-NOTES-2.4
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.26|RELEASE-NOTES-3.26]]
- RELEASE-NOTES-3.12
- RELEASE-NOTES-3.5
- RELEASE-NOTES-3.1
- RELEASE-NOTES-3.16
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.22|RELEASE-NOTES-3.22]]
- RELEASE-NOTES-3.0
- RELEASE-NOTES-3.17
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.23|RELEASE-NOTES-3.23]]
- RELEASE-NOTES-2.5
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.27|RELEASE-NOTES-3.27]]
- RELEASE-NOTES-3.13
- RELEASE-NOTES-3.4
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.30|RELEASE-NOTES-3.30]]
- RELEASE-NOTES-3.14
- RELEASE-NOTES-3.3
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.20|RELEASE-NOTES-3.20]]
- RELEASE-NOTES-2.6
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.24|RELEASE-NOTES-3.24]]
- RELEASE-NOTES-3.7
- RELEASE-NOTES-3.10
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.25|RELEASE-NOTES-3.25]]
- RELEASE-NOTES-3.6
- RELEASE-NOTES-3.11
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.31|RELEASE-NOTES-3.31]]
- RELEASE-NOTES-3.15
- RELEASE-NOTES-3.2
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.21|RELEASE-NOTES-3.21]]
- RELEASE-NOTES-3.9
- RELEASE-NOTES-3.8
- [[skills/Agent Orchestration Patterns|Agent Orchestration Patterns for FTA]] — Cross-reference
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.28|RELEASE-NOTES-3.28]]
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.29|RELEASE-NOTES-3.29]]
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.26|RELEASE-NOTES-3.26]]
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.23|RELEASE-NOTES-3.23]]
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.27|RELEASE-NOTES-3.27]]
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.30|RELEASE-NOTES-3.30]]
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.24|RELEASE-NOTES-3.24]]
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.25|RELEASE-NOTES-3.25]]
- [[domain-19-landscape-references/topic-release-notes/networking/calico/RELEASE-NOTES-3.31|RELEASE-NOTES-3.31]]
