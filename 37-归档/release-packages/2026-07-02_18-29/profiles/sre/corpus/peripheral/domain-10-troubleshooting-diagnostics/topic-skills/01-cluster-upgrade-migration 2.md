---
title: 集群升级与迁移故障诊断与修复 / Cluster Upgrade & Migration Failure Diagnosis & Remediation
description: Kubernetes 集群升级是运维中最具风险的操作之一，涉及控制平面组件（API Server、etcd、Scheduler、Controller
  Manager）、节点组件（kubelet、kube-proxy、容器运行时）、插件（CNI、CSI、Ingress Controller）以及工作负载 API
  版本的多层兼容性。升级过程中的版本偏移（Version Skew）、废弃 API、证书过
summary: Kubernetes 集群升级是运维中最具风险的操作之一，涉及控制平面组件（API Server、etcd、Scheduler、Controller
  Manager）、节点组件（kubelet、kube-proxy、容器运行时）、插件（CNI、CSI、Ingress Controller）以及工作负载 API
  版本的多层兼容性。升级过程中的版本偏移（Version Skew）、废弃 API、证书过
category: control-plane
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 15min
intent_queries:
- 集群升级与迁移故障诊断与修复 / Cluster Upgrade & Migration Failure Diagnosis & Remediation 是什么
- 如何 集群升级与迁移故障诊断与修复 / Cluster Upgrade & Migration Failure Diagnosis & Remediation
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- 集群升级与迁移故障诊断与修复 / Cluster Upgrade & Migration Failure Diagnosis & Remediation 故障排查
- 集群升级与迁移故障诊断与修复 / Cluster Upgrade & Migration Failure Diagnosis & Remediation 排障步骤
trigger_keywords:
- upgrade
- migration
- kubeadm upgrade
- version skew
- deprecated api
- 回滚
- rollback
- 节点升级后 NotReady
- etcd 升级
- 容器运行时升级
- control plane 升级失败
- 升级卡死
prerequisites:
- domain-01-cluster-fundamentals
- domain-10-troubleshooting-diagnostics
- kubeadm-basics
skill_id: SKILL-25_CLUSTER_UPGRADE_MIGRATION-001
skill_name: 集群升级与迁移故障诊断与修复 / Cluster Upgrade & Migration Failure Diagnosis & Remediation
version: 1.0.0
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/cluster-upgrade-fta.md
  label: 升级迁移故障树分析
- type: domain
  path: ../domain-10-troubleshooting-diagnostics/34-upgrade-migration-troubleshooting.md
  label: 升级迁移深度排查
- type: skill
  path: ./11-control-plane-failure.md
  label: etcd 与控制平面故障诊断
agent_execution_mode: L2-semi-auto
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 集群升级与迁移故障诊断与修复 / Cluster Upgrade & Migration Failure Diagnosis & Remediation

[[Kubernetes|Kubernetes]] 集群升级是运维中最具风险的操作之一，涉及控制平面组件（API Server、[[etcd|etcd]]、Scheduler、Controller Manager）、节点组件（[[kubelet|kubelet]]、kube-proxy、容器运行时）、插件（CNI、CSI、[[Ingress|Ingress]] Controller）以及工作负载 API 版本的多层兼容性。升级过程中的版本偏移（Version Skew）、废弃 API、证书过期、容器运行时变更、etcd 数据格式不兼容等问题都可能导致集群功能异常甚至完全不可用。

本 Skill 覆盖 kubeadm 升级卡死、版本偏移、API 废弃不兼容、etcd 升级失败、节点升级后异常、CNI/CSI 插件不兼容、跨集群迁移中断、升级回滚失败等 10 种根因的诊断和修复。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| kubeadm upgrade 命令执行失败或卡死 | `kubeadm upgrade plan` / `kubeadm upgrade apply` 输出 | 0.95 |
| 升级后节点版本不一致（Version Skew） | `kubectl get nodes` 显示多版本 | 0.90 |
| 升级后应用无法部署，提示 API 版本不存在 | `kubectl apply` 失败信息 | 0.90 |
| etcd 集群升级后失去 Leader | `etcdctl endpoint health` | 0.95 |
| 节点升级后进入 NotReady | `kubectl get nodes` | 0.90 |
| CNI/CSI Pod 升级后 CrashLoopBackOff | `kubectl get pods -n kube-system` | 0.85 |
| 升级后证书验证失败 | `openssl x509 -in` / 组件日志 | 0.90 |

**排除条件**: 未进行升级操作但控制平面异常 → SKILL-CP-001; 节点未升级但 NotReady → SKILL-NODE-001; 存储问题与升级无关 → SKILL-STORE-001; 网络问题与升级无关 → SKILL-NET-001

## 快速分级（2 分钟内完成）

```
升级阶段 + 影响范围
├── 控制平面升级失败（API Server/etcd 不可用）────→ P0（立即处理）
├── 升级后 >30% 节点 NotReady────────────────────→ P0（立即处理）
├── 升级后核心插件（CNI/CSI）失效────────────────→ P1（30min 内修复）
├── 升级后部分工作负载 API 不兼容───────────────→ P1（1h 内修复）
├── 单个节点升级失败─────────────────────────────→ P2（2h 内修复）
├── 升级版本偏移（Version Skew）在允许范围内─────→ P2（下次维护窗口）
└── 升级计划验证失败─────────────────────────────→ P3（预防性处理）
```

**立即升级条件**：
- 控制平面升级后 API Server 不可访问
- etcd 升级后集群失去 quorum
- 升级后超过 50% 节点 NotReady
- 升级回滚失败且集群处于不稳定状态

## 执行流程

