---
title: in-toto (entities)
description: '## 概述'
summary: '该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]'
category: entities
tags:
- k8s
- cncf
- supply-chain
- in-toto
- crd
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- in-toto 是什么
- 如何 in-toto
trigger_keywords:
- in-toto
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# in-toto

> **CNCF 状态**: Graduated | **类别**: Supply Chain | **主要语言**: Python, Go

## 概述

in-toto 是一个 CNCF 毕业项目，由 NYU 安全研究团队开发，是软件供应链安全验证框架。它定义了一种标准化的方式来验证软件从源代码到发布制品的全链路完整性。in-toto 通过元数据（Layout）定义供应链中的每个步骤及其执行者，并通过链接元数据（Link）记录每一步的实际执行情况。只有当所有步骤都按预期执行且未被篡改时，制品才被认为是安全的。项目被 Datadog、Google、Apache 等采用。

## Key Features（核心能力）

- **Layout 规范**：通过 Layout 文件定义供应链中的每个步骤、执行者和预期产物
- **Link 元数据**：每个步骤生成 Link 文件记录执行的命令、产物材料（Materials）和产物（Products）
- **签名验证**：所有元数据通过 GPG 密钥签名，验证身份和完整性
- **步骤隔离**：每个步骤的执行者密钥独立，实现职责分离
- **子 Layout**：支持 Layout 嵌套，实现供应链层级化验证
- **与 SLSA 兼容**：为 SLSA 框架提供具体的实现方式

## 架构与工作原理

in-toto 工作流分为三个阶段：Layout 定义阶段——项目所有者定义供应链 Layout（步骤序列、执行者公钥、预期命令和产物）；执行阶段——每个步骤的执行者运行 in-toto-run 生成 Link 元数据（记录材料哈希、命令、产物哈希）；验证阶段——in-toto-verify 收集 Layout 和所有 Link 文件，验证每个步骤是否按预期执行，产物是否未被篡改。

## K8s 集成

in-toto 在 Kubernetes 供应链安全中与 Sigstore/Cosign 配合使用。CI/CD 流水线中，每个构建步骤生成 in-toto Link 文件（attestation），Cosign 将这些 attestation 附加到容器镜像。部署时，K8s Admission Controller（如 Policy Controller）验证镜像上的 in-toto attestation，确保构建过程符合预期 Layout。

## 生产用例

- **软件供应链验证**：验证从代码到发布制品的全链路完整性
- **SLSA 合规**：满足 SLSA Level 2-4 的构建来源验证要求
- **安全审计**：为每次构建提供可追溯的审计链
- **防篡改保护**：防止构建过程中的恶意代码注入

## 安装与配置

```bash
# 🟢 安装 in-toto CLI
pip3 install in-toto

# 🟢 验证安装
in-toto-verify --help

# 🟢 生成密钥对
in-toto-keygen owner
in-toto-keygen build-step
in-toto-keygen package-step

# 🟢 定义 Layout (供应链布局)
in-toto-layout \
  --layout-name "myapp-supply-chain" \
  --steps build-step package-step \
  --keys build-step.pub package-step.pub \
  --signing-key owner

# 🟢 执行步骤并生成 Link
in-toto-run \
  --step-name build \
  --key build-step \
  --materials src/ \
  --products dist/ \
  --command make build

in-toto-run \
  --step-name package \
  --key package-step \
  --materials dist/ \
  --products release/myapp.tar.gz \
  --command make package

# 🟢 验证供应链
in-toto-verify \
  --layout root.layout \
  --verification-keys owner.pub \
  --link-dir ./links/
```

### Layout 文件示例

```json
{
  "_type": "layout",
  "expires": "2027-01-01T00:00:00Z",
  "steps": [
    {
      "name": "build",
      "pubkeys": ["build-step-public-key"],
      "expected_command": ["make", "build"],
      "expected_materials": [["MATCH", "src/*", "IN", "src/"]],
      "expected_products": [["CREATE", "dist/*"]],
      "threshold": 1
    },
    {
      "name": "package",
      "pubkeys": ["package-step-public-key"],
      "expected_command": ["make", "package"],
      "expected_materials": [["MATCH", "dist/*", "IN", "dist/"]],
      "expected_products": [["CREATE", "release/myapp.tar.gz"]],
      "threshold": 1
    }
  ],
  "inspect": [
    {
      "name": "verify-release",
      "expected_command": ["sha256sum", "release/myapp.tar.gz"]
    }
  ]
}
```

