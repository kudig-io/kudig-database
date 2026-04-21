# 节点存储

## 源码路径

`pkg/volume/`
`pkg/kubelet/volumemanager/`

---

## 节点存储类型

```
节点存储层次:
  ┌─────────────────────────────────────────────────────────────┐
  │  临时存储 (emptyDir)                                          │
  │  - 存储在节点磁盘                                            │
  │  - Pod 删除后清除                                            │
  │  - 可选 memory (tmpfs)                                       │
  └─────────────────────────────────────────────────────────────┘
  ┌─────────────────────────────────────────────────────────────┐
  │  主机存储 (hostPath)                                         │
  │  - 挂载节点文件系统                                           │
  │  - 用于日志收集、监控等                                        │
  └─────────────────────────────────────────────────────────────┘
  ┌─────────────────────────────────────────────────────────────┐
  │ 持久存储 (PV/PVC)                                            │
  │  - 云盘/NFS/本地存储                                         │
  │  - 通过 CSI 挂载                                             │
  └─────────────────────────────────────────────────────────────┘
```

---

## local PV

```yaml
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv
spec:
  capacity:
    storage: 100Gi
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-storage
  local:
    path: /mnt/disk
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - node-1
```

---

## CSI Node 插件

```bash
# CSI Node 插件职责:
# 1. NodeGetInfo: 报告节点存储能力
# 2. NodeStageVolume: 准备卷 (挂载到 staging 目录)
# 3. NodePublishVolume: 将卷挂载到 Pod
# 4. NodeUnpublishVolume: 卸载卷
# 5. NodeUnstageVolume: 清理 staging 目录

# 查看 CSI 插件
kubectl get pods -n kube-system -l app=csi
```

---

## 存储拓扑

```bash
# 延迟卷绑定确保调度到正确节点
volumeBindingMode: WaitForFirstConsumer

# 拓扑键:
topology.kubernetes.io/hostname
topology.kubernetes.io/zone
topology.kubernetes.io/region
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| PVC Pending | 无可用 PV | 检查 StorageClass |
| 卷挂载失败 | CSI 插件问题 | 检查 CSI driver |
| Pod 无法调度 | 拓扑限制 | 检查节点标签 |
