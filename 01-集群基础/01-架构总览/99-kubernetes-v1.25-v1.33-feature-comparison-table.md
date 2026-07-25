---
title: Kubernetes v1.25 - v1.33 特性对比总表
description: '## 二、网络 (Networking)'
summary: 'kubectl get --raw /api/v1/nodes/NODE/proxy/configz | jq '.kubeletconfig.featureGates''
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- etcd
- apiserver
- kubelet
- scheduler
- containerd
- hpa
- pdb
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 10min
intent_queries:
- Kubernetes v1.25 - v1.33 特性对比总表 是什么
- 如何 Kubernetes v1.25 - v1.33 特性对比总表
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Kubernetes
- v1.25
- v1.33
- 特性对比总表
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
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




# [[Kubernetes|Kubernetes]] v1.25 - v1.33 特性对比总表

> **适用版本**: Kubernetes v1.25 - v1.33  
> **最后更新**: 2026-04-24  
> **用途**: 全版本特性横向对比，快速定位功能引入版本

---

<!-- chunk: 一、工作负载 (Workloads) -->
## 一、工作负载 (Workloads)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **Sidecar 容器** | - | - | Alpha | Beta | Beta | GA | GA | GA | **GA** | init 容器 restartPolicy: Always |
| **ReadWriteOncePod** | - | - | - | - | **GA** | GA | GA | GA | GA | PVC 单 Pod 独占 |
| **[[17-系统基础/06-知识字典/scheduling/pod-scheduling-readiness.md|Pod Scheduling Readiness]]** | - | - | - | - | Beta | **GA** | GA | GA | GA | SchedulingGates |
| **In-Place Pod Resize** | - | - | - | - | Beta | Beta | Beta | Beta | **Alpha** | 原地调整资源 |
| **PodIndexLabel** | - | - | - | - | - | - | - | - | **GA** | [[StatefulSet|StatefulSet]] 自动标签 |
| **Job Mutable Scheduling Directives** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **PodDisruptionBudget (v1)** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **[[CronJob|CronJob]] 时区支持** | - | - | - | - | - | - | - | - | - | v1.25+ 已稳定 |
| **Job Tracking with [[Finalizers|Finalizers]]** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

<!-- chunk: 二、网络 (Networking) -->
## 二、网络 (Networking)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **Gateway API (v1)** | - | - | - | - | - | - | **GA** | GA | GA | Ingress 替代方案 |
| **nftables kube-proxy** | - | - | - | - | - | - | Alpha | Alpha | **Beta** | 新网络后端 |
| **IPv6 DualStack** | GA | GA | GA | GA | GA | GA | GA | GA | GA | 双栈网络 |
| **EndpointSlice (v1)** | GA | GA | GA | GA | GA | GA | GA | GA | GA | 大规模 Service |
| **Service Traffic Distribution** | - | - | - | - | - | - | Alpha | Alpha | Alpha | 拓扑感知路由 |
| **Network Policy Status** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

<!-- chunk: 三、存储 (Storage) -->
## 三、存储 (Storage)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **ReadWriteOncePod** | - | - | - | - | **GA** | GA | GA | GA | GA | 单 Pod 独占 |
| **CSI Migration (in-tree)** | - | - | - | - | - | 弃用 | 弃用 | 弃用 | 弃用 | 迁移到 CSI |
| **VolumeGroupSnapshot** | - | - | Beta | Beta | Beta | Beta | Beta | Beta | Beta | 卷组快照 |
| **VolumeAttributesClass** | - | - | - | - | - | - | - | - | **Alpha** | 动态存储性能 |
| **Cross-Namespace PVC** | - | - | - | - | - | - | - | - | **Alpha** | 跨命名空间引用 |
| **PV Last Phase Time** | - | - | - | - | - | - | **GA** | GA | GA | 状态转换时间 |
| **Retroactive Default SC** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

