# Week 2: 核心技术构建期 (Days 8-14)

## 本周目标

- 深入理解控制平面各组件的工作机制
- 掌握所有主要工作负载类型及生产模式
- 掌握 K8s 网络栈 (CNI、Service、DNS、Ingress)
- 掌握存储体系 (PV/PVC/StorageClass/CSI)
- **产出**: 生产级应用编排方案

---

## 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 8 | 控制平面: etcd + API Server | [day-8-control-plane-1.md](./day-8-control-plane-1.md) |
| Day 9 | 控制平面: Scheduler + Controller Manager | [day-9-control-plane-2.md](./day-9-control-plane-2.md) |
| Day 10 | 工作负载: Deployment + StatefulSet + DaemonSet | [day-10-workloads-1.md](./day-10-workloads-1.md) |
| Day 11 | 工作负载: Pod 生命周期 + 资源管理 + HPA | [day-11-workloads-2.md](./day-11-workloads-2.md) |
| Day 12 | 网络栈: CNI + Service + DNS | [day-12-networking-1.md](./day-12-networking-1.md) |
| Day 13 | 网络栈: Ingress + NetworkPolicy | [day-13-networking-2.md](./day-13-networking-2.md) |
| Day 14 | 存储体系 + 综合实践 | [day-14-storage-practice.md](./day-14-storage-practice.md) |

---

## 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

---

## 本周实践项目

**项目 P2**: [生产级应用全栈编排](../projects/p2-production-app-orchestration.md)

---

## 学习建议

1. **Day 8-9**: 控制平面是 K8s 大脑，理解其原理对排障至关重要
2. **Day 10-11**: 工作负载是日常运维的核心，重点掌握
3. **Day 12-13**: 网络是 K8s 最复杂的部分，需要反复实践
4. **Day 14**: 存储和综合实践，整合本周所学

---

## 关键概念清单

本周需要掌握的核心概念:

- [ ] etcd Raft 协议基础
- [ ] API Server 请求处理链
- [ ] Scheduler Filter/Score 机制
- [ ] Controller Reconcile 循环
- [ ] Deployment 滚动更新策略
- [ ] StatefulSet 有序部署
- [ ] CNI 网络模型
- [ ] Service 四种类型
- [ ] Ingress 路由规则
- [ ] PV/PVC 绑定机制
