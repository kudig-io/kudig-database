---
title: Kubernetes Testing Strategy & Quality Gates
description: K8s 测试策略与质量门禁 — 测试金字塔、E2E 测试框架、CI/CD 质量门禁、契约测试、性能测试
summary: 云原生应用的全面测试策略，涵盖单元测试到生产验证的完整质量保障体系
category: practice
tags:
- testing
- quality-gates
- e2e
- contract-testing
- ci-cd
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: release
---
# Kubernetes 测试策略与质量门禁

> 构建从代码到生产的全面质量保障体系。

## 测试金字塔（云原生版）

```
            ┌─────────┐
            │  E2E    │  ← 少量关键路径
           ┌┴─────────┴┐
           │ 集成测试   │  ← 服务间交互
          ┌┴───────────┴┐
          │  契约测试    │  ← API 兼容性
         ┌┴─────────────┴┐
         │   单元测试     │  ← 大量、快速
         └───────────────┘
```

## 各层测试实践

### 单元测试

```go
// Go 单元测试示例（handler 逻辑）
func TestHandleOrder(t *testing.T) {
    tests := []struct {
        name     string
        order    Order
        wantCode int
    }{
        {"valid order", Order{Item: "A", Qty: 1}, 201},
        {"zero quantity", Order{Item: "A", Qty: 0}, 400},
        {"missing item", Order{Qty: 1}, 400},
    }
    for _, tt := range tests {
        t.Run(tt.name, func(t *testing.T) {
            w := httptest.NewRecorder()
            HandleOrder(w, createRequest(tt.order))
            if w.Code != tt.wantCode {
                t.Errorf("got %d, want %d", w.Code, tt.wantCode)
            }
        })
    }
}
```

### 集成测试（Testcontainers）

```yaml
# docker-compose.test.yml
services:
  app:
    build: .
    environment:
      - DATABASE_URL=postgres://test:test@db:5432/test
      - REDIS_URL=redis://cache:6379
    depends_on:
      db:
        condition: service_healthy
  db:
    image: postgres:16
    environment:
      POSTGRES_PASSWORD: test
    healthcheck:
      test: ["CMD-SHELL", "pg_isready"]
      interval: 2s
      retries: 5
  cache:
    image: redis:7-alpine
```

### K8s 集成测试（envtest / kind）

```go
// 使用 envtest 测试 Controller
func TestReconcile(t *testing.T) {
    testEnv := &envtest.Environment{
        CRDDirectoryPaths: []string{"../config/crd/bases"},
    }
    cfg, err := testEnv.Start()
    require.NoError(t, err)
    defer testEnv.Stop()

    k8sClient, _ := client.New(cfg, client.Options{})
    
    // 创建测试资源
    app := &v1alpha1.MyApp{
        ObjectMeta: metav1.ObjectMeta{Name: "test-app"},
        Spec: v1alpha1.MyAppSpec{Replicas: 3},
    }
    err = k8sClient.Create(ctx, app)
    require.NoError(t, err)
    
    // 验证 Reconcile 结果
    // ...
}
```

### E2E 测试框架对比

| 框架 | 语言 | 特点 | 适用 |
|------|------|------|------|
| kuttl | YAML | 声明式、K8s 原生 | Operator 测试 |
| Ginkgo/Gomega | Go | BDD 风格、K8s 官方 | 控制器测试 |
| Cypress/Playwright | JS/TS | 浏览器 E2E | 前端集成 |
| k6 | JS | 性能/负载测试 | API 压测 |
| Litmus | YAML | 混沌 + E2E | 韧性验证 |

### kuttl 测试示例

```yaml
# kuttl-test.yaml
apiVersion: kuttl.dev/v1beta1
kind: TestSuite
testDirs:
  - ./tests/e2e
startKIND: true
kindConfig: ./kind-config.yaml
timeout: 300
---
# tests/e2e/00-install/00-assert.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
status:
  readyReplicas: 3
---
# tests/e2e/01-scale/00-scale.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
spec:
  replicas: 5
---
# tests/e2e/01-scale/00-assert.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app
status:
  readyReplicas: 5
```

## 契约测试（Contract Testing）

### Pact 消费者驱动契约

```javascript
// 消费者测试
const { Pact } = require('@pact-foundation/pact');

describe('Order API Consumer', () => {
  const provider = new Pact({
    consumer: 'order-frontend',
    provider: 'order-service',
  });

  it('creates an order', async () => {
    await provider.addInteraction({
      state: 'order service is available',
      uponReceiving: 'a request to create order',
      withRequest: {
        method: 'POST',
        path: '/api/orders',
        body: { item: 'widget', quantity: 2 },
      },
      willRespondWith: {
        status: 201,
        body: { id: expect.stringMatching(/^ord-/), status: 'created' },
      },
    });
    // 执行实际调用验证
  });
});
```

## CI/CD 质量门禁

### 管道阶段与门禁

