---
title: K8s Node NotReady 深度解析
description: 对 Node NotReady 各根因的「为什么」进行 prose 拆解，覆盖阿里云/专有云场景、边界条件、版本差异与禁忌操作
summary: 对 Node NotReady 各根因的「为什么」进行 prose 拆解，覆盖阿里云/专有云场景、边界条件、版本差异与禁忌操作
category: Kubernetes-Incident-Response
tags:
- k8s
- node
- notready
- aliyun
- ack
- deep-dive
- skills
tier: supporting
created: '2026-06-26'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 15min
intent_queries:
- 为什么 Node 会变成 NotReady
- 阿里云 ACK 节点 NotReady 怎么排查
- Node NotReady 的边界条件和版本差异
trigger_keywords:
- NotReady
- 节点异常
- kubelet
- PLEG
- 阿里云
- 专有云
prerequisites:
- k8s-node-notready-skill
k8s_versions:
- 1.28.x
- 1.30.x
- 1.32.x
skill_id: SKILL-SKILL-001-DEEP
skill_name: K8s Node NotReady 深度解析
version: 1.0.0
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s Node NotReady 深度解析

> 本文是 [[domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-node-notready/SKILL.md|K8s Node NotReady 诊断与修复]] 的深度补充，不替代原 Skill，而是解释「为什么」以及「在阿里云/专有云里会发生什么」。

## 1. 每种根因的「为什么」

### 1.1 kubelet 进程崩溃或未运行

kubelet 是节点上唯一负责向 API Server 上报心跳的组件。它通过 NodeLease（`kube-node-lease` 命名空间下的 Lease 对象）和控制平面保持「我还活着」的信号。一旦 kubelet 停止，控制平面在 `pod-eviction-timeout`（默认 5 分钟）后就会认为该节点已失联，并将其标记为 NotReady，随后驱逐非 DaemonSet Pod。

kubelet 崩溃的常见内因包括：配置解析失败（如 `/var/lib/kubelet/config.yaml` 格式错误）、被 OOM Killer 杀掉、或者 systemd 依赖链断裂。外因则多为磁盘压力导致 kubelet 无法写入状态文件，或内存压力触发系统级 OOM。排查时应先确认 systemd 状态，再看 journal 中的 fatal/panic，而不是一上来就重启——重启可能掩盖崩溃根因。

### 1.2 容器运行时（containerd）异常

kubelet 本身不直接操作容器，它通过 CRI（Container Runtime Interface）调用 containerd。containerd 负责镜像拉取、容器创建、shim 管理、cgroups 挂载等底层工作。当 containerd 无响应时，kubelet 的 PLEG（Pod Lifecycle Event Generator）就无法在 3 分钟内完成 relist，节点随即 NotReady。

containerd 异常通常表现为：进程存在但 `crictl ps` 超时（运行时 hang）、CRI socket 文件丢失、或者日志中出现 `failed to create shim`。这里需要区分「containerd 真正挂了」和「containerd 被下游资源卡住」——后者常见于磁盘 IO Hang 或 PID 耗尽，单纯重启 containerd 往往只能暂时缓解。

### 1.3 磁盘压力（DiskPressure）

kubelet 默认在根分区或 imagefs 使用率达到 85% 时设置 DiskPressure=True，并在 90% 左右触发驱逐。磁盘耗尽的直接后果是：kubelet 无法写入 Pod 状态、containerd 无法创建新的 overlay 层、镜像 GC 失败，进而级联导致 kubelet 或 containerd 异常。

排查磁盘压力不能只看 `df -h`。inode 耗尽是更隐蔽的杀手——当 `/var/lib/containerd` 或 `/var/log` 下存在海量小文件时，磁盘空间可能还剩 30%，但 inode 已用光，任何新文件创建都会失败。此外，日志轮转配置错误（如容器标准输出日志未切割）会导致 `/var/log/pods` 和 `/var/log/containers` 迅速膨胀。

### 1.4 内存压力（MemoryPressure）

kubelet 默认在节点可用内存低于 100Mi 时设置 MemoryPressure=True。与 OOMKilled 不同，MemoryPressure 是节点级别的保护机制：kubelet 会按 QoS 等级驱逐 Pod，优先驱逐 BestEffort，然后是 Burstable，最后是 Guaranteed（仅当必须时）。

