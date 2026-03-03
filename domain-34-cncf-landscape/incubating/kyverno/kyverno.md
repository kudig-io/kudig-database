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
Kyverno 是 Kubernetes 原生的策略引擎，使用 YAML 定义策略，无需学习新语言。

### 核心定位
Kyverno 提供验证、变更和生成策略，简化 Kubernetes 策略管理。

---

## 核心功能

### 主要特性
- **YAML 策略**: 无需学习 Rego
- **验证策略**: 资源准入控制
- **变更策略**: 自动修改资源
- **生成策略**: 自动创建资源

### 示例
```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-labels
spec:
  validationFailureAction: Enforce
  rules:
  - name: check-labels
    match:
      resources:
        kinds:
        - Pod
    validate:
      message: "The label 'app' is required."
      pattern:
        metadata:
          labels:
            app: "?*"
```

---

## 参考资源

- [官方文档](https://kyverno.io/docs)
- [GitHub Repo](https://github.com/kyverno/kyverno)
- [CNCF 项目页面](https://www.cncf.io/projects/kyverno/)

---

**维护者**: Kudig Team | **许可证**: MIT
