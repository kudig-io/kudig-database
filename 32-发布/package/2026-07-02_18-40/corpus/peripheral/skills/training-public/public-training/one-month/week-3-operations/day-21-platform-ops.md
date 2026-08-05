---
title: 'Day 21: 平台运维 + 综合实践'
description: 'title: Day 21: 平台运维 + 综合实践'
summary: 'title: Day 21: 平台运维 + 综合实践'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- prometheus
- grafana
- flannel
- coredns
- hpa
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 21: 平台运维 + 综合实践 是什么'
- '如何 Day 21: 平台运维 + 综合实践'
trigger_keywords:
- Day
- '21:'
- 平台运维
- 综合实践
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- backup-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 21: 平台运维 + 综合实践
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - [[Kubernetes|kubernetes]] 平台运维知识点
  - 集群生命周期管理备份恢复
  - kube-prometheus-stack 监控部署
  - k8s 综合运维实践
trigger_keywords:
  - 平台运维
  - 集群生命周期
  - 备份恢复
  - 监控
  - [[Prometheus|Prometheus]]
  - Alertmanager
  - 故障演练
  - 运维实践
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 240min
related_domains:
  - domain-07-platform-engineering
  - domain-06-observability
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-19-troubleshooting-methodology
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-20-troubleshooting-practice
---

# Day 21: 平台运维 + 综合实践

> **学习时间**: 4-5 小时 | **主题**: Week 3 总结与实践项目

---

## 概述

今天是 Week 3 的最后一天，将整合本周所学的安全体系、可观测性和故障排查方法论，完成一个综合实践项目。你将搭建一套完整的可观测性体系（监控 + 日志 + 告警），执行故障注入演练，并编写个人故障排查手册。

---

## 今日目标

- [ ] 了解集群生命周期管理
- [ ] 掌握备份恢复策略
- [ ] 完成综合实践项目 P3

---

## 核心概念

### 1. 平台运维全景

```
日常运维:
  ├── 集群生命周期 (创建/升级/删除)
  ├── 节点管理 (扩缩容/维护/修复)
  ├── 工作负载管理 (部署/更新/回滚)
  └── 配置管理 (ConfigMap/Secret)

保障体系:
  ├── 安全合规 (RBAC/PSS/审计日志)
  ├── 可观测性 (监控/日志/追踪)
  └── 备份恢复 (etcd/Velero)

应急能力:
  ├── 故障排查 (FTA/FEBM)
  ├── 变更管理 (审批/灰度/回滚)
  └── 容量规划 (资源预测/自动伸缩)
```

### 2. 备份恢复策略

| 备份类型 | 工具 | 备份内容 | 频率 |
|----------|------|---------|------|
| etcd 快照 | etcdctl | 集群状态数据 | 每 6 小时 |
| 资源备份 | Velero | K8s 资源 YAML + PV 数据 | 每天 |
| 应用备份 | 自定义脚本 | 关键应用配置和数据 | 按需 |

### 3. 故障演练类型

| 演练类型 | 注入方式 | 验证目标 |
|----------|---------|---------|
| Pod 问题 | 删除 Pod/OOM 注入 | 自动恢复/告警触发 |
| 节点问题 | cordon/drain | Pod 迁移/服务可用 |
| 网络问题 | [[NetworkPolicy|NetworkPolicy]]/iptables | 降级策略/超时处理 |
| 资源耗尽 | stress-ng | 告警/HPA/驱逐 |

---

## 理论学习 (2h)

### 必读文档

1. **集群生命周期管理**
   - 文件: `../../domain-07-platform-engineering/02-cluster-lifecycle-management.md`
   - 重点: 集群升级、维护窗口

2. **备份恢复策略**
   - 文件: `../../domain-07-platform-engineering/12-backup-recovery-strategy.md`
   - 重点: etcd 备份、Velero

3. **监控 Playbooks**
   - 文件: `../../[[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-06-observability/05-alerting/06-monitoring-playbooks|21-monitoring-playbooks]].md`
   - 重点: 监控配置模板

