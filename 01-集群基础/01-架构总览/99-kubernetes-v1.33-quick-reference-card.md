---
title: Kubernetes v1.33 速查卡
description: '| **Scheduler Queueing Hints** | **Beta** | 调度器队列提示，性能提升 10-30% | ✅
  默认启用 |'
summary: '| **Scheduler Queueing Hints** | **Beta** | 调度器队列提示，性能提升 10-30% | ✅ 默认启用
  |'
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- etcd
- apiserver
- kubelet
- scheduler
- istio
- containerd
- statefulset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes v1.33 速查卡 是什么
- 如何 Kubernetes v1.33 速查卡
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Kubernetes
- v1.33
- 速查卡
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- service-mesh-basics
- etcd-basics
- gpu-scheduling-basics
- observability-basics
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
- type: domain
  path: ../容器运行时/
  label: '相关知识域: 容器运行时'
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] v1.33 速查卡

> **一页纸速查**: v1.29 → v1.33 所有关键变更  
> **最后更新**: 2026-04-24

---

<!-- chunk: 🚀 v1.33 核心变更 (最新) -->
## 🚀 v1.33 核心变更 (最新)

| 特性 | 状态 | 一句话说明 | 是否启用 |
|:---|:---|:---|:---|
| **Sidecar 容器** | **GA** | init 容器支持 `restartPolicy: Always`，自动重启 | ✅ 默认启用 |
| **DRA** | **GA** | GPU/FPGA 动态资源分配，替代 Device Plugin | ⚠️ 需显式启用 FG |
| **TopologyManager Per Pod** | **GA** | Pod 级 NUMA 拓扑策略 | ⚠️ 需显式启用 FG |
| **Scheduler Queueing Hints** | **Beta** | 调度器队列提示，性能提升 10-30% | ✅ 默认启用 |
| **[[kubelet|Kubelet]] Resource Metrics** | **Beta** | `/metrics/resource` 端点，替代 Summary API | ✅ 默认启用 |
| **In-Place Pod Resize** | **Alpha** | 原地调整 Pod 资源，无需重启 | ❌ 需启用 FG |
| **Cross-Namespace PVC** | **Alpha** | PVC 跨命名空间引用数据源 | ❌ 需启用 FG |
| **PodIndexLabel** | **GA** | [[StatefulSet|StatefulSet]] 自动生成 `apps.kubernetes.io/pod-index` | ✅ 默认启用 |
| **Windows HostProcess** | **GA** | Windows 容器 HostProcess 模式稳定 | ✅ 默认启用 |

---

<!-- chunk: 📈 版本演进时间线 -->
## 📈 版本演进时间线

```
v1.29 (2023.12) ──► v1.30 (2024.04) ──► v1.31 (2024.08) ──► v1.32 (2024.12) ──► v1.33 (2025.04)
    │                    │                    │                    │                    │
    ├── Sidecar Beta     ├── CEL Admission GA ├── AppArmor GA      ├── DRA Beta         ├── Sidecar GA
    ├── ReadWriteOncePod ├── SchedulingGates  ├── Parallel Pulls   ├── TopologyManager  ├── DRA GA
    │   GA               │   GA               │   默认启用         │   Per Pod Beta     ├── Queueing Hints
    └── KMS v2 GA        └── BoundSA Token    └── nftables Alpha   └── Pod-level        │   Beta
                           GA                    └── OpenTelemetry    Resources Alpha    └── Kubelet
                                                  Tracing GA                            Metrics Beta
```

---

<!-- chunk: ⚡ 快速启用新特性 -->
## ⚡ 快速启用新特性

### Sidecar 容器 (GA, 立即可用)

```yaml
spec:
  initContainers:
  - name: proxy
    image: istio/proxyv2:1.24
    restartPolicy: Always      # ← 这就是全部
```

### DRA (GA, 需启用 Feature Gate)

```bash
# kube-apiserver, kube-scheduler, kubelet
--feature-gates=DynamicResourceAllocation=true
```

### In-Place Resize (Alpha, 实验性)

```bash
# kubelet
--feature-gates=InPlacePodVerticalScaling=true
```

```yaml
metadata:
  annotations:
    resize.policy/container.app: "RestartNotRequired"
```

---

<!-- chunk: 🔧 kubectl 快捷命令 -->
## 🔧 kubectl 快捷命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 版本检查
kubectl version

# 查看 Feature Gates
kubectl get --raw /api/v1/nodes/NODE/proxy/configz | jq '.kubeletconfig.featureGates'

# 检查已弃用 API
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis

# 查看 ValidatingAdmissionPolicy
kubectl get validatingadmissionpolicies

# Sidecar 容器检查
kubectl get pods -A -o json | jq '.items[].spec.initContainers[]? | select(.restartPolicy == "Always") | .name'

# 节点日志 (v1.30+, Alpha)
kubectl alpha node-logs NODE --service=kubelet

# 调试 Profile (v1.32+)
kubectl debug POD --profile=netadmin
```
---

<!-- chunk: 🔄 升级路径 -->
## 🔄 升级路径

```
当前版本 → 目标版本
    │
    ├── ≤v1.29 → 立即升级到 v1.33
    ├── v1.30  → 升级到 v1.33
    ├── v1.31  → 升级到 v1.33
    ├── v1.32  → 评估后升级到 v1.33
    └── v1.33  → 保持，等待 v1.34
```

---

<!-- chunk: 📋 生产检查清单 -->
## 📋 生产检查清单

- [ ] 集群版本 ≥ v1.32 (v1.33 推荐)
- [ ] 所有节点 containerd ≥ 1.7.18
- [ ] etcd ≥ 3.5.15
- [ ] CSI 驱动已安装 (in-tree 驱动已弃用)
- [ ] CCM 已部署 (kubelet --cloud-provider 已弃用)
- [ ] 无已弃用 API 使用
- [ ] Pod Security Admission 已配置
- [ ] ServiceAccount Token 自动轮转正常
- [ ] 匿名用户未绑定 cluster-admin

---

<!-- chunk: 📦 v1.32 核心变更 -->
## 📦 v1.32 核心变更

| 特性 | 状态 | 一句话说明 | 是否启用 |
|:---|:---|:---|:---|
| **DRA (Structured Parameters)** | **Beta** | 结构化参数动态资源分配 | ⚠️ 需启用 FG |
| **TopologyManager Per Pod** | **Beta** | Pod 级 NUMA 拓扑对齐 | ⚠️ 需启用 FG |
| **Pod-level Resources** | **Alpha** | Pod 级资源请求/限制 | ❌ 需启用 FG |
| **nftables ProxyMode** | **Beta** | kube-proxy nftables 后端 | ⚠️ 需启用 FG |
| **RecursiveReadOnlyMounts** | **Beta** | 递归只读挂载 | ✅ 默认启用 |
| **UserNamespaces** | **Beta** | Pod 用户命名空间隔离 | ⚠️ 需启用 FG |
| **Job ManagedBy** | **Beta** | Job 外部控制器管理 | ✅ 默认启用 |
| **APIServer Tracing** | **GA** | 控制平面 OpenTelemetry 追踪 | ✅ 默认启用 |

---

<!-- chunk: 🔗 组件版本兼容性矩阵 -->
## 🔗 组件版本兼容性矩阵

| 组件 | v1.31 推荐 | v1.32 推荐 | v1.33 推荐 | 说明 |
|:---|:---|:---|:---|:---|
| **etcd** | 3.5.15+ | 3.5.17+ | 3.5.18+ / 3.6.x | 3.6 需评估兼容性 |
| **containerd** | 1.7.18+ | 1.7.22+ | 2.0+ (可选) | containerd 2.0 需验证 CSI |
| **CoreDNS** | 1.11.1+ | 1.11.3+ | 1.12.0+ | 支持 nftables |
| **CNI (Calico)** | 3.28+ | 3.29+ | 3.30+ | eBPF 模式需 3.29+ |
| **CNI (Cilium)** | 1.16+ | 1.17+ | 1.17+ | Tetragon 1.3+ |
| **Ingress (NGINX)** | 1.11+ | 1.12+ | 1.12+ | 注意 Gateway API 迁移 |
| **cert-manager** | 1.15+ | 1.16+ | 1.17+ | 支持 LiteralSubject |
| **Prometheus** | 2.53+ | 2.55+ | 3.0+ (可选) | Prometheus 3.0 UI 重构 |
| **ArgoCD** | 2.12+ | 2.13+ | 2.14+ | 支持 v1.33 API |
| **Helm** | 3.15+ | 3.16+ | 3.17+ | 注意 CRD API 版本 |

---

<!-- chunk: 🎛️ Feature Gate 完整参考 -->
## 🎛️ Feature Gate 完整参考

### v1.33 关键 Feature Gates

| Feature Gate | 阶段 | 默认 | 影响组件 | 说明 |
|:---|:---|:---|:---|:---|
| `DynamicResourceAllocation` | GA | true | apiserver, scheduler, kubelet | GPU/FPGA 动态分配 |
| `SidecarContainers` | GA | true | kubelet, apiserver | Sidecar 容器支持 |
| `InPlacePodVerticalScaling` | Alpha | false | kubelet, apiserver | 原地资源调整 |
| `SchedulerQueueingHints` | Beta | true | scheduler | 调度性能优化 |
| `CrossNamespaceVolumeDataSource` | Alpha | false | apiserver, controller | 跨 NS PVC |
| `UserNamespaces` | Beta | false | kubelet | 用户命名空间 |
| `NFTablesProxyMode` | Beta | false | kube-proxy | nftables 后端 |
| `RecursiveReadOnlyMounts` | Beta | true | kubelet | 递归只读 |
| `PodLevelResources` | Alpha | false | kubelet, scheduler | Pod 级资源 |
| `StructuredAuthenticationConfiguration` | Beta | true | apiserver | 结构化认证配置 |

### Feature Gate 管理命令

```bash
# 🟢 查看当前节点 Feature Gates
kubectl get --raw /api/v1/nodes/NODE/proxy/configz | \
  jq '.kubeletconfig.featureGates | to_entries[] | select(.value == true)'

