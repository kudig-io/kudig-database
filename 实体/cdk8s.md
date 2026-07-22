---
title: cdk8s (Cloud Development Kit for Kubernetes)
description: '## 概述'
summary: 'cdk8s (Cloud Development Kit for Kubernetes) 是一个开源软件开发框架，允许使用熟悉的编程语言定义 Kubernetes 应用和可重用抽象。它生成标准的 Kubernetes YAML 清单，可与任何 Kubernetes 集群配合使用。cdk8s 借鉴了 AWS CDK 的理念，'
category: entities
tags:
- k8s
- cncf
- config
- cdk8s
- helm
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cdk8s (Cloud Development Kit for Kubernetes) 是什么
- 如何 cdk8s (Cloud Development Kit for Kubernetes)
trigger_keywords:
- cdk8s
- Cloud
- Development
- Kit
- for
- Kubernetes
prerequisites:
- kubectl-basics
- helm-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[cdk8s|cdk8s]] (Cloud Development Kit for Kubernetes)

> **CNCF 状态**: Sandbox | **类别**: Config | **主要语言**: TypeScript, Python, Go, Java

## 概述

cdk8s (Cloud Development Kit for Kubernetes) 是一个开源软件开发框架，允许使用熟悉的编程语言定义 Kubernetes 应用和可重用抽象。它生成标准的 Kubernetes YAML 清单，可与任何 Kubernetes 集群配合使用。cdk8s 借鉴了 AWS CDK 的理念，将基础设施即代码提升到使用真正编程语言的高度。

## 核心能力

- **多语言支持**: TypeScript、Python、Go、Java
- **类型安全**: 编译时类型检查和 IDE 支持
- **可复用组件**: Constructs 抽象层实现代码复用
- **导入 CRD**: 自动从 CRD 生成类型化 API
- **Helm 支持**: 将 Helm Chart 作为 Construct 使用
- **测试友好**: 支持单元测试和快照测试

## K8s 集成

该项目作为云原生生态系统的一部分，与 Kubernetes 深度集成。通过 CRD、Operator 模式或原生 API 与 K8s 控制平面交互，支持在 [[概念/kubernetes-architecture-overview.md|Kubernetes 架构]] 中无缝运行。^[inferred]

## 生产部署要点

- **模块化**: 将复杂逻辑封装为 Construct
- **类型安全**: 充分利用 TypeScript 类型检查
- **测试覆盖**: 使用快照测试和单元测试
- **版本管理**: 锁定 cdk8s 和 K8s API 版本
- **复用组件**: 发布 Construct 库供团队使用
- **CI/CD 集成**: 在管道中运行 synth 和测试

## 架构定位

在 CNCF 生态中，cdk8s 属于 **Config** 类别，为云原生应用提供关键基础设施能力。^[inferred]

## 安装与配置

```bash
# 🟢 安装 cdk8s CLI
npm install -g cdk8s-cli

# 🟢 初始化 TypeScript 项目
mkdir my-cdk8s-app && cd my-cdk8s-app
cdk8s init typescript-app

# 🟢 初始化 Python 项目
cdk8s init python-app

# 🟢 初始化 Go 项目
cdk8s init go-app

# 🟢 生成 YAML 清单
cdk8s synth

# 🟢 应用到集群
cdk8s synth && kubectl apply -f dist/

# 🟢 导入 CRD 生成类型
cdk8s import crd.yaml -o src/generated
```

### Construct 示例 (TypeScript)

