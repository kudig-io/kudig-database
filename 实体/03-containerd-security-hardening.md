---
title: containerd 安全加固
description: '# containerd 安全加固'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- runtime
- 03-containerd-security-hardening
- containerd
- falco
- networkpolicy
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- containerd 安全加固 是什么
- 如何 containerd 安全加固
trigger_keywords:
- containerd
- 安全加固
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# containerd 安全加固

> **CNCF 状态**: Graduated | **类别**: Runtime | **主要语言**: Go

## 概述

Containerd 安全加固是一套针对 containerd 容器运行时的安全最佳实践和配置指南。它涵盖运行时二进制文件保护、套接字权限控制、镜像内容信任、Seccomp/AppArmor/SELinux 策略配置、审计日志、容器镜像扫描等多个维度。通过系统性加固，可将 containerd 从默认配置提升到满足生产级安全合规要求的水平，防御容器逃逸、提权攻击等安全威胁。

## Key Features（核心能力）

- **套接字权限控制**：限制 containerd.sock 文件权限，仅允许 root 或特定组访问
- **内容信任**：启用 containerd image verification 和 Cosign/Notary 签名验证
- **Seccomp 默认策略**：通过默认 seccomp profile 限制系统调用
- **AppArmor/SELinux**：通过 MAC 策略限制容器进程行为
- **审计日志**：启用 containerd audit log 记录所有容器操作
- **镜像扫描**：集成 Trivy/Grype 进行容器镜像漏洞扫描

## 架构与工作原理

安全加固从多个层面实施：二进制层面（只读文件系统、文件完整性监控）；配置层面（最小权限配置、禁用不必要功能）；运行时层面（Seccomp/AppArmor/SELinux 策略约束容器行为）；网络层面（限制 containerd gRPC 端口暴露）；镜像层面（签名验证、漏洞扫描）。通过纵深防御策略，在各层建立安全控制点。

## K8s 集成

在 Kubernetes 中，containerd 安全加固通过 KubeletConfiguration（如 protectKernelDefaults、seccompDefault）、Pod Security Admission（seccompProfile、apparmorProfile）以及节点级配置文件（config.toml）实现。CRI 支持将 seccomp 和 AppArmor profile 通过 Pod SecurityContext 传递给 containerd。

## 生产用例

- **生产环境合规**：满足 CIS Benchmark 和等保 2.0 对容器运行时的安全要求
- **多租户集群安全**：防止容器逃逸和横向移动攻击
- **金融/政府场景**：满足严格的安全审计和访问控制要求
- **安全事件防御**：通过运行时安全策略减少攻击面和影响范围

## 安装与配置

### 安全加固配置 (config.toml)

```toml
# /etc/containerd/config.toml - 安全加固版本
version = 2
root = "/var/lib/containerd"
state = "/run/containerd"

# 禁用不必要的 gRPC 服务暴露
[grpc]
  address = "/run/containerd/containerd.sock"
  uid = 0
  gid = 0

[plugins."io.containerd.grpc.v1.cri"]
  # 禁止非特权端口绑定
  enable_unprivileged_ports = false
  enable_unprivileged_icmp = false
  # 禁用 cgroup 写入（仅读取）
  disable_cgroup = false
  # 启用 TLS 流式传输
  enable_tls_streaming = false
  # 限制最大容器日志大小
  max_container_log_line_size = 16384
  # 镜像拉取仅允许签名镜像
  [plugins."io.containerd.grpc.v1.cri".image_decryption]
    key_model = "node"
  [plugins."io.containerd.grpc.v1.cri".registry]
    config_path = "/etc/containerd/certs.d"
  # 默认 Seccomp Profile
  [plugins."io.containerd.grpc.v1.cri".containerd]
    default_runtime_name = "runc"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true
        # Seccomp 默认启用 RuntimeDefault
        # 通过 KubeletConfiguration 配置
```

