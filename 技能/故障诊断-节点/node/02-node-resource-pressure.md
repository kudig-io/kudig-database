---
title: 节点资源压力诊断与修复
description: 针对节点 MemoryPressure、DiskPressure、PIDPressure 三大资源压力故障的完整诊断技能，覆盖 10 种根因的分阶段诊断、Pod 驱逐分析及修复方案
summary: 节点资源压力是最常见但常被忽视的问题类型，处于压力状态的节点仍标记为 Ready 但会主动驱逐 Pod，本技能提供完整的压力诊断与修复路径
category: skill
tags:
- k8s
- node
- troubleshooting
- memorypressure
- diskpressure
- pidpressure
- eviction
- oom
- inode
- sop
sources:
- 故障诊断/技能体系/19-node-resource-pressure.md
- 故障诊断/资源排障/09-node-comprehensive-troubleshooting.md
- 故障诊断/FTA故障树/list/node-fta.md
- 故障诊断/高级排障/structural-02-node-components/01-kubelet-troubleshooting.md
- 故障诊断/高级排障/structural-02-node-components/04-node-troubleshooting.md
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- 节点内存压力怎么解决
- DiskPressure 如何排查
- Pod 被驱逐什么原因
- 节点 PID 压力怎么处理
- inode 耗尽怎么办
trigger_keywords:
- MemoryPressure
- DiskPressure
- PIDPressure
- Evicted
- 节点资源压力
- 内存压力
- 磁盘压力
- inode耗尽
- OOM
- 节点驱逐
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- linux-resource-management
skill_id: SKILL-NODE-002
skill_name: 节点资源压力诊断与修复
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L2-semi-auto
fta_path: TE-1 -> IE-1.4 -> BE-1.4~BE-1.6
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 节点资源压力诊断与修复

> **Skill ID**: SKILL-NODE-002
> **Agent 执行模式**: L2-semi-auto — 低风险操作自动执行，中/高风险需人工审批
> **预计修复时间**: 10-60 分钟
> **FTA 路径**: TE-1 → IE-1.4 → BE-1.4 (Disk) / BE-1.5 (Memory) / BE-1.6 (PID)

---

## 1. 概述

节点资源压力（MemoryPressure / DiskPressure / PIDPressure）是 Kubernetes 集群中最常见但常被忽视的问题类型。与 Node NotReady 不同，处于资源压力状态的节点仍标记为 `Ready`，但 kubelet 会主动驱逐 Pod 以回收资源。若不及时处理，资源压力可能级联扩散，导致大规模 Pod 驱逐、服务降级甚至集群雪崩。

本 Skill 覆盖内存压力、磁盘压力（含 inode 耗尽）、PID 压力、镜像/容器存储膨胀、系统 OOM 等全部 10 种根因的诊断和修复。

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 |
|---|---------|---------|--------|
| S1 | 节点 MemoryPressure=True | `kubectl get nodes` / Prometheus | 0.95 |
| S2 | 节点 DiskPressure=True | `kubectl get nodes` / Prometheus | 0.95 |
| S3 | 节点 PIDPressure=True | `kubectl get nodes` / Prometheus | 0.95 |
| S4 | Pod 状态为 Evicted | `kubectl get pods -A | grep Evicted` | 0.90 |
| S5 | Pod 被 OOMKilled (exit 137) | `kubectl get events --field-selector reason=OOMKilled` | 0.85 |
| S6 | 镜像拉取失败 + DiskPressure | `ImagePullBackOff` + 节点 DiskPressure | 0.80 |
| S7 | 调度失败 + 目标节点有 Pressure | `FailedScheduling` + 节点 Pressure | 0.85 |

### 2.2 排除标准

- 节点 NotReady → 使用 SKILL-NODE-001
- Pod 被驱逐但节点无 Pressure（手动 drain/污点）→ 检查维护操作
- 容器 OOMKilled 但节点内存充足（limits 过低）→ SKILL-POD-001
- 磁盘 I/O 性能问题但空间充足 → SKILL-PERF-001

---

## 3. 快速分级（2 分钟内完成）

```
压力类型 + 影响范围
├── 多节点同时 MemoryPressure ──────→ P0（立即处理）
├── 单节点 DiskPressure + 核心服务 ─→ P0（30min 内修复）
├── 单节点 MemoryPressure ─────────→ P1（1h 内修复）
├── 单节点 DiskPressure（非核心）───→ P1（2h 内修复）
├── PIDPressure ────────────────────→ P1（1h 内修复）
└── 轻微压力（阈值附近）────────────→ P2（4h 内处理）
```

**立即升级条件**：
- >30% 节点同类型 Pressure
- 控制平面节点 MemoryPressure（威胁 etcd）
- 磁盘 100% 满导致 kubelet 无法写入
- 压力节点上有状态服务无法迁移

