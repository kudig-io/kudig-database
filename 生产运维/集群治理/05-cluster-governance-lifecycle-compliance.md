---
title: Kubernetes Cluster Governance — Multi-Tenancy, Compliance, and Lifecycle Management
description: K8s 集群治理 — 多租户治理、合规自动化、集群生命周期、升级策略、资源治理、组织策略
summary: 生产级 Kubernetes 集群治理体系，涵盖多租户管理、合规自动化与集群全生命周期管理
category: practice
tags:
- cluster-governance
- multi-tenancy
- compliance
- lifecycle
- resource-management
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: operations
---
# Kubernetes 集群治理体系

> 构建可持续的集群治理框架，平衡开发者自由与组织合规。

## 治理框架全景

```
┌─────────────────────────────────────────────────────────┐
│  组织策略层                                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │命名规范  │  │标签体系  │  │RBAC 模型 │             │
│  └──────────┘  └──────────┘  └──────────┘             │
├─────────────────────────────────────────────────────────┤
│  准入控制层                                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │OPA/      │  │Resource  │  │LimitRange│             │
│  │Kyverno   │  │Quota     │  │          │             │
│  └──────────┘  └──────────┘  └──────────┘             │
├─────────────────────────────────────────────────────────┤
│  运行时治理层                                            │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │PSA       │  │Network   │  │审计日志  │             │
│  │          │  │Policy    │  │          │             │
│  └──────────┘  └──────────┘  └──────────┘             │
├─────────────────────────────────────────────────────────┤
│  生命周期层                                              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐             │
│  │升级策略  │  │证书轮换  │  │容量规划  │             │
│  └──────────┘  └──────────┘  └──────────┘             │
└─────────────────────────────────────────────────────────┘
```

## 多租户治理模型

### 命名空间策略

```yaml
# 命名空间模板（通过 Kyverno generate）
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: namespace-provisioning
spec:
  rules:
    - name: add-default-resources
      match:
        resources:
          kinds: ["Namespace"]
      generate:
        synchronize: true
        apiVersion: v1
        kind: ResourceQuota
        name: default-quota
        namespace: "{{request.object.metadata.name}}"
        data:
          spec:
            hard:
              requests.cpu: "20"
              requests.memory: 40Gi
              limits.cpu: "40"
              limits.memory: 80Gi
              persistentvolumeclaims: "10"
              services.loadbalancers: "2"
              services.nodeports: "0"
    - name: add-limit-range
      match:
        resources:
          kinds: ["Namespace"]
      generate:
        synchronize: true
        apiVersion: v1
        kind: LimitRange
        name: default-limits
        namespace: "{{request.object.metadata.name}}"
        data:
          spec:
            limits:
              - type: Container
                default:
                  cpu: "1"
                  memory: 1Gi
                defaultRequest:
                  cpu: 100m
                  memory: 128Mi
                max:
                  cpu: "8"
                  memory: 16Gi
                min:
                  cpu: 10m
                  memory: 32Mi
              - type: Pod
                max:
                  cpu: "16"
                  memory: 32Gi
    - name: add-network-policy
      match:
        resources:
          kinds: ["Namespace"]
      generate:
        synchronize: true
        apiVersion: networking.k8s.io/v1
        kind: NetworkPolicy
        name: default-deny-ingress
        namespace: "{{request.object.metadata.name}}"
        data:
          spec:
            podSelector: {}
            policyTypes: ["Ingress"]
```

### 租户隔离矩阵

| 隔离级别 | 机制 | 适用场景 |
|----------|------|----------|
| 逻辑隔离 | Namespace + RBAC + ResourceQuota | 内部团队 |
| 网络隔离 | + NetworkPolicy | 多团队共享集群 |
| 节点隔离 | + NodeAffinity + Taints | 合规/性能敏感 |
| 集群隔离 | 独立集群 / vCluster | 外部租户/强合规 |

## 合规自动化

### CIS Benchmark 自动扫描

```yaml
# 使用 kube-bench CronJob
apiVersion: batch/v1
kind: CronJob
metadata:
  name: kube-bench
  namespace: security
spec:
  schedule: "0 4 * * 1"  # 每周一凌晨 4 点
  jobTemplate:
    spec:
      template:
        spec:
          hostPID: true
          containers:
            - name: kube-bench
              image: aquasec/kube-bench:v0.8.0
              command: ["kube-bench", "run", "--targets", "node,policies", "--json"]
              volumeMounts:
                - name: var-lib-kubelet
                  mountPath: /var/lib/kubelet
                  readOnly: true
                - name: etc-kubernetes
                  mountPath: /etc/kubernetes
                  readOnly: true
          volumes:
            - name: var-lib-kubelet
              hostPath:
                path: /var/lib/kubelet
            - name: etc-kubernetes
              hostPath:
                path: /etc/kubernetes
          restartPolicy: Never
```

