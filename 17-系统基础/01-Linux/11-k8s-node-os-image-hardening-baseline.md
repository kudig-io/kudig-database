---
title: K8s 节点 OS 镜像加固基线
description: 面向 Kubernetes 生产节点的操作系统镜像加固基线，覆盖 CIS Benchmark、不可变基础设施、磁盘分区、auditd、sysctl、containerd 加固与更新节奏。
summary: 面向 Kubernetes 生产节点的操作系统镜像加固基线，覆盖 CIS Benchmark、不可变基础设施、磁盘分区、auditd、sysctl、containerd 加固与更新节奏。
category: system-foundation
tags:
- production
- best-practices
- playbook
- system-foundation
- linux
- hardening
- cis
- containerd
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 30min
intent_queries:
- Kubernetes 节点 OS 如何加固
- CIS Benchmark 在 K8s 节点如何落地
- 不可变基础设施与节点镜像加固
- containerd 安全加固基线
trigger_keywords:
- node os hardening
- cis benchmark
- immutable infrastructure
- containerd hardening
- sysctl
- auditd
- 节点加固
prerequisites:
- kubectl-basics
- linux-basics
- containerd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

# K8s 节点 OS 镜像加固基线

本指南定义 Kubernetes 生产节点操作系统镜像的加固基线，适用于基于镜像构建（Image-based Provisioning）或 Ansible/CloudInit 初始化的节点。目标是在节点加入集群前，通过标准化配置消除常见攻击面，并为后续容器运行时、kubelet 与可观测性 Agent 提供稳定、可审计的运行环境。节点是 Kubernetes 的底座，一旦节点被攻破，攻击者可能获取该节点上所有 Pod 的访问权限，甚至通过特权提升横向移动至整个集群，因此节点加固是生产安全的第一道防线。

## 1. 适用场景与范围

本基线适用于以下场景：

- 自建 Kubernetes 集群的节点镜像构建与初始化。
- 云厂商托管 Kubernetes（ACK/EKS/GKE/AKS 等）的自定义节点池镜像。
- 控制平面节点与工作节点均需遵循本基线，控制面节点应执行更严格的访问控制。

本基线不覆盖容器运行时内部安全策略，相关内容参考安全合规域与容器运行时域。同时，本基线也不覆盖应用层安全，如容器镜像扫描、NetworkPolicy 等。

## 2. 前置条件与工具

在开始加固前，请准备以下工具与流程：

- 镜像构建工具：Packer、Kickstart、CloudInit、Ansible、Container Linux Config 等。
- 合规扫描工具：CIS-CAT、OpenSCAP、Lynis 或 cloud provider 合规服务。
- 版本锁定：内核、containerd、kubelet 版本应在镜像构建时冻结，并通过包管理器 hold。
- 建议建立镜像仓库与 SBOM 管理流程，每次构建记录软件包清单与合规报告。

## 3. 核心概念

### 3.1 CIS Benchmark

CIS（Center for Internet Security）提供针对各类 Linux 发行版的加固基准。生产节点建议至少达到 CIS Level 1，控制面节点或高敏感环境达到 Level 2。关键控制项包括：

- 文件系统完整性（AIDE）
- 最小化软件包与服务
- 安全启动配置
- 审计策略
- 用户与权限管理
- 网络参数安全

CIS Benchmark 不是一成不变的清单，应根据实际业务需求与合规要求进行裁剪。例如，某些行业可能需要额外启用 FIPS 模式或禁用特定内核模块。

### 3.2 不可变基础设施

节点镜像应视为不可变 artifact：

- 所有配置通过镜像或初始化脚本注入，禁止登录节点后手工修改。
- 节点维护以“替换”代替“修复”，异常节点直接下线并重新扩缩容。
- 配置文件纳入 Git 版本管理，镜像构建流水线记录 SBOM 与合规报告。
- 需要持久化的运行时数据（日志、容器存储）应挂载到独立分区，避免污染根文件系统。

不可变基础设施能够显著降低配置漂移风险，并缩短节点恢复时间。当节点出现异常时，SRE 只需将其下线并等待新节点自动加入，而不是花费时间排查节点本地修改。

## 4. 标准操作流程

### 4.1 最小化系统服务

禁用非必要服务，减少攻击面：

```bash
# 示例：基于 systemd 的发行版
for svc in cups bluetooth firewalld postfix avahi-daemon; do
  systemctl disable --now $svc 2>/dev/null || true
done
```

保留服务：sshd（仅密钥认证）、chronyd/systemd-timesyncd、containerd、kubelet、auditd、rsyslog/journald。控制面节点还需保留 etcd 相关服务。在禁用任何服务前，应确认该服务不会被 Kubernetes、CNI 或云厂商 agent 依赖。

### 4.2 SSH 与访问控制

```bash
# /etc/ssh/sshd_config
PermitRootLogin no
PasswordAuthentication no
PubkeyAuthentication yes
AllowUsers admin@10.0.0.*
MaxAuthTries 3
ClientAliveInterval 300
ClientAliveCountMax 2
Banner /etc/ssh/banner
```

