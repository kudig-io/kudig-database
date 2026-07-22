---
title: Locust 分布式负载测试
description: 用 Locust Helm Chart 在 Kubernetes 上运行 Master/Worker 分布式负载测试
summary: Locust Master/Worker Helm 部署 + Python 测试脚本 + 自动扩 worker + 结果聚合
category: reliability
tags:
- slo
- sli
- reliability
- load-testing
- locust
- python
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
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# Locust 分布式负载测试

> **核心原则**：Locust 的优势是**用 Python 写真实业务流程**——登录、加购物车、结账多步串联，比单 URL 压测更接近真实用户行为。单机 Locust 受 GIL 限制通常撑不过几千并发；分布式 Master/Worker 架构才能逼近生产级流量规模。

## Master/Worker 架构

```
                 ┌──────────────────┐
                 │  Locust Master   │
                 │  (调度 + Web UI)  │
                 └────────┬─────────┘
                          │ 分发任务
        ┌─────────────────┼─────────────────┐
        ▼                 ▼                 ▼
   ┌─────────┐       ┌─────────┐       ┌─────────┐
   │ Worker1 │       │ Worker2 │       │ WorkerN │
   │ (greenlets)│    │ (greenlets)│    │ (greenlets)│
   └────┬────┘       └────┬────┘       └────┬────┘
        └─────────────────┼─────────────────┘
                          ▼
                 ┌──────────────────┐
                 │  Target Service  │
                 └──────────────────┘
```

Master 不发请求，只协调；所有真实流量来自 Worker。一个 Worker 通常跑 500–1000 个 greenlet（伪并发），瓶颈是 CPU 与单连接。

## 1. Helm 部署

```bash
# 🟡 中危：部署压测集群
helm repo add deliveryhero https://charts.deliveryhero.io/
helm install locust deliveryhero/locust \
  --namespace perf --create-namespace \
  --set master.image.tag=2.20.0 \
  --set worker.replicaCount=5 \
  --set worker.resources.requests.cpu=500m
```

## 2. Python 测试脚本（真实业务流程）

```python
# locustfile.py — 多步骤业务流程，比单 URL 更真实
from locust import HttpUser, task, between, tag

class ApiUser(HttpUser):
    wait_time = between(1, 3)          # 模拟用户思考时间
    host = "http://api.default.svc.cluster.local"

    def on_start(self):
        # 登录拿 token，每用户会话
        res = self.client.post("/login", json={"user":"load","pass":"x"})
        self.token = res.json().get("token", "")

    @tag("browse")
    @task(3)                            # 权重 3：最常见的浏览行为
    def browse_catalog(self):
        with self.client.get("/products",
                headers={"Authorization": self.token},
                name="/products",
                catch_response=True) as r:
            if r.elapsed.total_seconds() > 1:
                r.failure("slow > 1s")  # 自定义失败判定

    @tag("checkout")
    @task(1)                            # 权重 1：少数用户结账
    def checkout(self):
        self.client.post("/cart", json={"sku":"A1","qty":1},
                         headers={"Authorization": self.token})
        self.client.post("/checkout",
                         headers={"Authorization": self.token})
```

```bash
# 🟢 低危：打包脚本
kubectl create configmap locust-script \
  --from-file=locustfile.py -n perf
```

## 3. values.yaml 完整配置

```yaml
master:
  config:
    target-host: "http://api.default.svc.cluster.local"
  resources:
    requests: { cpu: 500m, memory: 512Mi }
worker:
  replicaCount: 10                      # ★ 按 VU 总需求扩
  config:
    locust-script: ""                   # 从 ConfigMap 挂载
  extraVolumes:
    - name: script
      configMap: { name: locust-script }
  extraVolumeMounts:
    - name: script
      mountPath: /home/locust
  resources:
    requests: { cpu: 1,    memory: 1Gi }
    limits:   { cpu: 2,    memory: 2Gi }
```

```bash
# 🟡 中危：升级配置（会触发 worker 滚动重启）
helm upgrade locust deliveryhero/locust -f values.yaml -n perf
```

## 4. 启动压测（Headless）

```bash
# 🟡 中危：生成真实流量
kubectl exec -n perf deployment/locust-master -- \
  locust -f /home/locust/locustfile.py \
    --headless \
    --host http://api.default.svc.cluster.local \
    -u 5000 \                          # 总虚拟用户数
    -r 100 \                           # 每秒新增用户（ramp-up）
    --run-time 30m \
    --csv /tmp/result                  # 输出 CSV
```