### CI/CD 集成 (GitHub Actions)

```yaml
name: Supply Chain Verification
on: [push]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4
    - name: Install in-toto
      run: pip install in-toto
    - name: Build with attestation
      run: |
        in-toto-run \
          --step-name build \
          --key ${{ secrets.BUILD_KEY }} \
          --materials . \
          --products dist/ \
          --command make build
    - name: Upload attestation
      uses: actions/upload-artifact@v4
      with:
        name: build-link
        path: build.link
  verify:
    needs: build
    runs-on: ubuntu-latest
    steps:
    - name: Verify supply chain
      run: |
        in-toto-verify \
          --layout root.layout \
          --verification-keys owner.pub
```

## 运维操作

### 常用命令

```bash
# 🟢 验证供应链完整性
in-toto-verify --layout root.layout --verification-keys owner.pub --link-dir ./links/

# 🟢 查看 Link 元数据
cat build.link | jq .

# 🟢 检查产物哈希
in-toto-record --step-name build --key build-step stop

# 🟢 验证单个 Link 签名
in-toto-verify-link --link build.link --key build-step.pub

# 🟢 生成 SLSA Provenance
in-toto-run --step-name build --slsa-provenance \
  --command make build
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| 验证失败 | 步骤未按 Layout 执行 | `in-toto-verify --verbose` | 检查 Link 与 Layout 匹配 |
| 签名无效 | 密钥不匹配 | 检查 Link 签名和公钥 | 使用正确的密钥对 |
| 产物哈希不匹配 | 产物被篡改 | 对比 Link 中的哈希 | 重新执行构建步骤 |
| 缺少 Link | 步骤未执行 | `ls *.link` | 重新执行缺失步骤 |
| Layout 过期 | expires 字段过期 | 检查 Layout expires | 更新 Layout 有效期 |

### 排查流程

```
1. in-toto-verify --verbose → 查看验证详情
2. 检查每个步骤的 Link 文件是否存在
3. 验证 Link 签名与 Layout 中的公钥匹配
4. 对比 Link 中的 materials/products 哈希
5. 检查 Layout 有效期和步骤顺序
```

## 生产案例

### 案例1: SLSA Level 3 合规
- **场景**: 金融软件需要满足 SLSA Level 3 构建来源验证
- **方案**: in-toto 记录每个构建步骤，生成完整 attestation 链
- **效果**: 通过 SLSA Level 3 审计，实现完整供应链可追溯

### 案例2: 防止构建注入攻击
- **场景**: 担心 CI 环境被入侵后注入恶意代码
- **方案**: in-toto 验证每步执行的命令和产物，检测异常
- **效果**: 即使 CI 被入侵，篡改的构建产物无法通过验证

## 对比替代方案

| 维度 | in-toto | TUF | SLSA | Sigstore |
|------|---------|-----|------|----------|
| 关注点 | 构建过程 | 仓库完整性 | 框架规范 | 签名透明 |
| 验证粒度 | 每步骤 | 仓库级 | 级别化 | 个体制品 |
| 工具支持 | CLI+库 | 多语言库 | 规范文档 | CLI+服务 |
| 防篡改 | 过程级 | 仓库级 | 指导 | 签名级 |
| CNCF | Graduated | Graduated | 非 CNCF | Graduated |

## 检查清单

- [ ] Layout 定义了完整的供应链步骤
- [ ] 每个步骤使用独立密钥 (职责分离)
- [ ] CI/CD 集成 in-toto-run 生成 Link
- [ ] 部署前执行 in-toto-verify 验证
- [ ] Layout 有效期合理且定期更新
- [ ] 密钥安全存储 (HSM/密钥管理服务)
- [ ] 与 SLSA 框架对齐
- [ ] 审计日志保留完整

## Related

- [[opentofu]] — OpenTofu
- [[cartography]] — Cartography
- [[volcano]] — Volcano
- [[bpfman]] — bpfman
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- in-toto
- [[23-实体/cncf-security.md|[[23-实体/15-参考与索引/cncf-security|CNCF 安全与合规项目全景]]]] — Cross-reference
- index/gitops-cicd-index|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