内存压力的根因往往不是没有 limits，而是 limits 设置不合理。大量 Pod 的 limits 远高于 requests，导致调度器认为节点还有余量，但实际运行时同时触顶，引发节点级 OOM。Linux 内核会首先选择 oom_score_adj 最高的进程杀死，kubelet、containerd 乃至系统进程都可能被波及。

### 1.5 PID 耗尽（PIDPressure）

Linux 内核的 `pid_max` 默认通常为 32768（老内核）或 4194304（新内核）。当节点上进程/线程总数接近上限时，kubelet 会报告 PIDPressure。容器化环境特别容易触发 PID 耗尽，因为一个 Java 应用可能创建数百线程，一个 shell 脚本 fork 炸弹会在几秒内占满 pid 空间。

PID 耗尽的危害在于它同时阻塞 kubelet 和 containerd：kubelet 无法 exec 探针，containerd 无法创建 shim，节点迅速 NotReady。排查时要看 `/proc/sys/kernel/pid_max` 和当前进程数，并用 `ps -eLf | awk '{print $1}' | sort | uniq -c | sort -nr | head` 定位线程大户。

### 1.6 节点与 API Server 网络不通

即使 kubelet 和 containerd 都正常，只要节点无法到达 API Server，心跳就会中断。典型的网络层根因包括：安全组规则变更、路由表丢失、VPC 网络抖动、物理交换机故障、或者 `nf_conntrack` 表满导致连接被丢弃。

诊断网络不通时，应先区分「TCP 层不通」和「TLS 层失败」。`nc -zv <apiserver-ip> 6443` 只能证明 TCP 可达；如果 TCP 可达但 HTTPS 健康检查失败，更可能是证书或时间同步问题，而非网络问题。

### 1.7 kubelet 客户端证书过期

kubelet 使用客户端证书与 API Server 建立 mTLS。证书过期后，TLS 握手失败，kubelet 无法上报心跳。虽然 Kubernetes 1.19+ 默认启用 `RotateKubeletClientCertificate`，但如果节点长时间离线、证书轮转被自定义 CA 策略阻断、或者 kubeadm 集群的证书管理脚本异常，仍会出现过期场景。

证书问题的排查要点是：先检查 `/var/lib/kubelet/pki/kubelet-client-current.pem` 的 `notAfter`，再看 journal 中是否有 `x509: certificate has expired`。注意，时间不同步（NTP 偏差）会让「证书明明没过期」也表现为 TLS 失败。

### 1.8 PLEG 不健康

PLEG 负责周期性地向 containerd 索要所有 Pod/容器列表，并生成生命周期事件。默认 relist 超时时间为 3 分钟。如果 containerd 响应慢（如 D 状态进程阻塞、大量容器、运行时 hang），PLEG 就会超时，节点进入 NotReady。

PLEG 不健康几乎总是一个「下游症状」而不是独立根因。直接重启 kubelet 只能重置 PLEG 计时器；若根因是 containerd 卡住或 IO Hang，节点很快会再次 NotReady。因此看到 `PLEG is not healthy` 时，必须同步检查 containerd 状态、节点容器数量、以及是否有 D 状态进程。

### 1.9 CNI 插件异常

CNI 负责 Pod 网络的创建和销毁。如果 CNI 配置文件缺失、CNI 二进制损坏、或者 CNI DaemonSet Pod（如 Calico、Terway、Flannel）异常，kubelet 会设置 `NetworkUnavailable=True`。虽然 NetworkUnavailable 不等同于 NotReady，但某些发行版或配置下，网络异常会进一步阻塞 kubelet 的正常心跳上报。

在阿里云 ACK 中，CNI 异常往往与 Terway 组件相关（见下文）。排查时应检查 `/etc/cni/net.d/` 是否存在有效配置，以及节点上 CNI Pod 的运行状态。

### 1.10 手动 cordon / drain

`kubectl cordon` 会将节点标记为 `SchedulingDisabled`，并在节点上添加 `node.kubernetes.io/unschedulable:NoSchedule` taint。这个操作本身不会导致 NotReady，但初学者容易把 `Ready,SchedulingDisabled` 误判为节点异常。真正的 NotReady 是 Ready condition 的 status=False 或 Unknown。

## 2. 阿里云 / 专有云特有场景

### 2.1 ECS 系统事件

阿里云的 ECS 会主动触发系统事件（System Event），例如「因底层硬件维护计划重启」「因实例停用而停止」。这些事件会在 ACK 控制台和 OpenAPI 的 `DescribeInstanceHistoryEvents` 中体现。如果节点在事件期间未提前 drain，业务 Pod 会被强制中断，节点状态可能变为 NotReady。

