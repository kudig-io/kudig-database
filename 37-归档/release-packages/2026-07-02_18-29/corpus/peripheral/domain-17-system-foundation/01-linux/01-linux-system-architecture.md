---
title: 01 - Linux 系统架构与内核深度解析：生产环境运维专家指南
description: '# 01 - Linux 系统架构与内核深度解析：生产环境运维专家指南'
summary: '本文档从生产环境运维专家视角，深入解析 Linux 系统架构、内核工作机制和企业级最佳实践。涵盖系统启动优化、内核参数调优、性能监控、故障排查等关键运维技能，为 [[Kubernetes|Kubernetes]] 和容器化环境提供坚实的基础支撑。'
category: linux
tags:
- linux
- system
- kernel
- kubelet
- scheduler
- prometheus
- grafana
- containerd
- cri-o
- docker
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
- Linux 系统架构与内核深度解析：生产环境运维专家指南 是什么
- 如何 Linux 系统架构与内核深度解析：生产环境运维专家指南
- Kubernetes 14 linux 最佳实践
trigger_keywords:
- Linux
- 系统架构与内核深度解析：生产环境运维专家指南
- linux
prerequisites:
- kubectl-basics
- cloud-provider-basics
- prometheus-basics
- monitoring-basics
- gpu-scheduling-basics
- policy-basics
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 01 - Linux 系统架构与内核深度解析：生产环境运维专家指南

> **适用版本**: Linux Kernel 5.x/6.x | **最后更新**: 2026-02 | **作者**: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: 摘要 -->## 摘要

本文档从生产环境运维专家视角，深入解析 Linux 系统架构、内核工作机制和企业级最佳实践。涵盖系统启动优化、内核参数调优、性能监控、故障排查等关键运维技能，为 [[Kubernetes|Kubernetes]] 和容器化环境提供坚实的基础支撑。

**核心价值**：
- 🏗️ **架构深度理解**：掌握 Linux 内核各子系统的交互机制
- ⚡ **性能优化实践**：生产环境内核参数调优和性能监控
- 🔧 **故障排查指南**：系统级问题诊断和解决方法
- 🛡️ **安全加固策略**：企业级安全配置和合规要求
- 🔄 **自动化运维**：系统管理脚本和监控告警配置

---

<!-- chunk: 目录 -->## 目录

