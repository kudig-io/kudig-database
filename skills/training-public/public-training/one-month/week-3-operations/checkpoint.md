---
title: 'Week 3 Checkpoint: 自测检验'
description: '- K8s 运维能力自测'
summary: '- K8s 运维能力自测'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- prometheus
- grafana
- containerd
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Week 3 Checkpoint: 自测检验 是什么'
- '如何 Week 3 Checkpoint: 自测检验'
trigger_keywords:
- Week
- 'Checkpoint:'
- 自测检验
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Week 3 Checkpoint: 自测检验
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[Kubernetes|kubernetes]] Week 3 自测
  - K8s 运维能力自测
  - 故障排查知识点检验
  - 安全监控自测题
trigger_keywords:
  - 自测
  - checkpoint
  - Week 3
  - 检验
  - RBAC
  - [[Prometheus|Prometheus]]
  - 故障排查
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 90min
related_domains:
  - 安全
  - 可观测性
  - 故障诊断
related_topics:
  - 生产运维/topic-learn/public-training/one-month/week-3-operations/day-15-security-1
  - 生产运维/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
  - 生产运维/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
---

# Week 3 Checkpoint: 自测检验

> 完成本周学习后，请独立完成以下自测题。

---

## 概述

Week 3 是运维作战能力建设的关键阶段，涵盖了安全合规体系、可观测性构建、故障排查方法论和平台运维实践四大核心领域。本周学习内容是日常运维工作的基础，掌握这些知识意味着你已经具备了独立处理大多数生产运维问题的能力。

本自测从概念理解、命令实操和场景分析三个维度全面检验你对 Week 3 内容的掌握程度。概念理解部分检验你对核心原理的深度理解，命令实操部分验证你的动手能力，场景分析部分考察你综合运用知识解决实际问题的能力。请独立完成所有题目，完成后对照参考答案进行自我评估。

**自测目标**：
- 检验 RBAC、Prometheus、节点排障、FTA/FEBM 等核心概念的掌握程度
- 验证 kubectl 命令实操的熟练度
- 评估综合场景分析和故障排查能力

---

## 一、概念理解 (每题 2 分，共 20 分)

### 1. RBAC 中 Role vs ClusterRole，RoleBinding vs ClusterRoleBinding 的区别和适用场景？

**你的回答:**

```
(在此写下你的答案)

```

**参考要点:**

| 资源类型 | 作用范围 | 典型使用场景 |
|----------|---------|-------------|
| **Role** | 单个 Namespace | 开发人员只能操作 dev 命名空间 |
| **ClusterRole** | 整个集群 | 节点管理、PV 管理、健康检查 |
| **RoleBinding** | 将角色绑定到 Namespace | 用户 → Role（限制在某个命名空间） |
| **ClusterRoleBinding** | 将角色绑定到集群 | 管理员 → ClusterRole（全集群权限） |

**重要组合**: ClusterRole + RoleBinding = 将集群级角色限制在特定 Namespace 使用。这种模式可以复用 ClusterRole 的定义，但将权限限制在某个命名空间内。例如，创建一个 "reader" ClusterRole，然后通过 RoleBinding 将不同团队绑定到各自的命名空间。

```yaml
# ClusterRole + RoleBinding 组合示例
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: namespace-reader
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps"]
  verbs: ["get", "list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: team-a-reader
  namespace: team-a
subjects:
- kind: User
  name: developer-a@company.com
roleRef:
  kind: ClusterRole
  name: namespace-reader
  apiGroup: rbac.authorization.k8s.io
```

---

### 2. PromQL 中 rate() 和 irate() 的区别？写出"过去5分钟容器 CPU 使用率"的查询

**你的回答:**

```
(在此写下你的答案)

```

**参考要点:**

| 函数 | 计算方式 | 特点 | 适用场景 |
|------|---------|------|---------|
| `rate()` | 使用时间窗口内所有数据点的平均变化率 | 平滑、稳定 | 告警规则、仪表盘 |
| `irate()` | 仅使用时间窗口内最后两个数据点的变化率 | 敏感、波动大 | 实时监控、快速变化指标 |