```
# 🟢 低风险：只读/信息收集，通常无副作用
工单/告警触发
    │
    ▼
┌──────────────┐    Step: D1.1-D1.5
│ Phase 1      │    内容: kubectl 快速检查（只读，零风险）
│ 快速检查      │
└──────┬───────┘
       │ 无法确认根因
       ▼
┌──────────────┐    Step: D2.1-D2.6
│ Phase 2      │    内容: 深度分析（只读，零风险）
│ 深度检查      │
└──────┬───────┘
       │ 需主动探测/修复
       ▼
┌──────────────┐    Step: D3.1-D3.3
│ Phase 3      │    内容: 主动探测（低风险，可能需审批）
│ 主动探测      │
└──────┬───────┘
       │ 确认根因
       ▼
┌──────────────┐    RC-001~010
│ 根因匹配      │
└──────┬───────┘
       │
       ▼
┌──────────────┐    REM-001~008
│ 修复操作      │    风险: LOW → MEDIUM → HIGH → CRITICAL
└──────┬───────┘
       │
       ▼
┌──────────────┐    V1~V6
│ 验证确认      │
└──────────────┘
```
## 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | kubeadm upgrade apply 失败或卡死 | `kubeadm upgrade plan` 输出错误 | 0.95 | 网络临时问题 |
| S2 | 节点版本不一致（Version Skew） | `kubectl get nodes` 显示多版本 | 0.90 | 升级过程中（临时） |
| S3 | API 资源 apply 失败，提示版本不存在 | `kubectl apply` 错误输出 | 0.90 | 拼写错误 |
| S4 | etcd 升级后 endpoint 不健康 | `etcdctl endpoint health` | 0.95 | 网络分区 |
| S5 | 节点升级后 NotReady，kubelet 版本不匹配 | `kubectl get nodes` + `describe node` | 0.90 | 节点硬件问题 |
| S6 | CNI/CSI Pod 升级后 CrashLoopBackOff | `kubectl get pods -n kube-system` | 0.85 | 镜像拉取失败 |
| S7 | 升级后证书验证失败 | 组件日志 TLS 错误 | 0.90 | 证书自然过期 |
| S8 | 容器运行时升级后 Pod 无法启动 | `crictl info` / Pod Event | 0.85 | 节点磁盘满 |

### 2.2 工单关键词映射

- "kubeadm upgrade 失败了"
- "升级后节点 NotReady"
- "API version 不存在，apply 失败"
- "etcd 升级后集群不健康"
- "CNI Pod 升级后一直重启"
- "升级后证书错误"
- "containerd 升级后 Pod 起不来"
- "版本不一致，kubectl 和 kubelet 版本不同"
- "升级回滚失败"
- "迁移后应用网络不通"

### 2.3 排除标准

- 未进行升级操作但控制平面异常 → 使用 SKILL-CP-001
- 节点硬件问题导致 NotReady → 使用 SKILL-NODE-001
- 存储问题与升级无关 → 使用 SKILL-STORE-001
- 网络问题与升级无关 → 使用 SKILL-NET-001
- 镜像拉取失败导致 Pod 无法启动 → 使用 SKILL-IMAGE-001

## 快速分级（2 分钟内完成）

### 3.1 影响评估

**Step T1**: 检查集群整体健康状态和版本分布
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 API Server 可用性
kubectl get --raw /healthz
kubectl get --raw /healthz/etcd

# 检查节点版本分布
kubectl get nodes -o jsonpath='{
  range .items[*]
}{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}'

# 统计 NotReady 节点数和版本偏移节点数
kubectl get nodes --no-headers | awk '
  $5 !~ /v1.3[0-2]/ {skew++}
  $5 ~ /NotReady/ {notready++}
  END {print "Version skew nodes:", skew; print "NotReady nodes:", notready}
'
```
> **判断规则**: 
> - 如果 API Server /healthz 返回非 200 → 控制平面问题，P0
> - 如果 NotReady 节点 > 30% → P0
> - 如果 Version Skew 节点 > 50% → P1
> - 如果仅单个节点受影响 → P2

**Step T2**: 检查核心组件状态

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 kube-system 中核心 Pod 状态
kubectl get pods -n kube-system -o jsonpath='{
  range .items[*]
}{.metadata.name}{"\t"}{.status.phase}{"\t"}{.spec.containers[0].image}{"\n"}{end}' | grep -E "Error|CrashLoopBackOff|ImagePullBackOff"

# 检查 etcd 健康状态
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
kubectl exec -n kube-system $ETCD_POD -- sh -c "
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key
etcdctl endpoint health --cluster
"
```
> **判断规则**: 如果 etcd 集群不健康或核心组件 CrashLoopBackOff → 升级为 P0

**Step T3**: 检查升级进度和失败点
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看最近升级相关事件
kubectl get events --all-namespaces --field-selector reason=NodeReady,reason=NodeNotReady --sort-by='.lastTimestamp' | tail -20

