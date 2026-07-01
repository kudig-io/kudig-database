---
title: 05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南
description: '# 05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南'
summary: '# 05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南'
category: linux
tags:
- linux
- system
- kernel
- etcd
- scheduler
- helm
- containerd
- docker
- ceph
- job
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 运维工程师
- SRE
- 系统管理员
estimated_read_time: 5min
intent_queries:
- Linux 存储管理与RAID配置：生产环境存储架构专家指南 是什么
- 如何 Linux 存储管理与RAID配置：生产环境存储架构专家指南
- Kubernetes 14 linux 最佳实践
trigger_keywords:
- Linux
- 存储管理与RAID配置：生产环境存储架构专家指南
- linux
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/linux.md
  label: '速查卡: linux'
---



# 05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南

> **适用版本**: Linux Kernel 5.x/6.x | **最后更新**: 2026-02 | **作者**: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: 摘要 -->## 摘要

本文档从生产环境存储架构专家视角，深入解析 Linux 存储管理、RAID配置和企业级存储解决方案。涵盖LVM逻辑卷管理、软件RAID配置、I/O性能优化、存储虚拟化等核心技术，为构建高可用、高性能的企业存储基础设施提供专业指导。

**核心价值**：
- 💾 **存储架构设计**：LVM、RAID、存储池的规划设计与实施
- ⚡ **性能优化**：I/O调度器调优、缓存策略、存储性能监控
- 🛡️ **数据保护**：RAID级别选择、故障恢复、数据备份策略
- 🔧 **运维管理**：存储资源监控、容量规划、自动化管理
- 💰 **成本优化**：存储资源利用率提升、分层存储策略

---

<!-- chunk: 目录 -->## 目录

