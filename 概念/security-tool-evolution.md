---
title: 安全工具演进
description: '# 安全工具演进'
summary: 'OPA 是通用策略引擎，Gatekeeper 是其 Kubernetes 特定的实现。'
category: concepts
tags:
- k8s
- release-notes
- falco
- opa
- trivy
- gatekeeper
- cert-manager
- security
- ingress
- crd
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 安全工具演进 是什么
- 如何 安全工具演进
trigger_keywords:
- 安全工具演进
prerequisites:
- kubectl-basics
- iac-basics
- ebpf-basics
- tls-basics
- policy-basics
status: stable
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 安全工具演进

> 本文档综合了 `生态参考/_archived-release-notes/security/` 目录下 5 个安全工具的 218 个版本发布说明 ^[inferred]

## 组件概览

| 组件 | 版本范围 | 定位 |
|---|---|---|
| [[falco|Falco]] | 43 个版本 | 运行时安全与异常检测 |
| [[opa|opa]] | 86 个版本 | 通用策略引擎 |
| Gatekeeper | 24 个版本 | OPA 的 Kubernetes 准入集成 |
| [[实体/trivy.md|[[Trivy|trivy]]]] | 28 个版本 | 容器和 IaC 安全扫描 |
| [[cert-manager|cert-manager]] | 37 个版本 | Kubernetes 证书管理 |

## Falco 版本演进

Falco 是云原生运行时安全项目，通过系统调用和行为检测异常。

### v0.10 关键变更

- **规则目录支持**：Falco 从 `/etc/falco/rules.d` 读取规则文件
- 支持所有系统调用（包括无参数提取的）
- 容器构建使用 gcc 5.0
- USR1 信号支持日志轮转
- 资源使用优化（限制系统调用集合）
- 新增规则：Disallowed SSH Connection、Unexpected K8s NodePort Connection、Unexpected UDP Traffic

### 后续演进

- eBPF 探针支持（替代内核模块）
- 改进的规则引擎
- 更好的 Kubernetes 集成
- 输出到多种后端（[[gRPC|gRPC]]、Webhook 等）^ [inferred]

### Falco 规则体系

Falco 通过规则定义安全策略，每个规则包含：
- 条件（使用系统调用过滤）
- 输出（告警消息）
- 优先级
- 标签

## OPA (Open Policy Agent) 版本演进

OPA 是通用策略引擎，Gatekeeper 是其 Kubernetes 特定的实现。

### v0.10 关键变更

- Hugo 文档发布到 GitHub Pages
- 新增 `array.slice` 内置函数
- 新增 `net.cidr_contains` 和 `net.cidr_intersects`（替代 `net.cidr_overlap`）
- 教程中 kube-mgmt 更新到 v0.8
- AST 集合和对象分配优化
- 新增 Kubernetes Admission Control 指南

### OPA 核心概念

| 概念 | 说明 |
|---|---|
| Rego | OPA 的策略语言 |
| Policy | 用 Rego 编写的规则 |
| Data | 策略评估的输入 |
| Query | 策略评估请求 |
| Decision | 策略评估结果（allow/deny） |

### 后续演进

- Rego 语言持续增强
- 性能优化
- 更好的 Kubernetes 集成
- WebAssembly 支持 ^[inferred]

## Gatekeeper 版本演进

Gatekeeper 将 OPA 集成到 Kubernetes 准入控制流程。

### 核心功能

- ValidatingAdmissionWebhook
- 约束模板（ConstraintTemplate）
- 约束（Constraint）
- 审计功能
- 外部数据源 ^[inferred]

## Trivy 版本演进

Trivy 是 Aqua Security 开发的全能安全扫描工具。

### 扫描能力

- 容器镜像漏洞扫描
- 文件系统扫描
- Git 仓库扫描
- IaC 扫描（Terraform、Kubernetes 等）
- SBOM 生成 ^[inferred]

## cert-manager 版本演进

cert-manager 自动化 Kubernetes 中的 TLS 证书管理。

### 核心功能

- 自动证书颁发和续期
- 支持 ACME（Let's Encrypt）
- 支持自签名和 CA 签发
- Ingress 集成
- Certificate CRD ^[inferred]

## 安全层次

```
供应链安全：Trivy（镜像扫描）+ cert-manager（证书）
    |
准入安全：OPA/Gatekeeper（策略准入）
    |
运行时安全：Falco（系统调用监控）
```

## 源码实现分析

### Falco 系统调用监控

