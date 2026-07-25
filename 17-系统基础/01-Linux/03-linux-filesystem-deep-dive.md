---
title: 03 - Linux 文件系统深度解析：生产环境存储管理专家指南
description: '# 03 - Linux 文件系统深度解析：生产环境存储管理专家指南'
summary: '本文档从生产环境存储管理专家视角，深入解析 Linux 文件系统架构、性能优化和企业级运维实践。涵盖 VFS 虚拟文件系统、各类文件系统选型、存储性能调优、数据保护策略等关键内容，为构建高可用、高性能的存储基础设施提供专业指导。'
category: linux
tags:
- linux
- system
- kernel
- kubelet
- containerd
- docker
- ceph
- mysql
- rag
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
- Linux 文件系统深度解析：生产环境存储管理专家指南 是什么
- 如何 Linux 文件系统深度解析：生产环境存储管理专家指南
- Kubernetes 14 linux 最佳实践
trigger_keywords:
- Linux
- 文件系统深度解析：生产环境存储管理专家指南
- linux
prerequisites:
- kubectl-basics
- cloud-provider-basics
- mysql-basics
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
  path: ../系统基础/topic-cheat-sheet/linux.md
  label: '速查卡: linux'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 03 - Linux 文件系统深度解析：生产环境存储管理专家指南

> **适用版本**: Linux Kernel 5.x/6.x | **最后更新**: 2026-02 | **作者**: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: 摘要 -->## 摘要

本文档从生产环境存储管理专家视角，深入解析 Linux 文件系统架构、性能优化和企业级运维实践。涵盖 VFS 虚拟文件系统、各类文件系统选型、存储性能调优、数据保护策略等关键内容，为构建高可用、高性能的存储基础设施提供专业指导。

**核心价值**：
- 🗂️ **文件系统选型**：生产环境文件系统对比分析和选型建议
- ⚡ **性能优化**：I/O 性能调优、缓存策略、挂载参数优化
- 🛡️ **数据保护**：备份策略、快照技术、灾难恢复方案
- 🔧 **运维实践**：自动化管理脚本、监控告警配置、故障诊断
- 📊 **容量规划**：存储容量预测、扩容策略、成本优化

---

<!-- chunk: 目录 -->## 目录

- [VFS 虚拟文件系统](#vfs-虚拟文件系统)
- [文件系统类型](#文件系统类型)
- [磁盘分区与挂载](#磁盘分区与挂载)
- [文件权限与 ACL](#文件权限与-acl)
- [inode 与链接](#inode-与链接)
- [文件系统管理](#文件系统管理)

---

<!-- chunk: VFS 虚拟文件系统 -->## VFS 虚拟文件系统

## VFS 架构

```
┌─────────────────────────────────────────────────────────────────┐
│                         用户空间                                 │
│     应用程序: open(), read(), write(), close()                   │
└───────────────────────────┬─────────────────────────────────────┘
                            │ 系统调用
┌───────────────────────────┴─────────────────────────────────────┐
│                  VFS (Virtual File System)                       │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │  通用文件模型: superblock, inode, dentry, file         │    │
│  └─────────────────────────────────────────────────────────┘    │
│       │              │              │              │             │
│       ▼              ▼              ▼              ▼             │
│  ┌─────────┐    ┌─────────┐   ┌─────────┐   ┌─────────┐        │
│  │  ext4   │    │   xfs   │   │  btrfs  │   │ overlay │        │
│  └─────────┘    └─────────┘   └─────────┘   └─────────┘        │
└─────────────────────────────────────────────────────────────────┘
```

## VFS 核心对象

| 对象 | 说明 | 作用 |
|:---|:---|:---|
| **superblock** | 文件系统元数据 | 文件系统类型、大小、状态 |
| **inode** | 文件元数据 | 权限、大小、时间、数据块位置 |
| **dentry** | 目录项 | 文件名到 inode 映射 |
| **file** | 打开的文件 | 进程文件描述符关联 |

---

<!-- chunk: 文件系统类型 -->## 文件系统类型

## 本地文件系统对比

| 文件系统 | 最大文件 | 最大卷 | 特点 | 推荐场景 |
|:---|:---|:---|:---|:---|
| **ext4** | 16TB | 1EB | 稳定、广泛支持 | 通用场景 |
| **xfs** | 8EB | 8EB | 高性能、大文件 | 生产环境 |
| **btrfs** | 16EB | 16EB | CoW、快照、校验 | 高级功能需求 |
| **zfs** | 16EB | 256ZB | 企业级、完整性 | 需要 ZFS 特性 |

## 特殊文件系统

| 文件系统 | 说明 | 挂载点 |
|:---|:---|:---|
| **tmpfs** | 内存文件系统 | /tmp, /dev/shm |
| **proc** | 进程信息 | /proc |
| **sysfs** | 设备/驱动信息 | /sys |
| **devtmpfs** | 设备节点 | /dev |
| **cgroup** | 控制组 | /sys/fs/cgroup |

---

<!-- chunk: 磁盘分区与挂载 -->## 磁盘分区与挂载

## 分区工具

```bash
# fdisk (MBR)
fdisk /dev/sdb

# gdisk (GPT)
gdisk /dev/sdb

# parted (通用)
parted /dev/sdb

# 查看分区
lsblk
fdisk -l
```

## 创建文件系统

```bash
# ext4
mkfs.ext4 /dev/sdb1

# xfs
mkfs.xfs /dev/sdb1

# 带标签
mkfs.ext4 -L data /dev/sdb1

# 查看文件系统
blkid
```

## 挂载管理

```bash
# 临时挂载
mount /dev/sdb1 /mnt/data
mount -t xfs /dev/sdb1 /mnt/data
mount -o rw,noatime /dev/sdb1 /mnt/data

# 卸载
umount /mnt/data

# 查看挂载
mount | grep sdb
df -Th
```

## /etc/fstab 配置

```bash
# /etc/fstab
# <device>       <mountpoint>  <type>  <options>      <dump> <pass>
/dev/sdb1        /data         xfs     defaults,noatime  0      2
UUID=xxx-xxx     /backup       ext4    defaults          0      2
LABEL=data       /mnt/data     xfs     defaults          0      2
```

## 常用挂载选项

| 选项 | 说明 |
|:---|:---|
| `defaults` | 默认选项 (rw,suid,dev,exec,auto,nouser,async) |
| `noatime` | 不更新访问时间 (性能优化) |
| `nodiratime` | 不更新目录访问时间 |
| `noexec` | 禁止执行 |
| `nosuid` | 忽略 SUID |
| `ro` | 只读 |
| `rw` | 读写 |

---

<!-- chunk: 文件权限与 ACL -->## 文件权限与 ACL

## 基本权限

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `chmod/chown -R`：递归改权限，误操作破坏系统文件访问

```bash
# 查看权限
ls -la file

# 修改权限
chmod 755 file
chmod u+x file
chmod go-w file

# 修改所有者
chown user:group file
chown -R user:group dir/
```

## 权限位

| 权限 | 数值 | 文件 | 目录 |
|:---:|:---:|:---|:---|
| r | 4 | 读取内容 | 列出内容 |
| w | 2 | 修改内容 | 创建/删除文件 |
| x | 1 | 执行 | 进入目录 |

## 特殊权限

| 权限 | 数值 | 位置 | 说明 |
|:---|:---:|:---|:---|
| SUID | 4000 | 用户x -> s | 以文件所有者执行 |
| SGID | 2000 | 组x -> s | 以文件所属组执行 |
| Sticky | 1000 | 其他x -> t | 仅所有者可删除 |

## ACL 扩展权限

```bash
# 查看 ACL
getfacl file

# 设置 ACL
setfacl -m u:username:rwx file
setfacl -m g:groupname:rx file
setfacl -m d:u:username:rwx dir/  # 默认 ACL

# 删除 ACL
setfacl -x u:username file
setfacl -b file  # 删除所有
```

---

<!-- chunk: inode 与链接 -->## inode 与链接

## inode 结构

| 内容 | 说明 |
|:---|:---|
| 文件类型 | 普通文件、目录、链接等 |
| 权限 | rwxrwxrwx |
| 所有者 | UID, GID |
| 大小 | 字节数 |
| 时间戳 | atime, mtime, ctime |
| 数据块指针 | 直接/间接块 |

## 查看 inode

```bash
# 查看 inode 信息
stat file
ls -i file

# inode 使用情况
df -i
```

## 硬链接 vs 软链接

| 特性 | 硬链接 | 软链接 |
|:---|:---|:---|
| inode | 相同 | 不同 |
| 跨文件系统 | 不可 | 可以 |
| 链接目录 | 不可 | 可以 |
| 源删除影响 | 无影响 | 失效 |

```bash
# 硬链接
ln source link

# 软链接
ln -s source link
```

---

<!-- chunk: 文件系统管理 -->## 文件系统管理

## 扩展文件系统

```bash
# ext4
resize2fs /dev/sdb1

# xfs (只能扩展)
xfs_growfs /mnt/data
```

## 检查修复

```bash
# 检查
fsck /dev/sdb1         # 通用
e2fsck /dev/sdb1       # ext 系列
xfs_repair /dev/sdb1   # xfs

# 注意：必须先卸载
```

## 磁盘配额

```bash
# 启用配额
mount -o usrquota,grpquota /dev/sdb1 /data

# 创建配额文件
quotacheck -cug /data
quotaon /data

# 设置配额
edquota -u username
setquota -u username 1000000 1500000 0 0 /data

# 查看配额
quota -u username
repquota /data
```

## 常用命令

```bash
# 磁盘使用
df -Th
du -sh *

# 块设备
lsblk
blkid

# 文件类型
file filename

# 查找
find /path -name "*.log" -size +100M
locate filename
```

---

<!-- chunk: 生产环境文件系统选型指南 -->## 生产环境文件系统选型指南

## 企业级文件系统对比

| 文件系统 | 最佳场景 | 性能特点 | 可靠性 | 运维复杂度 |
|:---|:---|:---|:---|:---|
| **ext4** | 通用服务器、数据库 | 稳定可靠，兼容性好 | 高 | 低 |
| **xfs** | 大文件、日志系统 | 高吞吐，扩展性好 | 高 | 中 |
| **btrfs** | 虚拟化、容器 | 快照、校验、压缩 | 中 | 高 |
| **zfs** | NAS、备份存储 | 完整性校验、快照 | 高 | 高 |

## 生产环境挂载参数优化

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 数据库存储优化
/dev/sdb1 /data/mysql xfs defaults,noatime,nobarrier,logbufs=8,logbsize=256k 0 2

# 日志存储优化  
/dev/sdc1 /var/log ext4 defaults,noatime,data=ordered,commit=30 0 2

# 临时存储优化
tmpfs /tmp tmpfs defaults,size=2G,mode=1777 0 0

# 容器存储优化
/dev/sdd1 /var/lib/docker xfs defaults,noatime,nobarrier,inode64 0 2
```
## 文件系统性能监控脚本

```bash
#!/bin/bash
# 文件系统性能监控脚本 - fs_performance_monitor.sh

LOG_DIR="/var/log/storage"
DATE=$(date +%Y%m%d)
HOSTNAME=$(hostname)

# 创建日志目录
mkdir -p $LOG_DIR

# I/O 性能监控
monitor_io_performance() {
    echo "=== I/O 性能监控 - $(date) ===" >> $LOG_DIR/fs_perf_$DATE.log
    
    # iostat 统计
    iostat -x 1 5 >> $LOG_DIR/fs_perf_$DATE.log
    
    # 文件系统使用情况
    echo "文件系统使用情况:" >> $LOG_DIR/fs_perf_$DATE.log
    df -h >> $LOG_DIR/fs_perf_$DATE.log
    
    # inode 使用情况
    echo "inode 使用情况:" >> $LOG_DIR/fs_perf_$DATE.log
    df -i >> $LOG_DIR/fs_perf_$DATE.log
    
    # I/O 等待进程
    echo "I/O 等待进程:" >> $LOG_DIR/fs_perf_$DATE.log
    iotop -bo 1 >> $LOG_DIR/fs_perf_$DATE.log 2>/dev/null || ps aux | awk '$8=="D"' >> $LOG_DIR/fs_perf_$DATE.log
}

# 文件系统健康检查
check_filesystem_health() {
    echo "=== 文件系统健康检查 - $(date) ===" >> $LOG_DIR/fs_health_$DATE.log
    
    # 检查只读文件系统
    mount | grep "ro," >> $LOG_DIR/fs_health_$DATE.log
    
    # 检查文件系统错误
    dmesg | grep -i "filesystem|error|corruption" >> $LOG_DIR/fs_health_$DATE.log
    
    # 检查磁盘空间预警
    df -h | awk '$5+0 > 85 {print "警告: "$6" 使用率超过85%: "$5}' >> $LOG_DIR/fs_health_$DATE.log
    
    # 检查 inode 使用率
    df -i | awk '$5+0 > 90 {print "警告: "$6" inode 使用率超过90%: "$5}' >> $LOG_DIR/fs_health_$DATE.log
}

# 自动清理脚本
auto_cleanup() {
    # 清理旧日志文件
    find $LOG_DIR -name "fs_*.log" -mtime +30 -delete
    
    # 清理临时文件
    find /tmp -type f -atime +7 -delete 2>/dev/null
    
    # 清理系统日志
    journalctl --vacuum-time=30d
}

# 根据参数执行相应功能
case "$1" in
    "monitor")
        monitor_io_performance
        ;;
    "health")
        check_filesystem_health
        ;;
    "cleanup")
        auto_cleanup
        ;;
    "all")
        monitor_io_performance
        check_filesystem_health
        auto_cleanup
        ;;
    *)
        echo "用法: $0 {monitor|health|cleanup|all}"
        echo "  monitor - I/O性能监控"
        echo "  health  - 文件系统健康检查"
        echo "  cleanup - 自动清理"
        echo "  all     - 执行所有检查"
        exit 1
        ;;
esac
```

<!-- chunk: 数据保护与备份策略 -->## 数据保护与备份策略

## 快照管理脚本

```bash
#!/bin/bash
# LVM 快照管理脚本 - lvm_snapshot_manager.sh

VOLUME_GROUP="vg_data"
LOGICAL_VOLUME="lv_data"
SNAPSHOT_SIZE="10G"
RETENTION_DAYS=7

create_snapshot() {
    local timestamp=$(date +%Y%m%d_%H%M%S)
    local snap_name="${LOGICAL_VOLUME}_snap_${timestamp}"
    
    echo "创建快照: $snap_name"
    
    # 创建快照
    lvcreate -L $SNAPSHOT_SIZE -s -n $snap_name /dev/$VOLUME_GROUP/$LOGICAL_VOLUME
    
    if [ $? -eq 0 ]; then
        echo "快照创建成功: /dev/$VOLUME_GROUP/$snap_name"
        
        # 记录快照信息
        echo "$(date): 创建快照 $snap_name" >> /var/log/lvm_snapshots.log
    else
        echo "快照创建失败"
        exit 1
    fi
}

remove_old_snapshots() {
    echo "清理过期快照..."
    
    # 获取过期快照列表
    lvs --noheadings -o lv_name,lv_attr | grep snap | while read line; do
        snap_name=$(echo $line | awk '{print $1}')
        creation_date=$(lvs --noheadings -o lv_creation_time /dev/$VOLUME_GROUP/$snap_name | tr -d ' ')
        
        # 计算年龄（简化处理）
        if [ $(date -d "$creation_date" +%s) -lt $(date -d "$RETENTION_DAYS days ago" +%s) ]; then
            echo "删除过期快照: $snap_name"
            lvremove -f /dev/$VOLUME_GROUP/$snap_name
        fi
    done
}

# 恢复数据
restore_from_snapshot() {
    local snap_name=$1
    local restore_point="/mnt/restore_$(date +%Y%m%d_%H%M%S)"
    
    if [ -z "$snap_name" ]; then
        echo "请指定要恢复的快照名称"
        echo "可用快照:"
        lvs --noheadings -o lv_name | grep snap
        exit 1
    fi
    
    # 挂载快照
    mkdir -p $restore_point
    mount /dev/$VOLUME_GROUP/$snap_name $restore_point
    
    if [ $? -eq 0 ]; then
        echo "快照已挂载到: $restore_point"
        echo "请手动复制需要恢复的数据"
        echo "恢复完成后执行: umount $restore_point"
    else
        echo "快照挂载失败"
        exit 1
    fi
}

# 主菜单
case "$1" in
    "create")
        create_snapshot
        ;;
    "cleanup")
        remove_old_snapshots
        ;;
    "restore")
        restore_from_snapshot "$2"
        ;;
    "list")
        echo "当前快照列表:"
        lvs --noheadings -o lv_name,lv_size,lv_creation_time | grep snap
        ;;
    *)
        echo "用法: $0 {create|cleanup|restore|list}"
        echo "  create  - 创建新快照"
        echo "  cleanup - 清理过期快照"
        echo "  restore - 从快照恢复 (需要指定快照名)"
        echo "  list    - 列出所有快照"
        exit 1
        ;;
esac
```

<!-- chunk: 容量规划与预测 -->## 容量规划与预测

## 存储容量预测模型

```bash
#!/bin/bash
# 存储容量预测脚本 - capacity_forecast.sh

DATA_DIR="/var/log/storage_trends"
REPORT_DIR="/var/reports/capacity"
DAYS_TO_PREDICT=90

# 收集历史数据
collect_storage_data() {
    local timestamp=$(date +%Y%m%d_%H%M%S)
    
    # 收集各文件系统使用情况
    df -B1G | grep -v tmpfs > $DATA_DIR/storage_$timestamp.csv
    
    # 收集 inode 使用情况
    df -i | grep -v tmpfs > $DATA_DIR/inode_$timestamp.csv
    
    # 收集 I/O 统计
    iostat -x 1 60 | tail -n +4 > $DATA_DIR/iostat_$timestamp.log
}

# 容量趋势分析
analyze_trends() {
    local fs_path=$1
    local days_history=${2:-30}
    
    echo "分析文件系统 $fs_path 的容量趋势..."
    
    # 计算每日增长率
    local current_usage=$(df -B1G $fs_path | awk 'NR==2 {print $3}')
    local usage_30days_ago=$(tail -n 30 $DATA_DIR/storage_*.csv | grep "$fs_path" | head -1 | awk '{print $3}')
    
    if [ -n "$usage_30days_ago" ] && [ "$usage_30days_ago" -gt 0 ]; then
        local growth_rate=$(echo "scale=4; ($current_usage - $usage_30days_ago) / $usage_30days_ago / 30" | bc)
        local predicted_usage=$(echo "$current_usage * (1 + $growth_rate * $DAYS_TO_PREDICT)" | bc)
        
        echo "当前使用: ${current_usage}GB"
        echo "30天增长率: $(echo "$growth_rate * 100" | bc)%/天"
        echo "90天预测使用: ${predicted_usage}GB"
        
        # 检查预警
        local total_space=$(df -B1G $fs_path | awk 'NR==2 {print $2}')
        local predicted_percentage=$(echo "$predicted_usage * 100 / $total_space" | bc)
        
        if [ "$predicted_percentage" -gt 85 ]; then
            echo "⚠️  警告: 预测使用率将超过85% (${predicted_percentage}%)"
            echo "建议: 考虑扩容或清理数据"
        fi
    else
        echo "历史数据不足，无法进行趋势分析"
    fi
}

# 生成容量报告
generate_report() {
    local report_file="$REPORT_DIR/capacity_report_$(date +%Y%m%d).html"
    
    mkdir -p $REPORT_DIR
    
    cat > $report_file << EOF
<!DOCTYPE html>
<html>
<head>
    <title>存储容量分析报告 - $(date)</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        table { border-collapse: collapse; width: 100%; }
        th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }
        th { background-color: #f2f2f2; }
        .warning { color: red; font-weight: bold; }
        .ok { color: green; }
    </style>
</head>
<body>
    <h1>存储容量分析报告</h1>
    <p>生成时间: $(date)</p>
    <p>主机名: $(hostname)</p>
    
    <h2>当前存储使用情况</h2>
    <table>
        <tr>
            <th>挂载点</th>
            <th>总容量</th>
            <th>已使用</th>
            <th>可用</th>
            <th>使用率</th>
            <th>状态</th>
        </tr>
EOF

    df -h | grep -v tmpfs | tail -n +2 | while read line; do
        mount_point=$(echo $line | awk '{print $6}')
        total=$(echo $line | awk '{print $2}')
        used=$(echo $line | awk '{print $3}')
        available=$(echo $line | awk '{print $4}')
        usage_percent=$(echo $line | awk '{print $5}' | tr -d '%')
        
        if [ "$usage_percent" -gt 85 ]; then
            status="<span class='warning'>⚠️ 高使用率</span>"
        elif [ "$usage_percent" -gt 70 ]; then
            status="<span class='warning'>注意</span>"
        else
            status="<span class='ok'>正常</span>"
        fi
        
        echo "        <tr>" >> $report_file
        echo "            <td>$mount_point</td>" >> $report_file
        echo "            <td>$total</td>" >> $report_file
        echo "            <td>$used</td>" >> $report_file
        echo "            <td>$available</td>" >> $report_file
        echo "            <td>${usage_percent}%</td>" >> $report_file
        echo "            <td>$status</td>" >> $report_file
        echo "        </tr>" >> $report_file
    done
    
    cat >> $report_file << EOF
    </table>
    
    <h2>容量规划建议</h2>
    <ul>
        <li>定期监控存储使用趋势</li>
        <li>实施数据生命周期管理策略</li>
        <li>考虑使用压缩和去重技术</li>
        <li>制定应急预案和扩容计划</li>
    </ul>
</body>
</html>
EOF

    echo "容量报告已生成: $report_file"
}

# 主程序
case "$1" in
    "collect")
        collect_storage_data
        ;;
    "analyze")
        analyze_trends "$2" "$3"
        ;;
    "report")
        generate_report
        ;;
    "all")
        collect_storage_data
        generate_report
        ;;
    *)
        echo "用法: $0 {collect|analyze|report|all}"
        echo "  collect - 收集存储数据"
        echo "  analyze - 分析趋势 (需要指定挂载点和历史天数)"
        echo "  report  - 生成容量报告"
        echo "  all     - 执行完整分析"
        exit 1
        ;;
esac
```

---

<!-- chunk: 与 [[Kubernetes|Kubernetes]] 的关系 -->## 与 Kubernetes 的关系

## 容器存储 (Container Storage)

Kubernetes 的容器存储建立在 Linux 文件系统之上，理解底层原理对于排查存储问题至关重要。

```
┌─────────────────────────────────────────────────────────────────┐
│                    K8s 容器存储架构                               │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │                    Pod                                    │  │
│  │  ┌──────────────┐  ┌──────────────┐                     │  │
│  │  │  Container 1  │  │  Container 2  │                     │  │
│  │  │  /app/data ←─┼──┼─→ /shared     │  ← Volume Mount    │  │
│  │  └──────────────┘  └──────────────┘                     │  │
│  └──────────────────────────────────────────────────────────┘  │
│                           │                                      │
│  ┌────────────────────────┴─────────────────────────────────┐  │
│  │                  Volume 抽象层                             │  │
│  │  emptyDir │ hostPath │ PVC │ ConfigMap │ Secret            │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │                                      │
│  ┌────────────────────────┴─────────────────────────────────┐  │
│  │                  PV / StorageClass                         │  │
│  │  NFS │ iSCSI │ Ceph RBD │ Local PV │ Cloud EBS             │  │
│  └────────────────────────┬─────────────────────────────────┘  │
│                           │                                      │
│  ┌────────────────────────┴─────────────────────────────────┐  │
│  │              Linux 存储栈                                  │  │
│  │  OverlayFS (容器层) → ext4/xfs (主机文件系统) → 块设备     │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

