---
title: 策略即代码（Policy as Code）
description: '## 概述'
summary: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- harbor
- opa
- ingress
- networkpolicy
- webhook
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 策略即代码（Policy as Code） 是什么
- 如何 策略即代码（Policy as Code）
trigger_keywords:
- 策略即代码
- Policy
- as
- Code
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- policy-basics
---



# 策略即代码（Policy as Code）

## 概述

**策略即代码（Policy as Code）** 是将组织的安全、合规和运维策略以可版本化、可自动化验证的代码形式定义和执行的方法论。在 [[Kubernetes|Kubernetes]] 环境中，策略即代码通过准入控制器（Admission Controller）在资源创建或更新时进行实时校验和变异，确保集群状态始终符合组织策略。2026 年的主流实现包括 **Open Policy Agent（OPA/Gatekeeper）** 和 **[[Kyverno|Kyverno]]**。

## 核心概念/原理

### 1. 为什么需要策略即代码

手动审查和配置管理无法应对 Kubernetes 的规模和变更速度：
- 防止开发者使用 `latest` 镜像标签
- 强制所有 Pod 必须定义资源请求和限制
- 禁止特权容器（Privileged Containers）
- 确保所有 [[Ingress|Ingress]] 使用 TLS
- 强制指定标签或注解以支持成本分摊

策略即代码将这些要求自动化，实现"安全左移"。

### 2. OPA / Gatekeeper

**Open Policy Agent（OPA）** 是 CNCF 毕业项目，一个通用策略引擎，使用声明式语言 **Rego** 定义策略：
- **Gatekeeper**：专门为 Kubernetes 设计的 OPA 准入控制器
- **ConstraintTemplate**：定义可复用的策略模板（Rego 逻辑）
- **Constraint**：基于模板创建具体的策略约束实例

```yaml
# Gatekeeper Constraint 示例：禁止 Privileged Pod
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sPSPPrivilegedContainer
metadata:
  name: psp-privileged-container
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
```

### 3. Kyverno

**Kyverno** 是专为 Kubernetes 设计的策略引擎，与 OPA 不同：
- **原生 YAML 语法**：无需学习 Rego，策略直接用 Kubernetes YAML 编写
- **变异（Mutate）能力**：自动为资源添加标签、注解、Sidecar 或修改配置
- **生成（Generate）能力**：在创建 Namespace 时自动生成 NetworkPolicy、Quota 等配套资源
- **验证（Validate）能力**：阻止不符合策略的资源提交

```yaml
# Kyverno 策略示例：强制 Pod 必须指定资源限制
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-resources-limits
spec:
  validationFailureAction: Enforce
  rules:
  - name: check-resources
    match:
      resources:
        kinds:
        - Pod
    validate:
      message: "Pod must specify resource limits"
      pattern:
        spec:
          containers:
          - resources:
              limits:
                memory: "?*"
```

### 4. OPA vs Kyverno 选型

| 维度 | OPA / Gatekeeper | Kyverno |
|------|------------------|---------|
| 学习曲线 | 需学习 Rego | 使用原生 YAML |
| 生态系统 | 更通用，跨云/跨平台 | 专为 Kubernetes 优化 |
| 性能 | 高，适合超大规模 | 高，社区增长迅速 |
| 变异能力 | 有限 | 强大且易用 |
| 生成能力 | 有限 | 原生支持 |

2026 年的趋势是：**简单场景用 Kyverno，复杂跨平台策略用 OPA**。

## 关键机制或特性

### 准入控制点

策略引擎在 Kubernetes API Server 的 **MutatingAdmissionWebhook** 和 **ValidatingAdmissionWebhook** 阶段生效：
1. **Mutating**：在资源持久化前修改资源（如自动注入 Sidecar、补充标签）
2. **Validating**：验证资源是否符合策略，不符合则返回 403 拒绝

### 策略测试与 CI 集成

- **OPA**：使用 `opa test` 对 Rego 策略进行单元测试
- **Kyverno**：使用 `kyverno test` CLI 对策略进行测试
- **最佳实践**：将策略代码纳入 Git，通过 CI Pipeline 自动测试后再部署到集群

### 审计与报告

- Gatekeeper 的 **Audit** 功能定期扫描现有资源，报告违反策略的情况
- Kyverno 的 **Policy Reports** 提供集群级合规性视图

## 使用场景

1. **强制安全基线**：禁止 Privileged 容器、只读根文件系统、hostNetwork 等高风险配置
2. **资源治理**：强制所有工作负载设置 Resource Requests/Limits，防止资源浪费和无限制占用
3. **标签与成本分摊**：自动为 Namespace 注入 `cost-center` 标签，并强制所有 Pod 继承
4. **镜像来源控制**：仅允许来自内部 Harbor 或特定 Registry 的镜像部署到生产集群
5. **网络策略自动生成**：创建 Namespace 时自动生成默认的 Deny-All NetworkPolicy

## 最佳实践/注意事项

- **渐进式启用**：新策略先在 `audit` 模式下运行一段时间，评估影响后再切换到 `enforce`
- **策略分组管理**：按安全、合规、运维等维度组织策略，避免单一大而全的策略文件
- **异常白名单机制**：为特殊情况提供 Namespace 或 Pod 级别的豁免（Exemption）能力
- **避免策略冲突**：多个 Mutating Webhook 可能互相覆盖，需明确执行顺序和优先级
- **Webhook 高可用**：策略引擎问题会导致所有资源创建被拒绝，必须确保 OPA/Kyverno 多副本运行
- **定期审查策略**：随着 Kubernetes 版本升级和业务变化，定期审查策略是否仍然适用
- **测试覆盖率**：每个策略都应有对应的通过/失败测试用例，纳入 CI 自动验证

## 参考链接

- [Open Policy Agent Documentation](https://www.openpolicyagent.org/docs/latest/)
- [Gatekeeper Documentation](https://open-policy-agent.github.io/gatekeeper/website/docs/)
- [Kyverno Documentation](https://kyverno.io/docs/)
- [Ajeet Singh Raina - Top 5 Trends Shaping Kubernetes in 2026](https://www.ajeetraina.com/top-5-trends-shaping-kubernetes-in-2026/)

## Related

- [[domain-19-landscape-references/topic-index/security-index.md|Security 安全知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]
