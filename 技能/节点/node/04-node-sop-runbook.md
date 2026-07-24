---
title: Node 异常标准操作流程（SOP）与 Runbook
description: Node 异常诊断的标准操作流程、升级决策树、修复 Runbook、预防措施及生产案例汇总，为 SRE 和 AI Agent 提供可执行的标准化操作指南
summary: 本技能提供 Node 异常处理的完整 SOP 体系，包含快速诊断检查表、修复动作速查、升级决策及预防性运维清单
category: skill
tags:
- k8s
- node
- troubleshooting
- sop
- runbook
- escalation
- prevention
- operations
sources:
- 故障诊断/技能体系/01-node-notready.md
- 故障诊断/技能体系/19-node-resource-pressure.md
- 故障诊断/技能体系/skill-set/k8s-node-notready/reference/remediation-playbook.md
- 故障诊断/核心排障/06-node-notready-diagnosis.md
created: '2026-07-23'
updated: '2026-07-23'
lifecycle: active
tier: core
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
- 值班工程师
estimated_read_time: 15min
intent_queries:
- Node 故障处理 SOP 是什么
- 节点异常的标准操作流程
- Node NotReady 的 Runbook
- 节点故障升级决策怎么做
- 如何预防节点故障
trigger_keywords:
- SOP
- Runbook
- 操作流程
- 升级决策
- 预防措施
- 节点维护
- 故障恢复
- 应急预案
prerequisites:
- kubectl-basics
- node-architecture
- troubleshooting-methodology
skill_id: SKILL-NODE-004
skill_name: Node 异常 SOP 与 Runbook
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
- 1.34.x
- 1.36.x
agent_execution_mode: L1-advisory
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Node 异常标准操作流程（SOP）与 Runbook

> **Skill ID**: SKILL-NODE-004
> **Agent 执行模式**: L1-advisory — 提供建议和操作指导，所有操作需人工确认

---

## 1. 快速诊断检查表

```
# 🟢 低风险：只读/信息收集，通常无副作用
□ 能 SSH 登录节点?
  ├── Yes → 继续
  └── No  → 检查网络/云平台实例状态

□ kubelet 进程运行?
  ├── systemctl status kubelet
  └── 不运行 → 查日志并重启

□ containerd 运行?
  ├── systemctl status containerd
  └── 不运行 → 查日志并重启

□ 能连接 API Server?
  ├── curl -k https://apiserver:6443/healthz
  └── 不能 → 检查网络/证书

□ 证书有效?
  ├── openssl x509 -in <cert> -noout -dates
  └── 过期 → 更新证书

□ 资源充足?
  ├── free -h / df -h
  └── 不足 → 清理资源

□ CNI 正常?
  ├── 检查 CNI Pod 状态
  └── 异常 → 重启 CNI
```

---

## 2. 修复动作速查表

| 根因 | 修复动作 | 风险 | 验证命令 |
|------|---------|------|---------|
| RC-012 节点被 cordon | `kubectl uncordon <node>` | 🟢 低 | `kubectl get node <node>` |
| RC-003 磁盘压力 | `crictl rmi --prune && journalctl --vacuum-time=2d` | 🟢 低 | `df -h /var/lib/containerd` |
| RC-001 kubelet 异常 | `systemctl restart kubelet` | 🟡 中 | `systemctl status kubelet` |
| RC-002 containerd 异常 | `systemctl restart containerd && systemctl restart kubelet` | 🟡 中 | `crictl ps` |
| RC-005 驱逐阈值 | 调整 kubelet evictionHard 后重启 | 🟡 中 | `kubectl get node <node>` |
| RC-007 证书过期 | 手动证书轮转或 CSR 批准 | 🟡 中 | `openssl x509 -dates` |
| RC-004 内存耗尽 | `kubectl drain` → 修复 → `uncordon` | 🔴 高 | `free -m` |
| RC-009 硬件问题 | 节点替换 | 🔴 高 | `kubectl get nodes` |
| RC-006 网络不通 | 网络修复（防火墙/路由/安全组） | 🔴 高 | `nc -zv apiserver 6443` |

