# Cluster Create — Kubernetes 集群新建源码分析

本模块基于 Kubernetes 官方源码 (`kubernetes/cmd/kubeadm`)，系统梳理集群新建的完整逻辑。

---

## 文档索引

| 文档 | 内容 |
|------|------|
| [01-overview](01-overview.md) | 流程总览、入口分析、阶段划分 |
| [02-preflight](02-preflight.md) | 预检阶段：系统检查、端口检查、证书检查 |
| [03-certs](03-certs.md) | 证书阶段：PKI 生成、证书列表、轮换 |
| [04-kubeconfig](04-kubeconfig.md) | kubeconfig 阶段：四类配置文件生成逻辑 |
| [05-control-plane](05-control-plane.md) | 控制面阶段：静态 Pod、kubelet 配置、wait 机制 |
| [06-join](06-join.md) | 节点加入流程：TLS Bootstrapping、Bootstrap Token |
| [07-etcd](07-etcd.md) | etcd 初始化细节：manifest、健康检查、备份恢复 |
| [08-ha](08-ha.md) | 高可用控制面：stacked/external etcd、负载均衡器配置 |
| [09-upgrade](09-upgrade.md) | 集群升级流程：升级阶段、版本兼容性、回滚 |
| [10-cloud-comparison](10-cloud-comparison.md) | 云厂商方案与 kubeadm 对比：EKS/AKS/GKE/ACK/TKE |
| [11-advanced](11-advanced.md) | 进阶机制：InitConfiguration、CoreDNS 部署、NodeRestriction、FeatureGates |
| [12-join-advanced](12-join-advanced.md) | 节点加入进阶：CSR 自动审批、Bootstrap Token 生命周期、Node 对象注册 |
| [13-etcd-advanced](13-etcd-advanced.md) | etcd 进阶：WAL/Snapshot、defragmentation、读写流程、成员变更 |
| [14-ha-advanced](14-ha-advanced.md) | 高可用进阶：upload-certs、kube-vip、Leader Election、Endpoints 更新 |
| [15-upgrade-advanced](15-upgrade-advanced.md) | 升级进阶：etcd 单独升级、Feature Gates、回滚、验证 |
| [16-security](16-security.md) | 安全机制：BoundServiceAccountTokenVolumeProjection、Audit、加密存储 |
| [17-init-phases](17-init-phases.md) | init 阶段详解：mark-control-plane、upload-config、kubeadm config 家族 |
| [18-cri-runtime](18-cri-runtime.md) | CRI 容器运行时：containerd/Docker/cri-o、Pause 容器、crictl |
| [19-cni-networking](19-cni-networking.md) | CNI 网络：Calico/Cilium/Flannel、DNS 解析流程、Pod 网络模型 |
| [20-node-registration](20-node-registration.md) | Node 注册进阶：kubeadm token、--node-name、PodCIDR 分配、污点标签 |
| [21-kube-proxy](21-kube-proxy.md) | kube-proxy：iptables/ipvs/nftables 模式对比、Service 负载均衡、ExternalTrafficPolicy |
| [22-storage-volumes](22-storage-volumes.md) | 存储与卷：emptyDir/hostPath/PV/PVC/StorageClass、CSI、本地存储与调度 |
| [23-scheduler](23-scheduler.md) | kube-scheduler：调度流程、Predicates/Scoring、污点/亲和性、抢占调度、调度插件 |
| [24-what-kubeadm-does-not-install](24-what-kubeadm-does-not-install.md) | kubeadm 不安装的组件：CNI/metrics-server/Dashboard/Ingress/存储/LoadBalancer |
| [25-resource-management](25-resource-management.md) | 资源配额：ResourceQuota/LimitRange/PriorityClass/Eviction/PDB/EndpointSlice |

---

## 源码参考

- kubeadm 入口: `kubernetes/cmd/kubeadm/app/cmd/init.go`
- Phase 定义: `kubernetes/cmd/kubeadm/app/cmd/phases`
- 各阶段实现: `kubernetes/cmd/kubeadm/app/phases/`
