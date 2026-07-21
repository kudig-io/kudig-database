---
title: Kubernetes Load Testing & Performance Benchmarking
description: K8s 负载测试与性能基准 — 压测工具链、API Server 基准、网络性能测试、容量规划方法论
summary: Kubernetes 集群与应用的性能测试完整指南，涵盖工具选型、基准建立、容量规划
category: practice
tags:
- load-testing
- benchmark
- k6
- locust
- capacity-planning
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: reliability
---
# Kubernetes 负载测试与性能基准

> 建立性能基线，验证容量规划，确保系统在高负载下稳定运行。

## 性能测试层次

| 层次 | 目标 | 工具 |
|------|------|------|
| 应用层 | API 延迟/吞吐 | k6/Locust/Gatling |
| 网络层 | Pod 间延迟/带宽 | iperf3/netperf |
| 存储层 | IOPS/延迟 | fio |
| 控制平面 | API Server/etcd | clusterloader2 |
| 端到端 | 全链路性能 | 自定义场景 |

## 应用层负载测试

### k6 高级场景

```javascript
// k6-load-test.js
import http from 'k6/http';
import { check, sleep, group } from 'k6';
import { Rate, Trend } from 'k6/metrics';

// 自定义指标
const errorRate = new Rate('errors');
const orderDuration = new Trend('order_duration', true);

export const options = {
  scenarios: {
    // 场景1：恒定负载
    constant_load: {
      executor: 'constant-arrival-rate',
      rate: 100,
      timeUnit: '1s',
      duration: '5m',
      preAllocatedVUs: 50,
      maxVUs: 200,
    },
    // 场景2：阶梯加压
    ramp_up: {
      executor: 'ramping-arrival-rate',
      startRate: 10,
      timeUnit: '1s',
      preAllocatedVUs: 20,
      maxVUs: 500,
      stages: [
        { target: 50, duration: '2m' },
        { target: 100, duration: '3m' },
        { target: 200, duration: '2m' },
        { target: 500, duration: '3m' },
      ],
    },
    // 场景3：峰值测试
    spike: {
      executor: 'shared-iterations',
      vus: 1000,
      iterations: 5000,
      maxDuration: '2m',
      startTime: '12m',  // 在前两个场景后执行
    },
  },
  thresholds: {
    http_req_duration: ['p(50)<100', 'p(95)<300', 'p(99)<800'],
    http_req_failed: ['rate<0.01'],
    errors: ['rate<0.05'],
    order_duration: ['p(95)<500'],
  },
};

const BASE_URL = __ENV.BASE_URL || 'http://api-service:8080';

export default function () {
  group('API Health', () => {
    const res = http.get(`${BASE_URL}/healthz`);
    check(res, { 'health ok': (r) => r.status === 200 });
  });

  group('Create Order', () => {
    const payload = JSON.stringify({
      item: `product-${Math.floor(Math.random() * 100)}`,
      quantity: Math.floor(Math.random() * 5) + 1,
    });
    const params = {
      headers: { 'Content-Type': 'application/json' },
    };
    const start = Date.now();
    const res = http.post(`${BASE_URL}/api/orders`, payload, params);
    orderDuration.add(Date.now() - start);
    
    const success = check(res, {
      'order created': (r) => r.status === 201,
      'has order id': (r) => r.json('id') !== undefined,
    });
    errorRate.add(!success);
  });

  sleep(Math.random() * 2);
}
```

### Locust 分布式测试

```python
# locustfile.py
from locust import HttpUser, task, between
from locust.runners import MasterRunner
import random

class APIUser(HttpUser):
    wait_time = between(0.5, 2.0)
    
    def on_start(self):
        # 登录获取 token
        resp = self.client.post("/auth/login", json={
            "username": f"user-{random.randint(1,1000)}",
            "password": "test-pass"
        })
        self.token = resp.json().get("token", "")
        self.client.headers["Authorization"] = f"Bearer {self.token}"
    
    @task(5)
    def get_products(self):
        self.client.get("/api/products?page=1&limit=20")
    
    @task(3)
    def create_order(self):
        self.client.post("/api/orders", json={
            "items": [{"sku": f"SKU-{random.randint(1,500)}", "qty": 1}]
        })
    
    @task(1)
    def search(self):
        self.client.get(f"/api/search?q=product-{random.randint(1,100)}")
```

```bash
# 分布式运行（K8s Job）
# Master
locust -f locustfile.py --master --host=http://api-service:8080

# Workers (多 Pod)
locust -f locustfile.py --worker --master-host=locust-master
```

## 控制平面性能测试

### clusterloader2（K8s 官方）

