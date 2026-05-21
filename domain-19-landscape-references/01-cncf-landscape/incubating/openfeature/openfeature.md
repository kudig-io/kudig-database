---
title: OpenFeature
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- OpenFeature 是什么
- 如何 OpenFeature
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- OpenFeature
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- observability-basics
---

title: OpenFeature
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- OpenFeature 是什么
- 如何 OpenFeature
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- OpenFeature
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

# OpenFeature

> **成熟度**: Incubating | **加入时间**: 2022-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://openfeature.dev |
| **GitHub** | https://github.com/open-feature |
| **许可证** | Apache-2.0 |
| **主要语言** | TypeScript, Go, Java, Python |
| **CNCF 分类** | App Definition & Feature Management |

---

## 项目概述

OpenFeature 是特性标志（Feature Flag）的开放标准，提供供应商无关的统一 API 和多语言 SDK。它允许开发者在不更换代码的情况下切换不同的特性标志提供商，实现渐进式发布、A/B 测试和功能开关。

## 核心特性

- **供应商无关**: 统一 API 支持多种后端提供商
- **多语言 SDK**: JavaScript, Go, Java, Python, .NET, PHP 等
- **Hooks 机制**: 在标志评估前后执行自定义逻辑
- **上下文支持**: 基于用户、环境等上下文进行评估
- **类型安全**: 支持布尔、字符串、数字、对象类型标志
- **可观测性**: 与追踪、日志系统集成

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                   OpenFeature Architecture                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Application Code                        │ │
│  │                                                            │ │
│  │  ┌──────────────────────────────────────────────────────┐ │ │
│  │  │              OpenFeature Client                      │ │ │
│  │  │  client.getBooleanValue("new-feature", false, ctx)  │ │ │
│  │  └──────────────────────────────────────────────────────┘ │ │
│  └────────────────────────────┬───────────────────────────────┘ │
│                               │                                  │
│                               ▼                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   OpenFeature API                          │ │
│  │                                                            │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │ │
│  │  │  Evaluation │  │   Hooks     │  │   Events        │   │ │
│  │  │    API      │  │   System    │  │   System        │   │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘   │ │
│  └────────────────────────────┬───────────────────────────────┘ │
│                               │                                  │
│                               ▼                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                     Provider Interface                     │ │
│  └────────────────────────────┬───────────────────────────────┘ │
│                               │                                  │
│       ┌───────────────────────┼───────────────────────────┐     │
│       ▼                       ▼                           ▼     │
│  ┌──────────┐          ┌──────────┐              ┌──────────┐  │
│  │LaunchDarkly│          │ Flagsmith│              │  flagd   │  │
│  │ Provider │          │ Provider │              │ Provider │  │
│  └──────────┘          └──────────┘              └──────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### JavaScript/TypeScript

```bash
npm install @openfeature/server-sdk
npm install @openfeature/flagd-provider  # 使用 flagd 提供商
```

```typescript
import { OpenFeature } from '@openfeature/server-sdk';
import { FlagdProvider } from '@openfeature/flagd-provider';

// 配置提供商
OpenFeature.setProvider(new FlagdProvider());

// 获取客户端
const client = OpenFeature.getClient();

// 评估特性标志
const showNewFeature = await client.getBooleanValue('new-checkout', false, {
  targetingKey: 'user-123',
  email: 'user@example.com',
  tier: 'premium'
});

if (showNewFeature) {
  // 显示新功能
}
```

### Go

```go
package main

import (
    "context"
    "github.com/open-feature/go-sdk/pkg/openfeature"
    flagd "github.com/open-feature/go-sdk-contrib/providers/flagd/pkg"
)

func main() {
    // 配置提供商
    provider := flagd.NewProvider()
    openfeature.SetProvider(provider)
    
    // 获取客户端
    client := openfeature.NewClient("my-app")
    
    // 评估上下文
    evalCtx := openfeature.NewEvaluationContext(
        "user-123",
        map[string]interface{}{
            "email": "user@example.com",
            "tier":  "premium",
        },
    )
    
    // 评估标志
    value, _ := client.BooleanValue(
        context.Background(),
        "new-checkout",
        false,
        evalCtx,
    )
    
    if value {
        // 显示新功能
    }
}
```

