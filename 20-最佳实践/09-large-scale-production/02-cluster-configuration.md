---
title: 集群配置最佳实践
description: 大规模 Kubernetes 集群控制面高可用、etcd 优化、APIServer 调优、kubelet 配置、CoreDNS、版本升级与备份恢复的生产级最佳实践
summary: 覆盖控制面 HA、etcd、APIServer APF 限流、kubelet 资源预留、DNS 高可用、证书与升级备份全链路配置实践
category: references
tags:
- k8s
- best-practices
- control-plane
- etcd
- apiserver
- kubelet
- production
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: advanced
audience:
- SRE
- 平台工程师
estimated_read_time: 25min
---

# 集群配置最佳实践

> 大规模集群的稳定性 70% 取决于控制面配置。本文覆盖：控制面 HA、etcd、APIServer、调度器与控制器、kubelet、CoreDNS、事件治理、证书、升级与备份。

## 1. 控制面高可用

### 1.1 拓扑要求

- Master 节点数：**3 或 5**（奇数），跨 3 个可用区部署
- Master 节点打上 `node-role.kubernetes.io/control-plane:NoSchedule` taint，禁止业务 Pod 调度
- APIServer 前置高可用 LB（如 haproxy + keepalived / 云 LB），LB 本身也要做 HA
- LB 健康检查使用 `/healthz` 或 `/readyz`，避免把流量打到未就绪实例

### 1.2 etcd 集群

| 项目 | 最佳实践 |
|---|---|
| 部署方式 | 堆叠式（与 Master 同机）适合中小集群；**大规模推荐外置 etcd 集群** |
| 节点数 | 3（容忍 1 故障）或 5（容忍 2 故障），不要更多——写性能随节点数下降 |
| 磁盘 | 本地 NVMe SSD 或高性能云盘（ESSD PL1+/io2），`fsync` 延迟 P99 < 10ms |
| 数据目录 | 独立磁盘/分区，不与 OS 混用 |
| 配额 | `--quota-backend-bytes=8589934592`（8 GiB，默认 2 GiB 对大集群太小） |
| 碎片整理 | 定期 `etcdctl defrag`（建议每周低峰期），配合 compaction |
| 备份 | 定时快照（每 1–6 小时）+ 异地保留，定期演练恢复 |
| 加密 | 启用 APIServer 层 `EncryptionConfiguration` 对 Secret 静态加密（KMS 或 aescbc） |

```bash
# etcd 健康检查 🟢
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --cluster -w table

# etcd 快照备份 🟡
etcdctl snapshot save /backup/etcd-$(date +%Y%m%d-%H%M).db
```

### 1.3 APIServer 调优

**大规模必调项：**

| 参数 | 建议 | 说明 |
|---|---|---|
| `--max-requests-inflight` | 400→800（大集群） | 非变更类读请求并发上限 |
| `--max-mutating-requests-inflight` | 200→400 | 变更类请求并发上限 |
| `--watch-cache-sizes` | 按资源类型调大 | 减少穿透到 etcd 的 LIST |
| `--enable-priority-and-fairness=true` | 必须启用（1.29+ 默认开） | APF 分级限流，防止单一客户端打爆 |
| `--audit-log-*` | 启用并外送 | 见 [[08-security-defense-checklist]] |
| `--encryption-provider-config` | 启用 | Secret 静态加密 |

**APF 实践：**

- 为系统组件（kubelet、controller、scheduler、监控）配置高优先级 `PriorityLevelConfiguration`
- 为批量任务/CI 系统配置低优先级 + 严格限流，防止压测式调用打满 APIServer
- 监控指标：`apiserver_flowcontrol_rejected_requests_total` 持续非零说明队列配置不合理

### 1.4 Scheduler / Controller-Manager

- 大集群调大客户端 QPS：`--kube-api-qps=100 --kube-api-burst=200`（默认 20/30 在数千节点规模下成为瓶颈）
- scheduler 关注 `scheduler_scheduling_attempt_duration_seconds` P99；超过 100ms 需排查
- controller-manager 在大集群下关注 `workqueue_depth`，队列积压说明 reconcile 跟不上

## 2. 节点与 kubelet 配置

### 2.1 节点规划

- **系统节点池**：承载 CoreDNS、Ingress Controller、Prometheus 等，taint `CriticalAddonsOnly`
- **业务节点池**：按工作负载类型分池（通用/计算型/内存型/GPU），标签规范化
- 节点 OS 内核 ≥ 4.19（推荐 5.x），关闭 swap（或 K8s 1.28+ 显式启用 swap 支持并配置策略）

### 2.2 kubelet 关键配置

```yaml
# /var/lib/kubelet/config.yaml 关键项
maxPods: 110                    # 与 CNI 单节点 IP 供给能力匹配
serializeImagePulls: false      # 并行拉镜像，提升冷启动速度（需磁盘 IO 支撑）
registryPullQPS: 10             # 镜像仓库限流，防止拉爆 registry
registryBurst: 20
eventRecordQPS: 5               # 大集群务必限流事件，默认 50 会打爆 etcd
systemReserved: {cpu: "500m", memory: "1Gi"}
kubeReserved:   {cpu: "500m", memory: "2Gi"}
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "10%"
  imagefs.available: "15%"
imageGCHighThresholdPercent: 85
imageGCLowThresholdPercent: 70
protectKernelDefaults: true     # 校验内核参数，不符合则拒绝启动
```

### 2.3 容器运行时

- 生产推荐 **containerd**（K8s 1.24+ 已移除 dockershim）
- containerd 配置：`max_concurrent_downloads` 按磁盘能力调整（默认 3，NVMe 可调大到 8–16）
- 启用镜像懒加载（stargz / Nydus）可大幅降低大集群冷启动镜像拉取压力