---

## 3. 升级决策树

```
Node 异常告警
    │
    ├── 影响范围评估
    │   ├── >50% 节点 NotReady ────────→ 立即升级（P0，跳过诊断）
    │   ├── 控制平面节点 NotReady ─────→ 立即升级（P0）
    │   ├── apiserver 不可达 ──────────→ 立即升级（P0）
    │   ├── 5min 内 NotReady 持续增加 → 立即升级（级联问题）
    │   └── 单节点/少量节点 ──────────→ 继续诊断
    │
    ├── 诊断结果
    │   ├── 根因明确 + 有标准修复方案 → 执行修复
    │   ├── 根因明确 + 需高风险操作 ──→ 升级审批后执行
    │   ├── 根因不明确 ─────────────→ 升级给高级 SRE
    │   └── 修复后未恢复 ───────────→ 升级给高级 SRE
    │
    └── 升级消息模板
        ├── 问题描述：[节点名/IP/状态/持续时间]
        ├── 影响范围：[受影响 Pod 数/namespace/业务]
        ├── 已执行操作：[诊断步骤和结果]
        ├── 初步判断：[疑似根因]
        └── 需要的支持：[具体请求]
```

---

## 4. 标准操作流程

### 4.1 单节点 NotReady SOP

| 步骤 | 操作 | 超时 | 预期结果 |
|------|------|------|---------|
| 1 | `kubectl get nodes -o wide` | 10s | 确认 NotReady 节点 |
| 2 | `kubectl describe node <node>` | 15s | 获取 Conditions |
| 3 | `kubectl get events --field-selector involvedObject.kind=Node` | 10s | 获取事件 |
| 4 | SSH 到节点检查 kubelet | 10s | 确认进程状态 |
| 5 | 检查 kubelet 日志 | 15s | 定位错误 |
| 6 | 检查 containerd | 10s | 确认运行时 |
| 7 | 检查系统资源 | 10s | 排除资源压力 |
| 8 | 检查网络连通性 | 15s | 排除网络问题 |
| 9 | 执行修复 | 视情况 | 修复根因 |
| 10 | 验证恢复 | 60s | 节点 Ready |

### 4.2 批量节点 NotReady SOP

| 步骤 | 操作 | 说明 |
|------|------|------|
| 1 | 统计 NotReady 节点数量和分布 | 判断影响范围 |
| 2 | 检查时间关联性 | 同时发生→共同根因 |
| 3 | 检查 Zone/Rack 分布 | 同区域→物理网络问题 |
| 4 | 检查控制平面健康 | etcd/apiserver/kcm |
| 5 | 检查网络基础设施 | 交换机/路由器/防火墙 |
| 6 | 如 >50% 或控制平面异常 | 立即升级 |
| 7 | 逐节点或分批恢复 | 按优先级处理 |

### 4.3 节点维护 SOP（计划内）

```bash
# 1. 标记节点不可调度
kubectl cordon <node-name>

# 2. 排空工作负载
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --grace-period=60

# 3. 确认 Pod 迁移完成
kubectl get pods --field-selector spec.nodeName=<node-name> -A
# 预期: 仅剩 DaemonSet Pod

# 4. 执行维护操作
# ...

# 5. 恢复调度
kubectl uncordon <node-name>

# 6. 验证节点状态
kubectl get node <node-name>
kubectl get pods --field-selector spec.nodeName=<node-name> -A
```

---

## 5. 预防性运维清单

### 5.1 监控配置

- [ ] 部署 Node Exporter
- [ ] 配置节点状态告警 (NotReady/Unknown)
- [ ] 配置资源使用告警 (CPU/Memory/Disk)
- [ ] 配置 kubelet 健康告警
- [ ] 配置证书过期监控（30 天前告警）
- [ ] 配置磁盘预测性告警（1h 后满）

### 5.2 资源管理