- [块设备与分区](#块设备与分区)
- [LVM 逻辑卷管理](#lvm-逻辑卷管理)
- [软件 RAID](#软件-raid)
- [I/O 调度器](#io-调度器)
- [存储性能分析](#存储性能分析)
- [磁盘配额](#磁盘配额)

---

<!-- chunk: 块设备与分区 -->## 块设备与分区

## 块设备概述

| 设备类型 | 命名 | 说明 |
|:---|:---|:---|
| SATA/SAS | /dev/sd[a-z] | 传统硬盘 |
| NVMe | /dev/nvme[0-9]n[1-9] | NVMe SSD |
| 虚拟磁盘 | /dev/vd[a-z] | virtio 磁盘 |
| 设备映射 | /dev/dm-[0-9] | LVM/LUKS |

## 查看块设备

```bash
# 列出块设备
lsblk
lsblk -f    # 显示文件系统

# 详细信息
blkid

# 磁盘信息
fdisk -l
```

## 分区操作

```bash
# GPT 分区 (推荐)
gdisk /dev/sdb
# n - 新建分区
# w - 写入保存

# parted
parted /dev/sdb
(parted) mklabel gpt
(parted) mkpart primary xfs 0% 100%
```

---

<!-- chunk: LVM 逻辑卷管理 -->## LVM 逻辑卷管理

## LVM 架构

```
┌─────────────────────────────────────────────────────────────────┐
│  Logical Volume (LV)          逻辑卷: 文件系统挂载              │
│     /dev/vg01/lv_data                                           │
├─────────────────────────────────────────────────────────────────┤
│  Volume Group (VG)            卷组: 存储池                       │
│     vg01                                                        │
├─────────────────────────────────────────────────────────────────┤
│  Physical Volume (PV)         物理卷: 磁盘/分区                  │
│     /dev/sdb1    /dev/sdc1    /dev/sdd1                        │
└─────────────────────────────────────────────────────────────────┘
```

## LVM 操作

```bash
# 创建物理卷
pvcreate /dev/sdb1 /dev/sdc1

# 查看物理卷
pvs
pvdisplay

# 创建卷组
vgcreate vg01 /dev/sdb1 /dev/sdc1

# 查看卷组
vgs
vgdisplay

# 创建逻辑卷
lvcreate -L 100G -n lv_data vg01
lvcreate -l 100%FREE -n lv_data vg01   # 使用全部空间

# 查看逻辑卷
lvs
lvdisplay

# 格式化并挂载
mkfs.xfs /dev/vg01/lv_data
mount /dev/vg01/lv_data /data
```

## LVM 扩展

```bash
# 扩展 VG (添加新磁盘)
pvcreate /dev/sdd1
vgextend vg01 /dev/sdd1

# 扩展 LV
lvextend -L +50G /dev/vg01/lv_data
lvextend -l +100%FREE /dev/vg01/lv_data

# 扩展文件系统
# ext4
resize2fs /dev/vg01/lv_data

# xfs
xfs_growfs /data
```

## LVM 快照

```bash
# 创建快照
lvcreate -L 10G -s -n lv_data_snap /dev/vg01/lv_data

# 挂载快照
mount /dev/vg01/lv_data_snap /mnt/snapshot

# 合并/恢复快照
lvconvert --merge /dev/vg01/lv_data_snap

# 删除快照
lvremove /dev/vg01/lv_data_snap
```

---

<!-- chunk: 软件 RAID -->## 软件 RAID

## RAID 级别

| 级别 | 最少磁盘 | 容量利用 | 特点 |
|:---|:---:|:---:|:---|
| RAID 0 | 2 | 100% | 条带化，无冗余 |
| RAID 1 | 2 | 50% | 镜像 |
| RAID 5 | 3 | (n-1)/n | 分布式校验 |
| RAID 6 | 4 | (n-2)/n | 双校验 |
| RAID 10 | 4 | 50% | 镜像+条带 |

## mdadm 操作

```bash
# 创建 RAID 1
mdadm --create /dev/md0 --level=1 --raid-devices=2 /dev/sdb1 /dev/sdc1

# 创建 RAID 5
mdadm --create /dev/md0 --level=5 --raid-devices=3 /dev/sdb1 /dev/sdc1 /dev/sdd1

# 查看状态
cat /proc/mdstat
mdadm --detail /dev/md0

# 保存配置
mdadm --detail --scan >> /etc/mdadm.conf
```

## RAID 管理

```bash
# 添加磁盘
mdadm --add /dev/md0 /dev/sde1

# 标记问题
mdadm --fail /dev/md0 /dev/sdc1

# 移除磁盘
mdadm --remove /dev/md0 /dev/sdc1

# 停止 RAID
mdadm --stop /dev/md0

# 重新组装
mdadm --assemble /dev/md0 /dev/sdb1 /dev/sdc1
```

---

<!-- chunk: I/O 调度器 -->## I/O 调度器

## 调度器类型

| 调度器 | 特点 | 适用场景 |
|:---|:---|:---|
| **none** | 无调度 | NVMe SSD |
| **mq-deadline** | 截止时间 | 通用 |
| **bfq** | 公平队列 | 桌面交互 |
| **kyber** | 低延迟 | 高性能 |

## 配置调度器

```bash
# 查看当前调度器
cat /sys/block/sda/queue/scheduler

# 临时修改
echo mq-deadline > /sys/block/sda/queue/scheduler

# 永久配置 (GRUB)
# GRUB_CMDLINE_LINUX="elevator=mq-deadline"
```

---

<!-- chunk: 存储性能分析 -->## 存储性能分析

## I/O 监控

```bash
# iostat
iostat -xz 1

# iotop
iotop -oP

# dstat
dstat -d
```

## iostat 字段

| 字段 | 说明 |
|:---|:---|
| r/s | 每秒读请求 |
| w/s | 每秒写请求 |
| rMB/s | 读吞吐 |
| wMB/s | 写吞吐 |
| await | 平均等待 (ms) |
| %util | 磁盘利用率 |

## 性能测试

```bash
# fio 测试
fio --name=test --rw=randread --bs=4k --numjobs=4 \
    --size=1G --runtime=60 --filename=/dev/sdb

# dd 简单测试
dd if=/dev/zero of=/test bs=1M count=1024 oflag=direct
dd if=/test of=/dev/null bs=1M iflag=direct
```

---

<!-- chunk: 磁盘配额 -->## 磁盘配额

## 启用配额

```bash
# 挂载选项
mount -o usrquota,grpquota /dev/sdb1 /data

# /etc/fstab
/dev/sdb1  /data  xfs  defaults,usrquota,grpquota  0  2

# 初始化 (ext4)
quotacheck -cug /data
quotaon /data
```

## 配置配额

```bash
# 编辑用户配额
edquota -u username

# 批量设置
setquota -u username 1000000 1500000 0 0 /data
# 参数: 用户 软块 硬块 软inode 硬inode 路径

# 查看配额
quota -u username
repquota /data
```

---

<!-- chunk: 文件系统管理 -->## 文件系统管理

## 文件系统类型对比

| 文件系统 | 最大卷 | 最大文件 | 日志 | 快照 | 适用场景 |
|---------|--------|---------|------|------|---------|
| **ext4** | 1EiB | 16TiB | JBD2 | 否 | 通用 Linux 文件系统 |
| **XFS** | 8EiB | 8EiB | XFS日志 | 否（LVM快照） | 大文件、高并发 |
| **Btrfs** | 16EiB | 16EiB | COW | 是 | 数据完整性要求高 |
| **ZFS** | 256ZiB | 16EiB | COW | 是 | 企业存储、NAS |

## ext4 调优

```bash
# 创建 ext4 文件系统（优化参数）
mkfs.ext4 -L data_vol -b 4096 -E stride=128,stripe-width=256 /dev/vg01/lv_data
# -b 4096        块大小 4K（通用推荐）
# stride         RAID 条带大小 / 块大小
# stripe-width   stride × 数据盘数

# 挂载优化参数
mount -o noatime,nodiratime,data=writeback,barrier=0 /dev/vg01/lv_data /data
# noatime          不更新访问时间
# nodiratime       不更新目录访问时间
# data=writeback   延迟写入（性能优先，风险略高）
# data=ordered     默认，安全顺序写入
# barrier=0        禁用写屏障（有电池保护时可用）

# /etc/fstab 持久化
/dev/vg01/lv_data  /data  ext4  defaults,noatime,nodiratime  0 2
```

## XFS 调优

```bash
# 创建 XFS（优化参数）
mkfs.xfs -f -L data_vol -d su=64k,sw=4 /dev/vg01/lv_data
# su=64k   条带单元大小（匹配 RAID chunk size）
# sw=4     条带宽度（数据盘数量）

# XFS 挂载优化
mount -o noatime,nodiratime,logbufs=8,logbsize=256k /dev/vg01/lv_data /data

# XFS 在线扩容
xfs_growfs /data

# XFS 无法缩容，规划时需预留空间

# /etc/fstab
/dev/vg01/lv_data  /data  xfs  defaults,noatime,nodiratime,logbufs=8  0 2
```

## 文件系统性能对比测试

```bash
#!/bin/bash
# fs-benchmark.sh - 文件系统性能对比测试
# 前提: 准备独立的测试卷

TEST_DEV="/dev/vg01/lv_test"
MOUNT_POINT="/mnt/fstest"
FIO_SIZE="2G"

for FS in ext4 xfs; do
  echo "========== 测试文件系统: $FS =========="

  # 格式化
  if [ "$FS" = "ext4" ]; then
    mkfs.ext4 -F "$TEST_DEV" >/dev/null 2>&1
  else
    mkfs.xfs -f "$TEST_DEV" >/dev/null 2>&1
  fi

  # 挂载
  mkdir -p "$MOUNT_POINT"
  mount -o noatime "$TEST_DEV" "$MOUNT_POINT"

  # 顺序读写测试
  echo "  顺序读写:"
  fio --name=seq-read --rw=read --bs=1M --numjobs=1 --size="$FIO_SIZE" \
      --directory="$MOUNT_POINT" --runtime=30 --time_based --minimal 2>/dev/null | \
      awk -F';' '{printf "    顺序读: %.1f MB/s\n", $7/1024}'

  fio --name=seq-write --rw=write --bs=1M --numjobs=1 --size="$FIO_SIZE" \
      --directory="$MOUNT_POINT" --runtime=30 --time_based --minimal 2>/dev/null | \
      awk -F';' '{printf "    顺序写: %.1f MB/s\n", $48/1024}'

  # 随机读写测试
  echo "  随机读写:"
  fio --name=rand-read --rw=randread --bs=4k --numjobs=4 --size="$FIO_SIZE" \
      --directory="$MOUNT_POINT" --runtime=30 --time_based --minimal 2>/dev/null | \
      awk -F';' '{printf "    随机读: %.0f IOPS\n", $8}'

  fio --name=rand-write --rw=randwrite --bs=4k --numjobs=4 --size="$FIO_SIZE" \
      --directory="$MOUNT_POINT" --runtime=30 --time_based --minimal 2>/dev/null | \
      awk -F';' '{printf "    随机写: %.0f IOPS\n", $49}'

  # 清理
  umount "$MOUNT_POINT"
  echo ""
done
```

---

<!-- chunk: 企业级 LVM 实践 -->## 企业级 LVM 实践

## LVM 条带化（Striping）

```bash
# 创建条带化 LV（提升顺序读写性能）
lvcreate -L 500G -i 4 -I 64K -n lv_stripe vg01
# -i 4      使用 4 个物理卷
# -I 64K    条带大小 64KB

# 条带化 + 镜像（性能 + 冗余）
lvcreate -L 200G -m 1 --mirrorlog mirrored -n lv_mirrored vg01
```

## LVM 缓存池

```bash
# 创建缓存池（SSD 加速 HDD）
# 1. 在 SSD 上创建缓存 PV
pvcreate /dev/nvme0n1p1
vgextend vg01 /dev/nvme0n1p1

# 2. 创建缓存池
lvcreate -L 50G -n lv_cache_pool vg01 /dev/nvme0n1p1
lvconvert --type cache-pool vg01/lv_cache_pool

# 3. 将缓存池附加到 HDD 逻辑卷
lvconvert --type cache --cachepool vg01/lv_cache_pool vg01/lv_data
```

## LVM 精简配置（Thin Provisioning）

```bash
# 创建精简池
lvcreate -L 200G -T vg01/thin_pool

# 创建精简卷（可超额分配）
lvcreate -V 100G -T vg01/thin_pool -n thin_vol1
lvcreate -V 150G -T vg01/thin_pool -n thin_vol2

# 监控精简池使用率
lvs -o+seg_monitor,lv_attr,lv_metadata_size,data_percent
# 当 data_percent 接近 100% 时需扩容
lvresize -L +50G vg01/thin_pool
```

---

<!-- chunk: 高级 RAID 运维 -->## 高级 RAID 运维

## RAID 性能调优

```bash
# RAID 条带大小优化
mdadm --create /dev/md0 --level=0 --raid-devices=4 \
  --chunk=512 /dev/sd[b-e]1
# chunk=512K: 适合大文件顺序读写
# chunk=64K:  适合小文件随机读写

# RAID 重建速度控制
echo 50000 > /proc/sys/dev/raid/speed_limit_min
echo 200000 > /proc/sys/dev/raid/speed_limit_max
# 最小/最大重建速度 (KB/s)

# RAID 位图（加速重建）
mdadm --grow --bitmap=internal /dev/md0

# 查看重建进度
cat /proc/mdstat | grep recovery
```

## RAID 磁盘故障处理 SOP

```bash
#!/bin/bash
# raid-failure-handler.sh - RAID 磁盘问题自动处理

MD_DEVICE="${1:-/dev/md0}"
FAILED_DISK="$2"

if [ -z "$FAILED_DISK" ]; then
  echo "用法: $0 <md设备> <问题磁盘设备>"
  exit 1
fi

echo "<!-- chunk: RAID 故障处理: $MD_DEVICE - $FAILED_DISK" -->## RAID 故障处理: $MD_DEVICE - $FAILED_DISK"

# 1. 确认问题
echo "1. 确认当前 RAID 状态..."
mdadm --detail "$MD_DEVICE" | grep -E "(State|Active|Working|Failed)"

# 2. 标记问题磁盘
echo "2. 标记磁盘为问题..."
mdadm --fail "$MD_DEVICE" "$FAILED_DISK"

# 3. 移除问题磁盘
echo "3. 移除问题磁盘..."
mdadm --remove "$MD_DEVICE" "$FAILED_DISK"

# 4. 物理更换磁盘后，添加新磁盘
echo "4. 请物理更换磁盘后执行:"
echo "   mdadm --add $MD_DEVICE <新磁盘设备>"

# 5. 监控重建进度
echo "5. 监控重建进度:"
echo "   watch cat /proc/mdstat"
```

---

<!-- chunk: 存储自动化巡检脚本 -->## 存储自动化巡检脚本

```bash
#!/bin/bash
# storage-health-check.sh - Linux 存储健康巡检

echo "=========================================="
echo "Linux 存储健康巡检报告"
echo "时间: $(date)"
echo "主机: $(hostname)"
echo "=========================================="

# 1. 磁盘使用率检查
echo ""
echo "<!-- chunk: 1. 磁盘使用率 (>80% 告警)" -->## 1. 磁盘使用率 (>80% 告警)"
df -h --type=ext4 --type=xfs --type=btrfs 2>/dev/null | \
  awk 'NR==1 || +$5 > 80 {print "   "$0}'

# 2. Inode 使用率检查
echo ""
echo "<!-- chunk: 2. Inode 使用率 (>80% 告警)" -->## 2. Inode 使用率 (>80% 告警)"
df -i --type=ext4 --type=xfs 2>/dev/null | \
  awk 'NR==1 || +$5 > 80 {print "   "$0}'

# 3. RAID 状态检查
echo ""
echo "<!-- chunk: 3. RAID 状态" -->## 3. RAID 状态"
if [ -f /proc/mdstat ]; then
  grep -E "(md[0-9]|raid|resync|recovery|failed)" /proc/mdstat || echo "   所有 RAID 正常"
else
  echo "   未配置软件 RAID"
fi

# 4. LVM 状态
echo ""
echo "<!-- chunk: 4. LVM 卷组使用率" -->## 4. LVM 卷组使用率"
vgs --units g --noheadings -o vg_name,vg_size,vg_free,vg_attr 2>/dev/null | \
  while read name size free attr; do
    used_pct=$(echo "scale=1; ($size - $free) * 100 / $size" | bc 2>/dev/null)
    echo "   VG: $name | 总计: ${size}G | 可用: ${free}G | 使用率: ${used_pct}%"
  done

# 5. SMART 磁盘健康检查
echo ""
echo "<!-- chunk: 5. SMART 磁盘健康" -->## 5. SMART 磁盘健康"
if command -v smartctl &>/dev/null; then
  for DISK in $(ls /dev/sd? 2>/dev/null); do
    HEALTH=$(smartctl -H "$DISK" 2>/dev/null | grep -i "overall" | awk '{print $NF}')
    echo "   $DISK: ${HEALTH:-未知}"
  done
else
  echo "   smartctl 未安装，跳过 SMART 检查"
fi

# 6. 挂载点异常检测
echo ""
echo "<!-- chunk: 6. 挂载点异常检测" -->## 6. 挂载点异常检测"
cat /proc/mounts | awk '{print $2}' | while read mp; do
  if ! timeout 3 stat "$mp" >/dev/null 2>&1; then
    echo "   ⚠️ 挂载点无响应: $mp"
  fi
done
echo "   挂载点检测完成"

# 7. I/O 错误检查
echo ""
echo "<!-- chunk: 7. 内核 I/O 错误" -->## 7. 内核 I/O 错误"
dmesg | grep -i -E "(i/o error|buffer i/o error|ext4-fs error|xfs error)" | tail -5 || echo "   无 I/O 错误"

echo ""
echo "=========================================="
echo "巡检完成"
echo "=========================================="
```

---

<!-- chunk: 网络文件系统 -->## 网络文件系统

## NFS 客户端优化

```bash
# NFS 挂载优化参数
mount -t nfs -o vers=4.1,rsize=1048576,wsize=1048576,hard,intr,noatime \
  nfs-server:/export/data /mnt/nfs

# 参数说明:
# vers=4.1       NFS v4.1（推荐）
# rsize/wsize    读写块大小（1MB 推荐）
# hard           硬挂载（I/O 等待恢复）
# intr           允许中断挂起的 I/O
# noatime        不更新访问时间

# /etc/fstab
nfs-server:/export/data  /mnt/nfs  nfs  vers=4.1,rsize=1048576,wsize=1048576,hard,intr,noatime  0 0
```

## iSCSI 配置

```bash
# 安装 iSCSI 客户端
yum install -y iscsi-initiator-utils
systemctl enable --now iscsid

# 发现 Target
iscsiadm -m discovery -t st -p 192.168.1.100:3260

# 登录 Target
iscsiadm -m node -T iqn.2024.storage:vol01 -p 192.168.1.100:3260 --login

# 查看映射的磁盘
lsblk
fdisk -l | grep "Disk /dev"

# 多路径配置（multipath）
yum install -y device-mapper-multipath
mpathconf --enable --with_multipathd y
multipath -ll
```

---

<!-- chunk: 与 [[Kubernetes|Kubernetes]] 的关系 -->## 与 Kubernetes 的关系

## K8s 持久化存储架构

Kubernetes 使用 PV (PersistentVolume) 和 PVC (PersistentVolumeClaim) 抽象存储管理，底层依赖 Linux 存储技术。

```
┌─────────────────────────────────────────────────────────────────┐
│                    K8s 持久化存储映射                              │
│                                                                  │
│  Kubernetes 抽象          Linux 底层技术                          │
│  ──────────────          ─────────────                           │
│  StorageClass         →  LVM 精简池 / Ceph Pool / NFS           │
│  PV (块模式)          →  /dev/sdX → mkfs → mount                │
│  PV (文件系统模式)     →  NFS mount / hostPath mount             │
│  PVC                  →  lvcreate (动态供应)                     │
│  Volume Mount         →  bind mount / overlay mount              │
│  emptyDir             →  tmpfs / host path (临时)               │
│  hostPath             →  直接挂载主机目录                        │
│  Local PV             →  本地块设备 + mount                      │
│                                                                  │
│  CSI (Container Storage Interface) 驱动:                         │
│  ├── Amazon EBS CSI     →  AWS EBS 卷                           │
│  ├── GCE PD CSI        →  GCE Persistent Disk                   │
│  ├── Azure Disk CSI    →  Azure Managed Disk                    │
│  ├── Ceph CSI          →  Ceph RBD / CephFS                     │
│  ├── NFS CSI           →  NFS 挂载                              │
│  ├── Local Path CSI    →  本地路径 (hostPath 增强)              │
│  └── TopoLVM CSI       →  LVM 动态供应                          │
└─────────────────────────────────────────────────────────────────┘
```

## etcd 存储要求

etcd 是 Kubernetes 的核心数据存储，对磁盘 I/O 延迟极度敏感：

```bash
# etcd 磁盘性能要求
# FSYNC 延迟 < 10ms (推荐 < 2ms)
# IOPS > 300 (推荐 > 1000)

# 测试 etcd 磁盘性能
fio --name=etcd-test --rw=write --bs=4k --numjobs=1 \
    --size=1G --runtime=60 --filename=/var/lib/etcd/test \
    --direct=1 --fsync=1 --ioengine=libaio

# etcd 数据目录
# 推荐使用独立 SSD
mount -o noatime,data=ordered /dev/nvme0n1p1 /var/lib/etcd

# 查看 etcd 磁盘使用
du -sh /var/lib/etcd/
etcdctl endpoint status --write-out=table
```

---

<!-- chunk: 性能调优 -->## 性能调优

## 存储性能优化矩阵

| 场景 | 文件系统 | I/O 调度器 | 挂载参数 | RAID 级别 |
|:---|:---|:---|:---|:---|
| **K8s 通用节点** | XFS | none (SSD) | noatime | RAID1 或 JBOD |
| **etcd** | XFS | none (SSD) | noatime,data=ordered | 独立 SSD |
| **数据库** | XFS | mq-deadline | noatime,logbufs=8 | RAID10 |
| **日志存储** | ext4 | mq-deadline | noatime,data=writeback | RAID5/6 |
| **容器镜像** | XFS | none (SSD) | noatime | JBOD |
| **NFS 服务** | XFS | mq-deadline | noatime | RAID6 |

## LVM 与 K8s 动态存储供应

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 使用 TopoLVM CSI 实现 LVM 动态供应
# 1. 在所有节点创建 VG
pvcreate /dev/nvme0n1p3
vgcreate vg-k8s /dev/nvme0n1p3

# 2. 安装 TopoLVM CSI
# helm install --namespace toplvm-system toplvm toplvm/topolvm

# 3. 创建 StorageClass
cat << 'EOF' | kubectl apply -f -
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: toplvm-ssd
provisioner: toplvm.cybozu.com
parameters:
  "csi.storage.k8s.io/fstype": xfs
volumeBindingMode: WaitForFirstConsumer
allowedTopologies:
- matchLabelExpressions:
  - key: toplvm.cybozu.com/ssd
    values: ["true"]
EOF
```

---

<!-- chunk: 安全加固 -->## 安全加固

## 存储安全

在生产环境中，存储安全是不可忽视的重要环节。数据加密、访问控制和完整性校验是存储安全的三大支柱。LUKS (Linux Unified Key Setup) 提供透明的块设备加密，确保即使物理磁盘被盗也无法读取数据。LVM 的精简配置需要特别监控，避免空间耗尽导致数据丢失。

```bash
# LUKS 磁盘加密
cryptsetup luksFormat /dev/sdb1
cryptsetup luksOpen /dev/sdb1 encrypted-data
mkfs.xfs /dev/mapper/encrypted-data
mount /dev/mapper/encrypted-data /secure-data

# 自动挂载 (需要 keyfile 或密码输入)
# /etc/crypttab
encrypted-data /dev/sdb1 /root/luks-key luks

# LVM 加密
cryptsetup luksFormat /dev/vg01/lv_secret
cryptsetup luksOpen /dev/vg01/lv_secret crypted
mkfs.xfs /dev/mapper/crypted

# 文件系统级别安全
# 挂载选项安全加固
mount -o noexec,nosuid,nodev /dev/sdb1 /data     # 禁止执行/SUID/设备文件
mount -o noatime,nodiratime /dev/sdb1 /backup     # 性能优化

# NFS 安全配置
mount -t nfs -o sec=krb5,vers=4.2 nfs-server:/export /mnt/nfs  # Kerberos 认证

# iSCSI 安全 (CHAP 认证)
iscsiadm -m node -T iqn.2024.storage:vol01 -p 192.168.1.100:3260 \
  --op=update -n node.session.auth.authmethod -v CHAP
iscsiadm -m node -T iqn.2024.storage:vol01 -p 192.168.1.100:3260 \
  --op=update -n node.session.auth.username -v admin
iscsiadm -m node -T iqn.2024.storage:vol01 -p 192.168.1.100:3260 \
  --op=update -n node.session.auth.password -v secret123
```

---

<!-- chunk: 最佳实践 -->## 最佳实践

1. **使用独立磁盘存放 etcd 数据**: etcd 对 I/O 延迟极其敏感
2. **LVM 用于动态存储**: 配合 CSI 驱动实现动态 PV 供应
3. **XFS 用于大文件**: 日志和数据库存储推荐 XFS
4. **监控磁盘 SMART**: 定期检查磁盘健康状态
5. **预留空间**: LVM 精简池预留 20% 安全余量
6. **RAID 选型**: 数据安全用 RAID1/10，归档用 RAID5/6

---

<!-- chunk: 故障排查 -->## 故障排查

## 存储故障诊断

```bash
# LVM 问题
vgs -o+vg_attr                        # VG 状态
lvs -a -o+lv_attr,devices             # LV 状态（含隐藏 LV）
pvs -a -o+pv_attr                     # PV 状态

# RAID 问题
cat /proc/mdstat                       # RAID 状态
mdadm --detail /dev/md0               # 详细信息
mdadm --examine /dev/sdb1             # 检查磁盘超级块

# 磁盘错误
smartctl -a /dev/sda                   # SMART 信息
smartctl -H /dev/sda                   # 健康状态
badblocks -sv /dev/sda                 # 坏块检测
```

## 存储性能监控脚本

```bash
#!/bin/bash
# storage-perf-monitor.sh - 存储性能监控

echo "=== 存储性能监控 $(date) ==="

echo -e "\n[1] I/O 延迟 (await > 10ms 警告)"
iostat -xz 1 3 | tail -n +4 | \
  awk 'NF>1 && $9+0 > 10 {print "  警告: "$1" await="$9"ms"}'

echo -e "\n[2] 磁盘利用率 (>80% 警告)"
iostat -xz 1 3 | tail -n +4 | \
  awk 'NF>1 && $14+0 > 80 {print "  警告: "$1" util="$14"%"}'

echo -e "\n[3] LVM 使用率"
lvs --units g --noheadings -o lv_name,vg_name,lv_size,data_percent 2>/dev/null | \
  while read lv vg size pct; do
    if [ -n "$pct" ] && [ "$(echo "$pct > 80" | bc 2>/dev/null)" = "1" ]; then
      echo "  警告: $vg/$lv 使用率 ${pct}%"
    fi
  done

echo -e "\n[4] RAID 状态"
if [ -f /proc/mdstat ]; then
  grep -E "resync|recovery|degraded|failed" /proc/mdstat || echo "  所有 RAID 正常"
else
  echo "  未配置软件 RAID"
fi

echo -e "\n[5] SMART 健康"
for disk in $(ls /dev/sd? 2>/dev/null); do
  health=$(smartctl -H "$disk" 2>/dev/null | grep -i "overall" | awk '{print $NF}')
  echo "  $disk: ${health:-未知}"
done

echo "=== 监控完成 ==="
```

## 存储容量规划

存储容量规划是避免磁盘空间耗尽导致服务中断的关键。在 Kubernetes 环境中，需要同时关注节点本地存储和持久化存储的容量趋势。

```bash
#!/bin/bash
# storage-capacity-check.sh - 存储容量检查

echo "=== 存储容量检查 $(date) ==="

echo -e "\n[1] 本地存储使用"
df -h --type=ext4 --type=xfs 2>/dev/null | awk '
NR==1 {print "  "$0; next}
{printf "  %-20s %6s / %6s (%s)\n", $6, $3, $2, $5}'

echo -e "\n[2] LVM 可用空间"
vgs --units g --noheadings -o vg_name,vg_free 2>/dev/null | \
  while read vg free; do
    echo "  VG $vg: ${free}G 可用"
  done

echo -e "\n[3] inode 使用率 (>80% 警告)"
df -i --type=ext4 --type=xfs 2>/dev/null | \
  awk 'NR==1 || +$5 > 80 {printf "  %-20s %s\n", $6, $5}'

echo -e "\n[4] 容器存储使用"
if [ -d /var/lib/docker ]; then
  docker system df 2>/dev/null
elif [ -d /var/lib/containerd ]; then
  du -sh /var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/ 2>/dev/null
fi

echo -e "\n[5] 大目录 TOP 10"
du -sh /* 2>/dev/null | sort -rh | head -10

echo "=== 检查完成 ==="
```

---

<!-- chunk: 相关文档 -->## 相关文档

- [Linux 文件系统详解](./03-linux-filesystem-deep-dive.md) - 文件系统深度解析
- [Linux 性能调优](./06-linux-performance-tuning.md) - 性能分析
- [Linux 系统架构](./01-linux-system-architecture.md) - 系统架构

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)

## See Also

- 03-linux-filesystem-deep-dive
- 04-linux-networking-configuration
- 06-linux-performance-tuning
- 07-linux-security-hardening

## Related

- index/pvc-index|PVC 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/etcd-index.md|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/storage-index.md|Storage 存储知识图谱索引]]
