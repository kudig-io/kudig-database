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

## 相关

- [[可靠性/性能测试/02-k6-load-testing-k8s.md|02 k6 load testing k8s]]
- [[可靠性/性能测试/01-load-testing-methodology.md|01 load testing methodology]]
- [[可靠性/性能测试/04-production-load-testing-playbook.md|04 production load testing playbook]]

<!-- risk-assessed -->
