---
title: Kyverno [entities]
description: Kyverno — Kubernetes 生产运维知识库
summary: Kyverno — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- security
- policy
- kyverno
- admission
- governance
- opa
- networkpolicy
- operator
- webhook
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kyverno 是什么
- 如何 Kyverno
trigger_keywords:
- Kyverno
prerequisites:
- kubectl-basics
- gitops-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kyverno

> Kyverno 是 CNCF 毕业项目，Kubernetes 原生策略引擎，使用 YAML 语法定义策略，无需学习 Rego，支持验证、变更、生成、清理、镜像验证五大模式。

## 基本信息

| 属性 | 值 |
|------|------|
| CNCF 状态 | 毕业 (Graduated, 2023 孵化) |
| 语言 | Go (策略用 YAML) |
| 机制 | Admission Webhooks (Validating/Mutating) |
| 模式 | Validate, Mutate, Generate, Cleanup, ImageVerify |
| 官网 | https://kyverno.io |
| GitHub | https://github.com/kyverno/kyverno |
| 策略库 | https://kyverno.io/policies/ (300+ 现成策略) |

## 架构设计

```
┌─────────────────────────────────────────────────────┐
│              Kubernetes API Server                    │
│                      │                               │
│              Admission Webhook                       │
│                      │                               │
│                      ▼                               │
│  ┌──────────────────────────────────────────┐  │
│  │            Kyverno Webhook Pod             │  │
│  │                                            │  │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐  │  │
│  │  │Validate │ │ Mutate  │ │Generate │  │  │
│  │  └─────────┘ └─────────┘ └─────────┘  │  │
│  │  ┌─────────┐ ┌───────────────┐  │  │
│  │  │ Cleanup │ │ ImageVerify   │  │  │
│  │  └─────────┘ └───────────────┘  │  │
│  └──────────────────────────────────────────┘  │
│                      │                               │
│                      ▼                               │
│  ┌──────────────────────────────────────────┐  │
│  │     Policy Report / ClusterPolicyReport    │  │
│  └──────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────┘
```

## 策略模式详解

| 模式 | 说明 | 典型场景 |
|------|------|----------|
| Validate | 拒绝不合规资源 | 强制 resource limits、禁止 root |
| Mutate | 自动修复不合规资源 | 添加默认 label、设置 securityContext |
| Generate | 创建依赖资源 | 每个 Namespace 自动创建 NetworkPolicy |
| Cleanup | 删除过期资源 | 清理过期测试环境 |
| ImageVerify | 验证镜像签名 | 阻止未签名镜像部署 |

## 安装与配置

```bash
# 🟢 Helm 安装
helm repo add kyverno https://kyverno.github.io/kyverno/
helm install kyverno kyverno/kyverno \
  --namespace kyverno \
  --create-namespace \
  --set replicaCount=3

# 🟢 验证安装
kubectl get pods -n kyverno
kubectl get clusterpolicies

# 🟢 查看策略报告
kubectl get policyreport -A
kubectl get clusterpolicyreport
```

## 策略示例

### Validate: 强制资源限制

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-resource-limits
spec:
  validationFailureAction: Enforce  # Enforce=拒绝, Audit=仅记录
  background: true
  rules:
  - name: check-resource-limits
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "CPU 和 Memory 的 requests 和 limits 必须设置"
      pattern:
        spec:
          containers:
          - resources:
              requests:
                memory: "?*"
                cpu: "?*"
              limits:
                memory: "?*"
                cpu: "?*"
```

### Mutate: 添加默认标签

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: add-default-labels
spec:
  rules:
  - name: add-team-label
    match:
      any:
      - resources:
          kinds:
          - Pod
    mutate:
      patchStrategicMerge:
        metadata:
          labels:
            +(managed-by): kyverno
            +(environment): "{{request.namespace}}"
```

### Generate: 自动创建 NetworkPolicy

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: generate-network-policy
spec:
  rules:
  - name: default-deny-ingress
    match:
      any:
      - resources:
          kinds:
          - Namespace
    generate:
      apiVersion: networking.k8s.io/v1
      kind: NetworkPolicy
      name: default-deny-ingress
      namespace: "{{request.object.metadata.name}}"
      synchronize: true
      data:
        spec:
          podSelector: {}
          policyTypes:
          - Ingress
```

### ImageVerify: 镜像签名验证

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signatures
spec:
  validationFailureAction: Enforce
  rules:
  - name: verify-cosign-signature
    match:
      any:
      - resources:
          kinds:
          - Pod
    verifyImages:
    - imageReferences:
      - "registry.example.com/*"
      attestors:
      - count: 1
        entries:
        - keys:
            publicKeys: |-
              -----BEGIN PUBLIC KEY-----
              MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
              -----END PUBLIC KEY-----
```

