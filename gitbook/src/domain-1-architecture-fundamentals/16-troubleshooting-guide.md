# 16 - Kubernetes 故障排查专家级指南

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **专家级别**: ⭐⭐⭐⭐⭐

## 一、架构级故障排查逻辑

在 Kubernetes 生产环境中，故障排查应遵循从底层到高层、从控制平面到数据平面的结构化路径。

### 1.1 排查漏斗模型
1. **基础设施层**：计算、存储、网络连通性。
2. **控制平面层**：API Server 响应、etcd 延迟、调度器状态。
3. **数据平面层**：Kubelet 状态、CNI/CSI 正常运行、节点资源压力。
4. **应用层**：Pod 状态、容器日志、探针失败。

---

## 二、核心组件故障诊断

### 2.1 API Server 故障
- **现象**：`kubectl` 响应慢或返回 5xx。
- **排查指标**：
  - `apiserver_request_duration_seconds` (P99 < 1s)
  - `apiserver_current_inflight_requests` (APF 限制检查)
- **常见原因**：etcd 性能瓶颈、APF 限流、大流量 Webhook 阻塞。

### 2.2 etcd 性能问题
- **现象**：Leader 频繁切换，写入延迟高。
- **诊断命令**：
  ```bash
  etcdctl endpoint status --write-out=table
  etcdctl check perf
  ```
- **优化点**：磁盘 IOPS (推荐 NVMe)、Heartbeat 间隔、Leaner 节点应用。

---

## 三、网络与存储排查

### 3.1 CNI 网络故障 (以 Cilium 为例)
- **工具**：`cilium-health`
- **检查项**：
  - BPF Map 是否满额
  - 身份验证 (Identity) 冲突
  - 跨节点 VXLAN 封包问题

### 3.2 存储挂载失败
- **常见错误**：`Multi-Attach error`, `Timeout on mount`
- **排查路径**：
  - 检查 CSI Controller 容器日志
  - 检查节点上的 `/var/log/messages` 或 `dmesg`
  - 验证 PVC/PV 状态及 StorageClass 配置

---

## 四、生产级诊断工具箱

| 工具 | 用途 | 专家提示 |
|:---|:---|:---|
| **kubectl-debug** | 启动诊断容器 | 使用 `--ephemeral-containers` (v1.25+ GA) |
| **Cilium Hubble** | 网络流量可视化 | 观察 L7 层的拒绝原因 |
| **Inspektor Gadget** | eBPF 实时监控 | 捕获进程级文件读写和网络连接 |
| **Prometheus** | 长期趋势分析 | 关注组件的 Memory/CPU 波动 |

---

## 五、经典案例库

### 5.1 案例：APF 导致的诡异请求延迟
- **诊断**：通过 `apiserver_flowcontrol_rejected_requests_total` 发现大量请求被 drop。
- **解决**：调整 `PriorityLevelConfiguration` 和 `FlowSchema`，为核心 Operator 预留带宽。

### 5.2 案例：etcd 磁盘 IO 抖动引发集群雪崩
- **现象**：Kubelet 失去连接，控制平面无法修改资源。
- **诊断**：`etcd_disk_wal_fsync_duration_seconds` 超过 50ms。
- **解决**：迁移 etcd 数据目录到专用 SSD，并设置内核 `ionice`。

---

**维护者**: Kusheet SRE Team | **作者**: Allen Galler