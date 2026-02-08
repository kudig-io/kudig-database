# Domain 31 - 硬件基础设施

## 领域概述

本领域聚焦云平台硬件基础设施，涵盖服务器架构、核心组件技术、硬件厂商生态及**硬件故障排查**。从技术专家角度提供专业、完整的硬件技术知识体系，特别强调硬件问题诊断与故障排查方法论。

## 文档目录

### 硬件基础架构 (01-04)

| 编号 | 文档 | 描述 |
|------|------|------|
| 01 | [云平台硬件基础架构](01-cloud-hardware-architecture.md) | 数据中心架构、服务器集群、存储系统、网络设备 |
| 02 | [服务器架构原理](02-server-architecture-principles.md) | 服务器内部架构、NUMA、PCIe总线、BMC管理 |
| 03 | [CPU技术深度解析](03-cpu-technology-deep-dive.md) | Intel/AMD/ARM处理器、微架构、性能优化 |
| 04 | [主板与芯片组技术](04-motherboard-chipset-technology.md) | 主板规格、芯片组功能、接口规范 |

### 核心组件技术 (05-09)

| 编号 | 文档 | 描述 |
|------|------|------|
| 05 | [内存技术深度解析](05-memory-technology-deep-dive.md) | DDR5架构、ECC、RAS特性、性能优化 |
| 06 | [机械硬盘技术](06-storage-hdd-technology.md) | HDD原理、S.M.A.R.T.监控、企业级特性 |
| 07 | [SSD固态硬盘技术](07-storage-ssd-technology.md) | NAND闪存、NVMe协议、耐久性管理 |
| 08 | [网络硬件技术](08-network-hardware-technology.md) | 高速网卡、RDMA、智能网卡、交换机 |
| 09 | [硬件厂商生态](09-hardware-vendors-ecosystem.md) | 服务器厂商、组件供应商、ODM/白牌 |

### 硬件故障排查 (10-18) - 重点内容

| 编号 | 文档 | 描述 |
|------|------|------|
| 10 | [硬件故障排查方法论](10-hardware-troubleshooting-methodology.md) | 故障分类、诊断流程、工具体系、最佳实践 |
| 11 | [CPU与内存故障排查](11-cpu-memory-troubleshooting.md) | MCE错误、ECC故障、温度问题、性能诊断 |
| 12 | [存储设备故障排查](12-storage-troubleshooting.md) | HDD/SSD诊断、RAID故障、数据恢复 |
| 13 | [网络硬件故障排查](13-network-hardware-troubleshooting.md) | 网卡诊断、光模块故障、性能问题 |
| 14 | [电源与散热故障排查](14-power-thermal-troubleshooting.md) | PSU故障、温度监控、风扇诊断 |
| 15 | [BIOS与固件故障排查](15-bios-firmware-troubleshooting.md) | POST错误、固件更新、CMOS问题 |
| 16 | [Kubernetes硬件故障专题](16-kubernetes-hardware-troubleshooting.md) | K8s场景硬件故障、Node NotReady、PLEG、etcd磁盘 |
| 17 | [硬件错误码速查大全](17-hardware-error-codes-reference.md) | MCE/SMART/IPMI/NVMe错误码详解、BIOS蜂鸣码 |
| 18 | [硬件故障实战案例库](18-hardware-failure-case-studies.md) | 生产环境真实故障案例、诊断过程、解决方案 |

## 技术栈组件

- **处理器**: Intel Xeon, AMD EPYC, ARM (Graviton, Ampere)
- **内存**: DDR5 RDIMM/LRDIMM, ECC, 多通道配置
- **存储**: NVMe SSD, 企业级HDD, RAID控制器
- **网络**: 25G/100G/400G NIC, RDMA, SmartNIC
- **管理**: IPMI, Redfish, BMC

## 核心能力

### 硬件架构理解
- 数据中心Tier等级设计
- 服务器NUMA架构原理
- PCIe总线与扩展能力
- 高可用冗余设计

### 组件技术掌握
- CPU微架构与性能调优
- 内存子系统与RAS特性
- 存储介质特性与选型
- 网络硬件与高速互联

### 故障排查能力
- 系统化诊断方法论
- 硬件诊断工具使用
- 故障定位与修复
- 预防性维护策略

## 适用人群

- 数据中心运维工程师
- 系统架构师
- 硬件运维专家
- 云平台技术人员
- IT基础设施管理者

## 关键诊断命令速查

```bash
# CPU诊断
mcelog --client        # MCE错误
turbostat              # CPU频率状态
sensors                # 温度监控

# 内存诊断
edac-util -s           # ECC错误统计
dmidecode -t memory    # 内存信息

# 存储诊断
smartctl -a /dev/sda   # HDD SMART
nvme smart-log /dev/nvme0  # NVMe健康
storcli64 /c0 show     # RAID状态

# 网络诊断
ethtool eth0           # 链路状态
ethtool -S eth0        # 统计信息

# 电源散热
ipmitool sensor        # 传感器数据
ipmitool sel elist     # 事件日志
```

## 参考资源

- Intel Server Documentation
- AMD EPYC Technical Reference
- NVMe Specification
- IPMI/Redfish Standards
- 各服务器厂商技术文档

---

*最后更新: 2026年2月*