<!-- chunk: 四、安全 (Security) -->
## 四、安全 (Security)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **Pod Security Admission** | **GA** | GA | GA | GA | GA | GA | GA | GA | GA | PSP 替代 |
| **PodSecurityPolicy 移除** | **移除** | - | - | - | - | - | - | - | - | 已移除 |
| **ValidatingAdmissionPolicy** | - | Alpha | Beta | Beta | Beta | **GA** | GA | GA | GA | CEL 准入 |
| **BoundServiceAccountToken** | - | - | - | - | - | **GA** | GA | GA | GA | 1h 过期 |
| **AppArmor Support** | - | - | - | - | - | - | **GA** | GA | GA | Linux 安全 |
| **User Namespaces** | Alpha | Alpha | Beta | Beta | Beta | Beta | **GA** | GA | GA | 用户隔离 |
| **匿名用户安全加固** | - | - | - | - | - | **默认** | 默认 | 默认 | 默认 | 禁止匿名 cluster-admin |
| **KMS v2** | - | - | - | - | **GA** | GA | GA | GA | GA | etcd 加密 |
| **SELinux Mount** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

<!-- chunk: 五、调度 (Scheduling) -->
## 五、调度 (Scheduling)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **DRA (Dynamic Resource Allocation)** | Alpha | Alpha | Alpha | Beta | Beta | Beta | Beta | **Beta** | **GA** | GPU/FPGA 分配 |
| **TopologyManager Per Pod** | - | - | - | - | - | - | - | **Beta** | **GA** | NUMA 拓扑 |
| **Scheduler Queueing Hints** | - | - | - | - | - | - | - | Alpha | **Beta** | 队列优化 |
| **Pod Scheduling Readiness** | - | - | - | - | Beta | **GA** | GA | GA | GA | 调度门控 |
| **MatchLabelKeys in PDB** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **MinDomains in PodTopologySpread** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

<!-- chunk: 六、可观测性 (Observability) -->
## 六、可观测性 (Observability)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **OpenTelemetry Tracing (kubelet)** | - | - | - | - | - | - | **GA** | GA | GA | 链路追踪 |
| **Kubelet Resource Metrics** | - | - | - | - | - | - | - | - | **Beta** | 资源指标端点 |
| **Node Log Query** | - | - | - | - | - | Alpha | Alpha | Alpha | Alpha | 节点日志查询 |
| **Component SLIs** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **PodAndContainerStatsFromCRI** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

<!-- chunk: 七、节点/运行时 (Node/Runtime) -->
## 七、节点/运行时 (Node/Runtime)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **Graceful Node Shutdown** | Beta | **GA** | GA | GA | GA | GA | GA | GA | GA | 优雅关机 |
| **Parallel Image Pulls** | - | - | - | - | - | - | **默认启用** | 默认 | 默认 | 并行拉取 |
| **Swap Support** | Alpha | Alpha | Alpha | Beta | Beta | Beta | Beta | Beta | Beta | 内存交换 |
| **User Namespaces** | Alpha | Alpha | Beta | Beta | Beta | Beta | **GA** | GA | GA | 用户隔离 |
| **Node Volume Health** | - | - | - | - | **GA** | GA | GA | GA | GA | 存储健康监测 |
| **Kubelet OpenTelemetry** | - | - | - | - | - | - | **GA** | GA | GA | 链路追踪 |
| **Kubelet Resource Metrics** | - | - | - | - | - | - | - | - | **Beta** | 资源指标 |
| **In-Place Pod Resize** | - | - | - | - | Beta | Beta | Beta | Beta | **Alpha** | 原地调整 |
| **containerd 1.7+** | - | - | - | - | - | - | - | - | - | 推荐运行时 |

---

<!-- chunk: 八、控制平面 (Control Plane) -->
## 八、控制平面 (Control Plane)

| 特性 | v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **API Priority & Fairness (v1)** | - | **GA** | GA | GA | GA | GA | GA | GA | GA | 请求优先级 |
| **Server-side Apply** | GA | GA | GA | GA | GA | GA | GA | GA | GA | 声明式管理 |
| **ValidatingAdmissionPolicy** | - | Alpha | Beta | Beta | Beta | **GA** | GA | GA | GA | CEL 准入 |
| **API Server Tracing** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **Aggregated Discovery** | - | - | - | - | - | - | - | - | - | 已稳定 |
| **Storage Version API** | - | - | - | - | - | - | - | - | - | 已稳定 |

