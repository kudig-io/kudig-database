---
title: kpt
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- kpt 是什么
- 如何 kpt
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- kpt
- cncf
- landscape
---

# kpt

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kpt.dev/ |
| **GitHub** | https://github.com/kptdev/kpt |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

kpt 是一个以 Git 为中心的 Kubernetes 配置包管理工具，由 Google 开发。它使用 Git 分发 Kubernetes 资源包（package），通过函数 (KRM Functions) 实现配置的声明式转换、验证和修改，并提供 GitOps 风格的资源管理能力。

### 核心特性

- **Git 原生**: 使用 Git 仓库作为配置包的分发和版本管理
- **KRM 函数**: 可复用的配置转换和验证函数
- **包管理**: fetch, update, publish 配置包的完整生命周期
- **Live apply**: 基于 ResourceGroup 的声明式资源管理
- **函数管道**: 将多个 KRM 函数串联为配置处理流水线
- **Porch**: Kubernetes 原生的包编排引擎

---

## 快速开始

### 安装

```bash
# macOS
brew install kpt

# 或下载二进制
curl -fsSL https://github.com/kptdev/kpt/releases/latest/download/kpt_$(uname -s)_$(uname -m).tar.gz | tar xz
sudo mv kpt /usr/local/bin/
```

### 获取配置包

```bash
# 从 Git 获取配置包
kpt pkg get https://github.com/GoogleContainerTools/kpt-samples.git/nginx@v0.9

# 查看包内容
cat nginx/Kptfile
ls nginx/
```

### Kptfile 配置

```yaml
apiVersion: kpt.dev/v1
kind: Kptfile
metadata:
  name: nginx
info:
  description: "Nginx deployment package"
pipeline:
  mutators:
    - image: gcr.io/kpt-fn/set-namespace:v0.4
      configMap:
        namespace: production
    - image: gcr.io/kpt-fn/set-labels:v0.2
      configMap:
        env: production
        team: platform
  validators:
    - image: gcr.io/kpt-fn/kubeval:v0.3
      configMap:
        strict: "true"
```

### 渲染和部署

```bash
# 执行函数管道渲染配置
kpt fn render nginx/

# 初始化资源清单
kpt live init nginx/

# 应用到集群
kpt live apply nginx/

# 查看状态
kpt live status nginx/

# 删除
kpt live destroy nginx/
```

---

## 配置详解

### KRM 函数使用

```bash
# 设置命名空间
kpt fn eval nginx/ --image gcr.io/kpt-fn/set-namespace:v0.4 -- namespace=staging

# 设置标签
kpt fn eval nginx/ --image gcr.io/kpt-fn/set-labels:v0.2 -- env=staging team=dev

# 验证配置
kpt fn eval nginx/ --image gcr.io/kpt-fn/kubeval:v0.3

# 搜索和替换
kpt fn eval nginx/ --image gcr.io/kpt-fn/search-replace:v0.2 -- \
  by-path="spec.replicas" \
  put-value="5"
```

### 包更新

```bash
# 更新到新版本
kpt pkg update nginx@v1.0

# 查看差异
kpt pkg diff nginx/
```

### 自定义 KRM 函数

```yaml
# 使用 Starlark 编写自定义函数
apiVersion: fn.kpt.dev/v1alpha1
kind: StarlarkRun
metadata:
  name: add-annotations
source: |
  def add_annotations(resources):
    for r in resources:
      if r["kind"] == "Deployment":
        r["metadata"].setdefault("annotations", {})
        r["metadata"]["annotations"]["managed-by"] = "kpt"
    return resources
```

---

## 最佳实践

1. **包复用**: 将通用配置封装为 kpt 包，通过 Git 共享
2. **函数管道**: 使用 pipeline 串联 mutator 和 validator
3. **版本锁定**: 使用 Git 标签锁定包版本
4. **Live 管理**: 使用 `kpt live` 替代 `kubectl apply` 实现声明式管理
5. **验证优先**: 在 pipeline 中添加 validator 在部署前检查配置

---

## 参考资源

- [kpt 官方文档](https://kpt.dev/)
- [kpt GitHub](https://github.com/kptdev/kpt)
- [KRM 函数目录](https://catalog.kpt.dev/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
