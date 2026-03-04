# OSCAL Compass

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://oscal-compass.github.io/ |
| **GitHub** | https://github.com/oscal-compass/compliance-trestle |
| **许可证** | Apache-2.0 |
| **开发语言** | Python |
| **CNCF 状态** | Sandbox |

---

## 项目概述

OSCAL Compass 是一套基于 NIST OSCAL (Open Security Controls Assessment Language) 标准的合规自动化工具集。它包括 Trestle (合规即代码框架)、C2P (合规到策略转换) 等组件，帮助组织将安全合规要求转换为可执行的代码和策略，实现从合规框架（如 FedRAMP、SOC 2、ISO 27001）到实际控制实施的自动化映射和验证。

### 核心特性

- **OSCAL 原生**: 基于 NIST OSCAL 标准，实现合规数据的结构化表达
- **合规即代码**: 将合规控制定义为代码，纳入 GitOps 工作流
- **策略生成**: 自动将合规要求转换为 OPA、Kyverno 等策略引擎的规则
- **证据收集**: 自动化收集合规证据，支持持续合规监控
- **多框架支持**: 支持 FedRAMP、NIST 800-53、SOC 2、ISO 27001 等
- **集成 Kubernetes**: 与 Kubernetes 准入控制和策略引擎深度集成

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│              OSCAL Compass Ecosystem                  │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │         Compliance Frameworks                  │    │
│  │  ┌──────────┐ ┌──────────┐ ┌──────────────┐  │    │
│  │  │FedRAMP   │ │NIST      │ │SOC 2/        │  │    │
│  │  │          │ │800-53    │ │ISO 27001     │  │    │
│  │  └─────┬────┘ └─────┬────┘ └──────┬───────┘  │    │
│  └────────┼────────────┼─────────────┼──────────┘    │
│           │            │             │                │
│  ┌────────▼────────────▼─────────────▼──────────┐    │
│  │         OSCAL Catalog/Profile/SSP             │    │
│  │         (结构化合规数据)                       │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │                              │
│  ┌─────────────────────▼────────────────────────┐    │
│  │              Trestle (合规即代码)              │    │
│  │  ┌──────────────┐  ┌───────────────────────┐ │    │
│  │  │ OSCAL        │  │ Authoring Tools       │ │    │
│  │  │ Validation   │  │ (SSP 编写)            │ │    │
│  │  └──────────────┘  └───────────────────────┘ │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │                              │
│  ┌─────────────────────▼────────────────────────┐    │
│  │              C2P (Compliance to Policy)       │    │
│  │  OSCAL Controls → OPA/Kyverno/Gatekeeper     │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │                              │
│  ┌─────────────────────▼────────────────────────┐    │
│  │           Policy Enforcement                   │    │
│  │  ┌──────┐  ┌──────────┐  ┌────────────────┐  │    │
│  │  │ OPA  │  │ Kyverno  │  │ Gatekeeper     │  │    │
│  │  └──────┘  └──────────┘  └────────────────┘  │    │
│  └──────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 Trestle

```bash
# 使用 pip 安装
pip install compliance-trestle

# 验证安装
trestle version
```

### 初始化 Trestle 工作空间

```bash
# 创建工作空间
mkdir compliance-workspace && cd compliance-workspace
trestle init

# 目录结构
# compliance-workspace/
# ├── catalogs/           # OSCAL 控制目录
# ├── profiles/           # 合规配置文件
# ├── component-definitions/  # 组件定义
# ├── system-security-plans/  # 系统安全计划
# └── assessment-results/     # 评估结果
```

### 导入 NIST 控制目录

```bash
# 导入 NIST 800-53 目录
trestle import \
  --file https://raw.githubusercontent.com/usnistgov/oscal-content/main/nist.gov/SP800-53/rev5/json/NIST_SP-800-53_rev5_catalog.json \
  --output catalogs/nist-800-53-rev5

# 查看导入的目录
trestle catalog list
```

### 创建合规 Profile

