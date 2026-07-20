---
title: "OPA/Gatekeeper × Kyverno × Policy-as-Code"
summary: "策略即代码三大方案对比：OPA/Gatekeeper 的 Rego 表达力、Kyverno 的 K8s 原生体验、ValidatingAdmissionPolicy 的零依赖未来"
category: synthesis
tags:
- opa
- gatekeeper
- kyverno
- policy-as-code
- admission-control
- validating-admission-policy
- security
tier: supporting
sources:
- 实体/opa.md
- 实体/kyverno.md
- 概念/k8s-security-compliance.md
- 概念/gitops-principles.md
created: '2026-07-19'
updated: '2026-07-19'
last_updated: '2026-07-19'
---

# OPA/Gatekeeper × Kyverno × Policy-as-Code

## The Connection（为什么这两个领域交叉）

Kubernetes 的 Admission Control 是集群安全的最后一道防线——在资源被持久化到 etcd 之前进行校验和变更。策略即代码（Policy-as-Code）将安全合规要求从"文档中的规范"转化为"可执行、可测试、可版本化的代码"，通过 Admission Webhook 在运行时强制执行。

OPA（Open Policy Agent）/Gatekeeper 和 Kyverno 是当前 K8s 策略引擎的两大主流方案，而 Kubernetes 1.30+ 引入的 ValidatingAdmissionPolicy（VAP）将策略执行内置到 API Server，无需外部 Webhook。三者代表了策略即代码的演进路径：从通用策略引擎（OPA）→ K8s 原生策略引擎（Kyverno）→ API Server 内置策略（VAP）。

交叉的核心价值在于：策略即代码将 DevOps 的"一切皆代码"理念延伸到安全治理领域。安全团队不再依赖运维人员"记得检查"，而是通过自动化策略确保每一个进入集群的资源都符合组织规范——镜像来源、资源限制、标签规范、网络策略、RBAC 权限，全部可编码、可审计、可追溯。

## Where They Co-occur（生产中的交叉场景）

### 场景一：镜像准入控制

生产集群要求所有容器镜像必须来自受信任的 Registry（如 `harbor.internal.com`），且必须经过漏洞扫描。OPA/Gatekeeper 通过 `ConstraintTemplate` + `Constraint` 实现：Rego 规则检查 `spec.containers[*].image` 前缀。Kyverno 通过 `ClusterPolicy` 的 `validate` 规则实现相同逻辑，语法更接近 K8s YAML。两者都支持 `enforce`（拒绝）和 `audit`（仅记录）模式。

### 场景二：资源配额与限制强制执行

防止开发者创建无资源限制的 Pod（导致节点资源争抢）。策略要求所有容器必须设置 `resources.requests` 和 `resources.limits`，且 limits 不超过节点可分配资源的 50%。OPA 用 Rego 表达数值比较，Kyverno 用 `validate.pattern` 或 `validate.cel` 表达。

### 场景三：标签与注解治理

组织要求所有生产资源必须携带 `team`、`environment`、`cost-center` 标签。策略在 Admission 时校验标签存在性和格式（如 `environment` 只能是 `staging|production`）。Kyverno 的 `mutate` 规则还能自动注入缺失标签（如从 namespace 标签继承）。

### 场景四：CI/CD 流水线中的策略测试

策略变更本身需要测试。OPA 有 `opa test` 框架（Rego 单元测试），Kyverno 有 `kyverno test` 命令（YAML 测试用例）。在 CI 流水线中：策略文件变更 → 运行策略单元测试 → 在 staging 集群 dry-run → 合并到 production GitOps 仓库 → ArgoCD 同步。

### 场景五：多集群策略一致性

多集群环境中，安全策略必须跨集群一致。策略文件存放在 GitOps 仓库中，通过 ArgoCD ApplicationSet 或 Kustomize 分发到所有集群。OPA/Gatekeeper 的 Constraint 和 Kyverno 的 ClusterPolicy 都是 CRD，天然适合 GitOps 管理。

### 场景六：ValidatingAdmissionPolicy 渐进采用

K8s 1.30+ 的 VAP 使用 CEL（Common Expression Language）表达策略，无需部署额外 Webhook。适合简单校验规则（如标签检查、镜像前缀），复杂规则（如跨资源查询、外部数据源）仍需 OPA/Kyverno。生产策略：简单规则用 VAP（零运维），复杂规则用 Kyverno/OPA。

## Production Patterns（生产模式与架构）

### 模式一：OPA/Gatekeeper 分层策略架构

