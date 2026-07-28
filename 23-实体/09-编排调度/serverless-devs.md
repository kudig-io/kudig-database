---
title: Serverless Devs [entities]
description: '## 概述'
summary: 'Serverless Devs 是一个开源的 Serverless 开发者平台和命令行工具，致力于为开发者提供强大便捷的 Serverless 应用全生命周期管理能力。项目采用组件化设计，支持多云厂商的 Serverless 服务，让开发者能够使用统一的开发体验在不同云平台上开发、部署和管理 Serverless 应用。'
category: entities
tags:
- k8s
- cncf
- serverless
- serverless-devs
- scheduler
- opa
- crd
- operator
- agent
tier: core
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Serverless Devs 是什么
- 如何 Serverless Devs
trigger_keywords:
- Serverless
- Devs
prerequisites:
- kubectl-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Serverless Devs

> **CNCF 状态**: Sandbox | **类别**: Serverless | **主要语言**: TypeScript / JavaScript

## 概述

Serverless Devs 是由阿里云（中国信息通信研究院联合发起）开发的开源 Serverless 开发者工具平台，2021 年进入 CNCF Sandbox。它致力于为开发者提供统一的 Serverless 应用全生命周期管理能力——从本地开发、调试、部署到运维监控。项目采用**组件化（Component）设计**，通过统一的 CLI 和 YAML 规范抽象不同云厂商的 Serverless 服务，让开发者获得跨云的一致体验。

Serverless Devs 的核心差异化在于**多云无锁定（Multi-Cloud, No Lock-in）**——同一份 `s.yaml` 配置可以部署到阿里云函数计算（FC）、AWS Lambda、腾讯云 SCF、华为云 FG 等不同云厂商。它由**Serverless CLI**（命令行工具）、**组件生态**（cloud-infra、fc、lambda 等可插拔组件）和**Serverless Application Model**（统一应用描述规范）构成。

## Key Features

- **多云统一**：阿里云 FC、AWS Lambda、腾讯云 SCF、华为云 FG 统一 CLI 体验
- **组件化架构**：通过组件（Component）扩展支持不同 Serverless 平台
- **s.yaml 规范**：Serverless Application Model 声明式描述应用
- **本地调试**：本地模拟 Serverless 运行环境进行调试
- **CI/CD 集成**：通过 CLI 脚本无缝集成到 CI/CD 管道
- **应用监控**：查看函数调用日志、性能指标和追踪信息

## Architecture

Serverless Devs 由 **Serverless CLI（s）**（主命令行工具）、**Component System**（组件系统，每个组件对应一种 Serverless 平台或服务）、**s.yaml**（应用描述文件）和**Package Registry**（组件仓库，类似 npm registry）组成。CLI 读取 `s.yaml` 中的应用定义，根据 `component` 字段加载对应组件（如 `fc` 或 `lambda`），将声明式配置翻译为目标平台的 API 调用。

## K8s 集成

Serverless Devs 支持 Kubernetes 上的 Serverless 部署。通过 `kubeless` 或 `knative` 组件，可以将函数部署到 K8s 中的 Serverless 平台。也支持部署到阿里云 ACK Serverless（基于虚拟节点和弹性容器实例），获得 Serverless 的弹性伸缩体验。

## 生产部署要点

- **组件选择**：根据目标云平台选择对应的组件版本
- **密钥管理**：使用 `s config` 安全管理不同云厂商的 access key
- **环境隔离**：为 dev/staging/prod 维护独立的 s.yaml 配置
- **CI/CD 集成**：在流水线中使用 `s deploy --use-local` 实现自动化部署
- **监控接入**：配置日志和追踪输出到统一监控平台

## 生产场景

1. **多云 Serverless 部署**：同一函数部署到阿里云 FC 和 AWS Lambda，实现多云容灾
2. **事件驱动应用**：上传文件触发图像处理函数，API 请求触发计算函数
3. **定时任务**：Cron 触发的数据同步和报表生成函数
4. **API 后端**：Serverless 化的 RESTful API，按需弹性扩缩

## 安装与配置

```bash
# 安装 Serverless Devs CLI
npm install -g @serverless-devs/s
# 或使用安装脚本
curl -fsSL https://serverless-ai.oss-cn-hangzhou.aliyuncs.com/install.sh | bash
s version

# 配置云厂商密钥
s config add --AccessKeyID xxx --AccessKeySecret yyy -a default

# 初始化新项目
s init devsapp/start-fc-http-nodejs14
cd start-fc-http-nodejs14
```

