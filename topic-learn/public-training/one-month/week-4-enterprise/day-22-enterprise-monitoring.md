# Day 22: 企业监控 - Prometheus 企业级 + Grafana

> **学习时间**: 4-5 小时 | **主题**: 企业级监控体系

---

## 今日目标

- [ ] 了解 Thanos 跨集群监控方案
- [ ] 设计 SLO/SLI 监控体系
- [ ] 配置高级 Grafana Dashboard

---

## 理论学习 (2h)

### 必读文档

1. **Prometheus 企业级监控**
   - 文件: `../../domain-20-enterprise-monitoring-alerting/01-prometheus-enterprise-monitoring.md`
   - 重点: 高可用部署、长期存储

2. **Grafana 企业级可观测性**
   - 文件: `../../domain-20-enterprise-monitoring-alerting/02-grafana-enterprise-observability.md`
   - 重点: Dashboard 设计、告警集成

3. **SLO/SLI 体系**
   - 文件: `../../domain-8-observability/18-slo-sli-system.md`
   - 重点: 错误预算、SLO 告警

---

## 实践任务 (2.5h)

### 任务 1: SLO/SLI 设计 (1h)

```bash
# SLI (Service Level Indicator) 定义
# 可用性 SLI: 成功请求比例
# (sum(rate(http_requests_total{status!~"5.."}[5m])) / sum(rate(http_requests_total[5m]))) * 100

# 延迟 SLI: P99 延迟
# histogram_quantile(0.99, sum(rate(http_request_duration_seconds_bucket[5m])) by (le))

# SLO (Service Level Objective) 目标
# 可用性 SLO: 99.9%
# 延迟 SLO: P99 < 500ms

# 创建 SLO 告警规则
cat > slo-rules.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: slo-alerts
  namespace: monitoring
spec:
  groups:
  - name: slo-availability
    rules:
    - alert: HighErrorRate
      expr: |
        (
          sum(rate(http_requests_total{status=~"5.."}[5m]))
          /
          sum(rate(http_requests_total[5m]))
        ) > 0.001
      for: 5m
      labels:
        severity: critical
        slo: availability
      annotations:
        summary: "Error rate > 0.1% (SLO: 99.9%)"
        error_budget_consumed: "{{ $value | humanizePercentage }}"
EOF

kubectl apply -f slo-rules.yaml
```

### 任务 2: 高级 Grafana Dashboard (1h)

设计包含以下面板的 Dashboard:

1. **黄金信号面板**
   - 请求率 (Rate)
   - 错误率 (Errors)
   - 延迟 (Duration)
   - 饱和度 (Saturation)

2. **SLO 面板**
   - 当前 SLO 达成率
   - 错误预算消耗
   - 30 天趋势

3. **资源面板**
   - CPU 使用率
   - 内存使用率
   - 网络流量

### 任务 3: 告警优化 (30min)

```bash
# 告警静默
# 在 Alertmanager 中配置维护窗口

# 告警抑制
# 当 critical 告警触发时，抑制相关 warning

# 配置告警通道
# 钉钉/企微/Slack webhook
```

---

## 费曼复述 (0.5h)

1. **什么是 SLO/SLI/SLA？三者的关系？**
2. **错误预算是什么？如何指导决策？**
3. **Thanos 如何实现跨集群查询？**

---

## 今日检验

- [ ] 理解 SLO/SLI 概念
- [ ] 能够设计 SLO 告警规则
- [ ] 能够创建高质量 Grafana Dashboard
