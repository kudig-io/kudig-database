---
title: 变更管理指南
summary: 变更管理指南：变更是生产环境不稳定的首要诱因。据统计，70% 以上的生产问题由变更引入。建立严格的变更管理流程，是保障系统稳定性的核心措施。
category: 生产运维
tags:
- domain-11
- 变更管理
- SRE
- 运维
- 灰度发布
- 回滚
- visibility/public
tier: core
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 变更管理指南

## 概述

变更是生产环境不稳定的首要诱因。据统计，70% 以上的生产问题由变更引入。建立严格的变更管理流程，是保障系统稳定性的核心措施。

## 变更类型分类

| 类型 | 定义 | 典型场景 | 风险等级 |
|---|---|---|---|
| 配置变更 | 修改 ConfigMap、Secret、环境变量 | 调整超时时间、更新证书 | 中 |
| 版本升级 | 应用镜像或基础设施组件版本更新 | 升级 Nginx Ingress、应用发版 | 高 |
| 扩缩容 | 调整副本数或节点规模 | 应对流量高峰、成本优化 | 低 |
| 架构调整 | 修改服务拓扑、引入新组件 | 拆分微服务、更换数据库 | 极高 |

## 变更风险评估矩阵

| 影响范围 \ 回滚难度 | 容易回滚 | 中等难度 | 难以回滚 |
|---|---|---|---|
| 单服务 | 低风险 | 中风险 | 高风险 |
| 多服务 | 中风险 | 高风险 | 极高风险 |
| 全局/核心 | 高风险 | 极高风险 | 禁止直接变更 |

> **禁止直接变更** 的场景必须通过蓝绿部署或影子流量验证后方可执行。

## 变更执行规范

### 四眼原则

所有高危及极高风险变更必须执行双人复核：
- 变更执行人：负责操作执行
- 变更审核人：负责方案审核与现场监督
- 双方需在变更单上签字确认

### 变更窗口

| 变更等级 | 时间窗口要求 |
|---|---|
| 低风险 | 任意时间，但避开业务高峰 |
| 中风险 | 非高峰时段（如夜间） |
| 高风险 | 预定维护窗口，提前 24h 公告 |
| 极高风险 | 需专项评审会批准，维护窗口执行 |

### 变更冻结期

- 重大节假日前 3 天至节后 1 天
- 促销活动期间（如双11）
- 冻结期内仅允许紧急问题修复变更

## 回滚策略

### 版本级回滚

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Deployment 回滚到上一版本
kubectl rollout undo deployment/my-app

# 回滚到指定版本
kubectl rollout undo deployment/my-app --to-revision=3
```
适用场景：镜像版本回退，回滚时间 < 30 秒。

### 配置级回滚

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 应用已保存的备份配置
kubectl apply -f configmap-backup.yaml
```
适用场景：ConfigMap/Secret 错误修改，需提前保存配置历史。

### 流量级回滚

- 金丝雀回滚：将流量权重立即切回旧版本（100% → 0%）
- Ingress 切流：修改 Ingress 规则指向旧版本 Service

适用场景：新版本的逻辑问题不影响部署状态，仅需切换流量。

### 回滚验证清单

- [ ] 旧版本 Pod 全部就绪（`kubectl get pods` 确认）
- [ ] 业务流量恢复（监控指标确认）
- [ ] 无新错误日志产生
- [ ] 用户投诉渠道无新增反馈

## 变更请求模板

```markdown
# 变更请求: CR-2026-XXXX

## 基本信息
- 提交人: <姓名>
- 日期: 2026-XX-XX
- 风险等级: 低/中/高/极高
- 变更类型: 配置/版本/扩缩容/架构
- 影响服务: <服务列表>
- 计划窗口: 2026-XX-XX HH:MM - HH:MM

## 变更描述
<具体变更内容>

## 影响分析
- 上游依赖: <调用方>
- 下游依赖: <被调用方>
- 影响用户数: <估算>
- 数据影响: 有/无

## 执行步骤
| 步骤 | 操作 | 验证命令 | 预期结果 |
|------|------|----------|----------|
| 1 | <操作> | <命令> | <结果> |
| 2 | ... | ... | ... |

## 回滚方案
| 步骤 | 操作 | 验证命令 | 预期结果 |
|------|------|----------|----------|
| 1 | <回滚操作> | <命令> | <结果> |

## 验证用例
- [ ] 核心 API 健康检查通过
- [ ] 业务场景 1: <描述>
- [ ] 业务场景 2: <描述>
- [ ] 监控指标无异常

## 审批
- [ ] 技术审核: <审核人> <日期>
- [ ] 风险确认: <负责人> <日期>
```

