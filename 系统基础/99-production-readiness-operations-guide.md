---
title: System Foundation 生产就绪运维指南
description: 面向 System Foundation 域（Linux、硬件、K8s 事件）的生产就绪检查、日常运维与故障排查指南
summary: 面向 System Foundation 域（Linux、硬件、K8s 事件）的生产就绪检查、日常运维与故障排查指南
category: system-foundation
tags:
- production
- best-practices
- system-foundation
- operations
- linux
- hardware
- kubernetes-events
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- System Foundation 生产就绪运维指南是什么
- 如何按生产环境要求运维 System Foundation
trigger_keywords:
- 生产就绪
- 运维指南
- System Foundation
- Linux
- 硬件
- K8s 事件
prerequisites:
- kubectl-basics
- linux-basics
- hardware-basics
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# System Foundation 生产就绪运维指南

System Foundation 是 Kubernetes 生产集群的“底座”，涵盖 Linux 操作系统、服务器硬件以及 Kubernetes 事件系统。底座不稳，上层控制面（etcd、API Server、kubelet）和负载都会出现偶发、难以定位的故障。根据当前内容缺口分析，系统基础 亟需补齐 **节点时间同步**、**systemd 与 kubelet 服务管理**、**内核实时补丁**、**OS 镜像与节点加固基线** 以及 **NUMA / CPU 拓扑感知** 等生产就绪动作。本指南聚焦该域的生产就绪检查清单、关键风险缓解、日常运维操作与故障排查速查，帮助 SRE 在上线前与运行期建立可观测、可回滚、可审计的系统基线。

> 本指南与 [[系统基础/01-linux/09-linux-operations-basics.md|Linux 运维基础与应急响应]]、[[系统基础/硬件/16-kubernetes-hardware-troubleshooting.md|K8s 硬件故障排查]] 互补，避免重复罗列通用命令，重点补充生产就绪视角的落地动作。

---

## 1. 生产环境检查清单

在将节点或集群宣布为 production-ready 之前，必须逐项确认以下基线。建议将检查结果以 Infrastructure-as-Code（Ansible/CloudInit/镜像构建脚本）固化，并在每次变更后回归验证。

1. **OS 镜像与加固基线已固化**  
   生产环境不应依赖人工逐台配置节点。使用 CIS Benchmark、OpenSCAP 或 Lynis 生成合规报告，将加固动作写入镜像构建脚本或 Ansible Playbook；禁用 cups、bluetooth、firewalld 等非必要服务，SSH 仅启用密钥认证并关闭 Root 直接登录。参考 [[系统基础/01-linux/07-linux-security-hardening.md|Linux 安全加固]]。

2. **时间同步服务已配置且漂移可控**  
   etcd 对时间极其敏感，控制平面节点必须运行 chrony 或 systemd-timesyncd，并配置多个上游 NTP 源。生产要求时钟漂移 < 50 ms，建议 < 10 ms。任何时间跳变都应通过审计日志追踪。详见 节点时间同步指南（待补充）。

3. **内核版本与实时补丁策略已确定**  
   统一冻结生产内核小版本，建立 KernelCare、kpatch 或 livepatch 实时补丁流程；高危 CVE 必须在变更窗口内升级，并在 staging 节点验证 24 小时以上，确认对 CNI、存储驱动、GPU 驱动无回归后再全量滚动。

4. **systemd 服务依赖与自愈策略已配置**  
   kubelet 启动必须晚于容器运行时，单元文件应显式声明 `After=containerd.service`（或 `crio.service`）、`Wants=containerd.service`，并设置 `Restart=always`、`RestartSec=10`。任何对单元文件的修改都需 `systemctl daemon-reload` 并回归验证。参考 systemd 与 kubelet 服务管理（待补充）。

5. **内核参数基线已下发并持久化**  
   通过 `/etc/sysctl.d/` 或 CloudInit 固化必须参数，包括 `net.ipv4.ip_forward=1`、`net.bridge.bridge-nf-call-iptables=1`、`vm.swappiness=1`、`fs.inotify.max_user_watches=524288`、`net.netfilter.nf_conntrack_max` 等；绝对避免使用已废弃的 `tcp_tw_recycle`，并注意 conntrack 与 CNI 的交互。

6. **节点资源压力阈值已合理设置**  
   根据实际磁盘与内存规格配置 `evictionHard`、`evictionSoft`、`imageGCHighThresholdPercent`、`imageGCLowThresholdPercent`，并在低峰期人为触发验证。阈值设置过晚会导致服务降级，设置过早则造成资源浪费。参考 [[系统基础/K8s事件/06-node-lifecycle-condition-events.md|节点生命周期与状态事件]]。

