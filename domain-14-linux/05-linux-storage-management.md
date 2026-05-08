# 05 - Linux 存储管理与RAID配置：生产环境存储架构专家指南

> **适用版本**: Linux Kernel 5.x/6.x | **最后更新**: 2026-02 | **作者**: Allen Galler (allengaller@gmail.com)

---

## 摘要

本文档从生产环境存储架构专家视角，深入解析 Linux 存储管理、RAID配置和企业级存储解决方案。涵盖LVM逻辑卷管理、软件RAID配置、I/O性能优化、存储虚拟化等核心技术，为构建高可用、高性能的企业存储基础设施提供专业指导。

**核心价值**：
- 💾 **存储架构设计**：LVM、RAID、存储池的规划设计与实施
- ⚡ **性能优化**：I/O调度器调优、缓存策略、存储性能监控
- 🛡️ **数据保护**：RAID级别选择、故障恢复、数据备份策略
- 🔧 **运维管理**：存储资源监控、容量规划、自动化管理
- 💰 **成本优化**：存储资源利用率提升、分层存储策略

---

## 目录

- [块设备与分区](#块设备与分区)
- [LVM 逻辑卷管理](#lvm-逻辑卷管理)
- [软件 RAID](#软件-raid)
- [I/O 调度器](#io-调度器)
- [存储性能分析](#存储性能分析)
- [磁盘配额](#磁盘配额)

---

## 块设备与分区

### 块设备概述

| 设备类型 | 命名 | 说明 |
|:---|:---|:---|
| SATA/SAS | /dev/sd[a-z] | 传统硬盘 |
| NVMe | /dev/nvme[0-9]n[1-9] | NVMe SSD |
| 虚拟磁盘 | /dev/vd[a-z] | virtio 磁盘 |
| 设备映射 | /dev/dm-[0-9] | LVM/LUKS |

### 查看块设备

```bash
# 列出块设备
lsblk
lsblk -f    # 显示文件系统

# 详细信息
blkid

# 磁盘信息
fdisk -l
```

### 分区操作

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

## LVM 逻辑卷管理

### LVM 架构

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

### LVM 操作

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

### LVM 扩展

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

### LVM 快照

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

## 软件 RAID

### RAID 级别

| 级别 | 最少磁盘 | 容量利用 | 特点 |
|:---|:---:|:---:|:---|
| RAID 0 | 2 | 100% | 条带化，无冗余 |
| RAID 1 | 2 | 50% | 镜像 |
| RAID 5 | 3 | (n-1)/n | 分布式校验 |
| RAID 6 | 4 | (n-2)/n | 双校验 |
| RAID 10 | 4 | 50% | 镜像+条带 |

### mdadm 操作

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

### RAID 管理

```bash
# 添加磁盘
mdadm --add /dev/md0 /dev/sde1

# 标记故障
mdadm --fail /dev/md0 /dev/sdc1

# 移除磁盘
mdadm --remove /dev/md0 /dev/sdc1

# 停止 RAID
mdadm --stop /dev/md0

# 重新组装
mdadm --assemble /dev/md0 /dev/sdb1 /dev/sdc1
```

---

## I/O 调度器

### 调度器类型

| 调度器 | 特点 | 适用场景 |
|:---|:---|:---|
| **none** | 无调度 | NVMe SSD |
| **mq-deadline** | 截止时间 | 通用 |
| **bfq** | 公平队列 | 桌面交互 |
| **kyber** | 低延迟 | 高性能 |

### 配置调度器

```bash
# 查看当前调度器
cat /sys/block/sda/queue/scheduler

# 临时修改
echo mq-deadline > /sys/block/sda/queue/scheduler

# 永久配置 (GRUB)
# GRUB_CMDLINE_LINUX="elevator=mq-deadline"
```

---

## 存储性能分析

### I/O 监控

```bash
# iostat
iostat -xz 1

# iotop
iotop -oP

# dstat
dstat -d
```

### iostat 字段

| 字段 | 说明 |
|:---|:---|
| r/s | 每秒读请求 |
| w/s | 每秒写请求 |
| rMB/s | 读吞吐 |
| wMB/s | 写吞吐 |
| await | 平均等待 (ms) |
| %util | 磁盘利用率 |

### 性能测试

```bash
# fio 测试
fio --name=test --rw=randread --bs=4k --numjobs=4 \
    --size=1G --runtime=60 --filename=/dev/sdb

# dd 简单测试
dd if=/dev/zero of=/test bs=1M count=1024 oflag=direct
dd if=/test of=/dev/null bs=1M iflag=direct
```

---

## 磁盘配额

### 启用配额

```bash
# 挂载选项
mount -o usrquota,grpquota /dev/sdb1 /data

# /etc/fstab
/dev/sdb1  /data  xfs  defaults,usrquota,grpquota  0  2

# 初始化 (ext4)
quotacheck -cug /data
quotaon /data
```

### 配置配额

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

## 文件系统管理

### 文件系统类型对比

| 文件系统 | 最大卷 | 最大文件 | 日志 | 快照 | 适用场景 |
|---------|--------|---------|------|------|---------|
| **ext4** | 1EiB | 16TiB | JBD2 | 否 | 通用 Linux 文件系统 |
| **XFS** | 8EiB | 8EiB | XFS日志 | 否（LVM快照） | 大文件、高并发 |
| **Btrfs** | 16EiB | 16EiB | COW | 是 | 数据完整性要求高 |
| **ZFS** | 256ZiB | 16EiB | COW | 是 | 企业存储、NAS |

### ext4 调优

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

### XFS 调优

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

### 文件系统性能对比测试

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

## 企业级 LVM 实践

### LVM 条带化（Striping）

```bash
# 创建条带化 LV（提升顺序读写性能）
lvcreate -L 500G -i 4 -I 64K -n lv_stripe vg01
# -i 4      使用 4 个物理卷
# -I 64K    条带大小 64KB

# 条带化 + 镜像（性能 + 冗余）
lvcreate -L 200G -m 1 --mirrorlog mirrored -n lv_mirrored vg01
```

### LVM 缓存池

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

### LVM 精简配置（Thin Provisioning）

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

## 高级 RAID 运维

### RAID 性能调优

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

### RAID 磁盘故障处理 SOP

```bash
#!/bin/bash
# raid-failure-handler.sh - RAID 磁盘故障自动处理

MD_DEVICE="${1:-/dev/md0}"
FAILED_DISK="$2"

if [ -z "$FAILED_DISK" ]; then
  echo "用法: $0 <md设备> <故障磁盘设备>"
  exit 1
fi

echo "## RAID 故障处理: $MD_DEVICE - $FAILED_DISK"

# 1. 确认故障
echo "1. 确认当前 RAID 状态..."
mdadm --detail "$MD_DEVICE" | grep -E "(State|Active|Working|Failed)"

# 2. 标记故障磁盘
echo "2. 标记磁盘为故障..."
mdadm --fail "$MD_DEVICE" "$FAILED_DISK"

# 3. 移除故障磁盘
echo "3. 移除故障磁盘..."
mdadm --remove "$MD_DEVICE" "$FAILED_DISK"

# 4. 物理更换磁盘后，添加新磁盘
echo "4. 请物理更换磁盘后执行:"
echo "   mdadm --add $MD_DEVICE <新磁盘设备>"

# 5. 监控重建进度
echo "5. 监控重建进度:"
echo "   watch cat /proc/mdstat"
```

---

## 存储自动化巡检脚本

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
echo "## 1. 磁盘使用率 (>80% 告警)"
df -h --type=ext4 --type=xfs --type=btrfs 2>/dev/null | \
  awk 'NR==1 || +$5 > 80 {print "   "$0}'

# 2. Inode 使用率检查
echo ""
echo "## 2. Inode 使用率 (>80% 告警)"
df -i --type=ext4 --type=xfs 2>/dev/null | \
  awk 'NR==1 || +$5 > 80 {print "   "$0}'

# 3. RAID 状态检查
echo ""
echo "## 3. RAID 状态"
if [ -f /proc/mdstat ]; then
  grep -E "(md[0-9]|raid|resync|recovery|failed)" /proc/mdstat || echo "   所有 RAID 正常"
else
  echo "   未配置软件 RAID"
fi

# 4. LVM 状态
echo ""
echo "## 4. LVM 卷组使用率"
vgs --units g --noheadings -o vg_name,vg_size,vg_free,vg_attr 2>/dev/null | \
  while read name size free attr; do
    used_pct=$(echo "scale=1; ($size - $free) * 100 / $size" | bc 2>/dev/null)
    echo "   VG: $name | 总计: ${size}G | 可用: ${free}G | 使用率: ${used_pct}%"
  done

# 5. SMART 磁盘健康检查
echo ""
echo "## 5. SMART 磁盘健康"
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
echo "## 6. 挂载点异常检测"
cat /proc/mounts | awk '{print $2}' | while read mp; do
  if ! timeout 3 stat "$mp" >/dev/null 2>&1; then
    echo "   ⚠️ 挂载点无响应: $mp"
  fi
done
echo "   挂载点检测完成"

# 7. I/O 错误检查
echo ""
echo "## 7. 内核 I/O 错误"
dmesg | grep -i -E "(i/o error|buffer i/o error|ext4-fs error|xfs error)" | tail -5 || echo "   无 I/O 错误"

echo ""
echo "=========================================="
echo "巡检完成"
echo "=========================================="
```

---

## 网络文件系统

### NFS 客户端优化

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

### iSCSI 配置

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

## 相关文档

- [Linux 文件系统详解](./03-linux-filesystem-deep-dive.md) - 文件系统深度解析
- [RAID 存储冗余](../domain-16-storage-fundamentals/03-raid-storage-redundancy.md) - RAID 深度配置
- [存储技术概览](../domain-16-storage-fundamentals/01-storage-technologies-overview.md) - 存储技术全景
- [Linux 存储性能](../domain-16-storage-fundamentals/06-storage-performance-iops.md) - IOPS 与性能基准

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)
