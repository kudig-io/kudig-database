---
title: 故障处置 Runbook 集
description: 大规模 Kubernetes 集群六大典型集群级故障的处置剧本：etcd 故障、APIServer 过载、DNS 故障、证书过期、节点批量 NotReady、调度雪崩，含止血步骤与根因排查路径
summary: 六大集群级故障的应急剧本：症状识别 → 止血动作 → 根因排查 → 恢复验证 → 事后改进，与值班告警直接挂钩
category: references
tags:
- k8s
- runbook
- incident-response
- troubleshooting
- production
tier: core
created: '2026-08-03'
last_updated: '2026-08-03'
difficulty: advanced
audience:
- SRE
- 值班工程师
estimated_read_time: 25min
---

# 故障处置 Runbook 集

> 集群级故障的处置铁律：**先止血保业务，再排查根因，最后恢复验证**。每个剧本按 症状 → 止血 → 排查 → 验证 → 改进 组织。命令风险标注：🟢 只读 / 🟡 可回滚 / 🔴 高风险。

## 1. etcd 故障

### 1.1 症状

- `etcd_server_has_leader = 0` 或 Leader 频繁切换告警
- APIServer 5xx 激增、`kubectl` 超时
- `etcd_mvcc_db_total_size_in_bytes` 逼近配额 / `etcd_server_quota_backend_bytes` 触发 alarm: NOSPACE

### 1.2 场景 A：磁盘写满（NOSPACE alarm）

```bash
# 确认 alarm 状态 🟢
etcdctl --endpoints=<ep> alarm list

# 1. 确认真实磁盘用量（db 文件 vs 配额）🟢
etcdctl --endpoints=<ep> endpoint status -w table

# 2. 执行 compaction（需先确认 revision）🔴
rev=$(etcdctl --endpoints=<ep> endpoint status --write-out="json" | jq '.[0].Status.header.revision')
etcdctl --endpoints=<ep> compact $rev

# 3. defrag 释放空间 🔴（逐节点滚动执行，Leader 最后）
etcdctl --endpoints=<ep> defrag

# 4. 解除 alarm 🟡
etcdctl --endpoints=<ep> alarm disarm
```

### 1.3 场景 B：Leader 频繁切换

- 排查顺序：`etcd_disk_wal_fsync_duration_seconds` P99（>10ms → 磁盘性能问题，最常见根因）→ 节点 IO/网络抖动 → etcd 进程 CPU 饥饿（与 APIServer 混部被抢）
- 止血：若磁盘性能不达标，将 etcd 迁移至 NVMe 节点（成员替换流程）
- **禁止动作**：未备份前不做任何成员移除/强制恢复操作

### 1.4 场景 C：数据损坏需恢复

