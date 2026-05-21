---
title: cdk8s
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- helm
- redis
- ingress
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- cdk8s 是什么
- 如何 cdk8s
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- cdk8s
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- redis-basics
- tls-basics
---

title: cdk8s
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- helm
- redis
- ingress
- crd
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- cdk8s 是什么
- 如何 cdk8s
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- cdk8s
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# cdk8s

> **成熟度**: Sandbox | **加入时间**: 2020-11 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://cdk8s.io |
| **GitHub** | https://github.com/cdk8s-team/cdk8s |
| **许可证** | Apache-2.0 |
| **开发语言** | TypeScript, Python, Go, Java |
| **CNCF 分类** | App Definition & Build |
| **维护组织** | AWS |

---

## 项目概述

cdk8s (Cloud Development Kit for Kubernetes) 是一个开源软件开发框架，允许使用熟悉的编程语言定义 Kubernetes 应用和可重用抽象。它生成标准的 Kubernetes YAML 清单，可与任何 Kubernetes 集群配合使用。cdk8s 借鉴了 AWS CDK 的理念，将基础设施即代码提升到使用真正编程语言的高度。

---

## 核心特性

- **多语言支持**: TypeScript、Python、Go、Java
- **类型安全**: 编译时类型检查和 IDE 支持
- **可复用组件**: Constructs 抽象层实现代码复用
- **导入 CRD**: 自动从 CRD 生成类型化 API
- **Helm 支持**: 将 Helm Chart 作为 Construct 使用
- **测试友好**: 支持单元测试和快照测试
- **标准输出**: 生成标准 Kubernetes YAML

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                      cdk8s Architecture                          │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Application Code                        │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │              Your cdk8s Application                  │ │   │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │ │   │
│  │  │  │ TypeScript  │  │   Python    │  │    Go      │  │ │   │
│  │  │  │   /Java     │  │             │  │            │  │ │   │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │                               │
│  ┌──────────────────────────────▼──────────────────────────┐   │
│  │                   cdk8s Core Library                      │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │                  Constructs                          │ │   │
│  │  │  ┌───────────────────────────────────────────────┐  │ │   │
│  │  │  │                    App                         │  │ │   │
│  │  │  │  ┌─────────────────────────────────────────┐  │  │ │   │
│  │  │  │  │               Charts                     │  │  │ │   │
│  │  │  │  │  ┌─────────────────────────────────────┐│  │  │ │   │
│  │  │  │  │  │           Constructs                ││  │  │ │   │
│  │  │  │  │  │  ┌──────────┐  ┌──────────────────┐││  │  │ │   │
│  │  │  │  │  │  │ Built-in │  │  cdk8s-plus     │││  │  │ │   │
│  │  │  │  │  │  │ K8s API  │  │  (High-level)   │││  │  │ │   │
│  │  │  │  │  │  └──────────┘  └──────────────────┘││  │  │ │   │
│  │  │  │  │  └─────────────────────────────────────┘│  │  │ │   │
│  │  │  │  └─────────────────────────────────────────┘  │  │ │   │
│  │  │  └───────────────────────────────────────────────┘  │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │ cdk8s synth                   │
│                                 ▼                               │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    Output (dist/)                         │   │
│  │  ┌─────────────────────────────────────────────────────┐ │   │
│  │  │              Kubernetes YAML Manifests               │ │   │
│  │  │  ┌────────────────┐  ┌────────────────────────────┐ │ │   │
│  │  │  │ deployment.yaml│  │ service.yaml              │ │ │   │
│  │  │  │ configmap.yaml │  │ ingress.yaml              │ │ │   │
│  │  │  └────────────────┘  └────────────────────────────┘ │ │   │
│  │  └─────────────────────────────────────────────────────┘ │   │
│  └──────────────────────────────┬──────────────────────────┘   │
│                                 │ kubectl apply                 │
│                                 ▼                               │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                   Kubernetes Cluster                      │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

| 概念 | 说明 |
|:---|:---|
| **App** | 应用入口，包含一个或多个 Chart |
| **Chart** | 资源集合，类似 Helm Chart |
| **Construct** | 可复用组件，封装 K8s 资源 |
| **cdk8s-plus** | 高级 API，简化常见用例 |

---

## 快速开始

### 安装 CLI

```bash
# npm
npm install -g cdk8s-cli

# yarn
yarn global add cdk8s-cli

# Homebrew
brew install cdk8s
```

### 初始化项目