# 查看 kubeadm 日志（如在控制平面节点上）
# journalctl -u kubelet -n 200 | grep -i "upgrade|version|deprecated"
```
### 3.2 严重性分级

| 条件 | 级别 | 说明 |
|------|------|------|
| API Server 不可访问或 etcd 失去 quorum | P0 | 集群完全不可用 |
| >30% 节点升级后 NotReady | P0 | 大规模服务中断 |
| 控制平面组件升级后 CrashLoopBackOff | P0 | 集群管控能力受损 |
| CNI/CSI 插件升级后失效 | P1 | 网络/存储功能中断 |
| 废弃 API 导致工作负载无法部署 | P1 | 影响新应用发布 |
| 单个节点升级失败 | P2 | 局部影响 |
| Version Skew 在允许范围内（<1个小版本） | P2 | 兼容性风险 |
| 升级计划阶段发现问题 | P3 | 预防性修复 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至人工**：
- API Server 完全不可用（kubectl 无法连接）
- etcd 集群失去 quorum
- 升级回滚操作失败
- 升级过程中发现数据丢失迹象
- 生产集群升级影响 >50% 业务负载

## 诊断工作流

### Phase 1: 快速检查（只读，零风险）

**Step D1.1**: 检查集群版本分布和升级状态
- **命令**:
  ```bash
  # 控制平面版本
  kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[*].image}'
  
  # 所有节点 kubelet 版本
  kubectl get nodes -o jsonpath='{
    range .items[*]
  }{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\n"}{end}'
  
  # kubeadm 升级计划（如可运行）
  kubeadm upgrade plan 2>/dev/null || echo "kubeadm upgrade plan unavailable"
  ```
- **超时**: 15s
- **预期输出模式**: 所有控制平面组件版本一致，节点 kubelet 版本与 API Server 版本差 <= 1 个小版本
- **判断规则**:
  - 如果 API Server 版本与 kubelet 版本差 > 1 个小版本 → RC-002（版本偏移）
  - 如果 kubeadm upgrade plan 显示废弃 API → RC-003（API 废弃）
- **版本差异**: **[v1.28+]** kubeadm 支持 `kubeadm upgrade node --certificate-renewal=true` 自动轮转证书

**Step D1.2**: 检查核心组件 Pod 状态
- **命令**:
  ```bash
  kubectl get pods -n kube-system -o wide
  kubectl get pods -n kube-system --field-selector status.phase!=Running,status.phase!=Succeeded
  ```
- **超时**: 10s
- **判断规则**:
  - 如果 etcd Pod 非 Running → RC-004（etcd 升级失败）
  - 如果 kube-apiserver/kube-scheduler/kube-controller-manager 非 Running → RC-001（控制平面升级失败）
  - 如果 CNI/CSI Pod 非 Running → RC-006（插件不兼容）

**Step D1.3**: 检查废弃 API 使用情况
- **命令**:
  ```bash
  # 检查当前集群中使用的 API 版本
  kubectl api-versions | grep -E "v1beta1|v1alpha1"
  
  # 检查工作负载使用的 API 版本
  kubectl get deployments --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\t"}{.apiVersion}{"\n"}{end}' | grep -v "apps/v1"
  
  kubectl get ingresses --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\t"}{.apiVersion}{"\n"}{end}' | grep -v "networking.k8s.io/v1"
  ```
- **超时**: 15s
- **判断规则**:
  - 如果存在大量 `extensions/v1beta1` 或 `apps/v1beta1` → RC-003（废弃 API 未迁移）

**Step D1.4**: 检查节点升级后状态
- **命令**:
  ```bash
  # 检查 NotReady 节点的详细状态
  kubectl get nodes | grep NotReady
  
  # 查看 NotReady 节点的 Conditions
  kubectl get nodes -o jsonpath='{
    range .items[?(@.status.conditions[?(@.type=="Ready")].status=="False")]
  }{.metadata.name}{"\n"}{range .status.conditions[*]}{"  "}{.type}{"="}{.status}{" "}{.reason}{"\n"}{end}{end}'
  ```
- **超时**: 10s
- **判断规则**:
  - 如果 NodeReady=False，Reason=KubeletNotReady → RC-005（kubelet 升级失败）
  - 如果 NodeReady=False，Reason=ContainerRuntimeNotReady → RC-007（容器运行时升级不兼容）

**Step D1.5**: 检查证书有效期
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 检查 API Server 证书
  kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[0].metadata.name}' | xargs -I{} kubectl exec -n kube-system {} -- sh -c "openssl x509 -in /etc/kubernetes/pki/apiserver.crt -text -noout | grep 'Not After'"
  
  # 检查节点 kubelet 证书
  kubectl get nodes -o jsonpath='{
    range .items[*]
  }{.metadata.name}{"\n"}{end}' | xargs -I{} kubectl debug node/{} --image=busybox:1.36 -it -- sh -c "openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -text -noout 2>/dev/null | grep 'Not After' || echo 'no cert'"
  ```
- **超时**: 30s
- **判断规则**:
  - 如果证书已过期或即将过期 → RC-008（升级后证书问题）

### Phase 2: 深度检查（只读，零风险）

**Step D2.1**: 分析 kubeadm 升级日志
- **命令**:
  ```bash
  # 在控制平面节点上查看 kubeadm 日志
  # journalctl -u kubelet --since "2 hours ago" | grep -iE "upgrade|kubeadm|version|error|fail"
  
  # 查看 kubelet 日志中的版本相关错误
  kubectl get nodes -o jsonpath='{range .items[?(@.status.conditions[?(@.type=="Ready")].status=="False")]}{.metadata.name}{"\n"}{end}' | head -1 | xargs -I{} sh -c "
    echo '=== Node: {} ==='
    kubectl debug node/{} --image=busybox:1.36 -it -- sh -c 'cat /var/log/pods/kube-system_kubelet-*/kubelet/*.log 2>/dev/null | tail -50 || echo \"log not found\"'
  "
  ```
- **超时**: 30s
- **判断规则**:
  - 如果日志显示 "incompatible kubelet version" → RC-002
  - 如果日志显示 "failed to pull image" 且镜像为暂停镜像（pause）→ RC-007
  - 如果日志显示 "certificate has expired" → RC-008

