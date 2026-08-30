---
title: Kubernetes v1.37 升级实操指南
description: 从 v1.36 升级到 v1.37 (Garhwal) 的完整实操指南，覆盖 ipvs 废弃、静态 Pod API 引用禁止、SELinux 挂载变更、metrics.k8s.io GA 等 1.37 关键变更
summary: K8s v1.37 升级实操指南 — 升级前检查清单（ipvs/静态 Pod/cgroup v1/kube-dns）、控制平面与节点滚动升级、新特性启用门禁表、回滚预案与常见问题排查
category: architecture-fundamentals
tags:
- k8s
- kubernetes
- upgrade
- kubeadm
- kubelet
- kube-proxy
- nftables
- selinux
- metrics
tier: core
created: '2026-08-28'
last_updated: '2026-08-28'
difficulty: advanced
audience:
- SRE
- 平台工程师
- 架构师
estimated_read_time: 15min
k8s_versions:
- '1.35'
- '1.36'
- '1.37'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险（只读/信息收集，无副作用）。

# Kubernetes v1.37 升级实操指南

> **适用版本**: 从 v1.36 升级到 v1.37（代号 Garhwal，2026-08-26 发布）
> **发布规模**: 67 项增强（16 Stable / 23 Beta / 27 Alpha）+ 多项废弃
> **难度**: 高级

---

## 📋 目录

