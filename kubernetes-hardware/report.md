# Kubernetes 硬件支持调研报告

> 调研范围: 2024-2026 | 生成日期: 2026-08-09 | Items 总数: 22

## 目录

1. [轻量级发行版 / ARM 边缘集群](#arm-edge-clusters) - 架构
2. [节点自动扩缩容硬件要求（Cluster Autoscaler / Karpenter 节点自动扩缩容方案，以及 Kubernetes 1.35+ Workload-Aware Scheduling 硬件感知调度）](#autoscaling-hardware-requirements) - 运维
3. [CPU 指令集架构 (ISA) - amd64/x86_64、arm64、s390x、ppc64le](#cpu-isa) - 架构
4. [CXL 内存池化与分层（Compute Express Link Memory Pooling & Tiering）](#cxl-memory) - 内存
5. [Kubernetes 控制平面硬件（etcd、kube-apiserver、HA 控制面节点、NUMA 拓扑）](#control-plane-hardware) - 控制平面
6. [DPU / IPU / SmartNIC 硬件卸载（NVIDIA BlueField、Intel IPU、AMD Pensando，配套 OVN-Kubernetes DPU 模式与模拟器）](#dpu-ipu-smartnic) - 网络
7. [DRA（Dynamic Resource Allocation，动态资源分配）](#dra-dynamic-resource-allocation) - 资源管理
8. [FPGA 可重构加速（Intel FPGA 设备插件 / AMD Xilinx Alveo 方案 / Funky 云原生 FPGA 编排 (2025)）](#fpga-acceleration) - 加速器
9. [GPU 加速器（NVIDIA GPU Operator / AMD ROCm 设备插件 / Intel XPU 插件，含 MIG、MPS、Time-Slicing、DRA 调度、GPU 直通等 GPU 共享与虚拟化技术）](#gpu-accelerators) - 加速器
10. [Intel DSA (Data Streaming Accelerator) 与 Intel IAA (In-Memory Analytics Accelerator) —— 英特尔数据流加速器与内存分析加速器](#intel-dsa-iaa) - 加速器
11. [Intel QAT (QuickAssist Technology) 英特尔快速辅助技术](#intel-qat) - 加速器
12. [内存管理](#memory-management) - 内存
13. [异构 / 多架构集群管理 (Multi-Arch Cluster Management)](#multi-arch-cluster-management) - 运维
14. [NPU / AI 推理芯片（Intel NPU（AI Boost）、Hailo-8/8L、Rockchip NPU（RK3588/RK3576）、Qualcomm AI Engine（Cloud AI 100/Ultra、Snapdragon NPU）、华为昇腾 NPU 等专用神经处理单元，含 K8s 设备插件/CDI/DRA 管理、KubeEdge 边缘部署、与 GPU 的选型对比）](#npu-ai-chips) - 加速器
15. [网络基础设施（CNI 插件、网卡、DPDK、SR-IOV、服务网格与 eBPF 硬件要求）](#networking-infrastructure) - 网络
16. [电源管理与能效（CPU 电源管理 / 碳感知调度 / 能耗可观测性）——涵盖 Intel Kubernetes Power Manager、Intel Infrastructure Power Manager (IPM) Operator、AMD EPYC 能效优化（amd-pstate/CPPC）、Linux CPU 频率/C-state/P-state 控制、StarlingX 可配置电源管理（Configurable Power Manager）、碳感知调度（Carbon-aware Scheduling）与 Kepler/Scaphandre 等能耗可观测性方案](#power-management-energy) - 运维
17. [RDMA / InfiniBand 高速互联（InfiniBand、RoCE v2，含 RDMA 设备插件与 CNI 生态）](#rdma-infiniband) - 网络
18. [Secure Boot / 硬件信任根（UEFI Secure Boot 与 TPM 2.0 信任根）](#secure-boot-trust-root) - 安全
19. [存储系统（节点本地存储、CSI 驱动、持久化存储后端、文件系统、NVMe 耐久度、本地 PV 调度）](#storage-systems) - 存储
20. [可信执行环境 (TEE) / 机密计算 (Confidential Computing)](#tee-confidential-computing) - 安全
21. [VPU / 媒体加速器（Intel QSV、NVIDIA NVENC/NVDEC、AMD VCE/VCN、NETINT VPU 等硬件视频编解码器，含 K8s 设备插件管理与云原生视频转码生态）](#vpu-media-accelerators) - 加速器
22. [Windows 节点](#windows-nodes) - 架构

## 1. 轻量级发行版 / ARM 边缘集群

**官方文档**: K3s: https://docs.k3s.io/ (Rancher/SUSE); KubeEdge: https://kubeedge.io/ (CNCF Graduated/Huawei); MicroK8s: https://canonical.com/microk8s (Canonical); K0s: https://k0sproject.io/ (CNCF); Talos Linux: https://www.siderolabs.com/ (Sidero Labs, 2025 边缘参考架构)

### 硬件规格

**最低配置**

K3s: 服务器节点: 2 核 CPU、2 GB RAM、建议 SSD 存储; 工作节点: 1 核 CPU、512 MB RAM、建议 SSD 存储。二进制文件 < 100 MB。; KubeEdge: 云端部分: 与上游 K8s 一致，约 2 核 CPU、2 GB RAM; 边缘节点 (EdgeCore): 最低 70-80 MB 内存，可在 100 MB RAM 设备上运行，需 1 核 CPU。; MicroK8s: 1 核 CPU、1 GB RAM (512 MB 不足以运行)、数 GB 磁盘空间 (snap 包约 216 MB)。需启用 cgroups。; K0s: 控制节点: 1 vCPU、1 GB RAM、约 0.5 GB 磁盘; 工作节点: 1 vCPU、0.5 GB RAM、约 1.3 GB 磁盘; 合设节点: 1 vCPU、1 GB RAM、约 1.7 GB 磁盘。; Talos Linux (Sidero Labs 2025): ARM64 设备: Raspberry Pi 4/5 (4 核, 1-8 GB RAM), NVIDIA Jetson 系列, Pine64/Rock 板。需 UEFI 或 DTB 引导。

**推荐配置**

K3s: 服务器节点: 4 核 CPU、4-8 GB RAM、SSD 存储; 工作节点: 2 核 CPU、1-2 GB RAM、SSD 存储。用于生产的小规模集群。; KubeEdge: 云端部分: 4 核 CPU、8 GB RAM、SSD; 边缘节点: 1-2 核 CPU、256 MB-1 GB RAM，根据实际工作负载调整。; MicroK8s: 2 核 CPU、2-4 GB RAM、SSD 存储 (避免 USB/SD 卡作为主存储因 I/O 压力)。; K0s: 控制节点: 2 核 CPU、2 GB RAM、SSD; 工作节点: 1-2 核 CPU、1 GB RAM、SSD。小规模集群 1-2 GB 内存即可。; Talos Linux (Sidero Labs 2025): 4 核 CPU、4-8 GB RAM、SSD (eMMC 或 NVMe), 支持 Ampere Altra 等服务器级 ARM64 平台。

**生产级配置**

K3s: 服务器节点: 8-16 核 CPU、16-32 GB RAM、SSD/NVMe、嵌入式 etcd 或外部数据库; 支持 1800+ 工作节点。; KubeEdge: 云端部分: 8-128 核 CPU、32-256 GB RAM、SSD (已验证 100,000 边缘节点规模); 边缘节点: 2-4 核 CPU、1-4 GB RAM、eMMC/SSD。; MicroK8s: 4-8 核 CPU、8-16 GB RAM、SSD/NVMe、高可用集群 (3+ 节点)。; K0s: 控制节点: 4-8 核 CPU、8-32 GB RAM、SSD; 大规模集群 (1000+ 节点): 32-64 GB RAM。; Talos Linux (Sidero Labs 2025): 零售、工厂自动化和机器人场景: 经认证的硬件设备，单节点可调度 K8s 环境，支持可信启动，Omni 管理平台进行集群生命周期管理。

### 兼容性

**支持的 K8s 版本范围**

K3s: 随上游 K8s 发布节奏同步更新，当前最新 v1.36.3+k3s1。支持从 v1.20 起的所有版本 (通过版本选择)。; KubeEdge: v1.23 支持 K8s 1.27-1.32; v1.22 支持 K8s 1.29-1.31; v1.20/1.21 支持 K8s 1.28-1.30; v1.19 支持 K8s 1.27-1.29。HEAD 分支支持 1.30-1.32。; MicroK8s: 随上游 K8s 发布节奏同步更新，当前最新 MicroK8s 1.36 (对应 K8s 1.36)。支持通过 snap 通道选择版本。; K0s: 直接基于上游 K8s 构建，版本号格式为 v{upstream_k8s}+k0s.X (如 v1.30.2+k0s.0)。支持 K8s 1.24+。; Talos Linux (Sidero Labs 2025): 通过 Omni 管理平台管理 K8s 版本，支持近期上游 K8s 版本。以不可变 OS 镜像方式提供。

**操作系统兼容性**

K3s: 大多数现代 Linux 发行版: Ubuntu、Debian、Raspbian/Raspberry Pi OS、CentOS、RHEL、Fedora、openSUSE、SUSE Linux Enterprise。ARM64 需内核支持 vxlan。; KubeEdge: 云端: Ubuntu、CentOS、Debian 等主流 Linux; 边缘节点: 主流 Linux 发行版，容器化 EdgeCore 运行。; MicroK8s: Ubuntu (首选，通过 snap 安装)、Debian、Raspberry Pi OS (64-bit)、以及其他支持 snapd 的 Linux 发行版。; K0s: Linux (所有主流发行版)、Windows Server 2019/2022 (仅工作节点)。; Talos Linux (Sidero Labs 2025): 专用不可变 Linux 操作系统 (Talos Linux)，非通用发行版。支持 ARM64 平台: Raspberry Pi 4/5 (rpi_generic)、NVIDIA Jetson (sbc-jetson)、Ampere Altra (metal-arm64)、AWS Graviton (aws-arm64)、Pine64/Rock 板 (SBC overlay)。

**K8s 上游支持阶段**

K3s: CNCF 孵化项目 (Incubation)，Rancher/SUSE 主导。生产就绪度高，广泛用于边缘和生产环境。; KubeEdge: CNCF 毕业项目 (Graduated, 2024 年 10 月)，华为云主导。云边协同场景的行业标准。; MicroK8s: Canonical 官方项目，社区项目 (非 CNCF)。生产就绪，广泛用于开发、测试和轻量级生产环境。; K0s: CNCF 项目 (Sandbox/Incubation, 2024 年 CNCF 接受)，社区活跃度高。生产就绪，零摩擦 K8s 发行版。; Talos Linux (Sidero Labs 2025): Sidero Labs 商业产品+开源项目。不可变 OS 方案，2025 边缘参考架构，面向生产级边缘部署。

**生态兼容性矩阵**

CNI: Flannel (K3s 默认, K0s 默认)、Calico、Cilium、Canal、Weave Net。所有发行版均支持主流 CNI 插件。; CSI: Rook/Ceph、Longhorn、OpenEBS、Local Path Provisioner (K3s 内置)。KubeEdge 支持边缘本地存储。; 监控集成: Prometheus 指标暴露、Grafana 仪表板、Kube-state-metrics、cAdvisor、Metrics Server。K3s 内置 Metrics Server。; Ingress/Service Mesh: Traefik (K3s 默认)、NGINX Ingress、Istio、Linkerd。KubeEdge 专为云边协同优化。; Operator 支持: 标准 K8s Operator 均可运行。KubeEdge 有专用 EdgeMesh 和 Device Management Operator。

### 限制与约束

**已知限制**

K3s: 默认使用 SQLite 而非 etcd (单节点)，HA 模式需嵌入式 etcd 或外部数据库。部分高级 K8s 功能 (如特定准入控制器) 默认禁用。; KubeEdge: 云边网络依赖消息中间件 (CloudHub/EdgeHub)，断连恢复后需重新同步状态。边缘节点不支持 kubectl exec/logs 直接操作 (需通过 cloud 中转)。设备孪生更新可能存在延迟。; MicroK8s: 仅通过 snap 包分发，非 Ubuntu 系统支持有限。资源占用高于 K3s/K0s。较新版本对 ARM64 的严格安全限制 (AppArmor) 可能影响某些工作负载。; K0s: 相对较新的项目，生态工具和文档丰富度不如 K3s。Windows 工作节点功能有限。; Talos Linux (Sidero Labs 2025): 不可变 OS 设计，无法在节点上直接安装额外软件包。需通过 系统扩展 (system extensions) 添加驱动和工具。对通用 Linux 用户的操作习惯有较大差异。

**性能开销**

K3s: 控制面开销约 5-6% CPU (Intel), 25-30% CPU (Pi4B)。内存开销约 1.2-1.6 GB (服务器), 268 MB (工作节点)。; KubeEdge: EdgeCore 内存开销约 70-100 MB。云端部分开销与上游 K8s 一致。; K0s: 空载时约 658 MB RAM (单节点合设), 略低于 K3s (750 MB)。CPU 开销与 K3s 相当。; Talos Linux (Sidero Labs 2025): 不可变 OS 无额外用户态进程，OS 自身开销极低。K8s 控制面开销取决于所选发行版组件。

**固件与驱动依赖**

K3s: ARM64 需内核模块支持 vxlan (Ubuntu 默认已包含)。Raspberry Pi 需启用 cgroups (cmdline.txt 中加 cgroup_memory=1 cgroup_enable=memory)。; KubeEdge: 边缘节点需 MQTT Broker (默认 Mosquitto) 用于设备通信。ARM64 设备需内核支持 device mapper 和网络隧道。; MicroK8s: ARM64 需启用 cgroups (boot 参数)。严格限制模式 (strict confinement) 需 AppArmor 支持。; K0s: ARM64 需内核支持 overlay 文件系统和 vxlan。标准 Linux 内核配置即可。; Talos Linux (Sidero Labs 2025): 通过系统扩展 (system extensions) 提供 GPU 驱动、固件和内核模块。Raspberry Pi 需特定 DTB 和 EEPROM 固件。Jetson 需 NVIDIA 驱动扩展。

### 配置与部署

**配置方式**

K3s: 单二进制安装脚本 (install.sh)，配置文件 /etc/rancher/k3s/config.yaml。支持环境变量、CLI 标志和配置文件三种方式。通过 Agent/Server 模式区分角色。; KubeEdge: 云端: keadm 命令行工具部署 CloudCore; 边缘: keadm 部署 EdgeCore。需配置 CloudHub IP/端口、边缘节点 ID、MQTT 代理。支持 Device CRD 进行设备管理。; MicroK8s: snap 安装 (snap install microk8s --classic)。通过 microk8s 命令管理 (如 microk8s.enable dns, microk8s.enable storage)。支持 add-node 命令加入集群。; K0s: k0s CLI 工具: k0s install controller, k0s install worker, k0s start。配置文件 /etc/k0s/k0s.yaml。支持自动生成 token 加入集群。; Talos Linux (Sidero Labs 2025): 通过 Omni SaaS 或 Talos CLI 进行声明式配置。使用 Image Factory 生成定制镜像。配置格式为 YAML，通过 talosctl 应用。

**部署位置与环境**

K3s: 边缘 (Raspberry Pi、工业网关、嵌入式设备)、裸金属、虚拟机、公有云 (AWS、GCP、Azure)、混合部署。支持单节点和 HA 集群。; KubeEdge: 边缘计算 (IoT、工业互联网、车联网、智慧城市)、移动节点、RF 恶劣环境 (断连场景)。云边协同架构: CloudCore 部署在云/数据中心，EdgeCore 部署在边缘。; MicroK8s: 开发/测试环境、边缘节点、IoT 网关、Ubuntu Core 设备、工作站。适合单节点和多节点集群。; K0s: 裸金属、虚拟机、边缘、IoT、公有云、私有云。支持静态 Pod 和控制器工作节点合设。; Talos Linux (Sidero Labs 2025): 零售业、工厂自动化、机器人领域。边缘设备 (Raspberry Pi、Jetson、Ampere Altra)。Omni 管理平台提供 SaaS 式集群管理。支持单节点可调度 K8s 环境。

### 性能特征

**基准性能数据**

K3s: 服务器空闲: CPU 5% (Intel i7) / 25% (Pi4B), 内存 1215-1613 MB, 存储 10-50 IOPS, 250-500 KiB/sec, 延迟 < 10 ms。工作节点空闲: CPU 3% (Intel) / 5% (Pi4B), 内存 268-275 MB。大规模集群: 1800+ 工作节点时服务器需 16+ 核、32 GB RAM。; KubeEdge: EdgeCore 内存占用: 空闲约 42 MB RSS (5 个 Pod 时约 42,784 KB RSS)。100,000 边缘节点验证: 云端控制面 128 核/256 GB RAM, 边缘节点上行流量约 0.25 kbit/s, 负载均衡入向约 3 MB/s。Pod 启动 P99 约 4087 ms (< 5000 ms 目标)。; K0s: 空载单节点合设约 658 MB RAM, 低于 K3s (750 MB)。1 vCPU/1 GB RAM VM 中部署 MySQL 后内存升至 868 MB, API 服务器可能无响应。

**扩展性上限**

K3s: 官方测试支持 1800+ 工作节点。CPU 是主要瓶颈 (内存利用率 < 80% 时 CPU 先达到上限)。加入 50-100 节点批次时服务器 CPU 峰值约增长 20%。; KubeEdge: 已验证支持 100,000 边缘节点、1,000,000 容器 (400 个逻辑分区, 每分区 250 节点, 每节点 10 容器)。云端需 5 个管理实例。

**每节点密度**

K3s: 默认 Pod 密度: 每节点 110 Pod (默认上限)。实际密度受限于节点硬件 (ARM 边缘设备 1-4 GB RAM 通常支持 10-30 个轻量级 Pod)。; KubeEdge: 边缘节点 Pod 密度取决于硬件: 256 MB RAM 设备可运行 5-10 个容器; 1 GB RAM 设备可运行 20-50 个容器。验证测试中每节点 10 容器。

### 安全

**安全特性**

K3s: CIS Kubernetes Benchmark 加固指南 (v1.7/v1.12 自评可用); Secrets 加密 (默认 AES-CBC); 网络策略 (基于 Canal/Calico); RBAC; Pod Security Standards; 配置文件只读权限; 支持使用 etcd TLS 加密。; KubeEdge: 边缘节点设备认证 (证书轮换); 云边通道 TLS 加密 (CloudHub-EdgeHub); 节点间通信加密; 设备身份管理; 可选的节点准入控制; 断连期间本地运行不受影响。; MicroK8s: CIS 加固指南 (MicroK8s 1.28+); 严格限制模式 (strict confinement, snap 隔离); AppArmor 强制访问控制; seccomp 系统调用过滤; RBAC; 网络策略 (Calico 默认); TLS 加密。; K0s: CIS Benchmark 合规 (v1.21+); 默认安全配置; RBAC; 网络策略 (Calico/Konvoy); 控制面组件 TLS 加密; etcd 加密; 支持 FIPS 140-2 合规。; Talos Linux (Sidero Labs 2025): 不可变文件系统 (只读根分区); 无 SSH (通过 talosctl API 管理); 可信启动 (trusted boot); 最小攻击面 (无包管理器、无 Shell)。Omni 提供集中式认证和审计。

**合规与认证**

K3s: CIS Kubernetes Benchmark 自评指南 (v1.7, v1.12)。Rancher/SUSE 提供企业级加固方案。无独立 K8s 一致性认证。; KubeEdge: CNCF 毕业项目，通过 K8s 一致性认证。边缘计算场景特有合规要求 (如设备管理、数据本地化) 需用户自行评估。; MicroK8s: CIS 加固指南 (1.28+)。Canonical 提供企业支持。Ubuntu 基础有 FIPS 140-2/3 和 Common Criteria 认证可选。; K0s: CIS Benchmark 合规。支持 FIPS 140-2 加密模块。K8s 一致性认证。; Talos Linux (Sidero Labs 2025): 不可变 OS 安全模型有助于满足合规要求 (如 PCI DSS、HIPAA 的不可变基础设施要求)。无独立安全认证。

### 运维与生命周期

**可观测性支持**

K3s: 内置 Metrics Server; Prometheus 指标暴露 (通过 /metrics); 支持集成 Grafana; 日志: journald 和文件日志; 健康检查: k3s check-health; 支持 Kubernetes 事件监控。; KubeEdge: EdgeCore 暴露 Prometheus 指标; EdgeMesh 提供服务网格可观测性; 云端 CloudCore 提供 Metrics; 边缘节点日志收集支持; 设备孪生状态监控。; MicroK8s: 内置 Metrics Server (可通过 microk8s.enable 启用); Prometheus Operator 可安装; 集成 Cosmos (Canonical 可观测性栈); 健康检查: microk8s status。; K0s: 标准 K8s Metrics Server; Prometheus 兼容; 支持 k0s status 和 k0s info 命令; 日志通过 journald 管理。; Talos Linux (Sidero Labs 2025): Talos 健康检查 API (talosctl health); 系统日志通过 talosctl logs 获取; 支持 Prometheus Node Exporter; Omni 控制面提供集中监控。

**维护与生命周期**

K3s: 升级: 内置升级控制器 (system-upgrade-controller); 支持自动升级。节点排水: 标准 kubectl drain。更新回滚: 通过二进制替换。数据存储: 默认 SQLite 或嵌入式 etcd, 支持外部 etcd。; KubeEdge: 升级: keadm 工具支持 CloudCore/EdgeCore 升级; 边缘节点需逐个升级。节点排水: 边缘节点需手动处理。边缘设备断连后自动恢复同步。; MicroK8s: 升级: snap refresh microk8s (自动更新); 支持通道切换 (stable/candidate/edge)。节点排水: 标准 kubectl drain。快照回滚: snap 支持自动回滚。; K0s: 升级: k0s upgrade 命令; 支持滚动升级。节点排水: 标准 kubectl drain。配置变更: 修改 k0s.yaml 后重启服务。; Talos Linux (Sidero Labs 2025): 升级: talosctl upgrade 命令 (不可变 OS 原子升级, 升级失败自动回滚)。节点排水: 自动执行。固件升级: 通过系统扩展。Omni 提供集群级生命周期管理。

**弹性与故障恢复**

K3s: 单点故障: 单节点模式无 HA; HA 模式需 3+ 个 server 节点 + 嵌入式 etcd 或外部数据库。故障切换: 嵌入式 etcd 选举, 约 30 秒。数据持久化: 内置 Local Path Provisioner 支持本地存储。; KubeEdge: 边缘自治: 断连后工作负载继续运行, 恢复后自动同步。单点故障: CloudCore 可 HA 部署。边缘节点故障: 需手动恢复。数据持久化: 边缘节点支持本地存储。; MicroK8s: HA 模式: 3+ 节点, 内置 dqlite (分布式 SQLite)。故障切换: dqlite 选举, 约 30-60 秒。数据持久化: 支持 HostPath 和 CSI 存储。; K0s: HA 模式: 3+ 控制节点, 内置 etcd。故障切换: etcd 选举, 约 30 秒。数据持久化: 标准 K8s 存储方案。; Talos Linux (Sidero Labs 2025): 不可变 OS 自带故障恢复: 系统分区损坏自动回滚。控制面 HA: 标准 etcd 集群。Omni 提供集群级故障检测和自动修复。边缘节点: 支持单节点可调度模式。

### 经济性

**总拥有成本 (TCO)**

硬件采购成本: ARM64 边缘设备 (Raspberry Pi 4/5: $35-80, NVIDIA Jetson Nano: $150-500, 工业 ARM 网关: $200-1000)。相比 x86 服务器 (入门级 $500-2000) 大幅降低。; 运行成本: ARM64 设备功耗 5-30W (vs x86 服务器 50-200W), 年电费节省 70-90%。无需专用冷却。; 维护成本: 开源发行版免费 (K3s/KubeEdge/MicroK8s/K0s/Talos 均为开源)。企业支持: Rancher/SUSE 支持 (K3s), Canonical 支持 (MicroK8s), Sidero Labs Omni 商业订阅。; 每节点成本: ARM 边缘节点: 总拥有成本约 $50-500/节点/年 (含硬件、功耗、运维)。等效 x86 节点: $500-2000/节点/年。

**成熟度与社区支持**

K3s: GitHub Stars: 28,000+; CNCF 孵化项目; Rancher/SUSE 主导; 生产用户包括大量企业; 社区活跃度极高; 文档完善。; KubeEdge: GitHub Stars: 13,000+; CNCF 毕业项目 (2024); 华为云主导; 厂商支持: 华为、ARM、Kubernetes 社区; 边缘计算领域影响力最大。; MicroK8s: GitHub Stars: 8,500+; Canonical 主导; 与 Ubuntu 生态深度集成; 企业支持来自 Canonical; 社区活跃但规模小于 K3s。; K0s: GitHub Stars: 6,000+; CNCF 项目; 社区驱动; 2024 年 CNCF 接受; 增长迅速; 文档完善。; Talos Linux (Sidero Labs 2025): Sidero Labs 商业支持; 开源社区 (GitHub 约 7,000+ Stars); 2025 边缘参考架构发布; 在零售、工厂自动化和机器人领域有商业部署。

---

## 2. 节点自动扩缩容硬件要求（Cluster Autoscaler / Karpenter 节点自动扩缩容方案，以及 Kubernetes 1.35+ Workload-Aware Scheduling 硬件感知调度）

**官方文档**: Kubernetes 节点自动扩缩容: https://kubernetes.io/docs/concepts/cluster-administration/node-autoscaling/ ; Cluster Autoscaler 官方仓库: https://github.com/kubernetes/autoscaler/tree/master/cluster-autoscaler ; Cluster Autoscaler FAQ: https://github.com/kubernetes/autoscaler/blob/master/cluster-autoscaler/FAQ.md ; Cluster Autoscaler AWS 支持: https://github.com/kubernetes/autoscaler/blob/master/cluster-autoscaler/cloudprovider/aws/README.md ; Karpenter 官方文档: https://karpenter.sh/ ; Karpenter NodePools: https://karpenter.sh/docs/concepts/nodepools/ ; Karpenter 兼容性矩阵: https://karpenter.sh/docs/upgrading/compatibility/ ; Karpenter 实例类型参考: https://karpenter.sh/docs/reference/instance-types/ ; Karpenter FAQ: https://karpenter.sh/docs/faq/ ; Kubernetes v1.35 Workload Aware Scheduling 博客: https://kubernetes.io/blog/2025/12/29/kubernetes-v1-35-introducing-workload-aware-scheduling/ ; Kubernetes v1.36 Advancing Workload-Aware Scheduling 博客: https://kubernetes.io/blog/2026/05/13/kubernetes-v1-36-advancing-workload-aware-scheduling/ ; GKE 集群自动扩缩容: https://docs.cloud.google.com/kubernetes-engine/docs/concepts/cluster-autoscaler ; EKS AI/ML 计算与自动扩缩容最佳实践: https://docs.aws.amazon.com/eks/latest/best-practices/aiml-compute.html ; Kueue 拓扑感知调度: https://kueue.sigs.k8s.io/docs/concepts/topology_aware_scheduling/ ; Node 资源拓扑感知调度 KEP (scheduler-plugins): https://github.com/kubernetes-sigs/scheduler-plugins/blob/master/kep/119-node-resource-topology-aware-scheduling/README.md

### 硬件规格

**最低配置**

Cluster Autoscaler 控制器: CA 本身为轻量控制面组件（单 Pod 部署），对节点硬件无特定要求，只需 Kubernetes API 与云提供商 API 的网络及 IAM/RBAC 访问权限；官方 FAQ 建议在较大集群中为其请求完整 1 核 CPU 以保证调度性能 [不确定 CA 自身内存需求精确值]；需 Kubernetes v1.3.0 及以上（实践中建议与集群版本配套）。; 被管理节点: 被自动创建/删除的节点资源最常见为云虚拟机（VM），无统一 CPU/内存最低值，只要满足 kubelet 默认每节点 110 Pod 上限与系统组件资源即可；实例规格完全由节点池/NodePool 配置决定。; 实例类型兼容性: CA 要求每个节点组（如 AWS ASG）内的实例容量大致相等，跨规格混合会导致调度与缩容异常；Karpenter 支持广泛实例类型（计算/内存/通用/GPU/裸金属），推荐 C、M、R 系列第三代及以上实例，GPU 实例（g/p 系列）与裸金属（metal）受支持。

**推荐配置**

控制器部署: CA 与 Karpenter 控制器各分配约 0.5-1 核 CPU、1Gi 内存即可支撑数百节点集群 [不确定精确推荐值]；生产环境建议将控制器部署在独立 namespace 并启用资源限制与监控告警。; 节点池规划: 按硬件规格拆分节点组/NodePool：CPU 通用池（C/M/R）、内存优化池、GPU 池（g/p 系列，如 L4/A10G/L40S/H100）、spot 池与按需池分离；GPU 节点使用 taint（如 nvidia.com/gpu: NoSchedule）+ toleration 隔离，防止 CPU 工作负载占用昂贵 GPU 节点。; Karpenter 硬件约束: NodePool 通过 requirements 声明实例类别（instance-category g/p）、代数（instance-generation Gt 3）、显存（instance-gpu-memory Gt 20480）等硬件属性标签约束实例选择；通过 limits 限制 CPU/内存/GPU 总量，防止超预算创建。; 节点启动优化: 使用最新优化的节点镜像（如 EKS 优化 AMI/Bottlerocket）缩短启动时间；避免复杂 user-data 脚本；GPU 节点镜像预装驱动（Bottlerocket Accelerated AMI 同时预装驱动与设备插件）。

**生产级配置**

大规模集群: GKE 集群上限 15,000 节点；大规模场景推荐 Karpenter 类按需供给方案替代固定节点组（EKS 官方建议动态负载用 Karpenter、稳态负载用 Managed Node Groups）；多实例类型 + 多可用区部署规避单实例容量不足（ICE）；GPU 大训练用 ML Capacity Blocks/预留容量保障 H100 等稀缺容量。; GPU 生产级: GPU 节点池单独配置 min/max；用 Karpenter 多 NodePool（spot 推理池 + 按需训练池）；长训练任务关闭激进合并（consolidationPolicy: WhenEmpty、consolidateAfter: 60m）防止缩容中断；MIG/时间切片在节点内提高 GPU 利用率。; 容灾设计: 多 AZ 节点池、跨可用区冗余；GPU 故障由 EKS 节点监控代理自动修复或依赖上层任务 checkpoint 恢复；大规模 AI 集群配 EFA/InfiniBand 网络拓扑感知调度（Kueue TAS/rack 级放置）。

### 兼容性

**支持的 K8s 版本范围**

Cluster Autoscaler: 要求 Kubernetes v1.3.0 及以上（CA 官方最低门槛）；实际生产建议 CA 版本与集群 K8s 版本同步（如 EKS 文档要求 CA 版本与 EKS 版本匹配），CA 1.12 起默认优先级阈值策略变化，新特性随版本演进 [不确定完整版本矩阵]。; Karpenter: 严格兼容矩阵：K8s 1.30 需 Karpenter >= 0.37；1.31 需 >= 1.0.5；1.32 需 >= 1.2；1.33 需 >= 1.5；1.34 需 >= 1.6；1.35 需 >= 1.9；1.36 需 >= 1.13。; Workload-Aware Scheduling: v1.35 引入（Alpha，默认关闭）：Workload API（scheduling.k8s.io/v1alpha1）、Gang Scheduling、Opportunistic Batching；v1.36 演进为 scheduling.k8s.io/v1alpha2，新增 PodGroup API、拓扑感知调度与工作负载感知抢占初版、DRA ResourceClaim 支持及 WorkloadWithJob feature gate（Job 控制器自动生成 workload 对象）。; 云托管集成: GKE/AKS/EKS 内建或托管 CA 随平台版本升级；EKS 上 DRA（Dynamic Resource Allocation）要求 EKS 1.33+ 且仅支持 Managed Node Groups，Karpenter 尚不支持 DRA。

**操作系统兼容性**

Cluster Autoscaler: 控制器运行在 Linux 控制面节点（容器镜像）；被管理节点支持各云镜像 OS（Ubuntu、AL2/AL2023、Bottlerocket、RHEL、Windows 节点池等）[不确定 Windows 节点池支持细节]。; Karpenter: 依赖云提供商 AMI/镜像与 NodeClass 配置；AWS 支持 AL2023、AL2、Bottlerocket、Ubuntu 等 [不确定完整镜像矩阵]；GPU 加速 AMI（Bottlerocket Accelerated / AL2023 Accelerated）预装 NVIDIA 驱动，Bottlerocket 额外预装设备插件，AL2023 需另行部署设备插件。; 裸金属: Karpenter 支持 metal 实例类型；节点 OS 由 NodeClass 自定义镜像决定。

**K8s 上游支持阶段**

Cluster Autoscaler: Kubernetes 官方子项目（kubernetes/autoscaler），生产就绪 GA 级，事实上的标准节点扩缩容组件。; Workload-Aware Scheduling: Kubernetes 上游 Alpha（v1.35 引入，v1.36 迭代至 v1alpha2），尚未 GA，官方明确不建议生产使用；目标是让多 Pod 工作负载成为 kube-scheduler 的一等公民并最终扩展到抢占与自动扩缩容。; 拓扑感知调度 (TAS): Kueue TAS 为 Kueue 项目（CNCF）功能；Node 资源拓扑感知调度为 scheduler-plugins 社区项目（KEP-119），均非 K8s 核心 GA。

**生态兼容性矩阵**

云提供商: CA 支持 AWS/GCP/Azure/阿里云/华为云等 40+ 提供商及 Cluster API；Karpenter 官方支持 AWS（karpenter-provider-aws），Azure 与 GCP 提供商已启动 [不确定各自成熟度]。; 调度与队列: kube-scheduler 原生调度（NodeResourcesFit 等插件）；Kueue（含 Provisioning admission check 与 CA 联动预留配额）；Volcano、Scheduler-plugins（NodeResourceTopologyMatch 硬件拓扑感知）等。; 工作负载伸缩: HPA/KEDA 应用级伸缩与 CA/Karpenter 节点级伸缩配合；Knative/OpenFaaS 等无服务器框架可驱动 GPU 节点 scale-to-zero。; 监控集成: CA 暴露 Prometheus 指标与日志；Karpenter 暴露 Prometheus 指标与事件；GKE 提供节点启动延迟监控指标；EKS 节点监控代理（自动修复）[不确定完整指标清单]。; GPU 生态: NVIDIA 设备插件/DRA、NVIDIA GPU Operator、DCGM exporter 与 GPU 节点池结合；Karpenter 通过 instance-gpu-name/instance-gpu-memory 等标签精确选择 GPU 硬件。

### 限制与约束

**已知限制**

决策基于请求而非实际用量: CA 与 Karpenter 的缩容/合并决策只考虑 Pod 资源请求（requests），不考虑真实资源使用，可能导致过度供应或缩容不精确。; CA 单节点组扩容: CA 一次扩容动作只向单个节点组扩容（每个周期选择一个节点组），多规格弹性依赖预配置的多个节点组；节点组内实例规格需近似相等。; GPU 节点标签问题: GPU 节点若未在加入集群前打上 GPU 资源标签，CA 会因未发现资源而重复扩容、浪费资源（AWS 文档明确要求预先打标签）。; Karpenter 与 DRA: EKS 上 Karpenter 当前不支持 DRA，DRA 仅可用于 Managed Node Groups（EKS 1.33+）。; 云容量与配额: 扩容可能因实例容量不足（ICE）、配额限制、配置不兼容而失败；GKE 失败后进入 5 分钟起、上限 30 分钟的 backoff。; 节点启动延迟: 端到端扩容耗时受实例启动（30-90 秒）、kubelet/CNI 注册（20-60 秒）、镜像拉取与 Pod 启动影响，通常 2-6 分钟，无法由自动扩缩容本身消除。; GPU 冷启动: GPU 节点完整冷启动 3-8 分钟（节点就绪 60-120 秒 + 镜像拉取 30-60 秒 + 模型权重下载 60-180 秒 + 权重加载至显存 10-60 秒 + CUDA context 初始化 5-30 秒），70B 级大模型完全冷启动可达 7-9 分钟，严重影响弹性响应。; 缩容保守策略: CA 默认两次缩容间隔 10 分钟；GKE 缩容优雅终止期最长 1 小时；Karpenter 合并需显式配置策略（WhenEmpty/WhenEmptyOrUnderutilized）。; GKE 特有限制: Standard 集群不会缩容到 0 节点（需至少 1 节点运行系统 Pod）；不支持 Local PersistentVolume 的自动扩缩容；不支持严格 DoNotSchedule 拓扑分布约束；不支持自定义调度器的部分场景。

**固件与驱动依赖**

GPU 节点: 依赖 NVIDIA 驱动与设备插件；EKS 加速 AMI 预装驱动（Bottlerocket 同时预装设备插件，AL2023 需手动部署）；GPU 节点须在加入前打标签；Karpenter 通过 AMI/NodeClass 管理镜像，新实例规格需对应驱动版本支持 [不确定驱动版本矩阵]。; 中断处理: Karpenter 依赖 SQS 队列接收 AWS 实例中断/Spot 通知（约 2 分钟提前量）实现优雅排水。

### 配置与部署

**配置方式**

Cluster Autoscaler: Helm/kubectl 部署控制器；云提供商凭据使用 IRSA（AWS）或 Workload Identity（GCP/Azure）；通过节点组标签自动发现（如 k8s.io/cluster-autoscaler/enabled）；以 flags 配置行为（--expander、--scale-down-delay-after-add、--balance-similar-node-groups、--max-nodes-total 等）。; Karpenter: Helm 部署控制器，声明式 CRD 配置：NodePool（调度约束、需求、limits、中断策略）+ NodeClass（AWS 为 EC2NodeClass：AMI、子网、安全组、实例配置）；AWS 需预建 SQS 队列与 IRSA 角色。; Workload-Aware Scheduling: kube-scheduler 内置功能，需在 v1.35+ 开启对应 feature gate（Alpha）；通过 Workload/PodGroup 对象描述多 Pod 工作负载的调度需求（v1.36 可启用 WorkloadWithJob 让 Job 控制器自动生成）；无需部署额外组件。; GPU 硬件感知: Karpenter/CA 通过节点标签与资源（nvidia.com/gpu、karpenter.k8s.aws/instance-gpu-*）实现硬件感知；更细粒度硬件属性选择走 DRA（DeviceClass/ResourceSlice，EKS 1.33+ Managed Node Groups）或 Kueue 拓扑资源配额。

### 性能特征

**基准性能数据**

CA 触发延迟: 官方 SLO：小集群从 Pod 不可调度到触发云请求不超过 30 秒（平均约 5 秒），大集群不超过 60 秒（平均约 15 秒）；实例供给时间不取决于 CA 而取决于云提供商。; Karpenter 供给速度: AWS 实测节点供给至少约 55 秒、常见 45-50 秒，优化后可达约 31 秒；用户体感 2-4 分钟通常包含 Pod 启动时间；影响因素：AMI、user-data、CNI、实例类型与区域。; 端到端扩容时间: 节点启动（实例启动 30-90 秒 + kubelet 注册 20-60 秒）通常共 1-3 分钟，端到端（不可调度到 Pod 就绪）常见 2-6 分钟；GKE 节点启动约 60-90 秒 [不确定具体数值]。; GPU 冷启动分解: 节点供给 60-120 秒、镜像拉取 30-60 秒、模型权重下载 60-180 秒、RAM 到显存传输 10-60 秒、CUDA context 5-30 秒，合计 3-8 分钟；70B 级模型完全冷启动（未缓存镜像）7-9 分钟。; 失败回退: GKE 扩容失败 backoff 从 5 分钟起、上限 30 分钟。

### 安全

**安全特性**

最小权限模型: CA/Karpenter 控制器使用 IRSA/Workload Identity 获取最小云 API 权限（AWS 建议限定 ASG ARN 或标签条件）；Karpenter 需独立 IAM 角色与 SQS 队列权限。; 工作负载隔离: GPU/专用硬件节点通过 taint/toleration 隔离；节点池间网络策略与命名空间配额配合；昂贵硬件（GPU）节点建议禁止非容忍工作负载调度。

### 运维与生命周期

**可观测性支持**

CA 指标: 暴露 Prometheus 指标（集群/节点组状态、不可调度 Pod 计数、扩容/缩容事件）与结构化日志；可对接 Grafana [不确定完整指标名列表]。; 平台级监控: GKE 提供节点启动延迟指标（启动耗时拆分监控）；EKS 节点监控代理（健康检查、自动修复、GPU/EFA 故障检测）；HPA/KEDA 指标与节点级指标联动。

**维护与生命周期**

升级流程: CA 随 autoscaler 仓库独立发版，升级需匹配集群 K8s 版本；Karpenter 升级必须遵循兼容矩阵（如 K8s 1.35 需 Karpenter >= 1.9）；Workload-Aware Scheduling 随 K8s 版本演进（v1.35 v1alpha1 -> v1.36 v1alpha2，API 有破坏性变更）。; 节点排水与缩容: 缩容前自动 drain 节点并尊重 PodDisruptionBudget；CA 默认缩容间隔 10 分钟；Karpenter 通过 disruption 策略控制（WhenEmpty/WhenEmptyOrUnderutilized/Balanced、consolidateAfter 时长）；GKE 缩容优雅终止期最长 1 小时。

**弹性与故障恢复**

容量保障: 多实例类型 + 多 AZ 降低 ICE 概率；GPU 训练可用 ML Capacity Blocks/预留容量；跨可用区节点池冗余。; 故障恢复: 节点级故障由自动修复机制替换节点（EKS 节点监控代理）；训练任务依赖 checkpoint（保存权重到 S3 定期恢复）；Kueue TAS 支持拓扑域 hot swap 在节点故障时寻找替换节点；GPU 冷启动 3-8 分钟决定恢复时间窗。

### 经济性

**总拥有成本 (TCO)**

效率优化: consolidation 将低利用率节点合并到更小/更便宜实例；MIG/时间切片提高单卡利用率摊薄成本；GPU 冷启动 3-8 分钟意味着峰值扩容需提前预留缓冲（minReplicas/常驻副本），存在闲置成本与响应速度的权衡 [不确定量化数据]。; 容量预留: ML Capacity Blocks/HyperPod 预留容量成本高于按需 spot，但保障稀缺 GPU 可用性。

**成熟度与社区支持**

Cluster Autoscaler: 最成熟：Kubernetes 官方子项目，多年生产验证，云厂商全支持（EKS/GKE/AKS 均有托管或官方支持），社区活跃。; Workload-Aware Scheduling: 上游 K8s Alpha（v1.35/1.36），跨 SIG 的大工程，2026 年持续演进；尚未 GA，生产采用需谨慎，社区关注度高（AI/ML 大规模调度场景）。; 拓扑感知/硬件感知调度: Kueue TAS 与 scheduler-plugins 为社区项目，生态中等；云厂商（AWS/GCP）在节点级硬件选择与容量保障上提供托管方案。

---

## 3. CPU 指令集架构 (ISA) - amd64/x86_64、arm64、s390x、ppc64le

### 硬件规格

**最低配置**

amd64/x86_64: 1 核 CPU、1 GB 内存、10 GB 磁盘空间，可运行最小化 Kubernetes 集群（kubeadm 单节点）; arm64: 1 核 CPU、1 GB 内存、10 GB 磁盘空间，可运行最小化 Kubernetes 集群（树莓派类设备）; s390x: IBM Z 或 LinuxONE 环境，最低 1 个 IFL（集成固件处理器）、4 GB 内存、50 GB 磁盘空间; ppc64le: IBM Power 服务器，最低 1 核 CPU、2 GB 内存、20 GB 磁盘空间

**推荐配置**

amd64/x86_64: 4 核 CPU、8 GB 内存、100 GB SSD 磁盘，适用于生产环境控制面节点; arm64: 4 核 CPU、8 GB 内存、100 GB SSD 磁盘，AWS Graviton / Ampere Altra 实例（如 m7g.medium）; s390x: 2-4 IFL、16 GB 内存、100 GB 磁盘，搭配 z/VM 或 KVM 虚拟化环境; ppc64le: 4 核 CPU、16 GB 内存、100 GB 磁盘，IBM Power9/10 服务器

**生产级配置**

amd64/x86_64: 8 核以上 CPU、32 GB 以上内存、NVMe SSD 磁盘，HA 控制面至少 3 节点; arm64: 8 核以上 CPU、32 GB 以上内存、NVMe SSD 磁盘，AWS Graviton3/4 实例（如 c7g.2xlarge 以上）; s390x: 4-8 IFL、64 GB 以上内存、企业级存储，HA 控制面配置，IBM Z 或 LinuxONE 企业环境; ppc64le: 8 核以上 CPU、64 GB 以上内存、企业级存储，IBM Power 服务器集群

### 兼容性

**支持的 K8s 版本范围**

从 Kubernetes 1.0 起即支持 amd64；arm64 自 K8s 1.3 开始获得官方支持；ppc64le 和 s390x 自 K8s 1.5-1.6 开始加入 CI 构建管道。当前所有受支持架构（amd64、arm64、arm、ppc64le、s390x）均可在最新稳定版 Kubernetes（v1.33+）中使用，具体支持程度取决于各架构的 CI 测试覆盖级别

**操作系统兼容性**

amd64/x86_64: 所有主流 Linux 发行版（Ubuntu、Debian、RHEL、CentOS、Rocky Linux、AlmaLinux、SUSE Linux Enterprise Server、Fedora CoreOS、Flatcar Container Linux）、Windows Server 2019/2022/2025、macOS（开发环境）; arm64: Ubuntu、Debian、RHEL、Fedora、Rocky Linux、AlmaLinux、SUSE Linux Enterprise Server、Flatcar Container Linux、Amazon Linux 2/2023（ARM 版本）。不支持 Windows Server 原生 arm64; s390x: RHEL for IBM Z and LinuxONE、SUSE Linux Enterprise Server for IBM Z、Ubuntu for IBM Z。支持有限，发行版选择较少; ppc64le: RHEL for Power (little endian)、Ubuntu for Power、SUSE Linux Enterprise Server for Power。支持有限，发行版选择较少

**K8s 上游支持阶段**

amd64/x86_64: GA（生产就绪），release-blocking CI 测试覆盖，是 Kubernetes 最成熟、测试最充分的架构; arm64: GA（生产就绪），release-blocking CI 测试覆盖，AWS Graviton、Ampere、华为鲲鹏等主流云厂商均提供 arm64 实例; arm: Beta/社区维护，非 release-blocking CI 测试，主要适用于边缘/IoT 场景; ppc64le: Beta/社区维护，release-informing CI 测试，IBM Power 生态系统支持，OpenShift 提供完整支持; s390x: Beta/社区维护，release-informing CI 测试，IBM Z 和 LinuxONE 生态系统支持，OpenShift 提供完整支持

### 限制与约束

**已知限制**

amd64/x86_64: x86 架构专利壁垒，生态封闭；功耗相对较高；部分遗留应用可能依赖特定 x86 指令集扩展（AVX、AVX-512、AMX）; arm64: 部分 x86 原生二进制/容器镜像无法直接运行，需重新编译或使用模拟层（如 QEMU 用户态模拟）；高性能计算中 AVX-512 等 SIMD 指令集不可用；某些专有软件（如 Oracle 数据库）对 arm64 支持有限; s390x: 生态最小，第三方软件和容器镜像支持极度有限；开发测试环境获取困难；高昂的硬件成本限制大规模部署；无法使用主流云服务商的基础设施即服务; ppc64le: 生态较小，第三方软件和容器镜像支持有限；硬件成本较高；云实例选择有限（仅 IBM Cloud 提供）; 通用: 多架构集群中，kubelet 自动通过 kubernetes.io/arch 标签标记节点架构，但调度器本身不自动阻止跨架构 Pod 调度（需用户配置 nodeSelector/nodeAffinity）

**混部兼容性**

多架构（混合 x86_64 + arm64）集群中，需要确保容器镜像架构与节点架构匹配。常见策略包括：(1) 使用多架构镜像（manifest list / OCI image index），由容器运行时（containerd）自动选择正确架构层；(2) 使用 nodeSelector 或 nodeAffinity 约束 Pod 调度到匹配架构的节点；(3) 使用 QEMU 用户态模拟（binfmt_misc）在非原生架构上运行异架构容器，但性能有显著损耗。混部时需注意 kubelet 和节点组件必须为原生架构编译

### 配置与部署

**配置方式**

Kubernetes 原生使用 kubernetes.io/arch 标签自动标记节点架构，通过 nodeSelector 或 nodeAffinity 控制 Pod 调度到指定架构节点。无需额外 Device Plugin 或 Operator。多架构镜像构建通过 docker buildx（基于 QEMU 模拟或原生构建器）或手动 manifest 操作实现

**配置示例**

nodeSelector 方式: apiVersion: v1
kind: Pod
metadata:
  name: multi-arch-pod
spec:
  nodeSelector:
    kubernetes.io/arch: arm64
  containers:
  - name: app
    image: registry.k8s.io/pause:3.10; nodeAffinity 方式: apiVersion: v1
kind: Deployment
metadata:
  name: multi-arch-deploy
spec:
  replicas: 3
  template:
    spec:
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: kubernetes.io/arch
                operator: In
                values:
                - amd64
                - arm64
      containers:
      - name: app
        image: myregistry/myapp:latest; docker buildx 构建多架构镜像: # 创建构建器并构建多架构镜像
docker buildx create --name multiarch --driver docker-container --use
docker buildx build --platform linux/amd64,linux/arm64,linux/ppc64le,linux/s390x -t myregistry/myapp:latest --push .; 手动创建 manifest list: docker manifest create myregistry/myapp:latest \
  myregistry/myapp:amd64 \
  myregistry/myapp:arm64
docker manifest push myregistry/myapp:latest

**部署位置与环境**

amd64/x86_64: 所有主流云服务商（AWS、Azure、GCP、阿里云、华为云）、裸金属服务器、虚拟机、边缘设备、混合云环境。覆盖最广泛的部署场景; arm64: AWS（Graviton）、GCP（Tau T2A）、Azure（Ampere Altra）、阿里云（倚天 710）、华为云（鲲鹏）、Oracle Cloud（Ampere）、裸金属服务器（如 Ampere Altra）、树莓派/Rockchip 等边缘设备。混合部署可通过多架构节点池实现; s390x: IBM Cloud、裸金属 IBM Z 和 LinuxONE 服务器、z/VM 虚拟化环境。主要面向金融、保险、大型企业核心交易系统; ppc64le: IBM Cloud、裸金属 IBM Power 服务器、PowerVM 虚拟化环境。主要面向企业传统工作负载迁移

### 性能特征

**基准性能数据**

amd64/x86_64: 单核性能领先，AVX-512/AMX 等 SIMD 指令集提供卓越的向量化计算能力。典型 SPEC CPU 2017 整数评分约 10-15/核（取决于具体型号）。内存延迟约 60-100ns（NUMA 架构）。Kubernetes 控制面性能基准：etcd 磁盘 IOPS 建议 10,000+，API Server 建议 2-4 核 CPU; arm64: 每瓦性能优势明显，AWS Graviton3 相较于同代 x86 实例提供高达 40% 的性价比提升。Graviton3 提供 30% 更好的计算性能、50% 更多核心、2x 内存带宽。ARM 容量在 Kubernetes 中以 3.5 倍于 x86 的速度增长，目前约占 Kubernetes 总 CPU 容量的 9%。SPEC CPU 2017 整数评分约 8-12/核; s390x: 企业级事务处理性能卓越，专为高可靠性、高可用性设计。单 IFL 可支撑大量并发容器。在 IBM Z 上运行 K8s 主要面向核心交易系统而非吞吐量敏感型批处理。具体基准数据需参考 IBM Z 性能手册; ppc64le: IBM Power 架构提供高吞吐量并行计算能力，适合大规模数据分析。具体基准数据需参考 IBM Power 性能指标

### 安全

**安全特性**

amd64/x86_64: 支持 TPM 2.0（可信平台模块）、UEFI Secure Boot、Intel TDX（机密计算）、AMD SEV-SNP（机密计算）、Intel SGX（飞地执行）、硬件信任根。Kubernetes 中可通过 Confidential Containers (CoCo) 项目使用 TDX/SEV-SNP; arm64: 支持 TPM 2.0（部分服务器级 SoC）、UEFI Secure Boot（ARM 版本）、ARM CCA（机密计算架构，较新硬件）、TrustZone（TEE）。ARM CCA 在 Kubernetes 中的集成仍在发展中; s390x: IBM Z 提供业界领先的硬件安全特性：Secure Execution（类似机密计算）、Crypto Express（硬件加密加速）、Secure Boot、逻辑分区（LPAR）隔离。IBM Z 是最高安全等级的硬件平台之一; ppc64le: 支持 TPM 2.0、Secure Boot、PowerVM 逻辑分区隔离。IBM Power 架构提供硬件加密加速。机密计算支持相对有限

### 运维与生命周期

### 经济性

**总拥有成本 (TCO)**

amd64/x86_64: 硬件采购成本相对较低（x86 服务器市场成熟、竞争充分），但功耗较高。每核授权费用可能因软件许可模式而异。在 Kubernetes 场景中，按需实例价格约为 arm64 的 1.2 倍; arm64: 硬件成本优势明显，AWS Graviton 按需实例每小时成本比同规格 x86 实例低约 19%。Spot 实例折扣可高达 90%。每瓦性能更高，可降低数据中心电力和冷却成本。部分软件许可（如按核付费）可进一步降低成本; s390x: 初始硬件采购成本极高（IBM Z 大型机），但单系统整合能力极强，可替代大量 x86 服务器。在大型企业核心交易场景中，综合 TCO 可能具有竞争力。IBM 提供按需容量（Tailored Fit Pricing）模式; ppc64le: 硬件成本高于 x86 但低于 IBM Z。IBM Power 服务器适合工作负载整合。在特定企业场景中具有竞争力

**成熟度与社区支持**

amd64/x86_64: 最成熟、社区支持最广泛的架构。Kubernetes 上游 CI 的主力测试架构。所有主流云服务商、操作系统、第三方工具、Operator 均优先支持 amd64。社区活跃度最高; arm64: 成熟度快速提升，已进入生产就绪阶段。AWS、GCP、Azure、阿里云、华为云均提供 arm64 K8s 节点。ARM 服务器在 Kubernetes 中的采用率快速增长（约占总容量的 9%）。社区支持日益完善，但仍有部分第三方工具仅支持 amd64; s390x: 生态较小，社区活跃度有限。主要由 IBM 和 Red Hat 推动（OpenShift on IBM Z），社区贡献者较少。Kubernetes 上游 s390x CI 为非 release-blocking 级别; ppc64le: 生态较小，社区活跃度有限。主要由 IBM 推动（OpenShift on Power），社区贡献者较少。Kubernetes 上游 ppc64le CI 为非 release-blocking 级别

---

## 4. CXL 内存池化与分层（Compute Express Link Memory Pooling & Tiering）

### 硬件规格

**最低配置**

平台: 支持 CXL 的 CPU：Intel 第四代至强 Sapphire Rapids 及以上（CXL 1.1）、AMD 第四代 EPYC Genoa 及以上（CXL 1.1）、NVIDIA Grace 等; 内存模块: 至少 1 个 CXL 内存扩展模块（如三星 CMM-D 128GB，EDSFF E3.S 2T 或 AIC 形态）; 链路: PCIe Gen5 x16 CXL 端口; 软件: Linux 内核 5.12+（cxl 驱动），推荐 6.0+（内存热插拔成熟）; 说明: 功能验证级最小配置

**推荐配置**

服务器: 双路服务器，每路 1-2 个 CXL 端口（PCIe Gen5 x16）; 内存模块: 2-4 个 CXL 内存模块（单模块 128-256GB，总计 256GB-1TB）; 软件: Linux 内核 6.x，cxl-cli/daxctl 工具链; 固件: BIOS/UEFI 启用 CXL 并配置 Memory Mode（1LM+Vol 软件分层模式）; 说明: 生产环境起步配置，适用于内存扩展与单节点分层场景

**生产级配置**

内存池化控制器: Astera Labs Leo P 系列 CXL 内存控制器 SoC（DDR5-5600，Gen5 x16，双内存通道）; CXL 交换机: Marvell Structera 系列（Structera S 30260：260 通道 CXL 3.0 交换机，机架级内存池，亚微秒访问，客户送样 Q3 2026）; 池化容量: 机架级数百 GB 至数 TB，集群级目标最高 100 TiB; 说明: 大规模/高可用场景当前仍处于送样与预生产阶段，量产生态尚未成熟

### 兼容性

**支持的 K8s 版本范围**

Kubernetes 无原生 CXL 支持；扩展资源（Extended Resource）自 v1.10 起可将 CXL 内存暴露为可调度资源；DRA（Dynamic Resource Allocation）自 v1.26 Alpha 引入，结构化参数于 v1.32 GA（resource.k8s.io/v1beta1），可用于动态分配 CXL 池化内存；Topology Manager（v1.27 GA）提供 NUMA 拓扑感知调度；实际可用性取决于 Linux 内核版本（5.12+ CXL 驱动、6.0+ 热插拔）

**操作系统兼容性**

Linux 为主：Ubuntu 24.04 LTS（HWE 内核）、RHEL 9.4/9.5、SLES 15.6（存在已知问题）等；内核需 5.12+（CXL 驱动），6.0+ 内存热插拔，6.15+ 修复 cgroup 分层限制；Windows Server 暂无 CXL 支持

**K8s 上游支持阶段**

社区/研究阶段（无上游 GA 功能）：CXL 内存需通过 Device Plugin 扩展资源或 DRA 暴露；Linux 内核内存分层（demotion/promotion）处于持续演进阶段；K8s 官方无 CXL 专用 KEP，相关研究包括 CXLAimPod、XpuPod、Tiresias 等

**生态兼容性矩阵**

Linux 工具链：cxl-cli（cxl list/create-region）、ndctl/daxctl（mode=system-ram/devdax）、numactl（NUMA 节点视图）；监控：Prometheus 搭配 node_exporter 的 NUMA 内存指标；CNCF 生态：Koordinator v1.6+ 支持异构资源调度；硬件生态：三星 CMM-D、SK 海力士 CMM-DDR5、Marvell Structera 交换机、Astera Labs Leo 控制器、Micron CZ120；软件栈：MemVerge 等 CXL 内存管理中间件

### 限制与约束

**已知限制**

- CXL 内存延迟约为本地 DRAM 的 2 倍（实测约 214-394ns vs 本地 81-114ns；跨插槽访问最高约 621ns）
- CXL 内存带宽远低于本地（实测约 18-52 GB/s vs 本地 52-246 GB/s）
- 流式/内存密集型负载性能下降可超过 50%，最坏情况执行时间增加 5.3 倍；实测约 1/5 的负载降幅超过 50%
- 硬件生态处于早期市场阶段：三星计划 2026 年底量产 CXL 3.2 CMM-D，SK 海力士未公布量产时间表
- K8s 无原生 CXL 内存支持，需自研 Device Plugin/DRA 驱动与调度策略
- 部分发行版存在已知问题：SLES 15.6 无法运行 CXL 分层配置、RHEL 9.4 CXL CLI 工具异常、Ubuntu 24.04/RHEL 9.5 误报 CXL 相关消息、cxl list 与 numactl 的 NUMA 节点报告不一致
- 内存池化扩大故障域：CXL 交换机或池化设备故障可能影响多个主机
- 内核分层机制在 v6.15 前存在 cgroup demotion 限制

**固件与驱动依赖**

BIOS/UEFI 需启用 CXL 端口并设置 Memory Mode（1LM+Vol 软件分层或 1LM 纯扩展）；固件需提供 CDAT/HMAT 表（内存性能坐标）；内核依赖：cxl 驱动（5.12+）、内存热插拔（6.0+）、cgroup demotion 修复（6.15+）；工具链 cxl-cli/daxctl/ndctl；PCIe Gen5 链路与 EDSFF E3.S/AIC 形态模块

### 配置与部署

**配置方式**

- BIOS/UEFI：启用 CXL 端口，Memory Mode 设为 1LM+Vol（软件分层）或 1LM（纯容量扩展）
- Linux 内核：激活 CXL region 并配置内存模式（system-ram 或 devdax）
- 命令行工具：cxl create-region、daxctl reconfigure-device --mode=system-ram
- K8s Device Plugin：将 CXL 内存暴露为扩展资源（如 cxl.io/memory）
- K8s DRA：通过 ResourceClass/ResourceClaim 动态分配池化内存（结构化参数 v1.32 GA）
- 调度策略：节点亲和/反亲和、污点与容忍、Topology Manager NUMA 对齐

**配置示例**

CXL 设备配置为系统内存: cxl create-region -d decoder0.0 -w 1 -m system-ram -s 128G
daxctl reconfigure-device --mode=system-ram dax0.0; Device Plugin 暴露扩展资源: apiVersion: v1
kind: Node
metadata:
  name: node-with-cxl
status:
  capacity:
    cxl.io/memory: 128Gi; DRA ResourceClaim（结构化参数）: apiVersion: resource.k8s.io/v1beta1
kind: ResourceClaim
metadata:
  name: cxl-memory-claim
spec:
  devices:
    requests:
    - deviceClassName: cxl-memory
      selectors:
      - cel:
          matchExpressions:
          - key: capacity
            operator: In
            values: ["256Gi"]; Topology Manager 配置: --topology-manager-policy=single-numa-node
--topology-manager-scope=pod

### 性能特征

**基准性能数据**

实测（arXiv 2409.14317）：本地 DRAM 延迟 81-114ns、带宽 52-246 GB/s；CXL 扩展器延迟约 214-394ns、带宽 18-52 GB/s；跨插槽 CXL 访问延迟最高约 621ns。共享内存池相对 200G RDMA 有 3.8 倍加速、相对 100G RDMA 有 6.5 倍加速（CXL 联盟）；语义化访问延迟 200-500ns。分层优化后热页远程流量可降至 5% 以下，LLM 训练性能损失小于 3%（Emergent Mind 综述）；向量检索 6.72 倍、专家混合路由 8.7 倍吞吐提升（精度损失仅 0.13%）。CXLAimPod（eBPF 调度框架）：LLM 文本生成提升 71.6%、向量数据库查询提升 9.1%、KV 存储平均提升 7.4%（顺序场景最高 150%）。数据库类负载（Redis/MySQL 类）在纯 CXL 内存上受延迟约束明显，流式负载最坏性能下降 5.3 倍

### 安全

### 运维与生命周期

### 经济性

**成熟度与社区支持**

生态处于早期市场阶段（三星官方称硬件生态尚不成熟）：三星 CMM-D（128/256GB，计划 2026 年底量产 CXL 3.2）、SK 海力士 CMM-DDR5（第二代原型，无量产时间表）、Marvell Structera 交换机（Structera S 30260 送样 Q3 2026）、Astera Labs Leo P 系列（已有数据中心实际部署）、Intel/AMD 平台支持（Sapphire Rapids/Genoa 起 CXL 1.1，Granite Rapids/Turin 支持 CXL 2.0）；Linux 内核社区活跃（cxl 驱动、内存分层 demotion/promotion）；K8s 侧无官方支持但学术与开源研究活跃（CXLAimPod、Tiresias、Beluga、OSDI'24）

---

## 5. Kubernetes 控制平面硬件（etcd、kube-apiserver、HA 控制面节点、NUMA 拓扑）

### 硬件规格

**最低配置**

控制面节点（kubeadm 官方）: 每台机器 2 GB 以上内存、控制面机器 2 核以上 CPU、全网络连通性与开放端口；仅可运行最小化测试集群，不满足生产需求; etcd（官方文档）: 2-4 核 CPU、8 GB 内存、50 顺序 IOPS 磁盘（推荐 SSD，若用机械盘需 15,000 RPM）、1GbE 网络；磁盘写延迟是最关键指标; API Server: 最低 1-2 核 CPU、1-2 GB 内存即可在测试环境启动，生产集群需远高于此

**推荐配置**

控制面节点（生产）: 4-8 vCPU、16-32 GB 内存、120-250 GB 本地 SSD/NVMe 磁盘；etcd 所在盘要求 SSD 且 10,000+ IOPS，NVMe 可达 50,000+ IOPS 为最佳; etcd（高负载）: 8-16 核专用 CPU、16-64 GB 内存、500 顺序 IOPS、10GbE 网络；官方要求 etcd 使用完全专用机器以避免资源争用; HA 拓扑: 3 台（奇数）控制面机器：堆叠 etcd 拓扑（控制面与 etcd 同机）或外部 etcd 拓扑（另加 3 台以上专用 etcd 机器）；TCP 负载均衡器以 6443 端口健康检查作为 API Server 入口; 托管服务对照: EKS/AKS/GKE/ACK 等托管控制面由云厂商承载，规格不可见，通常等效于 2-4 vCPU、8-16 GB 的 API Server 实例

**生产级配置**

大规模集群（数百至 5,000 节点）: 控制面节点 8-16 vCPU、32-64 GB 内存、500 GB NVMe；etcd 使用独立本地 NVMe 磁盘（与系统盘/日志盘分离）；事件对象写入独立 etcd 集群（--etcd-servers-overrides）；10GbE 控制面网络；3 或 5 节点奇数控制面，每故障域 1-2 个实例；API Server 内存需 16-32 GB+ 以承载 watch cache; 超大规模（超过 5,000 节点）: 官方单集群上限为 5,000 节点，超过需多集群拆分或采用云厂商定制存储后端（实测 30,000-130,000 节点方案均替换了 etcd 存储引擎）；社区经验 500-2,000 节点/集群为'甜蜜点'

### 兼容性

**支持的 K8s 版本范围**

etcd: Kubernetes 1.22+ 默认使用 etcd 3.5.x（kubeadm 在 v1.32-v1.34 支持 etcd v3.5.24），K8s 1.35 升级至 etcd 3.6.6；etcd 硬件建议（NVMe/SSD、IOPS、延迟）适用于所有受支持版本; Topology Manager / NUMA: K8s 1.16 引入（Alpha）、1.18 Beta、1.27 GA（TopologyManager feature gate 默认启用）；max-allowable-numa-nodes 策略选项（突破 8-NUMA 限制）1.31 随 TopologyManagerPolicyBetaOptions Beta 默认启用、1.35 GA、1.36 移除 feature gate; HA 与大规模集群: kubeadm HA 控制面为 GA 功能；5,000 节点 / 150,000 Pod 扩展目标自 K8s 1.6 确立并沿用至今

**操作系统兼容性**

控制面硬件与操作系统解耦：所有 K8s 支持的 Linux 发行版（Ubuntu、Debian、RHEL/CentOS/Rocky/AlmaLinux、SUSE、Fedora CoreOS、Flatcar Container Linux 等）均可承载控制面组件；etcd 官方支持 Linux、macOS、BSD、Windows，生产环境仅推荐 Linux；Windows Server 只能作为工作节点，不能作为控制面节点

**K8s 上游支持阶段**

控制面组件（kube-apiserver/kube-controller-manager/kube-scheduler/etcd）: GA（生产就绪）；etcd 为 CNCF 毕业项目; max-allowable-numa-nodes（突破 8-NUMA 限制）: K8s 1.35 起 GA; Topology Manager: K8s 1.27 起 GA; Streaming List（缓解 API Server 大集群内存压力）: K8s 1.33 Beta 默认启用，1.35 继续演进; kubeadm HA 控制面: GA，多发行版（Talos、RKE2、kOps、KubeSpray）均提供类似能力

### 限制与约束

**已知限制**

etcd: 默认存储配额 2 GiB、推荐上限 8 GiB，超限进入只读告警模式并冻结控制面；单 key 上限 1 MiB、单请求上限 1.5 MiB；Raft 单 Leader 串行写入限制写吞吐；磁盘 fsync 延迟超过心跳间隔（默认 100ms）会导致心跳丢失、请求超时与临时 Leader 丢失；MVCC 多版本需定期 compact 与 defrag，否则数据库膨胀至配额触发告警; API Server: list 请求内存占用随对象数无界增长，曾可在数秒内导致 OOM（K8s 1.33 引入 Streaming List 缓解）；watch cache 为 75 秒滑动窗口；读取约 1 GB 的 Pod 数据约消耗 5 GB 内存（含 etcd 与序列化/gRPC/解码开销）；webhook 与 API Priority and Fairness 排队会额外增加延迟; 扩展瓶颈拐点: 约 1,000+ 节点时 etcd 成为首要瓶颈（每个节点每 10 秒 PUT 一次状态/lease 续约，1,000 节点即约 100 次/秒写请求，快照写入期易超时）；etcd watch 通道爆炸发生在 1-2M 对象规模；调度器约 100 Pod/秒调度上限；控制器管理器在节点批量故障时工作队列堆积、GC 昂贵; NUMA: Topology Manager 历史上硬编码最多 8 个 NUMA 节点；K8s 1.35 GA 的 max-allowable-numa-nodes 允许调高，但官方警告高于默认值可能造成准入（admission）阶段性能下降（状态组合爆炸问题仍超出本增强范围）；K8s 1.36 修复了 36 NUMA 节点（如 NVIDIA GB200）上设备管理器 O(2^n) 拓扑枚举导致 kubelet 停滞的问题; 官方规模上限: 单集群 5,000 节点、每节点 110 Pod、总计 150,000 Pod、300,000 容器

**混部兼容性**

etcd 官方要求完全专用机器，避免与其他高 I/O 负载争用导致 fsync 延迟抖动；控制面节点默认带 node-role.kubernetes.io/control-plane 污点（NoSchedule），不建议与业务工作负载混部；堆叠 etcd 拓扑中 etcd 与控制面组件共享节点资源，须预留足够 CPU/内存并保证 etcd 进程的磁盘优先级（可 ionice）；监控/日志 Agent 等系统组件混部时需控制资源限额，避免 OOM 抢占关键控制面组件

### 配置与部署

**配置方式**

HA 控制面: kubeadm 堆叠 etcd（控制面与 etcd 同机）或外部 etcd（3+ 专用 etcd 机器）；TCP 负载均衡器以 kube-apiserver 6443 端口健康检查为入口；每个故障域部署 1-2 个控制面实例，负载均衡器按域路由避免跨域流量瓶颈; etcd 磁盘与配额: 为 etcd 分配独立本地 SSD/NVMe 分区（或独立物理盘）并预留足够空间（建议 >= 数据量 2 倍）；调整存储配额 --quota-backend-bytes（上限 8 GiB）；事件对象通过 --etcd-servers-overrides=/events#https://... 写入独立 etcd 集群; NUMA（K8s 1.35+ 突破 8-NUMA）: kubelet 配置 topologyManagerPolicy（none/best-effort/restricted/single-numa-node，1.27 GA），通过 topologyManagerPolicyOptions.max-allowable-numa-nodes 调高超过 8 个 NUMA 节点的限制; 扩展策略: 先垂直扩展（增大控制面节点 CPU/内存），收益递减后再水平扩展（增加控制面副本）；为监控/日志等 addon 使用 addon-resizer 或 VPA recommender 模式调整资源请求/限制，避免大集群下 OOM 与 CPU 节流

**配置示例**

kubelet Topology Manager（1.35 突破 8-NUMA 限制）: apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
topologyManagerPolicy: single-numa-node
topologyManagerPolicyOptions:
  max-allowable-numa-nodes: 16; kube-apiserver 事件写入独立 etcd: --etcd-servers=https://10.0.0.10:2379
--etcd-servers-overrides=/events#https://10.0.1.10:2379; kubeadm HA 控制面初始化: # 第一个控制面节点
sudo kubeadm init --control-plane-endpoint "lb.example.com:6443" --upload-certs
# 其余控制面节点加入（携带 --control-plane）
sudo kubeadm join lb.example.com:6443 --control-plane --certificate-key <key>

**部署位置与环境**

公有云托管控制面（EKS/AKS/GKE/ACK/TKE 等，硬件由云厂商承载，通过 SLA 保证 API 可用性）、私有云/裸金属（kubeadm、Talos、RKE2、kOps、KubeSpray）、虚拟机（绝大多数生产 K8s 以 VM 承载控制面，KVM/VMware/Hyper-V 均可）、边缘（K3s/KubeEdge 等轻量单节点或小规模控制面）；混合部署时控制面网络延迟应尽量低（同一数据中心/可用区内）

### 性能特征

**基准性能数据**

官方规模与 SLO: 官方支持上限：5,000 节点、150,000 Pod、300,000 容器（每节点 110 Pod）；K8s 上游 SLO：变更类 API 请求 P99 ≤ 1 秒、单资源读 P99 ≤ 1 秒、namespace 级 list ≤ 5 秒、集群级 list ≤ 30 秒、无状态 Pod 启动延迟（不含镜像拉取）P99 ≤ 5 秒; etcd: 标准集群 50 顺序 IOPS 即可运行，高负载需 500 顺序 IOPS；写延迟为决定性指标（社区实践 fsync p99 < 10 ms，超过心跳间隔 100 ms 即引发不稳定）；默认存储配额 2 GiB、推荐上限 8 GiB; 调度吞吐: 社区实测调度器约 100 Pod/秒 上限；1,000 节点集群中节点状态写入约 100 次/秒，对 etcd 构成持续压力; 大规模实测: PayPal 公开案例：4,000+ 节点 / 200,000 Pod；云厂商超大规模（30,000-130,000 节点）均以定制分布式存储后端替换 etcd；Sidero Labs 实测 3 节点控制面写性能优于 5 节点（约 -5% 吞吐）; 控制面节点规格效果: 4 vCPU/16 GB 可支撑数百节点；8 vCPU/32 GB 可支撑约 1,000-2,500 节点；8-16 vCPU/32-64 GB + NVMe 方可稳定逼近 5,000 节点上限（经验值）

**扩展性上限**

单集群官方上限：5,000 节点 / 150,000 Pod / 300,000 容器（每节点 110 Pod）；瓶颈拐点经验值：约 1,000+ 节点时 etcd 成为首要瓶颈（GitHub issue #20540：节点状态写入导致 etcd 超时），500-2,000 节点是社区公认的'甜蜜点'；2,000-5,000 节点需具备：独立事件 etcd、大内存 API Server（16-32 GB+）、10GbE 控制面网络、NVMe 盘、addon-resizer/VPA 调整组件配额、云配额预留；超过 5,000 节点需多集群拆分或定制存储后端

### 安全

**安全特性**

传输安全: kube-apiserver 与 kubelet、调度器等组件间 mTLS；kube-apiserver 至 etcd 使用 TLS；K8s 1.35 起 kube-apiserver 支持证书双向验证（mTLS）新特性; 静态数据加密: etcd 支持 EncryptionConfiguration 对 Secret 等敏感资源静态加密（AES-CBC / KMS 提供方），KMS 可由云厂商 KMS/HSM 承载密钥; 硬件信任根与节点身份: 控制面节点可启用 TPM 2.0 + UEFI Secure Boot 建立硬件信任根；通过远程证明/设备插件（如 Confidential Containers、kubelet 节点证明）向控制面验证节点身份; 访问控制与审计: API Server 内置 RBAC、准入控制与审计日志（控制面安全基线，与硬件配置无关）; 物理隔离: 控制面节点不调度业务负载，降低横向移动与资源争用攻击面；外部 etcd 拓扑进一步隔离数据面

### 运维与生命周期

**可观测性支持**

etcd 指标: 原生 Prometheus 指标：etcd_disk_wal_fsync_duration_seconds、etcd_disk_backend_commit_duration_seconds、etcd_server_leader_changes_seen_total、etcd_server_slow_apply_total 等；/health 与 /metrics HTTP 端点（kubeadm 1.35 起支持 HTTPEndpoints 与 gRPC 分离，便于安全暴露指标）; API Server 指标: apiserver_request_duration_seconds、apiserver_request_sli_duration_seconds（1.27+，不含 webhook 与排队时间）、kubelet_pod_start_sli_duration_seconds；SIG Scalability 提供 clusterloader2 Prometheus 规则与 Grafana SLO 仪表板; 健康检查: etcd /health、kube-apiserver /healthz、kube-scheduler 与 controller-manager /healthz；kubeadm 1.35 起以 quorum 方式检查 etcd 集群健康（多数成员健康即视为健康）

**维护与生命周期**

etcd 需定期 compact（压缩历史版本）与 defrag（碎片整理）以回收空间，超配额会触发只读告警；备份使用 etcd snapshot（etcdutl/etcdctl）并定期演练恢复；控制面升级按发行版流程逐节点排空（控制面排空需保证 etcd quorum，一次仅一个节点）；NVMe 需监控写入耐久度（TBW）与健康（nvme-cli、SMART 指标）；控制面节点故障替换需预先备份 etcd 数据目录与 PKI 证书；kubeadm 的 etcd learner 机制（1.35 增强）支持平滑成员加入/替换

**弹性与故障恢复**

etcd 共识: 3 节点容忍 1 台故障，5 节点容忍 2 台故障；Leader 选举依赖心跳（默认 100ms 间隔），故障切换通常秒级完成；2 节点控制面比 1 节点更差（两节点须同时在线且存在脑裂风险），故必须奇数节点; 跨可用区: 每个故障域至少 1 个控制面实例，负载均衡器按域路由避免跨域流量瓶颈；etcd 官方建议成员部署在同一数据中心（避免分区事件），跨可用区需评估网络延迟对 Raft 心跳与提交延迟的影响; 数据持久化: etcd 数据（数据目录 + WAL）位于本地持久化磁盘，必须保障磁盘故障不丢数据（硬件冗余/备份快照）；丢失多数 etcd 成员时需从快照 + 增量日志恢复；控制面组件本身无状态，可快速重建

### 经济性

**成熟度与社区支持**

控制面架构（etcd + kube-apiserver + kube-controller-manager + kube-scheduler）是 Kubernetes 最成熟的部分：etcd 为 CNCF 毕业项目，kubeadm HA 为 GA；Talos、RKE2、kOps、KubeSpray、KubeKey 等发行版生态成熟；EKS/AKS/GKE/ACK/TKE 等托管控制面广泛提供；SIG Scalability 持续维护 5,000 节点基准测试与 SLO（slos.md）；Topology Manager/NUMA 相关 KEP 由 SIG Node 活跃维护（1.35 GA、1.36 修复 36-NUMA 设备枚举问题），社区活跃度极高

---

## 6. DPU / IPU / SmartNIC 硬件卸载（NVIDIA BlueField、Intel IPU、AMD Pensando，配套 OVN-Kubernetes DPU 模式与模拟器）

**官方文档**: NVIDIA BlueField: https://www.nvidia.com/en-us/networking/products/data-processing-unit/ ; NVIDIA DOCA: https://developer.nvidia.com/doca ; NVIDIA DPF (DOCA Data Path Framework): https://networking-docs.nvidia.com/dpf ; OVN-Kubernetes: https://ovn-kubernetes.io/ (DPU 部署: https://ovn-kubernetes.io/installation/launching-ovn-kubernetes-with-dpu/ ; OVS-DOCA 卸载: https://ovn-kubernetes.io/features/hardware-offload/ovs-doca/) ; OVN-Kubernetes DPU 模拟器: https://github.com/ovn-kubernetes/dpu-simulator ; Intel IPU: https://www.intel.com/content/www/us/en/products/details/networking/ipu.html ; Intel IPU SoC E2100 规格书: https://cdrdv2-public.intel.com/818147/Intel%20Infrastructure%20Processing%20Unit%20SOC%20E2100.pdf ; AMD Pensando: https://www.amd.com/en/products/data-processing-units/pensando.html ; Platform9 DPU/SmartNIC 集成: https://docs.platform9.com/private-cloud-director/virtualized-networking/dpu-smartnic-integration-and-hardware-offloading

### 硬件规格

**最低配置**

NVIDIA BlueField-2: 单口 25GbE (ConnectX-6 Dx 控制器)，8 核 Arm A72 (2.2GHz)，16GB DDR4 + 64GB eMMC，PCIe Gen4 x8/x16 槽位，12V 供电最高 75W（部分型号需 6-pin ATX 辅助供电）。主机需 x86_64 或 arm64 Linux 服务器、支持 SR-IOV 与 switchdev。; NVIDIA BlueField-3: 单口 100GbE/200GbE (E 系列 8 核 Arm、16GB DDR5) 或 400GbE (P 系列 16 核 Arm、32GB DDR5)，PCIe Gen5 x16，需 DOCA 2.x+ 软件栈。; 软件侧 (OVN-Kubernetes DPU 模式): DPU 卡 + 支持 switchdev 的 DPU 主机，DPU 侧运行 arm64 容器镜像；DPU 模拟器最低要求：Kind 模式 8GB RAM + 容器运行时，VM 模式 12GB RAM + 100GB 磁盘 + KVM/QEMU (libvirt)，操作系统限 Fedora/RHEL/CentOS。

**推荐配置**

NVIDIA BlueField-3: 双口 200GbE 或 400GbE (B3220/B3210E 等)，16 核 Arm、32GB DDR5，PCIe Gen5 x16，配合 DOCA DPF 25.x + OVN-Kubernetes DPU 模式，用于生产裸金属 K8s 集群的完整网络/存储/安全卸载。; NVIDIA BlueField-2: 双口 100GbE (ConnectX-6 Dx)，8 核 Arm、16GB DDR4，用于 25-100GbE 规模的卸载场景或开发测试。; Intel IPU E2100: 双口 200GbE，16 核 Arm Neoverse N1，用于虚拟化/裸金属基础设施卸载场景 [不确定内存]。; AMD Pensando DSC2-200: 2x200GbE，P4 流水线硬件加速交换/路由/防火墙/负载均衡/加密，适合云厂商与 vSphere 8/OpenStack 环境 [不确定内存与功耗]。; 软件侧: 主机: RHEL 9.x / Ubuntu 22.04/24.04 LTS，内核支持 switchdev；部署 NVIDIA Network Operator (DPF)、SR-IOV Network Operator、Multus、cert-manager、Node Feature Discovery；DPU 侧 arm64 Ubuntu/RHEL 运行 ovnkube-node-dpu。

**生产级配置**

NVIDIA BlueField-3 + DOCA DPF: 双口 400GbE (NDR InfiniBand 或 400GbE) P 系列 16 核/32GB DDR5 型号，双集群架构: x86 管理集群 (运行应用与控制面) + DPU 上运行的 arm64 承载集群 (处理 OVN 覆盖网络、隧道封装/解封装、连接跟踪、安全策略)，Red Hat OpenShift 与 Canonical K8s (Ubuntu 24.04 + K8s 1.32) 已验证；支持裸金属大规模云。; AMD Pensando (云厂商场景): IBM Cloud 裸金属等大规模云环境使用 Pensando DSC 实现可扩展云原生架构；配合 vSphere 8/NSX 与 Dell PowerEdge 16G 服务器 [不确定具体集群规模]。; Intel IPU E2200 (Mount Morgan): 2025 年 8 月 Hot Chips 发布，TSMC N5 工艺，400G MAC + PCIe Gen5 x32 (内置 PCIe 交换机)，最高 24 核 Arm Neoverse N2 + 4 通道 LPDDR5，P4 可编程、内联加密引擎、RDMA 传输引擎、流量整形，支持多主机/无头/融合部署模式，面向下一代 400G 数据中心 [不确定量产状态]。

### 兼容性

**支持的 K8s 版本范围**

NVIDIA DPF: 25.4.0 / 25.7.x / 25.10.x / 26.4.0 等版本，支持双口 BlueField-3；Canonical 集成验证组合为 Ubuntu 24.04 LTS + K8s 1.32.2 + DPF 25.1；OpenShift 4.12+ 支持 DPU 相关功能 (OVN-Kubernetes DPU 模式)。

**操作系统兼容性**

主机侧: RHEL 8/9、Rocky Linux、Ubuntu 22.04/24.04 LTS、Fedora、CentOS Stream (DPU 模拟器明确要求 Fedora/RHEL/CentOS)；需要内核支持 switchdev、SR-IOV、VF representor。; DPU 侧: arm64 Linux (Ubuntu、RHEL 系)，运行 OVS/OVN 数据面与 ovnkube-node-dpu 容器；NVIDIA DOCA 支持 Ubuntu/RHEL/Rocky 特定版本。

**K8s 上游支持阶段**

OVN-Kubernetes: CNCF 项目 (2023 年进入 CNCF Sandbox [不确定当前阶段])，由 Red Hat/IBM/NVIDIA 等主导，生产就绪；DPU 模式与 SmartNIC (硬件卸载) 模式为 OVN-Kubernetes 提供的成熟功能。; NVIDIA DOCA DPF: 厂商 (NVIDIA) 方案，文档完善、版本化发布 (25.4-26.4)，与 OpenShift、Canonical、Spectro Palette、Platform9 等集成，生产级。; K8s 上游: K8s 上游无独立的 DPU GA 标准；依赖 CNI 生态 (OVN-Kubernetes)、设备插件 (SR-IOV Device Plugin) 与 DRA (Dynamic Resource Allocation) 演进承载 [不确定 DRA 对 DPU 的正式支持]。; Intel / AMD: Intel IPU E2100 已出货、E2200 刚发布 (2025)；AMD Pensando 在云厂商 (IBM Cloud) 与 vSphere 8 环境生产使用，K8s 生态集成度低于 NVIDIA 方案。

**生态兼容性矩阵**

Operator: NVIDIA Network Operator (DPF)、SR-IOV Network Operator、Node Feature Discovery、cert-manager、NVIDIA GPU Operator (可共存)；Canonical Juju/MAAS charms；Mirantis k0rdent；Spectro Cloud Palette 模板化集成。; 监控集成: Prometheus: OVN-Kubernetes/OVS exporter、NVIDIA Network Operator 指标、DOCA Flow 遥测、SR-IOV Operator 指标；Pensando 内置 P4 遥测 [不确定 Prometheus 导出器细节]。; 平台集成: Red Hat OpenShift、Canonical Kubernetes (Ubuntu 24.04 + DPF 25.1)、Spectro Cloud Palette、Platform9 Private Cloud Director (OpenStack 虚拟化网络)、Mirantis k0rdent、IBM Cloud 裸金属 (Pensando)。

### 限制与约束

**已知限制**

硬件依赖: 必须采购专用 DPU/IPU 卡 (BlueField-2/3、E2100/E2200、DSC2-200)，成本高、供货渠道有限；PCIe 槽位与功耗占用 (x16 槽)。; OVN-Kubernetes DPU 模式: 双集群架构 (主机集群 + DPU 集群) 显著增加部署与运维复杂度；官方明确 identity 功能 (UserDefinedNetwork 相关) 当前不支持 DPU/DPU-Host 集群；主机侧 OVS 被旁路，故障排查链路改变。; 硬件卸载能力边界: OVS 硬件卸载 (tc offload) 无法覆盖全部流表项，未命中硬件的流量回退软件路径 (性能毛刺)；连接跟踪、NAT 等有状态功能卸载受 ASIC/流水线能力限制 (Pensando P4 灵活但受表项容量约束)。; 软件栈绑定: NVIDIA 方案要求 DOCA 版本、MLNX_OFED、固件与 OVN-Kubernetes 版本严格对齐；Intel E2200 2025 年才发布，量产与生态验证有限；Pensando 在 K8s 原生场景资料较少。; DPU 模拟器: 仅用于开发/CI 模拟，无法完全复现硬件卸载性能与行为；要求 Fedora/RHEL/CentOS 主机。

### 配置与部署

**配置方式**

NVIDIA DPF + OVN-Kubernetes: 声明式 Operator/Helm/CRD 方式: 安装 NVIDIA Network Operator (DPF)、SR-IOV Network Operator、Multus、cert-manager、Node Feature Discovery；主机节点打标签 k8s.ovn.org/dpu-host=，DPU 节点打标签 k8s.ovn.org/dpu=；OVN-Kubernetes 控制面自动检测数据面类型，无需集群侧特殊开关。; SmartNIC 硬件卸载 (单集群): 手动配置: 分配大页、网卡切 switchdev 模式、启用 OVS 硬件卸载 (ovs-vsctl set Open_vSwitch . other_config:hw-offload=true)、外部 OVS 网桥与 OVN 集成网桥使用 netdev datapath、部署 SR-IOV 设备插件 + Multus 将 VF 分配给 Pod。; OVN-Kubernetes DPU 模拟器: YAML 配置 + dpu-sim CLI (依赖检查、生命周期管理、集群引导)；VM 模式走 libvirt/KVM + cloud-init + SSH，Kind 模式走 Docker/Podman 容器 + veth 数据通道；模拟环境需设置 global.simulateDpu=true。; 平台方案: Platform9: 管理面启用 DPU/SmartNIC 卸载支持，创建 switchdev 模式端口挂到服务器；Canonical: Juju/MAAS charms 声明式部署；Palette: 8 个变量的引导式模板约 30 分钟完成集群搭建。

**部署位置与环境**

主要场景: 裸金属数据中心 K8s 集群 (OpenShift、Canonical K8s、RKE2 类发行版)、私有云 (OpenStack/Platform9 PCD)、公有云裸金属 (IBM Cloud 使用 Pensando)。; 支持环境: x86_64 主机 + DPU 卡；BlueField-3 + DPF 验证于 OpenShift 与 Ubuntu 24.04；Pensando 验证于 vSphere 8/Dell PowerEdge 16G 与 OpenStack。

**虚拟化兼容性**

SR-IOV: 核心机制: Pod/VM 通过 SR-IOV VF + VF representor 接入，数据面绕过主机内核直达 DPU。

### 性能特征

**基准性能数据**

吞吐: BlueField-3: 400GbE/NDR InfiniBand 双口，Spectro Cloud 报告可卸载最高 400Gbps 网络带宽；BlueField-2: 25/100GbE；Intel E2100: 200GbE；E2200: 400G MAC；Pensando DSC2-200: 2x200GbE 线速。; CPU 节省: 官方无统一百分比。定性数据: Red Hat/Canonical/NVIDIA 均报告卸载后主机 CPU 利用率极低 (数据面开销趋近于零)，Spectro Cloud 报告将 vSwitch 任务移入专用芯片后可释放 CPU 循环给应用 [不确定具体核数]。; OVS-DOCA 卸载: 使用 DOCA-flow API 硬件流表替代软件流表，显著提升流表插入速率 (insertion rate) 与连接跟踪 (CT) 规模、吞吐与扩展性，支持更快的功能迭代。; Pensando: Principled Technologies 发布过 DSC-200 性能报告 (转发/安全功能硬件加速) [不确定具体数字]。; DPU 模拟器: 无硬件性能数据，仅模拟控制面行为供开发测试。

### 安全

**安全特性**

NVIDIA BlueField: Secure Boot、UEFI 安全启动选项、硬件加密加速 (crypto engine，支持 IPsec/TLS 卸载)、硬件信任根 (RoT)、DOCA 远程证明 (attestation) 能力；DPF/HBN 提供零信任安全隔离，每个 Pod/VM 独立安全域，安全策略在 DPU 硬件上强制执行 (OVN 网络策略卸载)。; Intel IPU: 基础设施与租户应用强隔离 (基础设施任务在 IPU 上独立运行)、P4 可编程流水线支持防火墙/加密卸载、内联加密引擎 (E2200)、Intel 平台安全特性 [不确定 TPM]。; AMD Pensando: 硬件状态防火墙、负载均衡、加密 (IPsec) 卸载、内嵌遥测，P4 流水线实现可编程安全策略，支持零信任多租户隔离 (IBM Cloud 采用)。; OVN-Kubernetes DPU: 网络策略/ACL 下放到 DPU 硬件执行，主机侧攻击面减小；DPU 集群通过 service account token 访问主机集群 (需安全保管)。

### 运维与生命周期

**可观测性支持**

NVIDIA: NVIDIA Network Operator 暴露指标、DOCA Flow 流表统计、OVN-Kubernetes/OVS Prometheus exporter、SR-IOV Operator 指标；支持遥测卸载 (DPU 上收集)。; Pensando: P4 流水线内嵌遥测 (丢包、延迟、流统计) [不确定 Prometheus 导出方式]；DSC 管理接口提供健康状态。

### 经济性

**总拥有成本 (TCO)**

运行成本: 每卡额外 25-75W 功耗 (含散热)；软件成本: DOCA/DPF 免费，但集成平台 (OpenShift、Canonical、Palette、Platform9) 需相应订阅许可。

**成熟度与社区支持**

NVIDIA BlueField/DOCA: 生态最成熟: 与 Red Hat OpenShift、Canonical、Spectro Palette、Platform9、Mirantis k0rdent、IBM 等大量集成；DOCA 文档与社区活跃；OVN-Kubernetes 为 CNCF 项目，由 Red Hat/IBM/NVIDIA 主导，是 K8s DPU 卸载的事实标准载体。; Intel IPU: E2100 已量产出货，E2200 (Mount Morgan) 2025 年 8 月发布，生态与部署案例少于 NVIDIA；有 DPDK/存储生态支撑。; AMD Pensando: 云厂商采用 (IBM Cloud 裸金属、vSphere 8/NSX、Dell PowerEdge 16G)，在虚拟化网络领域成熟；K8s 原生集成资料与社区活跃度较低。; DPU 模拟器: ovn-kubernetes 官方项目，用于 CNI 上游 CI/CD，社区持续维护。

---

## 7. DRA（Dynamic Resource Allocation，动态资源分配）

**官方文档**: K8s 官方概念文档: https://kubernetes.io/docs/concepts/scheduling-eviction/dynamic-resource-allocation/ ; K8s v1.34 DRA GA 博客: https://kubernetes.io/blog/2025/09/01/kubernetes-v1-34-dra-updates/ ; K8s v1.33 DRA 更新博客: https://kubernetes.io/blog/2025/05/01/kubernetes-v1-33-dra-updates/ ; K8s v1.36 DRA 更新博客: https://kubernetes.io/blog/2026/05/07/kubernetes-v1-36-dra-136-updates/ ; CNCF 理解 DRA (2026): https://www.cncf.io/blog/2026/07/01/understanding-dynamic-resource-allocation-in-kubernetes/ ; KEP-3063 (Classic DRA): https://kep.k8s.io/3063 ; KEP-4381 (结构化参数): https://kep.k8s.io/4381 ; KEP-5055 (设备污点与容忍): https://www.kubernetes.dev/resources/keps/5055/ ; NVIDIA DRA 驱动: https://github.com/kubernetes-sigs/dra-driver-nvidia-gpu ; NVIDIA GPU Operator DRA 安装: https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/latest/dra-intro-install.html ; AMD DRA 驱动: https://instinct.docs.amd.com/projects/gpu-operator/en/main/dra/dra-driver.html ; Intel 资源驱动: https://github.com/intel/intel-resource-drivers-for-kubernetes ; 上游示例驱动框架: https://github.com/kubernetes/dynamic-resource-allocation ; GKE DRA 文档: https://docs.cloud.google.com/kubernetes-engine/docs/concepts/about-dynamic-resource-allocation ; AWS EKS EFA DRA 公告: https://aws.amazon.com/about-aws/whats-new/2026/05/kubernetes-dra-elastic-fabric-adapter/

### 硬件规格

**最低配置**

控制面: Kubernetes 1.26+（Alpha 实验）至 1.34+（GA 稳定，resource.k8s.io/v1 默认启用），1.35 起功能门控锁定、默认开启；控制面组件（kube-apiserver、kube-scheduler、kube-controller-manager）需启用对应 API 组与功能门控；DRA 本身无专用硬件要求，可运行在标准控制面节点（约 2-4 核 CPU、4-8GB 内存）。; 节点侧: 标准 Linux 工作节点即可，无专用硬件；kubelet 需启用 DRA 支持（GA 后默认开启）；容器运行时需支持 CDI（Container Device Interface，如 containerd 1.7+、CRI-O 1.28+），以便驱动通过 CDI 将设备注入容器。; 驱动要求: 必须部署至少一个 DRA 驱动（由厂商提供，通常为控制器 + kubelet 插件 DaemonSet），驱动负责发布 ResourceSlice 描述节点设备清单并在分配时准备/释放设备。; 示例 (NVIDIA): NVIDIA DRA 驱动要求 Kubernetes 1.32+；经 GPU Operator 集成要求 v1.34.2+；使用 v1.34 时需显式启用 resource.k8s.io/v1 API 组 [不确定 v1.34 早期小版本的具体开关方式]。

**推荐配置**

生产集群: Kubernetes 1.34+（GA 核心 API）或 1.35+（功能门控锁定、完全默认启用，CNCF 2026 文章表述为 fully supported）；建议 1.36 以获得分区设备（Beta）、扩展资源过渡（Beta）、设备污点（Beta）、优先列表（Stable）等扩展能力；控制面按标准 HA（3 节点）配置，etcd 使用 NVMe SSD。; 驱动与生态: 选择与硬件匹配的官方 DRA 驱动：NVIDIA（GPU + ComputeDomain）、AMD（gpu.amd.com）、Intel（GPU/Gaudi/QAT，标记 beta/非生产）；生产 GPU 集群建议通过 GPU Operator 集成方式部署 NVIDIA DRA 驱动并为节点打上驱动标签；管理员预先创建集中式 DeviceClass（如 cost-optimized / high-performance）。; 运行时: containerd 1.7+ / CRI-O 1.28+ 启用 CDI，驱动生成的 CDI spec 由运行时加载以注入设备。; 存储与网络: DRA 核心无特殊要求；网络型 DRA 驱动（如 DRANET / AWS EFA）需对应网卡硬件（EFA、SR-IOV 网卡等）及驱动。

**生产级配置**

大规模异构集群: 管理数千节点的集群可并行部署多个 DRA 驱动（GPU + 网络 + 加速器）实现组合分配；NVIDIA ComputeDomain 支持 GB200/NVL72 级多节点 NVLink 域；每个驱动可管理多个 ResourceSlice 池（每池最多 128 项设备信息）[不确定大规模多驱动组合的公开生产案例]。; 云托管: EKS 支持 NVIDIA DRA 驱动（P6e/GB200 实例）与 EFA DRA（K8s 1.34+，拓扑感知分配 + EFA 接口共享）；GKE Standard 提供 DRA 预览（仅 GPU）；[不确定 2026 年各云厂商 GA 状态]。; 高可用: DRA 驱动控制器按多副本部署；分配状态持久化于 etcd（ResourceClaim allocation 状态），调度器重启后可恢复；设备故障通过 DeviceTaint / 硬件健康诊断（1.36）隔离不健康设备 [不确定生产级最佳实践细节]。

**功耗基线**

不适用：DRA 是 Kubernetes 控制面与节点上的软件框架，本身无独立功耗基线；其功耗影响仅为驱动组件（DaemonSet 容器）的轻微 CPU/内存占用，与所管理硬件（GPU、网卡等）的功耗无关，功耗基线由具体硬件决定。

### 兼容性

**支持的 K8s 版本范围**

DRA 核心: v1.26 引入（Alpha）；v1.32 API 演进至 v1beta1 并随多个子特性 Beta；v1.34 核心 API（resource.k8s.io/v1）GA 并默认启用；v1.35 功能门控锁定（完全支持、默认开启）；v1.36 继续扩展（分区设备、扩展资源过渡、设备污点、CPU/内存原生资源、PodGroup Claims 等）。; 结构化参数 (KEP-4381): 自 v1.27 起为推荐模型，GA 后成为唯一核心模型；Classic DRA（KEP-3063）的控制器实现作为可选兼容层未进入主路径。; NVIDIA DRA 驱动: 要求 Kubernetes 1.32+；GPU Operator 集成要求 v1.34.2+；使用 DRA 扩展资源过渡需 1.36+（对应功能门控默认开启）。; AMD DRA 驱动: 要求 Kubernetes 1.32+，需启用 DynamicResourceAllocation 功能门控（1.32 上）[不确定后续版本是否默认]；生产建议 1.34+。; Intel 资源驱动: Kubernetes 1.26+（项目明确标记 beta / 非生产）。

**操作系统兼容性**

驱动 OS 支持: 由各厂商驱动决定：NVIDIA 驱动支持主流 Linux（Ubuntu/RHEL 系等）；AMD 驱动要求 amdgpu 内核模块与 CDI 运行时；Intel 驱动依赖内核设备驱动（i915/xe 等）[不确定完整认证矩阵]。; 运行时要求: 容器运行时需支持 CDI（containerd 1.7+、CRI-O 1.28+ 等）以注入 DRA 分配的设备。

**K8s 上游支持阶段**

GA（稳定）：v1.34 核心 API 稳定并默认启用，v1.35 功能门控锁定，官方承诺核心无破坏性变更；扩展能力分级：Prioritized List（Stable, 1.36）、Partitionable Devices（Beta, 1.36）、Extended Resource 过渡（Beta, 1.36）、Device Taints/Tolerations（Beta, 1.36）、Admin Access（Beta, v1.32 起）、Consumable Capacity（Alpha, 1.34）、ResourcePoolStatusRequest（Alpha, 1.36）、CPU/内存原生资源与 PodGroup Claims（1.36 新特性）；DRA 已被明确为设备插件机制的官方演进替代方向。

**生态兼容性矩阵**

DRA 驱动: NVIDIA（GPU/ComputeDomain，Kubernetes SIG 托管仓库）; AMD（GPU，随 AMD GPU Operator 提供）; Intel（GPU/Gaudi/QAT 资源驱动，beta）; 上游社区示例驱动（kubernetes/dynamic-resource-allocation）; 网络类 DRANET 驱动（开源，AWS EFA DRA 基于其构建，支持拓扑感知分配与 EFA 接口共享）; CNI DRA driver（社区探索中，解决与 CNI 的集成问题）。; 调度器: 原生 kube-scheduler（内建 DRA 过滤/打分与分配流程）；Kueue/Volcano 等批调度器配合 DRA 使用 [不确定各批调度器与 DRA 集成的成熟度]；Cluster Autoscaler 对 DRA 的支持处于演进中（CNCF 2026 文章展望按 DRA 需求动态供给 GPU 节点）。; 监控集成: kubectl describe resourceclaim 查看分配状态与厂商元数据; ResourcePoolStatusRequest（1.36 Alpha）查询池 totalDevices/allocatedDevices/availableDevices; 硬件健康诊断（1.36）; GKE 限制 DRA GPU 节点无法使用托管 DCGM metrics 包 [不确定开箱即用的 Prometheus 指标集]。; 相关项目: CDI（容器设备接口，DRA 设备注入基础）、NRI（节点资源接口，与 DRA 互补）、KEP-5055 设备污点、NVIDIA GPU Operator（集成 DRA 驱动）、AWS DRANET（网络 DRA）。

### 限制与约束

**已知限制**

驱动生态成熟度: NVIDIA GPU 侧部分能力（动态 MIG 分配、time-slicing 参数化）尚未官方支持，GPU kubelet plugin 在 Helm 安装中默认关闭；MIG 配置变更需手动重启驱动 Pod；Intel 资源驱动明确标记 beta/非生产。; 云平台限制: GKE 仅 Standard 模式集群、仅支持 GPU、不支持 time-sharing/MIG/MPS、DRA GPU 节点不可用托管 DCGM metrics、自动扩缩容受限（第三方驱动需至少 1 个节点、静态 ResourceClaim 时 DaemonSet Pod 上限 128）；EKS 的 DRA 与 Karpenter 不兼容（AWS 明确）。; 调度语义: 拓扑保证完全依赖驱动发布的 ResourceSlice 约束与属性；设备分配在 Pod 生命周期内固定，不支持运行中变更；调度失败原因可见性有限（1.36 ResourcePoolStatusRequest Alpha 改进）。; 滚动更新问题: 滚动更新中终止的工作负载保留其分配，可能导致新工作负载只能接受降级硬件（CNCF 2026 文章指出）。; 配额/限制模型: DRA 无传统按容器的资源配额语义；每容器限制由驱动与 Consumable Capacity（Alpha）机制决定；ResourceSlice 每池上限 128 项设备。; 与设备插件并存: AMD 明确 DRA 驱动与设备插件不能同时启用；整体迁移需要应用改造（1.36 扩展资源过渡 Beta 缓解迁移成本）。; 可移植性: ResourceClaim 引用具体 DeviceClass/驱动，跨集群、跨厂商的可移植性依赖驱动命名与设备属性约定。

**固件与驱动依赖**

依赖各硬件厂商驱动栈：NVIDIA（NVIDIA 驱动 + Container Toolkit + CDI，ComputeDomain 依赖支持多节点 NVLink 的硬件如 GB200，内部编排 IMEX 原语）；AMD（amdgpu 内核模块 + CDI 运行时）；Intel（内核 i915/xe 等设备驱动）；网络（EFA/DRANET 需对应 RDMA 网卡及驱动）；驱动升级通常经 Helm/Operator 滚动更新，设备配置变更（如 MIG）需重启驱动 Pod。

### 配置与部署

**配置方式**

管理员: 安装 DRA 驱动（控制器 + kubelet 插件 DaemonSet）；驱动自动发布 ResourceSlice 描述节点设备；管理员创建集中式 DeviceClass 定义设备类别与匹配规则（分配策略：独占或共享）。; 应用开发者: 通过 ResourceClaim（手动生命周期，可被多个 Pod 共享同一设备）或 ResourceClaimTemplate（自动为每个 Pod 生成独立 claim，生命周期绑定 Pod）声明设备请求；使用 CEL 选择器按设备属性（型号、显存、拓扑、NVLink 等）精确过滤。; NVIDIA: GPU Operator 集成：为节点打 kubelet 插件标签 → 部署 Operator（关闭默认设备插件）→ 配置环境变量匹配节点标签 → 部署 DRA Helm chart；安装后生成 gpu.nvidia.com、mig.nvidia.com 及 ComputeDomain 等 DeviceClass。; AMD: GPU Operator Helm 中设置 deviceConfig.spec.draDriver.enable=true，或直接应用独立 YAML；驱动生成 gpu.amd.com DeviceClass 并发布 ResourceSlice；DRA 驱动与设备插件不能同时启用。; Intel: 部署 intel-resource-drivers-for-kubernetes（GPU/Gaudi/QAT 驱动），依赖 DRA 框架，K8s 1.26+ [不确定各设备类名与配置细节]。; 网络 (DRANET/EFA): 基于开源 DRANET 驱动；AWS EKS 提供 EFA DRA（K8s 1.34+），实现拓扑感知分配（网络端口与 GPU/Trainium/Inferentia 就近）与 EFA 接口共享 [不确定非 AWS 环境的部署方式]。

**配置示例**

ResourceClaimTemplate 典型示例（来自官方文档与社区）:
apiVersion: resource.k8s.io/v1
kind: ResourceClaimTemplate
metadata:
  name: gpu-claim
spec:
  spec:
    devices:
      requests:
      - name: gpu
        deviceClassName: gpu.nvidia.com
        selectors:
        - cel:
            expression: |
              device.attributes["gpu.nvidia.com"].memory_gb >= 80 &&
              device.attributes["gpu.nvidia.com"].family == "h100"
Pod 通过 spec.resourceClaims 引用模板（每副本生成独立 claim）；共享场景直接创建 ResourceClaim 并在多个 Pod 中引用同一 claim 名；可消耗容量场景可声明部分容量（如 40Gi 中请求 4Gi）[不确定具体字段随版本变化]。

**部署位置与环境**

裸金属/私有云: 完全支持，主流部署场景（NVIDIA HGX、AMD Instinct、Intel 加速器节点），驱动以 DaemonSet 部署。; 边缘/轻量发行版: k3s/RKE2/Talos 等支持 DRA（有 Talos + DRA Beta 实践），但驱动生态以数据中心硬件为主 [不确定边缘生产案例]。; 虚拟机: DRA 设备注入依赖宿主机设备直通（PCIe passthrough / SR-IOV VF）后在 VM 内呈现的设备 [不确定嵌套虚拟化场景的完整性]。

**虚拟化兼容性**

设备注入: DRA 通过 CDI 将设备直接注入容器，包含 Kata 等沙箱运行时的设备透传路径 [不确定 Kata + DRA 组合的生产成熟度]。; SR-IOV/直通: 网络类 DRA 驱动可管理 SR-IOV VF 的分配与共享；GPU 直通（vfio）场景可通过 DRA 驱动声明 [不确定驱动覆盖度]。; 热迁移: DRA 分配在 Pod 生命周期内固定，不支持容器/VM 热迁移；ComputeDomain 生命周期绑定工作负载。

### 性能特征

### 安全

**安全特性**

Admin Access (Beta): 通过 namespace 标签 resource.k8s.io/admin-access: "true" 限制只有具备该 namespace 访问权限的授权用户/软件可创建 ResourceClaim，防止 DRA 驱动授予额外权限导致的提权，并防止用户访问其他 namespace 中正常应用正在使用的设备。; 设备污点与容忍 (Beta, 1.36): 驱动或管理员可通过 ResourceSlice 或 DeviceTaintRule 标记设备污点（如过热 GPU）；NoSchedule 阻止新分配，NoExecute 驱逐不具匹配容忍度的工作负载。; RBAC 与审计: DRA 使用 resource.k8s.io API 组，ResourceClaim 的 allocation 状态持久化于 etcd，可通过标准 RBAC 与审计日志管控，分配过程可审计。; ComputeDomain 隔离: NVIDIA ComputeDomain 保证 MNNVL 域内 Pod 间可达、域外安全隔离（在底层编排 IMEX 域/通道原语）。; 平台信任边界: DRA 驱动以特权 DaemonSet 运行并访问物理设备，属于平台信任组件；CDI 注入的设备对容器可见性与传统设备插件一致 [不确定安全模型差异]。

### 运维与生命周期

**可观测性支持**

分配状态: kubectl describe resourceclaim 显示节点分配、驱动信息与厂商元数据；ResourceClaim .status.devices（DRAResourceClaimDeviceStatus，Beta）提供设备级状态。; 容量可见性 (Alpha, 1.36): ResourcePoolStatusRequest 可查询资源池 totalDevices/allocatedDevices/availableDevices，显著改进硬件短缺类调度失败排障。; 健康诊断 (1.36): 硬件健康诊断能力（如 NVIDIA DeviceHealthCheck 由 Alpha 走向 GA 的演进）用于检测设备故障并配合设备污点隔离 [不确定具体指标暴露方式]。

### 经济性

**总拥有成本 (TCO)**

软件成本: DRA 核心与主流驱动免费开源（NVIDIA DRA 驱动 Apache-2.0 并托管于 Kubernetes SIG；AMD/Intel 驱动随其开源 Operator/项目），无额外许可费用。; 硬件收益: 通过 GPU 分片与可消耗容量、多 Pod 共享同一 ResourceClaim、分区设备及精确属性匹配，显著提升硬件利用率、减少过度供给与碎片化（官方与 CNCF 文章均强调此为 DRA 核心价值）[不确定量化节省数据]。; 云成本: EKS/GKE 上 DRA 为免费平台功能，但受云平台支持范围限制（GKE 预览、EKS 1.34+），成本差异主要来自硬件利用率提升 [不确定计费差异]。

**成熟度与社区支持**

上游: Kubernetes 官方 GA（1.34）、功能门控锁定（1.35）、持续扩展（1.36），社区共识度高；CNCF 于 2026 年 7 月发文系统阐述 DRA 架构与最佳实践。; 厂商: NVIDIA（最成熟：驱动托管于 Kubernetes SIG，支持 ComputeDomain/GB200，GPU Operator 集成）; AMD（GPU Operator 集成，v1beta1 文档）; Intel（beta 资源驱动）; AWS（EKS + EFA DRA，2026/05 公告）; Google（GKE Standard 预览）; 网络领域开源 DRANET 驱动出现。; 生态预期: CNCF 2026 文章展望 cluster autoscaler 将按 DRA 需求动态供给 GPU 节点；设备插件向 DRA 迁移为明确方向；驱动生态从计算加速器扩展至网络及其他硬件类型 [不确定 2026 年底生态里程碑]。

---

## 8. FPGA 可重构加速（Intel FPGA 设备插件 / AMD Xilinx Alveo 方案 / Funky 云原生 FPGA 编排 (2025)）

**官方文档**: Intel 设备插件: https://github.com/intel/intel-device-plugins-for-kubernetes ; Intel 设备插件文档: https://intel.github.io/kubernetes-docs/device-plugins/index.html ; Intel 设备插件 Operator: https://operatorhub.io/operator/intel-device-plugins-operator ; Intel DRA 资源驱动: https://github.com/intel/intel-resource-drivers-for-kubernetes ; Intel FPGA PAC D5005 数据手册: https://www.mouser.com/pdfDocs/Intel_ds-pac-d5005.pdf ; Intel Open FPGA Stack (OFS): https://ofs.github.io/ ; Intel FlexRAN: https://github.com/intel/FlexRAN ; AMD/Xilinx fpga-operator: https://github.com/Xilinx/fpga-operator ; Xilinx Video SDK 部署到 K8s: https://xilinx.github.io/video-sdk/v1.5/deploying_with_kubernetes.html ; AMD Alveo 加速卡: https://www.amd.com/en/products/accelerators/alveo.html ; AMD XRT 文档: https://docs.amd.com/r/en-US/ug1301-getting-started-guide-alveo-accelerator-cards/XRT-and-Deployment-Platform-Installation-Procedures-on-RedHat-and-CentOS ; K8s 设备插件文档: https://kubernetes.io/docs/concepts/extend-kubernetes/compute-storage-net/device-plugins/ ; K8s DRA 文档: https://kubernetes.io/docs/concepts/scheduling-eviction/dynamic-resource-allocation/ ; Funky 论文 (SoCC 2025): https://arxiv.org/html/2510.15755v1 ; Funky 会议 Slides: https://dse.in.tum.de/wp-content/uploads/2025/11/Funky-SoCC-2025-slides.pdf

### 硬件规格

**最低配置**

Funky 原型 (2025): Alveo U50 FPGA 卡 + Vitis Shell (XDMA 配置，单一可重构槽位)，标准 x86 服务器；需构建 unikernel 沙箱镜像、host 侧 monitor (瘦 hypervisor) 与扩展编排守护进程，并启用自定义调度器与运行时钩子。

**推荐配置**

Intel 方案: Intel PAC D5005 (Stratix 10 SX，约 2.8M 逻辑单元、32GB DDR4、PCIe Gen3 x16) [不确定具体规格] 或更高性能 Agilex 7 卡；K8s 1.28+ 集群，配合 Node Feature Discovery (NFD) 发现 fpga 标签、Intel Device Plugins Operator (Helm) 声明式部署设备插件、admission webhook 与 OCI hook；位流与 AFU (Accelerator Function Unit) 通过 AcceleratorFunction CRD 管理。

### 兼容性

**支持的 K8s 版本范围**

Intel 插件: Intel 设备插件按 K8s 版本节奏发布: v0.36.0 支持 K8s 1.36 (release-0.36)，1.35 (release-0.35)，1.33-1.34 (release-0.34)；通常支持最近 3 个 minor 版本。

**操作系统兼容性**

AMD/Xilinx: Ubuntu 20.04/22.04、RHEL/CentOS 8/9、Rocky Linux [不确定精确版本矩阵]；XRT 提供 .deb/.rpm 安装包；不支持 Windows。

**K8s 上游支持阶段**

设备插件: K8s 上游 GA 稳定框架 (v1beta1)，所有厂商 FPGA 方案均基于此，生产就绪。; Funky: 学术研究原型 (2025 年 SoCC 论文 + GitHub 开源 [不确定仓库地址])，不可用于生产。

**生态兼容性矩阵**

Operator/工具: Intel Device Plugins Operator (Helm/OperatorHub)、Node Feature Discovery (fpga 标签)、Intel DRA 资源驱动、Xilinx fpga-operator (管理 XRT + 容器运行时 + 设备插件)、AWS EKS + VT1/F1 生态、Xilinx Video SDK (FFmpeg 插件)。

### 限制与约束

**已知限制**

资源粒度粗: 设备插件模式下资源以整卡/整区域为单位 (U30 卡两个设备必须同时挂载给同一 Pod)；Intel AFU 模式虽可细分，但依赖 admission webhook + OCI hook 组合，配置复杂。; Funky 限制: 当前实现仅支持单一可重构槽位，不支持空间共享 (space sharing)；checkpoint 为软件实现 (恢复 340.8ms)，无硬件辅助状态保存；依赖特定 Vitis Shell/XDMA 配置。; 厂商锁定: Intel (DFL/OFS)、AMD/Xilinx (XRT/Vitis)、AWS (aws-shell) 各自独立，资源命名、驱动、工具链互不通用。

### 配置与部署

**配置方式**

Intel 设备插件: 设备插件以 DaemonSet 部署 + AcceleratorFunction CRD (自定义资源) 定义 AFU (afuId/interfaceId/mode) + fpga admission webhook 将用户友好函数名翻译为资源名 (如 fpga.intel.com/af-695.d84.aVKNtusxV3qMNmj5-qCB9thCTcSko8QT-J5DNoP5BAs) + OCI createRuntime hook 在容器启动前向 PR 区域加载位流；亦可通过 Intel Device Plugins Operator (Helm/OperatorHub) 声明式部署，NFD 自动打 fpga 节点标签。; AMD/Xilinx: fpga-operator 自动化安装 XRT、容器运行时与 k8s-fpga-device-plugin；资源通过 extended resource 暴露 (如 xilinx.com/fpga-xilinx_u30_gen3x4_base_1-0)，Pod 在 limits 中请求；Xilinx Video SDK 提供 YAML 模板 + EKS 部署流程。; Funky: 扩展编排守护进程 + 自定义调度器 (优先级/抢占) + 自定义运行时 (驱逐/checkpoint)；通过标准 CRI/OCI 注解传递硬件元数据，Pod 请求虚拟 FPGA 设备。

**配置示例**

Intel 资源请求: resources.limits: fpga.intel.com/af-695.d84.aVKNtusxV3qMNmj5-qCB9thCTcSko8QT-J5DNoP5BAs: 1 ; AcceleratorFunction CRD 示例: afuId: d8424dc4a4a3c413f89e433683f9040b (含 interfaceId 与 mode: af/region 字段)；NFD 标签: intel.feature.node.kubernetes.io/fpga-arria10: 'true' [不确定完整 YAML]

**部署位置与环境**

裸金属: 主要场景: 数据中心裸金属集群 PCIe 直通安装 FPGA 卡，kubeadm/RKE2/OpenShift 均可承载 [不确定 OpenShift 官方验证]；边缘: 5G O-DU/MEC 机柜 (FlexRAN + Agilex)、低功耗 Alveo U50 边缘推理。; 公有云: AWS EC2 F1 (VU9P)、VT1 (U30 视频转码) 支持 EKS 原生集成；阿里云/腾讯云 FPGA 实例为 F1 类 (VU9P) [不确定具体集成度]；其余公有云 FPGA 实例有限。

**虚拟化兼容性**

VM 热迁移: 设备直通场景不支持热迁移；Funky 的 unikernel + checkpoint 是面向 FPGA 状态的软迁移方案 (恢复 340.8ms)。

### 性能特征

**基准性能数据**

Funky (2025): 相对原生执行仅 7.4% 性能开销；挂起 177.2ms / 恢复 340.8ms (大规模数据集)；基准覆盖 3D 渲染与光流 (optical flow) 等图像/视频任务。; 5G PHY: Intel FlexRAN + Agilex FPGA 加速 LDPC 编解码/FEC，替代多核 CPU 软编解码，满足 5G NR 实时性要求 [不确定具体吞吐数字]。

### 安全

**安全特性**

多租户隔离: PR 区域划分提供物理隔离 (Intel region 模式)；Funky 通过 unikernel 沙箱 + monitor 实现租户间隔离与状态隔离；设备插件保证设备独占 (不共享)。

### 运维与生命周期

### 经济性

**总拥有成本 (TCO)**

运行成本: 每卡 75-150W 额外功耗与散热；XRT/DFL 软件免费，但 Vitis/Vivado 开发工具与 IP 许可 (部分需商业许可) 是隐性成本；专业 FPGA 开发人力成本高。; 节省项: 特定负载下替代多个高功耗 CPU 核 (5G PHY、转码、HFT)，降低整机功耗与延迟；云上按需实例避免闲置硬件投入。

**成熟度与社区支持**

Intel: 生态最完整: intel-device-plugins-for-kubernetes 活跃维护 (v0.36，随 K8s 节奏发布)，Operator/Helm/OperatorHub 支持，OFS 开源平台 + FlexRAN 覆盖 5G 场景，DFL 驱动在主线上游。; Funky: 学术原型 (SoCC 2025 论文，德国慕尼黑工业大学 dse.in.tum.de 团队)，GitHub 开源供社区评估 [不确定仓库地址]；代表 2025 年云原生 FPGA 编排研究方向，尚未产品化。; 场景生态: 金融 (Silicom/Exegy 等商业方案)、5G (Intel FlexRAN/O-RAN 社区)、视频 (Xilinx Video SDK/FFmpeg)、EDA 与数据库加速均有活跃商业与社区实践。

---

## 9. GPU 加速器（NVIDIA GPU Operator / AMD ROCm 设备插件 / Intel XPU 插件，含 MIG、MPS、Time-Slicing、DRA 调度、GPU 直通等 GPU 共享与虚拟化技术）

**官方文档**: NVIDIA GPU Operator: https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/latest/ ; NVIDIA GPU 共享 (MIG/Time-Slicing/MPS): https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/latest/gpu-sharing.html ; NVIDIA MIG 用户指南: https://docs.nvidia.com/datacenter/tesla/mig-user-guide/ ; NVIDIA 设备插件: https://github.com/NVIDIA/k8s-device-plugin ; NVIDIA GPU Feature Discovery: https://github.com/NVIDIA/gpu-feature-discovery ; NVIDIA DCGM Exporter: https://github.com/NVIDIA/dcgm-exporter ; NVIDIA DRA 驱动: https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/latest/dra-intro-install.html ; GPU Operator + KubeVirt: https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/latest/gpu-operator-kubevirt.html ; AMD ROCm k8s-device-plugin: https://github.com/ROCm/k8s-device-plugin (文档: https://instinct.docs.amd.com/projects/k8s-device-plugin/en/latest/) ; AMD GPU Operator: https://github.com/ROCm/gpu-operator (文档: https://instinct.docs.amd.com/projects/gpu-operator/) ; Intel Device Plugins for Kubernetes: https://github.com/intel/intel-device-plugins-for-kubernetes (GPU 插件: https://intel.github.io/intel-device-plugins-for-kubernetes/cmd/gpu_plugin/README.html) ; K8s 设备插件概念: https://kubernetes.io/docs/concepts/extend-kubernetes/compute-storage-net/device-plugins/ ; K8s DRA: https://kubernetes.io/docs/concepts/scheduling-eviction/dynamic-resource-allocation/

### 硬件规格

**最低配置**

控制面/管理面: 支持 Kubernetes 的 Linux 节点（x86_64 或 arm64），安装 NVIDIA GPU Operator 需 Helm 3；节点需具备离散 GPU（NVIDIA 官方明确仅支持独立 GPU，不支持 Jetson 等嵌入式/集成 GPU）。; NVIDIA 软件栈: Linux 内核可加载 NVIDIA 驱动模块（含 open kernel module），容器运行时为 containerd/CRI-O/Docker，配合 NVIDIA Container Toolkit（nvidia-container-runtime hook）注入 GPU 设备；设备插件作为 DaemonSet 通过 kubelet 设备插件通道注册 nvidia.com/gpu 资源。; AMD 软件栈: 节点预装 ROCm 驱动栈（amdgpu 内核驱动），k8s-device-plugin 以 DaemonSet 运行并注册 amd.com/gpu 资源；要求 Kubernetes v1.18 及以上；文档要求 ROCm-capable AMD GPU 硬件。; Intel 软件栈: Linux 内核内置 i915（旧）或 xe（Xe2+ 新架构）驱动，节点提供 /dev/dri 设备；GPU 设备插件以 DaemonSet 注册 gpu.intel.com/i915 或 gpu.intel.com/xe 资源；支持集成显卡与数据中心独立显卡（Intel Data Center GPU Flex/Max）。; 典型入门 GPU: 推理/边缘场景常用 NVIDIA L4 (24GB, 72W)、A10 (24GB, 150W)、Intel Flex 170 等入门级数据中心卡 [不确定具体型号推荐组合]。

**推荐配置**

训练/生产: NVIDIA HGX 8 卡节点（A100/H100/H200），NVLink/NVSwitch 互联，搭配 Ubuntu 22.04/24.04 LTS 或 RHEL 8-10/Rocky 8-10/SLES 15-16，Kubernetes 1.32-1.36；使用 GPU Operator 全栈（驱动 DaemonSet、设备插件、DCGM exporter、GPU Feature Discovery、MIG Manager、validator）。; AMD 生产: AMD Instinct MI300X 等 ROCm 数据中心 GPU，Ubuntu 22.04/24.04（AMD GPU Operator v1.2+ 官方支持范围），ROCm 驱动与容器镜像配套。; Intel 生产: Intel Data Center GPU Max 系列（推理/训练）或 Flex 系列（媒体/推理），配合 XPU/oneAPI 软件栈。

**生产级配置**

大规模 AI 集群: HGX 8x H100/H200/B200（约 700W/卡以上）液冷或风冷节点，8 卡 NVLink 域 + NVSwitch，节点间 400GbE/InfiniBand (NDR) 网络满足 NCCL 分布式训练带宽；K8s 1.32+ 启用 DRA（v1.35 GA）实现细粒度 GPU 调度与共享，配 Kueue/Volcano 等批调度器承载多租户训练任务。; 高密度共享: 启用 MIG（每 GPU 最多 7 个实例）将 8 卡节点切成最多 56 个 GPU 分片；或 Time-Slicing 实现更高副本数的超卖（无隔离）。; 虚拟化生产: KubeVirt/OpenShift Virtualization + vGPU（NVIDIA vGPU SR-IOV，Ampere 及以上）或 vfio-pci 直通，为虚拟机提供 GPU [不确定大规模混合容器/VM 的实际生产占比]。

### 兼容性

**支持的 K8s 版本范围**

NVIDIA GPU Operator: 官方验证 Kubernetes 1.32-1.36（部分配置 1.33-1.35），并支持 Red Hat OpenShift；NVIDIA DRA 驱动要求 Kubernetes v1.34.2 及以上。; NVIDIA 设备插件: 设备插件机制自 K8s 1.10+ 为稳定功能；NVIDIA k8s-device-plugin 支持较广的 K8s 版本范围 [不确定精确下限]。; AMD: ROCm k8s-device-plugin 要求 Kubernetes v1.18+；AMD GPU Operator v1.2+ 支持 Ubuntu 22.04/24.04 上的 K8s，Red Hat OpenShift 4.18 支持在 v1.2.1 起加入 [不确定具体 K8s minor 版本矩阵]。; Intel: intel-device-plugins 通过 DaemonSet 部署，CDI（Container Device Interface）支持自 Kubernetes 1.28 起 [不确定最低 K8s 版本]。; K8s 上游 DRA: Dynamic Resource Allocation 自 v1.26 引入（Alpha），v1.35 标记为 stable（GA）并默认启用；结构化参数 (structured parameters) 是其核心模型。

**操作系统兼容性**

NVIDIA: Ubuntu 22.04/24.04 LTS、Red Hat CoreOS、RHEL 8/9/10、Rocky Linux 8/9/10、SUSE Linux Enterprise Server 15/16；支持 x86_64 与 arm64（如 Grace Hopper）。; AMD: GPU Operator 当前仅启用 Ubuntu 22.04 与 24.04；ROCm 驱动本身支持 Ubuntu/RHEL/Rocky 等更多发行版 [不确定完整矩阵]。; Intel: 主流 Linux 发行版（Ubuntu、RHEL 系、Fedora 等），依赖内核内置 i915/xe 驱动 [不确定官方认证矩阵]。

**K8s 上游支持阶段**

设备插件机制: Kubernetes 上游 GA（稳定），是 kubelet 扩展资源的标准机制，但模型简单: 资源整体分配、不支持共享与属性选择，正被 DRA 逐步演进替代。; DRA: Kubernetes 上游 GA（v1.35 stable，默认启用），NVIDIA/Intel/AMD 均有 DRA 驱动或规划 [不确定 AMD DRA 驱动成熟度]；NVIDIA DRA 驱动与 GPU Operator 集成（支持 ComputeDomains 多节点 NVLink 域）。; NVIDIA GPU Operator: 厂商（NVIDIA）主导的成熟生产项目，版本化发布（v25.x），与 OpenShift、RKE2、k3s、Canonical、Spectro Palette 等广泛集成。; AMD: k8s-device-plugin 为 ROCm 官方开源项目（社区活跃）；AMD GPU Operator 2024 年推出（v1.0），2025 年迭代至 v1.5，处于快速成长期。; Intel: intel-device-plugins 为 Intel 开源社区项目（非 CNCF），GPU 插件长期维护；XPU 是 Intel 的加速器统一抽象（oneAPI/XPU Manager 生态）[不确定其 K8s 上游状态]。

**生态兼容性矩阵**

调度器: 原生 kube-scheduler（扩展资源/DRA）、Kueue、Volcano、HAMi、Nebuly NOS 等 GPU 共享调度器 [不确定各方案生产成熟度]；NVIDIA DRA 驱动支持 CUDA 时间切片与 MPS 策略。; 监控: NVIDIA DCGM Exporter（Prometheus 指标）、AMD Metrics Exporter（30 秒轮询、ECC 健康检查）、Intel XPU Manager (xpumd)、Grafana 仪表盘。; Operator/发行版: NVIDIA GPU Operator（Helm/Operator Lifecycle Manager）、AMD GPU Operator（Helm）、Intel Helm charts；Red Hat OpenShift、RKE2、k3s、Canonical Kubernetes 均有 GPU 集成文档。; 虚拟化: KubeVirt、OpenShift Virtualization、NVIDIA vGPU (SR-IOV)、vfio-pci 直通、Kata Containers（NVIDIA Sandbox Device Plugin）。; AI 软件栈: PyTorch/TensorFlow/JAX 的 CUDA、ROCm、oneAPI 版本；NVIDIA Triton/vLLM 等推理栈；NCCL/RCCL 集合通信；GPUDirect Storage/NVMe-oF 存储加速。

### 限制与约束

**已知限制**

NVIDIA GPU Operator: 仅支持独立（离散）GPU，不支持 Jetson 等嵌入式平台；升级仅支持同一大版本内或升级到下一大版本；GPUDirect Storage 与 Secure Boot 兼容性受限；KubeVirt vGPU 与部分旧软件版本不兼容。; Time-Slicing: 副本间无内存隔离与故障隔离（共享显存，OOM 可互相影响）；DCGM-Exporter 在启用时无法将指标关联到容器；Operator 忽略 ConfigMap 修改，需手动重启 DaemonSet 生效；请求多个副本不保证获得等比例算力。; MIG: 仅支持最新几代 NVIDIA GPU（Ampere 及以上：A100/H100/H200 等）；MIG 实例计算与显存硬件级隔离，但分片组合有限制（如 A100/H100 每卡最多 7 实例）；MIG 与 vGPU 混用有约束；配置策略（none/single/mixed）复杂。; MPS: 仅共享计算资源（SM），不隔离显存；无 K8s 原生支持，需在容器内通过 CUDA_MPS_PIPE_DIRECTORY 等环境变量启用 MPS 控制守护进程；GPU Operator 25.x 起支持基于 MPS 的共享配置 [不确定细节]。; 设备插件模型: 整体分配、不支持设备共享与属性过滤（无 CEL 选择器）；扩展资源数量限制与调度为尽力而为，易碎片化；设备插件崩溃需 kubelet 重启恢复。; AMD: k8s-device-plugin 无类似 MIG 的硬件分区能力（MI300 系列无 MIG 等价物）[不确定 AMD GIM 在 K8s 的支持状态]；GPU Operator 早期版本仅支持 Ubuntu 22.04/24.04。; Intel: i915/xe 驱动 GPU 主要面向推理/媒体场景，大规模训练生态弱于 CUDA；默认限制每 GPU 仅 1 个容器（-shared-dev-num 可调大）。; GPU 直通: vfio-pci 直通要求主机开启 IOMMU（intel_iommu=on 或 AMD-Vi），BIOS 启用虚拟化与 SR-IOV（新硬件）；直通 GPU 不支持热迁移；GPU 节点无法同时运行容器 GPU 工作负载与 VM GPU 工作负载（GPU Operator 文档明确）。; 预热时间: 大模型 GPU Pod 初始化（权重加载、图编译、KV cache 准备）通常 40-90 秒，70B 级模型完全冷启动可达 7-9 分钟，影响弹性伸缩的响应速度。

### 配置与部署

**配置方式**

NVIDIA（推荐）: GPU Operator（Helm/OLM 声明式）一键部署驱动、设备插件、DCGM exporter、GFD、MIG Manager、validator、Container Toolkit；资源分配支持传统扩展资源 nvidia.com/gpu 或 DRA（ResourceClaim/DeviceClass）；GPU 共享通过 ClusterPolicy 配置 MIG 策略，Time-Slicing 通过 ConfigMap + ClusterPolicy 配置。; NVIDIA（手动）: 手动安装驱动与 Container Toolkit，部署 k8s-device-plugin DaemonSet，Pod 声明 nvidia.com/gpu: 1。; AMD: k8s-device-plugin 以 DaemonSet 部署（kubectl apply），注册 amd.com/gpu；AMD GPU Operator（Helm）管理 ROCm 驱动生命周期、设备插件与 Metrics Exporter；虚拟化场景使用 vfio 模式直通 [不确定配置细节]。; Intel: intel-device-plugins GPU plugin DaemonSet（-shared-dev-num 控制每 GPU 容器数），资源名 gpu.intel.com/i915 或 gpu.intel.com/xe；提供分配策略（balanced/packed/none）。; DRA: 管理员部署 DRA 驱动（如 NVIDIA DRA driver），创建 DeviceClass/ResourceSlice；用户通过 ResourceClaimTemplate 声明 GPU 请求（可含 CEL 选择器、共享、ComputeDomains 等参数）。

**部署位置与环境**

主要场景: 裸金属数据中心（HGX/整机柜）、公有云托管节点池（EKS/GKE/AKS 的 GPU 实例）、私有云与 OpenShift；边缘推理（Intel Flex/Arc、NVIDIA L4）[不确定边缘大规模案例]；k3s/RKE2 轻量发行版支持 GPU（RKE2 官方 GPU Operator 文档）。

**虚拟化兼容性**

GPU 直通 (Passthrough): vfio-pci 绑定物理 GPU 直通给 Pod/VM（KubeVirt、OpenShift Virtualization、Kata）；要求 IOMMU 开启；VM 内需自行安装厂商驱动（GPU Operator 不自动安装客户机驱动）；直通 GPU 无法热迁移。; vGPU (SR-IOV): NVIDIA vGPU 要求 Ampere 及以上架构，需私有 vGPU Manager 镜像；GPU Operator 部署 vGPU Manager + Device Manager，可动态创建分片 vGPU（含 MIG-backed 配置）；Intel 亦支持 SR-IOV vGPU [不确定成熟度]。; 限制: 一个 GPU 节点只能服务一种 GPU 工作负载类型（容器或 VM），不可混部；设备需在 KubeVirt CR 中显式授权；SR-IOV 需 BIOS 启用并配置 VF。

### 性能特征

**基准性能数据**

GPU 预热/冷启动: 大模型初始化（权重传输 8-45 秒 + 图编译 12-30 秒 + KV cache 5-10 秒）合计 40-90 秒；70B 模型完全冷启动（未缓存镜像）7-9 分钟，缓存镜像后约 85 秒；用户态 checkpoint（含显存）恢复可降至 2-5 秒（小模型）或约 40 秒（大模型）；图缓存可省 10-30 秒。; 通信: H100 NVLink 900GB/s（NVSwitch 全互联），节点间 NDR InfiniBand 400Gbps；NCCL 多节点扩展效率是分布式训练关键指标 [不确定具体 benchmark 数字]。

### 安全

**安全特性**

MIG 隔离: 硬件级计算与显存隔离（memory and fault isolation at hardware layer），不同 MIG 实例间故障隔离。; NVIDIA 机密计算: Hopper 及更新架构（H100/H200/B200）支持 GPU 机密计算（Confidential Computing），GPU Operator 提供相关支持 [不确定在 K8s 中的完整配置细节]。; 虚拟化隔离: vGPU/SR-IOV 提供设备级隔离；vfio-pci 直通将设备安全暴露给 VM（依赖 IOMMU）；Kata Containers 通过沙箱隔离。; 容器安全: 设备插件/GDU 组件以 DaemonSet 运行，Container Toolkit 通过 CDI 注入设备；GPU 节点建议配合节点专用 (dedicated) 污点与配额。

### 运维与生命周期

**可观测性支持**

NVIDIA: DCGM Exporter 暴露 Prometheus 指标（GPU 利用率 DCGM_FI_DEV_GPU_UTIL、显存利用率、温度 DCGM_FI_DEV_GPU_TEMP、功耗 DCGM_FI_DEV_POWER_USAGE、显存容量/使用等），Grafana 官方仪表盘；GPU Operator 提供 validator 与节点就绪状态检查；MIG 模式下可按实例采集指标。; AMD: AMD GPU Operator 内置 Metrics Exporter（每 30 秒轮询硬件健康，检测 ECC 错误，节点打标签如 metricsexporter.amd.com.gpu.0.state=unhealthy）[不确定 Prometheus 指标清单]。; Intel: XPU Manager (xpumd) 提供设备遥测 [不确定 Prometheus 导出方式]；设备插件可从 xpumd 获取健康数据。

### 经济性

**总拥有成本 (TCO)**

软件成本: NVIDIA GPU Operator、CUDA、ROCm、Intel oneAPI 均免费开源；可选商业订阅（NVIDIA AI Enterprise）；托管平台（EKS/GKE/AKS、OpenShift）另计费。; 节省手段: MIG/Time-Slicing/MPS 提升利用率（将闲置 GPU 切片给推理/小任务）；DRA 精确按显存/算力分配减少浪费；GPU 预热优化（checkpoint、图缓存、常驻副本）降低弹性伸缩成本 [不确定量化数据]。

**成熟度与社区支持**

NVIDIA: 最成熟: GPU Operator 为事实标准（支持 K8s 1.32-1.36、OpenShift/RKE2/k3s 等），DCGM/GFD/NRI 生态完善，文档与社区活跃；DRA 驱动与上游 K8s GA 同步。; AMD: ROCm 开源生态快速增长，k8s-device-plugin 社区活跃；AMD GPU Operator 2024-2025 快速迭代（v1.2→v1.5），OpenShift 支持推进中，生产案例（云厂商 MI300 集群）增加但整体少于 NVIDIA。; 上游: DRA 为 Kubernetes 官方 GA 能力（v1.35），NVIDIA/Intel 已提供 DRA 驱动，标志 GPU 调度从设备插件向 DRA 演进，社区共识度高。

---

## 10. Intel DSA (Data Streaming Accelerator) 与 Intel IAA (In-Memory Analytics Accelerator) —— 英特尔数据流加速器与内存分析加速器

**官方文档**: Intel DSA 产品页: https://www.intel.com/content/www/us/en/products/details/processors/xeon/features/data-streaming-accelerator.html ; Intel DSA 用户指南: https://cdrdv2-public.intel.com/759709/353216-004-intel-data-streaming-accelerator-user-guide.pdf ; Intel DSA 架构规范: https://cdrdv2.intel.com/v1/dl/getContent/671116 ; Intel IAA 用户指南: https://cdrdv2-public.intel.com/780887/354834_IAA_UserGuide_June23.pdf ; accel-config 工具: https://github.com/intel/idxd-config ; Intel Device Plugins for Kubernetes (DSA 插件): https://github.com/intel/intel-device-plugins-for-kubernetes/blob/main/cmd/dsa_plugin/README.md ; IAA 插件: https://github.com/intel/intel-device-plugins-for-kubernetes/blob/main/cmd/iaa_plugin/README.md ; Intel Device Plugins Operator: https://operatorhub.io/operator/intel-device-plugins-operator ; Intel QPL (Query Processing Library): https://intel.github.io/qpl/documentation/introduction_docs/introduction.html ; DPDK IDXD dmadev 驱动: https://doc.dpdk.org/guides/dmadevs/idxd.html ; SPDK IDXD 驱动: https://spdk.io/doc/idxd.html ; 内核 iaa_crypto 压缩驱动文档: https://docs.kernel.org/driver-api/crypto/iaa/iaa-crypto.html ; StarlingX DSA 集成文档: https://docs.starlingx.io/r/stx.10.0/node_management/kubernetes/data-streaming-accelerator-db88a67c930c.html ; DSA 量化研究 (arXiv): https://arxiv.org/html/2305.02480v5 ; 安全公告 CVE-2024-21823: https://seclists.org/oss-sec/2024/q2/242 ; Intel 官方安全指导 (DSA/IAA 错误报告): https://www.intel.com/content/www/us/en/developer/articles/technical/software-security-guidance/advisory-guidance/intel-dsa-and-intel-iaa-error-reporting.html

### 硬件规格

**最低配置**

软件: Linux 内核 5.18+（DSA 专用 WQ 自 5.6、共享 WQ 自 5.18；IAA 用户态提交需 ENQCMD/PASID，要求 5.18+；内核压缩驱动 iaa_crypto 需 6.8+）；accel-config (idxd-config) 用户态配置工具；x86_64 Linux（DSA/IAA 硬件加速仅支持 Linux，Windows 不支持）。; K8s 资源: Kubernetes 1.12+（Device Plugin v1beta1 稳定 API）；设备插件以 DaemonSet 运行需节点 root 权限；DPDK/vfio 场景需 vfio-pci 驱动并加 disable_denylist=1 参数（PCI ID 0b25 等）。

**推荐配置**

硬件: 第 4/5 代 Xeon Scalable 或 Xeon 6 (Granite Rapids)，按负载规划每节点 WQ：数据搬运用 DSA（dedicated/shared WQ），压缩与分析用 IAA；生产环境建议 NUMA 亲和（Pod 与其使用的 WQ/设备同 NUMA 节点），并预留内存带宽。; 软件: 内核 6.x+（提供更完整的 SVM/SWQ/复位支持）；accel-config 配置 engine/group/WQ；应用栈：QPL（IAA 压缩/分析）、DML（DSA 数据搬运库）、SPDK/DPDK dmadev（idxd）；部署 Intel Device Plugins Operator + dsa_plugin/iaa_plugin + intel-idxd-config-initcontainer 自动配置 WQ。; 集群: OpenShift 或主流 K8s 发行版（RHEL 8.7/9.1+、Ubuntu 22.10+、SLES 15 SP4+、CentOS Stream）；大数据/AI 管道场景结合 Spark/Hadoop/Hive、ClickHouse、RocksDB、Redis 等使用 IAA 压缩，存储/网络场景用 DSA 卸载数据搬运。

**生产级配置**

大规模场景: 双路裸金属集群，每节点 2-8 个 DSA/IAA 设备；按业务划分 dedicated（独占）与 shared（共享）WQ，共享 WQ 提高设备利用率；与 QAT/DLB/GPU 等其他加速器混部由 Intel Device Plugins Operator 统一编排。; 大数据/AI 管道: Spark/Hadoop/ClickHouse/RocksDB 集群 + IAA 压缩卸载（QPL 集成），DSA 用于分布式训练集合通信（libfabric/oneCCL，Intel 报告 BERT 预训练集合通信最多加速 3.3 倍）与存储数据面（SPDK）。; 网络/存储数据面: DPDK vhost 报文拷贝、SPDK 存储路径卸载 DSA（Intel DPDK 报文拷贝技术指南）；zswap/zram 场景启用 iaa_crypto 卸载内核页压缩。

### 兼容性

**支持的 K8s 版本范围**

Device Plugin 方式: Kubernetes Device Plugin 框架 v1beta1 自 K8s 1.12 起稳定可用；intel-device-plugins-for-kubernetes 最新版本 v0.36.0 对齐 K8s 1.36，较老版本插件因使用稳定 v1 API 通常可运行于更新集群。; DRA 方式: K8s Dynamic Resource Allocation (DRA) 自 1.26/1.27 引入并持续演进；Intel 的 DRA 资源驱动项目 (intel-resource-drivers-for-kubernetes，Beta/非生产) 目前仅覆盖 GPU/Gaudi/QAT，DSA/IAA 尚无 DRA 驱动，仍使用传统 Device Plugin 方式 [不确定后续 DRA 支持计划]。

**操作系统兼容性**

支持: Linux x86_64：RHEL 8.7/9.1+（RHEL 9.4 起对 DSA 提供完整支持 [不确定细节]）、SLES 15 SP4+、Ubuntu 22.10+、CentOS Stream 8/9、Debian/Fedora 系。; 内核要求: 内核 5.18+（DSA DWQ 5.6+、SWQ 5.18+；IAA 用户态需 ENQCMD，官方示例用 5.18+，ClickHouse 指南推荐 6.0+；iaa_crypto 需 6.8+）；驱动为内嵌 idxd 驱动，需 CONFIG_INTEL_IDXD、CONFIG_INTEL_IDXD_SVM、CONFIG_INTEL_IOMMU_SVM，引导参数 intel_iommu=on,sm_on。; 不支持: Windows Server 节点不支持（软件栈为 Linux-only，QPL 文档明确硬件加速不支持 Windows）；非 Intel x86 平台（AMD/ARM）无此硬件。

**K8s 上游支持阶段**

Device Plugin: K8s 上游 GA（v1 稳定 API），DSA/IAA 通过通用设备插件框架接入，无独立上游组件。; Intel 插件: intel-device-plugins-for-kubernetes 为 Intel 开源社区项目（Apache-2.0），长期维护、版本化发布（对齐每个 K8s minor 版本），生产就绪度高；Intel Device Plugins Operator 通过 OperatorHub 分发且为 Red Hat 认证容器镜像。; DRA: K8s 上游 DRA 仍处于 Beta 演进阶段；DSA/IAA 未接入 DRA（Intel DRA 资源驱动仅覆盖 GPU/Gaudi/QAT）。

**生态兼容性矩阵**

Operator: Intel Device Plugins Operator（Red Hat 认证、OperatorHub 可安装）、intel-idxd-config-initcontainer（通过 accel-config 自动配置 DSA/IAA 设备与 WQ 的 init 容器镜像）、ProvisioningConfig CRD。; 应用生态: QPL（deflate 压缩/解压 + scan/extract/select/expand 分析过滤 + CRC-64）、Intel DML（数据搬运库，支持 DSA）、ISA-L、DPDK dmadev (idxd) 与 compressdev、SPDK idxd 加速、内核 iaa_crypto（供 zswap/zram 使用 deflate-iaa）、RocksDB IAA 压缩插件（iaa_plugin-rocksdb）、ClickHouse/Redis 压缩集成、libfabric/oneCCL（MPI 集合通信）、dsa-perf-micros 性能微基准。; CNI/网络: 无强制 CNI 依赖；DPDK 场景常与 SR-IOV CNI + Multus 配合使用。

### 限制与约束

**已知限制**

无 SR-IOV: DSA/IAA 不是 SR-IOV 设备、无 VF 概念，资源共享通过 WQ（dedicated/shared）抽象实现；虚拟化场景只能整设备 vfio-pci 直通。; 小数据收益为负: 同步卸载在数据小于约 4-10 KB 时与 CPU memcpy 相当或更慢（提交/完成开销），异步批量约 256 字节起才稳定获益（arXiv 量化研究）。; 吞吐上限: 单 DSA 设备峰值吞吐约 30 GB/s（受 I/O fabric 限制）；访问 CXL 内存时因延迟更高性能下降；多设备大块传输会竞争系统内存带宽并可能压垮 DDIO 缓存分区。; IAA 压缩率: IAA deflate 压缩率低于软件高等级 zstd（与 zlib 低等级相当）[不确定具体对比数值]；QPL 的 deflate 历史窗口限制为 4 KB；Huffman-only 模式压缩率更低。; 共享 WQ 前提: SWQ 在极少量并发线程下吞吐差（arXiv 指出最少线程场景下共享队列不如专用队列）；SWQ 需要内核 5.18+。; 安全漏洞: CVE-2024-21823：DSA/IAA 硬件逻辑存在不安全的反同步（insecure de-synchronization），本地授权用户可导致拒绝服务，影响部分第 4/5 代 Xeon 处理器，需更新内核（限制非受信直连设备访问）、相关软件库（Intel DSA Transparent Offload Library 等）及微码。; 虚拟化限制: 嵌套虚拟化不支持；VM 热迁移对直通设备基本不可用。

**固件与驱动依赖**

内核 idxd 驱动（DSA/IAA 共用）：DWQ 自 5.6、SWQ 自 5.18；SVM/PASID 需 CONFIG_INTEL_IOMMU_SVM 与 intel_iommu=on,sm_on；iaa_crypto 需 6.8+ 且 IAA WQ 须配置为内核模式（driver_name=crypto）；用户态工具 accel-config/idxd-config；BIOS 需启用 VT-d、VMX、中断重映射；vfio-pci 场景需 disable_denylist=1；固件/微码随平台 BIOS 更新（CVE-2024-21823 修复依赖微码更新）；节点重启后 WQ 配置需重新应用（init 容器幂等处理）。

### 配置与部署

**配置方式**

Device Plugin（主要）: dsa_plugin/iaa_plugin 以 DaemonSet 运行，发现 DSA/IAA 工作队列并上报为节点资源：dsa.intel.com/wq-user-dedicated、dsa.intel.com/wq-user-shared、iaa.intel.com/wq-user-dedicated、iaa.intel.com/wq-user-shared；使用 vfio-pci 驱动时 DSA 另上报 dsa.intel.com/vfio；Pod 通过 limits 声明资源获取对应 WQ。; Operator（推荐）: 安装 Intel Device Plugins Operator，通过 CRD/ProvisioningConfig 声明式配置 DSA/IAA 的驱动、每设备 WQ 数、dedicated/shared 模式与分配策略。; 自动初始化: intel-idxd-config-initcontainer 使用 accel-config 按模板（demo/dsa.conf、demo/iaa.conf）在节点启动时自动创建 engine/group/WQ（默认 1 engine / 1 group / 1 user-dedicated WQ），可通过 ConfigMap 自定义或按节点名提供节点级配置。

**配置示例**

部署：kubectl apply -k https://github.com/intel/intel-device-plugins-for-kubernetes/deployments/dsa_plugin?ref=<RELEASE_VERSION>（IAA 同理替换为 deployments/iaa_plugin；自动配置用 overlays/dsa_initcontainer 或 overlays/iaa_initcontainer）；Pod 资源示例：resources.limits: { dsa.intel.com/wq-user-dedicated: 1 }、{ dsa.intel.com/wq-user-shared: 1 } 或 { iaa.intel.com/wq-user-shared: 1 }；accel-config 命令示例：accel-config config-wq --group-id=0 --mode=dedicated --type=user --name=wq-user-dedicated dsa0/wq0.0；完整 JSON 配置模板见 https://github.com/intel/intel-device-plugins-for-kubernetes/blob/main/demo/dsa.conf。

**部署位置与环境**

裸金属（主要）: 数据中心裸金属 K8s/OpenShift/StarlingX 集群，DSA 用于存储/网络数据面（SPDK、DPDK vhost）与分布式训练集合通信，IAA 用于数据库/大数据/内存压缩（ClickHouse、RocksDB、Spark、zswap）。; 边缘: Xeon 6 SoC 等边缘平台可提供 DSA/IAA [不确定具体型号支持矩阵]；电信/边缘 NFV 场景可与 DPDK 结合。

**虚拟化兼容性**

直通机制: 无 SR-IOV；VM 内使用需整设备 vfio-pci 直通（依赖 IOMMU/PASID）；容器内 DPDK 场景同样使用 vfio-pci（需 disable_denylist=1，PCI ID 0b25 等）。; 限制: 嵌套虚拟化不支持；VM 热迁移对直通设备不可用；同一设备直通后主机侧与 VM 侧资源互斥，共享需依赖 WQ 配置与 mdev 演进 [不确定 mdev 支持状态]。

### 性能特征

**基准性能数据**

DSA 数据搬运: arXiv 量化研究（2305.02480）：DSA 吞吐约为旧版 CBDMA 的 2.1 倍；单设备峰值约 30 GB/s（受 I/O fabric 限制）；同步卸载与 CPU memcpy 相当的分界点约 4-10 KB，异步批量约 256 字节起即可获益；BERT 预训练集合通信最多加速 3.3 倍；DPDK vhost 报文拷贝、SPDK、libfabric/MPI 等场景有公开收益数据。; IAA 压缩: LWN 公布的内核 iaa_crypto 补丁微基准（小块数据）：IAA 同步压缩 3,177 ns / 解压 2,235 ns，异步中断模式 6,847 ns / 5,840 ns，软件 deflate 压缩 108,978 ns / 解压 14,485 ns（同步压缩约快 34 倍）；Intel 指南：RocksDB 场景 IAA 吞吐优于软件 zstd、压缩率优于 LZ4；ClickHouse Star Schema Benchmark 测试有性能提升 [不确定具体数字]。; zswap: Intel 白皮书：zswap + IAA（iaa_crypto）可降低内核页压缩的 CPU 开销并改善延迟 [不确定具体百分比]。

### 安全

**安全特性**

定位: DSA/IAA 是数据搬运/压缩加速器而非信任根，不提供 TPM 等效的密钥保护；数据加密与密钥管理由上层软件负责。; 隔离机制: 依赖 IOMMU/PASID/SVM 进行 DMA 地址隔离与用户态提交隔离；WQ 由内核 idxd 驱动管理；CVE-2024-21823（硬件逻辑不安全的反同步）可被本地授权用户利用造成拒绝服务，影响部分第 4/5 代 Xeon，需内核、软件库与微码协同更新；Intel 官方发布 DSA/IAA 错误报告（error reporting）安全指导。; 平台安全: 与 Secure Boot/TPM 无直接关联；固件/微码随平台 BIOS 更新管理 [不确定细节]。

### 运维与生命周期

### 经济性

**总拥有成本 (TCO)**

硬件成本: DSA/IAA 集成于 Xeon CPU 封装内，无独立板卡采购成本（对比 QAT 独立板卡方案），是主要经济性优势；硬件成本随 CPU 采购一次性支付。; 运行成本: 无独立功耗项（计入 CPU TDP），不增加散热/机架功耗预算；软件栈（accel-config、QPL、DML、设备插件）全部开源免费。; 适用性判断: 对第 4 代及以上 Xeon 用户几乎零边际成本，数据搬运/压缩密集场景普遍值得启用；需投入 WQ 配置与 NUMA 亲和调优成本，小数据/低并发场景收益有限。

---

## 11. Intel QAT (QuickAssist Technology) 英特尔快速辅助技术

**官方文档**: Intel QAT 官方介绍: https://www.intel.com/content/www/us/en/products/docs/accelerator-engines/what-is-intel-qat.html ; Intel QAT 软件文档 (qatlib/QATzip): https://intel.github.io/quickassist/ ; Intel Device Plugins for Kubernetes (qatplugin): https://intel.github.io/intel-device-plugins-for-kubernetes/ 与 https://github.com/intel/intel-device-plugins-for-kubernetes/blob/main/cmd/qat_plugin/README.md ; Intel Device Plugins Operator: https://www.intel.com/content/www/us/en/developer/articles/technical/device-plugins-operator.html ; OperatorHub: https://operatorhub.io/operator/intel-device-plugins-operator ; Red Hat 认证目录 (Intel Device Plugins Operator Certified): https://catalog.redhat.com/software/container-stacks/detail/61e9f2d7b9cdd99018fc5736 ; QAT OpenSSL Engine/Provider: https://github.com/intel/qat_engine ; QATzip: https://github.com/intel/qatzip ; DPDK QAT Crypto PMD: https://doc.dpdk.org/guides/cryptodevs/qat.html ; K8s 官方博客 (QAT 加速 Ingress TLS 终结): https://kubernetes.io/blog/2019/04/24/hardware-accelerated-ssl/tls-termination-in-ingress-controllers-using-kubernetes-device-plugins-and-runtimeclass/ ; Ceph QAT 加速: https://docs.ceph.com/en/latest/radosgw/qat-accel/ ; Intel Envoy TLS 加速文章: https://www.intel.com/content/www/us/en/developer/articles/technical/envoy-tls-acceleration-with-quickassist-technology.html

### 硬件规格

**最低配置**

硬件: 任意集成或独立 QAT 加速器的 Intel 平台：第 2-6 代 Xeon Scalable/D 系列集成 QAT（V62x、V4xxx、QAT Gen6）或独立 QAT 板卡（C62x、C4xxx 等）；服务器需支持 VT-d（内核参数 intel_iommu=on）与 SR-IOV（BIOS 开启）以便向容器分配 VF。; 软件: Linux 内核 5.9+（内嵌 qat_c62x/qat_4xxx 驱动或 Intel 外部驱动 + qatlib 用户态库）；Kubernetes 1.12+（Device Plugin v1beta1 稳定 API 可用，现代版本对齐上游）；x86_64 Linux 节点（QAT 软件栈为 Linux-only）。; K8s 资源: 节点需 root 权限运行 kubelet 设备插件；使用 DPDK 场景需分配大页内存（hugepages）且 CPU Manager 为 static 模式。

**推荐配置**

硬件: 第 4/5/6 代 Xeon Scalable 集成 QAT（V62x/QAT Gen4/Gen6）或 QAT 4xxx 系列板卡（C4xxx），PCIe Gen3/Gen4 槽位；按需配置每 PF 多个 VF（SR-IOV），生产环境建议每节点规划 NUMA 亲和（QAT 与使用它的 Pod 同 NUMA 节点）。; 软件: 内核 6.x+（较新内核提供更完整的 Gen4/Gen6 服务与复位支持）；qatlib + qat_engine（OpenSSL 3.x Provider 方式）/ QATzip / DPDK QAT PMD；部署 Intel Device Plugins Operator + qatplugin（声明式管理 VF 创建与资源上报）。; 集群: OpenShift 或主流 K8s 发行版（RHEL 8/9/10、Ubuntu、SLES 等）；高吞吐 TLS 场景建议与高性能 Ingress（NGINX/HAProxy/Envoy）结合，配合 Multus/SR-IOV CNI 实现数据面加速。

**生产级配置**

大规模场景: 多节点裸金属集群 + 每节点 1-N 个 QAT 设备（集成或板卡），按服务类型拆分 VF（对称加密 sym、非对称加密 asym、压缩 comp、dcc），Gen4 单设备最多 2 种服务组合、Gen6 最多 3 种；与 Intel DSA/IAA/DLB 等其他加速器混部由 Intel Device Plugins Operator 统一编排。; 电信/边缘 NFV: OpenShift + SR-IOV + DPDK QAT PMD 用于 vRAN/UPF/IPsec 网关（Intel 与 Red Hat 联合方案）；边缘节点可用 Xeon D 集成 QAT。; 云厂商实例: 阿里云等基于 Intel 实例提供 QAT 加速（Intel 发布过阿里云 HAProxy+QAT 性能指南）[不确定云上具体实例类型与开放范围]。

### 兼容性

**支持的 K8s 版本范围**

Device Plugin 方式: Kubernetes Device Plugin 框架 v1beta1 自 K8s 1.12 起稳定可用（Intel 官方演示基于 K8s 1.12）；intel-device-plugins-for-kubernetes 最新版本 v0.36.0 对齐 K8s 1.36，较老版本插件因使用稳定 v1 API 通常可运行于更新集群（CRD 可能略有差异）。

**操作系统兼容性**

支持: Linux x86_64：RHEL 8/9/10、Rocky Linux、Ubuntu 22.04/24.04 LTS、Debian、Fedora、SLES、CentOS 系；OpenShift/RKE2 等发行版可用。; 内核要求: Linux 内核 5.9+（较旧硬件在新内核上需 vfio-pci disable_denylist=1）；不同服务/复位特性对内核版本有最低要求（README 提及 6.0/6.8/6.16 等版本门槛）；驱动为内嵌 qat_c62x/qat_4xxx/qat_420xx 或 Intel 外部驱动 + qatlib。; 不支持: Windows Server 节点不支持（QAT 驱动与容器生态为 Linux-only）；非 Intel x86 平台（ARM/AMD）无 QAT 硬件。

**K8s 上游支持阶段**

Device Plugin: K8s 上游 GA（v1 稳定 API），QAT 通过通用设备插件框架接入，无独立上游组件。; Intel 插件: intel-device-plugins-for-kubernetes 为 Intel 开源社区项目（Apache-2.0），长期维护、版本化发布（对齐每个 K8s minor 版本），生产就绪度高；Intel Device Plugins Operator 通过 OperatorHub 分发且为 Red Hat 认证容器镜像。

**生态兼容性矩阵**

Operator: Intel Device Plugins Operator（Red Hat 认证、OperatorHub 可安装）、intel-qat-initcontainer（自动配置 VF 的 init 容器镜像）、Intel QAT Engine 容器镜像。; 应用生态: NGINX/HAProxy（通过 qat_engine 异步卸载 RSA/ECDSA 握手）、Envoy（private_key_provider: qat 异步密钥提供）、OpenSSL 3.x Provider、strongSwan/IPsec（DPDK 或内核 bulk crypto）、Ceph RGW（加密 AES-256-CBC + zlib 压缩加速）、DPDK QAT PMD（对称/非对称/压缩）、QATzip（deflate/zstd 压缩）。; CNI/网络: 无强制 CNI 依赖；DPDK 场景常与 SR-IOV CNI + Multus + SR-IOV Network Device Plugin 配合。

### 限制与约束

**已知限制**

服务组合限制: Gen4（4xxx）单设备最多支持 2 种服务组合，Gen6 最多 3 种；dcc（数据压缩/解压）服务不允许与其它服务组合；组合由配置的 ServicesEnabled 决定。; DPDK 资源互斥: 每个 QAT VF 同时只能被一个 DPDK 进程使用；queue-pair 在 Intel CPU 上线程安全但 queue 本身不是，TX/RX 需独立线程。; 软件栈限制: OpenSSL Engine 接口在 OpenSSL 4.0 中被移除，必须迁移到 Provider；qat_engine 依赖 OpenSSL 异步 (async) 模型，应用需支持异步回调才能获得收益；igb_uio 驱动要求容器具备 SYS_ADMIN 权限。; 内核/驱动兼容: 旧内核（5.9-6.x 区间）对 Gen4/Gen6 设备存在 denylist 问题，需 vfio-pci disable_denylist=1；Intel 旧外部驱动（adf）与 Gen4 硬件不兼容，必须使用内嵌驱动或新 qatlib。; 虚拟化限制: 嵌套虚拟化下无法使用 QAT；VM 热迁移对 VF 直通设备基本不可用。

### 配置与部署

**配置方式**

Device Plugin（主要）: 通过 qatplugin（DaemonSet）向 kubelet 上报扩展资源，资源名：Gen4 之前为 qat.intel.com/generic，Gen4 及以后为 qat.intel.com/<服务名>（如 qat.intel.com/sym、qat.intel.com/asym、qat.intel.com/comp 等）；Pod 通过 limits 声明资源，容器内获得对应 VF 设备。; Operator（推荐）: 安装 Intel Device Plugins Operator（Red Hat 认证），通过 CRD 声明式配置 qatplugin 的驱动、每 PF VF 数、分配策略与 ServicesEnabled 服务组合。; 自动初始化: 使用 intel-qat-initcontainer + ConfigMap 在节点上自动创建 SR-IOV VF 并配置服务（ServicesEnabled=<值>）；DPDK 场景还需绑定 vfio-pci、分配大页、CPU Manager static 模式。; DRA（实验）: 可选使用 K8s DRA 资源声明方式 [不确定 QAT DRA 支持成熟度]；QAT 主要采用传统 Device Plugin 方式。

**配置示例**

典型部署：kubectl apply -k https://github.com/intel/intel-device-plugins-for-kubernetes/deployments/qat_plugin/base（或通过 OperatorHub 安装 Operator 后创建 DevicePlugin CRD 实例）；Pod 示例：resources.limits: { qat.intel.com/generic: 1 }（Gen4 前）或 { qat.intel.com/sym: 1 }（Gen4+）；init 容器 ConfigMap 示例：QAT_SERVICES: sym,asym / ServicesEnabled=sym。完整示例见 qat_plugin README 与 Intel Device Plugins 文档。

**部署位置与环境**

裸金属（主要）: 数据中心裸金属 K8s/OpenShift 集群，节点直连 QAT 设备（集成或板卡），生产 TLS 终结/IPsec/压缩场景。; 公有云: 阿里云等提供基于 Intel Xeon 的实例支持 QAT（Intel 发布过阿里云 HAProxy+QAT 性能指南）[不确定各云厂商对 QAT 透传/直通的具体支持范围]；多数公有云普通实例无 QAT 设备。; 边缘: Xeon D 系列集成 QAT 适合边缘/电信 NFV（OpenShift 边缘、vRAN）；虚拟化环境：VM 内使用需 SR-IOV VF 直通（VFIO）。; 不支持: 无 QAT 硬件的云虚机、Windows 节点、非 Intel 平台。

**虚拟化兼容性**

SR-IOV/VFIO: 核心机制：QAT PF 划分 VF，通过 vfio-pci 直通给 Pod/VM（Lenovo 等发布过在 Linux VM 中使用 QAT SR-IOV 的配置指南）；容器无需特权即可使用 vfio 直通设备。; 限制: 嵌套虚拟化不支持；VM 热迁移对 VF 直通设备不可用；每 VF 仅能被单个进程/容器使用；vSphere/NSX 场景 QAT 主要作为 NSX Edge 主机的 IPsec 加速（bare metal 部署）。

### 性能特征

**基准性能数据**

TLS 终结 (Envoy): Intel 官方测试（4 代 Xeon 8480+ 预生产平台，RSA-2048）：QAT 卸载后 4 线程吞吐提升 5 倍，四核 CPU 负载下降 30 个百分点（约 76% 下降），峰值约 2,000 次完整握手/秒（每连接全新握手）；NGINX/HAProxy 通过 qat_engine 异步卸载获得类似收益。; 压缩 (QATzip): Intel 官方引用企业实测：硬件压缩比纯软件快 9 倍至 137 倍；SQL Server 备份场景提升 2.56 倍、每瓦性能提升 1.64 倍；QATzip 支持 deflate/zstd（zstd 经外部插件），适用于大数据、存储与 API 网关响应压缩。; IPsec: QAT 提供 IPsec bulk crypto 卸载（对称加解密+哈希），VMware NSX Edge 等网关在裸金属主机上使用 QAT 加速 IPsec VPN；DPDK QAT PMD 支持线速级卸载 [不确定具体 Gbps 数字]。

### 安全

**安全特性**

隔离机制: SR-IOV VF 提供设备级隔离，Pod 通过 vfio-pci 直通独占 VF；DPDK 推荐 vfio-pci（替代已弃用的 UIO）以提升安全性；固件由驱动加载并校验 [不确定固件签名细节]。; 相关安全能力: 与平台 Secure Boot/TPM 无直接关联；在 OpenShift 上可结合 FIPS 模式使用 [不确定组合细节]。

### 运维与生命周期

### 经济性

**总拥有成本 (TCO)**

节省项: 卸载 TLS 握手（RSA/ECDSA）可释放大量 CPU 核（Intel 测试 4 核 CPU 负载降约 76%），降低或替代硬件 TLS 负载均衡器；压缩卸载提升存储/网关吞吐；按每瓦性能提升 1.64x 计，可降低服务器采购与功耗成本。; 适用性判断: 集成 QAT（Xeon 内置）几乎零边际成本，TLS 终结/IPsec/压缩场景普遍值得启用；独立板卡需评估带宽需求与回本周期，高并发 TLS 或大流量压缩场景收益明显。

**成熟度与社区支持**

社区活跃度: Intel 主导并持续投入：intel-device-plugins-for-kubernetes 活跃维护（版本对齐每个 K8s minor 版本，v0.36.0 支持 K8s 1.36）、qat_engine/qatzip/qatlib 均为活跃开源项目；DPDK 社区集成 QAT PMD。; 厂商与生态: Red Hat（OpenShift 认证 Operator、RHEL 内嵌驱动）、VMware（NSX Edge IPsec）、Ceph、阿里云等云厂商；NGINX/HAProxy/Envoy/strongSwan 等主流代理与安全软件均支持 QAT 卸载，生态成熟度高。; 生命周期: 硬件从 QAT 1.x（DH895x/8920/8955）演进至 2.0（C62x/V62x）、Gen4（4xxx）、Gen6（Xeon 6），Intel 保持前向软件兼容（qatlib 同时支持集成与独立设备）。

---

## 12. 内存管理

### 硬件规格

**最低配置**

控制面节点: 2 GiB 内存，2 CPU; 工作节点: 2 GiB 内存，1 CPU; 说明: 官方文档要求每台机器至少 2 GiB 内存，控制面节点至少 2 CPU

**推荐配置**

控制面节点: 4-8 GiB 内存，4-8 CPU（小规模集群）；8-16 GiB 内存，8-16 CPU（中等规模集群）; 工作节点: 4-8 GiB 内存，2-4 CPU（小规模集群）；8-32 GiB 内存，4-8 CPU（中等规模集群）; 说明: 社区推荐配置，实际需求取决于集群规模和工作负载类型

**生产级配置**

控制面节点: 16-64 GiB 内存，8-16 CPU（大规模集群 500-5000 节点），推荐 3 副本高可用部署，etcd 建议独立节点部署; 工作节点: 32-128 GiB 内存，8-32 CPU（根据工作负载密度调整），建议预留充足内存余量以应对节点压力驱逐; 说明: 大规模集群建议控制面与工作节点分离，etcd 专用节点建议 8-16 GiB 内存，SSD 存储

### 兼容性

**支持的 K8s 版本范围**

Kubernetes v1.0+ 基础内存管理；v1.22 Memory QoS Alpha；v1.25 cgroup v2 GA；v1.27 Memory QoS Beta、Topology Manager GA；v1.32 Memory Manager (Static) GA；v1.35 cgroup v1 弃用；v1.36 Tiered Memory Protection（Memory QoS 重大更新）

**操作系统兼容性**

Linux（主要支持，全功能）；Windows Server 2019/2022（部分支持，Memory Manager 仅 BestEffort Policy，Alpha 阶段）；cgroup v2 需要 Linux 内核 5.8+（Memory QoS 需要 5.9+）

**K8s 上游支持阶段**

内存请求与限制（GA，v1.0+）；节点压力驱逐（GA，v1.0+）；HugePages（GA，v1.14+）；cgroup v2（GA，v1.25+）；Topology Manager（GA，v1.27+）；Memory QoS（Beta，v1.27，v1.36 重大更新）；Memory Manager Static Policy（GA，v1.32）；Tiered Memory Protection（v1.36 新增，通过 MemoryReservationPolicy 配置）；cgroup v1 弃用（v1.35+）

**生态兼容性矩阵**

兼容 containerd v1.4+、CRI-O v1.20+（cgroup v2 必需）；与 Prometheus/cAdvisor v0.43.0+ 集成获取内存指标；与 Vertical Pod Autoscaler (VPA) 结合使用自动调整内存请求/限制；与 CPU Manager 和 Device Manager 协同实现 NUMA 拓扑感知；与 Node Problem Detector 集成检测内存问题

### 限制与约束

**已知限制**

- HugePages 不支持超卖（overcommit），请求必须等于限制
- HugePages 容器级隔离，每个容器在 cgroup 沙箱中有独立的限制
- Topology Manager 仅在 CPU Manager 启用时才能对齐 CPU 资源
- Memory Manager Static Policy 仅对 Guaranteed QoS Pod 生效
- Memory QoS 在 Linux 内核低于 5.9 时可能导致进程因内存分配超过内核回收能力而无限停滞
- cgroup v1 从 v1.35 起默认不支持 kubelet 启动，需设置 failCgroupV1: false 临时绕过
- Node.js 低于 v20.3.0 在 cgroup v2 下无法可靠检测内存限制，可能导致 OOM
- Java 应用需特定版本才能支持 cgroup v2（OpenJDK 8u372+, 11.0.16+, 15+）
- Go 应用使用 uber-go/automaxprocs 需 v1.5.1+
- 节点压力驱逐不遵守 PodDisruptionBudget（PDB）
- 硬驱逐阈值立即终止 Pod（0s 优雅期），可能导致工作负载中断

**混部兼容性**

内存混部时，Guaranteed QoS Pod 获得硬性内存保护（memory.min），Burstable Pod 获得软性保护（memory.low），BestEffort Pod 无保护；高优先级 Pod 可能驱逐低优先级 Pod；NUMA 拓扑感知可避免跨 NUMA 节点内存访问延迟

**性能开销**

内存虚拟化（cgroup 内存控制器）引入的开销通常可忽略（<1%）；跨 NUMA 节点内存访问可导致 1.3-2 倍延迟增加；Topology Manager 准入控制在 Pod 调度时引入轻微延迟

**固件与驱动依赖**

HugePages 需要在 BIOS/内核引导参数中预分配（hugepagesz=2M hugepages=512 等）；NUMA 拓扑需要在 BIOS 中启用 NUMA 支持；cgroup v2 需要 systemd cgroup 驱动；动态分配 HugePages 后需重启 kubelet

### 配置与部署

**配置方式**

- kubelet 命令行标志（--system-reserved, --kube-reserved, --eviction-hard 等）
- kubelet 配置文件（KubeletConfiguration）
- Pod 资源规范（resources.requests.memory, resources.limits.memory）
- Feature Gate 控制（MemoryQoS, WindowsCPUAndMemoryAffinity 等）
- kubelet 配置字段（memoryManagerPolicy, topologyManagerPolicy, memoryReservationPolicy 等）
- 内核引导参数（GRUB_CMDLINE_LINUX 配置 HugePages）

**配置示例**

kubelet 预留与驱逐配置: kubeReserved: {memory: "256Mi"}
systemReserved: {memory: "256Mi"}
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"; Pod 内存请求与限制: resources:
  requests:
    memory: "512Mi"
  limits:
    memory: "1Gi"; HugePages Pod 配置: resources:
  limits:
    hugepages-2Mi: 100Mi
    hugepages-1Gi: 2Gi
    memory: 100Mi
volumes:
- name: hugepage-2mi
  emptyDir:
    medium: HugePages-2Mi; Topology Manager 配置: --topology-manager-policy=single-numa-node
--topology-manager-scope=pod; Memory Manager 配置: memoryManagerPolicy: Static
reservedMemory:
  - numaNode: 0
    limits:
      memory: 1Gi; Memory QoS 配置（v1.36）: featureGates:
  MemoryQoS: true
memoryReservationPolicy: TieredReservation
memoryThrottlingFactor: 0.9; HugePages 内核引导参数: GRUB_CMDLINE_LINUX="hugepagesz=2M hugepages=1024 hugepagesz=1G hugepages=2"

**部署位置与环境**

裸金属（最佳 NUMA 性能，推荐生产环境）；虚拟机（支持，但嵌套虚拟化可能影响 NUMA 拓扑感知）；公有云（AWS EKS、GKE、AKS 均支持，但 HugePages 和 NUMA 亲和性取决于实例类型）；私有云/边缘（完全支持，但需根据硬件能力调整配置）

**虚拟化兼容性**

嵌套虚拟化场景下 NUMA 拓扑可能被隐藏或扭曲，影响 Topology Manager 效果；VM 热迁移时内存 NUMA 亲和性可能丢失；PCIe 直通/VFIO 设备需配合 Topology Manager 确保 NUMA 对齐；SR-IOV 设备需与 CPU Manager 和 Topology Manager 协同配置

### 性能特征

**基准性能数据**

Kubernetes 官方未提供统一内存基准数据；社区测试表明：Guaranteed QoS Pod 内存延迟最稳定；跨 NUMA 内存访问延迟约为本地 NUMA 的 1.3-2 倍；HugePages 可减少 TLB miss 提升内存密集型应用性能 10-30%；cgroup v2 内存控制器比 v1 更高效，支持 PSI（Pressure Stall Information）指标

**扩展性上限**

单集群最大 5000 节点；单节点最大 110 Pod；全集群最大 150000 Pod；etcd 建议事件存储独立部署以减轻内存压力；大规模集群（>1000 节点）控制面节点建议 16-64 GiB 内存

**每节点密度**

每节点 Pod 数上限默认 110，可通过 kubelet --max-pods 调整；实际 Pod 密度受节点内存总量和 Pod 内存请求限制；HugePages 不占用 Pod 数量限制但受限于预分配页面数

### 安全

**安全特性**

cgroup v2 提供更安全的子树委派（subtree delegation），降低容器逃逸风险；memory.min/memory.low 提供内存隔离保护，防止资源争用导致的安全问题；OOM 行为通过 oom_score_adj 控制（Guaranteed: -997，Burstable: 动态计算 2-999，BestEffort: 1000），确保优先级隔离；内存限制防止容器无限使用宿主机内存导致 DoS；HugePages 共享内存需通过 SHM_HUGETLB 标志并使用补充组控制访问

**合规与认证**

Kubernetes 一致性认证涵盖内存管理功能；cgroup v2 满足 Linux 基金会安全容器标准；FIPS 140-2/3 合规取决于底层操作系统和容器运行时，Kubernetes 内存管理本身不直接涉及加密认证

### 运维与生命周期

**可观测性支持**

- kubelet 暴露 /metrics/cadvisor 包含内存使用指标（container_memory_working_set_bytes, container_memory_rss 等）
- cAdvisor 提供容器级内存统计（v0.43.0+ 支持 cgroup v2）
- Prometheus 搭配 kube-state-metrics 提供 Pod/Node 级内存指标
- Kubernetes Events 记录 OOMKilled 和 Evicted 事件
- cgroup v2 PSI（Pressure Stall Information）指标提供内存压力预警
- kubectl top node/pod 显示实时内存使用
- Node Problem Detector 可检测内存相关问题
- v1.36 Memory QoS 新增内存预留和驱逐的可观测性指标

**维护与生命周期**

HugePages 动态分配后需重启 kubelet 才能被识别；cgroup v1 到 v2 迁移需要更新容器运行时和操作系统，不可完全热迁移；节点排水（drain）前需确保 Pod 可被重新调度；内存预留参数调整需重启 kubelet；etcd 内存使用随集群规模增长，需定期监控和调整

**弹性与故障恢复**

节点内存压力驱逐自动触发 Pod 重新调度；OOMKilled 容器由控制器自动重启；Guaranteed QoS Pod 受内存保护不易被驱逐；多副本控制面应对单节点故障；etcd 定期备份确保数据可恢复；PodDisruptionBudget 虽不适用于节点压力驱逐，但在主动排水时生效

### 经济性

**成熟度与社区支持**

Kubernetes 内存管理生态非常成熟，核心功能（requests/limits、驱逐策略）自 v1.0 起即为 GA；cgroup v2 从 v1.25 GA 至今已被广泛采用；Topology Manager 和 Memory Manager 已完成 GA 阶段；HugePages 支持自 v1.14 起为 GA；Memory QoS 持续演进中（v1.36 Tiered Memory Protection）；主要云厂商（AWS、GCP、Azure）、操作系统厂商（Red Hat、Ubuntu、Debian）、容器运行时（containerd、CRI-O）均全力支持

---

## 13. 异构 / 多架构集群管理 (Multi-Arch Cluster Management)

**官方文档**: Kubernetes 污点与容忍度: https://kubernetes.io/docs/concepts/scheduling-eviction/taint-and-toleration/; Kubernetes Well-Known Labels/Annotations/Taints: https://kubernetes.io/docs/reference/labels-annotations-taints/; Pod 拓扑分布约束: https://kubernetes.io/docs/concepts/scheduling-eviction/topology-spread-constraints/; Docker Buildx 多平台构建: https://docs.docker.com/build/building/multi-platform/; Karpenter NodePools: https://karpenter.sh/docs/concepts/nodepools/; AWS EKS 多架构集群: https://dev.to/aws-builders/multi-architecture-kubernetes-clusters-on-amazon-eks-2nol; AWS Graviton: https://aws.amazon.com/ec2/graviton/; Oracle OKE Arm 节点: https://docs.oracle.com/en-us/iaas/Content/ContEng/Tasks/contengrunningarmnodes.htm; GKE 迁移 x86 到多架构: https://cloud.google.com/kubernetes-engine/docs/tutorials/migrate-x86-to-multi-arch-arm; 阿里云 ACK ARM 调度: https://www.alibabacloud.com/help/zh/ack/ack-managed-and-ack-dedicated/user-guide/schedule-workloads-to-arm-based-nodes

### 硬件规格

**最低配置**

控制面节点 (amd64): 3 节点 etcd 集群每节点最低 2 核 CPU、4 GB RAM、20 GB SSD 磁盘; 单节点测试环境 1 核、2 GB RAM 可运行。; amd64 工作节点: 1-2 核 CPU、2 GB RAM、10 GB 磁盘，x86_64 (Intel/AMD) 指令集。; arm64 工作节点: 1-2 核 CPU (ARMv8-A/AArch64 架构，如 AWS Graviton、Ampere Altra、倚天 710)、2 GB RAM、10 GB 磁盘。; 混合集群最小规模: 1 个 amd64 控制面 + 至少各 1 个 amd64/arm64 工作节点即可构成异构集群 (云上建议每架构独立节点组)。

**推荐配置**

控制面节点 (amd64): 4-8 核 CPU、16 GB RAM、NVMe SSD (etcd 写入延迟 < 10 ms)。; amd64 工作节点: 4-8 核 CPU、16-32 GB RAM、SSD; 生产环境每架构至少 2 个节点。; arm64 工作节点: 4-8 核 CPU (Graviton3/4、Ampere Altra、倚天 710 等)、16-32 GB RAM、SSD。; 镜像仓库: 支持 OCI manifest list 的镜像仓库 (ECR、GCR、OCIR、Harbor、Docker Hub 等)，用于存放多架构镜像。

**生产级配置**

控制面: 8-16 核 CPU、32-64 GB RAM、NVMe SSD，3-5 节点 etcd HA; 控制面统一使用 amd64 以最大化生态兼容。; 工作节点: 按架构划分独立节点池/节点组 (EKS 多节点组、OKE 多节点池、GKE 多节点池)，每架构 3+ 节点支持跨架构容错; 大规模集群 (100+ 节点) 需按架构分别规划容量。; Karpenter/Cluster Autoscaler: 生产级自动扩缩容配置中，Karpenter 通过 NodePool requirements 同时声明 kubernetes.io/arch in [amd64, arm64]，按 Pod 架构需求自动供给对应架构实例。

### 兼容性

**支持的 K8s 版本范围**

通用: Kubernetes 对混合架构集群无特殊版本门槛，所有现代版本 (v1.20+) 均支持 amd64 + arm64 混合调度; amd64 自 v1.0 支持，arm64 自 v1.3 起官方支持，ppc64le/s390x 自 v1.5-1.6 加入。; Oracle OKE: ARM 形状 (Ampere A1) 节点池要求集群 Kubernetes 版本 1.19.7 或更高。; 阿里云 ACK: Kubernetes v1.24 及以上版本自动为 ARM 节点管理架构污点; 较早版本需手动添加。; GKE: ARM 节点池 (Tau T2A/T2D) 在所有受支持 GKE 版本可用，默认对 ARM 节点自动添加架构污点。

**操作系统兼容性**

amd64: 所有主流 Linux 发行版 (Ubuntu、Debian、RHEL、CentOS、Rocky Linux、AlmaLinux、SLES、Amazon Linux 2/2023、Fedora CoreOS、Flatcar) 及 Windows Server 2019/2022/2025 (Windows 节点仅 amd64)。; arm64: Ubuntu、Debian、RHEL、Fedora、Rocky Linux、AlmaLinux、SLES、Amazon Linux 2/2023 (ARM 版)、Flatcar Container Linux; 不支持 Windows Server 原生 arm64。; s390x: RHEL for IBM Z and LinuxONE、SLES for IBM Z、Ubuntu for IBM Z (发行版选择较少)。; ppc64le: RHEL for Power、Ubuntu for Power、SLES for Power (发行版选择较少)。

**K8s 上游支持阶段**

多架构节点 (amd64+arm64): GA (生产就绪); amd64/arm64 均为 release-blocking CI 覆盖，是 Kubernetes 官方全量支持架构。; 多架构集群管理能力: GA; 节点标签 (kubernetes.io/arch)、污点/容忍度、nodeSelector/nodeAffinity、拓扑分布约束均为上游 GA 功能，非实验特性。; ppc64le/s390x: 社区维护 (release-informing CI)，可加入混合集群但生态镜像与工具支持有限。; QEMU 模拟运行: 非官方支持方式，仅用于开发/测试或构建阶段，不推荐生产。

**生态兼容性矩阵**

CNI: Calico、Cilium、Flannel、OVN-Kubernetes、Weave Net 均发布 amd64 + arm64 多架构镜像; s390x/ppc64le 支持有限 (Cilium 提供 s390x/ppc64le，部分 CNI 不提供)。; CSI: AWS EBS CSI、GCE PD CSI、Azure Disk CSI、Ceph RBD/NFS、Longhorn、OpenEBS 均已支持 arm64; 部分厂商私有 CSI 仅 amd64。; 核心组件: pause、etcd、CoreDNS、kube-proxy、metrics-server、cAdvisor、Prometheus node_exporter 等上游核心组件均发布多架构镜像 (linux/amd64、linux/arm64、linux/ppc64le、linux/s390x)。; Operator 与工具: Karpenter、Cluster Autoscaler、Helm、Argo CD、Prometheus/Grafana、Istio、Linkerd 均支持多架构; 部分第三方业务级 Operator 仅提供 amd64 镜像，需逐项验证。; 云厂商支持: AWS (EKS + Graviton)、Google Cloud (GKE + Tau T2A)、Azure (AKS + Ampere Dpsv5/Epsv5)、Oracle (OKE + Ampere A1)、阿里云 (ACK + 倚天 710)、华为云 (CCE + 鲲鹏) 均支持同一集群混合 amd64/arm64 节点。

### 限制与约束

**已知限制**

无内置架构污点: Kubernetes 默认不自动为节点添加架构污点 (GKE/ACK 等部分云厂商自动添加)，需管理员手动配置; 未加污点时，单架构镜像 Pod 可能被调度到错误架构节点，容器启动即报 "exec format error" 并进入 CrashLoopBackOff。; 单架构镜像限制: 仅含单一平台的镜像无法在多架构集群中跨架构运行; 每架构需独立 tag 或多架构 manifest list。; Windows 架构限制: Windows 节点仅支持 amd64，无 arm64 Windows Server 支持，混合 Windows/Linux + 混合架构组合受限。; 第三方生态缺口: 部分第三方镜像、Operator、Helm 图表、监控 exporter 仅发布 amd64，arm64 节点上不可用 (无法拉取或运行失败)。; 指令集差异: 依赖 x86 特定指令集 (如 AVX-512、AMX) 或厂商闭源库的应用无法在 arm64 运行，需要源码级重新编译。; QEMU 模拟不适用生产: 通过 binfmt/QEMU 在 x86 节点模拟运行 arm64 镜像仅适合构建与测试，性能开销巨大，生产不推荐。; 节点池不可变性: Oracle OKE 等平台中，节点池创建后不可从 ARM 形状切换为 x86 形状 (反之亦然)，需新建节点池迁移。

**混部兼容性**

跨架构混部: 同一集群混部 amd64 与 arm64 工作负载是核心场景，前提是镜像为多架构 manifest list; 建议对非默认架构节点添加污点 (如 kubernetes.io/arch=arm64:NoSchedule) 并配合容忍度 + nodeSelector，防止架构不匹配。; 控制面混部: 控制面组件 (kube-apiserver、etcd、kube-controller-manager) 官方镜像为多架构，可在 amd64 或 arm64 节点运行; 生产建议控制面单一架构 (amd64) 以简化运维。; 与 GPU/加速器混部: GPU 节点 (NVIDIA A100/H100 等) 主要存在于 amd64; arm64 GPU (如 NVIDIA Grace Hopper、Jetson) 生态较弱，混部时需注意驱动镜像架构匹配。

**固件与驱动依赖**

arm64 节点: 需 ARMv8-A 及以上指令集; UEFI 或引导固件需支持 arm64 (如 Graviton 的 UEFI、Raspberry Pi 的 DTB); 内核模块与设备驱动必须为 aarch64 构建。; 驱动兼容矩阵: NVIDIA 驱动 (amd64/arm64 双架构提供)、网卡驱动 (部分厂商仅 x86)、存储 HBA 驱动需逐项核对架构支持; 使用 Node Feature Discovery (NFD) 可自动发现 CPU/内核/设备特性并打标签。; 云厂商 AMI/镜像: EKS/OKE/GKE 需为每架构选择对应 AMI/镜像 (如 EKS 的 AL2/AL2023 arm64 AMI、OKE 的 Oracle Linux 8 arm64 镜像; Oracle Linux 7 Arm 已于 2025-01-01 停止支持并停用 OL7 Arm 平台镜像)。

### 配置与部署

**配置方式**

节点标识: kubelet 自动打 kubernetes.io/arch / kubernetes.io/os 标签 (云厂商 CCM 额外打 node.kubernetes.io/instance-type); 无需手动配置。; 架构隔离: 手动 kubectl taint 为节点添加架构污点; 云厂商 (GKE、ACK v1.24+) 可自动为 ARM 节点池添加污点。; Pod 调度: nodeSelector: kubernetes.io/arch=arm64; nodeAffinity requiredDuringSchedulingIgnoredDuringExecution; topologySpreadConstraints (topologyKey 可用 kubernetes.io/arch 实现跨架构分布)。; 自动扩缩容: Karpenter NodePool requirements 声明 kubernetes.io/arch in [amd64, arm64]; Cluster Autoscaler 按节点组标签选择架构。; 多架构镜像构建: Docker Buildx (docker buildx build --platform linux/amd64,linux/arm64 --push)、docker manifest create、Podman/Buildah、buildctl (BuildKit)、ORAS; 通过 CI (GitHub Actions/GitLab) 构建并推送 manifest list。; 污点添加命令示例: kubectl taint nodes <arm64-node> kubernetes.io/arch=arm64:NoSchedule

**配置示例**

Pod 指定 arm64 节点: spec.tolerations: [{key: kubernetes.io/arch, operator: Equal, value: arm64, effect: NoSchedule}]; spec.nodeSelector: {kubernetes.io/arch: arm64}; 跨架构分布: spec.topologySpreadConstraints: [{maxSkew: 1, topologyKey: kubernetes.io/arch, whenUnsatisfiable: DoNotSchedule, labelSelector: {matchLabels: {app: demo}}}]; Karpenter NodePool requirements: requirements: [{key: kubernetes.io/arch, operator: In, values: [amd64, arm64]}, {key: karpenter.sh/capacity-type, operator: In, values: [on-demand, spot]}]; Buildx 构建多架构镜像: docker buildx build --platform linux/amd64,linux/arm64 -t repo/app:v1 --push . (需先 docker buildx create 启用多平台构建器)

**部署位置与环境**

公有云: AWS EKS (amd64 + Graviton 节点组)、GCP GKE (x86 + Tau T2A ARM)、Azure AKS (x86 + Ampere ARM)、Oracle OKE (x86 + Ampere A1)、阿里云 ACK (x86 + 倚天 710)、华为云 CCE (x86 + 鲲鹏) 均原生支持混合架构集群。; 私有云/裸金属: OpenShift、RKE2、K3s、Kubespray 等发行版支持在裸金属或私有云混合 x86/ARM 服务器组建集群。; 边缘/混合部署: 边缘侧 ARM 节点 (Raspberry Pi、Jetson、工业网关) 与数据中心 x86 节点组成混合集群是常见模式; KubeEdge/轻量发行版可扩展。; 开发环境: Apple Silicon (M 系列) 与 x86 开发机可组成小型多架构集群用于镜像验证。

**虚拟化兼容性**

虚拟机节点: 多架构集群中的节点可以是 VM，虚拟机架构由 vCPU 决定 (x86 宿主机只能提供 amd64 VM，ARM 宿主机提供 arm64 VM); 云端每个架构的实例类型天然对应。; 嵌套虚拟化: 嵌套虚拟化 (KVM-on-KVM) 在云实例中通常受限，架构需与物理宿主机一致; arm64 嵌套虚拟化 (如 Graviton 上运行 KVM) 生态支持有限。; 透传与 SR-IOV: GPU/网卡直通 (PCIe passthrough、SR-IOV) 在 amd64 生态成熟; arm64 服务器平台的透传支持依赖厂商固件与驱动，需逐项验证。

### 性能特征

**基准性能数据**

AWS Graviton 3/4: AWS 官方宣称 Graviton3 相比同代 x86 (Ice Lake) 计算性能提升约 25%，性价比提升约 20%; Graviton4 宣称性能/功耗比进一步提升 (厂商数据)。; Ampere Altra (Oracle A1): 每核性能与同代 x86 相当 (SPECrate 社区数据)，整机功耗更低; Oracle A1 免费层提供 4 OCPU/24 GB RAM 用于测试。; 社区多架构对比: Web 服务、容器运行时、内存型负载在 arm64 与 amd64 上性能差异通常在 ±10% 以内 (社区基准，因工作负载而异); 依赖 AVX-512 的计算负载在 x86 显著更快。

**每节点密度**

通用: 默认每节点 110 Pod (kubelet --max-pods)，与 CPU 架构无关; 实际密度由内存/CPU 决定。; arm64 边缘节点: 低规格 ARM 节点 (2-4 核/4-8 GB) 通常运行 20-50 个轻量级 Pod。; 云实例: EKS/GKE 按实例规格设置 max-pods (如 t3.medium 17 个、m6g.large 35 个等)，Graviton 与同规格 x86 实例的 Pod 密度上限一致。

### 安全

**安全特性**

架构隔离: 通过污点 (NoSchedule/NoExecute) 与容忍度实现架构级隔离，限制工作负载只能运行在特定架构节点，减少错误架构镜像执行风险。; 镜像供应链安全: cosign/notation 支持对多架构 manifest list 整体签名 (签名绑定镜像索引)，SBOM 可逐平台生成; 需在镜像仓库启用签名验证 (Kyverno/Ratify)。; 可信启动与 TPM: amd64 (Intel/AMD) 与 arm64 服务器均支持 UEFI Secure Boot/TPM; 云厂商提供不可变启动链 (如 AWS Nitro Enclaves、安全启动 AMI)，与架构无关。; 机密计算: Intel TDX/AMD SEV-SNP (amd64) 生态成熟; arm64 机密计算 (如 Ampere 与厂商合作方案、Confidential Containers 对 ARM 支持) 仍在演进，覆盖面较小。

### 运维与生命周期

**可观测性支持**

架构视图: kubectl get nodes -L kubernetes.io/arch 查看各架构节点分布; 按 kubernetes.io/arch 标签聚合 Prometheus 指标 (节点数、CPU/内存使用率、Pod 数、镜像拉取失败数)。; 镜像监控: 监控 "exec format error" 容器崩溃事件 (CrashLoopBackOff、ImagePullBackOff) 以发现单架构镜像调度错误; kubelet 事件与 Prometheus 告警 (KubePodCrashLooping)。; 工具链: node_exporter、kube-state-metrics、Metrics Server 均有多架构镜像，可在混合集群全部节点部署; K9s/Octant 等客户端工具需与运维工作站架构匹配。

**维护与生命周期**

多架构镜像发布流程: CI 流水线为每架构构建镜像并合并为 manifest list 推送 (buildx bake / GitHub Actions matrix)，单 tag 覆盖所有架构; 镜像更新需回归测试各架构。; 节点生命周期: 每架构节点池独立滚动升级 (EKS 节点组更新、OKE 节点池替换、GKE 节点池升级); kubectl drain 标准流程与架构无关。; 架构迁移: x86 → arm64 迁移路径: ① 添加 arm64 节点池并打污点 ② 应用改为多架构镜像 ③ 逐步添加容忍度与 nodeSelector 迁移工作负载 ④ 验证后下线 x86 节点 (AWS/GCP 官方迁移指南)。; 退役与替换: 节点池不可变平台 (如 OKE ARM 形状) 需新建节点池并排空旧节点; 节点污点在节点删除后自动消失。

**弹性与故障恢复**

跨架构容错: 多架构镜像 Pod 可在任一架构节点重建，某架构节点池故障时仍可调度到另一架构节点 (前提是容量足够); 单架构镜像只能恢复到同架构节点。; 冗余要求: 生产建议每架构至少 2 个节点、跨可用区部署; 控制面统一架构并 3+ 副本保障 etcd 可用性。; 故障切换: 节点故障由 NodeLifecycleController 处理 (默认 40 秒后驱逐，容忍度 300 秒)，与架构无关; Karpenter/CA 会自动补充故障架构的实例。; 数据持久化: 多架构集群中 CSI 存储按节点架构提供，同一 PV 可在不同架构节点间迁移 (取决于存储后端网络可达性)，本地 PV 则受限。

### 经济性

**总拥有成本 (TCO)**

硬件/实例成本: ARM64 云实例相比同规格 x86 通常便宜 20-30% (AWS Graviton vs 同代 x86、Ampere A1 vs AMD/Intel，厂商与社区报告); Oracle A1 免费层 (4 OCPU/24 GB) 可零成本起步。; 运行成本: arm64 服务器功耗约为同性能 x86 的 50-70% (Ampere/Graviton 厂商数据)，云上按小时计费直接节省; 多架构集群可把适合负载 (Web、内存型、CI) 调度到 ARM 节点降本。; 维护/开发成本: 多架构镜像 CI 双架构构建、每架构回归测试、驱动/Operator 兼容性验证增加研发与运维成本; 部分仅 amd64 的第三方软件需替换或自编译，需纳入 TCO 评估。; 综合建议: 社区案例 (EKS 迁移 Graviton) 报告整体集群成本节省 20-30%，但需扣除多架构改造的一次性投入。

**成熟度与社区支持**

上游成熟度: 多架构支持 (amd64+arm64) 为 Kubernetes GA 能力，全部主流云厂商与发行版 (EKS、GKE、AKS、OKE、ACK、OpenShift、RKE2、K3s) 原生支持。; 工具生态: Karpenter、Cluster Autoscaler、Buildx、Podman、Harbor、cosign/notation、Argo CD、Helm 均成熟支持多架构; Node Feature Discovery (CNCF) 提供硬件特性自动发现与标签。; 社区与厂商: ARM 公司 (learn.arm.com 多架构学习路径)、AWS (Graviton 迁移博客)、Google (GKE 多架构教程)、Oracle (Ampere OKE 指南)、阿里云均有活跃文档与案例; 社区讨论 (kubernetes.io、Reddit r/aws) 活跃。; 认证与支持: 多架构集群为通用功能，无需额外许可; 企业支持由各发行版/云厂商 SLA 覆盖，与架构无关。

---

## 14. NPU / AI 推理芯片（Intel NPU（AI Boost）、Hailo-8/8L、Rockchip NPU（RK3588/RK3576）、Qualcomm AI Engine（Cloud AI 100/Ultra、Snapdragon NPU）、华为昇腾 NPU 等专用神经处理单元，含 K8s 设备插件/CDI/DRA 管理、KubeEdge 边缘部署、与 GPU 的选型对比）

**官方文档**: Intel NPU 设备插件: https://intel.github.io/intel-device-plugins-for-kubernetes/cmd/npu_plugin/README.html ; Intel 设备插件集合: https://github.com/intel/intel-device-plugins-for-kubernetes ; Intel NPU (OpenVINO): https://docs.openvino.ai/2025/openvino-workflow/running-inference/inference-devices-and-modes/npu-device.html ; Hailo-8: https://hailo.ai/products/ai-accelerators/hailo-8-ai-accelerator/ ; Hailo-8L: https://hailo.ai/products/ai-accelerators/hailo-8l-ai-accelerator-for-ai-light-applications/ ; Hailo 社区 K8s 设备插件 (CDI): https://github.com/SNU-RTOS/hailo-device-plugin ; Rockchip RKNN-Toolkit2: https://github.com/airockchip/rknn-toolkit2 ; RKNPU 驱动: https://github.com/rockchip-linux/rknpu2 ; RKLLM/rkllama (llama.cpp NPU 后端): https://github.com/airockchip/rkllm-toolkit ; Qualcomm Cloud AI 100 Ultra: https://www.qualcomm.com/data-center/products/cloud-ai-100-ultra ; Qualcomm Cloud AI SDK (K8s 设备插件): https://quic.github.io/cloud-ai-sdk-pages/latest/Getting-Started/Installation/Docker/k8s/ ; K8s 设备插件概念: https://kubernetes.io/docs/concepts/extend-kubernetes/compute-storage-net/device-plugins/ ; K8s DRA: https://kubernetes.io/docs/concepts/scheduling-eviction/dynamic-resource-allocation/ ; KubeEdge: https://kubeedge.io/ ; 昇腾 CANN/设备插件: https://www.hiascend.com/

### 硬件规格

**最低配置**

Hailo: Hailo-8（26 TOPS、典型功耗 2.5W、集成片上内存无外置 DRAM）或 Hailo-8L（13 TOPS [不确定功耗约 1.5W]），以 M.2/PCIe/mPCIe 模块形式插在边缘主机/工控机上；需安装 HailoRT 驱动与固件；K8s 侧使用社区 CDI 设备插件（hailo.ai/npu）或手动挂载设备节点。; 华为昇腾: Atlas 300I Duo（Ascend 310P、280 TOPS INT8）等 PCIe 推理卡或 Atlas 200/500 边缘盒子 [不确定 310P 精确保密计算规格]；需 CANN 工具链、Ascend 驱动（npu-smi 可查），配合昇腾设备插件（huawei.com/Ascend310 等资源名）接入 K8s。

**推荐配置**

边缘推理节点: 4-8 核 CPU + 8-16GB 内存的工控机/迷你主机，配 1 个 Hailo-8 M.2（26 TOPS）或 Rockchip RK3588 板卡（6 TOPS），K3s/KubeEdge/MicroK8s 轻量集群，Ubuntu 22.04/24.04 或 Debian；用于视频分析、质检、车载等单模型 INT8 量化推理。; 服务器推理节点: 双路 x86 服务器插 1-8 张 Cloud AI 100/Ultra 或 Atlas 300I 推理卡，标准 K8s 1.28+，设备插件按卡注册资源；配合 vLLM（Qualcomm 提供 vLLM 适配版）承载 Llama 3.1 8B 等生成式模型服务。; Intel 客户端: Core Ultra Series 2 (Lunar Lake) 及以上笔记本/迷你主机（NPU 48 TOPS），OpenVINO 运行时 + NPU 设备插件，本地轻量 LLM/视觉模型推理。

**生产级配置**

多卡推理集群: Cloud AI 100 Ultra 8 卡服务器（单卡 64GB LPDDR4X）或 Atlas 300I 服务器，多卡 + vLLM 分布式推理，K8s + Volcano/Kueue 调度；华为云 CCE 提供昇腾 NPU 节点池托管能力 [不确定大规模生产占比]。; NPU 虚拟化切分: 昇腾提供 NPU 虚拟化/软切分（如 MindX、npu-virtualization）将单卡切分为多个推理实例 [不确定在 K8s 中的成熟度]；Intel/Hailo/Rockchip 无等价硬件级切分。

### 兼容性

**支持的 K8s 版本范围**

设备插件机制: K8s 设备插件 API 自 1.10 起 GA，NPU 厂商/社区插件均基于该机制，对 K8s 版本无特殊要求；CDI 方案要求容器运行时（containerd 1.7+）启用 CDI。; Intel: NPU 插件随 intel-device-plugins 发布（如 0.36.0），经 NFD/Intel Device Plugin Operator 部署 [不确定精确的 K8s minor 版本矩阵]。

**操作系统兼容性**

Linux 主流: Ubuntu 22.04/24.04、Debian、Fedora、openEuler、麒麟/统信等国产发行版（昇腾生态）；NPU 驱动以内核模块形式提供（ivpu/rknpu/qaic/drv），依赖内核版本 [不确定完整认证矩阵]。; Windows: Intel NPU 可在 Windows 上通过 OpenVINO 使用，但 K8s NPU 节点生态以 Linux 为主 [不确定 Windows 节点 NPU 支持]。; 嵌入式: Rockchip/Hailo 面向嵌入式 Linux（buildroot/Yocto），K8s 侧常用轻量发行版。

**K8s 上游支持阶段**

设备插件: Kubernetes 上游 GA 机制，NPU 资源整体分配、无共享与属性选择，是 NPU 当前主流接入方式。; KubeEdge: CNCF 毕业项目（2024），是边缘 NPU 部署的主要编排层。

**生态兼容性矩阵**

Intel: NFD + Intel Device Plugins Operator + OpenVINO 运行时，资源 npu.intel.com/accel；模型经 OpenVINO IR 优化，兼容 PyTorch/TensorFlow 导出。; Hailo: HailoRT 运行时 + Hailo Dataflow Compiler + Hailo Model Zoo（预训练模型仓库）；社区设备插件基于 CDI（containerd 1.7+）；监控依赖 hailortcli [不确定 Prometheus exporter]。; Qualcomm: Cloud AI SDK（ONNX 导入、QPC 编译、部署）+ QAic 设备插件 + vLLM 适配版（On-Prem Appliance）；支持 Llama/Mistral/TinyLlama/Codestral 等量化模型 [不确定完整模型清单]。; 昇腾: CANN + MindIE（推理引擎）+ MindSpore/PyTorch 适配 + 昇腾设备插件 + npu-exporter；华为云 CCE/CCE Turbo 原生集成，配合 Volcano 批调度。; 调度与监控: 与 K8s 原生调度器扩展资源、Volcano/Kueue 批调度兼容；监控以厂商 CLI（npu-smi/hailortcli）为主，Prometheus exporter 生态弱于 GPU（无 DCGM 等价物）。

### 限制与约束

**已知限制**

推理专用: NPU 明确不支持训练（Qualcomm Cloud AI 100 官方声明、Hailo/RKNN 均为推理编译器），模型须经厂商编译器量化转换（RKNN、Hailo Dataflow Compiler、QPC、OpenVINO IR、CANN OM），算子覆盖有限，不支持的算子需改写或回退 CPU。; 内存受限: Hailo 无外置 DRAM（流式执行，大模型需分块）；Cloud AI 100 Ultra 64GB LPDDR4X 但 70B 级模型扩展效率差（第三方评测显示极端规模有架构权衡）；RK3588 板卡内存 4-32GB 限制量化模型尺寸。; Intel: NPU 仅客户端形态（无服务器独立卡，数据中心走 Gaudi），默认每 NPU 仅 1 容器（-shared-dev-num 可调）；privileged 方式暴露设备有安全顾虑，官方建议 CDI。; 精度: INT8/INT4 量化引入精度损失，敏感模型需校准；NPU 无 GPU 的 FP64/BF16 大规模训练能力。

### 配置与部署

**配置方式**

Intel: 通过 NFD 检测 NPU 并打标签，NPU 设备插件 DaemonSet 注册 npu.intel.com/accel；支持 -shared-dev-num 共享模式（多容器共用 1 个 NPU）；支持 CDI 注入（推荐）或 privileged 模式；Pod 以 limits 声明资源。; Hailo: 社区 hailo-device-plugin（K8s 设备插件 API v1beta1 + CDI 动态生成）注册 hailo.ai/npu；部署 kubectl create -f 远程清单；要求 containerd 1.7+ 且启用 CDI [不确定对 CRI-O 的支持]。; Rockchip: 无官方插件；社区做法: 特权容器直接访问 /dev/rknpu 与 /dev/dri，或自研设备插件注册自定义资源 [不确定通用方案]；rkllama/rknn 推理容器以 DaemonSet 或 Deployment 部署在 NPU 节点。; Qualcomm: QAic K8s 设备插件（DaemonSet，容器镜像 cloud_ai_k8s_device_plugin）注册 NPU 资源；模型先用 Cloud AI SDK 编译为 QPC 格式并制作容器镜像，再以 vLLM 或 SDK 服务方式部署 [不确定资源名称格式]。

**部署位置与环境**

主要场景: 边缘为主战场: KubeEdge 云边协同、K3s/MicroK8s 单机边缘、工控机/车载/安防/零售盒子；服务器推理: Cloud AI 100/Ultra、Atlas 300I 裸金属或私有云；公有云: 华为云 CCE 昇腾节点池、阿里云等信创 NPU 实例 [不确定各云厂商完整矩阵]；混合部署: 边缘节点离线自治 + 云端模型管理（KubeEdge + Sedna）。

### 性能特征

**基准性能数据**

Hailo: Hailo-8 26 TOPS INT8（典型 2.5W），官方宣称分类任务 1000+ FPS；Hailo-8L 13 TOPS [不确定 8L 具体 FPS]；支持 CNN/Transformer，视觉模型（YOLO 系列、分割、姿态）性能强于同功耗 GPU/CPU [不确定具体 benchmark 表]。; Qualcomm: Cloud AI 100: 400 TOPS INT8；Ultra: 800 TOPS INT8；vLLM 承载 Llama 3.1/Codestral 等（量化）；第三方评测（arXiv 2507.00418）对 15 个开源模型对比 V100/A6000/A100/GH200: 能效整体占优，小模型最高 26 tokens/(s·W) 提升 [不确定绝对吞吐数字]；初期编译需数小时。

### 安全

### 运维与生命周期

**可观测性支持**

Qualcomm: Cloud AI SDK 提供运行时遥测/健康接口 [不确定 Prometheus 集成]，设备插件可上报设备健康状态。

### 经济性

**总拥有成本 (TCO)**

节省手段: 边缘推理场景以 NPU 替代 GPU 可显著降低单点成本（硬件+功耗+散热）；共享模式（Intel -shared-dev-num）提高利用率；vLLM 多卡并发提升服务器 NPU 吞吐 [不确定量化]。

**成熟度与社区支持**

Intel: OpenVINO 生态成熟（PC 侧 AI 事实标准），设备插件随 intel-device-plugins 长期维护；NPU 属 AI PC 战略一部分，社区活跃 [不确定 K8s 侧部署规模]。; Rockchip: 开发者社区活跃（rkllama/rknn_model_zoo、Orange Pi 生态），价格敏感边缘场景广泛使用；官方云原生支持缺失，方案偏 DIY [不确定生产案例]。; Qualcomm: Cloud AI SDK 文档完整、vLLM 适配；但数据中心推理业务处于收缩/转型期 [不确定 Cloud AI 100 是否停产]，长期投入存疑。; 上游: KubeEdge 为 CNCF 毕业项目，边缘 AI（Sedna 联邦学习/推理）社区活跃；K8s 侧 NPU 标准化（DRA）仍处早期，整体生态成熟度低于 GPU 的 CUDA 体系。

---

## 15. 网络基础设施（CNI 插件、网卡、DPDK、SR-IOV、服务网格与 eBPF 硬件要求）

### 硬件规格

**最低配置**

网卡: 任意 1GbE 网卡（Linux 内核驱动支持即可，如 Intel、Realtek、Broadcom、Mellanox 常见型号）；Kubernetes 官方对节点网络带宽无硬性要求; CPU/内存: 无额外硬性要求；CNI 组件为轻量级守护进程（Calico/Cilium Agent 通常占用数百 MB 内存级），kube-proxy 约 50-200 MB 内存; 内核: 常规数据面需要 Linux 内核 4.18+；Calico 官方要求内核 5.10+；eBPF 数据面需要支持 BTF（CONFIG_DEBUG_INFO_BTF=y）的内核; 说明: 1GbE 足以运行小规模测试/开发集群

**推荐配置**

网卡: 每节点 10GbE/25GbE（生产集群通用推荐）；网卡应支持硬件校验和卸载（checksum offload）、GRO/GSO、多队列（RSS），队列数建议与 vCPU 数匹配; 控制面节点: 1-10GbE 即可，但要求低延迟网络；etcd 与 API Server 之间网络延迟应保持在毫秒级; 说明: 主流云厂商工作节点实例默认提供 10-25 Gbps 带宽；数据密集型工作负载（存储、AI 训练、大数据）建议 25GbE 起步

**生产级配置**

网卡: 25/40/100GbE（大规模集群或网络密集型工作负载）；建议管理网络与数据网络物理分离（多网卡）；SR-IOV/DPDK 场景需支持虚拟化直通的高级网卡（Intel E810/X710、Mellanox ConnectX-5/6/7 等）; 内核与系统: Linux 内核 5.10+（eBPF 数据面）；BIOS 开启 VT-d/IOMMU（SR-IOV/DPDK）；预留大页内存 HugePages（DPDK 建议 1GiB 大页）；CPU 核隔离（isolcpus）供 DPDK 轮询模式使用; 大规模集群: >1000 节点建议节点间 10GbE+；控制面与 etcd 需要稳定低延迟网络；服务网格场景需为 sidecar 预留每 Pod 约 0.1-0.25 vCPU 与 50-150 MB 内存; 说明: eBPF 数据面在普通商用硬件上即可达到 100Gbps 近线速（Cilium 官方基准使用 AMD Ryzen 9 3950X + 128GB 内存 + 双口 Intel E810-CQDA2 100G 网卡）

### 兼容性

**支持的 K8s 版本范围**

CNI 规范要求插件兼容 v0.4.0+（Kubernetes 推荐 v1.0.0 兼容）；kubelet 直接管理 CNI 的机制已在 v1.24 移除，由容器运行时负责加载；NetworkPolicy 自 v1.7 起 GA。各主流 CNI 跟踪支持最近数个 K8s 次要版本：Cilium 文档要求 Kubernetes 1.16+；Calico v3.x 支持 K8s 1.16+；OVN-Kubernetes 与 OpenShift/上游同步演进；Flannel 无严格版本下限（精确版本矩阵以各项目发行说明为准）

**操作系统兼容性**

Calico：Ubuntu 20.04+、RHEL 8+、Debian 10+、Bottlerocket、Talos 等 Linux 发行版，另支持 Windows 节点（HNS，功能受限）；Cilium：仅 Linux（AMD64/AArch64），支持 Ubuntu 20.04+、Debian 10+、RHEL/CentOS 8.6+、Fedora CoreOS、Bottlerocket、Flatcar、Talos 1.5+、Amazon Linux 2、COS 85+；Flannel：Linux 与 Windows（VXLAN 后端，Windows 容器场景常用）；OVN-Kubernetes：主要 Linux，OpenShift 通过混合覆盖网络支持 Windows 节点

**K8s 上游支持阶段**

CNI 为 CNCF 项目（规范 v1.0.0 已发布）；NetworkPolicy 为 GA（v1.7+）；kube-proxy iptables 模式为默认且 GA，IPVS 模式为 GA（大集群推荐）；eBPF 数据面与 kube-proxy 替代（Cilium、Calico 社区实现）已在生产环境广泛使用；OVN-Kubernetes 为 OpenShift 默认网络插件；Multus 为 k8snetworkplumbingwg 维护的 CNI 元插件（多网卡事实标准）

**生态兼容性矩阵**

Calico：兼容 kube-proxy、提供自研 eBPF 数据面（可替代 kube-proxy）、完整 NetworkPolicy、与 Istio 集成、Prometheus 指标、支持 EKS/GKE/AKS/裸金属；Cilium：Hubble 可观测性、Tetragon 安全、Gateway API、kube-proxy 替代、ambient 服务网格、Prometheus 指标；Flannel：轻量 overlay，无原生网络策略（常与 Calico 组合为 Canal）；OVN-Kubernetes：OpenShift 深度集成、EgressFirewall、Prometheus、支持 DPU 硬件卸载；多网卡生态：Multus + SR-IOV 设备插件 + Intel userspace CNI（DPDK/vhost-user）+ RDMA 设备插件

### 限制与约束

**已知限制**

- Flannel：无 NetworkPolicy、无高级路由/多网段能力、封装性能较低，不适合大规模与多租户场景
- Calico：IPIP/VXLAN 封装带来额外开销；eBPF 数据面需要较新内核（5.10+）且不支持 Windows；大规模 BGP 需要部署路由反射器
- Cilium：内核要求高（5.10+ 且需 BTF），不支持 Windows 节点，功能面大导致学习与排障成本高
- OVN-Kubernetes：节点资源占用较高（ovs-vswitchd、ovn-controller 常驻，内存数百 MB 级），排障复杂度高，主要依赖 OpenShift/上游社区支持
- kube-proxy iptables 模式：服务数超过约 3000 时规则线性遍历导致性能退化，大集群建议 IPVS 或 eBPF
- SR-IOV：VF 固定分配不可超卖，直通设备无法随 VM/Pod 热迁移，每节点 VF 数量受网卡与 PCIe 资源限制，需专用支持 SR-IOV 的网卡
- DPDK：应用必须使用 DPDK 轮询模式库改造（普通应用无法直接受益），需要大页内存与 CPU 核隔离，与容器网络模型集成复杂（需 userspace CNI）
- 服务网格：每个 Pod 增加 sidecar 资源开销（Envoy 约 60-155 MB 内存、0.1-0.25 vCPU）与路径延迟，mTLS 加密带来额外 CPU 开销
- overlay 网络（VXLAN/Geneve/IPIP）存在封装开销，多一跳封装对延迟敏感型工作负载不利

**混部兼容性**

网络数据面（CNI、kube-proxy）与业务工作负载混部时会争抢节点 CPU（软中断/数据包处理），高吞吐网络负载与 CPU 密集型业务互相影响；DPDK 轮询模式会占满所绑定的 CPU 核，必须通过 isolcpus/cpuset 与业务隔离；SR-IOV 多租户共享同一物理网卡时缺乏默认带宽隔离，需借助网卡 QoS/限速策略；服务网格 sidecar 与业务容器同 Pod 共享资源，内存/CPU 预留不足会导致请求排队与尾延迟上升

**性能开销**

Flannel VXLAN（内核封装）吞吐较裸网络下降约 10-20%（社区测试，取决于 MTU 与 CPU）；Calico IPIP/VXLAN 开销相近，eBPF 数据面显著更低；Cilium eBPF 数据面在 100Gbps 接口可达近线速，官方基准显示并行压测时约消耗 30% 系统资源；OVN-Kubernetes 经 OVS 内核数据路径存在额外开销（社区测试 p99 延迟高于直连路由方案）；服务网格：Linkerd 官方基准 P50 延迟增加约 9-19ms，Envoy sidecar 内存约 155MB（Linkerd 约 18MB）；DPDK 可消除内核协议栈开销，在 10/25/40/100G 上达到近线速并大幅降低 CPU 占用

**固件与驱动依赖**

网卡固件版本（Intel NVM、Mellanox/Marvell 固件）与内核驱动（ice、i40e、ixgbe、mlx5_core）需匹配，固件升级通常需要节点维护窗口；SR-IOV 需要 BIOS 开启 VT-d/IOMMU 并加载 vfio-pci 模块；DPDK 需要 vfio-pci 与 IOMMU（iommu=pt），建议 1GiB HugePages；OVN-Kubernetes 依赖内核 openvswitch 模块；WireGuard/IPsec 加密依赖内核模块支持；eBPF 数据面依赖内核 BTF（CONFIG_DEBUG_INFO_BTF=y）与 bpffs（挂载于 /sys/fs/bpf）；PCIe 热插拔对 SR-IOV/DPDK 设备支持有限，设备变更通常需重建 Pod 或重启节点

### 配置与部署

**配置方式**

- CNI 配置文件（/etc/cni/net.d）与插件二进制（/opt/cni/bin）由容器运行时（containerd/CRI-O）加载管理（K8s v1.24+）
- Helm Chart / Operator 部署（Cilium、Calico、Multus、SR-IOV 设备插件均有官方 Helm/Operator）
- Multus：通过 NetworkAttachmentDefinition CRD 声明 Pod 附加网络（多网卡）
- SR-IOV：SriovNetworkNodePolicy CRD 配置 VF 分配，sriov-network-device-plugin 暴露资源
- DPDK：userspace CNI（Intel）+ HugePages + vfio-pci 直通 + Pod 资源注解（内存大页、CPU）
- kube-proxy 模式选择：iptables（默认）/ IPVS / 由 eBPF 数据面替代（Cilium/Calico）
- DRA（Dynamic Resource Allocation）可声明网卡带宽预留等网络硬件资源（K8s 1.34+ 演进方向）

**配置示例**

Multus SR-IOV 附加网络: apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: sriov-net
  annotations:
    k8s.v1.cni.cncf.io/resourceName: intel.com/sriov
spec:
  config: '{"type": "sriov", "ipam": {"type": "host-local", "subnet": "10.56.217.0/24"}}'; Pod 请求 SR-IOV VF: metadata:
  annotations:
    k8s.v1.cni.cncf.io/networks: sriov-net
spec:
  containers:
  - name: app
    resources:
      limits:
        intel.com/sriov: '1'; Cilium Helm 安装（eBPF 数据面）: helm install cilium cilium/cilium --version 1.16.x \
  --namespace kube-system \
  --set kubeProxyReplacement=true \
  --set ipam.mode=cluster-pool; kube-proxy IPVS 模式: kube-proxy 配置:
  mode: ipvs; DPDK 大页与 vfio 内核参数: GRUB_CMDLINE_LINUX="iommu=pt intel_iommu=on default_hugepagesz=1G hugepagesz=1G hugepages=8"
# 加载 vfio-pci 并绑定网卡（echo <pci> > /sys/bus/pci/drivers/vfio-pci/bind）

**部署位置与环境**

公有云：EKS 默认 AWS VPC CNI、GKE 默认 Dataplane V2（基于 Cilium）、AKS 默认 Azure CNI（可选 Cilium），均支持 Calico/Cilium 自选；裸金属：Calico/Cilium/OVN-Kubernetes 全功能（BGP、eBPF、SR-IOV/DPDK 直通），是高性能网络场景的最佳环境；虚拟机：支持各类 overlay（VXLAN/Geneve），但 SR-IOV/DPDK 需要虚拟机透传（依赖虚拟化平台支持），嵌套虚拟化封装性能下降；边缘：Flannel/K3s 轻量方案为主，Cilium 也支持边缘（kube-router 替代）；混合部署兼容性良好，overlay 网络天然跨环境互通

**虚拟化兼容性**

嵌套虚拟化下 VXLAN/Geneve 软件封装性能显著下降，建议关闭硬件卸载或使用直通；SR-IOV 需要宿主机 IOMMU 透传（vfio-pci），VM 热迁移（vMotion/在线迁移）不支持直通设备，迁移前需摘除 VF；云 VM 通常无法使用 SR-IOV/DPDK（需裸金属实例或专用 vCPU 类型）；DPDK 在云环境一般不可用；虚拟机内多网卡（virtio）与 Multus 兼容，但带宽受虚拟化层限制

### 性能特征

**基准性能数据**

Cilium 官方基准：eBPF 数据面在 100Gbps 接口上可接近线速，并行压测时约消耗 30% 系统资源（测试硬件：AMD Ryzen 9 3950X、128GB DDR4、双口 Intel E810-CQDA2 100G）；社区 CNI 对比测试（sanj.dev、DaoCloud 等）：Cilium（eBPF）TCP 吞吐最高、延迟最低，Calico 次之，Flannel VXLAN 吞吐最低（约低 10-20%）；服务网格（Linkerd 官方 2021 基准，Equinix Metal 裸金属）：200/2000 RPS 下 Linkerd P50 延迟总值为 17ms（较基线 +11ms/+9ms），Istio 较基线 +19ms/+15ms，P99.9 时 Linkerd 约 90ms、Istio 约 200ms；数据面内存：Linkerd proxy 平均最大 17.8MB，Envoy（Istio）154.6MB；Istio 官方：1000 RPS/线程负载下 sidecar 0.20 vCPU+60MB、waypoint 0.25 vCPU+60MB、ztunnel 0.06 vCPU+12MB，2000 Pod 规模下全网格 70000 RPS

**扩展性上限**

Kubernetes 官方大规模集群上限：5000 节点、150000 Pod、300000 容器、每节点默认 100-110 Pod、全集群约 10000 个 Service；kube-proxy iptables 模式在数千条 Service 规则时性能退化，IPVS/eBPF 可支撑万级 Service；Flannel 适合中小规模（社区经验数百节点以内），Calico/Cilium/OVN-Kubernetes 均支持数千节点（Calico/Cilium 需 route reflectors/cluster-pool IPAM 等调优）；服务网格规模受控制面资源与 sidecar 数量约束（Istio 官方测试 2000 Pod，社区实践可达上万 Pod，需横向扩展控制面）；etcd 网络延迟敏感，成员间 RTT 建议毫秒级

### 安全

**安全特性**

NetworkPolicy（K8s 原生，GA）实现 Pod 级网络隔离；Calico 提供全局网络策略与 Istio 集成（L4 为主，可扩展 L7）；Cilium 提供 L3-L7 策略（HTTP/gRPC/Kafka/FQDN），eBPF 下发策略无 iptables 规则膨胀问题；加密：Calico/Cilium/OVN-Kubernetes 支持 IPsec，Calico/Cilium 支持 WireGuard（内核级加密，性能优于 IPsec）；服务网格提供 mTLS 双向认证（Istio/Linkerd，自动证书轮换）；eBPF 安全可观测性（Tetragon）实现运行时威胁检测；DPDK/SR-IOV 直通场景需注意 VF 隔离与 vfio 权限控制（cgroup/设备插件限制）；网络安全策略误配置会导致服务不可达，建议变更前评审

**合规与认证**

Kubernetes 一致性认证涵盖网络功能（CNI 需通过 CNI 规范一致性测试）；Cilium、Linkerd、Istio 均为 CNCF 毕业项目，生态成熟；OpenShift（OVN-Kubernetes）支持 FIPS 模式，满足政府/金融合规要求；IPsec/WireGuard/mTLS 加密传输有助于满足 PCI DSS、HIPAA 等对传输加密的要求；Calico Enterprise 提供面向合规的审计与策略报告（商业版）；网络组件本身不直接涉及 FIPS 140-2/3 硬件密码模块认证，但可运行于满足 FIPS 的 OS 之上

### 运维与生命周期

**可观测性支持**

- Prometheus 指标：Calico（felix、calico-node）、Cilium（cilium_agent、cilium_endpoint）、Hubble（网络流可观测性）、OVN-Kubernetes（ovnkube 指标）、kube-proxy（iptables/ipvs 指标）
- node-exporter 提供 node_network_* 网卡收发字节/错误/丢包指标
- 诊断工具：cilium CLI（cilium monitor/bugtool）、calicoctl/calico-node、ovnkube-trace、netshoot、Multus 事件
- Hubble UI 可视化服务依赖与流量拓扑（Cilium 生态）
- Tetragon 提供 eBPF 级进程/网络事件审计
- Kubernetes Events 记录 CNI 添加/删除网络失败事件

**维护与生命周期**

CNI 组件（DaemonSet）支持滚动升级，升级期间建议逐个节点排水；节点排水后 Pod 重建会重新创建网络接口与 IP；网卡固件升级需要节点维护窗口（固件升级期间网卡不可用）；SR-IOV VF 策略（SriovNetworkNodePolicy）变更需要节点重启或重建相关 Pod；内核升级（含 BTF 变更）需验证 eBPF 程序兼容性；服务网格证书轮换（mTLS）自动化但需监控；etcd 与 API Server 的网络路径变更需谨慎（先验证再切换）；OVS（OVN-Kubernetes）升级需注意数据路径版本兼容

**弹性与故障恢复**

overlay 网络（VXLAN/Geneve）天然容忍底层链路故障（路由收敛）；Calico/Cilium BGP 模式支持 ECMP 多路径与故障切换；节点网卡故障导致 NodeNotReady，Pod 由控制器驱逐重调度（受 PDB 约束）；建议管理网与数据网分离并可选网卡绑定（bonding/链路聚合）提升可靠性；控制面多副本 + etcd 集群保障网络组件配置源高可用；NetworkPolicy/服务网格误配置可能导致大面积中断，建议灰度发布与策略先行验证；DPDK/SR-IOV 直通设备故障时对应 Pod 需重建（设备不可热迁移），关键负载建议保留备用 VF/回退网络路径

### 经济性

**成熟度与社区支持**

网络生态高度成熟：Calico 为最广泛部署的 CNI 之一（Tigera 主导），Cilium 为 CNCF 毕业项目且是 eBPF 数据面事实标准（2023 年毕业），Flannel 历史悠久但维护节奏放缓，OVN-Kubernetes 由 Red Hat 主导并作为 OpenShift 默认网络，Istio/Linkerd 均为 CNCF 毕业项目；主要云厂商（AWS EKS、GKE、AKS）均提供多种 CNI 选择；多网卡生态（Multus、SR-IOV 设备插件、Intel userspace CNI、RDMA 插件）由 k8snetworkplumbingwg 与 Intel/NVIDIA/Mellanox 等厂商持续维护；社区基准与性能调优资料丰富，厂商支持（Tigera、Isovalent/Cisco、Red Hat、Buoyant）完善

---

## 16. 电源管理与能效（CPU 电源管理 / 碳感知调度 / 能耗可观测性）——涵盖 Intel Kubernetes Power Manager、Intel Infrastructure Power Manager (IPM) Operator、AMD EPYC 能效优化（amd-pstate/CPPC）、Linux CPU 频率/C-state/P-state 控制、StarlingX 可配置电源管理（Configurable Power Manager）、碳感知调度（Carbon-aware Scheduling）与 Kepler/Scaphandre 等能耗可观测性方案

### 硬件规格

**最低配置**

Intel Kubernetes Power Manager: 需支持 Intel Speed Select Technology (SST) 的 Xeon 处理器：SST-CP 需 Xeon Scalable 系列，uncore 频率控制需 Xeon Scalable/D 系列，P-state 控制需 Sandy Bridge 或更新的 Intel CPU；软件层面至少 1 个节点的 Kubernetes 测试集群（K8s 1.23.3-1.25.4），kubelet 必须启用 static CPU 管理策略并预留系统 CPU; StarlingX 可配置电源管理: 仅支持第 3 代与第 4 代 Intel Xeon Scalable 处理器；需要 StarlingX 发行版节点并启用 static CPU 管理策略；不支持 AMD 与 Arm 处理器; AMD EPYC 能效优化: AMD EPYC 处理器（Zen2/Zen3 及以上，支持 CPPC），Linux 内核 5.x 以上且启用 amd-pstate 驱动（amd_pstate=active 等内核参数），BIOS 开启 CPPC 与 C-state 控制; Kepler 能耗可观测: 支持 RAPL/powercap 框架的 Intel 或 AMD CPU（RAPL 路径需读取 /sys/class/powercap）；无 RAPL 的硬件可用 ACPI/估算模式，但精度有限；节点需允许容器只读访问宿主机 /proc 与 /sys，无需 CAP_SYSADMIN 特权; 碳感知调度: 任意可运行 KEDA 或自定义调度器的 Kubernetes 集群；需要可访问的区域碳强度数据源（如 WattTime、Electricity Maps 或 Carbon Aware SDK 聚合 API）

**推荐配置**

Intel Kubernetes Power Manager: Intel Xeon Scalable（第 2 代及以上）裸金属服务器，Ubuntu 20.04/Rocky 8.6/CentOS 8，Kubernetes 1.23-1.25 且 kubelet 使用 static CPU 管理策略并预留系统 CPU；搭配 Node Feature Discovery (NFD) 自动检测硬件能力；每集群仅部署一个 PowerConfig; StarlingX 可配置电源管理: 第 3/4 代 Intel Xeon Scalable 服务器组成的 StarlingX 9.0+ 节点池，控制面与工作负载节点分离，工作负载以 Guaranteed QoS（requests == limits）运行以匹配每核电源配置; AMD EPYC 能效优化: AMD EPYC 9004/9005 系列（Genoa/Turin）裸金属节点，BIOS 选择 Maximum Efficiency 或自定义电源档位（Determinism Slider 设为 Power、Efficiency Mode 开启、Memory Power Down 开启），内核启用 amd-pstate active 或 guided 模式，配合 tuned 或 Node Tuning Operator 统一下发 sysfs 配置; Kepler 能耗可观测: 每节点以 DaemonSet 运行 Kepler 的 Kubernetes 集群（裸金属最佳），搭配 Prometheus 与 Grafana 展示 kepler_node_cpu_watts、kepler_container_package_joules_total 等指标；GPU 节点可附加 NVIDIA DCGM 数据; 碳感知调度: 生产集群部署 KEDA（含碳强度 Scaler）或碳感知调度器扩展（如 CarbonScaler、CNA Operator），接入 WattTime/Electricity Maps API；批处理与弹性任务优先，延迟敏感任务需评估碳延迟时间窗

### 兼容性

**支持的 K8s 版本范围**

Intel Kubernetes Power Manager: 支持 Kubernetes 1.23.3 至 1.25.4，项目已由 Intel 归档停止维护（仅适用于旧版集群）; Intel IPM Operator: 面向 Red Hat OpenShift 环境持续发布（如 26.05 版本），支持 OpenShift 相应版本（具体范围见发行说明，未核实）; StarlingX 可配置电源管理: 随 StarlingX 发行版交付（StarlingX 9.0 起提供），不面向上游 K8s 独立发布；文档覆盖 StarlingX 10.0/11.0/12.0; Kepler: 兼容主流 Kubernetes 版本（1.23+ 实测广泛），通过 Helm/Kustomize 部署，无严格上游版本绑定; 碳感知调度组件: KEDA 支持 Kubernetes 1.19+（KEDA 2.x 系列）；碳感知调度研究原型（CarbonScaler 等）多为实验性，无统一版本声明; Kubernetes 原生能力: CPU Manager 是 K8s 原生功能（static 策略 GA），是各电源管理方案的前置依赖；K8s 上游本身不提供电源管理 API

**操作系统兼容性**

Intel Kubernetes Power Manager: Ubuntu 20.04、Rocky 8.6、CentOS 8; Intel IPM Operator: Red Hat OpenShift（RHEL CoreOS）; StarlingX 可配置电源管理: StarlingX 发行版（基于 CentOS Stream 内核的容器化 Linux），版本 9.0 及以上; AMD EPYC 能效优化: 主流 Linux 发行版（RHEL/Rocky/AlmaLinux、Ubuntu、SLES 等），内核需支持 amd-pstate（内核 5.17+ 完善，SLES 15 SP6 提供 EPYC 9005 优化指南）；Windows Server 上的 K8s 节点无等效 amd-pstate 控制; Kepler: 主流 Linux（Ubuntu、RHEL、Fedora 等），依赖 powercap/RAPL 或可用的估算接口; Scaphandre: GNU/Linux 与 Windows 10/11、Windows Server 2016/2019/2022（提供 RHEL、Debian、Windows、NixOS 软件包）

**K8s 上游支持阶段**

Kubernetes 上游: 无原生电源管理功能（非 Alpha/Beta/GA 特性）；最接近的是 CPU Manager（static 策略，GA）与拓扑管理，它们只分配 CPU 不控制功耗; Intel Kubernetes Power Manager: 社区/厂商项目，已归档（archived），Intel 不再维护，仅适用于旧版集群，不推荐新部署; Intel IPM Operator: 厂商项目（Intel + Red Hat OpenShift 生态），持续发布，生产可用; StarlingX 可配置电源管理: 社区项目（StarlingX，OpenInfra 基金会），随 StarlingX 9.0+ 发布，面向电信/边缘生产场景; Kepler: CNCF Sandbox 项目（2023 年入选），由 sustainable-computing-io 社区（红帽、IBM 等）维护，生产部署案例持续增长，尚未达到 CNCF 孵化/毕业阶段; Scaphandre: 社区项目（hubblo-org），早期阶段（v1.0 路线图中），无 CNCF 身份; 碳感知调度: 2025 年学术研究热点（arXiv 调研、HotCarbon/e-Energy 会议），生产落地以 KEDA Scaler（CNCF 毕业项目 KEDA 的扩展）与碳感知 Operator 为主

### 限制与约束

**已知限制**

Intel Kubernetes Power Manager: 项目已归档（Intel 停止维护）；仅支持 K8s 1.23.3-1.25.4；每集群仅允许一个 PowerConfig；共享工作负载要求名称带 shared- 前缀；SST-BF 与 SST-TF 仅为规划功能未实现；v2.4.0 对 reservedCPUs CRD 结构引入破坏性不兼容变更；TDP 限制未实现; StarlingX 可配置电源管理: 仅支持第 3/4 代 Intel Xeon Scalable（不支持 AMD/Arm）；每个 Pod 仅支持单一 Power Profile；CPU 请求与限制不匹配（非 Guaranteed）时不受支持；启用管理标签后最大配置 MHz 参数被禁用；配置不一致可能导致状态异常或设置完全不生效; CPU 电源控制通用限制: 需要 kubelet CPU Manager static 策略并预留系统 CPU，与默认 CPU 管理/弹性调度冲突；深度 C-state 会引入唤醒延迟（影响延迟敏感型应用）；intel_pstate 的 powersave 与 performance 行为在部分发行版被映射，语义与 acpi-cpufreq 不同；云虚拟机中通常无法访问 RAPL/MSR，电源控制基本不可用; AMD EPYC 能效优化: amd-pstate 依赖 CPPC 硬件支持（较老的 Zen1 及部分 Zen2 不支持或支持不完整）；部分 BIOS 默认关闭 CPPC 或 C-state 需要重新配置并重启；动态 EPP（amd_dynamic_epp）默认可能未启用; Kepler/能耗可观测: 非 RAPL 估算方式（HWMon、NVIDIA GPU、Redfish/BMC）为实验性，准确性有限；云虚拟机环境 RAPL 不可用时只能估算；容器级功耗按活动 CPU 使用量分摊，与实际能耗存在误差; 碳感知调度: 依赖区域碳强度数据 API 的准确性与实时性（WattTime/Electricity Maps 覆盖范围与精度不一）；需要批处理/可延迟任务才有明显收益，在线延迟敏感工作负载收益有限；多数研究原型（CarbonScaler、Caspian、PCAPS 等）仍处于模拟/实验验证阶段

### 配置与部署

**配置方式**

Intel Kubernetes Power Manager: Kubernetes Operator + CRD（PowerConfig、PowerProfile、PowerWorkload）+ DaemonSet 节点代理，通过扩展资源（Extended Resource）而非 Device Plugin 暴露电源能力；要求 kubelet static CPU 管理策略与预留系统 CPU，推荐 NFD 自动打标签（也支持手动节点标签）；Helm Chart 安装; StarlingX 可配置电源管理: Operator + 节点代理 + CRD（NodePM、SharedProfile、WorkloadPM），管理员通过 YAML manifest 或 Helm overrides 定义电源 Profile（performance/balance-performance/balance-power 及自定义），监控 Guaranteed QoS Pod 并按核应用设置; CPU 频率/C-state/P-state 通用控制: sysfs 动态配置（/sys/devices/system/cpu/cpu*/cpufreq/scaling_governor、scaling_min/max_freq、cpuidle 状态）、内核启动参数（amd_pstate=active/passive/guided、intel_pstate=passive）、tuned/tuned-adm 配置集或红帽 Node Tuning Operator 统一下发; AMD EPYC 能效优化: BIOS 电源档位 + 内核 amd-pstate 驱动 + EPP（Energy Performance Preference）动态调整（amd_dynamic_epp=enable），结合 sysfs 或 tuned 配置 per-node 策略; 碳感知调度: KEDA ScaledObject + Carbon Intensity Scaler（Watts 指标或 API 数据源）；或碳感知 Operator（如 CNA/Carbon Aware K8s Operator）；或自定义调度器/调度插件（研究原型如 CarbonScaler 动态调整服务器分配、PEAKS、NPAKS 等）；接入 WattTime/Electricity Maps/Carbon Aware SDK; 能耗可观测性: Kepler（Helm 从 OCI registry 安装、Kustomize、Docker Compose、本地二进制）、Scaphandre（Helm Chart、prometheus exporter）、node_exporter（rapl/ipmi collector）、Redfish/BMC 遥测采集

**配置示例**

Intel Kubernetes Power Manager PowerProfile: apiVersion: power.intel.com/v1
kind: PowerProfile
metadata:
  name: performance
spec:
  powerParameters:
    epp: performance
    minFrequency: 2000000
    maxFrequency: 3800000
    cStates: ["C0", "C1"]; Intel Kubernetes Power Manager PowerConfig: apiVersion: power.intel.com/v1
kind: PowerConfig
metadata:
  name: power-config
spec:
  powerProfiles:
  - name: performance
    max: 100
  - name: balance-power
    max: 0; StarlingX NodePM 配置: # 通过 kubectl 应用 NodePM CR，指定节点与电源 Profile
apiVersion: starlingx.windriver.com/v1
kind: NodePM
metadata:
  name: node-pm-worker-0
spec:
  nodes:
  - worker-0
  powerProfile: balance-performance; sysfs 手动设置 governor: echo powersave | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
echo 1500000 | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq; 内核参数启用 amd-pstate: # /etc/default/grub 或内核命令行
GRUB_CMDLINE_LINUX="amd_pstate=active amd_prefcore=enable amd_dynamic_epp=enable"; KEDA 碳感知 ScaledObject: apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: carbon-aware-job
spec:
  scaleTargetRef:
    name: my-batch-job
  triggers:
  - type: carbon-intensity
    metadata:
      index: "0"
      carbon-intensity-percentile: "50"; Kepler Helm 安装: helm repo add kepler https://sustainable-computing-io.github.io/kepler-helm-chart
helm install kepler kepler/kepler --namespace kepler --create-namespace; tuned 电源 Profile: sudo tuned-adm profile powersave
sudo tuned-adm profile balanced

### 性能特征

**基准性能数据**

Intel 电源管理: Intel 官方：在支持的 Xeon 上通过 uncore 频率调整最多可节省 40% CPU 功耗；Intel 技术文章展示 TuneD + Kubernetes Power Manager 可优化资源利用率（具体数值因负载而异）；SST-CP 通过核心优先级（Performance/Balance Power）分配功率，效果取决于工作负载混合; AMD EPYC 能效: amd-pstate 相对 acpi-cpufreq 的每瓦性能测试（Tbench/Gitsource）显示两者差异很小，但 amd-pstate 提供更细粒度频率管理与 EPP 能效偏好；AMD 官方宣称 EPYC 每瓦性能领先（具体百分比未核实）；BIOS Maximum Efficiency 档位（开启 Efficiency Mode、Determinism=Power、Memory Power Down）可显著降低空载功耗（实测数据因平台而异）; 碳感知调度 (2025 学术研究): 2025 年 arXiv 调研与会议论文：PCAPS 框架在保持性能的同时最多减少 32.9% 排放；CarbonScaler 对批处理任务最高实现 51% 碳节约；Caspian 降低约 33% 排放；PPO 强化学习调度器最高 24% 能耗节约；GreenPod 对 AIoT 工作负载最高 39.1% 能耗节约；综合调度方案减少 10%-20% 功耗; 能耗可观测性: Kepler 提供节点/容器级功耗指标（如 kepler_node_cpu_watts、kepler_container_package_joules_total），基于 RAPL 的测量误差小（同一 socket RAPL 读数），估算模式精度随模型而异；Scaphandre 提供主机/进程级能耗指标；node_exporter 暴露 node_rapl_package_joules_total 等 RAPL 指标

### 安全

**安全特性**

Kepler: 设计为低权限：仅需只读访问宿主机 /proc 与 /sys，无需 CAP_SYSADMIN 等特权，降低被攻破后的影响面; RAPL/MSR 侧信道防护: RAPL 功率读数可被未授权用户态程序用于侧信道信息泄露（Platypus 类攻击），生产环境应锁定 MSR（msr-lock / 限制 msr 模块访问），电源管理 DaemonSet 需最小权限与隔离; 节点隔离: 电源管理组件（Operator/节点代理/DaemonSet）建议使用专用 ServiceAccount 与 RBAC 最小权限，避免授予集群级写权限; 碳感知数据链路: 依赖第三方碳强度 API（WattTime/Electricity Maps 等），需 TLS、API 密钥管理与数据完整性校验，防止篡改导致调度异常; StarlingX: 基于安全加固的发行版交付（StarlingX 提供认证与密钥管理等安全机制，具体电源管理相关安全特性未详细核实）

### 运维与生命周期

**可观测性支持**

能耗指标: Kepler（kepler_node_cpu_watts、kepler_container_package_joules_total、kepler_node_platform_watts 等）、Scaphandre Prometheus exporter、node_exporter（node_rapl_package_joules_total、node_ipmi_power_watts）、Redfish/BMC 平台功耗遥测; 电源状态指标: StarlingX 提供 Current Power Consumption 与 Per Core CPU Power usage 指标；sysfs 可查询当前频率/governor/C-state 状态；tuned-adm active 查看生效 Profile; 碳强度指标: WattTime / Electricity Maps API 提供区域实时碳强度（gCO2/kWh），KEDA carbon-intensity Scaler 提供 scaledobject 级碳指标; 监控集成: 全部通过 Prometheus + Grafana 生态集成，社区提供 Kepler/Scaphandre Grafana 面板；支持告警（如节点功耗超阈值、碳强度过高触发任务暂停）

### 经济性

**成熟度与社区支持**

Kepler: 活跃度高：CNCF Sandbox 项目，sustainable-computing-io 社区（红帽、IBM 等）维护，与 K8s 生态集成成熟，持续发布; KEDA 碳感知: KEDA 为 CNCF 毕业项目（碳强度 Scaler 为扩展生态），Azure、社区博客与咨询公司有生产实践分享，活跃度高; StarlingX CPM: 由 StarlingX/OpenInfra 社区维护，电信边缘场景成熟度较高，版本节奏稳定（半年一版）; Intel IPM Operator: Intel + Red Hat 联合维护，面向 OpenShift 生产环境，持续发布，成熟度高; Intel Kubernetes Power Manager: 已归档，社区停止维护，仅历史参考; Scaphandre: 社区活跃但项目早期（v1.0 前），CNCF 无官方身份，生产案例有限; 碳感知调度学术生态: 2025 年研究非常活跃（arXiv 2508.05949 综述、HotCarbon 2025、e-Energy 等会议），但生产化工具仍以 KEDA/Operator 为主

---

## 17. RDMA / InfiniBand 高速互联（InfiniBand、RoCE v2，含 RDMA 设备插件与 CNI 生态）

### 硬件规格

**最低配置**

至少 1 块支持 RDMA 的网卡：InfiniBand HCA（如 NVIDIA/Mellanox ConnectX-4/5/6/7/8）或支持 RoCE v2 的以太网网卡（ConnectX-5 及以上为主流，Intel/Broadcom 部分型号支持有限）；服务器需支持 SR-IOV（BIOS 开启）以便 VF 直通；Linux 内核需加载 mlx5_core、ib_core、ib_uverbs、ib_umad、rdma_cm 等模块并安装 rdma-core/MLNX_OFED 用户态库；InfiniBand 网络必须存在活动的 Subnet Manager（OpenSM 可部署于任意低配节点，主备模式）；CPU/内存无特殊硬性要求（RDMA 数据面卸载到网卡）

**推荐配置**

每节点 1-8 个 100/200/400Gb/s HCA（生产主流 NVIDIA ConnectX-6/7/8 或 BlueField），AI 训练节点通常按每 GPU 一个 200G/400G 端口配置；RoCE 场景要求无损/半无损以太网基础设施（PFC、ECN、DSCP QoS 分类）；InfiniBand 场景需专用交换机（如 Quantum-2 NDR 400G）与 OpenSM/UFM；GPU 节点启用 GPUDirect RDMA 并与 NCCL 配合；内核 5.6+（RDMA 网络命名空间隔离与 GUID 管理需要）；大页内存与 IOMMU（vfio-pci）用于 DPDK 等场景

**生产级配置**

InfiniBand 方案：NVIDIA Quantum-2（NDR 400G）/Quantum-3 交换机胖树或两层 CLOS 拓扑，全网无损；每节点 8 口 400G HCA（与 8 卡 GPU 一一对应）；UFM（Unified Fabric Manager）集中管理 + OpenSM 主备；多租户启用 PKey 分区与 QoS 策略。RoCE 方案：400G 以太网（Meta 2024 年生产集群已从 200G 升级到 400G，单集群 24K-65K GPU），结合 PFC/ECN 与接收端驱动的流量准入控制。控制面/管理网络与 RDMA 数据网络物理分离

### 兼容性

**支持的 K8s 版本范围**

设备插件 API 自 K8s 1.26 起为 stable（GA），RDMA 设备插件（rdma-shared-device-plugin、sriov-network-device-plugin）为社区项目，在 K8s 1.13+ 上长期生产实践；NVIDIA Network Operator v25.4.0 官方验证支持 K8s 1.29-1.32；DevicePluginCDIDevices 特性（CDI 方式分配设备）v1.29 进入 Beta、v1.31 起 GA；DRA（Dynamic Resource Allocation）v1.34 起 GA，面向网络设备的 DRA 资源声明（KEP-4817）仍在演进，是 RDMA/网卡资源管理的下一代方向

**操作系统兼容性**

仅 Linux（Windows 节点不支持 RDMA 设备插件，RDMA 在 Windows 上支持极其有限）；NVIDIA Network Operator v25.4.0 验证的发行版：Ubuntu 22.04/24.04 LTS、RHEL 8/9、Red Hat CoreOS、SLES 15 SP6；MLNX_OFED/DOCA-OFED 驱动支持主流 Linux 发行版与内核版本；RDMA 网络命名空间隔离需要内核 5.6+；rootless/用户命名空间（KEP-2033，K8s 1.36 GA）场景在 Ubuntu 等发行版上已有 2025 年研究验证

**K8s 上游支持阶段**

设备插件 API 为 GA（stable，v1.26+）；DRA 为 GA（v1.34+）；RDMA 相关组件（rdma-shared-device-plugin、rdma-cni、ib-sriov-cni、sriov-network-device-plugin、sriov-network-operator）为 k8snetworkplumbingwg 社区项目，生产环境广泛使用但非 K8s 上游官方特性；NVIDIA Network Operator 为厂商官方支持的生产级部署方案；InfiniBand 本身不是 K8s 上游功能，完全依赖厂商与社区生态

**生态兼容性矩阵**

Multus CNI（Pod 附加网络/多网卡事实标准）、Whereabouts IPAM（跨节点 IP 分配）、sriov-network-operator（SR-IOV 生命周期管理）、NVIDIA Network Operator（NicClusterPolicy CRD 一体化部署驱动/插件/CNI/IPAM）、Node Feature Discovery（NFD 提供 rdma.available 等节点标签供调度）、ib-kubernetes + UFM（InfiniBand PKey/GUID 动态管理）、perftest/ib_write_bw（基准测试）、NCCL/UCX/OpenMPI/MPI（HPC 与 AI 训练通信库，RDMA 是事实标准传输）、Prometheus 指标（UFM 遥测、Network Operator 指标、node_exporter 网卡计数）、Topology Manager（NUMA 感知调度）

### 限制与约束

**已知限制**

- RDMA 直通设备不可热迁移：节点排水/故障后 Pod 必须重建，VF 重新分配，RDMA 连接无法迁移，训练作业需 checkpoint 恢复
- K8s NetworkPolicy 只作用于主 CNI 网络，无法管控 Multus 附加的 RDMA/RoCE 网络，租户隔离需依赖 IB PKey/分区、VLAN、ACL 或 UFM 策略
- 共享模式（rdma-shared-device-plugin）多个 Pod 共享同一 HCA，缺乏原生带宽/QoS 隔离；2025 年研究（Noisy Neighbor）证实恶意租户可发起 RDMA 资源耗尽攻击，导致邻居吞吐下降约 94%、延迟放大千倍以上
- InfiniBand 依赖 Subnet Manager 常驻（OpenSM 主备或 UFM），SM 故障会导致整网不稳定
- RoCE v2 需要无损以太网配置（PFC、ECN、QoS DSCP），公有云 VM 默认不可用，需裸金属或专用 HPC 实例
- 网卡生态集中：生产级 RDMA 基本依赖 NVIDIA/Mellanox ConnectX 系列，Intel/Broadcom 的 RoCE 支持有限
- RDMA 数据面无原生加密（InfiniBand 无 IPsec 支持，RoCE 数据面加密方案有限），安全敏感场景需额外措施
- 共享 HCA 场景需谨慎管理 /dev/infiniband 设备暴露与 IPC_LOCK 等权限，容器逃逸后攻击面较大
- 每端口 VF 数量受限（ConnectX-7 每端口最多 127 个 VF），大规模多租户 VF 资源可能不足

**混部兼容性**

RDMA 网卡与普通业务混部时，共享模式下 HCA 的 QP/PD/带宽资源会被多个租户抢占且无内置隔离，网络密集负载会互相干扰（Noisy Neighbor 攻击研究已证实）；与 GPU 混部需保证 PCIe/NUMA 拓扑亲和（Topology Manager + 设备插件 GetPreferredAllocation），避免跨 NUMA 访问降低 GPUDirect RDMA 性能；建议管理网（TCP）与数据网（RDMA）物理分离，避免拥塞互相影响；InfiniBand PKey 分区与 CPU/内存混部无直接冲突

**性能开销**

RDMA 绕过内核协议栈与 CPU 拷贝，容器化开销极低：社区实测 400Gb/s 链路容器内可达约 46GB/s 带宽、约 2.7µs 延迟，较 TCP/IP 延迟降低 20-40 倍；SR-IOV VF 直通无 vSwitch/虚拟化开销；共享模式下多租户争用会导致吞吐与尾延迟劣化；rootless（用户命名空间）容器场景下，2025 年研究（Usernetes + InfiniBand + GPU）显示部分基准测试在大规模下存在可测量的额外开销，需要进一步优化

**固件与驱动依赖**

网卡固件需与 PSID 匹配（NVIDIA ConnectX 系列通过 mlxup/NVIDIA 固件工具升级，升级需节点维护窗口，通常需重启网卡或节点）；驱动依赖 MLNX_OFED/DOCA-OFED 与内核模块（mlx5_core、ib_core、ib_uverbs、ib_umad、rdma_cm）及用户态 rdma-core；RDMA 网络命名空间隔离需内核 5.6+；SR-IOV 需 BIOS 开启 VT-d/IOMMU 并加载 vfio-pci；InfiniBand 交换机固件需与 OpenSM/UFM 版本匹配；网卡热插拔支持有限，设备变更通常需重建 Pod 或重启节点

### 配置与部署

**配置方式**

- RDMA 共享设备插件（DaemonSet + JSON ConfigMap，NFD 标签选择节点，资源名如 rdma/rdma_shared_device_a，单 HCA 默认最多 1000 Pod 共享）
- SR-IOV 设备插件（sriov-network-operator + SriovNetworkNodePolicy CRD 分配 VF，selectors 中启用 isRdma 声明 RDMA 资源池）
- Multus CNI + rdma-cni / ib-sriov-cni：将 RDMA 接口移入 Pod 网络命名空间或直接挂载 VF（附带 NetworkAttachmentDefinition CRD 与 Pod 注解）
- NVIDIA Network Operator（Helm 安装 + NicClusterPolicy CRD）：一体化部署 DOCA 驱动、RDMA/SR-IOV 设备插件、Multus、IPAM、NFD、OVS 卸载等
- DRA（K8s 1.34+ GA）：以 ResourceClaim 声明网络硬件资源，网络 DRA 驱动（KEP-4817）仍在演进，未来可替代部分 CNI/设备插件职责
- InfiniBand 专用：ib-kubernetes 守护进程 + UFM 插件动态管理 PKey 与 GUID（Pod 携带 mellanox.infiniband.app 标签自动获得分区键）
- 手动部署：裸金属上安装驱动（mlnxofedinstall）、加载模块、手工配置设备插件 ConfigMap

**配置示例**

RDMA 共享设备插件 ConfigMap: kind: ConfigMap
metadata:
  name: rdma-devices
  namespace: kube-system
data:
  config.json: |
    {
      "periodicUpdateInterval": 300,
      "configList": [{
        "resourceName": "rdma_shared_device_a",
        "rdmaHcaMax": 1000,
        "selectors": {
          "vendors": ["15b3"],
          "deviceIDs": ["101b", "101d"],
          "drivers": ["mlx5_core"],
          "linkTypes": ["IB", "Ethernet"]
        }
      }]
    }; SriovNetworkNodePolicy（SR-IOV VF + RDMA）: apiVersion: sriovnetwork.openshift.io/v1
kind: SriovNetworkNodePolicy
metadata:
  name: rdma-policy
spec:
  nodeSelector:
    feature.node.kubernetes.io/rdma.available: "true"
  numVfs: 8
  resourceName: rdma_shared_vf
  deviceType: netdevice
  isRdma: true
  nicSelector:
    vendor: "15b3"
    pfNames: ["ens1f0"]; Pod 请求 RDMA 资源（共享模式）: spec:
  containers:
  - name: hpc-app
    image: my-hpc-app:latest
    securityContext:
      capabilities:
        add: ["IPC_LOCK"]
    resources:
      limits:
        rdma/rdma_shared_device_a: 1
        memory: 1Gi
      requests:
        rdma/rdma_shared_device_a: 1
        memory: 1Gi; SR-IOV 附加网络 + Pod 注解（直通模式）: apiVersion: k8s.cni.cncf.io/v1
kind: NetworkAttachmentDefinition
metadata:
  name: ib-net
spec:
  config: '{"cniVersion":"0.3.1","type":"ib-sriov","deviceID":"0000:3b:00.2","pkey":"0x8001","ipam":{"type":"whereabouts","range":"10.56.217.0/24"}}'
---
# Pod:
metadata:
  annotations:
    k8s.v1.cni.cncf.io/networks: ib-net
spec:
  containers:
  - name: hpc-app
    resources:
      limits:
        rdma/rdma_shared_vf: 1

**部署位置与环境**

裸金属自建集群是 RDMA/InfiniBand 的主流与最佳环境（HPC、AI 训练）；私有云（OpenStack/KubeVirt 等）通过 SR-IOV VF 直通支持；公有云：部分云厂商提供 RDMA 能力（Azure HPC/InfiniBand 实例、AWS EFA（类 RDMA 但非 IB/RoCE 标准）、阿里云 eRDMA/RoCE 实例、Oracle Cloud HPC 实例），均需专用实例类型或裸金属；虚拟化环境需 PCIe 直通，VM 内再跑容器（嵌套）性能有损；边缘场景一般不涉及；混合部署兼容性一般，IB 与 RoCE 网络不能直接互通（可经网关/双栈节点桥接，非透明）

**虚拟化兼容性**

SR-IOV VF 直通给虚拟机（KubeVirt/OpenStack/VMware）是常见形态，需宿主机开启 IOMMU（vt-d/amd-vi）并以 vfio-pci 透传；VM 热迁移（vMotion/在线迁移）不支持 RDMA 直通设备，迁移前必须摘除 VF；嵌套虚拟化（VM 中再跑容器使用 RDMA）性能有损但仍可用；公有云普通 VM 无 RDMA 直通能力，需专用 HPC/裸金属实例；DPDK 等用户态数据面在虚拟化环境中依赖同样的 vfio 透传路径

### 性能特征

**基准性能数据**

InfiniBand 端到端延迟约 0.7µs（HCA 到 HCA，NDR 400G），RoCE v2 约 1.5-2µs；K8s 容器内实测（共享插件 + 400Gb/s 链路）：ib_write_bw 约 46GB/s（接近线速）、延迟约 2.7µs，较 TCP 延迟低 20-40 倍（社区测试）；NVIDIA ConnectX-7 支持 400Gb/s NDR InfiniBand 与 400GbE（PCIe Gen4/5 x16）；Meta SIGCOMM 2024 论文：RoCE 支撑 24K-65K GPU 规模的分布式 AI 训练（200G 起步、400G 演进），通过优化交换机哈希与 QP 扩展使 AllReduce 性能提升最高 40%；NCCL over RDMA（RoCE/IB）是大模型训练通信的事实标准，NVIDIA 官方在 A100/H100 集群上验证 GPUDirect RDMA 显著降低通信时间

**扩展性上限**

InfiniBand 单子网规模受 Subnet Manager 与 LID 空间限制，可支撑数万节点（多子网经 UFM 统一管理可进一步扩展）；Meta 生产验证单集群 24K-65K GPU 规模 RoCE 网络；K8s 侧受标准集群上限（约 5000 节点、15 万 Pod）约束；RDMA 设备插件每节点可分配资源受网卡 VF 数（ConnectX-7 每端口最多 127 VF）与共享上限（rdmaHcaMax 默认 1000）限制；大规模下瓶颈常出现在 Subnet Manager 处理能力、拥塞控制（PFC/ECN）与 NCCL 通信调度

### 安全

**安全特性**

InfiniBand PKey（分区键）实现网络级租户隔离，ib-kubernetes + UFM 可动态为 Pod 分配 PKey（自动分区），未授权分区间不可通信；RoCE 场景用 VLAN + QoS（DSCP 优先级）与交换机 ACL 实现租户隔离；SR-IOV VF 提供硬件级设备隔离；设备插件与设备 cgroup 控制 /dev/infiniband 与 uverbs 设备访问；Topology Manager 与设备插件 GetPreferredAllocation 保证 NUMA 亲和；2025 年研究（Noisy Neighbor，arXiv 2510.12629）揭示共享 HCA 的 RDMA 资源耗尽攻击（状态饱和与流水线饱和，可致邻居吞吐下降约 94%、延迟放大千倍以上），提出 HT-Verbs 限流框架（三级温度分层限制恶意任务）作为缓解；rootless/用户命名空间（K8s 1.36 用户命名空间 GA）2025 年研究（Usernetes + RDMA + GPU）证明无特权用户可在 rootless K8s 中使用 RDMA，降低容器逃逸与提权风险，但大规模下有性能开销；RDMA 数据面无原生加密，敏感数据需信任网络或叠加 MACsec 等链路层加密

### 运维与生命周期

**可观测性支持**

- NVIDIA UFM（Unified Fabric Manager）：全网拓扑、遥测、告警、租户（PKey）审计，支持 Prometheus 集成
- 命令行工具：ibstat/ibstatus、ibv_devinfo、mst、ibdiagnet（网络健康检查）、opensm 日志
- perftest 套件：ib_write_bw/ib_write_lat、ib_send_bw/lat 等微基准测试
- RoCE 侧：ethtool -S（网卡收发与丢包计数）、PFC/ECN 计数（交换机侧）
- K8s 侧：node_exporter 网卡指标、NVIDIA Network Operator 指标、NFD 节点标签、设备插件资源上报与 Pod 事件

**维护与生命周期**

网卡固件升级（NVIDIA mlxup 工具）需要节点维护窗口（升级期间网卡不可用，通常需重启）；节点排水时 RDMA Pod 重建并重新分配 VF/PKey；SriovNetworkNodePolicy 变更需要节点重启或重建相关 Pod；驱动升级（MLNX_OFED/DOCA-OFED）需与内核版本匹配并滚动重启节点；InfiniBand 交换机与 OpenSM/UFM 升级需主备切换保障连续性；共享模式下设备插件可滚动升级而不中断已有连接；RDMA 连接本身无迁移能力，维护事件前建议完成作业 checkpoint

**弹性与故障恢复**

InfiniBand 网络依赖 Subnet Manager 高可用（OpenSM 主备或 UFM HA），SM 故障会导致全网路由不稳定；IB 自适应路由（Adaptive Routing）与多路径（LAG/多网卡）可容忍单链路故障；RoCE 依赖以太网路由收敛与 PFC/ECN 拥塞管理，链路故障收敛速度由以太网协议决定；节点或网卡故障时 RDMA Pod 无法热迁移，需重建并通过 NCCL/MPI 作业重启（结合训练 checkpoint 恢复）；多租户场景建议将不同训练作业隔离到不同分区/VF，缩小单点故障爆炸半径；GPU+RDMA 组合节点故障对训练作业影响大，生产集群普遍要求冗余数据网络与控制面

### 经济性

**成熟度与社区支持**

生态成熟：NVIDIA 主导 InfiniBand/RoCE（ConnectX 系列 HCA、Quantum/Spectrum 交换机、Network Operator、UFM、DOCA），文档完善且为 AI/HPC 行业事实标准；k8snetworkplumbingwg（K8s 网络工作组）维护 rdma-shared-device-plugin、rdma-cni、ib-sriov-cni、sriov-network-device-plugin 等社区项目，长期活跃；NVIDIA Network Operator 是生产级厂商支持方案（OpenShift/RKE/上游 K8s 均有部署指南）；Meta（RoCE 65K GPU）、微软 Azure（InfiniBand HPC）、各大超算中心（Top500 绝大多数使用 InfiniBand）大规模生产验证；NCCL/UCX/MPI 通信库对 RDMA 支持成熟；2025 年研究热点：rootless RDMA 容器（Usernetes）、DRA 网络资源（KEP-4817）、RDMA 多租户安全（Noisy Neighbor 攻击与防御）

---

## 18. Secure Boot / 硬件信任根（UEFI Secure Boot 与 TPM 2.0 信任根）

**官方文档**: https://uefi.org/specs/UEFI/2.10/（UEFI Secure Boot 规范）
https://trustedcomputinggroup.org/resource/tpm-library-specification/（TCG TPM 2.0 规范）
https://support.microsoft.com/en-us/servicing/os/secure-boot/2025/06/windows-secure-boot-certificate-expiration-and-ca-updates（2026 证书到期官方公告）
https://docs.spectrocloud.com/clusters/edge/trusted-boot/（Spectro Cloud Palette 可信启动）
https://docs.siderolabs.com/talos/latest/platform-specific-installations/bare-metal-platforms/secureboot（Talos Linux Secure Boot）

### 硬件规格

**最低配置**

UEFI 固件（2.3.1+，支持 Secure Boot 模式）；64 位 CPU（x86-64 / ARM64）；TPM 2.0 芯片（实现度量启动与密钥密封，无 TPM 时仍可仅启用签名验证但无法密封磁盘密钥）；UEFI 变量存储（NVRAM）至少约 256 KB 以上以存放 PK/KEK/db/dbx 签名数据库；EFI 系统分区（ESP）100 MB+（可信启动需容纳 UKI 统一内核镜像）。启用 Secure Boot 本身不要求额外内存或存储，但引导分区需为 GPT+EFI 布局，传统 BIOS/MBR 不可用。

**推荐配置**

TPM 2.0（优先独立式离散 TPM，FIPS 140-2/3 认证型号）；UEFI 2.7+ 固件并开启 Secure Boot + 度量启动；ESP 分区 512 MB+；服务器平台（Intel/AMD/ARM）或云 vTPM 实例（AKS Gen2 VM、GKE Shielded VM）；用于 Kubernetes 节点时建议配合节点完整性监控（如 GKE Integrity Monitoring）与密钥托管流程（HSM 或离线 CA 保管 PK/KEK）。

**生产级配置**

离散 TPM 2.0（FIPS 140-2/3 认证、支持抗回滚与单调计数器）；HSM 或离线根 CA 管理签名密钥（PK/KEK/db），建立启动镜像签名流水线（CI/CD 对 UKI/启动加载器签名）；大容量固件闪存（≥8 MB）以容纳不断增长的 dbx 吊销列表；远程证明基础设施（Keylime 等）持续校验节点 PCR 度量；边缘场景叠加全盘加密（LUKS/BitLocker）实现可信启动闭环。

### 兼容性

**支持的 K8s 版本范围**

非 K8s 版本绑定特性（节点平台/固件级能力，K8s 无版本门槛）。云厂商实现有最低版本要求：AKS Trusted Launch 需 Kubernetes 1.25.2+ 且节点为 Gen2 VM、Azure CLI 2.66.0+；GKE Shielded Nodes 覆盖全部受支持 GKE 版本（Autopilot 默认强制启用且不可关闭）；Talos Linux、Kairos、Bottlerocket 等社区方案支持其各自支持的 K8s 版本（通常 1.24+ 至最新版）。

**操作系统兼容性**

Windows：Windows 10（1607–22H2）、Windows 11（21H2–24H2）、Windows Server 2012 ESU 至 2025（微软 2023 证书更新覆盖范围）；Linux：Ubuntu 22.04/24.04、Fedora 38+（Rawhide 已采用多证书签名的 shim）、RHEL/RHCOS（OpenShift）、Debian、openSUSE、Rocky Linux、Talos Linux、Kairos 各受支持发行版、AWS Bottlerocket；Alpine 默认无签名启动产物，需自行生成签名密钥。

**K8s 上游支持阶段**

非 K8s 上游功能（无原生 API 支持，属节点操作系统与平台固件能力）；云厂商实现均为 GA：AKS Trusted Launch（GA）、GKE Shielded Nodes（GA，Autopilot 默认启用）、OpenShift 裸金属/vSphere Secure Boot（GA）；社区项目（Talos Linux、Kairos CNCF、Bottlerocket、Keylime）达到生产可用成熟度。

**生态兼容性矩阵**

云平台：AKS Trusted Launch（vTPM+Secure Boot）、GKE Shielded Nodes（vTPM+Secure Boot+Integrity Monitoring）、AWS EC2 Nitro TPM 与 Bottlerocket（裸金属支持 Secure Boot）；节点 OS/发行版：Talos Linux、Kairos（K3s/PXK-E/RKE2）、Bottlerocket、Ubuntu（shim+GRUB+sbctl）、Fedora、RHCOS；远程证明/监控：Keylime（CNCF 项目，TPM/IMA 度量证明）、Intel Trust Authority、tpm2-tools、GKE Integrity Monitoring 日志、Windows 事件日志（Event ID 1801/5000）。

### 限制与约束

**已知限制**

1) 仅支持 UEFI 固件模式，传统 BIOS/MBR 布局不可用（Talos 明确不支持 x86 BIOS 模式）；2) 启用 Secure Boot 后无法加载未签名的第三方内核模块（GKE Ubuntu、RHCOS 等需使用签名模块或 MOK）；3) Secure Boot 仅验证启动链签名，不防护启动后的运行时攻击（内核漏洞、恶意容器逃逸），需配合度量启动与远程证明；4) 2026 年证书事件：未在到期前安装 2023 证书的系统将无法接收新的 dbx 吊销更新，吊销列表永久冻结，面临 BlackLotus（CVE-2023-24932）/BootHole 类固件攻击风险，且 BIOS 重置可能导致无法启动（需 SecureBootRecovery.efi 与 BitLocker 恢复密钥）；5) dbx 吊销数据库容量受 NVRAM 限制（空间不足时报 Event ID 1801）；6) Talos 可信启动使用 UKI，内核命令行参数固定、修改需重建镜像，且不支持从 GRUB 布局迁移（需全新安装）；7) Kairos 的 Alpine 发行版不支持 Secure Boot（无默认签名产物）；8) AKS Trusted Launch 不支持 Windows 节点、虚拟节点、可用性集、Flatcar 及部分 GPU 池场景；9) 裸金属需手动注册平台密钥（PK），PK 私钥丢失将导致设备无法信任新签名组件；10) TPM 丢失/损坏会使密封的磁盘密钥无法恢复，必须建立恢复密钥流程。

**固件与驱动依赖**

固件必须支持 UEFI Secure Boot，并能通过固件更新（capsule update）安装新签名数据库（2023 证书、dbx 吊销列表）；2026 事件要求先安装厂商 BIOS/固件更新、再部署 OS 与 2023 证书（顺序不可颠倒），并更新网络启动镜像（PXE）；NVRAM 容量需容纳 PK/KEK/db/dbx；启用 Secure Boot 后未签名内核驱动不可加载（NVIDIA 等需发行版签名或 MOK 签名流程）；密钥注册、dbx 更新、固件更新均需重启节点（先 kubectl drain 再操作），不支持热插拔类固件操作。

### 配置与部署

**配置方式**

云厂商管理面配置：AKS（az aks create / az aks nodepool add --enable-secure-boot --enable-vtpm）、GKE（gcloud container clusters create --shielded-secure-boot --enable-shielded-nodes）、EKS 通过 Bottlerocket 镜像变体；裸金属/边缘：固件 Setup 菜单注册平台密钥（PK/KEK/db）、切换 Secure Boot 模式、使用签名工具链（sbctl、talosctl gen secureboot、Kairos 密钥生成）构建并签名 UKI/启动镜像，CI/CD 签名流水线；Kairos/Palette Edge 通过 secure_boot/trusted_boot 配置项 + EdgeForge 构建不可变镜像。

**配置示例**

# GKE 创建启用 Secure Boot 的集群
gcloud container clusters create CLUSTER --enable-shielded-nodes --shielded-secure-boot
# AKS 创建启用 Secure Boot + vTPM 的节点池
az aks create -n CLUSTER -g RG --enable-secure-boot --enable-vtpm
az aks nodepool add -n NP -c CLUSTER -g RG --enable-secure-boot --enable-vtpm
# Talos Linux 生成签名 ISO（启动菜单注册密钥）
talosctl gen secureboot iso --output secureboot.iso
# Kairos 构建启用可信启动的镜像（配置中声明 secure_boot / trusted_boot: true）

**部署位置与环境**

公有云：AKS Trusted Launch、GKE Shielded Nodes、EKS Bottlerocket（vTPM，通常无额外费用）；私有云：OpenStack/KVM/VMware 需虚拟化平台提供 vTPM/安全启动支持；裸金属：BIOS/固件配置 + 密钥注册；边缘：Spectro Cloud Palette Edge/Kairos 可信启动（无人值守设备防物理篡改，结合 FDE+Secure Boot+TPM 度量）；混合部署均支持。

### 性能特征

### 安全

**安全特性**

UEFI Secure Boot：基于 PK/KEK/db/dbx 四层签名数据库体系，仅允许签名启动代码（固件驱动、引导加载器、内核）执行，阻止 bootkit/rootkit 注入；TPM 2.0 硬件信任根：PCR 平台配置寄存器、密封/解除密封、密钥层级、单调计数器、远程证明，构成可信计算基（TCB）核心；度量启动（Measured Boot）：将各启动阶段哈希写入 TPM PCR 并记录事件日志，供审计与远程证明比对；可信启动（Trusted Boot）：TPM 仅在 PCR 度量匹配已知良好值时释放磁盘加密密钥（BitLocker/LUKS），实现无人值守设备安全解密；vTPM 虚拟化（AKS Trusted Launch、GKE Shielded、EC2 Nitro）；启动完整性监控（GKE Integrity Monitoring 默认开启）；远程证明生态（Keylime、Intel Trust Authority、云厂商证明服务）；与机密计算衔接：Secure Boot/度量启动是机密 VM（SEV-SNP、TDX）信任链的前置环节。

### 运维与生命周期

**维护与生命周期**

2026 微软 Secure Boot 证书到期为全行业重大维护事件：KEK CA 2011（2026-06-24 到期）、UEFI CA 2011（2026-06 下旬到期，各来源标注 26–27 日存在差异）、Windows Production PCA 2011（2026-10-19 到期），须在到期前安装 2023 证书（Intune/组策略/注册表），按'先固件后 OS'顺序更新，更新 PXE/网络启动镜像，保留 BitLocker 恢复密钥并演练恢复流程；dbx 吊销列表随月度补丁更新（仅持有新 KEK 的系统可接收新吊销）；PK/KEK/db 密钥轮换需固件配合；Talos UKI 内核参数变更需重建镜像并重装；Kairos/Bottlerocket 采用 A/B 不可变镜像滚动更新；节点级固件/密钥操作需 drain 后重启。

**弹性与故障恢复**

平台密钥（PK）私钥丢失 = 设备无法信任任何新签名组件，签名密钥应离线保管或存 HSM，并建立密钥托管与轮换制度；TPM 损坏/更换 = 密封的磁盘密钥丢失，必须依赖 BitLocker 恢复密钥或 LUKS 恢复口令（建议集中管理恢复密钥）；BIOS 重置可能导致已吊销旧签名组件的设备无法启动（需 SecureBootRecovery.efi + 恢复密钥）；启用失败可回退为关闭 Secure Boot 的传统启动（失去保护但可恢复业务）；集群层面建议多节点冗余，避免单节点密钥/固件故障影响控制面（控制面节点也应启用可信启动）。

### 经济性

**成熟度与社区支持**

生态成熟：微软（Windows Secure Boot 与 2026 证书迁移官方 playbook）、Google（GKE Shielded Nodes）、AWS（Bottlerocket、EC2 Nitro TPM）、Red Hat（OpenShift RHCOS Secure Boot）；社区项目活跃：Talos Linux（Sidero Labs）、Kairos（CNCF Sandbox，Spectro Cloud 主导，被 Palette Edge 商业化）、Bottlerocket（AWS 开源）、Keylime（CNCF，远程证明）；2026 证书到期事件为全行业热点，微软与 Fedora 等发行版均已发布迁移指南；上游 K8s 无原生 API，成熟度体现在发行版与云平台层，整体属高成熟度安全基础设施。

---

## 19. 存储系统（节点本地存储、CSI 驱动、持久化存储后端、文件系统、NVMe 耐久度、本地 PV 调度）

### 硬件规格

**最低配置**

工作节点: 本地磁盘约 20-40 GB（nodefs，存放容器镜像、可写层、日志与 emptyDir 临时数据）；官方未规定硬性容量下限，社区实践建议至少 20 GB；无持久化需求时可仅依赖节点本地盘，不要求任何特定 IOPS; 控制面节点: etcd 数据盘官方要求常规负载 50 顺序 IOPS、恢复带宽 10 MB/s，且明确建议使用 SSD（万不得已用 HDD 时选择 15000 RPM）；容量无官方规定，社区实践约 8-16 GB 起; 说明: Kubernetes 官方未发布节点本地存储的统一最低容量标准；上述为官方 IOPS 建议（etcd）与社区实践的汇总

**推荐配置**

工作节点: 本地磁盘 100-500 GB SSD/NVMe；生产环境建议 nodefs 与 imagefs 分离（根盘与镜像盘独立）；运行本地 PV 时按需配置独立 NVMe 数据盘（如 1-2 块企业级 NVMe）; 控制面节点: etcd 专用 SSD（推荐 NVMe），重负载 500 顺序 IOPS、恢复带宽 100 MB/s 以上；磁盘容量建议 50 GB 以上以容纳 WAL、快照与压缩余量；强烈建议避免使用 NAS/网络存储承载 etcd; 说明: etcd 对磁盘写延迟（fsync）高度敏感，官方硬件指南明确要求 SSD 承载，慢盘会导致心跳超时与集群不稳定

**生产级配置**

控制面节点: etcd 独立 NVMe 数据盘（建议 3 DWPD 及以上写入耐久度），建议 RAID1/双盘镜像；3 副本集群；磁盘 IOPS 预留 2-3 倍余量以应对峰值写入与快照恢复; 工作节点: nodefs/imagefs/数据盘三分离（根盘 + 镜像盘 + 本地 PV 数据盘），全部使用企业级 NVMe；大规模集群建议存储网络 10GbE 及以上并做链路聚合; 分布式存储（Ceph/Rook）: 每个 OSD 独立磁盘且容量不低于 1 TB（官方建议），每 BlueStore OSD 分配约 8 GB 内存，OSD 节点至少 1 核（推荐 2 核）/OSD，网络官方建议 10 Gb/s 并做 bonding；WAL/DB 放 SSD/NVMe 加速，1 块 NVMe 可支撑约 10 个 HDD OSD，SATA SSD 约 4-5 个; NFS 高可用: 双机 keepalived + DRBD 或商业 HA NFS 方案，消除单点故障

### 兼容性

**支持的 K8s 版本范围**

CSI 规范 v1.0 GA（K8s v1.13+）；Local Persistent Volume v1.10 Alpha、v1.14 GA；in-tree 到 CSI 迁移 v1.17 Alpha、v1.25 整体 GA；CSIStorageCapacity v1.21 Alpha、v1.24 GA；卷快照（VolumeSnapshot）v1.20 GA；卷在线扩容 v1.24 GA（CSI）；v1.33 新增存储容量节点评分（Storage Capacity Scoring）；in-tree 存储插件自 v1.25/v1.26 起逐步移除（ScaleIO、Quobyte、StorageOS、GlusterFS 等，具体以各版本 Release Notes 为准）

**操作系统兼容性**

Linux 为主流（主流发行版的 ext4/xfs/btrfs 均可作节点根文件系统）；containerd 默认 overlayfs 快照器要求底层文件系统支持 d_type（XFS 必须以 ftype=1 格式化）；Windows Server 2019/2022 仅支持 NTFS，本地 PV 与多数本地存储 CSI 不支持 Windows；各 CSI 驱动有独立 OS 支持矩阵，云厂商驱动通常支持主流 Linux 发行版

**K8s 上游支持阶段**

CSI：GA（v1.13+）；Local PV：GA（v1.14+）；CSI 迁移：GA（v1.25+）；CSIStorageCapacity：GA（v1.24+）；卷快照：GA（v1.20+）；卷扩容：GA（v1.24，CSI）；本地存储调度依赖 kube-scheduler 的 WaitForFirstConsumer 延迟绑定（GA）；in-tree NFS/hostPath/emptyDir 仍可用但 in-tree 插件整体弃用，推荐迁移至 CSI

**生态兼容性矩阵**

本地存储：kubernetes-sigs/local-static-provisioner（sig-storage 官方参考实现）、TopoLVM（LVM 动态供应）、HwameiStor、Open-Local、Rancher Local Path Provisioner；分布式存储：Rook（Ceph 编排，CNCF 毕业项目 2020）、ceph-csi（RBD/CephFS）、Longhorn、OpenEBS、CubeFS（CNCF 毕业 2025）；NFS：nfs.csi.k8s.io（kubernetes-csi 官方维护）；云厂商：AWS EBS/EFS/FSx for Lustre/Mountpoint S3、GCE PD/Filestore/GCS、Azure Disk/File/Blob、阿里云/腾讯云/华为云 CSI；企业存储：vSphere CSI、OpenStack Cinder CSI、NetApp Trident、Dell/EMC CSI、HPE CSI、Portworx；监控集成：node_exporter（node_filesystem_*、node_disk_*）、smartmon_exporter、Ceph mgr Prometheus 模块、kubelet cAdvisor 卷指标（kubelet_volume_stats_*）

### 限制与约束

**已知限制**

- 本地 PV 与节点强绑定，节点故障会导致数据不可用，必须依赖应用层复制（etcd Raft、数据库主从）或上层备份兜底
- 本地 PV 无动态供应能力：需预建目录/磁盘，由 local-static-provisioner 自动发现或手动创建 PV；PV 容量等于目录/磁盘大小，无法超卖
- emptyDir 与节点临时存储无性能 SLA，官方明确说明应用不能期望任何 IOPS 保障
- NFS 存在单点故障风险（非 HA 场景）、锁语义与强一致性较弱，不适合高并发随机写与数据库类负载
- Ceph 对网络要求高（官方建议 10 Gb/s+），OSD 故障恢复占用带宽与 IO，小规模集群（数节点）管理与资源开销偏大
- 云盘（EBS/GCE PD/Azure Disk）绑定单一可用区，跨可用区容灾需快照复制或应用层复制；IOPS/吞吐受卷类型与容量限制
- XFS 必须 ftype=1 格式化才能作为 overlayfs 底层；btrfs 与 overlayfs 不兼容，containerd 需改用 btrfs 原生快照器
- Windows 节点仅支持 NTFS，不支持本地 PV 及多数本地存储 CSI 驱动
- CSI 快照、扩容、容量跟踪等特性依赖具体驱动实现，并非所有驱动完整支持；部分驱动需要额外用户态依赖（如 Ceph RBD 需 open-iscsi）
- in-tree 存储插件已弃用，GlusterFS、ScaleIO、Quobyte、StorageOS 等已被移除，存量数据需迁移至 CSI

**混部兼容性**

本地盘与计算负载混部时存在 IO 竞争，建议为高写入负载（etcd、数据库）独占 NVMe 盘或启用 IO 限流；Ceph OSD 与计算混部需预留 CPU/内存（每 OSD 约 1-2 核、8 GB 内存），避免影响延迟敏感应用；日志/大数据类高写入负载加速 NVMe 磨损，需按 DWPD 规划寿命；本地 PV 数据盘不建议与 kubelet 临时目录（nodefs）共用同一磁盘，避免磁盘压力驱逐

**性能开销**

CSI 块/文件挂载路径为内核原生机制，开销通常可忽略（<1%）；NFS 每次读写增加网络往返，局域网约 0.1-1 ms 额外延迟；Ceph 引入网络与 CPU 开销，典型约 5-15%；FUSE 用户态驱动（S3 挂载类，如 Mountpoint S3）开销较大，约 10-30%；磁盘加密（LUKS/dm-crypt/云盘加密）增加 CPU 开销约 1-5%（具体取决于加密算法与 CPU 指令集，如 AES-NI）

**固件与驱动依赖**

Ceph RBD 要求节点安装 open-iscsi（iscsi-initiator-utils）与 multipath-tools（多路径场景）；NFS 卷要求节点安装 nfs-utils（含 rpcbind）；NVMe 需要内核 nvme 模块与厂商固件支持；XFS 需 ftype=1 格式化；overlayfs 需要内核 4.9+；磁盘健康监控依赖 smartmontools/nvme-cli；NVMe 固件升级通常需节点排水后离线执行，部分场景影响热插拔探测

### 配置与部署

**配置方式**

- CSI 驱动部署（Helm/DaemonSet/Operator，如 Rook、TopoLVM、云厂商 CSI 驱动）
- StorageClass 定义（provisioner、volumeBindingMode、参数、allowVolumeExpansion）
- PVC/PV 声明与绑定（动态供应或静态预建）
- 本地静态供应器（local-static-provisioner DaemonSet + 目录自动发现）
- kubelet 配置（evictionHard 磁盘驱逐阈值、imagefs 分离、--max-pods）
- 手动 PV（hostPath、emptyDir、NFS 等 in-tree 卷）

**配置示例**

本地存储 StorageClass（延迟绑定）: apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: local-storage
provisioner: kubernetes.io/no-provisioner
volumeBindingMode: WaitForFirstConsumer; 本地 PV（带节点亲和性）: apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv-1
spec:
  capacity:
    storage: 500Gi
  volumeMode: Filesystem
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: local-storage
  local:
    path: /mnt/local-pv/vol-1
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - node-01; Ceph（Rook）StorageClass: apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: rook-ceph-block
provisioner: rook-ceph.rbd.csi.ceph.com
parameters:
  clusterID: rook-ceph
  pool: replicapool
  csi.storage.k8s.io/fstype: ext4
allowVolumeExpansion: true; NFS CSI StorageClass: apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: nfs-csi
provisioner: nfs.csi.k8s.io
parameters:
  server: nfs-server.example.com
  share: /exports/data
reclaimPolicy: Delete
volumeBindingMode: Immediate; kubelet 磁盘驱逐阈值: evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"; PVC 声明: apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: data-claim
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: local-storage
  resources:
    requests:
      storage: 100Gi

**部署位置与环境**

公有云（EBS/GCE PD/Azure Disk 等云盘 CSI 成熟，推荐动态供应，按需快照与扩容）；裸金属（本地 NVMe + 本地 PV，或 Ceph/Rook 分布式存储）；虚拟机（虚拟磁盘支持快照、在线扩容与热迁移，数据位于存储后端；本地直通盘不支持热迁移）；边缘（以本地存储为主，K3s + Local Path Provisioner/本地 CSI）；混合云（跨云复制依赖应用层或对象存储中转）

**虚拟化兼容性**

VM 中的云盘/虚拟磁盘支持快照、在线扩容与热迁移（数据在存储后端，VM 位置无关）；PCIe 直通 NVMe 盘不支持 VM 热迁移，需冷迁移；嵌套虚拟化下直通与 SR-IOV 存储设备依赖宿主支持；Ceph RBD 可同时作为 OpenStack VM 磁盘后端与 K8s 卷后端；NFS/CephFS 卷对 VM 位置无要求，天然支持迁移

### 性能特征

**基准性能数据**

etcd 官方硬件基准：常规负载 50 顺序 IOPS、重负载 500 顺序 IOPS，恢复带宽 10/100 MB/s；企业级 NVMe SSD 典型随机读 100 万+ IOPS、随机写 10-50 万 IOPS、顺序读 3-7 GB/s（视型号）；企业级 SATA SSD 典型随机读约 10 万 IOPS；企业 HDD 约 100-200 IOPS；社区测试表明 etcd 采用 NVMe 相比 HDD 可将 fsync 提交延迟降低一个数量级以上；Ceph 性能按“每核 IOPS”选型，单 OSD 吞吐受磁盘与网络双重限制

**扩展性上限**

K8s 官方：单集群最大 5000 节点、全集群 150000 Pod、单节点默认 110 Pod；每节点可挂载卷数由驱动限制（GCE PD 16、Azure Disk 16、AWS EBS 视实例类型默认约 39）；etcd 建议 3-5 成员；Ceph 可扩展至数千 OSD，但集群越大故障恢复时间越长；CSIStorageCapacity 提供拓扑感知调度，缓解卷容量不足导致的调度失败

**每节点密度**

默认每节点 110 Pod（kubelet --max-pods 可调）；每节点卷数受驱动限额约束；本地 PV 密度受物理磁盘/目录数量限制（通常 1-2 块 NVMe 数据盘 + 若干子目录分区）；Ceph 建议一盘一 OSD，避免单盘多 OSD 争抢 IO

### 安全

**安全特性**

静态加密：云盘加密（EBS/GCE PD/Azure Disk）、LUKS/dm-crypt、Ceph OSD 加密、支持加密卷的 CSI 驱动；传输加密：Ceph msgr2 加密、NFS Kerberos（krb5）、云 API TLS；访问控制：RBAC 隔离 StorageClass/PVC/VolumeSnapshot/CSI 资源，PVC 可限定拓扑（allowedTopologies）；CSI 凭据通过 Secret 管理；PV 采用 Retain 回收策略防止误删数据；本地 PV 数据残留需手动安全擦除（shred 或依赖全盘加密）

**合规与认证**

CSI 驱动可通过 CSI 一致性测试（CSI Conformance）；Kubernetes 一致性认证（SIG Storage）覆盖存储 API；FIPS 140-2/3 合规取决于加密实现（LUKS/云盘加密/内核加密模块）；PCI DSS、HIPAA 等要求静态数据加密与访问审计，可通过云盘加密、LUKS、Ceph 加密满足

### 运维与生命周期

**可观测性支持**

- kubelet cAdvisor：kubelet_volume_stats_available_bytes/used_bytes/inodes 等卷级指标
- node_exporter：node_filesystem_*、node_disk_*（IOPS、吞吐、延迟、队列深度）
- smartmontools（smartctl）与 nvme-cli（nvme smart-log）：磁盘健康、温度、剩余寿命与写入量
- Ceph：mgr Prometheus 模块 + Rook CRD 状态，提供集群健康、OSD 状态与性能指标
- CSI 驱动自身指标（如卷状态、请求统计）
- Kubernetes Events：卷挂载失败、Attach/Detach 错误、VolumeFailedMount 等事件
- kubectl describe pv/pvc、kubectl get events 排查绑定与挂载问题

**维护与生命周期**

NVMe 寿命管理：依据 TBW（总写入字节）与 DWPD（每日整盘写入次数）评估寿命，读写混合型 1-3 DWPD、写入密集型 3-5 DWPD 为常见等级，5 年质保期内耗尽 TBW 是主要失效模式；定期执行 smartctl/nvme smart-log 检查剩余寿命、温度与重映射扇区；固件升级需 drain 节点离线执行；NVMe 支持热插拔，但 K8s 侧通常需重新探测或重启 kubelet；节点排水前确认 PVC 数据已迁移或备份；etcd 定期压缩（defrag）与快照备份；Ceph OSD 更换遵循 Rook 删除-替换流程；卷扩容与快照可通过 CSI 在线执行

**弹性与故障恢复**

本地盘建议 RAID1/RAID10 或硬件 RAID（etcd 等关键场景）；FC/iSCSI 多路径（DM Multipath）提供链路冗余；Ceph 默认 3 副本或纠删码（EC）自愈，容忍 OSD/节点故障；NFS 高可用通过双机 keepalived+DRBD 或商业 HA 方案；云盘依赖快照与跨可用区复制（通常需应用层配合）；本地 PV 单节点故障即数据不可用，必须依赖应用层复制（etcd Raft 3/5、数据库主从）；备份策略：Velero（支持 CSI 快照）+ 定期恢复演练

### 经济性

**成熟度与社区支持**

存储生态成熟：CSI 规范 GA 多年，AWS/GCP/Azure 及主流存储厂商全面支持；Rook 为 CNCF 毕业项目（2020 年），CubeFS 2025 年 CNCF 毕业；local-static-provisioner 为 sig-storage 官方参考实现；ceph-csi 与 nfs.csi.k8s.io 由 kubernetes-csi 社区维护；Longhorn、OpenEBS、TopoLVM 等社区活跃；本地 NVMe 方案硬件成本低但冗余依赖应用层，Ceph/云盘方案单 GB 成本更高但自带冗余与运维便利

---

## 20. 可信执行环境 (TEE) / 机密计算 (Confidential Computing)

**官方文档**: K8s 官方博客 (Confidential Kubernetes): https://kubernetes.io/blog/2023/07/06/confidential-kubernetes/ ; Confidential Containers (CoCo, CNCF 项目): https://confidentialcontainers.org/ 与 GitHub: https://github.com/confidential-containers/confidential-containers ; CoCo 证明文档 (Trustee/KBS): https://confidentialcontainers.org/docs/attestation/ ; Intel TDX 文档: https://www.intel.com/content/www/us/en/developer/tools/trust-domain-extensions/documentation.html 与 TDX 性能文章: https://www.intel.com/content/www/us/en/developer/articles/technical/trust-domain-extensions-on-4th-gen-xeon-processors.html ; Intel SGX 设备插件: https://intel.github.io/intel-device-plugins-for-kubernetes/cmd/sgx_plugin/README.html ; AMD SEV 开发者页面: https://www.amd.com/en/developer/sev.html ; NVIDIA 机密计算: https://www.nvidia.com/en-us/data-center/solutions/confidential-computing/ 与 GPU Operator 机密容器文档: https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/25.3.4/gpu-operator-confidential-containers.html ; CoCo NVIDIA GPU 示例: https://confidentialcontainers.org/docs/examples/nvidia-gpu-examples/ ; GKE 机密节点: https://docs.cloud.google.com/kubernetes-engine/docs/how-to/confidential-gke-nodes ; AKS 机密容器: https://learn.microsoft.com/en-us/azure/aks/confidential-containers-overview ; Azure 机密 VM 选项: https://learn.microsoft.com/en-us/azure/confidential-computing/virtual-machine-options ; CNCF 博客 (TPM 组合远程证明, 2025): https://www.cncf.io/blog/2025/10/08/a-tpm-based-combined-remote-attestation-method-for-confidential-computing/ ; Red Hat CoCo 裸金属部署: https://developers.redhat.com/articles/2025/02/19/how-deploy-confidential-containers-bare-metal

### 硬件规格

**最低配置**

Intel TDX: 第 4 代至强可扩展 (Sapphire Rapids) 及更新 CPU，需 BIOS/UEFI 开启 TDX 与 MKTME (多密钥全内存加密) 并从物理内存中划分 TDX 预留内存；Linux 内核 5.19+ (TDX 支持并入主线 5.19)；KVM/QEMU 虚拟化栈。; AMD SEV-SNP: AMD EPYC 7003 (Milan) 及更新 (9004 Genoa 等)，需 BIOS 开启 SEV/SEV-SNP 与 IOMMU，并预留 SNP 内存 (系统建议 8GB+ 内存，16GB 以上更佳)；Linux 内核 5.19+ (SEV-SNP guest 支持)。; Intel SGX: 支持 SGX 的 Intel 平台 (第 3 代至强可扩展 Ice Lake 等服务器 CPU 或客户端 CPU)，需 BIOS 开启 SGX 与 Flexible Launch Control (FLC)；EPC (Enclave Page Cache) 内存由 BIOS 静态划分；Linux 内核 5.11+ 内嵌驱动或外部 DCAP 驱动 1.41+；SGX 应用需改写为 enclave (LibOS 如 Gramine/Occlum)。; NVIDIA 机密 GPU: NVIDIA Hopper H100 及以上 (Blackwell、RTX Pro 6000 BSE) 加速卡，运行在支持 SEV-SNP (AMD) 或 TDX (Intel) 的平台上；需 NVIDIA 优化 Linux 内核与定制 initramfs，GPU 绑定 vfio-pci 驱动；主机需开启 IOMMU、BIOS 硬件虚拟化与 Access Control Services (ACS)。; 软件栈 (通用): Kubernetes 1.30+ (CoCo 部署要求) + containerd (NVIDIA 机密模式仅支持 containerd；CoCo 的 crio 支持仍在演进) + Kata Containers 3.6+ 作为机密容器运行时，通过 RuntimeClass 选择 kata-qemu-tdx / kata-qemu-snp / kata-qemu-nvidia-gpu-tdx 等运行时。

**推荐配置**

硬件: Intel: 第 5 代至强 (Emerald Rapids) 或 Xeon 6 上启用 TDX (TDX 全功能支持从第 5 代起)，64GB+ 总内存并为 TDX 划分足够预留；AMD: EPYC 9004 (Genoa) 启用 SEV-SNP；NVIDIA: H100 机密计算整机 (CPU TEE + H100 GPU + NVSwitch 用于多 GPU)。; 软件: Linux 内核 6.x+ (TDX/SEV-SNP 主机侧支持更完整)；CoCo 使用 Helm chart 安装 (helm install coco oci://ghcr.io/confidential-containers/charts/confidential-containers)；部署 Trustee 栈 (KBS 密钥代理服务 + AS 证明服务 + RVPS 参考值服务)；NVIDIA 场景使用 GPU Operator 机密容器模式。; 集群: 云上使用托管机密节点池 (AKS Confidential Containers preview 或 GKE Confidential Nodes)；裸金属使用 OpenShift (Red Hat CoCo Validated Pattern) 或标准 K8s 发行版 (RHEL 9/10、Ubuntu 24.04、Azure Linux 等)；机密 Pod 与普通 Pod 通过 RuntimeClass 与节点池隔离。

**生产级配置**

大规模场景: 专用机密节点池 + 高可用 KBS (避免单点)；Azure: SEV-SNP 系列 DCasv5/DCadsv5/ECasv5/ECadsv5/DCasv6/ECasv6、TDX 系列 DCesv6/ECesv6、机密 GPU 系列 NCCadsH100v5 (SEV-SNP + H100)；GKE: n2d 系列 (SEV-SNP) 与 C3 系列 (TDX) 机密节点；所有机密 VM 需全盘加密 (Azure securityType=DiskWithVMGuestState 或 GKE Hyperdisk Balanced 机密模式) [不确定具体大规模部署的容量规划数据]; 高可用: 机密节点池跨可用区部署；KBS/Trustee 多副本；密钥备份与轮换流程；AKS/GKE 提供机密节点池与普通节点池混部，但机密容器仅支持受限功能 (AKS preview 仅 Azure Linux)。; NVIDIA 机密 AI: H100 + 受保护 PCIe (PPCIE) + NVSwitch 多 GPU 直通 (MPT)，使用 kata-qemu-nvidia-gpu-tdx 运行时与 nvidia.com/pgpu 资源；仅支持初始安装与配置，不支持集群升级 (GPU Operator 机密模式限制)。

### 兼容性

**支持的 K8s 版本范围**

RuntimeClass: Kubernetes RuntimeClass 自 1.20 GA，是机密容器接入 K8s 的核心机制 (通过 runtimeClassName 选择 Kata 机密运行时)。; 设备插件: 设备插件框架 v1beta1 自 K8s 1.12 稳定；Intel SGX 设备插件基于该框架，Intel intel-device-plugins-for-kubernetes 最新版本对齐每个 K8s minor 版本 (如 v0.36.0 对齐 K8s 1.36)。; CoCo 项目: CoCo 部署要求 Kubernetes 1.30+ 与 Kata Containers 3.6+；CoCo 与 K8s 的完整集成仍在进行中 (v0.12.0, 2025-01 发布，社区每 6 周发版)。; 云托管: AKS Confidential Containers (preview) 与 GKE Confidential Nodes 随各自云服务版本更新 [不确定具体支持的 K8s 版本范围]。

**操作系统兼容性**

支持: Linux: RHEL 9/10、Ubuntu 22.04/24.04 LTS、Azure Linux (AKS 机密容器 preview 仅支持 Azure Linux)、Google COS 与 Ubuntu (GKE 机密节点，不支持 Windows 节点镜像)、SLES/openSUSE、Fedora 等主流发行版。; 内核要求: Linux 内核 5.19+ (TDX/SEV-SNP 基本支持)，6.x+ 更完善 (主机侧 TDX/SNP 支持)；SGX 需 5.11+ 内嵌驱动或 DCAP 驱动 1.41+；NVIDIA 机密模式需要 NVIDIA 优化 Linux 内核与定制 initramfs。; 不支持: Windows Server 节点不支持机密容器 (Kata/CoCo 为 Linux-only)；非 TEE 硬件平台 (ARM 等) 不可用 (CoCo 也支持 IBM s390x SE，ARM CCA 未列入 CoCo 发布说明 [不确定 ARM 状态])。

**K8s 上游支持阶段**

K8s 上游: 机密计算无 K8s 上游统一组件；由 RuntimeClass (GA) + 设备插件 (GA) + 容器运行时 (Kata) + 第三方组件 (CoCo/Trustee) 组合实现。; CoCo: CNCF 孵化项目 (Incubating)，社区项目性质，生产就绪度较高但 K8s 集成仍不完整 (kubectl exec 等操作与机密模型冲突)。; Intel SGX 插件: intel-device-plugins-for-kubernetes 为 Intel 开源社区项目 (Apache-2.0)，长期维护，生产就绪度高。; 云服务: AKS Confidential Containers 为 preview 状态；GKE Confidential Nodes 为正式服务；Azure SEV-SNP 机密 VM 系列 (DCasv5/ECasv5) 已 GA，TDX 系列 (DCesv6/ECesv6) 已推出 (但 AKS 对部分 TDX 机密 VM 存在已知问题 [不确定当前状态])。

**生态兼容性矩阵**

运行时: Kata Containers (机密模式)、containerd (主要支持)、cri-o (CoCo 支持演进中)；Pod Sandboxing (AKS)。; 组件生态: Confidential Containers 生态: Trustee (KBS 密钥代理 + AS 证明服务 + RVPS 参考值服务 + CDH 机密数据枢纽)、CoCo Helm chart (operator 已弃用)、加密容器镜像 (coco-keyprovider/ocicrypt)、Nydus 快照器 (不稳定)、KBS 后端支持 Azure Key Vault 等 [不确定其余后端列表]。; 设备插件: Intel SGX 设备插件 (sgx.intel.com/enclave/provision/epc)、NVIDIA GPU Operator (机密容器模式, nvidia.com/pgpu、nvidia.com/nvswitch)、社区 TPM 设备插件 (如 2gis/tpm-device-plugin、salrashid123/tpm_kubernetes) [不确定各项目维护状态]。; 云集成: Azure AKS/机密 VM、Google GKE 机密节点、IBM Cloud VPC (SGX/TDX 实例)、阿里云 SGX 加密计算 (七代+ 实例) [不确定覆盖范围]；CNI 无强制依赖 (Kata 使用自己的网络栈)。

### 限制与约束

**已知限制**

Intel SGX: 应用必须改写为 enclave 形式 (需 LibOS/框架如 Gramine、Occlum)，迁移成本高；EPC 内存有限且静态划分 (第 3 代至强单路最高 512GB、多路系统最高 1TB)，EPC 换页性能惩罚大；SGX 已从第 11/12 代客户端 CPU 移除，Intel 战略重心转向 TDX；远程证明需配置 PCCS 服务。; Intel TDX: 仅第 4 代至强 (Sapphire Rapids) 及以上支持；TDX 预留内存从物理内存静态划分、不可回收；I/O 密集负载开销高 (Redis 最高 25%)；远程证明依赖基于 SGX 的引用架构；跨节点迁移受限 [不确定 TDX 热迁移支持状态]。; AMD SEV-SNP: 需 EPYC 7003+；SEV(-ES) 不支持远程证明 (CoCo 仅对 SEV-SNP 提供证明支持)；SNP 内存开销约 2-10%；需要 BIOS/固件 (PSP) 配合；证明依赖 AMD VCEK 证书与 KDS 服务。; CoCo 项目: 与 K8s 集成不完整: kubectl exec/日志等可能失败或向主机泄露信息 (需安全策略修改)；镜像在 guest 内拉取性能差；加密镜像 + Nydus 快照器尚不稳定；主机 SELinux 不支持；容器日志/元数据部分未测量；认证 registry 凭据可能暴露给主机；不推荐 latest 镜像标签。; NVIDIA 机密模式: 仅支持本地证明 (local attestation)；仅单 GPU 直通 (多 GPU 与 vGPU/MIG 不支持；Hopper 多 GPU 需 PPCIE+NVSwitch)；仅 containerd；仅限初始安装与配置，不支持集群升级。; 云服务限制: AKS 机密容器 preview: 不支持 resource requests (仅 limits)、服务/LB/EndpointSlice 仅 TCP、策略生成仅 IPv4、ConfigMap/Secret 注入后不可变、终止日志不可读、仅 Azure Linux；GKE 机密节点: 不支持 sole tenant、本地 SSD 仅作临时存储、自动置备仅 AMD 系列、热迁移依赖特定硬件。

**性能开销**

Intel TDX: 计算密集负载开销最高约 5%；SPECrate 约下降 3%；SPECjbb 最高约 4.5%；数据库类 (HammerDB) 约 9.3%；Redis 等 I/O 密集负载约 3.6%-25% (取决于 CPU 余量)；远 NUMA 内存访问因加密额外延迟，建议确保本地内存访问。; AMD SEV-SNP: CPU 负载约 2-5% 开销，内存密集任务约 5-10% 开销 [不确定不同工作负载的精确分布]。; CoCo 容器: 机密容器启动延迟约 10-30 秒 (VM 启动 + 测量 + 证明)；整体处理开销约 5-15%。

**固件与驱动依赖**

BIOS/UEFI: 必须开启 TDX 与 MKTME (Intel)、SEV/SEV-SNP 与 IOMMU (AMD)、SGX + FLC (Intel SGX)、ACS 与硬件虚拟化 (NVIDIA 平台)；配置为 BIOS 级设置，更改需重启节点。; 内核/驱动: Linux 5.19+ (TDX/SNP)，6.x+ 完善；SGX 需内嵌驱动 (5.11+) 或 DCAP 驱动 1.41+；NVIDIA 需优化内核与定制 initramfs、vfio-pci 绑定。; 固件: AMD PSP (平台安全处理器) 固件与 VCEK 证书、Intel TDX 模块与平台固件 (随 BIOS/ME 更新)、SGX 架构 enclave (AESM 服务)；固件升级一般需节点重启维护。; SGX 特例: SGX 远程证明默认 localhost URL 在容器内不可用，需预先配置 PCCS URL。

### 配置与部署

**配置方式**

RuntimeClass (核心): 通过 RuntimeClass 选择机密运行时，例如 runtimeClassName: kata-qemu-tdx / kata-qemu-snp / kata-qemu-nvidia-gpu-tdx；CoCo Helm chart 自动创建 RuntimeClass。; CoCo 安装: 官方推荐 Helm: helm install coco oci://ghcr.io/confidential-containers/charts/confidential-containers --namespace coco-system (原 CoCo Operator 已弃用并迁移至 Helm chart)；辅以 Trustee (KBS/AS/RVPS) 部署用于证明与密钥释放。; Intel SGX 设备插件: 通过 Intel Device Plugin Operator 或 kustomize 部署 sgx_plugin，向 kubelet 上报 sgx.intel.com/enclave、sgx.intel.com/provision、sgx.intel.com/epc 资源；需要基于 cert-manager 的准入 webhook。; NVIDIA 机密 GPU: GPU Operator 机密容器模式 + 节点标签控制运行模式 (默认 on，Hopper 多 GPU 需标签 ppcie)；Pod 声明 nvidia.com/pgpu、nvidia.com/nvswitch 资源。; TPM 设备插件: 社区方案 (如 tpm-device-plugin) 将物理/虚拟 TPM 设备 (/dev/tpm0) 作为设备插件资源暴露给 Pod，用于本地 TPM 证明与密钥操作 [不确定 CNCF 2025 文章所提方案是否有正式插件实现]；GKE 机密节点提供 vTPM 与证明报告。; 安全策略: CoCo/AKS 机密容器要求为 Pod 生成并注入安全策略 (security policy)，定义允许的镜像/配置测量值；AKS 通过 Pod 注解标记机密容器并附加策略。

**配置示例**

RuntimeClass 示例: apiVersion: node.k8s.io/v1, kind: RuntimeClass, metadata: {name: kata-qemu-tdx}, handler: kata-qemu-tdx ; Pod 示例: spec.runtimeClassName: kata-qemu-tdx，resources.limits: {nvidia.com/pgpu: 1} (GPU 机密 Pod) 或 {sgx.intel.com/epc: 512Mi} (SGX 场景)；CoCo 安装: helm install coco oci://ghcr.io/confidential-containers/charts/confidential-containers --namespace coco-system ; AKS 部署: az aks create --enable-pod-sandboxing 配合机密容器注解 [不确定当前 AKS CLI 参数准确名称]。

**部署位置与环境**

公有云: Azure (DCasv5/DCadsv5/ECasv5/ECadsv5/DCasv6/ECasv6 系列 SEV-SNP、DCesv6/ECesv6 系列 TDX、NCCadsH100v5 机密 GPU；AKS Confidential Containers preview)、Google Cloud (GKE Confidential Nodes: n2d SEV-SNP、C3 TDX；支持机密 GPU 与 Hyperdisk 机密模式)、IBM Cloud VPC (SGX/TDX)、阿里云 (SGX 加密计算) [不确定 AWS 是否有对等的 K8s 机密容器托管方案]。; 裸金属: CoCo 支持裸金属部署 (Red Hat 发布裸金属部署指南)；OpenShift 机密计算 Validated Pattern (CoCo Pattern)。; 私有云/边缘: OpenShift/标准 K8s 私有云；边缘场景受 TEE 硬件限制 [不确定边缘机密计算普及度]。

**虚拟化兼容性**

VM 级 TEE: TDX/SEV-SNP 本质是 VM 级机密计算：Kata 将 Pod 封装进机密 VM，主机/虚拟化层无法读取 guest 内存；云上的机密 VM (AKS/GKE 节点) 同样受硬件 TEE 保护。; GPU 直通: 机密 GPU 通过 vfio-pci 直通给机密 VM (H100 使用受保护 PCIe)；不支持 vGPU/MIG 共享；多 GPU 需 NVSwitch 与 PPCIE。; 热迁移: 机密 VM 热迁移受限: GKE 机密节点热迁移依赖特定硬件；TDX/SEV-SNP 的实时迁移支持有限 [不确定各平台当前支持状态]；SGX enclave 无迁移概念。

### 性能特征

**基准性能数据**

Intel TDX (官方, 4 代至强): 计算密集负载开销最高约 5%；SPECrate 约降 3%；SPECjbb 最高约降 4.5%；HammerDB (数据库) 约降 9.3%；Redis 开销 3.6%-25% (取决于 CPU 余量)；建议保持内存访问本地化以降低跨 socket 加密延迟。; AMD SEV-SNP: CPU 密集负载开销约 2-5%，内存密集负载约 5-10% [不确定官方 SPEC 级基准数据]；AMD 发布过 Google n2d SEV-SNP 实例性能白皮书 [不确定具体数字]。; CoCo 机密容器: Pod 启动延迟约 10-30 秒 (相比 runc 明显增加)；整体处理开销约 5-15%；加密镜像在 guest 内拉取有额外性能惩罚；AKS 中 Kata-CC UVM 基线 2GB 内存。; Intel SGX: 性能取决于 enclave 转换频率与 EPC 命中率；EPC 换页 (paging) 惩罚显著 [不确定具体基准百分比]；LibOS 方案 (Gramine/Occlum) 额外增加开销。

### 安全

**安全特性**

Intel TDX: VM 级机密: MKTME 多密钥全内存加密、TDX 模块 (TDX Module) 对启动与镜像的硬件测量、远程证明 (TDX quote，通过基于 SGX 的引用架构生成)、信任根在 CPU 内；无代码修改即可保护未改动 OS/应用。; AMD SEV-SNP: 内存加密 + 完整性保护 (反向映射/嵌套页表防重放)、SNP 远程证明 (ATTESTATION 请求、VCEK 证书链、与 KDS 交互)、隔离来自虚拟化层 (hypervisor) 的访问。; Intel SGX: 应用级 enclave 隔离、EPC 内存加密、远程证明 (EPID 或 ECDSA/DCAP)、Flexible Launch Control；信任模型为应用级，宿主 OS 不可信。; NVIDIA 机密 GPU: GPU 内存加密 (数据使用中保护)、受保护 PCIe (PPCIE) 防止 CPU-GPU 通道窥探、NVIDIA Attestation SDK 在 guest pre-start 钩子中验证硬件证书链与运行测量；支持 Blackwell/Hopper/RTX Pro 6000 BSE。; CoCo 层: 加密容器镜像、Sealed Secrets (环境变量/卷方式注入)、认证 registry、客户定义的安全策略 (镜像/配置测量)、KBS 条件释放密钥 (仅当证明通过)、TCB 全开源可审计。; TPM 与证明 (CNCF 2025): 物理 TPM 提供硬件信任根，虚拟 TPM (在安全监视器内) 测量机密 VM 启动完整性，guest 应用获取签名报告；CNCF 2025 博客提出 TPM 组合远程证明方法 (物理 TPM + vTPM 统一证明，支持 Hygon CSV，规划中支持 AMD SNP/Intel TDX)；GKE 机密节点提供 vTPM 与证明报告；社区 TPM 设备插件将 TPM 暴露给 Pod [不确定插件项目成熟度]。; 密钥管理: CoCo Trustee KBS 通过 KBS 协议在证明通过后将密钥/机密释放给 guest 内 Confidential Data Hub (CDH)，后端支持 Azure Key Vault 等；K8s 侧可与 KMS v2 (etcd 静态加密插件) 配合，或集成 HashiCorp Vault 等外部密钥管理 [不确定 CoCo 与 Vault 的官方集成状态]。

### 运维与生命周期

**维护与生命周期**

节点维护: TEE 功能为 BIOS 级配置，节点维护/固件升级需排水后重启；TDX/SEV-SNP 内存划分在 BIOS 设置，更改需重启；机密节点池不支持热迁移 (依赖硬件)。; 版本升级: CoCo 每 6 周发版，Helm chart 升级；NVIDIA GPU Operator 机密模式不支持集群升级 (仅初始安装配置)；AKS 机密容器 preview 功能随 AKS 版本演进，Azure Linux 2.0 已冻结 (2025-11 起停止安全更新)。; 固件: AMD PSP 固件/VCEK 证书、Intel 平台固件 (含 TDX 模块) 随厂商固件更新流程，需厂商 BIOS 升级；SGX AESM/DCAP 服务随软件包更新。

### 经济性

**总拥有成本 (TCO)**

运行成本: 性能开销 2-10% (极端 I/O 25%) 需额外计算资源；Kata 每 Pod VM 增加内存开销 (2GB 级基线)；软件栈 (CoCo/Trustee/Kata/SGX 插件) 开源免费。; 节省项: 避免专用 HSM/安全硬件采购；降低数据泄露与合规罚款风险；多云/裸金属场景可用同一套 CoCo 栈；SGX enclave 适合密钥管理/Web3 等小负载场景。

**成熟度与社区支持**

社区活跃度: CoCo 为 CNCF 孵化项目，由 Red Hat/IBM/Intel/AMD/Microsoft/NVIDIA 等主导，每 6 周发版 (v0.12.0 于 2025-01)；Kata Containers 成熟活跃；Intel SGX 设备插件长期维护；Trustee 项目活跃。; 厂商支持: Intel (TDX 战略重心、SGX 插件)、AMD (SEV-SNP 云支持广泛)、NVIDIA (机密计算 + GPU Operator 参考架构)、Microsoft Azure (机密 VM/机密容器产品化程度高)、Google Cloud (GKE 机密节点正式服务)、Red Hat (OpenShift CoCo pattern)、IBM (s390x SE 与云 VPC)。; 生态成熟度: 云托管方案 (AKS/GKE) 产品化程度高但部分功能仍 preview；裸金属/自建以 CoCo 为主流；SGX 生态 (Gramine/Occlum/Enarx/Web3) 细分领域成熟但 Intel 重心转移；整体机密计算在 K8s 处于快速成熟期，生产采用集中在金融/医疗/政务/机密 AI 场景。

---

## 21. VPU / 媒体加速器（Intel QSV、NVIDIA NVENC/NVDEC、AMD VCE/VCN、NETINT VPU 等硬件视频编解码器，含 K8s 设备插件管理与云原生视频转码生态）

**官方文档**: K8s 设备插件概念: https://kubernetes.io/docs/concepts/extend-kubernetes/compute-storage-net/device-plugins/ ; K8s DRA: https://kubernetes.io/docs/concepts/scheduling-eviction/dynamic-resource-allocation/ ; Intel Device Plugins for Kubernetes (gpu_plugin): https://github.com/intel/intel-device-plugins-for-kubernetes 与 https://intel.github.io/intel-device-plugins-for-kubernetes/cmd/gpu_plugin/README.html ; Intel oneVPL/Media SDK: https://github.com/oneapi-src/oneVPL ; Intel Media Transport Library: https://github.com/OpenVisualCloud/Media-Transport-Library ; Intel Flex 系列媒体基准: https://github.com/intel/media-delivery/blob/master/doc/benchmarks/intel-data-center-gpu-flex-series/intel-data-center-gpu-flex-series.rst ; NVIDIA Video Codec SDK (NVENC/NVDEC): https://developer.nvidia.com/video-codec-sdk ; NVIDIA 编解码支持矩阵: https://forums.developer.nvidia.com/t/video-encode-and-decode-gpu-support-matrix/64780 ; NVIDIA k8s-device-plugin: https://github.com/NVIDIA/k8s-device-plugin ; NVIDIA 容器化 FFmpeg 指南: https://docs.nvidia.com/video-technologies/video-codec-sdk/13.0/ffmpeg-with-nvidia-gpu/index.html ; AMD GPU k8s-device-plugin: https://instinct.docs.amd.com/projects/k8s-device-plugin/en/latest/ 与 https://github.com/ROCm/k8s-device-plugin ; AMD MxGPU 虚拟化: https://instinct.docs.amd.com/projects/virt-drv/en/latest/userguides/Getting_started_with_MxGPU.html ; NETINT k8_device_plugin: https://github.com/NETINT-Technologies/k8_device_plugin ; NETINT VPU as a Service: https://netint.com/research/blog/vpu-as-a-service-video/ ; Akamai VPU 8 卡方案: https://www.akamai.com/blog/cloud/scaling-media-workloads-akamais-new-8-card-vpu-plan ; AWS EC2 VT1 (Alveo U30): https://aws.amazon.com/blogs/aws/new-amazon-ec2-vt1-instances-for-live-multi-stream-video-transcoding/ 与 https://aws.amazon.com/blogs/compute/deep-dive-on-amazon-ec2-vt1-instances/

### 硬件规格

**最低配置**

硬件: Intel QSV：任何带 Quick Sync Video 的 Intel 平台（第 6 代酷睿起核显、部分 Xeon 型号、Arc/Flex/Max 独显），需提供 /dev/dri 设备；NVIDIA：任意支持 NVENC/NVDEC 的 GPU（T4/A2/L4/A10 等数据中心卡或 GeForce/RTX 消费卡）；AMD：带 VCN（Video Core Next，VCE 后继）的 Radeon/Radeon Pro 显卡（amdgpu 驱动）；NETINT：T408/T432（Quadra）VPU 板卡（PCIe）。; 软件: Linux 节点（x86_64 为主）；Intel：内核 i915/xe 驱动 + libva/iHD 用户态驱动 + oneVPL/Media SDK；NVIDIA：NVIDIA 驱动 + NVIDIA Container Toolkit（运行时注入设备）+ FFmpeg NVENC/CUVID 或 Video Codec SDK；AMD：amdgpu 内核模块 + Mesa VA-API（radeonsi/VCN）；NETINT：主机驱动 + 容器内 libxcoder 库。; K8s 资源: 设备插件以 DaemonSet 运行（需特权/root 访问 kubelet 设备插件通道）；Pod 通过扩展资源声明（如 gpu.intel.com/i915、nvidia.com/gpu、netint.ca/ASIC）；K8s 1.28+ 可启用 CDI（Container Device Interface，1.29 默认开启）[不确定各插件 CDI 支持完整度]。

**推荐配置**

硬件: Intel：Data Center GPU Flex 140（卡 TDP 75W，支持 SR-IOV）或 Flex 170（150W），或 12/13/14 代酷睿核显做低成本转码；NVIDIA：L4（Ada，支持 AV1 编解码、72W）或 A10/T4（H.264/HEVC 转码）；NETINT：T408/T432 高密度 VPU 卡，按直播/VOD 并发规划每节点卡数。; 软件: 主流 Linux 发行版（Ubuntu 22.04/24.04、RHEL 8-10、Rocky Linux、Debian、SLES 等）+ K8s 1.28+；FFmpeg（qsv/vaapi/nvenc/cuvid/amf 硬件后端）或 GStreamer + 厂商 SDK；部署 intel-gpu-plugin / NVIDIA k8s-device-plugin（或 GPU Operator）/ NETINT k8_device_plugin。; 集群: 按每卡并发会话容量规划调度（媒体会话数限制是关键）；低延迟直播建议 SR-IOV 分片 + NUMA 亲和 + 预留 CPU 核做封装/解复用；配 Node Feature Discovery 为节点打媒体能力标签。

**生产级配置**

大规模转码集群: 裸金属多节点 + 每节点多卡（如 4-8 块 NETINT 或 Intel Flex 卡），整卡或 SR-IOV VF 直通给 Pod，配合 HPA/KEDA 按转码队列伸缩、NFD 标签调度、FFmpeg/专用转码服务（Nimble Streamer、OpenVisualCloud、MainConcept 等）[不确定具体大规模案例公开数据]。; 公有云媒体集群: AWS EC2 VT1 实例（Xilinx/AMD Alveo U30 媒体加速卡，每实例 1-8 卡、约 16-128 路并发转码通道）[不确定精确通道数]；Akamai VPU 计划（NETINT 8 卡/节点、按卡计费）；NetActuate NETINT 加速裸机/云主机。; 视频会议/直播: 低延迟 WebRTC 网关（Intel OWT、LiveKit、mediasoup 等 SFU）与硬件编码边缘节点混部，边缘就近转码降低回源带宽 [不确定主流厂商方案细节]。

### 兼容性

**支持的 K8s 版本范围**

Device Plugin: K8s 设备插件框架 v1beta1 自 1.10 起稳定（GA），是媒体加速器的标准接入方式；intel-device-plugins 每个 minor 版本对齐（v0.36.0 支持 K8s 1.36）；NVIDIA k8s-device-plugin、AMD k8s-device-plugin、NETINT 插件支持较广版本范围 [不确定各插件精确下限]。; CDI: Intel GPU 插件 CDI 支持要求 K8s 1.28（feature gate）+ containerd 1.7+/CRI-O 1.24+ 运行时，K8s 1.29 起默认启用 [不确定其它插件 CDI 支持状态]。; DRA: Dynamic Resource Allocation 自 K8s 1.26 引入（Alpha）并持续演进 [不确定 GA 状态]；媒体加速器场景仍以 Device Plugin 为主，DRA 主要面向 GPU 计算域。

**操作系统兼容性**

支持: Linux x86_64：Ubuntu 22.04/24.04、RHEL 8-10、Rocky Linux、Debian、SLES 等；容器运行时 containerd/CRI-O/Docker；OpenShift、RKE2、k3s 等发行版可用。; 不支持: Windows Server 节点（Windows 容器不支持 GPU/媒体设备直通，NVENC 在 Windows 主机可用但不属 K8s 节点生态）；macOS。

**K8s 上游支持阶段**

Device Plugin: K8s 上游 GA 稳定机制，是媒体加速器接入的标准方式。; 厂商插件: intel-device-plugins（Intel 开源项目，活跃维护、版本对齐 K8s minor）、NVIDIA k8s-device-plugin（NVIDIA 官方，配合 GPU Operator）、ROCm k8s-device-plugin（AMD 官方）、NETINT k8_device_plugin（NETINT 官方）；均为厂商/社区项目，非 CNCF 托管。; DRA: K8s 上游演进中（结构化参数等能力逐步加入）[不确定媒体场景 DRA 接入成熟度]。

**生态兼容性矩阵**

应用生态: FFmpeg（qsv/vaapi/nvenc/cuvid/amf 硬件后端）、GStreamer、Intel oneVPL/Media SDK、NVIDIA Video Codec SDK、AMD AMF、Jellyfin/Plex（媒体服务）、OpenVisualCloud（Intel Media Transport Library、Open WebRTC Toolkit）、Nimble Streamer、MainConcept Easy Video API（对接 NETINT VPU）。; 调度与弹性: 原生扩展资源调度、KEDA/HPA（按转码队列/并发伸缩）、Node Feature Discovery（媒体能力标签）、Kueue/Volcano 等批调度 [不确定媒体共享调度成熟度]。; CNI/网络: 无强制 CNI 依赖；低延迟/ST 2110 直播场景配合 DPDK + SR-IOV CNI（Intel Media Transport Library 基于 DPDK）。

### 限制与约束

**已知限制**

NVENC 会话限制: 消费级 GeForce 并发编码会话上限从 5 路提升至 8 路（GTX 1630 仅 3 路），覆盖 Maxwell 至 Ada 架构；专业/数据中心卡（T4/A2/A10/L4、RTX Pro、A 系列）不限制会话数 [不确定官方文档精确表述]。; Intel 限制: 多卡系统中 QSV/旧式视频加速接口可能无法自动定位正确硬件（官方提供 shell 脚本显式传 render 设备文件名）；-shared-dev-num=1 时每卡仅 1 个 Pod；SR-IOV 需主机 BIOS/驱动配合创建 VF。; NETINT 限制: Pod 内至少需要 libxcoder 才能使用加速器；默认 Docker Hub 镜像仅支持 Quadra 硬件（T4xx 需自行构建或使用其他镜像）；资源按卡粒度申请，通过 requests/limits 控制每 Pod VPU 数量。; 通用限制: 设备插件为整设备分配，无媒体会话级分片（除 SR-IOV VF 外）；vfio 直通设备不支持 VM 热迁移；调度器不感知每卡会话容量，需自定义策略避免超卖。

### 配置与部署

**配置方式**

Intel（主要）: intel-device-plugins gpu_plugin DaemonSet，资源名 gpu.intel.com/i915（或新架构 gpu.intel.com/xe）；-shared-dev-num 控制每卡共享 Pod 数（媒体场景常用 >1 实现多容器共享一卡）；-allocation-policy 支持 balanced/packed/none；SR-IOV vGPU 由主机配置后插件将 VF 分配给 Pod；支持 CDI 与监控/健康管理（-enable-monitoring、-health-management、xpumd）。; NVIDIA: k8s-device-plugin DaemonSet（或 GPU Operator 全栈），资源名 nvidia.com/gpu；容器内通过 FFmpeg NVENC/CUVID、Video Codec SDK 或 GStreamer 使用编解码引擎；GPU Operator 提供驱动、插件、DCGM 监控一体化管理。; AMD: ROCm k8s-device-plugin 注册 amd.com/gpu [不确定媒体专用配置路径]；VA-API 场景将 /dev/dri/renderD* 注入容器。; NETINT: Helm 部署（make deploy / make deploy-netint），特权 DaemonSet（kube-system 命名空间，pod 名 netint-device-plugin-#####），资源名 netint.ca/ASIC（T4xx）/ netint.ca/Quadra；Pod 内需 libxcoder。; DRA（实验）: 可选使用 K8s DRA ResourceClaim 方式声明媒体资源 [不确定媒体场景 DRA 支持]；生产主流仍为传统 Device Plugin。

**部署位置与环境**

裸金属（主要）: 数据中心/边缘直播、VOD 批处理转码、视频会议网关（主要生产形态）；OpenShift/RKE2/k3s 均支持。; 虚拟化/边缘: SR-IOV/VFIO 直通给 VM 或容器；边缘节点就近转码（核显 QSV 常见于边缘媒体网关）。

**虚拟化兼容性**

SR-IOV: Intel Flex 140 支持 SR-IOV（官方媒体基准文档标注 62 个虚拟实例，Flex 170 标注 31 [不确定，与具体配置/固件相关]）；AMD MxGPU（Radeon Pro V340 双 VCN，SR-IOV 分片）；NVIDIA vGPU；VF 可直通给 Pod 或 VM。; VFIO 直通: 需主机 IOMMU（intel_iommu=on / AMD-Vi）+ BIOS 开启虚拟化；容器/VM 内需自行准备驱动与用户态库；直通设备不支持热迁移。; 限制: 嵌套虚拟化下无法使用媒体加速；同一节点容器与 VM 媒体工作负载不可混部（设备独占）；SR-IOV 需主机配置 VF 数并持久化。

### 性能特征

**基准性能数据**

NVIDIA: 数据中心/专业卡并发编码会话不限，消费卡限 8 路；L4（Ada）支持 AV1 编解码，A10/T4 支持 H.264/HEVC 编解码；Video Codec SDK 13.1 引入零拷贝转码、AV1 B 帧与帧精确定位；官方未统一公布每卡 1080p60 流数 [不确定具体 benchmark]。; NETINT: Akamai/Cires21 联合测试：单块 NETINT VPU 以 12W 支撑 19 路 1080p60 同时编码（约 0.63W/路），AV1/HEVC 编码质量与 GPU 方案相当且效率更高；T408/T432 每卡支持数十路 1080p60 并发 [不确定精确数字]。; 对比结论: 硬件转码相对 CPU x264 吞吐高一个数量级、每路功耗低数十倍 [不确定精确倍数]；相比 GPU 转码，VPU（NETINT）在每瓦性能与每流成本上更优（Akamai 测试结论）。

### 安全

**安全特性**

信任根: 媒体加速器是编解码引擎而非安全信任根，不提供 TPM 等效的密钥保护；内容保护依赖上层 DRM（PlayReady/Widevine/FairPlay）与加密传输（SRT/RTMPS/WebRTC DTLS）[不确定硬件级 DRM 支持细节]。; 平台安全: Secure Boot/TPM/机密计算由服务器平台提供，与媒体加速无直接关联；视频会议安全（E2EE）由应用层（WebRTC/MLS）实现。

### 运维与生命周期

### 经济性

**总拥有成本 (TCO)**

运行成本: 功耗极低（NETINT 约 0.63W/路、Flex 140 75W/卡），散热与电费远低于 CPU/GPU 转码；软件栈开源免费（FFmpeg/GStreamer/设备插件）。; 节省项: 相对 CPU x264 转码可削减绝大部分转码核数 [不确定精确百分比]，降低服务器采购与电费；相对 GPU 实例，VPU 每流成本与每瓦性能更优（Akamai/NETINT 测试结论）；云上 VT1/Akamai VPU 按卡/小时计费 [不确定具体单价]。; 适用性判断: 直播、VOD 批处理、视频会议转码规模越大收益越明显；小规模或低并发场景核显 QSV/消费级 NVENC 即可满足，无需独立 VPU。

---

## 22. Windows 节点

**官方文档**: https://kubernetes.io/docs/concepts/windows/intro/

### 硬件规格

**最低配置**

2 vCPU, 4 GB 内存, 128 GB 磁盘。Windows Server 2022/2025 操作系统, containerd 1.7.0+ 或 Docker 容器运行时。AKS 系统节点池要求至少 2 vCPU 和 4 GB 内存, 用户节点池要求至少 2 vCPU 和 2 GB 内存。不支持 B 系列或 Av1 系列 VM。Windows 容器基础进程 (Server Core) 需额外约 512 MB 内存。

**推荐配置**

4 vCPU, 8 GB 内存, 256 GB SSD。Windows Server 2025 LTSC 操作系统, Gen2 VM (默认), containerd 运行时。AKS 推荐 Standard_D2s_v3 或更高规格 VM。配合 Linux 系统节点池 (至少 2 节点) 运行集群基础设施组件。

**生产级配置**

8 vCPU, 32 GB 内存, 512 GB SSD。多节点 Windows 节点池 (建议至少 2 个节点), 配合 Linux 系统节点池。使用 Gen2 VM 和 Trusted Launch。如需 GPU 工作负载, 使用 GPU 加速型 VM (如 Standard_NC* 系列)。使用 Azure CNI Overlay 网络模式以获得最佳可扩展性。

### 兼容性

**支持的 K8s 版本范围**

Kubernetes 1.14 及以上版本 (Windows 节点支持自 1.14 起正式 GA)。Windows Server 2022 支持 K8s 1.25 至 1.35 (AKS 默认), 不支持 K8s 1.37+。Windows Server 2025 支持 K8s 1.36+ (AKS 默认)。Windows Server 2019 不支持 K8s 1.33+。

**操作系统兼容性**

Windows Server 2025 LTSC (推荐), Windows Server 2022 LTSC, Windows Server 2019 LTSC (已终止支持, 不支持 K8s 1.33+)。不支持 Windows Server Annual Channel (AKS 将于 2026-2027 停止支持)。Kubernetes 仅支持 process isolation 模式, 不支持 Hyper-V 隔离。控制面必须运行在 Linux 节点上。Windows Server 容器主机必须将 Windows 安装到 C: 盘。

**K8s 上游支持阶段**

GA (自 Kubernetes 1.14 起 Windows 节点支持已正式 GA)

**生态兼容性矩阵**

CNI: Azure CNI (AKS 默认, 推荐 Overlay 模式), Calico (支持网络策略), Flannel (overlay)。CSI: Azure Disk (NTFS 卷), Azure Files (NTFS 卷)。监控: Windows Exporter (Prometheus, 端口 19182), Managed Prometheus + Grafana (AKS)。认证: gMSA (Group Managed Service Account) - GA。容器运行时: containerd (推荐), Docker (遗留支持)。网络策略: Azure Network Policy Manager, Calico Network Policy。

### 限制与约束

**已知限制**

1) 不支持 Hyper-V 隔离 (Kubernetes 仅支持 process isolation)。2) 不支持 HugePages。3) 不支持特权容器 (需使用 HostProcess Containers 替代)。4) 不支持 hostPID、hostIPC、shareProcessNamespace。5) 不支持 Linux 安全上下文 (SELinux、seccomp、Capabilities、readOnlyRootFilesystem、allowPrivilegeEscalation、procMount)。6) Windows kubelet 不强制执行内存/CPU 限制 (--kube-reserved / --system-reserved 仅从 NodeAllocatable 扣除, 不保证资源预留)。7) 不支持 OOM 驱逐和 PIDPressure 检测。8) 集群约 500 个服务时可能遇到端口耗尽 (External Traffic Policy=Cluster, 约 16K 动态端口池)。9) 不支持客户端源 IP 保留。10) 默认 TCP 超时 4 分钟 (不可配置)。11) 不能与 Linux 容器在同一个 Pod 中运行。12) Windows Server 2019 不支持 Gen2 VM。13) GKE 上不支持 GPU/TPU、Autopilot、Dataplane V2、机密节点。14) 仅支持 NTFS 文件系统。15) Windows 容器镜像较大 (Server Core ~4GB, Nano Server ~1GB)。16) 不支持 kubenet 网络模式 (AKS Windows 必须使用 Azure CNI)。

**混部兼容性**

支持 Windows/Linux 混合节点集群。控制面必须运行在 Linux 节点上。每个 Pod 只能包含单一操作系统的容器 (Windows 和 Linux 容器不能混部于同一 Pod)。通过 nodeSelector (kubernetes.io/os: windows) 或 os.name 字段指定目标操作系统。Kubernetes 自动为 Windows 节点添加 node.kubernetes.io/os=windows:NoSchedule 污点。建议使用 Linux 节点池运行基础设施组件 (如 NGINX ingress、CoreDNS、监控代理)。Windows 和 Linux 工作负载可以共享同一集群但使用不同的节点池。AKS 创建集群时默认创建 Linux 系统节点池。

**性能开销**

Windows 容器基线内存开销: 每个 Server Core 容器约 2 GB+ (基础系统进程), Nano Server 约 1 GB+。容器镜像大小: Server Core ~4 GB, Nano Server ~1 GB (远大于 Linux 容器镜像)。Windows 节点上 kubelet 和系统进程的资源开销高于 Linux 节点。Hyper-V 隔离额外开销 (K8s 不支持, 仅作为参考): 额外 ~256 MB+ VM 开销。容器启动时间较 Linux 慢, 因基础镜像较大。

**固件与驱动依赖**

Windows Server 主机必须安装容器功能 (Docker 或 containerd)。需要 containerd 1.7.0+ 或 Docker 引擎。GPU 加速: 需要 NVIDIA WDDM/MCDM 驱动 (Windows Server), 需使用第三方设备插件 (如 TensorWorks 开源方案)。GPU 直通需要特定 GPU 设备类 GUID (5B45201D-F2F2-4F3B-85BB-30FF1F953599)。AKS 每月发布新的 Windows 节点 VHD (包含最新安全更新)。Windows 节点更新需要创建新节点池并迁移工作负载。Windows 必须安装在 C: 盘 (process isolation 容器)。gMSA 需要 Active Directory 域环境。

### 配置与部署

**配置方式**

云服务商节点池配置 (AKS/GKE/EKS 的 Windows 节点池)。节点污点 (node.kubernetes.io/os=windows:NoSchedule 自动添加)。Pod 调度: nodeSelector kubernetes.io/os: windows 或 spec.os.name: windows。HostProcess Containers 替代特权容器。gMSA 配置 (Group Managed Service Account) 用于 Windows 身份验证。GPU 加速: 第三方设备插件 (TensorWorks 开源方案, 支持 WDDM/MCDM)。AKS 推荐使用 Azure CNI Overlay 网络模式。

**配置示例**

apiVersion: v1
kind: Pod
metadata:
  name: windows-app
spec:
  os:
    name: windows
  containers:
  - name: windows-container
    image: mcr.microsoft.com/windows/servercore:ltsc2025
  nodeSelector:
    kubernetes.io/os: windows

**部署位置与环境**

公有云: 支持 AKS (Azure)、GKE (Google Cloud)、EKS (AWS)。私有云/本地数据中心: 支持 (需要 Windows Server 许可)。裸金属: 支持 Windows Server 直接部署。虚拟机: 支持 Hyper-V、VMware 等虚拟化平台。边缘: 有限支持 (Windows IoT / Windows Server 边缘场景)。不支持仅 Windows 集群 (必须至少有一个 Linux 节点池运行系统组件)。

**虚拟化兼容性**

支持 Gen2 VM (Windows Server 2022+), Gen2 是 Windows Server 2025 的默认配置。支持嵌套虚拟化 (但 Hyper-V 隔离在 K8s 中不支持)。不支持 PCIe 直通/透传 (GPU 直通仅通过第三方设备插件方案, 非原生 K8s 支持)。不支持 SR-IOV (Windows 网络受限)。不支持 VM 热迁移对加速器的影响。Azure 上 Windows Server 2025 默认使用 Gen2 VM。

### 性能特征

**基准性能数据**

Windows 容器启动时间: 较 Linux 容器慢, 因基础镜像较大 (Server Core ~4 GB, Nano Server ~1 GB)。内存基线: 每个 Server Core 容器约 2 GB+ (含系统进程), Nano Server 约 1 GB+。端口限制: 约 500 个服务 (External Traffic Policy=Cluster 时, 约 16K 动态端口池)。单节点 Pod 密度: 受限于 Windows 资源开销, 通常低于同等 Linux 节点。AKS 默认最大 Pod 数: 每节点 30 (Azure CNI Overlay) 至 110 (取决于网络配置)。Windows 节点 kubelet 不强制执行资源限制。

**扩展性上限**

集群约 500 个服务时可能遇到端口耗尽 (External Traffic Policy=Cluster 时)。TCP 动态端口池约 16,384 个端口 (默认)。节点数上限与云服务商限制相同 (AKS 标准限制)。建议每节点运行 10-30 个 Windows Pod 以获得最佳性能。

**每节点密度**

每节点最大 Pod 数: 取决于节点规格和网络配置 (AKS 默认 30, 可配置至 110)。实际密度受限于 Windows 容器内存开销, 建议每节点运行 10-30 个 Windows Pod。GPU 分片: 不支持原生 GPU 分片 (MIG/MPS 不适用于 Windows)。

### 安全

**安全特性**

gMSA (Group Managed Service Account) 支持 - GA, 支持 Active Directory 集成。HostProcess Containers 替代特权容器。Windows 安全标识符 (SID) 替代 Linux UID/GID。Windows 访问控制列表 (ACL) 替代 POSIX 权限位。TPM 支持 (通过 Windows 节点硬件)。Secure Boot 支持 (Gen2 VM / AKS Trusted Launch)。FIPS 支持 (AKS 支持 FIPS 节点)。不支持 Linux 安全上下文 (SELinux、seccomp、AppArmor)。不支持机密计算节点 (Confidential Containers / CoCo)。不支持 Hyper-V 隔离增强安全。

**合规与认证**

K8s 一致性认证: Windows 节点通过 Kubernetes 一致性测试套件。FIPS 140-2/3: AKS 支持 FIPS 节点。Windows Server 安全合规: 符合 Windows Server 安全基线。Active Directory 集成: 支持 Windows 身份验证。PCI DSS / HIPAA: 取决于底层云服务商合规认证, Windows 节点本身不额外提供认证。

### 运维与生命周期

**可观测性支持**

Windows Exporter (Prometheus), 端口 19182。默认采集器: cpu, cpu_info, cs, container, logical_disk, memory, net, os, process, service, system, textfile。Grafana 仪表板: Kubernetes/Compute Resources/Cluster (Windows), Kubernetes/Compute Resources/Namespace (Windows), Kubernetes/Compute Resources/Pod (Windows), Kubernetes/USE Method/Cluster (Windows), Kubernetes/USE Method/Node (Windows)。Managed Prometheus + Managed Grafana (AKS)。容器日志: kubectl logs (通过 Windows 事件跟踪)。不支持原生 PIDPressure 和 OOM 指标暴露。

**维护与生命周期**

每月 Windows 节点更新: AKS 每月发布新 VHD (含最新安全补丁)。Windows OS 版本升级 (如从 2022 升级到 2025): 需创建新节点池并迁移工作负载, 不支持原地升级。节点排水: 支持标准 kubectl drain。不支持热迁移 (Windows 容器)。不支持热插拔。固件升级: 通过节点池镜像更新。Windows Server 2022 支持至 2028 年 6 月 (AKS)。Windows Server 2025 为当前推荐版本。

**弹性与故障恢复**

单点故障: 建议至少 2 个 Windows 节点以确保高可用。自动修复: AKS 提供 Windows 节点自动修复功能。节点故障: 标准 Pod 驱逐和重新调度机制。Windows 更新后需要重启节点 (通过节点池镜像升级实现)。数据持久化: 使用 Azure Disk / Azure Files 作为持久卷 (NTFS)。冗余: 建议配合 Linux 系统节点池 (2+ 节点) 运行集群关键组件。

### 经济性

**总拥有成本 (TCO)**

Windows Server 许可费用: 显著额外成本 (每个节点需 Windows Server 授权)。更大的 VM 规格需求: 因 Windows 容器 ~2 GB+ 内存基线开销, 需选择更大规格 VM。更大的存储需求: Windows 容器镜像 ~4 GB+ (Server Core)。每月维护窗口: Windows 更新需要节点池滚动更新, 增加运维成本。总体拥有成本显著高于同等 Linux 节点。建议评估 Windows 容器化是否确实必要 (通常仅用于 .NET Framework 等 Windows 依赖应用)。

**成熟度与社区支持**

微软: 主要厂商支持 (Azure AKS, Windows Server 团队持续投入)。Google: GKE 支持 Windows 节点池。AWS: EKS 支持 Windows 节点池。上游 K8s: GA 级支持 (自 1.14)。社区: 活跃度低于 Linux 生态, 但微软持续贡献 SIG Windows。生态: 专注于 .NET Framework 应用迁移、传统 Windows 工作负载容器化、以及 Active Directory 集成场景。Windows 容器生态成熟度仍在发展中, 不如 Linux 容器成熟。

---