### Kubelet 安全配置

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
protectKernelDefaults: true  # 禁止 Pod 修改内核参数
seccompDefault: true  # 默认启用 Seccomp RuntimeDefault
featureGates:
  AppArmor: true
  SecurityContextPrivileged: false
streamingConnectionIdleTimeout: 5m
makeIPTablesUtilChains: true
```

### Seccomp 自定义 Profile

```json
{
  "defaultAction": "SCMP_ACT_ERRNO",
  "architectures": ["SCMP_ARCH_X86_64"],
  "syscalls": [
    {
      "names": ["accept", "accept4", "access", "arch_prctl", "bind",
                "brk", "capget", "capset", "chdir", "chmod",
                "clone", "close", "connect", "dup", "dup2",
                "epoll_create", "epoll_ctl", "epoll_wait",
                "execve", "exit", "exit_group", "fcntl",
                "fstat", "futex", "getcwd", "getdents64",
                "getpid", "getuid", "ioctl", "kill",
                "listen", "madvise", "mkdir", "mmap",
                "mount", "mprotect", "munmap", "nanosleep",
                "open", "openat", "pipe", "poll",
                "prctl", "read", "readlink", "recvfrom",
                "rt_sigaction", "rt_sigprocmask", "rt_sigreturn",
                "sched_yield", "select", "sendto",
                "set_tid_address", "setgid", "setgroups",
                "setuid", "socket", "stat", "umask",
                "uname", "unlink", "wait4", "write", "writev"],
      "action": "SCMP_ACT_ALLOW"
    }
  ]
}
```

### Pod Security Admission 配置

```yaml
# 命名空间级别强制 Restricted 安全策略
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
---
# 符合 Restricted 的 Pod 示例
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    fsGroup: 2000
    seccompProfile:
      type: RuntimeDefault
  containers:
  - name: app
    image: registry.example.com/app:v1.2@sha256:abc123...
    securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop: ["ALL"]
    resources:
      limits:
        memory: "512Mi"
        cpu: "500m"
```

### 镜像签名验证 (Cosign)

```bash
# 🟢 安装 Cosign
go install github.com/sigstore/cosign/v2/cmd/cosign@latest

# 🟢 生成密钥对
cosign generate-key-pair

# 🟢 签名镜像
cosign sign --key cosign.key registry.example.com/app:v1.2

# 🟢 验证签名
cosign verify --key cosign.pub registry.example.com/app:v1.2

# 🟢 containerd 配置镜像验证 (policy.json)
cat > /etc/containerd/image-policy.json << 'EOF'
{
  "default": [{"type": "reject"}],
  "rules": [
    {
      "match": "registry.example.com/*",
      "policy": [{"type": "cosign", "keyPath": "/etc/containerd/cosign.pub"}]
    }
  ]
}
EOF
```

## 运维操作

```bash
# 🟢 检查 containerd 安全配置
containerd config dump | grep -E 'security|seccomp|apparmor|privileged'

# 🟢 检查套接字权限
ls -la /run/containerd/containerd.sock
stat -c '%a %U %G' /run/containerd/containerd.sock

# 🟢 检查容器 Seccomp 状态
for pid in $(crictl inspect $(crictl ps -q) | jq '.info.pid'); do
  echo "PID $pid: $(grep Seccomp /proc/$pid/status)"
done

# 🟢 检查容器 Capabilities
crictl inspect <container-id> | jq '.info.runtimeSpec.process.capabilities'

# 🟢 检查只读文件系统
crictl inspect <container-id> | jq '.info.runtimeSpec.root.readonly'

# 🟢 镜像漏洞扫描
trivy image registry.example.com/app:v1.2
trivy fs /var/lib/containerd/io.containerd.content.v1.content/

