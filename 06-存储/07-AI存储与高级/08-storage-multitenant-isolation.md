---
title: "存储多租户隔离"
description: "Kubernetes 存储多租户隔离策略：Namespace 配额、StorageClass 访问控制与网络隔离"
summary: "覆盖 Namespace 级 PVC ResourceQuota、StorageClass RBAC 访问控制、CSI 驱动级隔离、容量配额、存储网络分离与审计合规"
category: 存储
tags:
- storage
- multi-tenancy
- isolation
- rbac
- quota
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- AI 工程师
estimated_read_time: 20min
intent_queries:
- "K8s 存储如何实现多租户隔离"
- "如何限制 Namespace 的存储使用量"
- "StorageClass 访问控制如何配置"
trigger_keywords:
- 多租户
- 存储隔离
- ResourceQuota
- StorageClass
- 配额
- RBAC
prerequisites:
- kubectl-basics
- storage-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 存储多租户隔离

## 概述

在多团队共享的 Kubernetes 集群中，存储资源的隔离是平台安全的核心维度之一。一个团队的存储误用（如无限创建 PVC、占满共享存储池、访问其他团队的卷）可能影响整个平台的稳定性和数据安全。存储多租户隔离需要从配额控制、访问权限、网络分离和审计追踪四个层面构建纵深防御体系。

本文覆盖从 Namespace 级 ResourceQuota 到 CSI 驱动级隔离的完整策略链，特别关注 AI 平台场景中多团队共享 GPU 集群时的存储隔离需求——训练数据隔离、模型 artifact 保护和 Checkpoint 存储配额是 AI 平台的典型挑战。

## 架构与核心概念

### 存储隔离层次模型

```
Storage Multi-Tenancy Isolation Layers:

Layer 1: Namespace 隔离
├── ResourceQuota (PVC 数量/容量限制)
├── LimitRange (单 PVC 大小限制)
└── NetworkPolicy (存储网络访问控制)

Layer 2: StorageClass 访问控制
├── RBAC (谁可以使用哪个 StorageClass)
├── 专用 StorageClass (团队专属存储池)
└── 默认 StorageClass 限制

Layer 3: CSI 驱动级隔离
├── 独立 CSI 实例 (不同后端存储)
├── 卷级加密 (per-tenant encryption key)
└── 存储后端隔离 (独立存储集群)

Layer 4: 数据级隔离
├── 卷加密 (Encryption at Rest)
├── 访问模式限制 (RWO vs RWX)
└── 数据分类标签

Layer 5: 审计与合规
├── 存储操作审计日志
├── 数据访问追踪
└── 合规报告
```

### 隔离策略对比

| 隔离级别 | 实现方式 | 隔离强度 | 管理复杂度 | 适用场景 |
|---------|---------|---------|-----------|---------|
| Namespace Quota | ResourceQuota | 低（共享存储池） | 低 | 内部团队 |
| StorageClass RBAC | Role/RoleBinding | 中（限制供给） | 中 | 多业务线 |
| 专用存储池 | 独立 CSI/后端 | 高（物理隔离） | 高 | 合规要求 |
| 独立集群 | 多集群 | 最高（完全隔离） | 最高 | 强监管行业 |

## 生产部署

### Namespace 级存储配额

🟡 中风险：ResourceQuota 会限制 Namespace 内新 PVC 的创建

```yaml
# AI 训练团队存储配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: ai-training-storage-quota
  namespace: ai-training
spec:
  hard:
    # PVC 总数限制
    persistentvolumeclaims: "20"
    # 总存储容量限制
    requests.storage: "50Ti"
    # 按 StorageClass 细分
    tier-hot-nvme.storageclass.storage.k8s.io/requests.storage: "10Ti"
    tier-hot-nvme.storageclass.storage.k8s.io/persistentvolumeclaims: "5"
    tier-warm-ssd.storageclass.storage.k8s.io/requests.storage: "30Ti"
    tier-warm-ssd.storageclass.storage.k8s.io/persistentvolumeclaims: "10"
    tier-cold-hdd.storageclass.storage.k8s.io/requests.storage: "10Ti"
    tier-cold-hdd.storageclass.storage.k8s.io/persistentvolumeclaims: "5"
---
# 单 PVC 大小限制
apiVersion: v1
kind: LimitRange
metadata:
  name: storage-limit-range
  namespace: ai-training
spec:
  limits:
    - type: PersistentVolumeClaim
      max:
        storage: 5Ti  # 单个 PVC 最大 5Ti
      min:
        storage: 1Gi  # 最小 1Gi，防止误创建
---
# 推理团队配额（较小）
apiVersion: v1
kind: ResourceQuota
metadata:
  name: ai-inference-storage-quota
  namespace: ai-inference
spec:
  hard:
    persistentvolumeclaims: "10"
    requests.storage: "5Ti"
    tier-hot-nvme.storageclass.storage.k8s.io/requests.storage: "2Ti"
    tier-warm-ssd.storageclass.storage.k8s.io/requests.storage: "3Ti"
```

