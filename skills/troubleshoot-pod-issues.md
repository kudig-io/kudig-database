---
title: Troubleshoot Pod Issues
description: Troubleshoot Pod Issues — Kubernetes 生产运维知识库
summary: Troubleshoot Pod Issues — Kubernetes 生产运维知识库
category: skills
tags:
- k8s
- troubleshooting
- pod
- pending
- crashloop
- debugging
- diagnosis
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Troubleshoot Pod Issues 是什么
- 如何 Troubleshoot Pod Issues
- Troubleshoot Pod Issues 故障排查
- Troubleshoot Pod Issues 排障步骤
trigger_keywords:
- Troubleshoot
- Pod
- Issues
prerequisites:
- kubectl-basics
---



# Troubleshoot Pod Issues

## Diagnostic Workflow

### Step 1: Check Pod Status

```bash
kubectl get pods -o wide
kubectl describe pod <pod-name>
kubectl get pod <pod-name> -o jsonpath='{.status.conditions}'
```

Key indicators:
- **Phase**: Pending, Running, Failed, Unknown
- **Conditions**: PodScheduled, Initialized, ContainersReady, Ready
- **Container Status**: waiting, running, terminated (with reason/exit code)

### Step 2: Analyze Events

```bash
kubectl describe pod <pod-name> | grep -A5 Events
kubectl get events --sort-by='.lastTimestamp'
```

Common events and their meaning:
- `FailedScheduling`: No suitable node found (check resources, taints, node selectors)
- `PullBackOff` / `ErrImagePull`: Image pull failure (check image name, tag, registry credentials)
- `CrashLoopBackOff`: Container keeps crashing (check logs, resource limits, application errors)

### Step 3: Check Logs

```bash
kubectl logs <pod-name>                    # Current container
kubectl logs <pod-name> --previous          # Previous crashed container
kubectl logs <pod-name> -c <container-name> # Specific container
```

### Step 4: Common Issues

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| **Pending** | Insufficient resources, taints, PVC unbound | Check `kubectl describe pod`, increase resources, fix PVC |
| **CrashLoopBackOff** | App crash, bad config, OOM | Check logs, validate config, increase memory limits |
| **ImagePullBackOff** | Wrong image/tag, no pull secret | Fix image reference, create imagePullSecret |
| **OOMKilled** | Memory limit too low, memory leak | Increase limits, fix leak, configure JVM -XX:MaxRAMPercentage |
| **Init:Error** | Init container failure | Check init container logs, fix init logic |

### Step 5: Debug with Ephemeral Containers

```bash
kubectl debug -it <pod-name> --image=busybox --target=<container-name>
```

## JVM-Specific Guidance

For Java applications in containers:
- Set `-XX:MaxRAMPercentage=75.0` to limit JVM heap to container memory
- Set `-XX:+UseContainerSupport` (enabled by default since Java 10)
- Account for off-heap memory: Metaspace, direct buffers, thread stacks, code cache

## Related

- [[skills/monitor-kubernetes-metrics.md|monitor-kubernetes-metrics]] — [[skills/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]
- [[skills/configure-health-probes.md|configure-health-probes]] — [[skills/configure-health-probes.md|Configure Health Probes]]
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[concepts/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[skills/skill-reference-diagnostic-workflow.md|skill-reference-diagnostic-workflow]] — Diagnostic Workflow
- [[pod-lifecycle|Pod Lifecycle]]
- [[concepts/resource-management.md|Resource Management]]
- [[skills/configure-health-probes.md|Configure Health Probes]]
- [[skills/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]
- [[concepts/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]