重启 SSH：

```bash
systemctl restart sshd
```

建议为不同角色配置独立的 sudoers 文件，限制可执行的命令集，并通过 auditd 记录所有 sudo 调用。对于控制面节点，应进一步限制可登录用户列表，并启用多因素认证（MFA）。

### 4.3 磁盘分区规划

建议将以下目录挂载到独立分区或独立磁盘：

| 挂载点 | 建议大小 | 目的 |
|---|---|---|
| / | 50–100 GiB | 操作系统与只读软件包 |
| /var/lib/etcd | 控制面节点 100+ GiB SSD | etcd 数据，要求 fsync P99 < 10 ms |
| /var/lib/containerd | 200–500 GiB SSD | 容器镜像与层数据 |
| /var/lib/kubelet | 100–200 GiB | Pod 数据、emptyDir、secret/configmap |
| /var/log/pods /var/log/audit | 100 GiB+ | 容器日志与审计日志 |

```bash
# fstab 示例（etcd 数据盘）
UUID=<uuid> /var/lib/etcd ext4 noatime,nodiratime,barrier=1 0 2
```

独立分区的价值在于：当容器日志或镜像数据异常增长时，不会导致根分区满而影响系统服务；同时便于针对 etcd 等关键路径使用更高性能的存储。对于 etcd 数据盘，强烈建议使用 RAID-1 或云厂商提供的多副本块存储，以提升可靠性。

### 4.4 内核参数基线

创建 `/etc/sysctl.d/99-k8s-node.conf`：

```ini
# 网络转发与桥接
net.ipv4.ip_forward=1
net.bridge.bridge-nf-call-iptables=1
net.bridge.bridge-nf-call-ip6tables=1

# conntrack
net.netfilter.nf_conntrack_max=1048576

# 文件描述符与 inotify
fs.file-max=2097152
fs.inotify.max_user_watches=524288
fs.inotify.max_user_instances=8192

# 内存与 swap
vm.swappiness=1
vm.overcommit_memory=1
vm.panic_on_oom=0
kernel.panic=10
kernel.panic_on_oops=1

# 禁用源路由与重定向
net.ipv4.conf.all.send_redirects=0
net.ipv4.conf.default.send_redirects=0
net.ipv4.conf.all.accept_source_route=0
net.ipv4.conf.default.accept_source_route=0
```

应用：

```bash
sysctl --system
```

注意：`tcp_tw_recycle` 已废弃，不应再使用；`nf_conntrack_max` 需与 CNI 规模匹配，避免连接跟踪表满导致网络异常。同时，应监控 conntrack 使用率，并在大规模集群中适当提升该值。

### 4.5 auditd 审计策略

创建 `/etc/audit/rules.d/k8s-node.rules`：

```bash
# 监控关键文件变更
-w /etc/kubernetes/ -p wa -k k8s-config
-w /etc/containerd/ -p wa -k containerd-config
-w /etc/cni/ -p wa -k cni-config
-w /usr/bin/dockerd -p x -k docker-daemon
-w /usr/bin/containerd -p x -k containerd-daemon
-w /usr/bin/kubelet -p x -k kubelet

# 监控用户/权限变更
-w /etc/passwd -p wa -k identity
-w /etc/group -p wa -k identity
-w /etc/shadow -p wa -k identity

# 监控特权命令
-a always,exit -F arch=b64 -S setuid -S setgid -S setreuid -S setregid -k privilege
```

重启 auditd：

```bash
systemctl restart auditd
ausearch -k k8s-config --start today
```

auditd 日志应集中采集到 SIEM 或日志平台，保留期限根据合规要求设定，通常不少于 90 天。审计策略应避免过宽，否则会产生大量噪声日志，反而掩盖关键事件。

### 4.6 containerd 加固

编辑 `/etc/containerd/config.toml`：

```toml
version = 2
[plugins."io.containerd.grpc.v1.cri"]
  sandbox_image = "registry.k8s.io/pause:3.9"
  [plugins."io.containerd.grpc.v1.cri".containerd]
    default_runtime_name = "runc"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
      runtime_type = "io.containerd.runc.v2"
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
        SystemdCgroup = true
  [plugins."io.containerd.grpc.v1.cri".registry]
    config_path = "/etc/containerd/certs.d"
```

限制镜像拉取来源，仅允许内部 Harbor/ACR 等可信仓库：

```toml
[plugins."io.containerd.grpc.v1.cri".registry.mirrors]
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
    endpoint = ["https://internal-mirror.example.com"]
```

同时建议启用镜像内容信任（Notary/cosign），拒绝未签名的镜像。对于多租户场景，可以通过 ImagePolicyWebhook 或 Kyverno 进一步限制允许拉取的镜像仓库。

### 4.7 kubelet 加固

在 kubelet 配置中启用：

