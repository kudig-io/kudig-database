---
title: Release Engineering Strategy — Versioning, Branching, and Delivery Pipeline Design
description: K8s 发布工程 — 版本策略、分支模型、发布流水线、变更管理、回滚策略、发布编排
summary: 构建生产级发布工程体系，涵盖版本管理、分支策略、流水线设计与变更控制
category: practice
tags:
- release-engineering
- versioning
- branching
- pipeline
- change-management
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: release
---
# 发布工程策略

> 版本管理、分支模型与交付流水线的系统化设计。

## 发布工程全景

```
┌─────────────────────────────────────────────────────────────────┐
│  代码提交 → 构建 → 测试 → 扫描 → 打包 → 部署 → 验证 → 监控    │
│     │        │      │      │      │      │      │      │        │
│   Git      CI    单元   安全   镜像   GitOps  金丝雀  告警     │
│   PR    Compile  集成   扫描  签名  ArgoCD  渐进   SLO       │
│         构建    E2E   策略   推送  Sync   发布   监控       │
└─────────────────────────────────────────────────────────────────┘
```

## 版本策略

### 语义化版本（SemVer）

```
v{MAJOR}.{MINOR}.{PATCH}[-{PRERELEASE}][+{BUILD}]

MAJOR: 不兼容的 API 变更
MINOR: 向后兼容的功能新增
PATCH: 向后兼容的缺陷修复
PRERELEASE: alpha.1, beta.2, rc.1
BUILD: 构建元数据（不影响优先级）

示例:
v1.0.0          → 正式版本
v1.1.0-rc.1     → 候选版本
v1.1.0          → 功能发布
v1.1.1          → 热修复
```

### 镜像标签策略

| 标签类型 | 格式 | 用途 | 示例 |
|----------|------|------|------|
| Git SHA | `sha-<7位>` | 精确追溯 | `sha-a1b2c3d` |
| 语义版本 | `v1.2.3` | 正式发布 | `v2.1.0` |
| 分支最新 | `main-latest` | 开发环境 | `main-a1b2c3d` |
| 环境标签 | `prod-current` | 当前生产 | 可变标签 |
| 日期构建 | `20260721-a1b2c3d` | 时间追溯 | `20260721-a1b2c3d` |

```bash
# 构建时多标签
GIT_SHA=$(git rev-parse --short HEAD)
VERSION=$(cat VERSION)  # 或从 tag 获取

docker build \
  -t registry.example.com/app:${GIT_SHA} \
  -t registry.example.com/app:${VERSION} \
  -t registry.example.com/app:latest \
  .

# 推送所有标签
docker push registry.example.com/app --all-tags
```

## 分支策略

### Trunk-Based Development（推荐）

```
main ─────────────────────────────────────────────────────▶
  │         │         │         │         │
  ├── feat-a (短命) ──┤         │         │
  │                   │         │         │
  │         ├── fix-b (短命) ──┤         │
  │                   │         │         │
  │                   │    ├── feat-c ───┤
  │                   │         │         │
  ▼                   ▼         ▼         ▼
 v1.0.0            v1.1.0   v1.1.1    v1.2.0
 (tag)             (tag)    (tag)     (tag)

规则:
- 分支生命周期 < 2 天
- Feature Flag 控制未完成功能
- main 始终可部署
- Tag 触发发布
```

### 发布分支（大型项目）

```
main ─────────────────────────────────────────────────────▶
  │                    │
  ├── release/1.2 ─────┼──────────────────────────────────▶
  │   (稳定化)         │    cherry-pick 热修复
  │   v1.2.0-rc.1     │    v1.2.1
  │   v1.2.0          │
  │                    │
  ├── release/1.3 ─────┼──────────────────────────────────▶
  │   v1.3.0-rc.1     │
  │   v1.3.0          │
```

## 发布流水线

### 完整 CI/CD Pipeline