`-u 5000` 会被自动分摊到 10 个 worker，每 worker 500 个 greenlet。

## 5. 结果采集

```bash
# 🟢 低危：拉取结果
kubectl cp perf/$(kubectl get pod -n perf -l app=locust-master -o jsonpath='{.items[0].metadata.name}'):/tmp/result_stats.csv ./result.csv

# 关键指标：RPS、失败率、P50/P95/P99、用户数
```

也支持推 Prometheus：用 `locust-exporter` sidecar 暴露指标，进 Grafana SLO 面板对齐。

## k6 vs Locust 选型

| 维度 | k6 | Locust |
|------|-----|--------|
| 语言 | JavaScript | Python |
| 单机并发 | 高（Go 运行时） | 中（GIL，靠多进程） |
| 脚本易写 | 中（需学 API） | 高（Python 生态） |
| 业务流程模拟 | 中 | 高（串联、条件分支强） |
| K8s 原生 | Operator 成熟 | Helm Chart 成熟 |

**口诀**：压简单接口、追求极致 RPS → k6；压复杂业务流程、要灵活逻辑 → Locust。

## 常见陷阱

1. **worker 资源不足**：worker 自己先 throttle，测出来的是压测机瓶颈。监控 worker CPU 应 < 70%。
2. **greenlet 不是真并发**：5000 greenlet ≠ 5000 真并发请求，它更像"5000 个交替执行的用户"。对连接池/锁敏感的系统要校准。
3. **没 ramp-up**：直接 `-u 5000` 会瞬间打爆，像 DDoS。务必用 `-r` 缓慢爬升。
4. **从集群外压集群内**：经过 Ingress/NAT 会扭曲延迟。runner 与被测服务同 namespace 或同 VPC。

## 高级测试场景

### 多用户行为模拟

```python
# locustfile-advanced.py — 多用户角色模拟
from locust import HttpUser, task, between, tag, events
from locust.runners import MasterRunner, WorkerRunner
import random
import json

class BaseUser(HttpUser):
    abstract = True
    wait_time = between(1, 3)
    
    def on_start(self):
        """用户会话初始化"""
        self.login()
        self.cart_items = []
    
    def login(self):
        """登录获取 token"""
        res = self.client.post("/api/login", json={
            "username": f"user_{random.randint(1, 10000)}",
            "password": "test123"
        })
        self.token = res.json().get("token", "")
        self.headers = {"Authorization": f"Bearer {self.token}"}
    
    def on_stop(self):
        """用户会话清理"""
        self.client.post("/api/logout", headers=self.headers)


class BrowserUser(BaseUser):
    """浏览型用户 - 70% 流量"""
    weight = 7
    
    @tag("browse")
    @task(5)
    def browse_products(self):
        """浏览商品列表"""
        with self.client.get("/api/products?page=1&size=20",
                headers=self.headers,
                name="/api/products",
                catch_response=True) as res:
            if res.elapsed.total_seconds() > 1:
                res.failure("slow > 1s")
    
    @tag("browse")
    @task(3)
    def view_product_detail(self):
        """查看商品详情"""
        product_id = random.randint(1, 1000)
        self.client.get(f"/api/products/{product_id}",
                       headers=self.headers,
                       name="/api/products/[id]")
    
    @tag("search")
    @task(2)
    def search_products(self):
        """搜索商品"""
        keywords = ["phone", "laptop", "tablet", "watch"]
        self.client.get(f"/api/search?q={random.choice(keywords)}",
                       headers=self.headers,
                       name="/api/search")


class BuyerUser(BaseUser):
    """购买型用户 - 20% 流量"""
    weight = 2
    
    @tag("cart")
    @task(3)
    def add_to_cart(self):
        """加入购物车"""
        product_id = random.randint(1, 1000)
        self.client.post("/api/cart",
                        json={"productId": product_id, "quantity": 1},
                        headers=self.headers,
                        name="/api/cart")
        self.cart_items.append(product_id)
    
    @tag("checkout")
    @task(1)
    def checkout(self):
        """结账"""
        if not self.cart_items:
            return
        with self.client.post("/api/checkout",
                json={"items": self.cart_items},
                headers=self.headers,
                name="/api/checkout",
                catch_response=True) as res:
            if res.status_code == 200:
                self.cart_items = []
                res.success()
            else:
                res.failure(f"status {res.status_code}")


class VIPUser(BaseUser):
    """VIP 用户 - 10% 流量，更高并发"""
    weight = 1
    wait_time = between(0.5, 1.5)  # 更短等待时间
    
    @tag("vip")
    @task(5)
    def vip_browse(self):
        """VIP 专属浏览"""
        self.client.get("/api/vip/products",
                       headers=self.headers,
                       name="/api/vip/products")
    
    @tag("vip")
    @task(2)
    def vip_exclusive(self):
        """VIP 专属商品"""
        self.client.get("/api/vip/exclusive",
                       headers=self.headers,
                       name="/api/vip/exclusive")
```

