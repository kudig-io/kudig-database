---
title: k6 在 Kubernetes 上的负载测试
description: 用 k6 Operator 在 Kubernetes 上运行分布式与云输出模式的负载测试
summary: k6 Operator + TestRun CRD 实现分布式压测，含测试脚本、分布式执行与云结果聚合
category: reliability
tags:
- slo
- sli
- reliability
- load-testing
- k6
- kubernetes
tier: core
created: '2026-07-11'
last_updated: 2026-07
difficulty: advanced
audience:
- SRE
- 架构师
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# k6 在 Kubernetes 上的负载测试

> **核心原则**：k6 在 K8s 上的价值不是"换了个运行环境"，而是**让压测成为可声明、可复现、可分布式扩展的集群资源**。一个 `TestRun` CRD 就能拉起 N 个 worker 同时压，结果自动聚合——比单机脚本更接近真实流量规模，也更易纳入 GitOps。

## k6 Operator 架构

```
┌──────────────────────────────────────────────┐
│  k6 Operator (Controller)                     │
│  监听 TestRun CR                              │
└──────────────────┬───────────────────────────┘
                   │ 创建
                   ▼
┌──────────────────────────────────────────────┐
│  k6 starter Job                               │
│  ┌────────┐ ┌────────┐ ┌────────┐            │
│  │runner 1│ │runner 2│ │runner N│  并行压测   │
│  └────┬───┘ └────┬───┘ └────┬───┘            │
└───────┼──────────┼──────────┼────────────────┘
        └──────────┼──────────┘
                   ▼
          ┌─────────────────┐
          │ Target Service  │
          └────────┬────────┘
                   ▼
          ┌─────────────────┐
          │ 结果输出          │
          │ Prometheus/Cloud │
          └─────────────────┘
```

## 1. 安装 Operator

```bash
# 🟡 中危：安装集群组件
kubectl create namespace k6-operator-system
kubectl apply -f https://github.com/grafana/k6-operator/releases/download/v0.0.16/k6-operator-0.0.16.yaml

# 验证
kubectl get pods -n k6-operator-system
```

## 2. 测试脚本 ConfigMap

```javascript
// k6-test.js — 典型阶梯压测脚本
import http from 'k6/http';
import { check, sleep } from 'k6';
import { Trend } from 'k6/metrics';

const latency = new Trend('latency_ms');

export const options = {
  stages: [
    { duration: '2m', target: 100 },    // 2 分钟升到 100 VU
    { duration: '5m', target: 100 },    // 稳定 5 分钟
    { duration: '2m', target: 500 },    // 升到峰值
    { duration: '5m', target: 500 },    // 峰值稳定
    { duration: '2m', target: 0 },      // 缩回 0
  ],
  thresholds: {
    http_req_failed: ['rate<0.01'],           // 错误率 < 1%
    http_req_duration: ['p(99)<500'],         // P99 < 500ms
  },
};

export default function () {
  const res = http.get('http://api.default.svc.cluster.local/health', {
    headers: { 'Content-Type': 'application/json' },
  });
  latency.add(res.timings.duration);
  check(res, { 'status 200': (r) => r.status === 200 });
  sleep(1);
}
```

```bash
# 🟢 低危：打包脚本进 ConfigMap
kubectl create configmap k6-test --from-file=k6-test.js -n perf
```

## 3. 分布式 TestRun CRD

```yaml
apiVersion: k6.io/v1alpha1
kind: TestRun
metadata: { name: api-load-test, namespace: perf }
spec:
  parallelism: 5                    # ★ 5 个 runner 并行，分摊负载
  script:
    configMap: { name: k6-test, file: k6-test.js }
  runner:
    env:
      - { name: K6_STATSD_ENABLE, value: "true" }
      - { name: K6_STATSD_ADDR, value: "statsd.monitoring:9125" }
    resources:
      requests: { cpu: 500m, memory: 512Mi }
      limits:   { cpu: 1,    memory: 1Gi }
  arguments: "--out experimental-prometheus-rw"   # 结果推 Prometheus
```

```bash
# 🟡 中危：启动压测（会生成真实流量）
kubectl apply -f testrun.yaml -n perf
kubectl get testrun -n perf -w   # 等待 completed
```

## 4. 结果输出三选一

