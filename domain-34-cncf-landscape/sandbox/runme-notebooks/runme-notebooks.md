# Runme

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://runme.dev/ |
| **GitHub** | https://github.com/stateful/runme |
| **许可证** | Apache-2.0 |
| **开发语言** | Go, TypeScript |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Runme 是一个交互式 Markdown 运行时，可以将 Markdown 文档中的代码块转化为可执行的交互式笔记本。它让开发者可以直接在 VS Code 中运行 README、runbook 和文档中的命令，并保存执行结果。Runme 特别适合 DevOps、SRE 运维手册和开发文档的交互式执行。

### 核心特性

- **交互式 Markdown**: 在 Markdown 中直接运行 shell、Python、SQL 等代码
- **VS Code 集成**: 原生 VS Code 扩展，无缝融入开发工作流
- **会话持久化**: 保存命令执行输出，创建可重复的运行记录
- **环境变量管理**: 安全管理和注入环境变量
- **云集成**: 支持 AWS、GCP、Kubernetes 等云服务的交互式操作
- **协作分享**: 分享带有执行结果的 Notebook

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                     Runme Ecosystem                    │
│                                                       │
│  ┌──────────────────────────────────────────────┐    │
│  │           VS Code / IDE Integration            │    │
│  │  ┌──────────────────────────────────────────┐│    │
│  │  │       Runme VS Code Extension            ││    │
│  │  │  ┌────────────┐  ┌─────────────────────┐││    │
│  │  │  │ Notebook   │  │ Cell Execution      │││    │
│  │  │  │ Renderer   │  │ Engine              │││    │
│  │  │  └────────────┘  └─────────────────────┘││    │
│  │  └──────────────────────────────────────────┘│    │
│  └─────────────────────┬────────────────────────┘    │
│                        │                              │
│  ┌─────────────────────▼────────────────────────┐    │
│  │              Runme Kernel                      │    │
│  │  ┌──────────────┐  ┌───────────────────────┐ │    │
│  │  │ Session      │  │ Environment Manager   │ │    │
│  │  │ Manager      │  │ (Secrets/Vars)        │ │    │
│  │  └──────────────┘  └───────────────────────┘ │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │                              │
│  ┌─────────────────────▼────────────────────────┐    │
│  │             Execution Backends                 │    │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌─────────────┐  │    │
│  │  │Bash  │ │Python│ │ SQL  │ │ Kubernetes  │  │    │
│  │  └──────┘ └──────┘ └──────┘ └─────────────┘  │    │
│  └──────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装 VS Code 扩展

```bash
# 通过 VS Code Marketplace 安装
# 1. 打开 VS Code
# 2. Extensions (Ctrl+Shift+X)
# 3. 搜索 "Runme"
# 4. 点击 Install

# 或使用命令行
code --install-extension stateful.runme
```

### 安装 CLI

```bash
# macOS
brew install runme

# Linux
curl -sSL https://runme.dev/install.sh | bash

# 验证
runme --version
```

### 创建 Runme Notebook

```markdown
# My Runbook

## Setup

First, let's check our environment:

```bash
echo "Current directory: $(pwd)"
echo "User: $(whoami)"
```

## Deploy Application

```bash
kubectl apply -f deployment.yaml
```

Check deployment status:

```bash
kubectl rollout status deployment/my-app
```

## Verify

```bash
kubectl get pods -l app=my-app
```
```

### 在 VS Code 中运行

1. 使用 VS Code 打开 Markdown 文件
2. VS Code 自动识别为 Runme Notebook
3. 点击代码块旁的 "Run" 按钮执行
4. 输出会显示在代码块下方

---

## 高级功能

### 环境变量管理

```markdown
```bash {"name":"set-env"}
export CLUSTER_NAME="production"
export NAMESPACE="my-app"
```

```bash {"name":"use-env"}
kubectl config use-context $CLUSTER_NAME
kubectl get pods -n $NAMESPACE
```
```

### 交互式输入

```markdown
```bash {"interactive":"true"}
read -p "Enter deployment name: " DEPLOY_NAME
kubectl describe deployment $DEPLOY_NAME
```
```

### 背景执行

```markdown
```bash {"background":"true","name":"port-forward"}
kubectl port-forward svc/my-app 8080:80
```

Now you can access the app at http://localhost:8080
```

### 条件执行

```markdown
```bash {"name":"check-cluster","promptEnv":"true"}
# This will prompt for CLUSTER_NAME if not set
kubectl cluster-info --context=$CLUSTER_NAME
```
```

### 多语言支持

```markdown
## Python 示例

```python {"name":"analyze-data"}
import pandas as pd

data = pd.read_csv('metrics.csv')
print(data.describe())
```

## SQL 查询

```sql {"name":"query-db"}
SELECT * FROM users WHERE created_at > '2024-01-01' LIMIT 10;
```
```

---

## 与其他方案对比

| 特性 | Runme | Jupyter | Zeppelin | Observable |
|:---|:---|:---|:---|:---|
| 文件格式 | Markdown | .ipynb | JSON | JS |
| Shell 支持 | 原生 | 需 kernel | 支持 | 有限 |
| Git 友好 | 纯文本 | JSON | JSON | 专有 |
| IDE 集成 | VS Code 原生 | 独立/插件 | Web | Web |
| 运维场景 | 专为运维设计 | 数据科学 | 大数据 | 可视化 |
| 环境管理 | 内置 | 需配置 | 需配置 | N/A |

---

## 最佳实践

1. **文档即代码**: 将 runbook 和文档作为代码纳入版本控制
2. **环境隔离**: 使用 Runme 的环境变量功能隔离不同环境配置
3. **分段执行**: 将长流程拆分为多个单元格，便于调试和复用
4. **结果保存**: 保存执行输出，便于问题排查和审计
5. **协作分享**: 使用 Runme Cloud 分享带结果的 Notebook

---

## 参考资源

- [Runme 官方文档](https://docs.runme.dev/)
- [Runme GitHub](https://github.com/stateful/runme)
- [VS Code 扩展](https://marketplace.visualstudio.com/items?itemName=stateful.runme)
- [Runme 示例](https://github.com/stateful/runme/tree/main/examples)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
