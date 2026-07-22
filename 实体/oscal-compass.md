---
title: OSCAL Compass (entities)
description: '## 概述'
summary: 'OSCAL Compass 是一套基于 NIST OSCAL (Open Security Controls Assessment Language) 标准的合规自动化工具集。'
category: entities
tags:
- k8s
- cncf
- security
- oscal-compass
- crd
- operator
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- OSCAL Compass 是什么
- 如何 OSCAL Compass
trigger_keywords:
- OSCAL
- Compass
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# OSCAL Compass

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Python

## 概述

OSCAL Compass 是一套基于 NIST OSCAL (Open Security Controls Assessment Language) 标准的合规自动化工具集，由 Red Hat 推动开发，2024 年加入 CNCF 沙箱。它包括 Trestle（合规即代码框架）、C2P（Compliance to Policy，合规到策略转换）等组件，帮助组织将安全合规要求转换为可执行的代码和策略。OSCAL Compass 实现了从合规框架（如 FedRAMP、SOC 2、ISO 27001、NIST 800-53）到实际控制实施（如 Kyverno 策略、OPA 规则）的自动化转换闭环，让合规审计从手工文档操作升级为可编程、可验证的自动化流程。

## 核心能力

- **OSCAL 标准**: 完全兼容 NIST OSCAL JSON/XML 格式（Catalog、Profile、Component、SSP、AP、AR）
- **合规即代码**: 将合规文档（如 FedRAMP SSP）转化为 Git 管理的代码资产
- **C2P 转换**: 自动将 OSCAL 合规控制映射到 Kubernetes 策略（Kyverno、OPA）
- **Trestle 框架**: Python 工具链，提供 OSCAL 文档的创建、编辑、验证和转换能力
- **持续监控**: 通过 OSCAL Assessment Results 持续收集和记录合规证据
- **多框架支持**: FedRAMP、SOC 2、ISO 27001、NIST 800-53、CIS Benchmark

## 架构

OSCAL Compass 围绕 NIST OSCAL 数据模型构建：

- **OSCAL Catalog**: 安全控制目录（如 NIST 800-53 的 1000+ 控制项定义）
- **OSCAL Profile**: 从 Catalog 中选取适用的控制子集（基线）
- **OSCAL Component Definition**: 组件的控制实施声明（如 Kyverno 策略如何满足某控制）
- **OSCAL SSP**: System Security Plan，描述系统如何满足合规要求
- **Trestle CLI**: 操作 OSCAL 文档的命令行工具，支持 assemble/validate/split
- **C2P Engine**: 将 Component Definition 中的实施声明转换为 Kubernetes 策略 CRD

合规流程：`OSCAL Catalog → Profile → Component Definition → C2P → Kyverno/OPA 策略 → 集群执行`

## K8s 集成

OSCAL Compass 的 C2P 组件将 OSCAL 合规定义转换为 Kubernetes 策略资源。例如，将 NIST 800-53 的 "AC-2 账户管理" 控制映射为 Kyverno ClusterPolicy，要求所有 Pod 必须设置特定的 SecurityContext。C2P 以 Operator 或 CLI 方式运行，读取 OSCAL 格式的合规定义，生成 Kyverno 或 OPA 策略 CRD 并应用到集群。结合 OSCAL Assessment Results，可以持续验证策略执行状态并生成合规报告。与 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中的准入控制（Admission Webhook）机制配合，实现策略的强制执行。

## 生产场景

1. **FedRAMP 合规**: 为政府云服务自动化生成和验证 FedRAMP SSP 文档
2. **多框架合规**: 同时满足 NIST 800-53、SOC 2、ISO 27001 要求，避免重复工作
3. **策略即代码**: 将安全控制自动转化为 Kyverno/OPA 策略，在集群中强制执行
4. **持续合规审计**: 定期运行评估，生成 OSCAL Assessment Results 供审计使用

## 安装与配置

```bash
# 安装 Trestle CLI
pip install compliance-trestle

# 初始化 Trestle 项目
trestle init -v

# 导入 NIST 800-53 Catalog
trestle import -f nist_800-53_catalog.json -o nist80053

# 创建合规 Profile
trestle profile create -n fedramp-low -c nist80053

# 安装 C2P (Compliance to Policy)
pip install c2p
# 将 OSCAL 定义转换为 Kyverno 策略
c2p convert --input component-definition.json --engine kyverno --output policies/

# 验证 OSCAL 文档
trestle validate -f dist/catalogs/nist80053/catalog.json
```

```yaml
# OSCAL Component Definition 示例（映射 Kyverno 策略）
component-definition:
  uuid: "comp-def-001"
  metadata:
    title: "Kubernetes Security Controls"
    version: "1.0.0"
  components:
  - uuid: "kyverno-policies"
    type: service
    title: "Kyverno Policy Engine"
    control-implementations:
    - uuid: "ci-001"
      source: "https://raw.githubusercontent.com/usnistgov/oscal-content/main/nist.gov/SP800-53/rev5/json/NIST_SP-800-53_rev5_catalog.json"
      implemented-requirements:
      - uuid: "ir-001"
        control-id: "sc-7"  # 边界保护
        description: "通过 NetworkPolicy 限制 Pod 网络边界"
        props:
        - name: kyverno-policy
          value: restrict-egress-traffic
      - uuid: "ir-002"
        control-id: "ac-6"  # 最小权限
        description: "禁止容器以 root 运行"
        props:
        - name: kyverno-policy
          value: disallow-privileged-containers
```

