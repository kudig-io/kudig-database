---
title: 初始化配置检查项
description: 大规模 Kubernetes 集群交付前的初始化配置检查清单，覆盖基础设施、OS 内核、容器运行时、集群组件、安全基线五大类逐项核验
summary: 集群初始化交付 checklist：基础设施规划、OS 与内核参数、运行时、控制面与节点组件、安全基线逐项检查表
category: references
tags:
- k8s
- checklist
- initialization
- production
- best-practices
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: intermediate
audience:
- SRE
- 平台工程师
estimated_read_time: 20min
---

# 初始化配置检查项

> **使用场景**：新集群交付前逐项核验。每条标注：✅ 必须 / 🔶 建议。未通过的 ✅ 项阻断交付。
>
> 检查方式：逐条执行验证命令或核对配置，结果记录到交付文档。

## 1. 基础设施规划

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 1.1 | ✅ | Pod CIDR、Service CIDR、节点网段、VPC/IDC 网段两两不冲突，且预留 2 倍增长空间 | 网段规划表评审 |
| 1.2 | ✅ | 单节点 IP 供给能力 ≥ `maxPods + buffer`（VPC CNI 场景核验 ENI 数 × 每 ENI IP 数） | 云控制台/CNI 配置核对 |
| 1.3 | ✅ | Master 节点 3/5 台，跨 3 个 AZ | 拓扑图核验 |
| 1.4 | ✅ | etcd 使用 SSD 级存储，fsync P99 延迟 < 10ms | `fio --fsync=1` 压测 |
| 1.5 | ✅ | APIServer 前置 LB 高可用（双机/云 LB），健康检查配置正确 | 断一台 LB 节点验证 |
| 1.6 | 🔶 | 系统组件独立节点池规划完成（taint + 标签） | 节点池清单 |
| 1.7 | ✅ | 时钟同步：所有节点 NTP/chrony 配置，偏差 < 100ms | `chronyc tracking` |
| 1.8 | ✅ | 节点主机名唯一且符合 RFC 1123，云主机名不冲突 | 逐节点核对 |

## 2. OS 与内核

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 2.1 | ✅ | 内核版本 ≥ 4.19（eBPF CNI 需 ≥ 5.4） | `uname -r` |
| 2.2 | ✅ | swap 关闭（或 1.28+ 显式配置 swap 策略） | `swapon --show` 为空 |
| 2.3 | ✅ | 内核模块加载：`overlay`、`br_netfilter`、`ip_vs`（IPVS 模式时） | `lsmod` |
| 2.4 | ✅ | sysctl：`net.bridge.bridge-nf-call-iptables=1`、`net.ipv4.ip_forward=1` | `sysctl -a \| grep` |
| 2.5 | ✅ | conntrack 表调大：`net.netfilter.nf_conntrack_max` ≥ 1048576（大节点 2M+） | `sysctl` 核对 |
| 2.6 | ✅ | 文件句柄：`fs.file-max` 与 `nofile`（systemd unit 级 LimitNOFILE ≥ 1048576） | `cat /proc/sys/fs/file-max` |
| 2.7 | ✅ | inotify：`fs.inotify.max_user_instances` ≥ 1024（容器多日志采集需更多） | `sysctl` 核对 |
| 2.8 | ✅ | 关闭透明大页或配置为 `madvise`（数据库型节点必关） | `cat /sys/kernel/mm/transparent_hugepage/enabled` |
| 2.9 | 🔶 | irqbalance / 网卡多队列 IRQ 亲和 | 网卡中断分布检查 |
| 2.10 | ✅ | 安全加固基线：禁用密码登录 SSH、最小化安装、安全补丁最新 | 基线扫描 |
| 2.11 | ✅ | 节点镜像包含故障诊断工具：tcpdump、strace、perf、conntrack | 镜像清单 |

## 3. 容器运行时

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 3.1 | ✅ | containerd 版本与 K8s 兼容矩阵匹配 | `crictl version` |
| 3.2 | ✅ | cgroup driver 与 kubelet 一致（均为 systemd） | `/etc/containerd/config.toml` + kubelet config |
| 3.3 | ✅ | 镜像仓库配置：受信仓库、registry mirror / 代理配置 | `crictl pull` 实测 |
| 3.4 | ✅ | `max_concurrent_downloads` 按磁盘能力调整（SSD ≥ 8） | config.toml 核对 |
| 3.5 | ✅ | 容器日志轮转：kubelet `containerLogMaxSize`（≤ 100Mi）、`containerLogMaxFiles`（≤ 5） | kubelet config |
| 3.6 | 🔶 | 镜像垃圾回收阈值：imageGCHighThreshold 85 / Low 70 | kubelet config |
| 3.7 | 🔶 | 大集群配置 P2P 镜像分发（Dragonfly/Nydus）或镜像预热方案 | 方案文档 |

## 4. 集群组件配置

