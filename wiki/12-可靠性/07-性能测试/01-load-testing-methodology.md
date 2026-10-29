---
title: 负载测试方法论
description: '├── 监控: Prometheus + Grafana'
summary: '├── 监控: Prometheus + Grafana'
category: domain
tags:
- performance-testing
- load-testing
- sre
- capacity
- prometheus
- grafana
- helm
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
- 负载测试方法论 是什么
- 如何 负载测试方法论
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- 负载测试方法论
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
- helm-basics
- prometheus-basics
- monitoring-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 负载测试方法论

## 四种测试类型

| 类型 | 目的 | 负载模式 |
|------|------|---------|
| **Load Test** | 验证系统在正常负载下的性能 | 预期的正常峰值 |
| **Stress Test** | 找到系统崩溃的临界点 | 逐步增加到系统崩溃 |
| **Spike Test** | 验证突发流量的应对能力 | 突然大幅增加 |
| **Soak Test** | 发现长时间运行的隐藏问题 | 持续数小时/天 |

## K8s 环境工具链

```
# 🟢 低风险：只读/信息收集，通常无副作用
工具选择:
├── 流量生成: k6, Locust, JMeter, Gatling
├── 监控: Prometheus + Grafana
├── 分布式执行: k6-operator, Locust Helm Chart
└── 报告: k6 Cloud, Grafana Dashboard
```
## k6 示例

```javascript
// load-test.js
import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  stages: [
    { duration: '2m', target: 100 },   //  ramp up
    { duration: '5m', target: 100 },   //  steady
    { duration: '2m', target: 200 },   //  ramp up
    { duration: '5m', target: 200 },   //  steady
    { duration: '2m', target: 0 },     //  ramp down
  ],
  thresholds: {
    http_req_duration: ['p(95)<500'],
    http_req_failed: ['rate<0.01'],
  },
};

export default function () {
  const res = http.get('http://order-service/api/v1/orders');
  check(res, {
    'status is 200': (r) => r.status === 200,
    'response time < 500ms': (r) => r.timings.duration < 500,
  });
  sleep(1);
}
```

## 测试环境准备

### 环境隔离策略

| 环境类型 | 用途 | 配置 | 数据 |
|---------|------|------|------|
| **Perf-Dev** | 开发自测 | 单副本、最小资源 | Mock 数据 |
| **Perf-Staging** | 集成测试 | 与生产同构 | 脱敏生产数据 |
| **Perf-Prod** | 生产验证 | 生产环境低峰期 | 真实数据（只读） |

### 测试命名空间配置

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: load-testing
  labels:
    purpose: performance-testing
---
# 资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: load-testing-quota
  namespace: load-testing
spec:
  hard:
    requests.cpu: "20"
    requests.memory: 40Gi
    limits.cpu: "40"
    limits.memory: 80Gi
    pods: "50"
---
# 网络策略：允许访问目标服务
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-to-target
  namespace: load-testing
spec:
  podSelector: {}
  policyTypes:
    - Egress
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              purpose: performance-target
```

## 分布式负载测试

### k6 Operator 部署

```bash
# 🟡 中风险：部署 k6 Operator
helm repo add grafana https://grafana.github.io/helm-charts
helm install k6-operator grafana/k6-operator \
  --namespace k6-operator-system \
  --create-namespace

# 验证部署
kubectl get pods -n k6-operator-system
```

### 分布式测试配置

```yaml
apiVersion: k6.io/v1alpha1
kind: TestRun
metadata:
  name: distributed-load-test
  namespace: load-testing
spec:
  parallelism: 4  # 4 个并行执行器
  script:
    configMap:
      name: k6-test-script
  arguments: --out prometheus=remoteWrite
  runner:
    image: grafana/k6:latest
    resources:
      requests:
        cpu: "1"
        memory: 2Gi
      limits:
        cpu: "2"
        memory: 4Gi
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: k6-test-script
  namespace: load-testing
