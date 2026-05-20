---
title: containerd 安全加固生产指南
description: '## 1. 安全加固概述'
category: cncf-landscape
tags:
- k8s
- cncf
- security
- containerd
- fips
- air-gapped
- hardening
- compliance
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- 运维工程师
- 合规团队
estimated_read_time: 10min
intent_queries:
- containerd 安全加固 如何
- containerd FIPS 配置
- containerd Air-gapped 环境配置
trigger_keywords:
- containerd 安全加固
- containerd FIPS
- containerd 合规
---

# containerd 安全加固生产指南

> **版本**: v1.0 | **适用版本**: containerd 1.6+ / 2.0 | **最后更新**: 2026-05

---

## 1. 安全加固概述

### 1.1 安全加固目标

| 目标 | 说明 |
|------|------|
| **机密性** | 保护容器镜像、配置、密钥不被未授权访问 |
| **完整性** | 确保 containerd 及组件不被篡改 |
| **可用性** | 防止安全加固影响运行时稳定性 |
| **合规性** | 满足 FIPS 140-2、PCI-DSS、SOC2 等要求 |

### 1.2 安全加固层级

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                         containerd 安全加固层级                                  │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  第5层: 合规审计                                                                 │
│  ─────────────────────────────────────────────────────────                      │
│  第4层: 网络安全                                                                 │
│  ─────────────────────────────────────────────────────────                      │
│  第3层: 运行时安全                                                               │
│  ─────────────────────────────────────────────────────────                      │
│  第2层: 容器隔离                                                                 │
│  ─────────────────────────────────────────────────────────                      │
│  第1层: 基础加固                                                                 │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 基础加固 (第1层)

### 2.1 文件系统权限

```bash
# containerd 数据目录权限
chown -R root:root /var/lib/containerd
chmod -R 755 /var/lib/containerd

# socket 文件权限
chown root:root /run/containerd/containerd.sock
chmod 660 /run/containerd/containerd.sock

# 配置文件权限
chown root:root /etc/containerd/config.toml
chmod 640 /etc/containerd/config.toml

# 审计日志目录
mkdir -p /var/log/containerd
chown -R root:adm /var/log/containerd
chmod -R 750 /var/log/containerd
```

### 2.2 运行时版本安全

```bash
# 定期检查安全更新
apt-get update && apt-get install containerd.io

# 或手动检查版本
containerd --version

# 订阅安全公告
# https://github.com/containerd/containerd/security/advisories
```

### 2.3 安全启动配置

```toml
# /etc/containerd/config.toml
version = 2

# 禁用不安全的特性
[plugins]
  # 禁用远程调试
  [debug]
    level = "info"  # 不使用 debug 级别
    address = ""  # 不启用远程调试 socket
  
  # gRPC 安全配置
  [grpc]
    address = "/run/containerd/containerd.sock"
    max_recv_message_size = 8388608  # 限制最大消息大小 (8MB)
    max_send_message_size = 8388608
    uid = 0
    gid = 0
```

---

## 3. 容器隔离 (第2层)

### 3.1 用户命名空间

```yaml
# Kubernetes Pod 使用用户命名空间
apiVersion: v1
kind: Pod
metadata:
  name: rootless-pod
spec:
  securityContext:
    runAsUser: 10000
    runAsGroup: 10000
    runAsNonRoot: true
  containers:
  - name: app
    image: myapp:latest
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
```

### 3.2 Capabilities 细粒度控制

```yaml
# 最小权限 capabilities
apiVersion: v1
kind: Pod
metadata:
  name: minimal-capabilities
spec:
  securityContext:
    runAsNonRoot: true
  containers:
  - name: app
    image: nginx
    securityContext:
      capabilities:
        drop:
        - ALL
        add:
        - NET_BIND_SERVICE
```

### 3.3 Seccomp 配置