```c
// falco/userspace/falco/falco.cpp
// Falco 通过 eBPF/内核模块捕获系统调用，匹配规则引擎
void falco_processor::process_event(sinsp_evt *evt) {
    // 1. 从 eBPF probe 获取系统调用事件
    uint16_t type = evt->get_type();  // PPME_SYSCALL_OPEN_E, etc.
    
    // 2. 匹配规则引擎
    for (auto &rule : m_rules) {
        if (rule.matches(evt)) {
            // 3. 触发告警
            emit_alert(rule, evt);
            // 例: "Terminal shell in container"
            // evt: open() by uid=0 in container nginx
        }
    }
}

// Falco 规则示例 (YAML)
// - rule: Terminal shell in container
//   desc: A shell was used as the entrypoint/exec point into a container
//   condition: >
//     spawned_process and container and
//     proc.name in (bash, sh, zsh)
//   output: "Shell opened in container (user=%user.name container=%container.name)"
//   priority: WARNING
```

### 安全工具架构对比

```
┌───────────────────────────────────────────────────────────┐
│          安全工具架构对比                            │
├───────────────────────────────────────────────────────────┤
│                                                           │
│  供应链安全 (CI/CD 阶段)                                │
│  ────────────────────                                    │
│  Trivy: 镜像扫描 (CVE/密钥/配置)                    │
│       → 集成到 CI 流水线，阻断高危镜像             │
│  cert-manager: 自动证书管理 (Let's Encrypt/内部 CA) │
│       → Certificate CRD → 自动签发/续期             │
│                                                           │
│  准入安全 (部署阶段)                                    │
│  ────────────────────                                    │
│  OPA/Gatekeeper: 策略即代码 (Rego)                   │
│       → ConstraintTemplate + Constraint              │
│       → 拒绝不合规资源 (privileged/无 limit)       │
│                                                           │
│  运行时安全 (运行阶段)                                  │
│  ────────────────────                                    │
│  Falco: 系统调用监控 (eBPF/内核模块)               │
│       → 实时检测异常行为 (shell/文件篡改)         │
│       → 告警到 Slack/PagerDuty                       │
│                                                           │
│  安全层次:                                               │
│  供应链 → 准入 → 运行时 (纵深防御)                │
└───────────────────────────────────────────────────────────┘
```

### 生产安全配置示例（🟡 部署到集群）

```yaml
# Gatekeeper Constraint: 禁止特权容器
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sPSPPrivilegedContainer
metadata:
  name: no-privileged-containers
spec:
  match:
    kinds:
    - apiGroups: [""]
      kinds: ["Pod"]
    excludedNamespaces: ["kube-system"]  # 系统组件除外
---
# Falco 规则: 检测容器内 shell
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-custom-rules
data:
  custom_rules.yaml: |
    - rule: Shell in Production Container
      condition: >
        spawned_process and container and
        proc.name in (bash, sh, zsh) and
        k8s.ns.name = production
      output: "Shell in prod container (user=%user.name ns=%k8s.ns.name)"
      priority: CRITICAL
```

## 常见误区

| 误区 | 正确理解 |
|------|----------|
| 镜像扫描一次就够了 | 新 CVE 不断披露，需持续扫描运行中镜像 |
| Gatekeeper 可以替代 PSA | PSA 是内置基础策略，Gatekeeper 补充复杂策略 |
| Falco 无性能开销 | eBPF 模式开销小，内核模块模式有可观开销 |
| 安全工具装了就行 | 必须配置告警路由和响应流程，否则无意义 |
| cert-manager 不需要监控 | 证书签发失败会导致服务中断，必须监控 |
| 安全是安全团队的事 | 安全是每个人的责任，DevSecOps 文化 |

## 面试要点

1. **安全工具链的层次和分工？**
   - 供应链：Trivy（镜像扫描）+ cert-manager（证书）
   - 准入：OPA/Gatekeeper（策略准入）
   - 运行时：Falco（系统调用监控）
   - 纵深防御：每层都有独立防护

2. **OPA/Gatekeeper 的工作原理？**
   - ConstraintTemplate：定义策略模板（Rego 语言）
   - Constraint：实例化策略，应用到集群
   - 准入 Webhook：拦截不合规资源

3. **Falco 与 PSA 的区别？**
   - PSA：内置准入策略，阻止不合规 Pod 创建
   - Falco：运行时检测，发现异常行为
   - 互补：PSA 防于未然，Falco 检测于运行时

4. **生产环境安全工具部署顺序？**
   - 1. cert-manager（证书基础设施）
   - 2. Trivy（CI/CD 镜像扫描）
   - 3. Gatekeeper（准入策略）
   - 4. Falco（运行时监控）

## 来源文档

- 生态参考/_archived-release-notes/security/falco/（43 个文件）
- 生态参考/_archived-release-notes/security/opa/（86 个文件）
- 生态参考/_archived-release-notes/security/gatekeeper/（24 个文件）
- 生态参考/_archived-release-notes/security/trivy/（28 个文件）
- 生态参考/_archived-release-notes/security/cert-manager/（37 个文件）

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[opa]] — OPA (Open Policy Agent)
- [[falco]] — Falco
- [[实体/trivy.md|trivy]] — Trivy
- [[cert-manager]] — cert-manager

- [[系统基础/速查卡/k8s.md|k8s]]

<!-- risk-assessed -->
