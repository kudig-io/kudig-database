# Week 2: 安全认证与监控运维 (Days 8-14)

## 本周目标

- 掌握 RBAC 权限模型与配置实践
- 理解 RAM 账号与 K8S 的集成方案
- 了解常见漏洞类型与安全风险防范
- 掌握集群审计日志配置与分析
- 掌握集群监控体系搭建与告警配置
- **产出**: 能够配置集群 RBAC 权限、识别安全风险、搭建基础监控

---

## 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 8 | K8S 集群 RBAC | [day-8-rbac.md](./day-8-rbac.md) |
| Day 9 | RAM 账号管理 | [day-9-ram-integration.md](./day-9-ram-integration.md) |
| Day 10 | ACK/ACR/K8S 漏洞 | [day-10-vulnerability.md](./day-10-vulnerability.md) |
| Day 11 | 风险点识别与防范 | [day-11-risk-prevention.md](./day-11-risk-prevention.md) |
| Day 12 | K8S 集群审计 | [day-12-cluster-audit.md](./day-12-cluster-audit.md) |
| Day 13 | K8S 集群监控 | [day-13-cluster-monitoring.md](./day-13-cluster-monitoring.md) |
| Day 14 | K8S 集群配额 & License | [day-14-quota-license.md](./day-14-quota-license.md) |

---

## 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

---

## 本周实践项目

**项目 P2**: [安全认证与监控体系搭建](../projects/p2-security-monitoring-setup.md)

---

## 学习建议

1. **Day 8-9**: RBAC 和 RAM 是日常权限管理的核心，务必动手配置
2. **Day 10-11**: 安全意识是运维工程师的基本素养，关注最新漏洞公告
3. **Day 12**: 审计日志是事故追溯和合规的重要手段
4. **Day 13-14**: 监控和配额是保障集群稳定运行的基础

---

## 关键概念清单

本周需要掌握的核心概念:

- [ ] RBAC 四种资源: Role, ClusterRole, RoleBinding, ClusterRoleBinding
- [ ] RAM 用户、角色、策略与 ACK 权限的映射关系
- [ ] 常见 K8S 安全漏洞类型 (CVE)
- [ ] 安全基线与风险评估方法
- [ ] 审计日志的配置与 SLS 集成
- [ ] Prometheus + Grafana 监控架构
- [ ] ResourceQuota 和 LimitRange 配置
- [ ] ACK 集群配额与 License 管理