- [ ] 配置 systemReserved 和 kubeReserved
- [ ] 配置合理的驱逐阈值
- [ ] 磁盘使用率 80% 告警
- [ ] 定期清理无用镜像和容器
- [ ] 配置容器日志轮转

### 5.3 证书管理

- [ ] 启用证书自动轮转（RotateKubeletClientCertificate）
- [ ] 配置证书过期监控
- [ ] 定期检查 CSR 状态
- [ ] 确保 bootstrap token 有效

### 5.4 运维准备

- [ ] 文档化故障处理流程 (Runbook)
- [ ] 定期演练恢复流程
- [ ] 配置节点自愈（如 ACK 节点自愈）
- [ ] 准备诊断脚本
- [ ] 配置 NTP 时间同步

### 5.5 推荐 Prometheus 告警规则

```yaml
groups:
- name: node-health
  rules:
  - alert: NodeNotReady
    expr: kube_node_status_condition{condition="Ready",status="true"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "节点 {{ $labels.node }} NotReady"

  - alert: NodeDiskPressure
    expr: (node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"}) < 0.1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "节点 {{ $labels.instance }} 磁盘剩余不足 10%"

  - alert: NodeMemoryPressure
    expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
    for: 5m
    labels:
      severity: warning

  - alert: NodeHighCPU
    expr: 100 - (avg by(instance)(irate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 90
    for: 10m
    labels:
      severity: warning

  - alert: KubeletCertExpiringSoon
    expr: (kubelet_certificate_manager_client_ttl_seconds < 2592000)
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "kubelet 证书将在 30 天内过期"
```

---

## 6. 验证确认协议

> 以下内容整合自 `故障诊断/技能体系/skill-set/k8s-node-notready/reference/remediation-playbook.md`

### 6.1 即时验证（修复后 1-2 分钟内）

```bash
# 🟢 低风险：只读/信息收集
# V1: 确认节点状态恢复为 Ready
kubectl get node <node-name>
# 预期: STATUS 列显示 Ready

# V2: 确认所有 Conditions 恢复正常
kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
# 预期: MemoryPressure=False, DiskPressure=False, PIDPressure=False, Ready=True

# V3: 确认 Node Lease 正常续租
kubectl get lease -n kube-node-lease <node-name> -o jsonpath='{.spec.renewTime}'
# 预期: 时间戳为最近几秒内

# V4: 确认 Pod 恢复调度和运行
kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces
# 预期: Pod 状态为 Running

# V5: 确认节点上 kubelet 版本和运行时信息正确
kubectl get node <node-name> -o jsonpath='kubelet={.status.nodeInfo.kubeletVersion} runtime={.status.nodeInfo.containerRuntimeVersion}'
# 预期: 版本信息与集群其他节点一致
```

### 6.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|----------|
| 节点 CPU 使用率 | `kubectl top node <node-name>` | 稳定在正常范围 | 持续 >90% 超过 5 分钟 |
| 节点内存使用率 | `node_memory_MemAvailable_bytes` | 可用内存保持在驱逐阈值以上 | 可用内存 <200Mi 且持续下降 |
| 节点磁盘使用率 | `node_filesystem_avail_bytes` | 磁盘使用率保持在 85% 以下 | 持续上升并再次接近阈值 |
| kubelet 运行中 Pod 数 | `kubelet_running_pods` | Pod 数量恢复到问题前水平 | 持续为 0 或远低于预期 |
| PLEG 延迟 | `kubelet_pleg_relist_duration_seconds` | P99 < 10s | P99 > 60s 或 relist 超时 |
| 容器重启次数 | `kube_pod_container_status_restarts_total` | 无异常增长 | 修复后持续增加 |

### 6.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] 节点 STATUS 显示 Ready，且持续 Ready 超过 5 分钟
- [ ] 所有 Conditions（MemoryPressure, DiskPressure, PIDPressure）均为 False
- [ ] Node Lease 正常续租（renewTime 持续更新）
- [ ] 节点上的 Pod 已恢复正常运行（Running 状态）
- [ ] kubelet 和 containerd 进程稳定运行（无崩溃重启）
- [ ] 节点系统资源（CPU、内存、磁盘、PID）处于安全水位
- [ ] 无新增 Warning 事件
- [ ] 根因已明确记录并采取了预防措施（如需要）