```
┌─────────────────────────────────────────────────────┐
│  OPA/Gatekeeper Policy Architecture                 │
├─────────────────────────────────────────────────────┤
│                                                     │
│  GitOps Repo (策略源码)                             │
│    ├── constraint-templates/                        │
│    │   ├── k8sallowedrepos.yaml                    │
│    │   ├── k8srequiredlabels.yaml                  │
│    │   └── k8sresourcelimits.yaml                  │
│    ├── constraints/                                 │
│    │   ├── production/                             │
│    │   │   ├── require-team-label.yaml            │
│    │   │   └── trusted-registry.yaml              │
│    │   └── staging/                                │
│    │       └── trusted-registry-relaxed.yaml       │
│    └── tests/                                       │
│        ├── allowedrepos_test.rego                  │
│        └── requiredlabels_test.rego                │
│                                                     │
│  Cluster (运行时)                                   │
│    ├── Gatekeeper Controller (admission webhook)   │
│    ├── Gatekeeper Audit (定期扫描存量资源)          │
│    └── Constraint Violations (可观测)              │
│                                                     │
└─────────────────────────────────────────────────────┘
```

### 模式二：Kyverno 策略即 YAML

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-trusted-registry
spec:
  validationFailureAction: Enforce
  background: true
  rules:
  - name: validate-image-registry
    match:
      any:
      - resources:
          kinds:
          - Pod
    exclude:
      any:
      - resources:
          namespaces:
          - kube-system
          - gatekeeper-system
    validate:
      message: "镜像必须来自受信任的 Registry (harbor.internal.com)"
      pattern:
        spec:
          containers:
          - image: "harbor.internal.com/*"
```

### 模式三：ValidatingAdmissionPolicy (K8s 1.30+)

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicy
metadata:
  name: require-resource-limits
spec:
  failurePolicy: Fail
  matchConstraints:
    resourceRules:
    - apiGroups: [""]
      apiVersions: ["v1"]
      operations: ["CREATE", "UPDATE"]
      resources: ["pods"]
  validations:
  - expression: |
      object.spec.containers.all(c,
        c.resources.?limits.?cpu.hasValue() &&
        c.resources.?limits.?memory.hasValue()
      )
    message: "所有容器必须设置 CPU 和内存 limits"
---
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingAdmissionPolicyBinding
metadata:
  name: require-resource-limits-binding
spec:
  policyName: require-resource-limits
  validationActions: [Deny]
  matchResources:
    namespaceSelector:
      matchLabels:
        environment: production
```

### 模式四：策略测试 CI 流水线

```
策略变更 PR → CI Pipeline:
  1. 语法校验 (opa check / kyverno validate)
  2. 单元测试 (opa test / kyverno test)
  3. 集成测试 (kind 集群 + 策略部署 + 测试用例)
  4. Dry-run 报告 (audit 模式扫描存量资源)
  5. 人工审批 (安全团队 review)
  6. 合并 → GitOps 同步到生产集群
```

### 模式五：策略与可观测性集成

策略违规不仅是"拒绝"，更是可观测信号。Gatekeeper 的 `constraint_violations` 指标、Kyverno 的 `kyverno_policy_results_total` 指标接入 Prometheus，Grafana 面板展示：违规趋势、Top 违规 namespace、Top 违规策略。高频违规意味着开发者体验问题——策略太严或文档不足。

## Trade-offs & Decision Matrix（权衡与决策）

| 维度 | OPA/Gatekeeper | Kyverno | ValidatingAdmissionPolicy |
|------|---------------|---------|--------------------------|
| 策略语言 | Rego（专用语言） | YAML + CEL（K8s 风格） | CEL（内置） |
| 学习曲线 | 陡峭（Rego 语法独特） | 平缓（K8s 开发者友好） | 中等（CEL 需学习） |
| 表达能力 | 极强（图灵完备） | 强（支持 API 查询） | 中（仅当前对象） |
| 部署依赖 | Gatekeeper + OPA | Kyverno Controller | 无（API Server 内置） |
| 性能影响 | Webhook 延迟 +2-5ms | Webhook 延迟 +1-3ms | 无额外网络跳转 |
| Mutate 支持 | 有限（需额外配置） | 原生支持 | 不支持（仅 Validate） |
| 审计模式 | 支持（定期扫描） | 支持（background scan） | 不支持 |
| 外部数据 | 支持（HTTP 数据源） | 支持（API 查询） | 不支持 |
| 测试框架 | `opa test`（成熟） | `kyverno test`（成熟） | 无专用框架 |
| 社区生态 | CNCF 毕业，企业广泛 | CNCF 孵化，增长快 | K8s 原生，未来趋势 |
| 多集群分发 | GitOps 友好 | GitOps 友好 | GitOps 友好 |
| 高可用 | 需多副本 + PDB | 需多副本 + PDB | API Server 原生 HA |
| 故障影响 | Webhook 不可用 → 请求阻塞/放行 | Webhook 不可用 → 请求阻塞/放行 | 无单点故障 |

### 决策矩阵

- **团队无专职安全工程师，K8s 经验为主** → Kyverno（YAML 友好，上手快）
- **需要复杂策略逻辑（图查询、外部数据）** → OPA/Gatekeeper（Rego 表达力）
- **K8s 1.30+ 且策略简单** → ValidatingAdmissionPolicy（零运维）
- **需要 Mutate（自动注入标签/sidecar）** → Kyverno（原生 mutate）
- **已有 OPA 用于非 K8s 场景（API 网关、CI）** → OPA/Gatekeeper（统一策略栈）
- **混合策略** → VAP 做基础校验 + Kyverno 做复杂策略（推荐演进路径）

