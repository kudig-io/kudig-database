---
title: "Flagger + KEDA 渐进式交付与 Dagger.io 便携 CI/CD"
description: "2025 年渐进式交付最佳实践：Flagger 自动化金丝雀发布结合 KEDA 事件驱动自动扩缩；Dagger.io v0.12+ 便携 CI/CD 管道设计与 K8s 集成"
summary: "深入讲解 Flagger 1.38+ 与 KEDA 2.14 联合使用实现基于 Prometheus 指标的智能金丝雀发布、A/B 测试和蓝绿部署；Dagger.io 核心概念（Pipeline/Container/Service）、CUE/Python/Go SDK、在 GitHub Actions/GitLab CI 中嵌入使用，以及 Dagger Cloud 企业功能"
category: gitops-ci-cd
tags:
- flagger
- keda
- progressive-delivery
- canary
- blue-green
- dagger
- ci-cd
- pipeline-as-code
- event-driven
- autoscaling
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
reading_level: advanced
audience:
- DevOps 工程师
- SRE
- 平台工程师
estimated_read_time: 22min
intent_queries:
- "Flagger 如何结合 KEDA 做自动化金丝雀发布"
- "Dagger.io 如何替代 Jenkins 流水线"
- "渐进式交付 2025 最佳实践"
- "KEDA 如何基于 Prometheus 指标扩缩"
trigger_keywords:
- Flagger
- KEDA
- 金丝雀发布
- Dagger
- 渐进式交付
- Pipeline as Code
prerequisites:
- kubernetes-basics
- helm-basics
- prometheus-basics
sources:
- https://flagger.app/
- https://keda.sh/
- https://dagger.io/
- https://github.com/fluxcd/flagger
- https://github.com/kedacore/keda
---

# Flagger + KEDA 渐进式交付与 Dagger.io 便携 CI/CD

> 2025 年渐进式交付的核心进展：Flagger 与 KEDA 深度联动实现"基于实际负载的智能晋级"，Dagger.io 让 CI/CD 管道真正可移植。

## Flagger 1.38+ 与 KEDA 联合使用

### 架构概述

```
传统金丝雀发布：
发布 → 固定流量比例 → 等待固定时间 → 晋级/回滚

Flagger + KEDA 智能发布：
发布 → KEDA 基于事件扩缩流量副本 → Flagger 分析真实指标
     → 自适应流量权重调整 → 基于 SLO 指标晋级/回滚
```

### 安装

```bash
# 安装 Flagger（支持 Istio/Nginx/Contour/Linkerd）
helm repo add flagger https://flagger.app
helm repo update

helm upgrade -i flagger flagger/flagger \
  --namespace=istio-system \
  --set crd.create=true \
  --set meshProvider=istio \
  --set metricsServer=http://prometheus.monitoring:9090 \
  --set slack.url=${SLACK_WEBHOOK} \
  --set slack.channel="#deployments"

# 安装 KEDA
helm repo add kedacore https://kedacore.github.io/charts
helm upgrade -i keda kedacore/keda \
  --namespace keda \
  --create-namespace \
  --version 2.14.0
```

### Flagger Canary 配置（生产级）

```yaml
# 基于 Prometheus 指标的自动化金丝雀发布
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: payment-service
  namespace: production
spec:
  # 目标 Deployment
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: payment-service
  # 进度截止时间
  progressDeadlineSeconds: 600
  # HPA（金丝雀版本也受 HPA 控制）
  autoscalerRef:
    apiVersion: autoscaling/v2
    kind: HorizontalPodAutoscaler
    name: payment-service
  service:
    port: 80
    targetPort: 8080
    # Istio 流量策略
    trafficPolicy:
      connectionPool:
        http:
          http2MaxRequests: 1000
          http1MaxPendingRequests: 1000
      outlierDetection:
        consecutive5xxErrors: 5
        interval: 30s
        baseEjectionTime: 30s
  # 金丝雀分析配置
  analysis:
    # 每分钟推进一次分析
    interval: 1m
    # 晋级前需连续通过的分析次数
    threshold: 5
    # 最大失败分析次数（超过则回滚）
    maxWeight: 50          # 最大流量权重 50%
    stepWeight: 10         # 每次晋级步进 10%
    # 指标分析
    metrics:
    # 成功率 > 99%
    - name: request-success-rate
      thresholdRange:
        min: 99
      interval: 1m
    # P99 延迟 < 500ms
    - name: request-duration
      thresholdRange:
        max: 500
      interval: 30s
    # 自定义指标：业务成功率
    - name: payment-success-rate
      templateRef:
        name: payment-success-rate
        namespace: production
      thresholdRange:
        min: 98.5
      interval: 2m
    # 预置测试（smoke test）
    webhooks:
    - name: smoke-test
      type: pre-rollout
      url: http://flagger-loadtester.flagger-system/
      timeout: 30s
      metadata:
        type: bash
        cmd: "curl -sd 'test' http://payment-service-canary.production/health | grep OK"
    - name: load-test
      type: rollout
      url: http://flagger-loadtester.flagger-system/
      metadata:
        type: cmd
        cmd: "hey -z 2m -q 10 -c 2 http://payment-service-canary.production/api/ping"
        logCmdOutput: "true"
    # 发布完成通知
    - name: notify-slack
      type: confirm-rollout
      url: ${SLACK_WEBHOOK}
      metadata:
        type: slack
        channel: "#deployments"
```