# 🟢 查看 API Server Feature Gates
kubectl -n kube-system get pod -l component=kube-apiserver -o yaml | \
  grep -A 50 'feature-gates'

# 🟢 查看 Scheduler Feature Gates
kubectl -n kube-system get pod -l component=kube-scheduler -o yaml | \
  grep 'feature-gates'

# 🟡 修改 kubelet Feature Gate（通过 KubeletConfiguration）
# /var/lib/kubelet/config.yaml:
# featureGates:
#   InPlacePodVerticalScaling: true
#   UserNamespaces: true
```

---

<!-- chunk: 🏗️ 生产升级决策框架 -->
## 🏗️ 生产升级决策框架

### 升级决策树

```
当前版本是什么？
│
├── ≤ v1.29 (EOL)
│   └── 🔴 立即规划升级！安全补丁已停止
│       ├── 目标: v1.32 (LTS 策略) 或 v1.33 (最新)
│       └── 注意: 跨 2+ 版本需逐版本升级
│
├── v1.30
│   └── 🟡 建议升级至 v1.32/v1.33
│       ├── 检查: extensions API 已移除
│       └── 窗口: 3 个月内完成
│
├── v1.31
│   └── 🟢 正常维护周期
│       ├── 可选升级至 v1.33
│       └── 关注: Beta API 变更
│
├── v1.32
│   └── 🟢 评估 v1.33 新特性
│       ├── Sidecar GA 可简化 Istio 部署
│       └── DRA GA 可替代 Device Plugin
│
└── v1.33 (最新)
    └── ✅ 保持，等待 v1.34
```

### 升级前自动化检查脚本

```bash
#!/bin/bash
# 🟢 pre-upgrade-check.sh — 升级前全面检查
set -euo pipefail

TARGET_VERSION="${1:-1.33}"
echo "══════════════════════════════════════════"
echo "  Kubernetes 升级前检查 → v${TARGET_VERSION}"
echo "══════════════════════════════════════════"

# 1. 当前版本
echo -e "\n📌 [1/8] 当前集群版本"
kubectl version --short 2>/dev/null || kubectl version

# 2. 节点状态
echo -e "\n📌 [2/8] 节点状态"
NOT_READY=$(kubectl get nodes --no-headers | grep -v ' Ready' | wc -l)
if [ "$NOT_READY" -gt 0 ]; then
  echo "🔴 有 $NOT_READY 个节点未就绪！"
  kubectl get nodes | grep -v ' Ready'
else
  echo "✅ 所有节点就绪"
fi

# 3. 废弃 API
echo -e "\n📌 [3/8] 废弃 API 使用"
DEPRECATED=$(kubectl get --raw /metrics 2>/dev/null | grep 'apiserver_requested_deprecated_apis' | grep -v '^#' | wc -l)
if [ "$DEPRECATED" -gt 0 ]; then
  echo "🔴 发现 $DEPRECATED 个废弃 API 调用"
  kubectl get --raw /metrics | grep 'apiserver_requested_deprecated_apis' | grep -v '^#'
else
  echo "✅ 无废弃 API 使用"
fi

# 4. PDB 覆盖
echo -e "\n📌 [4/8] PodDisruptionBudget 覆盖"
kubectl get pdb -A --no-headers | wc -l
echo "  (生产工作负载应全部配置 PDB)"