### 6.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|----------|
| 节点状态稳定性 | `kube_node_status_condition{condition="Ready"}` 监控 | 持续 | 再次 NotReady → 重新进入诊断流程 |
| 磁盘使用趋势 | `node_filesystem_avail_bytes` 趋势图 | 每小时 | 线性增长 → 排查磁盘空间消耗源头 |
| 内存使用趋势 | `node_memory_MemAvailable_bytes` 趋势图 | 每小时 | 线性下降 → 排查内存泄漏 Pod |
| kubelet 重启次数 | systemd service 重启计数 | 每 4 小时 | 24h 内重启 >2 次 → 深度排查 |
| OOM 事件 | `dmesg | grep -i oom` | 每 4 小时 | 新的 OOM → 检查内存限制配置 |
| 证书有效期 | `openssl x509 -noout -enddate` | 每日 | 有效期 <7 天 → 预防性轮转 |

---

## 7. 升级协议

### 7.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|----------|
| **诊断超时** | 诊断工作流执行超过 **10 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后 V1-V5 验证失败 |
| **严重性升级** | 初始分级为 P2 但影响面扩大 | NotReady 节点数增加 |
| **未知根因** | 完成所有诊断步骤但无法匹配 RC-001~RC-015 | 所有步骤均无明确异常 |
| **权限不足** | 无 SSH 访问权限，无法执行 Phase 2+ | Phase 1 完成后需要 SSH 但无权限 |
| **安全疑虑** | 发现可疑安全指标（异常进程、未知网络连接） | 任何诊断步骤中发现安全异常 |

### 7.2 升级消息模板

```
【{severity}】节点 NotReady 诊断与修复 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: 节点 {node_name} ({node_ip}) 状态为 NotReady，持续 {duration}
- 影响范围: 
  - 受影响节点: {affected_node_count}/{total_node_count}
  - 受影响 Pod: {affected_pod_count} 个（namespace: {affected_namespaces}）
  - 是否涉及控制平面: {control_plane_affected}
- 已完成诊断:
  - Phase 1 快速检查: {phase1_summary}
  - Phase 2 深度检查: {phase2_summary}
  - Phase 3 主动探测: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

### 7.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤及每步输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
4. **关键资源快照**:
   ```bash
   kubectl describe node <node-name> > node-describe.txt
   kubectl get events --field-selector involvedObject.name=<node-name> --sort-by=.lastTimestamp > node-events.txt
   kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces -o wide > node-pods.txt
   ssh <node-ip> "journalctl -u kubelet --since '1 hour ago' --no-pager" > kubelet-logs.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件按时间排列

---

## 8. 生产案例速查

| 案例 | 根因 | 修复 | 教训 |
|------|------|------|------|
| 多节点同时 NotReady | 交换机固件升级导致网络中断 | 恢复网络配置 | 变更需通知 K8s 运维 |
| 节点反复 NotReady | kubelet 证书自动轮转 CSR 未批准 | 配置 CSR 自动批准 | 监控 Pending CSR |
| 磁盘满导致 NotReady | 容器日志未配置轮转，单 Pod 日志 50GB | 清理日志 + 配置 logMaxSize | 必须配置日志轮转 |
| 节点时好时坏 | NTP 服务停止，时钟偏差 >2min | 修复 NTP + 重启 kubelet | 监控时间同步 |
| PLEG 不健康 | NFS 挂载卡住导致 D 状态进程 | 修复 NFS + 重启 containerd | 避免在 Pod 中使用不稳定 NFS |

---

## 9. 版本升级 SOP 与兼容性注意事项

> 详细版本差异请参考 [reference/node-version-differences.md](reference/node-version-differences.md)

### 版本升级关键检查清单

