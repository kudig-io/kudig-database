---
title: 07 - Linux 安全加固与合规管理：生产环境安全运维专家指南
description: '# 07 - Linux 安全加固与合规管理：生产环境安全运维专家指南'
category: linux
tags:
- linux
- system
- kernel
- docker
- opa
- falco
- ceph
- hpa
- networkpolicy
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 运维工程师
- SRE
- 系统管理员
estimated_read_time: 5min
intent_queries:
- Linux 安全加固与合规管理：生产环境安全运维专家指南 是什么
- 如何 Linux 安全加固与合规管理：生产环境安全运维专家指南
- Kubernetes 14 linux 最佳实践
trigger_keywords:
- Linux
- 安全加固与合规管理：生产环境安全运维专家指南
- linux
prerequisites:
- kubectl-basics
- cloud-provider-basics
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
created: "2026-05-23"
---

# 07 - Linux 安全加固与合规管理：生产环境安全运维专家指南

> **适用版本**: Linux Kernel 5.x/6.x | **最后更新**: 2026-02 | **作者**: Allen Galler (allengaller@gmail.com)

---

<!-- chunk: 概述 -->## 概述


安全是生产环境 Linux 系统运维的基石。在 Kubernetes 环境中，一个被攻破的节点意味着攻击者可能获取集群中所有工作负载的访问权限。本文档从内核安全机制到应用层防护，全面深入地讲解 Linux 安全加固的各个方面，包括用户权限管理、SSH 安全、PAM 认证、SELinux/AppArmor 强制访问控制、审计日志、容器安全特性（Namespaces、cgroups、Capabilities、Seccomp）以及与 Kubernetes 安全策略（PodSecurityPolicy/PodSecurityStandards、NetworkPolicy）的紧密关联。掌握这些内容是构建安全可靠的容器平台基础设施的必要前提。

---

<!-- chunk: 核心概念详解 -->## 核心概念详解

## Linux 安全模型

Linux 安全模型是一个多层防御体系，从外到内逐层保护系统资源：

```
┌─────────────────────────────────────────────────────────────────┐
│                     Linux 安全模型层次                            │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  第一层: 网络安全                                          │  │
│  │  防火墙 (iptables/nftables/firewalld)                     │  │
│  │  网络隔离 (VPC, Security Groups)                          │  │
│  │  入侵检测 (Snort, Suricata)                               │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  第二层: 认证与授权                                        │  │
│  │  用户认证 (PAM, LDAP, SSSD)                               │  │
│  │  权限控制 (DAC - 自主访问控制)                              │  │
│  │  提权控制 (sudo, PolicyKit)                                │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  第三层: 强制访问控制 (MAC)                                 │  │
│  │  SELinux / AppArmor                                        │  │
│  │  基于安全策略的细粒度访问控制                               │  │
│  │  即使 root 用户也受约束                                    │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  第四层: 内核安全                                          │  │
│  │  Capabilities (权限细分)                                   │  │
│  │  Seccomp (系统调用过滤)                                    │  │
│  │  Namespaces (资源隔离)                                     │  │
│  │  cgroups (资源限制)                                        │  │
│  └──────────────────────────────────────────────────────────┘  │
│                                                                  │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  第五层: 审计与监控                                        │  │
│  │  auditd (系统审计)                                         │  │
│  │  AIDE (文件完整性)                                         │  │
│  │  Falco (运行时安全)                                        │  │
│  │  日志分析 (ELK, Splunk)                                    │  │
│  └──────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 用户与权限管理

## 用户管理基础

Linux 通过 /etc/passwd、/etc/shadow、/etc/group 三个文件管理用户和组信息。每个用户有唯一的 UID，系统通过 UID 而非用户名来识别身份。

```bash
# /etc/passwd 文件格式
# 用户名:x:UID:GID:描述:家目录:Shell
root:x:0:0:root:/root:/bin/bash
nobody:x:65534:65534:Nobody:/:/sbin/nologin

# /etc/shadow 文件格式 (需要 root 权限查看)
# 用户名:加密密码:最后修改日期:最小修改间隔:最大有效期:警告期:不活动期:过期日期
root:$6$salt$hash:19000:0:99999:7:::

