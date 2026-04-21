# Node Create — Kubernetes 节点生命周期管理

本模块系统梳理 Kubernetes 节点从注册到运维管理的完整生命周期。

---

## 文档索引

| 文档 | 内容 |
|------|------|
| [01-overview](01-overview.md) | 节点生命周期总览、节点组件、状态流转 |
| [02-registration](02-registration.md) | 节点注册流程：kubelet bootstrap、CSR、Node 对象 |
| [03-condition](03-condition.md) | 节点状态与健康检查：Ready/Pressure/Disk/Network |
| [04-drain](04-drain.md) | 节点维护：drain/cordon/uncordon、PDB、Pod 驱逐 |
| [05-upgrade](05-upgrade.md) | 节点升级：kubeadm upgrade node、kubelet 升级、OS patch |
| [06-certificate](06-certificate.md) | 节点证书轮换：kubelet 证书自动续期、CSR 审批 |
| [07-autoscaling](07-autoscaling.md) | 节点弹性伸缩：Cluster Autoscaler、AWS/GCP/Azure 集成 |
| [08-troubleshooting](08-troubleshooting.md) | 节点故障排查：NotReady/NoExecute、kubelet 异常、网络问题 |
| [09-cni-node](09-cni-node.md) | 节点网络：CNI 配置、Pod 网络命名空间、veth pair |
| [10-kubelet-config](10-kubelet-config.md) | kubelet 进阶配置：cgroup driver、资源管理、日志配置 |
| [11-eviction](11-eviction.md) | 节点资源压力：Eviction Thresholds、QoS、OOM Kill |
| [12-monitoring](12-monitoring.md) | 节点监控：metrics-node-exporter、kubectl top、节点指标 |
| [13-security](13-security.md) | 节点安全：Node Authorization、NodeRestriction、PSP |
| [14-storage-node](14-storage-node.md) | 节点存储：local PV、CSI Node 插件、存储拓扑 |
| [15-cloud-node](15-cloud-node.md) | 云厂商节点：AWS EC2/GCE/GCE/Azure 集成、provider-id |