### 自定义指标收集

```python
# locustfile-metrics.py — 自定义指标
from locust import HttpUser, task, events
from locust.stats import stats_history, stats_printer
from prometheus_client import Counter, Histogram, Gauge, start_http_server
import time

# Prometheus 指标
REQUEST_COUNT = Counter('locust_requests_total', 'Total requests', ['method', 'endpoint', 'status'])
REQUEST_LATENCY = Histogram('locust_request_duration_seconds', 'Request latency', ['endpoint'])
ACTIVE_USERS = Gauge('locust_active_users', 'Active users')
ERROR_COUNT = Counter('locust_errors_total', 'Total errors', ['type'])

@events.request.add_listener
def on_request(request_type, name, response_time, response_length, exception, **kwargs):
    """请求完成回调"""
    status = kwargs.get('response').status_code if kwargs.get('response') else 0
    REQUEST_COUNT.labels(method=request_type, endpoint=name, status=status).inc()
    REQUEST_LATENCY.labels(endpoint=name).observe(response_time / 1000)
    if exception:
        ERROR_COUNT.labels(type=type(exception).__name__).inc()

@events.test_start.add_listener
def on_test_start(environment, **kwargs):
    """测试开始"""
    if not isinstance(environment.runner, WorkerRunner):
        start_http_server(9090)  # 暴露 Prometheus 指标

@events.user_spawn.add_listener
def on_user_spawn(user, **kwargs):
    ACTIVE_USERS.inc()

@events.user_stop.add_listener
def on_user_stop(user, **kwargs):
    ACTIVE_USERS.dec()


class ApiUser(HttpUser):
    wait_time = between(1, 3)
    
    @task
    def get_products(self):
        self.client.get("/api/products", name="/api/products")
```

## CI/CD 集成

### GitHub Actions 集成

```yaml
# .github/workflows/locust-test.yml
name: Locust Load Test
on:
  pull_request:
    branches: [main]
  schedule:
    - cron: '0 2 * * *'  # 每晚 2 点

jobs:
  load-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      
      - name: Deploy to staging
        run: |
          argocd app sync api-staging
          kubectl rollout status deployment/api -n staging --timeout=5m
      
      - name: Run Locust test
        run: |
          helm upgrade locust deliveryhero/locust \
            --namespace perf --create-namespace \
            --set worker.replicaCount=5 \
            --set master.config.target-host=http://api.staging.svc.cluster.local
          
          kubectl exec -n perf deployment/locust-master -- \
            locust -f /home/locust/locustfile.py \
              --headless \
              -u 1000 -r 50 \
              --run-time 10m \
              --csv /tmp/result \
              --exit-code-on-error 1
      
      - name: Check results
        run: |
          kubectl cp perf/$(kubectl get pod -n perf -l app=locust-master -o jsonpath='{.items[0].metadata.name}'):/tmp/result_stats.csv ./result.csv
          
          # 检查失败率
          FAIL_RATE=$(awk -F',' 'NR>1 {sum+=$6} END {print sum/NR}' result.csv)
          if (( $(echo "$FAIL_RATE > 0.01" | bc -l) )); then
            echo "::error::失败率 $FAIL_RATE 超过 1%"
            exit 1
          fi
      
      - name: Upload results
        uses: actions/upload-artifact@v4
        with:
          name: locust-results
          path: result.csv
```

### Argo Workflow 集成

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: locust-load-test
  namespace: perf
