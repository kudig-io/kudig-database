---
title: 金丝雀发布
summary: 金丝雀发布（Canary Deployment）是一种渐进式发布策略，通过将少量生产流量导入新版本，在真实环境中验证新版本的稳定性，再逐步扩大流量比例直至全量发布。
category: concepts
tags:
- core-concept
- domain-08-release-change-management
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---



# 金丝雀发布

金丝雀发布（Canary Deployment）是一种渐进式发布策略，通过将少量生产流量导入新版本，在真实环境中验证新版本的稳定性，再逐步扩大流量比例直至全量发布。

## 核心原理

金丝雀发布的名称源于历史上矿工使用金丝雀检测有毒气体的做法。其基本流程为：

1. 部署新版本，仅导入少量流量（如 1% 或 5%）
2. 观察新版本的关键指标
3. 若指标正常，逐步增加流量比例（10% → 25% → 50% → 100%）
4. 若指标异常，立即停止推广并回滚

这种方式将发布风险控制在最小范围内，是新版本在生产环境中验证的黄金标准。

## Kubernetes 实现方式

在 Kubernetes 生态中，金丝雀发布有多种实现路径：

### 原生方式

通过两个 Deployment（稳定版和金丝雀版）共享同一个 Service，手动调整副本数来控制流量比例：

- 稳定版 Deployment：9 个副本
- 金丝雀版 Deployment：1 个副本
- 总流量比例约为 9:1

这种方式简单直接，但粒度受限于 Pod 数量，且切换不够灵活。

### Ingress 方式

利用 Ingress Controller 的流量分割能力实现精细控制：

- **Nginx Ingress**：通过 `canary-by-header`、`canary-weight` 等 annotation 控制
- **ALB Ingress**：支持基于权重的路由规则，可直接设置百分比

Ingress 方式无需修改 Service 或 Deployment，仅需调整 Ingress 资源即可。

### Service Mesh 方式

通过 Istio 等 Service Mesh 实现最精细的流量控制：

```yaml
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
spec:
  http:
    - route:
        - destination:
            host: my-app-stable
          weight: 90
        - destination:
            host: my-app-canary
          weight: 10
```

Service Mesh 支持基于 HTTP 头、Cookie、用户身份等维度的路由，灵活性最高。更多演进路线参见 [[service-mesh-evolution]]。

## 关键监控指标

金丝雀发布期间必须关注以下指标：

| 指标类别 | 具体指标 | 异常阈值参考 |
|----------|----------|-------------|
| 错误率 | HTTP 5xx 比例 | 较基线上升 > 0.1% |
| 延迟 | P99 响应时间 | 较基线上升 > 20% |
| 吞吐量 | QPS/TPS | 较基线下降 > 10% |
| 业务指标 | 订单成功率、转化率 | 较基线下降 > 5% |
| 资源使用 | CPU、内存、GC | 较基线上升 > 30% |

## 远程顾问指导要点

作为远程顾问，指导现场执行金丝雀发布时应注意：

- **设置初始比例**：建议从 1% 或 5% 开始，确保即使出现问题影响面也极小
- **选择流量特征**：优先将内部用户、非关键业务流量或特定地域流量导入金丝雀版本
- **定义明确的决策标准**：提前确定错误率、延迟等指标的红线，避免主观判断
- **设定观察时长**：每个阶段至少观察 15-30 分钟，确保覆盖正常业务周期
- **准备一键回滚**：金丝雀版本出现异常时，能够快速将流量比例归零或切回稳定版本
- **渐进推广节奏**：建议按 5% → 10% → 25% → 50% → 100% 的节奏推进，每个阶段充分验证

更多部署排错方法请参考 [[domain-10-troubleshooting-diagnostics/高级排障/05-workloads/02-deployment-troubleshooting.md|deployment-troubleshooting]]。


## 参见

- [[kubernetes]] — core-concept 领域核心页面

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
