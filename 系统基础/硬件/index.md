---
title: 硬件知识体系
description: 云原生基础设施硬件知识体系，覆盖服务器架构、CPU、内存、存储、网络硬件、故障排查、错误代码、生产案例等 18 个子领域
summary: 硬件知识体系总索引，覆盖服务器架构、CPU/内存/存储/网络技术、故障排查方法论、错误代码参考、生产案例
category: index
tags:
- index
- hardware
- server
- troubleshooting
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: intermediate
audience:
- SRE
- 平台工程师
- 硬件运维工程师
---

# 硬件知识体系

> 本知识体系覆盖云原生基础设施硬件的全域知识，是 SRE 和平台工程师理解底层硬件、排查硬件故障、优化集群性能的权威参考。

## 领域概述

硬件是云原生基础设施的物理基石，包括：

- **服务器架构**：机架式/刀片式/塔式、NUMA、PCIe
- **CPU 技术**：多核/多线程、缓存、指令集、GPU
- **内存技术**：DDR5、ECC、NUMA 拓扑、大页
- **存储技术**：HDD、SSD、NVMe、RAID、分布式存储
- **网络硬件**：网卡、交换机、DPU、RDMA
- **故障排查**：方法论、工具、错误代码、案例

## 文档索引

### 架构与原理

| 文档 | 内容 | 行数 |
|------|------|------|
| [[系统基础/硬件/01-cloud-hardware-architecture.md|云硬件架构]] | 数据中心、机架、供电、制冷 | 682 |
| [[系统基础/硬件/02-server-architecture-principles.md|服务器架构原理]] | 主板、总线、NUMA、PCIe | 748 |
| [[系统基础/硬件/03-cpu-technology-deep-dive.md|CPU 技术深度]] | 多核、缓存、指令集、GPU | 812 |
| [[系统基础/硬件/04-motherboard-chipset-technology.md|主板芯片组技术]] | 芯片组、BMC、BIOS/UEFI | 754 |
| [[系统基础/硬件/05-memory-technology-deep-dive.md|内存技术深度]] | DDR5、ECC、NUMA、大页 | 709 |

### 存储与网络

| 文档 | 内容 | 行数 |
|------|------|------|
| [[系统基础/硬件/06-storage-hdd-technology.md|HDD 技术]] | 机械硬盘、RAID、SAS | 698 |
| [[系统基础/硬件/07-storage-ssd-technology.md|SSD 技术]] | NVMe、TLC/QLC、磨损均衡 | 741 |
| [[系统基础/硬件/08-network-hardware-technology.md|网络硬件技术]] | 网卡、交换机、DPU、RDMA | 394 |

### 生态与厂商

| 文档 | 内容 | 行数 |
|------|------|------|
| [[系统基础/硬件/09-hardware-vendors-ecosystem.md|硬件厂商生态]] | Dell/HPE/浪潮/华为/超微 | 311 |

### 故障排查

| 文档 | 内容 | 行数 |
|------|------|------|
| [[系统基础/硬件/10-hardware-troubleshooting-methodology.md|故障排查方法论]] | 系统化排查流程、工具 | 749 |
| [[系统基础/硬件/11-cpu-memory-troubleshooting.md|CPU/内存故障]] | MCE、ECC 错误、降频 | 626 |
| [[系统基础/硬件/12-storage-troubleshooting.md|存储故障]] | 磁盘坏道、RAID 降级、IO 延迟 | 632 |
| [[系统基础/硬件/13-network-hardware-troubleshooting.md|网络硬件故障]] | 网卡、光模块、交换机 | 382 |
| [[系统基础/硬件/14-power-thermal-troubleshooting.md|电源/散热故障]] | PSU、风扇、温度告警 | 438 |
| [[系统基础/硬件/15-bios-firmware-troubleshooting.md|BIOS/固件故障]] | 固件升级、POST 失败 | 360 |

### K8s 与案例

