---
title: Pod Security Admission 深度配置
description: 'PSA 替代 PSP：Privileged/Baseline/Restricted 三级策略、命名空间标签配置、从 PSP 迁移到 PSA、自定义准入策略补充、渐进式部署'
summary: 'PSA 替代 PSP：三级安全策略、命名空间标签配置、PSP 迁移与渐进式部署'
category: security-compliance
tags:
- pod-security
- admission-controller
- psp
- psa
- security-policy
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 安全工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Pod Security Admission 是什么
- 如何配置 PSA 替代 PSP
trigger_keywords:
- Pod Security Admission
- PSA
- PSP
- Pod Security Standards
- 准入控制
prerequisites:
- kubectl-basics
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


# Pod Security Admission 深度配置

## 概述

Pod Security Admission (PSA) 是 Kubernetes 1.25+ 正式移除 PodSecurityPolicy (PSP) 后的官方替代方案。PSA 基于 Pod Security Standards 定义的三级安全模型，通过命名空间标签实现声明式安全控制，无需维护复杂的 Webhook。

## 1. Pod Security Standards 三级模型

### 1.1 Privileged（特权级）

无限制策略，允许所有已知的特权提升。适用于系统级工作负载。

```yaml
# 典型使用场景
# - kube-system 命名空间
# - CNI 插件（Calico、Cilium DaemonSet）
# - 节点级日志采集器（Fluentd、Filebeat）
# - CSI 驱动
```

允许的特权操作：
- `hostNetwork: true`
- `hostPID: true`
- `hostIPC: true`
- `privileged: true`
- 所有 Volume 类型（包括 hostPath）
- 所有 seccomp/AppArmor 配置
- `runAsUser: 0`（root）

### 1.2 Baseline（基线级）

限制已知的特权提升，阻止最危险的配置。适用于大多数工作负载。

核心限制：
```yaml
# 禁止的配置
spec:
  hostNetwork: false          # 禁止使用主机网络
  hostPID: false              # 禁止使用主机 PID 命名空间
  hostIPC: false              # 禁止使用主机 IPC 命名空间
  containers:
  - securityContext:
      privileged: false       # 禁止特权容器
      allowPrivilegeEscalation: false  # 禁止特权提升
    # 禁止的 capabilities
    # - SYS_ADMIN, SYS_PTRACE, SYS_MODULE, etc.
```

Volume 类型限制：
```yaml
# Baseline 允许的 Volume 类型
- configMap
- csi
- downwardAPI
- emptyDir
- ephemeral
- persistentVolumeClaim
- projected
- secret

# Baseline 禁止的 Volume 类型
- hostPath      # 需要 Restricted 或 Privileged
- flexVolume    # 需要 Privileged
```

### 1.3 Restricted（限制级）

最严格策略，强制最佳安全实践。适用于高安全要求的工作负载。

核心要求：
```yaml
spec:
  securityContext:
    runAsNonRoot: true          # 必须以非 root 运行
    seccompProfile:
      type: RuntimeDefault     # 必须设置 seccomp
  containers:
  - securityContext:
      allowPrivilegeEscalation: false
      capabilities:
        drop:
          - ALL                # 必须丢弃所有 capabilities
      runAsNonRoot: true
      runAsUser: 65534         # 推荐使用 nobody
      seccompProfile:
        type: RuntimeDefault
```

## 2. 命名空间标签配置

### 2.1 标签语法

PSA 通过三个命名空间标签控制策略：

```yaml
metadata:
  labels:
    # 模式：enforce / warn / audit
    pod-security.kubernetes.io/enforce: <level>
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/warn: <level>
    pod-security.kubernetes.io/warn-version: latest
    pod-security.kubernetes.io/audit: <level>
    pod-security.kubernetes.io/audit-version: latest
```

三种模式说明：
- `enforce`：拒绝不合规的 Pod 创建请求
- `warn`：允许创建但显示警告
- `audit`：允许创建但记录审计日志

### 2.2 典型配置模式

```yaml
# 生产命名空间：enforce Restricted + warn/audit 同级
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/warn: restricted
    pod-security.kubernetes.io/warn-version: latest
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/audit-version: latest
---
# 预发布命名空间：enforce Baseline + warn Restricted
apiVersion: v1
kind: Namespace
metadata:
  name: staging
  labels:
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/warn: restricted
    pod-security.kubernetes.io/warn-version: latest
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/audit-version: latest
---
# 开发命名空间：enforce Baseline + audit Restricted（不阻断开发）
apiVersion: v1
kind: Namespace
metadata:
  name: development
  labels:
    pod-security.kubernetes.io/enforce: baseline
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/warn: baseline
    pod-security.kubernetes.io/warn-version: latest
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/audit-version: latest
```

