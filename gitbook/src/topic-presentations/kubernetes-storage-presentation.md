# Kubernetes 存储体系全栈进阶培训 (从入门到专家)

> **适用版本**: Kubernetes v1.28 - v1.32 | **文档类型**: 全栈技术实战指南
> **目标受众**: 初级运维、存储架构师、SRE
> **核心原则**: 理解持久化本质、掌握 CSI 挂载机制、确保数据容灾闭环

---

## 🔰 第一阶段：快速入门与核心概念

### 1.1 为什么需要持久化存储？
*   **容器特性**: 容器文件系统是临时的（Ephemeral），Pod 重启后数据会丢失。
*   **核心资源**:
    *   **PV (PersistentVolume)**: 实际的存储资源（如一块云盘、一个 NFS 目录）。
    *   **PVC (PersistentVolumeClaim)**: 用户对存储的需求申请（“我要 10G，RWO 权限”）。
    *   **StorageClass**: 存储的“模板”，实现自动按需创建 PV (Dynamic Provisioning)。

### 1.2 简单示例 (使用 StorageClass)
```yaml
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: my-pvc
spec:
  accessModes:
    - ReadWriteOnce
  resources:
    requests:
      storage: 10Gi
  storageClassName: alibabacloud-disk-ssd
```

---

## 📘 第二阶段：核心架构与深度原理 (Deep Dive)

### 2.1 CSI 挂载全流程
1.  **Provision**: 创建云盘。
2.  **Attach**: 将云盘挂载到 ECS 节点。
3.  **Mount**: 
    *   **Stage**: 格式化并挂载到 Node 目录。
    *   **Publish**: Bind-mount 到 Pod 容器目录。

### 2.2 存储拓扑感知
*   **延迟绑定 (`WaitForFirstConsumer`)**: 确保云盘创建在 Pod 被调度的那个可用区 (AZ)，解决跨区挂载失败问题。

---

## ⚡ 第三阶段：生产部署与极致优化

### 2.1 性能调优
*   **文件系统**: 优化 `mountOptions`。
*   **IOPS 预留**: 根据业务峰值选择合适的存储规格（如 ESSD PL1/PL2）。

### 2.2 监控告警
*   **容量告警**: 剩余可用空间 < 10% 报警。
*   **Inode 告警**: 小文件场景下防止 Inode 耗尽。

---

## 🛠️ 第四阶段：故障诊断与数据容灾 (SRE Ops)

### 3.1 典型故障排查
*   **Multi-Attach**: Pod A 挂载着盘，Pod B 在另一个节点也尝试挂载。
*   **Stuck in ContainerCreating**: 检查 CSI 驱动日志及云平台 OpenAPI 权限。

### 3.2 备份与恢复 (DR)
*   **Velero**: 业界标准的 K8S 备份工具。
*   **Snapshot**: 利用云平台快照实现数据秒级备份。

---

## 🛡️ 第五阶段：安全加固与总结

### 🏆 SRE 运维红线
*   *红线 1: 生产环境核心数据必须使用 `Retain` 回收策略。*
*   *红线 2: 严禁直接在 Pod 中使用 HostPath 存储敏感数据。*
*   *红线 3: 必须配置延迟绑定策略以支持多可用区集群。*
