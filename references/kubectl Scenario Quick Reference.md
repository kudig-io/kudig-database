---
title: kubectl Scenario Quick Reference
description: kubectl Scenario Quick Reference — Kubernetes 生产运维知识库
category: reference
tags:
- k8s
- kubectl
- cheatsheet
- troubleshooting
- kubelet
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubectl Scenario Quick Reference 是什么
- 如何 kubectl Scenario Quick Reference
trigger_keywords:
- kubectl
- Scenario
- Quick
- Reference
prerequisites:
- kubectl-basics
created: "2026-05-23"
---

# kubectl Scenario Quick Reference

> Organized by **fault scenario** (not resource type) for on-call engineers.
> Compatible with Kubernetes v1.28 - v1.33.

## Node Fault Scenarios

### Node NotReady / Unknown

```bash
# 3-step diagnosis
kubectl get nodes -o wide                                    # Step 1: Check node status
kubectl describe node <node-name>                            # Step 2: View conditions and events
ssh <node-ip> "sudo journalctl -u kubelet --since 30m | tail -50"  # Step 3: kubelet logs

# Quick fix (low risk)
kubectl uncordon <node-name>                                 # After recovery

# Fix (medium risk, needs approval)
ssh <node-ip> "sudo systemctl restart kubelet"
```

### Node Disk/Memory Pressure

```bash
ssh <node-ip> "df -h / /var/lib/kubelet"                     # Check disk
ssh <node-ip> "free -h"                                      # Check memory
kubectl top nodes                                            # Resource usage

# Quick fix: Evict pods
kubectl cordon <node-name> && kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
```

### Batch Node Maintenance

```bash
# Before maintenance
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets --grace-period=60 --timeout=300s

# After maintenance
ssh <node-ip> "sudo reboot"
sleep 30 && kubectl get nodes <node-name>
kubectl uncordon <node-name>
```

## Pod Fault Scenarios

### Pod Pending (Scheduling Failure)

```bash
kubectl get pods -o wide
kubectl describe pod <pod-name> | grep -A20 "Events:"

# Common causes and fixes:
# Cause 1: Insufficient resources
kubectl describe nodes | grep -A5 "Allocated resources"

# Cause 2: Taint not tolerated
kubectl get nodes -o jsonpath='{.items[*].spec.taints}'

# Cause 3: nodeSelector mismatch
kubectl label node <node-name> <label-key>=<value>
```

### Pod CrashLoopBackOff / Error

```bash
kubectl get pods -o wide                                     # Check restart count
kubectl describe pod <pod-name> | grep -A15 "Events:"        # Crash reason
kubectl logs <pod-name> --previous                           # Previous container logs
kubectl logs <pod-name> -c <container-name>                  # Specific container

# Quick fix (low risk)
kubectl rollout restart deployment <deploy-name> -n <namespace>
```

### Pod OOMKilled (Exit Code 137)

```bash
kubectl describe pod <pod-name> | grep -A10 "Last State"
kubectl top pods

# Fix: Increase memory limits
kubectl patch deployment <deploy-name> -n <namespace> --patch \
  '{"spec":{"template":{"spec":{"containers":[{"name":"app","resources":{"limits":{"memory":"2Gi"},"requests":{"memory":"1Gi"}}}]}}}}'
```

### Pod ImagePullBackOff

```bash
kubectl describe pod <pod-name> | grep -A10 "ImagePull"
# Check: image name, tag, registry auth, network to registry
kubectl get secret <image-pull-secret> -n <namespace> -o yaml
```

## Cluster Health Checks

```bash
# Health endpoints (v1.25+)
kubectl get --raw='/readyz?verbose' | jq
kubectl get --raw='/livez?verbose' | jq

# API resources and versions
kubectl api-resources
kubectl api-versions

# Component status (deprecated in v1.19+, use /livez instead)
# kubectl get componentstatuses  # DO NOT USE
```

## Version Compatibility Notes

- `kubectl version --short` deprecated in v1.28+, use `--output=yaml`
- `kubectl get componentstatuses` deprecated in v1.19+, use `/livez` `/readyz` APIs
- `kubectl top` requires metrics-server v0.6.0+

## Related

- [[references/kubectl-quick-reference.md|kubectl-quick-reference]] — Kubectl Quick Reference
- [[references/fta-febm-methodology.md|fta-febm-methodology]] — 故障树分析（FTA）与取证循证方法论（FEBM）
- [[deployment]] — Deployment
- [[entities/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[skills/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[skills/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]]
- [[references/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]
- [[references/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]]