```bash
# TypeScript 项目
cdk8s init typescript-app

# Python 项目
cdk8s init python-app

# Go 项目
cdk8s init go-app

# Java 项目
cdk8s init java-app
```

---

## TypeScript 示例

### 基础示例

```typescript
// main.ts
import { App, Chart } from 'cdk8s';
import { KubeDeployment, KubeService, IntOrString } from './imports/k8s';
import { Construct } from 'constructs';

class MyChart extends Chart {
  constructor(scope: Construct, id: string) {
    super(scope, id);

    const label = { app: 'hello-k8s' };

    new KubeDeployment(this, 'deployment', {
      spec: {
        replicas: 3,
        selector: { matchLabels: label },
        template: {
          metadata: { labels: label },
          spec: {
            containers: [{
              name: 'hello-kubernetes',
              image: 'paulbouwer/hello-kubernetes:1.7',
              ports: [{ containerPort: 8080 }]
            }]
          }
        }
      }
    });

    new KubeService(this, 'service', {
      spec: {
        type: 'LoadBalancer',
        ports: [{ port: 80, targetPort: IntOrString.fromNumber(8080) }],
        selector: label
      }
    });
  }
}

const app = new App();
new MyChart(app, 'hello');
app.synth();
```

### 使用 cdk8s-plus (高级 API)

```typescript
// main.ts
import { App, Chart, Duration } from 'cdk8s';
import * as kplus from 'cdk8s-plus-27';
import { Construct } from 'constructs';

class WebAppChart extends Chart {
  constructor(scope: Construct, id: string) {
    super(scope, id);

    // 创建 Deployment
    const deployment = new kplus.Deployment(this, 'deployment', {
      replicas: 3,
      containers: [{
        image: 'nginx:latest',
        portNumber: 80,
        resources: {
          cpu: {
            request: kplus.Cpu.millis(100),
            limit: kplus.Cpu.millis(500)
          },
          memory: {
            request: kplus.Size.mebibytes(128),
            limit: kplus.Size.mebibytes(512)
          }
        },
        readiness: kplus.Probe.fromHttpGet('/healthz', {
          initialDelaySeconds: Duration.seconds(10),
          periodSeconds: Duration.seconds(5)
        })
      }]
    });

    // 创建 Service
    deployment.exposeViaService({
      serviceType: kplus.ServiceType.LOAD_BALANCER,
      ports: [{ port: 80 }]
    });

    // 创建 ConfigMap
    const config = new kplus.ConfigMap(this, 'config', {
      data: {
        'app.conf': 'key=value'
      }
    });

    // 挂载 ConfigMap
    deployment.containers[0].mount('/etc/config', kplus.Volume.fromConfigMap(this, 'config-vol', config));

    // 创建 Ingress
    new kplus.Ingress(this, 'ingress', {
      rules: [{
        host: 'myapp.example.com',
        backend: kplus.IngressBackend.fromService(deployment.service!)
      }]
    });
  }
}

const app = new App();
new WebAppChart(app, 'webapp');
app.synth();
```

---

## Python 示例

```python
# main.py
from constructs import Construct
from cdk8s import App, Chart
from imports import k8s

class MyChart(Chart):
    def __init__(self, scope: Construct, id: str):
        super().__init__(scope, id)

        label = {"app": "hello-k8s"}

        k8s.KubeDeployment(self, "deployment",
            spec=k8s.DeploymentSpec(
                replicas=3,
                selector=k8s.LabelSelector(match_labels=label),
                template=k8s.PodTemplateSpec(
                    metadata=k8s.ObjectMeta(labels=label),
                    spec=k8s.PodSpec(containers=[
                        k8s.Container(
                            name="hello-kubernetes",
                            image="nginx:latest",
                            ports=[k8s.ContainerPort(container_port=80)]
                        )
                    ])
                )
            )
        )

        k8s.KubeService(self, "service",
            spec=k8s.ServiceSpec(
                type="LoadBalancer",
                ports=[k8s.ServicePort(port=80, target_port=k8s.IntOrString.from_number(80))],
                selector=label
            )
        )

app = App()
MyChart(app, "hello")
app.synth()
```

---

## 导入 CRD

### 导入 Kubernetes API

```bash
# 导入标准 K8s API
cdk8s import k8s

# 指定版本
cdk8s import k8s@1.28.0
```

### 导入自定义 CRD

