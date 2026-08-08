---
title: Kubernetes 节点 OS 问题全清单
description: Kubernetes 节点操作系统相关问题全清单，覆盖生命周期与兼容性、资源耗尽、内核与运行时、网络、Windows 节点、安全 6 大类问题，含现象、根因、诊断命令、解决方案与预防措施
summary: K8s 节点 OS 问题全清单：生命周期/兼容性、资源耗尽、内核运行时、网络、Windows 节点、安全问题及诊断与治理
category: troubleshooting
tags:
- kubernetes
- node
- os
- linux
- windows
- troubleshooting
- disk
- memory
- kernel
- conntrack
- oom
tier: core
created: '2026-08-06'
last_updated: '2026-08-06'
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 平台工程师
- 架构师
estimated_read_time: 30min
intent_queries:
- Kubernetes 节点 OS 有哪些常见问题
- 节点磁盘满怎么办
- 节点 OOM 如何排查
- cgroup 驱动不一致导致节点 NotReady
- conntrack 表满怎么解决
- 内核模块缺失 CNI 失败
- 发行版 EOL 对 K8s 集群的影响
- Windows 节点常见问题
trigger_keywords:
- 节点问题
- OS 问题
- 磁盘满
- inode 耗尽
- OOM
- conntrack
- cgroup
- 内核 panic
- 时钟偏移
- SELinux
- AppArmor
- 发行版 EOL
prerequisites:
- kubectl-basics
- linux-basics
- troubleshooting-methodology
k8s_versions:
- '1.25'
- '1.26'
- '1.27'
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Kubernetes 节点 OS 问题全清单

> 节点操作系统问题占 K8s 生产故障的相当比例。本文档按问题类别整理 OS 相关的常见问题、故障现象、根因分析、诊断命令与治理措施，作为节点 OS 运维的权威参考。配套文档见 [[17-系统基础/01-Linux/16-k8s-node-os-support-matrix.md|K8s 节点 OS 支持矩阵]]。

## 问题总览速查表

| 类别 | 问题 | 高频度 | 严重度 | 核心信号 |
|------|------|:------:|:------:|---------|
| 兼容性 | 发行版 EOL | 低 | 高 | 无安全补丁 |
| 兼容性 | OS 与 K8s 版本失配 | 低 | 高 | kubelet 启动失败 |
| 兼容性 | 内核版本过低 | 中 | 中 | 特性不可用 |
| 兼容性 | 自动更新破坏集群 | 中 | 高 | 节点重启后 NotReady |
| 兼容性 | 架构不匹配 | 低 | 中 | Pod 调度失败 |
| 资源 | **磁盘满** | **高** | **高** | DiskPressure、驱逐 |
| 资源 | inode 耗尽 | 中 | 高 | 无法创建文件 |
| 资源 | **内存不足 / OOM** | **高** | **高** | MemoryPressure、OOM Kill |
| 资源 | PID 耗尽 | 中 | 中 | 无法创建进程 |
| 资源 | 文件描述符耗尽 | 中 | 中 | 连接失败 |
| 内核 | **cgroup 驱动不一致** | **高** | **高** | 节点 NotReady |
| 内核 | 内核模块缺失 | 高 | 高 | CNI/CSI 失败 |
| 内核 | swap 未禁用 | 低 | 中 | kubelet 拒绝启动 |
| 内核 | 内核 panic | 低 | 极高 | 节点宕机 |
| 内核 | 时钟偏移 | 中 | 高 | 证书验证失败 |
| 网络 | ip_forward 未开启 | 中 | 高 | Pod 网络不通 |
| 网络 | **conntrack 表满** | **高** | **高** | 新连接被丢弃 |
| 网络 | iptables 规则冲突 | 中 | 高 | 服务间歇性不可达 |
| 网络 | 网卡驱动异常 | 低 | 中 | 丢包、错包 |
| Windows | 镜像体积大 | 高 | 低 | 拉取慢 |
| Windows | 资源隔离不完善 | 中 | 高 | 无 cgroup v2 |
| Windows | CNI/CSI 支持受限 | 中 | 中 | 部分插件不可用 |
| 安全 | SELinux/AppArmor 配置错误 | 中 | 高 | Pod 启动失败 |
| 安全 | 内核漏洞未修补 | 低 | 高 | 安全事件 |
| 安全 | 不可变 OS 更新通道异常 | 低 | 中 | 节点长期无补丁 |

---

## 一、生命周期与兼容性问题（选型/升级阶段）

这类问题在节点交付前就存在，通常在集群扩容、升级或合规审计时暴露。**预防成本远低于修复成本。**

### 1.1 发行版 EOL（End of Life）

| 项目 | 内容 |
|------|------|
| **现象** | 发行版停止官方维护，无安全补丁；部分软件源失效导致无法安装/更新软件；合规审计不通过 |
| **根因** | 未规划发行版生命周期；历史遗留（如 CentOS 7 于 2024-06 EOL） |
| **影响** | 节点存在已知漏洞风险；新 kubelet/containerd 版本可能不再提供对应包 |