排查时应在 ACK 控制台 **节点 > 节点事件** 或云监控 **事件中心** 中查看是否有系统事件。对于计划内事件，推荐配置 **节点维护策略** 或 **节点自动排水（node-problem-detector + draino）**。

### 2.2 云盘 IO Hang

ACK 节点普遍使用 ESSD 云盘作为系统盘。云盘出现 IO Hang 时，所有依赖磁盘写入的进程（containerd、kubelet、systemd-journald）都会进入 D 状态，节点表现为：SSH 卡顿、`kubectl exec` 无响应、`crictl ps` 超时。由于 D 状态进程无法被 kill，即使重启 kubelet 也往往无效，必须重启或更换 ECS。

云盘 IO Hang 的间接迹象包括：`dmesg` 中出现 `task xxx blocked for more than 120 seconds`、`EXT4-fs error`、`I/O error`；ACK 节点诊断报告中的 **磁盘健康检查** 异常。

### 2.3 Terway ENI IP 耗尽

在 ACK Terway 网络模式下，每个 Pod 独占一个 ENI 辅助 IP。当交换机可用 IP 耗尽时，新 Pod 无法分配 IP，而节点上的 Terway Pod 也可能因持续报错进入异常状态。虽然 Terway IP 耗尽主要表现为 Pod 无法启动或网络不通，但在极端情况下，Terway DaemonSet Pod 自身异常会导致 kubelet 报告网络异常，进而影响节点 Ready 状态。

排查时应查看交换机网段剩余 IP、Terway Pod 日志、以及 `kubectl get node <node> -o json | jq '.status.conditions[] | select(.type=="NetworkUnavailable")'`。

### 2.4 安全组变更

ACK 节点安全组如果被误修改（例如删除了 6443、10250、8472/4789 等端口），节点与 API Server、节点间 Pod 网络都会中断。安全组问题容易被误判为路由或 CNI 问题，因为其症状是「部分端口通、部分不通」。

排查时应核对安全组规则是否允许：
- 控制平面到节点 10250（kubelet）、10256（kube-proxy）
- 节点间 8472（VXLAN）或 4789（Geneve），取决于 CNI
- 节点到 API Server 6443

### 2.5 专有云底座异常

在专有云（Apsara Stack）环境中，节点状态还依赖于底座组件，如飞天分布式文件系统、盘古、洛神网络等。如果底座出现网络分区或存储抖动，即使 Kubernetes 组件本身正常，节点也可能因无法完成底层 IO 而 NotReady。

专有云排障通常需要平台侧介入。现场工程师应第一时间收集：底座告警、节点 BMC/带外日志、以及专有云运维平台的节点健康报告。

## 3. 边界条件

### 3.1 多个节点同时 NotReady

当 NotReady 节点数 > 50% 或所有控制平面节点都 NotReady 时，问题几乎不可能出在单个节点上，必须立即升级：优先检查 API Server、etcd、控制平面网络、以及云平台级别的网络/存储事件。

此时 `kubectl get nodes` 可能本身超时。如果 kubectl 超时，说明客户端到 API Server 的网络或 API Server 自身有问题，而非工作节点问题。

### 3.2 控制平面节点 NotReady

控制平面节点 NotReady 的威胁远大于工作节点：如果所有控制平面节点都不可用，集群将完全失去调度、扩缩容、配置变更能力。处理控制平面节点问题时，应优先保证 etcd 数据一致性，避免在多个控制平面节点上同时执行可能破坏 quorum 的操作。

### 3.3 kubectl 本身超时

如果 `kubectl get nodes`  hang 住或返回超时，不要急于判断为节点问题。应使用 `--request-timeout` 参数测试 API Server 响应，例如 `kubectl get nodes --request-timeout=5s`。若 API Server 无响应，优先排查控制平面、客户端网络、以及 kubectl 版本兼容性。

## 4. 版本差异

### 4.1 NodeLease

NodeLease 在 K8s 1.14 引入，1.17 成为默认机制。在 1.28/1.30/1.32 中，NodeLease 的行为基本一致：kubelet 默认每 10 秒续租一次，控制平面根据 Lease 的 renewTime 判断节点健康。不同版本的主要差异在于Lease 失败后的降级策略和控制器对 heartbeat 的容忍度，生产环境中应关注 `kubelet --node-status-update-frequency` 与控制器 `--node-monitor-grace-period` 的匹配。