# 用户管理命令
useradd -m -s /bin/bash -u 1001 username     # 创建用户
usermod -aG docker username                    # 添加到组
userdel -r username                            # 删除用户
passwd username                                # 设置密码

# 查看用户信息
id username                                    # UID/GID/组
groups username                                # 所属组
finger username                                # 详细信息

# 切换用户
su - username                                  # 切换用户
sudo -u username command                       # 以指定用户执行
```

## 密码策略

```bash
# /etc/login.defs - 密码策略全局配置
PASS_MAX_DAYS   90      # 密码最大有效期 (天)
PASS_MIN_DAYS   7       # 最小修改间隔 (天)
PASS_MIN_LEN    12      # 最小长度
PASS_WARN_AGE   14      # 过期前警告天数
UMASK           027     # 默认 umask
ENCRYPT_METHOD  SHA512  # 加密算法

# /etc/security/pwquality.conf - 密码质量要求
minlen = 12              # 最小长度
dcredit = -1             # 至少 1 个数字
ucredit = -1             # 至少 1 个大写字母
lcredit = -1             # 至少 1 个小写字母
ocredit = -1             # 至少 1 个特殊字符
minclass = 3             # 至少包含 3 种字符类型
maxrepeat = 3            # 最多连续重复 3 个相同字符
difok = 5                # 与旧密码至少 5 个字符不同

# 设置密码过期
chage -M 90 username                # 90 天后过期
chage -W 14 username                # 提前 14 天警告
chage -l username                   # 查看策略
chage -E $(date -d "+90 days" +%Y-%m-%d) username  # 设置过期日期

# 锁定/解锁账户
passwd -l username                  # 锁定
passwd -u username                  # 解锁
usermod -L username                 # 锁定 (另一种方式)
usermod -U username                 # 解锁
usermod -s /sbin/nologin username   # 禁止登录
```

## sudo 权限控制

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# /etc/sudoers 或 /etc/sudoers.d/ 目录下的文件
# 使用 visudo 编辑，自动检查语法

# 基本格式: 用户 主机=(以谁身份) 命令

# 管理员完全权限
admin ALL=(ALL) NOPASSWD: ALL

# 限制为特定命令
webadmin ALL=(ALL) /bin/systemctl restart nginx
webadmin ALL=(ALL) /bin/systemctl reload nginx

# 限制为特定服务的所有管理命令
operator ALL=(ALL) /usr/bin/docker *, /usr/bin/kubectl *

# 组权限 (%groupname)
%ops-admin ALL=(ALL) NOPASSWD: /usr/bin/systemctl, /usr/bin/docker, /usr/bin/kubectl

# 禁止使用 sudo su
%admins ALL=(ALL) ALL, !/bin/su, !/bin/bash

# sudo 日志配置
Defaults logfile="/var/log/sudo.log"
Defaults log_input, log_output
Defaults!SUDOREPLAY !log_output
Defaults timestamp_timeout=15          # sudo 密码缓存时间 (分钟)
Defaults passwd_tries=3                # 密码尝试次数
```

---

## SSH 安全配置

SSH 是 Linux 服务器最主要的远程管理方式，也是攻击者最常尝试入侵的入口。严格的安全配置至关重要。

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# /etc/ssh/sshd_config - SSH 服务端安全配置

# ===== 基本安全 =====
Port 22022                          # 修改默认端口（减少扫描）
Protocol 2                          # 仅使用 SSHv2
PermitRootLogin no                  # 禁止 root 直接登录

# ===== 认证方式 =====
PasswordAuthentication no           # 禁用密码认证
PubkeyAuthentication yes            # 启用公钥认证
ChallengeResponseAuthentication no  # 禁用挑战响应
GSSAPIAuthentication no             # 禁用 GSSAPI
KerberosAuthentication no           # 禁用 Kerberos
HostbasedAuthentication no          # 禁用基于主机的认证

# ===== 访问控制 =====
AllowUsers admin deploy             # 仅允许指定用户
AllowGroups sshusers                # 或使用组控制
DenyUsers guest test                # 明确拒绝

# ===== 连接限制 =====
MaxAuthTries 3                      # 最大认证尝试
MaxSessions 5                       # 最大会话数
LoginGraceTime 30                   # 登录超时 (秒)
MaxStartups 10:30:60               # 未认证连接限制