# 5. etcd 健康
echo -e "\n📌 [5/8] etcd 集群健康"
ETCD_POD=$(kubectl -n kube-system get pod -l component=etcd -o name | head -1)
if [ -n "$ETCD_POD" ]; then
  kubectl -n kube-system exec $ETCD_POD -- etcdctl endpoint health --cluster 2>/dev/null || echo "⚠️ 无法直接检查 etcd"
fi

# 6. 证书过期
echo -e "\n📌 [6/8] 证书有效期"
kubeadm certs check-expiration 2>/dev/null || echo "⚠️ 非 kubeadm 集群，跳过"

# 7. 资源余量
echo -e "\n📌 [7/8] 集群资源余量"
kubectl top nodes 2>/dev/null | head -10 || echo "⚠️ metrics-server 未部署"

# 8. 关键 Operator 版本
echo -e "\n📌 [8/8] 关键 Operator/Addon 版本"
for ns in kube-system monitoring ingress-nginx cert-manager; do
  echo "  [$ns]"
  kubectl -n $ns get deploy -o custom-columns='NAME:.metadata.name,IMAGE:.spec.template.spec.containers[0].image' --no-headers 2>/dev/null | head -5
done

echo -e "\n══════════════════════════════════════════"
echo "  检查完成。请根据结果决定是否继续升级。"
echo "══════════════════════════════════════════"
```

---

<!-- chunk: 📊 性能基准与调优 -->
## 📊 性能基准与调优

### v1.33 性能改进

| 改进项 | 版本 | 效果 | 度量方式 |
|:---|:---|:---|:---|
| Scheduler Queueing Hints | v1.33 Beta | 调度吞吐 +10-30% | `scheduler_scheduling_attempt_total` |
| APIServer Tracing | v1.32 GA | 请求延迟可观测 | OTel traces |
| Kubelet Resource Metrics | v1.33 Beta | 替代 Summary API | `/metrics/resource` |
| Parallel Image Pulls | v1.31+ | 镜像拉取加速 | `kubelet_image_pull_duration_seconds` |
| Consistent List from Cache | v1.31+ | List 请求延迟降低 | `apiserver_request_duration_seconds` |

### 控制平面调优参数

```yaml
# kube-apiserver 生产推荐配置
apiVersion: kubeadm.k8s.io/v1beta4
kind: ClusterConfiguration
apiServer:
  extraArgs:
    # 请求并发
    max-requests-inflight: "400"          # 默认 400
    max-mutating-requests-inflight: "200" # 默认 200
    # 缓存
    watch-cache-sizes: "pods=1000,nodes=100"  # 大集群增加
    # 审计
    audit-log-maxage: "30"
    audit-log-maxbackup: "10"
    audit-log-maxsize: "100"
    # 性能
    enable-priority-and-fairness: "true"  # APF 默认开启
---
# kube-scheduler 调优
apiVersion: kubescheduler.config.k8s.io/v1
kind: KubeSchedulerConfiguration
percentageOfNodesToScore: 50  # 大集群降低（默认 0=自适应）
```

### etcd 性能基线

| 指标 | 健康阈值 | 告警阈值 | 检查命令 |
|:---|:---|:---|:---|
| WAL fsync 延迟 | < 10ms | > 25ms | `etcd_disk_wal_fsync_duration_seconds` |
| Backend commit 延迟 | < 25ms | > 100ms | `etcd_disk_backend_commit_duration_seconds` |
| DB 大小 | < 4GB | > 6GB | `etcd_mvcc_db_total_size_in_bytes` |
| Leader 切换频率 | < 1次/天 | > 3次/天 | `etcd_server_leader_changes_seen_total` |
| 慢查询 | < 5/min | > 20/min | `etcd_server_slow_apply_total` |

---

<!-- chunk: 🚨 升级故障排查 -->
## 🚨 升级故障排查

| 故障现象 | 可能原因 | 诊断命令 | 修复措施 |
|:---|:---|:---|:---|
| kubelet 升级后 CrashLoopBackOff | 配置不兼容 | `journalctl -u kubelet --since '10min ago'` | 回滚 kubelet 二进制，修正配置 |
| API Server 无法启动 | Feature Gate 冲突 | `kubectl -n kube-system logs kube-apiserver-*` | 移除已 GA 的 FG（不再需要显式设置） |
| Pod 调度失败 | Scheduler 配置变更 | `kubectl describe pod <pod>` | 检查调度插件兼容性 |
| 节点 NotReady | CNI 不兼容 | `kubectl -n kube-system logs -l k8s-app=calico-node` | 升级 CNI 到兼容版本 |
| etcd 数据不一致 | 升级中断 | `etcdctl endpoint status --cluster -w table` | 从快照恢复 |
| Webhook 拒绝请求 | admissionReviewVersions 不匹配 | `kubectl get validatingwebhookconfigurations -o yaml` | 更新 Webhook 配置 |
| DNS 解析失败 | CoreDNS 版本不兼容 | `kubectl -n kube-system logs -l k8s-app=kube-dns` | 升级 CoreDNS |

### 回滚策略

```bash
# 🔴 控制平面回滚（kubeadm 集群）
# 1. 回滚 API Server 静态 Pod 镜像
# /etc/kubernetes/manifests/kube-apiserver.yaml:
#   image: registry.k8s.io/kube-apiserver:v1.32.x  ← 改回旧版本

