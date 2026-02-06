# 09 - PV/PVC故障排查与解决方案

> **适用版本**: Kubernetes v1.25 - v1.32 | **运维重点**: 故障诊断、问题解决、预防措施 | **最后更新**: 2026-02

## 目录

1. [概述](#概述)
2. [存储架构](#存储架构)
3. [常见故障场景](#常见故障场景)
4. [诊断工具与方法](#诊断工具与方法)
5. [解决方案大全](#解决方案大全)
6. [预防措施](#预防措施)
7. [企业级案例分析](#企业级案例分析)
8. [最佳实践总结](#最佳实践总结)

---

### PV/PVC 生命周期

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              PV/PVC 生命周期状态机                                   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                           PersistentVolume 生命周期                          │   │
│   │                                                                              │   │
│   │   ┌───────────┐      ┌───────────┐      ┌───────────┐      ┌───────────┐   │   │
│   │   │ Available │ ───► │  Bound    │ ───► │ Released  │ ───► │  Failed   │   │   │
│   │   │           │      │           │      │           │      │           │   │   │
│   │   │ 等待绑定   │      │ 已绑定PVC │      │ PVC已删除 │      │ 回收失败  │   │   │
│   │   └─────┬─────┘      └─────┬─────┘      └─────┬─────┘      └───────────┘   │   │
│   │         │                  │                  │                            │   │
│   │         │                  │                  │                            │   │
│   │         │                  │            ┌─────┴─────┐                      │   │
│   │         │                  │            │  Reclaim  │                      │   │
│   │         │                  │            │  Policy   │                      │   │
│   │         │                  │            └─────┬─────┘                      │   │
│   │         │                  │                  │                            │   │
│   │         │                  │     ┌───────────┼───────────┐                │   │
│   │         │                  │     │           │           │                │   │
│   │         │                  │     ▼           ▼           ▼                │   │
│   │         │                  │  Retain     Recycle     Delete              │   │
│   │         │                  │  (保留)      (回收)      (删除)              │   │
│   │         │                  │     │           │           │                │   │
│   │         │                  │     │           │           │                │   │
│   │         │                  │     ▼           ▼           ▼                │   │
│   │         │                  │  手动清理    重新可用    卷被删除            │   │
│   │         │                  │  数据后      (已废弃)                        │   │
│   │         │                  │  Available                                   │   │
│   │         │                  │                                              │   │
│   └─────────┴──────────────────┴──────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                        PersistentVolumeClaim 生命周期                        │   │
│   │                                                                              │   │
│   │   ┌───────────┐      ┌───────────┐      ┌───────────┐                      │   │
│   │   │  Pending  │ ───► │  Bound    │ ───► │  Lost     │                      │   │
│   │   │           │      │           │      │           │                      │   │
│   │   │ 等待绑定   │      │ 已绑定PV  │      │ PV丢失    │                      │   │
│   │   └─────┬─────┘      └───────────┘      └───────────┘                      │   │
│   │         │                                                                   │   │
│   │         │ 动态供应/静态匹配                                                 │   │
│   │         │                                                                   │   │
│   │         ▼                                                                   │   │
│   │   ┌───────────────────────────────────────────────────────────────────┐    │   │
│   │   │                      绑定条件检查                                  │    │   │
│   │   │                                                                   │    │   │
│   │   │  • 存储大小: PV >= PVC                                            │    │   │
│   │   │  • 访问模式: PV ⊇ PVC                                             │    │   │
│   │   │  • StorageClass: PV == PVC (或为空)                               │    │   │
│   │   │  • Selector: PV labels 匹配 PVC selector                         │    │   │
│   │   │  • VolumeMode: PV == PVC (Filesystem/Block)                      │    │   │
│   │   │                                                                   │    │   │
│   │   └───────────────────────────────────────────────────────────────────┘    │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### CSI 架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              CSI (Container Storage Interface) 架构                  │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                              Control Plane                                   │   │
│   │                                                                              │   │
│   │   ┌────────────────────┐      ┌────────────────────┐                        │   │
│   │   │   kube-controller  │      │  external-provisioner │                     │   │
│   │   │     -manager       │      │                       │                     │   │
│   │   │                    │      │  • 监听 PVC 创建       │                     │   │
│   │   │  • PV Controller   │      │  • 调用 CSI CreateVolume │                  │   │
│   │   │  • AttachDetach    │      │  • 创建 PV 对象        │                     │   │
│   │   │    Controller      │      └───────────┬───────────┘                     │   │
│   │   └────────────────────┘                  │                                 │   │
│   │                                           │                                 │   │
│   │   ┌────────────────────┐      ┌───────────┴───────────┐                     │   │
│   │   │  external-attacher │      │  external-snapshotter │                     │   │
│   │   │                    │      │                       │                     │   │
│   │   │  • 监听 VolumeAttachment │ │  • 监听 VolumeSnapshot │                   │   │
│   │   │  • 调用 CSI ControllerPublish │ • 调用 CSI CreateSnapshot │              │   │
│   │   └────────────────────┘      └───────────────────────┘                     │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                           │                                          │
│                                           │ gRPC                                     │
│                                           │                                          │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                               CSI Driver                                     │   │
│   │                                                                              │   │
│   │   ┌────────────────────────────────────────────────────────────────────┐    │   │
│   │   │                         Controller Plugin                           │    │   │
│   │   │                                                                     │    │   │
│   │   │   CreateVolume    DeleteVolume    ControllerPublishVolume          │    │   │
│   │   │   CreateSnapshot  DeleteSnapshot  ControllerUnpublishVolume        │    │   │
│   │   │   ControllerExpandVolume         ListVolumes                       │    │   │
│   │   │                                                                     │    │   │
│   │   └────────────────────────────────────────────────────────────────────┘    │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                           │                                          │
│                                           │ gRPC (Node Socket)                       │
│                                           │                                          │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                              Node (每个节点)                                 │   │
│   │                                                                              │   │
│   │   ┌────────────────────┐      ┌────────────────────────────────────────┐    │   │
│   │   │      kubelet       │      │              Node Plugin               │    │   │
│   │   │                    │      │                                        │    │   │
│   │   │  Volume Manager    │ ───► │  NodeStageVolume    (挂载到staging)    │    │   │
│   │   │                    │      │  NodePublishVolume  (挂载到Pod)        │    │   │
│   │   │  • Mount/Unmount   │      │  NodeUnstageVolume  (卸载staging)     │    │   │
│   │   │  • Attach/Detach   │      │  NodeUnpublishVolume(卸载Pod)         │    │   │
│   │   │                    │      │  NodeGetVolumeStats (获取统计)        │    │   │
│   │   └────────────────────┘      └────────────────────────────────────────┘    │   │
│   │                                           │                                 │   │
│   │                                           │                                 │   │
│   │   ┌────────────────────────────────────────────────────────────────────┐    │   │
│   │   │                         Storage Backend                            │    │   │
│   │   │                                                                     │    │   │
│   │   │   ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐          │    │   │
│   │   │   │AWS EBS   │  │Azure Disk│  │GCE PD    │  │Ceph RBD  │          │    │   │
│   │   │   └──────────┘  └──────────┘  └──────────┘  └──────────┘          │    │   │
│   │   │                                                                     │    │   │
│   │   │   ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐          │    │   │
│   │   │   │阿里云盘   │  │NFS       │  │iSCSI     │  │Local PV  │          │    │   │
│   │   │   └──────────┘  └──────────┘  └──────────┘  └──────────┘          │    │   │
│   │   │                                                                     │    │   │
│   │   └────────────────────────────────────────────────────────────────────┘    │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

## 状态详解

### PVC 状态说明

| 状态 | 描述 | 常见原因 | 解决方向 |
|-----|------|---------|---------|
| **Pending** | 等待绑定 | 无匹配 PV、StorageClass 问题、配额不足 | 检查匹配条件、SC 配置、配额 |
| **Bound** | 已绑定到 PV | 正常状态 | - |
| **Lost** | 绑定的 PV 不存在 | PV 被误删、后端存储故障 | 恢复 PV 或重新创建 |

### PV 状态说明

| 状态 | 描述 | 常见原因 | 解决方向 |
|-----|------|---------|---------|
| **Available** | 可用,等待绑定 | 新创建或回收后 | 正常状态 |
| **Bound** | 已绑定到 PVC | 正常状态 | - |
| **Released** | PVC 已删除,等待回收 | PVC 删除后 | 根据回收策略处理 |
| **Failed** | 回收操作失败 | 后端存储问题 | 检查存储后端 |

### 访问模式对比

| 访问模式 | 缩写 | 描述 | 支持的存储类型 |
|---------|-----|------|--------------|
| **ReadWriteOnce** | RWO | 单节点读写 | 块存储 (EBS, Azure Disk, GCE PD) |
| **ReadOnlyMany** | ROX | 多节点只读 | NFS, CephFS, Azure File |
| **ReadWriteMany** | RWX | 多节点读写 | NFS, CephFS, Azure File, EFS |
| **ReadWriteOncePod** | RWOP | 单 Pod 读写 (v1.22+) | 部分 CSI 驱动 |

## 故障诊断流程

### PVC Pending 诊断

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              PVC Pending 诊断决策树                                  │
│                                                                                      │
│                              PVC 状态为 Pending                                      │
│                                     │                                                │
│                                     ▼                                                │
│                         ┌─────────────────────┐                                     │
│                         │ 检查 StorageClass   │                                     │
│                         │ 是否存在且正确?      │                                     │
│                         └──────────┬──────────┘                                     │
│                                    │                                                │
│                    ┌───────────────┴───────────────┐                                │
│                    │                               │                                │
│                   否                              是                                │
│                    │                               │                                │
│                    ▼                               ▼                                │
│         ┌──────────────────┐          ┌─────────────────────┐                      │
│         │ 创建/修正        │          │ 检查 provisioner    │                      │
│         │ StorageClass     │          │ 是否正常运行?       │                      │
│         └──────────────────┘          └──────────┬──────────┘                      │
│                                                  │                                  │
│                                  ┌───────────────┴───────────────┐                  │
│                                  │                               │                  │
│                                 否                              是                  │
│                                  │                               │                  │
│                                  ▼                               ▼                  │
│                       ┌──────────────────┐          ┌─────────────────────┐        │
│                       │ 检查 CSI Driver  │          │ 检查存储后端        │        │
│                       │ Pod 状态和日志    │          │ 容量和配额          │        │
│                       └──────────────────┘          └──────────┬──────────┘        │
│                                                                │                    │
│                                                ┌───────────────┴───────────────┐    │
│                                                │                               │    │
│                                              不足                            足够    │
│                                                │                               │    │
│                                                ▼                               ▼    │
│                                     ┌──────────────────┐        ┌────────────────┐ │
│                                     │ 扩容存储或清理   │        │ 检查 PVC 参数  │ │
│                                     │ 资源释放配额     │        │ 与 PV 匹配条件 │ │
│                                     └──────────────────┘        └────────┬───────┘ │
│                                                                          │         │
│                                                          ┌───────────────┴─────┐   │
│                                                          │                     │   │
│                                                        不匹配               匹配   │
│                                                          │                     │   │
│                                                          ▼                     ▼   │
│                                               ┌──────────────────┐  ┌────────────┐ │
│                                               │ 调整 PVC 参数    │  │ 检查节点   │ │
│                                               │ 或创建匹配的 PV  │  │ 拓扑约束   │ │
│                                               └──────────────────┘  └────────────┘ │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### 综合诊断脚本

```bash
#!/bin/bash
# pv-pvc-diagnostics.sh
# PV/PVC 综合诊断脚本

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

print_header() {
    echo -e "\n${GREEN}=== $1 ===${NC}"
}

print_warning() {
    echo -e "${YELLOW}[警告] $1${NC}"
}

print_error() {
    echo -e "${RED}[错误] $1${NC}"
}

echo "=========================================="
echo "     PV/PVC 综合诊断报告"
echo "=========================================="
echo "时间: $(date)"
echo ""

# 检查参数
PVC_NAME=${1:-""}
NAMESPACE=${2:-"default"}

if [ -n "$PVC_NAME" ]; then
    echo "诊断目标: PVC $PVC_NAME (namespace: $NAMESPACE)"
fi

print_header "1. PV/PVC 概览"

echo "--- PVC 统计 ---"
echo "Pending PVC:"
kubectl get pvc --all-namespaces --field-selector=status.phase=Pending 2>/dev/null | wc -l
echo ""
echo "Bound PVC:"
kubectl get pvc --all-namespaces --field-selector=status.phase=Bound 2>/dev/null | wc -l
echo ""

echo "--- PV 统计 ---"
echo "Available PV:"
kubectl get pv --field-selector=status.phase=Available 2>/dev/null | wc -l
echo ""
echo "Bound PV:"
kubectl get pv --field-selector=status.phase=Bound 2>/dev/null | wc -l
echo ""
echo "Released PV:"
kubectl get pv --field-selector=status.phase=Released 2>/dev/null | wc -l
echo ""

print_header "2. Pending PVC 详情"

PENDING_PVCS=$(kubectl get pvc --all-namespaces --field-selector=status.phase=Pending -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}' 2>/dev/null)

if [ -z "$PENDING_PVCS" ]; then
    echo "没有 Pending 状态的 PVC"
else
    echo "发现以下 Pending PVC:"
    for pvc in $PENDING_PVCS; do
        NS=$(echo $pvc | cut -d'/' -f1)
        NAME=$(echo $pvc | cut -d'/' -f2)
        echo ""
        echo "--- $NS/$NAME ---"
        
        # 获取 PVC 详情
        kubectl get pvc $NAME -n $NS -o yaml 2>/dev/null | grep -E "storageClassName|accessModes|storage:" | head -10
        
        # 获取事件
        echo "最近事件:"
        kubectl get events -n $NS --field-selector involvedObject.name=$NAME --sort-by='.lastTimestamp' 2>/dev/null | tail -5
    done
fi

print_header "3. StorageClass 状态"

echo "--- StorageClass 列表 ---"
kubectl get sc -o custom-columns=\
NAME:.metadata.name,\
PROVISIONER:.provisioner,\
RECLAIM:.reclaimPolicy,\
BINDING:.volumeBindingMode,\
EXPANSION:.allowVolumeExpansion,\
DEFAULT:.metadata.annotations."storageclass\.kubernetes\.io/is-default-class"

echo ""

# 检查默认 StorageClass
DEFAULT_SC=$(kubectl get sc -o jsonpath='{.items[?(@.metadata.annotations.storageclass\.kubernetes\.io/is-default-class=="true")].metadata.name}' 2>/dev/null)
if [ -z "$DEFAULT_SC" ]; then
    print_warning "没有设置默认 StorageClass"
else
    echo "默认 StorageClass: $DEFAULT_SC"
fi

print_header "4. CSI Driver 状态"

echo "--- CSI Driver 列表 ---"
kubectl get csidriver 2>/dev/null || echo "未找到 CSI Driver"
echo ""

echo "--- CSI 相关 Pod ---"
kubectl get pods --all-namespaces -l app.kubernetes.io/component=csi-driver 2>/dev/null || \
kubectl get pods --all-namespaces | grep -E "csi|ebs|disk|storage" || echo "未找到 CSI Pod"
echo ""

# 检查 CSI Pod 状态
CSI_PODS=$(kubectl get pods --all-namespaces -o wide 2>/dev/null | grep -i csi | grep -v Running || true)
if [ -n "$CSI_PODS" ]; then
    print_warning "发现异常 CSI Pod:"
    echo "$CSI_PODS"
fi

print_header "5. VolumeAttachment 状态"

echo "--- VolumeAttachment 列表 ---"
kubectl get volumeattachment 2>/dev/null | head -20 || echo "未找到 VolumeAttachment"
echo ""

# 检查 Pending VolumeAttachment
PENDING_VA=$(kubectl get volumeattachment -o jsonpath='{range .items[?(@.status.attached==false)]}{.metadata.name}{"\n"}{end}' 2>/dev/null)
if [ -n "$PENDING_VA" ]; then
    print_warning "发现未 Attached 的 VolumeAttachment:"
    echo "$PENDING_VA"
fi

print_header "6. 节点存储状态"

echo "--- 节点存储容量 ---"
kubectl get nodes -o custom-columns=\
NAME:.metadata.name,\
EPHEMERAL:.status.allocatable.ephemeral-storage 2>/dev/null

echo ""

echo "--- CSINode 状态 ---"
kubectl get csinode 2>/dev/null | head -10 || echo "未找到 CSINode"

print_header "7. ResourceQuota 检查"

echo "--- 存储相关 ResourceQuota ---"
kubectl get resourcequota --all-namespaces -o custom-columns=\
NAMESPACE:.metadata.namespace,\
NAME:.metadata.name,\
PVC_USED:.status.used.persistentvolumeclaims,\
PVC_HARD:.status.hard.persistentvolumeclaims,\
STORAGE_USED:.status.used.requests\\.storage,\
STORAGE_HARD:.status.hard.requests\\.storage 2>/dev/null || echo "未配置存储配额"

print_header "8. 常见问题检查"

# 检查 Released 但未清理的 PV
RELEASED_PV=$(kubectl get pv --field-selector=status.phase=Released -o name 2>/dev/null | wc -l)
if [ "$RELEASED_PV" -gt 0 ]; then
    print_warning "发现 $RELEASED_PV 个 Released 状态的 PV,需要手动处理"
    kubectl get pv --field-selector=status.phase=Released 2>/dev/null
fi

# 检查 Failed 的 PV
FAILED_PV=$(kubectl get pv --field-selector=status.phase=Failed -o name 2>/dev/null | wc -l)
if [ "$FAILED_PV" -gt 0 ]; then
    print_error "发现 $FAILED_PV 个 Failed 状态的 PV"
    kubectl get pv --field-selector=status.phase=Failed 2>/dev/null
fi

# 检查 Lost 的 PVC
LOST_PVC=$(kubectl get pvc --all-namespaces --field-selector=status.phase=Lost -o name 2>/dev/null | wc -l)
if [ "$LOST_PVC" -gt 0 ]; then
    print_error "发现 $LOST_PVC 个 Lost 状态的 PVC"
    kubectl get pvc --all-namespaces --field-selector=status.phase=Lost 2>/dev/null
fi

# 特定 PVC 诊断
if [ -n "$PVC_NAME" ]; then
    print_header "9. 特定 PVC 诊断: $PVC_NAME"
    
    echo "--- PVC 详情 ---"
    kubectl get pvc $PVC_NAME -n $NAMESPACE -o yaml 2>/dev/null || echo "PVC 不存在"
    
    echo ""
    echo "--- PVC 事件 ---"
    kubectl describe pvc $PVC_NAME -n $NAMESPACE 2>/dev/null | grep -A 20 "Events:" || true
    
    # 获取关联的 PV
    PV_NAME=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.spec.volumeName}' 2>/dev/null)
    if [ -n "$PV_NAME" ]; then
        echo ""
        echo "--- 关联 PV: $PV_NAME ---"
        kubectl get pv $PV_NAME -o yaml 2>/dev/null
    fi
    
    # 检查使用该 PVC 的 Pod
    echo ""
    echo "--- 使用该 PVC 的 Pod ---"
    kubectl get pods -n $NAMESPACE -o json 2>/dev/null | jq -r ".items[] | select(.spec.volumes[]?.persistentVolumeClaim.claimName == \"$PVC_NAME\") | .metadata.name" 2>/dev/null || echo "未找到"
fi

print_header "10. 诊断建议"

echo "根据以上检查结果,请注意以下几点:"
echo ""
echo "1. Pending PVC 常见原因:"
echo "   - StorageClass 不存在或配置错误"
echo "   - CSI Driver 未正常运行"
echo "   - 存储后端容量或配额不足"
echo "   - 节点拓扑约束不满足"
echo ""
echo "2. 挂载失败常见原因:"
echo "   - VolumeAttachment 失败"
echo "   - 节点无法连接存储后端"
echo "   - 多节点同时挂载 RWO 卷"
echo ""
echo "3. 建议执行的后续检查:"
echo "   - kubectl logs <csi-controller-pod> -c csi-provisioner"
echo "   - kubectl logs <csi-node-pod> -c csi-node"
echo "   - 检查云厂商存储服务状态"

echo ""
echo "=========================================="
echo "       诊断报告结束"
echo "=========================================="
```

## 常见问题与解决方案

### PVC Pending 原因矩阵

| 原因类别 | 具体原因 | 诊断命令 | 解决方案 |
|---------|---------|---------|---------|
| **StorageClass** | SC 不存在 | `kubectl get sc` | 创建或修正 SC 名称 |
| | SC provisioner 错误 | `kubectl describe sc <name>` | 修正 provisioner 配置 |
| | 无默认 SC | `kubectl get sc` | 设置默认 SC |
| **CSI Driver** | Driver 未安装 | `kubectl get csidriver` | 安装 CSI Driver |
| | Controller Pod 异常 | `kubectl get pods -n kube-system` | 检查 Pod 日志 |
| | Node Plugin 异常 | `kubectl get pods -n kube-system -o wide` | 检查节点上的 CSI Pod |
| **容量/配额** | 存储后端容量不足 | 检查云厂商控制台 | 扩容或清理存储 |
| | ResourceQuota 限制 | `kubectl get resourcequota` | 调整配额或释放资源 |
| | LimitRange 限制 | `kubectl get limitrange` | 调整限制或 PVC 大小 |
| **匹配条件** | 容量不匹配 | `kubectl get pv,pvc` | 调整 PV/PVC 大小 |
| | 访问模式不匹配 | `kubectl get pv,pvc -o wide` | 修正访问模式 |
| | Label Selector 不匹配 | `kubectl describe pvc` | 调整 selector/labels |
| **拓扑约束** | 节点不在允许区域 | `kubectl describe pv` | 检查 nodeAffinity |
| | WaitForFirstConsumer 模式 | `kubectl describe sc` | Pod 调度后自动解决 |

### Mount 失败诊断

```bash
#!/bin/bash
# mount-failure-diagnostics.sh
# Pod 挂载失败诊断脚本

POD_NAME=$1
NAMESPACE=${2:-"default"}

if [ -z "$POD_NAME" ]; then
    echo "用法: $0 <pod-name> [namespace]"
    exit 1
fi

echo "=== Pod 挂载诊断: $POD_NAME ==="
echo ""

# 获取 Pod 信息
echo "--- Pod 状态 ---"
kubectl get pod $POD_NAME -n $NAMESPACE -o wide
echo ""

# 获取 Pod 事件
echo "--- Pod 事件 ---"
kubectl describe pod $POD_NAME -n $NAMESPACE | grep -A 30 "Events:" | head -35
echo ""

# 获取 Pod 使用的 PVC
echo "--- Pod 使用的 PVC ---"
PVC_NAMES=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.volumes[*].persistentVolumeClaim.claimName}')
for pvc in $PVC_NAMES; do
    echo ""
    echo "PVC: $pvc"
    kubectl get pvc $pvc -n $NAMESPACE
    
    # 获取关联的 PV
    PV_NAME=$(kubectl get pvc $pvc -n $NAMESPACE -o jsonpath='{.spec.volumeName}')
    if [ -n "$PV_NAME" ]; then
        echo "PV: $PV_NAME"
        kubectl get pv $PV_NAME
        
        # 检查 VolumeAttachment
        echo ""
        echo "VolumeAttachment:"
        kubectl get volumeattachment | grep $PV_NAME || echo "未找到关联的 VolumeAttachment"
    fi
done

# 获取节点信息
NODE_NAME=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.nodeName}')
if [ -n "$NODE_NAME" ]; then
    echo ""
    echo "--- 节点信息: $NODE_NAME ---"
    kubectl get node $NODE_NAME
    
    echo ""
    echo "--- 节点 CSI 状态 ---"
    kubectl get csinode $NODE_NAME -o yaml 2>/dev/null || echo "CSINode 信息不可用"
    
    echo ""
    echo "--- 节点上的 CSI Pod ---"
    kubectl get pods --all-namespaces -o wide --field-selector spec.nodeName=$NODE_NAME | grep -i csi
fi

echo ""
echo "--- Kubelet 日志 (需要节点访问权限) ---"
echo "请在节点上运行: journalctl -u kubelet | grep -i 'volume\\|mount\\|attach' | tail -50"
```

### 存储扩容故障排查

```bash
#!/bin/bash
# volume-expansion-diagnostics.sh
# 卷扩容诊断脚本

PVC_NAME=$1
NAMESPACE=${2:-"default"}

if [ -z "$PVC_NAME" ]; then
    echo "用法: $0 <pvc-name> [namespace]"
    exit 1
fi

echo "=== 卷扩容诊断: $PVC_NAME ==="
echo ""

# 检查 PVC 状态
echo "--- PVC 状态 ---"
kubectl get pvc $PVC_NAME -n $NAMESPACE -o yaml
echo ""

# 检查 PVC conditions
echo "--- PVC Conditions ---"
kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.status.conditions}' | jq . 2>/dev/null || \
kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.status.conditions}'
echo ""

# 获取 StorageClass 信息
SC_NAME=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.spec.storageClassName}')
echo "--- StorageClass: $SC_NAME ---"
kubectl get sc $SC_NAME -o yaml
echo ""

# 检查是否支持扩容
ALLOW_EXPANSION=$(kubectl get sc $SC_NAME -o jsonpath='{.allowVolumeExpansion}')
if [ "$ALLOW_EXPANSION" != "true" ]; then
    echo "[错误] StorageClass 不支持卷扩容 (allowVolumeExpansion: $ALLOW_EXPANSION)"
    echo "解决方案: 修改 StorageClass 设置 allowVolumeExpansion: true"
    exit 1
fi

# 获取 PV 信息
PV_NAME=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.spec.volumeName}')
if [ -n "$PV_NAME" ]; then
    echo "--- PV 状态: $PV_NAME ---"
    kubectl get pv $PV_NAME -o yaml
fi

# 检查使用 PVC 的 Pod
echo ""
echo "--- 使用该 PVC 的 Pod ---"
PODS=$(kubectl get pods -n $NAMESPACE -o json | jq -r ".items[] | select(.spec.volumes[]?.persistentVolumeClaim.claimName == \"$PVC_NAME\") | .metadata.name")
if [ -n "$PODS" ]; then
    echo "Pod 列表: $PODS"
    echo ""
    echo "[提示] 某些 CSI 驱动要求 Pod 重启后才能完成文件系统扩容"
    echo "可以尝试: kubectl rollout restart deployment/<deployment-name> -n $NAMESPACE"
else
    echo "无 Pod 使用此 PVC"
fi

# 检查 CSI Driver 日志
echo ""
echo "--- CSI Controller 最近日志 ---"
CSI_CONTROLLER=$(kubectl get pods --all-namespaces -l app.kubernetes.io/component=csi-driver -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)
if [ -n "$CSI_CONTROLLER" ]; then
    CSI_NS=$(kubectl get pods --all-namespaces -l app.kubernetes.io/component=csi-driver -o jsonpath='{.items[0].metadata.namespace}' 2>/dev/null)
    kubectl logs $CSI_CONTROLLER -n $CSI_NS -c csi-resizer --tail=20 2>/dev/null || echo "无法获取 CSI resizer 日志"
fi
```

## StorageClass 配置

### 常见 StorageClass 示例

```yaml
# storageclass-examples.yaml

---
# AWS EBS gp3 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ebs-gp3
  annotations:
    storageclass.kubernetes.io/is-default-class: "true"
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  encrypted: "true"
  fsType: ext4
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true
mountOptions:
  - noatime
  - nodiratime

---
# Azure Disk Premium StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: azure-premium-ssd
provisioner: disk.csi.azure.com
parameters:
  skuName: Premium_LRS
  cachingMode: ReadOnly
  fsType: ext4
  networkAccessPolicy: AllowPrivate
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true

---
# GCP PD SSD StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: gcp-pd-ssd
provisioner: pd.csi.storage.gke.io
parameters:
  type: pd-ssd
  replication-type: regional-pd
  fsType: ext4
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true

---
# 阿里云 ESSD PL1 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: alicloud-essd-pl1
provisioner: diskplugin.csi.alibabacloud.com
parameters:
  type: cloud_essd
  performanceLevel: PL1
  fsType: ext4
  encrypted: "true"
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer
allowVolumeExpansion: true

---
# NFS StorageClass (使用 NFS CSI Driver)
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-csi
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs-server.example.com
  share: /exported/path
  mountPermissions: "0755"
reclaimPolicy: Delete
volumeBindingMode: Immediate
mountOptions:
  - nfsvers=4.1
  - hard
  - noresvport

---
# Local PV StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: local-storage
provisioner: kubernetes.io/no-provisioner
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer

---
# Ceph RBD StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: ceph-rbd
provisioner: rbd.csi.ceph.com
parameters:
  clusterID: <cluster-id>
  pool: kubernetes
  imageFeatures: layering
  csi.storage.k8s.io/provisioner-secret-name: csi-rbd-secret
  csi.storage.k8s.io/provisioner-secret-namespace: ceph-csi
  csi.storage.k8s.io/controller-expand-secret-name: csi-rbd-secret
  csi.storage.k8s.io/controller-expand-secret-namespace: ceph-csi
  csi.storage.k8s.io/node-stage-secret-name: csi-rbd-secret
  csi.storage.k8s.io/node-stage-secret-namespace: ceph-csi
  csi.storage.k8s.io/fstype: ext4
reclaimPolicy: Delete
volumeBindingMode: Immediate
allowVolumeExpansion: true
```

### 拓扑感知配置

```yaml
# topology-aware-storageclass.yaml

---
# 带拓扑约束的 StorageClass
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: topology-aware-ebs
provisioner: ebs.csi.aws.com
parameters:
  type: gp3
  fsType: ext4
reclaimPolicy: Delete
volumeBindingMode: WaitForFirstConsumer  # 延迟绑定,等待 Pod 调度
allowVolumeExpansion: true
allowedTopologies:
  - matchLabelExpressions:
      - key: topology.ebs.csi.aws.com/zone
        values:
          - us-east-1a
          - us-east-1b
          - us-east-1c

---
# PV 示例 (带 nodeAffinity)
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv-example
spec:
  capacity:
    storage: 100Gi
  volumeMode: Filesystem
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-storage
  local:
    path: /mnt/disks/ssd1
  nodeAffinity:
    required:
      nodeSelectorTerms:
        - matchExpressions:
            - key: kubernetes.io/hostname
              operator: In
              values:
                - node-1
                - node-2
```

## 快照与恢复

### VolumeSnapshot 配置

```yaml
# volume-snapshot-example.yaml

---
# VolumeSnapshotClass
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshotClass
metadata:
  name: ebs-snapshot-class
driver: ebs.csi.aws.com
deletionPolicy: Delete
parameters:
  # 可选参数
  # tagSpecification_1: "key=environment,value=production"

---
# 创建快照
apiVersion: snapshot.storage.k8s.io/v1
kind: VolumeSnapshot
metadata:
  name: mysql-data-snapshot
  namespace: database
spec:
  volumeSnapshotClassName: ebs-snapshot-class
  source:
    persistentVolumeClaimName: mysql-data-pvc

---
# 从快照恢复 PVC
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data-restored
  namespace: database
spec:
  storageClassName: ebs-gp3
  dataSource:
    name: mysql-data-snapshot
    kind: VolumeSnapshot
    apiGroup: snapshot.storage.k8s.io
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi  # 必须 >= 原始大小

---
# 从现有 PVC 克隆
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: mysql-data-clone
  namespace: database
spec:
  storageClassName: ebs-gp3
  dataSource:
    name: mysql-data-pvc
    kind: PersistentVolumeClaim
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 100Gi
```

### 快照故障排查

```bash
#!/bin/bash
# snapshot-diagnostics.sh
# 快照诊断脚本

SNAPSHOT_NAME=$1
NAMESPACE=${2:-"default"}

if [ -z "$SNAPSHOT_NAME" ]; then
    echo "用法: $0 <snapshot-name> [namespace]"
    exit 1
fi

echo "=== 快照诊断: $SNAPSHOT_NAME ==="
echo ""

# 获取快照状态
echo "--- VolumeSnapshot 状态 ---"
kubectl get volumesnapshot $SNAPSHOT_NAME -n $NAMESPACE -o yaml
echo ""

# 获取 VolumeSnapshotContent
CONTENT_NAME=$(kubectl get volumesnapshot $SNAPSHOT_NAME -n $NAMESPACE -o jsonpath='{.status.boundVolumeSnapshotContentName}')
if [ -n "$CONTENT_NAME" ]; then
    echo "--- VolumeSnapshotContent: $CONTENT_NAME ---"
    kubectl get volumesnapshotcontent $CONTENT_NAME -o yaml
fi

# 检查快照类
SC_NAME=$(kubectl get volumesnapshot $SNAPSHOT_NAME -n $NAMESPACE -o jsonpath='{.spec.volumeSnapshotClassName}')
if [ -n "$SC_NAME" ]; then
    echo ""
    echo "--- VolumeSnapshotClass: $SC_NAME ---"
    kubectl get volumesnapshotclass $SC_NAME -o yaml
fi

# 检查事件
echo ""
echo "--- 相关事件 ---"
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$SNAPSHOT_NAME --sort-by='.lastTimestamp'
```

## 监控告警

### Prometheus 监控规则

```yaml
# pv-pvc-monitoring-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: pv-pvc-monitoring-rules
  namespace: monitoring
spec:
  groups:
    # =================================================================
    # PVC 状态告警
    # =================================================================
    - name: pvc.alerts
      interval: 30s
      rules:
        - alert: PVCPending
          expr: |
            kube_persistentvolumeclaim_status_phase{phase="Pending"} == 1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "PVC 处于 Pending 状态"
            description: "PVC {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 已 Pending 超过 5 分钟"
            
        - alert: PVCPendingLong
          expr: |
            kube_persistentvolumeclaim_status_phase{phase="Pending"} == 1
          for: 30m
          labels:
            severity: critical
          annotations:
            summary: "PVC 长时间 Pending"
            description: "PVC {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 已 Pending 超过 30 分钟"
            
        - alert: PVCLost
          expr: |
            kube_persistentvolumeclaim_status_phase{phase="Lost"} == 1
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "PVC 丢失"
            description: "PVC {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 状态为 Lost,可能数据丢失"
            
    # =================================================================
    # PV 状态告警
    # =================================================================
    - name: pv.alerts
      interval: 30s
      rules:
        - alert: PVFailed
          expr: |
            kube_persistentvolume_status_phase{phase="Failed"} == 1
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "PV 状态为 Failed"
            description: "PV {{ $labels.persistentvolume }} 状态为 Failed"
            
        - alert: PVReleased
          expr: |
            kube_persistentvolume_status_phase{phase="Released"} == 1
          for: 1h
          labels:
            severity: warning
          annotations:
            summary: "PV 长时间处于 Released 状态"
            description: "PV {{ $labels.persistentvolume }} 已 Released 超过 1 小时,需要手动处理"
            
    # =================================================================
    # 存储容量告警
    # =================================================================
    - name: storage.capacity.alerts
      interval: 1m
      rules:
        - alert: PVCStorageCapacityLow
          expr: |
            (
              kubelet_volume_stats_available_bytes / 
              kubelet_volume_stats_capacity_bytes
            ) < 0.15
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "PVC 存储容量低"
            description: "PVC {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 剩余容量低于 15%"
            
        - alert: PVCStorageCapacityCritical
          expr: |
            (
              kubelet_volume_stats_available_bytes / 
              kubelet_volume_stats_capacity_bytes
            ) < 0.05
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "PVC 存储容量严重不足"
            description: "PVC {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} 剩余容量低于 5%"
            
        - alert: PVCInodeUsageHigh
          expr: |
            (
              kubelet_volume_stats_inodes_used / 
              kubelet_volume_stats_inodes
            ) > 0.9
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "PVC Inode 使用率高"
            description: "PVC {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} Inode 使用率超过 90%"
            
    # =================================================================
    # CSI Driver 告警
    # =================================================================
    - name: csi.alerts
      interval: 30s
      rules:
        - alert: CSIDriverNotReady
          expr: |
            sum(kube_pod_status_ready{pod=~".*csi.*", condition="true"}) by (pod, namespace) == 0
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "CSI Driver Pod 未就绪"
            description: "CSI Driver Pod {{ $labels.namespace }}/{{ $labels.pod }} 未就绪"
            
        - alert: CSIProvisionerErrors
          expr: |
            rate(csi_operations_seconds_count{driver_name=~".+", operation_name="CreateVolume", status="error"}[5m]) > 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "CSI Provisioner 创建卷失败"
            description: "CSI Driver {{ $labels.driver_name }} 创建卷操作出现错误"
            
    # =================================================================
    # VolumeAttachment 告警
    # =================================================================
    - name: volumeattachment.alerts
      interval: 30s
      rules:
        - alert: VolumeAttachmentFailed
          expr: |
            time() - kube_volumeattachment_created > 300 
            and kube_volumeattachment_status_attached == 0
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "卷挂载失败"
            description: "VolumeAttachment {{ $labels.volumeattachment }} 创建超过 5 分钟仍未完成"
            
    # =================================================================
    # 记录规则
    # =================================================================
    - name: storage.recording
      interval: 30s
      rules:
        - record: pvc:storage:usage_ratio
          expr: |
            kubelet_volume_stats_used_bytes / 
            kubelet_volume_stats_capacity_bytes
            
        - record: pvc:storage:available_bytes
          expr: kubelet_volume_stats_available_bytes
          
        - record: pvc:inode:usage_ratio
          expr: |
            kubelet_volume_stats_inodes_used / 
            kubelet_volume_stats_inodes
            
        - record: cluster:pvc:pending_count
          expr: |
            count(kube_persistentvolumeclaim_status_phase{phase="Pending"})
            
        - record: cluster:pv:released_count
          expr: |
            count(kube_persistentvolume_status_phase{phase="Released"})
```

### 常用监控命令

```bash
#!/bin/bash
# storage-monitoring-commands.sh
# 存储监控常用命令集合

echo "=== PVC 容量使用情况 ==="
kubectl get pvc --all-namespaces -o custom-columns=\
NAMESPACE:.metadata.namespace,\
NAME:.metadata.name,\
STATUS:.status.phase,\
VOLUME:.spec.volumeName,\
CAPACITY:.status.capacity.storage,\
STORAGECLASS:.spec.storageClassName

echo ""
echo "=== 按 StorageClass 统计 PVC ==="
kubectl get pvc --all-namespaces -o json | jq -r '
  .items | group_by(.spec.storageClassName) | 
  map({
    storageClass: .[0].spec.storageClassName,
    count: length,
    totalSize: (map(.spec.resources.requests.storage // "0") | join(", "))
  }) | .[] | "\(.storageClass): \(.count) PVCs"'

echo ""
echo "=== 卷使用率 Top 10 (需要 metrics) ==="
kubectl top pvc --all-namespaces 2>/dev/null | head -11 || echo "需要安装 metrics-server"

echo ""
echo "=== 检查即将满的 PVC ==="
# 通过 kubelet metrics 检查
kubectl get --raw "/api/v1/nodes/$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')/proxy/stats/summary" 2>/dev/null | \
jq -r '.pods[].volume[]? | select(.usedBytes != null and .capacityBytes != null) | 
  select((.usedBytes / .capacityBytes) > 0.8) | 
  "PVC: \(.pvcRef.name // .name) Usage: \((.usedBytes / .capacityBytes * 100 | floor))%"' 2>/dev/null || \
echo "无法获取卷使用率数据"
```

## 版本变更记录

| 版本 | 变更内容 | 影响 |
|-----|---------|------|
| **v1.31** | VolumeAttributesClass GA | 支持动态修改卷属性 |
| | RecoverVolumeExpansionFailure GA | 自动恢复扩容失败 |
| **v1.30** | ReadWriteOncePod GA | 单 Pod 独占访问 |
| | CSI VolumeHealth GA | 卷健康监控 |
| **v1.29** | VolumeGroupSnapshot Alpha | 支持多卷一致性快照 |
| | CrossNamespaceVolumeDataSource Beta | 跨命名空间数据源 |
| **v1.28** | CSI SELinux Mount GA | SELinux 挂载选项支持 |
| | VolumeResourceQuota GA | 卷资源配额支持 |

## 最佳实践总结

### 故障预防清单

- [ ] 为生产环境配置 StorageClass 默认值
- [ ] 启用 `allowVolumeExpansion` 以支持在线扩容
- [ ] 使用 `WaitForFirstConsumer` 避免跨 AZ 调度问题
- [ ] 配置合适的 `reclaimPolicy` (生产环境建议 Retain)
- [ ] 部署存储容量监控和告警
- [ ] 定期检查 Released/Failed 状态的 PV
- [ ] 为重要数据配置定期快照
- [ ] 记录各 CSI Driver 的限制和最佳实践

---
## 企业级案例分析

### 案例一：大规模电商平台存储故障

```yaml
# 故障背景
incident_details:
  company: "某大型电商平台"
  environment: "生产环境"
  scale: "1000+节点集群，50000+ PVC"
  incident_time: "2026-01-15 14:30"
  impact: "核心交易系统响应时间增加300%，订单处理失败率上升至15%"

# 故障现象
symptoms:
  - pvc_pending_rate: "突然增加到25%"
  - storage_provisioning_latency: "> 60秒"
  - csi_controller_unhealthy: "3/5副本异常"
  - volume_attachment_failures: "大量挂载失败"

# 根因分析过程
root_cause_analysis:
  timeline:
    - "14:30": "监控告警：PVC Pending数量激增"
    - "14:32": "初步排查：CSI Controller Pod异常重启"
    - "14:35": "深入分析：发现Leader选举冲突"
    - "14:40": "根本原因：版本升级引入的竞争条件bug"
    
  diagnostic_commands:
    - "kubectl get pods -n kube-system -l app=csi-controller -o wide"
    - "kubectl logs -n kube-system -l app=csi-controller --previous"
    - "kubectl get csidriver"
    - "kubectl describe storageclass"

# 解决方案
resolution:
  immediate_actions:
    - rollback_csi_version: "回滚到稳定版本v1.18.0"
    - restart_controller_pods: "强制重启所有Controller Pod"
    - clear_volume_attachments: "清理异常的VolumeAttachment对象"
    
  long_term_fixes:
    - implement_health_checks: "增强健康检查机制"
    - add_leader_election_timeout: "优化Leader选举超时配置"
    - upgrade_testing_pipeline: "完善升级前测试流程"

# 经验教训
lessons_learned:
  prevention_measures:
    - canary_deployment_strategy: "采用金丝雀部署策略"
    - enhanced_monitoring: "增加CSI组件详细监控"
    - automated_rollback: "建立自动回滚机制"
    - chaos_engineering: "定期进行混沌工程演练"
```

### 案例二：金融行业数据一致性问题

```yaml
# 故障场景
financial_sector_incident:
  industry: "银行业"
  system: "核心交易数据库"
  requirement: "99.99%可用性，RPO=0，RTO=15分钟"
  incident_description: "数据库主从同步延迟导致数据不一致"

# 技术细节
technical_analysis:
  storage_setup:
    primary_db: "MySQL with ESSD PL3"
    replica_db: "MySQL with ESSD PL2"
    sync_method: "semi-sync replication"
    storage_encryption: "enabled with KMS"
    
  failure_point:
    symptom: "从库延迟逐渐增大至30分钟以上"
    root_cause: "存储I/O性能下降导致同步阻塞"
    contributing_factors:
      - high_write_load_during_peak_hours
      - insufficient_IOPS_reservation
      - lack_of_performance_monitoring

# 解决过程
troubleshooting_process:
  detection:
    - monitoring_alert_triggered: "复制延迟告警"
    - performance_analysis: "使用pt-query-digest分析慢查询"
    - storage_inspection: "检查云盘性能指标"
    
  resolution_steps:
    - immediate_scaling: "临时升级存储PL等级"
    - query_optimization: "优化高负载SQL语句"
    - load_balancing: "分散写入负载到多个时段"
    - capacity_planning: "重新评估存储容量需求"

# 最终解决方案
permanent_solution:
  architecture_improvement:
    - multi_zone_deployment: "跨可用区部署提高可用性"
    - read_write_separation: "读写分离减轻主库压力"
    - intelligent_caching: "引入Redis缓存层"
    - automated_failover: "配置MHA自动故障转移"
    
  monitoring_enhancement:
    - real_time_replication_monitoring: "实时复制延迟监控"
    - predictive_capacity_planning: "基于AI的容量预测"
    - automated_performance_tuning: "自适应性能调优"
```

---
## 自动化运维工具

### 智能故障诊断系统

```python
# PV/PVC智能故障诊断工具
import json
import subprocess
import yaml
from datetime import datetime
from typing import Dict, List, Optional

class StorageTroubleshooter:
    def __init__(self):
        self.diagnosis_rules = {
            'pvc_pending': self.diagnose_pvc_pending,
            'mount_failure': self.diagnose_mount_failure,
            'provisioning_failed': self.diagnose_provisioning_failed,
            'performance_issue': self.diagnose_performance_issue
        }
        
    def diagnose_cluster(self) -> Dict:
        """全面诊断存储系统健康状况"""
        diagnosis_report = {
            'timestamp': datetime.now().isoformat(),
            'cluster_status': {},
            'issues_found': [],
            'recommendations': []
        }
        
        # 检查基础资源状态
        diagnosis_report['cluster_status'] = self.check_basic_status()
        
        # 执行各类诊断
        for issue_type, diagnostic_func in self.diagnosis_rules.items():
            issues = diagnostic_func()
            if issues:
                diagnosis_report['issues_found'].extend(issues)
                
        # 生成修复建议
        diagnosis_report['recommendations'] = self.generate_recommendations(
            diagnosis_report['issues_found']
        )
        
        return diagnosis_report
    
    def check_basic_status(self) -> Dict:
        """检查基础资源状态"""
        status = {}
        
        # 检查StorageClass
        try:
            result = subprocess.run(
                ["kubectl", "get", "storageclass", "-o", "json"],
                capture_output=True, text=True, timeout=30
            )
            sc_data = json.loads(result.stdout)
            status['storageclasses'] = len(sc_data.get('items', []))
        except Exception as e:
            status['storageclasses_error'] = str(e)
            
        # 检查PVC状态
        try:
            result = subprocess.run(
                ["kubectl", "get", "pvc", "--all-namespaces", "-o", "json"],
                capture_output=True, text=True, timeout=30
            )
            pvc_data = json.loads(result.stdout)
            pvc_items = pvc_data.get('items', [])
            
            status['pvc_stats'] = {
                'total': len(pvc_items),
                'bound': len([p for p in pvc_items if p.get('status', {}).get('phase') == 'Bound']),
                'pending': len([p for p in pvc_items if p.get('status', {}).get('phase') == 'Pending']),
                'lost': len([p for p in pvc_items if p.get('status', {}).get('phase') == 'Lost'])
            }
        except Exception as e:
            status['pvc_error'] = str(e)
            
        return status
    
    def diagnose_pvc_pending(self) -> List[Dict]:
        """诊断Pending状态的PVC"""
        issues = []
        
        try:
            result = subprocess.run(
                ["kubectl", "get", "pvc", "--all-namespaces", 
                 "--field-selector=status.phase=Pending", "-o", "json"],
                capture_output=True, text=True, timeout=30
            )
            
            if result.returncode == 0:
                pending_pvcs = json.loads(result.stdout).get('items', [])
                
                for pvc in pending_pvcs:
                    issue = {
                        'type': 'pvc_pending',
                        'severity': 'high',
                        'namespace': pvc['metadata']['namespace'],
                        'name': pvc['metadata']['name'],
                        'storage_class': pvc['spec'].get('storageClassName'),
                        'requested_size': pvc['spec']['resources']['requests']['storage'],
                        'diagnosis': self.analyze_pvc_pending_causes(pvc)
                    }
                    issues.append(issue)
                    
        except Exception as e:
            issues.append({
                'type': 'diagnostic_error',
                'severity': 'medium',
                'error': f"PVC Pending诊断失败: {str(e)}"
            })
            
        return issues
    
    def analyze_pvc_pending_causes(self, pvc: Dict) -> List[str]:
        """分析PVC Pending的具体原因"""
        causes = []
        
        storage_class = pvc['spec'].get('storageClassName')
        if not storage_class:
            causes.append("未指定StorageClass")
            return causes
            
        # 检查StorageClass是否存在
        try:
            result = subprocess.run(
                ["kubectl", "get", "storageclass", storage_class],
                capture_output=True, text=True, timeout=10
            )
            if result.returncode != 0:
                causes.append(f"StorageClass '{storage_class}' 不存在")
        except:
            pass
            
        # 检查资源配额
        namespace = pvc['metadata']['namespace']
        try:
            result = subprocess.run(
                ["kubectl", "describe", "resourcequota", "-n", namespace],
                capture_output=True, text=True, timeout=10
            )
            if "exceeded quota" in result.stdout.lower():
                causes.append("命名空间资源配额已满")
        except:
            pass
            
        return causes
    
    def diagnose_mount_failure(self) -> List[Dict]:
        """诊断挂载失败问题"""
        issues = []
        
        # 检查Pod事件中的挂载错误
        try:
            result = subprocess.run(
                ["kubectl", "get", "events", "--field-selector", 
                 "reason=FailedMount", "--all-namespaces", "-o", "json"],
                capture_output=True, text=True, timeout=30
            )
            
            if result.returncode == 0:
                events = json.loads(result.stdout).get('items', [])
                for event in events[:10]:  # 限制数量避免过多
                    issue = {
                        'type': 'mount_failure',
                        'severity': 'high',
                        'namespace': event['metadata']['namespace'],
                        'pod': event['involvedObject']['name'],
                        'message': event['message'],
                        'timestamp': event['firstTimestamp']
                    }
                    issues.append(issue)
                    
        except Exception as e:
            issues.append({
                'type': 'diagnostic_error',
                'severity': 'medium',
                'error': f"挂载失败诊断失败: {str(e)}"
            })
            
        return issues
    
    def generate_recommendations(self, issues: List[Dict]) -> List[str]:
        """根据发现问题生成修复建议"""
        recommendations = []
        
        issue_types = [issue['type'] for issue in issues]
        
        if 'pvc_pending' in issue_types:
            recommendations.extend([
                "检查StorageClass配置是否正确",
                "验证CSI驱动状态和服务账户权限",
                "审查命名空间资源配额设置",
                "确认云服务商存储配额充足"
            ])
            
        if 'mount_failure' in issue_types:
            recommendations.extend([
                "检查节点上的存储插件状态",
                "验证VolumeAttachment对象状态",
                "确认网络连接和防火墙规则",
                "检查存储后端服务可用性"
            ])
            
        # 通用建议
        recommendations.extend([
            "建立完善的监控告警体系",
            "定期进行存储健康检查",
            "制定标准的故障响应流程",
            "维护详细的运维操作手册"
        ])
        
        return recommendations

# 使用示例
def main():
    troubleshooter = StorageTroubleshooter()
    report = troubleshooter.diagnose_cluster()
    
    print("=== 存储系统诊断报告 ===")
    print(f"诊断时间: {report['timestamp']}")
    print("\n集群状态:")
    for key, value in report['cluster_status'].items():
        print(f"  {key}: {value}")
    
    print(f"\n发现问题数量: {len(report['issues_found'])}")
    for i, issue in enumerate(report['issues_found'], 1):
        print(f"\n问题 {i}:")
        print(f"  类型: {issue['type']}")
        print(f"  严重程度: {issue['severity']}")
        if 'diagnosis' in issue:
            print(f"  诊断结果: {', '.join(issue['diagnosis'])}")
    
    print("\n修复建议:")
    for i, rec in enumerate(report['recommendations'], 1):
        print(f"  {i}. {rec}")

if __name__ == "__main__":
    main()
```

### 自动化修复脚本

```bash
#!/bin/bash
# storage-auto-healing.sh

# 存储系统自动化修复工具
AUTO_HEALING_ENABLED=true
LOG_FILE="/var/log/storage-auto-healing.log"

log_message() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a $LOG_FILE
}

# 自动修复Pending PVC
auto_heal_pending_pvc() {
    log_message "开始自动修复Pending PVC..."
    
    PENDING_PVCS=$(kubectl get pvc --all-namespaces --field-selector=status.phase=Pending -o json | \
        jq -r '.items[] | "\(.metadata.namespace)/\(.metadata.name)"')
    
    if [ -z "$PENDING_PVCS" ]; then
        log_message "未发现Pending PVC"
        return 0
    fi
    
    echo "$PENDING_PVCS" | while read pvc_info; do
        NAMESPACE=$(echo $pvc_info | cut -d'/' -f1)
        PVC_NAME=$(echo $pvc_info | cut -d'/' -f2)
        
        log_message "处理Pending PVC: $NAMESPACE/$PVC_NAME"
        
        # 获取PVC详细信息
        PVC_DETAILS=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o json)
        STORAGE_CLASS=$(echo $PVC_DETAILS | jq -r '.spec.storageClassName')
        
        # 检查StorageClass是否存在
        if ! kubectl get storageclass $STORAGE_CLASS >/dev/null 2>&1; then
            log_message "警告: StorageClass $STORAGE_CLASS 不存在"
            continue
        fi
        
        # 检查CSI驱动状态
        CSI_DRIVER=$(kubectl get storageclass $STORAGE_CLASS -o jsonpath='{.provisioner}')
        if ! kubectl get pods -n kube-system -l app=csi-controller | grep -q "Running"; then
            log_message "尝试重启CSI控制器..."
            kubectl delete pods -n kube-system -l app=csi-controller
            sleep 30
        fi
        
        # 如果问题仍然存在，尝试重新创建PVC
        sleep 60
        PVC_STATUS=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.status.phase}')
        if [ "$PVC_STATUS" = "Pending" ]; then
            log_message "PVC仍未绑定，尝试重新创建..."
            # 这里可以添加更复杂的重新创建逻辑
        fi
    done
}

# 自动清理异常的VolumeAttachment
auto_clean_volume_attachments() {
    log_message "清理异常的VolumeAttachment..."
    
    # 查找长时间未更新的VolumeAttachment
    OLD_ATTACHMENTS=$(kubectl get volumeattachment -o json | \
        jq -r '.items[] | select(.metadata.creationTimestamp < "'$(date -d '1 hour ago' --iso-8601)'") | .metadata.name')
    
    if [ -n "$OLD_ATTACHMENTS" ]; then
        echo "$OLD_ATTACHMENTS" | while read attachment; do
            log_message "删除旧的VolumeAttachment: $attachment"
            kubectl delete volumeattachment $attachment
        done
    fi
}

# 存储健康度检查
storage_health_check() {
    log_message "执行存储健康度检查..."
    
    HEALTH_SCORE=100
    
    # 检查PVC状态
    PENDING_PVC_COUNT=$(kubectl get pvc --all-namespaces --field-selector=status.phase=Pending | wc -l)
    if [ $PENDING_PVC_COUNT -gt 10 ]; then
        HEALTH_SCORE=$((HEALTH_SCORE - 20))
        log_message "警告: Pending PVC数量过多 ($PENDING_PVC_COUNT)"
    fi
    
    # 检查CSI驱动状态
    UNHEALTHY_CSI=$(kubectl get pods -n kube-system | grep csi | grep -v Running | wc -l)
    if [ $UNHEALTHY_CSI -gt 0 ]; then
        HEALTH_SCORE=$((HEALTH_SCORE - 30))
        log_message "警告: 发现异常的CSI Pod ($UNHEALTHY_CSI)"
    fi
    
    # 检查存储使用率
    HIGH_USAGE=$(kubectl get pvc --all-namespaces -o json | \
        jq -r '[.items[] | select(.status.capacity.storage and .spec.resources.requests.storage) |
                .usage = (.status.capacity.storage | split("Gi")[0] | tonumber) /
                        (.spec.resources.requests.storage | split("Gi")[0] | tonumber) |
                select(.usage > 0.9)] | length')
    
    if [ $HIGH_USAGE -gt 5 ]; then
        HEALTH_SCORE=$((HEALTH_SCORE - 15))
        log_message "警告: 高使用率PVC数量较多 ($HIGH_USAGE)"
    fi
    
    log_message "存储健康度评分: $HEALTH_SCORE/100"
    
    # 如果健康度低于阈值，发送告警
    if [ $HEALTH_SCORE -lt 70 ]; then
        log_message "严重: 存储系统健康度低于阈值，建议立即检查"
        # 这里可以集成告警通知系统
    fi
}

# 主执行循环
main() {
    log_message "=== 存储自动化运维系统启动 ==="
    
    while $AUTO_HEALING_ENABLED; do
        log_message "开始新一轮检查..."
        
        # 执行各项检查和修复
        auto_heal_pending_pvc
        auto_clean_volume_attachments
        storage_health_check
        
        log_message "本轮检查完成，等待下次执行..."
        sleep 300  # 5分钟执行一次
    done
}

# 信号处理
trap 'log_message "收到停止信号，正在退出..."; AUTO_HEALING_ENABLED=false' TERM INT

# 启动主程序
main
```

---
## 最佳实践总结

### 🎯 故障预防体系

1. **主动监控**: 建立全面的存储监控体系，提前发现潜在问题
2. **自动化运维**: 实施智能化的故障检测和自动修复机制
3. **预案完善**: 制定详细的故障响应预案和恢复流程
4. **知识沉淀**: 建立故障案例库和经验分享机制

### 📋 运维检查清单

```markdown
## 存储系统健康检查清单

### 每日检查
- [ ] PVC状态检查（Pending/Lost数量）
- [ ] CSI驱动组件运行状态
- [ ] 存储使用率监控（>85%告警）
- [ ] 关键存储指标趋势分析

### 每周检查
- [ ] 存储性能基准测试
- [ ] 备份策略执行情况验证
- [ ] 容量规划和扩容准备
- [ ] 安全合规性审计

### 每月检查
- [ ] 存储成本分析和优化
- [ ] 灾备演练执行
- [ ] 技术债务清理
- [ ] 运维流程优化

### 季度检查
- [ ] 存储架构评审和优化
- [ ] 新技术评估和试点
- [ ] 团队技能提升计划
- [ ] 供应商关系维护
```

---

**参考资料**:
- [Kubernetes 持久化存储](https://kubernetes.io/docs/concepts/storage/persistent-volumes/)
- [CSI 规范](https://github.com/container-storage-interface/spec)
- [Volume Snapshots](https://kubernetes.io/docs/concepts/storage/volume-snapshots/)
