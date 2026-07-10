---
title: 渐进式交付生产模式
description: 生产级渐进式发布：Canary 金丝雀、蓝绿部署、特性开关与 Argo Rollouts 自动化回滚实践
summary: 生产级渐进式发布：Canary 金丝雀、蓝绿部署、特性开关与 Argo Rollouts 自动化回滚实践，含发布安全门控与回滚清单。
category: application-patterns
tags:
- canary
- blue-green
- progressive-delivery
- argo-rollouts
- feature-flags
- production
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 应用开发者
estimated_read_time: 16min
intent_queries:
- K8s 渐进式交付是什么
- 如何做 Canary 金丝雀发布
trigger_keywords:
- Canary
- 金丝雀
- 蓝绿部署
- Argo Rollouts
- 特性开关
prerequisites:
- kubectl-basics
- deployment-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含发布操作。执行前确认回滚方案已就绪，发布窗口符合变更冻结策略。命令风险等级标注：🔴 高风险、🟡 中风险、🟢 低风险。

# 渐进式交付生产模式

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产模式参考

渐进式交付通过逐步将流量切到新版本，在异常时快速回滚，是降低发布风险的核心手段。全量一次性发布（`kubectl set image` + 等待 rollout）在版本有缺陷时会全量受影响。本文涵盖 Canary 金丝雀、蓝绿部署、特性开关和 Argo Rollouts 自动化实践。

---

## 1. 策略对比

| 策略 | 流量切换 | 资源开销 | 回滚速度 | 自动化程度 | 适用场景 |
|---|---|---|---|---|---|
| **RollingUpdate** | 逐批替换 | 低（maxSurge） | 慢（需反向 rollout） | 原生 | 常规迭代 |
| **Canary** | 按比例小流量先行 | 中（双版本并存） | 快（流量切回） | Argo Rollouts | 高风险变更 |
| **蓝绿** | 100% 瞬间切换 | 高（完整双环境） | 极快（切回旧环境） | 中等 | 数据库迁移、大版本升级 |
| **特性开关** | 代码级控制 | 低 | 极快（开关关闭） | 应用层 | 功能级灰度、A/B 测试 |

---

## 2. Canary 金丝雀（Argo Rollouts）

### 2.1 为什么不用原生 Deployment

原生 Deployment 的 RollingUpdate 是"全或无"——一旦开始就无法在 5% 流量时停下观察。Argo Rollouts 引入 `Rollout` CRD，支持基于指标的自动渐进和回滚。

### 2.2 生产 Rollout 模板

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: api-server
spec:
  replicas: 10
  strategy:
    canary:
      canaryService: api-canary       # 金丝雀 Service
      stableService: api-stable       # 稳定版 Service
      trafficRouting:
        istio:
          virtualService:
            name: api-vsvc            # Istio VirtualService 控制流量权重
            routes: [primary]
      steps:
        - setWeight: 5                # 5% 流量到新版本
        - pause: { duration: 10m }    # 观察 10 分钟
        - analysis:                   # 自动分析指标
            templates:
              - templateName: success-rate
            args:
              - name: service-name
                value: api-canary
        - setWeight: 25               # 通过后 25%
        - pause: { duration: 10m }
        - setWeight: 50
        - pause: { duration: 5m }
        - setWeight: 100              # 全量
```

### 2.3 自动分析门控

```yaml
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: success-rate
spec:
  args:
    - name: service-name
  metrics:
    - name: success-rate
      interval: 30s
      successCondition: result[0] >= 0.99   # 成功率 ≥ 99%
      failureLimit: 3                        # 连续 3 次失败 → 自动回滚
      provider:
        prometheus:
          address: http://prometheus:9090
          query: |
            sum(rate(http_requests_total{service="{{args.service-name}}",code!~"5.."}[2m]))
            / sum(rate(http_requests_total{service="{{args.service-name}}"}[2m]))
```

> ⚠️ **关键设计**: `failureLimit` 触发自动回滚，无需人工干预。确保分析窗口（`interval`）足够长以避免噪声，但足够短以快速止损。生产建议 30s interval + 3 failureLimit = 90s 内自动回滚。

### 2.4 回滚操作

```bash
# 🟡 中风险：手动回滚到上一稳定版
kubectl argo rollouts undo rollout/api-server

