---
title: OPA (Open Policy Agent)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- security
- opa
- crd
- operator
- rag
- agent
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OPA (Open Policy Agent) 是什么
- 如何 OPA (Open Policy Agent)
trigger_keywords:
- OPA
- Open
- Policy
- Agent
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# OPA (Open Policy Agent)

> **CNCF 状态**: Graduated | **类别**: Security | **主要语言**: Go

## 概述

title: Open Policy Agent (OPA)

## 核心能力

- 详见源文档获取完整信息 ^[inferred]

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- 建议参考官方文档获取最新部署指南 ^[inferred]

## 架构定位

在 CNCF 生态中，OPA 属于 **Security** 类别，为云原生应用提供统一的策略引擎能力。

## 安装与配置

```bash
# 安装 Gatekeeper（OPA 的 K8s 实现）
kubectl apply -f https://raw.githubusercontent.com/open-policy-agent/gatekeeper/master/deploy/gatekeeper.yaml

# 或使用 Helm
helm repo add gatekeeper https://open-policy-agent.github.io/gatekeeper/charts
helm install gatekeeper gatekeeper/gatekeeper -n gatekeeper-system --create-namespace

# 创建 ConstraintTemplate
kubectl apply -f - <<EOF
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8srequiredlabels
spec:
  crd:
    spec:
      names:
        kind: K8sRequiredLabels
      validation:
        openAPIV3Schema:
          type: object
          properties:
            labels:
              type: array
              items:
                type: string
  targets:
  - target: admission.k8s.gatekeeper.sh
    rego: |
      package k8srequiredlabels
      violation[{"msg": msg}] {
        provided := {label | input.review.object.metadata.labels[label]}
        required := {label | label := input.parameters.labels[_]}
        missing := required - provided
        count(missing) > 0
        msg := sprintf("Missing required labels: %v", [missing])
      }
EOF

# 创建 Constraint
kubectl apply -f - <<EOF
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sRequiredLabels
metadata:
  name: require-team-label
spec:
  match:
    kinds:
    - apiGroups: [""]
      kinds: ["Namespace"]
  parameters:
    labels: ["team", "environment"]
EOF
```

## 运维操作

```bash
# 🟢 查看策略状态
kubectl get constrainttemplates
kubectl get constraints
kubectl get k8srequiredlabels -o yaml

# 🟢 查看违规资源
kubectl get k8srequiredlabels require-team-label -o jsonpath='{.status.violations}'

# 🟢 测试 Rego 策略
opa test policy_test.rego policy.rego
opa eval -d policy.rego -i input.json 'data.k8srequiredlabels'

# 🟡 切换为强制执行模式
kubectl patch k8srequiredlabels require-team-label --type=merge -p '{"spec":{"enforcementAction":"deny"}}'

# 🟡 排除特定命名空间
kubectl patch k8srequiredlabels require-team-label --type=merge -p '{"spec":{"match":{"excludedNamespaces":["kube-system","gatekeeper-system"]}}}'

# 🔴 删除策略（停止执行）
kubectl delete constraint k8srequiredlabels require-team-label
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 策略未生效 | ConstraintTemplate 编译错误 | `kubectl get constrainttemplate -o yaml` | 检查 Rego 语法 |
| 合法请求被拒绝 | 策略过于严格 | `kubectl logs -n gatekeeper-system -l control-plane=controller-manager` | 调整为 dryrun 模式 |
| API Server 延迟增加 | Webhook 响应慢 | `kubectl get validatingwebhookconfigurations` | 增加超时/减少策略数 |
| 升级后策略丢失 | CRD 不兼容 | `kubectl get crd \| grep gatekeeper` | 重新应用 CRD |
| Audit 未运行 | Pod 资源不足 | `kubectl top pods -n gatekeeper-system` | 增加资源配额 |

```
排查流程:
├── 策略不生效
│   ├── kubectl get constrainttemplates → 编译状态
│   ├── kubectl get constraints → 执行状态
│   ├── opa eval → 本地测试 Rego
│   └── 检查 enforcementAction → deny/dryrun
├── 误拦截问题
│   ├── kubectl logs gatekeeper → 拒绝日志
│   ├── 检查 match 规则 → 作用范围
│   └── 切换为 dryrun → 观察违规
└── 性能问题
    ├── webhook 超时配置 → timeoutSeconds
    ├── 策略数量 → 合并相似策略
    └── 节点资源 → 扩容 Gatekeeper Pod
