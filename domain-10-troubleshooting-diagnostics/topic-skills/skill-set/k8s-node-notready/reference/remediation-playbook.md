---
title: "node notready Remediation Playbook"
category: remediation
skill_set: "k8s-node-notready"
created: "2026-05-22"
updated: "2026-05-22"
tags: ["reference", "remediation", "playbook", "visibility/public"]
---

---
title: 修复操作手册 / Remediation Playbook
description: '- [REM-002 清理磁盘空间（容器镜像和日志）](#rem-002-清理磁盘空间容器镜像和日志)'
category: skills
tags:
- k8s
- skills
- sop
- runbook
- [[kubelet|kubelet]]
- [[containerd|containerd]]
- pdb
- [[StatefulSet|statefulset]]
- daemonset
- rag
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 10min
intent_queries:
- 修复操作手册 / Remediation Playbook 是什么
- 如何 修复操作手册 / Remediation Playbook
trigger_keywords:
- 修复操作手册
- Remediation
- Playbook
- skills
prerequisites:
- kubectl-basics
- troubleshooting-methodology
skill_id: SKILL-REMEDIATION_PLAYBOOK-001
skill_name: 修复操作手册 / Remediation Playbook
version: 1.0.0
---

# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-NODE-001 v1.0 — 节点 NotReady 诊断与修复
> **本文档**: 提取自 Section 6（修复操作）、Section 7（验证确认）和 Section 8（升级协议）

---

## 目录