# 🟢 CIS Benchmark 检查
# 检查 containerd 二进制文件权限
stat -c '%a' /usr/local/bin/containerd  # 应为 755 或更严格
stat -c '%a' /etc/containerd/config.toml  # 应为 644 或更严格
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 被拒绝创建 | PSA enforce 策略过严 | `kubectl describe ns` | 调整 namespace label/修改 Pod spec |
| 容器启动失败 | Seccomp 拦截必要 syscall | `dmesg`; `audit.log` | 调整 seccomp profile 允许必要调用 |
| 镜像拉取被拒绝 | 签名验证失败 | `journalctl -u containerd` | 检查 cosign 密钥/更新 policy |
| 套接字连接失败 | 权限过严 | `ls -la containerd.sock` | 调整 sock 文件权限/组 |
| AppArmor 拒绝 | MAC 策略过严 | `dmesg | grep apparmor` | 调整 profile 为 complain 模式调试 |

### 安全加固排查流程

```
安全策略导致服务异常
├── Pod 无法创建？
│   ├── kubectl get events → PSA 拒绝？
│   ├── 检查 namespace labels
│   └── 调整 Pod securityContext
├── 容器启动失败？
│   ├── dmesg | grep -i seccomp → 被拦截的 syscall
│   ├── dmesg | grep -i apparmor → MAC 拒绝
│   └── 调整 profile 或添加必要权限
└── 镜像拉取失败？
    ├── 检查签名: cosign verify
    ├── 检查 policy.json 配置
    └── 检查 Registry 证书/网络
```

## 生产案例

### 案例1：容器逃逸攻击防御

- **场景**：安全团队发现恶意镜像尝试通过 CAP_SYS_ADMIN 挂载宿主机文件系统
- **排查**：Falco 告警 "Container launched with privileged mode"；审计日志显示 mount syscall
- **方案**：强制 PSA Restricted + drop ALL capabilities + readOnlyRootFilesystem + Seccomp RuntimeDefault
- **效果**：攻击链被多层防御截断，容器无法执行 mount/ptrace 等危险操作

### 案例2：Seccomp 策略导致 Java 应用崩溃

- **场景**：自定义 Seccomp profile 应用后 Java 应用 JVM 崩溃
- **排查**：`dmesg` 显示 "audit: type=1326 syscall=234" 被拦截；syscall 234 = tgkill（JVM GC 需要）
- **方案**：在自定义 profile 中添加 tgkill、futex、mmap 等 JVM 必需 syscall；或回退到 RuntimeDefault
- **效果**：应用正常运行，同时保持对危险 syscall 的拦截

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| containerd + PSA + Seccomp | 原生集成、多层防御 | 配置复杂、可能影响应用 | 标准生产环境 |
| gVisor (runsc) | 用户态内核、强隔离 | 性能开销、兼容性问题 | 不可信工作负载 |
| Kata Containers | VM级隔离、透明 | 资源开销、启动稍慢 | 多租户/强隔离 |
| SELinux + CRI-O | MAC 强制、Red Hat支持 | 学习曲线陡、调试困难 | OpenShift/RHEL |
| Falco 运行时检测 | 实时检测、不阻断 | 仅检测不防御、需配合响应 | 安全监控/审计 |

## 检查清单

- [ ] containerd.sock 权限仅 root 可访问 (0600)
- [ ] Seccomp 默认启用 (RuntimeDefault)
- [ ] PSA enforce 设置为 restricted（生产命名空间）
- [ ] 所有容器 drop ALL capabilities
- [ ] readOnlyRootFilesystem = true
- [ ] runAsNonRoot = true
- [ ] 镜像签名验证已启用 (Cosign/Notary)
- [ ] 镜像漏洞扫描集成到 CI/CD
- [ ] 审计日志已启用并发送到 SIEM
- [ ] CIS Benchmark 检查通过

## Related

- [[inclavare-containers]] — Inclavare Containers
- [[bank-vaults]] — Bank-Vaults
- [[thanos]] — Thanos
- [[containerd]] — containerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 03-containerd-security-hardening


<!-- risk-assessed -->