### s.yaml 配置示例

```yaml
edition: 3.0.0
name: my-app
access: default
vars:
  region: cn-hangzhou
resources:
  hello_world:
    component: fc3
    props:
      region: ${vars.region}
      functionName: hello-world
      runtime: nodejs18
      handler: index.handler
      code:
        source: ./code
      memorySize: 256
      timeout: 30
      triggers:
        - triggerName: http-trigger
          triggerType: http
          triggerConfig:
            authType: anonymous
            methods: ["GET", "POST"]
```

```bash
# 部署
s deploy
# 调用
s invoke
# 查看日志
s logs --tail
```

## 运维操作

```bash
# 🟢 查看函数状态
s info

# 🟢 查看实时日志
s logs --tail

# 🟡 部署函数（更新代码/配置）
s deploy

# 🟡 本地调用测试
s invoke --event '{"key":"value"}'

# 🟡 回滚到上一版本
s rollback

# 🔴 删除函数及触发器
s remove
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| deploy 失败 | 密钥过期/权限不足 | `s config get -a default` | 更新 AccessKey |
| 函数超时 | 内存不足/代码死循环 | `s logs` | 增加 memorySize/修复代码 |
| 触发器无响应 | 触发器配置错误 | `s info` | 检查 trigger 配置 |
| 冷启动慢 | 依赖包过大 | 检查 code 目录大小 | 精简依赖/使用层 |
| 网络访问失败 | VPC 配置缺失 | 检查函数 VPC 配置 | 添加 VPC 绑定 |

```
排查流程:
├── 部署失败
│   ├── s deploy --debug → 详细错误
│   ├── 检查 s.yaml 语法
│   └── 确认云账号权限充足
├── 函数执行异常
│   ├── s logs --tail → 实时日志
│   ├── s invoke → 手动触发测试
│   └── 检查 runtime 版本兼容性
└── 性能问题
    ├── 检查冷启动时间
    ├── 优化依赖包大小
    └── 考虑预留实例数
```

## 生产案例

### 案例 1: 多云函数统一管理

- **场景**: 团队同时使用阿里云 FC 和 AWS Lambda，管理工具分散
- **方案**: 使用 Serverless Devs 统一管理，通过不同 access 配置多云凭据；统一 s.yaml 规范
- **效果**: 部署流程统一，新函数上线时间从 2h 缩短到 15min

### 案例 2: CI/CD 集成自动化部署

- **场景**: 函数代码更新后需要手动部署，容易遗漏
- **方案**: GitHub Actions 集成 `s deploy`；PR 合并后自动部署到生产；添加 `s invoke` 烟雾测试
- **效果**: 部署自动化 100%，发布事故减少 90%

## 对比

| 特性 | Serverless Devs | Serverless Framework | SAM | funcraft | 适用场景 |
|------|----------------|---------------------|-----|----------|----------|
| 多云 | ✅ 阿里云/AWS/腾讯 | ✅ 多云 | ❌ AWS only | ❌ 阿里云 only | 多云管理 |
| 开源 | ✅ | ⚠️ 核心 | ✅ | ✅ | 自主可控 |
| 组件生态 | ✅ Registry | ✅ Plugins | ⚠️ | ❌ | 扩展能力 |
| K8s 支持 | ⚠️ | ✅ | ❌ | ❌ | 混合部署 |
| 阿里云优化 | ✅ 原生 | ⚠️ | ❌ | ✅ | 阿里云用户 |

## 参考链接

- [[22-概念/08-可靠性与运维/microservice-resilience-patterns.md|microservice-resilience-patterns]]
- [[22-概念/05-安全/secrets-management.md|secrets-management]]
- [[23-实体/02-K8s核心组件/kube-scheduler.md|kube-scheduler]]
- [[22-概念/09-平台与发布/ci-cd-pipeline-patterns.md|ci-cd-pipeline-patterns]]

## Related

- [[oauth2-proxy]] — OAuth2 Proxy
- [[schemahero]] — SchemaHero
- [[composefs]] — composefs
- [[opa]] — OPA (Open Policy Agent)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- serverless-devs
- [[23-实体/slimfaas.md|[[slimfaas|SlimFaas]]]]
- [[23-实体/15-参考与索引/cncf-edge-ai.md|CNCF 边缘计算与 AI/ML 项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