```

## 生产案例

### 案例1: 多租户标签强制策略

- **场景**: 200+ 团队共享集群，资源缺少 team 标签导致成本无法分摊
- **排查**: 60% 的 Namespace 缺少必要标签，成本报告不完整
- **方案**:
  1. 创建 K8sRequiredLabels ConstraintTemplate
  2. 先以 dryrun 模式运行 2 周，收集违规报告
  3. 通知团队整改后切换为 deny 模式
- **效果**: 标签合规率从 40% 提升至 100%，成本分摊完整

### 案例2: 禁止特权容器策略

- **场景**: 安全审计发现多个生产 Pod 以 privileged 模式运行
- **排查**: 开发团队为调试方便设置了 privileged: true
- **方案**:
  1. 创建 K8sPSPPrivilegedContainer Constraint
  2. 生产命名空间强制执行，开发命名空间仅告警
  3. 提供替代方案文档（securityContext capabilities）
- **效果**: 生产环境特权容器归零，安全审计通过

## 参考链接

- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/storage-model.md|storage-model]]
- [[pod-lifecycle]]
- [[概念/security-defense-depth.md|security-defense-depth]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[modelpack]] — ModelPack
- [[oauth2-proxy]] — OAuth2 Proxy
- [[schemahero]] — SchemaHero
- [[composefs]] — composefs
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- 14-policy-engines-opa-kyverno
- 09-opa-gatekeeper-policy
- 99-opa-gatekeeper-policy-guide
- copa
- opa
- RELEASE-NOTES-0.43
- RELEASE-NOTES-0.12
- RELEASE-NOTES-0.26
- RELEASE-NOTES-1.9
- RELEASE-NOTES-0.8
- RELEASE-NOTES-0.67
- RELEASE-NOTES-0.36
- RELEASE-NOTES-0.53
- RELEASE-NOTES-0.22
- RELEASE-NOTES-0.16
- RELEASE-NOTES-0.47
- RELEASE-NOTES-0.57
- RELEASE-NOTES-0.32
- RELEASE-NOTES-0.63
- RELEASE-NOTES-0.23
- RELEASE-NOTES-0.17
- RELEASE-NOTES-0.46
- RELEASE-NOTES-0.56
- RELEASE-NOTES-0.33
- RELEASE-NOTES-0.62
- RELEASE-NOTES-0.42
- RELEASE-NOTES-0.13
- RELEASE-NOTES-0.27
- RELEASE-NOTES-1.8
- RELEASE-NOTES-0.66
- RELEASE-NOTES-0.9
- RELEASE-NOTES-0.37
- RELEASE-NOTES-0.52
- RELEASE-NOTES-0.49
- RELEASE-NOTES-0.18
- RELEASE-NOTES-1.3
- RELEASE-NOTES-0.2
- RELEASE-NOTES-0.59
- RELEASE-NOTES-0.28
- RELEASE-NOTES-1.7
- RELEASE-NOTES-1.12
- RELEASE-NOTES-0.38
- RELEASE-NOTES-0.69
- RELEASE-NOTES-0.6
- RELEASE-NOTES-0.29
- RELEASE-NOTES-1.6
- RELEASE-NOTES-1.13
- RELEASE-NOTES-0.39
- RELEASE-NOTES-0.7
- RELEASE-NOTES-0.68
- RELEASE-NOTES-0.48
- RELEASE-NOTES-0.19
- RELEASE-NOTES-1.2
- RELEASE-NOTES-0.3
- RELEASE-NOTES-0.58
- RELEASE-NOTES-1.5
- RELEASE-NOTES-1.10
- RELEASE-NOTES-0.4
- RELEASE-NOTES-1.14
- RELEASE-NOTES-1.1
- RELEASE-NOTES-1.15
- RELEASE-NOTES-1.0
- RELEASE-NOTES-0.1
- RELEASE-NOTES-1.4
- RELEASE-NOTES-1.11
- RELEASE-NOTES-0.5
- RELEASE-NOTES-0.20
- RELEASE-NOTES-0.45
- RELEASE-NOTES-0.14
- RELEASE-NOTES-0.55
- RELEASE-NOTES-0.61
- RELEASE-NOTES-0.30
- RELEASE-NOTES-0.10
- RELEASE-NOTES-0.41
- RELEASE-NOTES-0.24
- RELEASE-NOTES-0.34
- RELEASE-NOTES-0.65
- RELEASE-NOTES-0.51
- RELEASE-NOTES-0.11
- RELEASE-NOTES-0.40
- RELEASE-NOTES-0.25
- RELEASE-NOTES-0.35
- RELEASE-NOTES-0.64
- RELEASE-NOTES-0.50
- RELEASE-NOTES-0.21
- RELEASE-NOTES-0.70
- RELEASE-NOTES-0.44
- RELEASE-NOTES-0.15
- RELEASE-NOTES-0.54
- RELEASE-NOTES-0.60
- RELEASE-NOTES-0.31
- [[实体/release-notes-security.md|发布说明索引 — 安全]] — Cross-reference
- [[概念/纵深防御 x 供应链安全.md|纵深防御 x 供应链安全]] — Cross-reference
- [[概念/控制器模式 × Operator 模式.md|控制器模式 × Operator 模式]] — Cross-reference
- [[概念/multi-tenancy-isolation.md|Multi-Tenancy Isolation]] — Cross-reference
- [[概念/security-tool-evolution.md|安全工具演进]] — Cross-reference
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/security-index.md|Security 安全知识图谱索引]]


<!-- risk-assessed -->