**Step D2.2**: 检查 etcd 集群详细状态
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
  kubectl exec -n kube-system $ETCD_POD -- sh -c "
  export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
  export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
  export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
  export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key
  
  echo '=== Member List ==='
  etcdctl member list -w table
  
  echo '=== Endpoint Status ==='
  etcdctl endpoint status --cluster -w table
  
  echo '=== Alarm List ==='
  etcdctl alarm list
  
  echo '=== DB Size ==='
  etcdctl endpoint status --cluster -w json | grep -o '"dbSize":[0-9]*'
  "
  ```
- **超时**: 20s
- **判断规则**:
  - 如果 member list 显示不一致的 etcd 版本 → RC-004
  - 如果 alarm list 包含 NOSPACE → 需先清理 etcd 空间
  - 如果 endpoint status 显示 leader 为 none → RC-004

**Step D2.3**: 检查 CNI/CSI 插件版本兼容性
- **命令**:
  ```bash
  # 检查 CNI 插件版本
  kubectl get pods -n kube-system -l k8s-app=calico-node -o jsonpath='{.items[*].spec.containers[*].image}'
  kubectl get pods -n kube-system -l k8s-app=flannel -o jsonpath='{.items[*].spec.containers[*].image}'
  kubectl get pods -n kube-system -l k8s-app=cilium -o jsonpath='{.items[*].spec.containers[*].image}'
  
  # 检查 CSI 驱动版本
  kubectl get pods -n kube-system -o jsonpath='{
    range .items[*]
  }{range .spec.containers[*]}{.image}{"\n"}{end}{end}' | grep -iE "csi-driver|csi-provisioner|csi-attacher" | sort | uniq
  
  # 检查 CNI 配置是否存在
  kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | head -1 | xargs -I{} kubectl debug node/{} --image=busybox:1.36 -it -- sh -c "cat /etc/cni/net.d/*.conf 2>/dev/null || echo 'no cni config'"
  ```
- **超时**: 30s
- **判断规则**:
  - 如果 CNI 镜像版本与 K8s 版本不兼容 → RC-006
  - 如果节点上无 CNI 配置文件 → RC-006

**Step D2.4**: 检查容器运行时兼容性
- **命令**:
  ```bash
  # 检查节点容器运行时版本
  kubectl get nodes -o jsonpath='{
    range .items[*]
  }{.metadata.name}{"\t"}{.status.nodeInfo.containerRuntimeVersion}{"\n"}{end}'
  
  # 检查 containerd 状态（通过节点调试）
  kubectl get nodes -o jsonpath='{range .items[?(@.status.conditions[?(@.type=="Ready")].status=="False")]}{.metadata.name}{"\n"}{end}' | head -1 | xargs -I{} kubectl debug node/{} --image=busybox:1.36 -it -- sh -c "crictl info 2>/dev/null || echo 'crictl failed'"
  ```
- **超时**: 30s
- **判断规则**:
  - 如果 containerd 版本与 kubelet 不兼容 → RC-007
  - 如果 crictl info 返回错误 → 容器运行时未正确运行

**Step D2.5**: 检查升级回滚可行性
- **命令**:
  ```bash
  # 检查 Deployment/StatefulSet/DaemonSet 的历史版本
  kubectl get deployments --all-namespaces -o jsonpath='{
    range .items[*]
  }{.metadata.namespace}{"/"}{.metadata.name}{"\t"}{.spec.revisionHistoryLimit}{"\n"}{end}'
  
  # 检查是否有 etcd 备份
  ls -la /etc/kubernetes/backup/etcd-* 2>/dev/null || echo "No local etcd backup found"
  
  # 检查 kubeadm 配置版本
  kubectl get configmap -n kube-system kubeadm-config -o jsonpath='{.data.ClusterConfiguration}' | grep kubernetesVersion
  ```
- **超时**: 15s
- **判断规则**:
  - 如果 revisionHistoryLimit=0 → 无法回滚 Deployment
  - 如果无 etcd 备份 → 回滚风险极高

**Step D2.6**: 检查跨集群迁移状态（如适用）
- **命令**:
  ```bash
  # 检查迁移后的 Endpoint 状态
  kubectl get endpoints --all-namespaces -o jsonpath='{
    range .items[*]
  }{if .subsets}{.metadata.namespace}{"/"}{.metadata.name}{"\t"}{range .subsets[*]}{range .addresses[*]}{.ip}{","}{end}{end}{"\n"}{end}{end}'
  
  # 检查 Service 后端 Pod 状态
  kubectl get endpoints -n <namespace> <service-name> -o yaml
  ```
- **超时**: 15s
- **判断规则**:
  - 如果 Endpoint 无后端地址但 Pod 运行正常 → 可能是迁移后标签选择器不匹配

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: 测试 API 兼容性（创建测试资源）
- **目的**: 验证目标 API 版本是否可用
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 测试 Deployment apps/v1 兼容性
  cat <<EOF | kubectl apply -f -
  apiVersion: apps/v1
  kind: Deployment
  metadata:
    name: api-compat-test
    namespace: default
  spec:
    replicas: 1
    selector:
      matchLabels:
        app: api-compat-test
    template:
      metadata:
        labels:
          app: api-compat-test
      spec:
        containers:
        - name: test
          image: busybox:1.36
          command: ["sh", "-c", "sleep 30"]
  EOF
  
  kubectl wait --for=condition=available deployment/api-compat-test --timeout=60s
  kubectl delete deployment api-compat-test
  ```
- **超时**: 60s
- **风险级别**: 🟢 低风险
- **判断规则**:
  - 如果 Deployment 创建成功 → API Server 基本正常
  - 如果创建失败并提示 API 版本错误 → RC-003
- **回滚**: `kubectl delete deployment api-compat-test`

**Step D3.2**: 测试节点加入/重新加入集群
- **目的**: 验证升级后的节点是否可以正常注册
- **命令**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 对问题节点执行 kubeadm reset + join（仅限可替换节点）  # ⚠️ 清理节点所有 K8s 配置
  # 先在问题节点上执行：
  # kubeadm reset --force  # ⚠️ 清理节点所有 K8s 配置
  # kubeadm join <control-plane-endpoint> --token <token> --discovery-token-ca-cert-hash <hash>
  
  # 或者仅重启 kubelet
  kubectl debug node/<node-name> --image=busybox:1.36 -it -- sh -c "
  nsenter -t 1 -m -u -i -n sh -c 'systemctl restart kubelet && sleep 5 && systemctl status kubelet'
  "
  ```
- **超时**: 60s
- **风险级别**: 🟡 中风险（重启 kubelet 会触发 Pod 重新创建）
- **审批提示**: "建议在节点 <node-name> 上重启 kubelet，将触发 Pod 重建，是否批准？"
- **判断规则**:
  - 如果重启后节点变为 Ready → RC-005（kubelet 配置未正确加载）
  - 如果重启后仍为 NotReady → 需进一步检查

**Step D3.3**: 验证 etcd 数据完整性
- **目的**: 确认 etcd 升级后数据未损坏
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
  kubectl exec -n kube-system $ETCD_POD -- sh -c "
  export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
  export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
  export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
  export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key
  
  # 统计关键数据前缀
  echo 'Namespaces:' $(etcdctl get /registry/namespaces --prefix --keys-only | wc -l)
  echo 'Deployments:' $(etcdctl get /registry/deployments --prefix --keys-only | wc -l)
  echo 'Pods:' $(etcdctl get /registry/pods --prefix --keys-only | wc -l)
  echo 'Services:' $(etcdctl get /registry/services --prefix --keys-only | wc -l)
  
  # 验证数据可读性
  etcdctl get /registry/namespaces/default -w json | head -5
  "
  ```