```yaml
# GitHub Actions — 完整发布流水线
name: Release Pipeline
on:
  push:
    tags: ['v*']

env:
  REGISTRY: registry.example.com
  IMAGE: app

jobs:
  # Phase 1: 构建与测试
  build:
    runs-on: ubuntu-latest
    outputs:
      image-digest: ${{ steps.build.outputs.digest }}
    steps:
      - uses: actions/checkout@v4
        with:
          fetch-depth: 0

      - name: Build & Test
        run: |
          make build
          make test-unit
          make test-integration

      - name: Build Image
        id: build
        run: |
          DIGEST=$(docker build -q \
            -t $REGISTRY/$IMAGE:${{ github.ref_name }} \
            -t $REGISTRY/$IMAGE:$(git rev-parse --short HEAD) .)
          docker push $REGISTRY/$IMAGE --all-tags
          echo "digest=$DIGEST" >> $GITHUB_OUTPUT

  # Phase 2: 安全扫描
  security:
    needs: build
    runs-on: ubuntu-latest
    steps:
      - name: Image Scan
        run: |
          trivy image --exit-code 1 \
            --severity HIGH,CRITICAL \
            $REGISTRY/$IMAGE:${{ github.ref_name }}
      - name: Sign Image
        run: |
          cosign sign $REGISTRY/$IMAGE:${{ github.ref_name }}

  # Phase 3: 部署到 Staging
  deploy-staging:
    needs: security
    runs-on: ubuntu-latest
    environment: staging
    steps:
      - name: Update GitOps Repo
        run: |
          git clone https://x-access-token:${{ secrets.GITOPS_TOKEN }}@github.com/org/gitops.git
          cd gitops/apps/app/overlays/staging
          kustomize edit set image $REGISTRY/$IMAGE=${{ github.ref_name }}
          git add . && git commit -m "release: app ${{ github.ref_name }} to staging"
          git push

      - name: Wait for Healthy
        run: |
          sleep 60
          for i in $(seq 1 30); do
            if curl -sf https://app.staging.example.com/health; then
              echo "✅ Staging healthy"
              exit 0
            fi
            sleep 10
          done
          echo "❌ Staging unhealthy"
          exit 1

  # Phase 4: 部署到 Production（金丝雀）
  deploy-production:
    needs: deploy-staging
    runs-on: ubuntu-latest
    environment: production  # 需要审批
    steps:
      - name: Update GitOps Repo
        run: |
          git clone https://x-access-token:${{ secrets.GITOPS_TOKEN }}@github.com/org/gitops.git
          cd gitops/apps/app/overlays/production
          kustomize edit set image $REGISTRY/$IMAGE=${{ github.ref_name }}
          git add . && git commit -m "release: app ${{ github.ref_name }} to production"
          git push

      - name: Monitor Canary
        run: |
          # Argo Rollouts 自动金丝雀
          # 监控 5 分钟错误率
          sleep 300
          ERROR_RATE=$(curl -s "$PROM_URL/api/v1/query?query=rate(http_errors_total[5m])" | jq '.data.result[0].value[1]')
          if (( $(echo "$ERROR_RATE > 0.05" | bc -l) )); then
            echo "❌ 错误率过高，触发回滚"
            kubectl argo rollouts abort app -n production
            exit 1
          fi
          echo "✅ 金丝雀通过，完成发布"
```

## 变更管理

### 变更分类

| 类型 | 风险 | 审批 | 窗口 | 回滚 |
|------|------|------|------|------|
| 标准变更 | 低 | 自动 | 任何时间 | 自动 |
| 普通变更 | 中 | 1 人审批 | 工作时间 | 手动 |
| 重大变更 | 高 | CAB 审批 | 变更窗口 | 预案 |
| 紧急变更 | 高 | 事后补审 | 任何时间 | 即时 |

### 变更冻结日历

```yaml
# 变更冻结规则
freeze-windows:
  - name: "春节冻结"
    start: "2026-01-25"
    end: "2026-02-05"
    scope: production
    exceptions: ["P0-hotfix"]
  - name: "周五冻结"
    recurring: "FRI 15:00 - MON 09:00"
    scope: production
    exceptions: ["security-patch"]
  - name: "大促冻结"
    start: "2026-11-01"
    end: "2026-11-12"
    scope: all
    exceptions: []
```

