# Week 1: 地基建设期 (Days 1-7)

## 本周目标

- 掌握 Docker 完整生命周期与容器原理
- 具备 Linux 运维基础 (进程、网络、文件系统)
- 理解 K8s 架构全貌并流利使用 kubectl
- **产出**: 成功部署一个 K8s 集群，跑通第一个 Deployment

---

## 每日学习导航

| Day | 主题 | 文件 |
|-----|------|------|
| Day 1 | Docker 容器基础 | [day-1-docker-basics.md](./day-1-docker-basics.md) |
| Day 2 | Docker 网络 + 存储 + 安全 | [day-2-docker-advanced.md](./day-2-docker-advanced.md) |
| Day 3 | Linux 核心基础 | [day-3-linux-core.md](./day-3-linux-core.md) |
| Day 4 | Linux 网络 + 性能调优 | [day-4-linux-network.md](./day-4-linux-network.md) |
| Day 5 | K8s 架构全貌 | [day-5-k8s-architecture.md](./day-5-k8s-architecture.md) |
| Day 6 | K8s 架构深化 + 集群配置 | [day-6-k8s-cluster.md](./day-6-k8s-cluster.md) |
| Day 7 | 周复习 + 综合实践 | [day-7-review-practice.md](./day-7-review-practice.md) |

---

## 本周自测

完成本周学习后，请完成 [checkpoint.md](./checkpoint.md) 中的自测题。

---

## 本周实践项目

**项目 P1**: [从零搭建一个可运行 nginx 的 K8s 集群](../projects/p1-k8s-cluster-setup.md)

---

## 学习建议

1. **Day 1-2**: Docker 是 K8s 的基石，务必理解容器本质
2. **Day 3-4**: Linux 基础决定了排障能力的上限
3. **Day 5-6**: K8s 架构是后续所有学习的地图
4. **Day 7**: 综合实践是检验学习效果的最好方式

---

## 关键概念清单

本周需要掌握的核心概念:

- [ ] 容器 vs 虚拟机
- [ ] Docker 镜像分层原理
- [ ] Linux namespace 和 cgroup
- [ ] K8s Master/Node 架构
- [ ] etcd、API Server、Scheduler、Controller Manager 职责
- [ ] kubelet、kube-proxy 职责
- [ ] kubectl 常用命令
- [ ] Pod、Deployment、Service 基本概念