## GitOps 变更管理

### Git 驱动的变更流程

```
开发者提交 PR → CI 自动检查 → 代码审查 → 合并到 main
                                              │
                                              ▼
                                    ArgoCD/Flux 检测变更
                                              │
                                              ▼
                                    自动同步到集群 (Dev)
                                              │
                                              ▼
                                    验证通过 → Promotion
                                              │
                                              ▼
                                    Staging → Production
                                    (自动)     (审批/金丝雀)
```

### 变更审计（Git 历史）

```bash
# 🟢 查看最近变更历史
git log --oneline --since="24 hours ago" -- apps/

# 查看具体变更内容
git diff HEAD~1 -- apps/order-service/overlays/production/

# 回滚变更（Git revert）
git revert <commit-sha>
git push  # ArgoCD 自动同步回滚

# ArgoCD 变更历史
argocd app history order-service-production
argocd app rollback order-service-production <revision>
```

### 自动化变更验证

```yaml
# ArgoCD Sync Wave（有序变更）
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: order-service
  annotations:
    # 变更前后钩子
    argocd.argoproj.io/hook: PreSync
spec:
  source:
    path: apps/order-service/overlays/production
  syncPolicy:
    syncOptions:
      - Validate=true  # 同步前验证
---
# PreSync Hook: 变更前检查
apiVersion: batch/v1
kind: Job
metadata:
  name: pre-sync-check
  annotations:
    argocd.argoproj.io/hook: PreSync
spec:
  template:
    spec:
      containers:
        - name: check
          image: bitnami/kubectl
          command:
            - /bin/sh
            - -c
            - |
              # 检查当前服务健康
              kubectl get pods -n production -l app=order-service
              # 检查资源余量
              kubectl top nodes
              echo "✅ Pre-sync check passed"
      restartPolicy: Never
---
# PostSync Hook: 变更后验证
apiVersion: batch/v1
kind: Job
metadata:
  name: post-sync-verify
  annotations:
    argocd.argoproj.io/hook: PostSync
spec:
  template:
    spec:
      containers:
        - name: verify
          image: curlimages/curl
          command:
            - /bin/sh
            - -c
            - |
              sleep 30  # 等待 Pod 就绪
              # 健康检查
              curl -sf http://order-service.production:8080/health || exit 1
              # 业务验证
              curl -sf http://order-service.production:8080/api/v1/status || exit 1
              echo "✅ Post-sync verification passed"
      restartPolicy: Never
```

## 渐进式交付（Progressive Delivery）

### Argo Rollouts 金丝雀

```yaml
apiVersion: argoproj.io/v1alpha1
kind: Rollout
metadata:
  name: order-service
  namespace: production
spec:
  replicas: 5
  strategy:
    canary:
      steps:
        - setWeight: 5
        - pause: { duration: 5m }   # 观察 5 分钟
        - analysis:                   # 自动分析
            templates:
              - templateName: success-rate
        - setWeight: 25
        - pause: { duration: 10m }
        - analysis:
            templates:
              - templateName: success-rate
              - templateName: latency-p99
        - setWeight: 50
        - pause: { duration: 15m }
        - setWeight: 100
      canaryService: order-service-canary
      stableService: order-service-stable
---
# 分析模板（自动回滚条件）
apiVersion: argoproj.io/v1alpha1
kind: AnalysisTemplate
metadata:
  name: success-rate
spec:
  metrics:
    - name: success-rate
      interval: 2m
      successCondition: result[0] >= 0.99
      failureLimit: 3
      provider:
        prometheus:
          address: http://prometheus.monitoring:9090
          query: |
            sum(rate(http_requests_total{app="order-service",status!~"5.."}[5m]))
            /
            sum(rate(http_requests_total{app="order-service"}[5m]))
```