## 回滚策略

### 快速回滚（< 2 min）

```bash
# 方法 1: GitOps 回滚（推荐）
cd gitops/apps/app/overlays/production
git revert HEAD
git push
# ArgoCD 自动同步到上一版本

# 方法 2: Argo Rollouts 回滚
kubectl argo rollouts undo app -n production

# 方法 3: K8s 原生回滚
kubectl rollout undo deployment/app -n production
kubectl rollout status deployment/app -n production

# 方法 4: 指定版本回滚
kubectl rollout undo deployment/app -n production --to-revision=5
```

### 数据库迁移回滚

```bash
# 向前兼容迁移（推荐）
# 1. 添加新列（不删旧列）
# 2. 双写（新旧列同时写）
# 3. 切换读取到新列
# 4. 停止写旧列
# 5. 删除旧列（下一个版本）

# 回滚安全: 步骤 1-3 任何时候可回滚（旧列仍在）
```

## 发布度量

| 指标 | 目标 | 说明 |
|------|------|------|
| 部署频率 | 每日+ | DORA Elite |
| 变更前置时间 | < 1h | 提交到生产 |
| 变更失败率 | < 5% | 需要回滚的比例 |
| 恢复时间(MTTR) | < 1h | 故障到恢复 |
| 金丝雀通过率 | > 95% | 自动通过比例 |
| 回滚时间 | < 2min | 从决策到完成 |

## 最佳实践

| 实践 | 说明 |
|------|------|
| 不可变镜像 | 同一镜像从 Dev 到 Prod |
| 自动化一切 | 手动步骤是风险点 |
| 渐进式发布 | 金丝雀/蓝绿降低风险 |
| 特性开关 | 解耦部署与发布 |
| 可观测性 | 发布后自动监控 SLO |
| 回滚演练 | 定期验证回滚流程 |
| 变更日志 | 自动生成 CHANGELOG |
| 版本锁定 | 生产环境固定版本 |

## 故障排查表

| 问题现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|--------|
| 发布后 SLO 下降 | 新版本引入性能回退 | 对比发布前后 Grafana 面板 | 立即回滚，分析 p99 变化 |
| 回滚失败 | 数据库迁移不可逆 | `kubectl rollout undo deploy/<name>` | 使用 expand-contract 模式 |
| 变更日志缺失 | CI 未配置自动生成 | 检查 CI pipeline 中的 changelog step | 集成 conventional-commits + auto-changelog |
| 发布窗口冲突 | 多团队同时发布 | 查看发布日历/变更管理系统 | 建立发布列车（Release Train）机制 |
| 特性开关泄漏 | 开关未清理 | 搜索代码中 `feature_flag` 引用 | 定期清理已全量开关 |

## 发布策略对比

| 策略 | 风险 | 回滚速度 | 适用场景 |
|------|------|---------|--------|
| 滚动更新 | 中 | 快 | 无状态服务默认 |
| 蓝绿部署 | 低 | 极快(切流) | 有状态/数据库变更 |
| 金丝雀发布 | 极低 | 快 | 高流量核心服务 |
| A/B 测试 | 极低 | 快 | 用户体验验证 |
| 暗影流量 | 无 | N/A | 新版本预热验证 |

## 相关工具

| 工具 | 用途 | 场景 |
|------|------|------|
| Argo Rollouts | 渐进式发布控制器 | 金丝雀/蓝绿自动化 |
| Flagger | 自动化金丝雀分析 | 基于指标自动 Promotion |
| LaunchDarkly | 特性开关平台 | 解耦部署与发布 |
| semantic-release | 自动版本管理 | 基于 commit 生成版本号 |
| Spinnaker | 多环境发布编排 | 企业级复杂发布流程 |

## Related

- [[发布变更/index.md|发布变更]]
- [[发布变更/GitOps/index.md|GitOps]]
- [[发布变更/Progressive-Delivery/index.md|Progressive Delivery]]
- [[发布变更/部署方案/05-multi-environment-deployment-strategy.md|多环境部署]]
