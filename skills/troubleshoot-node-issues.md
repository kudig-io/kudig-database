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
## Related

- [[entities/kubelet.md|kubelet]] — kubelet
- [[containerd]] — containerd
- [[cni]] — CNI (Container Network Interface)
- [[concepts/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[skills/skill-reference-diagnostic-workflow.md|skill-reference-diagnostic-workflow]] — Diagnostic Workflow
- [[entities/kubelet.md|kubelet]]
- [[entities/container-runtime.md|Container Runtime]]
- [[concepts/resource-management.md|Resource Management]]
- [[skills/troubleshoot-pod-issues.md|Troubleshoot Pod Issues]]
- [[concepts/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[skills/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[skills/node-fta.md|Node 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