- **超时**: 30s
- **风险级别**: 🟢 低风险（只读操作）
- **判断规则**:
  - 如果关键数据数量异常（如为 0）→ RC-004（etcd 数据损坏）
  - 如果数据可读性正常 → etcd 数据层正常

## 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | kubeadm 控制平面升级失败（镜像拉取/配置验证） | 高 | D1.2: apiserver/controller/scheduler Pod 非 Running; kubeadm 日志错误 | FTA-UPGRADE-001 |
| RC-002 | 版本偏移（Version Skew）超过允许范围 | 高 | D1.1: kubelet 与 API Server 版本差 > 1; D2.1: "incompatible version" 日志 | FTA-UPGRADE-002 |
| RC-003 | 废弃 API 未迁移，升级后资源无法管理 | 中 | D1.3: 存在 extensions/v1beta1; D3.1: apply 失败 | FTA-UPGRADE-003 |
| RC-004 | etcd 升级后集群不健康或数据不兼容 | 中 | D1.2: etcd Pod 非 Running; D2.2: endpoint unhealthy; D3.3: 数据异常 | FTA-UPGRADE-004 |
| RC-005 | 节点 kubelet 升级后配置不兼容或启动失败 | 中 | D1.4: NodeReady=False KubeletNotReady; D3.2: 重启 kubelet 无效 | FTA-UPGRADE-005 |
| RC-006 | CNI/CSI 插件版本与新 K8s 版本不兼容 | 中 | D1.2: CNI/CSI Pod CrashLoopBackOff; D2.3: 版本不匹配 | FTA-UPGRADE-006 |
| RC-007 | 容器运行时（containerd/cri-o）升级后不兼容 | 低 | D1.4: ContainerRuntimeNotReady; D2.4: crictl info 失败 | FTA-UPGRADE-007 |
| RC-008 | 升级后证书未正确轮转导致 TLS 失败 | 中 | D1.5: 证书过期; D2.1: "certificate has expired" 日志 | FTA-UPGRADE-008 |
| RC-009 | 升级回滚失败，集群处于中间状态 | 低 | D2.5: 无有效备份; 组件版本混合 | FTA-UPGRADE-009 |
| RC-010 | 跨集群迁移后网络/存储配置不匹配 | 低 | D2.6: Endpoint 无后端; Service 选择器不匹配 | FTA-MIGRATION-001 |

## 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: 重拉控制平面镜像并重启 Pod
- **适用根因**: RC-001
- **前置检查**:
  ```bash
  # 确认控制平面 Pod 确实异常
  kubectl get pods -n kube-system -l component=kube-apiserver --field-selector status.phase!=Running
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 删除异常的控制平面 Pod（由 static pod 自动重建）
  kubectl delete pod -n kube-system -l component=kube-apiserver --field-selector status.phase!=Running
  kubectl delete pod -n kube-system -l component=kube-scheduler --field-selector status.phase!=Running
  kubectl delete pod -n kube-system -l component=kube-controller-manager --field-selector status.phase!=Running
  
  # 等待重建
  sleep 30
  kubectl get pods -n kube-system
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n kube-system -l component=kube-apiserver,component=kube-scheduler,component=kube-controller-manager
  kubectl get --raw /healthz
  ```
- **回滚命令**: 无法直接回滚，如重建失败需检查 static pod manifest

#### REM-002: 废弃 API 资源批量迁移
- **适用根因**: RC-003
- **前置检查**:
  ```bash
  # 统计需要迁移的资源
  kubectl get deployments --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\t"}{.apiVersion}{"\n"}{end}' | grep -v "apps/v1"
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 获取资源并修改 API 版本后重新应用
  kubectl get deployment <name> -n <namespace> -o yaml | sed 's/apiVersion: apps\/v1beta1/apiVersion: apps\/v1/' | sed 's/apiVersion: extensions\/v1beta1/apiVersion: apps\/v1/' > /tmp/migrated-deployment.yaml
  
  # 删除旧版本资源（保留 Pod）
  kubectl delete deployment <name> -n <namespace> --cascade=orphan
  
  # 应用新版本
  kubectl apply -f /tmp/migrated-deployment.yaml
  
  # 对 Ingress 执行类似操作
  kubectl get ingress <name> -n <namespace> -o yaml | sed 's/apiVersion: extensions\/v1beta1/apiVersion: networking.k8s.io\/v1/' > /tmp/migrated-ingress.yaml
  # 注意：networking.k8s.io/v1 的 Ingress spec 格式有变化，需手动调整 backend 为 spec.rules
  ```
- **后置验证**:
  ```bash
  kubectl get deployment <name> -n <namespace> -o jsonpath='{.apiVersion}'
  kubectl get pods -n <namespace> -l app=<label>
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 如果迁移失败，从原始备份恢复
  kubectl apply -f /tmp/backup-deployment.yaml
  ```

#### REM-003: 重启 etcd Pod 恢复集群健康
- **适用根因**: RC-004
- **前置检查**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
  kubectl exec -n kube-system $ETCD_POD -- sh -c "
  export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
  export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
  export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
  export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key
  etcdctl endpoint health
  "
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 逐个重启 etcd Pod（每次等待集群恢复）
  for pod in $(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[*].metadata.name}'); do
    echo "Restarting etcd pod: $pod"
    kubectl delete pod -n kube-system $pod
    sleep 60
    
    # 验证集群健康
    kubectl exec -n kube-system $(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}') -- sh -c "
    export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
    export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
    export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
    export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key
    etcdctl endpoint health --cluster
    "
  done
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  kubectl get --raw /healthz/etcd
  kubectl exec -n kube-system $(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}') -- sh -c "
  export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
  export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
  export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
  export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key
  etcdctl endpoint status --cluster -w table
  "
  ```
- **回滚命令**: 如果 etcd 数据损坏，需从备份恢复（见 REM-007）

### 6.2 🟡 中风险（Agent 建议，人工审批）

#### REM-004: 对问题节点执行 kubeadm reset + join
- **适用根因**: RC-005
- **影响说明**: 节点上所有 Pod 将被删除并重新调度，业务会短暂中断
- **审批提示**: "建议对节点 <node-name> 执行 kubeadm reset + join，将清空节点上所有 Pod，是否批准？"
- **前置检查**:
  ```bash
  # 确认 Pod 可以被驱逐
  kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name>
  
  # 确认有 PDB 保护的关键服务
  kubectl get pdb --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\t"}{.spec.minAvailable}{"\n"}{end}'
  ```
- **执行命令**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubeadm reset`：清理节点所有 K8s 配置/证书/CNI，节点脱离集群
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `iptables -F/-P DROP`：清空/改防火墙规则，可能立即断网(含SSH)
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

  ```bash
  # 1. 排空节点
  kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force
  
  # 2. 在节点上执行（通过 SSH 或节点调试）
  # kubeadm reset --force  # ⚠️ 清理节点所有 K8s 配置
  # iptables -F && iptables -t nat -F && iptables -t mangle -F && iptables -X
  # ipvsadm --clear 2>/dev/null || true
  # rm -rf /etc/cni/net.d/*  # ⚠️ 删除系统/数据文件
  # systemctl restart containerd
  
  # 3. 重新加入集群
  # kubeadm join <control-plane-endpoint>:<port> --token <token> --discovery-token-ca-cert-hash sha256:<hash>
  
  # 4. 取消污点
  kubectl uncordon <node-name>
  ```
- **后置验证**:
  ```bash
  kubectl get node <node-name>
  kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name>
  ```
- **回滚命令**: 无法直接回滚，如 join 失败需重新生成 token

#### REM-005: 升级 CNI/CSI 插件到兼容版本
- **适用根因**: RC-006
- **影响说明**: 升级插件期间网络/存储功能可能短暂中断
- **审批提示**: "建议升级 CNI/CSI 插件到兼容版本，期间网络/存储可能受影响，是否批准？"
- **前置检查**:
  ```bash
  # 确认当前插件版本
  kubectl get daemonset -n kube-system -l k8s-app=calico-node -o jsonpath='{.items[0].spec.template.spec.containers[*].image}'
  
  # 确认目标兼容版本（需参考官方文档）
  # Calico v3.26+ 兼容 K8s 1.28-1.32
  # Cilium 1.14+ 兼容 K8s 1.28-1.32
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 备份当前插件配置
  kubectl get daemonset,deployment,configmap -n kube-system -l k8s-app=calico-node -o yaml > /tmp/calico-backup.yaml
  
  # 应用新版插件 YAML（需提前下载）
  kubectl apply -f https://raw.githubusercontent.com/projectcalico/calico/v3.27.0/manifests/calico.yaml
  
  # 等待滚动更新完成
  kubectl rollout status daemonset/calico-node -n kube-system --timeout=300s
  kubectl rollout status deployment/calico-kube-controllers -n kube-system --timeout=300s
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n kube-system -l k8s-app=calico-node
  kubectl run network-test --image=busybox:1.36 -n default --rm -it -- sh -c "ping -c 3 8.8.8.8"
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f /tmp/calico-backup.yaml
  ```

### 6.3 🔴 高风险（Agent 仅提供指导）

#### REM-006: 回滚 kubelet 到上一版本
- **适用根因**: RC-005, RC-007
- **影响说明**: 降级 kubelet 可能导致与 API Server 的兼容性问题
- **操作步骤**:
  1. 排空节点：`kubectl drain <node> --ignore-daemonsets`
  2. 在节点上停止 kubelet：`systemctl stop kubelet`
  3. 降级 kubelet 包（根据发行版）：
     - Ubuntu/Debian: `apt-get install kubelet=<old-version> kubeadm=<old-version>`
     - CentOS/RHEL: `yum downgrade kubelet-<old-version> kubeadm-<old-version>`
  4. 重启 kubelet：`systemctl start kubelet`
  5. 验证节点状态：`kubectl get node <node>`
  6. 取消排空：`kubectl uncordon <node>`
- **安全检查**: 确保目标版本与 API Server 版本兼容（版本差 <= 1 个小版本）
- **回滚方案**: 重新升级回当前版本

#### REM-007: 从 etcd 备份恢复
- **适用根因**: RC-004, RC-009
- **影响说明**: 恢复 etcd 备份将丢失备份时间点之后的所有集群变更
- **操作步骤**:
  1. 停止所有 API Server 实例
  2. 停止所有 etcd 实例
  3. 在每个 etcd 节点上执行恢复：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退

     ```bash
     etcdctl snapshot restore <backup-file> \
       --data-dir=/var/lib/etcd-restored \
       --initial-cluster=... \
       --initial-advertise-peer-urls=...
     ```
  4. 替换数据目录并启动 etcd
  5. 启动 API Server
  6. 验证集群状态
- **安全检查**: 确认备份文件完整且时间点在问题发生之前
- **回滚方案**: 保留原始 etcd 数据目录，如恢复失败可切回

### 6.4 ⚫ 严重（需高级 SRE 审批）

#### REM-008: 强制回滚整个集群版本
- **适用根因**: RC-009
- **审批要求**: 需架构师或高级 SRE 审批，通常需要变更窗口期
- **数据备份**: 必须已完成 etcd 全量备份和所有工作负载 YAML 导出
- **操作步骤**:
  1. 执行 etcd 备份恢复（REM-007）
  2. 降级所有控制平面节点：
     - 逐个节点执行 `kubeadm upgrade node` 到目标版本
     - 降级 kubelet、kubeadm、kubectl 包
  3. 降级所有工作节点：
     - 排空节点
     - 降级 kubelet、kubeadm
     - 重启 kubelet
  4. 验证所有组件版本一致
  5. 验证工作负载正常运行