# 🟢 查看发布状态
kubectl argo rollouts get rollout api-server --watch

# 🟢 查看发布历史
kubectl argo rollouts history rollout/api-server
```

---

## 3. 蓝绿部署

### 3.1 适用场景

蓝绿部署维护两套完整环境（蓝=当前版，绿=新版），通过 Service selector 切换流量。适合：
- **数据库 schema 迁移**：需要新旧版本短暂并存验证
- **大版本升级**：回滚必须瞬间完成
- **不可逆变更**：Canary 无法逐步回退的场景

### 3.2 切换操作

```bash
# 🟢 当前: Service 指向蓝环境 (slot: blue)
kubectl get svc api -o jsonpath='{.spec.selector.slot}'

# 🟡 中风险：切换到绿环境
kubectl patch svc api -p '{"spec":{"selector":{"slot":"green"}}}'

# 🔴 回滚: 切回蓝（瞬间完成，因为蓝环境仍在运行）
kubectl patch svc api -p '{"spec":{"selector":{"slot":"blue"}}}'
```

> ⚠️ **资源成本**: 蓝绿需要双倍资源。生产中常用于关键服务的特定窗口（如季度大版本），非常态使用。

---

## 4. 特性开关 (Feature Flags)

### 4.1 与 Canary 的互补

| 维度 | Canary (流量级) | 特性开关 (功能级) |
|---|---|---|
| 控制粒度 | 整个版本流量百分比 | 单个功能 on/off |
| 回滚粒度 | 回滚整个版本 | 关闭单个功能 |
| 适用场景 | 版本稳定性验证 | 功能级灰度、A/B 测试 |
| 实现层 | 流量管理层（Istio/Argo） | 应用代码层（OpenFeature/LaunchDarkly） |

### 4.2 生产建议

- **高风险新功能**：特性开关 + Canary 双保险——先 Canary 验证版本稳定性，再用开关逐步开放功能
- **开关生命周期**：特性开关应有 TTL，功能全量后及时移除，避免开关债务
- **默认安全**：开关默认关闭，验证后才打开；远端开关服务不可用时 fallback 到关闭态

---

## 5. 发布安全门控清单

| # | 门控项 | 验证方法 | 合格标准 |
|---|---|---|---|
| 1 | 非生产环境验证通过 | CI 流水线记录 | Staging 已通过冒烟测试 |
| 2 | Canary 自动分析已配置 | 检查 AnalysisTemplate | 成功率/延迟指标门控已生效 |
| 3 | 回滚方案已验证 | 预案演练记录 | 回滚可在 90s 内完成 |
| 4 | 变更窗口合规 | 审批工单 | 非变更冻结期、非高峰时段 |
| 5 | 监控大盘就绪 | Grafana Dashboard | 发布前后关键指标可对比 |
| 6 | 通知已发送 | Slack/钉钉 | 值班已知悉发布计划 |

---

## 6. 排障速查

| 症状 | 可能根因 | 修复 |
|---|---|---|
| Rollout 卡在某步骤不推进 | AnalysisTemplate 指标无数据 / pause 未到时间 | 检查 Prometheus 查询 + `kubectl argo rollouts get` |
| Canary 流量切不过去 | VirtualService 配置错 / Service selector 不匹配 | 检查 trafficRouting 配置 + Endpoints |
| 自动回滚触发 | 新版本成功率 < 阈值 | 查看 `kubectl argo rollouts history` 分析失败指标 |
| 蓝绿切换后 502 | 绿环境 Pod 未就绪 | 确认 readinessProbe 通过后再切 selector |

---

## 7. 跨域协作

- **GitOps 发布管理**: 见 `domain-08-release-change-management/01-gitops/`
- **Argo Rollouts 深入**: 见 `domain-08-release-change-management/01-gitops/`
- **SLO 与错误预算**: 见 `domain-06-observability/99-slo-operations-guide.md`


<!-- risk-assessed -->