### 影响评估命令

```bash
# 🟢 低风险
# T1: 统计压力节点
kubectl get nodes -o json | jq -r '.items[] | select(.status.conditions[]?.status == "True" and (.status.conditions[]?.type | test("Pressure"))) | .metadata.name'

# T2: 检查被驱逐 Pod
kubectl get pods -A --field-selector=status.phase=Failed | grep Evicted | awk '{print $1}' | sort | uniq -c | sort -rn

# T3: 检查节点资源使用
kubectl top node <node-name>
```

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

**Step D1.1**: 获取压力节点概览
```bash
kubectl describe node <node-name> | grep -A 10 "Conditions:"
kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name> \
  --sort-by=.lastTimestamp | tail -20
```

**Step D1.2**: 检查被驱逐 Pod 详情
```bash
kubectl get pods -A --field-selector=status.phase=Failed -o json | \
  jq -r '.items[] | select(.status.reason=="Evicted") |
  "\(.metadata.namespace)/\(.metadata.name) | \(.spec.nodeName) | \(.status.message)"' | head -20
```
- Message 含 `memory` → 内存压力
- Message 含 `disk`/`ephemeral-storage` → 磁盘压力
- Message 含 `pid` → PID 压力

**Step D1.3**: 检查节点资源分配
```bash
kubectl describe node <node-name> | grep -A 20 "Allocated resources"
kubectl top node <node-name>
```

---

### Phase 2: 深度检查（只读，需 SSH）

**Step D2.1**: 检查系统内存
```bash
ssh <node-ip> "free -m && echo '---' && cat /proc/meminfo | grep -E '^(Mem|Swap|Buffers|Cached)'"
```
- available < 100Mi → 严重内存不足

**Step D2.2**: 检查磁盘使用（含 inode）
```bash
ssh <node-ip> "df -h / /var/lib/kubelet /var/lib/containerd /var/log /tmp && echo '---INODE---' && df -i / /var/lib/kubelet /var/lib/containerd"
```
- /var/lib/containerd > 85% → 镜像/容器层膨胀
- /var/log > 85% → 日志膨胀
- inode > 90% → inode 耗尽

**Step D2.3**: 检查容器运行时存储
```bash
ssh <node-ip> "crictl ps -a | wc -l && crictl images | wc -l && du -sh /var/lib/containerd"
```

**Step D2.4**: 检查 kubelet 驱逐日志
```bash
ssh <node-ip> "journalctl -u kubelet --since '30 minutes ago' --no-pager | grep -iE 'evict|pressure|threshold' | tail -30"
```

**Step D2.5**: 检查 PID 使用
```bash
ssh <node-ip> "ps aux --no-heading | wc -l && cat /proc/sys/kernel/pid_max && ps -eo user | sort | uniq -c | sort -rn | head -10"
```

**Step D2.6**: 检查大文件
```bash
ssh <node-ip> "du -h /var/log /var/lib/containerd /var/lib/kubelet /tmp 2>/dev/null | sort -rh | head -20"
ssh <node-ip> "find /var/log -type f -size +100M -exec ls -lh {} \; | head -10"
```

**Step D2.7**: 检查 OOM Killer 日志
```bash
ssh <node-ip> "dmesg -T | grep -i 'killed process|oom-killer|out of memory' | tail -20"
```

**Step D2.8**: 检查 kubelet 驱逐阈值配置
```bash
ssh <node-ip> "cat /var/lib/kubelet/config.yaml | grep -A 20 'evictionHard'"
```
默认阈值：`memory.available<100Mi`，`nodefs.available<10%`，`imagefs.available<15%`，`nodefs.inodesFree<5%`

---

## 5. 根因分类（10 种）

| RC ID | 根因 | 概率 | 诊断证据 |
|-------|------|------|---------|
| RC-001 | 节点内存实际耗尽 | 高 | D2.1 available<100Mi; D2.7 OOM |
| RC-002 | kubelet 驱逐阈值过严 | 中 | D2.8 阈值设置; 实际内存充足 |
| RC-003 | 容器/应用内存泄漏 | 中 | D1.3 单 Pod 内存异常高 |
| RC-004 | 日志文件膨胀（未轮转） | 高 | D2.2 /var/log 高; D2.6 大文件 |
| RC-005 | kubelet/emptyDir 数据占用 | 中 | D2.2 /var/lib/kubelet 高 |
| RC-006 | 镜像/容器层膨胀 | 高 | D2.3 镜像数量多; dangling 多 |
| RC-007 | inode 耗尽（大量小文件） | 低 | D2.2 inode>90% |
| RC-008 | 系统 PID 上限耗尽 | 低 | D2.5 PID 接近上限 |
| RC-009 | 容器/应用线程泄漏 | 低 | D2.5 单用户线程异常高 |
| RC-010 | 临时文件堆积 | 中 | D2.2 /tmp 高 |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可自动执行）