### StorageClass RBAC 访问控制

🟡 中风险：修改 RBAC 可能影响现有工作负载的存储供给

```yaml
# 限制 ai-training 团队只能使用特定 StorageClass
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: storage-class-user
  namespace: ai-training
rules:
  # 允许查看可用的 StorageClass
  - apiGroups: ["storage.k8s.io"]
    resources: ["storageclasses"]
    verbs: ["get", "list"]
  # 允许管理自己 Namespace 的 PVC
  - apiGroups: [""]
    resources: ["persistentvolumeclaims"]
    verbs: ["get", "list", "watch", "create", "update", "delete"]
  # 允许创建快照
  - apiGroups: ["snapshot.storage.k8s.io"]
    resources: ["volumesnapshots"]
    verbs: ["get", "list", "create", "delete"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: ai-training-storage-binding
  namespace: ai-training
subjects:
  - kind: Group
    name: ai-training-team
    apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: storage-class-user
  apiGroup: rbac.authorization.k8s.io
---
# 集群级：限制哪些 StorageClass 对哪些 Namespace 可见
# 通过 Admission Webhook 实现（如 OPA/Gatekeeper）
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sAllowedStorageClasses
metadata:
  name: restrict-storage-classes
spec:
  match:
    namespaces:
      - "ai-training"
  parameters:
    allowedStorageClasses:
      - "tier-hot-nvme"
      - "tier-warm-ssd"
      - "tier-cold-hdd"
```

### 存储网络隔离

🟡 中风险：NetworkPolicy 配置不当可能阻断合法存储访问

```yaml
# 限制只有特定 Namespace 可以访问存储网络
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: storage-network-isolation
  namespace: storage-system
spec:
  podSelector:
    matchLabels:
      app: minio
  policyTypes:
    - Ingress
  ingress:
    # 只允许 ai-platform 和 ai-training 访问 MinIO
    - from:
        - namespaceSelector:
            matchLabels:
              storage-access: "ai-data"
      ports:
        - protocol: TCP
          port: 9000
        - protocol: TCP
          port: 9001
---
# 为 Namespace 添加存储访问标签
# kubectl label namespace ai-training storage-access=ai-data
# kubectl label namespace ai-inference storage-access=ai-data
```

## 运维操作

### 配额使用审计

🟢 低风险/只读：查看各 Namespace 存储使用情况

```bash
# 查看各 Namespace PVC 使用量
kubectl get pvc --all-namespaces -o json | \
  jq -r '.items[] | [.metadata.namespace, .metadata.name, .spec.storageClassName, .status.capacity.storage // "pending"] | @tsv' | \
  sort -k1,1 -k3,3

# 查看 ResourceQuota 使用率
kubectl get resourcequota --all-namespaces -o json | \
  jq -r '.items[] | {namespace: .metadata.namespace, name: .metadata.name, 
  storage_used: .status.used["requests.storage"], 
  storage_limit: .status.hard["requests.storage"],
  pvc_used: .status.used["persistentvolumeclaims"],
  pvc_limit: .status.hard["persistentvolumeclaims"]}'

# 按 StorageClass 汇总容量
kubectl get pv -o json | \
  jq -r '.items[] | [.spec.storageClassName, .spec.capacity.storage, .spec.claimRef.namespace] | @tsv' | \
  awk '{sc[$1]+=$2; ns[$3]+=$2} END {print "=== By StorageClass ==="; for(s in sc) print s, sc[s]; print "=== By Namespace ==="; for(n in ns) print n, ns[n]}'
```

### 跨 Namespace 卷访问防护

🟢 低风险/只读：检查是否存在跨 Namespace 卷引用

```bash
# 检查 PV 的 claimRef 是否指向正确的 Namespace
kubectl get pv -o json | \
  jq -r '.items[] | select(.spec.claimRef != null) | 
  {pv: .metadata.name, namespace: .spec.claimRef.namespace, pvc: .spec.claimRef.name}'

# 检查是否有 Pod 引用了其他 Namespace 的 PVC（不应存在）
kubectl get pods --all-namespaces -o json | \
  jq -r '.items[] | {pod: .metadata.name, ns: .metadata.namespace, 
  volumes: [.spec.volumes[]? | select(.persistentVolumeClaim != null) | .persistentVolumeClaim.claimName]}'
```

### 容量配额告警

🟢 低风险/只读：配置 Prometheus 告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: storage-quota-alerts
  namespace: monitoring