→ 转 [[18-disaster-recovery-runbook#2. etcd 快照恢复（自建集群）]]。

### 1.5 恢复验证

- [ ] `endpoint status --cluster` 全部健康、Leader 唯一
- [ ] APIServer 错误率回落至基线
- [ ] alarm list 为空，db size 低于配额 60%

## 2. APIServer 过载

### 2.1 症状

- `apiserver_request_duration_seconds` P99 > 1s，请求排队/超时
- `apiserver_flowcontrol_rejected_requests_total` 激增（APF 拒绝）
- `apiserver_current_inflight_requests` 打满上限

### 2.2 止血（按顺序）

1. **定位压力源** 🟢：
   ```bash
   # 按 verb/资源聚合请求量（客户端身份结合审计日志中的 userAgent 定位）
   kubectl get --raw '/metrics' | grep apiserver_request_total | sort -k2 -nr | head
   ```
   或用 APF 指标：`apiserver_flowcontrol_current_executing_requests` 按 priorityLevel 拆解
2. **阻断压力源**：
   - 失控客户端（脚本/控制器风暴）→ 吊销其 SA token / 删除该控制器副本 🟡
   - LIST 风暴 → 临时调低该客户端并发；确认是否有人跑 `kubectl get --all-namespaces` 循环脚本
3. **临时扩容余量** 🟡：提高 `max-requests-inflight`（需重启 apiserver，逐台滚动）；云上托管集群提交工单升配
4. 万不得已：对非关键 priorityLevel 收紧 APF 队列，保 system 与 leader-election

### 2.3 常见根因对照

| 现象 | 根因 |
|---|---|
| 每天定时发作 | CronJob/巡检脚本集中 LIST |
| 大促/发布时发作 | 控制器并发不足 vs 客户端 QPS 未限流 |
| 持续缓慢上升 | watch 频繁重建（客户端反复断线重连）、etcd 慢拖累 |
| 节点批量加入时发作 | kubelet 全量 LIST（确认是否启用 watchlist 特性） |

### 2.4 恢复验证

- [ ] P99 < 1s 持续 30 分钟
- [ ] APF rejected 归零
- [ ] 压力源客户端已限流/修复并登记

## 3. 集群 DNS 故障

### 3.1 症状

- 大量 Pod 报 `Temporary failure in name resolution`
- CoreDNS CPU 打满 / OOM / SERVFAIL 率飙升
- NodeLocal DNSCache DaemonSet 异常

### 3.2 止血

1. **快速扩容 CoreDNS** 🟡：`kubectl scale deploy coredns -n kube-system --replicas=<当前×2>`（临时绕过 autoscaler）
2. **NodeLocal 异常时**：逐节点重启该 DaemonSet Pod（分批！避免全集群同时失去本地缓存）🟡
3. **上游 DNS 故障**（forward 目标不可达）：CoreDNS 切备用上游 🟡
4. **conntrack 打满导致**（UDP DNS 大量短连接）：确认节点 `nf_conntrack` 使用率，临时调大并根治上 NodeLocal

### 3.3 排查

- `coredns_dns_request_duration_seconds` 分类型拆解：是 forward 慢（上游问题）还是 kubernetes 插件慢（APIServer 问题）
- 检查是否某服务产生 DNS 放大（ndots 默认值导致 search 域遍历，单域名变 5 次查询）

### 3.4 恢复验证

- [ ] SERVFAIL 率 < 0.1%、P99 解析 < 10ms
- [ ] 核心业务探活恢复

## 4. 证书过期

### 4.1 症状

- `kubectl` 报 `x509: certificate has expired`
- kubelet NotReady（无法连接 apiserver）、控制面组件日志 x509 报错

### 4.2 处置

```bash
# 确认过期范围 🟢
kubeadm certs check-expiration

# 逐台 Master 轮换并重启静态 Pod 🟡
kubeadm certs renew all
crictl stop $(crictl ps -q --name kube-apiserver)   # 其余组件同理
```

- kubelet 证书过期：确认 `rotateCertificates` 开启，手动 approve 积压 CSR：`kubectl certificate approve <csr>` 🟡
- CA 过期（10 年根）：不是轮换能解决的——按 CA 轮换专项方案执行，**没有预案时先联系最有经验的人，不要临场发挥**
- 预防见 [[13-upgrade-certificate-runbook#6. 证书生命周期管理]]——此类故障 100% 可防

## 5. 节点批量 NotReady

### 5.1 症状

- 数十至数百节点同时 NotReady，大量 Pod 重调度
- 常见诱因：网络分区、kubelet 批量崩溃（运行时 bug/证书）、云平台故障

### 5.2 处置决策树

1. **判断是否控制面问题**：`kubectl` 是否正常？控制面挂 → 按场景 1/2 处置
2. **判断是否网络问题**：跨节点 ping/安全组变更审计 → 网络层修复
3. **判断是否 kubelet 共性崩溃**：登录抽样节点看 `journalctl -u kubelet` 共性报错（证书/CNI/运行时）
4. **止血关键动作**：
   - 确认 `pod-eviction-timeout`（默认 5min）——批量 NotReady 超过该时间会触发大规模驱逐，可能压垮 APIServer 与镜像仓库
   - 预判重调度风暴：必要时临时调大 scheduler QPS、对镜像仓库限流预案准备
5. **分批恢复**：节点修复分批 uncordon，每批观察驱逐/调度指标

### 5.3 恢复验证

- [ ] NotReady 归零、无 Pending 积压
- [ ] 关键业务 PDB 未被击穿（`kubectl get pdb -A` 无 ALLOWED DISRUPTIONS = 0 的卡死）

## 6. 调度雪崩（大规模 Pending）

### 6.1 症状

- `scheduler_pending_pods` 持续积压，新 Pod 大面积 Pending
- 触发场景：批量发布、HPA 共振扩容、节点池容量耗尽

### 6.2 排查顺序

| 检查 | 命令 |
|---|---|
| Pending 原因分布 | `kubectl get pods -A --field-selector=status.phase=Pending` + describe 看事件 |
| 资源不足 vs 约束不满足 | 事件是 `Insufficient cpu/memory` 还是亲和/污点不匹配 |
| 调度器本身 | `scheduler_scheduling_attempt_duration_seconds`、调度器日志报错 |
| 供给链路 | CA/Karpenter 是否在工作？云配额是否打满？（[[11-autoscaling-capacity#4. 云配额与扩容速率治理（官方大规模注意事项）]]） |

### 6.3 止血

- 容量不足 → CA/Karpenter 扩容；云配额打满 → 提额工单 + 临时跨池调度
- 供给正常但调度慢 → 滚动重启 scheduler 并临时调大 QPS/Burst 🟡
- 发布触发 → 暂停发布流水线（冻结并发 Deployment 滚动）

## 7. 通用处置纪律

1. **每次操作留痕**：处置过程中所有执行命令记入事件时间线
2. **一次只改一个变量**：多个止血动作叠加后无法判断哪个生效
3. **熔断意识**：止血动作本身有爆炸半径（如大规模重启 DaemonSet），评估后再执行
4. **24 小时内复盘**：时间线、根因（5-Why）、改进项进 backlog 并跟踪闭环
5. **改进项回归**：本 Runbook 每次实战后必须更新

## Related

- [[09-observability|可观测性体系最佳实践（告警定位）]]
- [[18-disaster-recovery-runbook|灾备恢复 Runbook]]
- [[15-slo-chaos-engineering|SLO 与混沌工程（演练验证这些剧本）]]
- [[19-故障诊断/README|故障诊断域]]
