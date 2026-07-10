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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Troubleshoot Pod Issues

## Diagnostic Workflow

### Step 1: Check Pod Status

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -o wide
kubectl describe pod <pod-name>
kubectl get pod <pod-name> -o jsonpath='{.status.conditions}'
```
Key indicators:
- **Phase**: Pending, Running, Failed, Unknown
- **Conditions**: PodScheduled, Initialized, ContainersReady, Ready
- **Container Status**: waiting, running, terminated (with reason/exit code)

### Step 2: Analyze Events

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe pod <pod-name> | grep -A5 Events
kubectl get events --sort-by='.lastTimestamp'
```
Common events and their meaning:
- `FailedScheduling`: No suitable node found (check resources, taints, node selectors)
- `PullBackOff` / `ErrImagePull`: Image pull failure (check image name, tag, registry credentials)
- `CrashLoopBackOff`: Container keeps crashing (check logs, resource limits, application errors)

### Step 3: Check Logs

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl debug -it <pod-name> --image=busybox --target=<container-name>
```
## JVM-Specific Guidance

For Java applications in containers:
- Set `-XX:MaxRAMPercentage=75.0` to limit JVM heap to container memory
- Set `-XX:+UseContainerSupport` (enabled by default since Java 10)
- Account for off-heap memory: Metaspace, direct buffers, thread stacks, code cache

## Related

- [[技能/monitor-kubernetes-metrics.md|monitor-kubernetes-metrics]] — [[技能/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]
- [[技能/configure-health-probes.md|configure-health-probes]] — [[技能/configure-health-probes.md|Configure Health Probes]]
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[技能/skill-reference-diagnostic-workflow.md|skill-reference-diagnostic-workflow]] — Diagnostic Workflow
- [[pod-lifecycle|Pod Lifecycle]]
- [[概念/resource-management.md|Resource Management]]
- [[技能/configure-health-probes.md|Configure Health Probes]]
- [[技能/monitor-kubernetes-metrics.md|Monitor Kubernetes Metrics]]
- [[概念/Symptom-SOP-RootCause Mapping.md|Symptom-SOP-RootCause Mapping]]
- [[技能/FTA Diagnostic Execution Engine.md|FTA Diagnostic Execution Engine]]


<!-- risk-assessed -->