#### REM-001: 清理已停止容器
```bash
ssh <node-ip> "crictl ps -a | grep Exited | awk '{print \$1}' | xargs -r crictl rm"
```

#### REM-002: 清理 dangling 镜像
```bash
ssh <node-ip> "crictl rmi --prune"
```

#### REM-003: 清理 Pod emptyDir（删除 Pod 让其重建）
```bash
kubectl delete pod <pod-name> -n <namespace>
```

### 6.2 🟡 中风险（人工审批）

#### REM-004: 清理日志文件
```bash
ssh <node-ip> "find /var/log -type f -size +500M -exec truncate -s 0 {} \;"
ssh <node-ip> "journalctl --vacuum-size=500M"
ssh <node-ip> "logrotate -f /etc/logrotate.conf"
```

#### REM-005: 调整 kubelet 驱逐阈值（临时）
```bash
ssh <node-ip> "cp /var/lib/kubelet/config.yaml /var/lib/kubelet/config.yaml.bak"
# 修改阈值
ssh <node-ip> "systemctl restart kubelet"
```

#### REM-006: 排空节点释放资源
```bash
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force
# 清理后
kubectl uncordon <node-name>
```

### 6.3 🔴 高风险

#### REM-007: 节点扩容/替换
- 云环境：通过 Node Group 扩容或替换实例
- 裸金属：增加磁盘/内存或替换节点

---

## 7. 验证确认

```bash
# 🟢 低风险
kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
# 预期: MemoryPressure=False, DiskPressure=False, PIDPressure=False

ssh <node-ip> "df -h / /var/lib/containerd /var/lib/kubelet"
# 预期: 使用率 < 85%

kubectl get pods -A --field-selector spec.nodeName=<node-name> | grep -c Evicted
# 预期: 无新增 Evicted
```

---

## 8. 预防措施

### 监控告警配置

```yaml
# PrometheusRule - 节点资源压力
- alert: NodeMemoryPressure
  expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
  for: 5m
  labels:
    severity: warning
- alert: NodeDiskPressure
  expr: (node_filesystem_avail_bytes / node_filesystem_size_bytes) < 0.1
  for: 5m
  labels:
    severity: warning
- alert: NodeDiskPredictFull
  expr: predict_linear(node_filesystem_avail_bytes[1h], 3600) < 0
  for: 10m
  labels:
    severity: critical
```

### 日常巡检要点

- 磁盘使用率告警阈值设为 80%
- 配置容器日志轮转（`logMaxSize: 100Mi`, `logMaxFiles: 5`）
- 配置 imageGC 阈值（`imageGCHighThresholdPercent: 85`）
- 定期清理无用镜像和停止容器
- 配置 systemReserved 和 kubeReserved

---

## 版本兼容性注意事项

> 详细版本差异请参考 [reference/node-version-differences.md](reference/node-version-differences.md)

| 版本 | 关键差异 | 诊断影响 |
|------|---------|----------|
| ≤1.27 | NodeSwap Alpha(默认关)，kubelet 要求禁用 swap | 有 swap 则 kubelet 启动失败 |
| 1.28~1.29 | NodeSwap Beta(默认关) | 可选启用 swap |
| 1.30~1.33 | NodeSwap Beta(默认开) | 默认允许 swap，MemoryPressure 诊断需区分物理内存与 swap |
| 1.34+ | NodeSwap **GA(locked)** | 始终允许 swap，无法禁用 |
| 1.33+ | InPlacePodVerticalScaling Beta | Pod 资源可原地调整，不触发重新调度 |
| 1.36+ | NodeStatus 显示 swap 容量 | `kubectl get node -o yaml` 可查看 swap 信息 |

**版本特定诊断命令**：

```bash
# 🟢 检查节点 swap 状态（1.30+ 尤其重要）
free -h

# 🟢 检查 kubelet swap 配置
grep -i swap /var/lib/kubelet/config.yaml

# 🟢 检查节点 swap 容量（1.36+）
kubectl get node <node-name> -o jsonpath='{.status}' | jq '.conditions[] | select(.type=="MemoryPressure")'
```

**注意**：1.30+ 集群中，MemoryPressure 诊断时必须区分物理内存压力与 swap 使用情况。`memorySwap.swapBehavior` 配置决定了 kubelet 如何计算内存压力（`LimitedSwap` 或 `NoSwap`）。

---

## 9. 驱逐配置深度治理