| 模式 | 配置 | 适用 |
|------|------|------|
| 标准输出 | 默认 | 调试 |
| Prometheus | `--out experimental-prometheus-rw` | 长期归档、Grafana 可视化 |
| k6 Cloud | `K6_CLOUD_TOKEN` + `--out cloud` | 跨集群、协同分析 |

推荐生产用 **Prometheus 输出**：压测数据自动进现有观测栈，与 SLO 面板对齐。

## 5. CI/CD 集成门控

```yaml
# .github/workflows/perf-gate.yml
- name: Run k6 load test
  run: |
    kubectl apply -f testrun.yaml -n perf
    kubectl wait testrun/api-load-test -n perf \
      --for=jsonpath='{.status.stage}'=finished --timeout=30m
    kubectl get testrun api-load-test -n perf -o yaml | grep -q passed
```

阈值失败（如 P99 > 500ms）即阻断发布。

## 分布式注意事项

1. **parallelism 与总 VU**：5 个 runner × 100 VU = 500 总并发，脚本里写的是每 runner 的 VU。
2. **runner 资源**：CPU 不够会自我 throttle，测出来的是"压测机瓶颈"而非"被测系统瓶颈"。监控 runner 的 CPU 使用率应 < 70%。
3. **客户端连接池**：高并发下 DNS/连接复用会成为瓶颈，脚本里预热连接池。
4. **网络位置**：runner 要和被测服务同集群或邻近可用区，否则测的是网络而非服务。

## 常见陷阱

1. **从本机压生产**：本机出口带宽与延迟会扭曲结果，永远从集群内压。
2. **thresholds 设太松**：`p(99)<2000ms` 这种阈值永远过，等于没设。与 SLO 对齐。
3. **只测 happy path**：忘了测登录/支付等慢路径，掩盖真实瓶颈。
4. **没 baseline**：没有基线就没法判断“这次结果是变好还是变差”。每次发版前先跑基线。

## 高级测试场景

### 多场景测试脚本

```javascript
// k6-multi-scenario.js — 多场景测试
import http from 'k6/http';
import { check, sleep, group } from 'k6';
import { Rate, Trend } from 'k6/metrics';

const errorRate = new Rate('errors');
const apiLatency = new Trend('api_latency');
const dbLatency = new Trend('db_latency');

export const options = {
  scenarios: {
    // 场景 1: 常规浏览
    browse: {
      executor: 'ramping-vus',
      startVUs: 0,
      stages: [
        { duration: '2m', target: 100 },
        { duration: '5m', target: 100 },
        { duration: '2m', target: 0 },
      ],
      exec: 'browseScenario',
      tags: { scenario: 'browse' },
    },
    // 场景 2: 高并发 API
    api_stress: {
      executor: 'constant-arrival-rate',
      rate: 500,
      timeUnit: '1s',
      duration: '5m',
      preAllocatedVUs: 50,
      maxVUs: 200,
      exec: 'apiScenario',
      startTime: '1m',
      tags: { scenario: 'api' },
    },
    // 场景 3: 突发流量
    spike: {
      executor: 'shared-iterations',
      vus: 100,
      iterations: 10000,
      maxDuration: '5m',
      exec: 'spikeScenario',
      startTime: '3m',
      tags: { scenario: 'spike' },
    },
  },
  thresholds: {
    http_req_failed: ['rate<0.01'],
    http_req_duration: ['p(99)<500'],
    'api_latency': ['p(95)<300'],
    'db_latency': ['p(95)<100'],
  },
};

export function browseScenario() {
  group('Browse Products', () => {
    const res = http.get('http://api.default.svc/products');
    errorRate.add(res.status !== 200);
    apiLatency.add(res.timings.duration);
    check(res, { 'status 200': (r) => r.status === 200 });
  });
  sleep(Math.random() * 3);
}

export function apiScenario() {
  const res = http.post('http://api.default.svc/orders', JSON.stringify({
    items: [{ sku: 'A1', qty: 1 }],
  }), {
    headers: { 'Content-Type': 'application/json' },
  });
  errorRate.add(res.status >= 400);
  apiLatency.add(res.timings.duration);
}

export function spikeScenario() {
  const res = http.get('http://api.default.svc/health');
  errorRate.add(res.status !== 200);
}
```

### 数据参数化测试

