# Kubernetes v1.33 速查卡

> **一页纸速查**: v1.29 → v1.33 所有关键变更  
> **最后更新**: 2026-04-24

---

## 🚀 v1.33 核心变更 (最新)

| 特性 | 状态 | 一句话说明 | 是否启用 |
|:---|:---|:---|:---|
| **Sidecar 容器** | **GA** | init 容器支持 `restartPolicy: Always`，自动重启 | ✅ 默认启用 |
| **DRA** | **GA** | GPU/FPGA 动态资源分配，替代 Device Plugin | ⚠️ 需显式启用 FG |
| **TopologyManager Per Pod** | **GA** | Pod 级 NUMA 拓扑策略 | ⚠️ 需显式启用 FG |
| **Scheduler Queueing Hints** | **Beta** | 调度器队列提示，性能提升 10-30% | ✅ 默认启用 |
| **Kubelet Resource Metrics** | **Beta** | `/metrics/resource` 端点，替代 Summary API | ✅ 默认启用 |
| **In-Place Pod Resize** | **Alpha** | 原地调整 Pod 资源，无需重启 | ❌ 需启用 FG |
| **Cross-Namespace PVC** | **Alpha** | PVC 跨命名空间引用数据源 | ❌ 需启用 FG |
| **PodIndexLabel** | **GA** | StatefulSet 自动生成 `apps.kubernetes.io/pod-index` | ✅ 默认启用 |
| **Windows HostProcess** | **GA** | Windows 容器 HostProcess 模式稳定 | ✅ 默认启用 |

---

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

## 🔧 kubectl 快捷命令

```bash
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

## 📚 相关文档

| 文档 | 内容 |
|:---|:---|
| [99-kubernetes-v1.29-v1.33-features-guide.md](./99-kubernetes-v1.29-v1.33-features-guide.md) | 按版本详解 |
| [99-kubernetes-core-components-v1.29-v1.33-update.md](./99-kubernetes-core-components-v1.29-v1.33-update.md) | 按组件速查 |
| [99-kubernetes-v1.33-upgrade-guide.md](./99-kubernetes-v1.33-upgrade-guide.md) | 升级实操 |
| [99-kubectl-v1.29-v1.33-new-commands-guide.md](./99-kubectl-v1.29-v1.33-new-commands-guide.md) | kubectl 新命令 |
| [99-kubernetes-v1.33-production-best-practices.md](./99-kubernetes-v1.33-production-best-practices.md) | 生产最佳实践 |
| [99-kubernetes-version-lifecycle-support-policy.md](./99-kubernetes-version-lifecycle-support-policy.md) | 版本生命周期 |
| [99-kubernetes-v1.33-ecosystem-compatibility-matrix.md](./99-kubernetes-v1.33-ecosystem-compatibility-matrix.md) | 兼容性矩阵 |
