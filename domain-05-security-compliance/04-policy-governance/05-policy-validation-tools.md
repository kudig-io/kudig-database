---
title: 05 - 策略校验与准入控制工具 (Policy Validation)
description: '| **Polaris** | YAML | 配置审计、仪表盘 | 极简 |'
category: security
tags:
- k8s
- security
- rbac
- authentication
- authorization
- opa
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- 策略校验与准入控制工具 (Policy Validation) 是什么
- 如何 策略校验与准入控制工具 (Policy Validation)
- Kubernetes 7 security 最佳实践
trigger_keywords:
- 策略校验与准入控制工具
- Policy
- Validation
- security
prerequisites:
- kubectl-basics
- rbac-basics
- policy-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/tls-pki.md
  label: '速查卡: tls-pki'
created: "2026-05-23"
---

# 05 - 策略校验与准入控制工具 (Policy Validation)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

<!-- chunk: 策略引擎对比 -->
## 策略引擎对比

| 工具 (Tool) | 策略语言 (Language) | 核心能力 (Capabilities) | 学习曲线 |
|------------|-------------------|----------------------|---------|
| **OPA/Gatekeeper** | Rego | 通用策略引擎、强大灵活 | 陡峭 |
| **[[Kyverno|Kyverno]]** | YAML | K8s 原生、易上手 | 平缓 |
| **Polaris** | YAML | 配置审计、仪表盘 | 极简 |
| **[[Kubewarden|Kubewarden]]** | WebAssembly | 多语言策略、高性能 | 中等 |

<!-- chunk: Kyverno 生产实践 -->
## Kyverno 生产实践

### 1. 强制镜像来源
```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-registry
spec:
  validationFailureAction: enforce
  rules:
  - name: check-registry
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "镜像必须来自可信仓库"
      pattern:
        spec:
          containers:
          - image: "registry.cn-hangzhou.aliyuncs.com/*"
```

### 2. 自动注入 Sidecar
```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: inject-sidecar
spec:
  rules:
  - name: add-logging-sidecar
    match:
      any:
      - resources:
          kinds:
          - Deployment
          namespaces:
          - production
    mutate:
      patchStrategicMerge:
        spec:
          template:
            spec:
              containers:
              - name: log-collector
                image: fluent/fluent-bit:latest
```

### 3. 资源配额验证
```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-resources
spec:
  validationFailureAction: enforce
  rules:
  - name: check-resources
    match:
      any:
      - resources:
          kinds:
          - Pod
    validate:
      message: "必须设置资源 requests 和 limits"
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

<!-- chunk: OPA/Gatekeeper 高级策略 -->
## OPA/Gatekeeper 高级策略

### 1. 禁止特权容器
```yaml
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sPSPPrivilegedContainer
metadata:
  name: psp-privileged-container
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    excludedNamespaces: ["kube-system"]
```

### 2. 镜像签名验证
```rego
package kubernetes.admission

deny[msg] {
  input.request.kind.kind == "Pod"
  image := input.request.object.spec.containers[_].image
  not image_signed(image)
  msg := sprintf("镜像未签名: %v", [image])
}

image_signed(image) {
  # 调用外部签名验证服务
  http.send({
    "method": "GET",
    "url": sprintf("https://notary.example.com/verify?image=%v", [image])
  }).status_code == 200
}
```

<!-- chunk: Polaris 配置审计 -->
## Polaris 配置审计

### 仪表盘指标
- **安全性**: 特权容器、只读根文件系统
- **可靠性**: 探针配置、副本数
- **效率**: 资源限制、镜像标签

### 命令行扫描
```bash
polaris audit --audit-path ./manifests/ --format=json > audit-report.json
```

<!-- chunk: 策略治理最佳实践 -->
## 策略治理最佳实践

| 实践 (Practice) | 说明 (Description) |
|----------------|-------------------|
| **分层策略** | 集群级 + 命名空间级 |
| **审计模式** | 先 audit 后 enforce |
| **例外管理** | 使用 Annotation 豁免 |
| **持续监控** | 定期审计现有资源 |
| **文档化** | 策略说明与修复指南 |


---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-05-security-compliance KUDIG Database — Global MOC
- [[domain-05-security-compliance/README|Security Domain]]
- [[domain-05-security-compliance/00-open-source-projects-index|Domain-7 安全 — 开源项目索引]]
- Kubernetes 认证授权体系详解
- 网络安全策略与零信任架构
- 运行时安全防护与威胁检测
- 04 - 审计日志与合规性管理
- 06 - Pod安全标准详解
- 07 - RBAC权限矩阵表
- 08 - 安全最佳实践表
- Kubernetes 安全加固
- 证书管理与 TLS 配置

## See Also

- 03-runtime-security-defense
- 04-audit-logging-compliance
- 06-pod-security-standards
- 07-rbac-matrix-configuration

- [[domain-05-security-compliance/README|返回目录]]

## Related

- [[domain-19-landscape-references/topic-index/security-index|Security 安全知识图谱索引]]