### 4.2 Eviction（驱逐）

K8s 1.28 起，kubelet 的驱逐行为对内存压力的判断更加严格；1.30+ 若启用 Node Swap Support，MemoryPressure 的计算会纳入 swap 使用量。1.32 进一步优化了驱逐日志，便于区分「硬驱逐（eviction-hard）」和「软驱逐（eviction-soft）」触发的原因。

### 4.3 GracefulNodeShutdown

GracefulNodeShutdown 在 1.21 引入，1.24+ 默认启用。它允许节点在关机前按优先级优雅终止 Pod。在 1.28/1.30/1.32 中，该功能增加了对 critical Pod（如 kube-system 中标记 `priorityClassName: system-node-critical` 的 Pod）的保护。如果节点因 ECS 系统事件被强制关机而 GracefulNodeShutdown 未生效，可能是因为 shutdown 信号未被正确传递，或配置中的 `shutdownGracePeriod` 过短。

## 5. 常见错误与禁忌操作

### 5.1 常见误诊

- 把 `Ready,SchedulingDisabled` 当成 NotReady：前者是运维操作的结果，后者才是故障。
- 看到 PLEG 不健康只重启 kubelet：不排查 containerd 和 IO，问题会反复。
- 把 TLS 证书失败当成网络不通：TCP 通但 HTTPS 失败时，应先查证书和时间。
- 忽略 inode 耗尽：磁盘空间充足不代表可以创建新文件。

### 5.2 禁忌操作

- **不要在未 drain 的情况下直接重启控制平面节点**：可能破坏 etcd quorum。
- **不要批量重启所有节点的 kubelet**：如果根因是 API Server 或网络问题，批量重启只会加剧抖动。
- **不要直接删除 `/var/lib/kubelet`**：这会丢失 Pod 状态、卷挂载信息，可能导致有状态服务数据不一致。
- **不要随意修改 `--eviction-hard` 阈值以「消除」压力条件**：这只是隐藏告警，不会解决资源不足。

### 5.3 推荐的诊断顺序

1. 先确认影响范围（单节点 / 多节点 / 控制平面）。
2. 再确认是 Ready=False 还是 Ready=Unknown 还是 SchedulingDisabled。
3. 查看 Lease 是否更新，区分 kubelet 侧问题和控制平面侧问题。
4. SSH 到节点后，按 kubelet → containerd → 资源 → 网络 → 证书 → 内核/硬件的顺序排查。
5. 在阿里云/专有云环境中，同步查看云平台事件和底座告警。

## 6. 关键诊断命令示例（含「为什么」）

以下命令用于在远程顾问或半自动化场景下快速收集关键证据。每个命令前说明其目的，避免变成无意义的命令堆砌。

**查看节点 Ready 状态和消息**。这条命令告诉我们节点是 NotReady 还是 Unknown，以及 kubelet 给出的原因摘要：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get node <node> -o json | jq '.status.conditions[] | select(.type=="Ready") | {status, reason, message}'
```
**检查 NodeLease 是否过期**。Lease 的 renewTime 超过 40 秒未更新，说明 kubelet 已经停止向 API Server 上报心跳，问题多半在节点侧或网络侧：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get lease -n kube-node-lease <node> -o json | jq '{renewTime: .spec.renewTime, holderIdentity: .spec.holderIdentity}'
```
**检查 kubelet 和 containerd 状态**。如果 SSH 可达，这是确认节点上核心组件是否存活的最直接方式：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
ssh <node> 'systemctl is-active kubelet containerd && journalctl -u kubelet --since "10 min ago" | grep -E "PLEG|eviction|x509|fatal" | tail -10'
```
**检查证书有效期**。TLS 证书问题最容易被误判为网络问题，先排除证书可以少走很多弯路：
```bash
ssh <node> 'openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates'
```

## 7. 相关链接

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/node-fta.md|Node 异常 FTA 树]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-node-notready/SKILL.md|K8s Node NotReady 诊断与修复 Skill]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/skill-set/k8s-node-notready/reference/root-cause-catalog.md|根因分类目录]]
- [[domain-19-landscape-references/topic-index/node-index.md|Node 知识图谱索引]]
- Terway 网络专题

## Related

- [[deep-dive|#deep-dive Hub]] — tag hub


<!-- risk-assessed -->