# 2. 回滚 kubelet
# apt-get install -y kubelet=1.32.x-1.1 kubectl=1.32.x-1.1
# systemctl restart kubelet

# 3. 验证
kubectl get nodes
kubectl get cs

# ⚠️ 注意: etcd 数据格式升级后通常不可逆
# 如需回滚 etcd，必须从升级前快照恢复
```

---

<!-- chunk: 📋 生产检查清单（扩展版） -->
## 📋 生产检查清单（扩展版）

### 升级前

- [ ] 集群版本 ≥ v1.32 (v1.33 推荐)
- [ ] 所有节点 containerd ≥ 1.7.18
- [ ] etcd ≥ 3.5.15（推荐 3.5.18+）
- [ ] CSI 驱动已安装 (in-tree 驱动已弃用)
- [ ] CCM 已部署 (kubelet --cloud-provider 已弃用)
- [ ] 无已弃用 API 使用（kubent/pluto 扫描通过）
- [ ] Pod Security Admission 已配置
- [ ] ServiceAccount Token 自动轮转正常
- [ ] 匿名用户未绑定 cluster-admin
- [ ] etcd 快照已备份（升级前 1 小时内）
- [ ] 所有 PDB 配置正确
- [ ] CNI 版本兼容目标 K8s 版本
- [ ] 关键 Operator 兼容目标版本
- [ ] 监控告警正常（升级期间需密切观察）

### 升级后

- [ ] 所有节点 Ready
- [ ] 系统 Pod 全部 Running
- [ ] DNS 解析正常
- [ ] Ingress/Gateway 流量正常
- [ ] 存储卷挂载正常
- [ ] 监控指标采集正常
- [ ] 日志采集正常
- [ ] 业务应用健康检查通过
- [ ] 无新增告警
- [ ] 性能指标无退化

---

<!-- chunk: 📚 相关文档 -->
## 📚 相关文档

| 文档 | 内容 |
|:---|:---|
| [99-kubernetes-v1.29-v1.33-features-guide.md](./99-kubernetes-v1.29-v1.33-features-guide.md) | 按版本详解 |
| [99-kubernetes-core-components-v1.29-v1.33-update.md](./99-kubernetes-core-components-v1.29-v1.33-update.md) | 按组件速查 |
| [99-kubernetes-v1.33-upgrade-guide.md](../06-%E5%8D%87%E7%BA%A7%E8%B7%AF%E5%BE%84/99-kubernetes-v1.33-upgrade-guide.md) | 升级实操 |
| [99-kubectl-v1.29-v1.33-new-commands-guide.md](../05-kubectl/99-kubectl-v1.29-v1.33-new-commands-guide.md) | kubectl 新命令 |
| [99-kubernetes-v1.33-production-best-practices.md](./99-kubernetes-v1.33-production-best-practices.md) | 生产最佳实践 |
| [99-kubernetes-version-lifecycle-support-policy.md](../04-API%E7%89%88%E6%9C%AC/99-kubernetes-version-lifecycle-support-policy.md) | 版本生命周期 |
| [99-kubernetes-v1.33-ecosystem-compatibility-matrix.md](./99-kubernetes-v1.33-ecosystem-compatibility-matrix.md) | 兼容性矩阵 |

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 MOC
- [[01-集群基础/README.md|Domain-1: Kubernetes架构基础]]
- Domain-1 架构基础 — 开源项目索引
- Kubernetes 架构全景图
- Kubernetes 核心组件深度剖析
- 03 - 功能和API表
- 04 - Kubernetes 源码结构深度解析
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)

## See Also

- 99-kubernetes-v1.33-practical-cookbook
- 99-kubernetes-v1.33-production-best-practices
- 99-kubernetes-v1.33-upgrade-guide
- 99-kubernetes-version-lifecycle-support-policy


<!-- risk-assessed -->