---

<!-- chunk: 九、API 废弃与移除 -->
## 九、API 废弃与移除

| API/功能 | 废弃版本 | 移除版本 | 替代方案 |
|:---|:---|:---|:---|
| PodSecurityPolicy | v1.21 | **v1.25** | Pod Security Admission |
| CronJob v1beta1 | v1.21 | **v1.25** | batch/v1 |
| EndpointSlice v1beta1 | v1.21 | **v1.25** | discovery.k8s.io/v1 |
| Event v1beta1 | v1.19 | **v1.25** | events.k8s.io/v1 |
| HPA v2beta1 | v1.19 | **v1.25** | autoscaling/v2 |
| PDB v1beta1 | v1.21 | **v1.25** | policy/v1 |
| RuntimeClass v1beta1 | v1.22 | **v1.25** | node.k8s.io/v1 |
| FlowSchema v1beta1 | v1.26 | **v1.26** | flowcontrol/v1 |
| PriorityLevelConfiguration v1beta1 | v1.26 | **v1.26** | flowcontrol/v1 |
| CSIStorageCapacity v1beta1 | v1.24 | **v1.27** | storage.k8s.io/v1 |
| FlowSchema v1beta2 | v1.26 | **v1.29** | flowcontrol/v1 |
| Node v1beta1 metrics | v1.29 | 预计 v1.34+ | metrics/v1 |
| in-tree storage drivers | v1.30 | 预计 v1.35+ | CSI 驱动 |
| kubelet --cloud-provider | v1.31 | 预计 v1.35+ | 外部 CCM |

---

<!-- chunk: 十、Feature Gate 状态总览 -->
## 十、Feature Gate 状态总览

| Feature Gate | v1.29 | v1.30 | v1.31 | v1.32 | v1.33 | 说明 |
|:---|:---:|:---:|:---:|:---:|:---:|:---|
| SidecarContainers | Beta | GA | GA | GA | **GA** | 原生 Sidecar |
| DynamicResourceAllocation | Beta | Beta | Beta | Beta | **GA** | DRA |
| InPlacePodVerticalScaling | Beta | Beta | Beta | Beta | **Alpha** | 原地调整 |
| NFTablesProxyMode | - | - | Alpha | Alpha | **Beta** | nftables |
| SchedulerQueueingHints | - | - | - | Alpha | **Beta** | 队列提示 |
| KubeletResourceMetrics | - | - | - | - | **Beta** | 资源指标 |
| CrossNamespaceVolumeDataSource | - | - | - | - | **Alpha** | 跨 NS 存储 |
| NodeLogQuery | - | Alpha | Alpha | Alpha | **Alpha** | 节点日志 |
| PodLevelResources | - | - | - | - | **Alpha** | Pod 级资源 |

---

<!-- chunk: 快速参考 -->
## 快速参考

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查当前版本
kubectl version

# 查看所有 Feature Gates
kubectl get --raw /api/v1/nodes/NODE/proxy/configz | jq '.kubeletconfig.featureGates'

# 检查已弃用 API
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis

# 查看支持的 API 版本
kubectl api-versions

# 检查 PSP (已移除 v1.25)
kubectl get psp 2>/dev/null || echo "PSP 已移除"