```yaml
protectKernelDefaults: true
readOnlyPort: 0
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
authorization:
  mode: Webhook
serializeImagePulls: false
 evictionHard:
   memory.available: "500Mi"
   nodefs.available: "10%"
   nodefs.inodesFree: "5%"
```

`protectKernelDefaults: true` 会阻止 kubelet 运行时不安全的内核参数，因此必须在镜像阶段将 sysctl 参数固化正确。该选项是 kubelet 安全加固的重要开关，但也会提高对镜像基线一致性的要求。

### 4.8 更新节奏

- **内核更新**：仅通过镜像替换方式升级，staging 节点验证 ≥24 小时后按节点池灰度滚动。
- **containerd/kubelet 更新**：冻结小版本，高危 CVE 在变更窗口内升级。
- **安全补丁**：高危漏洞应在 7–14 天内修复；极高危漏洞应在 72 小时内启动应急补丁流程。
- **基线复审**：每季度重新执行 CIS 扫描，更新废弃配置项。

更新节奏应在 SRE、安全与业务团队之间达成共识，并纳入变更日历，避免在业务高峰期执行节点替换。

## 5. 关键检查点与验证命令

| 检查项 | 命令 |
|---|---|
| CIS 合规扫描 | `oscap xccdf eval --profile xccdf_org.ssgproject.content_profile_cis_level1_server ...` |
| 开放端口 | `ss -tulnp` |
| 已启用服务 | `systemctl list-unit-files --state=enabled` |
| SSH 配置 | `sshd -T \| grep -E "permitrootlogin|passwordauthentication"` |
| sysctl 生效 | `sysctl -a \| grep -E "ip_forward|conntrack_max|swappiness"` |
| auditd 规则 | `auditctl -l` |
| containerd 配置 | `containerd config dump \| grep -E "sandbox_image|SystemdCgroup"` |
| kubelet 安全参数 | `ps aux \| grep kubelet` |
| 时间同步 | `chronyc tracking` |

## 6. 常见故障与 Remediation

| 现象 | 可能根因 | 处置 |
|---|---|---|
| 节点 NotReady，kubelet 无法启动 | `protectKernelDefaults=true` 与系统 sysctl 冲突 | 检查 `/etc/sysctl.d/` 中参数是否被 kubelet 视为不安全；统一由镜像固化 |
| 容器无法拉取镜像 | containerd 镜像仓库镜像配置错误 | 检查 `config_path` 与证书；使用 `crictl pull` 验证 |
| auditd 日志占满磁盘 | 审计规则过宽 / retention 未配置 | 调整规则粒度；配置 logrotate 或 journald 容量限制 |
| SSH 密钥登录失败 | 权限过宽或 SELinux 限制 | `chmod 700 ~/.ssh; chmod 600 ~/.ssh/authorized_keys`; 检查 audit 日志 |
| 时间漂移导致证书校验失败 | chrony 未运行或上游不可用 | `chronyc tracking`; 配置多个 NTP 源 |
| containerd 沙箱镜像版本不兼容 | sandbox_image 与集群版本不匹配 | 对齐 pause 镜像标签至集群要求版本 |
| 节点启动后 sysctl 未生效 | 参数文件命名顺序或语法错误 | 检查 `/etc/sysctl.d/` 文件名排序；执行 `sysctl --system` 查看报错 |

## 7. 风险与注意事项

- **不要直接在生产节点登录修改**：所有变更应回归镜像构建流水线，登录仅用于排障并需记录审计日志。
- **sysctl 冲突**：启用 `protectKernelDefaults` 后，kubelet 会拒绝不符合安全基线的内核参数，必须在镜像阶段统一设置。
- **升级节奏**：内核与 containerd 升级应在 staging 节点验证 ≥24 小时，确认对 CNI、CSI、GPU 驱动无回归后灰度滚动。
- **合规不是一次性动作**：每季度重新执行 CIS 扫描，对新出现的 CVE 与废弃配置项进行修复。
- **避免过度禁用服务**：某些云厂商 agent（如 cloud-init、qemu-guest-agent）是节点注册与元数据获取所必需，禁用前需确认影响。
- **镜像供应链安全**：使用私有镜像仓库、镜像签名与 SBOM，避免拉取被篡改的公网镜像。

## 8. 相关 Runbook / 推荐阅读

- [[17-系统基础/01-Linux/08-linux-security-hardening.md|Linux 安全加固与合规管理]]
- [[17-系统基础/00-总览/01-production-readiness-operations-guide.md|System Foundation 生产就绪运维指南]]
- [[13-生产运维/00-总览/01-production-readiness-operations-guide.md|生产运维域生产就绪运维指南]]
- [[08-安全/README.md|安全合规域]]
- 节点时间同步指南（待补充）
- systemd 与 kubelet 服务管理（待补充）
- 内核实时补丁指南（待补充）

---

*本基线应根据组织使用的 Linux 发行版、CIS 版本与云厂商基线进行裁剪，建议通过 Infrastructure-as-Code 固化并在每次镜像构建后自动执行合规扫描。*