```yaml
# GitHub Actions 质量门禁
name: Quality Gates
on: [pull_request]

jobs:
  unit-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: go test ./... -coverprofile=coverage.out
      - name: Check Coverage
        run: |
          COVERAGE=$(go tool cover -func=coverage.out | grep total | awk '{print $3}' | tr -d '%')
          if (( $(echo "$COVERAGE < 80" | bc -l) )); then
            echo "Coverage $COVERAGE% < 80% threshold"
            exit 1
          fi

  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: golangci/golangci-lint-action@v4
        with:
          version: latest

  security-scan:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Trivy Image Scan
        uses: aquasecurity/trivy-action@master
        with:
          image-ref: 'app:${{ github.sha }}'
          severity: 'CRITICAL,HIGH'
          exit-code: '1'

  e2e-test:
    needs: [unit-test, lint, security-scan]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Setup kind
        uses: helm/kind-action@v1
      - name: Run kuttl tests
        run: kubectl kuttl test --config kuttl-test.yaml
```

### 质量门禁矩阵

| 门禁 | 阈值 | 阻断级别 |
|------|------|----------|
| 单元测试覆盖率 | ≥ 80% | 强制 |
| 静态分析 | 0 Critical/High | 强制 |
| 镜像漏洞扫描 | 0 Critical | 强制 |
| 集成测试通过率 | 100% | 强制 |
| E2E 测试通过率 | ≥ 95% | 强制 |
| 性能回归 | ≤ 10% 退化 | 警告 |
| 契约测试 | 全部通过 | 强制 |

## 性能测试

### k6 负载测试

```javascript
import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  stages: [
    { duration: '1m', target: 50 },   // 预热
    { duration: '3m', target: 200 },  // 负载
    { duration: '1m', target: 500 },  // 峰值
    { duration: '2m', target: 0 },    // 冷却
  ],
  thresholds: {
    http_req_duration: ['p(95)<200', 'p(99)<500'],
    http_req_failed: ['rate<0.01'],
  },
};

export default function () {
  const res = http.get('http://api-service/api/products');
  check(res, {
    'status is 200': (r) => r.status === 200,
    'response time < 200ms': (r) => r.timings.duration < 200,
  });
  sleep(1);
}
```

## 测试环境管理

| 环境 | 用途 | 数据 | 生命周期 |
|------|------|------|----------|
| local | 开发自测 | Mock/Seed | 按需 |
| CI | 自动化测试 | 测试数据集 | PR 级别 |
| staging | 预发布验证 | 脱敏生产数据 | 持久 |
| canary | 生产验证 | 真实流量(小比例) | 发布期间 |

## 最佳实践

1. **Shift-Left**：尽早发现问题（本地 lint + 预提交钩子）
2. **测试即代码**：测试与源码同仓库、同版本管理
3. **快速反馈**：单元测试 < 5min，集成测试 < 15min
4. **环境一致性**：使用容器确保测试环境可复现
5. **Flaky 测试零容忍**：不稳定测试立即修复或隔离
6. **生产验证**：金丝雀分析 + 合成监控确认发布质量

## 故障排查表

| 问题现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|--------|
| CI 测试超时 | 资源不足或依赖服务不可用 | `kubectl get pods -n ci --field-selector=status.phase!=Running` | 增加 runner 资源，添加依赖健康检查 |
| Flaky 测试反复失败 | 时序依赖/共享状态/网络抨动 | 查看测试日志中的随机失败模式 | 隔离到 quarantine suite，限期修复 |
| 覆盖率门禁阻塞 PR | 新增代码未写测试 | `go test -coverprofile=cover.out && go tool cover -func=cover.out` | 补充单元测试，或调整门禁阈值 |
| 集成测试环境不稳定 | 共享环境被其他 PR 干扰 | `kubectl get ns -l test-env=true` | 使用临时 Namespace 或独立集群 |
| 金丝雀分析误报 | 指标采集窗口太短或基线异常 | `kubectl get analysisrun -o yaml` | 增加分析窗口，确认基线健康 |
| 负载测试数据污染生产 | 测试数据库未隔离 | 检查 DB 连接串配置 | 严格环境隔离，测试使用独立实例 |

## 质量门禁流水线示例

```yaml
# .github/workflows/quality-gates.yml
name: Quality Gates
on: [pull_request]
jobs:
  lint:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: make lint
  unit-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: make test-unit
      - name: Coverage Gate
        run: |
          COV=$(go tool cover -func=cover.out | grep total | awk '{print $3}' | tr -d '%')
          [ $(echo "$COV >= 80" | bc) -eq 1 ] || exit 1
  integration-test:
    needs: [lint, unit-test]
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - run: make test-integration
```

## 相关工具

| 工具 | 用途 | 场景 |
|------|------|------|
| kubeconform | K8s YAML Schema 验证 | PR 中检查清单合法性 |
| kube-score | 静态分析 K8s 对象 | 发现资源配置缺陷 |
| SonarQube | 代码质量门禁 | 覆盖率/复杂度/安全漏洞 |
| k6 / Locust | 负载测试 | 发布前性能基线验证 |
| Litmus / Chaos Mesh | 混沌测试 | 验证容错与恢复能力 |

## Related

- [[11-发布变更/04-变更管理/index.md|变更管理]]
- [[11-发布变更/03-Progressive-Delivery/index.md|Progressive Delivery]]
- [[12-可靠性/07-性能测试/index.md|性能测试]]
