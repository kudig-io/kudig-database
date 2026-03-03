# Week 4: 网络与存储 (Day 22-28)

> **目标**: 掌握集群网络架构与存储管理

---

## 本周目标

- 理解 Service 类型 (ClusterIP / NodePort / LoadBalancer) 与配置
- 掌握 Ingress 路由规则与 ALB/Nginx Ingress Controller
- 深入了解 Terway 和 Flannel 两种 CNI 方案的架构差异
- 能够创建、挂载和管理存储卷 (PV/PVC/StorageClass)
- 完成 4 周培训的综合复习

## 本周产出

- [ ] 配置 Service 和 Ingress 暴露应用
- [ ] 对比 Terway 和 Flannel 网络方案
- [ ] 完成 PV/PVC 存储卷创建与挂载
- [ ] 通过 Week 4 自测

---

## 每日学习导航

| Day | 主题 | 文件 | 预计时间 |
|:---:|------|------|:-------:|
| Day 22 | Service 基础 | [day-22-service-basics.md](day-22-service-basics.md) | 4-5h |
| Day 23 | Ingress | [day-23-ingress.md](day-23-ingress.md) | 4-5h |
| Day 24 | Terway 网络 | [day-24-terway-cni.md](day-24-terway-cni.md) | 4-5h |
| Day 25 | Flannel 网络 | [day-25-flannel-cni.md](day-25-flannel-cni.md) | 4-5h |
| Day 26 | 存储卷创建 & 删除 | [day-26-storage-create-delete.md](day-26-storage-create-delete.md) | 4-5h |
| Day 27 | 存储卷挂载 | [day-27-storage-mount.md](day-27-storage-mount.md) | 4-5h |
| Day 28 | 综合复习与实践 | [day-28-comprehensive-review.md](day-28-comprehensive-review.md) | 4-5h |

---

## 本周自测

完成全部学习后，请进行自测: [checkpoint.md](checkpoint.md)

## 实践项目

- [P4: 网络与存储综合实践](../projects/p4-network-storage-practice.md)
- [P5: 毕业综合项目](../projects/p5-graduation-project.md)

---

## 学习建议

1. **Day 22-23** 是应用暴露的核心，务必动手实践 Service 和 Ingress
2. **Day 24-25** CNI 对比是 ACK 特色内容，理解两种方案的适用场景
3. **Day 26-27** 存储是有状态应用的基础，关注云盘和 NAS 两种存储类型
4. **Day 28** 综合复习，回顾 4 周所有内容，查漏补缺

## 关键概念清单

- [ ] Service 类型: ClusterIP / NodePort / LoadBalancer
- [ ] Ingress 路由规则与 IngressClass
- [ ] ALB Ingress Controller vs Nginx Ingress Controller
- [ ] Terway ENI 模式 / ENIIP 模式
- [ ] Flannel VxLAN 模式
- [ ] PersistentVolume (PV) / PersistentVolumeClaim (PVC)
- [ ] StorageClass 与动态供给
- [ ] 阿里云云盘 / NAS / OSS 存储
