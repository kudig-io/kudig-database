---
title: 蓝绿部署
summary: 蓝绿部署（Blue-Green Deployment）是一种发布策略，通过维护两套完全相同的生产环境（蓝环境和绿环境），实现应用的瞬时切换与零停机发布。
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



# 蓝绿部署

蓝绿部署（Blue-Green Deployment）是一种发布策略，通过维护两套完全相同的生产环境（蓝环境和绿环境），实现应用的瞬时切换与零停机发布。

## 核心原理

蓝绿部署的基本思路是：

1. **蓝环境**：当前正在对外提供服务的生产环境
2. **绿环境**：部署新版本但尚未接入流量的环境
3. **切换流量**：验证绿环境就绪后，将流量从蓝环境瞬间切换到绿环境
4. **回滚能力**：若出现问题，可立即切回蓝环境

在任意时刻，只有一个环境承载生产流量，但两个环境都处于运行状态。

## 与 RollingUpdate 的区别

| 维度 | RollingUpdate | 蓝绿部署 |
|------|--------------|----------|
| 发布方式 | 渐进式替换旧 Pod | 瞬时切换全部流量 |
| 资源占用 | 峰值约为原容量的 125%（由 maxSurge 控制） | 需要双倍资源 |
| 停机时间 | 理论上零停机 | 零停机 |
| 回滚速度 | 需要重新滚动 | 瞬间切回 |
| 版本共存 | 新旧版本同时服务 | 只有一个版本对外服务 |

蓝绿部署适用于对回滚速度要求极高、不接受多版本同时服务的场景。

## Kubernetes 实现方式

在 Kubernetes 中，蓝绿部署通常通过以下结构实现：

- **两个 Deployment**：`app-blue` 和 `app-green`，分别运行不同版本
- **一个 Service**：通过切换 `selector` 中的版本标签来决定流量指向

```yaml
# Service 示例：初始指向 blue
spec:
  selector:
    app: my-app
    version: blue
```

切换流量时，只需修改 Service 的 `selector`：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch service my-app -p '{"spec":{"selector":{"version":"green"}}}'
```

Service 的 selector 变更会立即生效，所有新请求将被路由到绿环境。

## 优缺点分析

**优点**：

- 零停机时间，用户体验无感知
- 回滚 instantaneous，切换 selector 即可恢复
- 发布前可在绿环境完成完整的功能验证
- 不存在新旧版本同时服务的兼容性问题

**缺点**：

- 需要双倍资源，成本较高
- 数据库 schema 变更需要额外处理（两个环境共享数据层）
- 未充分利用 Kubernetes 的原生滚动更新能力
- 会话保持需要考虑，切换瞬间的请求可能被中断

## 远程顾问指导要点

作为远程顾问，指导现场工程师执行蓝绿部署时，建议遵循以下步骤：

- **部署绿环境**：确认绿版本 Deployment 的所有 Pod 均已就绪（`Ready` 状态且通过健康检查）
- **预验证绿环境**：可通过独立 Service 或端口转发直接访问绿环境进行冒烟测试
- **执行流量切换**：修改生产 Service 的 `selector`，将流量从蓝环境切至绿环境
- **监控关键指标**：切换后立即观察错误率、响应延迟、业务指标是否异常
- **保留蓝环境**：在观察期内不要立即删除蓝环境 Deployment，确保随时可回滚
- **决策回滚或保留**：若观察期内指标正常，蓝环境可保留作为下次发布的绿环境；若异常，立即切回蓝环境并分析问题

更多部署排错方法请参考 [[domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/02-deployment-troubleshooting.md|deployment-troubleshooting]]，其他部署策略参见 [[concepts/deployments.md|deployment-strategies]]。


## 参见

- [[kubernetes]] — core-concept 领域核心页面

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub
