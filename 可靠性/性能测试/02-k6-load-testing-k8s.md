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
4. **没 baseline**：没有基线就没法判断"这次结果是变好还是变差"。每次发版前先跑基线。

## 相关

- [[可靠性/性能测试/01-load-testing-methodology.md|01 load testing methodology]]
- [[可靠性/性能测试/03-locust-distributed-testing.md|03 locust distributed testing]]
- [[可靠性/性能测试/04-production-load-testing-playbook.md|04 production load testing playbook]]
- [[可靠性/SRE实践/06-slo-dashboard-design.md|06 slo dashboard design]]

<!-- risk-assessed -->
