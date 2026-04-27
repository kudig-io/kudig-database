# 网络与存储故障排查内容全面加强

## 需求背景

当前 `topic-structural-trouble-shooting` 知识库中，网络和存储相关内容存在以下薄弱环节：

1. **Terway（阿里云CNI）**：全库零覆盖，而 Terway 作为阿里云 ACK 默认网络方案，在生产环境大量使用，其 ENI 模式、IPVlan 模式、IPAM 机制、安全组绑定、网络策略兼容性等问题具有鲜明的技术特点，与标准 CNI（Calico/Flannel/Cilium）有显著差异。
2. **Flannel**：仅在通用 CNI 排查文档中零散提及（约 30 处），缺乏专项深度排查指南。Flannel 的 VXLAN/host-gw/UDP 模式、子网分配（etcd vs Kubernetes API 后端）、MTU 问题、跨节点通信失败等核心故障场景需要系统化覆盖。
3. **PV/PVC**：已有 1287 行深度文档，覆盖较全面，但 StorageClass 相关内容分散在 PV/PVC 和 CSI 文档中，缺乏以 StorageClass 为核心的专项排查指南。

## 目标

创建 3 篇高质量专项故障排查文档，填补上述薄弱环节，并同步更新 README 索引。

## 新增文档规划

### 1. `03-networking/07-terway-troubleshooting.md`（Terway 深度排查）

**适用场景**：阿里云 ACK/ASK 集群中基于 Terway 的 Pod 网络故障。

**核心内容模块**：
- **Terway 架构解析**：ENI 模式 vs Veth 模式 vs IPVlan 模式，弹性网卡分配机制，Trunk ENI
- **IPAM 故障**：固定 IP、弹性 IP、共享 ENI、IP 资源池耗尽、IP 分配冲突
- **Pod 网络不通**：同节点/跨节点通信失败、VPC 路由缺失、安全组规则阻断
- **网络策略问题**：Calico NetworkPolicy 与 Terway 的集成、策略不生效
- **性能与稳定性**：高并发场景下 ENI 分配延迟、kube-proxy 与 Terway 的交互
- **内核与版本兼容性**：Terway 对内核版本的要求、BPF 程序加载失败
- **调试工具链**：`terway-cli` 使用、节点 Annotation 查看、弹性网卡控制台核对

**预期篇幅**：约 800-1000 行

### 2. `03-networking/08-flannel-troubleshooting.md`（Flannel 专项排查）

**适用场景**：使用 Flannel 作为 CNI 的 Kubernetes 集群网络故障。

**核心内容模块**：
- **Flannel 架构与后端模式**：VXLAN（默认）、host-gw、UDP（已废弃）、扩展后端（AliVPC/AWS VPC/GCE）
- **子网分配故障**：etcd 后端 vs Kubernetes API 后端、Subnet 冲突/CIDR 重叠、节点子网未分配
- **VXLAN 隧道问题**：VTEP MAC 学习失败、FDB 表异常、VXLAN 端口（4789）被阻、MTU 不匹配
- **host-gw 模式问题**：直连路由缺失、二层连通性要求、跨子网不可达
- **跨节点通信失败**：ARP 表异常、iptables 规则冲突、flanneld 守护进程异常
- **Pod IP 分配异常**：IPAM 池耗尽、CNI 配置错误、flannel cni 插件与 kubelet 版本不兼容
- **升级与迁移**：从 etcd 后端迁移到 Kubernetes API 后端、Flannel 版本升级兼容性
- **与其他组件冲突**：与 NetworkPolicy（不支持）、与 Calico 共存（Canal）

**预期篇幅**：约 800-1000 行

### 3. `04-storage/05-storageclass-troubleshooting.md`（StorageClass 专项排查）

**适用场景**：因 StorageClass 配置错误导致的存储供给、绑定、性能、扩容等故障。

**核心内容模块**：
- **StorageClass 核心参数解析**：`provisioner`、`parameters`、`reclaimPolicy`、`volumeBindingMode`、`allowVolumeExpansion`、`mountOptions`
- **动态供给失败**：Provisioner 未注册、参数错误、后端配额耗尽、API 调用失败
- **绑定模式问题**：`Immediate` vs `WaitForFirstConsumer` 的适用场景与故障表现、拓扑延迟绑定
- **默认 StorageClass 问题**：多默认类冲突、默认类被删除、新 PVC 绑定到错误类
- **扩容失败**：`allowVolumeExpansion=false`、底层存储不支持、文件系统未扩展
- **性能等级选择错误**：IOPS/吞吐参数配置不当、延迟敏感应用使用标准盘
- **SnapshotClass 关联**：SnapshotClass 与 StorageClass 的 `driver` 匹配、从快照恢复时类选择
- **多租户与配额**：不同 Namespace 使用不同 StorageClass、ResourceQuota 与存储类限制
- **云厂商特定参数**：AWS EBS (`type`, `iops`, `encrypted`)、阿里云 Disk (`type`, `regionId`, `zoneId`)、GCP PD (`type`, `replication-type`)