---

## 综合实践项目 P3 (2.5h)

**项目: 可观测性体系搭建 + 故障演练**

详细指南见: [../projects/p3-observability-fault-drill.md](../projects/p3-observability-fault-drill.md)

### Step 1: 确认监控栈 (15min)

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确认 Prometheus 和相关组件已部署
kubectl get pods -n monitoring
# NAME                                   READY   STATUS    RESTARTS   AGE
# prometheus-k8s-0                       2/2     Running   0          5d
# prometheus-k8s-1                       2/2     Running   0          5d
# grafana-xxxxxxxxxx-xxxxx              2/2     Running   0          5d
# alertmanager-main-0                   2/2     Running   0          5d
# kube-state-metrics-xxxxxxxxxx-xxxxx   1/1     Running   0          5d
# node-exporter-xxxxx                   1/1     Running   0          5d

# 确认 Grafana 可访问
kubectl port-forward -n monitoring svc/grafana 3000:80
# 打开浏览器访问 http://localhost:3000
# 默认账号: admin / admin

# 确认 Prometheus 目标正常
kubectl port-forward -n monitoring svc/prometheus-k8s 9090:9090
# 访问 http://localhost:9090/targets 确认所有 target UP
```
---

### Step 2: 配置核心告警规则 (30min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > core-alerts.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: core-alerts
  namespace: monitoring
spec:
  groups:
  - name: k8s-core
    rules:
    - alert: PodCrashLooping
      expr: rate(kube_pod_container_status_restarts_total[15m]) > 0
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} crash looping"
        runbook: "kubectl logs {{ $labels.pod }} -n {{ $labels.namespace }} --previous"

    - alert: NodeNotReady
      expr: kube_node_status_condition{condition="Ready",status="true"} == 0
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ $labels.node }} is not ready"

    - alert: HighCPUUsage
      expr: (1 - avg(rate(node_cpu_seconds_total{mode="idle"}[5m])) by (instance)) > 0.9
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Node {{ $labels.instance }} CPU > 90%"

    - alert: HighMemoryUsage
      expr: (1 - node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes) > 0.9
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ $labels.instance }} memory > 90%"

    - alert: DiskSpaceLow
      expr: node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"} < 0.1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ $labels.instance }} disk < 10% available"

    - alert: PVCAlmostFull
      expr: kubelet_volume_stats_used_bytes / kubelet_volume_stats_capacity_bytes > 0.9
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "PVC {{ $labels.namespace }}/{{ $labels.persistentvolumeclaim }} > 90% used"

    - alert: DeploymentReplicasMismatch
      expr: kube_deployment_status_replicas_available != kube_deployment_spec_replicas
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "Deployment {{ $labels.namespace }}/{{ $labels.deployment }} replicas mismatch"
EOF

kubectl apply -f core-alerts.yaml

# 验证规则已加载
kubectl get prometheusrules -n monitoring
kubectl describe prometheusrule core-alerts -n monitoring
```
---

### Step 3: 配置 Grafana Dashboard (30min)

导入以下推荐 Dashboard:

| Dashboard ID | 名称 | 用途 |
|-------------|------|------|
| 315 | Kubernetes cluster monitoring | 集群概览 |
| 6417 | Kubernetes pods monitoring | Pod 监控 |
| 13639 | Loki & Promtail | 日志系统 |
| 1860 | Node Exporter Full | 节点详细监控 |
| 15760 | Kubernetes Views | 多集群视图 |

导入步骤:

```
1. 打开 Grafana → Dashboards → Import
2. 输入 Dashboard ID (如 315)
3. 点击 Load
4. 选择 Prometheus 数据源
5. 点击 Import
```

---

### Step 4: 故障注入与排查演练 (1h)

#### 4.1 准备测试环境

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl create namespace fault-drill
kubectl create deployment app --image=nginx:alpine -n fault-drill --replicas=3
kubectl expose deployment app --port=80 -n fault-drill