```promql
# 过去5分钟容器 CPU 使用率 (rate)
rate(container_cpu_usage_seconds_total{namespace="default"}[5m])

# 过去5分钟容器 CPU 使用率 (irate，更敏感)
irate(container_cpu_usage_seconds_total{namespace="default"}[5m])

# 获取 CPU 使用百分比 (乘以100)
rate(container_cpu_usage_seconds_total[5m]) * 100

# 按 Pod 聚合
sum(rate(container_cpu_usage_seconds_total[5m])) by (pod)

# 完整的 CPU 使用率查询（考虑多核）
sum(rate(container_cpu_usage_seconds_total{container!="", container!="POD"}[5m])) by (pod) /
sum(kube_pod_container_resource_limits{resource="cpu"}) by (pod) * 100
```

---

### 3. Node 突然 NotReady，你的完整排查步骤（至少列出 8 步）？

**你的回答:**

```
(在此写下你的答案)

```

**参考要点 - 完整排查流程**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 查看节点状态和 Conditions
kubectl describe node <node-name>

# 关注 Conditions 部分:
# Ready            False   ...   KubeletNotReady  ...
# MemoryPressure   True    ...   ...
# DiskPressure     True    ...   ...

# Step 2: 检查 kubelet 状态 (SSH 到节点)
systemctl status kubelet

# 预期输出:
# ● kubelet.service - Kubernetes Kubelet
#    Active: inactive (dead)  或  Active: active (running)
# 如果 inactive，查看原因:
journalctl -u kubelet --since "10 minutes ago" --no-pager

# Step 3: 检查容器运行时
systemctl status containerd
crictl ps  # 查看容器列表

# Step 4: 检查磁盘空间
df -h
# 关注 /var 和 /var/lib/kubelet 是否满了

# Step 5: 检查内存使用
free -h
# 如果可用内存很少，检查占用进程:
top -o %MEM | head -20

# Step 6: 检查网络连通性
ping <api-server-address>
curl -k https://<api-server-address>:6443/healthz

# Step 7: 检查系统日志
dmesg -T | tail -50
# 查找 OOM killer、内核 panic 等信息

# Step 8: 检查证书有效期
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
# 如果证书过期，需要更新证书

# Step 9: 检查节点资源水位
kubectl describe node <node-name> | grep -A 20 "Allocated resources"
```
---

### 4. FTA 和 FEBM 分别适用于什么场景？两者如何协作？

**你的回答:**

```
(在此写下你的答案)

```

**参考要点:**

| 方法论 | 核心思想 | 适用场景 | 关键产出 |
|--------|---------|---------|---------|
| **FTA** | 自顶向下，逻辑门分解所有可能原因 | 问题预防分析、知识库建设、培训 | 故障树图、问题路径清单 |
| **FEBM** | 收集证据 → 生成假设 → 逐一验证 | 实时故障排查、根因分析、事故复盘 | 证据表、假设验证矩阵 |

**协作方式**：
1. **先 FTA 后 FEBM**: 用 FTA 构建故障分析框架（所有可能原因），再用 FEBM 收集证据逐一验证
2. **FEBM 补充 FTA**: 排查过程中发现的新故障模式，反馈到 FTA 故障树中
3. **日常积累**: 每次问题修复后，用 FTA 更新问题知识库，提高下次排查效率

---

### 5. etcd 备份的命令是什么？备份应该多久做一次？为什么？

**你的回答:**

```
(在此写下你的答案)

```

**参考要点:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# etcd 备份命令
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')

kubectl exec -n kube-system $ETCD_POD -- \
  etcdctl snapshot save /var/lib/etcd/snapshot-$(date +%Y%m%d).db \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 验证备份
kubectl exec -n kube-system $ETCD_POD -- \
  etcdctl snapshot status /var/lib/etcd/snapshot-20260518.db --write-table

# 预期输出:
# +----------+----------+------------+------------+
# |   REVISION |   HASH   | COMPACTION |  RAFT TERM |
# +----------+----------+------------+------------+
# |  12345678 |  abcdef12 |   12340000 |        567 |
# +----------+----------+------------+------------+
```
**备份策略**：

| 频率 | 场景 | 原因 |
|------|------|------|
| **每天至少一次** | 常规备份 | 最小化数据丢失窗口 |
| **变更前必须备份** | 升级/配置变更 | 提供回滚基线 |
| **每次重大操作前** | 集群迁移/节点替换 | 确保可恢复 |
| **自动备份** | ACK 托管版 | 阿里云默认提供 |

**为什么重要**: etcd 存储了所有集群状态数据（Pod、Service、ConfigMap、Secret 等），是 Kubernetes 的"数据库"。etcd 数据丢失意味着整个集群状态的丢失，必须通过备份恢复。

---

## 二、命令实操 (每题 2 分，共 10 分)