data:
  test.js: |
    import http from 'k6/http';
    import { check, sleep } from 'k6';
    import { Counter, Rate, Trend } from 'k6/metrics';

    // 自定义指标
    const errorRate = new Rate('errors');
    const orderCreation = new Counter('orders_created');
    const orderLatency = new Trend('order_latency');

    export const options = {
      scenarios: {
        // 场景 1: 正常负载
        normal_load: {
          executor: 'ramping-vus',
          startVUs: 0,
          stages: [
            { duration: '2m', target: 100 },
            { duration: '5m', target: 100 },
            { duration: '2m', target: 0 },
          ],
          exec: 'normalTraffic',
        },
        // 场景 2: 峰值负载
        spike_load: {
          executor: 'shared-iterations',
          vus: 50,
          iterations: 1000,
          maxDuration: '5m',
          exec: 'spikeTraffic',
          startTime: '10m',
        },
      },
      thresholds: {
        http_req_duration: ['p(95)<500', 'p(99)<1000'],
        http_req_failed: ['rate<0.01'],
        errors: ['rate<0.05'],
        order_latency: ['p(95)<800'],
      },
    };

    export function normalTraffic() {
      const res = http.get('http://order-service.production/api/v1/orders');
      check(res, {
        'status is 200': (r) => r.status === 200,
        'response time < 500ms': (r) => r.timings.duration < 500,
      });
      errorRate.add(res.status >= 500);
      sleep(1);
    }

    export function spikeTraffic() {
      const payload = JSON.stringify({
        item: 'test-product',
        quantity: 1,
      });
      const params = { headers: { 'Content-Type': 'application/json' } };
      
      const res = http.post('http://order-service.production/api/v1/orders', payload, params);
      
      orderCreation.add(res.status === 201);
      orderLatency.add(res.timings.duration);
      errorRate.add(res.status >= 500);
      
      check(res, {
        'order created': (r) => r.status === 201,
      });
    }
```

## 测试场景设计

### 场景矩阵

| 场景 | 目的 | 负载模式 | 持续时间 | 通过标准 |
|-----|------|---------|---------|----------|
| **基准测试** | 建立性能基线 | 低负载 (10 VU) | 10min | P95 < 200ms |
| **负载测试** | 验证正常峰值 | 预期峰值 (100 VU) | 30min | P95 < 500ms, 错误率 < 1% |
| **压力测试** | 找到崩溃点 | 递增至崩溃 | 至崩溃 | 记录崩溃点 |
| **峰值测试** | 验证突发流量 | 突然 10x | 5min | 自动扩容生效, P95 < 1s |
| **浸泡测试** | 发现内存泄漏 | 持续中等负载 | 4-24h | 内存无持续增长 |
| **容量测试** | 验证最大容量 | 递增至 SLO 边界 | 30min | 确定最大 VU |

### 测试数据准备

```javascript
// 测试数据生成器
import { SharedArray } from 'k6/data';

const users = new SharedArray('users', function () {
  const data = [];
  for (let i = 0; i < 10000; i++) {
    data.push({
      id: i,
      email: `user${i}@example.com`,
      token: `token-${i}`,
    });
  }
  return data;
});

export function setup() {
  // 预热：创建测试数据
  const res = http.post('http://api/test-data/seed', JSON.stringify({
    users: 10000,
    products: 1000,
  }));
  return { startTime: Date.now() };
}

export default function (data) {
  const user = users[Math.floor(Math.random() * users.length)];
  // 使用 user 进行测试...
}

export function teardown(data) {
  // 清理测试数据
  http.del('http://api/test-data/cleanup');
  console.log(`Test duration: ${(Date.now() - data.startTime) / 1000}s`);
}
```

## 性能指标与阈值

### 关键指标定义

| 指标 | 定义 | 目标值 | 测量方法 |
|-----|------|-------|----------|
| **P50 延迟** | 50% 请求的响应时间 | < 100ms | k6 http_req_duration |
| **P95 延迟** | 95% 请求的响应时间 | < 500ms | k6 http_req_duration |
| **P99 延迟** | 99% 请求的响应时间 | < 1000ms | k6 http_req_duration |
| **错误率** | 5xx 响应占比 | < 1% | k6 http_req_failed |
| **吞吐量** | 每秒请求数 | > 1000 RPS | k6 http_reqs |
| **并发用户** | 同时活跃用户数 | 按场景 | k6 vus |
| **CPU 使用率** | 目标服务 CPU | < 70% | Prometheus |
| **内存使用率** | 目标服务内存 | < 80% | Prometheus |

### 阈值配置示例

```javascript
export const options = {
  thresholds: {
    // 延迟阈值
    http_req_duration: [
      'p(50)<100',    // P50 < 100ms
      'p(95)<500',    // P95 < 500ms
      'p(99)<1000',   // P99 < 1000ms
    ],
    // 错误率阈值
    http_req_failed: ['rate<0.01'],  // < 1%
    // 自定义指标阈值
    errors: ['rate<0.05'],           // < 5%
    order_latency: ['p(95)<800'],    // P95 < 800ms
    // 资源使用阈值（通过 Prometheus 远程写入）
    'cpu_usage': ['avg<0.7'],        // 平均 CPU < 70%
    'memory_usage': ['avg<0.8'],     // 平均内存 < 80%
  },
};
```

## 测试报告模板

### 自动报告生成

```javascript
// k6 报告生成器
import { textSummary } from 'https://jslib.k6.io/k6-summary/0.0.1/index.js';
import { jUnit } from 'https://jslib.k6.io/k6-summary/0.0.1/index.js';

