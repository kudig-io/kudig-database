---
title: Kubeadm Fta
description: 'description: B_OR --> B4["B4. 网络插件初始化失败<br/>CNI 配置冲突 / calico 节点状态"]'
summary: 'description: B_OR --> B4["B4. 网络插件初始化失败<br/>CNI 配置冲突 / calico 节点状态"]'
category: skills
tags:
- k8s
- fta
- troubleshooting
- etcd
- apiserver
- kubelet
- scheduler
- calico
- containerd
- ingress
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kubeadm Fta 是什么
- 如何 Kubeadm Fta
trigger_keywords:
- Kubeadm
- Fta
prerequisites:
- kubectl-basics
- cni-basics
- etcd-basics
fta_id: FTA-KUBEADM-001
component: Kubeadm
severity: critical
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubeadm Fta

title: kubeadm FTA 树：集群生命周期故障诊断
description: B_OR --> B4["B4. 网络插件初始化失败<br/>CNI 配置冲突 / calico 节点状态"]
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- etcd
- apiserver
- [[kubelet|kubelet]]
- scheduler
- calico
- containerd
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- kubeadm FTA 树：集群生命周期故障诊断 是什么
- 如何 kubeadm FTA 树：集群生命周期故障诊断
- kubeadm FTA 树：集群生命周期故障诊断 根因分析
- kubeadm FTA 树：集群生命周期故障诊断 故障树
trigger_keywords:
- kubeadm
- FTA
- 树：集群生命周期故障诊断
- fta
fta_metadata:
  fta_id: FTA-KUBEADM-001
  top_event: kubeadm 操作异常 (init/join/reset/upgrade 失败)
  top_event_id: TE-KUBEADM-001
  bottom_events_count: 20
  gate_types:
  - OR
  - AND
  entry_conditions:
  - kubeadm init/join/reset/upgrade 命令执行失败
  - kubectl get nodes 显示 NotReady
  - journalctl -u kubelet 显示 kubeadm 相关错误
agent_notes:
  decision_tree_entry: kubeadm init --dry-run 检查配置错误; journalctl -u kubelet 检查 kubelet
    日志
  critical_commands:
  - kubeadm init --dry-run
  - kubeadm phase certs check-expiration
  - kubeadm upgrade plan
  - journalctl -u kubelet --since '1 hour ago'
  - cat /etc/kubernetes/manifests/*.yaml | grep -E 'image|pull'
  danger_operations:
  - action: kubeadm reset --force
    risk: 重置会删除所有 Kubernetes 配置和数据，集群需要重新创建
    requires_confirmation: true
  - action: rm -rf /etc/kubernetes/manifests/*
    risk: 删除 manifest 会导致控制面组件被移除，集群不可用
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
<!-- condition: kubeadm init/join/reset/upgrade 命令返回错误码或 kubectl get nodes 显示 NotReady -->

# kubeadm FTA 树：集群生命周期故障诊断

> **fta_id**: FTA-KUBEADM-001
> **component**: cluster-lifecycle / kubeadm
> **severity**: P0-P2
> **k8s_versions**: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
> **top_event_id**: TE-KUBEADM-001
> **last_updated**: 2026-05
> **authors**: KUDIG Team

---

## Mermaid FTA 树

```mermaid
flowchart TD
  TE["顶事件: kubeadm 操作异常<br/>init/join/reset/upgrade 失败"]
  OR0{{OR}}
  TE --> OR0

  %% ======== 一级分类 ========
  OR0 --> CAT_INIT["A. kubeadm init 失败"]
  OR0 --> CAT_JOIN["B. kubeadm join 失败"]
  OR0 --> CAT_RESET["C. kubeadm reset 失败"]
  OR0 --> CAT_UPGRADE["D. kubeadm upgrade 失败"]
  OR0 --> CAT_CONFIG["E. kubeadm config 生成错误"]
  OR0 --> CAT_CERTS["F. 证书相关问题"]

  %% ======== A. init ========
  A_OR{{OR}}
  CAT_INIT --> A_OR
  A_OR --> A1["A1. Pre-flight 检查失败<br/>端口占用 / 缺失工具"]
  A_OR --> A2["A2. 证书生成失败<br/>PKI 目录不存在 / 权限问题"]
  A_OR --> A3["A3. etcd 集群初始化失败<br/>超时 / 端口冲突"]
  A_OR --> A4["A4. 控制平面组件启动失败<br/>kubelet 不健康 / 端口冲突"]
  A_OR --> A5["A5. upload-certs 失败<br/>secret 不存在 / 权限问题"]

  %% ======== B. join ========
  B_OR{{OR}}
  CAT_JOIN --> B_OR
  B_OR --> B1["B1. TLS bootstrapping 失败<br/>token 过期 / CA 凭证不对"]
  B_OR --> B2["B2. kubelet 注册失败<br/>node name 冲突 / 角色不匹配"]
  B_OR --> B3["B3. crictl check 失败<br/>容器运行时未正常启动"]
  B_OR --> B4["B4. 网络插件初始化失败<br/>CNI 配置冲突 / calico 节点状态"]
  B_OR --> B5["B5. n

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[entities/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[README]]
- [[nginx-ingress-fta]]
- [[故障诊断/FTA故障树/list/kubeadm-fta.md|kubeadm-fta]]
- [[skills/learn-05-ingress-basics.md|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference


<!-- risk-assessed -->