- [Linux 内核架构](#linux-内核架构)
- [系统启动过程](#系统启动过程)
- [systemd 服务管理](#systemd-服务管理)
- [内核参数调优](#内核参数调优)
- [内核模块管理](#内核模块管理)
- [主流发行版对比](#主流发行版对比)
- [生产环境最佳实践](#生产环境最佳实践)
- [系统监控与告警](#系统监控与告警)
- [故障排查与诊断](#故障排查与诊断)
- [安全加固配置](#安全加固配置)
- [自动化运维脚本](#自动化运维脚本)

---

<!-- chunk: Linux 内核架构 -->## Linux 内核架构

## 内核层次结构

```
┌─────────────────────────────────────────────────────────────────┐
│                         用户空间                                 │
│   应用程序 │ Shell │ 库 (glibc) │ 系统工具                       │
└─────────────────────────────────┬───────────────────────────────┘
                                  │ 系统调用 (syscall)
┌─────────────────────────────────┴───────────────────────────────┐
│                         内核空间                                 │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  系统调用接口 (System Call Interface)                     │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌────────────┐ ┌────────────┐ ┌────────────┐ ┌────────────┐  │
│  │ 进程管理    │ │ 内存管理    │ │ 文件系统    │ │ 网络协议栈  │  │
│  │ (Scheduler)│ │ (MM)       │ │ (VFS)      │ │ (TCP/IP)   │  │
│  └────────────┘ └────────────┘ └────────────┘ └────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  设备驱动程序 (Device Drivers)                            │  │
│  │  块设备 │ 字符设备 │ 网络设备                              │  │
│  └──────────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  硬件抽象层 (HAL)                                         │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
                                  │
┌─────────────────────────────────┴───────────────────────────────┐
│                          硬件                                    │
│   CPU │ 内存 │ 磁盘 │ 网卡 │ GPU │ 其他外设                      │
└─────────────────────────────────────────────────────────────────┘
```

## 内核子系统

| 子系统 | 功能 | 核心组件 |
|:---|:---|:---|
| **进程管理** | 进程调度、创建、终止 | CFS 调度器、fork/exec |
| **内存管理** | 虚拟内存、分页、缓存 | 页表、slab 分配器 |
| **文件系统** | VFS、各种文件系统 | ext4、xfs、btrfs |
| **网络子系统** | TCP/IP 协议栈 | socket、netfilter |
| **设备驱动** | 硬件抽象、驱动框架 | 块设备、字符设备 |
| **安全模块** | 访问控制 | SELinux、AppArmor |

## 内核版本

| 版本系列 | LTS 支持 | 主要特性 |
|:---|:---|:---|
| **5.4** | 2024-12 | 基础稳定版本 |
| **5.10** | 2026-12 | exFAT、稳定改进 |
| **5.15** | 2026-10 | NTFS 驱动、改进 |
| **6.1** | 2026-12 | Rust 支持、性能提升 |
| **6.6** | 2026-12+ | 持续改进 |

---

<!-- chunk: 系统启动过程 -->## 系统启动过程

## 启动流程

```
电源开启
    │
    ▼
┌───────────────┐
│  BIOS/UEFI    │  POST 自检、硬件初始化
└───────┬───────┘
        │
        ▼
┌───────────────┐
│  Bootloader   │  GRUB2: 加载内核和 initramfs
└───────┬───────┘
        │
        ▼
┌───────────────┐
│  Linux Kernel │  解压、初始化硬件和驱动
└───────┬───────┘
        │
        ▼
┌───────────────┐
│  initramfs    │  临时根文件系统、挂载真实根
└───────┬───────┘
        │
        ▼
┌───────────────┐
│  systemd      │  PID 1、服务管理、目标切换
│  (init)       │
└───────┬───────┘
        │
        ▼
┌───────────────┐
│  用户空间服务  │  网络、登录、应用服务
└───────────────┘
```

## GRUB2 配置

```bash
# 配置文件
/etc/default/grub          # 主配置
/boot/grub2/grub.cfg       # 生成的配置 (勿直接编辑)

# 常用参数
GRUB_TIMEOUT=5
GRUB_CMDLINE_LINUX="quiet rhgb"
GRUB_DISABLE_RECOVERY="true"

# 重新生成配置
grub2-mkconfig -o /boot/grub2/grub.cfg
```

## 内核启动参数

| 参数 | 说明 | 示例 |
|:---|:---|:---|
| `quiet` | 减少启动信息 | `quiet` |
| `init=` | 指定 init 程序 | `init=/bin/bash` |
| `root=` | 根文件系统 | `root=/dev/sda1` |
| `single` / `1` | 单用户模式 | `single` |
| `selinux=0` | 禁用 SELinux | `selinux=0` |
| `mem=` | 限制内存 | `mem=4G` |

---

<!-- chunk: systemd 服务管理 -->## systemd 服务管理

## 常用命令

| 命令 | 说明 |
|:---|:---|
| `systemctl start <unit>` | 启动服务 |
| `systemctl stop <unit>` | 停止服务 |
| `systemctl restart <unit>` | 重启服务 |
| `systemctl reload <unit>` | 重载配置 |
| `systemctl enable <unit>` | 开机自启 |
| `systemctl disable <unit>` | 禁止自启 |
| `systemctl status <unit>` | 查看状态 |
| `systemctl is-active <unit>` | 检查是否运行 |
| `systemctl list-units` | 列出所有单元 |
| `systemctl daemon-reload` | 重载 unit 文件 |

## Unit 文件

```ini
# /etc/systemd/system/myapp.service
[Unit]
Description=My Application
After=network.target
Wants=network-online.target

[Service]
Type=simple
User=appuser
Group=appgroup
WorkingDirectory=/opt/myapp
ExecStart=/opt/myapp/bin/server
ExecReload=/bin/kill -HUP $MAINPID
Restart=on-failure
RestartSec=5
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
```

## Service 类型

| 类型 | 说明 |
|:---|:---|
| `simple` | 默认，ExecStart 进程即主进程 |
| `forking` | fork 后父进程退出 |
| `oneshot` | 一次性任务 |
| `notify` | 服务就绪时通知 systemd |
| `dbus` | 注册 D-Bus 后就绪 |

## 日志查看

```bash
# 查看服务日志
journalctl -u myapp.service

# 实时跟踪
journalctl -u myapp.service -f

# 最近 N 行
journalctl -u myapp.service -n 100

# 本次启动日志
journalctl -u myapp.service -b
```

---

<!-- chunk: 内核参数调优 -->## 内核参数调优

## sysctl 配置

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `sysctl -w`：实时修改内核参数，全局生效

```bash
# 查看参数
sysctl -a | grep <pattern>
sysctl net.ipv4.ip_forward

# 临时修改
sysctl -w net.ipv4.ip_forward=1

# 永久配置
echo "net.ipv4.ip_forward = 1" >> /etc/sysctl.d/99-custom.conf
sysctl --system
```

## 常用内核参数

## 网络参数

| 参数 | 说明 | 推荐值 |
|:---|:---|:---|
| `net.ipv4.ip_forward` | IP 转发 | 1 (容器/路由) |
| `net.core.somaxconn` | 监听队列 | 65535 |
| `net.ipv4.tcp_max_syn_backlog` | SYN 队列 | 65535 |
| `net.core.netdev_max_backlog` | 网络设备队列 | 65535 |
| `net.ipv4.tcp_fin_timeout` | FIN 超时 | 15 |
| `net.ipv4.tcp_tw_reuse` | TIME_WAIT 重用 | 1 |

## 内存参数

| 参数 | 说明 | 推荐值 |
|:---|:---|:---|
| `vm.swappiness` | swap 倾向 | 10-30 |
| `vm.dirty_ratio` | 脏页比例 | 20 |
| `vm.dirty_background_ratio` | 后台刷盘比例 | 5 |
| `vm.overcommit_memory` | 内存过量分配 | 0/1/2 |

## 文件系统参数

| 参数 | 说明 | 推荐值 |
|:---|:---|:---|
| `fs.file-max` | 最大文件数 | 2097152 |
| `fs.inotify.max_user_watches` | inotify 监控数 | 524288 |

## 生产配置示例

```bash
# /etc/sysctl.d/99-kubernetes.conf
net.bridge.bridge-nf-call-iptables = 1
net.bridge.bridge-nf-call-ip6tables = 1
net.ipv4.ip_forward = 1
net.ipv4.conf.all.forwarding = 1

vm.swappiness = 10
vm.max_map_count = 262144

fs.file-max = 2097152
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 8192
```

---

<!-- chunk: 内核模块管理 -->## 内核模块管理

## 模块操作

```bash
# 查看已加载模块
lsmod

# 加载模块
modprobe br_netfilter
modprobe overlay

# 卸载模块
modprobe -r <module>

# 模块信息
modinfo br_netfilter

# 开机加载
echo "br_netfilter" >> /etc/modules-load.d/kubernetes.conf
```

## 容器相关模块

| 模块 | 用途 |
|:---|:---|
| `overlay` | OverlayFS 存储驱动 |
| `br_netfilter` | 网桥 iptables 过滤 |
| `ip_vs` | IPVS 负载均衡 |
| `ip_vs_rr` | IPVS 轮询调度 |
| `nf_conntrack` | 连接跟踪 |

---

<!-- chunk: 主流发行版对比 -->## 主流发行版对比

| 发行版 | 包管理 | 生命周期 | 适用场景 |
|:---|:---|:---|:---|
| **RHEL/CentOS Stream** | dnf/yum | 10 年 | 企业生产 |
| **Ubuntu LTS** | apt | 5 年 | 云/容器 |
| **Debian** | apt | 5 年 | 稳定性优先 |
| **SUSE/openSUSE** | zypper | 10+ 年 | 企业生产 |
| **Alpine** | apk | 2 年 | 容器基础镜像 |
| **Fedora** | dnf | 1 年 | 新技术验证 |

## 容器推荐

| 场景 | 推荐发行版 |
|:---|:---|
| **容器运行时** | RHEL CoreOS, Flatcar, Ubuntu |
| **容器基础镜像** | Alpine, Distroless, Debian-slim |
| **K8s 节点** | Ubuntu, RHEL, Flatcar |

---

<!-- chunk: 生产环境最佳实践 -->## 生产环境最佳实践

## 系统基线配置

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# Linux 生产环境基线配置脚本

# 系统时间同步
timedatectl set-ntp true
chronyc sources

# 内核参数优化
cat > /etc/sysctl.d/99-production.conf << 'EOF'
# 网络性能优化
net.core.somaxconn = 65535
net.core.netdev_max_backlog = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.ipv4.ip_forward = 1

# 内存管理优化
vm.swappiness = 10
vm.max_map_count = 262144
vm.dirty_ratio = 20
vm.dirty_background_ratio = 5

# 文件系统优化
fs.file-max = 2097152
fs.inotify.max_user_watches = 524288
fs.inotify.max_user_instances = 8192

# 安全加固
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.all.send_redirects = 0
net.ipv4.conf.all.log_martians = 1
kernel.dmesg_restrict = 1
kernel.kptr_restrict = 2
EOF

sysctl --system

# 资源限制配置
cat > /etc/security/limits.d/99-production.conf << 'EOF'
* soft nofile 65536
* hard nofile 65536
* soft nproc 65536
* hard nproc 65536
root soft nofile 65536
root hard nofile 65536
EOF

# 禁用不必要的服务
systemctl disable --now firewalld NetworkManager
systemctl mask firewalld

# 启用必要的服务
systemctl enable --now chronyd rsyslog
```
## 内核版本管理策略

| 场景 | 推荐策略 | 说明 |
|:---|:---|:---|
| **Kubernetes 节点** | LTS 版本 + 定期更新 | 稳定性优先，每季度评估更新 |
| **数据库服务器** | 长期支持版本 | 优先稳定性，避免频繁变更 |
| **Web 应用服务器** | 最新稳定版本 | 平衡性能和稳定性 |
| **开发测试环境** | 最新版本 | 获取最新特性和安全补丁 |

## 系统分区规划

```bash
# 生产环境推荐分区方案
# /boot     1GB    - 启动分区
# /         20GB   - 根分区  
# /var      30GB   - 日志和变量数据
# /var/log  20GB   - 系统日志专用
# /home     10GB   - 用户家目录
# /tmp      10GB   - 临时文件
# swap      内存大小 - 交换分区
```

---

<!-- chunk: 系统监控与告警 -->## 系统监控与告警

## 核心监控指标

| 指标类别 | 关键指标 | 告警阈值 | 监控工具 |
|:---|:---|:---|:---|
| **CPU** | 使用率、负载、上下文切换 | >80%, >CPU核数*2 | top, sar, [[Prometheus|Prometheus]] |
| **内存** | 使用率、swap使用、cache/buffer | >85%, >10% | free, vmstat |
| **磁盘** | 使用率、IOPS、延迟 | >85%, >50ms | iostat, df |
| **网络** | 带宽使用、连接数、错误包 | >80%, >1% | ss, ifstat |
| **系统** | 进程数、文件句柄、登录用户 | 异常增长 | ps, lsof |

## Prometheus Node Exporter 配置

```yaml
# /etc/prometheus/node_exporter.yml
scrape_configs:
  - job_name: 'node'
    static_configs:
      - targets: ['localhost:9100']
    scrape_interval: 15s
    metrics_path: /metrics
    
# 关键告警规则
groups:
- name: node_alerts
  rules:
  - alert: HighCPUUsage
    expr: 100 - (avg by(instance) (irate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 80
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Instance {{ $labels.instance }} CPU usage is above 80%"
      
  - alert: HighMemoryUsage  
    expr: (node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes) / node_memory_MemTotal_bytes * 100 > 85
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Instance {{ $labels.instance }} memory usage is above 85%"
```

## Grafana 仪表板配置

```json
{
  "dashboard": {
    "title": "Linux Production Monitoring",
    "panels": [
      {
        "title": "System Overview",
        "type": "graph",
        "targets": [
          {
            "expr": "100 - (avg by(instance) (irate(node_cpu_seconds_total{mode=\"idle\"}[5m])) * 100)",
            "legendFormat": "CPU Usage"
          },
          {
            "expr": "(node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes) / node_memory_MemTotal_bytes * 100",
            "legendFormat": "Memory Usage"
          }
        ]
      }
    ]
  }
}
```

---

<!-- chunk: 故障排查与诊断 -->## 故障排查与诊断

## 系统健康检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 系统健康检查脚本 - production_health_check.sh

LOG_FILE="/var/log/system_health_$(date +%Y%m%d).log"
EMAIL="admin@company.com"

# 检查函数
check_disk_usage() {
    echo "=== 磁盘使用情况 ===" >> $LOG_FILE
    df -h | grep -v tmpfs >> $LOG_FILE
    df -h | awk '$5+0 > 85 {print "警告: "$6" 使用率超过85%: "$5}' >> $LOG_FILE
}

check_memory_usage() {
    echo "=== 内存使用情况 ===" >> $LOG_FILE
    free -h >> $LOG_FILE
    mem_usage=$(free | awk 'NR==2{printf "%.0f", $3*100/$2}')
    if [ $mem_usage -gt 85 ]; then
        echo "警告: 内存使用率 ${mem_usage}%" >> $LOG_FILE
    fi
}

check_cpu_load() {
    echo "=== CPU 负载 ===" >> $LOG_FILE
    uptime >> $LOG_FILE
    load_avg=$(uptime | awk -F'load average:' '{print $2}' | awk '{print $1}' | sed 's/,//')
    cpu_cores=$(nproc)
    if (( $(echo "$load_avg > $((cpu_cores * 2))" | bc -l) )); then
        echo "警告: 系统负载过高: ${load_avg}" >> $LOG_FILE
    fi
}

check_services() {
    echo "=== 关键服务状态 ===" >> $LOG_FILE
    services=("sshd" "chronyd" "rsyslog")
    for service in "${services[@]}"; do
        if systemctl is-active --quiet $service; then
            echo "$service: 运行正常" >> $LOG_FILE
        else
            echo "警告: $service 服务异常" >> $LOG_FILE
        fi
    done
}

check_network() {
    echo "=== 网络连接 ===" >> $LOG_FILE
    ss -s >> $LOG_FILE
    echo "监听端口:" >> $LOG_FILE
    ss -tlnp >> $LOG_FILE
}

# 执行检查
{
    echo "系统健康检查报告 - $(date)"
    echo "========================================"
    check_disk_usage
    check_memory_usage
    check_cpu_load
    check_services
    check_network
    echo "========================================"
} > $LOG_FILE

# 发送告警邮件
if grep -q "警告" $LOG_FILE; then
    mail -s "系统健康检查告警 - $(hostname)" $EMAIL < $LOG_FILE
fi
```
## 内核崩溃诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启用内核崩溃转储
echo "kernel.core_pattern = /var/crash/core.%e.%p.%h.%t" >> /etc/sysctl.conf
sysctl -p

# 安装 crash 工具
yum install crash kexec-tools  # RHEL/CentOS
apt install crash kdump-tools   # Ubuntu/Debian

# 配置 kdump
systemctl enable kdump
systemctl start kdump

# 分析崩溃转储
crash /var/crash/vmcore /usr/lib/debug/lib/modules/$(uname -r)/vmlinux
```
## 系统性能瓶颈分析流程

```
1. 初步评估
   ├── uptime 查看负载
   ├── dmesg 检查内核消息
   └── top/htop 查看资源使用

2. 深入分析
   ├── CPU瓶颈: mpstat, perf top
   ├── 内存瓶颈: vmstat, free, slabtop
   ├── I/O瓶颈: iostat, iotop
   └── 网络瓶颈: ss, ifstat, tcpdump

3. 根因定位
   ├── 进程分析: strace, ltrace
   ├── 系统调用: perf record
   └── 火焰图分析: perf script + flamegraph
```

---

<!-- chunk: 安全加固配置 -->## 安全加固配置

## SELinux 生产配置

```bash
# 检查 SELinux 状态
getenforce
sestatus

# 生产环境推荐配置
cat > /etc/selinux/config << 'EOF'
SELINUX=enforcing
SELINUXTYPE=targeted
SETLOCALDEFS=0
EOF

# 常用 SELinux 管理命令
# 查看布尔值
getsebool -a | grep httpd

# 设置布尔值
setsebool -P httpd_can_network_connect on
setsebool -P nis_enabled off

# 管理文件上下文
semanage fcontext -a -t httpd_sys_content_t "/web(/.*)?"
restorecon -Rv /web

# 查看端口标签
semanage port -l | grep http
```

## 系统审计配置

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装审计工具
yum install audit audispd-plugins  # RHEL/CentOS
apt install auditd                  # Ubuntu/Debian

# 核心审计规则
cat > /etc/audit/rules.d/production.rules << 'EOF'
# 用户和组管理
-w /etc/passwd -p wa -k identity
-w /etc/shadow -p wa -k identity
-w /etc/group -p wa -k identity

# 系统认证
-w /etc/gshadow -p wa -k identity
-w /etc/security/opasswd -p wa -k identity

# SSH 配置
-w /etc/ssh/sshd_config -p wa -k ssh

# sudo 配置
-w /etc/sudoers -p wa -k priv_esc
-w /etc/sudoers.d/ -p wa -k priv_esc

# 系统启动
-w /etc/inittab -p wa -k init
-w /etc/grub.conf -p wa -k boot
-w /etc/grub.d/ -p wa -k boot

# 网络配置
-w /etc/network/ -p wa -k network
-w /etc/sysconfig/network-scripts/ -p wa -k network

# 关键系统调用
-a always,exit -F arch=b64 -S execve -k exec
-a always,exit -F arch=b32 -S execve -k exec
-a always,exit -F arch=b64 -S open,openat,creat -F dir=/etc -k etc_access
EOF

# 重启审计服务
systemctl restart auditd

# 查看审计日志
ausearch -k identity --start recent
aureport --summary
```
## 防火墙生产配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# firewalld 生产配置
systemctl enable firewalld
systemctl start firewalld

# 基础安全规则
firewall-cmd --permanent --add-service=ssh
firewall-cmd --permanent --remove-service=dhcpv6-client
firewall-cmd --permanent --remove-service=cockpit

# 端口管理
firewall-cmd --permanent --add-port=80/tcp
firewall-cmd --permanent --add-port=443/tcp

# IP 白名单
firewall-cmd --permanent --add-rich-rule='rule family="ipv4" source address="192.168.1.0/24" port protocol="tcp" port="22" accept'

# 拒绝策略
firewall-cmd --permanent --set-target=DROP

# 生效配置
firewall-cmd --reload
```
---

<!-- chunk: 自动化运维脚本 -->## 自动化运维脚本

## 系统批量管理脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 批量系统管理脚本 - batch_system_manager.sh

HOSTS_FILE="/etc/ansible/hosts"
LOG_DIR="/var/log/batch_ops"

# 创建日志目录
mkdir -p $LOG_DIR

# 批量执行命令
batch_execute() {
    local cmd="$1"
    local log_file="$LOG_DIR/batch_$(date +%Y%m%d_%H%M%S).log"
    
    echo "执行命令: $cmd" | tee -a $log_file
    echo "执行时间: $(date)" | tee -a $log_file
    echo "================================" | tee -a $log_file
    
    ansible all -i $HOSTS_FILE -m shell -a "$cmd" | tee -a $log_file
    
    echo "================================" | tee -a $log_file
    echo "执行完成: $(date)" | tee -a $log_file
}

# 系统更新
system_update() {
    batch_execute "yum update -y"  # RHEL/CentOS
    # batch_execute "apt update && apt upgrade -y"  # Ubuntu/Debian
}

# 安全补丁安装
security_patch() {
    batch_execute "yum update --security -y"
}

# 服务状态检查
service_check() {
    batch_execute "systemctl list-units --type=service --state=running | head -20"
}

# 磁盘清理
disk_cleanup() {
    batch_execute "find /var/log -name '*.log' -mtime +30 -delete"
    batch_execute "journalctl --vacuum-time=30d"
}

# 根据参数执行相应操作
case "$1" in
    "update")
        system_update
        ;;
    "patch")
        security_patch
        ;;
    "check")
        service_check
        ;;
    "cleanup")
        disk_cleanup
        ;;
    *)
        echo "用法: $0 {update|patch|check|cleanup}"
        echo "  update  - 系统更新"
        echo "  patch   - 安全补丁"
        echo "  check   - 服务检查"
        echo "  cleanup - 磁盘清理"
        exit 1
        ;;
esac
```
## 配置备份脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 系统配置备份脚本 - config_backup.sh

BACKUP_DIR="/backup/system_config"
DATE=$(date +%Y%m%d_%H%M%S)

# 创建备份目录
mkdir -p $BACKUP_DIR/$DATE

# 备份关键配置文件
CONFIG_FILES=(
    "/etc/passwd"
    "/etc/group"
    "/etc/shadow"
    "/etc/sudoers"
    "/etc/ssh/sshd_config"
    "/etc/sysctl.conf"
    "/etc/security/limits.conf"
    "/etc/fstab"
    "/etc/hosts"
    "/etc/resolv.conf"
)

for file in "${CONFIG_FILES[@]}"; do
    if [ -f "$file" ]; then
        cp "$file" "$BACKUP_DIR/$DATE/"
        echo "已备份: $file"
    fi
done

# 备份服务配置
mkdir -p "$BACKUP_DIR/$DATE/services"
systemctl list-unit-files --type=service --state=enabled | awk '{print $1}' | while read service; do
    if [ -f "/etc/systemd/system/$service" ]; then
        cp "/etc/systemd/system/$service" "$BACKUP_DIR/$DATE/services/"
    elif [ -f "/usr/lib/systemd/system/$service" ]; then
        cp "/usr/lib/systemd/system/$service" "$BACKUP_DIR/$DATE/services/"
    fi
done

# 备份网络配置
mkdir -p "$BACKUP_DIR/$DATE/network"
cp -r /etc/sysconfig/network-scripts/ "$BACKUP_DIR/$DATE/network/"

# 创建备份清单
cat > "$BACKUP_DIR/$DATE/manifest.txt" << EOF
备份时间: $(date)
主机名: $(hostname)
内核版本: $(uname -r)
系统版本: $(cat /etc/os-release | grep PRETTY_NAME | cut -d'"' -f2)
备份文件列表:
$(find "$BACKUP_DIR/$DATE" -type f | sed "s|$BACKUP_DIR/$DATE||")
EOF

# 压缩备份
tar -czf "$BACKUP_DIR/system_config_$DATE.tar.gz" -C "$BACKUP_DIR" "$DATE"

# 清理旧备份 (保留最近7天)
find $BACKUP_DIR -name "system_config_*.tar.gz" -mtime +7 -delete

echo "配置备份完成: $BACKUP_DIR/system_config_$DATE.tar.gz"
```
---

<!-- chunk: 与 Kubernetes 的关系 -->## 与 Kubernetes 的关系

## systemd 在 K8s 中的角色

Kubernetes 的 kubelet 就是作为 systemd 服务运行的。每个 Kubelet 管理的容器最终也由容器运行时（containerd/CRI-O）通过 systemd 或 cgroupfs 管理 cgroup。理解 systemd 对于管理 K8s 节点至关重要。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubelet 作为 systemd 服务
systemctl status kubelet
journalctl -u kubelet -f

# containerd 作为 systemd 服务
systemctl status containerd
journalctl -u containerd -f

# 查看容器 slice 层级
systemd-cgls                          # 查看完整 cgroup 树
systemd-cgls | grep -A10 kubepods     # K8s Pod 的 cgroup
```
## cgroups 与 K8s 资源管理

Kubernetes 通过 cgroups 实现资源管理。kubelet 支持 cgroupfs 和 systemd 两种 cgroup 驱动：

| cgroup 驱动 | 管理方式 | 配置 | 推荐 |
|:---|:---|:---|:---|
| **systemd** | 通过 systemd 管理单位 | kubelet: `--cgroup-driver=systemd` | 推荐（与发行版默认一致） |
| **cgroupfs** | 直接操作 cgroup 文件系统 | kubelet: `--cgroup-driver=cgroupfs` | 仅在特定场景使用 |

```
# 🟢 低风险：只读/信息收集，通常无副作用
K8s 节点 cgroup 层级 (systemd 驱动):

/sys/fs/cgroup/
├── kubepods.slice/
│   ├── kubepods-pod<pod-uid>.slice/
│   │   ├── cri-containerd-<id>.scope/
│   │   │   ├── cpu.max         ← resources.limits.cpu
│   │   │   ├── memory.max      ← resources.limits.memory
│   │   │   └── cgroup.procs    ← 容器内所有进程
│   │   └── cri-containerd-<id>.scope/
│   └── kubepods-pod<pod-uid>.slice/
├── system.slice/
│   ├── containerd.service
│   ├── kubelet.service
│   └── docker.service
└── user.slice/
```
## 内核模块与 K8s

Kubernetes 依赖以下内核模块才能正常工作：

```bash
# 必需的内核模块
modprobe overlay                    # OverlayFS - 容器存储驱动
modprobe br_netfilter               # 网桥 netfilter - kube-proxy 必需
modprobe ip_vs                      # IPVS - kube-proxy IPVS 模式
modprobe ip_vs_rr                   # IPVS 轮询调度
modprobe ip_vs_wrr                  # IPVS 加权轮询
modprobe ip_vs_sh                   # IPVS 源地址哈希
modprobe nf_conntrack               # 连接跟踪 - Service 会话管理

# 永久加载
cat > /etc/modules-load.d/k8s.conf << 'EOF'
overlay
br_netfilter
ip_vs
ip_vs_rr
ip_vs_wrr
ip_vs_sh
nf_conntrack
EOF

# 必需的内核参数
cat > /etc/sysctl.d/99-kubernetes.conf << 'EOF'
net.bridge.bridge-nf-call-iptables  = 1
net.bridge.bridge-nf-call-ip6tables = 1
net.ipv4.ip_forward                 = 1
EOF
sysctl --system
```

---

<!-- chunk: 最佳实践 -->## 最佳实践

1. **使用 LTS 内核**: 生产环境使用长期支持版本（5.10, 5.15, 6.1）
2. **统一 cgroup 驱动**: kubelet 和容器运行时使用相同的 cgroup 驱动（推荐 systemd）
3. **预加载内核模块**: 将 K8s 所需的内核模块加入 /etc/modules-load.d/
4. **合理规划分区**: /var/lib/containerd 和 /var/lib/kubelet 需要足够空间
5. **启用 kdump**: 配置内核崩溃转储，便于分析内核 panic
6. **定期更新内核**: 保持内核安全补丁更新，但避免跨大版本升级

---

<!-- chunk: 故障排查 -->## 故障排查

## 内核相关问题

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 内核 panic 后分析
crash /var/crash/vmcore /usr/lib/debug/lib/modules/$(uname -r)/vmlinux

# 查看内核日志
dmesg -T | tail -50
journalctl -k --since "1 hour ago"

# 查看内核崩溃历史
last reboot | head -20

# 检查内核模块冲突
lsmod | sort -k 2 -n -r | head
cat /proc/modules | wc -l
```
---

<!-- chunk: 相关文档 -->## 相关文档

- [02-linux-process-management](32-发布/package/2026-07-02_18-29/corpus/supporting/domain-17-system-foundation/01-linux/01-linux-process-management.md) - 进程管理
- [03-linux-filesystem-deep-dive](02-linux-filesystem-deep-dive.md) - 文件系统
- [08-linux-container-fundamentals](07-linux-container-fundamentals.md) - 容器基础

---

## See Also

- 09-linux-operations-basics
- 99-linux-commands-reference
- 02-linux-process-management
- 03-linux-filesystem-deep-dive

## Related

- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
