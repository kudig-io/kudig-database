# 103 - Node NotReady 状态深度诊断 (Node NotReady Diagnosis)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **难度**: 中级-高级 | **参考**: [kubernetes.io/docs/concepts/architecture/nodes](https://kubernetes.io/docs/concepts/architecture/nodes/)

---

## 目录

1. [概述与状态机制](#1-概述与状态机制)
2. [诊断决策树](#2-诊断决策树)
3. [kubelet 问题诊断](#3-kubelet-问题诊断)
4. [容器运行时诊断](#4-容器运行时诊断)
5. [资源耗尽诊断](#5-资源耗尽诊断)
6. [网络问题诊断](#6-网络问题诊断)
7. [证书问题诊断](#7-证书问题诊断)
8. [内核与系统问题](#8-内核与系统问题)
9. [ACK/云环境特定问题](#9-ack云环境特定问题)
10. [自动化诊断工具](#10-自动化诊断工具)
11. [监控告警配置](#11-监控告警配置)
12. [节点恢复操作](#12-节点恢复操作)
13. [版本特定变更](#13-版本特定变更)
14. [多角色视角](#14-多角色视角)
15. [最佳实践](#15-最佳实践)

---

## 1. 概述与状态机制

### 1.1 节点状态检测架构

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                    Node NotReady 状态检测与处理架构 (v1.25-v1.32)                        │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                          │
│  ┌─────────────────────────────────────────────────────────────────────────────────┐   │
│  │                              Control Plane                                       │   │
│  │                                                                                  │   │
│  │   ┌──────────────────────────────────────────────────────────────────────────┐ │   │
│  │   │                   kube-controller-manager                                 │ │   │
│  │   │                                                                           │ │   │
│  │   │   ┌─────────────────────────────────────────────────────────────────┐   │ │   │
│  │   │   │              NodeLifecycleController                             │   │ │   │
│  │   │   │                                                                  │   │ │   │
│  │   │   │   监控间隔: --node-monitor-period=5s                            │   │ │   │
│  │   │   │   宽限期: --node-monitor-grace-period=40s                       │   │ │   │
│  │   │   │   驱逐超时: --pod-eviction-timeout=5m (默认)                    │   │ │   │
│  │   │   │   驱逐速率: --node-eviction-rate=0.1 (10%节点/秒)               │   │ │   │
│  │   │   │                                                                  │   │ │   │
│  │   │   │   功能:                                                          │   │ │   │
│  │   │   │   • 监控 Lease 对象更新时间                                     │   │ │   │
│  │   │   │   • 监控 NodeStatus.conditions 变化                             │   │ │   │
│  │   │   │   • 添加 node.kubernetes.io/unreachable Taint                   │   │ │   │
│  │   │   │   • 触发 TaintBasedEviction (v1.18+ 默认)                       │   │ │   │
│  │   │   └─────────────────────────────────────────────────────────────────┘   │ │   │
│  │   └──────────────────────────────────────────────────────────────────────────┘ │   │
│  │                                                                                  │   │
│  │   ┌──────────────────────────────────────────────────────────────────────────┐ │   │
│  │   │                        etcd (Lease 存储)                                  │ │   │
│  │   │                                                                           │ │   │
│  │   │   kube-node-lease namespace:                                              │ │   │
│  │   │   ├── node-001  (holderIdentity: node-001, renewTime: T1)                │ │   │
│  │   │   ├── node-002  (holderIdentity: node-002, renewTime: T2)                │ │   │
│  │   │   └── node-003  (holderIdentity: node-003, renewTime: T3)                │ │   │
│  │   │                                                                           │ │   │
│  │   │   Lease 更新频率: 每 10s (kubelet --node-lease-duration-seconds=40)      │ │   │
│  │   └──────────────────────────────────────────────────────────────────────────┘ │   │
│  └─────────────────────────────────────────────────────────────────────────────────┘   │
│                                          │                                             │
│                          ┌───────────────┴───────────────┐                            │
│                          │ 心跳检测 (两种机制)            │                            │
│                          ├───────────────────────────────┤                            │
│                          │ 1. Lease 更新 (轻量级, 主要)  │                            │
│                          │ 2. NodeStatus 更新 (重量级)   │                            │
│                          └───────────────────────────────┘                            │
│                                          │                                             │
│  ┌───────────────────────────────────────┴───────────────────────────────────────┐   │
│  │                            Worker Node                                         │   │
│  │                                                                                │   │
│  │   ┌────────────────────────────────────────────────────────────────────────┐  │   │
│  │   │                           kubelet                                       │  │   │
│  │   │                                                                         │  │   │
│  │   │   心跳上报:                                                             │  │   │
│  │   │   ├── Lease 更新: 每 10s (--node-status-update-frequency 已弃用)       │  │   │
│  │   │   └── NodeStatus 更新: 每 5m 或状态变化时                               │  │   │
│  │   │                                                                         │  │   │
│  │   │   依赖检查:                                                             │  │   │
│  │   │   ├── 容器运行时 (containerd/CRI-O)                                     │  │   │
│  │   │   ├── CNI 插件健康状态                                                  │  │   │
│  │   │   ├── 系统资源 (CPU/Memory/Disk/PID)                                    │  │   │
│  │   │   └── 证书有效性                                                         │  │   │
│  │   │                                                                         │  │   │
│  │   │   健康检查端点:                                                         │  │   │
│  │   │   ├── :10248/healthz (本地健康检查)                                     │  │   │
│  │   │   ├── :10250/healthz (API健康检查)                                      │  │   │
│  │   │   └── :10255/healthz (只读, 已废弃)                                     │  │   │
│  │   └────────────────────────────────────────────────────────────────────────┘  │   │
│  │                                                                                │   │
│  │   ┌────────────────────────────────────────────────────────────────────────┐  │   │
│  │   │                  Container Runtime (containerd)                         │  │   │
│  │   │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐   │  │   │
│  │   │  │ containerd  │  │containerd-  │  │    runc     │  │   Images    │   │  │   │
│  │   │  │  daemon     │  │   shim      │  │ (OCI 运行时) │  │   Store     │   │  │   │
│  │   │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘   │  │   │
│  │   └────────────────────────────────────────────────────────────────────────┘  │   │
│  └────────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                          │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Node Condition 状态详解

| Condition | 正常值 | 异常含义 | 检测机制 | 触发影响 | 恢复条件 |
|-----------|--------|---------|---------|---------|---------|
| **Ready** | True | kubelet 不健康或无法与 API Server 通信 | Lease/NodeStatus | Pod 无法调度; 超时后驱逐 | 心跳恢复后 40s |
| **MemoryPressure** | False | 可用内存低于 eviction 阈值 | kubelet cAdvisor | BestEffort Pod 驱逐 | 内存释放后立即 |
| **DiskPressure** | False | 磁盘空间/inode 不足 | kubelet 磁盘监控 | 禁止调度; Pod 驱逐 | 磁盘释放后立即 |
| **PIDPressure** | False | 进程数接近系统限制 | kubelet PID 监控 | 禁止调度; 可能驱逐 | PID 释放后立即 |
| **NetworkUnavailable** | False | 节点网络未正确配置 | CNI 插件报告 | Pod 网络故障 | CNI 修复后 |

### 1.3 NotReady 状态时间线

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                         NotReady 状态演变时间线                                          │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                          │
│   T=0s        kubelet 停止心跳 (故障发生)                                               │
│    │                                                                                     │
│    ├── T+10s   Lease 超过更新周期 (kubelet 未更新)                                      │
│    │           ⚠️ 此时节点仍然显示 Ready=True                                           │
│    │                                                                                     │
│    ├── T+40s   Node Controller 标记 Ready=Unknown                                       │
│    │           ⚠️ 触发告警: NodeUnknown                                                 │
│    │           • nodeMonitorGracePeriod 超时                                            │
│    │                                                                                     │
│    ├── T+45s   添加 Taint: node.kubernetes.io/unreachable:NoSchedule                    │
│    │           ⚠️ 新 Pod 不再调度到此节点                                               │
│    │                                                                                     │
│    ├── T+50s   添加 Taint: node.kubernetes.io/unreachable:NoExecute                     │
│    │           ⚠️ 触发 TaintBasedEviction                                               │
│    │           • 没有 tolerationSeconds 的 Pod 立即驱逐                                 │
│    │           • 有 tolerationSeconds 的 Pod 等待超时后驱逐                             │
│    │                                                                                     │
│    ├── T+5m    默认 tolerationSeconds=300s 到期                                         │
│    │           ⚠️ 大部分工作负载 Pod 开始被驱逐                                         │
│    │           • DaemonSet Pod 保留 (默认容忍 unreachable)                              │
│    │           • 带 node.kubernetes.io/unreachable 容忍的 Pod 保留                      │
│    │                                                                                     │
│    └── T+10m+  节点持续 NotReady                                                        │
│                ⚠️ 所有不容忍 unreachable 的 Pod 已驱逐                                  │
│                • 可能触发节点自愈/替换流程                                               │
│                                                                                          │
│   恢复时间线:                                                                            │
│    │                                                                                     │
│    ├── 故障修复  kubelet 恢复心跳                                                       │
│    │                                                                                     │
│    ├── +10s     Lease 更新成功                                                          │
│    │                                                                                     │
│    ├── +40s     Node Controller 检测到心跳恢复                                          │
│    │            Ready=Unknown → Ready=True                                               │
│    │                                                                                     │
│    └── +45s     移除 unreachable Taint                                                  │
│                 节点恢复正常调度                                                         │
│                                                                                          │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### 1.4 紧急程度分类

| 紧急级别 | NotReady 持续时间 | 影响范围 | 响应时间 | 处理优先级 |
|---------|-----------------|---------|---------|-----------|
| **P0 - 紧急** | > 10min | 多节点/关键业务 | < 15min | 立即处理 |
| **P1 - 高** | > 5min | 生产单节点 | < 30min | 优先处理 |
| **P2 - 中** | > 2min | 非核心节点 | < 2h | 计划处理 |
| **P3 - 低** | < 2min | 开发/测试环境 | < 24h | 常规处理 |

---

## 2. 诊断决策树

### 2.1 快速诊断流程图

```
┌─────────────────────────────────────────────────────────────────────────────────────────┐
│                          Node NotReady 快速诊断决策树                                    │
├─────────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                          │
│   Node NotReady 告警触发                                                                │
│           │                                                                              │
│           ▼                                                                              │
│   ┌───────────────────────────────────────┐                                             │
│   │ Step 1: 确认问题范围                   │                                             │
│   │ kubectl get nodes                      │                                             │
│   └───────────────────────────────────────┘                                             │
│           │                                                                              │
│           ├─────────────────────────────────────────────────────┐                       │
│           │                                                     │                       │
│     单节点 NotReady                                      多节点 NotReady                │
│           │                                                     │                       │
│           ▼                                                     ▼                       │
│   ┌───────────────────┐                              ┌───────────────────────┐          │
│   │ Step 2: 检查连通性│                              │ 检查控制平面/网络    │          │
│   │ SSH 能否连接?     │                              │ • API Server 状态    │          │
│   └───────────────────┘                              │ • 网络基础设施       │          │
│           │                                          │ • 云平台故障        │          │
│      ┌────┴────┐                                     └───────────────────────┘          │
│      │         │                                                                        │
│   能 SSH    不能 SSH                                                                    │
│      │         │                                                                        │
│      ▼         ▼                                                                        │
│  继续诊断   检查云平台/                                                                 │
│             物理机状态                                                                   │
│      │                                                                                   │
│      ▼                                                                                   │
│   ┌───────────────────────────────────────┐                                             │
│   │ Step 3: 检查 kubelet                  │                                             │
│   │ systemctl status kubelet              │                                             │
│   └───────────────────────────────────────┘                                             │
│           │                                                                              │
│      ┌────┴────┐                                                                        │
│      │         │                                                                        │
│   运行中    未运行                                                                       │
│      │         │                                                                        │
│      │         └──▶ 检查 kubelet 日志 ──▶ 跳转第3章                                    │
│      │                                                                                   │
│      ▼                                                                                   │
│   ┌───────────────────────────────────────┐                                             │
│   │ Step 4: 检查容器运行时                 │                                             │
│   │ systemctl status containerd           │                                             │
│   │ crictl info                           │                                             │
│   └───────────────────────────────────────┘                                             │
│           │                                                                              │
│      ┌────┴────┐                                                                        │
│      │         │                                                                        │
│   正常      异常 ──▶ 跳转第4章                                                          │
│      │                                                                                   │
│      ▼                                                                                   │
│   ┌───────────────────────────────────────┐                                             │
│   │ Step 5: 检查系统资源                   │                                             │
│   │ free -h / df -h / ps aux              │                                             │
│   └───────────────────────────────────────┘                                             │
│           │                                                                              │
│      ┌────┴────┐                                                                        │
│      │         │                                                                        │
│   充足      不足 ──▶ 跳转第5章                                                          │
│      │                                                                                   │
│      ▼                                                                                   │
│   ┌───────────────────────────────────────┐                                             │
│   │ Step 6: 检查网络连通性                 │                                             │
│   │ curl -k https://apiserver:6443/healthz│                                             │
│   └───────────────────────────────────────┘                                             │
│           │                                                                              │
│      ┌────┴────┐                                                                        │
│      │         │                                                                        │
│   可达      不可达 ──▶ 跳转第6章                                                        │
│      │                                                                                   │
│      ▼                                                                                   │
│   ┌───────────────────────────────────────┐                                             │
│   │ Step 7: 检查证书                       │                                             │
│   │ openssl x509 -in <cert> -noout -dates │                                             │
│   └───────────────────────────────────────┘                                             │
│           │                                                                              │
│      ┌────┴────┐                                                                        │
│      │         │                                                                        │
│   有效      过期 ──▶ 跳转第7章                                                          │
│      │                                                                                   │
│      ▼                                                                                   │
│   ┌───────────────────────────────────────┐                                             │
│   │ Step 8: 深度诊断                       │                                             │
│   │ 检查 dmesg / journalctl               │                                             │
│   │ 跳转第8章: 内核与系统问题             │                                             │
│   └───────────────────────────────────────┘                                             │
│                                                                                          │
└─────────────────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 故障原因分类矩阵

| 原因类别 | 具体原因 | 发生频率 | 影响范围 | 诊断难度 | 恢复难度 |
|---------|---------|---------|---------|---------|---------|
| **kubelet** | kubelet 进程崩溃 | 高 | 单节点 | 低 | 低 |
| **kubelet** | kubelet 配置错误 | 中 | 单节点 | 中 | 中 |
| **kubelet** | kubelet 证书过期 | 低 | 可能多节点 | 低 | 中 |
| **kubelet** | PLEG 超时 | 中 | 单节点 | 中 | 中 |
| **运行时** | containerd 故障 | 中 | 单节点 | 低 | 低 |
| **运行时** | containerd OOM | 中 | 单节点 | 中 | 中 |
| **运行时** | shim 进程泄漏 | 中 | 单节点 | 中 | 低 |
| **运行时** | 镜像存储损坏 | 低 | 单节点 | 高 | 高 |
| **资源** | 内存耗尽 | 高 | 单节点 | 低 | 中 |
| **资源** | 磁盘满 | 高 | 单节点 | 低 | 低 |
| **资源** | inode 耗尽 | 中 | 单节点 | 低 | 低 |
| **资源** | PID 耗尽 | 低 | 单节点 | 低 | 低 |
| **网络** | API Server 不可达 | 中 | 单/多节点 | 中 | 中 |
| **网络** | CNI 插件故障 | 高 | 多节点 | 中 | 中 |
| **网络** | DNS 解析失败 | 中 | 多节点 | 低 | 低 |
| **网络** | 网卡故障 | 低 | 单节点 | 中 | 高 |
| **证书** | kubelet 客户端证书过期 | 低 | 多节点 | 低 | 中 |
| **证书** | CA 证书不匹配 | 低 | 多节点 | 中 | 高 |
| **内核** | 内核 panic | 低 | 单节点 | 高 | 高 |
| **内核** | 内核死锁 | 低 | 单节点 | 高 | 高 |
| **内核** | cgroup 异常 | 低 | 单节点 | 高 | 中 |
| **硬件** | 物理服务器宕机 | 低 | 单节点 | - | 高 |
| **云平台** | 实例被回收 | 中(竞价) | 单节点 | 低 | 中 |
| **云平台** | 云平台故障 | 低 | 多节点 | - | - |

---

## 3. kubelet 问题诊断

### 3.1 kubelet 状态检查

```bash
#!/bin/bash
# kubelet-diagnosis.sh - kubelet 完整诊断脚本

echo "=============================================="
echo "  kubelet 诊断报告 - $(hostname)"
echo "  时间: $(date)"
echo "=============================================="

# 1. 服务状态
echo ""
echo "=== 1. kubelet 服务状态 ==="
systemctl status kubelet --no-pager | head -20
echo ""
echo "服务是否启用: $(systemctl is-enabled kubelet 2>/dev/null)"
echo "服务是否运行: $(systemctl is-active kubelet 2>/dev/null)"
echo "服务是否失败: $(systemctl is-failed kubelet 2>/dev/null)"

# 2. 进程信息
echo ""
echo "=== 2. kubelet 进程信息 ==="
KUBELET_PID=$(pgrep -x kubelet)
if [ -n "$KUBELET_PID" ]; then
    echo "PID: $KUBELET_PID"
    ps -p $KUBELET_PID -o pid,ppid,%cpu,%mem,vsz,rss,etime,args --no-headers
    echo ""
    echo "打开文件数: $(ls /proc/$KUBELET_PID/fd 2>/dev/null | wc -l)"
    echo "线程数: $(ls /proc/$KUBELET_PID/task 2>/dev/null | wc -l)"
else
    echo "kubelet 进程未运行!"
fi

# 3. 配置文件检查
echo ""
echo "=== 3. kubelet 配置 ==="
KUBELET_CONFIG="/var/lib/kubelet/config.yaml"
if [ -f "$KUBELET_CONFIG" ]; then
    echo "配置文件: $KUBELET_CONFIG"
    echo "--- 关键配置 ---"
    grep -E "^(clusterDNS|clusterDomain|containerRuntimeEndpoint|cgroupDriver|evictionHard|systemReserved|kubeReserved)" "$KUBELET_CONFIG" 2>/dev/null
else
    echo "配置文件不存在: $KUBELET_CONFIG"
fi

# 4. 健康检查端点
echo ""
echo "=== 4. 健康检查端点 ==="
echo "本地健康检查 (:10248/healthz):"
curl -s --connect-timeout 5 http://localhost:10248/healthz && echo " [OK]" || echo " [FAIL]"
echo ""
echo "API 健康检查 (:10250/healthz):"
curl -sk --connect-timeout 5 https://localhost:10250/healthz && echo " [OK]" || echo " [FAIL]"

# 5. API Server 连接
echo ""
echo "=== 5. API Server 连接 ==="
KUBECONFIG="/etc/kubernetes/kubelet.conf"
if [ -f "$KUBECONFIG" ]; then
    API_SERVER=$(grep "server:" "$KUBECONFIG" | awk '{print $2}')
    echo "API Server: $API_SERVER"
    echo "连接测试:"
    curl -sk --connect-timeout 5 "${API_SERVER}/healthz" && echo " [OK]" || echo " [FAIL]"
fi

# 6. 错误日志分析
echo ""
echo "=== 6. 最近错误日志 ==="
echo "--- 最近 30 分钟内的错误 ---"
journalctl -u kubelet --since "30 minutes ago" --no-pager 2>/dev/null | grep -iE "error|fail|fatal|panic" | tail -20

# 7. PLEG 状态
echo ""
echo "=== 7. PLEG 状态 ==="
PLEG_ERRORS=$(journalctl -u kubelet --since "10 minutes ago" --no-pager 2>/dev/null | grep -c "PLEG is not healthy")
echo "最近 10 分钟 PLEG 不健康次数: $PLEG_ERRORS"
if [ $PLEG_ERRORS -gt 0 ]; then
    echo "最近 PLEG 错误:"
    journalctl -u kubelet --since "10 minutes ago" --no-pager 2>/dev/null | grep "PLEG" | tail -5
fi

# 8. Node 状态同步
echo ""
echo "=== 8. Node 状态同步 ==="
journalctl -u kubelet --since "5 minutes ago" --no-pager 2>/dev/null | grep -E "node status|heartbeat" | tail -5
```

### 3.2 kubelet 常见错误及解决

| 错误日志模式 | 原因 | 解决方案 |
|-------------|------|---------|
| `failed to run Kubelet: running with swap on is not supported` | 未禁用 swap | `swapoff -a && sed -i '/swap/d' /etc/fstab` |
| `error: failed to run Kubelet: unable to load bootstrap kubeconfig` | bootstrap 配置丢失 | 重新执行 kubeadm join |
| `Unable to update cni config: no networks found` | CNI 未配置 | 安装 CNI 插件 |
| `PLEG is not healthy: pleg was last seen active` | 容器运行时慢/故障 | 重启 containerd/清理容器 |
| `failed to get node info` | API Server 不可达 | 检查网络/证书 |
| `certificate has expired` | 证书过期 | 更新证书 |
| `error killing pod: context deadline exceeded` | 运行时响应慢 | 重启 containerd |
| `failed to update node status` | etcd/apiserver 问题 | 检查控制平面 |

### 3.3 kubelet 配置优化

```yaml
# /var/lib/kubelet/config.yaml - 生产环境推荐配置
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration

# === 基础配置 ===
clusterDNS:
- 10.96.0.10
clusterDomain: cluster.local
staticPodPath: /etc/kubernetes/manifests

# === 容器运行时 ===
containerRuntimeEndpoint: unix:///run/containerd/containerd.sock
cgroupDriver: systemd

# === 资源管理 ===
maxPods: 110
podsPerCore: 0  # 0 表示不限制

# 系统预留资源 (为系统进程预留)
systemReserved:
  cpu: 500m
  memory: 1Gi
  ephemeral-storage: 5Gi

# Kubernetes 预留资源 (为 kubelet/containerd 预留)
kubeReserved:
  cpu: 500m
  memory: 1Gi
  ephemeral-storage: 5Gi

# 强制资源预留
enforceNodeAllocatable:
- pods
- system-reserved
- kube-reserved

# === 驱逐配置 ===
# 硬驱逐阈值 (立即驱逐)
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"

# 软驱逐阈值 (等待宽限期后驱逐)
evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"
  imagefs.available: "20%"

evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "1m30s"
  imagefs.available: "1m30s"

# 最小回收量
evictionMinimumReclaim:
  memory.available: "100Mi"
  nodefs.available: "1Gi"
  imagefs.available: "2Gi"

evictionMaxPodGracePeriod: 90
evictionPressureTransitionPeriod: 30s

# === 镜像管理 ===
imageGCHighThresholdPercent: 80
imageGCLowThresholdPercent: 70
imageMinimumGCAge: 2m

# === 日志 ===
containerLogMaxSize: 50Mi
containerLogMaxFiles: 5

# === 证书 ===
rotateCertificates: true
serverTLSBootstrap: true

# === 特性门控 (v1.25+) ===
featureGates:
  RotateKubeletServerCertificate: true
  GracefulNodeShutdown: true
  GracefulNodeShutdownBasedOnPodPriority: true

# 优雅关机配置
shutdownGracePeriod: 60s
shutdownGracePeriodCriticalPods: 20s

# === 调试配置 (问题排查时启用) ===
# logging:
#   format: text
#   verbosity: 4
```

### 3.4 kubelet 重启与恢复

```bash
#!/bin/bash
# kubelet-recovery.sh - kubelet 恢复脚本

echo "=== kubelet 恢复流程 ==="

# 1. 检查当前状态
echo "当前状态:"
systemctl status kubelet --no-pager | head -5

# 2. 清理可能的问题文件
echo ""
echo "清理临时文件..."
rm -f /var/lib/kubelet/cpu_manager_state
rm -f /var/lib/kubelet/memory_manager_state

# 3. 重启 kubelet
echo ""
echo "重启 kubelet..."
systemctl daemon-reload
systemctl restart kubelet

# 4. 等待并验证
echo ""
echo "等待 kubelet 启动..."
sleep 10

# 5. 检查状态
echo ""
echo "验证状态:"
systemctl status kubelet --no-pager | head -10

# 6. 检查健康端点
echo ""
echo "健康检查:"
for i in {1..6}; do
    result=$(curl -s --connect-timeout 5 http://localhost:10248/healthz 2>/dev/null)
    if [ "$result" = "ok" ]; then
        echo "kubelet 健康检查通过"
        break
    fi
    echo "等待 kubelet 就绪... ($i/6)"
    sleep 5
done

# 7. 检查节点状态
echo ""
echo "检查节点状态 (从外部 kubectl):"
echo "请在可访问 API Server 的机器上执行:"
echo "  kubectl get node $(hostname) -o wide"
```

---

## 4. 容器运行时诊断

### 4.1 containerd 完整诊断

```bash
#!/bin/bash
# containerd-full-diagnosis.sh

echo "=============================================="
echo "  containerd 诊断报告 - $(hostname)"
echo "  时间: $(date)"
echo "=============================================="

# 1. 服务状态
echo ""
echo "=== 1. containerd 服务状态 ==="
systemctl status containerd --no-pager | head -15
echo ""
echo "服务状态: $(systemctl is-active containerd)"

# 2. 进程信息
echo ""
echo "=== 2. containerd 进程 ==="
CONTAINERD_PID=$(pgrep -x containerd)
if [ -n "$CONTAINERD_PID" ]; then
    echo "PID: $CONTAINERD_PID"
    ps -p $CONTAINERD_PID -o pid,%cpu,%mem,vsz,rss,etime --no-headers
    echo ""
    echo "打开文件数: $(ls /proc/$CONTAINERD_PID/fd 2>/dev/null | wc -l)"
    echo "内存映射数: $(wc -l < /proc/$CONTAINERD_PID/maps 2>/dev/null)"
else
    echo "containerd 进程未运行!"
fi

# 3. Socket 状态
echo ""
echo "=== 3. containerd Socket ==="
ls -la /run/containerd/containerd.sock 2>/dev/null
echo ""
echo "CRI 信息:"
crictl info 2>&1 | head -30

# 4. 容器统计
echo ""
echo "=== 4. 容器统计 ==="
echo "运行中容器: $(crictl ps -q 2>/dev/null | wc -l)"
echo "所有容器: $(crictl ps -aq 2>/dev/null | wc -l)"
echo "已退出容器: $(crictl ps -aq --state exited 2>/dev/null | wc -l)"

# 5. Pod Sandbox 统计
echo ""
echo "=== 5. Pod Sandbox 统计 ==="
echo "运行中 Sandbox: $(crictl pods -q --state ready 2>/dev/null | wc -l)"
echo "所有 Sandbox: $(crictl pods -q 2>/dev/null | wc -l)"
echo "NotReady Sandbox: $(crictl pods -q --state notready 2>/dev/null | wc -l)"

# 6. shim 进程
echo ""
echo "=== 6. containerd-shim 进程 ==="
SHIM_COUNT=$(ps aux | grep containerd-shim | grep -v grep | wc -l)
echo "shim 进程数: $SHIM_COUNT"
if [ $SHIM_COUNT -gt 100 ]; then
    echo "⚠️ 警告: shim 进程过多，可能存在泄漏!"
fi

# 7. 镜像统计
echo ""
echo "=== 7. 镜像统计 ==="
echo "镜像数量: $(crictl images -q 2>/dev/null | wc -l)"
echo ""
echo "磁盘占用:"
du -sh /var/lib/containerd 2>/dev/null

# 8. 存储状态
echo ""
echo "=== 8. 存储状态 ==="
echo "--- /var/lib/containerd 子目录 ---"
du -sh /var/lib/containerd/* 2>/dev/null | sort -hr | head -5

# 9. 错误日志
echo ""
echo "=== 9. 最近错误日志 ==="
journalctl -u containerd --since "30 minutes ago" --no-pager 2>/dev/null | grep -iE "error|fail|panic" | tail -15

# 10. CRI 响应时间测试
echo ""
echo "=== 10. CRI 响应时间测试 ==="
echo "执行 crictl info..."
time (crictl info > /dev/null 2>&1)
```

### 4.2 containerd 问题解决

| 问题 | 症状 | 诊断命令 | 解决方案 |
|------|------|---------|---------|
| **进程崩溃** | kubelet 日志显示 CRI 不可用 | `systemctl status containerd` | 重启 containerd |
| **OOM** | containerd 进程被 kill | `dmesg \| grep containerd` | 增加系统内存/限制容器 |
| **shim 泄漏** | shim 进程过多 | `ps aux \| grep shim \| wc -l` | 清理并重启 |
| **存储满** | 镜像拉取失败 | `df -h /var/lib/containerd` | 清理镜像/扩容 |
| **Socket 异常** | CRI 调用超时 | `crictl info` | 重启 containerd |
| **快照损坏** | 容器启动失败 | `ctr snapshots list` | 清理损坏的快照 |

```bash
# containerd 常见修复操作

# 1. 重启 containerd
systemctl restart containerd
sleep 5
crictl info

# 2. 清理已退出容器
crictl rm $(crictl ps -aq --state exited) 2>/dev/null

# 3. 清理未使用镜像
crictl rmi --prune

# 4. 清理 containerd 内部垃圾
ctr -n k8s.io content prune references
ctr -n k8s.io snapshots cleanup

# 5. 强制清理 (危险操作 - 会丢失所有容器)
# systemctl stop kubelet
# systemctl stop containerd
# rm -rf /var/lib/containerd/io.containerd.metadata.v1.bolt/*
# systemctl start containerd
# systemctl start kubelet

# 6. 重置 containerd (最后手段)
# systemctl stop kubelet
# systemctl stop containerd
# rm -rf /var/lib/containerd/*
# systemctl start containerd
# systemctl start kubelet
# # 需要重新拉取所有镜像
```

### 4.3 containerd 配置检查

```toml
# /etc/containerd/config.toml - 生产环境配置
version = 2

# 根目录
root = "/var/lib/containerd"
state = "/run/containerd"

# gRPC 配置
[grpc]
  address = "/run/containerd/containerd.sock"
  max_recv_message_size = 16777216
  max_send_message_size = 16777216

# 插件配置
[plugins]
  # CRI 插件
  [plugins."io.containerd.grpc.v1.cri"]
    # Sandbox 镜像
    sandbox_image = "registry.k8s.io/pause:3.9"
    
    # 并发下载数
    max_concurrent_downloads = 3
    
    # 禁用 TCP 服务 (安全)
    disable_tcp_service = true
    
    [plugins."io.containerd.grpc.v1.cri".containerd]
      # 快照驱动
      snapshotter = "overlayfs"
      default_runtime_name = "runc"
      
      [plugins."io.containerd.grpc.v1.cri".containerd.runtimes]
        [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
          runtime_type = "io.containerd.runc.v2"
          
          [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
            SystemdCgroup = true
            
    # 镜像加速配置
    [plugins."io.containerd.grpc.v1.cri".registry]
      [plugins."io.containerd.grpc.v1.cri".registry.mirrors]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."docker.io"]
          endpoint = ["https://mirror.ccs.tencentyun.com", "https://registry-1.docker.io"]
        [plugins."io.containerd.grpc.v1.cri".registry.mirrors."registry.k8s.io"]
          endpoint = ["https://registry.aliyuncs.com/google_containers"]

# 调试配置
[debug]
  level = "info"  # 问题排查时改为 "debug"

# 指标配置
[metrics]
  address = "127.0.0.1:1338"
```

---

## 5. 资源耗尽诊断

### 5.1 内存问题诊断

```bash
#!/bin/bash
# memory-diagnosis.sh - 内存问题诊断

echo "=============================================="
echo "  内存问题诊断报告"
echo "=============================================="

# 1. 内存概览
echo ""
echo "=== 1. 内存概览 ==="
free -h

# 2. 详细内存信息
echo ""
echo "=== 2. 内存详情 ==="
cat /proc/meminfo | grep -E "MemTotal|MemFree|MemAvailable|Buffers|Cached|SwapTotal|SwapFree|Slab|SReclaimable|SUnreclaim"

# 3. 内存压力
echo ""
echo "=== 3. 内存压力状态 ==="
if [ -f /proc/pressure/memory ]; then
    cat /proc/pressure/memory
else
    echo "内存压力统计不可用"
fi

# 4. 内存使用 Top 进程
echo ""
echo "=== 4. 内存使用 Top 15 进程 ==="
ps aux --sort=-%mem | head -16 | awk '{printf "%-10s %-8s %6s %6s %s\n", $1, $2, $4"%", $6/1024"MB", $11}'

# 5. cgroup 内存统计
echo ""
echo "=== 5. cgroup 内存统计 ==="
if [ -d "/sys/fs/cgroup/memory/kubepods" ]; then
    echo "cgroup v1 检测到"
    echo "kubepods 内存限制: $(numfmt --to=iec < /sys/fs/cgroup/memory/kubepods/memory.limit_in_bytes 2>/dev/null)"
    echo "kubepods 当前使用: $(numfmt --to=iec < /sys/fs/cgroup/memory/kubepods/memory.usage_in_bytes 2>/dev/null)"
elif [ -f "/sys/fs/cgroup/kubepods/memory.max" ]; then
    echo "cgroup v2 检测到"
    cat /sys/fs/cgroup/kubepods/memory.max
    cat /sys/fs/cgroup/kubepods/memory.current
fi

# 6. OOM 历史
echo ""
echo "=== 6. 最近 OOM 事件 ==="
dmesg | grep -i "oom\|killed process" | tail -10

# 7. 内存分配失败
echo ""
echo "=== 7. 内存分配统计 ==="
cat /proc/vmstat | grep -E "pgfault|pgmajfault|oom_kill"

# 8. SWAP 使用
echo ""
echo "=== 8. SWAP 使用 ==="
swapon -s 2>/dev/null || echo "SWAP 未启用"

# 9. 大内存进程分析
echo ""
echo "=== 9. containerd/kubelet 内存使用 ==="
for proc in containerd kubelet; do
    PID=$(pgrep -x $proc)
    if [ -n "$PID" ]; then
        RSS=$(cat /proc/$PID/status | grep VmRSS | awk '{print $2}')
        echo "$proc (PID $PID): $((RSS/1024)) MB"
    fi
done
```

### 5.2 磁盘问题诊断

```bash
#!/bin/bash
# disk-diagnosis.sh - 磁盘问题诊断

echo "=============================================="
echo "  磁盘问题诊断报告"
echo "=============================================="

# 1. 磁盘空间
echo ""
echo "=== 1. 磁盘空间使用 ==="
df -h | grep -E "Filesystem|^/dev"

# 2. inode 使用
echo ""
echo "=== 2. inode 使用 ==="
df -i | grep -E "Filesystem|^/dev"

# 3. 大目录
echo ""
echo "=== 3. /var/lib 大目录 Top 10 ==="
du -sh /var/lib/* 2>/dev/null | sort -hr | head -10

# 4. containerd 存储
echo ""
echo "=== 4. containerd 存储详情 ==="
du -sh /var/lib/containerd/* 2>/dev/null | sort -hr

# 5. kubelet 存储
echo ""
echo "=== 5. kubelet 存储详情 ==="
du -sh /var/lib/kubelet/* 2>/dev/null | sort -hr | head -10

# 6. 日志目录
echo ""
echo "=== 6. 日志目录 ==="
du -sh /var/log/* 2>/dev/null | sort -hr | head -10

# 7. 容器日志
echo ""
echo "=== 7. 容器日志 Top 10 ==="
find /var/log/pods -name "*.log" -type f -exec du -h {} \; 2>/dev/null | sort -hr | head -10

# 8. 大文件
echo ""
echo "=== 8. 大文件 (>500MB) ==="
find /var -type f -size +500M 2>/dev/null | head -10

# 9. 磁盘 IO
echo ""
echo "=== 9. 磁盘 IO 统计 ==="
iostat -x 1 2 2>/dev/null | tail -20 || echo "iostat 不可用"

# 10. 磁盘健康
echo ""
echo "=== 10. 磁盘健康检查 ==="
for disk in $(lsblk -dn -o NAME | grep -E "^sd|^nvme"); do
    echo "--- $disk ---"
    smartctl -H /dev/$disk 2>/dev/null | grep -E "PASSED|FAILED" || echo "smartctl 不可用"
done
```

### 5.3 资源清理操作

```bash
#!/bin/bash
# resource-cleanup.sh - 资源清理脚本

echo "=== 资源清理脚本 ==="
read -p "确认执行清理? (yes/no): " CONFIRM
if [ "$CONFIRM" != "yes" ]; then
    echo "已取消"
    exit 0
fi

# 1. 清理已退出容器
echo ""
echo "=== 1. 清理已退出容器 ==="
EXITED=$(crictl ps -aq --state exited 2>/dev/null | wc -l)
echo "已退出容器数: $EXITED"
if [ $EXITED -gt 0 ]; then
    crictl rm $(crictl ps -aq --state exited) 2>/dev/null
    echo "已清理"
fi

# 2. 清理未使用镜像
echo ""
echo "=== 2. 清理未使用镜像 ==="
crictl rmi --prune 2>/dev/null

# 3. 清理 containerd 垃圾
echo ""
echo "=== 3. 清理 containerd 垃圾 ==="
ctr -n k8s.io content prune references 2>/dev/null

# 4. 清理日志
echo ""
echo "=== 4. 清理日志 ==="
# 清理 journald 日志
journalctl --vacuum-size=1G
journalctl --vacuum-time=7d

# 截断大容器日志
echo "截断大容器日志..."
find /var/log/containers -name "*.log" -size +100M -exec truncate -s 0 {} \; 2>/dev/null
find /var/log/pods -name "*.log" -size +100M -exec truncate -s 0 {} \; 2>/dev/null

# 5. 清理临时文件
echo ""
echo "=== 5. 清理临时文件 ==="
rm -rf /tmp/* 2>/dev/null
rm -rf /var/tmp/* 2>/dev/null

# 6. 清理 coredump
echo ""
echo "=== 6. 清理 coredump ==="
find /var/lib/systemd/coredump -type f -mtime +7 -delete 2>/dev/null

# 7. 清理旧内核
echo ""
echo "=== 7. 旧内核清理提示 ==="
echo "如需清理旧内核，请手动执行:"
echo "  RHEL/CentOS: package-cleanup --oldkernels --count=2"
echo "  Ubuntu/Debian: apt autoremove"

# 8. 最终状态
echo ""
echo "=== 清理完成 ==="
echo "磁盘使用:"
df -h | grep -E "/$|/var"
echo ""
echo "内存使用:"
free -h
```

---

## 6. 网络问题诊断

### 6.1 网络连通性诊断

```bash
#!/bin/bash
# network-diagnosis.sh - 网络问题诊断

echo "=============================================="
echo "  网络问题诊断报告"
echo "=============================================="

# 1. 网络接口
echo ""
echo "=== 1. 网络接口 ==="
ip addr show | grep -E "^[0-9]+:|inet "

# 2. 默认路由
echo ""
echo "=== 2. 路由表 ==="
ip route show
echo ""
echo "默认网关:"
ip route show default

# 3. DNS 配置
echo ""
echo "=== 3. DNS 配置 ==="
cat /etc/resolv.conf

# 4. API Server 连通性
echo ""
echo "=== 4. API Server 连通性 ==="
API_SERVER=$(grep "server:" /etc/kubernetes/kubelet.conf 2>/dev/null | awk '{print $2}')
if [ -n "$API_SERVER" ]; then
    echo "API Server: $API_SERVER"
    echo "TCP 连接测试:"
    timeout 5 bash -c "echo > /dev/tcp/${API_SERVER#https://}/6443" 2>/dev/null && echo "  [OK]" || echo "  [FAIL]"
    echo ""
    echo "HTTPS 健康检查:"
    curl -sk --connect-timeout 5 "${API_SERVER}/healthz" && echo " [OK]" || echo " [FAIL]"
fi

# 5. DNS 解析测试
echo ""
echo "=== 5. DNS 解析测试 ==="
echo "kubernetes.default.svc.cluster.local:"
nslookup kubernetes.default.svc.cluster.local 2>/dev/null | tail -5 || echo "  解析失败"
echo ""
echo "外部域名 (google.com):"
nslookup google.com 2>/dev/null | tail -3 || echo "  解析失败"

# 6. 网络连接统计
echo ""
echo "=== 6. 网络连接统计 ==="
ss -s

# 7. 网络错误
echo ""
echo "=== 7. 网络接口错误 ==="
ip -s link show | grep -A 5 "^[0-9]" | grep -E "^[0-9]|errors"

# 8. iptables 规则
echo ""
echo "=== 8. iptables 规则统计 ==="
echo "NAT 规则数: $(iptables -t nat -L -n 2>/dev/null | wc -l)"
echo "Filter 规则数: $(iptables -t filter -L -n 2>/dev/null | wc -l)"

# 9. conntrack
echo ""
echo "=== 9. conntrack 状态 ==="
echo "当前连接数: $(cat /proc/sys/net/netfilter/nf_conntrack_count 2>/dev/null)"
echo "最大连接数: $(cat /proc/sys/net/netfilter/nf_conntrack_max 2>/dev/null)"

# 10. 网络延迟
echo ""
echo "=== 10. 网络延迟测试 ==="
if [ -n "$API_SERVER" ]; then
    HOST=$(echo $API_SERVER | sed 's|https://||' | cut -d: -f1)
    echo "Ping API Server ($HOST):"
    ping -c 3 $HOST 2>/dev/null | tail -3 || echo "  Ping 失败"
fi
```

### 6.2 CNI 插件诊断

```bash
#!/bin/bash
# cni-diagnosis.sh - CNI 插件诊断

echo "=============================================="
echo "  CNI 插件诊断报告"
echo "=============================================="

# 1. CNI 配置
echo ""
echo "=== 1. CNI 配置文件 ==="
ls -la /etc/cni/net.d/

echo ""
echo "--- 配置文件内容 ---"
for conf in /etc/cni/net.d/*.conf /etc/cni/net.d/*.conflist; do
    if [ -f "$conf" ]; then
        echo ""
        echo "文件: $conf"
        cat "$conf" | head -30
    fi
done

# 2. CNI 二进制
echo ""
echo "=== 2. CNI 二进制 ==="
ls -la /opt/cni/bin/ | head -15

# 3. 检测 CNI 类型
echo ""
echo "=== 3. CNI 类型检测 ==="
if ls /etc/cni/net.d/ | grep -q calico; then
    echo "检测到: Calico"
    echo ""
    echo "Calico 节点状态:"
    calicoctl node status 2>/dev/null || kubectl get pods -n kube-system -l k8s-app=calico-node -o wide
elif ls /etc/cni/net.d/ | grep -q cilium; then
    echo "检测到: Cilium"
    echo ""
    echo "Cilium 状态:"
    cilium status 2>/dev/null || kubectl get pods -n kube-system -l k8s-app=cilium -o wide
elif ls /etc/cni/net.d/ | grep -q flannel; then
    echo "检测到: Flannel"
    echo ""
    kubectl get pods -n kube-system -l app=flannel -o wide
elif ls /etc/cni/net.d/ | grep -q terway; then
    echo "检测到: Terway (ACK)"
    echo ""
    kubectl get pods -n kube-system -l app=terway -o wide
else
    echo "未知 CNI 类型"
fi

# 4. CNI 日志
echo ""
echo "=== 4. CNI 相关日志 ==="
journalctl -u kubelet --since "10 minutes ago" --no-pager 2>/dev/null | grep -i cni | tail -10
```

### 6.3 Calico 网络诊断

```bash
#!/bin/bash
# calico-diagnosis.sh

echo "=== Calico 网络诊断 ==="

# 1. 节点状态
echo ""
echo "=== 1. Calico 节点状态 ==="
calicoctl node status 2>/dev/null || echo "calicoctl 不可用"

# 2. BGP Peers
echo ""
echo "=== 2. BGP Peers ==="
calicoctl get bgppeer -o wide 2>/dev/null

# 3. IP Pool
echo ""
echo "=== 3. IP Pool ==="
calicoctl get ippool -o yaml 2>/dev/null | head -30

# 4. Felix 状态
echo ""
echo "=== 4. Felix 状态 ==="
curl -s http://localhost:9099/readiness 2>/dev/null || echo "Felix 健康检查不可用"

# 5. BIRD 状态
echo ""
echo "=== 5. BIRD BGP 状态 ==="
birdcl show protocols 2>/dev/null | head -20 || echo "BIRD 不可用"

# 6. Calico Pod 状态
echo ""
echo "=== 6. Calico Pod 状态 ==="
kubectl get pods -n kube-system -l k8s-app=calico-node -o wide
```

---

## 7. 证书问题诊断

### 7.1 证书检查脚本

```bash
#!/bin/bash
# certificate-diagnosis.sh - 证书诊断

echo "=============================================="
echo "  证书诊断报告"
echo "=============================================="

# 1. kubelet 客户端证书
echo ""
echo "=== 1. kubelet 客户端证书 ==="
KUBELET_CERT="/var/lib/kubelet/pki/kubelet-client-current.pem"
if [ -f "$KUBELET_CERT" ]; then
    echo "文件: $KUBELET_CERT"
    echo ""
    echo "证书信息:"
    openssl x509 -in "$KUBELET_CERT" -noout -dates -subject -issuer 2>/dev/null
    echo ""
    # 计算剩余天数
    EXPIRY=$(openssl x509 -in "$KUBELET_CERT" -noout -enddate 2>/dev/null | cut -d= -f2)
    if [ -n "$EXPIRY" ]; then
        EXPIRY_EPOCH=$(date -d "$EXPIRY" +%s 2>/dev/null)
        NOW_EPOCH=$(date +%s)
        DAYS_LEFT=$(( (EXPIRY_EPOCH - NOW_EPOCH) / 86400 ))
        echo "剩余天数: $DAYS_LEFT 天"
        if [ $DAYS_LEFT -lt 30 ]; then
            echo "⚠️ 警告: 证书将在 30 天内过期!"
        fi
        if [ $DAYS_LEFT -lt 0 ]; then
            echo "❌ 错误: 证书已过期!"
        fi
    fi
else
    echo "证书文件不存在: $KUBELET_CERT"
fi

# 2. kubelet 服务证书
echo ""
echo "=== 2. kubelet 服务证书 ==="
KUBELET_SERVER_CERT="/var/lib/kubelet/pki/kubelet.crt"
if [ -f "$KUBELET_SERVER_CERT" ]; then
    echo "文件: $KUBELET_SERVER_CERT"
    openssl x509 -in "$KUBELET_SERVER_CERT" -noout -dates 2>/dev/null
fi

# 3. CA 证书
echo ""
echo "=== 3. CA 证书 ==="
CA_CERT="/etc/kubernetes/pki/ca.crt"
if [ -f "$CA_CERT" ]; then
    echo "文件: $CA_CERT"
    openssl x509 -in "$CA_CERT" -noout -dates -subject 2>/dev/null
fi

# 4. 证书链验证
echo ""
echo "=== 4. 证书链验证 ==="
if [ -f "$CA_CERT" ] && [ -f "$KUBELET_CERT" ]; then
    echo "验证 kubelet 客户端证书:"
    openssl verify -CAfile "$CA_CERT" "$KUBELET_CERT" 2>&1
fi

# 5. kubeadm 证书检查
echo ""
echo "=== 5. kubeadm 证书状态 ==="
if command -v kubeadm &>/dev/null; then
    kubeadm certs check-expiration 2>/dev/null | head -20
else
    echo "kubeadm 不可用"
fi

# 6. 证书轮转配置
echo ""
echo "=== 6. 证书轮转配置 ==="
if [ -f "/var/lib/kubelet/config.yaml" ]; then
    grep -E "rotateCertificates|serverTLSBootstrap" /var/lib/kubelet/config.yaml
fi
```

### 7.2 证书更新操作

```bash
#!/bin/bash
# certificate-renewal.sh - 证书更新

echo "=== 证书更新流程 ==="

# 1. 检查当前证书状态
echo ""
echo "=== 1. 当前证书状态 ==="
kubeadm certs check-expiration 2>/dev/null

# 2. 更新所有证书 (Master 节点)
echo ""
echo "=== 2. 更新证书 ==="
echo "在 Master 节点执行: kubeadm certs renew all"

# 3. 重启控制平面组件
echo ""
echo "=== 3. 重启控制平面组件 ==="
echo "执行以下命令重启:"
echo "  kubectl delete pod -n kube-system -l component=kube-apiserver"
echo "  kubectl delete pod -n kube-system -l component=kube-controller-manager"
echo "  kubectl delete pod -n kube-system -l component=kube-scheduler"

# 4. Worker 节点证书更新
echo ""
echo "=== 4. Worker 节点证书更新 ==="
echo "方法 1: 自动轮转 (推荐)"
echo "  确保 kubelet 配置: rotateCertificates: true"
echo ""
echo "方法 2: 手动更新"
echo "  # 删除旧证书"
echo "  rm /var/lib/kubelet/pki/kubelet-client-current.pem"
echo "  # 重启 kubelet"
echo "  systemctl restart kubelet"

# 5. 验证
echo ""
echo "=== 5. 验证 ==="
echo "验证节点状态: kubectl get nodes"
echo "验证证书有效期: kubeadm certs check-expiration"
```

---

## 8. 内核与系统问题

### 8.1 内核问题诊断

```bash
#!/bin/bash
# kernel-diagnosis.sh - 内核问题诊断

echo "=============================================="
echo "  内核与系统诊断报告"
echo "=============================================="

# 1. 内核版本
echo ""
echo "=== 1. 内核信息 ==="
uname -a
echo ""
echo "发行版:"
cat /etc/os-release | grep -E "^NAME=|^VERSION="

# 2. 内核错误
echo ""
echo "=== 2. 内核错误日志 ==="
dmesg | grep -iE "error|fail|panic|bug|oops" | tail -20

# 3. 系统运行时间
echo ""
echo "=== 3. 系统运行时间 ==="
uptime
echo ""
echo "最近重启:"
last reboot | head -5

# 4. 系统负载
echo ""
echo "=== 4. 系统负载 ==="
cat /proc/loadavg
echo ""
echo "CPU 信息:"
lscpu | grep -E "^CPU\(s\)|^Model name"

# 5. cgroup 状态
echo ""
echo "=== 5. cgroup 状态 ==="
if [ -f /sys/fs/cgroup/cgroup.controllers ]; then
    echo "cgroup v2 (unified)"
    echo "控制器: $(cat /sys/fs/cgroup/cgroup.controllers)"
else
    echo "cgroup v1 (legacy)"
    ls /sys/fs/cgroup/
fi

# 6. 系统调用错误
echo ""
echo "=== 6. 系统调用统计 ==="
if [ -f /proc/pressure/cpu ]; then
    echo "CPU 压力:"
    cat /proc/pressure/cpu
fi
if [ -f /proc/pressure/io ]; then
    echo ""
    echo "IO 压力:"
    cat /proc/pressure/io
fi

# 7. 文件描述符
echo ""
echo "=== 7. 文件描述符 ==="
echo "当前打开: $(cat /proc/sys/fs/file-nr | awk '{print $1}')"
echo "最大限制: $(cat /proc/sys/fs/file-max)"

# 8. 进程限制
echo ""
echo "=== 8. 进程限制 ==="
echo "当前进程数: $(ps aux | wc -l)"
echo "PID 最大值: $(cat /proc/sys/kernel/pid_max)"

# 9. 内核参数
echo ""
echo "=== 9. 关键内核参数 ==="
sysctl net.ipv4.ip_forward
sysctl net.bridge.bridge-nf-call-iptables 2>/dev/null
sysctl net.netfilter.nf_conntrack_max 2>/dev/null
sysctl fs.inotify.max_user_watches 2>/dev/null
sysctl fs.inotify.max_user_instances 2>/dev/null
```

### 8.2 cgroup 问题诊断

```bash
#!/bin/bash
# cgroup-diagnosis.sh

echo "=== cgroup 诊断 ==="

# 检测 cgroup 版本
if [ -f /sys/fs/cgroup/cgroup.controllers ]; then
    echo "cgroup 版本: v2"
    
    # kubepods 状态
    echo ""
    echo "kubepods cgroup:"
    if [ -d /sys/fs/cgroup/kubepods ]; then
        echo "  memory.max: $(cat /sys/fs/cgroup/kubepods/memory.max 2>/dev/null)"
        echo "  memory.current: $(cat /sys/fs/cgroup/kubepods/memory.current 2>/dev/null)"
        echo "  cpu.max: $(cat /sys/fs/cgroup/kubepods/cpu.max 2>/dev/null)"
    fi
else
    echo "cgroup 版本: v1"
    
    # kubepods 状态
    echo ""
    echo "kubepods cgroup:"
    if [ -d /sys/fs/cgroup/memory/kubepods ]; then
        echo "  memory.limit: $(numfmt --to=iec < /sys/fs/cgroup/memory/kubepods/memory.limit_in_bytes 2>/dev/null)"
        echo "  memory.usage: $(numfmt --to=iec < /sys/fs/cgroup/memory/kubepods/memory.usage_in_bytes 2>/dev/null)"
    fi
    if [ -d /sys/fs/cgroup/cpu/kubepods ]; then
        echo "  cpu.shares: $(cat /sys/fs/cgroup/cpu/kubepods/cpu.shares 2>/dev/null)"
    fi
fi

# 检查 kubelet cgroup driver 配置
echo ""
echo "kubelet cgroupDriver 配置:"
grep cgroupDriver /var/lib/kubelet/config.yaml 2>/dev/null

echo ""
echo "containerd cgroup driver 配置:"
grep -A 5 "SystemdCgroup" /etc/containerd/config.toml 2>/dev/null
```

---

## 9. ACK/云环境特定问题

### 9.1 ACK 节点诊断

```bash
#!/bin/bash
# ack-node-diagnosis.sh - ACK 节点诊断

echo "=============================================="
echo "  ACK 节点诊断报告"
echo "=============================================="

# 1. 节点基础信息
echo ""
echo "=== 1. 节点信息 ==="
echo "节点 ID:"
curl -s http://100.100.100.200/latest/meta-data/instance-id 2>/dev/null || echo "无法获取"
echo ""
echo "实例类型:"
curl -s http://100.100.100.200/latest/meta-data/instance-type 2>/dev/null || echo "无法获取"
echo ""
echo "可用区:"
curl -s http://100.100.100.200/latest/meta-data/zone-id 2>/dev/null || echo "无法获取"

# 2. 节点池信息
echo ""
echo "=== 2. 节点池信息 ==="
kubectl get node $(hostname) -o jsonpath='{.metadata.labels.alibabacloud\.com/nodepool-id}' 2>/dev/null || echo "无法获取"

# 3. 竞价实例检查
echo ""
echo "=== 3. 竞价实例检查 ==="
SPOT=$(kubectl get node $(hostname) -o jsonpath='{.metadata.labels.alibabacloud\.com/spot-instance}' 2>/dev/null)
if [ "$SPOT" = "true" ]; then
    echo "⚠️ 这是竞价实例，可能被回收"
else
    echo "这是按量/包年包月实例"
fi

# 4. 云监控 Agent
echo ""
echo "=== 4. 云监控 Agent ==="
systemctl status cloudmonitor --no-pager 2>/dev/null | head -5 || echo "cloudmonitor 未安装"

# 5. Terway 网络 (如使用)
echo ""
echo "=== 5. Terway 网络检查 ==="
if [ -f /etc/cni/net.d/10-terway.conf ]; then
    echo "Terway CNI 已配置"
    echo ""
    echo "ENI 网卡:"
    ip addr show | grep -E "^[0-9]+: eth" | head -5
fi

# 6. GPU 检查 (如有)
echo ""
echo "=== 6. GPU 检查 ==="
if command -v nvidia-smi &>/dev/null; then
    nvidia-smi --query-gpu=name,memory.total,memory.used --format=csv 2>/dev/null
else
    echo "未检测到 GPU"
fi

# 7. 节点自愈状态
echo ""
echo "=== 7. 节点状态 ==="
kubectl get node $(hostname) -o wide 2>/dev/null
```

### 9.2 ACK 特定问题解决

| 问题 | 症状 | 原因 | 解决方案 |
|------|------|------|---------|
| **节点自动回收** | 节点突然消失 | 竞价实例被回收 | 使用混合实例策略 |
| **节点池扩容失败** | 节点数不增加 | 库存不足/配额限制 | 检查配额/更换规格 |
| **Terway 网络故障** | Pod 网络不通 | ENI 分配失败 | 检查 VSwitch/安全组 |
| **云盘挂载失败** | PVC Pending | 云盘不在同可用区 | 使用 WaitForFirstConsumer |
| **节点标签丢失** | 节点池标签不生效 | 节点池配置问题 | 重新同步节点池配置 |

```bash
# ACK 常用运维命令

# 查看节点池
kubectl get nodes -o custom-columns='NAME:.metadata.name,POOL:.metadata.labels.alibabacloud\.com/nodepool-id'

# 触发节点修复 (通过 API)
# aliyun cs POST /clusters/{ClusterId}/nodes/{NodeName}/repair

# 节点排水
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 移除节点
kubectl delete node <node-name>

# ACK 诊断工具
# curl -O https://alibabacloud-china.github.io/diagnose-tools/scripts/installer.sh
# bash installer.sh
# ack-diagnose node --cluster-id <cluster-id> --node-name <node-name>
```

---

## 10. 自动化诊断工具

### 10.1 完整诊断脚本

```bash
#!/bin/bash
# node-notready-full-diagnosis.sh - Node NotReady 完整诊断
# 版本: 1.0 | 适用: Kubernetes v1.25-v1.32

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

NODE_NAME=${1:-$(hostname)}
OUTPUT_FILE="node-diagnosis-$(date +%Y%m%d-%H%M%S).txt"

log() { echo -e "${GREEN}[INFO]${NC} $1"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 主诊断函数
diagnose() {
    echo "=============================================="
    echo "  Node NotReady 诊断报告"
    echo "  节点: $NODE_NAME"
    echo "  时间: $(date)"
    echo "=============================================="
    
    # 1. kubelet 状态
    echo ""
    echo "=== 1. kubelet 状态 ==="
    systemctl status kubelet --no-pager 2>/dev/null | head -10
    KUBELET_STATUS=$(systemctl is-active kubelet 2>/dev/null)
    if [ "$KUBELET_STATUS" != "active" ]; then
        error "kubelet 未运行!"
        echo "最近日志:"
        journalctl -u kubelet -n 20 --no-pager 2>/dev/null
    fi
    
    # 2. containerd 状态
    echo ""
    echo "=== 2. containerd 状态 ==="
    systemctl status containerd --no-pager 2>/dev/null | head -10
    CONTAINERD_STATUS=$(systemctl is-active containerd 2>/dev/null)
    if [ "$CONTAINERD_STATUS" != "active" ]; then
        error "containerd 未运行!"
    else
        echo ""
        echo "CRI 状态:"
        timeout 10 crictl info 2>&1 | head -10 || warn "CRI 响应超时"
    fi
    
    # 3. 系统资源
    echo ""
    echo "=== 3. 系统资源 ==="
    echo "--- 内存 ---"
    free -h
    MEM_AVAIL=$(free -m | awk '/^Mem:/{print $7}')
    if [ $MEM_AVAIL -lt 500 ]; then
        warn "可用内存低于 500MB!"
    fi
    
    echo ""
    echo "--- 磁盘 ---"
    df -h | grep -E "^/dev|Filesystem"
    DISK_USE=$(df / | tail -1 | awk '{print $5}' | tr -d '%')
    if [ $DISK_USE -gt 85 ]; then
        warn "根分区使用率超过 85%!"
    fi
    
    # 4. 网络连通性
    echo ""
    echo "=== 4. 网络连通性 ==="
    API_SERVER=$(grep "server:" /etc/kubernetes/kubelet.conf 2>/dev/null | awk '{print $2}')
    if [ -n "$API_SERVER" ]; then
        echo "API Server: $API_SERVER"
        if curl -sk --connect-timeout 5 "${API_SERVER}/healthz" &>/dev/null; then
            log "API Server 可达"
        else
            error "API Server 不可达!"
        fi
    fi
    
    # 5. 证书状态
    echo ""
    echo "=== 5. 证书状态 ==="
    CERT="/var/lib/kubelet/pki/kubelet-client-current.pem"
    if [ -f "$CERT" ]; then
        EXPIRY=$(openssl x509 -in "$CERT" -noout -enddate 2>/dev/null | cut -d= -f2)
        echo "证书过期时间: $EXPIRY"
        EXPIRY_EPOCH=$(date -d "$EXPIRY" +%s 2>/dev/null)
        NOW_EPOCH=$(date +%s)
        DAYS_LEFT=$(( (EXPIRY_EPOCH - NOW_EPOCH) / 86400 ))
        if [ $DAYS_LEFT -lt 0 ]; then
            error "证书已过期!"
        elif [ $DAYS_LEFT -lt 30 ]; then
            warn "证书将在 $DAYS_LEFT 天后过期"
        else
            log "证书有效期: $DAYS_LEFT 天"
        fi
    fi
    
    # 6. 错误日志汇总
    echo ""
    echo "=== 6. 错误日志汇总 ==="
    echo "--- kubelet 错误 ---"
    journalctl -u kubelet --since "30 minutes ago" --no-pager 2>/dev/null | grep -iE "error|fail" | tail -10
    
    echo ""
    echo "--- 内核错误 ---"
    dmesg | grep -iE "error|fail|panic|oom" | tail -10
    
    # 7. 诊断建议
    echo ""
    echo "=== 7. 诊断建议 ==="
    if [ "$KUBELET_STATUS" != "active" ]; then
        echo "1. 重启 kubelet: systemctl restart kubelet"
    fi
    if [ "$CONTAINERD_STATUS" != "active" ]; then
        echo "2. 重启 containerd: systemctl restart containerd"
    fi
    if [ $MEM_AVAIL -lt 500 ]; then
        echo "3. 清理内存: echo 1 > /proc/sys/vm/drop_caches"
    fi
    if [ $DISK_USE -gt 85 ]; then
        echo "4. 清理磁盘: crictl rmi --prune"
    fi
    
    echo ""
    echo "=============================================="
    echo "  诊断完成"
    echo "=============================================="
}

# 执行诊断
diagnose 2>&1 | tee "$OUTPUT_FILE"
log "诊断报告已保存: $OUTPUT_FILE"
```

---

## 11. 监控告警配置

### 11.1 Prometheus 告警规则

```yaml
# node-notready-alerts.yaml
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
    # 节点 NotReady
    - alert: NodeNotReady
      expr: kube_node_status_condition{condition="Ready",status="true"} == 0
      for: 2m
      labels:
        severity: critical
        category: infrastructure
      annotations:
        summary: "节点 {{ $labels.node }} NotReady"
        description: "节点已 NotReady 超过 2 分钟"
        runbook_url: "https://wiki.example.com/runbooks/node-notready"
    
    # 节点 Unknown
    - alert: NodeUnknown
      expr: kube_node_status_condition{condition="Ready",status="unknown"} == 1
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "节点 {{ $labels.node }} 状态 Unknown"
        description: "可能存在网络分区或 kubelet 故障"
    
    # 内存压力
    - alert: NodeMemoryPressure
      expr: kube_node_status_condition{condition="MemoryPressure",status="true"} == 1
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.node }} 内存压力"
    
    # 磁盘压力
    - alert: NodeDiskPressure
      expr: kube_node_status_condition{condition="DiskPressure",status="true"} == 1
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.node }} 磁盘压力"
    
    # PID 压力
    - alert: NodePIDPressure
      expr: kube_node_status_condition{condition="PIDPressure",status="true"} == 1
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.node }} PID 压力"

  - name: kubelet.health
    rules:
    # kubelet 不可用
    - alert: KubeletDown
      expr: up{job="kubelet"} == 0
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "kubelet {{ $labels.instance }} 不可用"
    
    # PLEG 延迟高
    - alert: KubeletPLEGDurationHigh
      expr: |
        histogram_quantile(0.99, sum(rate(kubelet_pleg_relist_duration_seconds_bucket[5m])) by (instance, le)) > 3
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "kubelet {{ $labels.instance }} PLEG 延迟高"
        description: "PLEG P99 延迟: {{ $value | printf \"%.2f\" }}s"

  - name: node.resources
    rules:
    # CPU 使用率高
    - alert: NodeCPUHigh
      expr: 100 - (avg by(instance) (rate(node_cpu_seconds_total{mode="idle"}[5m])) * 100) > 85
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.instance }} CPU 高"
        description: "CPU 使用率: {{ $value | printf \"%.1f\" }}%"
    
    # 内存使用率高
    - alert: NodeMemoryHigh
      expr: (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) * 100 > 85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.instance }} 内存高"
        description: "内存使用率: {{ $value | printf \"%.1f\" }}%"
    
    # 磁盘使用率高
    - alert: NodeDiskHigh
      expr: (1 - node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"}) * 100 > 85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "节点 {{ $labels.instance }} 磁盘高"
        description: "根分区使用率: {{ $value | printf \"%.1f\" }}%"
```

### 11.2 关键监控指标

| 指标 | 含义 | 健康基准 | 告警阈值 |
|------|------|---------|---------|
| `kube_node_status_condition{condition="Ready"}` | 节点 Ready 状态 | status=true | status!=true for 2m |
| `kubelet_node_status_update_interval_seconds` | 节点状态更新间隔 | < 15s | > 30s |
| `kubelet_pleg_relist_duration_seconds` | PLEG 重列延迟 | P99 < 1s | P99 > 3s |
| `node_memory_MemAvailable_bytes` | 可用内存 | > 20% | < 10% |
| `node_filesystem_avail_bytes` | 可用磁盘 | > 15% | < 10% |
| `up{job="kubelet"}` | kubelet 存活 | 1 | 0 |

---

## 12. 节点恢复操作

### 12.1 标准恢复流程

```bash
#!/bin/bash
# node-recovery.sh

NODE_NAME=$1
if [ -z "$NODE_NAME" ]; then
    echo "用法: $0 <node-name>"
    exit 1
fi

echo "=== 节点 $NODE_NAME 恢复流程 ==="

# 1. 隔离节点
echo ""
echo "=== 1. 隔离节点 ==="
kubectl cordon $NODE_NAME

# 2. 检查并修复
echo ""
echo "=== 2. 请 SSH 到节点执行修复 ==="
cat << 'EOF'
# 在节点上执行:

# 检查并重启 kubelet
systemctl status kubelet
systemctl restart kubelet

# 检查并重启 containerd
systemctl status containerd
systemctl restart containerd

# 清理资源
crictl rmi --prune
crictl rm $(crictl ps -aq --state exited)

# 检查资源
free -h
df -h
EOF

# 3. 等待恢复
echo ""
echo "=== 3. 等待节点恢复 ==="
for i in {1..30}; do
    STATUS=$(kubectl get node $NODE_NAME -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}' 2>/dev/null)
    if [ "$STATUS" = "True" ]; then
        echo "节点已恢复 Ready!"
        break
    fi
    echo "等待中... ($i/30)"
    sleep 10
done

# 4. 取消隔离
echo ""
echo "=== 4. 取消隔离 ==="
kubectl uncordon $NODE_NAME

# 5. 验证
echo ""
echo "=== 5. 最终状态 ==="
kubectl get node $NODE_NAME
kubectl get pods -A -o wide --field-selector spec.nodeName=$NODE_NAME | head -20
```

### 12.2 紧急重启流程

```bash
#!/bin/bash
# emergency-reboot.sh

NODE_NAME=$1
echo "⚠️ 紧急重启节点: $NODE_NAME"
read -p "确认? (yes/no): " CONFIRM
[ "$CONFIRM" != "yes" ] && exit 0

# 1. 驱逐 Pod
kubectl drain $NODE_NAME --ignore-daemonsets --delete-emptydir-data --force --grace-period=30 --timeout=5m

# 2. 重启节点
echo "请 SSH 到节点执行: sudo reboot"
echo "或通过云平台控制台重启"

# 3. 等待恢复
echo "等待节点恢复..."
for i in {1..60}; do
    STATUS=$(kubectl get node $NODE_NAME -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}' 2>/dev/null)
    [ "$STATUS" = "True" ] && break
    sleep 10
done

# 4. 取消隔离
kubectl uncordon $NODE_NAME
kubectl get node $NODE_NAME
```

---

## 13. 版本特定变更

### 13.1 节点管理功能演进

| 版本 | 特性 | 状态 | 影响 |
|------|------|------|------|
| **v1.24** | 移除 Dockershim | GA | 必须使用 containerd/CRI-O |
| **v1.25** | Lease API 稳定版 | GA | 心跳机制更可靠 |
| **v1.26** | 节点日志 API | Beta | 可远程获取 kubelet 日志 |
| **v1.27** | GracefulNodeShutdown | GA | 优雅关机支持 |
| **v1.27** | 改进节点状态上报 | - | 更精确的状态检测 |
| **v1.28** | SidecarContainers | Beta | 影响 Pod 生命周期 |
| **v1.29** | 节点内存交换支持 | Beta | 新的内存管理选项 |
| **v1.30** | 用户命名空间支持 | Beta | 增强安全隔离 |
| **v1.31** | RecoverVolumeExpansionFailure | Alpha | 卷扩展故障恢复 |
| **v1.32** | DRA GA 阶段推进 | Beta | 动态资源分配增强 |

### 13.2 版本特定配置

```yaml
# v1.27+ 优雅关机配置
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
shutdownGracePeriod: 60s
shutdownGracePeriodCriticalPods: 20s
shutdownGracePeriodByPodPriority:
- priority: 2000000000  # system-cluster-critical
  shutdownGracePeriodSeconds: 20
- priority: 1000000000  # system-node-critical
  shutdownGracePeriodSeconds: 15
- priority: 0
  shutdownGracePeriodSeconds: 10

# v1.29+ 内存交换支持 (实验性)
# memorySwap:
#   swapBehavior: LimitedSwap
```

---

## 14. 多角色视角

### 14.1 架构师视角

| 关注点 | 设计要点 | 推荐配置 |
|-------|---------|---------|
| **高可用** | 多节点冗余 | 至少 3 Worker 节点 |
| **资源预留** | 系统资源保护 | systemReserved + kubeReserved |
| **故障隔离** | 节点组/节点池 | 不同业务使用不同节点池 |
| **自动恢复** | 节点自愈 | ACK 节点自愈 / Cluster Autoscaler |
| **监控覆盖** | 全面监控 | Node Exporter + kubelet metrics |

### 14.2 测试工程师视角

| 测试类型 | 测试目标 | 测试方法 |
|---------|---------|---------|
| **节点故障测试** | 验证故障检测时间 | 强制停止 kubelet |
| **恢复测试** | 验证自动恢复 | 模拟后恢复节点 |
| **驱逐测试** | 验证 Pod 迁移 | 触发节点 NotReady |
| **资源压力测试** | 验证驱逐策略 | 消耗内存/磁盘 |
| **混沌测试** | 验证整体稳定性 | ChaosBlade 节点故障 |

### 14.3 产品经理视角

| SLA 指标 | 目标值 | 监控方式 |
|---------|-------|---------|
| **节点可用率** | > 99.9% | 节点 Ready 时间比例 |
| **故障检测时间** | < 1min | NotReady 告警触发时间 |
| **故障恢复时间** | < 5min | NotReady 到 Ready 时间 |
| **Pod 迁移完成时间** | < 10min | Pod 重新调度完成时间 |

---

## 15. 最佳实践

### 15.1 预防措施清单

- [ ] **监控配置**
  - [ ] 部署 Node Exporter
  - [ ] 配置节点状态告警 (NotReady/Unknown)
  - [ ] 配置资源使用告警 (CPU/Memory/Disk)
  - [ ] 配置 kubelet 健康告警

- [ ] **资源管理**
  - [ ] 配置 systemReserved 和 kubeReserved
  - [ ] 配置合理的驱逐阈值
  - [ ] 磁盘使用率 80% 告警
  - [ ] 定期清理无用镜像和容器

- [ ] **证书管理**
  - [ ] 启用证书自动轮转
  - [ ] 配置证书过期监控 (30 天前告警)
  - [ ] 定期检查证书状态

- [ ] **运维准备**
  - [ ] 文档化故障处理流程 (Runbook)
  - [ ] 定期演练恢复流程
  - [ ] 配置节点自愈 (如 ACK 节点自愈)
  - [ ] 准备诊断脚本

### 15.2 快速诊断检查表

```
□ 能 SSH 登录节点?
  ├── Yes → 继续
  └── No  → 检查网络/云平台

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

### 15.3 相关文档

| 主题 | 文档编号 | 说明 |
|------|---------|------|
| Pod Pending 诊断 | 102-pod-pending-diagnosis | Pod 调度问题 |
| OOM 诊断 | 104-oom-memory-diagnosis | 内存问题 |
| kubelet 深度解析 | 39-kubelet-deep-dive | kubelet 架构 |
| containerd 配置 | 15-container-runtime | 容器运行时 |
| 证书管理 | 77-certificate-management | 证书操作 |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com) | 最后更新: 2026-01 | 版本: v1.25-v1.32