| 升级路径 | 关键检查项 | 风险等级 |
|---------|-----------|----------|
| 1.28 → 1.30 | NodeSwap 默认启用，检查 swap 配置；NodeLogQuery 升级为 Beta | 🟡 中 |
| 1.30 → 1.32 | KubeletCrashLoopBackOffMax 引入；RuntimeHandlers API 可用 | 🟢 低 |
| 1.32 → 1.34 | NodeMonitorGracePeriod 40s→50s；NodeSwap GA；WindowsGracefulNodeShutdown Beta | 🟡 中 |
| 1.34 → 1.36 | NodeLogQuery GA；UserNamespacesSupport GA；NodeDeclaredFeatures Beta；InPlacePodVerticalScaling GA | 🟡 中 |

### 版本特定故障模式

| 版本 | 特有故障模式 | 诊断要点 |
|------|------------|----------|
| 1.30+ | Swap 相关 OOM 行为变化 | 检查 `memorySwap.swapBehavior` 配置 |
| 1.32+ | CrashLoopBackOff 退避策略变化 | 检查 `crashLoopBackOffMax` 配置 |
| 1.34+ | NotReady 判定延迟增加 10s | 调整监控告警阈值 |
| 1.36+ | 节点特性声明不匹配 | 检查 `declaredFeatures` 与实际能力 |

### 升级前诊断检查

```bash
# 🟢 检查当前集群版本
kubectl version --short 2>/dev/null || kubectl version

# 🟢 检查节点 kubelet 版本一致性
kubectl get nodes -o custom-columns=NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion

# 🟢 检查节点 Feature Gate 配置
cat /var/lib/kubelet/config.yaml | grep -A 20 "featureGates"

# 🟢 检查节点当前 Taint（升级前基线）
kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints[*].key
```

---

## 9. 自动化诊断工具

> 来源：`故障诊断/核心排障/06-node-notready-diagnosis.md` §10

### 9.1 Node NotReady 完整诊断脚本

```bash
#!/bin/bash
# node-notready-full-diagnosis.sh - Node NotReady 完整诊断
# 版本: 1.0 | 适用: Kubernetes v1.25-v1.32
# 🟢 低风险：只读/信息收集

set -e
NODE_NAME=${1:-$(hostname)}
OUTPUT_FILE="node-diagnosis-$(date +%Y%m%d-%H%M%S).txt"

log() { echo -e "\033[0;32m[INFO]\033[0m $1"; }
warn() { echo -e "\033[1;33m[WARN]\033[0m $1"; }
error() { echo -e "\033[0;31m[ERROR]\033[0m $1"; }

diagnose() {
    echo "=== 1. kubelet 状态 ==="
    systemctl status kubelet --no-pager 2>/dev/null | head -10
    KUBELET_STATUS=$(systemctl is-active kubelet 2>/dev/null)
    [ "$KUBELET_STATUS" != "active" ] && error "kubelet 未运行!" && journalctl -u kubelet -n 20 --no-pager

    echo "=== 2. containerd 状态 ==="
    systemctl status containerd --no-pager 2>/dev/null | head -10
    CONTAINERD_STATUS=$(systemctl is-active containerd 2>/dev/null)
    [ "$CONTAINERD_STATUS" != "active" ] && error "containerd 未运行!" || timeout 10 crictl info 2>&1 | head -10

    echo "=== 3. 系统资源 ==="
    free -h
    MEM_AVAIL=$(free -m | awk '/^Mem:/{print $7}')
    [ $MEM_AVAIL -lt 500 ] && warn "可用内存低于 500MB!"
    df -h | grep -E "^/dev|Filesystem"
    DISK_USE=$(df / | tail -1 | awk '{print $5}' | tr -d '%')
    [ $DISK_USE -gt 85 ] && warn "根分区使用率超过 85%!"

    echo "=== 4. 网络连通性 ==="
    API_SERVER=$(grep "server:" /etc/kubernetes/kubelet.conf 2>/dev/null | awk '{print $2}')
    if [ -n "$API_SERVER" ]; then
        curl -sk --connect-timeout 5 "${API_SERVER}/healthz" &>/dev/null && log "API Server 可达" || error "API Server 不可达!"
    fi

    echo "=== 5. 证书状态 ==="
    CERT="/var/lib/kubelet/pki/kubelet-client-current.pem"
    if [ -f "$CERT" ]; then
        EXPIRY=$(openssl x509 -in "$CERT" -noout -enddate 2>/dev/null | cut -d= -f2)
        echo "证书过期时间: $EXPIRY"
    fi

    echo "=== 6. 错误日志汇总 ==="
    journalctl -u kubelet --since "30 minutes ago" --no-pager 2>/dev/null | grep -iE "error|fail" | tail -10
    dmesg | grep -iE "error|fail|panic|oom" | tail -10
}

diagnose 2>&1 | tee "$OUTPUT_FILE"
log "诊断报告已保存: $OUTPUT_FILE"
```