### 策略合规报告

```bash
#!/bin/bash
# compliance-report.sh — 生成合规报告
echo "=== 集群合规报告 $(date) ==="

echo "--- PSA 标签覆盖 ---"
kubectl get ns -o json | jq -r '.items[] | 
  select(.metadata.labels["pod-security.kubernetes.io/enforce"] == null) | 
  .metadata.name' | grep -v "kube-"

echo "--- 无资源限制的 Deployment ---"
kubectl get deploy -A -o json | jq -r '.items[] | 
  select(.spec.template.spec.containers[] | .resources.limits == null) | 
  "\(.metadata.namespace)/\(.metadata.name)"'

echo "--- 特权容器 ---"
kubectl get pods -A -o json | jq -r '.items[] | 
  select(.spec.containers[]?.securityContext.privileged == true) | 
  "\(.metadata.namespace)/\(.metadata.name)"'

echo "--- 无 NetworkPolicy 的命名空间 ---"
for ns in $(kubectl get ns -o name | cut -d/ -f2 | grep -v kube); do
  count=$(kubectl get netpol -n $ns --no-headers 2>/dev/null | wc -l)
  [ "$count" -eq 0 ] && echo "  $ns: 无 NetworkPolicy"
done

echo "--- 镜像来源检查 ---"
kubectl get pods -A -o json | jq -r '.items[].spec.containers[].image' | \
  grep -v "registry.internal.example.com" | sort -u
```

## 集群生命周期管理

### 升级策略

| 策略 | 适用场景 | 风险 | 回滚 |
|------|----------|------|------|
| 原地升级（kubeadm） | 自建集群 | 中 | 快照回滚 |
| 滚动替换（云托管） | EKS/AKS/GKE | 低 | 新节点组 |
| 蓝绿集群 | 关键业务 | 最低 | 切回旧集群 |
| Cluster API | 多集群管理 | 低 | 声明式回滚 |

### 升级前检查

```bash
#!/bin/bash
# pre-upgrade-check.sh
TARGET="1.30"

echo "=== 升级前检查 → v$TARGET ==="

# 1. 废弃 API 检查
echo "--- 废弃 API ---"
kubent --target-versions $TARGET

# 2. 插件兼容性
echo "--- 插件版本 ---"
kubectl get deploy -n kube-system -o custom-columns=NAME:.metadata.name,IMAGE:.spec.template.spec.containers[0].image

# 3. 节点状态
echo "--- 节点状态 ---"
kubectl get nodes -o wide

# 4. PDB 检查
echo "--- PDB 状态 ---"
kubectl get pdb -A

# 5. 证书过期
echo "--- 证书检查 ---"
kubeadm certs check-expiration

# 6. etcd 健康
echo "--- etcd 健康 ---"
kubectl exec -n kube-system etcd-master-0 -- etcdctl endpoint health --cluster
```

## 资源治理

### 资源利用率审计

```promql
# 命名空间 CPU 利用率
sum(rate(container_cpu_usage_seconds_total{namespace!="kube-system"}[5m])) by (namespace)
/
sum(kube_pod_container_resource_requests{resource="cpu", namespace!="kube-system"}) by (namespace)

# 未使用 PVC
kube_persistentvolumeclaim_info * on(namespace, persistentvolumeclaim) 
  (kube_persistentvolumeclaim_status_phase{phase="Bound"} == 0)

# 过度配置的工作负载
avg(rate(container_cpu_usage_seconds_total[7d])) by (namespace, pod)
/
avg(kube_pod_container_resource_requests{resource="cpu"}) by (namespace, pod) < 0.2
```

### 自动 Right-Sizing（VPA 推荐）

```yaml
apiVersion: autoscaling.k8s.io/v1
kind: VerticalPodAutoscaler
metadata:
  name: all-workloads-vpa
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: "*"  # 所有 Deployment
  updatePolicy:
    updateMode: "Off"  # 仅推荐，不自动修改
  resourcePolicy:
    containerPolicies:
      - containerName: "*"
        minAllowed:
          cpu: 50m
          memory: 64Mi
```

## 治理度量

| 指标 | 目标 | 采集 |
|------|------|------|
| 命名空间合规率 | > 95% | Kyverno 策略违规数 |
| 资源利用率 | CPU > 40%, Mem > 50% | Prometheus |
| 升级及时性 | N-1 版本内 | 集群版本检查 |
| 证书有效期 | > 30 天 | cert-manager |
| 策略覆盖率 | 100% 命名空间有 NetPol | 审计脚本 |
| 镜像合规率 | 100% 来自内部 Registry | 准入控制 |

## Related

- [[生产运维/集群治理/index.md|集群治理]]
- [[生产运维/集群治理/03-admission-policy-governance.md|准入策略]]
- [[安全/合规审计/index.md|合规审计]]
