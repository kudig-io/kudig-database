---
title: 生产环境压测规范
description: '生产环境安全压测的完整规范：安全边界、流量录制回放、渐进式加压与清理流程'
summary: '生产环境安全压测的完整规范：安全边界、流量录制回放、渐进式加压与清理流程'
category: reliability-engineering
tags:
- performance-testing
- production
- load-testing
- goreplay
- traffic-replay
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- 生产环境压测规范 是什么
- 如何在生产环境安全压测
trigger_keywords:
- production
- load-testing
- goreplay
- tcpcopy
- canary
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 生产环境压测规范

## 1. 概述

生产环境压测（Production Load Testing）是在真实生产集群上验证系统容量和稳定性的最终手段。与预发布环境不同，生产压测面临数据安全、用户体验和服务可用性的多重约束。本手册定义安全边界、工具选型、执行流程和清理规范。

核心原则：
- **安全第一**：所有操作可回滚，绝不影响真实用户流量
- **渐进可控**：从 1% 流量开始，逐步放大，每步可观测
- **数据隔离**：压测数据与生产数据完全隔离，压测后彻底清理

## 2. 安全边界定义

### 2.1 红线清单

| 红线 | 说明 | 违反后果 |
|------|------|---------|
| 禁止修改生产数据库 | 不得执行 INSERT/UPDATE/DELETE | 立即终止 |
| 禁止调用外部支付接口 | 压测请求不得触发真实扣款 | 立即终止 |
| 禁止发送真实通知 | 压测数据不得触发短信/邮件/推送 | 立即终止 |
| 禁止超过容量阈值 | CPU < 80%, 内存 < 85%, 磁盘 I/O < 70% | 自动降压 |
| 禁止跨区域压测 | 压测流量限制在同一可用区 | 立即终止 |

### 2.2 准入条件

生产压测前必须满足：

```yaml
# pre-flight checklist
prerequisites:
  - name: 容量评估完成
    owner: SRE
    gate: true
    check: |
      当前集群资源使用率 < 60%
      目标服务副本数 >= 日常 2 倍
      
  - name: 监控告警就绪
    owner: SRE
    gate: true
    check: |
      Prometheus + Grafana 看板已部署
      关键指标告警阈值已调整为压测模式
      
  - name: 回滚方案就绪
    owner: SRE
    gate: true
    check: |
      流量切换方案已验证
      HPA 扩容上限已临时调整
      
  - name: 数据隔离确认
    owner: 开发
    gate: true
    check: |
      压测标识 Header 已注入
      下游服务已识别并隔离压测数据
```

### 2.3 压测标识体系

所有压测流量必须携带统一标识，供下游服务识别和隔离：

```yaml
# 压测 Header 规范
headers:
  X-Load-Test: "true"              # 压测标识
  X-Load-Test-Id: "lt-20260702-01" # 压测批次号
  X-Load-Test-Round: "3"           # 第几轮
```

```go
// Go 服务中间件示例
func LoadTestFilter(next http.Handler) http.Handler {
    return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        if r.Header.Get("X-Load-Test") == "true" {
            // 标记上下文为压测流量
            ctx := context.WithValue(r.Context(), ctxKeyLoadTest, true)
            r = r.WithContext(ctx)
            
            // 跳过写操作
            if r.Method != http.MethodGet && r.Method != http.MethodHead {
                w.WriteHeader(http.StatusOK)
                w.Write([]byte(`{"status":"skipped","reason":"load-test"}`))
                return
            }
        }
        next.ServeHTTP(w, r)
    })
}
```

## 3. 流量录制回放

### 3.1 Goreplay 方案

Gor（Goreplay）是生产级流量录制回放工具，支持实时镜像和延迟回放。

#### 录制阶段

```bash
# 在 Ingress 节点录制生产流量
gor --input-raw :80 \
  --output-file ./traffic-recordings/2026-07-02-%Y-%m-%d_%H-%M.gz \
  --output-file-append \
  --split-output \
  --file-size-limit 1G

# 只录制特定路径
gor --input-raw :80 \
  --http-allow-path '/api/v[12]/' \
  --output-file ./traffic.gz
```

#### 回放阶段

```bash
# 1x 速率回放（基线）
gor --input-file ./traffic.gz \
  --output-http "http://target-service:80" \
  --output-http-workers 10 \
  --output-http-track-response \
  --stats --output-http-stats-min 30

# 2x 速率回放（倍压）
gor --input-file ./traffic.gz \
  --output-http "http://target-service:80" \
  --input-file-multiplier 2 \
  --output-http-workers 20

# 注入压测 Header
gor --input-file ./traffic.gz \
  --output-http "http://target-service:80" \
  --http-set-header 'X-Load-Test: true' \
  --http-set-header 'X-Load-Test-Id: lt-20260702-01'
```