## 3. CoreDNS 与集群 DNS

大规模集群 DNS 是最常见的隐性瓶颈：

1. **水平扩展**：按节点数/CoreDNS 实例比 8:1 ~ 16:1 部署，或使用 `cluster-proportional-autoscaler` 自动扩缩
2. **NodeLocal DNSCache**：每节点 DaemonSet 本地缓存，消除 conntrack 表溢出与跨节点解析延迟——**1,000+ 节点集群必装**
3. **调优 Corefile**：启用 `cache 30`、`autopath`（谨慎，内存换性能）、`loadbalance`
4. **避免 DNS 放大**：应用端合理设置 `dnsConfig.options`（如 `ndots: 2`，减少无效 search 域查询）

## 4. 事件与审计治理

- 事件（Event）写入 etcd，大集群下是隐藏的写放大源：kubelet `eventRecordQPS` 限流 + Event 对象 TTL 治理
- 使用 `event-exporter` 把事件外送到 ES/Loki/Kafka，减少依赖 `kubectl get events` 直查 etcd
- 审计日志**不要**直接写 APIServer 本地盘后不管：配置 log backend 轮转 + webhook backend 外送
- 审计策略分级：元数据级记录常规请求，RequestResponse 级只针对 Secret 等敏感资源

## 5. 证书与 RBAC 配置

- 证书有效期管理：kubeadm 默认 1 年，生产必须建立**证书到期巡检**（kube-prometheus 的 `kubeadm` 证书告警或自研巡检）
- 证书轮换纳入季度例行操作，并演练"全集群证书轮换"流程
- RBAC 基线：
  - 禁止向普通用户授予 `cluster-admin`
  - ServiceAccount 默认不自动挂载 token：`automountServiceAccountToken: false`，按需开启
  - 聚合 ClusterRole 分层管理，运维权限按域收敛

## 6. 版本与升级策略

| 项目 | 最佳实践 |
|---|---|
| 版本选择 | 生产使用社区仍在维护的 N-1 版本（比最新稳定版低一个小版本），避开 .0 版本 |
| 升级节奏 | 每 6–12 个月升级一次，不跨越两个以上小版本 |
| 升级顺序 | etcd 备份 → 控制面逐台 → 节点池分批（每批 ≤ 5%，配合 PDB 观察） |
| 演练 | 升级前在预发集群全流程演练；准备回滚方案（etcd 快照） |
| 兼容性检查 | 升级前用 `kubeadm upgrade plan` + 检查废弃 API（`kubectl get --raw /metrics \| grep deprecated` 或 Pluto/kubepug 扫描） |

## 7. 备份与灾难恢复

**必须备份的对象：**

1. **etcd 快照** —— 集群状态的最终来源
2. **声明式配置** —— 所有资源应有 GitOps 源（Git 仓库），etcd 损坏时可重建
3. **PV 数据** —— 走存储层快照/备份（见 [[05-storage]]）

**恢复演练要求：**

- 每季度至少一次"从 etcd 快照恢复集群"演练，记录 RTO
- 每半年一次"整集群重建"演练（新集群 + GitOps 恢复 + 数据卷挂载）
- 备份的可用性以"恢复成功"为准，不以"备份任务成功"为准

## 8. 大规模专属调优清单

| 领域 | 措施 |
|---|---|
| LIST 风暴治理 | 客户端统一走 informer/cache；禁止轮询式 `kubectl get --all` 脚本；大数据量 LIST 启用 `resourceVersion=0` 读 watch cache |
| 控制器拓扑 | **禁止每节点独立控制器全量 Watch**（启动时全节点同时 LIST 打垮 APIServer）——采用中心化控制器：集群级 1 个/少数实例统一监听（阿里云 ACK 大规模建议） |
| watch 连接 | APIServer `--target-ram-mb` 合理配置；监控 `apiserver_longrunning_requests` |
| 调度吞吐 | scheduler QPS/Burst 调大；评估 `percentageOfNodesToScore`（默认 50，大集群可调低提升吞吐） |
| 镜像分发 | 集群内 P2P 分发（Dragonfly/Nydus）；仓库多副本 + 就近拉取 |
| etcd 对象体积 | 单对象 < 1.5 MiB；禁止把大配置塞进 ConfigMap/CRD |
| 关键组件优先级 | CoreDNS、metrics-server 等关键 Addon 使用系统 PriorityClass（`system-cluster-critical` / `system-node-critical`），保证节点资源紧张时优先调度且不被抢占 |
| 准入插件基线 | 启用 `NodeRestriction`、`AlwaysPullImages`、`EventRateLimit`（CIS 基线，详见 [[12-security-hardening-baseline]]） |

## 9. 常见反模式

| 反模式 | 后果 | 正确做法 |
|---|---|---|
| etcd 用普通云盘 | Leader 切换频繁、APIServer 超时 | NVMe/高性能盘 + fsync 延迟监控 |
| 全组件默认 QPS | 节点多时 controller/scheduler 跟不上 | 按规模调大 QPS/Burst |
| Event 无限流 | etcd 写爆、磁盘打满 | kubelet eventRecordQPS 限流 + 外送 |
| 证书到期无人管 | 集群整体不可用 | 证书巡检 + 告警 + 例行轮换 |
| 只备份不演练 | 真正出事时恢复不了 | 季度恢复演练 |

## Related

- [[01-overview|大规模集群总览与规模基线]]
- [[06-initialization-checklist|初始化配置检查项]]
- [[07-pre-production-checklist|生产上线前检查项]]
- [[20-最佳实践/07-scenarios/upgrade-migration|升级迁移场景]]
- [[20-最佳实践/07-scenarios/backup-restore|备份恢复场景]]
