# 项目 P5: 毕业综合实践项目

> **所属周**: Week 4 | **预计时间**: 2.5+ 小时

---

## 项目目标

搭建一个完整的生产级 K8s 平台，综合运用一个月所学的所有知识:

- 应用编排: Deployment + StatefulSet
- 网络存储: Ingress + PVC + NetworkPolicy
- 安全: RBAC + Pod Security
- 可观测性: Prometheus + Loki
- GitOps: ArgoCD
- 运维: 故障排查手册

---

## 项目架构

```
┌─────────────────────────────────────────────────────────────┐
│                        Ingress                               │
│                    (TLS + 路由)                              │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│  │  Frontend   │    │   Backend   │    │   Backend   │     │
│  │ Deployment  │───▶│ StatefulSet │───▶│ StatefulSet │     │
│  │  (HPA)      │    │   (API-1)   │    │   (API-2)   │     │
│  └─────────────┘    └─────────────┘    └─────────────┘     │
│         │                  │                  │             │
│         └──────────────────┴──────────────────┘             │
│                            │                                 │
│                   NetworkPolicy                              │
├─────────────────────────────────────────────────────────────┤
│                        Storage                               │
│               (StorageClass + PVC)                          │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│  │ Prometheus  │    │   Grafana   │    │    Loki     │     │
│  │ + Alertmgr  │    │             │    │ + Promtail  │     │
│  └─────────────┘    └─────────────┘    └─────────────┘     │
├─────────────────────────────────────────────────────────────┤
│                        ArgoCD                                │
│                   (GitOps Pipeline)                          │
└─────────────────────────────────────────────────────────────┘
```

---

## 验收清单

### 1. 应用编排

- [ ] 前端 Deployment (至少 2 副本)
- [ ] 前端 HPA (CPU 阈值 70%)
- [ ] 后端 StatefulSet (至少 2 副本)
- [ ] 每个 StatefulSet Pod 有独立 PVC
- [ ] 配置了 liveness/readiness 探针
- [ ] 配置了合理的 resources

### 2. 网络

- [ ] ClusterIP Service 用于内部通信
- [ ] Ingress 配置路由规则
- [ ] Ingress 配置 TLS (可以是自签名)
- [ ] NetworkPolicy 限制 Pod 间访问

### 3. 存储

- [ ] 使用 StorageClass 动态供应
- [ ] PVC 成功绑定
- [ ] 数据在 Pod 重启后持久化

### 4. 安全

- [ ] 创建专用 ServiceAccount
- [ ] 配置 RBAC (最小权限)
- [ ] Pod 以非 root 用户运行
- [ ] 配置 securityContext

### 5. 可观测性

- [ ] Prometheus 采集应用指标
- [ ] 配置至少 3 条告警规则
- [ ] Grafana Dashboard 可视化
- [ ] Loki 收集应用日志
- [ ] Alertmanager 路由配置

### 6. GitOps

- [ ] 应用配置存储在 Git 仓库
- [ ] ArgoCD Application 配置完成
- [ ] 修改 Git 能触发同步

### 7. 文档

- [ ] 架构设计文档
- [ ] 部署操作手册
- [ ] 故障排查手册 (基于 FTA/FEBM)
- [ ] 变更管理 SOP

---

## 实施步骤

### Phase 1: 基础设施 (30min)

```bash
# 创建 namespace
kubectl create namespace graduation-project

# 确认监控组件就绪
kubectl get pods -n monitoring

# 确认 ArgoCD 就绪
kubectl get pods -n argocd
```

### Phase 2: 应用部署 (45min)

参考 P2 项目，部署:
- Frontend Deployment + HPA
- Backend StatefulSet + PVC
- Service + Ingress + NetworkPolicy

### Phase 3: 安全加固 (30min)

```bash
# 创建 ServiceAccount
kubectl create serviceaccount app-sa -n graduation-project

# 配置 Role 和 RoleBinding
# 配置 Pod SecurityContext
```

### Phase 4: 可观测性 (30min)

参考 P3 项目，配置:
- PrometheusRule 告警规则
- ServiceMonitor 指标采集
- Grafana Dashboard
- Loki 日志查询

### Phase 5: GitOps (30min)

参考 P4 项目，配置:
- Git 仓库结构
- ArgoCD Application

### Phase 6: 文档编写 (30min)

创建以下文档:
- `architecture.md`: 架构设计
- `deployment-guide.md`: 部署手册
- `troubleshooting-handbook.md`: 故障排查手册
- `change-management-sop.md`: 变更管理 SOP

---

## 演示要点

1. **架构讲解**
   - 能够清晰解释整体架构
   - 能够解释组件间的关系

2. **操作演示**
   - 通过 Ingress 访问应用
   - 演示 HPA 自动扩容
   - 演示滚动更新

3. **故障演练**
   - 注入一个故障
   - 按 FTA 方法定位
   - 修复并验证

4. **GitOps 演示**
   - 修改 Git 仓库
   - 观察自动同步

---

## 评分标准

| 项目 | 分值 | 评分标准 |
|------|------|----------|
| 应用编排 | 15 | 完成所有组件部署 |
| 网络存储 | 15 | Ingress/PVC/NetworkPolicy 正常 |
| 安全 | 10 | RBAC/SecurityContext 配置 |
| 可观测性 | 15 | 监控告警日志完整 |
| GitOps | 10 | ArgoCD 自动同步 |
| 文档 | 15 | 文档完整清晰 |
| 演示 | 20 | 能够清晰讲解和演示 |
| **总分** | **100** | |

---

恭喜完成毕业项目!
