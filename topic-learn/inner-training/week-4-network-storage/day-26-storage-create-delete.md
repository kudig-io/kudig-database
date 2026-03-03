# Day 26: 存储卷创建 & 删除

> **学习时间**: 4-5 小时 | **主题**: PV/PVC 创建与生命周期管理

---

## 今日目标

- [ ] 理解 PV / PVC / StorageClass 三者关系
- [ ] 掌握阿里云云盘 (Disk) 和 NAS 类型的 PV 创建
- [ ] 能通过静态和动态方式创建存储卷
- [ ] 了解存储卷的回收策略与删除注意事项

---

## 理论学习 (2h)

### 必读文档

1. **K8S 存储基础**
   - 文件: `../../../domain-07-storage/01-storage-overview.md`
   - 重点: PV/PVC 概念、绑定机制、访问模式

2. **ACK 存储管理**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/270-ack-storage.md`
   - 重点: ACK CSI 插件、阿里云存储产品集成

3. **StorageClass 与动态供给**
   - 文件: `../../../domain-07-storage/02-storage-class.md`
   - 重点: StorageClass 参数、默认 StorageClass

### 阅读要点

- **PV (PersistentVolume)**: 集群级存储资源，管理员创建或动态供给
- **PVC (PersistentVolumeClaim)**: 用户的存储请求，绑定到 PV
- **StorageClass**: 定义存储类型和供给策略，实现动态创建 PV
- **ACK 存储类型**: 云盘 (alicloud-disk)、NAS (alicloud-nas)、OSS (alicloud-oss)
- **访问模式**: ReadWriteOnce (RWO) / ReadOnlyMany (ROX) / ReadWriteMany (RWX)
- **回收策略**: Retain (保留) / Delete (删除)
- 云盘仅支持 RWO，NAS 支持 RWX

---

## 实践任务 (2.5h)

### 任务 1: 查看默认 StorageClass (30min)

```bash
# 查看已有的 StorageClass
kubectl get sc

# 查看默认 StorageClass 详情
kubectl describe sc alicloud-disk-ssd 2>/dev/null || kubectl describe sc $(kubectl get sc -o jsonpath='{.items[0].metadata.name}')

# 查看 CSI 插件状态
kubectl get pods -n kube-system | grep csi

# 查看 CSI Driver
kubectl get csidrivers
```

### 任务 2: 动态创建云盘 PVC (40min)

```bash
# 创建 PVC (动态供给，自动创建云盘)
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: disk-pvc-demo
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: alicloud-disk-ssd
  resources:
    requests:
      storage: 20Gi
EOF

# 查看 PVC 状态 (等待首次挂载时才创建云盘)
kubectl get pvc disk-pvc-demo

# 创建 Pod 触发云盘创建
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: disk-pod-demo
spec:
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
    volumeMounts:
    - name: disk-vol
      mountPath: /data
  volumes:
  - name: disk-vol
    persistentVolumeClaim:
      claimName: disk-pvc-demo
EOF

# 等待 Pod Running
kubectl get pod disk-pod-demo -w

# 查看 PVC 和 PV 绑定状态
kubectl get pvc disk-pvc-demo
kubectl get pv

# 验证挂载
kubectl exec disk-pod-demo -- df -h /data
kubectl exec disk-pod-demo -- sh -c 'echo "test data" > /data/test.txt && cat /data/test.txt'
```

### 任务 3: 静态创建 NAS PV (40min)

```bash
# 创建 NAS 类型 PV (静态方式，需要已有 NAS 文件系统)
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolume
metadata:
  name: nas-pv-demo
  labels:
    type: nas
spec:
  capacity:
    storage: 50Gi
  accessModes:
  - ReadWriteMany
  persistentVolumeReclaimPolicy: Retain
  csi:
    driver: nasplugin.csi.alibabacloud.com
    volumeHandle: nas-pv-demo
    volumeAttributes:
      server: "<nas-mount-target>.cn-hangzhou.nas.aliyuncs.com"
      path: "/training-demo"
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: nas-pvc-demo
spec:
  accessModes:
  - ReadWriteMany
  resources:
    requests:
      storage: 50Gi
  selector:
    matchLabels:
      type: nas
EOF

# 查看绑定状态
kubectl get pv nas-pv-demo
kubectl get pvc nas-pvc-demo
```

### 任务 4: 存储卷删除与回收策略 (30min)

```bash
# 查看当前 PV 的回收策略
kubectl get pv -o custom-columns='NAME:.metadata.name,RECLAIM:.spec.persistentVolumeReclaimPolicy,STATUS:.status.phase'

# 删除 Pod (PVC 不受影响)
kubectl delete pod disk-pod-demo

# 删除 PVC (触发回收策略)
# - Delete 策略: PV 和底层云盘一起删除
# - Retain 策略: PV 变为 Released，底层云盘保留
kubectl delete pvc disk-pvc-demo

# 查看 PV 状态变化
kubectl get pv

# 清理 NAS PV
kubectl delete pvc nas-pvc-demo
kubectl delete pv nas-pv-demo

# 注意: Retain 策略下，需要手动清理 Released 状态的 PV
kubectl get pv | grep Released
# kubectl delete pv <pv-name>  # 手动清理
```

---

## 费曼复述 (0.5h)

1. **PV、PVC、StorageClass 三者的关系是什么？动态供给的流程是怎样的？**
2. **为什么阿里云云盘只支持 ReadWriteOnce？NAS 支持 ReadWriteMany 的原理是什么？**
3. **回收策略 Delete 和 Retain 的区别是什么？生产环境应该选哪种？**

---

## 今日检验

- [ ] 能查看 StorageClass 和 CSI 插件状态
- [ ] 能通过动态供给创建云盘 PVC
- [ ] 理解静态创建 NAS PV 的配置
- [ ] 了解存储卷删除和回收策略

---

## 核心概念总结

| 存储类型 | StorageClass | 访问模式 | 适用场景 |
|---------|-------------|---------|---------|
| 云盘 SSD | alicloud-disk-ssd | RWO | 数据库、有状态应用 |
| 云盘高效 | alicloud-disk-efficiency | RWO | 一般存储需求 |
| NAS | alicloud-nas | RWX | 共享存储、多 Pod 读写 |
| OSS | alicloud-oss | ROX | 静态资源、日志归档 |

| 回收策略 | 删除 PVC 后行为 | 推荐场景 |
|---------|---------------|---------|
| Delete | PV + 底层存储一起删除 | 测试/临时数据 |
| Retain | PV 变 Released，存储保留 | **生产环境推荐** |

---

## 明日预告

Day 27 将学习存储卷的挂载方式与最佳实践。