### 自定义 MetricTemplate

```yaml
# 自定义 Prometheus 查询指标
apiVersion: flagger.app/v1beta1
kind: MetricTemplate
metadata:
  name: payment-success-rate
  namespace: production
spec:
  provider:
    type: prometheus
    address: http://prometheus.monitoring:9090
  query: |
    sum(
      rate(
        payment_transactions_total{
          status="success",
          pod=~"{{ target }}-[0-9a-zA-Z]+(-[0-9a-zA-Z]+)"
        }[{{ interval }}]
      )
    ) /
    sum(
      rate(
        payment_transactions_total{
          pod=~"{{ target }}-[0-9a-zA-Z]+(-[0-9a-zA-Z]+)"
        }[{{ interval }}]
      )
    ) * 100
---
# 自定义 Datadog 指标（适用于 Datadog 用户）
apiVersion: flagger.app/v1beta1
kind: MetricTemplate
metadata:
  name: error-rate-datadog
  namespace: production
spec:
  provider:
    type: datadog
    address: https://api.datadoghq.com
    secretRef:
      name: datadog-secret
  query: |
    sum:trace.servlet.request.errors{env:production,service:{{ target }}}
    / sum:trace.servlet.request.hits{env:production,service:{{ target }}} * 100
```

### KEDA ScaledObject 与 Flagger 联动

```yaml
# KEDA 基于 Prometheus 指标自动扩缩（与 Flagger 联用）
apiVersion: keda.sh/v1alpha1
kind: ScaledObject
metadata:
  name: payment-service
  namespace: production
  # KEDA 2.14：Flagger 识别并同步到金丝雀版本
  annotations:
    flagger.app/scale-stable: "true"
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: payment-service
  pollingInterval: 15
  cooldownPeriod: 60
  minReplicaCount: 2
  maxReplicaCount: 20
  advanced:
    restoreToOriginalReplicaCount: true
    scalingModifiers:
      formula: "payment_qps_scaler + burst_scaler"
      target: "10"
      activationTarget: "5"
  triggers:
  # 基于 Prometheus QPS
  - type: prometheus
    metadata:
      serverAddress: http://prometheus.monitoring:9090
      metricName: payment_requests_per_second
      query: |
        sum(rate(http_requests_total{
          service="payment-service",
          method="POST"
        }[2m]))
      threshold: "100"        # 每 100 QPS 扩 1 个副本
      activationThreshold: "10"
  # 基于 RabbitMQ 队列深度
  - type: rabbitmq
    metadata:
      host: amqp://rabbitmq.messaging.svc.cluster.local:5672
      queueName: payment-queue
      queueLength: "100"
      protocol: amqp
  # 基于 Kafka Lag
  - type: kafka
    metadata:
      bootstrapServers: kafka.messaging.svc.cluster.local:9092
      consumerGroup: payment-consumer
      topic: payment-events
      lagThreshold: "50"
      offsetResetPolicy: latest
```

### A/B 测试与蓝绿部署

```yaml
# A/B 测试：基于 HTTP Header 路由
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: recommendation-service
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: recommendation-service
  analysis:
    interval: 1m
    threshold: 10
    iterations: 10
    match:
    - headers:
        x-user-segment:
          regex: "^(beta|experiment).*"   # 只有实验用户看到新版本
    metrics:
    - name: recommendation-ctr           # 点击率分析
      templateRef:
        name: recommendation-ctr
      thresholdRange:
        min: 2.5                         # CTR 必须 > 2.5%
---
# 蓝绿部署：零流量切换
apiVersion: flagger.app/v1beta1
kind: Canary
metadata:
  name: checkout-service
  namespace: production
spec:
  targetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: checkout-service
  analysis:
    interval: 30s
    threshold: 3
    iterations: 5
    # 蓝绿：新版本接收 0% 流量直到分析通过，然后一次性切换到 100%
    stepWeight: 0
    maxWeight: 0
    metrics:
    - name: request-success-rate
      thresholdRange:
        min: 99.9
    webhooks:
    - name: integration-test
      type: pre-rollout
      url: http://flagger-loadtester/
      metadata:
        cmd: "pytest /tests/integration/checkout_test.py -v"
```