# 验证
kubectl get pods -n fault-drill
kubectl get svc -n fault-drill
```
#### 4.2 问题 1: 模拟 OOM

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
cat > oom-inject.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: oom-inject
  namespace: fault-drill
spec:
  containers:
  - name: stress
    image: polinux/stress
    command: ["stress"]
    args: ["--vm", "1", "--vm-bytes", "200M", "--timeout", "60s"]
    resources:
      limits:
        memory: 100Mi
EOF

kubectl apply -f oom-inject.yaml

# 在 Grafana 中观察:
# - PodCrashLooping 告警是否触发
# - OOMKilled 事件是否记录

# 排查步骤 (按 FEBM 方法论):
# Step 1: 收集证据
kubectl get pod oom-inject -n fault-drill
# STATUS: OOMKilled

kubectl describe pod oom-inject -n fault-drill | grep -A 5 "Last State"
# Last State:     Terminated
#   Reason:       OOMKilled
#   Exit Code:    137

# Step 2: 分析根因
# limits.memory: 100Mi < stress 需要: 200M

# Step 3: 修复
# 方案 A: 增大 limits.memory
# 方案 B: 减小 stress 内存需求
# 方案 C: 优化应用内存使用

kubectl delete pod oom-inject -n fault-drill
```
#### 4.3 问题 2: 模拟 Service 不可用

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 删除 Endpoints (模拟 selector 不匹配)
kubectl delete endpoints app -n fault-drill

# 排查步骤:
# Step 1: 发现问题
kubectl get svc app -n fault-drill
# ClusterIP: 10.96.0.xxx  ← Service 存在
kubectl get endpoints app -n fault-drill
# No resources found  ← Endpoints 消失

# Step 2: 分析原因
kubectl describe svc app -n fault-drill | grep -A 3 "Selector"
# Selector:          app=app
kubectl get pods -n fault-drill --show-labels
# 检查 Pod 标签是否匹配 selector

# Step 3: 修复
# 方案 A: 重新创建 Service (自动重建 Endpoints)
kubectl delete svc app -n fault-drill
kubectl expose deployment app --port=80 -n fault-drill
# 方案 B: 修复 Pod 标签

# 验证修复
kubectl get endpoints app -n fault-drill
# NAME   ENDPOINTS                                AGE
# app    10.244.1.x:80,10.244.2.x:80,10.244.3.x:80   5s
```
#### 4.4 问题 3: 模拟节点问题

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
# 选择一个节点
NODE=$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')
echo "目标节点: $NODE"

# cordon 节点 (停止调度新 Pod)
kubectl cordon $NODE

# drain 节点 (驱逐所有 Pod)
kubectl drain $NODE --ignore-daemonsets --delete-emptydir-data --timeout=120s

# 在 Grafana 中观察:
# - Pod 迁移过程
# - NodeNotReady 告警是否触发

# 恢复节点
kubectl uncordon $NODE

# 验证
kubectl get nodes
kubectl get pods -n fault-drill -o wide
```
---

### Step 5: 产出故障排查手册 (30min)

创建 `~/troubleshooting-handbook.md`:

```markdown
# K8s 故障排查手册

## 1. Pod 问题

### 1.1 Pod Pending
- **症状**: Pod 长时间 Pending
- **排查**: `kubectl describe pod <name>` 查看 Events
- **常见原因**:
  | 原因 | 事件信息 | 修复方法 |
  |------|---------|---------|
  | 资源不足 | Insufficient cpu/memory | 增加节点或降低请求 |
  | nodeSelector 无匹配 | MatchNodeSelector | 检查节点标签 |
  | taints 阻止 | TaintsTolerations | 添加 toleration |
  | PVC 未绑定 | FailedScheduling + volume | 检查 StorageClass |

### 1.2 CrashLoopBackOff
- **症状**: Pod 反复重启
- **排查**: `kubectl logs <name> --previous`
- **常见原因**:
  | 原因 | Exit Code | 修复方法 |
  |------|----------|---------|
  | 应用错误 | 1 | 检查日志修复应用 |
  | OOMKilled | 137 | 增大 limits.memory |
  | 健康检查失败 | 0/1 | 调整 probe 参数 |
  | 配置缺失 | 1 | 检查 ConfigMap/Secret |

### 1.3 ImagePullBackOff
- **症状**: 镜像拉取失败
- **排查**: `kubectl describe pod <name>` 查看 Events
- **常见原因**: 镜像不存在、权限不足、网络不通
- **修复**: 检查镜像名/标签、配置 imagePullSecrets、检查网络

## 2. Service 问题

### 2.1 无法访问
- **排查步骤**:
  1. `kubectl get endpoints <name>` — Endpoints 是否有 Pod IP
  2. `kubectl describe svc <name>` — Selector 是否正确
  3. `kubectl get pods --show-labels` — Pod 标签是否匹配
  4. Pod 内部测试: `curl <service-ip>:<port>`

### 2.2 DNS 解析失败
- **排查**: `kubectl run test --image=busybox --rm -it -- nslookup <svc>`
- **常见原因**: coredns 异常、kube-dns Service 异常

## 3. 节点问题

### 3.1 Node NotReady
- **排查步骤**:
  1. `kubectl describe node <name>` — 查看 Conditions
  2. SSH 到节点: `systemctl status kubelet`
  3. 检查磁盘: `df -h`
  4. 检查内存: `free -h`
  5. 检查网络: `curl -k https://<api-server>:6443/healthz`

### 3.2 节点磁盘压力
- **症状**: DiskPressure condition 为 True
- **排查**: `df -h` / `du -sh /var/lib/kubelet/*`
- **修复**: 清理日志、镜像、未使用的容器

## 4. 监控告警响应

### 4.1 PodCrashLooping
- 告警: Pod 重启频繁
- 响应: 检查日志和资源使用
- 修复: 根据 exit code 对应处理

### 4.2 NodeNotReady
- 告警: 节点不健康
- 响应: 检查 kubelet 和系统资源
- 升级: 如果 5 分钟内无法恢复，升级到 P1

### 4.3 HighCPUUsage / HighMemoryUsage
- 告警: 资源使用率超阈值
- 响应: 检查 Top Pod，确认是哪个应用
- 处理: 扩容/优化/限流
```

---

## 自测检验

完成 [checkpoint.md](./checkpoint.md) 中的 Week 3 自测题。

---

## Week 3 总结

### 学习路径

```
Day 15-16: 安全体系 (RBAC, Pod Security, Secret)
Day 17-18: 可观测性 (Prometheus, Loki, Alertmanager)
Day 19-20: 故障排查 (FTA, FEBM, 实战演练)
Day 21:    平台运维 + 综合实践
```

### 关键收获

| 能力域 | 核心收获 | 实践产出 |
|--------|---------|---------|
| 安全 | RBAC 最小权限、Pod 安全标准 | 安全加固的 Deployment 模板 |
| 监控 | Prometheus + Grafana + Alertmanager | 告警规则 + Dashboard |
| 日志 | Loki + Promtail | 日志查询配置 |
| 排障 | FTA 故障树 + FEBM 取证方法 | 故障排查手册 |

### 下周预告

Week 4 将学习网络与存储，包括 Service/Ingress 配置、Terway/Flannel 网络排查、PV/PVC 存储管理。

---

恭喜完成 Week 3 的学习!

---

## 延伸阅读

- [集群生命周期管理](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-07-platform-engineering/operate/01-cluster-lifecycle-management.md)
- [备份恢复策略](32-发布/package/2026-07-02_18-40/corpus/peripheral/domain-07-platform-engineering/operate/06-backup-recovery-strategy.md)
- [监控 Playbooks](32-发布/package/2026-07-02_18-40/corpus/supporting/domain-06-observability/05-alerting/06-monitoring-playbooks.md)
- [Pod 综合排障](32-发布/package/2026-07-02_18-40/corpus/core/domain-10-troubleshooting-diagnostics/00-core-troubleshooting/07-pod-comprehensive-troubleshooting.md)

```

<!-- risk-assessed -->
