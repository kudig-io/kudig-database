---
title: 蓝绿部署
summary: 蓝绿部署（Blue-Green Deployment）是一种发布策略，通过维护两套完全相同的生产环境（蓝环境和绿环境），实现应用的瞬时切换与零停机发布。
category: concepts
tags:
- core-concept
- 发布变更
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-07
last_updated: 2026-07
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
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

## 技术深度解析

### Service Selector 切换机制

蓝绿部署的核心是 Service 的 label selector 原子切换。Kubernetes Service 通过 selector 匹配后端 Pod，修改 selector 后，Endpoints Controller 会立即更新 Endpoints 对象：

```
切换前:
  Service selector: {app: my-app, version: blue}
  Endpoints: [pod-blue-1, pod-blue-2, pod-blue-3]

切换后 (kubectl patch selector):
  Service selector: {app: my-app, version: green}
  Endpoints Controller 检测 selector 变更
  → 移除 blue Pod endpoints
  → 添加 green Pod endpoints
  Endpoints: [pod-green-1, pod-green-2, pod-green-3]
```

**关键细节**：Endpoints 更新有时间窗口（kube-proxy 同步间隔），切换瞬间可能有少量已有连接仍在 blue 环境——需要配合 graceful shutdown 处理。

### 与 Ingress/Service Mesh 的集成

```yaml
# Istio VirtualService 实现蓝绿切换（更平滑）
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: my-app-vs
spec:
  http:
  - route:
    - destination:
        host: my-app-blue
        weight: 0                    # 蓝环境流量归零
    - destination:
        host: my-app-green
        weight: 100                   # 绿环境承载全部流量
```

Service Mesh 方式可以做到更平滑的权重迁移（100→0 渐进切换），而非 Service selector 的瞬时切换。

## 生产实现完整示例

```yaml
# 蓝环境（当前生产版本）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app-blue
spec:
  replicas: 3
  selector:
    matchLabels:
      app: my-app
      version: blue
  template:
    metadata:
      labels:
        app: my-app
        version: blue
    spec:
      containers:
      - name: app
        image: my-app:v1.0
        readinessProbe:
          httpGet: {path: /health, port: 8080}
---
# 绿环境（新版本，待切换）
apiVersion: apps/v1
kind: Deployment
metadata:
  name: my-app-green
spec:
  replicas: 3
  selector:
    matchLabels:
      app: my-app
      version: green
  template:
    metadata:
      labels:
        app: my-app
        version: green
    spec:
      containers:
      - name: app
        image: my-app:v2.0
        readinessProbe:
          httpGet: {path: /health, port: 8080}
---
# Service（当前指向蓝环境）
apiVersion: v1
kind: Service
metadata:
  name: my-app
spec:
  selector:
    app: my-app
    version: blue              # 切换为 green 即可完成蓝绿切换
  ports:
  - port: 80
    targetPort: 8080
```

## 数据库迁移挑战

蓝绿部署最大的技术挑战是**数据库 schema 变更**——蓝绿环境通常共享同一个数据库：

```
兼容性策略:
  Phase 1 (蓝运行 v1.0):
    → DB schema: v1
    
  Phase 2 (部署绿 v2.0，蓝绿并行):
    → DB schema: v2（向后兼容）
    → v2.0 代码同时兼容 v1 和 v2 schema
    → 执行 schema 迁移（ADD COLUMN，不删列）
    
  Phase 3 (切换到绿):
    → 绿 v2.0 使用 v2 schema
    → 蓝保留但不再接收流量
    
  Phase 4 (清理):
    → 确认绿稳定后删除蓝
    → 下一版本可删除 v1 兼容代码
```

## 最佳实践

- **绿环境部署后必须预验证**：绿环境 Pod 全部 Ready 后，通过独立 Service 或 `kubectl port-forward` 直接测试，确认功能正常后再切换
- **保留蓝环境至少 1 小时**：切换后不要立即删除蓝环境 Deployment，保留作为快速回滚目标
- **使用 ArgoCD 管理蓝绿切换**：将蓝绿版本号纳入 GitOps，通过 Git PR 审核切换决策
- **数据库变更必须向后兼容**：蓝绿共享数据库时，schema 变更必须遵循"扩展而非修改"原则（先加列，后续再删列）
- **监控切换瞬间指标**：切换后立即观察错误率、P99 延迟、业务指标——配置 1 分钟高灵敏度告警窗口

## 常见陷阱

- **切换瞬间请求丢失**：Service selector 切换时，已有连接可能被中断——需要配置 Pod 的 graceful shutdown 和 preStop hook
- **数据库 schema 不兼容**：蓝版本代码不兼容绿版本 schema（或反之），导致回滚后服务不可用——必须使用兼容性扩展策略
- **Session 粘滞问题**：用户 session 绑定在蓝环境 Pod，切换后 session 丢失——需要使用外部 session 存储（Redis）

更多部署排错方法请参考 [[19-故障诊断/04-高级排障/05-workloads/02-deployment-troubleshooting.md|deployment-troubleshooting]]，其他部署策略参见 [[22-概念/09-平台与发布/canary-deployment.md|canary-deployment]]、[[22-概念/02-工作负载/deployments.md|deployment-strategies]]。


## 参见

- [[kubernetes]] — core-concept 领域核心页面

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