## OverlayFS 与容器存储

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Kubernetes 使用的 overlay 挂载
mount | grep overlay

# containerd 的 overlay 目录
/var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/

# 查看 overlay 层数
docker inspect <image> --format '{{len .RootFS.Layers}}'

# overlay2 的磁盘使用
du -sh /var/lib/docker/overlay2/
docker system df -v
```
## inode 耗尽问题

inode 耗尽是 Kubernetes 集群中常见的存储问题，特别是日志量大的场景：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 inode 使用
df -i

# inode 耗尽的症状:
# - "No space left on device" 但 df -h 显示有空间
# - 无法创建新文件
# - Pod 无法启动

# 常见原因和解决方案:
# 1. 容器日志文件过多
find /var/log/containers/ -type f | wc -l
# 解决: 配置日志轮转
cat > /etc/docker/daemon.json << 'EOF'
{
  "log-driver": "json-file",
  "log-opts": {
    "max-size": "10m",
    "max-file": "3"
  }
}
EOF

# 2. 已删除但被进程占用的文件
lsof +L1 | grep deleted
# 解决: 重启占用文件的进程

# 3. 小文件过多 (大量 ConfigMap/Secret 挂载)
find /var/lib/kubelet/pods/ -type f | wc -l
```
---

<!-- chunk: 性能调优 -->## 性能调优

