# 218 - Linux 运维基础补充：系统监控、性能调优与故障排查

## 系统监控基础

### 常用监控命令

| 命令 | 用途 | 说明 |
|------|------|------|
| `top` | 实时进程监控 | 显示CPU、内存使用情况及活跃进程 |
| `htop` | 增强版进程监控 | 更友好的界面，支持交互操作 |
| `vmstat` | 虚拟内存统计 | 报告进程、内存、I/O、CPU活动 |
| `iostat` | I/O统计 | 报告CPU使用率和磁盘I/O统计 |
| `sar` | 系统活动报告 | 收集并报告系统活动信息 |
| `free` | 内存使用情况 | 显示空闲和已用内存 |
| `df -h` | 磁盘使用情况 | 报告文件系统磁盘空间使用情况 |
| `du -sh` | 目录大小统计 | 估算文件空间使用情况 |

### 系统性能指标

| 指标 | 正常范围 | 警告范围 | 危险范围 |
|------|----------|----------|----------|
| CPU 使用率 | < 70% | 70%-85% | > 85% |
| 内存使用率 | < 80% | 80%-90% | > 90% |
| 磁盘使用率 | < 85% | 85%-95% | > 95% |
| 系统负载(load avg) | < CPU核数 | CPU核数-2*CPU核数 | > 2*CPU核数 |
| 网络带宽使用 | < 70% | 70%-85% | > 85% |

## 进程和服务管理

### 服务管理命令

```bash
# Systemd 服务管理
systemctl start <service>      # 启动服务
systemctl stop <service>       # 停止服务
systemctl restart <service>    # 重启服务
systemctl status <service>     # 查看服务状态
systemctl enable <service>     # 设置开机自启
systemctl disable <service>    # 禁用开机自启
systemctl list-units --type=service --state=running  # 查看运行中的服务

# 传统 SysVinit 命令
service <service> start        # 启动服务
service <service> stop         # 停止服务
chkconfig <service> on         # 设置开机自启
```

### 进程管理

```bash
# 查看进程
ps aux                    # 显示所有进程详细信息
ps -ef                    # 显示所有进程（另一种格式）
pstree                    # 显示进程树
pgrep <process_name>      # 按名称查找进程ID

# 进程控制
kill <pid>                # 终止进程
kill -9 <pid>             # 强制终止进程
kill -HUP <pid>           # 重新加载进程配置
pkill <process_name>      # 按名称终止进程
killall <process_name>    # 终止所有同名进程

# 后台作业管理
jobs                      # 列出后台作业
bg %job_number            # 将停止的作业转到后台运行
fg %job_number            # 将后台作业调至前台
nohup <command> &         # 后台运行命令，忽略挂断信号
```

## 网络运维基础

### 网络配置与诊断

```bash
# 网络接口管理
ip addr show              # 显示网络接口信息
ip link set <interface> up/down  # 启用/禁用网络接口
ifconfig <interface>      # 传统接口配置命令

# 网络连接查看
ss -tuln                  # 显示监听的端口
netstat -tuln             # 显示网络连接状态
ss -tulpn | grep <port>   # 查看特定端口占用

# 网络诊断
ping <host>               # 测试连通性
traceroute <host>         # 追踪路由路径
mtr <host>                # 结合ping和traceroute
telnet <host> <port>      # 测试端口连通性
nc -zv <host> <port>      # Netcat测试端口连通性
dig <domain>              # DNS查询
nslookup <domain>         # DNS查询
```

### 防火墙管理

```bash
# iptables 基础命令
iptables -L              # 列出规则
iptables -A INPUT -p tcp --dport <port> -j ACCEPT  # 允许端口
iptables -D INPUT -p tcp --dport <port> -j ACCEPT  # 删除规则
iptables -F              # 清空规则
service iptables save    # 保存规则

# firewalld 命令 (CentOS/RHEL 7+)
firewall-cmd --list-all               # 查看当前配置
firewall-cmd --permanent --add-port=<port>/tcp  # 添加端口
firewall-cmd --reload                # 重载配置
firewall-cmd --zone=public --list-ports  # 查看开放端口

# ufw 命令 (Ubuntu)
ufw status               # 查看防火墙状态
ufw allow <port>         # 允许端口
ufw deny <port>          # 拒绝端口
ufw enable/disable       # 启用/禁用防火墙
```