# 检查 CSI 驱动
kubectl get csidrivers
```
---

<!-- chunk: 参考链接 -->
## 升级决策矩阵

### 版本跨度风险评估

| 升级路径 | 风险 | 关键破坏性变更 | 建议 |
|----------|------|----------------|------|
| v1.25 → v1.26 | 低 | FlowSchema v1beta1 移除 | 直接升级 |
| v1.26 → v1.27 | 低 | CSIStorageCapacity v1beta1 移除 | 直接升级 |
| v1.27 → v1.28 | 低 | 无重大破坏 | 直接升级 |
| v1.28 → v1.29 | 低 | FlowSchema v1beta2 移除 | 检查 APF 配置 |
| v1.29 → v1.30 | 中 | in-tree 存储驱动弃用警告 | 确认 CSI 迁移 |
| v1.30 → v1.31 | 中 | kubelet --cloud-provider 弃用 | 确认外部 CCM |
| v1.31 → v1.32 | 低 | 无重大破坏 | 直接升级 |
| v1.32 → v1.33 | 低 | DRA GA、nftables Beta | 直接升级 |
| v1.25 → v1.33 (跨多版本) | **高** | PSP 移除 + 多个 API 废弃 | 逐版本升级 |

### 升级前必检项目

```bash
#!/bin/bash
# 🟢 只读：升级前自动化检查脚本
TARGET_VERSION="${1:-1.33}"
echo "=== Kubernetes 升级前检查 (目标: v$TARGET_VERSION) ==="

# 1. 当前版本
echo -n "[1/7] 当前版本: "
kubectl version -o json | jq -r '.serverVersion.gitVersion'

# 2. 已弃用 API 使用检查
echo "[2/7] 已弃用 API 使用情况:"
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis | \
  grep -v "^#" | awk '{print "  ⚠️  " $1, $2}'

# 3. PSP 检查 (v1.25 移除)
echo -n "[3/7] PodSecurityPolicy: "
kubectl get psp 2>/dev/null && echo "  ❗ 需迁移到 PSA" || echo "✅ 无 PSP"

# 4. Feature Gate 检查
echo "[4/7] 自定义 Feature Gates:"
for node in $(kubectl get nodes -o name | head -3); do
  echo "  $node:"
  kubectl get --raw "/api/v1/${node#node/}/proxy/configz" 2>/dev/null | \
    jq -r '.kubeletconfig.featureGates // {} | to_entries[] | select(.value != null) | "    \(.key)=\(.value)"'
done

# 5. 扩展组件兼容性
echo "[5/7] 扩展组件版本:"
echo -n "  CoreDNS: "
kubectl get deploy coredns -n kube-system -o jsonpath='{.spec.template.spec.containers[0].image}' 2>/dev/null || echo "N/A"
echo ""
echo -n "  CNI: "
kubectl get ds -n kube-system -l k8s-app=cilium -o jsonpath='{.items[0].spec.template.spec.containers[0].image}' 2>/dev/null || echo "N/A"
echo ""

# 6. 节点内核版本
echo "[6/7] 节点内核版本:"
kubectl get nodes -o custom-columns=NAME:.metadata.name,KERNEL:.status.nodeInfo.kernelVersion,OS:.status.nodeInfo.osImage

# 7. etcd 版本
echo -n "[7/7] etcd 版本: "
kubectl get pod -n kube-system -l component=etcd -o jsonpath='{.items[0].spec.containers[0].image}' 2>/dev/null || echo "N/A"

echo ""
echo "=== 检查完成 ==="
```

## 版本特定迁移指南

### v1.25 关键迁移：PSP → PSA

```yaml
# 旧: PodSecurityPolicy (v1.25 移除)
# apiVersion: policy/v1beta1
# kind: PodSecurityPolicy

# 新: Pod Security Admission (命名空间标签)
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted    # 强制执行
    pod-security.kubernetes.io/audit: restricted       # 审计记录
    pod-security.kubernetes.io/warn: restricted        # 警告提示
```

### v1.29 关键迁移：FlowSchema v1beta2 → v1

```bash
# 🟢 只读：检查是否使用了废弃的 APF API
kubectl get flowschemas -o yaml | grep apiVersion
kubectl get prioritylevelconfigurations -o yaml | grep apiVersion

# 🟡 中风险：导出并转换到 v1
kubectl get flowschema -o yaml > flowschemas-backup.yaml
kubectl get prioritylevelconfiguration -o yaml > plc-backup.yaml
```

### v1.30+ 关键迁移：in-tree 存储 → CSI

```bash
# 🟢 只读：检查 in-tree 存储驱动使用情况
kubectl get pv -o json | jq '[.items[] | select(.spec | has("awsElasticBlockStore") or has("gcePersistentDisk") or has("azureDisk"))] | length'

