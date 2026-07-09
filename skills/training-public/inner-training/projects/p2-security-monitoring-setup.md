---
title: 'P2: 安全与监控体系搭建'
description: 'title: P2: 安全与监控体系搭建'
summary: 'title: P2: 安全与监控体系搭建'
category: learning
tags:
- k8s
- training
- hands-on
- prometheus
- rbac
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'P2: 安全与监控体系搭建 是什么'
- '如何 P2: 安全与监控体系搭建'
trigger_keywords:
- 'P2:'
- 安全与监控体系搭建
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: P2: 安全与监控体系搭建
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - ACK RBAC RAM two-layer permission model
  - [[Prometheus|Prometheus]] monitoring|monitoring alerting]] configuration
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] audit log SLS integration
  - ResourceQuota LimitRange configuration
  - Security hardening best practices
trigger_keywords:
  - RBAC
  - RAM
  - permission
  - Prometheus
  - alert
  - audit log
  - SLS
  - ResourceQuota
  - LimitRange
  - security
reading_level: advanced
audience:
  - ACK operators
  - SRE engineers
  - Security engineers
estimated_read_time: 45min
related_domains:
  - 安全
  - 可观测性
  - 云厂商
related_topics:
  - ram-integration
  - vulnerability
  - risk-prevention
  - cluster-monitoring
---

# P2: 安全与监控体系搭建

> **对应周次**: Week 2 | **预计时间**: 3-4 小时 | **难度**: ⭐⭐⭐

---

## 项目目标

为 ACK 集群搭建完整的安全权限体系和监控告警系统：配置 RBAC + RAM 双层权限，部署 Prometheus 监控，配置审计日志，实施资源配额管理。

## 前置条件

- [ ] 完成 Week 2 全部教案 (Day 8-14)
- [ ] 有运行中的 ACK 集群
- [ ] 拥有 RAM 管理权限
- [ ] 了解 RBAC 和 Prometheus 基础

---

## 实施步骤

### Step 1: RBAC 权限配置 (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1.1 创建 Namespace 隔离
kubectl create namespace team-dev
kubectl create namespace team-ops

# 1.2 创建 dev 团队 Role (只读 + Pod 管理)
cat <<EOF | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: dev-role
  namespace: team-dev
rules:
- apiGroups: [""]
  resources: ["pods", "pods/log", "services", "configmaps"]
  verbs: ["get", "list", "watch", "create", "update", "delete"]
- apiGroups: ["apps"]
  resources: ["deployments", "replicasets"]
  verbs: ["get", "list", "watch", "create", "update", "delete"]
- apiGroups: [""]
  resources: ["secrets"]
  verbs: ["get", "list"]
EOF

# 1.3 创建 ops 团队 ClusterRole (全集群只读)
cat <<EOF | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: ops-readonly
rules:
- apiGroups: [""]
  resources: ["*"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["apps", "batch"]
  resources: ["*"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["networking.k8s.io"]
  resources: ["*"]
  verbs: ["get", "list", "watch"]
EOF

# 1.4 绑定到用户
kubectl create rolebinding dev-binding --role=dev-role --user=dev-user@example.com -n team-dev
kubectl create clusterrolebinding ops-binding --clusterrole=ops-readonly --user=ops-user@example.com

# 1.5 验证权限
kubectl auth can-i create pods -n team-dev --as=dev-user@example.com
kubectl auth can-i delete nodes --as=ops-user@example.com
```
### Step 2: RAM 账号与 ACK 集成 (30min)

```bash
# 2.1 通过 ACK API 授权 RAM 用户
aliyun cs POST /clusters/<cluster_id>/grant_permissions --body '{
  "body": [
    {
      "user_id": "<ram-user-id>",
      "permissions": [
        {
          "cluster": "<cluster_id>",
          "role_type": "cluster",
          "role_name": "dev",
          "namespace": "team-dev",
          "is_custom": false,
          "is_ram_role": false
        }
      ]
    }
  ]
}'

# 2.2 查看用户权限
aliyun cs GET /clusters/<cluster_id>/grant_permissions

# 2.3 为 RAM 用户生成 kubeconfig
# 通过控制台: 集群 → 连接信息 → 生成 kubeconfig (指定 RAM 用户)
```

### Step 3: 监控体系搭建 (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 3.1 确认 ARMS Prometheus 已安装
kubectl get pods -n arms-prom 2>/dev/null || kubectl get pods -n monitoring 2>/dev/null

# 3.2 安装 Prometheus (如未安装, 通过控制台)
# 路径: 集群 → 运维管理 → Prometheus 监控 → 开启

# 3.3 部署自定义 ServiceMonitor
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: app-monitor
  namespace: team-dev
spec:
  selector:
    matchLabels:
      monitor: "true"
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
EOF

# 3.4 配置告警规则
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: pod-alerts
  namespace: team-dev
spec:
  groups:
  - name: pod-health
    rules:
    - alert: PodCrashLooping
      expr: rate(kube_pod_container_status_restarts_total[15m]) > 0
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ \$labels.pod }} 频繁重启"
    - alert: PodNotReady
      expr: kube_pod_status_ready{condition="true"} == 0
      for: 10m
      labels:
        severity: critical
      annotations:
        summary: "Pod {{ \$labels.pod }} 持续未就绪"
EOF

# 3.5 常用 PromQL 查询
echo "CPU 使用率: sum(rate(container_cpu_usage_seconds_total[5m])) by (pod)"
echo "内存使用: sum(container_memory_working_set_bytes) by (pod)"
echo "Pod 重启: kube_pod_container_status_restarts_total"
```
### Step 4: 审计日志配置 (30min)

```bash
# 4.1 确认审计日志状态
aliyun cs GET /clusters/<cluster_id> | grep audit

# 4.2 通过控制台开启审计日志
# 路径: 集群 → 集群信息 → 集群审计 → 开启 (投递到 SLS)

# 4.3 查看审计日志 (SLS 查询)
# 在 SLS 控制台查询示例:
# * and verb:delete      → 所有删除操作
# * and user.username:admin  → admin 用户操作
# * and objectRef.resource:secrets  → Secret 相关操作
```

### Step 5: 资源配额管理 (20min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 5.1 设置 Namespace 配额
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ResourceQuota
metadata:
  name: team-dev-quota
  namespace: team-dev
spec:
  hard:
    requests.cpu: "8"
    requests.memory: 16Gi
    limits.cpu: "16"
    limits.memory: 32Gi
    pods: "50"
    services: "20"
    persistentvolumeclaims: "10"
---
apiVersion: v1
kind: LimitRange
metadata:
  name: team-dev-limits
  namespace: team-dev
spec:
  limits:
  - default:
      cpu: 500m
      memory: 512Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    max:
      cpu: "4"
      memory: 8Gi
    type: Container
EOF

# 5.2 验证配额
kubectl describe quota team-dev-quota -n team-dev
kubectl describe limitrange team-dev-limits -n team-dev
```
---

## 验收清单

- [ ] RBAC Role/ClusterRole 配置正确，权限验证通过
- [ ] RAM 用户成功绑定到 ACK 集群权限
- [ ] Prometheus 监控部署并可查询指标
- [ ] 配置了至少 2 条告警规则
- [ ] 审计日志开启并可在 SLS 查询
- [ ] ResourceQuota 和 LimitRange 配置生效

---

## 清理资源

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
kubectl delete namespace team-dev team-ops  # ⚠️ 不可逆：永久删除命名空间及全部资源
kubectl delete clusterrole ops-readonly
kubectl delete clusterrolebinding ops-binding
```

<!-- risk-assessed -->
