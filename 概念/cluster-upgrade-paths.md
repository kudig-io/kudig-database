---
title: 集群升级路径
summary: 集群升级路径：Kubernetes 集群升级的路径规划和最佳实践。
category: concepts
tags:
- core-concept
- k8s
- operations
- upgrade
- visibility/public
tier: core
sources:
- KUDIG Stub Generation 2026-05-24
created: 2026-05-24
updated: 2026-07-11
last_updated: 2026-07
status: stable
---



# 集群升级路径

## 概述

Kubernetes 集群升级是指把控制平面（kube-apiserver / controller-manager / scheduler / etcd）和工作节点（kubelet / kube-proxy / 容器运行时）从一个次版本升级到下一个次版本的过程。由于 Kubernetes 每年发布 3 个次版本、每个版本仅维护约 14 个月，**定期升级是生产集群的必修课**。升级遵循严格规则：**每次只能跳一个次版本**（v1.28→v1.29→v1.30），不能跨版本直跳（v1.28→v1.30 需分两步）。升级路径规划、API 弃用兼容性检查、节点滚动策略和回滚预案是升级成功的关键。

## 架构与工作原理

```
升级顺序（控制平面 + 节点）：

1. 先升主控制平面节点
   ┌─────────────────────────────────────────┐
   │ kubeadm upgrade plan / apply            │
   │  etcd → apiserver → controller-mgr →     │
   │  scheduler → kube-proxy → CNI 插件       │
   └─────────────────────────────────────────┘
2. 再升其余控制平面节点（逐个）
3. 最后升工作节点（cordon + drain + upgrade + uncordon）
   ┌─────────────────────────────────────────┐
   │ kubeadm upgrade node                     │
   │ kubelet / kube-proxy / containerd        │
   └─────────────────────────────────────────┘
4. 验证：所有节点 Ready，所有 Pod 正常，组件版本一致
```

**版本兼容矩阵**：kubelet 可比 apiserver 旧最多 3 个次版本（如 apiserver v1.30 支持 kubelet v1.27~v1.30），但**不能比 apiserver 新**。kube-proxy 应与 kubelet 同版本。

**升级工具链**：
- **kubeadm**：自建集群标准工具，`kubeadm upgrade apply` 升级控制平面。
- **托管集群**：云厂商（EKS/GKE/AKS）升级是控制平面一键操作，节点用 managed node group / cluster-autoscaler 配合。
- **GitOps（Cluster API / ArgoCD）**：声明式升级，整个集群作为代码管理。
- **原地升级 vs 蓝绿**：原地升级风险低但慢；蓝绿（建新集群迁移）最安全但成本高，关键生产常用。

## 关键组件与特性

| 关注点 | 说明 |
|--------|------|
| 版本跨度 | 每次只跨 1 个次版本（N → N+1） |
| API 弃用 | 升级前用 `kubectl convert` / pluto / ketall 检查弃用 API |
| 组件顺序 | etcd → apiserver → 其它控制平面 → kubelet → kube-proxy |
| kubelet 滞后 | 可比 apiserver 旧 ≤3 版本，但不能新 |
| etcd 版本 | 跟随 kubeadm 推荐，跨大版本需先迁移数据 |
| 回滚 | 难！etcd 快照恢复是最后手段，优先用蓝绿 |

## 配置示例

**kubeadm 升级流程（自建集群）**：

```bash
# 1. 升级前：备份 etcd 快照（必做！）
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%F).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/etcd/ca.crt --cert=/etc/etcd/peer.crt --key=/etc/etcd/peer.key

# 2. 升级 kubeadm
apt-mark unhold kubeadm
apt-get install kubeadm=1.30.x-00
apt-mark hold kubeadm

# 3. 检查升级计划
kubeadm upgrade plan

# 4. 应用控制平面升级（首个控制平面节点）
kubeadm upgrade apply v1.30.x

# 5. 升级其余控制平面节点
kubeadm upgrade node

# 6. 升级 kubelet / kube-proxy（每个节点）
apt-get install kubelet=1.30.x-00 kubectl=1.30.x-00
systemctl daemon-reload && systemctl restart kubelet

# 7. 升级工作节点（cordon + drain + upgrade + uncordon）
kubectl cordon node-1
kubectl drain node-1 --ignore-daemonsets --delete-emptydir-data
# SSH 到节点：apt install kubelet=1.30.x-00 && systemctl restart kubelet
kubectl uncordon node-1

# 8. 验证
kubectl get nodes        # 全部 Ready 且 VERSION 一致
kubectl get pods -A      # 无异常
```