- **回滚方案**: 由于这是回滚操作本身，如失败需从 etcd 备份重新恢复

## 验证确认

### 7.1 即时验证（修复后 1 分钟内）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# V1: 验证控制平面健康
kubectl get --raw /healthz
kubectl get --raw /healthz/etcd
# 预期: 均返回 "ok"

# V2: 验证节点版本一致性
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\n"}{end}'
# 预期: 所有节点版本与 API Server 版本差 <= 1 个小版本

# V3: 验证核心组件 Running
kubectl get pods -n kube-system
# 预期: 所有核心 Pod 状态为 Running

# V4: 验证测试 Pod 可创建和运行
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: upgrade-verify
  namespace: default
spec:
  containers:
  - name: test
    image: busybox:1.36
    command: ["sh", "-c", "sleep 30"]
EOF
kubectl wait --for=condition=Ready pod/upgrade-verify --timeout=60s
kubectl delete pod upgrade-verify
# 预期: Pod 成功创建并运行
```
### 7.2 短期监控（5-15 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| 节点就绪率 | `kubectl get nodes` | 所有节点 Ready | 任何节点 NotReady |
| API Server 延迟 | `apiserver_request_duration_seconds` | 稳定在正常范围 | P99 > 1s |
| etcd Leader | `etcd_server_has_leader == 1` | 值为 1 | 值为 0 |
| Pod 重建成功率 | `kubectl get pods --all-namespaces` | CrashLoopBackOff 减少 | 新增 CrashLoopBackOff |
| CNI Pod 状态 | `kubectl get pods -n kube-system -l k8s-app=calico-node` | 全部 Running | 任何非 Running |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：
- [ ] API Server /healthz 和 /healthz/etcd 返回 ok
- [ ] 所有节点状态为 Ready
- [ ] 所有控制平面 Pod 状态为 Running
- [ ] etcd 集群有 Leader 且所有 endpoint 健康
- [ ] 测试 Pod 可以正常创建并运行
- [ ] 无新的废弃 API 使用错误

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| 节点版本偏移 | `kubectl get nodes` | 每 4 小时 | 如有版本不一致，安排统一升级 |
| 证书有效期 | `openssl x509 -in` | 每次升级后 | 如即将过期，提前轮转 |
| 插件兼容性 | 官方兼容性矩阵 | 升级后 | 如有不兼容，计划插件升级 |
| 废弃 API 使用 | API Server 审计日志 | 持续监控 | 如有使用，通知应用团队迁移 |

## 升级协议

### 8.1 自动升级条件

| 条件 | 说明 |
|------|------|
| 诊断超时 | 诊断工作流执行超过 30 分钟未确认根因 |
| 修复失败 | 同一修复操作执行 2 次仍未通过验证 |
| 严重性升级 | 初始分级为 P2 但影响面扩大到 P0 级别 |
| 未知根因 | 诊断完成但无法匹配任何已知根因 |
| 回滚失败 | 升级回滚操作执行失败 |

### 8.2 升级消息模板

```
【{severity}】{skill_name} - {cluster_name}
- 问题概述: 集群升级/迁移过程中出现 {component} 异常
- 影响范围: {affected_nodes}/{total_nodes} 节点，{affected_workloads} 工作负载
- 已完成诊断: {completed_steps}
- 初步发现: {root_cause_candidate}
- 升级阶段: {upgrade_phase}
- 需要: {action_needed}
- 工单编号: {ticket_id}
```

### 8.3 交接信息包

升级时，Agent 需准备以下信息：
1. 完整诊断路径和每步输出
2. 集群升级前后的版本对比
3. 已排除的根因及原因
4. 可能的根因假设
5. etcd 备份位置和时间点
6. 最近 30 分钟的关键事件时间线
7. 受影响的 Namespace 和工作负载列表

## 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| kubeadm 自动证书轮转 | 支持 | 支持 | 支持 | 支持 | 支持 |
| 废弃 API 移除进度 | batch/v1beta1 移除 | flowcontrol/v1beta2 移除 | - | - | - |
| kubelet 版本兼容性 | +/-1 版本 | +/-1 版本 | +/-1 版本 | +/-1 版本 | +/-1 版本 |
| containerd 最低版本 | 1.6+ | 1.6+ | 1.6+ | 1.6+ | 1.7+ |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubeadm upgrade plan` | 标准输出 | 标准输出 | 标准输出 | 标准输出 | 标准输出 |
| `kubectl get componentstatuses` | 支持（已废弃） | 支持 | 支持 | 支持 | 支持 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| Deployment | apps/v1 | apps/v1 | apps/v1 | apps/v1 | apps/v1 |
| Ingress | networking.k8s.io/v1 | networking.k8s.io/v1 | networking.k8s.io/v1 | networking.k8s.io/v1 | networking.k8s.io/v1 |
| CronJob | batch/v1 | batch/v1 | batch/v1 | batch/v1 | batch/v1 |
| PodDisruptionBudget | policy/v1 | policy/v1 | policy/v1 | policy/v1 | policy/v1 |
| HorizontalPodAutoscaler | autoscaling/v2 | autoscaling/v2 | autoscaling/v2 | autoscaling/v2 | autoscaling/v2 |