### 2.3 批量配置脚本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# 批量为命名空间设置 PSA 标签
set -euo pipefail

LEVEL="${1:-baseline}"

for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}'); do
  # 跳过系统命名空间
  if [[ "$ns" == "kube-system" || "$ns" == "kube-public" || "$ns" == "kube-node-lease" ]]; then
    echo "Skipping system namespace: $ns"
    continue
  fi

  echo "Configuring PSA for namespace: $ns (level: $LEVEL)"
  kubectl label namespace "$ns" \
    pod-security.kubernetes.io/enforce="$LEVEL" \
    pod-security.kubernetes.io/enforce-version=latest \
    pod-security.kubernetes.io/warn="$LEVEL" \
    pod-security.kubernetes.io/warn-version=latest \
    pod-security.kubernetes.io/audit=restricted \
    pod-security.kubernetes.io/audit-version=latest \
    --overwrite
done
```
## 3. 从 PSP 迁移到 PSA

### 3.1 PSP 到 PSA 映射

| PSP 字段 | PSA 等效 | 说明 |
|---------|---------|------|
| `privileged: false` | Baseline | Privileged 模式禁止 |
| `allowPrivilegeEscalation: false` | Restricted | 禁止特权提升 |
| `requiredDropCapabilities` | Restricted (drop ALL) | 必须丢弃所有 capabilities |
| `runAsUser: MustRunAsNonRoot` | Restricted | 必须非 root 运行 |
| `hostNetwork: false` | Baseline | 禁止主机网络 |
| `hostPID: false` | Baseline | 禁止主机 PID |
| `volumes: [configMap, secret, emptyDir, ...]` | Baseline | 限制 Volume 类型 |
| `seLinux` / `supplementalGroups` | 无直接等效 | 需要自定义准入补充 |

### 3.2 迁移检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 检查哪些命名空间还在使用 PSP
echo "=== Namespaces with PSP-restricted workloads ==="

for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}'); do
  # 检查是否有 Pod 使用了需要 PSP 的配置
  issues=$(kubectl get pods -n "$ns" -o json 2>/dev/null | \
    jq -r '.items[] | select(
      .spec.hostNetwork == true or
      .spec.hostPID == true or
      .spec.containers[].securityContext.privileged == true or
      .spec.containers[].securityContext.allowPrivilegeEscalation == true
    ) | .metadata.name' 2>/dev/null)

  if [ -n "$issues" ]; then
    echo ""
    echo "Namespace: $ns"
    echo "$issues" | while read -r pod; do
      echo "  - Pod: $pod"
    done
  fi
done

echo ""
echo "=== Namespaces without PSA labels ==="
kubectl get ns -o json | jq -r '.items[] |
  select(.metadata.labels["pod-security.kubernetes.io/enforce"] == null) |
  .metadata.name'
```
### 3.3 PSP 与 PSA 并行过渡

在 Kubernetes 1.25 之前的版本，可以同时运行 PSP 和 PSA：

```yaml
# 过渡期：同时启用 PSP 和 PSA
# Step 1: 标记所有命名空间为 warn（不阻断）
# Step 2: 分析警告日志，修复不合规工作负载
# Step 3: 切换到 enforce

# 使用 Kyverno 作为过渡期的策略引擎
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: psp-migration-policy
spec:
  validationFailureAction: audit  # 先审计模式
  background: true
  rules:
  - name: restrict-privileged
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "Privileged containers are not allowed"
      pattern:
        spec:
          containers:
          - securityContext:
              privileged: false
```

## 4. 自定义准入策略补充

PSA 不覆盖所有安全场景，需要结合其他准入控制器：

### 4.1 Kyverno 策略补充

```yaml
# 限制 hostPath 挂载路径
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: restrict-hostpath
spec:
  validationFailureAction: enforce
  background: true
  rules:
  - name: validate-hostpath
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "hostPath volumes are restricted to /data"
      pattern:
        spec:
          =(volumes):
          - =(hostPath):
              path: /data/*
---
# 强制设置资源限制
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-resource-limits
spec:
  validationFailureAction: enforce
  background: true
  rules:
  - name: check-resource-limits
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "CPU and memory limits are required"
      pattern:
        spec:
          containers:
          - resources:
              limits:
                memory: "?*"
                cpu: "?*"
```

### 4.2 OPA Gatekeeper 策略补充

```yaml
# 限制镜像来源
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sAllowedRepos
metadata:
  name: allowed-repos
spec:
  match:
    kinds:
    - apiGroups: [""]
      kinds: ["Pod"]
    namespaces:
    - production
    - staging
  parameters:
    repos:
    - "registry.example.com/"
    - "gcr.io/google-containers/"
```