7. **磁盘分区与 I/O 基线满足 etcd/容器运行时要求**  
   `/var/lib/etcd`、`/var/lib/containerd` 与 `/var/log/pods` 应独立分区或独立磁盘，避免根分区写满导致节点 NotReady。etcd fsync 延迟目标 < 10 ms，PLEG 敏感路径磁盘 `await` < 50 ms；建议定期使用 `fio` 或 `etcdctl check perf` 复测。

8. **硬件健康监控与告警已接入**  
   部署 Node Problem Detector（NPD）并配置 Prometheus 规则覆盖 SMART/NVMe 寿命、ECC 内存错误、CPU MCE、温度、IPMI SEL。对可纠正错误设置增长阈值告警，对不可纠正错误立即触发 P0 隔离。参考 [[系统基础/硬件/16-kubernetes-hardware-troubleshooting.md|K8s 硬件故障排查]]。

9. **日志与审计 retention 已规划**  
   `systemd-journald` 配置 `Storage=persistent` 与 `SystemMaxUse`（如 2G）；容器日志启用 `--log-max-size=100Mi --log-max-files=5`；logrotate 策略必须覆盖 `/var/log/pods`、EmptyDir 日志以及 Kubernetes 审计日志，防止磁盘满导致驱逐。

10. **事件监控与关键告警已启用**  
    对 `NodeNotReady`、`EvictionThresholdMet`、`MemoryPressure`、`DiskPressure`、`PIDPressure`、`FailedMount` 等事件配置 P1/P2 告警，并关联值班响应流程。事件是节点状态变化的第一手信号，必须纳入 On-Call 手册。

11. **SSH / sudo / break-glass 访问受控**  
    限制可登录用户与 sudo 命令集，记录所有审计日志，保留带外访问能力（BMC/IPMI/串口/云厂商 VNC）。当节点因网络或 kubelet 故障失联时，带外通道是最后的恢复手段。

---

## 2. 关键风险与缓解措施

以下五类风险在 System Foundation 域最为常见，且往往表现为上层 Kubernetes 症状，容易误导排查方向。每项风险都给出了可直接落地的配置或命令，SRE 应在上线前完成基线加固，并在值班手册中预留标准处置路径。

| 风险 | 生产影响 | 缓解措施与关键命令/配置 |
|------|----------|------------------------|
| **时钟漂移** | etcd 选主抖动、TLS 证书校验失败、lease 异常 | 1. 所有 control-plane 节点运行 chrony：`chronyc tracking` 检查偏移 < 50 ms<br>2. Prometheus 告警：`abs(node_timex_offset_seconds) > 0.05`<br>3. 节点时间不同步时先 `kubectl cordon <node>`，修复 NTP 后再 `kubectl uncordon <node>` |
| **内核/运行时版本漂移** | 节点行为不一致、CVE 暴露、升级回滚困难 | 1. 镜像构建脚本锁定 `kernel-$(uname -r)` 与 `containerd.io` 版本<br>2. 使用包管理器 hold：`apt-mark hold linux-image-generic containerd.io` / `yum versionlock add kernel containerd.io`<br>3. 灰度 1~2 节点验证 24h 后再全量滚动 |
| **硬件静默故障** | 随机 OOM、I/O 超时、PLEG unhealthy、Node NotReady | 1. NPD 永久条件检测 `MemoryHardwareError`、`CPUHardwareError`<br>2. Prometheus：`increase(node_edac_uncorrectable_errors_total[1h]) > 0` → critical<br>3. 发现 UCE/MCE 立即 `kubectl drain <node> --ignore-daemonsets --delete-emptydir-data` 并更换硬件 |
| **节点资源压力导致批量驱逐** | 服务中断、雪崩、恢复后再次触发 | 1. kubelet 配置示例：<br>`evictionHard: memory.available<"500Mi":, nodefs.available<"10%":, nodefs.inodesFree<"5%":`<br>2. 容器日志限制：`--log-max-size=100Mi --log-max-files=5`<br>3. 磁盘告警阈值应早于 kubelet 驱逐触发 |
| **systemd 服务启动顺序错误** | kubelet 先起、containerd 后起，导致 PLEG 异常、节点 NotReady | 1. kubelet 单元增加：<br>`After=containerd.service`<br>`Wants=containerd.service`<br>2. 变更后执行 `systemctl daemon-reload && systemctl restart kubelet` 并观察 `kubectl get nodes` |