## 文件系统性能优化

```bash
# 挂载参数优化
# 数据库
mount -o noatime,nodiratime,data=ordered,barrier=1 /dev/sdb1 /data

# 容器存储
mount -o noatime,nodiratime,logbufs=8,logbsize=256k /dev/sdb1 /var/lib/containerd

# 日志存储
mount -o noatime,nodiratime,data=writeback,commit=30 /dev/sdc1 /var/log

# 调整文件系统预留空间
tune2fs -m 1 /dev/sdb1              # ext4: 预留 1%（默认 5%）

# 调整文件系统检查间隔
tune2fs -i 0 -c 0 /dev/sdb1         # 禁用定期检查

# 查看文件系统参数
tune2fs -l /dev/sdb1                 # ext4 详细参数
xfs_info /data                       # XFS 参数
```

---

<!-- chunk: 安全加固 -->## 安全加固

## 文件系统安全

```bash
# 查找 SUID 文件 (安全审计)
find / -perm -4000 -type f -exec ls -la {} \; 2>/dev/null

# 查找可写文件 (安全审计)
find / -type f -perm -0002 ! -path "/proc/*" ! -path "/sys/*" 2>/dev/null

# 设置不可变属性 (保护关键文件)
chattr +i /etc/resolv.conf           # 防止修改
lsattr /etc/resolv.conf              # 查看属性
chattr -i /etc/resolv.conf           # 取消不可变

# 加密文件系统 (LUKS)
cryptsetup luksFormat /dev/sdb1
cryptsetup luksOpen /dev/sdb1 encrypted
mkfs.ext4 /dev/mapper/encrypted
mount /dev/mapper/encrypted /secure
```