| 文档 | 内容 | 行数 |
|------|------|------|
| [[系统基础/硬件/16-kubernetes-hardware-troubleshooting.md|K8s 硬件故障]] | 节点异常、硬件导致的 Pod 问题 | 909 |
| [[系统基础/硬件/17-hardware-error-codes-reference.md|错误代码参考]] | 各厂商错误代码详解 | 1275 |
| [[系统基础/硬件/18-hardware-failure-case-studies.md|故障案例研究]] | 生产真实故障案例 | 859 |

## 硬件选型指南

### K8s 节点硬件推荐

| 角色 | CPU | 内存 | 存储 | 网络 |
|------|-----|------|------|------|
| Control Plane | 8核+ | 32GB+ | SSD 200GB+ | 10GbE |
| Worker (通用) | 16核+ | 64GB+ | SSD 500GB+ | 25GbE |
| Worker (计算) | 32核+ | 128GB+ | NVMe 1TB+ | 25GbE |
| Worker (存储) | 16核+ | 64GB+ | NVMe 4TB+ | 25GbE |
| Worker (GPU) | 32核+ | 256GB+ | NVMe 2TB+ | 100GbE |
| 边缘节点 | 4核+ | 16GB+ | SSD 256GB+ | 1GbE |

### 关键硬件指标

| 指标 | 说明 | K8s 影响 |
|------|------|----------|
| CPU 核心数 | 物理核/逻辑核 | 可调度 CPU 资源 |
| 内存容量 | ECC 推荐 | 可调度内存资源 |
| 磁盘 IOPS | 随机读写性能 | etcd/镜像拉取 |
| 磁盘延迟 | fsync 延迟 | etcd 稳定性 |
| 网络带宽 | 节点间通信 | Pod 网络/CNI |
| NUMA 拓扑 | 内存亲和性 | 性能优化 |

## 常见硬件故障与 K8s 影响

| 硬件故障 | K8s 影响 | 排查方向 |
|----------|----------|----------|
| 磁盘故障 | 节点 DiskPressure、Pod 驱逐 | smartctl、dmesg |
| 内存 ECC 错误 | 节点不稳定、Pod 崩溃 | edac-util、mcelog |
| CPU 降频 | 性能下降、延迟增加 | turbostat、cpufreq |
| 网卡故障 | 节点 NotReady、Pod 网络中断 | ethtool、dmesg |
| 电源故障 | 节点宕机、数据丢失 | IPMI/BMC 日志 |
| 风扇故障 | 温度过高、CPU 降频 | ipmitool sdr |
| BIOS 故障 | 节点无法启动 | POST 代码、BMC |
| PCIe 错误 | 设备不可用、系统崩溃 | lspci、dmesg |

## 硬件监控命令

```bash
# CPU 信息
lscpu
cat /proc/cpuinfo
turbostat --interval 5  # 实时频率/温度

# 内存信息
free -h
dmidecode -t memory
edac-util -v  # ECC 错误
numactl --hardware  # NUMA 拓扑

# 磁盘信息
lsblk -f
smartctl -a /dev/sda
iostat -xz 1 5
nvme list  # NVMe 设备

# 网络硬件
ethtool eth0
lspci | grep -i net
cat /proc/net/dev

# 系统硬件总览
dmidecode -t system
dmidecode -t baseboard
lshw -short

# IPMI/BMC
ipmitool sdr list  # 传感器
ipmitool sel list  # 系统事件日志
ipmitool chassis status  # 电源状态

# 内核日志（硬件相关）
dmesg | grep -i "error\|fail\|hardware\|mce"
journalctl -k | grep -i "mce\|edac\|hardware error"
```

## 硬件故障排查流程