---

## Dagger.io：便携 CI/CD 管道

### 核心理念

```
传统 CI/CD 问题：
• YAML 地狱（Jenkins Groovy/GitHub Actions YAML）
• 本地无法复现 CI 环境
• CI 平台锁定（GitHub Actions → GitLab CI 迁移成本高）
• 调试困难（必须推送到 CI 才能测试）

Dagger 解决方案：
• 管道即代码（Python/Go/TypeScript SDK）
• 本地与 CI 运行完全一致（都在容器中）
• 平台无关（任何 CI 都能运行 dagger run）
• 即时本地调试
```

### Dagger Python SDK 实践

```python
# pipeline.py - 完整 CI/CD 管道
import dagger
import sys
from pathlib import Path


async def main():
    async with dagger.Connection(dagger.Config(log_output=sys.stderr)) as client:
        # 获取源码
        source = (
            client.host()
            .directory(".", exclude=["node_modules/", ".git/", "dist/"])
        )

        # 构建 Docker 镜像
        image = await build(client, source)

        # 运行测试
        await test(client, source)

        # 安全扫描
        await security_scan(client, image)

        # 推送镜像（仅 main 分支）
        if await is_main_branch(client):
            await push(client, image)
            await deploy_to_staging(client, image)


async def build(client: dagger.Client, source: dagger.Directory) -> dagger.Container:
    """构建应用镜像"""
    return (
        client.container()
        .from_("python:3.11-slim")
        .with_directory("/app", source)
        .with_workdir("/app")
        .with_exec(["pip", "install", "-r", "requirements.txt"])
        .with_exec(["pip", "install", "-r", "requirements-dev.txt"])
    )


async def test(client: dagger.Client, source: dagger.Directory) -> None:
    """运行测试套件"""
    # 启动 PostgreSQL 作为测试依赖
    postgres = (
        client.container()
        .from_("postgres:16-alpine")
        .with_env_variable("POSTGRES_PASSWORD", "test")
        .with_env_variable("POSTGRES_DB", "testdb")
        .with_exposed_port(5432)
        .as_service()
    )

    result = await (
        client.container()
        .from_("python:3.11-slim")
        .with_service_binding("postgres", postgres)
        .with_directory("/app", source)
        .with_workdir("/app")
        .with_exec(["pip", "install", "-r", "requirements-dev.txt"])
        .with_env_variable("DATABASE_URL", "postgresql://postgres:test@postgres/testdb")
        .with_exec([
            "pytest", "tests/",
            "-v",
            "--cov=app",
            "--cov-report=xml",
            "--cov-fail-under=80",
        ])
        .stdout()
    )
    print(f"Test output:\n{result}")


async def security_scan(client: dagger.Client, image: dagger.Container) -> None:
    """Trivy 安全扫描"""
    await (
        client.container()
        .from_("aquasec/trivy:latest")
        .with_mounted_file("/tmp/image.tar",
            await image.export("/tmp/image.tar"))
        .with_exec([
            "trivy", "image",
            "--input", "/tmp/image.tar",
            "--exit-code", "1",
            "--severity", "HIGH,CRITICAL",
            "--ignore-unfixed",
        ])
        .stdout()
    )


async def push(client: dagger.Client, image: dagger.Container) -> str:
    """推送镜像到 OCI 注册表"""
    registry_secret = client.set_secret(
        "registry-password",
        await client.host().env_variable("REGISTRY_PASSWORD").value()
    )

    return await (
        image
        .with_registry_auth(
            "ghcr.io",
            "my-company-bot",
            registry_secret
        )
        .publish("ghcr.io/my-company/my-app:latest")
    )


if __name__ == "__main__":
    import asyncio
    asyncio.run(main())
```

### 在 GitHub Actions 中使用 Dagger

```yaml
# .github/workflows/ci.yml
name: CI/CD

on:
  push:
    branches: [main]
  pull_request:

jobs:
  pipeline:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v4

    - name: Run Dagger Pipeline
      uses: dagger/dagger-for-github@v6
      with:
        version: "0.12.0"
        verb: call                   # 或 run
        module: github.com/my-company/ci-module@main
        args: >
          build
          --source=.
          --push=${{ github.ref == 'refs/heads/main' }}
      env:
        REGISTRY_PASSWORD: ${{ secrets.REGISTRY_PASSWORD }}
        GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}
```

