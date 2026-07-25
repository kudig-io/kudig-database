---
title: Troubleshoot Node Issues
description: Troubleshoot Node Issues — Kubernetes 生产运维知识库
summary: Troubleshoot Node Issues — Kubernetes 生产运维知识库
category: skills
tags:
- k8s
- troubleshooting
- node
- notready
- diagnosis
- kubelet
- apiserver
- containerd
- daemonset
- rag
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Troubleshoot Node Issues 是什么
- 如何 Troubleshoot Node Issues
- Troubleshoot Node Issues 故障排查
- Troubleshoot Node Issues 排障步骤
trigger_keywords:
- Troubleshoot
- Node
- Issues
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Troubleshoot Node Issues

## Diagnostic Workflow

### Step 1: Check Node Status

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get nodes
kubectl describe node <node-name>
kubectl get node <node-name> -o jsonpath='{.status.conditions}'
```
Key conditions:
- **Ready**: Node is healthy and accepting [[Pods|Pods]]
- **MemoryPressure**: [[kubelet|kubelet]] will evict Pods
- **DiskPressure**: kubelet will evict Pods
- **PIDPressure**: Too many processes

### Step 2: Check kubelet

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
systemctl status kubelet
journalctl -u kubelet --since "10 minutes ago"
```
Common kubelet issues:
- Certificate expiration (check `--tls-cert-file`)
- API Server connectivity failure
- CRI socket not responding
- cgroup driver mismatch with container runtime

### Step 3: Check Container Runtime

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
systemctl status containerd
crictl ps    # Check running containers
crictl images  # Check available images
```
### Step 4: Check Networking

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Verify CNI plugin is running
kubectl get pods -n kube-system | grep <cni-name>

# Check node network connectivity
ping <other-node-ip>
curl -k https://<apiserver-ip>:6443/healthz
```
### Step 5: Check Resources

```bash
df -h           # Disk space (especially /var/lib/kubelet and /var/lib/containerd)
free -m         # Memory
ls /var/log/containers/  # Disk I/O from container logs
```

### Step 6: Check Certificates

```bash
kubeadm certs check-expiration
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -text -noout | grep -A2 "Validity"
```

## Recovery Actions

| Issue | Recovery |
|-------|----------|
| kubelet stopped | `systemctl restart kubelet` |
| Certificate expired | `kubeadm certs renew all` then restart kubelet |
| Disk full | Clean up old images, logs, or add storage |
| CNI failure | Restart CNI Pods, check CNI config |
| Runtime crash | `systemctl restart containerd` |
| Node unresponsive | Drain node, replace it |

## Node Drain and Replace

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

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
kubectl cordon <node-name>        # Mark unschedulable
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
# Fix the issue, then:
kubectl uncordon <node-name>
```
## 生产案例

### 案例 1: 节点 NotReady——kubelet 进程挂起

| 时间 | 事件 |
|------|------|
| 03:00 | 监控告警: 节点 NotReady |
| 03:05 | SSH 登录，`systemctl status kubelet` 显示 inactive(dead) |
| 03:08 | `journalctl -u kubelet` 显示 "PLEG is not healthy" |
| 03:10 | 🔴 `systemctl restart kubelet`，节点恢复 |

**根因**: 容器运行时(containerd)响应慢，PLEG 超时导致 kubelet 自杀。

### 案例 2: 节点时间偏移导致证书验证失败

**现象**: 节点 NotReady，kubelet 日志 "x509: certificate has expired or is not yet valid"。

**诊断**: `date` 显示节点时间偏差 10min，NTP 服务停止

**修复**: 🟡 重启 chronyd/ntpd 同步时间

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 多节点同时 NotReady | 检查控制平面和网络 |
| P1 | 单节点 NotReady | SSH 排查 kubelet |
| P2 | 节点性能下降 | 检查资源使用 |

## 面试要点

1. **Q: 节点 NotReady 的排查路径？**
   A: ① `kubectl describe node` 查看 Conditions ② SSH 检查 kubelet 状态 ③ `journalctl -u kubelet` 查看日志 ④ 检查容器运行时 ⑤ 检查网络连通性 ⑥ 检查证书和时间。

2. **Q: PLEG 是什么？如何排查？**
   A: PLEG(Pod Lifecycle Event Generator) 负责检测容器状态变化。PLEG 不健康通常因为容器运行时响应慢。排查: `crictl ps`、`journalctl -u containerd`、检查磁盘 I/O。

3. **Q: 节点维护的标准流程？**
   A: ① `kubectl cordon node`(禁止调度) ② `kubectl drain node --ignore-daemonsets --delete-emptydir-data`(驱逐 Pod) ③ 执行维护 ④ `kubectl uncordon node`(恢复调度) ⑤ 验证 Pod 重新调度。

## Related

- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[cni]] — CNI (Container Network Interface)
- [[22-概念/07-调度与资源/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[26-技能/04-工作负载/pod/方法论/skill-reference-diagnostic-workflow.md|skill-reference-diagnostic-workflow]] — Diagnostic Workflow
- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]]
- [[23-实体/02-K8s核心组件/container-runtime.md|Container Runtime]]
- [[22-概念/07-调度与资源/resource-management.md|Resource Management]]
- [[26-技能/04-工作负载/pod/诊断排障/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]
- [[22-概念/08-可靠性与运维/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[26-技能/04-工作负载/pod/方法论/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[26-技能/03-节点/node-fta.md|Node 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