**弃用 API 检查（升级前必做）**：

```bash
# 用 pluto 检查清单中已弃用的 API 版本
pluto detect-files --directory ./manifests/

# 常见弃用：
# - extensions/v1beta1 Ingress（v1.22 移除）
# - networking.k8s.io/v1beta1 Ingress（v1.22 移除）
# - batch/v1beta1 CronJob（v1.25 移除）→ batch/v1
# - policy/v1beta1 PodSecurityPolicy（v1.25 移除）
# - autoscaling/v2beta2 HPA（v1.26 移除）→ autoscaling/v2

# 升级后迁移：用 kubectl convert 或 kubebuilder 重生成
kubectl convert -f old-ingress.yaml --output-version networking.k8s.io/v1
```

## 常用操作与命令

```bash
# 升级前状态盘点
kubectl get nodes -o wide
kubectl version --short
kubectl cluster-info
kubectl get componentstatuses          # 组件健康

# API 版本检查
kubectl api-resources --api-group=networking.k8s.io
kubectl get ingress --all-namespaces -o jsonpath='{range .items[*]}{.apiVersion}{"\n"}{end}' | sort -u

# 升级进度观察
watch kubectl get nodes
kubectl get pods -n kube-system -w

# 升级后验证
kubeadm upgrade plan                   # 应显示 "your cluster is now up to date"
kubectl get nodes --show-labels | grep -i version

# 失败回滚（最后手段）：恢复 etcd 快照
systemctl stop kube-apiserver
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-xxx.db \
  --data-dir=/var/lib/etcd-restored
```

## 最佳实践

1. **每次只跳一个次版本**：v1.28→v1.30 必须分两步，否则 API 弃用与 etcd 迁移出问题。
2. **升级前必做弃用 API 扫描**：用 pluto/kube-no-trouble 扫描 GitOps 仓库与集群内资源。
3. **etcd 快照 + 验证恢复**：升级前快照，并在测试环境验证可恢复；etcd 是状态唯一真相。
4. **先非生产后生产**：在 dev/staging 验证完整升级链路与应用兼容性再上生产。
5. **节点逐个 drain**：避免同时驱逐多节点导致服务不可用，配 PDB 保护。
6. **维护窗口 + 通知**：升级有窗口期风险，选业务低峰，提前通知干系人。
7. **托管集群优先**：能力允许时用 EKS/GKE/AKS，控制平面升级云厂商负责，只关心节点。
8. **关键生产用蓝绿**：建新版本集群，逐步迁移流量，旧集群保留作回滚。
9. **自动化**：用 Cluster API 或托管集群节点组自动升级，减少人工失误。

## 常见陷阱

- **跨版本直跳**：v1.27→v1.29 直接 apply 失败或留下兼容性炸弹，必须分步。
- **API 弃用导致资源不可用**：升级后 Ingress/CronJob/PSP 资源消失，业务中断；升级前扫描清单。
- **kubelet 比 apiserver 新**：节点 kubelet 升级早于 apiserver，kubelet 拒绝连接。
- **CNI/CRI 不兼容**：升级 kubelet 到新版本但 containerd/Calico 太旧，节点 NotReady。
- **升级中 etcd 损坏**：跨大 etcd 版本未迁移数据，集群状态丢失；快照是救命稻草。
- **回滚几乎不可能**：apiserver 升级会写新格式资源到 etcd，回滚到旧 apiserver 可能不识别。
- **节点 drain 卡住**：PDB 阻止驱逐或裸 Pod 无 controller，加 `--disable-eviction` 临时绕过。
- **证书过期**：升级同时忘记 renew 证书，升级后不久 apiserver 证书过期集群宕机。

## 源码实现分析

### kubeadm 升级流程