# ===== 超时配置 =====
ClientAliveInterval 300             # 保活探测间隔 (秒)
ClientAliveCountMax 2               # 保活探测次数

# ===== 加密算法 =====
# 仅使用强加密算法
KexAlgorithms curve25519-sha256,curve25519-sha256@libssh.org,diffie-hellman-group16-sha512
Ciphers chacha20-poly1305@openssh.com,aes256-gcm@openssh.com,aes128-gcm@openssh.com
MACs hmac-sha2-512-etm@openssh.com,hmac-sha2-256-etm@openssh.com
HostKeyAlgorithms ssh-ed25519,rsa-sha2-512,rsa-sha2-256

# ===== 其他安全 =====
PermitEmptyPasswords no             # 禁止空密码
X11Forwarding no                    # 禁止 X11 转发
AllowTcpForwarding no               # 禁止 TCP 转发
AllowAgentForwarding no             # 禁止 Agent 转发
PermitTunnel no                     # 禁止隧道
PrintMotd yes                       # 显示 MOTD
Banner /etc/issue.net               # 登录前横幅

# 重启生效
systemctl restart sshd
```

## SSH 密钥管理

```bash
# 生成密钥对 (推荐 ed25519)
ssh-keygen -t ed25519 -C "user@host" -f ~/.ssh/id_ed25519

# 生成 RSA 密钥 (兼容性好)
ssh-keygen -t rsa -b 4096 -C "user@host" -f ~/.ssh/id_rsa

# 复制公钥到远程主机
ssh-copy-id -i ~/.ssh/id_ed25519.pub user@host

# 手动添加公钥
cat ~/.ssh/id_ed25519.pub | ssh user@host "mkdir -p ~/.ssh && cat >> ~/.ssh/authorized_keys"

# 权限设置（非常重要！）
chmod 700 ~/.ssh
chmod 600 ~/.ssh/authorized_keys
chmod 600 ~/.ssh/id_ed25519
chmod 644 ~/.ssh/id_ed25519.pub

# SSH Agent 使用
eval $(ssh-agent -s)
ssh-add ~/.ssh/id_ed25519
ssh-add -l                          # 列出已添加的密钥

# SSH 配置文件 (~/.ssh/config)
Host production-*
    IdentityFile ~/.ssh/id_ed25519
    User admin
    StrictHostKeyChecking yes
Host jump-server
    HostName bastion.example.com
    User admin
    Port 22022
    IdentityFile ~/.ssh/id_ed25519
Host internal-*
    ProxyJump jump-server
```

---

## PAM 认证配置

PAM (Pluggable Authentication Modules) 是 Linux 的可插拔认证框架，控制着系统登录、密码修改、sudo 等所有认证行为。

```bash
# PAM 配置文件位置
/etc/pam.d/                  # PAM 配置目录
/etc/pam.d/system-auth       # RHEL 系统认证
/etc/pam.d/common-auth       # Ubuntu 认证
/etc/pam.d/sshd              # SSH 认证
/etc/pam.d/login             # 本地登录

# 登录失败锁定
# /etc/pam.d/system-auth 或 /etc/pam.d/password-auth
auth required pam_faillock.so preauth silent deny=5 unlock_time=900 even_deny_root root_unlock_time=60
auth sufficient pam_unix.so try_first_pass
auth required pam_faillock.so authfail deny=5 unlock_time=900 even_deny_root root_unlock_time=60

# 查看锁定状态
faillock --user username
faillock --user username --reset       # 解锁

# 密码历史 (防止重复使用旧密码)
# /etc/pam.d/system-auth
password required pam_pwhistory.so remember=12 use_authtok

# 限制 su 命令
# /etc/pam.d/su
auth required pam_wheel.so use_uid    # 只有 wheel 组可以 su