### 9.2 内核与 cgroup 诊断脚本

```bash
#!/bin/bash
# kernel-cgroup-diagnosis.sh
# 🟢 低风险：只读

echo "=== 内核参数 ==="
sysctl net.ipv4.ip_forward
sysctl net.bridge.bridge-nf-call-iptables 2>/dev/null
sysctl net.netfilter.nf_conntrack_max 2>/dev/null
sysctl fs.inotify.max_user_watches 2>/dev/null
sysctl fs.inotify.max_user_instances 2>/dev/null

echo "=== 文件描述符 ==="
echo "当前打开: $(cat /proc/sys/fs/file-nr | awk '{print $1}')"
echo "最大限制: $(cat /proc/sys/fs/file-max)"

echo "=== 进程限制 ==="
echo "当前进程数: $(ps aux | wc -l)"
echo "PID 最大值: $(cat /proc/sys/kernel/pid_max)"

echo "=== cgroup 诊断 ==="
if [ -f /sys/fs/cgroup/cgroup.controllers ]; then
    echo "cgroup 版本: v2"
    [ -d /sys/fs/cgroup/kubepods ] && cat /sys/fs/cgroup/kubepods/memory.current 2>/dev/null
else
    echo "cgroup 版本: v1"
    [ -d /sys/fs/cgroup/memory/kubepods ] && numfmt --to=iec < /sys/fs/cgroup/memory/kubepods/memory.usage_in_bytes 2>/dev/null
fi

echo "=== cgroup 驱动一致性 ==="
echo "kubelet: $(grep cgroupDriver /var/lib/kubelet/config.yaml 2>/dev/null)"
echo "containerd: $(grep -A 5 'SystemdCgroup' /etc/containerd/config.toml 2>/dev/null)"
```

---

## 10. ACK/云环境特定诊断

> 来源：`故障诊断/核心排障/06-node-notready-diagnosis.md` §9

### 10.1 ACK 节点诊断脚本

```bash
#!/bin/bash
# ack-node-diagnosis.sh - 阿里云 ACK 节点诊断
# 🟢 低风险：只读

echo "=== 1. 节点信息 ==="
echo "节点 ID: $(curl -s http://100.100.100.200/latest/meta-data/instance-id 2>/dev/null)"
echo "实例类型: $(curl -s http://100.100.100.200/latest/meta-data/instance-type 2>/dev/null)"
echo "可用区: $(curl -s http://100.100.100.200/latest/meta-data/zone-id 2>/dev/null)"

echo "=== 2. 节点池信息 ==="
kubectl get node $(hostname) -o jsonpath='{.metadata.labels.alibabacloud\.com/nodepool-id}' 2>/dev/null

echo "=== 3. 竞价实例检查 ==="
SPOT=$(kubectl get node $(hostname) -o jsonpath='{.metadata.labels.alibabacloud\.com/spot-instance}' 2>/dev/null)
[ "$SPOT" = "true" ] && echo "⚠️ 这是竞价实例，可能被回收" || echo "按量/包年包月实例"

echo "=== 4. Terway 网络检查 ==="
if [ -f /etc/cni/net.d/10-terway.conf ]; then
    echo "Terway CNI 已配置"
    ip addr show | grep -E "^[0-9]+: eth" | head -5
fi

echo "=== 5. GPU 检查 ==="
command -v nvidia-smi &>/dev/null && nvidia-smi --query-gpu=name,memory.total,memory.used --format=csv || echo "未检测到 GPU"
```