```yaml
# perf-test-config.yaml
name: density
namespace:
  number: 100
pod:
  number: 5000
  image: registry.k8s.io/pause:3.9
tuningSets:
  - name: Uniform5qps
    qpsLoad:
      qps: 5
steps:
  - name: Create pods
    phases:
      - namespaceRange:
          min: 1
          max: 100
        replicasPerNamespace: 50
        tuningSet: Uniform5qps
        objectBundle:
          - basename: pod
            objectTemplatePath: pod.yaml
  - name: Measure
    measurements:
      - Identifier: PodStartupLatency
        Method: PodStartupLatency
        Params:
          action: start
      - Identifier: APICallLatency
        Method: APICallLatency
        Params:
          action: start
```

```bash
# 运行
clusterloader2 \
  --testconfig=perf-test-config.yaml \
  --provider=aws \
  --kubeconfig=$HOME/.kube/config \
  --report-dir=./results
```

### API Server 基准指标

| 指标 | 健康阈值 | 告警阈值 |
|------|----------|----------|
| apiserver_request_duration P99 | < 1s | > 4s |
| apiserver_current_inflight_requests | < 400 | > 600 |
| etcd_request_duration P99 | < 100ms | > 500ms |
| apiserver_flowcontrol_rejected_requests | 0 | > 0 持续 |
| workqueue_depth | < 50 | > 100 |

## 网络性能测试

### iperf3 Pod 间测试

```yaml
# iperf3-server.yaml
apiVersion: v1
kind: Pod
metadata:
  name: iperf3-server
  labels:
    app: iperf3
spec:
  containers:
    - name: iperf3
      image: networkstatic/iperf3
      args: ["-s"]
      ports:
        - containerPort: 5201
---
# iperf3-client.yaml
apiVersion: v1
kind: Pod
metadata:
  name: iperf3-client
spec:
  containers:
    - name: iperf3
      image: networkstatic/iperf3
      args: ["-c", "iperf3-server", "-t", "30", "-P", "4"]
  restartPolicy: Never
```

### 网络延迟矩阵

```bash
# 使用 netperf 测试不同场景
# 同节点 Pod-to-Pod
netperf -H <pod-ip> -t TCP_RR -- -r 1,1

# 跨节点 Pod-to-Pod
netperf -H <remote-pod-ip> -t TCP_STREAM -l 30

# Pod-to-Service
netperf -H <service-cluster-ip> -t TCP_RR

# 结果记录
# 同节点: ~25μs RTT
# 跨节点(同AZ): ~80μs RTT
# 跨AZ: ~500μs RTT
```

## 存储性能测试

### fio 基准

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: fio-benchmark
spec:
  containers:
    - name: fio
      image: ljishen/fio
      command: ["sleep", "infinity"]
      volumeMounts:
        - name: test-volume
          mountPath: /data
  volumes:
    - name: test-volume
      persistentVolumeClaim:
        claimName: bench-pvc
```

```bash
# 随机读写 IOPS
fio --name=rand-rw --ioengine=libaio --direct=1 \
  --bs=4k --iodepth=64 --size=1G \
  --rw=randrw --rwmixread=70 \
  --runtime=60 --filename=/data/fio-test

# 顺序吞吐
fio --name=seq-rw --ioengine=libaio --direct=1 \
  --bs=128k --iodepth=32 --size=4G \
  --rw=rw --runtime=60 --filename=/data/fio-test
```

## 容量规划方法论

### 规划公式

```
所需副本数 = 峰值QPS / 单副本处理能力 × 安全系数(1.3)

所需节点数 = 总资源需求 / 单节点可用资源 × 冗余系数(1.2)
```

### 容量规划检查清单

- [ ] 建立性能基线（当前负载下的 P50/P95/P99）
- [ ] 确定 SLO 目标（延迟/可用性/吞吐）
- [ ] 执行阶梯加压测试找到拐点
- [ ] 记录资源利用率拐点（CPU > 70% 性能退化）
- [ ] 验证自动缩放响应时间
- [ ] 模拟峰值流量（2-3x 正常负载）
- [ ] 测试故障场景（节点丢失/依赖降级）
- [ ] 输出容量规划报告与扩容建议

## 性能测试报告模板

```markdown
## 性能测试报告

### 测试环境
- 集群版本: K8s 1.30
- 节点: 3x m5.2xlarge (8C32G)
- CNI: Cilium 1.15
- 测试工具: k6 v0.50

### 测试结果
| 指标 | 目标 | 实际 | 状态 |
|------|------|------|------|
| P95 延迟 | < 200ms | 156ms | ✅ |
| P99 延迟 | < 500ms | 380ms | ✅ |
| 错误率 | < 1% | 0.3% | ✅ |
| 最大吞吐 | > 1000 RPS | 1250 RPS | ✅ |

### 瓶颈分析
- CPU 在 800 RPS 时达到 75% 利用率
- 数据库连接池在 1000 RPS 时耗尽

### 建议
- HPA 目标 CPU 设为 60%
- 数据库连接池从 50 增加到 100
```

## Related

- [[可靠性/性能测试/index.md|性能测试目录]]
- [[可观测性/指标/index.md|指标 Metrics]]
- [[集群基础/性能调优/index.md|性能调优]]
