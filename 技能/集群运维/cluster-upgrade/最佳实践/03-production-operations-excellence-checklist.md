---
title: Kubernetes Production Best Practices — Operations Excellence Checklist
description: K8s 生产最佳实践 — 运维卓越清单、资源管理、安全加固、高可用、可观测性、变更管理
summary: 汇总 Kubernetes 生产环境运维的最佳实践清单，覆盖安全、可靠性、性能、成本四大维度
category: practice
tags:
- best-practices
- production
- operations
- checklist
- reliability
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: intermediate
domain: skills
---
# Kubernetes 生产最佳实践清单

> 覆盖安全、可靠性、性能、成本四大维度的运维卓越标准。

## 工作负载最佳实践

### 资源管理

```yaml
# ✅ 正确: 设置合理的资源请求和限制
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-server
spec:
  template:
    spec:
      containers:
        - name: api
          image: registry.example.com/api:v2.1.0  # ✅ 固定版本
          resources:
            requests:
              cpu: "250m"       # 基于 P50 使用量
              memory: "256Mi"   # 基于 P95 使用量
            limits:
              cpu: "1000m"      # 允许突发
              memory: "512Mi"   # 硬限防 OOM
          # ✅ 存活探针
          livenessProbe:
            httpGet:
              path: /healthz
              port: 8080
            initialDelaySeconds: 15
            periodSeconds: 10
            failureThreshold: 3
          # ✅ 就绪探针
          readinessProbe:
            httpGet:
              path: /ready
              port: 8080
            initialDelaySeconds: 5
            periodSeconds: 5
          # ✅ 启动探针（慢启动应用）
          startupProbe:
            httpGet:
              path: /healthz
              port: 8080
            failureThreshold: 30
            periodSeconds: 2
```

### Pod 安全标准

```yaml
# ✅ 安全上下文（Restricted 级别）
securityContext:
  runAsNonRoot: true
  runAsUser: 1000
  runAsGroup: 1000
  fsGroup: 1000
  seccompProfile:
    type: RuntimeDefault
containers:
  - securityContext:
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      capabilities:
        drop: ["ALL"]
```

### 高可用配置

```yaml
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0    # ✅ 零停机
  template:
    spec:
      # ✅ 反亲和（分散到不同节点）
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
            - weight: 100
              podAffinityTerm:
                labelSelector:
                  matchLabels:
                    app: api-server
                topologyKey: kubernetes.io/hostname
      # ✅ 跨 AZ 分布
      topologySpreadConstraints:
        - maxSkew: 1
          topologyKey: topology.kubernetes.io/zone
          whenUnsatisfiable: DoNotSchedule
          labelSelector:
            matchLabels:
              app: api-server
      # ✅ 优雅关闭
      terminationGracePeriodSeconds: 60
      containers:
        - lifecycle:
            preStop:
              exec:
                command: ["/bin/sh", "-c", "sleep 10"]  # 等待 LB 摘流
```

## 网络最佳实践

### NetworkPolicy 基线

```yaml
# ✅ 默认拒绝所有入站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}
  policyTypes: ["Ingress"]
---
# ✅ 仅允许必要流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-api-ingress
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-server
  policyTypes: ["Ingress"]
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              name: ingress-nginx
      ports:
        - port: 8080
          protocol: TCP
```

### DNS 优化

```yaml
# ✅ Pod DNS 优化（减少无效查询）
spec:
  dnsConfig:
    options:
      - name: ndots
        value: "2"          # 减少 search domain 尝试
      - name: timeout
        value: "2"
      - name: attempts
        value: "2"
      - name: single-request-reopen
```

## 存储最佳实践

| 实践 | 说明 |
|------|------|
| 使用 StorageClass | 动态供给，避免手动 PV |
| 设置 reclaimPolicy | 生产用 Retain，开发用 Delete |
| 启用卷快照 | 定期快照用于备份 |
| 监控卷使用率 | > 80% 告警 |
| 使用 CSI 驱动 | 云厂商原生 CSI |
| 分离日志与数据 | 不同性能等级的存储 |

## 可观测性最佳实践

### 必备监控指标

```yaml
# ✅ 四大黄金指标告警
groups:
  - name: golden-signals
    rules:
      # 延迟
      - alert: HighLatencyP99
        expr: histogram_quantile(0.99, rate(http_request_duration_seconds_bucket[5m])) > 1
        for: 5m
        labels:
          severity: warning
      # 流量
      - alert: TrafficDrop
        expr: rate(http_requests_total[5m]) < rate(http_requests_total[5m] offset 1h) * 0.5
        for: 10m
        labels:
          severity: critical
      # 错误率
      - alert: HighErrorRate
        expr: rate(http_requests_total{status=~"5.."}[5m]) / rate(http_requests_total[5m]) > 0.05
        for: 5m
        labels:
          severity: critical
      # 饱和度
      - alert: HighMemoryUsage
        expr: container_memory_working_set_bytes / container_spec_memory_limit_bytes > 0.9
        for: 10m
        labels:
          severity: warning
```

### 日志标准