### 4.1 控制面

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 4.1.1 | ✅ | APIServer：启用 APF（1.29+ 默认），`max-requests-inflight` 按规模调整 | 启动参数核对 |
| 4.1.2 | ✅ | APIServer：`--encryption-provider-config` 配置，Secret 静态加密生效 | 创建 Secret 后查 etcd 中为密文 |
| 4.1.3 | ✅ | APIServer：审计策略配置并外送（webhook backend 或日志采集） | 审计日志验证 |
| 4.1.4 | ✅ | APIServer：禁用匿名访问（`--anonymous-auth=false`）、不安全端口关闭 | 启动参数核对 |
| 4.1.5 | ✅ | etcd：`--quota-backend-bytes=8GiB`、快照备份任务配置 | 参数 + crontab/备份平台核对 |
| 4.1.6 | ✅ | scheduler/controller-manager：QPS/Burst 按规模调大（≥ 100/200） | 启动参数核对 |
| 4.1.7 | ✅ | 控制面组件静态 Pod 资源监控接入（apiserver/etcd/scheduler 指标） | Prometheus targets 核验 |

### 4.2 节点组件

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 4.2.1 | ✅ | kubelet：`systemReserved`/`kubeReserved`/`evictionHard` 按节点规格配置 | kubelet config |
| 4.2.2 | ✅ | kubelet：`maxPods` 与 CNI IP 供给匹配 | 核对 |
| 4.2.3 | ✅ | kubelet：`eventRecordQPS ≤ 5` | kubelet config |
| 4.2.4 | ✅ | kube-proxy：IPVS 或 eBPF 模式（大规模禁止 iptables 默认配置） | `kubectl get cm kube-proxy -n kube-system -o yaml` |
| 4.2.5 | ✅ | CoreDNS：副本 ≥ 2 跨节点 + PDB；大规模启用 autoscaler | 部署核验 |
| 4.2.6 | ✅ | NodeLocal DNSCache 部署并验证生效 | Pod 内 dig 走 169.254.20.10 |
| 4.2.7 | ✅ | metrics-server 部署，HPA 可用 | `kubectl top node` |

### 4.3 必要系统服务

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 4.3.1 | ✅ | Ingress Controller：HA 部署 + 独立节点池 + PDB | 部署核验 |
| 4.3.2 | ✅ | cert-manager 或证书管理方案就位 | CRD 核验 |
| 4.3.3 | ✅ | 日志采集（fluent-bit/vector DaemonSet）+ 资源限额 | 部署核验 |
| 4.3.4 | ✅ | 监控 Agent（node-exporter 等）部署 | 部署核验 |
| 4.3.5 | 🔶 | Cluster Autoscaler / Karpenter 配置并与云权限打通 | 模拟扩容验证 |
| 4.3.6 | 🔶 | 集群组件全量纳入 GitOps（Git 仓库为唯一变更入口） | 仓库核验 |

## 5. 安全基线

| # | 级别 | 检查项 | 验证方法 |
|---|---|---|---|
| 5.1 | ✅ | RBAC：无普通用户 cluster-admin；运维权限按域收敛 | `kubectl get clusterrolebindings` 审计 |
| 5.2 | ✅ | 所有 Namespace 配置 Pod Security Standards 标签（baseline/restricted） | `kubectl get ns --show-labels` |
| 5.3 | ✅ | ServiceAccount token 自动挂载默认关闭 | 抽查 Pod spec |
| 5.4 | ✅ | 默认 NetworkPolicy：至少 kube-system 外的管理面策略就位 | 策略清单 |
| 5.5 | ✅ | 证书有效期检查：CA 10 年、其他组件证书到期时间登记 + 告警 | `kubeadm certs check-expiration` |
| 5.6 | ✅ | etcd 通信 TLS 启用且证书独立 | 参数核对 |
| 5.7 | ✅ | 节点安全组/防火墙：仅开放必要端口（6443 受控来源、10250 仅控制面、NodePort 范围受控） | 安全组规则审计 |
| 5.8 | 🔶 | 镜像扫描流水线接入（Trivy/云扫描），阻断高危镜像 | CI 核验 |
| 5.9 | 🔶 | 准入控制（OPA Gatekeeper/Kyverno）部署，强制：禁 latest、必须有 requests、禁 privileged | 策略核验 |

## 6. 交付验收

| # | 级别 | 检查项 |
|---|---|---|
| 6.1 | ✅ | `kubectl get nodes` 全部 Ready，`get pods -A` 无异常 |
| 6.2 | ✅ | etcd 集群健康：`endpoint status` 无 alarm，leader 稳定 |
| 6.3 | ✅ | 创建测试 Deployment + Service + Ingress 全链路验证（调度、网络、DNS、存储） |
| 6.4 | ✅ | 故障演练：杀 1 台 Master 服务不中断；杀 1 个 etcd 节点集群正常 |
| 6.5 | ✅ | 备份任务执行一次并验证快照可恢复（小规模演练） |
| 6.6 | ✅ | 交付文档：拓扑图、网段表、账号权限清单、证书到期表、运维 SOP 移交 |

## Related

- [[01-overview|大规模集群总览与规模基线]]
- [[02-cluster-configuration|集群配置最佳实践]]
- [[07-pre-production-checklist|生产上线前检查项]]
- [[20-最佳实践/07-scenarios/cluster-deployment|集群部署场景]]