```
1. 现象确认
   - K8s 层面: 节点状态? Pod 状态? 事件?
   - OS 层面: dmesg? journalctl? 系统日志?

2. 硬件诊断
   - IPMI/BMC: 传感器、事件日志、远程控制台
   - 厂商工具: Dell iDRAC / HPE iLO / 浪潮 BMC
   - OS 工具: smartctl, edac-util, mcelog, turbostat

3. 故障定位
   - CPU/内存: MCE 日志、ECC 错误计数
   - 磁盘: SMART 数据、IO 错误、RAID 状态
   - 网络: 网卡错误计数、光模块状态
   - 电源/散热: PSU 状态、温度传感器

4. 修复/替换
   - 热插拔: 磁盘、电源、风扇
   - 冷更换: CPU、内存、主板
   - 固件升级: BIOS、BMC、网卡固件

5. 验证恢复
   - 硬件自检通过
   - 节点重新加入集群
   - Pod 正常调度运行
   - 监控指标恢复正常
```

## 硬件与 K8s 性能优化

### NUMA 感知调度

```yaml
# K8s NUMA 感知调度 (Topology Manager)
# kubelet 配置
topologyManagerPolicy: single-numa-node
cpuManagerPolicy: static
memoryManagerPolicy: Static
```

```bash
# 查看 NUMA 拓扑
numactl --hardware
lscpu | grep NUMA

# 查看进程 NUMA 分布
numastat -p <pid>

# 绑定 NUMA 节点运行
numactl --cpunodebind=0 --membind=0 ./myapp
```

### 大页内存配置

```bash
# 配置 2MB 大页
echo 1024 > /proc/sys/vm/nr_hugepages

# 配置 1GB 大页（需内核参数）
# /etc/default/grub: GRUB_CMDLINE_LINUX="hugepagesz=1G hugepages=4"

# 验证
cat /proc/meminfo | grep Huge
```

```yaml
# Pod 使用大页
resources:
  limits:
    hugepages-2Mi: 512Mi
    memory: 1Gi
    cpu: 1
```

### 磁盘 IO 优化

```bash
# 查看 IO 调度器
cat /sys/block/sda/queue/scheduler

# 设置 IO 调度器（SSD 推荐 none/mq-deadline）
echo none > /sys/block/nvme0n1/queue/scheduler

# 查看 IO 统计
iostat -xz 1 5
iotop -oP

# etcd 磁盘优化（必须 SSD）
# 检查 fsync 延迟
etcdctl endpoint status --write-out=table
```

## 硬件生命周期管理

### 采购与上架

```
需求分析 → 选型 → 采购 → 到货验收 → 上架 → 配置 → 测试 → 交付
   │          │       │        │        │       │       │       │
 容量规划  对比评测  合同   硬件检查  布线   BIOS   压测   纳管
 预算     兼容性  物流   配件清点  标签   网络   验证   监控
```

### 日常运维

| 任务 | 频率 | 工具 |
|------|------|------|
| 硬件巡检 | 每日 | IPMI/BMC、监控 |
| 固件检查 | 每月 | 厂商工具 |
| SMART 检查 | 每周 | smartctl |
| ECC 错误检查 | 每周 | edac-util |
| 温度检查 | 每日 | ipmitool sdr |
| 磁盘健康 | 每周 | smartctl、megacli |
| 固件升级 | 季度 | 厂商工具 |
| 硬件清灰 | 半年 | 现场维护 |

### 下线与报废

```
1. 节点 cordon + drain
   kubectl cordon <node>
   kubectl drain <node> --ignore-daemonsets --delete-emptydir-data

2. 数据迁移/备份
   - 迁移本地存储数据
   - 备份配置

3. 从集群移除
   kubectl delete node <node>

4. 数据擦除
   - 磁盘安全擦除（shred/blkdiscard）
   - 移除敏感配置

5. 物理下线
   - 断电、拔线
   - 下架、报废/回收
```

## 常见硬件错误代码

| 错误 | 含义 | 处理 |
|------|------|------|
| MCE (Machine Check Exception) | CPU/内存硬件错误 | 检查 mcelog、更换硬件 |
| ECC Uncorrectable | 内存不可纠正错误 | 立即更换内存条 |
| ECC Correctable | 内存可纠正错误 | 监控计数，超阈值更换 |
| SMART Reallocated Sectors | 磁盘坏道重映射 | 准备更换磁盘 |
| SMART Pending Sectors | 磁盘待重映射扇区 | 尽快更换磁盘 |
| PCIe AER Error | PCIe 总线错误 | 检查设备/插槽 |
| PSU Failure | 电源故障 | 更换电源模块 |
| Thermal Trip | 温度过高关机 | 检查散热/清灰 |
| POST Failure | 开机自检失败 | 检查内存/CPU/主板 |
| BMC Watchdog Timeout | BMC 看门狗超时 | 检查 BMC 固件 |
| DIMM Configuration Error | 内存配置错误 | 检查内存插槽顺序 |
| CPU Throttling | CPU 降频 | 检查温度/电源 |
| NIC Link Down | 网卡链路断开 | 检查网线/光模块 |
| RAID Degraded | RAID 降级 | 更换故障磁盘、重建 |
| NVMe Critical Warning | NVMe 严重警告 | 检查备用块/温度 |

## 硬件监控指标

### 关键监控指标

| 指标 | 阈值 | 告警级别 |
|------|------|----------|
| CPU 温度 | > 85°C | Warning |
| CPU 温度 | > 95°C | Critical |
| 内存 ECC CE | > 10/天 | Warning |
| 内存 ECC UE | > 0 | Critical |
| 磁盘使用率 | > 85% | Warning |
| 磁盘使用率 | > 95% | Critical |
| SMART Reallocated | > 0 | Warning |
| 磁盘 IO 延迟 | > 10ms (SSD) | Warning |
| 网卡错误包 | > 100/min | Warning |
| PSU 状态 | 非 Normal | Critical |
| 风扇转速 | < 50% | Warning |
| 节点温度 | > 35°C (进风) | Warning |

### Prometheus 硬件监控

```yaml
# Node Exporter 硬件指标
- alert: NodeHardwareTemperature
  expr: node_hwmon_temp_celsius > 85
  for: 5m
  labels:
    severity: warning

- alert: NodeDiskSmartError
  expr: node_smartmon_reallocated_sector_count_raw > 0
  for: 1m
  labels:
    severity: warning

- alert: NodeMemoryECCError
  expr: increase(node_edac_correctable_errors_total[1h]) > 10
  for: 5m
  labels:
    severity: warning

- alert: NodeDiskIOSaturation
  expr: rate(node_disk_io_time_seconds_total[5m]) > 0.9
  for: 10m
  labels:
    severity: warning
```

## 检查清单

### 硬件就绪检查

- [ ] 服务器固件为最新版本（BIOS/BMC/网卡）
- [ ] ECC 内存已启用且无错误
- [ ] 磁盘 SMART 状态正常
- [ ] etcd 使用 SSD/NVMe（fsync < 10ms）
- [ ] 冗余电源已配置
- [ ] IPMI/BMC 远程管理已配置
- [ ] 硬件监控已接入 Prometheus
- [ ] 节点标签已标注硬件信息（机型/配置）
- [ ] 备用硬件已准备（磁盘/内存/电源）
- [ ] 硬件故障应急流程已建立

## 学习路径

```
入门: 服务器架构 → CPU/内存基础 → 磁盘基础
中级: 存储技术 → 网络硬件 → 故障排查方法论
高级: K8s 硬件故障 → 错误代码 → 生产案例
专家: 硬件选型优化 → NUMA 调优 → 故障预测
```

## 参考链接

- https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
- https://www.smartmontools.org/
- https://www.kernel.org/doc/html/latest/admin-guide/ras.html
- https://www.dell.com/support/home/en-us/product-support/product/poweredge-r750/docs
- https://support.hpe.com/hpesc/public/home
- https://www.supermicro.com/en/support
- https://www.kernel.org/doc/html/latest/admin-guide/mm/hugetlbpage.html

## Related

- [[系统基础/Linux/index.md|Linux 知识]]
- [[系统基础/K8s事件/06-node-lifecycle-condition-events.md|节点生命周期事件]]
- [[系统基础/知识字典/fundamentals/index.md|K8s 基础知识]]