- [一、升级前检查清单](#一升级前检查清单)
- [二、控制平面升级](#二控制平面升级)
- [三、工作节点升级](#三工作节点升级)
- [四、升级后验证](#四升级后验证)
- [五、启用 v1.37 新特性](#五启用-v1.37-新特性)
- [六、回滚预案](#六回滚预案)
- [七、常见问题排查](#七常见问题排查)

---

## 一、升级前检查清单

### 1.1 v1.37 必查项（本版本特有）

以下五项是 v1.37 独有的破坏性/行为变更，升级前**必须**逐项检查：

#### ① kube-proxy ipvs 模式已废弃（v1.40 默认禁用，v1.43 移除）

```bash
# 🟢 检查当前 proxy 模式
kubectl -n kube-system get configmap kube-proxy -o jsonpath='{.data.config\.conf}' | grep 'mode:'
```

- v1.37 起 ipvs 模式启动时打印废弃告警；官方建议迁移到 **nftables** 后端（v1.37 起未显式指定 mode 时会告警提示，iptables 仍可用）
- 迁移窗口：v1.40 前完成；KEP-5495

#### ② 静态 Pod 禁止引用 Secret/ConfigMap（硬性移除）

```bash
# 🟢 扫描节点上的静态 Pod 清单是否引用了 API 对象
grep -rn "configMapRef\|secretRef\|configMapKeyRef\|secretKeyRef" /etc/kubernetes/manifests/ 2>/dev/null
```

- 此前版本 Static Pod 可通过 bug 引用 `configMapRef`/`secretRef`，v1.37 起**严格禁止**，且逃生门 `PreventStaticPodAPIReferences` 特性门禁已移除
- 命中即需要改造静态 Pod（将配置内联到清单或改为挂载宿主机文件）

#### ③ cgroup v1 依赖检查（自 v1.35 起 failCgroupV1 默认 true）

```bash
# 🟢 确认节点 cgroup 版本
stat -fc %T /sys/fs/cgroup/
# 输出 cgroup2fs → v2（OK）；tmpfs → v1（kubelet 将启动失败）
```

- cgroup v1 节点需临时设置 `KubeletConfiguration` 中 `failCgroupV1: false`（短期方案），并尽快迁移至 cgroup v2（KEP-5573，未来版本移除）

#### ④ kube-dns 遗留检测

```bash
# 🟢 检查是否仍运行 kube-dns
kubectl get deployment kube-dns -n kube-system 2>/dev/null || echo "未部署 kube-dns"
```

- kube-dns 子项目已退役，v1.40 后不再出新包；EndpointSlices、双栈 Service 等特性不可用。仍运行 kube-dns 的集群应先迁移到 CoreDNS 再升级

#### ⑤ SELinux 卷挂载行为 GA（SELinuxMount / SELinuxChangePolicy）

- GA 后卷以 `-o context=<label>` 挂载（需 CSI 驱动 `.spec.seLinuxMount: true` 支持），替代递归重打标
- **风险**：同一节点上不同 SELinux 标签的 Pod 共享同一卷时，旧递归重标模式下可共存，新行为下会**启动失败**
- 临时回退：Pod 上设置 `.spec.seLinuxChangePolicy: Recursive`；集群级禁用在 v1.38 前仍可用
- 未启用 SELinux 的集群不受影响

### 1.2 版本兼容性确认

```bash
# 🟢 当前版本（升级源必须为 v1.34/v1.35/v1.36）
kubectl version -o json | jq '.serverVersion.gitVersion'

# 🟢 kubelet 版本偏差（API Server 与 kubelet 最多相差 2 个 minor）
kubectl get nodes -o custom-columns=NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion

# 🟢 已弃用 API 使用情况
kubectl get --raw /metrics 2>/dev/null | grep apiserver_requested_deprecated_apis || echo "无已弃用 API"

# 🟢 检查集群是否处于 v1.36 的 maxUnavailable 异常恢复期
kubectl get statefulsets -A -o json | jq -r '.items[] | select(.spec.updateStrategy.rollingUpdate.maxUnavailable != null) | "\(.metadata.namespace)/\(.metadata.name)"'
```

> 注：v1.36 中 `MaxUnavailableStatefulSet` 存在卡死 bug（kubernetes#137409），v1.37 重新默认启用并修复。若集群中存在依赖旧行为的 StatefulSet，升级前在测试环境验证其滚动更新。

### 1.3 备份与预案

```bash
# 🟡 备份 etcd 快照（在任一控制平面节点执行）
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-snapshot-$(date +%F).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 🟢 验证快照完整性
ETCDCTL_API=3 etcdctl snapshot status /backup/etcd-snapshot-*.db --table

# 🟡 备份 kube-proxy 配置与静态 Pod 清单
cp -r /etc/kubernetes/manifests /backup/manifests-$(date +%F)/
kubectl -n kube-system get configmap kube-proxy -o yaml > /backup/kube-proxy-cm-$(date +%F).yaml
```

---

## 二、控制平面升级

### 2.1 升级 kubeadm

```bash
# 🟡 在第一个控制平面节点执行
sudo apt-get update && sudo apt-get install -y kubeadm=1.37.0-1.1  # Debian/Ubuntu
# sudo yum install -y kubeadm-1.37.0                                # RHEL 系

# 🟢 预检（不执行变更，输出可升级项与告警）
sudo kubeadm upgrade plan
```

`kubeadm upgrade plan` 输出中重点确认：

- 无静态 Pod 引用 Secret/ConfigMap 的告警（对应 1.1 节 ②）
- 组件配置中的废弃项提示

### 2.2 执行控制平面升级

```bash
# 🔴 升级第一个控制平面节点
sudo kubeadm upgrade apply v1.37.0

# 🟡 其余控制平面节点
sudo kubeadm upgrade node
```

### 2.3 升级 kubelet 与 kubectl（控制平面节点）

```bash
# 🟡 升级节点上的 kubelet/kubectl 并重启
sudo apt-get install -y kubelet=1.37.0-1.1 kubectl=1.37.0-1.1
sudo systemctl daemon-reload && sudo systemctl restart kubelet
```

---

## 三、工作节点升级

```bash
# 1. 🟡 驱逐节点（每批 1-2 个，保持容量余量）
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data

# 2. 🟡 升级二进制
sudo apt-get install -y kubelet=1.37.0-1.1 kubectl=1.37.0-1.1 kubeadm=1.37.0-1.1

# 3. 🟡 节点级 kubeadm 升级
sudo kubeadm upgrade node

# 4. 🟡 重启 kubelet
sudo systemctl daemon-reload && sudo systemctl restart kubelet

# 5. 🟢 恢复调度
kubectl uncordon <node>
```

> cgroup v1 节点如需临时续命，在此步同步下发 `failCgroupV1: false` 的 KubeletConfiguration 覆盖，并列入 cgroup v2 迁移计划。

---

## 四、升级后验证

### 4.1 基础健康检查

```bash
# 🟢 版本与节点状态
kubectl get nodes -o wide
kubectl get cs | grep -v Healthy || echo "组件健康"

# 🟢 关键系统工作负载
kubectl get pods -n kube-system -o wide | grep -Ev "Running|Completed" || echo "系统 Pod 正常"

# 🟢 metrics API（v1.37 起 metrics.k8s.io 提供 v1 Stable 版本，v1beta1 按弃用策略继续可用）
kubectl top nodes
kubectl get apiservices | grep metrics.k8s.io
```

### 4.2 v1.37 行为变更专项验证

```bash
# 🟢 验证 kube-proxy 模式与 nftables 状态（推荐迁移目标）
kubectl -n kube-system get configmap kube-proxy -o jsonpath='{.data.config\.conf}' | grep 'mode:'

# 🟢 验证 ipvs 废弃告警是否出现（若仍为 ipvs）
kubectl -n kube-system logs -l k8s-app=kube-proxy | grep -i "deprecat" | head -3

# 🟢 验证静态 Pod 正常（引用 Secret/ConfigMap 的静态 Pod 此时应已修复）
kubectl get pods -A | grep -i static

# 🟢 验证 StatefulSet 滚动更新正常（maxUnavailable 重新默认启用）
kubectl rollout status statefulset/<name> -n <ns>
```

### 4.3 API 服务器 429 行为确认（大集群重点）

v1.37 中 watchcache 初始化加固完全锁定（`WatchCacheInitializationPostStartHook` GA 并锁定）：apiserver 启动/恢复期间不再放任 list/watch 请求冲击 etcd，超出配额的请求直接返回 **HTTP 429 + Retry-After**。

```bash
# 🟢 观察 apiserver 重启/恢复窗口期的限流情况
kubectl get --raw /metrics | grep -E "apiserver_flowcontrol_rejected_requests_total" | head -5
```

- 升级后巡检自研 Operator/控制器的日志，确认其正确处理 429（尊重 `Retry-After`、指数退避）
- 未实现退避的控制器在大集群恢复窗口会被批量限流，表现为短暂 reconcile 延迟

---

## 五、启用 v1.37 新特性

### 5.1 默认生效的 Stable/Beta 特性

| 特性 | 成熟度 | 说明 |
|------|--------|------|
| metrics.k8s.io v1 API | GA | Stable API，`v1beta1` 按弃用策略继续服务 |
| Pod Certificates / ClusterTrustBundles | GA | Pod 级 X.509 证书与信任锚分发 |
| SELinuxMount / SELinuxChangePolicy | GA | 见 1.1 节 ⑤，行为有破坏性变更 |
| Node Declared Features | GA | Node `.status.declaredFeatures` 声明节点能力，控制面按版本偏差自适应 |
| StorageVersionMigration (storagemigration.k8s.io/v1) | GA | 存储版本迁移内置化，升级/加密变更后重写存量数据 |
| Resilient Watchcache Initialization（含 WatchCacheInitializationPostStartHook 锁定） | GA | 见 4.3 节 |
| KYAML 输出 | GA | `kubectl -o kyaml`，规避 YAML "Norway bug"（如 `no`→false） |
| HPA scale to zero | Beta（默认启用） | object/external 指标可缩到 0 副本（CPU/内存不支持），`spec.minReplicas: 0`，状态条件 `ScaledToZero` |
| Memory QoS (cgroups v2) | Beta（默认启用） | 基于 memory request/limit 配置 cgroup 内存保护与限流 |
| PVC last used（PersistentVolumeClaimUnusedSinceTime） | Beta（默认启用） | PVC 增加 `Unused` 条件，支撑容量治理/FinOps |
| maxUnavailable (StatefulSet) | Beta（重新默认启用） | v1.36 bug 已修复 |
| Gang scheduling | Beta | AI/ML 训练作业成组调度 |
| nftables 代理方向（KEP-5343） | 过渡开始 | 未显式指定 mode 时回退 iptables 会产生告警；nftables 性能改用 netlink 接口 |

### 5.2 默认关闭的 Beta 特性

```yaml
# KubeletConfiguration / API Server --feature-gates 按需启用
PodLevelResourceManagers: true   # 🟡 Pod 级拓扑/CPU/内存资源管理器
```

### 5.3 Alpha 特性（默认关闭，需显式启用）

| 特性门禁 | KEP | 用途 |
|----------|-----|------|
| `PodLevelCheckpointRestore` | 5823 | Pod 级 checkpoint/restore（需容器运行时实现 CRI RPC） |
| `StatefulSetRecreateStrategy` | 3541 | StatefulSet Recreate 更新策略（有停机窗口；改 strategy 不触发滚动，需改 template 或 rollout restart） |
| `SchedulerPreemptionForPodResize` | 5836 | 就地扩容被 Deferred 时调度器抢占低优 Pod |
| `InPlacePodVerticalScalingMemoryBackedVolumes` | 6030 | emptyDir(memory) 的 sizeLimit 就地调整 |
| `H2CContainerProbe` | 5999 | HTTP/2 明文（h2c）探针 |
| `GRPCContainerProbeTLS` | 4939 | gRPC 探针 TLS 模式 |
| `VolumeBindMountOptions` | 5855 | volumeMount 绑定挂载选项（noexec/nosuid/nodev） |
| `EmptyDirVolumeMode` | 5502 | emptyDir 权限 mode 字段 |
| `CSIVolumeHealth` | 1432 | 标准化卷健康状态（Inaccessible/DataLoss/Degraded 等） |
| `KubeProxyNFTablesLocalhostNodePorts` | 6032 | nftables 后端 localhost NodePort（仅 TCP） |
| `DefaultPodSysctls` | 5996 | Kubelet 级 Pod 默认 sysctls |
| `CompositePodGroup` | 6012 | 层级化 PodGroup，多级 gang 调度（AI/ML prefill/decode） |
| `DRADerivedAttributes` / `DRADeviceCompatibilityGroups` / `DRAOptionalNodePreparation` | 6080/5963/5945 | DRA 调度能力增强 |
| `VolumeSnapshotTopology` | 5943 | 快照拓扑感知恢复 |
| `AtomicWriteVolumeUserFields` | 5936 | ConfigMap/Secret 投影文件属主控制 |

> Alpha 特性在生产集群启用前务必在隔离环境验证；门禁名以当版本 `--feature-gates` 实际支持为准。

---

## 六、回滚预案

```bash
# 1. 🔴 控制平面回滚：kubeadm 不支持原地降级，使用 etcd 快照恢复
sudo systemctl stop kubelet etcd  # 停止控制平面组件
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-snapshot-<date>.db \
  --data-dir=/var/lib/etcd-restore
# 恢复后按 v1.36 二进制重新引导控制平面

# 2. 🟡 单节点回滚：降级 kubelet 并恢复静态清单
sudo apt-get install -y kubelet=1.36.x-1.1 kubeadm=1.36.x-1.1
sudo systemctl restart kubelet

# 3. 🟡 SELinux 行为回退（未到 v1.38 前可集群级关闭）
#    或按工作负载设置 seLinuxChangePolicy: Recursive

# 4. 🟡 kube-proxy 回退到 iptables/ipvs（显式配置 mode 可继续使用 ipvs 至 v1.40）
kubectl -n kube-system edit configmap kube-proxy   # mode: ipvs / iptables
kubectl -n kube-system rollout restart ds kube-proxy
```

---

## 七、常见问题排查

### 7.1 升级后静态 Pod 起不来

```bash
# 🟢 查看事件
crictl ps -a | grep -i static; journalctl -u kubelet | grep -i "static pod" | tail -10
```

- 原因：静态 Pod 引用了 Secret/ConfigMap（v1.37 禁止，逃生门已移除）
- 处理：将配置内联进静态清单，或改用宿主机文件挂载

### 7.2 kube-proxy 日志出现 ipvs 废弃告警

- 属预期行为。确认迁移计划：先切换到 nftables（或 iptables 过渡），v1.40 前 ipvs 默认禁用、v1.43 移除
- 切换前在预发环境验证 Service/NodePort/负载均衡行为

### 7.3 大集群恢复窗口大量 429

- v1.37 watchcache 初始化加固的预期表现，非故障
- 检查客户端是否实现 `Retry-After` 退避；必要时错峰重启 apiserver

### 7.4 SELinux 标签不同的 Pod 抢同一卷失败

- GA 后 `-o context=` 挂载语义变更所致
- 短期：Pod 级 `seLinuxChangePolicy: Recursive`；长期：统一工作负载 SELinux 标签或等待 v1.38 前集群级关闭

### 7.5 cgroup v1 节点 kubelet 启动失败

```bash
# 🟢 确认 cgroup 版本
stat -fc %T /sys/fs/cgroup/
```

- 紧急：KubeletConfiguration 设 `failCgroupV1: false`（临时）
- 根治：升级宿主机/镜像到 cgroup v2（Memory QoS、内存卷就地扩容等特性仅 v2 可用）

---

## 参考链接

- [Kubernetes v1.37: Garhwal 发布公告](https://kubernetes.io/blog/2026/08/26/kubernetes-v1-37-release/)
- [Kubernetes v1.37 Sneak Peek（废弃预告）](https://kubernetes.io/blog/2026/07/31/kubernetes-v1-37-sneak-peek/)
- [KEP-5495 废弃 kube-proxy ipvs 模式](https://www.kubernetes.dev/resources/keps/5495/)
- [KEP-1710 SELinux 挂载变更](https://www.kubernetes.dev/resources/keps/1710/)
- [SELinux Volume Label Changes GA 影响分析](https://kubernetes.io/blog/2026/04/22/breaking-changes-in-selinux-volume-labeling/)
- [kubernetes/kubernetes CHANGELOG-1.37](https://github.com/kubernetes/kubernetes/blob/master/CHANGELOG/CHANGELOG-1.37.md)

## Related

- [[01-集群基础/06-升级路径/02-upgrade-paths-strategy|升级策略]]
- [[01-集群基础/06-升级路径/03-upgrade-migration-strategy|升级迁移策略]]
- [[01-集群基础/06-升级路径/04-kubernetes-v1.33-upgrade-guide|v1.33 升级指南]]
- [[01-集群基础/03-控制平面/39-cluster-upgrade-runbook|集群升级 Runbook]]
- [[01-集群基础/03-控制平面/38-certificate-pki-lifecycle-runbook|证书/PKI 生命周期 Runbook]]