# 限制 root 直接登录 (除了 console)
# /etc/securetty 中只保留 console 和 tty 设备
echo "console" > /etc/securetty
```

---

## SELinux 深度解析

SELinux (Security-Enhanced Linux) 是由 NSA 开发的强制访问控制 (MAC) 系统，默认在 RHEL/CentOS 中启用。它通过安全上下文标签对每个进程和文件进行细粒度访问控制。

## SELinux 模式和策略

```
┌─────────────────────────────────────────────────────────────────┐
│                     SELinux 工作模式                              │
│                                                                  │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐        │
│  │  Disabled    │   │  Permissive  │   │  Enforcing   │        │
│  │  完全禁用     │   │  宽容模式     │   │  强制模式     │        │
│  │              │   │              │   │              │        │
│  │ 不检查任何    │   │ 检查但仅记录  │   │ 检查并强制    │        │
│  │ 访问违规      │   │ 不阻止操作    │   │ 阻止违规操作  │        │
│  │              │   │              │   │              │        │
│  │ ⚠️ 不推荐    │   │ 调试/排障用  │   │ ✅ 生产推荐  │        │
│  └──────────────┘   └──────────────┘   └──────────────┘        │
│                                                                  │
│  策略类型:                                                       │
│  - targeted  (默认): 仅约束特定网络服务进程                       │
│  - mls       (多级安全): 军事级安全分级                           │
│  - minimum   (最小): 仅约束少量进程                               │
└─────────────────────────────────────────────────────────────────┘
```

## SELinux 上下文

```bash
# 查看文件安全上下文
ls -Z /var/www/html/
# 输出: system_u:object_r:httpd_sys_content_t:s0 /var/www/html/index.html
# 格式: 用户:角色:类型:级别

# 查看进程安全上下文
ps -eZ | grep nginx
ps -eZ | grep docker

# 查看所有上下文
semanage fcontext -l                  # 文件上下文规则
semanage port -l                      # 端口上下文

# 修改文件上下文
semanage fcontext -a -t httpd_sys_content_t "/web(/.*)?"
restorecon -Rv /web

# 临时修改上下文
chcon -t httpd_sys_content_t /web/index.html
chcon -R -t httpd_sys_content_t /web/

# 永久修改端口标签
semanage port -a -t http_port_t -p tcp 8080
semanage port -m -t http_port_t -p tcp 8080  # 修改
semanage port -d -p tcp 8080                  # 删除

# SELinux 布尔值 (开关)
getsebool -a                           # 查看所有布尔值
getsebool -a | grep httpd              # 过滤
setsebool -P httpd_can_network_connect on   # 允许 HTTP 连接网络
setsebool -P virt_use_nfs on                 # 允许虚拟机使用 NFS

# 常用布尔值
setsebool -P container_manage_cgroup on      # 容器管理 cgroup
setsebool -P container_use_ceph on           # 容器使用 Ceph
```

## SELinux 故障排查

```bash
# 查看 SELinux 日志
ausearch -m avc -ts recent
sealert -a /var/log/audit/audit.log

# 使用 troubleshoot 工具
yum install -y setroubleshoot-server
sealert -a /var/log/audit/audit.log | less

# 生成自定义策略模块
grep httpd /var/log/audit/audit.log | audit2allow -M myhttpd
semodule -i myhttpd.pp

# 临时设置 SELinux 模式
setenforce 0      # Permissive
setenforce 1      # Enforcing
getenforce        # 查看当前模式

# 永久配置 /etc/selinux/config
SELINUX=enforcing
SELINUXTYPE=targeted
```

---

## AppArmor

AppArmor 是 Canonical 开发的 MAC 系统，在 Ubuntu 中默认启用，通过路径匹配实现访问控制。

```bash
# 查看 AppArmor 状态
aa-status
cat /sys/kernel/security/apparmor/profiles

# 配置文件目录
/etc/apparmor.d/

# 管理配置文件
aa-enforce /etc/apparmor.d/usr.sbin.nginx    # 强制模式
aa-complain /etc/apparmor.d/usr.sbin.nginx   # 抱怨模式（仅记录）
aa-disable /etc/apparmor.d/usr.sbin.nginx    # 禁用

# AppArmor 配置文件示例
# /etc/apparmor.d/usr.sbin.nginx
#include <tunables/global>