> **处置优先级提示**：硬件不可纠正错误（UCE、MCE、NVMe critical warning）和 etcd fsync 延迟 > 100 ms 属于 P0 风险，必须立即隔离节点；时钟漂移和版本漂移属于 P1，应在 15 分钟内完成根因确认与修复。

---

## 3. 日常运维操作

日常运维的核心是“在故障发生前发现趋势”。System Foundation 域的巡检应覆盖 K8s 节点状态、系统日志、硬件健康、时间同步、容器运行时与 etcd 磁盘延迟六个维度。以下命令可直接用于值班脚本或自动化巡检平台。

### 3.1 节点健康巡检

建议每日自动执行一次，关键指标写入 Prometheus 或统一日志平台。对 NotReady、DiskPressure、MemoryPressure 等异常状态应即时告警。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 集群节点状态与资源概览
kubectl get nodes -o wide
kubectl top nodes
kubectl describe node <node> | grep -A 8 "Conditions"

# 2. 节点系统层健康（登录节点或通过 kubectl debug node）
journalctl -u kubelet --since "1 hour ago" | tail -50
journalctl -u containerd --since "1 hour ago" | tail -30
dmesg | grep -iE "oom|error|mce|edac|i/o error" | tail -20

# 3. 磁盘与 inode 使用
df -h
df -i
du -sh /var/lib/containerd /var/lib/kubelet /var/log/pods
```
### 3.2 计划内节点维护

所有涉及系统内核、容器运行时、硬件固件或节点重启的变更，都必须先 cordon 再 drain，确保业务 Pod 有充足时间优雅终止。变更完成后需观察节点 Conditions 稳定至少 5 分钟再 uncordon。

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
# 1. 隔离并排空
kubectl cordon <node>
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --grace-period=120

# 2. 执行内核/安全补丁/硬件维护后 reboot

# 3. 恢复调度
kubectl uncordon <node>
kubectl get nodes -w
kubectl describe node <node> | grep -A 5 "Conditions"
```
### 3.3 时间同步健康检查

时间漂移通常在分钟级才会触发明显故障，但到此时 etcd 已经可能产生选主异常。建议每日检查一次 offset，并在控制平面节点设置秒级 Prometheus 告警。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# chrony
timedatectl status
chronyc tracking | grep -E "Reference ID|Stratum|Last offset|RMS offset"
chronyc sources -v

# 若偏移过大，先 cordon 节点，再重启 chronyd
sudo systemctl restart chronyd
chronyc makestep  # 强制同步（部分环境需谨慎）
```
### 3.4 容器运行时与镜像清理

镜像和停止容器是节点磁盘压力的主要来源之一。清理操作必须限定在“已停止容器”和“未使用镜像”，严禁使用 `docker system prune -a` 或 `crictl rmi -a` 等会删除运行中依赖的强制命令。

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看容器运行时状态
systemctl status containerd
crictl info | jq '.status'

# 安全清理（仅删除已停止容器与未使用镜像，不影响运行中 Pod）
crictl rm $(crictl ps -a -q --state Exited)
crictl rmi --prune

# 查看镜像与容器占用
crictl images -v | awk '{sum+=$5} END {print sum/1024/1024/1024 " GB"}'
```
### 3.5 etcd 磁盘延迟快速验证

etcd 对 fsync 延迟极度敏感，磁盘老化、RAID 卡电池失效、NVMe 磨损都会导致集群级抖动。建议每周在低峰期执行一次快速验证，控制平面节点优先。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 简单 fsync 测试（在 etcd 数据目录执行）
for i in {1..10}; do
  start=$(date +%s%N)
  dd if=/dev/zero of=/var/lib/etcd/latency-test bs=4k count=1 conv=fsync 2>/dev/null
  end=$(date +%s%N)
  echo "$(( (end-start)/1000000 )) ms"
done
rm -f /var/lib/etcd/latency-test