## 运维操作

### 常用命令

```bash
# 🟢 查看所有策略
kubectl get clusterpolicies
kubectl get policies -A  # 命名空间级

# 🟢 查看策略详情
kubectl describe clusterpolicy require-resource-limits

# 🟢 查看策略报告
kubectl get policyreport -A -o wide
kubectl get clusterpolicyreport -o yaml

# 🟢 测试策略 (dry-run)
kubectl apply --dry-run=server -f pod.yaml

# 🟡 临时禁用策略
kubectl patch clusterpolicy <name> -p '{"spec":{"validationFailureAction":"Audit"}}'

# 🟡 排除特定命名空间
kubectl patch clusterpolicy <name> --type merge -p '
  {"spec":{"rules":[{"name":"rule1","exclude":{"resources":{"namespaces":["kube-system"]}}}]}}'

# 🟢 查看 Webhook 状态
kubectl get validatingwebhookconfigurations | grep kyverno
kubectl get mutatingwebhookconfigurations | grep kyverno
```

## 故障排查

### 常见问题

| 问题 | 原因 | 解决方案 |
|------|------|----------|
| 策略不生效 | Webhook 未注册 | 检查 webhookconfiguration |
| 误拒绝合法资源 | 策略规则过严 | 先设为 Audit 模式测试 |
| Kyverno Pod CrashLoop | 资源不足 | 增加 CPU/Memory |
| 策略冲突 | 多个 Mutate 策略冲突 | 调整优先级/合并策略 |
| API Server 超时 | Webhook 响应慢 | 检查 Kyverno 负载 |
| Generate 失败 | RBAC 权限不足 | 检查 Kyverno ServiceAccount |

### 排查流程

```
1. 检查 Kyverno Pod 状态
   kubectl get pods -n kyverno
       │
2. 检查 Webhook 注册
   kubectl get validatingwebhookconfigurations | grep kyverno
       │
3. 查看策略状态
   kubectl describe clusterpolicy <name>
       │
4. 查看策略报告
   kubectl get policyreport -A
       │
5. 查看 Kyverno 日志
   kubectl logs -n kyverno -l app.kubernetes.io/name=kyverno --tail=100
```

## Kyverno vs OPA/Gatekeeper

| 特性 | Kyverno | OPA/Gatekeeper |
|------|---------|----------------|
| 策略语言 | YAML (K8s 原生) | Rego (专用语言) |
| 学习曲线 | 低 (K8s 工程师即可) | 高 (需学 Rego) |
| Mutate | 支持 | 支持 |
| Generate | 支持 | 不支持 |
| ImageVerify | 原生支持 | 需额外工具 |
| 策略库 | 300+ | 200+ |
| 性能 | 中等 | 高 (编译后) |
| CEL 支持 | 支持 (1.11+) | 支持 |

## 生产案例

### 案例1：策略误拒绝导致部署失败

**症状：** 所有新 Pod 无法创建

**根因：** 新策略设为 Enforce 但未充分测试

**解决：** 先设为 Audit，观察 policyreport 确认无误后再切 Enforce

### 案例2：Kyverno Webhook 导致 API Server 超时

**症状：** kubectl 操作超时

**根因：** Kyverno Pod OOMKilled，Webhook 无响应

**解决：** 增加资源限制，配置 `failurePolicy: Ignore` 作为安全网

## 版本兼容矩阵

| Kyverno | K8s | 重要变化 |
|---------|-----|----------|
| 1.10+ | 1.25+ | CEL 支持 |
| 1.11+ | 1.27+ | ValidatingAdmissionPolicy |
| 1.12+ | 1.28+ | 性能优化 |

## 检查清单

- [ ] 理解 Kyverno 五大策略模式
- [ ] 能编写 Validate/Mutate/Generate 策略
- [ ] 掌握 Audit vs Enforce 模式区别
- [ ] 能排查策略不生效问题
- [ ] 了解镜像签名验证 (ImageVerify)
- [ ] 掌握 Kyverno vs OPA 选型
- [ ] 能配置策略报告和监控

## Related

- [[deployment]] — Deployment
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/cloud-native-defense-in-depth.md|Cloud Native Defense in Depth]]
- [[supply-chain-security]] — Software Supply Chain Security
- [[实体/networkpolicy.md|NetworkPolicy]]
- [[实体/argocd.md|ArgoCD]]
- [[实体/trivy.md|Trivy]]
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]]
- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]

<!-- risk-assessed -->