- [风险级别说明](#风险级别说明)
- [修复操作](#修复操作)
  - [🟢 低风险 — Agent 可建议自动执行](#-低风险--agent-可建议自动执行)
    - [REM-001 取消节点 cordon 标记（Uncordon）](#rem-001-取消节点-cordon-标记uncordon)
    - [REM-002 清理磁盘空间（容器镜像和日志）](#rem-002-清理磁盘空间容器镜像和日志)
  - [🟡 中风险 — Agent 建议，人工审批后执行](#-中风险--agent-建议人工审批后执行)
    - [REM-003 重启 kubelet 服务](#rem-003-重启-kubelet-服务)
    - [REM-004 重启 containerd 服务](#rem-004-重启-containerd-服务)
    - [REM-005 调整 kubelet 驱逐阈值](#rem-005-调整-kubelet-驱逐阈值)
  - [🔴 高风险 — Agent 仅提供指导，人工执行](#-高风险--agent-仅提供指导人工执行)
    - [REM-006 排空节点并重启](#rem-006-排空节点并重启)
    - [REM-007 替换节点（云环境）](#rem-007-替换节点云环境)
    - [REM-008 手动证书轮转](#rem-008-手动证书轮转)
  - [⚫ 严重 — 需高级 SRE 审批](#-严重--需高级-sre-审批)
    - [REM-009 内核热补丁或 OS 升级](#rem-009-内核热补丁或-os-升级)
    - [REM-010 硬件更换](#rem-010-硬件更换)
- [验证确认](#验证确认)
  - [V1-V5 即时验证](#71-即时验证修复后-1-2-分钟内)
  - [短期监控（5-30 分钟）](#72-短期监控5-30-分钟)
  - [解决确认标准](#73-解决确认标准)
  - [24 小时回归检测](#74-回归检测24-小时内关注)
- [升级协议](#升级协议)
  - [自动升级条件](#81-自动升级条件)
  - [升级消息模板](#82-升级消息模板)
  - [交接信息包](#83-交接信息包)

---

## 风险级别说明

| 风险级别 | 标识 | 含义 | Agent 行为 |
|---------|------|------|-----------|
| 低风险 | 🟢 | 只读操作或影响极小的修改，基本不会造成业务中断 | 可建议自动执行 |
| 中风险 | 🟡 | 可能导致短暂服务中断（秒级到分钟级），影响范围限于单节点 | 建议操作并等待人工审批后执行 |
| 高风险 | 🔴 | 将导致节点上工作负载中断（分钟级），可能影响 Pod 调度和数据 | 仅提供操作指导，由人工执行 |
| 严重 | ⚫ | 涉及 OS/内核级变更或硬件操作，不可逆性高 | 仅提供建议，需高级 SRE + Team Lead 审批 |

---

## 修复操作

### 🟢 低风险 — Agent 可建议自动执行

#### REM-001: 取消节点 cordon 标记（Uncordon）

- **适用根因**: RC-012
- **前置检查**:
  ```bash
  # 确认节点确实处于 SchedulingDisabled 状态且非维护状态
  kubectl get node <node-name> -o jsonpath='{.spec.unschedulable}'
  # 预期: true
  # 检查是否有维护 annotation
  kubectl get node <node-name> -o jsonpath='{.metadata.annotations.maintenance\.scheduled}'
  # 预期: 无输出（无维护标记）
  ```
- **执行命令**:
  ```bash
  kubectl uncordon <node-name>
  ```
- **后置验证**:
  ```bash
  kubectl get node <node-name>
  # 预期: STATUS 列显示 Ready（不包含 SchedulingDisabled）
  ```
- **回滚命令**:
  ```bash
  kubectl cordon <node-name>
  ```

---

#### REM-002: 清理磁盘空间（容器镜像和日志）

- **适用根因**: RC-003
- **前置检查**:
  ```bash
  # 确认磁盘确实紧张
  ssh <node-ip> "df -h / /var/lib/containerd /var/lib/kubelet /var/log"
  # 确认使用率 > 85%
  ```
- **执行命令**:
  ```bash
  # Step 1: 清理已退出的容器
  ssh <node-ip> "crictl rmi --prune"

  # Step 2: 清理未使用的容器镜像（仅清理无运行容器引用的镜像）
  ssh <node-ip> "crictl rmi --prune"

  # Step 3: 清理旧的日志文件（仅清理已归档的日志）
  ssh <node-ip> "find /var/log -name '*.gz' -mtime +7 -delete 2>/dev/null; \
    find /var/log -name '*.old' -mtime +3 -delete 2>/dev/null"

  # Step 4: 清理 journal 日志（保留最近 2 天）
  ssh <node-ip> "journalctl --vacuum-time=2d"

  # Step 5: 手动触发容器 GC（可选，kubelet 会自动执行）
  # kubelet 的 imageGCHighThresholdPercent 默认 85%
  ```
- **后置验证**:
  ```bash
  ssh <node-ip> "df -h / /var/lib/containerd /var/lib/kubelet /var/log"
  # 预期: 使用率下降到 85% 以下
  kubectl get node <node-name> -o jsonpath='{.status.conditions[?(@.type=="DiskPressure")].status}'
  # 预期: False（可能需要等 1-2 分钟 kubelet 重新评估）
  ```
- **回滚命令**:
  ```bash
  # 磁盘清理为不可逆操作，但删除的仅是缓存/日志，不影响服务
  # 如需恢复镜像，kubelet 会在调度 Pod 时自动拉取
  ```

---

### 🟡 中风险 — Agent 建议，人工审批后执行

#### REM-003: 重启 kubelet 服务

- **适用根因**: RC-001, RC-008
- **影响说明**: 重启 kubelet 会导致节点上所有 Pod 短暂中断健康检查上报，正在运行的容器不会被终止（除非 kubelet 启动后发现不一致需要重建）。重启过程中节点无法接受新的 Pod 调度。
- **审批提示**: "建议重启节点 `<node-name>` 上的 kubelet 服务。该操作不会终止正在运行的容器，但节点在重启期间（约 10-30s）无法调度新 Pod。是否批准？"
- **前置检查**:
  ```bash
  # 确认 kubelet 确实异常
  ssh <node-ip> "systemctl status kubelet"
  # 预期: inactive/failed/activating 或 active 但日志有错误

  # 记录当前运行的 Pod 列表（用于后续对比）
  kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces -o wide > /tmp/pods-before-restart.txt
  ```
- **执行命令**:
  ```bash
  ssh <node-ip> "systemctl restart kubelet"
  ```
- **后置验证**:
  ```bash
  # 等待 30 秒后检查
  sleep 30

  # 检查 kubelet 状态
  ssh <node-ip> "systemctl status kubelet"
  # 预期: Active: active (running)

  # 检查节点状态
  kubectl get node <node-name>
  # 预期: STATUS 列显示 Ready

  # 检查 Conditions
  kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
  # 预期: Ready=True, 所有压力条件为 False
  ```
- **回滚命令**:
  ```bash
  # kubelet 重启为幂等操作，无需回滚
  # 如果重启后问题恶化，可再次停止 kubelet 并升级
  ssh <node-ip> "systemctl stop kubelet"
  # 注意: 停止 kubelet 后节点将确定变为 NotReady
  ```

---

#### REM-004: 重启 containerd 服务

- **适用根因**: RC-002, RC-008
- **影响说明**: 重启 containerd **会导致节点上所有容器短暂中断**。containerd 重启后会重新 recover 所有已有的 shim 进程，大多数容器会恢复运行。但如果容器的 restart policy 触发，可能导致部分 Pod 重启。这是比重启 kubelet 更具侵入性的操作。
- **审批提示**: "建议重启节点 `<node-name>` 上的 containerd 服务。**该操作会导致该节点上所有容器短暂中断（约 30-60s）**，大部分容器会自动恢复。请确认该节点上的工作负载可以承受短暂中断。是否批准？"
- **前置检查**:
  ```bash
  # 确认 containerd 确实异常
  ssh <node-ip> "systemctl status containerd"

  # 记录当前容器列表
  ssh <node-ip> "crictl ps -a" > /tmp/containers-before-restart.txt 2>/dev/null

  # 检查该节点上是否有 stateful 工作负载
  kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name} ownerKind={.metadata.ownerReferences[0].kind}{"\n"}{end}' | grep -i statefulset
  # 如果有 StatefulSet Pod，需要额外谨慎
  ```
- **执行命令**:
  ```bash
  ssh <node-ip> "systemctl restart containerd"

  # 等待 containerd 完全恢复
  sleep 10

  # 重启 kubelet 以确保重新同步
  ssh <node-ip> "systemctl restart kubelet"
  ```
- **后置验证**:
  ```bash
  # 等待 60 秒后检查
  sleep 60

  # 检查 containerd 状态
  ssh <node-ip> "systemctl status containerd"
  # 预期: Active: active (running)

  # 检查 kubelet 状态
  ssh <node-ip> "systemctl status kubelet"
  # 预期: Active: active (running)

  # 检查节点状态
  kubectl get node <node-name>
  # 预期: STATUS 列显示 Ready

  # 检查 Pod 状态
  kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces
  # 预期: 所有 Pod 恢复 Running 状态
  ```
- **回滚命令**:
  ```bash
  # containerd 重启为幂等操作
  # 如果重启后问题未解决，不要反复重启，应升级处理
  ```

---

#### REM-005: 调整 kubelet 驱逐阈值

- **适用根因**: RC-003, RC-004, RC-005
- **影响说明**: 修改 kubelet 驱逐阈值配置。需要重启 kubelet 生效。降低驱逐阈值可以暂时缓解资源压力导致的 NotReady，但需要同时解决资源问题的根本原因。
- **审批提示**: "建议调整节点 `<node-name>` 上的 kubelet 驱逐阈值以暂时缓解资源压力。修改后需重启 kubelet，节点将短暂不可用。是否批准？"
- **前置检查**:
  ```bash
  # 查看当前 kubelet 配置的驱逐阈值
  ssh <node-ip> "cat /var/lib/kubelet/config.yaml | grep -A 10 evictionHard"
  # 默认值:
  # evictionHard:
  #   imagefs.available: 15%
  #   memory.available: 100Mi
  #   nodefs.available: 10%
  #   nodefs.inodesFree: 5%
  #   pid.available: -1
  ```
- **执行命令**:
  ```bash
  # 备份现有配置
  ssh <node-ip> "cp /var/lib/kubelet/config.yaml /var/lib/kubelet/config.yaml.bak"

  # 根据具体资源压力类型调整阈值（示例：降低磁盘阈值）
  # 注意: 这只是临时缓解，必须同步清理磁盘或扩容
  ssh <node-ip> "sed -i 's/imagefs.available: 15%/imagefs.available: 10%/' /var/lib/kubelet/config.yaml"

  # 重启 kubelet 使配置生效
  ssh <node-ip> "systemctl restart kubelet"
  ```
- **后置验证**:
  ```bash
  sleep 30
  kubectl get node <node-name>
  # 预期: STATUS 列显示 Ready

  kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
  # 预期: 压力条件恢复为 False
  ```
- **回滚命令**:
  ```bash
  # 恢复原始配置
  ssh <node-ip> "cp /var/lib/kubelet/config.yaml.bak /var/lib/kubelet/config.yaml && systemctl restart kubelet"
  ```

---

### 🔴 高风险 — Agent 仅提供指导，人工执行

#### REM-006: 排空节点并重启

- **适用根因**: RC-001, RC-002, RC-008, RC-009
- **影响说明**: 排空（drain）节点将驱逐所有非 DaemonSet Pod，这些 Pod 将被重新调度到其他节点。重启操作会导致该节点上的所有工作负载中断。如果集群资源紧张，被驱逐的 Pod 可能无法被调度到其他节点。
- **操作步骤**:
  1. **确认集群有足够资源接纳被驱逐的 Pod**:
     ```bash
     kubectl top nodes
     # 确认其他节点有足够 CPU 和内存余量
     ```
  2. **排空节点**:
     ```bash
     kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force --grace-period=60 --timeout=300s
     ```
  3. **等待 Pod 完成迁移**:
     ```bash
     kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces
     # 预期: 仅剩 DaemonSet Pod
     ```
  4. **重启节点**:
     ```bash
     ssh <node-ip> "reboot"
     ```
  5. **等待节点恢复**（约 2-5 分钟）:
     ```bash
     # 持续检查节点状态
     watch kubectl get node <node-name>
     ```
  6. **取消 cordon 标记**:
     ```bash
     kubectl uncordon <node-name>
     ```
- **安全检查**:
  - 确认 PodDisruptionBudget (PDB) 不会阻止 drain（`kubectl get pdb --all-namespaces`）
  - 确认无 local storage 的有状态工作负载（emptyDir 数据会丢失）
  - 确认集群其他节点可承载被迁移的 Pod
- **回滚方案**:
  ```bash
  # 如果 drain 过程中需要中止
  # Ctrl+C 中断 drain 命令，然后 uncordon
  kubectl uncordon <node-name>
  # 注意: 已经被驱逐的 Pod 不会自动回到原节点
  ```

---

#### REM-007: 替换节点（云环境）

- **适用根因**: RC-009, RC-001（反复发生且无法修复时）
- **影响说明**: 在云环境中，直接终止问题节点实例并创建新实例加入集群。这要求集群使用了 node autoscaler 或有手动添加节点的运维流程。
- **操作步骤**:
  1. **排空节点**（同 REM-006 步骤 1-3）
  2. **从集群中删除节点对象**:
     ```bash
     kubectl delete node <node-name>
     ```
  3. **在云平台终止实例**（具体命令取决于云平台）:
     ```bash
     # AWS 示例
     aws ec2 terminate-instances --instance-ids <instance-id>
     # 或通过 Node Group / ASG 管理
     ```
  4. **创建新节点**:
     ```bash
     # 如果使用 Cluster Autoscaler，新节点会自动创建
     # 如果手动管理，按集群 join 流程添加新节点
     ```
  5. **验证新节点加入**:
     ```bash
     kubectl get nodes -w
     ```
- **安全检查**:
  - 确认节点上没有 local PV（本地持久化数据会丢失）
  - 确认 node group / ASG 的容量限制允许替换
  - 通知相关 team 即将执行节点替换
- **回滚方案**:
  - 节点替换后无法回滚到原实例
  - 需确保数据已通过其他方式备份（PV、远程存储等）

---

#### REM-008: 手动证书轮转

- **适用根因**: RC-007
- **影响说明**: 手动批准或重新生成 kubelet 证书。如果自动轮转机制失败，需要手动干预。操作不当可能导致节点永久失联。
- **操作步骤**:
  1. **检查待批准的 CSR**:
     ```bash
     kubectl get csr | grep -i pending
     ```
  2. **如有待批准的 CSR，手动批准**:
     ```bash
     kubectl certificate approve <csr-name>
     ```
  3. **如果无 CSR 或证书已过期，需要重新 bootstrap**:
     ```bash
     # 在节点上删除旧证书
     ssh <node-ip> "rm -f /var/lib/kubelet/pki/kubelet-client-current.pem"

     # 确保 bootstrap token 有效
     kubeadm token list
     # 如无有效 token，创建新 token
     kubeadm token create

     # 重启 kubelet 触发重新 bootstrap
     ssh <node-ip> "systemctl restart kubelet"
     ```
  4. **批准新的 CSR**:
     ```bash
     # 等待新的 CSR 出现
     kubectl get csr --watch
     # 批准
     kubectl certificate approve <new-csr-name>
     ```
- **安全检查**:
  - 确认 CSR 请求来源确实是目标节点（检查 CSR 的 requestor 和 subject）
  - 确认 bootstrap token 的有效期和权限范围
- **回滚方案**:
  ```bash
  # 如果手动轮转导致问题恶化，恢复旧证书（如果有备份）
  ssh <node-ip> "cp /var/lib/kubelet/pki/kubelet-client-current.pem.bak /var/lib/kubelet/pki/kubelet-client-current.pem && systemctl restart kubelet"
  ```

---

### ⚫ 严重 — 需高级 SRE 审批

#### REM-009: 内核热补丁或 OS 升级

- **适用根因**: RC-009
- **审批要求**: 需要高级 SRE + 基础设施 Team Lead 审批
- **数据备份**: 升级前确保节点上无 local PV 数据，或已完成数据备份
- **操作步骤**:
  1. **排空节点**（同 REM-006）
  2. **评估内核问题**:
     ```bash
     ssh <node-ip> "uname -r"
     ssh <node-ip> "dmesg -T | grep -i 'bug\|error\|panic\|oops'"
     ```
  3. **应用内核补丁**（具体取决于 OS 发行版）:
     ```bash
     # RHEL/CentOS
     ssh <node-ip> "yum update kernel -y"

     # Ubuntu
     ssh <node-ip> "apt-get update && apt-get install linux-image-generic -y"
     ```
  4. **重启节点以应用新内核**:
     ```bash
     ssh <node-ip> "reboot"
     ```
  5. **等待节点恢复并验证**
- **回滚方案**:
  - 大多数 Linux 发行版支持在 GRUB 中选择旧内核启动
  - 如果新内核导致问题，通过 IPMI/iLO/云控制台重启到旧内核
  ```bash
  # 查看可用内核列表
  ssh <node-ip> "grep menuentry /boot/grub2/grub.cfg"
  # 设置默认启动为旧内核
  ssh <node-ip> "grub2-set-default 1 && reboot"
  ```

---

#### REM-010: 硬件更换

- **适用根因**: RC-009
- **审批要求**: 需要高级 SRE + 数据中心运维 Team 审批
- **数据备份**: 确认所有需要保留的数据已备份到外部存储
- **操作步骤**:
  1. **排空节点并从集群中移除**（同 REM-007 步骤 1-2）
  2. **提交数据中心硬件更换工单**:
     - 记录问题硬件信息（服务器型号、序列号、问题组件）
     - 附上 dmesg 和硬件诊断日志
  3. **硬件更换完成后**:
     - 重新安装 OS 和 K8s 组件
     - 按集群 join 流程重新加入节点
  4. **验证新硬件**:
     ```bash
     # 运行硬件诊断
     ssh <new-node-ip> "smartctl -a /dev/sda"  # 磁盘健康
     ssh <new-node-ip> "mcelog --client"        # CPU/内存错误
     ```
- **回滚方案**:
  - 硬件更换为不可逆操作
  - 保留问题硬件的日志和诊断信息用于事后分析

---

## 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

```bash
# V1: 确认节点状态恢复为 Ready
kubectl get node <node-name>
# 预期: STATUS 列显示 Ready

# V2: 确认所有 Conditions 恢复正常
kubectl get node <node-name> -o jsonpath='{range .status.conditions[*]}{.type}={.status}{"\n"}{end}'
# 预期输出:
# MemoryPressure=False
# DiskPressure=False
# PIDPressure=False
# Ready=True

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

---

### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| 节点 CPU 使用率 | `node_cpu_seconds_total` 或 `kubectl top node <node-name>` | 恢复后 CPU 使用率稳定在正常范围 | CPU 使用率持续 >90% 超过 5 分钟 |
| 节点内存使用率 | `node_memory_MemAvailable_bytes` 或 `kubectl top node <node-name>` | 可用内存保持在驱逐阈值以上 | 可用内存 <200Mi 且持续下降 |
| 节点磁盘使用率 | `node_filesystem_avail_bytes` 或 SSH `df -h` | 磁盘使用率保持在 85% 以下 | 磁盘使用率持续上升并再次接近阈值 |
| kubelet 运行中 Pod 数 | `kubelet_running_pods` | Pod 数量恢复到问题前水平 | Pod 数量持续为 0 或远低于预期 |
| kubelet 心跳 | `kube_node_status_condition{condition="Ready",status="true"}` | 持续为 1 | 值变为 0（节点再次 NotReady） |
| PLEG 延迟 | `kubelet_pleg_relist_duration_seconds` | P99 < 10s | P99 > 60s 或 relist 超时 |
| 容器重启次数 | `kube_pod_container_status_restarts_total` | 无异常增长 | 修复后容器重启次数持续增加 |
| 节点事件 | `kubectl get events --field-selector involvedObject.name=<node-name>` | 无新的 Warning 事件 | 出现新的 NodeNotReady 或资源压力事件 |

---

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] 节点 STATUS 显示 Ready，且持续 Ready 超过 5 分钟
- [ ] 所有 Conditions（MemoryPressure, DiskPressure, PIDPressure）均为 False
- [ ] Node Lease 正常续租（renewTime 持续更新）
- [ ] 节点上的 Pod 已恢复正常运行（Running 状态）
- [ ] kubelet 和 containerd 进程稳定运行（无崩溃重启）
- [ ] 节点系统资源（CPU、内存、磁盘、PID）处于安全水位
- [ ] 无新增 Warning 事件
- [ ] 根因已明确记录并采取了预防措施（如需要）

---

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| 节点状态稳定性 | `kube_node_status_condition{condition="Ready"}` 监控 | 持续 | 如果再次 NotReady → 重新进入本 Skill 诊断流程 |
| 磁盘使用趋势 | `node_filesystem_avail_bytes` 趋势图 | 每小时 | 使用率线性增长 → 排查磁盘空间消耗源头（日志、镜像缓存） |
| 内存使用趋势 | `node_memory_MemAvailable_bytes` 趋势图 | 每小时 | 可用内存线性下降 → 排查内存泄漏 Pod |
| kubelet 重启次数 | `kubelet` systemd service 重启计数 | 每 4 小时 | 24h 内重启 >2 次 → 深度排查 kubelet 崩溃原因 |
| OOM 事件 | `dmesg | grep -i oom` | 每 4 小时 | 新的 OOM 事件 → 检查内存限制配置 |
| 证书有效期 | `openssl x509 ... -noout -enddate` | 每日 | 有效期 <7 天 → 预防性轮转或检查自动轮转机制 |
| 节点上 Pod 调度 | `kubectl get pods --field-selector spec.nodeName=<node-name>` | 每 4 小时 | 新 Pod 无法调度到该节点 → 检查 taints 和 node conditions |

---

## 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **诊断超时** | 诊断工作流执行超过 **10 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后 V1-V5 验证失败 |
| **严重性升级** | 初始分级为 P2 但影响面扩大（如更多节点变为 NotReady） | 诊断过程中 NotReady 节点数增加 |
| **未知根因** | 完成 Phase 1-3 所有诊断步骤但无法匹配任何已知根因（RC-001 至 RC-012） | 所有诊断步骤均无明确异常发现 |
| **操作权限不足** | Agent 或操作人员无 SSH 访问权限，无法执行 Phase 2+ 诊断 | Phase 1 完成后需要 SSH 但无权限 |
| **安全疑虑** | 诊断过程中发现可疑安全指标（异常进程、未知网络连接） | 任何诊断步骤中发现安全异常 |

---

### 8.2 升级消息模板

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
- Skill 版本: SKILL-NODE-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

---

### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤（D1.1 → D1.2 → ... → D3.3）及每步输出摘要
2. **已排除的根因**: 列出已通过诊断排除的根因及排除依据
   - 例: "RC-003 已排除 — D2.5 显示磁盘使用率 42%，低于阈值"
3. **可能的根因假设**: 基于已有证据提出的根因假设及置信度
   - 例: "疑似 RC-006（网络分区）— D2.7 TCP 测试超时，但 D2.2 日志中无明确连接拒绝信息"
4. **关键资源快照**:
   ```bash
   # 节点描述
   kubectl describe node <node-name> > node-describe.txt
   # 节点事件
   kubectl get events --field-selector involvedObject.name=<node-name> --sort-by=.lastTimestamp > node-events.txt
   # 节点上的 Pod 状态
   kubectl get pods --field-selector spec.nodeName=<node-name> --all-namespaces -o wide > node-pods.txt
   # kubelet 日志（最近 1 小时）
   ssh <node-ip> "journalctl -u kubelet --since '1 hour ago' --no-pager" > kubelet-logs.txt
   ```
5. **事件时间线**: 最近 30 分钟内的关键事件按时间排列
   - `HH:MM:SS` - 首次检测到 NotReady
   - `HH:MM:SS` - 开始诊断
   - `HH:MM:SS` - 发现异常 [描述]
   - `HH:MM:SS` - 尝试修复 [操作]
   - `HH:MM:SS` - 修复结果 [成功/失败]
   - `HH:MM:SS` - 决定升级

## Related

- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