**诊断命令：**

```bash
# 🟢 查看发行版版本与 EOL 状态
cat /etc/os-release
cat /etc/redhat-release 2>/dev/null
# 检查软件源是否可用
apt update 2>&1 | tail -5   # Debian/Ubuntu
yum makecache 2>&1 | tail -5   # RHEL 系
# 检查已知 CVE 状态（需借助外部工具）
trivy image --severity HIGH,CRITICAL <node-image>
```

**解决方案：**

1. 制定发行版生命周期管理计划，在 EOL 前 6-12 个月启动迁移
2. 迁移路径参考 [[17-系统基础/01-Linux/16-k8s-node-os-support-matrix.md|OS 支持矩阵]] 中"已退役/不推荐"对照表
3. 云托管集群优先跟随厂商默认 OS（厂商负责维护）

**预防措施：**

- 建立发行版版本台账，标注 EOL 日期并设置提前告警
- 新集群一律使用处于标准支持期的发行版（Ubuntu LTS、RHEL 9 等）
- 生产环境禁止使用滚动发行版（Arch、Fedora 非 Server 版）

### 1.2 OS 与 K8s 版本失配

| 项目 | 内容 |
|------|------|
| **现象** | 升级 K8s 后部分节点 kubelet 无法启动；`kubeadm init/join` 报版本不支持；新特性不生效 |
| **根因** | 未核对 K8s 版本的 validated distributions 列表；跨多个 minor 版本升级未检查 OS 兼容性 |
| **影响** | 集群版本不一致，混合版本节点管理复杂，严重时集群无法升级 |

**诊断命令：**

```bash
# 🟢 查看节点 OS 与 kubelet 版本
kubectl get nodes -o wide
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\t"}{.status.nodeInfo.osImage}{"\n"}{end}'
# 查看 kubelet 日志中的版本错误
journalctl -u kubelet --no-pager | grep -i "unsupported\|version" | tail -20
```

**解决方案：**

