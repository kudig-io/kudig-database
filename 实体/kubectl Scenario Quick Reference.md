---
title: kubectl Scenario Quick Reference
description: kubectl Scenario Quick Reference — Kubernetes 生产运维知识库
summary: kubectl Scenario Quick Reference — Kubernetes 生产运维知识库
category: reference
tags:
- k8s
- kubectl
- cheatsheet
- troubleshooting
- kubelet
- daemonset
tier: core
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kubectl Scenario Quick Reference

> Organized by **fault scenario** (not resource type) for on-call engineers.
> Compatible with Kubernetes v1.28 - v1.33.

## Node Fault Scenarios

### Node NotReady / Unknown

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
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
ssh <node-ip> "df -h / /var/lib/kubelet"                     # Check disk
ssh <node-ip> "free -h"                                      # Check memory
kubectl top nodes                                            # Resource usage

# Quick fix: Evict pods
kubectl cordon <node-name> && kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
```
### Batch Node Maintenance

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

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
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

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get pods -o wide                                     # Check restart count
kubectl describe pod <pod-name> | grep -A15 "Events:"        # Crash reason
kubectl logs <pod-name> --previous                           # Previous container logs
kubectl logs <pod-name> -c <container-name>                  # Specific container

# Quick fix (low risk)
kubectl rollout restart deployment <deploy-name> -n <namespace>
```
### Pod OOMKilled (Exit Code 137)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl describe pod <pod-name> | grep -A10 "Last State"
kubectl top pods

# Fix: Increase memory limits
kubectl patch deployment <deploy-name> -n <namespace> --patch \
  '{"spec":{"template":{"spec":{"containers":[{"name":"app","resources":{"limits":{"memory":"2Gi"},"requests":{"memory":"1Gi"}}}]}}}}'
```
### Pod ImagePullBackOff

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl describe pod <pod-name> | grep -A10 "ImagePull"
# Check: image name, tag, registry auth, network to registry
kubectl get secret <image-pull-secret> -n <namespace> -o yaml
```
## Cluster Health Checks

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

- [[实体/kubectl-quick-reference.md|kubectl-quick-reference]] — Kubectl Quick Reference
- [[实体/fta-febm-methodology.md|fta-febm-methodology]] — 故障树分析（FTA）与取证循证方法论（FEBM）
- [[deployment]] — Deployment
- [[实体/kubelet.md|kubelet]] — kubelet
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[技能/fta-方法论/top-events-index/Kubernetes FTA Top Events Index.md|Kubernetes FTA Top Events Index]]
- [[技能/fta-方法论/diagnostic-overview/Kubernetes Diagnostic Skills Overview.md|Kubernetes Diagnostic Skills Overview]]
- [[实体/KUDIG Tag Dictionary.md|KUDIG Tag Dictionary]]
- [[实体/KUDIG Scenario Taxonomy.md|KUDIG Scenario Taxonomy]]


<!-- risk-assessed -->