## Anti-patterns & Pitfalls（反模式）

### 反模式一：策略全部 Enforce 无过渡期

新策略直接设为 `Enforce`/`Deny`，导致大量存量工作负载更新失败、CI/CD 流水线中断。**正确做法**：先 `Audit`/`Warn` 模式运行 1-2 周，观察违规数量和影响范围，修复存量问题后再切换为 `Enforce`。

### 反模式二：Webhook failurePolicy 设为 Ignore

为避免策略引擎故障阻塞部署，将 `failurePolicy` 设为 `Ignore`。结果：策略引擎宕机时所有策略失效，等同于无策略。**正确做法**：生产环境用 `Fail`（安全优先），同时确保策略引擎高可用（3 副本 + PDB + 健康检查）。对非关键 namespace 可用 `Ignore`。

### 反模式三：策略无版本控制和测试

策略直接在集群中 `kubectl apply`，无 Git 版本控制、无测试、无 review。策略变更导致误拦截时无法快速回滚。**正确做法**：策略文件入 Git，变更走 PR + CI 测试 + 审批流程，与 GitOps 集成。

### 反模式四：Rego 策略过于复杂

单个 ConstraintTemplate 中 Rego 规则超过 100 行，嵌套多层 `not`/`some`/`every`，难以维护和调试。**正确做法**：拆分为多个小策略，每个策略只做一件事；使用 `opa test` 覆盖边界情况；复杂逻辑用 Kyverno 的 API 查询或外部数据源简化。

### 反模式五：忽略策略冲突

OPA 和 Kyverno 同时部署，对同一资源执行不同策略，产生冲突（一个允许一个拒绝）。开发者困惑于"为什么我的 Pod 被拒绝了"。**正确做法**：明确策略引擎职责划分（如 OPA 管安全策略，Kyverno 管治理策略），避免重叠；或统一为单一引擎。

### 反模式六：策略排除过多 namespace

`kube-system`、`monitoring`、`logging` 等系统 namespace 全部排除策略检查。攻击者利用这些 namespace 的宽松策略部署恶意工作负载。**正确做法**：系统 namespace 也应有策略，只是规则不同（如允许特权容器但限制镜像来源）。

## Operational Checklist（运维检查清单）

### 部署前

- [ ] 评估策略引擎资源需求：Gatekeeper 每副本 256MB-512MB，Kyverno 每副本 128MB-256MB
- [ ] 配置高可用：≥3 副本 + PodDisruptionBudget + 反亲和
- [ ] 设置 `failurePolicy: Fail`（生产）或 `Ignore`（非关键环境）
- [ ] 配置 Webhook 超时：建议 10s（默认 30s 过长）
- [ ] 排除策略引擎自身 namespace（避免自锁）
- [ ] 准备紧急绕过方案：`kubectl delete validatingwebhookconfiguration`（紧急时使用）

### 策略开发

- [ ] 每个策略必须有对应的测试用例（`opa test` / `kyverno test`）
- [ ] 新策略先 Audit 模式运行 ≥ 7 天
- [ ] 策略 message 必须清晰说明违规原因和修复方法
- [ ] 策略变更走 PR + 安全团队 review
- [ ] 使用 `match`/`exclude` 精确限定策略作用范围

### 运行监控

- [ ] Prometheus 指标：策略违规计数、Webhook 延迟、策略引擎健康
- [ ] Grafana 面板：违规趋势、Top 违规者、策略覆盖率
- [ ] 告警：策略引擎不可用 > 1 分钟、违规突增 > 基线 3 倍
- [ ] 定期审计：每月检查 Audit 模式下的存量违规

### 故障排查

- [ ] Pod 创建被拒绝 → `kubectl get events` 查看拒绝原因
- [ ] 策略引擎延迟高 → 检查 Webhook 配置、策略复杂度、副本数
- [ ] 策略不生效 → 检查 `match`/`exclude` 条件、namespace 标签
- [ ] 误拦截 → 临时添加 `exclude` 规则 → 修复策略 → 移除 exclude

## Related

- [[实体/opa.md|OPA]]
- [[实体/kyverno.md|Kyverno]]
- [[概念/k8s-security-compliance.md|K8s 安全合规]]
- [[概念/gitops-principles.md|GitOps 原则]]
- [[综合/argocd-gitops.md|ArgoCD × GitOps]]
- [[综合/service-mesh-mtls-zero-trust.md|Service Mesh × mTLS × Zero Trust]]
- [[综合/compliance-k8s-soc2-hipaa.md|合规 × K8s × SOC2/HIPAA]]
- [[综合/container-registry-image-scanning.md|容器镜像仓库 × 镜像扫描]]