/usr/sbin/nginx {
  #include <abstractions/base>
  #include <abstractions/nameservice>

  /etc/nginx/** r,
  /var/log/nginx/* rw,
  /var/www/html/** r,
  /tmp/** rw,

  network inet tcp,
  network inet6 tcp,

  capability net_bind_service,
  capability setgid,
  capability setuid,

  deny /etc/shadow r,
  deny /root/** rw,
}

# 重新加载配置
apparmor_parser -r /etc/apparmor.d/usr.sbin.nginx
```

---

<!-- chunk: 常用命令参考 -->## 常用命令参考

## 安全检查命令

```bash
# 检查 UID 0 账户 (应仅有 root)
awk -F: '$3==0' /etc/passwd

# 检查空密码账户 (应无)
awk -F: '$2==""' /etc/shadow

# 检查可登录账户
grep -v "nologin\|false\|sync\|halt\|shutdown" /etc/passwd

# 检查 sudo 权限
cat /etc/sudoers /etc/sudoers.d/* 2>/dev/null | grep -v "^#\|^$"

# 检查 SSH 配置
sshd -T                              # 查看生效的 SSH 配置

# 检查监听端口
ss -tlnp
lsof -i -P -n | grep LISTEN

# 检查 SUID/SGID 文件
find / -perm -4000 -type f 2>/dev/null   # SUID
find / -perm -2000 -type f 2>/dev/null   # SGID
find / -perm -4000 -type f -exec ls -la {} \; 2>/dev/null

# 检查可写目录
find / -type d -perm -0002 ! -perm -1000 2>/dev/null

# 检查最近修改的文件
find /etc -type f -mtime -1           # 24 小时内修改
find / -type f -ctime -1 -not -path "/proc/*" -not -path "/sys/*"

# 检查登录历史
last                                  # 成功登录
lastb                                 # 失败登录
lastlog                               # 最后登录时间

# 检查当前登录用户
w
who
```

---

<!-- chunk: 性能调优 -->## 性能调优

## 安全与性能的平衡

```bash
# SELinux 性能影响通常 < 5%，不建议为性能禁用
# 如果确有性能问题，检查 audit 日志量
auditctl -l                           # 查看审计规则
auditctl -D                           # 临时清空规则（调试用）

# 减少不必要的审计日志
# 使用更精确的审计规则，避免通配符
# 例如: -w /etc/passwd -p wa -k identity
# 而非: -w /etc/ -p wa -k etc_changes

# AppArmor 比 SELinux 性能开销更低
# 但功能也相对简单
```

---

<!-- chunk: 安全加固 -->## 安全加固

## 系统安全加固脚本

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
#!/bin/bash
# production-security-harden.sh - 生产环境安全加固脚本

set -euo pipefail

echo "=== 开始安全加固 ==="

# 1. 用户安全
echo "1. 配置密码策略..."
cat > /etc/security/pwquality.conf << 'EOF'
minlen = 12
dcredit = -1
ucredit = -1
lcredit = -1
ocredit = -1
minclass = 3
maxrepeat = 3
difok = 5
EOF

cat > /etc/login.defs << 'EOF'
PASS_MAX_DAYS   90
PASS_MIN_DAYS   7
PASS_MIN_LEN    12
PASS_WARN_AGE   14
UMASK           027
ENCRYPT_METHOD  SHA512
EOF

# 2. 内核安全参数
echo "2. 配置内核安全参数..."
cat > /etc/sysctl.d/99-security.conf << 'EOF'
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.all.send_redirects = 0
net.ipv4.conf.all.accept_source_route = 0
net.ipv4.conf.default.accept_redirects = 0
net.ipv4.conf.default.send_redirects = 0
net.ipv4.conf.default.accept_source_route = 0
net.ipv4.conf.all.rp_filter = 1
net.ipv4.conf.default.rp_filter = 1
net.ipv4.conf.all.log_martians = 1
net.ipv4.icmp_echo_ignore_broadcasts = 1
net.ipv4.icmp_ignore_bogus_error_responses = 1
net.ipv4.tcp_syncookies = 1
kernel.dmesg_restrict = 1
kernel.kptr_restrict = 2
kernel.yama.ptrace_scope = 2
kernel.perf_event_paranoid = 2
kernel.unprivileged_bpf_disabled = 1
fs.suid_dumpable = 0
fs.protected_regular = 1
fs.protected_fifos = 1
fs.protected_symlinks = 1
fs.protected_hardlinks = 1
EOF

sysctl --system

# 3. 禁用不必要的服务
echo "3. 禁用不必要的服务..."
services_to_disable=(
    "rpcbind"
    "avahi-daemon"
    "cups"
    "bluetooth"
    "ModemManager"
)
for svc in "${services_to_disable[@]}"; do
    systemctl disable --now "$svc" 2>/dev/null || true
done

# 4. 文件权限加固
echo "4. 加固文件权限..."
chmod 700 /root
chmod 600 /etc/ssh/sshd_config
chmod 600 /etc/crontab
chmod 700 /etc/cron.*
chmod 700 /etc/sudoers.d
chmod 600 /etc/sudoers.d/*
chown root:root /etc/passwd /etc/shadow /etc/group /etc/gshadow
chmod 644 /etc/passwd /etc/group
chmod 600 /etc/shadow /etc/gshadow

# 5. 限制 cron 访问
echo "5. 限制 cron 访问..."
echo "root" > /etc/cron.allow
chmod 600 /etc/cron.allow
echo "" > /etc/cron.deny
chmod 600 /etc/cron.deny

# 6. umask 设置
echo "6. 设置默认 umask..."
echo "umask 027" >> /etc/profile
echo "umask 027" >> /etc/bashrc

# 7. 登录横幅
echo "7. 设置登录横幅..."
cat > /etc/issue.net << 'EOF'
**********************************************************************
*  WARNING: Unauthorized access to this system is prohibited.        *
*  All connections are monitored and recorded.                       *
*  Disconnect IMMEDIATELY if you are not an authorized user.         *
**********************************************************************
EOF

echo "=== 安全加固完成 ==="
```

## 审计配置

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

```bash
# /etc/audit/rules.d/production.rules

# 删除所有已有规则
-D

# 缓冲区大小
-b 8192

# 失败模式
-f 1

# ===== 身份认证文件 =====
-w /etc/passwd -p wa -k identity
-w /etc/shadow -p wa -k identity
-w /etc/group -p wa -k identity
-w /etc/gshadow -p wa -k identity
-w /etc/security/opasswd -p wa -k identity

# ===== SSH 配置 =====
-w /etc/ssh/sshd_config -p wa -k ssh
-w /etc/ssh/sshd_config.d/ -p wa -k ssh

# ===== sudo 配置 =====
-w /etc/sudoers -p wa -k priv_esc
-w /etc/sudoers.d/ -p wa -k priv_esc

# ===== 系统启动 =====
-w /etc/grub.conf -p wa -k boot
-w /etc/grub.d/ -p wa -k boot
-w /etc/grub2.cfg -p wa -k boot

# ===== 网络配置 =====
-w /etc/hosts -p wa -k network
-w /etc/resolv.conf -p wa -k network
-w /etc/sysconfig/network-scripts/ -p wa -k network
-w /etc/network/ -p wa -k network

# ===== 系统调用 =====
-a always,exit -F arch=b64 -S execve -k exec
-a always,exit -F arch=b32 -S execve -k exec
-a always,exit -F arch=b64 -S open,openat,creat -F dir=/etc -k etc_access

# ===== 权限提升 =====
-w /bin/su -p x -k priv_esc
-w /usr/bin/sudo -p x -k priv_esc
-w /etc/pam.d/ -p wa -k pam

# 重启审计
systemctl restart auditd

# 查看审计日志
ausearch -k identity --start recent
aureport --summary
aureport -x --summary                    # 可执行文件报告
aureport -u --summary                    # 用户报告
```

## 文件完整性监控 (AIDE)

```bash
# 安装 AIDE
yum install -y aide          # RHEL
apt install -y aide          # Ubuntu

# 初始化数据库
aide --init

# 复制数据库
cp /var/lib/aide/aide.db.new.gz /var/lib/aide/aide.db.gz

# 检查文件完整性
aide --check

# 更新数据库（在合法修改后）
aide --update
cp /var/lib/aide/aide.db.new.gz /var/lib/aide/aide.db.gz

# 配置定期检查 (cron)
echo "0 3 * * * /usr/sbin/aide --check | mail -s 'AIDE Report' admin@company.com" | crontab -
```

---

<!-- chunk: 与 Kubernetes 的关系 -->## 与 Kubernetes 的关系

## 容器安全特性

Kubernetes 利用 Linux 内核安全特性来隔离和保护容器：

| 安全特性 | Kubernetes 使用方式 | 说明 |
|:---|:---|:---|
| **Namespaces** | Pod 隔离 | PID/Net/Mount/UTS/IPC/User 隔离 |
| **cgroups** | 资源限制 | CPU/Memory/IO 限制 |
| **Capabilities** | SecurityContext | 控制容器权限 |
| **Seccomp** | RuntimeClass/Profile | 限制系统调用 |
| **SELinux** | SELinuxOptions | 文件访问控制 |
| **AppArmor** | Annotation | 进程行为限制 |

## Pod 安全上下文 (SecurityContext)

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: security-pod
spec:
  securityContext:
    runAsNonRoot: true            # 禁止以 root 运行
    runAsUser: 1000               # 指定用户
    runAsGroup: 3000              # 指定组
    fsGroup: 2000                 # 文件系统组
    seccompProfile:
      type: RuntimeDefault        # 使用默认 seccomp 配置
    seLinuxOptions:
      level: "s0:c123,c456"       # SELinux 上下文
  containers:
  - name: app
    image: nginx
    securityContext:
      allowPrivilegeEscalation: false  # 禁止提权
      readOnlyRootFilesystem: true     # 只读根文件系统
      capabilities:
        drop: ["ALL"]                   # 删除所有 capabilities
        add: ["NET_BIND_SERVICE"]       # 仅添加必要的
    volumeMounts:
    - name: cache
      mountPath: /var/cache/nginx
  volumes:
  - name: cache
    emptyDir: {}
```

## Pod Security Standards

```yaml
# Pod Security Standards (PSS) 通过命名空间标签实施
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted     # 强制
    pod-security.kubernetes.io/audit: restricted       # 审计
    pod-security.kubernetes.io/warn: restricted        # 警告

# 三个级别:
# privileged  - 无限制 (系统级 Pod)
# baseline    - 最小限制 (禁止危险特性)
# restricted  - 严格限制 (推荐所有应用)
```

---

<!-- chunk: 最佳实践 -->## 最佳实践

1. **最小权限原则**: 容器以非 root 运行，删除所有不必要的 capabilities
2. **启用 SELinux/AppArmor**: 不要禁用强制访问控制
3. **只读根文件系统**: 使用 `readOnlyRootFilesystem: true`
4. **镜像安全扫描**: 使用 Trivy/Clair 扫描镜像漏洞
5. **网络策略**: 使用 NetworkPolicy 限制 Pod 间通信
6. **密钥管理**: 使用 Vault/Sealed Secrets 管理敏感数据
7. **审计日志**: 启用 Kubernetes 审计日志
8. **运行时安全**: 部署 Falco 进行运行时威胁检测

---

<!-- chunk: 故障排查 -->## 故障排查

## 安全相关故障诊断

```bash
# SELinux 阻止了操作
# 1. 查看拒绝日志
ausearch -m avc -ts recent | tail -10
sealert -a /var/log/audit/audit.log

# 2. 临时设置为 Permissive 模式测试
setenforce 0

# 3. 生成修复策略
grep <process> /var/log/audit/audit.log | audit2allow -M mypolicy
semodule -i mypolicy.pp

# SSH 连接失败
# 1. 检查 sshd 配置
sshd -T

# 2. 查看 SSH 日志
journalctl -u sshd -f

# 3. 调试模式
sshd -d -p 2222

# 权限被拒绝
# 1. 检查文件权限
ls -laZ /path/to/file

# 2. 检查 ACL
getfacl /path/to/file

# 3. 检查 SELinux 上下文
ls -Z /path/to/file
matchpathcon /path/to/file

# PAM 认证失败
# 1. 查看认证日志
tail -f /var/log/secure      # RHEL
tail -f /var/log/auth.log    # Ubuntu

# 2. 检查 PAM 配置
pam_tally2 --user=username    # 查看失败计数
faillock --user username       # 查看锁定状态
```

---

<!-- chunk: 相关文档 -->## 相关文档

- [01-linux-system-architecture](./01-linux-system-architecture.md) - 系统架构
- [08-linux-container-fundamentals](./08-linux-container-fundamentals.md) - 容器基础

---

**维护者**: Allen Galler (allengaller@gmail.com) | **许可证**: MIT

## See Also

- 05-linux-storage-management
- 06-linux-performance-tuning
- 08-linux-container-fundamentals
- 09-linux-operations-basics

```