### Java

```java
import dev.openfeature.sdk.*;
import dev.openfeature.contrib.providers.flagd.*;

public class App {
    public static void main(String[] args) {
        // 配置提供商
        OpenFeatureAPI api = OpenFeatureAPI.getInstance();
        api.setProvider(new FlagdProvider());
        
        // 获取客户端
        Client client = api.getClient();
        
        // 构建上下文
        EvaluationContext ctx = new ImmutableContext("user-123",
            new HashMap<String, Value>() {{
                put("email", new Value("user@example.com"));
                put("tier", new Value("premium"));
            }});
        
        // 评估标志
        boolean showNewFeature = client.getBooleanValue("new-checkout", false, ctx);
    }
}
```

---

## flagd 提供商

flagd 是 OpenFeature 的轻量级特性标志后端。

### 部署 flagd

```yaml
# flagd-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: flagd
spec:
  replicas: 1
  selector:
    matchLabels:
      app: flagd
  template:
    metadata:
      labels:
        app: flagd
    spec:
      containers:
        - name: flagd
          image: ghcr.io/open-feature/flagd:latest
          args:
            - start
            - --uri
            - file:./flags.json
          ports:
            - containerPort: 8013
          volumeMounts:
            - name: flags
              mountPath: /flags.json
              subPath: flags.json
      volumes:
        - name: flags
          configMap:
            name: flag-config
```

### 标志配置

```json
{
  "flags": {
    "new-checkout": {
      "state": "ENABLED",
      "variants": {
        "on": true,
        "off": false
      },
      "defaultVariant": "off",
      "targeting": {
        "if": [
          {
            "in": ["premium", { "var": "tier" }]
          },
          "on",
          "off"
        ]
      }
    },
    "banner-color": {
      "state": "ENABLED",
      "variants": {
        "blue": "#0000FF",
        "green": "#00FF00",
        "red": "#FF0000"
      },
      "defaultVariant": "blue",
      "targeting": {
        "fractional": [
          ["blue", 50],
          ["green", 25],
          ["red", 25]
        ]
      }
    }
  }
}
```

---

## Hooks 机制

```typescript
// 日志 Hook
const loggingHook: Hook = {
  before: (hookContext) => {
    console.log(`Evaluating flag: ${hookContext.flagKey}`);
  },
  after: (hookContext, evaluationDetails) => {
    console.log(`Flag ${hookContext.flagKey} = ${evaluationDetails.value}`);
  },
  error: (hookContext, err) => {
    console.error(`Error evaluating ${hookContext.flagKey}: ${err}`);
  }
};

// 注册全局 Hook
OpenFeature.addHooks(loggingHook);

// 或注册到特定客户端
client.addHooks(loggingHook);
```

### OpenTelemetry 集成 Hook

```typescript
import { TracingHook } from '@openfeature/open-telemetry-hooks';

OpenFeature.addHooks(new TracingHook());
```

---

## Kubernetes Operator

```yaml
# 安装 OpenFeature Operator
kubectl apply -f https://github.com/open-feature/open-feature-operator/releases/latest/download/release.yaml

# FeatureFlagConfiguration CRD
apiVersion: core.openfeature.dev/v1alpha1
kind: FeatureFlagConfiguration
metadata:
  name: my-flags
spec:
  featureFlagSpec:
    flags:
      new-checkout:
        state: ENABLED
        variants:
          "on": true
          "off": false
        defaultVariant: "off"
```

---

## 最佳实践

1. **命名规范**: 使用清晰的标志名称如 `feature-name-enabled`
2. **默认值**: 总是提供合理的默认值
3. **上下文丰富**: 传递足够的上下文信息支持精准定向
4. **清理旧标志**: 定期删除不再使用的特性标志
5. **监控集成**: 使用 Hooks 与可观测性系统集成

---

## 参考资源

- [官方文档](https://openfeature.dev/docs)
- [GitHub Repo](https://github.com/open-feature)
- [flagd 文档](https://flagd.dev)
- [规范文档](https://openfeature.dev/specification)
- [提供商列表](https://openfeature.dev/ecosystem)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/openfeature.md|openfeature]]
- [[entities/cncf-infrastructure|CNCF 基础设施与混沌工程项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