```json
// /etc/containerd/seccomp/default.json
{
  "defaultAction": "SCMP_ACT_ERRNO",
  "architectures": ["SCMP_ARCH_X86_64", "SCMP_ARCH_AARCH64"],
  "syscalls": [
    {
      "names": ["accept", "accept4", "bind", "socket"],
      "action": "SCMP_ACT_ALLOW"
    },
    {
      "names": ["execve", "exit_group", "wait4"],
      "action": "SCMP_ACT_ALLOW"
    }
  ]
}
```

```toml
# containerd 配置使用 seccomp
[plugins."io.containerd.grpc.v1.cri"]
  enable_seccomp = true
  seccomp_profile = "/etc/containerd/seccomp/default.json"
```

### 3.4 AppArmor 配置

```bash
# 安装 AppArmor
apt-get install apparmor apparmor-utils

# 创建 containerd profile
cat > /etc/apparmor.d/containerd << 'EOF'
profile containerd flags=(attach_disconnected,mediate_deleted) {
  # 允许基本操作
  network,
  signal (send,receive),
  
  # 文件访问限制
  /var/lib/containerd/** r,
  /run/containerd/** rw,
  
  # 拒绝危险操作
  deny mount,
  deny ptrace (read,readby),
}
EOF

# 加载 profile
apparmor_parser -r /etc/apparmor.d/containerd
```

---

## 4. 运行时安全 (第3层)

### 4.1 OCI Runtime 安全配置

```toml
# /etc/containerd/config.toml
[plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
  runtime_type = "io.containerd.runc.v2"
  privileged_without_host_devices = true
  
  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
    # 安全相关选项
    SystemdCgroup = true
    NoNewKeyring = true
    Root = "/run/containerd/runc"
    ShimCgroup = "/system.slice/containerd-shim"
    
    # 资源限制
    # 不在这里设置，由 Kubernetes Pod spec 控制
```

### 4.2 runc 安全配置

```bash
# runc 安全选项
# /etc/runc/runc.conf
{
  "defaultRuntime": "runc",
  "runtimes": {
    "runc": {
      "path": "/usr/bin/runc",
      "runtimeArgs": [
        "--log", "/var/log/runc.log",
        "--log-format", "json",
        "--root", "/run/containerd/runc"
      ]
    }
  }
}

# 使用 seccomp
runc --seccomp /etc/runc/seccomp.json run mycontainer

# 使用 no-new-privileges
runc --no-new-privs run mycontainer
```

### 4.3 Shim 安全隔离

```bash
# Shim 进程隔离
# 每个容器使用独立的 shim 进程，shim 进程间隔离

# 检查 shim 进程
ps aux | grep containerd-shim

# shim 进程数量应等于容器数量
# 如果有泄漏，检查是否正常清理
```

---

## 5. 网络安全 (第4层)

### 5.1 CNI 网络策略

```yaml
# NetworkPolicy 限制 Pod 访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: containerd-net-policy
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: containerd-workload
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          role: frontend
  egress:
  - to:
    - podSelector:
        matchLabels:
          role: database
```

### 5.2 TLS 配置

```toml
# containerd TLS 配置 (如果需要远程管理)
# /etc/containerd/config.toml
[grpc]
  address = "/run/containerd/containerd.sock"
  
  # TLS 配置
  [grpc.tls]
    cert_file = "/etc/containerd/tls/cert.pem"
    key_file = "/etc/containerd/tls/key.pem"
    ca_file = "/etc/containerd/tls/ca.pem"

# 注意：kubelet 使用 unix socket，不需要 TLS
# TLS 仅用于远程管理场景
```

### 5.3 防火墙规则

```bash
# 限制 containerd API 访问
# 只允许 kubelet 和相关组件访问

# 查看监听端口
ss -tlnp | grep containerd

# 添加 iptables 规则（如果需要）
iptables -A INPUT -p tcp --dport 1338 -s 10.0.0.0/8 -j ACCEPT
iptables -A INPUT -p tcp --dport 1338 -j DROP
```

---

## 6. 合规审计 (第5层)

### 6.1 FIPS 140-2 合规

