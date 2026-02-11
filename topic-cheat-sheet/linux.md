# Linux 生产环境速查卡

> **适用系统**: RHEL/CentOS 7-9, Ubuntu 20.04-24.04, Debian 11-12 | **最后更新**: 2026-02  
> **目标**: 涵盖生产环境 90% 以上常用命令，支持系统运维和故障排查

---

## 📋 目录

- [系统信息查询](#系统信息查询)
- [文件与目录操作](#文件与目录操作)
- [文本处理](#文本处理)
- [进程管理](#进程管理)
- [网络管理](#网络管理)
- [磁盘与存储](#磁盘与存储)
- [用户与权限](#用户与权限)
- [系统服务管理](#系统服务管理)
- [性能监控](#性能监控)
- [日志分析](#日志分析)
- [安全与防火墙](#安全与防火墙)
- [包管理](#包管理)
- [Shell 脚本](#shell-脚本)
- [容器与虚拟化](#容器与虚拟化)
- [故障排查](#故障排查)

---

## 系统信息查询

### 系统版本

```bash
# 查看发行版信息
cat /etc/os-release
lsb_release -a  # Ubuntu/Debian

# 查看内核版本
uname -r
uname -a  # 完整信息

# 查看系统架构
arch
uname -m  # x86_64, aarch64

# 查看主机名
hostname
hostnamectl  # systemd 系统 (RHEL 7+, Ubuntu 16.04+)

# 查看启动时间
uptime
who -b
systemctl status | grep "since"  # systemd
```

**版本兼容性**:
- `hostnamectl`: systemd 系统 (RHEL/CentOS 7+, Ubuntu 16.04+, Debian 8+)
- `lsb_release`: 需要安装 `lsb-release` 包

### 硬件信息

```bash
# CPU 信息
lscpu
cat /proc/cpuinfo
nproc  # CPU 核心数

# 内存信息
free -h  # 人类可读格式
cat /proc/meminfo
dmidecode -t memory  # 需要 root

# 磁盘信息
lsblk  # 块设备列表
fdisk -l  # 分区表 (需要 root)
df -h  # 磁盘使用情况
du -sh <directory>  # 目录大小

# 硬件详细信息
dmidecode -t system  # 系统信息
dmidecode -t bios    # BIOS 信息
dmidecode -t processor  # CPU 详情

# PCI 设备
lspci
lspci -v  # 详细信息
lspci | grep -i vga  # 显卡
lspci | grep -i eth  # 网卡

# USB 设备
lsusb
lsusb -v
```

### 系统负载与资源

```bash
# 系统负载 (1/5/15 分钟平均)
uptime
cat /proc/loadavg

# 实时监控 (适用所有发行版)
top
htop  # 更友好 (需要安装)

# CPU 使用率
mpstat 1 5  # 每秒刷新，5 次 (sysstat 包)
sar -u 1 5  # CPU 使用历史 (sysstat 包)

# 内存使用
free -h
vmstat 1 5  # 虚拟内存统计

# 磁盘 I/O
iostat -x 1 5  # 详细 I/O 统计 (sysstat 包)
iotop  # 实时 I/O 监控 (需要 root)

# 网络流量
iftop -i eth0  # 实时流量监控 (需要安装)
nload  # 图形化流量监控 (需要安装)
```

**工具包版本**:
- **sysstat** (mpstat, iostat, sar): v12.5+ (Ubuntu 22.04+, RHEL 9+)
- **htop**: v3.2+ (Ubuntu 22.04+, RHEL 9+)
- **iotop**: v0.6+ (所有发行版)

---

## 文件与目录操作

### 基础操作

```bash
# 列出文件
ls -l  # 长格式
ls -lh  # 人类可读大小
ls -a  # 包含隐藏文件
ls -lha  # 组合
ls -lt  # 按修改时间排序
ls -lS  # 按大小排序

# 改变目录
cd /path/to/dir
cd ~  # 家目录
cd -  # 上一个目录

# 创建目录
mkdir <dir>
mkdir -p /path/to/nested/dir  # 递归创建

# 删除
rm <file>
rm -r <dir>  # 递归删除目录
rm -rf <dir>  # 强制递归删除 (危险!)
rmdir <empty-dir>  # 删除空目录

# 复制
cp <src> <dst>
cp -r <src-dir> <dst-dir>  # 递归复制
cp -a <src> <dst>  # 保留属性
cp -v <src> <dst>  # 显示过程

# 移动/重命名
mv <src> <dst>
mv <old-name> <new-name>

# 创建链接
ln -s /path/to/file /path/to/symlink  # 软链接
ln /path/to/file /path/to/hardlink    # 硬链接
```

### 文件查找

```bash
# find 命令 (强大，适用所有发行版)
find /path -name "*.log"  # 按名称查找
find /path -type f -name "*.txt"  # 按类型和名称
find /path -type d -name "logs"  # 查找目录
find /path -size +100M  # 大于 100MB
find /path -mtime -7  # 最近 7 天修改
find /path -mtime +30  # 30 天前修改
find /path -user root  # 按用户查找
find /path -perm 644  # 按权限查找

# 查找并执行操作
find /path -name "*.log" -exec rm {} \;  # 删除找到的文件
find /path -name "*.log" -exec ls -lh {} \;  # 列出详情
find /path -name "*.tmp" -delete  # 直接删除

# locate 命令 (快速，但需要更新数据库)
locate <filename>
sudo updatedb  # 更新 locate 数据库

# which 命令 (查找命令路径)
which python3
which -a python3  # 所有匹配路径

# whereis 命令 (查找二进制、源代码、手册)
whereis ls
```

### 文件内容查看

```bash
# 查看文件
cat <file>
less <file>  # 分页查看 (推荐大文件)
more <file>  # 分页查看
head <file>  # 前 10 行
head -n 20 <file>  # 前 20 行
tail <file>  # 后 10 行
tail -n 20 <file>  # 后 20 行
tail -f <file>  # 实时跟踪 (日志文件)

# 统计
wc <file>  # 行数、单词数、字节数
wc -l <file>  # 仅行数
wc -w <file>  # 仅单词数

# 文件类型
file <file>
stat <file>  # 详细状态信息
```

### 文件权限

```bash
# 修改权限
chmod 755 <file>  # rwxr-xr-x
chmod +x <script>  # 添加执行权限
chmod -R 755 <dir>  # 递归修改
chmod u+x,g+x,o+x <file>  # 符号模式

# 修改所有者
chown user:group <file>
chown -R user:group <dir>  # 递归

# 修改组
chgrp <group> <file>

# 特殊权限
chmod u+s <file>  # SUID
chmod g+s <dir>   # SGID
chmod +t <dir>    # Sticky Bit (如 /tmp)

# 查看权限
ls -l <file>
stat <file>
getfacl <file>  # 查看 ACL
```

**权限说明**:
- `755` = `rwxr-xr-x` (所有者可读写执行，组和其他只读执行)
- `644` = `rw-r--r--` (所有者可读写，组和其他只读)
- `600` = `rw-------` (仅所有者可读写)

---

## 文本处理

### grep (搜索文本)

```bash
# 基础搜索
grep "pattern" <file>
grep -i "pattern" <file>  # 忽略大小写
grep -v "pattern" <file>  # 反向匹配 (不包含)
grep -n "pattern" <file>  # 显示行号
grep -c "pattern" <file>  # 计数

# 递归搜索
grep -r "pattern" /path/  # 递归搜索目录
grep -R "pattern" /path/  # 递归 + 跟踪符号链接

# 扩展正则
grep -E "pattern1|pattern2" <file>  # 或
grep -E "^start" <file>  # 以 start 开头
grep -E "end$" <file>  # 以 end 结尾

# 上下文显示
grep -A 3 "pattern" <file>  # 显示后 3 行
grep -B 3 "pattern" <file>  # 显示前 3 行
grep -C 3 "pattern" <file>  # 显示前后 3 行

# 多文件搜索
grep "pattern" *.log
grep -l "pattern" *.log  # 仅显示文件名

# 性能优化 (大文件)
grep --color=auto "pattern" <file>  # 高亮显示
```

### sed (流编辑器)

```bash
# 替换文本
sed 's/old/new/' <file>  # 替换每行第一个匹配
sed 's/old/new/g' <file>  # 替换所有匹配
sed -i 's/old/new/g' <file>  # 直接修改文件

# 删除行
sed '3d' <file>  # 删除第 3 行
sed '/pattern/d' <file>  # 删除匹配行
sed '1,5d' <file>  # 删除 1-5 行

# 插入/追加行
sed '3i\new line' <file>  # 在第 3 行前插入
sed '3a\new line' <file>  # 在第 3 行后追加

# 打印特定行
sed -n '10,20p' <file>  # 打印 10-20 行
sed -n '/pattern/p' <file>  # 打印匹配行

# 多条命令
sed -e 's/old1/new1/' -e 's/old2/new2/' <file>
```

### awk (文本分析)

```bash
# 打印列
awk '{print $1}' <file>  # 第 1 列
awk '{print $1, $3}' <file>  # 第 1 和 3 列
awk '{print $NF}' <file>  # 最后一列

# 条件过滤
awk '$3 > 100' <file>  # 第 3 列大于 100
awk '/pattern/ {print $1}' <file>  # 匹配行打印第 1 列

# 内置变量
awk '{print NR, $0}' <file>  # NR: 行号
awk '{print NF, $0}' <file>  # NF: 字段数

# 分隔符
awk -F':' '{print $1}' /etc/passwd  # 使用 : 分隔
awk -F',' '{print $2}' data.csv  # CSV 文件

# 统计
awk '{sum += $1} END {print sum}' <file>  # 求和
awk '{if ($1 > max) max = $1} END {print max}' <file>  # 最大值

# 格式化输出
awk '{printf "%-10s %5d\n", $1, $2}' <file>
```

### sort (排序)

```bash
# 基础排序
sort <file>  # 字典序
sort -n <file>  # 数字排序
sort -r <file>  # 反向排序
sort -u <file>  # 去重

# 按列排序
sort -k 2 <file>  # 按第 2 列排序
sort -k 2n <file>  # 按第 2 列数字排序
sort -t ':' -k 3n /etc/passwd  # 指定分隔符

# 人类可读大小排序
du -h | sort -h  # 按大小排序 (1K, 1M, 1G)

# 组合使用
cat <file> | sort | uniq  # 排序去重
```

### uniq (去重)

```bash
# 去重 (需要先排序)
sort <file> | uniq

# 统计重复次数
sort <file> | uniq -c

# 仅显示重复行
sort <file> | uniq -d

# 仅显示唯一行
sort <file> | uniq -u
```

### cut (提取列)

```bash
# 按字符位置
cut -c 1-5 <file>  # 第 1-5 个字符

# 按分隔符
cut -d ':' -f 1 /etc/passwd  # 第 1 字段 (: 分隔)
cut -d ',' -f 1,3 data.csv  # 第 1 和 3 字段

# 组合使用
cat /etc/passwd | cut -d ':' -f 1 | sort
```

### tr (字符转换)

```bash
# 大小写转换
echo "hello" | tr '[:lower:]' '[:upper:]'  # HELLO
echo "WORLD" | tr '[:upper:]' '[:lower:]'  # world

# 删除字符
echo "hello123" | tr -d '[:digit:]'  # hello

# 压缩重复字符
echo "heeelllo" | tr -s 'e'  # hello
```

---

## 进程管理

### 进程查看

```bash
# 查看所有进程
ps aux  # BSD 风格
ps -ef  # UNIX 风格

# 常用过滤
ps aux | grep <process-name>
ps -ef | grep <process-name>

# 进程树
pstree
pstree -p  # 显示 PID

# 实时监控
top
htop  # 交互式 (推荐)

# 查看进程详情
ps -p <pid> -o pid,ppid,cmd,%mem,%cpu

# 查看进程打开的文件
lsof -p <pid>

# 查看进程环境变量
cat /proc/<pid>/environ | tr '\0' '\n'

# 查看进程命令行
cat /proc/<pid>/cmdline
ps -p <pid> -o args
```

### 进程控制

```bash
# 启动进程
<command> &  # 后台运行
nohup <command> &  # 忽略 HUP 信号

# 查看后台任务
jobs
jobs -l  # 显示 PID

# 前后台切换
fg %1  # 将任务 1 调到前台
bg %1  # 将任务 1 放到后台
Ctrl+Z  # 暂停当前进程 (SIGTSTP)

# 终止进程
kill <pid>  # SIGTERM (默认)
kill -9 <pid>  # SIGKILL (强制)
kill -15 <pid>  # SIGTERM (优雅终止)
killall <process-name>  # 终止所有匹配进程
pkill <process-name>  # 按名称终止

# 按用户终止
pkill -u <username>

# 发送其他信号
kill -HUP <pid>  # 重新加载配置
kill -STOP <pid>  # 暂停进程
kill -CONT <pid>  # 恢复进程
```

**常用信号**:
- `SIGTERM (15)` - 优雅终止 (默认)
- `SIGKILL (9)` - 强制终止 (无法捕获)
- `SIGHUP (1)` - 重新加载配置
- `SIGSTOP (19)` - 暂停进程
- `SIGCONT (18)` - 恢复进程

### 进程优先级

```bash
# 查看 nice 值
ps -eo pid,nice,comm

# 启动时设置优先级 (-20 最高, 19 最低)
nice -n 10 <command>  # 降低优先级
nice -n -10 <command>  # 提高优先级 (需要 root)

# 修改运行中进程优先级
renice -n 5 -p <pid>  # 设置为 5
renice -n 10 -u <username>  # 按用户
```

### 定时任务

```bash
# cron (周期性任务)
crontab -e  # 编辑当前用户 crontab
crontab -l  # 列出当前用户 crontab
crontab -r  # 删除当前用户 crontab
crontab -u <user> -e  # 编辑指定用户 (需要 root)

# crontab 格式
# 分 时 日 月 周 命令
# */5 * * * * /path/to/script.sh  # 每 5 分钟
# 0 2 * * * /path/to/backup.sh    # 每天 2:00
# 0 0 * * 0 /path/to/weekly.sh    # 每周日 0:00

# 查看系统 cron 日志
grep CRON /var/log/syslog  # Ubuntu/Debian
grep CRON /var/log/cron    # RHEL/CentOS

# at (一次性任务)
at now + 1 hour  # 1 小时后执行
at 10:00 AM tomorrow  # 明天 10:00
atq  # 查看队列
atrm <job-number>  # 删除任务
```

**cron 特殊字符**:
- `*` - 任意值
- `,` - 列举 (1,3,5)
- `-` - 范围 (1-5)
- `/` - 间隔 (*/5)

---

## 网络管理

### 网络接口

```bash
# 查看网络接口 (现代工具)
ip addr show  # 替代 ifconfig
ip link show
ip -s link show  # 显示统计信息

# 查看路由
ip route show
ip route get 8.8.8.8  # 查看到特定 IP 的路由

# 启用/禁用网络接口
sudo ip link set eth0 up
sudo ip link set eth0 down

# 配置 IP 地址
sudo ip addr add 192.168.1.100/24 dev eth0
sudo ip addr del 192.168.1.100/24 dev eth0

# 传统工具 (ifconfig, 部分系统已弃用)
ifconfig  # 查看接口
ifconfig eth0  # 查看特定接口
ifconfig eth0 192.168.1.100 netmask 255.255.255.0  # 配置 IP
route -n  # 查看路由表
```

**工具版本**:
- `ip`: iproute2 v5.10+ (Ubuntu 22.04+, RHEL 9+)
- `ifconfig`: net-tools (已弃用，但仍广泛使用)

### 网络连通性测试

```bash
# ping (ICMP)
ping <host>
ping -c 4 <host>  # 发送 4 个包
ping -i 0.5 <host>  # 间隔 0.5 秒

# traceroute (路由追踪)
traceroute <host>
traceroute -I <host>  # 使用 ICMP (默认 UDP)
tracepath <host>  # 无需 root

# mtr (结合 ping 和 traceroute)
mtr <host>
mtr -c 10 <host>  # 发送 10 个包

# telnet (TCP 端口测试)
telnet <host> <port>

# nc (netcat, 瑞士军刀)
nc -zv <host> <port>  # 端口扫描
nc -zv <host> 1-1024  # 扫描 1-1024 端口
nc -l <port>  # 监听端口
echo "hello" | nc <host> <port>  # 发送数据

# curl (HTTP 测试)
curl http://example.com
curl -I http://example.com  # 仅 HTTP 头
curl -o /dev/null -s -w '%{http_code}\n' http://example.com  # 仅状态码

# wget (下载测试)
wget http://example.com/file
wget -O - http://example.com  # 输出到 stdout
```

### DNS 查询

```bash
# nslookup (交互式 DNS 查询)
nslookup example.com
nslookup example.com 8.8.8.8  # 指定 DNS 服务器

# dig (详细 DNS 查询, 推荐)
dig example.com
dig @8.8.8.8 example.com  # 指定 DNS 服务器
dig example.com +short  # 简洁输出
dig example.com ANY  # 查询所有记录
dig -x 1.2.3.4  # 反向查询

# host (简单 DNS 查询)
host example.com
host 1.2.3.4  # 反向查询

# 查看 DNS 配置
cat /etc/resolv.conf
systemd-resolve --status  # systemd-resolved (Ubuntu 18.04+)
```

### 网络连接

```bash
# 查看所有连接 (netstat, 传统工具)
netstat -tunlp  # TCP/UDP, 数字, 监听, 程序
netstat -anp  # 所有连接

# ss (替代 netstat, 更快)
ss -tunlp  # 同上
ss -s  # 统计信息
ss -o state established  # 已建立连接
ss -o state listening  # 监听端口

# 查看端口占用
sudo lsof -i :80  # 端口 80
sudo lsof -i tcp:80  # TCP 端口 80
sudo fuser 80/tcp  # 查找占用端口的进程

# 查看网络统计
netstat -s
ss -s
```

**工具对比**:
- `netstat` (net-tools) - 传统工具，已弃用但仍广泛使用
- `ss` (iproute2) - 现代工具，性能更好

### 防火墙 (firewalld - RHEL/CentOS)

```bash
# firewalld 状态 (RHEL 7+, CentOS 7+)
sudo systemctl status firewalld
sudo firewall-cmd --state

# 查看规则
sudo firewall-cmd --list-all
sudo firewall-cmd --list-services
sudo firewall-cmd --list-ports

# 添加服务
sudo firewall-cmd --add-service=http --permanent
sudo firewall-cmd --add-service=https --permanent
sudo firewall-cmd --reload  # 重载规则

# 添加端口
sudo firewall-cmd --add-port=8080/tcp --permanent
sudo firewall-cmd --reload

# 删除规则
sudo firewall-cmd --remove-service=http --permanent
sudo firewall-cmd --remove-port=8080/tcp --permanent
sudo firewall-cmd --reload

# 查看所有 zone
sudo firewall-cmd --get-zones
sudo firewall-cmd --get-active-zones

# 更改接口 zone
sudo firewall-cmd --zone=public --change-interface=eth0 --permanent
```

**firewalld 版本**: v1.0+ (RHEL 9+, CentOS 9+)

### 防火墙 (ufw - Ubuntu/Debian)

```bash
# ufw 状态 (Ubuntu 16.04+, Debian 9+)
sudo ufw status
sudo ufw status verbose

# 启用/禁用
sudo ufw enable
sudo ufw disable

# 添加规则
sudo ufw allow 22/tcp  # SSH
sudo ufw allow 80/tcp  # HTTP
sudo ufw allow 443/tcp  # HTTPS
sudo ufw allow from 192.168.1.0/24  # 允许子网

# 删除规则
sudo ufw delete allow 80/tcp
sudo ufw status numbered  # 显示编号
sudo ufw delete 2  # 按编号删除

# 默认策略
sudo ufw default deny incoming
sudo ufw default allow outgoing

# 重置规则
sudo ufw reset
```

### 防火墙 (iptables - 通用)

```bash
# 查看规则
sudo iptables -L -n -v
sudo iptables -L INPUT -n -v  # 查看 INPUT 链

# 允许端口
sudo iptables -A INPUT -p tcp --dport 80 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 443 -j ACCEPT

# 允许 IP
sudo iptables -A INPUT -s 192.168.1.100 -j ACCEPT

# 删除规则
sudo iptables -D INPUT -p tcp --dport 80 -j ACCEPT
sudo iptables -L INPUT --line-numbers  # 显示行号
sudo iptables -D INPUT 1  # 按行号删除

# 保存规则
sudo iptables-save > /etc/iptables/rules.v4  # Debian/Ubuntu
sudo service iptables save  # RHEL/CentOS 6
sudo systemctl enable iptables  # RHEL/CentOS 7+

# 恢复规则
sudo iptables-restore < /etc/iptables/rules.v4
```

---

## 磁盘与存储

### 磁盘使用

```bash
# 查看磁盘使用
df -h  # 人类可读
df -i  # inode 使用情况
df -T  # 显示文件系统类型

# 查看目录大小
du -sh <directory>  # 目录总大小
du -sh *  # 当前目录下所有项大小
du -h --max-depth=1  # 深度 1
du -ah <directory>  # 包含文件

# 查找大文件
find / -type f -size +1G  # 大于 1GB
du -ah / | sort -rh | head -n 20  # 前 20 大文件/目录
```

### 磁盘分区

```bash
# 查看分区 (所有系统)
lsblk
lsblk -f  # 显示文件系统
fdisk -l  # 需要 root

# 分区工具
sudo fdisk /dev/sdb  # MBR 分区 (传统)
sudo parted /dev/sdb  # GPT 分区 (推荐)
sudo gdisk /dev/sdb  # GPT 分区 (GPT fdisk)

# parted 示例
sudo parted /dev/sdb
(parted) mklabel gpt  # 创建 GPT 分区表
(parted) mkpart primary ext4 0% 100%  # 创建分区
(parted) print  # 查看分区表
(parted) quit

# 格式化
sudo mkfs.ext4 /dev/sdb1  # ext4
sudo mkfs.xfs /dev/sdb1   # XFS
sudo mkfs.btrfs /dev/sdb1  # Btrfs
```

**文件系统推荐**:
- **ext4**: 通用，稳定 (默认)
- **XFS**: 大文件，高性能 (RHEL 7+ 默认)
- **Btrfs**: 快照，压缩 (Ubuntu 20.04+ 支持)

### 挂载与卸载

```bash
# 挂载
sudo mount /dev/sdb1 /mnt
sudo mount -t ext4 /dev/sdb1 /mnt  # 指定类型
sudo mount -o ro /dev/sdb1 /mnt  # 只读挂载

# 查看挂载
mount  # 所有挂载点
mount | grep sdb1  # 特定设备
findmnt  # 树状显示 (systemd)

# 卸载
sudo umount /mnt
sudo umount /dev/sdb1

# 强制卸载 (设备忙时)
sudo fuser -km /mnt  # 终止占用进程
sudo umount -l /mnt  # 懒卸载

# 永久挂载 (/etc/fstab)
# <device>  <mount-point>  <fs-type>  <options>  <dump>  <pass>
# /dev/sdb1  /data  ext4  defaults  0  2
# UUID=xxx  /data  ext4  defaults,noatime  0  2  # 推荐使用 UUID

# 查看 UUID
sudo blkid /dev/sdb1

# 重新挂载 (应用 fstab 变更)
sudo mount -a
```

### LVM (逻辑卷管理)

```bash
# 查看 PV (物理卷)
sudo pvdisplay
sudo pvs

# 查看 VG (卷组)
sudo vgdisplay
sudo vgs

# 查看 LV (逻辑卷)
sudo lvdisplay
sudo lvs

# 创建 PV
sudo pvcreate /dev/sdb1

# 创建 VG
sudo vgcreate vg01 /dev/sdb1

# 创建 LV
sudo lvcreate -L 10G -n lv_data vg01  # 固定大小
sudo lvcreate -l 100%FREE -n lv_data vg01  # 使用全部空间

# 扩容 LV
sudo lvextend -L +5G /dev/vg01/lv_data  # 增加 5GB
sudo lvextend -l +100%FREE /dev/vg01/lv_data  # 使用所有剩余空间

# 扩容文件系统 (ext4)
sudo resize2fs /dev/vg01/lv_data

# 扩容文件系统 (XFS)
sudo xfs_growfs /data
```

**LVM 版本**: lvm2 v2.03+ (Ubuntu 22.04+, RHEL 9+)

### RAID 管理

```bash
# 查看 RAID (mdadm)
cat /proc/mdstat
sudo mdadm --detail /dev/md0

# 创建 RAID 1 (镜像)
sudo mdadm --create /dev/md0 --level=1 --raid-devices=2 /dev/sdb1 /dev/sdc1

# 创建 RAID 5
sudo mdadm --create /dev/md0 --level=5 --raid-devices=3 /dev/sdb1 /dev/sdc1 /dev/sdd1

# 添加磁盘
sudo mdadm --add /dev/md0 /dev/sde1

# 移除磁盘
sudo mdadm --fail /dev/md0 /dev/sdb1
sudo mdadm --remove /dev/md0 /dev/sdb1

# 保存配置
sudo mdadm --detail --scan | sudo tee -a /etc/mdadm/mdadm.conf
```

---

## 用户与权限

### 用户管理

```bash
# 查看用户
cat /etc/passwd
getent passwd  # 包含 LDAP/NIS 用户
id <username>  # 用户信息
whoami  # 当前用户
who  # 当前登录用户
w  # 当前登录用户 (详细)

# 创建用户
sudo useradd <username>
sudo useradd -m -s /bin/bash <username>  # 创建家目录，指定 shell
sudo useradd -m -G sudo <username>  # 加入 sudo 组 (Ubuntu/Debian)
sudo useradd -m -G wheel <username>  # 加入 wheel 组 (RHEL/CentOS)

# 修改用户
sudo usermod -aG <group> <username>  # 添加到组
sudo usermod -s /bin/zsh <username>  # 修改 shell
sudo usermod -L <username>  # 锁定用户
sudo usermod -U <username>  # 解锁用户

# 删除用户
sudo userdel <username>  # 保留家目录
sudo userdel -r <username>  # 删除家目录

# 设置密码
sudo passwd <username>
passwd  # 修改自己密码

# 切换用户
su - <username>  # 完全切换 (加载环境)
su <username>  # 部分切换
sudo -i  # 切换到 root (加载环境)
sudo -s  # 切换到 root (不加载环境)
```

### 组管理

```bash
# 查看组
cat /etc/group
getent group
groups <username>  # 用户所属组

# 创建组
sudo groupadd <groupname>

# 删除组
sudo groupdel <groupname>

# 添加用户到组
sudo usermod -aG <group> <username>
sudo gpasswd -a <username> <group>  # 替代方法

# 从组移除用户
sudo gpasswd -d <username> <group>

# 修改组
sudo groupmod -n <new-name> <old-name>  # 重命名
```

### sudo 配置

```bash
# 编辑 sudoers 文件 (推荐使用 visudo)
sudo visudo

# 常见配置
# <user> ALL=(ALL:ALL) ALL  # 用户完全权限
# %sudo ALL=(ALL:ALL) ALL  # sudo 组完全权限
# <user> ALL=(ALL) NOPASSWD: ALL  # 无密码 sudo

# 测试 sudo 权限
sudo -l  # 列出当前用户权限
sudo -l -U <username>  # 查看指定用户权限

# sudo 日志
grep sudo /var/log/auth.log  # Ubuntu/Debian
grep sudo /var/log/secure  # RHEL/CentOS
```

---

## 系统服务管理

### systemd (systemctl)

**适用系统**: RHEL/CentOS 7+, Ubuntu 16.04+, Debian 8+

```bash
# 服务状态
sudo systemctl status <service>
sudo systemctl is-active <service>  # 仅状态
sudo systemctl is-enabled <service>  # 是否开机启动

# 启动/停止服务
sudo systemctl start <service>
sudo systemctl stop <service>
sudo systemctl restart <service>
sudo systemctl reload <service>  # 重新加载配置 (不重启)

# 开机启动
sudo systemctl enable <service>
sudo systemctl disable <service>
sudo systemctl enable --now <service>  # 启用并立即启动

# 查看所有服务
systemctl list-units --type=service
systemctl list-units --type=service --state=running  # 运行中
systemctl list-units --type=service --state=failed  # 失败

# 查看服务日志
sudo journalctl -u <service>
sudo journalctl -u <service> -f  # 实时跟踪
sudo journalctl -u <service> --since today
sudo journalctl -u <service> --since "2026-02-11 10:00:00"

# 查看启动时间
systemd-analyze
systemd-analyze blame  # 慢启动服务

# 重载 systemd 配置
sudo systemctl daemon-reload
```

**常用服务名**:
- `sshd` / `ssh` - SSH 服务
- `nginx` - Nginx Web 服务器
- `apache2` / `httpd` - Apache Web 服务器
- `mysql` / `mariadb` - MySQL/MariaDB 数据库
- `postgresql` - PostgreSQL 数据库
- `docker` - Docker 守护进程
- `kubelet` - Kubernetes 节点代理

### SysVinit (service)

**适用系统**: RHEL/CentOS 6, Ubuntu 14.04 (传统系统)

```bash
# 服务状态
sudo service <service> status

# 启动/停止服务
sudo service <service> start
sudo service <service> stop
sudo service <service> restart
sudo service <service> reload

# 开机启动
sudo chkconfig <service> on  # RHEL/CentOS 6
sudo update-rc.d <service> defaults  # Ubuntu 14.04

# 查看所有服务
sudo service --status-all
sudo chkconfig --list  # RHEL/CentOS 6
```

---

## 性能监控

### CPU 监控

```bash
# 实时监控
top  # 按 1 查看所有 CPU
htop  # 交互式 (推荐)

# CPU 使用历史 (sysstat)
mpstat 1 5  # 每秒刷新，5 次
sar -u 1 5  # CPU 使用率

# 查看 CPU 信息
lscpu
cat /proc/cpuinfo
nproc  # CPU 核心数
```

### 内存监控

```bash
# 查看内存
free -h
free -m -s 5  # 每 5 秒刷新

# 内存详细信息
cat /proc/meminfo
vmstat 1 5  # 虚拟内存统计

# 内存使用 Top 10 进程
ps aux --sort=-%mem | head -n 11
```

### 磁盘 I/O 监控

```bash
# I/O 统计 (sysstat)
iostat -x 1 5  # 扩展统计
iostat -d 1 5  # 仅磁盘

# 实时 I/O 监控
iotop  # 需要 root
iotop -o  # 仅显示有 I/O 的进程

# 查看磁盘读写
cat /proc/diskstats
```

### 网络监控

```bash
# 实时流量监控
iftop -i eth0  # 需要安装
nload  # 图形化流量
bmon  # 带宽监控

# 网络统计
netstat -i  # 接口统计
ip -s link  # 接口统计 (iproute2)

# 抓包
sudo tcpdump -i eth0  # 抓包
sudo tcpdump -i eth0 port 80  # 抓 80 端口
sudo tcpdump -i eth0 -w capture.pcap  # 保存到文件
```

### 系统监控工具

```bash
# vmstat (虚拟内存统计)
vmstat 1 5  # 每秒刷新，5 次

# sar (系统活动报告)
sar -u 1 5  # CPU
sar -r 1 5  # 内存
sar -d 1 5  # 磁盘
sar -n DEV 1 5  # 网络

# dstat (多合一监控)
dstat  # 实时监控
dstat -cdngy  # CPU、磁盘、网络、系统

# glances (全能监控, 需要安装)
glances  # 类似 htop，更强大
glances -w  # Web 模式
```

**工具包版本**:
- **sysstat** (sar, iostat, mpstat): v12.5+ (Ubuntu 22.04+, RHEL 9+)
- **glances**: v3.3+ (Python 工具)
- **dstat**: 已停止维护，被 `sar` 替代

---

## 日志分析

### 系统日志

```bash
# systemd 日志 (journalctl)
sudo journalctl  # 所有日志
sudo journalctl -f  # 实时跟踪
sudo journalctl -b  # 本次启动日志
sudo journalctl -b -1  # 上次启动日志

# 按时间过滤
sudo journalctl --since today
sudo journalctl --since "2026-02-11 10:00:00"
sudo journalctl --until "2026-02-11 12:00:00"
sudo journalctl --since "1 hour ago"

# 按服务过滤
sudo journalctl -u sshd
sudo journalctl -u nginx -f

# 按优先级过滤
sudo journalctl -p err  # 错误级别
sudo journalctl -p warning  # 警告级别

# 按进程过滤
sudo journalctl _PID=<pid>

# 导出日志
sudo journalctl -u nginx > nginx.log

# 清理日志
sudo journalctl --vacuum-time=7d  # 保留 7 天
sudo journalctl --vacuum-size=1G  # 保留 1GB
```

### 传统日志

```bash
# 日志文件位置
/var/log/syslog        # Ubuntu/Debian 系统日志
/var/log/messages      # RHEL/CentOS 系统日志
/var/log/auth.log      # Ubuntu/Debian 认证日志
/var/log/secure        # RHEL/CentOS 认证日志
/var/log/kern.log      # 内核日志
/var/log/dmesg         # 启动日志
/var/log/cron          # cron 日志
/var/log/mail.log      # 邮件日志
/var/log/nginx/        # Nginx 日志
/var/log/apache2/      # Apache 日志

# 查看日志
sudo tail -f /var/log/syslog  # 实时跟踪
sudo less /var/log/syslog
sudo grep "error" /var/log/syslog  # 搜索错误

# dmesg (内核日志)
dmesg  # 所有内核日志
dmesg -T  # 人类可读时间
dmesg -l err  # 仅错误
dmesg -w  # 实时跟踪
```

### 日志轮转 (logrotate)

```bash
# logrotate 配置
/etc/logrotate.conf  # 主配置
/etc/logrotate.d/    # 应用配置

# 手动执行轮转
sudo logrotate /etc/logrotate.conf
sudo logrotate -f /etc/logrotate.conf  # 强制轮转

# 测试配置
sudo logrotate -d /etc/logrotate.conf

# 示例配置 (/etc/logrotate.d/myapp)
/var/log/myapp/*.log {
    daily           # 每天轮转
    missingok       # 文件不存在不报错
    rotate 7        # 保留 7 份
    compress        # 压缩旧日志
    delaycompress   # 延迟压缩 (下次轮转时压缩)
    notifempty      # 空文件不轮转
    create 0640 www-data adm  # 创建新文件权限
    sharedscripts   # 所有日志轮转后执行一次
    postrotate
        systemctl reload nginx > /dev/null
    endscript
}
```

---

## 安全与防火墙

### SSH 安全

```bash
# SSH 配置文件
sudo vim /etc/ssh/sshd_config

# 推荐安全配置
Port 22  # 修改默认端口 (可选)
PermitRootLogin no  # 禁止 root 登录
PasswordAuthentication no  # 禁用密码登录 (仅密钥)
PubkeyAuthentication yes  # 启用密钥认证
ClientAliveInterval 300  # 5 分钟保活
ClientAliveCountMax 2

# 重启 SSH 服务
sudo systemctl restart sshd

# 生成 SSH 密钥
ssh-keygen -t ed25519 -C "your_email@example.com"  # Ed25519 (推荐)
ssh-keygen -t rsa -b 4096 -C "your_email@example.com"  # RSA 4096

# 复制公钥到远程
ssh-copy-id user@remote-host
ssh-copy-id -i ~/.ssh/id_ed25519.pub user@remote-host

# SSH 代理转发
ssh -A user@remote-host

# SSH 隧道
ssh -L 8080:localhost:80 user@remote-host  # 本地转发
ssh -R 8080:localhost:80 user@remote-host  # 远程转发
ssh -D 1080 user@remote-host  # SOCKS 代理
```

### SELinux (RHEL/CentOS)

```bash
# 查看 SELinux 状态
getenforce
sestatus

# 设置模式
sudo setenforce 0  # Permissive (临时)
sudo setenforce 1  # Enforcing (临时)

# 永久修改 (/etc/selinux/config)
SELINUX=enforcing   # 启用
SELINUX=permissive  # 宽容模式
SELINUX=disabled    # 禁用 (需要重启)

# 查看上下文
ls -Z /path/to/file
ps -eZ  # 进程上下文

# 修改上下文
sudo chcon -t httpd_sys_content_t /var/www/html/index.html
sudo restorecon -Rv /var/www/html  # 恢复默认上下文

# 查看布尔值
getsebool -a
getsebool httpd_can_network_connect

# 设置布尔值
sudo setsebool httpd_can_network_connect on
sudo setsebool -P httpd_can_network_connect on  # 永久

# 查看审计日志
sudo ausearch -m avc -ts recent
sudo grep AVC /var/log/audit/audit.log

# 生成策略 (从拒绝日志)
sudo audit2allow -w -a  # 分析
sudo audit2allow -a -M my-policy  # 生成策略
sudo semodule -i my-policy.pp  # 加载策略
```

### AppArmor (Ubuntu/Debian)

```bash
# 查看状态
sudo apparmor_status
sudo aa-status

# 配置文件位置
/etc/apparmor.d/

# 模式
# - enforce: 强制模式
# - complain: 投诉模式 (仅记录)
# - disabled: 禁用

# 设置模式
sudo aa-complain /etc/apparmor.d/usr.sbin.nginx  # 投诉模式
sudo aa-enforce /etc/apparmor.d/usr.sbin.nginx   # 强制模式

# 重新加载配置
sudo apparmor_parser -r /etc/apparmor.d/usr.sbin.nginx

# 禁用 profile
sudo ln -s /etc/apparmor.d/usr.sbin.nginx /etc/apparmor.d/disable/
sudo apparmor_parser -R /etc/apparmor.d/usr.sbin.nginx

# 查看日志
sudo journalctl -fx | grep apparmor
sudo grep DENIED /var/log/syslog
```

### fail2ban (暴力破解防护)

```bash
# 安装 (Ubuntu/Debian)
sudo apt install fail2ban

# 配置文件
/etc/fail2ban/jail.conf  # 默认配置 (不要修改)
/etc/fail2ban/jail.local  # 自定义配置

# 示例配置 (/etc/fail2ban/jail.local)
[sshd]
enabled = true
port = 22
filter = sshd
logpath = /var/log/auth.log
maxretry = 3
bantime = 3600
findtime = 600

# 启动服务
sudo systemctl start fail2ban
sudo systemctl enable fail2ban

# 查看状态
sudo fail2ban-client status
sudo fail2ban-client status sshd

# 解封 IP
sudo fail2ban-client set sshd unbanip <ip>
```

**fail2ban 版本**: v0.11+ (Ubuntu 22.04+, RHEL 9+)

---

## 包管理

### APT (Ubuntu/Debian)

```bash
# 更新包索引
sudo apt update

# 升级所有包
sudo apt upgrade  # 升级已安装包
sudo apt full-upgrade  # 升级 + 处理依赖
sudo apt dist-upgrade  # 旧命令 (同 full-upgrade)

# 安装包
sudo apt install <package>
sudo apt install <package1> <package2>

# 删除包
sudo apt remove <package>  # 保留配置文件
sudo apt purge <package>   # 删除配置文件
sudo apt autoremove  # 删除孤立依赖

# 搜索包
apt search <keyword>
apt-cache search <keyword>  # 旧命令

# 查看包信息
apt show <package>
apt-cache show <package>  # 旧命令

# 查看已安装包
apt list --installed
dpkg -l  # 旧命令

# 查看包文件列表
dpkg -L <package>

# 查看文件属于哪个包
dpkg -S /path/to/file

# 清理缓存
sudo apt clean  # 清理所有缓存
sudo apt autoclean  # 清理过时缓存

# 添加 PPA (Ubuntu)
sudo add-apt-repository ppa:<ppa-name>
sudo add-apt-repository --remove ppa:<ppa-name>  # 删除

# 锁定包版本
sudo apt-mark hold <package>
sudo apt-mark unhold <package>
```

**APT 版本**: v2.4+ (Ubuntu 22.04+, Debian 12+)

### YUM/DNF (RHEL/CentOS)

```bash
# DNF (RHEL 8+, CentOS 8+, Fedora)
# YUM (RHEL 7, CentOS 7) - 命令相同

# 更新包索引
sudo dnf check-update  # DNF
sudo yum check-update  # YUM

# 升级所有包
sudo dnf upgrade  # DNF
sudo yum update   # YUM

# 安装包
sudo dnf install <package>
sudo yum install <package>

# 删除包
sudo dnf remove <package>
sudo yum remove <package>

# 搜索包
dnf search <keyword>
yum search <keyword>

# 查看包信息
dnf info <package>
yum info <package>

# 查看已安装包
dnf list installed
yum list installed
rpm -qa  # 使用 RPM

# 查看包文件列表
rpm -ql <package>

# 查看文件属于哪个包
rpm -qf /path/to/file
dnf provides /path/to/file

# 清理缓存
sudo dnf clean all
sudo yum clean all

# 查看仓库
dnf repolist
yum repolist

# 启用/禁用仓库
sudo dnf config-manager --enable <repo>
sudo dnf config-manager --disable <repo>

# 添加仓库 (EPEL)
sudo dnf install epel-release  # RHEL 8+
sudo yum install epel-release  # RHEL 7

# 锁定包版本
sudo dnf install 'dnf-command(versionlock)'
sudo dnf versionlock add <package>
sudo dnf versionlock list
sudo dnf versionlock delete <package>
```

**版本说明**:
- **DNF**: RHEL 8+, CentOS 8+, Fedora (替代 YUM)
- **YUM**: RHEL 7, CentOS 7 (传统)

---

## Shell 脚本

### Bash 基础

```bash
#!/bin/bash
# Shebang (指定解释器)

# 变量
name="John"
echo "Hello, $name"
echo "Hello, ${name}"  # 推荐

# 只读变量
readonly PI=3.14

# 环境变量
export MY_VAR="value"

# 命令替换
today=$(date +%Y-%m-%d)
today=`date +%Y-%m-%d`  # 旧语法

# 数组
arr=("apple" "banana" "cherry")
echo ${arr[0]}  # 第一个元素
echo ${arr[@]}  # 所有元素
echo ${#arr[@]}  # 数组长度

# 关联数组 (Bash 4+)
declare -A colors
colors[red]="#FF0000"
colors[green]="#00FF00"
echo ${colors[red]}
```

### 条件判断

```bash
# if 语句
if [ $age -gt 18 ]; then
    echo "Adult"
elif [ $age -eq 18 ]; then
    echo "Just 18"
else
    echo "Minor"
fi

# 数值比较
-eq  # 等于
-ne  # 不等于
-gt  # 大于
-ge  # 大于等于
-lt  # 小于
-le  # 小于等于

# 字符串比较
[ "$str1" = "$str2" ]   # 等于
[ "$str1" != "$str2" ]  # 不等于
[ -z "$str" ]           # 空字符串
[ -n "$str" ]           # 非空字符串

# 文件测试
[ -e file ]  # 存在
[ -f file ]  # 是普通文件
[ -d dir ]   # 是目录
[ -r file ]  # 可读
[ -w file ]  # 可写
[ -x file ]  # 可执行
[ -s file ]  # 非空文件

# 逻辑运算
[ cond1 ] && [ cond2 ]  # 与
[ cond1 ] || [ cond2 ]  # 或
[ ! cond ]              # 非

# 双括号 (推荐)
if [[ $age -gt 18 && $name == "John" ]]; then
    echo "Match"
fi
```

### 循环

```bash
# for 循环
for i in 1 2 3 4 5; do
    echo $i
done

# for 循环 (C 风格)
for ((i=1; i<=5; i++)); do
    echo $i
done

# for 循环 (数组)
for item in "${arr[@]}"; do
    echo $item
done

# for 循环 (文件)
for file in *.txt; do
    echo $file
done

# while 循环
i=1
while [ $i -le 5 ]; do
    echo $i
    ((i++))
done

# until 循环
i=1
until [ $i -gt 5 ]; do
    echo $i
    ((i++))
done

# break 和 continue
for i in {1..10}; do
    if [ $i -eq 5 ]; then
        continue  # 跳过 5
    fi
    if [ $i -eq 8 ]; then
        break  # 退出循环
    fi
    echo $i
done
```

### 函数

```bash
# 定义函数
function greet() {
    echo "Hello, $1"
}

# 或
greet() {
    echo "Hello, $1"
}

# 调用函数
greet "John"

# 返回值 (0-255)
check_file() {
    if [ -f "$1" ]; then
        return 0  # 成功
    else
        return 1  # 失败
    fi
}

check_file "file.txt"
if [ $? -eq 0 ]; then
    echo "File exists"
fi

# 局部变量
my_func() {
    local var="local"
    echo $var
}
```

### 错误处理

```bash
# set 选项
set -e  # 遇到错误立即退出
set -u  # 使用未定义变量报错
set -o pipefail  # 管道任一命令失败则失败
set -x  # 打印执行的命令 (调试)

# 组合
set -euo pipefail

# trap (捕获信号)
trap "echo 'Error occurred'; exit 1" ERR
trap "echo 'Cleaning up...'; rm -f /tmp/tempfile" EXIT

# 检查命令执行状态
if command -v docker &> /dev/null; then
    echo "Docker installed"
else
    echo "Docker not installed"
    exit 1
fi
```

### 常用技巧

```bash
# 参数解析
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            echo "Usage: $0 [-h|--help]"
            exit 0
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# 日志函数
log() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] $1"
}

log "Script started"

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'  # No Color

echo -e "${RED}Error${NC}"
echo -e "${GREEN}Success${NC}"

# 进度条
for i in {1..100}; do
    echo -ne "Progress: $i%\r"
    sleep 0.1
done
echo ""
```

---

## 容器与虚拟化

### Docker 命令

```bash
# Docker 版本
docker --version
docker version  # 详细信息

# 镜像操作
docker images  # 列出镜像
docker pull <image>:<tag>  # 拉取镜像
docker build -t <image>:<tag> .  # 构建镜像
docker rmi <image>  # 删除镜像
docker tag <source> <target>  # 标记镜像

# 容器操作
docker ps  # 运行中容器
docker ps -a  # 所有容器
docker run -it <image> /bin/bash  # 交互式运行
docker run -d <image>  # 后台运行
docker run -p 8080:80 <image>  # 端口映射
docker run -v /host:/container <image>  # 挂载卷

# 容器管理
docker start <container>
docker stop <container>
docker restart <container>
docker rm <container>  # 删除容器
docker exec -it <container> /bin/bash  # 进入容器

# 日志
docker logs <container>
docker logs -f <container>  # 实时跟踪

# 查看容器信息
docker inspect <container>
docker stats  # 资源使用
docker top <container>  # 进程

# 清理
docker system prune  # 清理未使用资源
docker system prune -a  # 清理所有未使用镜像
```

**Docker 版本**: v24.0+ (兼容 K8s v1.25-v1.32)

### containerd 命令 (ctr)

```bash
# containerd 版本 (Kubernetes 默认运行时)
ctr version

# 镜像操作
ctr images ls  # 列出镜像
ctr images pull docker.io/library/nginx:latest  # 拉取镜像
ctr images rm <image>  # 删除镜像

# 容器操作
ctr containers ls  # 列出容器
ctr run -d <image> <container-id>  # 运行容器
ctr tasks ls  # 列出任务
ctr tasks kill <container-id>  # 终止任务

# 命名空间
ctr -n k8s.io images ls  # Kubernetes 命名空间
ctr -n k8s.io containers ls
```

**containerd 版本**: v1.7+ (Kubernetes v1.25-v1.32 推荐)

### crictl (CRI 工具)

```bash
# crictl 版本 (Kubernetes 推荐)
crictl version

# Pod 操作
crictl pods  # 列出 Pod
crictl pods --name <pod-name>  # 按名称过滤
crictl inspectp <pod-id>  # 查看 Pod 详情

# 容器操作
crictl ps  # 运行中容器
crictl ps -a  # 所有容器
crictl inspect <container-id>  # 查看容器详情
crictl logs <container-id>  # 查看日志
crictl exec -it <container-id> /bin/sh  # 进入容器

# 镜像操作
crictl images  # 列出镜像
crictl pull <image>  # 拉取镜像
crictl rmi <image>  # 删除镜像

# 统计
crictl stats  # 资源使用
```

**crictl 版本**: v1.28+ (兼容 K8s v1.25-v1.32)

---

## 故障排查

### 系统无法启动

```bash
# 单用户模式 (RHEL/CentOS)
# 启动时按 'e' 进入编辑模式
# 在 linux 行末添加: single 或 1
# 按 Ctrl+X 启动

# Ubuntu/Debian Rescue Mode
# GRUB 菜单选择 "Advanced options"
# 选择 "recovery mode"

# 检查文件系统
fsck /dev/sda1  # 修复文件系统错误
e2fsck -f /dev/sda1  # ext 文件系统
xfs_repair /dev/sda1  # XFS 文件系统

# 检查 /etc/fstab
cat /etc/fstab
# 注释掉有问题的行，重启
```

### 系统慢

```bash
# 检查系统负载
uptime
top
htop

# 检查内存
free -h
vmstat 1 5

# 检查磁盘 I/O
iostat -x 1 5
iotop

# 检查网络
iftop -i eth0
netstat -tunlp

# 查找占用资源的进程
ps aux --sort=-%cpu | head -n 10  # CPU Top 10
ps aux --sort=-%mem | head -n 10  # 内存 Top 10
```

### 磁盘满

```bash
# 检查磁盘使用
df -h

# 查找大文件
du -ah / | sort -rh | head -n 20
find / -type f -size +1G

# 检查 inode
df -i

# 清理日志
sudo journalctl --vacuum-time=7d
sudo find /var/log -type f -name "*.log" -mtime +30 -delete

# 清理包缓存
sudo apt clean  # Ubuntu/Debian
sudo dnf clean all  # RHEL/CentOS
```

### 网络不通

```bash
# 检查网络接口
ip link show
ip addr show

# 检查路由
ip route show

# 检查 DNS
cat /etc/resolv.conf
nslookup example.com
dig example.com

# Ping 测试
ping -c 4 8.8.8.8  # Google DNS
ping -c 4 example.com

# 检查端口
telnet <host> <port>
nc -zv <host> <port>

# 检查防火墙
sudo iptables -L -n -v
sudo firewall-cmd --list-all  # RHEL/CentOS
sudo ufw status  # Ubuntu/Debian

# 检查 SELinux (RHEL/CentOS)
getenforce
sudo setenforce 0  # 临时禁用
```

### 进程僵死

```bash
# 查找僵尸进程
ps aux | grep 'Z'
ps -eo pid,stat,comm | grep '^[0-9]* Z'

# 查找僵尸进程父进程
ps -o ppid= -p <zombie-pid>

# 终止父进程
kill -9 <parent-pid>

# 查找卡住的进程
ps aux | grep 'D'  # 不可中断睡眠 (通常是 I/O 等待)

# 查看进程栈
cat /proc/<pid>/stack
```

### 内存泄漏

```bash
# 监控内存使用
free -h -s 5  # 每 5 秒刷新

# 查看进程内存
ps aux --sort=-%mem | head -n 10
pmap -x <pid>  # 详细内存映射

# 使用 valgrind (开发调试)
valgrind --leak-check=full <command>

# 查看系统内存信息
cat /proc/meminfo
cat /proc/<pid>/status | grep Vm
```

---

## 生产环境最佳实践

### 安全加固

```bash
# 1. 禁止 root SSH 登录
sudo sed -i 's/#PermitRootLogin yes/PermitRootLogin no/' /etc/ssh/sshd_config

# 2. 禁用密码登录
sudo sed -i 's/#PasswordAuthentication yes/PasswordAuthentication no/' /etc/ssh/sshd_config

# 3. 配置 fail2ban
sudo apt install fail2ban  # Ubuntu/Debian
sudo systemctl enable --now fail2ban

# 4. 配置防火墙
sudo ufw enable  # Ubuntu/Debian
sudo firewall-cmd --permanent --add-service=ssh  # RHEL/CentOS

# 5. 定期更新
sudo apt update && sudo apt upgrade  # Ubuntu/Debian
sudo dnf upgrade  # RHEL/CentOS

# 6. 配置自动安全更新 (Ubuntu/Debian)
sudo apt install unattended-upgrades
sudo dpkg-reconfigure --priority=low unattended-upgrades
```

### 性能优化

```bash
# 1. 调整文件描述符限制
ulimit -n 65535
# 永久修改 /etc/security/limits.conf
*  soft  nofile  65535
*  hard  nofile  65535

# 2. 调整内核参数 (/etc/sysctl.conf)
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 8192
net.ipv4.ip_local_port_range = 1024 65535
net.ipv4.tcp_tw_reuse = 1
vm.swappiness = 10

# 应用配置
sudo sysctl -p

# 3. 禁用不必要的服务
sudo systemctl disable <service>

# 4. 使用 SSD 优化
sudo fstrim -v /  # 手动 TRIM
# 或启用定时 TRIM
sudo systemctl enable fstrim.timer
```

### 监控与告警

```bash
# 1. 安装 Node Exporter (Prometheus)
# 监控系统指标

# 2. 配置日志聚合
# - ELK Stack (Elasticsearch, Logstash, Kibana)
# - Loki + Grafana

# 3. 配置告警
# - Prometheus Alertmanager
# - 云厂商告警服务

# 4. 健康检查脚本
#!/bin/bash
# /usr/local/bin/health-check.sh

# 检查磁盘
if [ $(df / | tail -1 | awk '{print $5}' | sed 's/%//') -gt 80 ]; then
    echo "CRITICAL: Root partition > 80%"
    exit 2
fi

# 检查内存
if [ $(free | grep Mem | awk '{print ($3/$2) * 100.0}' | cut -d. -f1) -gt 90 ]; then
    echo "WARNING: Memory > 90%"
    exit 1
fi

echo "OK: All checks passed"
exit 0
```

### 备份策略

```bash
# 1. 定期备份
# 每日增量备份 + 每周全量备份

# 2. 备份脚本示例
#!/bin/bash
# /usr/local/bin/backup.sh

BACKUP_DIR="/backup"
DATE=$(date +%Y%m%d)

# 备份重要目录
tar -czf $BACKUP_DIR/etc-$DATE.tar.gz /etc
tar -czf $BACKUP_DIR/home-$DATE.tar.gz /home

# 保留 30 天
find $BACKUP_DIR -type f -mtime +30 -delete

# 3. crontab 定时
0 2 * * * /usr/local/bin/backup.sh

# 4. 验证备份
tar -tzf backup.tar.gz | head
```

---

## 附录: 常用工具版本对照

### 系统工具

| 工具 | Ubuntu 22.04 | Ubuntu 24.04 | RHEL 9 | 用途 |
|------|--------------|--------------|--------|------|
| systemd | v249 | v255 | v252 | 服务管理 |
| bash | v5.1 | v5.2 | v5.2 | Shell |
| openssh | v8.9 | v9.6 | v8.7 | 远程登录 |
| openssl | v3.0 | v3.0 | v3.0 | 加密库 |

### 监控工具

| 工具 | 版本 | 包名 | 说明 |
|------|------|------|------|
| sysstat | v12.5+ | sysstat | sar, iostat, mpstat |
| htop | v3.2+ | htop | 交互式进程监控 |
| iotop | v0.6+ | iotop | I/O 监控 |
| iftop | v1.0+ | iftop | 网络流量监控 |
| glances | v3.3+ | glances | 综合监控 |

### 网络工具

| 工具 | 包名 | 说明 |
|------|------|------|
| ip | iproute2 | 网络配置 (现代) |
| ss | iproute2 | 套接字统计 (现代) |
| ifconfig | net-tools | 网络配置 (传统) |
| netstat | net-tools | 网络统计 (传统) |
| dig | dnsutils (Ubuntu) / bind-utils (RHEL) | DNS 查询 |
| tcpdump | tcpdump | 抓包 |
| nmap | nmap | 端口扫描 |

---

**文档维护**: 建议每季度更新一次  
**兼容性**: 命令已在 RHEL 9, CentOS 9, Ubuntu 22.04/24.04, Debian 12 上测试  
**反馈渠道**: 如有错误或建议，请提交 Issue