```yaml
# ✅ 结构化日志 + 合理级别
env:
  - name: LOG_LEVEL
    value: "info"           # 生产用 info
  - name: LOG_FORMAT
    value: "json"           # 结构化 JSON
  - name: LOG_OUTPUT
    value: "stdout"         # 标准输出（容器日志最佳实践）
```

## 安全最佳实践

### RBAC 最小权限

```yaml
# ✅ 应用专用 ServiceAccount（最小权限）
apiVersion: v1
kind: ServiceAccount
metadata:
  name: api-server-sa
  namespace: production
automountServiceAccountToken: false  # ✅ 不需要 API 访问时禁用
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: api-server-role
  namespace: production
rules:
  - apiGroups: [""]
    resources: ["configmaps"]
    resourceNames: ["api-config"]  # ✅ 限定具体资源
    verbs: ["get"]
```

### 镜像安全

| 实践 | 说明 |
|------|------|
| 固定镜像摘要 | `image@sha256:...` 防篡改 |
| 私有 Registry | 不直接拉取公网镜像 |
| 镜像扫描 | CI 中 Trivy/Grype 扫描 |
| 最小基础镜像 | distroless/alpine |
| 禁止 latest | 必须使用语义化版本 |
| 签名验证 | Cosign/Notary 签名 |

## 变更管理最佳实践

### 发布安全

```yaml
# ✅ PodDisruptionBudget（保证滚动更新可用性）
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: api-server-pdb
  namespace: production
spec:
  minAvailable: 2    # 至少 2 个可用
  selector:
    matchLabels:
      app: api-server
```

### 回滚准备

```bash
# ✅ 部署前记录当前版本
CURRENT=$(kubectl get deploy api-server -n production -o jsonpath='{.spec.template.spec.containers[0].image}')
echo "当前版本: $CURRENT"

# 部署新版本
kubectl set image deploy/api-server api=registry.example.com/api:v2.2.0 -n production

# ✅ 验证部署
kubectl rollout status deploy/api-server -n production --timeout=300s

# 快速回滚（< 30s）
kubectl rollout undo deploy/api-server -n production
```

## 成本优化最佳实践

| 实践 | 预期节省 |
|------|----------|
| Right-Sizing（基于实际使用） | 20-40% |
| 开发环境缩零（KEDA Cron） | 30-50%（非工作时间） |
| Spot/抢占式实例（无状态） | 60-80% |
| 节点自动缩放（Cluster Autoscaler） | 15-30% |
| 资源配额（防过度申请） | 10-20% |
| 存储生命周期（冷热分层） | 30-50%（存储） |

## 运维检查清单

### 日常巡检

```bash
#!/bin/bash
# daily-check.sh
echo "=== 集群健康检查 $(date) ==="

# 节点状态
echo "--- 节点状态 ---"
kubectl get nodes -o wide
kubectl top nodes

# 异常 Pod
echo "--- 异常 Pod ---"
kubectl get pods -A --field-selector status.phase!=Running,status.phase!=Succeeded

# 最近事件
echo "--- 警告事件 ---"
kubectl get events -A --field-selector type=Warning --sort-by='.lastTimestamp' | tail -20

# 证书有效期
echo "--- 证书检查 ---"
kubeadm certs check-expiration 2>/dev/null || echo "非 kubeadm 集群"

# PVC 使用率
echo "--- 存储使用 ---"
kubectl get pvc -A | grep -v Bound

# 资源配额
echo "--- 配额使用 ---"
kubectl get resourcequota -A -o custom-columns=NS:.metadata.namespace,NAME:.metadata.name,CPU:.status.used.requests\\.cpu,MEM:.status.used.requests\\.memory
```

### 周度检查

| 检查项 | 命令/方法 |
|--------|-----------|
| 镜像漏洞扫描 | Trivy 扫描所有运行中镜像 |
| 资源使用趋势 | Grafana 周报 |
| 告警有效性审查 | 检查静默/过期告警 |
| 备份验证 | 恢复测试 |
| 证书到期预警 | 30 天内到期的证书 |
| 依赖版本 | K8s/组件版本 EOL 检查 |

## 反模式清单

| 反模式 | 风险 | 正确做法 |
|--------|------|----------|
| 使用 latest 标签 | 不可回滚 | 语义化版本/SHA |
| 无资源限制 | 资源争抢 | 设置 requests+limits |
| 共享 default SA | 权限过大 | 专用 SA + 最小权限 |
| 无健康检查 | 流量打到死 Pod | 三种探针全配 |
| 单副本生产 | 单点故障 | ≥ 3 副本 + PDB |
| 日志写文件 | 容器重启丢失 | stdout + 日志收集 |
| 无 NetworkPolicy | 东西向无隔离 | 默认拒绝 + 白名单 |
| 手动 kubectl apply | 无审计/回滚 | GitOps |
| 忽略 PDB | 维护导致全停 | 配置 PDB |
| 无备份 | 数据丢失 | Velero + 定期验证 |

## Related

- [[技能/集群运维/cluster-upgrade/最佳实践/bp-index.md|最佳实践]]
- [[技能/集群运维/cluster-upgrade/最佳实践/common-best-practices.md|通用最佳实践]]
- [[生产运维/index.md|生产运维]]
- [[安全/策略治理/index.md|策略治理]]