```yaml
# C2P 生成的 Kyverno 策略示例
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-privileged-containers
  annotations:
    oscal-compass/control-id: "ac-6"
    oscal-compass/component: "kyverno-policies"
spec:
  validationFailureAction: Enforce
  rules:
  - name: deny-privileged
    match:
      resources:
        kinds: ["Pod"]
    validate:
      message: "容器不允许以特权模式运行 (OSCAL AC-6)"
      pattern:
        spec:
          containers:
          - securityContext:
              privileged: false
```

## 运维操作

```bash
# 🟢 低风险：验证 OSCAL 文档
trestle validate -f dist/catalogs/nist80053/catalog.json
trestle validate -f component-definition.json

# 🟢 低风险：查看合规状态
kubectl get clusterpolicy -l oscal-compass/control-id
c2p assess --input component-definition.json --results assessment-results.json

# 🟡 中风险：生成并应用策略
c2p convert --input component-definition.json --engine kyverno --output policies/
kubectl apply -f policies/

# 🟡 中风险：更新合规 Profile
trestle profile assemble -n fedramp-low
trestle href -n fedramp-low -hr '#'

# 🔴 高风险：删除合规策略（失去合规保护）
kubectl delete clusterpolicy -l oscal-compass/component=kyverno-policies
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| trestle validate 失败 | OSCAL JSON 格式错误 | `trestle validate -f <file> -v` | 修复 JSON schema 违规 |
| C2P 转换失败 | control-id 不匹配 | `c2p convert --input <file> --verbose` | 检查 Component Definition 中的 control-id |
| 策略未强制执行 | validationFailureAction=Audit | `kubectl get clusterpolicy <name> -o yaml` | 修改为 Enforce 模式 |
| 合规报告不完整 | 部分控制未映射 | `c2p assess --verbose` | 补充缺失的 implemented-requirements |
| Kyverno 策略冲突 | 多个策略规则矛盾 | `kubectl get clusterpolicy -o wide` | 审查策略优先级和排除规则 |

```
排查流程：
├── OSCAL 文档验证失败？
│   ├── trestle validate -v → 查看详细错误
│   ├── 检查 JSON schema 合规性
│   └── 确认 OSCAL 版本兼容性
├── 策略生成失败？
│   ├── 检查 Component Definition 格式
│   ├── 确认 control-id 与 Catalog 匹配
│   └── 查看 C2P 转换日志
└── 合规状态异常？
    ├── kubectl get clusterpolicy → 检查策略状态
    ├── c2p assess → 重新评估
    └── 对比 Assessment Results 与预期
```

## 生产案例

### 案例 1：FedRAMP 合规自动化

- **场景**：政府云服务商需要在 3 个月内完成 FedRAMP Low 合规认证
- **排查**：手工编写 SSP 文档需要 6 个月，且难以保持持续合规
- **方案**：使用 Trestle 管理 OSCAL SSP，C2P 自动生成 320+ Kyverno 策略，持续评估生成 Assessment Results
- **效果**：合规准备时间从 6 个月缩短至 6 周，审计发现减少 90%

### 案例 2：多框架合规统一管理

- **场景**：金融公司同时需要满足 SOC 2、ISO 27001、NIST 800-53 三套合规要求
- **排查**：三套合规文档独立维护，存在大量重复工作和不一致
- **方案**：建立统一 OSCAL Catalog，通过不同 Profile 派生各框架要求，C2P 统一生成策略
- **效果**：合规维护工作量减少 60%，消除框架间不一致问题

## 对比

| 特性 | OSCAL Compass | Compliance-as-Code | Chef InSpec | OpenSCAP |
|------|--------------|-------------------|-------------|----------|
| OSCAL 标准 | ✅ 原生 | ❌ | ❌ | ⚠️ 部分 |
| K8s 策略生成 | ✅ Kyverno/OPA | ⚠️ 需手动 | ❌ | ❌ |
| 合规框架 | 多框架 | 单一 | 多框架 | 多框架 |
| 持续监控 | ✅ | ⚠️ | ✅ | ✅ |

## 架构定位

在 CNCF 生态中，OSCAL Compass 属于 **Security** 类别，为云原生应用提供合规自动化和策略转换能力。

## 参考链接

- [[kyverno]]
- [[概念/gitops-principles.md|gitops-principles]]
- [[概念/security-defense-depth.md|security-defense-depth]]
- [[概念/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[urunc]] — urunc
- [[connect-rpc]] — Connect RPC
- [[antrea]] — Antrea
- [[linkerd]] — Linkerd
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- oscal-compass
- [[实体/openfga.md|[[OpenFGA|OpenFGA]]]]
- [[实体/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
