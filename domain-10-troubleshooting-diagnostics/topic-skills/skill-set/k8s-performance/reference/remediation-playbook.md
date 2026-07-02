---
title: Performance Bottleneck Remediation Playbook
summary: Performance Bottleneck Remediation Playbook：kubectl top pod <pod> -n <namespace>
  kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.containers[0].resources}'
category: remediation
tags:
- reference
- remediation
- playbook
- visibility/public
tier: supporting
created: '2026-05-22'
updated: '2026-05-22'
skill_set: k8s-performance
last_updated: 2026-05-22
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-PERF-001 v1.0 — Performance Bottleneck 诊断与修复

## 目录

- [风险级别说明](#风险级别说明)
- [修复操作](#修复操作)
  - [🟢 低风险](#-低风险)
    - [REM-001 调整 CPU limit/request](#rem-001)
    - [REM-002 调整内存 limit/request](#rem-002)
    - [REM-005 IO 优化](#rem-005)
  - [🟡 中风险](#-中风险)
    - [REM-003 节点扩容/调度调整](#rem-003)
    - [REM-004 应用级优化](#rem-004)
    - [REM-006 网络优化](#rem-006)
- [验证确认](#验证确认)
- [升级协议](#升级协议)

## 风险级别说明

| 风险级别 | 标识 | 含义 | Agent 行为 |
|---------|------|------|-----------|
| 低风险 | 🟢 | 资源调整 | 可建议自动执行 |
| 中风险 | 🟡 | 节点/应用变更 | 建议操作并等待人工审批 |

## 修复操作

### 🟢 低风险

#### REM-001: 调整 CPU limit/request

- **适用根因**: RC-001
- **前置检查**:
  ```bash
  kubectl top pod <pod> -n <namespace>
  kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.containers[0].resources}'
  # 查询 Prometheus: rate(container_cpu_cfs_throttled_seconds_total{pod="<pod>"}[5m])
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 增加 CPU limit
  kubectl patch deployment <name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/template/spec/containers/0/resources/limits/cpu", "value": "2"},
   {"op": "replace", "path": "/spec/template/spec/containers/0/resources/requests/cpu", "value": "500m"}]'

  # 或使用 kubectl set resources
  kubectl set resources deployment <name> -n <namespace> -c <container> \
    --limits=cpu=2,memory=2Gi --requests=cpu=500m,memory=1Gi
  ```
- **后置验证**:
  ```bash
  kubectl top pod <pod> -n <namespace>
  # 观察 CPU 使用率是否下降，throttle 是否消失
  ```

#### REM-002: 调整内存 limit/request

- **适用根因**: RC-002
- **前置检查**:
  ```bash
  kubectl top pod <pod> -n <namespace>
  kubectl get pod <pod> -n <namespace> -o jsonpath='{.spec.containers[0].resources}'
  kubectl get events -n <namespace> --field-selector reason=OOMKilling
  ```
- **执行命令**:
  ```bash
  # 增加内存 limit
  kubectl set resources deployment <name> -n <namespace> -c <container> \
    --limits=memory=4Gi --requests=memory=2Gi
  ```
- **后置验证**:
  ```bash
  kubectl get pod <pod> -n <namespace>
  # 预期: 无 OOMKilled，Restart 次数不再增加
  ```

#### REM-005: IO 优化

- **适用根因**: RC-005
- **前置检查**:
  ```bash
  # 在节点上
  iostat -x 1 5
  kubectl top node <node>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案 A: 使用更快存储类
  kubectl patch pvc <name> -n <namespace> -p '{"spec":{"storageClassName":"fast-ssd"}}'

  # 方案 B: 增加 emptyDir 内存挂载（临时数据）
  kubectl patch deployment <name> -n <namespace> --type='json' -p='
  [{"op": "add", "path": "/spec/template/spec/volumes/-", "value":
    {"name":"tmp","emptyDir":{"medium":"Memory"}}}]'
  ```
- **后置验证**:
  ```bash
  kubectl get pvc -n <namespace>
  # 应用 IO 指标改善
  ```

### 🟡 中风险

#### REM-003: 节点扩容/调度调整

- **适用根因**: RC-003
- **前置检查**:
  ```bash
  kubectl top nodes
  kubectl describe node <node>
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 方案 A: 添加新节点（云环境）
  # 通过 Cluster Autoscaler 或手动添加节点

  # 方案 B: 将 Pod 调度到其他节点
  kubectl cordon <overloaded-node>
  kubectl delete pod <pod> -n <namespace>
  # Pod 会重新调度到可用节点

  # 方案 C: 使用 pod anti-affinity 分散负载
  kubectl patch deployment <name> -n <namespace> --type='json' -p='
  [{"op": "add", "path": "/spec/template/spec/affinity", "value":
    {"podAntiAffinity":{"preferredDuringSchedulingIgnoredDuringExecution":[{"weight":100,"podAffinityTerm":{"labelSelector":{"matchExpressions":[{"key":"app","operator":"In","values":["<app>"]}]},"topologyKey":"kubernetes.io/hostname"}}]}}}]'
  ```
- **后置验证**:
  ```bash
  kubectl get nodes
  kubectl top nodes
  ```

#### REM-004: 应用级优化

- **适用根因**: RC-004
- **前置检查**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 应用 profiling（如果支持）
  kubectl exec <pod> -n <namespace> -- curl -s localhost:6060/debug/pprof/heap
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 应用优化需要开发团队协作，常见措施：
  # - 优化 JVM 参数（Java 应用）
  kubectl patch deployment <name> -n <namespace> --type='json' -p='
  [{"op": "add", "path": "/spec/template/spec/containers/0/env/-", "value":
    {"name":"JAVA_OPTS","value":"-XX:+UseG1GC -XX:MaxRAMPercentage=75.0"}}]'

  # - 增加 worker 进程数
  # - 优化数据库查询
  # - 启用缓存
  ```
- **后置验证**:
  ```bash
  kubectl top pod <pod> -n <namespace>
  # 观察延迟指标改善
  ```

#### REM-006: 网络优化

- **适用根因**: RC-006
- **前置检查**:
  ```bash
  # 测量 Pod 间网络延迟
  kubectl run netperf --rm -i --restart=Never --image=nicolaka/netshoot -- ping -c 5 <target-pod-ip>
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 方案 A: 使用本地 Service（拓扑感知路由）
  kubectl patch service <name> -n <namespace> -p '{"spec":{"internalTrafficPolicy":"Local"}}'

  # 方案 B: 调整 CoreDNS 缓存
  kubectl patch configmap coredns -n kube-system --type='json' -p='
  [{"op": "add", "path": "/data/Corefile", "value":
    ".:53 {\n    errors\n    health\n    ready\n    kubernetes cluster.local in-addr.arpa ip6.arpa {\n      pods insecure\n      fallthrough in-addr.arpa ip6.arpa\n    }\n    prometheus :9153\n    cache 60\n    loop\n    reload\n    loadbalance\n}"}]'
  kubectl rollout restart deployment coredns -n kube-system
  ```
- **后置验证**:
  ```bash
  # 重新测量网络延迟
  ```

## 验证确认

### 即时验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# V1: 节点资源正常
kubectl top nodes

# V2: Pod 无 OOM
kubectl get pod <pod> -n <namespace>

# V3: 重启次数低
kubectl get pod <pod> -n <namespace> -o jsonpath='{.status.containerStatuses[0].restartCount}'

# V4: 无节点压力
kubectl get nodes -o json | jq '.items[].status.conditions[] | select(.type | test("Pressure")) | .status'
```
### 解决确认标准

- [ ] 节点 CPU/Memory 使用率 < 85%
- [ ] Pod 无 OOMKilled
- [ ] Pod 重启次数稳定
- [ ] 应用延迟（P99）恢复至 SLA 以内
- [ ] 无 CPU Throttling（或 throttling < 5%）
- [ ] 无节点 Pressure 条件

## 升级协议

### 自动升级条件

| 条件 | 说明 |
|------|------|
| 应用代码级 Bug | 需要开发团队修复 |
| 集群级资源耗尽 | 需要基础设施扩容 |

### 升级消息模板

```
【{severity}】Performance Bottleneck - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: 性能瓶颈
- 影响范围: 
  - 受影响服务: {affected_services}
  - 延迟指标: {latency_metrics}
  - 资源使用: {resource_usage}
- 可能根因: {suspected_root_cause}
- 已尝试修复: {attempted_remediation}
- Skill 版本: SKILL-PERF-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## Related

- [[reference|#reference Hub]] — tag hub

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