## 存储和文件系统运维

### 文件系统管理

```bash
# 磁盘分区管理
fdisk -l                 # 列出磁盘分区
parted /dev/sdX print    # 查看分区表
mkfs -t ext4 /dev/sdX1   # 创建文件系统
mount /dev/sdX1 /mnt     # 挂载分区
umount /dev/sdX1         # 卸载分区

# LVM 管理
pvcreate /dev/sdX        # 创建物理卷
vgcreate vg_name /dev/sdX  # 创建卷组
lvcreate -L 10G -n lv_name vg_name  # 创建逻辑卷
mkfs -t ext4 /dev/vg_name/lv_name  # 在逻辑卷上创建文件系统
resize2fs /dev/vg_name/lv_name     # 扩展ext4文件系统
lvextend -L +5G /dev/vg_name/lv_name  # 扩展逻辑卷

# 挂载选项
mount -o remount,rw /    # 重新挂载为读写
mount -o ro /dev/sdX1 /mnt  # 只读挂载
mount -a                 # 挂载fstab中的所有文件系统
```

### 存储性能优化

| 优化项 | 参数 | 说明 |
|--------|------|------|
| 磁盘调度算法 | `deadline`, `noop`, `cfq` | deadline适合数据库，noop适合SSD/虚拟机 |
| 文件系统 | `ext4`, `xfs`, `btrfs` | xfs适合大文件，ext4通用 |
| 挂载选项 | `noatime`, `relatime` | 减少磁盘I/O，提升性能 |
| I/O调度 | `nr_requests`, `read_ahead_kb` | 调整队列深度和预读大小 |

## 日志管理

### 系统日志

```bash
# 传统日志位置
/var/log/messages         # 系统消息 (RedHat/CentOS)
/var/log/syslog           # 系统日志 (Ubuntu/Debian)
/var/log/auth.log         # 认证日志
/var/log/kern.log         # 内核日志
/var/log/boot.log         # 启动日志

# journalctl (systemd系统)
journalctl                # 查看所有日志
journalctl -u <service>   # 查看服务日志
journalctl -f             # 实时跟踪日志
journalctl --since "2023-01-01" --until "2023-01-02"  # 时间范围
journalctl -n 50          # 显示最近50行
journalctl -b             # 仅显示本次启动日志
```

### 日志轮转(logrotate)

```bash
# logrotate 配置示例 (/etc/logrotate.d/myapp)
/path/to/app.log {
    daily                   # 每天轮转
    rotate 30               # 保留30个归档
    compress                # 压缩归档
    delaycompress           # 延迟压缩
    copytruncate            # 截断原文件
    missingok               # 文件不存在不报错
    notifempty              # 空文件不轮转
    postrotate              # 轮转后执行
        systemctl reload rsyslog > /dev/null 2>&1 || true
    endscript
}
```

## 安全运维基础

### 用户和权限管理

```bash
# 用户管理
useradd -m -s /bin/bash username  # 创建用户
userdel -r username        # 删除用户及其家目录
passwd username            # 修改密码
usermod -aG groupname username   # 添加用户到组
id username                # 显示用户ID和组信息

# 权限管理
chmod 755 filename         # 修改文件权限
chown user:group filename  # 修改文件属主
chgrp groupname filename   # 修改文件组
umask 022                  # 设置默认权限掩码

# 特殊权限
chmod u+s file             # 设置SUID
chmod g+s file             # 设置SGID
chmod o+t file             # 设置Sticky Bit
```

### SSH 安全配置

```bash
# /etc/ssh/sshd_config 安全配置
Port 2222                 # 修改默认端口
PermitRootLogin no        # 禁止root直接登录
PasswordAuthentication no # 禁用密码认证，使用密钥
PubkeyAuthentication yes  # 启用公钥认证
MaxAuthTries 3            # 最大认证尝试次数
ClientAliveInterval 300   # 客户端存活间隔
ClientAliveCountMax 2     # 客户端最大无响应次数
AllowUsers user1 user2    # 允许特定用户
DenyUsers user3          # 拒绝特定用户
AllowGroups sshusers      # 允许特定组
```

## 故障排查基础

### 系统故障排查流程

1. **初步评估**
   - 检查系统整体状态
   - 确认问题影响范围
   - 收集基本信息