### 6. 如何检查当前用户是否有权限创建 Deployment？

**你的回答:**

```
# 🟢 低风险：只读/信息收集，通常无副作用
```

**参考答案:**

```bash
# 方法1: 直接检查
kubectl auth can-i create deployments
# 预期输出: yes

# 方法2: 检查特定命名空间
kubectl auth can-i create deployments -n production
# 预期输出: no (如果没有权限)

# 方法3: 检查其他用户/ServiceAccount的权限
kubectl auth can-i create deployments --as=system:serviceaccount:default:my-sa
# 预期输出: yes/no

# 方法4: 列出当前用户所有权限
kubectl auth can-i --list
# 预期输出:
# Resources   Non-Resource URLs   Resource Names   Verbs
# pods        []                  []               [get list watch]
# deployments []                  []               [get list watch create update delete]
```
---

### 7. 如何查看 Pod 的上一次崩溃日志？

**你的回答:**

```
# 🟢 低风险：只读/信息收集，通常无副作用
```

**参考答案:**

```bash
# 查看 Pod 上一次崩溃的日志
kubectl logs <pod-name> --previous
kubectl logs <pod-name> -p  # 简写

# 查看特定容器上一次崩溃的日志
kubectl logs <pod-name> -c <container-name> --previous

# 查看最近 100 行
kubectl logs <pod-name> --previous --tail=100

# 带时间戳查看
kubectl logs <pod-name> --previous --timestamps

# 预期输出:
# 2026-05-18T10:25:30.123456789Z Error: Cannot connect to database at db-service:3306
# 2026-05-18T10:25:30.456789012Z at com.app.Database.connect(Database.java:42)
# 2026-05-18T10:25:30.789012345Z Caused by: java.net.ConnectException: Connection refused
```
---

### 8. 如何列出所有 firing 状态的告警？

**你的回答:**

```
# 🟢 低风险：只读/信息收集，通常无副作用
```

**参考答案:**

```bash
# 方法1: 通过 Alertmanager UI
# 浏览器访问 Alertmanager: http://<alertmanager-address>:9093
# 查看默认页面的 Active Alerts

# 方法2: 使用 amtool 命令行
amtool alert --alertmanager.url=http://alertmanager:9093

# 预期输出:
# Alertname            Starts At                Summary
# PodCrashLoopBackOff  2026-05-18 10:25:30 UTC  Pod my-app-xxx is crash looping
# NodeNotReady         2026-05-18 10:30:00 UTC  Node worker-3 is not ready

# 方法3: 通过 Prometheus API 查询 firing 告警
curl -s http://prometheus:9090/api/v1/alerts | jq '.data.alerts[] | select(.state=="firing")'

# 方法4: kubectl 查询 PrometheusRule
kubectl get prometheusrules -A -o yaml | grep -A5 "alert:"
```
---

### 9. 如何查看某个 namespace 的所有事件并按时间排序？

**你的回答:**

```
# 🟢 低风险：只读/信息收集，通常无副作用
```

**参考答案:**

```bash
# 按 lastTimestamp 排序
kubectl get events -n <namespace> --sort-by='.lastTimestamp'

# 预期输出:
# LAST SEEN   TYPE      REASON    OBJECT              MESSAGE
# 2m          Warning   Failed    pod/my-app-xyz      Error: ImagePullBackOff
# 1m          Normal    Pulled    pod/my-app-xyz      Successfully pulled image "nginx:1.25"
# 30s         Normal    Created   pod/my-app-xyz      Created container app
# 10s         Normal    Started   pod/my-app-xyz      Started container app

# 查看所有类型的事件（包括非 warning）
kubectl get events -n <namespace> --sort-by='.lastTimestamp' --all-namespaces

# 使用 wide 输出格式
kubectl get events -n <namespace> --sort-by='.lastTimestamp' -o wide

# 持续监控新事件
kubectl get events -n <namespace> --sort-by='.lastTimestamp' --watch

# 查看事件详情（YAML格式）
kubectl get events -n <namespace> --sort-by='.lastTimestamp' -o yaml
```
---

### 10. 如何使用 ServiceAccount 的 Token 进行 API 调用？

**你的回答:**

```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
```

**参考答案:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# Step 1: 创建 ServiceAccount（如果没有）
kubectl create serviceaccount my-sa -n default

# Step 2: 获取 Token (K8s 1.24+ 方式)
TOKEN=$(kubectl create token my-sa --duration=3600s)
echo $TOKEN
# 预期输出: eyJhbGciOiJSUzI1NiIsImtpZCI6...