- 升级前核对 [Kubernetes CHANGELOG](https://git.k8s.io/kubernetes/CHANGELOG) 的 validated distributions 列表
- OS 与 K8s 按"先 OS 后 K8s"顺序升级，避免同时变更引入复合故障
- 使用 `kubeadm upgrade plan` 预检兼容性

**预防措施：**

- 升级前在测试环境完整验证 OS × K8s 组合
- 维护集群版本与节点 OS 版本的对照台账（见支持矩阵的"卸载周期对照"章节）

### 1.3 内核版本过低

| 项目 | 内容 |
|------|------|
| **现象** | cgroup v2 不可用；eBPF 型 CNI（Cilium）功能异常；io_uring 无法使用；`kubeadm` 预检警告 |
| **根因** | 发行版自带内核过旧或长期未升级；云镜像内核锁定 |
| **影响** | 关键特性缺失，性能受限，部分组件无法运行 |

**诊断命令：**

```bash
# 🟢 查看内核版本
uname -r
# 查看 cgroup 版本（v1 或 v2）
stat -fc %T /sys/fs/cgroup
# 检查内核特性
cat /boot/config-$(uname -r) | grep CONFIG_CGROUP_BPF
```

**内核版本要求速查：**

| 功能 | 最低内核 | 推荐内核 |
|------|---------|---------|
| Kubernetes 基线 | 4.15 | 5.15+ |
| cgroup v2 | 5.2 | 6.2+ |
| eBPF（Cilium/Calico） | 4.18 | 5.10+ |
| BPF CO-RE | 5.10 | 6.2+ |
| nftables | 4.18 | 6.4+ |

**解决方案：**

- 升级内核包（🔴 高风险：需滚动重启节点，先排空再重启）
- 云环境更换为内核较新的镜像（如 Ubuntu 22.04+ / COS）
- 无法升级时降级功能需求（如改用 iptables 模式 CNI）

### 1.4 自动更新破坏集群

| 项目 | 内容 |
|------|------|
| **现象** | 节点自动重启后 NotReady；重启后 kubelet/containerd 无法启动；内核与运行时版本不匹配 |
| **根因** | 云镜像自动安全更新（如 COS auto-update、unattended-upgrades）升级了内核或运行时；重启导致集群批量节点同时离线 |
| **影响** | 批量节点同时重启 = 集群容量骤降，控制面抖动 |

**诊断命令：**

```bash
# 🟢 查看节点重启历史
last reboot
uptime
# 查看节点重启后 kubelet 状态
systemctl status kubelet --no-pager
journalctl -u kubelet --no-pager | tail -50
# 查看自动更新配置（Ubuntu/Debian）
cat /etc/apt/apt.conf.d/20auto-upgrades 2>/dev/null
```

**解决方案：**

- 生产节点**关闭**内核自动更新，改为受控维护窗口更新
- 云厂商镜像如需自动更新，配置为"只更新容器运行时、不更新内核"
- 更新后执行节点排空-重启-恢复的滚动流程

**预防措施：**

- 节点更新策略：统一走维护窗口（如每月一次），配合 `kubectl drain`
- 使用不可变 OS（Flatcar/Talos）时，更新前在测试节点验证新版本
- 监控节点重启事件（`kubectl get events --field-selector reason=Rebooted`）

### 1.5 架构不匹配

| 项目 | 内容 |
|------|------|
| **现象** | Pod 处于 Pending，事件报 `0/5 nodes are available` + `Incompatible architecture`；镜像拉取失败 |
| **根因** | 节点架构（如 arm64）与镜像架构（amd64）不匹配；多架构集群未配置镜像 manifest list |
| **影响** | 工作负载无法调度到特定架构节点 |

**诊断命令：**

```bash
# 🟢 查看节点架构
kubectl get nodes -o wide --show-labels | grep kubernetes.io/arch
# 查看 Pod 调度失败原因
kubectl describe pod <pod-name> | grep -A5 Events
# 查看节点标签
kubectl get node <node-name> --show-labels
```

**解决方案：**

- 使用多架构镜像（`docker buildx build --platform linux/amd64,linux/arm64`）
- 为异构节点池打架构标签，用 nodeSelector/亲和性调度
- 边缘场景使用 K3s + arm64 镜像方案

---

## 二、资源耗尽类问题（运行阶段，最高频）

资源耗尽类问题占 OS 相关故障的比例最高，且**通常有先兆指标可监控**，属于"应该被提前发现"的问题。

### 2.1 磁盘满（DiskPressure）

| 项目 | 内容 |
|------|------|
| **现象** | 节点条件 `DiskPressure=True`；kubelet 开始驱逐 Pod；容器创建失败报 `no space left on device`；镜像拉取失败 |
| **根因** | 容器日志/镜像/挂载卷占满根分区或 `/var/lib/containerd` 分区；临时文件堆积；inotify 事件激增 |
| **影响** | Pod 被驱逐，服务中断；严重时节点进入只读状态 |

**诊断命令：**

```bash
# 🟢 磁盘使用率（重点看 / 和 /var/lib）
df -h
df -h /var/lib/containerd
# 定位大目录
du -sh /var/lib/containerd/* 2>/dev/null | sort -rh | head -10
du -sh /var/log/* 2>/dev/null | sort -rh | head -10
# 查看驱逐阈值配置
kubectl get node <node-name> -o yaml | grep -A10 "evictionHard\|evictionSoft"
# 查看节点条件
kubectl describe node <node-name> | grep -A20 "Conditions"
```

**解决方案：**

- 应急：清理无主镜像与无用日志
  ```bash
  # 🟡 清理无主镜像（不影响运行中容器）
  crictl rmi --prune
  # 🟡 清理已停止容器日志（需先确认无审计需求）
  journalctl --vacuum-time=7d
  ```
- 根治：为 `/var/lib/containerd` 单独分区并扩容；配置 kubelet 日志轮转
- 治理：配置磁盘使用率监控告警（80% 警告、90% 紧急）

**预防措施：**

- 节点初始化时按支持矩阵规范分区（见 [[17-系统基础/01-Linux/11-k8s-node-os-image-hardening-baseline.md|OS 加固基线]] 分区章节）
- 容器日志限制：`kubelet` 配置 `--container-log-max-size=100Mi --container-log-max-files=5`
- 镜像垃圾回收策略：`imageGCHighThresholdPercent=85, imageGCLowThresholdPercent=80`

### 2.2 inode 耗尽

| 项目 | 内容 |
|------|------|
| **现象** | `df -h` 显示有空间但写文件报 `No space left on device`；Pod 反复 CrashLoop；容器启动失败 |
| **根因** | 小文件过多（容器层、日志、缓存）耗尽 inode；`df -i` 使用率 100% |
| **影响** | 节点上所有写操作失败，Pod 无法创建/重启 |

**诊断命令：**

```bash
# 🟢 查看 inode 使用率
df -i
df -i /var/lib/containerd
# 定位 inode 占用大户
find /var/lib/containerd -xdev -type f | wc -l
find / -xdev -type d -size +1M 2>/dev/null | head -20
```

**解决方案：**

- 应急：清理临时目录 `/tmp`、容器快照
  ```bash
  # 🟡 删除无主容器快照
  crictl rmi --prune
  # 🟡 清理空目录与临时文件
  find /var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/snapshots -maxdepth 1 -empty -delete
  ```
- 根治：重新分区（提高 inode 比例或改用支持大 inode 的文件系统）
- 治理：监控 `df -i`，inode 使用率 ≥ 90% 告警

**预防措施：**

- 镜像瘦身（减少层数）；日志按天轮转并压缩归档
- 使用 XFS（inode 动态分配）替代 ext4（固定 inode 数）

### 2.3 内存不足 / OOM（MemoryPressure）

| 项目 | 内容 |
|------|------|
| **现象** | 节点条件 `MemoryPressure=True`；Pod 被 OOM Kill（`ExitCode 137`）；系统 OOM Killer 杀掉关键进程（kubelet/containerd）|
| **根因** | Pod 内存超限；节点内存碎片；内核 `vm.overcommit_memory` 配置不当；swap 与 cgroup 交互异常 |
| **影响** | 服务中断；严重时 kubelet 被杀导致节点 NotReady |

**诊断命令：**

```bash
# 🟢 节点内存概览
free -h
# 🟢 查看 OOM 记录
dmesg -T | grep -i "oom\|killed process" | tail -20
journalctl -k --no-pager | grep -i oom | tail -20
# 🟢 查看节点内存条件
kubectl describe node <node-name> | grep -A10 "MemoryPressure"
# 🟢 查看节点内存分配
kubectl top node <node-name>
```

**解决方案：**

- 应急：扩容节点 / 驱逐低优先级 Pod
  ```bash
  # 🟡 驱逐指定 Pod（自动遵守 PDB）
  kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
  ```
- 根治：调整 Pod request/limit；配置合理的驱逐阈值（`evictionHard.memory.available<500Mi`）
- 治理：为关键系统进程设置 `oom_score_adj` 保护
  ```bash
  # 🟡 降低 kubelet/containerd 被 OOM Kill 概率（写入 systemd unit 或 rc.local）
  echo -1000 > /proc/$(pidof kubelet)/oom_score_adj
  ```

**预防措施：**

- 严格规范 Pod 资源 request/limit；限制 namespace 配额（见 [[19-故障诊断/02-资源排障/16-quota-limitrange-troubleshooting.md|Quota/LimitRange 排障]]）
- 预留节点系统保留内存：`--system-reserved=memory=1Gi --kube-reserved=memory=1Gi`
- 监控节点内存使用率与 OOM 事件，联动告警

### 2.4 PID 耗尽

| 项目 | 内容 |
|------|------|
| **现象** | 节点报 `fork: Cannot allocate memory`；容器内无法创建进程；kubelet 报 PID 压力 |
| **根因** | `kernel.pid_max` 过小；Pod 内 PID 泄漏（僵尸进程）；无 `--pod-max-pids` 限制 |
| **影响** | 新 Pod 无法启动，已有工作负载功能异常 |

**诊断命令：**

```bash
# 🟢 当前 PID 使用
cat /proc/sys/kernel/pid_max
ps -eLf | wc -l
# 查看僵尸进程
ps aux | awk '$8 ~ /Z/'
```

**解决方案：**

- 临时调大：`sysctl -w kernel.pid_max=4194304`（🟡 中风险，重启失效）
- 永久生效：写入 `/etc/sysctl.d/99-k8s.conf`
- 配置 kubelet `--pod-max-pids=4096` 限制单 Pod 进程数
- 治理僵尸进程源头（应用自身问题）

### 2.5 文件描述符耗尽

| 项目 | 内容 |
|------|------|
| **现象** | 应用报 `Too many open files`；kubelet/containerd 连接失败；TCP 连接数异常高 |
| **根因** | `ulimit -n` 限制过低（默认 1024）；连接泄漏；`fs.file-max` 系统级限制 |
| **影响** | 服务连接失败，节点组件异常 |

**诊断命令：**

```bash
# 🟢 查看限制
ulimit -n
cat /proc/sys/fs/file-max
cat /proc/sys/fs/nr_open
# 🟢 查看当前打开数
cat /proc/sys/fs/file-nr
# 🟢 查看进程 FD 使用 TOP
for pid in $(ls /proc | grep -E '^[0-9]+$'); do echo "$pid $(ls /proc/$pid/fd 2>/dev/null | wc -l)"; done | sort -k2 -rn | head -10
```

**解决方案：**

- 提高系统限制：`fs.file-max = 1048576`、`fs.nr_open = 1048576`（写入 sysctl）
- 提高进程限制：systemd unit 中 `LimitNOFILE=1048576`
- 应用层排查连接泄漏（通常为 HTTP 连接池未复用）

---

## 三、内核与运行时问题

### 3.1 cgroup 驱动不一致（经典问题）

| 项目 | 内容 |
|------|------|
| **现象** | 节点 NotReady；kubelet 报 `failed to run Kubelet: failed to create kubelet config` 或 `systemd cgroup driver` 相关错误 |
| **根因** | kubelet 配置 `cgroupDriver=systemd` 而 containerd 使用 `cgroupfs`（或反之）；两者必须一致 |
| **影响** | 节点无法加入集群，是生产环境最高频的节点初始化问题之一 |

**诊断命令：**

```bash
# 🟢 kubelet 的 cgroup 驱动
cat /var/lib/kubelet/config.yaml | grep cgroupDriver
# 🟢 containerd 的 cgroup 驱动
containerd config dump | grep -i cgroup
# 🟢 查看 kubelet 启动错误
journalctl -u kubelet --no-pager | grep -i "cgroup" | tail -10
```

**解决方案：**

- 统一为 `systemd` 驱动（社区推荐）：
  ```bash
  # 🟡 修改 containerd 配置
  containerd config default > /etc/containerd/config.toml
  sed -i 's/SystemdCgroup = false/SystemdCgroup = true/' /etc/containerd/config.toml
  systemctl restart containerd
  # 🟡 确认 kubelet 使用 systemd 驱动
  # /var/lib/kubelet/config.yaml 中 cgroupDriver: systemd
  systemctl restart kubelet
  ```

**预防措施：**

- 节点初始化脚本统一固化两份配置，加入一致性校验步骤
- 使用 kubeadm 时确认 `kubeadm init --config` 中的 `cgroupDriver` 字段

### 3.2 内核模块缺失

| 项目 | 内容 |
|------|------|
| **现象** | CNI 初始化失败（Pod 无 IP）；CSI 挂载失败；`modprobe br_netfilter` 报错；节点网络异常 |
| **根因** | 发行版精简安装未包含必要内核模块：`br_netfilter`、`overlay`、`ip_vs`（IPVS 模式 kube-proxy 需要） |
| **影响** | Pod 网络不可用，存储挂载失败，集群功能瘫痪 |

**诊断命令：**

```bash
# 🟢 检查关键模块
lsmod | grep -E "br_netfilter|overlay|ip_vs"
# 🟢 尝试加载
modprobe br_netfilter && modprobe overlay && modprobe ip_vs
# 🟢 确认转发相关 sysctl
sysctl net.bridge.bridge-nf-call-iptables
```

**解决方案：**

- 永久加载模块：
  ```bash
  # 🟡 写入模块加载配置
  cat > /etc/modules-load.d/k8s.conf <<'EOF'
  br_netfilter
  overlay
  ip_vs
  ip_vs_rr
  ip_vs_wrr
  ip_vs_sh
  nf_conntrack
  EOF
  systemctl restart systemd-modules-load
  ```
- 设置转发参数：
  ```bash
  # 🟡 写入 sysctl（节点初始化必做）
  cat > /etc/sysctl.d/99-k8s.conf <<'EOF'
  net.ipv4.ip_forward = 1
  net.bridge.bridge-nf-call-iptables = 1
  net.bridge.bridge-nf-call-ip6tables = 1
  EOF
  sysctl --system
  ```

**预防措施：**

- 节点交付脚本包含模块加载与校验清单（见 [[17-系统基础/01-Linux/index.md|Linux 索引]] 的"节点就绪检查"章节）
- 使用云厂商托管节点时默认已配置，自建节点必须显式配置

### 3.3 swap 未禁用

| 项目 | 内容 |
|------|------|
| **现象** | kubelet 启动失败：`running with swap on is not supported`（K8s < 1.22）；K8s 1.22+ 默认警告（`NodeSwap` 特性门控） |
| **根因** | 节点存在 swap 分区/文件，kubelet 检测到 swap |
| **影响** | 旧版本无法加入集群；新版本性能不可预测 |

**诊断命令：**

```bash
# 🟢 查看 swap
swapon --show
free -h | grep -i swap
```

**解决方案：**

- 禁用 swap：
  ```bash
  # 🔴 高风险：会清空 swap 内容，先确认无依赖
  swapoff -a
  sed -i '/swap/d' /etc/fstab
  ```
- 或启用 K8s 1.28+ 的 `NodeSwap` 特性（`--feature-gates=NodeSwap=true`），需理解其行为后再用

### 3.4 内核 panic

| 项目 | 内容 |
|------|------|
| **现象** | 节点宕机重启；`dmesg` 有 panic 堆栈；节点监控断点 |
| **根因** | 硬件故障（内存、CPU）、驱动缺陷（网卡/存储）、内核 bug |
| **影响** | 节点完全不可用，Pod 重新调度，有状态服务可能受损 |

**诊断命令：**

```bash
# 🟢 查看 panic 记录（重启后）
journalctl -k --no-pager -b -1 | grep -i "panic\|Oops" | tail -20
# 检查硬件错误（需启用 EDAC）
ls /sys/devices/system/edac/mc/ 2>/dev/null
dmesg | grep -i "hardware error\|mce"
```

**解决方案：**

- 应急：节点重建/替换（内核 panic 后节点状态不可信）
- 收集 panic 堆栈提交给内核/驱动厂商
- 排查硬件：内存测试（memtest86+）、磁盘 SMART 检查

**预防措施：**

- 升级到稳定内核版本；避免使用有已知 panic 问题的驱动组合
- 有状态工作负载做好跨节点容灾

### 3.5 时钟偏移（Clock Drift）

| 项目 | 内容 |
|------|------|
| **现象** | `x509: certificate has expired or is not yet valid`；Token 验证失败；日志时间戳错乱；Prometheus 数据时间线混乱 |
| **根因** | NTP/chrony 未配置或失效；虚拟机时钟（vDSO）漂移；物理机 CMOS 电池问题 |
| **影响** | 集群安全通信失败（证书/Token 校验），审计与监控失真 |

**诊断命令：**

```bash
# 🟢 查看当前时间偏差
chronyc tracking 2>/dev/null | grep -E "System time|Stratum"
timedatectl
# 🟢 对比控制面与节点时间
date; kubectl get node <node-name> -o wide
```

**解决方案：**

- 配置 chrony/ntpd 并启用开机自启：
  ```bash
  # 🟡 安装并启动 chrony
  apt install -y chrony  # 或 yum install -y chrony
  systemctl enable --now chronyd
  chronyc makestep
  ```
- 云环境启用主机时间同步服务（如 AWS 的 chrony 配置）
- 容器内使用挂载的宿主机 `/etc/localtime`

**预防措施：**

- 节点交付检查 `chronyc tracking` stratum ≤ 3
- 监控时钟偏移指标（`node_timex_offset_seconds`）

---

## 四、网络相关问题

### 4.1 ip_forward 未开启

| 项目 | 内容 |
|------|------|
| **现象** | Pod 间通信正常但 Pod 访问外部网络失败；`kube-proxy` 转发异常；NodePort 外部访问不通 |
| **根因** | `net.ipv4.ip_forward=0`（部分发行版默认关闭）；容器网络依赖宿主机 IP 转发 |
| **影响** | 集群网络核心功能不可用 |

**诊断命令：**

```bash
# 🟢 检查
sysctl net.ipv4.ip_forward
cat /proc/sys/net/ipv4/ip_forward
```

**解决方案：**

```bash
# 🟡 开启并持久化
sysctl -w net.ipv4.ip_forward=1
echo "net.ipv4.ip_forward = 1" >> /etc/sysctl.d/99-k8s.conf
sysctl --system
```

### 4.2 conntrack 表满

| 项目 | 内容 |
|------|------|
| **现象** | 新 TCP 连接被丢弃（表现为服务间歇性超时）；`dmesg` 报 `nf_conntrack: table full, dropping packet`；监控可见 conntrack 使用率接近上限 |
| **根因** | 高并发连接（尤其短连接场景）打满 `nf_conntrack_max`；`nf_conntrack_tcp_timeout_established` 过长导致条目堆积 |
| **影响** | 服务不可用，是生产环境**高频且隐蔽**的网络故障 |

**诊断命令：**

```bash
# 🟢 当前使用与上限
sysctl net.netfilter.nf_conntrack_max
cat /proc/sys/net/netfilter/nf_conntrack_count
conntrack -L 2>/dev/null | wc -l
# 🟢 查看丢包记录
dmesg | grep -i "nf_conntrack" | tail -10
# 🟢 查看超时配置
sysctl net.netfilter.nf_conntrack_tcp_timeout_established
```

**解决方案：**

- 应急扩容：
  ```bash
  # 🟡 调大 conntrack（需评估内存：每个条目约 350B）
  sysctl -w net.netfilter.nf_conntrack_max=1048576
  ```
- 缩短长连接超时（释放堆积条目）：
  ```bash
  # 🟡 建议值：600s（原 86400s 过长）
  sysctl -w net.netfilter.nf_conntrack_tcp_timeout_established=600
  ```
- 根治：改造应用减少短连接（连接池）；大集群考虑切换到 IPVS 或 eBPF 模式 kube-proxy

**预防措施：**

- 按"集群规模 × 单节点预估连接数"预置 `nf_conntrack_max`
- 监控 `conntrack 使用率 = count/max`，≥ 80% 告警
- 高 QPS 服务优先 IPVS 模式（`kube-proxy --proxy-mode=ipvs`）

### 4.3 iptables 规则冲突

| 项目 | 内容 |
|------|------|
| **现象** | Service 间歇性不可达；某些端口被莫名丢弃；防火墙规则与 kube-proxy 规则互相覆盖 |
| **根因** | OS 自带防火墙（firewalld/ufw）插入规则与 kube-proxy 的规则链冲突；多 CNI 共存 |
| **影响** | 网络行为不可预测，排查困难 |

**诊断命令：**

```bash
# 🟢 查看规则链
iptables -t nat -L -n -v | head -50
iptables -L -n -v | head -50
# 🟢 查看 kube-proxy 与防火墙服务状态
systemctl status kube-proxy firewalld ufw --no-pager 2>/dev/null
```

**解决方案：**

- 生产节点禁用 firewalld/ufw，统一由 CNI/kube-proxy 管理规则：
  ```bash
  # 🟡 禁用防火墙（需确认无其他依赖）
  systemctl disable --now firewalld
  systemctl stop firewalld
  ```
- 若必须保留防火墙：规则必须放行 kubelet（10250）、NodePort 段等，并测试全链路

### 4.4 网卡驱动异常

| 项目 | 内容 |
|------|------|
| **现象** | 高负载下丢包/错包；`ethtool -S` 显示 rx_errors/dropped 增长；节点网络延迟抖动 |
| **根因** | 驱动 bug、固件过旧、多队列未开启（RSS）、ring buffer 过小 |
| **影响** | 网络性能劣化，Pod 服务质量下降 |

**诊断命令：**

```bash
# 🟢 查看驱动与队列
ethtool -i eth0
ethtool -l eth0
# 🟢 查看错误计数
ethtool -S eth0 | grep -i "error\|drop" | head -20
# 🟢 查看 ring buffer
ethtool -g eth0
```

**解决方案：**

- 更新驱动/固件（🔴 高风险：需维护窗口）
- 开启多队列：`ethtool -L eth0 combined 8`（需网卡支持）
- 增大 ring buffer：`ethtool -G eth0 rx 4096 tx 4096`

---

## 五、Windows 节点特有

Windows 节点的问题与 Linux 差异显著，详见 [[17-系统基础/01-Linux/14-windows-containers-k8s.md|Windows 容器实践]] 与 [[01-集群基础/01-架构总览/08-windows-containers-support.md|Windows 容器支持]]。

### 5.1 镜像体积大

| 项目 | 内容 |
|------|------|
| **现象** | Windows 容器镜像 4-8GB，拉取耗时数分钟；节点启动/扩容速度慢 |
| **根因** | Windows 基础镜像包含完整系统库，无法像 Linux 一样精简 |
| **影响** | 扩容效率低，Pod 调度延迟 |

**解决方案：**

- 使用 Windows Server Core（4GB+）而非 Full（8GB+）；评估 Nano Server 可行性
- 集群内预拉镜像（DaemonSet 预热）
- 使用镜像缓存/加速器

### 5.2 资源隔离不完善

| 项目 | 内容 |
|------|------|
| **现象** | 无 cgroup v2；内存/CPU 限制精度差；单容器 OOM 可能影响同节点其他容器 |
| **根因** | Windows 使用 Job Objects，隔离粒度与 Linux cgroup 不同 |
| **影响** | 资源超卖风险高，故障隔离弱 |

**解决方案：**

- 为 Windows Pod 设置保守的资源 limit
- 监控层面对 Windows 节点使用单独的指标采集方案（如 Windows Exporter）

### 5.3 CNI/CSI 支持受限

| 项目 | 内容 |
|------|------|
| **现象** | 部分 CNI（如 Cilium eBPF、Calico VXLAN 部分模式）不支持 Windows；CSI 驱动缺失 |
| **根因** | Windows 无 eBPF，网络插件适配成本高；存储插件厂商支持不齐 |
| **影响** | 混合集群网络方案受限，存储方案选择少 |

**解决方案：**

- 网络：使用支持 Windows 的 CNI（Flannel host-gateway / Calico 特定模式）
- 存储：选择官方支持 Windows 的 CSI（如 Azure Disk/File）
- 混合集群中 Windows 节点单独使用 `nodeSelector` 标记

---

## 六、安全问题

### 6.1 SELinux / AppArmor 配置错误

| 项目 | 内容 |
|------|------|
| **现象** | Pod 启动失败：`Permission denied`（实际为 SELinux 拒绝）；`failed to generate spec: no seccomp profile`；存储挂载后无读写权限 |
| **根因** | SELinux 强制模式（Enforcing）下容器标签不匹配；AppArmor profile 未加载；容器运行时与 LSM 交互异常 |
| **影响** | 应用无法启动或功能异常，且错误信息有误导性 |

**诊断命令：**

```bash
# 🟢 查看 SELinux 状态（RHEL 系）
getenforce
sestatus
# 🟢 查看 AVC 拒绝日志
ausearch -m avc -ts recent 2>/dev/null | tail -20
# 🟢 查看 AppArmor 状态（Ubuntu/Debian）
aa-status
```

**解决方案：**

- 正确配置而非关闭：为容器运行时创建匹配的 SELinux 策略或使用 `container_t` 类型
- containerd 开启 SELinux 支持：`selinux = true`（config.toml）
- AppArmor：加载对应 profile 并设置 Pod annotation
- 仅排障时可临时 `setenforce 0`（🟡 中风险，需立即恢复）

### 6.2 内核漏洞未修补

| 项目 | 内容 |
|------|------|
| **现象** | 安全扫描发现节点内核存在已知 CVE（如 Dirty Pipe、脏牛）；无法通过安全审计 |
| **根因** | 内核更新滞后；不可变 OS 更新通道未开启或未执行 |
| **影响** | 容器逃逸/提权风险，合规风险 |

**解决方案：**

- 建立节点内核补丁节奏（月度/季度维护窗口）
- 使用 `kured` 或云厂商节点维护机制自动重启以应用内核补丁
- 高危 CVE 走紧急通道立即修复

### 6.3 不可变 OS 更新通道异常

| 项目 | 内容 |
|------|------|
| **现象** | Flatcar/Talos 节点长期停留在旧版本；更新后容器运行时与内核不匹配；无法回滚 |
| **根因** | 更新通道配置错误；版本锁定策略过严；更新后未做兼容性验证 |
| **影响** | 安全补丁缺失；更新事故影响面大 |

**解决方案：**

- 配置分阶段更新：先灰度池（5-10%），验证后全量
- 更新前记录当前版本（`flatcar_update` / `talosctl version`），失败可回滚上一分区
- 与 K8s 升级联动规划（OS 更新和 K8s 升级不要同时进行）

---

## 七、OS 问题诊断方法论

### 7.1 诊断顺序

```text
1. 节点状态 → kubectl get nodes / describe node（看 Conditions）
2. 资源概览 → free / df / df -i / top（排除资源耗尽）
3. 内核日志 → dmesg / journalctl -k（找内核线索）
4. 组件日志 → journalctl -u kubelet / -u containerd
5. 网络状态 → sysctl 网络参数 / conntrack / iptables
6. 时间同步 → chronyc tracking
7. 汇总根因 → 按本清单定位到具体问题
```

### 7.2 一键采集脚本（诊断信息收集）

```bash
# 🟢 只读：收集节点诊断信息
collect_node_info() {
  OUT=/tmp/node-diag-$(hostname)-$(date +%Y%m%d%H%M%S)
  mkdir -p $OUT
  {
    echo "=== OS ==="; cat /etc/os-release
    echo "=== KERNEL ==="; uname -a
    echo "=== UPTIME ==="; uptime
    echo "=== MEMORY ==="; free -h
    echo "=== DISK ==="; df -h; df -i
    echo "=== SWAP ==="; swapon --show
    echo "=== CGROUP ==="; stat -fc %T /sys/fs/cgroup
    echo "=== SYSCTL ==="; sysctl net.ipv4.ip_forward net.bridge.bridge-nf-call-iptables net.netfilter.nf_conntrack_max 2>/dev/null
    echo "=== CONNTRACK ==="; cat /proc/sys/net/netfilter/nf_conntrack_count
    echo "=== KERNEL MODULES ==="; lsmod | grep -E "br_netfilter|overlay|ip_vs|nf_conntrack"
    echo "=== TIME ==="; chronyc tracking 2>/dev/null; timedatectl
    echo "=== KUBELET ==="; systemctl status kubelet --no-pager | head -20
    echo "=== OOM ==="; dmesg -T | grep -i "oom\|killed" | tail -10
  } > $OUT/diag.txt 2>&1
  echo "诊断信息已保存: $OUT/diag.txt"
}
```

### 7.3 问题优先级建议

| 优先级 | 问题 | 处理时限 |
|:------:|------|---------|
| P0 | 内核 panic、磁盘满（节点只读）、OOM 持续 | 立即 |
| P1 | conntrack 满、cgroup 驱动错误、内核模块缺失 | 30 分钟内 |
| P2 | 时钟偏移、SELinux 错误、网卡驱动 | 当日 |
| P3 | 发行版 EOL、内核补丁、不可变 OS 更新 | 按计划 |

---

## 相关文档

- [[17-系统基础/01-Linux/16-k8s-node-os-support-matrix.md|K8s 节点 OS 支持矩阵]]
- [[17-系统基础/01-Linux/11-k8s-node-os-image-hardening-baseline.md|K8s 节点 OS 镜像加固基线]]
- [[17-系统基础/01-Linux/13-kernel-tuning-container-performance.md|内核调优与容器性能]]
- [[17-系统基础/01-Linux/14-windows-containers-k8s.md|Windows 容器实践]]
- [[19-故障诊断/02-资源排障/01-node-comprehensive-troubleshooting.md|Node 全面故障排查]]
- [[19-故障诊断/03-基础设施排障/01-network-connectivity-troubleshooting.md|网络连通性排障]]
- [[19-故障诊断/03-基础设施排障/09-performance-bottleneck-troubleshooting.md|性能瓶颈排障]]
- [[23-实体/15-参考与索引/k8s-node-create.md|Kubernetes 节点管理操作指南]]

## 参考资料

- [Kubernetes Node 条件与驱逐文档](https://kubernetes.io/docs/concepts/scheduling-eviction/node-pressure-eviction/)
- [Kubernetes 节点问题诊断](https://kubernetes.io/docs/tasks/debug/debug-cluster/)
- [Linux 内核 cgroup v2 文档](https://docs.kernel.org/admin-guide/cgroup-v2.html)
- [conntrack 与网络调试](https://wiki.nftables.org/)
- [Kubernetes Windows 支持](https://kubernetes.io/docs/setup/best-practices/windows/)