export function handleSummary(data) {
  return {
    'stdout': textSummary(data, { indent: ' ', enableColors: true }),
    'reports/summary.json': JSON.stringify(data, null, 2),
    'reports/junit.xml': jUnit(data),
    'reports/report.md': generateMarkdownReport(data),
  };
}

function generateMarkdownReport(data) {
  const metrics = data.metrics;
  return `
# 负载测试报告

## 执行信息
- 时间: ${new Date().toISOString()}
- 持续时间: ${data.state.testRunDurationMs / 1000}s
- 峰值 VU: ${metrics.vus.values.max}

## 关键指标
| 指标 | 值 | 目标 | 状态 |
|-----|-----|------|------|
| P95 延迟 | ${metrics.http_req_duration.values['p(95)'].toFixed(2)}ms | < 500ms | ${metrics.http_req_duration.values['p(95)'] < 500 ? '✅' : '❌'} |
| 错误率 | ${(metrics.http_req_failed.values.rate * 100).toFixed(2)}% | < 1% | ${metrics.http_req_failed.values.rate < 0.01 ? '✅' : '❌'} |
| 吞吐量 | ${metrics.http_reqs.values.rate.toFixed(2)} RPS | > 1000 | ${metrics.http_reqs.values.rate > 1000 ? '✅' : '❌'} |

## 结论
${metrics.http_req_duration.values['p(95)'] < 500 && metrics.http_req_failed.values.rate < 0.01 ? '✅ 测试通过' : '❌ 测试失败'}
`;
}
```

## CI/CD 集成

### GitHub Actions 集成

```yaml
name: Performance Test
on:
  pull_request:
    paths:
      - 'src/**'
  schedule:
    - cron: '0 2 * * *'  # 每日凌晨 2 点

jobs:
  load-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Setup k6
        uses: grafana/setup-k6@v1

      - name: Run load test
        run: |
          k6 run --out json=results.json tests/load-test.js

      - name: Check thresholds
        run: |
          # 解析结果并检查阈值
          P95=$(cat results.json | jq '.metrics.http_req_duration["p(95)"]')
          ERROR_RATE=$(cat results.json | jq '.metrics.http_req_failed.rate')
          
          if (( $(echo "$P95 > 500" | bc -l) )); then
            echo "❌ P95 延迟超标: ${P95}ms"
            exit 1
          fi
          
          if (( $(echo "$ERROR_RATE > 0.01" | bc -l) )); then
            echo "❌ 错误率超标: ${ERROR_RATE}"
            exit 1
          fi
          
          echo "✅ 性能测试通过"

      - name: Upload report
        uses: actions/upload-artifact@v4
        with:
          name: load-test-report
          path: reports/
```

## 常见问题排查

| 问题 | 可能原因 | 解决方案 |
|-----|---------|----------|
| 测试结果波动大 | 资源竞争、网络抖动 | 增加预热时间、多次运行取平均 |
| 无法达到目标 RPS | 测试机瓶颈、连接数限制 | 使用分布式测试、调整连接池 |
| 延迟持续增加 | 内存泄漏、连接泄漏 | 运行浸泡测试、检查资源释放 |
| 错误率突然升高 | 服务过载、依赖故障 | 检查服务日志、监控依赖状态 |
| HPA 未触发 | 指标延迟、阈值设置 | 检查 metrics-server、调整 HPA |

## 相关

- 可靠性/03-capacity-planning/01-capacity-planning-guide


<!-- risk-assessed -->