```bash
# 检查 FIPS 模式
cat /proc/sys/crypto/fips_enabled
# 0 = 非 FIPS, 1 = FIPS 模式

# 如果需要 FIPS 模式
# 1. 操作系统需要启用 FIPS
fips-mode-setup --enable

# 2. 使用 FIPS 认证的加密库
# containerd 需要链接到 FIPS 认证的 OpenSSL/GnuTLS

# 3. 验证 containerd 使用的加密库
ldd $(which containerd) | grep -E "ssl|crypto"
```

```toml
# FIPS 模式下的 containerd 配置
# /etc/containerd/config.toml
version = 2

[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
    # 使用 FIPS 认证的加密算法
    # TLS 1.2+ only, 禁用 TLS 1.0/1.1
    
    [plugins."io.containerd.grpc.v1.cri".registry]
      # 仅允许 TLS 1.2+
      config_path = "/etc/containerd/certs.d"
```

### 6.2 安全审计日志

```yaml
# auditd 规则
# /etc/audit/rules.d/containerd.rules

# 监控 containerd 配置文件变更
-w /etc/containerd/config.toml -p wa -k containerd_config

# 监控 containerd 数据目录
-w /var/lib/containerd -p wa -k containerd_data

# 监控 containerd socket
-w /run/containerd/containerd.sock -p rw -k containerd_socket

# 监控 containerd 进程
-a always,exit -F arch=b64 -S execve -F path=/usr/bin/containerd -k containerd_exec
```

```bash
# 查看审计日志
ausearch -k containerd_config
ausearch -k containerd_exec

# 实时监控
auditctl -w /etc/containerd/config.toml -p wa -k containerd_config
```

### 6.3 合规检查清单