### Dagger 模块化（可复用管道）

```typescript
// ci/src/index.ts - 可在任何项目中复用的 Dagger 模块
import { dag, Container, Directory, object, func } from "@dagger.io/dagger";

@object()
export class MyCiModule {
  /**
   * 构建并推送 Docker 镜像
   * 使用方式: dagger call build-and-push --source=. --tag=v1.0.0
   */
  @func()
  async buildAndPush(
    source: Directory,
    tag: string,
    registry: string = "ghcr.io/my-company",
  ): Promise<string> {
    const image = await dag
      .container()
      .build(source)
      .publish(`${registry}/my-app:${tag}`);

    return image;
  }

  /**
   * 运行完整测试套件（含集成测试）
   */
  @func()
  async test(source: Directory): Promise<string> {
    const postgres = dag
      .container()
      .from("postgres:16-alpine")
      .withEnvVariable("POSTGRES_PASSWORD", "test")
      .asService();

    return dag
      .container()
      .from("python:3.11-slim")
      .withServiceBinding("postgres", postgres)
      .withDirectory("/app", source)
      .withWorkdir("/app")
      .withExec(["pip", "install", "-r", "requirements.txt"])
      .withExec(["pytest", "tests/", "-v", "--cov=app"])
      .stdout();
  }

  /**
   * 部署到 Kubernetes（使用 kubectl/Helm）
   */
  @func()
  async deploy(
    kubeconfig: Secret,
    imageTag: string,
    namespace: string = "production",
  ): Promise<string> {
    return dag
      .container()
      .from("bitnami/kubectl:latest")
      .withMountedSecret("/root/.kube/config", kubeconfig)
      .withExec([
        "helm", "upgrade", "--install", "my-app", "./helm/my-app",
        "--namespace", namespace,
        "--set", `image.tag=${imageTag}`,
        "--wait", "--timeout", "5m",
      ])
      .stdout();
  }
}
```

### Dagger Cloud 企业特性（2025）

| 特性 | 说明 |
|------|------|
| 缓存共享 | 跨 CI Runner 共享 Dagger 层缓存，节省 70%+ 构建时间 |
| 可观测性 | 每次管道运行的详细 DAG 可视化 |
| 秘钥管理 | 集中管理 Pipeline 使用的 Secret |
| Module Registry | 私有 Dagger 模块仓库 |
| Dagger Engine on K8s | Runner 直接跑在 K8s 上，弹性扩缩 |

```yaml
# Dagger Engine 部署到 K8s（自托管）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: dagger-engine
  namespace: dagger
spec:
  replicas: 3
  template:
    spec:
      containers:
      - name: dagger-engine
        image: registry.dagger.io/engine:v0.12.0
        securityContext:
          privileged: true           # Dagger 需要特权模式运行 containerd
        resources:
          requests:
            cpu: "4"
            memory: "16Gi"
            ephemeral-storage: "100Gi"
          limits:
            cpu: "8"
            memory: "32Gi"
            ephemeral-storage: "200Gi"
        volumeMounts:
        - name: dagger-cache
          mountPath: /var/lib/dagger
      volumes:
      - name: dagger-cache
        persistentVolumeClaim:
          claimName: dagger-cache-pvc
```

---

## 渐进式交付选型建议

| 场景 | 推荐方案 | 关键配置 |
|------|---------|---------|
| 高流量微服务 | Flagger + Istio + KEDA | 基于 RPS 指标晋级 |
| 无状态 API | Flagger + Nginx + Prometheus | 成功率 + 延迟双指标 |
| 机器学习模型 | Flagger + KServe | 基于业务指标（准确率）晋级 |
| 数据库变更 | Flyway + Liquibase（单独管理） | 不适用 Flagger |
| CI/CD 标准化 | Dagger.io | 统一多语言项目管道 |
| 预览环境 | Argo CD ApplicationSet + PR Generator | 每 PR 一个环境 |

---

## 参考资源

- [Flagger 官方文档](https://flagger.app/)
- [KEDA 文档](https://keda.sh/docs/)
- [Dagger 文档](https://docs.dagger.io/)
- [Dagger Python SDK](https://dagger-io.readthedocs.io/)
- [渐进式交付 CNCF 白皮书](https://github.com/cncf/tag-app-delivery/blob/main/progressive-delivery-whitepaper/progressive-delivery-whitepaper.md)