```bash
# 从 URL 导入
cdk8s import https://raw.githubusercontent.com/cert-manager/cert-manager/master/deploy/crds/crd-certificates.yaml

# 从本地文件导入
cdk8s import ./crds/my-crd.yaml

# 从 Helm Chart 导入
cdk8s import helm:bitnami/redis
```

### 使用导入的 CRD

```typescript
import { Certificate } from './imports/cert-manager.io';

new Certificate(this, 'cert', {
  metadata: { name: 'my-cert' },
  spec: {
    secretName: 'my-cert-secret',
    issuerRef: {
      name: 'letsencrypt-prod',
      kind: 'ClusterIssuer'
    },
    dnsNames: ['example.com']
  }
});
```

---

## 自定义 Construct

### 创建可复用组件

```typescript
// constructs/web-service.ts
import { Construct } from 'constructs';
import { Chart } from 'cdk8s';
import * as kplus from 'cdk8s-plus-27';

export interface WebServiceProps {
  image: string;
  replicas?: number;
  port?: number;
  env?: { [key: string]: string };
}

export class WebService extends Construct {
  public readonly deployment: kplus.Deployment;
  public readonly service: kplus.Service;

  constructor(scope: Construct, id: string, props: WebServiceProps) {
    super(scope, id);

    const { image, replicas = 2, port = 80, env = {} } = props;

    // 创建环境变量
    const envVars: kplus.EnvValue[] = Object.entries(env).map(
      ([name, value]) => ({ name, value: kplus.EnvValue.fromValue(value) })
    );

    this.deployment = new kplus.Deployment(this, 'deployment', {
      replicas,
      containers: [{
        image,
        portNumber: port,
        envVariables: envVars.reduce((acc, { name, value }) => {
          acc[name] = value;
          return acc;
        }, {} as { [key: string]: kplus.EnvValue })
      }]
    });

    this.service = this.deployment.exposeViaService({
      ports: [{ port }]
    });
  }
}

// 使用
const web = new WebService(this, 'web', {
  image: 'nginx:latest',
  replicas: 3,
  port: 80,
  env: { 'NODE_ENV': 'production' }
});
```

---

## 测试

### 快照测试

```typescript
// test/main.test.ts
import { Testing } from 'cdk8s';
import { MyChart } from '../lib/main';

describe('MyChart', () => {
  test('snapshot', () => {
    const app = Testing.app();
    const chart = new MyChart(app, 'test');
    const results = Testing.synth(chart);
    expect(results).toMatchSnapshot();
  });
});
```

### 单元测试

```typescript
import { Testing } from 'cdk8s';
import { MyChart } from '../lib/main';

test('deployment has correct replicas', () => {
  const app = Testing.app();
  const chart = new MyChart(app, 'test');
  const results = Testing.synth(chart);
  
  const deployment = results.find(
    r => r.kind === 'Deployment' && r.metadata.name === 'deployment'
  );
  
  expect(deployment?.spec?.replicas).toBe(3);
});
```

---

## 构建和部署

```bash
# 生成 YAML
cdk8s synth

# 查看生成的文件
ls dist/

# 部署到集群
kubectl apply -f dist/

# 或使用管道
cdk8s synth -o - | kubectl apply -f -
```

---

## 与 Helm 集成

```typescript
import { Helm } from 'cdk8s';

// 使用 Helm Chart 作为 Construct
new Helm(this, 'nginx-ingress', {
  chart: 'ingress-nginx/ingress-nginx',
  version: '4.8.0',
  values: {
    controller: {
      replicaCount: 2,
      service: {
        type: 'LoadBalancer'
      }
    }
  }
});
```

---

## 最佳实践

1. **模块化**: 将复杂逻辑封装为 Construct
2. **类型安全**: 充分利用 TypeScript 类型检查
3. **测试覆盖**: 使用快照测试和单元测试
4. **版本管理**: 锁定 cdk8s 和 K8s API 版本
5. **复用组件**: 发布 Construct 库供团队使用
6. **CI/CD 集成**: 在管道中运行 synth 和测试

---

## 参考资源

- [官方文档](https://cdk8s.io/docs/latest/)
- [GitHub Repo](https://github.com/cdk8s-team/cdk8s)
- [cdk8s-plus](https://cdk8s.io/docs/latest/plus/)
- [Construct Hub](https://constructs.dev/search?q=cdk8s)
- [示例项目](https://github.com/cdk8s-team/cdk8s/tree/master/examples)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-orchestration|CNCF 编排与应用管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