**预期篇幅**：约 700-900 行

## 受影响的现有文件

| 文件 | 修改类型 | 说明 |
|------|----------|------|
| `03-networking/01-cni-troubleshooting.md` | 引用更新 | 在相关章节添加指向新 Flannel 专项文档的交叉引用 |
| `04-storage/01-pv-pvc-troubleshooting.md` | 引用更新 | 在 StorageClass 相关章节添加指向新 StorageClass 专项文档的交叉引用 |
| `topic-structural-trouble-shooting/README.md` | 更新 | 添加新文档到目录结构表、按症状查找、按组件查找 |

## README 更新计划

- **文档总数**：60 → 63（+3 篇新文档）
- **03-networking 类别文档数**：6 → 8
- **04-storage 类别文档数**：4 → 5
- **新增索引条目**：
  - 按症状：Terway Pod 网络不通、Flannel 跨节点通信失败、StorageClass 配置错误
  - 按组件：Terway、Flannel、StorageClass

## 技术选型与边界条件

- **文档格式**：严格遵循知识库已有的"四要素法"模板
- **代码示例**：所有命令经过验证，适用于 Kubernetes v1.25-v1.32
- **兼容性**：Terway 文档基于 ACK 集群常见版本（Terway v1.2+）；Flannel 文档覆盖 v0.20+；StorageClass 覆盖 K8s 内置 + 主流 CSI 驱动
- **边界处理**：
  - 不覆盖已废弃的 Flannel UDP 后端在生产环境的新部署场景，但保留历史系统排查内容
  - Terway 文档聚焦开源/阿里云标准版，不涉及私有定制版本
  - StorageClass 文档聚焦动态供给，静态 PV 的 StorageClass 绑定逻辑已在 PV/PVC 文档覆盖

## 数据流与排查路径

### Terway 排查路径
```
Pod 无 IP / 网络不通
  ├─ Terway Pod 状态 → kubectl get pods -n kube-system -l app=terway
  ├─ 弹性网卡分配 → terway-cli show | 阿里云控制台
  ├─ 模式确认 → ENI / Veth / IPVlan
  ├─ IPAM 检查 → 固定 IP / 共享 ENI / IP 池耗尽
  ├─ VPC 路由 → 路由表是否包含 Pod CIDR
  ├─ 安全组 → 是否放通 Pod 间通信端口
  └─ 网络策略 → Calico 策略是否正确生效
```

### Flannel 排查路径
```
跨节点 Pod 不通
  ├─ flanneld 状态 → DaemonSet Pod Running?
  ├─ 后端模式 → VXLAN / host-gw / UDP
  ├─ 子网分配 → cat /run/flannel/subnet.env
  ├─ CNI 配置 → /etc/cni/net.d/10-flannel.conflist
  ├─ VTEP/FDB → ip -d link show flannel.1; bridge fdb show
  ├─ 路由表 → ip route | grep flannel
  ├─ MTU 检查 → ping -M do -s 1472 <pod-ip>
  └─ 端口防火墙 → UDP 4789 (VXLAN) / 8472 (host-gw)
```

### StorageClass 排查路径
```
PVC Pending / 供给失败
  ├─ StorageClass 存在性 → kubectl get sc
  ├─ Provisioner 注册 → kubectl get csidriver / 内置 provisioner
  ├─ 参数验证 → parameters 是否符合后端要求
  ├─ 绑定模式 → Immediate vs WaitForFirstConsumer
  ├─ 拓扑约束 → allowedTopologies / 可用区匹配
  ├─ 后端配额 → 云厂商存储配额是否充足
  ├─ 默认类冲突 → 是否多个 StorageClass 标记为 default
  └─ CSI 驱动日志 → external-provisioner 具体报错
```

## 预期产出

1. `03-networking/07-terway-troubleshooting.md` — 阿里云 Terway CNI 专项排查（800-1000 行）
2. `03-networking/08-flannel-troubleshooting.md` — Flannel CNI 专项排查（800-1000 行）
3. `04-storage/05-storageclass-troubleshooting.md` — StorageClass 专项排查（700-900 行）
4. `README.md` 更新 — 目录索引、快速定位、统计数同步