spec:
  entrypoint: main
  templates:
    - name: main
      steps:
        - - name: setup
            template: setup-locust
        - - name: run-test
            template: run-locust
        - - name: analyze
            template: analyze-results
        - - name: cleanup
            template: cleanup-locust
    
    - name: setup-locust
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            helm upgrade locust deliveryhero/locust \
              --namespace perf --create-namespace \
              --set worker.replicaCount=10
            kubectl rollout status deployment/locust-worker -n perf --timeout=5m
    
    - name: run-locust
      container:
        image: bitnami/kubectl:latest
        command: [sh, -c]
        args:
          - |
            kubectl exec -n perf deployment/locust-master -- \
              locust -f /home/locust/locustfile.py \
                --headless \
                -u 5000 -r 100 \
                --run-time 30m \
                --csv /tmp/result
    
    - name: analyze-results
      container:
        image: python:3.11-slim
        command: [python, -c]
        args:
          - |
            import pandas as pd
            df = pd.read_csv('/tmp/result_stats.csv')
            print(df.to_markdown())
            if df['Failure %'].mean() > 1:
                exit(1)
```

## 监控与告警

### PrometheusRule 压测告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: locust-alerts
  namespace: monitoring
spec:
  groups:
    - name: locust.rules
      rules:
        # 压测进行中
        - alert: LocustTestRunning
          expr: |
            locust_active_users > 0
          for: 1m
          labels:
            severity: info
          annotations:
            summary: "Locust 压测正在进行，当前用户数: {{ $value }}"

        # Worker 资源不足
        - alert: LocustWorkerHighCPU
          expr: |
            rate(container_cpu_usage_seconds_total{pod=~"locust-worker.*"}[5m]) > 0.8
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Locust Worker CPU 使用率超过 80%，结果可能不准确"

        # 压测失败率过高
        - alert: LocustHighErrorRate
          expr: |
            rate(locust_errors_total[1m]) / rate(locust_requests_total[1m]) > 0.05
          for: 2m
          labels:
            severity: warning
          annotations:
            summary: "Locust 压测失败率超过 5%"
```

### Grafana Dashboard

```json
{
  "dashboard": {
    "title": "Locust 压测监控",
    "panels": [
      {
        "title": "活跃用户数",
        "type": "graph",
        "targets": [
          { "expr": "locust_active_users" }
        ]
      },
      {
        "title": "RPS",
        "type": "graph",
        "targets": [
          { "expr": "rate(locust_requests_total[1m])" }
        ]
      },
      {
        "title": "P99 延迟",
        "type": "graph",
        "targets": [
          { "expr": "histogram_quantile(0.99, rate(locust_request_duration_seconds_bucket[1m]))" }
        ]
      },
      {
        "title": "错误率",
        "type": "graph",
        "targets": [
          { "expr": "rate(locust_errors_total[1m]) / rate(locust_requests_total[1m])" }
        ]
      }
    ]
  }
}
```

## 故障排查

### 常见问题诊断

| 问题 | 可能原因 | 解决方案 |
|-----|---------|----------|
| Worker 无法连接 Master | 网络策略 | 检查 NetworkPolicy，允许 5557 端口 |
| 压测 RPS 上不去 | Worker CPU 瓶颈 | 增加 Worker 副本数或 CPU |
| 结果不准确 | greenlet 限制 | 减少每 Worker 用户数，增加 Worker 数 |
| Master OOM | 用户数过多 | 增加 Master 内存 |
| 脚本错误 | Python 依赖 | 检查 requirements.txt 是否完整 |

### 调试命令

```bash
# 🟢 低风险：查看 Locust Pod 状态
kubectl get pods -n perf -l app=locust

# 🟢 低风险：查看 Master 日志
kubectl logs -n perf -l app=locust-master --tail=100

# 🟢 低风险：查看 Worker 日志
kubectl logs -n perf -l app=locust-worker --tail=100

# 🟢 低风险：查看 Worker 资源使用
kubectl top pods -n perf -l app=locust-worker

# 🟢 低风险：访问 Locust Web UI
kubectl port-forward -n perf svc/locust-master 8089:8089
# 浏览器访问 http://localhost:8089

# 🟢 低风险：检查 Prometheus 指标
curl -s http://locust-master.perf:9090/metrics | grep locust_
```

## 相关

- [[可靠性/性能测试/02-k6-load-testing-k8s.md|02 k6 load testing k8s]]
- [[可靠性/性能测试/01-load-testing-methodology.md|01 load testing methodology]]
- [[可靠性/性能测试/04-production-load-testing-playbook.md|04 production load testing playbook]]

<!-- risk-assessed -->
