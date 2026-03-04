# Sermant

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://sermant.io/ |
| **GitHub** | https://github.com/sermant-io/Sermant |
| **许可证** | Apache-2.0 |
| **开发语言** | Java |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Sermant 是华为开源的基于 Java Agent 的无代理服务网格方案，通过 Java Instrumentation 机制（字节码增强）为 Java 微服务提供服务治理能力，无需修改应用代码或部署 Sidecar 代理。它支持流量路由、限流熔断、负载均衡、服务注册发现等功能，特别适合 Java 技术栈的微服务架构。

### 核心特性

- **无侵入**: 基于 Java Agent 字节码增强，无需修改业务代码
- **无 Sidecar**: 不需要额外的代理容器，零网络跳转延迟
- **流量治理**: 标签路由、灰度发布、流量染色
- **弹性治理**: 限流、熔断、重试、隔离
- **注册中心适配**: 支持在不同注册中心间透明迁移（如 Eureka → Nacos）
- **插件化**: 通过插件机制按需加载治理能力

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│              Sermant Backend                       │
│  ┌──────────────────────────────────────────┐     │
│  │  管理控制台 / 配置下发 / 心跳管理         │     │
│  └──────────────────┬───────────────────────┘     │
└─────────────────────┼─────────────────────────────┘
                      │ 配置下发
┌─────────────────────▼─────────────────────────────┐
│              Java Application (JVM)                 │
│                                                     │
│  ┌──────────────────────────────────────────┐      │
│  │           Sermant Agent                   │      │
│  │  (Java Agent / 字节码增强)                │      │
│  │                                            │      │
│  │  ┌─────────┐ ┌──────────┐ ┌──────────┐  │      │
│  │  │流量路由 │ │限流熔断  │ │负载均衡  │  │      │
│  │  │插件     │ │插件      │ │插件      │  │      │
│  │  └─────────┘ └──────────┘ └──────────┘  │      │
│  │  ┌─────────┐ ┌──────────┐ ┌──────────┐  │      │
│  │  │注册迁移 │ │标签路由  │ │监控上报  │  │      │
│  │  │插件     │ │插件      │ │插件      │  │      │
│  │  └─────────┘ └──────────┘ └──────────┘  │      │
│  └──────────────────────────────────────────┘      │
│                                                     │
│  ┌──────────────────────────────────────────┐      │
│  │        Spring Boot / Dubbo / gRPC         │      │
│  │        (业务应用 - 无需修改)               │      │
│  └──────────────────────────────────────────┘      │
└─────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 下载 Sermant
curl -LO https://github.com/sermant-io/Sermant/releases/latest/download/sermant-agent.tar.gz
tar xzf sermant-agent.tar.gz
```

### 挂载到 Java 应用

```bash
# 通过 -javaagent 参数挂载
java -javaagent:/path/to/sermant-agent/agent/sermant-agent.jar=appName=my-service \
  -jar my-application.jar

# Kubernetes 中通过 Init Container 注入
```

### Kubernetes 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-service
spec:
  template:
    spec:
      initContainers:
        - name: sermant-init
          image: sermant/sermant-agent:latest
          command: ['cp', '-r', '/sermant-agent', '/agent']
          volumeMounts:
            - name: agent
              mountPath: /agent
      containers:
        - name: app
          image: my-service:latest
          env:
            - name: JAVA_TOOL_OPTIONS
              value: "-javaagent:/agent/sermant-agent/agent/sermant-agent.jar=appName=my-service"
          volumeMounts:
            - name: agent
              mountPath: /agent
      volumes:
        - name: agent
          emptyDir: {}
```

### 配置流量路由

```yaml
# 标签路由规则
serviceName: my-service
matchRules:
  - headers:
      x-user-type:
        exact: vip
    route:
      - tags:
          version: v2
        weight: 100
  - route:
      - tags:
          version: v1
        weight: 80
      - tags:
          version: v2
        weight: 20
```

### 配置限流熔断

```yaml
# 限流规则
serviceName: my-service
rateLimitRules:
  - apiPath: /api/orders
    rate: 100        # 每秒 100 次
  - apiPath: /api/search
    rate: 500

# 熔断规则
circuitBreakerRules:
  - apiPath: /api/payment
    failureRateThreshold: 50      # 失败率 50% 触发熔断
    slowCallRateThreshold: 80      # 慢调用率 80% 触发
    slowCallDurationThreshold: 3000 # 3秒视为慢调用
    waitDurationInOpenState: 30000  # 熔断 30 秒后半开
```

---

## 与其他方案对比

| 特性 | Sermant | Istio (Sidecar) | Kmesh (eBPF) | Spring Cloud |
|:---|:---|:---|:---|:---|
| 实现方式 | Java Agent | Sidecar 代理 | 内核 eBPF | SDK 集成 |
| 语言限制 | Java 仅 | 任意语言 | 任意语言 | Java 仅 |
| 额外延迟 | ~0 (进程内) | ~1-3ms | ~0.1ms | ~0 (进程内) |
| 代码侵入 | 无 | 无 | 无 | 高 |
| 资源开销 | 低 (JVM 内) | 中 (Sidecar) | 低 (内核) | 无额外开销 |
| 适用场景 | Java 微服务 | 多语言 | 多语言 | Java 开发 |

---

## 最佳实践

1. **插件按需加载**: 只启用需要的插件，减少 Agent 对应用启动时间的影响
2. **灰度验证**: 先在测试环境挂载 Agent，验证字节码增强不影响业务逻辑
3. **版本兼容**: 确认 Sermant 版本与目标框架版本（Spring Boot/Dubbo）兼容
4. **配置热更新**: 利用 Sermant Backend 实现运行时动态调整治理策略
5. **监控集成**: 开启监控插件将治理指标上报到 Prometheus

---

## 参考资源

- [Sermant 官方文档](https://sermant.io/docs/)
- [Sermant GitHub](https://github.com/sermant-io/Sermant)
- [Sermant 插件列表](https://sermant.io/docs/plugin/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