> 来源：`故障诊断/高级排障/structural-02-node-components/01-kubelet-troubleshooting.md` §3

### 9.1 驱逐策略完整配置

```yaml
# /var/lib/kubelet/config.yaml
# 硬驱逐（立即驱逐，无宽限期）
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"

# 软驱逐（宽限期后驱逐）
evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"
evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "2m"

# 驱逐最小 Pod 年龄保护
evictionMinimumReclaim:
  memory.available: "0Mi"
  nodefs.available: "500Mi"
```

### 9.2 驱逐顺序（QoS 优先级）

1. **BestEffort** Pod（无资源请求）— 最先驱逐
2. **Burstable** Pod 且使用量超过请求量
3. **Burstable** Pod 且使用量未超请求量
4. **Guaranteed** Pod（requests = limits）— 最后驱逐

### 9.3 资源预留配置（防止节点夿死）

生产环境必须配置资源预留，否则当 Pod 负载过高时，kubelet 自身会因申请不到 CPU/内存而假死：

```yaml
# /var/lib/kubelet/config.yaml
systemReserved:
  cpu: "500m"
  memory: "1Gi"
kubeReserved:
  cpu: "500m"
  memory: "1Gi"
enforceNodeAllocatable: ["pods", "system-reserved", "kube-reserved"]
```

### 9.4 磁盘压力深度治理

```yaml
# 镜像 GC 策略
imageGCHighThresholdPercent: 80   # 磁盘使用率超过此值触发 GC
imageGCLowThresholdPercent: 70    # GC 直到降至此值

# 容器日志轮转
containerLogMaxSize: "100Mi"
containerLogMaxFiles: 5
```

日志轮转优化：修改 `/etc/logrotate.d/` 确保宿主机日志不挤占空间。

### 9.5 证书自动轮转

配置 `rotateCertificates: true` 仅是第一步，还需确保：
1. RBAC：kubelet 有权创建 CSR
2. Controller Manager 配置了签发参数
3. 如果证书已过期无法启动：手动续签并重启

---

## 10. 节点健康自愈与自动化预防

> 来源：`故障诊断/高级排障/structural-02-node-components/01-kubelet-troubleshooting.md` §4

### 10.1 Node Problem Detector (NPD) + Draino

| 组件 | 职责 | 关键能力 |
|------|------|----------|
| NPD | 监测内核死锁、文件系统只读、内存坏道等异常 | 暴露自定义 Condition |
| Draino | 根据 NPD Condition 自动驱离 Pod | 自动 drain + 重启节点 |
| Descheduler | 重新平衡集群 Pod 分布 | 策略化重调度 |

### 10.2 节点驱逐保护机制

在大规模集群中，节点批量 NotReady 是极度危险的场景：

1. **驱逐速率限制**：当集群中超过 20% 节点 NotReady 时，Node Controller 进入"部分问题"模式，驱逐速率降至每秒 0.01 个节点
2. **Graceful Node Shutdown**（v1.26+ 默认开启）：kubelet 感知节点关机信号，优先终止 Pod，给予关键应用数据刷盘时间
3. **PodDisruptionConditions**（v1.25+）：Pod 被驱逐时记录原因 Condition

### 10.3 生产环境典型"节点陷阱"

| 陷阱 | 现象 | 原因 | 对策 |
|--------|------|------|------|
| CPU 节流 | CPU 使用率不高但 Pod 响应变慢 | CFS 调度器周期限制与 CPU Limit 冲突 | CPU Manager `static` 策略 |
| 幽灵节点 | 节点已在云端删除但仍显示 NotReady | CCM 同步异常 | 手动 `kubectl delete node` |
| 批量磁盘爆满 | 多节点同时 NotReady | 日志累积/镜像缓存膨胀 | 磁盘清理策略+监控告警 |
| 恶意挖矿 | CPU 飙升+节点压力异常 | 安全漏洞被利用 | 安全加固+准入控制 |

---

## 相关链接

- [[技能/故障诊断-节点/node/README.md|Node 异常诊断技能集]]
- [[技能/故障诊断-节点/node/01-node-notready-diagnosis.md|Node NotReady 诊断]]
- [[技能/故障诊断-节点/node/03-node-component-troubleshooting.md|节点组件故障排查]]
- [[技能/故障诊断-节点/node/04-node-sop-runbook.md|Node SOP 与 Runbook]]
- [[技能/故障诊断-节点/node/05-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查]]
- [[技能/故障诊断-节点/node/reference/node-version-differences.md|版本差异对比]]
- [[故障诊断/FTA故障树/list/node-fta.md|Node 故障树分析]]
- [[故障诊断/高级排障/structural-02-node-components/01-kubelet-troubleshooting.md|kubelet 故障排查指南（原始文件）]]
