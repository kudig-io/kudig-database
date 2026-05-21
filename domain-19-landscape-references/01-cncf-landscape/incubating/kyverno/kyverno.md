---
title: Kyverno
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- opa
- ingress
- networkpolicy
- argocd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Kyverno 是什么
- 如何 Kyverno
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Kyverno
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- gitops-basics
- policy-basics
---

title: Kyverno
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- opa
- ingress
- networkpolicy
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Kyverno 是什么
- 如何 Kyverno
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Kyverno
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Kyverno

> **成熟度**: Incubating | **加入时间**: 2020-11 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://kyverno.io |
| **GitHub** | https://github.com/kyverno/kyverno |
| **文档** | https://kyverno.io/docs |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Security |

---

## 项目概述

### 简介
Kyverno 是 Kubernetes 原生的策略引擎，使用 YAML 定义策略，无需学习新的策略语言。它支持验证、变更和生成资源，简化 Kubernetes 策略管理。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2019 | Nirmata 公司创建 |
| 2020-11 | 加入 CNCF Sandbox |
| 2022-07 | 晋升为 CNCF Incubating |

### 核心定位
Kyverno 是 OPA/Gatekeeper 的 Kubernetes 原生替代方案，以"无需学习新语言"著称，使用熟悉的 YAML 定义策略。

---

## 架构设计

### 策略类型

```
┌─────────────────────────────────────────────────────────────────┐
│                   Kyverno 策略类型                               │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. Validate (验证)                                              │
│     ┌──────────┐     ┌──────────┐     ┌──────────┐             │
│     │ Resource │────►│ Kyverno  │────►│ Allow/   │             │
│     │ Request  │     │ Validate │     │ Deny     │             │
│     └──────────┘     └──────────┘     └──────────┘             │
│                                                                  │
│  2. Mutate (变更)                                                │
│     ┌──────────┐     ┌──────────┐     ┌──────────┐             │
│     │ Resource │────►│ Kyverno  │────►│ Modified │             │
│     │ Request  │     │ Mutate   │     │ Resource │             │
│     └──────────┘     └──────────┘     └──────────┘             │
│                                                                  │
│  3. Generate (生成)                                              │
│     ┌──────────┐     ┌──────────┐     ┌──────────┐             │
│     │ Resource │────►│ Kyverno  │────►│ New      │             │
│     │ Created  │     │ Generate │     │ Resource │             │
│     └──────────┘     └──────────┘     └──────────┘             │
│                                                                  │
│  4. VerifyImages (镜像验证)                                      │
│     ┌──────────┐     ┌──────────┐     ┌──────────┐             │
│     │ Pod with │────►│ Kyverno  │────►│ Allow if │             │
│     │ Image    │     │ Verify   │     │ Signed   │             │
│     └──────────┘     └──────────┘     └──────────┘             │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 策略示例

### 验证策略

```yaml
# 要求所有 Pod 必须有资源限制
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-resource-limits
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: validate-resources
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

### 变更策略

```yaml
# 自动添加标签
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: add-labels
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
              managed-by: kyverno
```

### 生成策略

```yaml
# 自动创建 NetworkPolicy
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: generate-network-policy
spec:
  rules:
    - name: default-deny
      match:
        any:
          - resources:
              kinds:
                - Namespace
      generate:
        apiVersion: networking.k8s.io/v1
        kind: NetworkPolicy
        name: default-deny
        namespace: "{{request.object.metadata.name}}"
        data:
          spec:
            podSelector: {}
            policyTypes:
              - Ingress
              - Egress
```

### 镜像签名验证

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signature
spec:
  validationFailureAction: Enforce
  rules:
    - name: verify-signature
      match:
        any:
          - resources:
              kinds:
                - Pod
      verifyImages:
        - imageReferences:
            - "registry.example.com/*"
          attestors:
            - entries:
                - keys:
                    publicKeys: |
                      -----BEGIN PUBLIC KEY-----
                      ...
                      -----END PUBLIC KEY-----
```

---

## 安装部署

```bash
# Helm 安装
helm repo add kyverno https://kyverno.github.io/kyverno/
helm install kyverno kyverno/kyverno -n kyverno --create-namespace

# 验证
kubectl get pods -n kyverno
kubectl get clusterpolicies
```

---

## Kyverno vs OPA/Gatekeeper

| 特性 | Kyverno | OPA/Gatekeeper |
|:---|:---|:---|
| **策略语言** | YAML | Rego |
| **学习曲线** | 低 | 高 |
| **变更策略** | 原生支持 | 需额外配置 |
| **生成资源** | 原生支持 | 不支持 |
| **镜像验证** | 原生支持 | 需 Cosign |

---

## 参考资源

- [官方文档](https://kyverno.io/docs)
- [GitHub Repo](https://github.com/kyverno/kyverno)
- [CNCF 项目页面](https://www.cncf.io/projects/kyverno/)
- [策略库](https://kyverno.io/policies/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[synthesis/IaC x 多集群管理|基础设施即代码 x 多集群管理]] — Cross-reference
- [[synthesis/纵深防御 x 供应链安全|纵深防御 x 供应链安全]] — Cross-reference
- [[synthesis/控制器模式 × Operator 模式|控制器模式 × Operator 模式]] — Cross-reference
- [[concepts/cloud-native-defense-in-depth|Cloud Native Defense in Depth]] — Cross-reference
- [[entities/argocd|ArgoCD]] — Cross-reference
- [[entities/trivy|Trivy]] — Cross-reference
- [[entities/cncf-security|CNCF 安全与合规项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