spec:
  groups:
    - name: storage-quota
      rules:
        - alert: NamespaceStorageQuotaNearLimit
          expr: |
            kube_resourcequota{type="used", resource="requests.storage"} /
            kube_resourcequota{type="hard", resource="requests.storage"} > 0.85
          for: 1h
          labels:
            severity: warning
          annotations:
            summary: "Namespace {{ $labels.namespace }} 存储配额使用超过 85%"
        - alert: PVCCreationThrottled
          expr: |
            kube_resourcequota{type="used", resource="persistentvolumeclaims"} ==
            kube_resourcequota{type="hard", resource="persistentvolumeclaims"}
          for: 30m
          labels:
            severity: critical
          annotations:
            summary: "Namespace {{ $labels.namespace }} PVC 数量已达上限"
```

## 故障排查

### PVC 创建被拒绝

🟢 低风险/只读：诊断配额/RBAC 拒绝

```bash
# 查看 PVC 创建失败事件
kubectl get events -n ai-training --field-selector reason=FailedCreate

# 检查 ResourceQuota 是否已满
kubectl describe resourcequota -n ai-training

# 检查 RBAC 权限
kubectl auth can-i create persistentvolumeclaims -n ai-training --as=system:serviceaccount:ai-training:default

# 检查 StorageClass 是否被 Gatekeeper 策略阻止
kubectl get events -n ai-training --field-selector reason=DeniedByPolicy
```

### 常见隔离问题

| 问题 | 原因 | 排查 | 修复 |
|------|------|------|------|
| PVC 创建报 "exceeded quota" | ResourceQuota 已满 | `kubectl describe resourcequota` | 清理无用 PVC 或提升配额 |
| "storageclass not found" | RBAC 限制或 SC 不存在 | `kubectl auth can-i` | 检查 Role/ClusterRole |
| Pod 无法挂载 PVC | PVC 在其他 Namespace | 检查 PVC 和 Pod 的 Namespace | PVC 必须与 Pod 同 Namespace |
| 存储网络不通 | NetworkPolicy 阻断 | `kubectl describe networkpolicy` | 添加 Namespace 标签 |
| 卷加密密钥无法访问 | KMS 权限隔离 | 检查 CSI 驱动 ServiceAccount | 配置 per-tenant KMS key |

### 审计日志分析

```bash
# 🟢 低风险/只读：查看存储相关操作审计
# 通过 Kubernetes Audit Log
kubectl logs -n kube-system -l component=kube-apiserver | \
  grep -E "persistentvolume|storageclass" | tail -50

# 查看最近的 PVC 创建/删除操作
kubectl get events --all-namespaces --field-selector reason=ProvisioningSucceeded --sort-by='.lastTimestamp'
```

## 最佳实践

1. **配额先行**：每个 Namespace 创建时必须配置 ResourceQuota，按 StorageClass 细分限额
2. **最小权限**：通过 RBAC 限制团队只能使用授权的 StorageClass，参考 [[22-概念/05-安全/rbac-authorization.md|RBAC 授权]]
3. **存储网络分离**：存储流量（iSCSI/NFS/MinIO）使用独立 VLAN 或 NetworkPolicy 隔离
4. **加密隔离**：敏感数据使用 per-tenant KMS Key 加密，参考 [[06-存储/01-K8s存储/18-storage-encryption-at-rest.md|存储加密]]
5. **定期审计**：每月生成各团队存储使用报告，识别异常增长和闲置资源
6. **标签规范**：所有 PVC 必须携带 `team`、`project`、`data-classification` 标签
7. **自动清理**：配置 CronJob 清理超过保留期的 PVC 和快照，参考 [[06-存储/07-AI存储与高级/04-data-tiering-ilm-archival.md|数据分层与生命周期管理]]
8. **AI 数据隔离**：训练数据按项目/团队隔离桶或目录，MinIO IAM Policy 限制跨团队访问，参考 [[06-存储/07-AI存储与高级/01-minio-object-storage-ai.md|MinIO 对象存储]]
9. **合规报告**：为监管要求生成数据驻留和访问审计报告，参考 [[22-概念/05-安全/multi-tenancy-isolation.md|多租户隔离概念]]

## Related

- [[22-概念/05-安全/multi-tenancy-isolation.md|多租户隔离概念]]
- [[22-概念/05-安全/rbac-authorization.md|RBAC 授权]]
- [[06-存储/01-K8s存储/18-storage-encryption-at-rest.md|存储加密]]
- [[06-存储/07-AI存储与高级/04-data-tiering-ilm-archival.md|数据分层与生命周期管理]]
- [[06-存储/01-K8s存储/13-storage-security-compliance.md|存储安全合规]]