### 10.2 ACK 特定问题速查表

| 问题 | 症状 | 原因 | 解决方案 |
|------|------|------|----------|
| 节点自动回收 | 节点突然消失 | 竞价实例被回收 | 使用混合实例策略 |
| 节点池扩容失败 | 节点数不增加 | 库存不足/配额限制 | 检查配额/更换规格 |
| Terway 网络问题 | Pod 网络不通 | ENI 分配失败 | 检查 VSwitch/安全组 |
| 云盘挂载失败 | PVC Pending | 云盘不在同可用区 | 使用 WaitForFirstConsumer |
| 节点标签丢失 | 节点池标签不生效 | 节点池配置问题 | 重新同步节点池配置 |

### 10.3 ACK 常用运维命令

```bash
# 🟢 查看节点池分布
kubectl get nodes -o custom-columns='NAME:.metadata.name,POOL:.metadata.labels.alibabacloud\.com/nodepool-id'

# 🟡 节点排水
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 🔴 移除节点
kubectl delete node <node-name>

# ACK 诊断工具
# curl -O https://alibabacloud-china.github.io/diagnose-tools/scripts/installer.sh
# bash installer.sh
# ack-diagnose node --cluster-id <cluster-id> --node-name <node-name>
```

---

## 11. 监控告警配置

> 来源：`故障诊断/核心排障/06-node-notready-diagnosis.md` §11

### Prometheus 告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: node-notready-alerts
  namespace: monitoring
spec:
  groups:
  - name: node.status
    interval: 30s
    rules:
    - alert: NodeNotReady
      expr: kube_node_status_condition{condition="Ready",status="true"} == 0
      for: 2m
      labels:
        severity: critical
        category: infrastructure
      annotations:
        summary: "节点 {{ $labels.node }} NotReady"
        description: "节点已 NotReady 超过 2 分钟"
    - alert: NodeUnknown
      expr: kube_node_status_condition{condition="Ready",status="unknown"} == 1
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "节点 {{ $labels.node }} 状态 Unknown"
    - alert: NodeMemoryPressure
      expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.node }} 内存压力"
    - alert: NodeDiskPressure
      expr: kube_node_status_condition{condition="DiskPressure",status="true"} == 1
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.node }} 磁盘压力"
```

### kubelet 核心监控指标

| 指标 | 含义 | 风险点 |
|------|------|--------|
| `kubelet_pleg_relist_duration_seconds` | PLEG 周期耗时 | 持续 > 1s 表示运行时压力大 |
| `kubelet_node_config_error` | 配置错误计数 | > 0 表示配置未生效 |
| `kubelet_runtime_operations_errors_total` | CRI 操作错误数 | 增长表示运行时异常 |
| `kubelet_certificate_expiration_seconds` | 证书剩余有效期 | < 7天需紧急处理 |
| `kubelet_evictions` | 驱逐次数 | 突增表示资源压力 |

---

## 相关链接

- [[技能/故障诊断-节点/node/README.md|Node 异常诊断技能集]]
- [[技能/故障诊断-节点/node/01-node-notready-diagnosis.md|Node NotReady 诊断]]
- [[技能/故障诊断-节点/node/02-node-resource-pressure.md|节点资源压力诊断]]
- [[技能/故障诊断-节点/node/03-node-component-troubleshooting.md|节点组件故障排查]]
- [[技能/故障诊断-节点/node/05-gpu-device-plugin-troubleshooting.md|GPU 与设备插件故障排查]]
- [[技能/故障诊断-节点/node/reference/node-version-differences.md|版本差异对比]]
- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[故障诊断/核心排障/06-node-notready-diagnosis.md|Node NotReady 深度诊断（原始文件）]]