# 🟢 只读：检查 CSI 驱动状态
kubectl get csidrivers
kubectl get csinodes

# 确认 CSI Migration Feature Gate 已启用
kubectl get --raw /api/v1/nodes/NODE/proxy/configz | \
  jq '.kubeletconfig.featureGates | {CSIMigration, CSIMigrationAWS, CSIMigrationGCE}'
```

## Feature Gate 启用指南

### 安全启用流程

```
1. 查阅官方文档确认 Feature Gate 状态 (Alpha/Beta/GA)
2. 在非生产集群验证
3. 确认依赖组件兼容性
4. 滚动更新控制平面组件
5. 滚动更新 Kubelet
6. 监控异常指标 48h
```

### 常见 Feature Gate 配置

```yaml
# kube-apiserver 配置示例
apiVersion: kubeadm.k8s.io/v1beta4
kind: ClusterConfiguration
apiServer:
  extraArgs:
    feature-gates: "NFTablesProxyMode=true,NodeLogQuery=true"
---
# Kubelet 配置示例
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
featureGates:
  NFTablesProxyMode: true
  NodeLogQuery: true
```

### Feature Gate 风险分级

| 状态 | 风险 | 建议 |
|------|------|------|
| Alpha | 高 — 可能变更/移除 | 仅开发/测试环境 |
| Beta | 中 — 默认启用但可关闭 | 生产可用，关注变更 |
| GA | 低 — 永久启用 | 无需配置，不可关闭 |
| Deprecated | 警告 — 即将移除 | 尽快迁移 |

## 兼容性测试检查单

### 升级前测试矩阵

| 测试项 | 命令/方法 | 通过标准 |
|--------|----------|----------|
| API 兼容性 | `kubectl api-versions` | 无缺失 API |
| 废弃 API 扫描 | `kubectl get --raw /metrics \| grep deprecated` | 无废弃调用 |
| 扩展组件兼容 | 检查 Operator/Controller 版本 | 支持目标版本 |
| CNI 兼容 | 检查 CNI 版本 changelog | 支持目标版本 |
| CSI 兼容 | 检查 CSI 驱动版本 | 支持目标版本 |
| Webhook 兼容 | 检查 admissionReviewVersions | 包含 v1 |
| CRD 兼容 | `kubectl get crd -o yaml \| grep apiVersion` | 无废弃 API |
| 工作负载测试 | 在 staging 集群运行 E2E | 全部通过 |

### 升级后验证

```bash
# 🟢 只读：升级后快速验证
echo "=== 升级后验证 ==="

# 控制平面健康
kubectl get componentstatuses 2>/dev/null || kubectl get --raw /healthz

# 所有节点 Ready
kubectl get nodes | grep -v Ready

# 系统 Pod 状态
kubectl get pods -n kube-system | grep -v Running | grep -v Completed

# 核心功能验证
kubectl create namespace test-upgrade --dry-run=server -o yaml
kubectl run test --image=busybox --restart=Never --rm -it -- echo "OK"

# DNS 解析
kubectl run dns-test --image=busybox:1.36 --restart=Never --rm -it -- \
  nslookup kubernetes.default

# 清理
kubectl delete namespace test-upgrade --ignore-not-found
echo "=== 验证完成 ==="
```

## 参考链接

- [K8s Feature Gates](https://kubernetes.io/docs/reference/command-line-tools-reference/feature-gates/)
- [K8s 版本发布](https://kubernetes.io/releases/)
- [K8s API 变更](https://kubernetes.io/docs/reference/using-api/deprecation-guide/)

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

- 99-kubernetes-core-components-v1.29-v1.33-update
- 99-kubernetes-core-features-mermaid-diagrams
- 99-kubernetes-v1.29-v1.33-complete-feature-gates-reference
- 99-kubernetes-v1.29-v1.33-features-guide


<!-- risk-assessed -->
