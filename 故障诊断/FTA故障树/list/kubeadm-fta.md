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

## 生产案例

### 案例1: kubeadm init 失败 - 端口被占用

**时间线**:
- 09:00 执行 `kubeadm init` 初始化控制平面
- 09:02 失败: `port 6443 is already in use`
- 09:05 确认根因: 上次失败的 init 残留进程占用端口
- 09:10 `kubeadm reset` 后重新 init 成功

**根因链**:
```
上次init失败 → 残留进程占用6443端口 → 未执行reset
→ 再次init时端口冲突 → 初始化失败
```

**修复**:
```bash
# 🟡 重置 kubeadm 状态
kubeadm reset -f
rm -rf /etc/kubernetes/manifests /var/lib/etcd
systemctl restart kubelet
# 🟡 重新初始化
kubeadm init --config kubeadm-config.yaml
```

### 案例2: kubeadm join 失败 - token 过期

**现象**: `kubeadm join` 报错 `token has expired`

**根因**: kubeadm init 生成的 token 默认 24h 过期

**修复**:
```bash
# 🟡 重新生成 token
kubeadm token create --print-join-command
# 在新节点上执行输出的 join 命令
```

## 预防与监控

### 告警规则

```yaml
groups:
- name: kubeadm-alerts
  rules:
  - alert: KubeletClientCertExpiring
    expr: apiserver_client_certificate_expiration_seconds_bucket{le="604800"} > 0
    for: 1h
    labels:
      severity: warning
  - alert: ControlPlaneComponentDown
    expr: up{job=~"kube-apiserver|kube-controller-manager|kube-scheduler"} == 0
    for: 2m
    labels:
      severity: critical
```

### 预防措施

| 措施 | 说明 | 优先级 |
|------|------|--------|
| 证书自动轮转 | 启用 rotateCertificates | P0 |
| init 前检查 | 确认端口/进程/磁盘干净 | P0 |
| 配置文件管理 | 用 kubeadm-config.yaml 而非命令行参数 | P1 |
| 多控制平面 | 生产至少 3 个 master | P1 |

## 面试要点

1. **Q: kubeadm init 的完整流程？**
   A: 预检查(PreFlight) → 生成证书 → 生成 kubeconfig → 启动静态 Pod → 应用 CoreDNS/kube-proxy → 生成 join token

2. **Q: kubeadm init 失败的排查？**
   A: 查看错误消息 → 检查端口占用 → 确认系统要求(内核模块/swap) → 检查镜像拉取 → `kubeadm reset` 后重试

3. **Q: kubeadm 集群升级的步骤？**
   A: 升级 kubeadm → `kubeadm upgrade plan` → `kubeadm upgrade apply` → 升级 kubelet/kubectl → `kubectl drain` + 重启 kubelet → `kubectl uncordon`

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]

## Related

- [[实体/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- [[README]]
- [[nginx-ingress-fta]]
- [[故障诊断/FTA故障树/list/kubeadm-fta.md|kubeadm-fta]]
- [[技能/learn-05-ingress-basics.md|第五课：Ingress - 外部 HTTP/HTTPS 访问]] — Cross-reference


<!-- risk-assessed -->
