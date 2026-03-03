# Week 1: ACK/ACR 基础与集群生命周期 (Days 1-7)

## 本周目标

- 了解 ACK/ACR 服务架构与管控层基本概念
- 掌握 ACK SDK 使用与 API 调用方式
- 熟悉 ACK/ACR 控制台界面与核心功能操作
- 掌握集群创建、删除、升级、证书管理全流程
- **产出**: 能够独立完成集群创建、升级、删除全流程操作

---

## 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 1 | ACK/ACR 管控 SR | [day-1-ack-acr-sr.md](./day-1-ack-acr-sr.md) |
| Day 2 | ACK SDK & API | [day-2-ack-sdk-api.md](./day-2-ack-sdk-api.md) |
| Day 3 | ACK/ACR 控制台 & 功能 | [day-3-ack-acr-console.md](./day-3-ack-acr-console.md) |
| Day 4 | K8S 新建集群 | [day-4-cluster-creation.md](./day-4-cluster-creation.md) |
| Day 5 | K8S 集群删除 | [day-5-cluster-deletion.md](./day-5-cluster-deletion.md) |
| Day 6 | K8S 集群升级 | [day-6-cluster-upgrade.md](./day-6-cluster-upgrade.md) |
| Day 7 | K8S 集群证书 | [day-7-cluster-certificate.md](./day-7-cluster-certificate.md) |

---

## 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

---

## 本周实践项目

**项目 P1**: [ACK 集群全生命周期管理](../projects/p1-ack-cluster-lifecycle.md)

---

## 学习建议

1. **Day 1-2**: ACK/ACR 管控层和 SDK/API 是后续所有操作的基础，务必理解服务架构
2. **Day 3**: 控制台操作是日常工作中最常用的方式，熟悉每个功能入口
3. **Day 4-5**: 集群创建和删除是最基础的操作，注意资源清理和依赖关系
4. **Day 6-7**: 集群升级和证书管理是生产环境中最关键的运维操作

---

## 关键概念清单

本周需要掌握的核心概念:

- [ ] ACK 托管版 vs 专有版 vs Serverless 架构差异
- [ ] ACR 企业版 vs 个人版区别
- [ ] ACK OpenAPI 核心接口 (DescribeCluster, CreateCluster, DeleteCluster)
- [ ] 集群创建参数: VPC、vSwitch、实例规格、节点池配置
- [ ] 集群删除流程与资源清理注意事项
- [ ] 集群升级策略: 原地升级 vs 替换升级
- [ ] 集群证书类型: CA 证书、kubeconfig、组件证书
- [ ] 证书轮换机制与过期处理