| 检查项 | 标准 | 说明 |
|--------|------|------|
| **文件系统权限** | PCI-DSS 3.2 | /var/lib/containerd 权限 750 |
| **Socket 权限** | SOC2 | /run/containerd/*.sock 权限 660 |
| **日志审计** | SOC2/AIS | 启用 auditd 记录所有操作 |
| **TLS 配置** | PCI-DSS | 远程管理使用 TLS 1.2+ |
| **Seccomp** | CIS Kubernetes | 启用默认 seccomp profile |
| **Capabilities** | CIS Kubernetes | 容器使用最小权限 capabilities |
| **用户命名空间** | SOC2 | 考虑使用用户命名空间增强隔离 |
| **版本更新** | PCI-DSS | 定期更新 containerd 到最新安全版本 |

---

## 7. Air-gapped 环境配置

### 7.1 离线安装

```bash
# 在有网络的环境中下载所有依赖

# 1. 下载 containerd
wget https://github.com/containerd/containerd/releases/download/v2.0.0/containerd-2.0.0-linux-amd64.tar.gz

# 2. 下载 runc
wget https://github.com/opencontainers/runc/releases/download/v1.2.0/runc.amd64

# 3. 下载 CNI plugins
wget https://github.com/containernetworking/plugins/releases/download/v1.4.0/cni-plugins-linux-amd64-v1.4.0.tgz

# 4. 下载 crictl
wget https://github.com/kubernetes-sigs/cri-tools/releases/download/v1.28.0/crictl-v1.28.0-linux-amd64.tar.gz

# 5. 下载 pause 镜像
docker pull registry.k8s.io/pause:3.10
docker save registry.k8s.io/pause:3.10 -o pause-3.10.tar

# 传输到 air-gapped 环境
scp *.tar.gz user@air-gapped-node:/tmp/

# 6. 安装
tar xvf containerd-2.0.0-linux-amd64.tar.gz -C /usr/local
mv runc.amd64 /usr/bin/runc
chmod +x /usr/bin/runc

mkdir -p /opt/cni/bin
tar xvf cni-plugins-linux-amd64-v1.4.0.tgz -C /opt/cni/bin

# 7. 加载 pause 镜像
crictl load -i pause-3.10.tar
```

### 7.2 私有仓库配置

```toml
# /etc/containerd/config.toml
version = 2

[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
    # 私有仓库配置
    [plugins."io.containerd.grpc.v1.cri".registry]
      config_path = "/etc/containerd/certs.d"
      
      # 配置信任的仓库
      [plugins."io.containerd.grpc.v1.cri".registry.configs]
        [plugins."io.containerd.grpc.v1.cri".registry.configs."my-registry.example.com"]
          auth = ""
          
      # mirrors 配置
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
          endpoint = ["https://my-mirror.example.com"]
```

### 7.3 Air-gapped 证书配置

```bash
# 私有仓库证书
mkdir -p /etc/containerd/certs.d/my-registry.example.com

# 复制 CA 证书
cp ca.crt /etc/containerd/certs.d/my-registry.example.com/

# 创建 hosts.toml
cat > /etc/containerd/certs.d/my-registry.example.com/hosts.toml << 'EOF'
server = "https://my-registry.example.com"

[host."https://my-registry.example.com"]
  capabilities = ["pull", "resolve", "push"]
  ca = "/etc/containerd/certs.d/my-registry.example.com/ca.crt"
EOF

# 如果需要客户端认证
mkdir -p /etc/containerd/certs.d/my-registry.example.com
cp client.crt /etc/containerd/certs.d/my-registry.example.com/
cp client.key /etc/containerd/certs.d/my-registry.example.com/

# hosts.toml 更新
cat > /etc/containerd/certs.d/my-registry.example.com/hosts.toml << 'EOF'
server = "https://my-registry.example.com"

[host."https://my-registry.example.com"]
  capabilities = ["pull", "resolve", "push"]
  ca = "/etc/containerd/certs.d/my-registry.example.com/ca.crt"
  client = [
    ["/etc/containerd/certs.d/my-registry.example.com/client.crt", "/etc/containerd/certs.d/my-registry.example.com/client.key"]
  ]
EOF

# 重启 containerd
systemctl restart containerd
```

---

## 8. 安全监控

### 8.1 关键安全指标

| 指标 | 告警阈值 | 说明 |
|------|----------|------|
| **未授权访问尝试** | > 0 | 检测到任何失败的身份验证 |
| **配置变更** | > 0 | containerd 配置被修改 |
| **Shim 进程泄漏** | > 容器数 | shim 进程数量异常 |
| **Seccomp 违规** | > 0 | 容器尝试危险系统调用 |

### 8.2 Falco 安全规则

```yaml
# /etc/falco/rules.d/containerd-rules.yaml
- rule: containerd config modified
  desc: containerd configuration file was modified
  condition: >
    modify and 
    (file.name eq "/etc/containerd/config.toml")
  output: >
    containerd config modified (user=%user.name command=%proc.cmdline)
  priority: WARNING

- rule: suspicious containerd activity
  desc: >
    suspicious activity detected on containerd
  condition: >
    (proc.name eq "containerd" and 
     (proc.args contains "--debug" or
      proc.args contains "exec" and proc.args contains "sh"))
  output: >
    suspicious containerd activity (user=%user.name proc=%proc.name cmd=%proc.cmdline)
  priority: CRITICAL

- rule: containerd shim anomaly
  desc: >
    containerd shim process count anomaly
  condition: >
    proc.name eq "containerd-shim" and 
    (evt.type = "clone" and count > 100)
  output: >
    containerd shim anomaly detected
  priority: WARNING
```

---

## 9. 灾难恢复与备份

### 9.1 安全配置备份

```bash
# 自动备份脚本
#!/bin/bash
# /usr/local/bin/backup-containerd-config.sh

BACKUP_DIR="/backup/containerd"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p $BACKUP_DIR

# 备份配置
cp /etc/containerd/config.toml $BACKUP_DIR/config_$DATE.toml

# 备份证书
tar czf $BACKUP_DIR/certs_$DATE.tar.gz /etc/containerd/certs.d/

# 备份审计规则
cp /etc/audit/rules.d/containerd.rules $BACKUP_DIR/audit-rules_$DATE

# 清理 30 天前的备份
find $BACKUP_DIR -type f -mtime +30 -delete

echo "Backup completed: $DATE"
```

### 9.2 安全事件响应

```bash
# 安全事件响应剧本

# 1. 检测到未授权访问
# - 检查 auditd 日志
ausearch -k containerd_socket --interpret

# - 检查 containerd 日志
journalctl -u containerd --since "1h" | grep -i "auth\|fail\|deny"

# 2. 配置被篡改
# - 对比配置哈希
sha256sum /etc/containerd/config.toml
# 与上次备份的哈希对比

# - 恢复配置
cp /backup/containerd/config_$(date -d "yesterday" +%Y%m%d).toml /etc/containerd/config.toml

# 3. 恶意镜像检测
# - 检查镜像来源
crictl images | grep -v "registry.k8s.io\|my-registry"

# - 扫描镜像漏洞
trivy image --severity HIGH,CRITICAL <image-name>

# 4. 事件上报
# - 记录到安全事件管理系统
```

---

## 10. 生产安全配置模板

```toml
# /etc/containerd/config.toml (生产安全加固版)
version = 2

# 全局设置
root = "/var/lib/containerd"
state = "/run/containerd"
oom_score = -999

# gRPC 配置
[grpc]
  address = "/run/containerd/containerd.sock"
  max_recv_message_size = 8388608
  max_send_message_size = 8388608
  uid = 0
  gid = 0

# 调试配置（生产禁用）
[debug]
  level = "info"
  address = ""

# 指标配置
[metrics]
  address = "127.0.0.1:1338"
  grpc_histogram = true

# 插件配置
[plugins]
  [plugins."io.containerd.grpc.v1.cri"]
    sandbox_image = "registry.k8s.io/pause:3.10"
    enable_selinux = true
    enable_apparmor = false
    enable_unprivileged_ports = false
    enable_unprivileged_icmp = false
    max_container_log_line_size = 16384
    max_concurrent_downloads = 10
    
    [plugins."io.containerd.grpc.v1.cri".containerd]
      snapshotter = "overlayfs"
      default_runtime_name = "runc"
      
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
        runtime_type = "io.containerd.runc.v2"
        privileged_without_host_devices = true
        
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
          SystemdCgroup = true
          NoNewKeyring = true
    
    [plugins."io.containerd.grpc.v1.cri".registry]
      config_path = "/etc/containerd/certs.d"

  # Snapshotter 配置
  [plugins."io.containerd.snapshotter.v1.overlayfs"]
    root_path = ""

# 超时配置
[timeouts]
  "io.containerd.timeout.bolt.open" = "5s"
  "io.containerd.timeout.shim.cleanup" = "5s"
  "io.containerd.timeout.shim.shutdown" = "3s"

# 传输安全
[transport]
  # 禁用不安全的传输
  allow_insecure = false
```

---

## 附录: 合规检查脚本

```bash
#!/bin/bash
# containerd-security-check.sh

echo "=== containerd 安全合规检查 ==="
echo ""

# 1. 文件权限检查
echo "1. 文件权限检查:"
stat -c "%a %n" /etc/containerd/config.toml
stat -c "%a %n" /var/lib/containerd
stat -c "%a %n" /run/containerd/containerd.sock
echo ""

# 2. 版本检查
echo "2. containerd 版本:"
containerd --version
echo ""

# 3. 运行用户检查
echo "3. containerd 进程用户:"
ps aux | grep containerd | grep -v grep | awk '{print $1}'
echo ""

# 4. Seccomp 配置检查
echo "4. Seccomp 配置:"
grep -i seccomp /etc/containerd/config.toml
echo ""

# 5. SELinux 配置检查
echo "5. SELinux 状态:"
getenforce
echo ""

# 6. Audit 日志检查
echo "6. Audit 配置:"
auditctl -l | grep containerd
echo ""

# 7. 网络端口检查
echo "7. containerd 监听端口:"
ss -tlnp | grep containerd
echo ""

echo "=== 检查完成 ==="
```

---

**维护者**: Kudig Team | **许可证**: MIT