## 5. 渐进式部署策略

### 5.1 四阶段部署

```
阶段 1: dry-run（审计模式）
  ├── 所有命名空间设置 audit: restricted
  ├── 收集审计日志，识别不合规工作负载
  ├── 建立基线：记录当前合规率
  └── 时间：1-2 周

阶段 2: warn（警告模式）
  ├── 添加 warn: restricted 标签
  ├── 开发者开始看到警告信息
  ├── 修复高频不合规问题
  └── 时间：2-4 周

阶段 3: enforce 基线级
  ├── enforce: baseline（阻断最危险配置）
  ├── warn: restricted（继续警告）
  ├── 确保所有工作负载满足 Baseline
  └── 时间：2-4 周

阶段 4: enforce 限制级
  ├── enforce: restricted
  ├── 仅允许符合安全最佳实践的工作负载
  ├── 例外通过专门的 privileged 命名空间处理
  └── 持续监控和优化
```

### 5.2 监控合规状态

```yaml
# Prometheus 规则：监控 PSA 违规
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: psa-compliance
spec:
  groups:
  - name: psa.rules
    rules:
    - alert: PSAPolicyViolation
      expr: |
        sum(rate(apiserver_audit_event_total{
          verb="create",
          objectRef_resource="pods",
          response_status_code=~"403|409"
        }[5m])) > 0
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "PSA policy violation detected"
        description: "{{ $value }} pod creation requests rejected by PSA in last 5m"

    - record: psa:compliance:rate
      expr: |
        1 - (
          sum(rate(apiserver_audit_event_total{
            verb="create",
            objectRef_resource="pods",
            response_status_code=~"403"
          }[1h]))
          /
          sum(rate(apiserver_audit_event_total{
            verb="create",
            objectRef_resource="pods"
          }[1h]))
        )
      labels:
        check: "psa_compliance"
```

## 6. 常见问题排查

### 6.1 PSA 拒绝 Pod 创建

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 PSA 拒绝的具体原因
kubectl get events -n <namespace> --field-selector reason=FailedCreate

# 常见错误信息：
# Warning  FailedCreate  pod "app-xxx"  forbidden: violates PodSecurity
# "restricted:latest": allowPrivilegeEscalation != false
#   (container "app" must set securityContext.allowPrivilegeEscalation=false),
#   unrestricted capabilities (container "app" must set
#   securityContext.capabilities.drop=["ALL"])

# 修复示例
kubectl patch deployment app -n <namespace> --type=json -p='[
  {"op": "add", "path": "/spec/template/spec/containers/0/securityContext", "value": {
    "allowPrivilegeEscalation": false,
    "capabilities": {"drop": ["ALL"]},
    "runAsNonRoot": true,
    "seccompProfile": {"type": "RuntimeDefault"}
  }}
]'
```
### 6.2 绕过 PSA 的正确方式

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 错误方式：直接降低命名空间安全级别
# 正确方式：将特权工作负载隔离到专用命名空间

# 创建专用的 privileged 命名空间
kubectl create namespace system-privileged
kubectl label namespace system-privileged \
  pod-security.kubernetes.io/enforce=privileged \
  pod-security.kubernetes.io/enforce-version=latest \
  pod-security.kubernetes.io/warn=privileged \
  pod-security.kubernetes.io/audit=privileged
```
## 7. 最佳实践总结

```
PSA 部署检查清单：

□ 评估现有工作负载的安全需求
□ 确定每个命名空间的目标安全级别
□ 使用 audit 模式收集基线数据
□ 修复不合规工作负载（而非降低策略）
□ 渐进式推进：audit → warn → enforce baseline → enforce restricted
□ 配合 Kyverno/Gatekeeper 补充 PSA 未覆盖的策略
□ 监控 PSA 违规指标
□ 定期审查命名空间标签配置
□ 将特权工作负载隔离到专用命名空间
□ 记录所有例外和豁免原因
```

## Related

- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-05-security-compliance/01-identity-access/06-rbac-matrix-configuration|RBAC 最佳实践]]
- [[32-发布/package/2026-07-02_18-53/corpus/supporting/domain-05-security-compliance/03-runtime-security/04-runtime-security-defense|Seccomp 与 AppArmor]]

## See Also

- [Kubernetes Pod Security Standards](https://kubernetes.io/docs/concepts/security/pod-security-standards/)
- [PSA 官方文档](https://kubernetes.io/docs/concepts/security/pod-security-admission/)


<!-- risk-assessed -->