2. **信息收集**
   ```bash
   uptime                  # 系统运行时间和负载
   whoami && id            # 当前用户信息
   hostname                # 主机名
   date && timedatectl     # 系统时间
   dmesg | tail -50        # 最近内核消息
   ```

3. **资源瓶颈分析**
   - CPU：`top`, `vmstat`, `sar -u`
   - 内存：`free`, `vmstat`, `sar -r`
   - 磁盘：`iostat`, `df`, `du`
   - 网络：`ss`, `netstat`, `iftop`

4. **服务故障排查**
   ```bash
   # 检查服务状态
   systemctl status <service>
   journalctl -u <service> -f
   ps aux | grep <service>
   
   # 检查端口占用
   ss -tulnp | grep <port>
   lsof -i :<port>
   ```

### 常见故障诊断命令

```bash
# 内存泄漏检测
pmap <pid>                # 进程内存映射
cat /proc/<pid>/status    # 进程状态信息
cat /proc/meminfo         # 内存信息
slabtop                   # 内核slab分配器统计

# 磁盘I/O问题
iotop                     # 实时I/O监控
iostat -x 1               # 详细I/O统计
lsof +D /path             # 列出目录下打开的文件

# 网络问题
tcpdump -i eth0 host <ip> # 抓包分析
ss -s                     # 概述套接字使用情况
cat /proc/net/dev         # 网络接口统计

# 进程问题
strace -p <pid>           # 跟踪系统调用
ltrace -p <pid>           # 跟踪库调用
kill -USR1 <pid>          # 请求进程输出统计信息
```

## 备份与恢复

### 备份策略

| 备份类型 | 命令示例 | 说明 |
|----------|----------|------|
| 完全备份 | `tar -czf backup.tar.gz /data` | 备份整个目录 |
| 增量备份 | `tar -g snapshot_file -czf inc_backup.tar.gz /data` | 基于快照的增量备份 |
| 差异备份 | `find /data -newer last_full_backup -print | tar -czf diff_backup.tar.gz -T -` | 自上次完整备份以来的更改 |
| 数据库备份 | `mysqldump -u user -p db_name > backup.sql` | MySQL数据库备份 |

### rsync 同步

```bash
# 基本同步命令
rsync -avz /source/ /destination/     # 基本同步
rsync -avz --delete /source/ /destination/  # 同步删除
rsync -avz --exclude='*.tmp' /source/ /destination/  # 排除特定文件
rsync -avz -e ssh /source/ user@remote:/destination/  # 远程同步
rsync --dry-run -avz /source/ /destination/  # 预览操作
```

## 自动化运维脚本

### 监控脚本示例

```bash
#!/bin/bash
# 系统健康检查脚本
check_disk_usage() {
    usage=$(df / | awk 'NR==2 {print $5}' | sed 's/%//')
    if [ $usage -gt 80 ]; then
        echo "警告: 磁盘使用率 ${usage}%"
        return 1
    fi
    return 0
}

check_memory_usage() {
    usage=$(free | awk 'NR==2{printf "%.0f", $3*100/$2}')
    if [ $usage -gt 85 ]; then
        echo "警告: 内存使用率 ${usage}%"
        return 1
    fi
    return 0
}

check_services() {
    services=("nginx" "mysql" "redis")
    for service in "${services[@]}"; do
        if ! systemctl is-active --quiet "$service"; then
            echo "警告: 服务 $service 未运行"
            return 1
        fi
    done
    return 0
}

# 执行检查
check_disk_usage && check_memory_usage && check_services
```

### 定期任务(cron)

```bash
# 编辑当前用户的cron任务
crontab -e

# cron格式: 分 时 日 月 周 命令
# 每天凌晨2点执行备份
0 2 * * * /usr/local/bin/backup_script.sh

# 每小时检查系统状态
0 * * * * /usr/local/bin/system_check.sh

# 每周一清理日志
0 3 * * 1 /usr/local/bin/cleanup_logs.sh

# 每5分钟运行监控脚本
*/5 * * * * /usr/local/bin/monitor.sh
```

## 总结

Linux运维是Kubernetes运维的重要基础，掌握这些基础技能对于有效管理和维护Kubernetes集群至关重要。运维人员应熟练使用这些工具和命令，并结合实际场景进行应用。

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)