```go
// k8s.io/kubernetes/cmd/kubeadm/app/cmd/upgrade/apply.go
// kubeadm upgrade apply 执行控制平面升级
func (a *apply) Run() error {
    // 1. 预检查：版本兼容性、API 弃用扫描
    if err := preflight.Checks(a.cfg); err != nil {
        return err
    }
    // 2. 升级 etcd（如果是自管 etcd）
    if a.cfg.Etcd.IsExternal() == false {
        upgradeEtcd(a.cfg.Etcd.Local.ImageTag)
    }
    // 3. 升级 kube-apiserver 静态 Pod 镜像
    upgradeControlPlane(a.cfg, "kube-apiserver")
    upgradeControlPlane(a.cfg, "kube-controller-manager")
    upgradeControlPlane(a.cfg, "kube-scheduler")
    // 4. 升级 kubelet 配置
    upgradeKubeletConfig(a.cfg)
    // 5. 验证集群健康
    waitForClusterHealthy()
}
```

### 升级流程架构

```
┌───────────────────────────────────────────────────────────┐
│          K8s 集群升级流程                            │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  升级前准备:                                             │
│  ─────────                                              │
│  1. 扫描弃用 API (pluto/kube-no-trouble)              │
│  2. 备份 etcd (etcdctl snapshot save)                  │
│  3. 测试环境验证完整升级链路                         │
│  4. 确认 CNI/CRI 版本兼容性                          │
│                                                           │
│  控制平面升级 (严格顺序):                              │
│  ─────────                                              │
│  etcd → kube-apiserver → controller-manager           │
│       → kube-scheduler → kubelet (逐节点)            │
│                                                           │
│  节点升级 (逐个):                                      │
│  ─────────                                              │
│  kubectl cordon <node>                                  │
│  kubectl drain <node> --ignore-daemonsets             │
│  升级 kubelet + containerd                             │
│  systemctl restart kubelet                             │
│  kubectl uncordon <node>                               │
│                                                           │
│  升级后验证:                                             │
│  ─────────                                              │
│  kubectl get nodes (all Ready)                         │
│  kubectl get pods -A (no CrashLoop)                    │
│  kubectl get componentstatuses                         │
│  证书有效期检查 (kubeadm certs check-expiration)     │
└───────────────────────────────────────────────────────────┘
```

### 升级前检查脚本（🟢 只读）

```bash
#!/bin/bash
# 升级前检查脚本
echo "=== 当前版本 ==="
kubectl version --short 2>/dev/null || kubectl version

echo "=== 弃用 API 扫描 ==="
# pluto detect-all-in-cluster
kubectl get --raw /metrics | grep apiserver_requested_deprecated_apis

echo "=== etcd 健康 ==="
kubectl exec -n kube-system etcd-master -- etcdctl endpoint health --cluster

echo "=== 证书有效期 ==="
kubeadm certs check-expiration 2>/dev/null || \
  openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -enddate

echo "=== 节点状态 ==="
kubectl get nodes -o wide
```

## 面试要点

1. **K8s 集群升级的严格顺序？**
   - etcd → kube-apiserver → controller-manager → scheduler
   - 然后逐节点升级 kubelet
   - 原因：下层依赖必须先就绪

2. **为什么不能跨版本升级？**
   - API 弃用：旧 API 可能在新版本移除
   - etcd 数据格式：新版本可能写入不兼容格式
   - kubelet 兼容性：最多落后 apiserver 2 个小版本

3. **升级前必须做哪些检查？**
   - 弃用 API 扫描（pluto/kube-no-trouble）
   - etcd 备份 + 验证恢复
   - CNI/CRI 版本兼容性确认
   - 证书有效期检查

4. **升级失败如何回滚？**
   - 控制平面：从 etcd 快照恢复 + 回滚静态 Pod 镜像
   - 节点：回滚 kubelet/containerd 版本
   - 关键：升级前必须备份 etcd

## 参见

- [[kubernetes]] — core-concept 领域核心页面
- [[概念/container-runtime.md|Container Runtime]] — 同步升级
- [[概念/k8s-mttr-benchmark.md|K8s MTTR 基准]]
- [[README]]

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
