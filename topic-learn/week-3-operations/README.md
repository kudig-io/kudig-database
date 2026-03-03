# Week 3: 运维作战能力期 (Days 15-21)

## 本周目标

- 建立安全合规体系认知 (RBAC、网络策略、Pod 安全标准)
- 构建完整可观测性体系 (Metrics + Logs + Traces + Alerting)
- 掌握结构化故障排查方法论
- **产出**: 监控告警配置 + 故障排查手册

---

## 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 15 | 安全体系: RBAC + 认证授权 | [day-15-security-1.md](./day-15-security-1.md) |
| Day 16 | 安全体系: Pod 安全 + 密钥管理 | [day-16-security-2.md](./day-16-security-2.md) |
| Day 17 | 可观测性: 监控 + Prometheus | [day-17-observability-1.md](./day-17-observability-1.md) |
| Day 18 | 可观测性: 日志 + 分布式追踪 | [day-18-observability-2.md](./day-18-observability-2.md) |
| Day 19 | 故障排查方法论 (关键日) | [day-19-troubleshooting-methodology.md](./day-19-troubleshooting-methodology.md) |
| Day 20 | 故障排查实战 | [day-20-troubleshooting-practice.md](./day-20-troubleshooting-practice.md) |
| Day 21 | 平台运维 + 综合实践 | [day-21-platform-ops.md](./day-21-platform-ops.md) |

---

## 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

---

## 本周实践项目

**项目 P3**: [可观测性体系搭建 + 故障演练](../projects/p3-observability-fault-drill.md)

---

## 学习建议

1. **Day 15-16**: 安全是生产环境的底线，不能忽视
2. **Day 17-18**: 可观测性是运维能力的核心
3. **Day 19-20**: FTA/FEBM 方法论是排障的系统化框架
4. **Day 21**: 综合实践，检验所学

---

## 关键概念清单

本周需要掌握的核心概念:

- [ ] RBAC: Role, ClusterRole, RoleBinding, ClusterRoleBinding
- [ ] ServiceAccount 和 Token
- [ ] Pod Security Standards
- [ ] Prometheus 数据模型和 PromQL
- [ ] Alertmanager 告警路由
- [ ] Loki 日志聚合
- [ ] FTA 故障树分析
- [ ] FEBM 取证循证方法