# Step 3: 使用 Token 调用 API
# 方法A: 通过 kubectl proxy
kubectl proxy --port=8001 &
curl -H "Authorization: Bearer $TOKEN" \
  http://localhost:8001/api/v1/namespaces/default/pods

# 方法B: 直接调用 API Server
APISERVER=$(kubectl config view --minify -o jsonpath='{.clusters[0].cluster.server}')
curl -k -H "Authorization: Bearer $TOKEN" \
  $APISERVER/api/v1/namespaces/default/pods

# 预期输出:
# {
#   "kind": "PodList",
#   "apiVersion": "v1",
#   "items": [...]
# }

# Step 4: 清理
kill %1  # 关闭 kubectl proxy
```
---

## 三、场景分析 (每题 5 分，共 20 分)

### 11. 设计一个 RBAC 方案：开发人员只能在 dev namespace 中查看和创建 Deployment/Pod/Service

**你的答案:**

```yaml
# Step 1: 创建 Role
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: dev-developer
  namespace: dev
rules:
- apiGroups: ["apps"]
  resources: ["deployments"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["pods", "services"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["pods/log"]
  verbs: ["get", "list"]
- apiGroups: [""]
  resources: ["pods/exec"]
  verbs: ["create"]
---
# Step 2: 创建 RoleBinding
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: dev-developer-binding
  namespace: dev
subjects:
- kind: User
  name: developer@company.com
  apiGroup: rbac.authorization.k8s.io
- kind: Group
  name: dev-team
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: dev-developer
  apiGroup: rbac.authorization.k8s.io
```

**验证方案**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证开发人员的权限
kubectl auth can-i create deployments --as=developer@company.com -n dev
# 预期输出: yes

kubectl auth can-i create deployments --as=developer@company.com -n production
# 预期输出: no

kubectl auth can-i delete pods --as=developer@company.com -n dev
# 预期输出: yes

kubectl auth can-i get secrets --as=developer@company.com -n dev
# 预期输出: no
```
---

### 12. 描述如何使用 Prometheus + Grafana + Alertmanager 构建完整的监控告警链路

**参考要点:**

完整的监控告警链路包含数据采集、存储、可视化、告警规则和通知分发五个环节：

```
应用/Pod/Node → Prometheus(采集+存储) → Grafana(可视化)
                        ↓
                PrometheusRule(告警规则)
                        ↓
                 Alertmanager(分组/路由/抑制)
                        ↓
              钉钉/企微/Slack/Email/Webhook
```

**详细组件职责**：

| 组件 | 职责 | 配置方式 |
|------|------|---------|
| **Prometheus** | 采集和存储时序指标 | ServiceMonitor / PodMonitor |
| **PrometheusRule** | 定义告警触发条件 | PrometheusRule CRD |
| **Alertmanager** | 告警分组、路由、抑制、静默 | Secret 配置 |
| **Grafana** | 指标可视化展示 | Dashboard JSON |
| **Notification** | 告警通知分发 | Webhook / Email / IM |

```yaml
# PrometheusRule 示例
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: pod-alerts
  namespace: monitoring
spec:
  groups:
  - name: pod-alerts
    rules:
    - alert: PodCrashLoopBackOff
      expr: rate(kube_pod_container_status_restarts_total[15m]) > 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} is crash looping"
        message: "Pod has restarted {{ $value }} times in the last 15 minutes"
    - alert: HighMemoryUsage
      expr: |
        sum(container_memory_working_set_bytes{container!=""}) by (pod)
        /
        sum(kube_pod_container_resource_limits{resource="memory"}) by (pod)
        > 0.85
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ $labels.pod }} memory usage exceeds 85%"
```

---

### 13. 如果线上出现 Pod OOMKilled，完整的分析和修复流程是什么？

**参考要点:**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 确认 OOMKilled 状态
kubectl describe pod <pod-name> -n <namespace>

# 关注 Last State:
# Last State:     Terminated
#   Reason:       OOMKilled
#   Exit Code:    137

# Step 2: 查看当前资源限制
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[0].resources}'

# 预期输出:
# {"limits":{"memory":"256Mi"},"requests":{"memory":"128Mi"}}

# Step 3: 查看实际内存使用趋势
kubectl top pod <pod-name> -n <namespace>

# 在 Grafana 中查看内存使用趋势:
# container_memory_working_set_bytes{pod="<pod-name>"}

# Step 4: 判断是内存泄漏还是 limits 过低
# - 内存使用持续上升 -> 内存泄漏
# - 内存使用在某个值附近波动 -> limits 设置过低

# Step 5: 修复方案
# 方案A: 增大 memory limits (limits 过低)
kubectl set resources deployment/<deploy-name> \
  -c <container> \
  --limits=memory=512Mi \
  --requests=memory=256Mi \
  -n <namespace>

# 方案B: 修复内存泄漏 (代码问题)
# - 联系开发团队排查内存泄漏
# - 临时增大 limits 作为缓解
# - 配置定期重启（在 Deployment 中设置 activeDeadlineSeconds）

# Step 6: 配置告警防止再次发生
# 在 PrometheusRule 中添加:
# - alert: MemoryUsageHigh
#   expr: container_memory_working_set_bytes / kube_pod_container_resource_limits{resource="memory"} > 0.85
```
---

### 14. 解释 Pod Security Standards 的三个级别，以及如何在生产环境实施

**参考要点:**

| 级别 | 限制强度 | 允许的操作 | 适用场景 |
|------|---------|-----------|---------|
| **Privileged** | 无限制 | 特权容器、hostPath、所有 capabilities | 系统组件、基础设施 Pod |
| **Baseline** | 基本限制 | 禁止特权容器、禁止宿主机命名空间 | 大多数应用、中间件 |
| **Restricted** | 最严格 | 非 root 运行、删除所有 capabilities、只读文件系统 | 安全敏感应用、前端服务 |

**生产环境实施方案**：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Step 1: 为不同命名空间设置 PSS 级别
# kube-system: privileged（系统组件需要特权）
kubectl label namespace kube-system pod-security.kubernetes.io/enforce=privileged
kubectl label namespace kube-system pod-security.kubernetes.io/audit=privileged
kubectl label namespace kube-system pod-security.kubernetes.io/warn=privileged

# monitoring: baseline（监控组件需要部分权限）
kubectl label namespace monitoring pod-security.kubernetes.io/enforce=baseline

# default, production: restricted（应用遵循最严格标准）
kubectl label namespace default pod-security.kubernetes.io/enforce=restricted
kubectl label namespace default pod-security.kubernetes.io/audit=restricted
kubectl label namespace default pod-security.kubernetes.io/warn=restricted

# Step 2: 验证 PSS 标签
kubectl get namespaces --show-labels | grep pod-security

# Step 3: 测试（enforce 模式下不合规的 Pod 会被拒绝）
kubectl run test-privileged --image=nginx --restart=Never --overrides='{"spec":{"containers":[{"name":"app","image":"nginx","securityContext":{"privileged":true}}]}}'
# 预期输出:
# Error from server (Forbidden): pods "test-privileged" is forbidden: violates PodSecurity "restricted:latest": ...
```
---

## 四、评分统计

| 部分 | 得分 | 满分 |
|------|------|------|
| 概念理解 | __ | 20 |
| 命令实操 | __ | 10 |
| 场景分析 | __ | 20 |
| **总分** | __ | **50** |

**评估标准**：
- **45-50 分**: 优秀，完全掌握本周内容，可以独立处理安全、监控、排障相关运维
- **35-44 分**: 良好，核心概念理解，部分细节需加强
- **25-34 分**: 及格，建议重点复习薄弱环节
- **< 25 分**: 不及格，建议重新学习本周内容

---

## 五、薄弱点记录

```
1.


2.


3.

```

---

## 要点总结

- **RBAC** 四种资源各有适用场景，ClusterRole + RoleBinding 是常用组合
- **rate()** 适合告警（平滑稳定），**irate()** 适合实时监控（敏感）
- **节点 NotReady** 排查：Conditions → kubelet → 运行时 → 磁盘 → 内存 → 网络 → 日志 → 证书
- **FTA 提供分析框架，FEBM 提供验证方法**，两者结合效果最佳
- **etcd 备份**每天至少一次，变更前必须备份
- **PSS 三个级别**: Privileged → Baseline → Restricted，生产推荐 Restricted

---

## 延伸阅读

- [RBAC 官方文档](https://kubernetes.io/docs/reference/access-authn-authz/rbac/)
- [PromQL 学习指南](https://prometheus.io/docs/prometheus/latest/querying/basics/)
- [Pod Security Standards](https://kubernetes.io/docs/concepts/security/pod-security-standards/)
- [Alertmanager 配置](https://prometheus.io/docs/alerting/latest/configuration/)

## Related

- [[生态参考/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