```javascript
// k6-parameterized.js — 数据驱动测试
import http from 'k6/http';
import { SharedArray } from 'k6/data';
import { check } from 'k6';

// 从 ConfigMap 加载测试数据
const users = new SharedArray('users', function () {
  return JSON.parse(open('/data/users.json'));
});

const products = new SharedArray('products', function () {
  return JSON.parse(open('/data/products.json'));
});

export default function () {
  const user = users[Math.floor(Math.random() * users.length)];
  const product = products[Math.floor(Math.random() * products.length)];

  // 登录
  const loginRes = http.post('http://api.default.svc/login', JSON.stringify({
    username: user.username,
    password: user.password,
  }));

  const token = loginRes.json('token');

  // 浏览商品
  const browseRes = http.get(`http://api.default.svc/products/${product.id}`, {
    headers: { 'Authorization': `Bearer ${token}` },
  });

  check(browseRes, {
    'status 200': (r) => r.status === 200,
    'has product': (r) => r.json('id') === product.id,
  });
}
```

## 监控与告警

### PrometheusRule 压测告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: k6-load-testing-alerts
  namespace: monitoring
spec:
  groups:
    - name: k6.rules
      rules:
        # 压测进行中
        - alert: K6LoadTestRunning
          expr: |
            k6_vus{testid=~".+"} > 0
          for: 1m
          labels:
            severity: info
          annotations:
            summary: "压测 {{ $labels.testid }} 正在进行，当前 VU: {{ $value }}"

        # 压测失败率过高
        - alert: K6HighErrorRate
          expr: |
            rate(k6_http_req_failed_total[1m]) > 0.05
          for: 2m
          labels:
            severity: warning
          annotations:
            summary: "压测失败率超过 5%"

        # 压测延迟过高
        - alert: K6HighLatency
          expr: |
            histogram_quantile(0.99, rate(k6_http_req_duration_seconds_bucket[1m])) > 1
          for: 2m
          labels:
            severity: warning
          annotations:
            summary: "压测 P99 延迟超过 1s"
```

### Grafana Dashboard

```json
{
  "dashboard": {
    "title": "k6 压测监控",
    "panels": [
      {
        "title": "VU 数量",
        "type": "graph",
        "targets": [
          { "expr": "k6_vus" }
        ]
      },
      {
        "title": "RPS",
        "type": "graph",
        "targets": [
          { "expr": "rate(k6_http_reqs_total[1m])" }
        ]
      },
      {
        "title": "P99 延迟",
        "type": "graph",
        "targets": [
          { "expr": "histogram_quantile(0.99, rate(k6_http_req_duration_seconds_bucket[1m]))" }
        ]
      },
      {
        "title": "错误率",
        "type": "graph",
        "targets": [
          { "expr": "rate(k6_http_req_failed_total[1m])" }
        ]
      }
    ]
  }
}
```

## 测试报告生成

### 自动生成报告脚本

```bash
#!/bin/bash
# 🟢 低风险：生成压测报告
set -euo pipefail

TEST_NAME=${1:-"api-load-test"}
NAMESPACE=${2:-"perf"}
OUTPUT_FILE="/tmp/k6-report-$(date +%Y%m%d-%H%M%S).md"

echo "=== 生成压测报告 ==="

# 获取测试结果
STATUS=$(kubectl get testrun $TEST_NAME -n $NAMESPACE -o jsonpath='{.status.stage}')
START_TIME=$(kubectl get testrun $TEST_NAME -n $NAMESPACE -o jsonpath='{.status.startTime}')
END_TIME=$(kubectl get testrun $TEST_NAME -n $NAMESPACE -o jsonpath='{.status.completionTime}')

# 从 Prometheus 获取指标
P99_LATENCY=$(curl -sG "$PROM/api/v1/query" \
  --data-urlencode 'query=histogram_quantile(0.99, sum by(le)(rate(k6_http_req_duration_seconds_bucket[5m])))' \
  | jq -r '.data.result[0].value[1]')

ERROR_RATE=$(curl -sG "$PROM/api/v1/query" \
  --data-urlencode 'query=sum(rate(k6_http_req_failed_total[5m]))' \
  | jq -r '.data.result[0].value[1]')

RPS=$(curl -sG "$PROM/api/v1/query" \
  --data-urlencode 'query=sum(rate(k6_http_reqs_total[5m]))' \
  | jq -r '.data.result[0].value[1]')

cat > $OUTPUT_FILE <<EOF
# k6 压测报告

**测试名称**: $TEST_NAME
**执行时间**: $START_TIME - $END_TIME
**测试状态**: $STATUS

## 性能指标

| 指标 | 结果 | 阈值 | 状态 |
|-----|------|------|------|
| P99 延迟 | ${P99_LATENCY}s | < 0.5s | $([ $(echo "$P99_LATENCY < 0.5" | bc) -eq 1 ] && echo "✓" || echo "✗") |
| 错误率 | ${ERROR_RATE} | < 0.01 | $([ $(echo "$ERROR_RATE < 0.01" | bc) -eq 1 ] && echo "✓" || echo "✗") |
| RPS | ${RPS} | > 1000 | $([ $(echo "$RPS > 1000" | bc) -eq 1 ] && echo "✓" || echo "✗") |

## 建议

- 继续监控生产环境指标
- 下次压测建议增加并发数

---
*本报告由自动化脚本生成*
EOF

echo "报告已生成: $OUTPUT_FILE"
cat $OUTPUT_FILE
```

## 性能基线管理

### 基线存储 ConfigMap

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: k6-baseline
  namespace: perf
data:
  baseline.json: |
    {
      "version": "v1.2.3",
      "timestamp": "2026-07-01T10:00:00Z",
      "metrics": {
        "p99_latency_ms": 450,
        "error_rate": 0.005,
        "rps": 1200,
        "concurrent_users": 500
      }
    }
```

### 基线对比脚本

```bash
#!/bin/bash
# 🟢 低风险：对比基线
set -euo pipefail

BASELINE=$(kubectl get configmap k6-baseline -n perf -o jsonpath='{.data.baseline\.json}')
CURRENT_P99=${1:-0.5}

BASELINE_P99=$(echo $BASELINE | jq -r '.metrics.p99_latency_ms')

DIFF=$(echo "($CURRENT_P99 * 1000 - $BASELINE_P99) / $BASELINE_P99 * 100" | bc)

echo "=== 基线对比 ==="
echo "基线 P99: ${BASELINE_P99}ms"
echo "当前 P99: $(echo "$CURRENT_P99 * 1000" | bc)ms"
echo "变化: ${DIFF}%"

if (( $(echo "$DIFF > 20" | bc -l) )); then
  echo "⚠️ 性能下降超过 20%，需要调查"
  exit 1
elif (( $(echo "$DIFF < -20" | bc -l) )); then
  echo "✓ 性能提升超过 20%，建议更新基线"
else
  echo "✓ 性能在正常范围内"
fi
```

## 故障排查

### 常见问题诊断

| 问题 | 可能原因 | 解决方案 |
|-----|---------|----------|
| TestRun 一直 Pending | 资源不足 | 检查节点资源，调整 runner resources |
| 压测 RPS 上不去 | runner CPU 瓶颈 | 增加 parallelism 或 runner CPU |
| 结果不准确 | 网络延迟 | 确保 runner 与被测服务同集群 |
| 阈值总是失败 | 阈值设置不合理 | 与 SLO 对齐，调整阈值 |
| Operator 不工作 | RBAC 权限 | 检查 ServiceAccount 权限 |

### 调试命令

```bash
# 🟢 低风险：查看 TestRun 状态
kubectl get testrun -n perf -o wide

# 🟢 低风险：查看 runner 日志
kubectl logs -n perf -l app=k6-runner --tail=100

# 🟢 低风险：查看 runner 资源使用
kubectl top pods -n perf -l app=k6-runner

# 🟢 低风险：查看 Operator 日志
kubectl logs -n k6-operator-system -l app=k6-operator --tail=100

# 🟢 低风险：检查 Prometheus 指标
curl -sG "$PROM/api/v1/query" --data-urlencode 'query=k6_vus' | jq
```

## 相关

- [[可靠性/性能测试/01-load-testing-methodology.md|01 load testing methodology]]
- [[可靠性/性能测试/03-locust-distributed-testing.md|03 locust distributed testing]]
- [[可靠性/性能测试/04-production-load-testing-playbook.md|04 production load testing playbook]]
- [[可靠性/SRE实践/06-slo-dashboard-design.md|06 slo dashboard design]]

<!-- risk-assessed -->