## 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| 将网络问题误诊为升级问题 | 升级后 Pod 无法通信 | CNI 插件版本不兼容 | 检查 CNI Pod 状态和版本兼容性矩阵 |
| 将证书过期误诊为升级问题 | 升级后 API Server 无法访问 | 证书自然过期，与升级时间重合 | 检查证书有效期是否确实与升级相关 |
| 将资源不足误诊为升级问题 | 升级后节点 NotReady | 节点 DiskPressure/MemoryPressure | 检查节点 Conditions |
| 将镜像拉取失败误诊为升级问题 | 控制平面 Pod 无法启动 | 镜像仓库不可达 | 检查镜像拉取事件 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：
- kubeadm 升级原理 → `domain-01-cluster-fundamentals/kubeadm-upgrade-mechanism.md`
- etcd 升级和数据迁移 → `domain-01-cluster-fundamentals/etcd-upgrade.md`
- Kubernetes 版本偏移策略 → `domain-01-cluster-fundamentals/version-skew-policy.md`
- 废弃 API 迁移指南 → `domain-01-cluster-fundamentals/deprecated-api-migration.md`
- CNI 插件兼容性矩阵 → `domain-03-networking-traffic/cni-compatibility.md`

### 10.3 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-05 | v1.0 | 初始版本 | 覆盖集群升级与迁移故障诊断 |

## 云厂商特异性

| 平台 | 差异 | 诊断命令 | 备注 |
|------|------|---------|------|
| ACK | 托管控制平面，用户无法执行 kubeadm upgrade | `aliyun cs GET /clusters/{cluster-id}` | 升级由平台托管，问题需提工单 |
| EKS | 托管控制平面，节点升级通过 EKS Managed Node Group | `aws eks describe-nodegroup` | 注意 AMI 版本与 K8s 版本对应 |
| GKE | 自动升级通道（Release Channel）可能导致意外升级 | `gcloud container clusters describe` | 检查维护窗口和发布通道设置 |
| AKS | 支持自动升级和 Planned Maintenance | `az aks show` | 注意 Node Image Upgrade 与 K8s 升级的区别 |

## 自动化集成接口

### 12.1 脚本入口

- **diagnose-quick.sh**: Phase 1 快速诊断脚本入口
  - 调用约定: `./scripts/diagnose-quick.sh --cluster <CLUSTER_NAME>`
  - 输出: 版本分布、组件状态、废弃 API 统计
- **diagnose-deep.sh**: Phase 2 深度诊断脚本入口
  - 调用约定: `./scripts/diagnose-deep.sh --cluster <CLUSTER_NAME>`
  - 输出: etcd 健康状态、CNI/CSI 版本兼容性、证书有效期
- **verify.sh**: 修复后验证脚本入口
  - 调用约定: `./scripts/verify.sh --cluster <CLUSTER_NAME>`
  - 输出: 控制平面健康、节点就绪、测试 Pod 创建

### 12.2 Webhook 回调

- **告警路由**: 从 AlertManager/Prometheus 告警自动触发 Skill
- **回调格式**: JSON payload 含 skill_id、trigger_source、context

### 12.3 输出规范

| 脚本 | 用途 | 示例调用 |
|------|------|----------|
| diagnose-quick.sh | Phase 1 快速检查 | `./scripts/diagnose-quick.sh --cluster prod` |
| diagnose-deep.sh | Phase 2 深度检查 | `./scripts/diagnose-deep.sh --cluster prod` |
| verify.sh | 修复后验证 | `./scripts/verify.sh --cluster prod` |

### 12.4 Webhook 配置示例

```yaml
# AlertManager Webhook 示例
receivers:
- name: skill-trigger
  webhook_configs:
  - url: 'http://agent-gateway/skill/SKILL-CP-002'
    send_resolved: true
```

### 12.5 输出 JSON Schema

```json
{
  "skill_id": "SKILL-CP-002",
  "findings": [
    { "step": "D1.1", "result": "kubelet version skew detected", "severity": "critical" }
  ],
  "root_cause_candidates": [
    { "rc_id": "RC-002", "confidence": 0.90, "evidence": ["D1.1", "D2.1"] }
  ],
  "recommended_action": {
    "rem_id": "REM-004",
    "risk_level": "medium",
    "command": "kubeadm reset + join on node",
    "rollback": "re-upgrade kubelet to target version"
  }
}
```

---

## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/技能体系/11-control-plane-failure.md|SKILL-CP-001 etcd 与控制平面故障诊断]]
- [[domain-10-troubleshooting-diagnostics/技能体系/24-namespace-quota-limitrange.md|SKILL-CONFIG-002 Namespace/Quota/LimitRange 问题]]
- [[domain-10-troubleshooting-diagnostics/技能体系/19-node-resource-pressure.md|SKILL-NODE-002 节点资源压力诊断]]
- [[domain-10-troubleshooting-diagnostics/技能体系/20-networkpolicy-connectivity.md|SKILL-NET-004 NetworkPolicy 连通性问题]]
- [[domain-10-troubleshooting-diagnostics/技能体系/21-statefulset-failure.md|SKILL-WORK-002 StatefulSet 故障诊断]]
- [[domain-10-troubleshooting-diagnostics/技能体系/22-daemonset-failure.md|SKILL-WORK-003 DaemonSet 故障诊断]]
- [[domain-10-troubleshooting-diagnostics/技能体系/23-job-cronjob-failure.md|SKILL-WORK-004 Job/CronJob 故障诊断]]
- [[domain-10-troubleshooting-diagnostics/基础设施排障/34-upgrade-migration-troubleshooting.md|升级迁移深度排查]]
- [[domain-10-troubleshooting-diagnostics/FTA故障树/list/cluster-upgrade-fta.md|升级迁移故障树分析]]

## Related

- [[domain-17-system-foundation/速查卡/k8s.md|k8s]]
- [[domain-10-troubleshooting-diagnostics/技能体系/19-node-resource-pressure.md|19-node-resource-pressure]]
- [[domain-10-troubleshooting-diagnostics/技能体系/20-networkpolicy-connectivity.md|20-networkpolicy-connectivity]]
- [[domain-10-troubleshooting-diagnostics/技能体系/24-namespace-quota-limitrange.md|24-namespace-quota-limitrange]]
- [[domain-10-troubleshooting-diagnostics/技能体系/22-daemonset-failure.md|22-daemonset-failure]]

```

<!-- risk-assessed -->