```yaml
# profiles/my-profile/profile.yaml
profile:
  uuid: "550e8400-e29b-41d4-a716-446655440000"
  metadata:
    title: "My Organization Security Profile"
    version: "1.0.0"
  imports:
    - href: "../catalogs/nist-800-53-rev5/catalog.json"
      include-controls:
        - with-ids:
            - ac-1   # Access Control Policy
            - ac-2   # Account Management
            - ac-3   # Access Enforcement
            - au-1   # Audit Policy
            - au-2   # Audit Events
            - cm-1   # Configuration Management Policy
  modify:
    set-parameters:
      - param-id: ac-1_prm_1
        values:
          - "annually"
```

### 使用 C2P 生成策略

```bash
# 安装 C2P
pip install compliance-to-policy

# 从 OSCAL 生成 Kyverno 策略
c2p generate \
  --profile profiles/my-profile/profile.json \
  --output-format kyverno \
  --output policies/kyverno/

# 生成的策略示例
# policies/kyverno/
# ├── ac-2-account-management.yaml
# ├── au-2-audit-events.yaml
# └── cm-2-baseline-configuration.yaml
```

---

## 高级功能

### 编写 SSP (System Security Plan)

```yaml
# system-security-plans/my-system/ssp.yaml
system-security-plan:
  uuid: "550e8400-e29b-41d4-a716-446655440001"
  metadata:
    title: "My Cloud System Security Plan"
    version: "1.0.0"
  
  import-profile:
    href: "../../profiles/my-profile/profile.json"
  
  system-characteristics:
    system-name: "My Cloud System"
    description: "Production Kubernetes cluster"
    security-sensitivity-level: moderate
    authorization-boundary:
      description: "Kubernetes cluster in AWS VPC"
  
  system-implementation:
    components:
      - uuid: "comp-001"
        title: "Kubernetes Cluster"
        type: software
        status:
          state: operational
        props:
          - name: "vendor"
            value: "AWS EKS"
  
  control-implementation:
    implemented-requirements:
      - uuid: "impl-001"
        control-id: ac-2
        statements:
          - statement-id: ac-2_smt.a
            uuid: "stmt-001"
            description: |
              User accounts are managed through AWS IAM and 
              Kubernetes RBAC with automated provisioning.
            by-components:
              - component-uuid: "comp-001"
                description: "Kubernetes RBAC enforcement"
```

### 持续合规监控

```bash
# 收集评估证据
trestle assess \
  --ssp system-security-plans/my-system/ssp.json \
  --output assessment-results/2024-03/

# 生成合规报告
trestle report \
  --assessment assessment-results/2024-03/assessment-results.json \
  --format markdown \
  --output reports/2024-03-compliance-report.md
```

### 与 GitOps 集成

```yaml
# .github/workflows/compliance.yaml
name: Compliance Check
on: [push, pull_request]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Install Trestle
        run: pip install compliance-trestle
      
      - name: Validate OSCAL
        run: trestle validate -a
      
      - name: Generate Policies
        run: |
          c2p generate \
            --profile profiles/my-profile/profile.json \
            --output-format kyverno \
            --output policies/
      
      - name: Apply Policies
        run: kubectl apply -f policies/
```

---

## 与其他方案对比

| 特性 | OSCAL Compass | Regula | Checkov | Prowler |
|:---|:---|:---|:---|:---|
| 标准 | NIST OSCAL | 自定义 | 自定义 | AWS/CIS |
| 合规框架 | 多框架 | 自定义 | 多框架 | 云厂商 |
| 策略生成 | 自动 | 手动 | 内置 | 内置 |
| SSP 支持 | 完整 | 无 | 无 | 无 |
| 证据收集 | 自动 | 无 | 有限 | 有限 |
| K8s 集成 | OPA/Kyverno | Rego | 有限 | 有限 |

---

## 最佳实践

1. **合规即代码**: 将所有合规文档纳入 Git 版本控制
2. **自动化验证**: 在 CI/CD 中自动验证 OSCAL 文档和生成的策略
3. **持续监控**: 定期运行评估，持续收集合规证据
4. **模块化设计**: 将通用控制封装为可复用的组件定义
5. **审计追踪**: 使用 OSCAL Assessment Results 记录所有合规评估

---

## 参考资源

- [OSCAL Compass 文档](https://oscal-compass.github.io/)
- [Trestle GitHub](https://github.com/oscal-compass/compliance-trestle)
- [NIST OSCAL 规范](https://pages.nist.gov/OSCAL/)
- [C2P (Compliance to Policy)](https://github.com/oscal-compass/compliance-to-policy)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
