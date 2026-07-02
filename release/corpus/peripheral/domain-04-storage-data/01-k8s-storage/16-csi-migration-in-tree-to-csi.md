---
title: 16 - CSI 迁移：从 In-Tree 存储插件到 CSI
description: '# 16 - CSI 迁移：从 In-Tree 存储插件到 CSI'
summary: 'Kubernetes 自 v1.26 起正式移除了以下 In-Tree 存储插件：'
category: storage
tags:
- k8s
- storage
- pv
- pvc
- storageclass
- apiserver
- kubelet
- scheduler
- controller-manager
- helm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 存储工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- CSI 迁移：从 In-Tree 存储插件到 CSI 是什么
- 如何 CSI 迁移：从 In-Tree 存储插件到 CSI
- Kubernetes 6 storage 最佳实践
trigger_keywords:
- CSI
- 迁移：从
- In-Tree
- 存储插件到
- CSI
- storage
prerequisites:
- kubectl-basics
- storage-basics
- helm-basics
- redis-basics
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
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-04-storage-data/
  label: '相关知识域: domain-04-storage-data'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/csi-fta.md
  label: '故障树: csi'
---



# 16 - CSI 迁移：从 In-Tree 存储插件到 CSI

> **适用版本**: [[Kubernetes|Kubernetes]] v1.23 - v1.32 | **运维重点**: In-Tree 插件迁移、CSI 兼容性、升级风险控制
> **最后更新**: 2026-02

<!-- chunk: 目录 -->
## 目录

1. [迁移背景与必要性](#迁移背景与必要性)
2. [迁移机制原理](#迁移机制原理)
3. [各云厂商迁移状态](#各云厂商迁移状态)
4. [迁移前评估](#迁移前评估)
5. [迁移操作步骤](#迁移操作步骤)
6. [回滚方案](#回滚方案)
7. [迁移验证脚本](#迁移验证脚本)
8. [常见问题与排障](#常见问题与排障)

---

<!-- chunk: 迁移背景与必要性 -->
## 迁移背景与必要性

### 为什么要迁移？

Kubernetes 自 v1.26 起正式移除了以下 In-Tree 存储插件：

| 移除版本 | 插件 | CSI 替代方案 |
|---------|------|-------------|
| v1.26 | `kubernetes.io/aws-ebs` | `ebs.csi.aws.com` |
| v1.26 | `kubernetes.io/gce-pd` | `pd.csi.storage.gke.io` |
| v1.26 | `kubernetes.io/cinder` | `cinder.csi.openstack.org` |
| v1.26 | `kubernetes.io/azure-disk` | `disk.csi.azure.com` |
| v1.26 | `kubernetes.io/azure-file` | `file.csi.azure.com` |
| v1.26 | `kubernetes.io/vsphere-volume` | `csi.vsphere.vmware.com` |
| v1.29 | `kubernetes.io/portworx-volume` | `pxd.portworx.com` |
| v1.29 | `kubernetes.io/storageos` | `storageos` |

### 迁移时间线

```
v1.14 (2019) ─── CSI Migration Alpha ─── feature gate: CSIMigration
     │
v1.17 (2020) ─── CSI Migration Beta ─── 部分插件默认开启
     │
v1.23 (2021) ─── CSI Migration GA ─── AWS/GCE/Azure/OpenStack
     │
v1.26 (2022) ─── In-Tree 代码移除 ─── 7 个云厂商插件代码删除
     │
v1.29 (2024) ─── Portworx/StorageOS 移除
     │
v1.32 (2026) ─── 当前 ─── 所有主流存储均已 CSI 化
```

### 迁移的好处

| 维度 | In-Tree 插件 | CSI 驱动 |
|------|-------------|---------|
| **更新节奏** | 跟随 K8s 发版 | 独立更新 |
| **Bug 修复** | 需升级集群 | 热更新 CSI Pod |
| **功能扩展** | 修改 K8s 源码 | 插件化开发 |
| **社区维护** | 逐渐废弃 | 活跃维护 |
| **安全修复** | 绑定发版周期 | 快速响应 |

---

<!-- chunk: 迁移机制原理 -->
## 迁移机制原理

### CSI Migration 工作流程

```
                 迁移前 (In-Tree)
                 ═════════════════
    PVC ──→ StorageClass(provisioner: kubernetes.io/aws-ebs)
                     │
                     ▼
            In-Tree AWS EBS Plugin
                     │
                     ▼
                 AWS EBS Volume

                 迁移后 (CSI)
                 ═════════════════
    PVC ──→ StorageClass(provisioner: kubernetes.io/aws-ebs)  ← 无需修改!
                     │
                     ▼
            CSI Migration Translation Layer  ← kube-controller-manager 内置
                     │
                     ▼
            CSI Driver: ebs.csi.aws.com  ← 实际执行
                     │
                     ▼
                 AWS EBS Volume
```

### 关键特性门控

| Feature Gate | 默认状态 | 说明 |
|-------------|---------|------|
| `CSIMigration` | GA (v1.17+) | 总开关，将 In-Tree 操作重定向到 CSI |
| `CSIMigrationAWS` | GA (v1.23+) | AWS EBS 迁移 |
| `CSIMigrationGCE` | GA (v1.23+) | GCE Persistent Disk 迁移 |
| `CSIMigrationAzureDisk` | GA (v1.23+) | Azure Disk 迁移 |
| `CSIMigrationAzureFile` | GA (v1.23+) | Azure File 迁移 |
| `CSIMigrationOpenStack` | GA (v1.23+) | OpenStack Cinder 迁移 |
| `CSIMigrationvSphere` | GA (v1.26+) | vSphere 迁移 |
| `InTreePluginAWSUnregister` | GA (v1.26+) | 完全注销 AWS In-Tree 插件 |

### 迁移翻译机制

```yaml
# 原始 StorageClass（无需修改）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gp2
provisioner: kubernetes.io/aws-ebs
parameters:
  type: gp2
  zone: us-east-1a

# CSI Migration 翻译层自动将:
#   kubernetes.io/aws-ebs → ebs.csi.aws.com
#   type: gp2 → type: gp3 (如需要)
#   zone: us-east-1a → topology约束
```

---

<!-- chunk: 各云厂商迁移状态 -->
## 各云厂商迁移状态

### AWS EBS

```yaml
# 迁移前 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gp2-legacy
provisioner: kubernetes.io/aws-ebs
parameters:
  type: gp2

# 迁移后（自动翻译为 CSI 调用）
# 实际调用: ebs.csi.aws.com
# 也可显式切换 provisioner:
---
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gp3-csi
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iopsPerGB: "3000"
  throughput: "125"
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

### GCE Persistent Disk

```yaml
# 迁移前
provisioner: kubernetes.io/gce-pd
parameters:
  type: pd-ssd
  zone: us-central1-a

# CSI 驱动
provisioner: pd.csi.storage.gke.io
parameters:
  type: pd-ssd
```

### Azure Disk / Azure File

```yaml
# Azure Disk 迁移前
provisioner: kubernetes.io/azure-disk
parameters:
  storageaccounttype: Premium_LRS
  kind: Managed

# Azure Disk CSI
provisioner: disk.csi.azure.com
parameters:
  storageaccounttype: Premium_LRS

# Azure File 迁移前
provisioner: kubernetes.io/azure-file
parameters:
  skuName: Premium_LRS

# Azure File CSI
provisioner: file.csi.azure.com
parameters:
  skuName: Premium_LRS
```

### vSphere

```yaml
# 迁移前
provisioner: kubernetes.io/vsphere-volume
parameters:
  datastore: datastore1

# vSphere CSI
provisioner: csi.vsphere.vmware.com
parameters:
  datastore: datastore1
  storagepolicyname: "vSAN Default Storage Policy"
```

### OpenStack Cinder

```yaml
# 迁移前
provisioner: kubernetes.io/cinder
parameters:
  type: high-speed
  availability: nova

# Cinder CSI
provisioner: cinder.csi.openstack.org
parameters:
  type: high-speed
```

---

<!-- chunk: 迁移前评估 -->
## 迁移前评估

### 评估检查清单

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
#!/bin/bash
# csi-migration-assessment.sh - CSI 迁移前评估工具

echo "=========================================="
echo "CSI 迁移前评估报告"
echo "时间: $(date)"
echo "集群版本: $(kubectl version --short 2>/dev/null | grep Server | awk '{print $3}')"
echo "=========================================="

# 1. 检查 In-Tree StorageClass
echo ""
echo "## 1. In-Tree StorageClass 检测"
INTREE_SC=$(kubectl get sc -o json | jq -r '.items[] | select(.provisioner | startswith("kubernetes.io/")) | "\(.metadata.name) → \(.provisioner)"')
if [ -n "$INTREE_SC" ]; then
  echo "   ⚠️ 发现 In-Tree StorageClass:"
  echo "$INTREE_SC" | sed 's/^/   /'
  INTREE_SC_COUNT=$(echo "$INTREE_SC" | wc -l)
  echo "   数量: $INTREE_SC_COUNT"
else
  echo "   ✅ 未发现 In-Tree StorageClass"
fi

# 2. 检查使用 In-Tree SC 的 PVC
echo ""
echo "## 2. 使用 In-Tree SC 的 PVC"
if [ -n "$INTREE_SC" ]; then
  INTREE_SC_NAMES=$(echo "$INTREE_SC" | awk '{print $1}')
  for SC_NAME in $INTREE_SC_NAMES; do
    PVC_COUNT=$(kubectl get pvc --all-namespaces -o json | jq -r --arg sc "$SC_NAME" '[.items[] | select(.spec.storageClassName==$sc)] | length')
    if [ "$PVC_COUNT" -gt 0 ]; then
      echo "   StorageClass '$SC_NAME': $PVC_COUNT 个 PVC"
      kubectl get pvc --all-namespaces -o json | jq -r --arg sc "$SC_NAME" '.items[] | select(.spec.storageClassName==$sc) | "     - \(.metadata.namespace)/\(.metadata.name) (\(.status.phase))"' | head -10
    fi
  done
fi

# 3. 检查 CSI 驱动安装状态
echo ""
echo "## 3. CSI 驱动安装状态"
kubectl get csidriver -o wide 2>/dev/null || echo "   无 CSI 驱动注册"

# 4. 检查 Feature Gate 状态
echo ""
echo "## 4. Feature Gate 状态"
API_SERVER_POD=$(kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ -n "$API_SERVER_POD" ]; then
  FG=$(kubectl exec -n kube-system "$API_SERVER_POD" -- kube-apiserver --help 2>/dev/null | grep -o 'feature-gates=[^ ]*' || echo "无法获取")
  echo "   API Server: $FG"
fi

# 5. 节点 CSI 插件状态
echo ""
echo "## 5. CSI Node 插件"
kubectl get pods --all-namespaces -l app=csi-node -o wide --no-headers 2>/dev/null || \
  kubectl get pods --all-namespaces -o json | jq -r '.items[] | select(.metadata.name | test("csi.*node")) | "\(.metadata.namespace)/\(.metadata.name) \(.status.phase)"' | head -10

echo ""
echo "=========================================="
echo "评估完成"
echo "=========================================="
```

---

<!-- chunk: 迁移操作步骤 -->
## 迁移操作步骤

### 标准迁移流程（5 阶段）

```
阶段 1: 评估 ──→ 阶段 2: 安装 CSI 驱动 ──→ 阶段 3: 确认 Feature Gate
         ↓                                        ↓
阶段 5: 逐步迁移 ←────────────────────── 阶段 4: 创建新 StorageClass
```

#### 阶段 1: 迁移前评估

```bash
# 运行迁移前评估脚本（见"迁移前评估"章节）
bash csi-migration-assessment.sh
```

#### 阶段 2: 安装 CSI 驱动

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

```bash
# AWS EBS CSI 驱动安装示例
helm repo add aws-ebs-csi-driver https://kubernetes-sigs.github.io/aws-ebs-csi-driver
helm repo update
helm upgrade --install aws-ebs-csi-driver \
  aws-ebs-csi-driver/aws-ebs-csi-driver \
  --namespace kube-system \
  --set controller.serviceAccount.create=true \
  --set node.serviceAccount.create=true
```

#### 阶段 3: 确认 Feature Gate 已启用

```bash
# Kubernetes v1.23+ 默认已启用 CSIMigration
# 检查 kube-apiserver 和 kubelet 启动参数
ps aux | grep kube-apiserver | grep -o 'feature-gates=[^ ]*'
ps aux | grep kubelet | grep -o 'feature-gates=[^ ]*'

# 如果未启用，需要添加:
# --feature-gates=CSIMigration=true,CSIMigrationAWS=true
```

#### 阶段 4: 创建新的 CSI StorageClass

```yaml
# 新建 CSI StorageClass（保留原有 In-Tree SC 不变）
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gp3-csi
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iopsPerGB: "3000"
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
```

#### 阶段 5: 逐步迁移 PVC

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 方案 A: 新 PVC 使用新 SC（推荐渐进式）
# 仅修改应用 Deployment 中的 storageClassName

# 方案 B: 批量切换默认 SC
kubectl annotate storageclass gp2 storageclass.kubernetes.io/is-default-class=false
kubectl annotate storageclass gp3-csi storageclass.kubernetes.io/is-default-class=true

# 已绑定的 PVC 无需修改（CSI Migration 自动翻译）
# 仅新的 PVC 会使用新 StorageClass
```

---

<!-- chunk: 回滚方案 -->
## 回滚方案

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 如果迁移后出现问题，回滚步骤:

# 1. 将新 PVC 切回 In-Tree StorageClass
kubectl patch pvc <pvc-name> -p '{"spec":{"storageClassName":"gp2-legacy"}}'

# 2. 关闭 CSI Migration（仅限 v1.25 之前）
# 修改 kube-apiserver 和 kubelet 参数:
# --feature-gates=CSIMigration=false,CSIMigrationAWS=false

# 3. 重启控制平面
systemctl restart kube-apiserver kube-controller-manager kube-scheduler

# 4. 重启 kubelet
systemctl restart kubelet

# 注意: v1.26+ In-Tree 代码已移除，无法回滚到 In-Tree 插件
# 必须确保 CSI 驱动正常运行
```

---

<!-- chunk: 迁移验证脚本 -->
## 迁移验证脚本

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
#!/bin/bash
# csi-migration-verify.sh - 迁移后验证工具

echo "=========================================="
echo "CSI 迁移验证报告"
echo "时间: $(date)"
echo "=========================================="

# 1. 验证 CSI 驱动功能
echo ""
echo "## 1. 动态供给验证"
TEST_SC="csi-migration-test-$(date +%s)"
cat <<EOF | kubectl apply -f -
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: $TEST_SC
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
EOF

cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: csi-migration-test-pvc
spec:
  accessModes: [ReadWriteOnce]
  storageClassName: $TEST_SC
  resources:
    requests:
      storage: 1Gi
EOF

# 创建 Pod 触发绑定
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: csi-migration-test-pod
spec:
  containers:
  - name: test
    image: busybox
    command: ["sleep", "60"]
    volumeMounts:
    - name: data
      mountPath: /data
  volumes:
  - name: data
    persistentVolumeClaim:
      claimName: csi-migration-test-pvc
EOF

sleep 15
PVC_STATUS=$(kubectl get pvc csi-migration-test-pvc -o jsonpath='{.status.phase}' 2>/dev/null)
POD_STATUS=$(kubectl get pod csi-migration-test-pod -o jsonpath='{.status.phase}' 2>/dev/null)

if [ "$PVC_STATUS" = "Bound" ] && [ "$POD_STATUS" = "Running" ]; then
  echo "   ✅ CSI 动态供给验证通过"
else
  echo "   ❌ CSI 动态供给验证失败: PVC=$PVC_STATUS, Pod=$POD_STATUS"
fi

# 清理
kubectl delete pod csi-migration-test-pod --force 2>/dev/null  # ⚠️ 跳过优雅终止，可能丢数据
kubectl delete pvc csi-migration-test-pvc 2>/dev/null
kubectl delete sc "$TEST_SC" 2>/dev/null

# 2. 验证已有 PVC 仍可正常挂载
echo ""
echo "## 2. 已有 PVC 挂载验证"
BOUND_PVCS=$(kubectl get pvc --all-namespaces -o json | jq -r '.items[] | select(.status.phase=="Bound") | "\(.metadata.namespace)/\(.metadata.name)"' | head -5)
for PVC_REF in $BOUND_PVCS; do
  NS=$(echo "$PVC_REF" | cut -d'/' -f1)
  PVC=$(echo "$PVC_REF" | cut -d'/' -f2)
  PV=$(kubectl get pvc "$PVC" -n "$NS" -o jsonpath='{.spec.volumeName}')
  PV_DRIVER=$(kubectl get pv "$PV" -o jsonpath='{.spec.csi.driver}' 2>/dev/null)
  echo "   $PVC_REF → PV: $PV (driver: ${PV_DRIVER:-in-tree})"
done

# 3. 验证卷操作（扩容/快照）
echo ""
echo "## 3. CSI 卷操作能力验证"
kubectl get csidriver -o json | jq -r '.items[] | "   \(.metadata.name): 扩容=\(.spec.expandVolume) // \"未知\" 快照支持=\(.spec.podInfoOnMount)"' 2>/dev/null

echo ""
echo "=========================================="
echo "验证完成"
echo "=========================================="
```

---

<!-- chunk: 常见问题与排障 -->
## 常见问题与排障

| 问题 | 原因 | 解决方案 |
|------|------|---------|
| PVC Pending（迁移后） | CSI 驱动未安装 | 安装对应 CSI 驱动 |
| VolumeAttachment 失败 | CSI Controller 未运行 | 检查 CSI Controller Pod |
| 已有 PVC 无法挂载 | CSI Node 插件缺失 | 确保 [[DaemonSet|DaemonSet]] 部署到所有节点 |
| ProvisioningFailed | CSI 驱动无云商权限 | 配置 ServiceAccount/RBAC |
| 卷扩容失败 | CSI 驱动不支持 | 检查 CSIDriver spec |
| Multi-Attach | 已有卷仍被 In-Tree 管理 | 等待卷 Detach 后由 CSI 接管 |
| `failed to provision` | StorageClass 参数不兼容 | 检查 CSI 驱动支持的参数格式 |

### In-Tree 到 CSI 参数映射

```bash
# AWS EBS 参数映射
# In-Tree                    → CSI
# type: gp2                  → type: gp2 (不变)
# type: io1                  → type: io1
# iopsPerGB: "10"            → iopsPerGB: "10" (不变)
# encrypted: "true"          → encrypted: "true" (不变)
# kmsKeyId: "arn:..."        → kmsKeyId: "arn:..." (不变)

# Azure Disk 参数映射
# In-Tree                    → CSI
# storageaccounttype         → storageaccounttype (不变)
# kind: Managed              → (CSI 默认 Managed)
# kind: Shared               → (不再支持，需改用 Azure File)

# GCE PD 参数映射
# In-Tree                    → CSI
# type: pd-ssd               → type: pd-ssd (不变)
# replication-type: none     → replication-type: none (不变)
```

---

<!-- chunk: 相关文档 -->
## 相关文档

- [01 - 存储架构概览](./01-storage-architecture-overview.md) - 存储架构与核心组件
- [05 - CSI 驱动集成](./05-csi-drivers-integration.md) - CSI 驱动深度配置
- [09 - PV/PVC 故障排查](./09-pv-pvc-troubleshooting.md) - PVC 诊断与排障
- [../../domain-01-cluster-fundamentals/22-container-storage-deep-dive.md](../domain-01-cluster-fundamentals/22-container-storage-deep-dive.md) - CSI 架构原理

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)

## See Also

- 14-cloud-native-storage
- 15-storage-disaster-recovery
- completion-summary
- quality-check-report

## Related

- [[domain-19-landscape-references/topic-index/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/storage-index.md|Storage 存储知识图谱索引]]
- [[domain-19-landscape-references/topic-index/csi-index.md|CSI (Container Storage Interface) 知识图谱索引]]