# 使用 etcd 自带工具
ETCDCTL_API=3 etcdctl check perf --load="s"
ETCDCTL_API=3 etcdctl endpoint status --cluster -w table
```
---

## 4. 故障排查速查

当 Kubernetes 出现节点级异常时，优先区分“系统层问题”与“Kubernetes 层问题”。下表汇总了 System Foundation 域最常见的症状、根因、确认命令与标准处置，便于 On-Call 工程师按图索骥。

| 现象 | 可能根因 | 确认命令 | 处置 |
|------|----------|----------|------|
| Node `NotReady`，kubelet 报 `PLEG is not healthy` | 磁盘 I/O 延迟高 / 容器运行时无响应 | `iostat -x 1 5`、`crictl info`、`journalctl -u containerd` | 重启 containerd；若硬件延迟高，cordon 后 drain 并更换磁盘 |
| etcd 选主频繁切换 / API Server 响应慢 | etcd fsync 延迟高 / 时钟漂移 | `fio fsync` 测试、`chronyc tracking`、`etcdctl endpoint status` | 隔离慢节点，修复磁盘或 NTP；必要时重建 etcd 成员 |
| Pod 随机 `OOMKilled`，但 `kubectl top pod` 内存未满 | ECC 内存错误 / NUMA 不均衡 | `dmesg \| grep -iE "edac\|mce"`、`numactl --hardware`、`numastat -c` | 立即 cordon 节点，迁移 Pod，更换 DIMM |
| 节点 `DiskPressure` / 镜像拉取失败 | imagefs 满或 inode 耗尽 | `df -h`、`df -i`、`crictl images` | 清理镜像/停止容器；扩容磁盘；调整 GC 阈值 |
| 节点重启后 `NetworkUnavailable` | CNI 未就绪 / 内核模块缺失 | `systemctl status <cni-daemon>`、`modprobe overlay br_netfilter`、`ip link` | 重新加载模块，重启 CNI Pod；检查 Cloud Controller Manager 路由 |
| 节点 `PIDPressure`，新容器无法创建 | 单 Pod 创建大量进程 / pid_max 过低 | `ps -eLf \| wc -l`、`cat /sys/fs/cgroup/pids/kubepods/pids.current` | 限制 Pod `shareProcessNamespace` 与进程数；必要时提升 `kernel.pid_max` |

---

## 5. 与其他域的协作边界

System Foundation 的问题往往会“向上透传”为集群、网络、存储或安全域的症状。明确边界可避免不同值班团队反复排查同一节点。以下是本域与相关域的职责划分与交接原则。

- **集群基础**：控制平面组件（API Server、etcd、kubelet）的安装与升级依赖本域的 OS/内核/时间同步基线；当问题上升到组件级崩溃或证书异常时，移交给集群基础域。
- **容器运行时**：containerd/CRI-O 的安装、镜像仓库配置、运行时版本升级由运行时域主导；本域负责 systemd 服务依赖、内核参数、节点资源清理等“承载层”配合。
- **网络**：CNI 插件依赖 `br_netfilter`、`vxlan` 等内核模块以及网络 sysctl；网络策略、Service 拓扑、Ingress 问题在网络域闭环，本域提供节点网络环境排查。
- **安全**：节点加固、SSH/sudo 审计、Pod Security Standards / OPA Gatekeeper 由安全域定义策略；本域负责落地到 OS 镜像与 kubelet 配置。
- **可观测性**：Node Exporter、NPD、journald、事件监控由可观测域统一采集；本域输出需要被监控的节点级关键指标与事件。
- **故障诊断**：当出现复杂 Node NotReady、硬件故障树或跨域根因时，使用 [[故障诊断/README.md|故障排查诊断]] 域的 FTA 与决策树。
- **生产运维**：值班响应、变更管理、SLO 定义由生产运维域统一调度；本域提供变更窗口内的节点维护 SOP。

---

## 6. 推荐阅读

### 同域参考

- [[系统基础/01-linux/09-linux-operations-basics.md|Linux 运维基础与应急响应]]
- [[系统基础/01-linux/06-linux-performance-tuning.md|Linux 性能调优]]
- [[系统基础/01-linux/07-linux-security-hardening.md|Linux 安全加固]]
- [[系统基础/硬件/16-kubernetes-hardware-troubleshooting.md|K8s 硬件故障排查]]
- [[系统基础/K8s事件/06-node-lifecycle-condition-events.md|节点生命周期与状态事件]]
- [[系统基础/速查卡/kubectl-scene-cheatsheet.md|kubectl 场景速查卡]]
- 节点时间同步指南（待补充）（待补充）
- systemd 与 kubelet 服务管理（待补充）（待补充）

### 跨域参考

- [[集群基础/README.md|集群基础组件]]
- [[容器运行时/README.md|容器运行时]]
- [[安全/README.md|安全合规]]
- [[故障诊断/README.md|故障排查诊断]]
- [[生产运维/README.md|生产运维]]

---

*本文件基于 系统基础 当前内容缺口分析编写，重点补齐时间同步、systemd 服务管理、内核实时补丁、OS 加固基线与硬件 NUMA/CPU 拓扑感知等生产就绪动作。*


<!-- risk-assessed -->
