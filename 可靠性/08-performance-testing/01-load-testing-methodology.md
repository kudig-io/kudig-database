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

## 相关

- 可靠性/03-capacity-planning/01-capacity-planning-guide


<!-- risk-assessed -->