```typescript
import { Construct } from 'constructs';
import { App, Chart, ChartProps } from 'cdk8s';
import { KubeDeployment, KubeService } from 'cdk8s-plus-27';

// 可复用的 Web 服务 Construct
export class WebService extends Construct {
  constructor(scope: Construct, id: string, props: {
    image: string;
    replicas: number;
    port: number;
  }) {
    super(scope, id);

    const deployment = new KubeDeployment(this, 'deployment', {
      spec: {
        replicas: props.replicas,
        selector: { matchLabels: { app: id } },
        template: {
          metadata: { labels: { app: id } },
          spec: {
            containers: [{
              name: 'main',
              image: props.image,
              ports: [{ containerPort: props.port }],
              resources: {
                requests: { cpu: '100m', memory: '128Mi' },
                limits: { cpu: '500m', memory: '512Mi' },
              },
            }],
          },
        },
      },
    });

    new KubeService(this, 'service', {
      spec: {
        selector: { app: id },
        ports: [{ port: 80, targetPort: props.port }],
      },
    });
  }
}

// 使用 Construct
const app = new App();
const chart = new Chart(app, 'my-app');
new WebService(chart, 'frontend', {
  image: 'myorg/frontend:v1',
  replicas: 3,
  port: 8080,
});
app.synth();
```

## 运维操作

```bash
# 🟢 生成并查看 YAML
cdk8s synth
cat dist/my-app.k8s.yaml

# 🟢 运行测试
npm test  # TypeScript
pytest    # Python

# 🟡 更新 K8s API 版本
npm install cdk8s-plus-28  # 升级到 K8s 1.28 API

# 🟡 导入新的 CRD
cdk8s import https://raw.githubusercontent.com/org/crd.yaml -o src/generated

# 🟢 应用到集群
cdk8s synth && kubectl apply -f dist/ --dry-run=server
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| synth 失败 | 类型错误 | `npm run build` | 修复 TypeScript 类型错误 |
| 生成的 YAML 无效 | API 版本不匹配 | `kubectl apply --dry-run` | 更新 cdk8s-plus 版本 |
| CRD 导入失败 | CRD 格式错误 | `cdk8s import --debug` | 验证 CRD YAML 格式 |
| 测试失败 | 快照变更 | `npm test -- -u` | 审查并更新快照 |

## 生产案例

### 案例1：平台工程 Construct 库
- **场景**：平台团队需要为 20+ 业务团队提供标准化的 K8s 部署模板
- **方案**：将 Deployment+Service+HPA+PDB 封装为 Construct；发布为内部 npm 包；业务团队只需传入镜像和副本数
- **效果**：新服务接入时间从 2天 缩短到 30分钟，配置一致性 100%

### 案例2：多环境配置管理
- **场景**：同一应用需要部署到 dev/staging/prod，配置差异大
- **方案**：cdk8s Construct + 环境参数化；通过环境变量控制副本数、资源限制、镜像 tag；单元测试验证各环境配置
- **效果**：环境配置错误减少 90%，配置变更可追溯

## 对比替代方案

| 维度 | cdk8s | Helm | Kustomize | Pulumi |
|------|-------|------|-----------|--------|
| 语言 | TS/Py/Go/Java | Go模板 | YAML | TS/Py/Go |
| 类型安全 | 强 | 无 | 无 | 强 |
| 测试 | 单元+快照 | 无 | 无 | 单元 |
| 复用性 | Construct | Chart | Overlay | Component |
| 学习曲线 | 中 | 中 | 低 | 中 |

## 检查清单

- [ ] cdk8s CLI 已安装且版本正确
- [ ] 项目已初始化且可 synth
- [ ] 单元测试和快照测试已配置
- [ ] CRD 导入已验证
- [ ] CI/CD 中已集成 synth + test
- [ ] Construct 库已发布供团队复用

## 参考链接

- [[deployment]]
- [[实体/crd-custom-resources.md|crd-custom-resources]]
- [[概念/controller-pattern.md|controller-pattern]]
- [[概念/secrets-management.md|secrets-management]]
- [[pod-lifecycle]]

## Related

- [[kuasar]] — Kuasar
- [[longhorn]] — Longhorn
- [[open-cluster-management]] — [[实体/open-cluster-management.md|Open Cluster Management (OCM)]]
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[helm]] — Helm

- cdk8s
- [[实体/kpt.md|kpt]]
- [[实体/cncf-orchestration.md|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[生态参考/领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[生态参考/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