---

<!-- chunk: 最佳实践 -->## 最佳实践

1. **XFS 用于容器存储**: XFS 处理大文件和并发 I/O 性能更优
2. **ext4 用于通用场景**: ext4 稳定性和兼容性最好
3. **noatime 挂载**: 减少不必要的磁盘写入
4. **监控 inode 使用**: 防止 inode 耗尽导致的问题
5. **预留 10% 空间**: XFS 在 95% 满时性能严重下降
6. **日志轮转**: 配置 logrotate 防止日志撑满磁盘
7. **使用 Local PV**: 对 I/O 敏感的数据库使用本地持久卷

---

<!-- chunk: 故障排查 -->## 故障排查

## 文件系统常见问题

```bash
# 文件系统变为只读
# 原因: 磁盘问题、文件系统损坏
dmesg | grep -i "remounting|read-only|error"
# 修复: fsck (ext4) 或 xfs_repair
umount /data
fsck -y /dev/sdb1
# 或
xfs_repair /dev/sdb1

# 磁盘空间不足但 du 显示正常
# 原因: 已删除文件被进程占用
lsof +L1 | grep deleted
lsof +L1 | awk '{sum+=$7} END {print sum/1024/1024 "MB"}'
# 解决: 重启占用进程或 truncate
> /proc/<pid>/fd/<fd>

# inode 耗尽
df -i
# 查找大量小文件
find /path -type f | awk -F/ '{print NF-1, $0}' | sort -n | tail -20
```

---

<!-- chunk: 相关文档 -->## 相关文档

- [01-linux-system-architecture](./01-linux-system-architecture.md) - 系统架构
- [05-linux-storage-management](./05-linux-storage-management.md) - 存储管理
- [06-linux-performance-tuning](./06-linux-performance-tuning.md) - 性能调优

---

## 生产环境文件系统运维脚本

```bash
#!/bin/bash
# fs-health-check.sh - 文件系统健康巡检

echo "=== 文件系统健康巡检 $(date) ==="
echo "主机: $(hostname)"

echo -e "\n[1] 磁盘空间使用 (>85% 警告)"
df -h --type=ext4 --type=xfs --type=btrfs 2>/dev/null | \
  awk 'NR==1 || +$5 > 85 {print "  "$0}'

echo -e "\n[2] inode 使用 (>90% 警告)"
df -i --type=ext4 --type=xfs 2>/dev/null | \
  awk 'NR==1 || +$5 > 90 {print "  "$0}'

echo -e "\n[3] 只读文件系统检测"
mount | grep " ro," && echo "  警告: 发现只读文件系统!" || echo "  正常"

echo -e "\n[4] 内核文件系统错误"
dmesg | grep -i -E "ext4.*error|xfs.*error|I/O error" | tail -5 || echo "  无错误"

echo -e "\n[5] 大文件 TOP 10"
find / -type f -size +100M -exec ls -lh {} \; 2>/dev/null | \
  sort -k5 -hr | head -10

echo -e "\n[6] 磁盘 I/O 等待"
iostat -xz 1 1 2>/dev/null | tail -n +4 | \
  awk '$12+0 > 10 {print "  警告: "$1" await="$12"%"}'

echo "=== 巡检完成 ==="
```

---

**维护者**: Allen Galler (allengaller@gmail.com) | **许可证**: MIT

## See Also

- 01-linux-system-architecture
- 02-linux-process-management
- 04-linux-networking-configuration
- 05-linux-storage-management

## Related

- index/etcd-index|etcd 知识图谱索引]]


<!-- risk-assessed -->