## 数据库变更安全

### 向前兼容迁移原则

```
安全变更顺序（可回滚）:

1. 添加新列（不删旧列）
   ALTER TABLE orders ADD COLUMN new_status VARCHAR(50);

2. 双写（新旧列同时写）
   UPDATE orders SET new_status = status;  -- 数据迁移

3. 切换读取到新列
   -- 应用代码修改读取 new_status

4. 停止写旧列
   -- 应用代码移除旧列写入

5. 删除旧列（下一个版本）
   ALTER TABLE orders DROP COLUMN status;

危险操作（禁止直接执行）:
- ❌ 直接删列
- ❌ 修改列类型
- ❌ 添加 NOT NULL 无默认值
- ❌ 大表 DDL（无 pt-osc/gh-ost）
```

### K8s 中的数据库迁移 Job

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: db-migration-v2-3
  namespace: production
  annotations:
    argocd.argoproj.io/hook: PreSync
    argocd.argoproj.io/hook-delete-policy: HookSucceeded
spec:
  backoffLimit: 3
  template:
    spec:
      containers:
        - name: migrate
          image: registry.example.com/order-service:v2.3.0
          command: ["./migrate", "up"]
          env:
            - name: DATABASE_URL
              valueFrom:
                secretKeyRef:
                  name: db-credentials
                  key: url
      restartPolicy: Never
```

## 变更度量与报告

### DORA 指标

| 指标 | 目标 (Elite) | 计算 | 改进方向 |
|------|-------------|------|----------|
| 部署频率 | 每日+ | 月度部署次数 | 自动化/小批量 |
| 变更前置时间 | < 1h | 提交→生产 | CI/CD 优化 |
| 变更失败率 | < 5% | 回滚次数/总部署 | 测试/金丝雀 |
| 恢复时间 | < 1h | 故障→恢复 | 回滚速度/监控 |

### 变更质量审查（月度）

```bash
# 统计本月变更
argocd app list -o json | jq '[.[] | select(.status.sync.status=="Synced")] | length'

# 统计回滚次数
argocd app list -o json | jq '[.[] | select(.status.operationState.phase=="Failed")] | length'

# 变更失败率
echo "scale=2; $FAILED / $TOTAL * 100" | bc
```

## 远程顾问指导要点

远程顾问无法直接执行变更，但可以通过以下方式为客户提供专业保障：

1. **方案预审**：要求客户提前提交变更方案，审核内容包含：
   - 影响范围分析（调用链上下游梳理）
   - 回滚步骤的每一步具体命令
   - 验证用例（至少 3 个核心业务场景）
2. **风险评估**：使用风险评估矩阵与客户共同判定风险等级，必要时建议拆分为多次低风险变更
3. **在线护航**：变更执行期间保持实时沟通，每完成一步由客户报告状态，顾问确认后再进入下一步
4. **事后复盘**：变更完成后 24h 内收集监控数据，验证变更效果，输出变更总结
5. **模式识别**：分析变更失败模式，提出系统性改进（如加强测试、改进金丝雀策略）

> 远程顾问的核心价值在于降低变更风险，而非替代客户执行操作。建立标准化的审核清单和沟通机制是关键。

## 相关链接

- [[生产运维/01-production-sre-daily-ops.md|production-sre-daily-ops]] — 日常巡检与值班手册
- [[概念/cluster-upgrade-paths.md|cluster-upgrade-paths]] — 集群升级路径
- [[发布变更/98-merged-indexes/index.md|gitops-deployment-patterns]] — GitOps 部署模式
- [[发布变更/02-release-engineering-strategy.md|发布工程策略]] — 版本管理与发布流水线
- [[发布变更/Progressive-Delivery/index.md|Progressive Delivery]] — 渐进式交付
- [[生产运维/04-incident-response-template.md|incident-response-playbook]] — 事件响应操作手册

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