### 3.2 tcpcopy 方案

tcpcopy 工作在 TCP 层，对应用透明，适合无法修改 Header 的场景。

```bash
# 在 Ingress 服务器（录制端）
modprobe ip_queue
tcpcopy -x 80-8080@target-server-ip

# 在目标服务器（回放端）
intercept -i eth0 -F 'tcp and port 8080'
```

### 3.3 方案选型对比

| 维度 | Goreplay | tcpcopy |
|------|----------|---------|
| 工作层 | HTTP 层 | TCP 层 |
| 流量过滤 | 支持 Path/Header 过滤 | 仅端口级 |
| 速率控制 | 支持倍速/降速 | 需配合 tc 控制 |
| Header 注入 | 原生支持 | 不支持 |
| 部署复杂度 | 低 | 高（需内核模块） |
| 推荐场景 | HTTP API 压测 | 全栈 TCP 压测 |

## 4. 渐进式加压策略

### 4.1 四阶段加压模型

```
阶段 1: 影子验证（Shadow Validation）
  │  流量比例: 1% 压测 + 99% 生产
  │  持续时间: 30 分钟
  │  目标: 验证压测链路正确性，确认指标采集正常
  │  退出条件: 无错误日志、指标数据完整
  │
  ▼
阶段 2: 轻度加压（Light Load）
  │  流量比例: 10% 压测
  │  持续时间: 30 分钟
  │  目标: 验证服务水平未明显下降
  │  退出条件: P99 延迟增幅 < 10%，错误率 < 0.1%
  │
  ▼
阶段 3: 中度加压（Medium Load）
  │  流量比例: 50% 压测
  │  持续时间: 30 分钟
  │  目标: 验证系统在 1.5x 负载下的表现
  │  退出条件: P99 延迟增幅 < 20%，CPU < 75%
  │
  ▼
阶段 4: 峰值压测（Peak Load）
  │  流量比例: 100% 压测（2x 正常流量）
  │  持续时间: 15 分钟
  │  目标: 验证系统极限容量
  │  退出条件: 触发自动降压或达到目标 QPS
```

### 4.2 自动降压机制

```yaml
# 压测自动降压规则
auto-throttle:
  rules:
    - metric: container_cpu_usage_percent
      threshold: 80
      action: reduce_50_percent
      
    - metric: apiserver_request_duration_p99
      threshold: 2000    # 2s
      action: reduce_50_percent
      
    - metric: container_oom_killed_total
      threshold: 1
      action: stop_immediately
      
    - metric: pod_crash_loop_count
      threshold: 3
      action: stop_immediately
```

### 4.3 K8s 资源调整

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 压测前临时调整 HPA 上限
kubectl patch hpa my-service -n production \
  --type='json' -p='[{"op":"replace","path":"/spec/maxReplicas","value":50}]'

# 压测后恢复
kubectl patch hpa my-service -n production \
  --type='json' -p='[{"op":"replace","path":"/spec/maxReplicas","value":10}]'
```
## 5. 压测指标采集与分析

### 5.1 核心指标体系

```yaml
# 压测期间重点关注的指标
metrics:
  application:
    - name: http_request_duration_seconds
      aggregation: [p50, p95, p99, max]
      alert_threshold_p99: 2000ms
      
    - name: http_request_total
      aggregation: [rate]
      alert_threshold: 目标 QPS
      
    - name: http_requests_errors_total
      aggregation: [rate, ratio]
      alert_threshold_ratio: 0.1%

  infrastructure:
    - name: container_cpu_usage_seconds_total
      aggregation: [rate, max]
      alert_threshold: 80%
      
    - name: container_memory_working_set_bytes
      aggregation: [max]
      alert_threshold: 85%
      
    - name: node_network_transmit_bytes_total
      aggregation: [rate]
      alert_threshold: 网卡容量 80%

  kubernetes:
    - name: apiserver_request_duration_seconds
      aggregation: [p99]
      alert_threshold: 1000ms
      
    - name: scheduler_scheduling_algorithm_duration_seconds
      aggregation: [p99]
      alert_threshold: 500ms
```

### 5.2 Grafana 压测看板

```json
{
  "dashboard": {
    "title": "Production Load Test Dashboard",
    "panels": [
      {
        "title": "QPS vs Latency",
        "targets": [
          {
            "expr": "sum(rate(http_request_total{X-Load-Test!=\"true\"}[1m]))",
            "legendFormat": "生产 QPS"
          },
          {
            "expr": "sum(rate(http_request_total{X-Load-Test=\"true\"}[1m]))",
            "legendFormat": "压测 QPS"
          }
        ]
      },
      {
        "title": "P99 延迟对比",
        "targets": [
          {
            "expr": "histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[1m])) by (le))",
            "legendFormat": "P99"
          }
        ]
      }
    ]
  }
}
```

### 5.3 压测报告模板

```markdown
## 压测报告 - [日期]

### 基本信息
- 集群: [名称], K8s 版本: [版本]
- 节点: [数量] x [机型]
- 目标服务: [服务名], 副本数: [当前/最大]

### 测试结果
| 阶段 | QPS | P50 | P99 | 错误率 | CPU峰值 | 内存峰值 |
|------|-----|-----|-----|--------|---------|---------|
| 基线 | | | | | | |
| 10% | | | | | | |
| 50% | | | | | | |
| 100% | | | | | | |

### 结论
- 容量评估: [是否满足 N 倍日常峰值]
- 瓶颈分析: [主要瓶颈组件]
- 建议: [扩容/优化建议]
```

## 6. 压测后清理流程

### 6.1 清理检查清单

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# post-loadtest-cleanup.sh

set -euo pipefail

TEST_ID="${1:?Usage: $0 <load-test-id>}"
NAMESPACE="production"

echo "=== 压测清理开始: ${TEST_ID} ==="

# 1. 确认压测流量已停止
echo "[1/6] 检查压测流量..."
LT_QPS=$(curl -s "http://prometheus:9090/api/v1/query" \
  --data-urlencode 'query=sum(rate(http_request_total{X-Load-Test="true"}[1m]))' \
  | jq -r '.data.result[0].value[1] // "0"')

if (( $(echo "$LT_QPS > 0.1" | bc -l) )); then
  echo "ERROR: 压测流量仍在发送 (QPS: ${LT_QPS})"
  exit 1
fi

# 2. 恢复 HPA 配置
echo "[2/6] 恢复 HPA 上限..."
kubectl patch hpa my-service -n ${NAMESPACE} \
  --type='json' -p='[{"op":"replace","path":"/spec/maxReplicas","value":10}]'

# 3. 恢复告警阈值
echo "[3/6] 恢复告警阈值..."
kubectl apply -f alerting/production-rules.yml

# 4. 清理压测数据（写操作产生的数据）
echo "[4/6] 清理压测写入数据..."
# 按压测标识删除测试数据
curl -X POST "http://cleanup-service/internal/purge" \
  -H "Content-Type: application/json" \
  -d "{\"testId\": \"${TEST_ID}\", \"dryRun\": false}"

# 5. 归档压测结果
echo "[5/6] 归档压测结果..."
mkdir -p "archive/${TEST_ID}"
cp -r collected-metrics/ "archive/${TEST_ID}/"
cp post-test-report.md "archive/${TEST_ID}/"

# 6. 验证集群健康
echo "[6/6] 验证集群健康..."
kubectl get nodes -o wide
kubectl top pods -n ${NAMESPACE} --sort-by=cpu | head -20
kubectl get events -n ${NAMESPACE} --sort-by='.lastTimestamp' | tail -20

echo "=== 压测清理完成: ${TEST_ID} ==="
```
### 6.2 清理后验证

```yaml
# 验证项目清单
post_cleanup_verify:
  - name: 压测流量归零
    check: prometheus_query('sum(rate(http_request_total{X-Load-Test="true"}[5m]))') == 0
    
  - name: 生产指标恢复正常
    check: prometheus_query('http_request_duration_seconds:p99') < baseline_p99 * 1.1
    
  - name: 无残余压测资源
    check: kubectl get pods -A -l load-test-id | wc -l == 0
    
  - name: 集群资源正常
    check: kubectl top nodes | all_nodes_cpu < 60%
    
  - name: 告警全部恢复
    check: alertmanager_active_alerts == 0
```

## 7. 最佳实践

### 7.1 时间窗口选择

- 选择业务低峰期（凌晨 2-6 点或周末）
- 提前通知相关团队
- 安排至少 2 名工程师值班

### 7.2 一键启停脚本

```bash
# 压测启动（需双人确认）
./loadtest-start.sh --test-id lt-20260702-01 --confirm-by user1,user2

# 紧急停止
./loadtest-stop.sh --test-id lt-20260702-01 --reason "manual-stop"
```

## Related

- [[01-load-testing-methodology|负载测试方法论]]
- [[04-benchmarking-methodology-kube-burner|kube-burner 基准测试]]

## See Also

- [Gor 官方文档](https://github.com/buger/goreplay)
- [tcpcopy 项目](https://github.com/wangbin579/tcpcopy)


<!-- risk-assessed -->
