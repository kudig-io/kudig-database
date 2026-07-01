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



# Troubleshoot Node Issues

## Diagnostic Workflow

### Step 1: Check Node Status

```bash
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

```bash
systemctl status kubelet
journalctl -u kubelet --since "10 minutes ago"
```

Common kubelet issues:
- Certificate expiration (check `--tls-cert-file`)
- API Server connectivity failure
- CRI socket not responding
- cgroup driver mismatch with container runtime

### Step 3: Check Container Runtime

```bash
systemctl status containerd
crictl ps    # Check running containers
crictl images  # Check available images
```

### Step 4: Check Networking

```bash
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

```bash
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
