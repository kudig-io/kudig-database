# Day 1: ACK/ACR 管控 SR

> **学习时间**: 4-5 小时 | **主题**: ACK/ACR 服务架构与管控层基本概念

---

## 今日目标

- [ ] 理解 ACK 服务架构 (托管版、专有版、Serverless)
- [ ] 理解 ACR 服务架构 (个人版、企业版)
- [ ] 掌握 ACK/ACR 管控层组件和工作流程
- [ ] 了解内部 SR (Service Request) 处理流程

---

## 理论学习 (2h)

### 必读文档

1. **ACK 服务总览**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/alicloud-ack-overview.md`
   - 重点: ACK 产品形态、托管版 vs 专有版 vs Serverless 架构差异、管控组件

2. **ACK 实操指南**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/service-ack-practical-guide.md`
   - 重点: 实际操作流程、常见场景

3. **Kubernetes 架构总览**
   - 文件: `../../../domain-1-architecture-fundamentals/01-kubernetes-architecture-overview.md`
   - 重点: K8S 整体架构与 ACK 托管架构的对应关系

### 阅读要点

- ACK 托管版: 管控面由阿里云托管，用户只需维护 Worker 节点
- ACK 专有版: 用户完全管理 Master 和 Worker 节点
- ACK Serverless: 无需管理节点，按 Pod 计费
- ACR 企业版: 独享实例、多地域同步、安全扫描
- ACR 个人版: 共享资源，适合个人开发测试
- 管控层负责集群创建、组件部署、版本升级等

---

## 实践任务 (2.5h)

### 任务 1: 了解 ACK 集群类型 (45min)

```bash
# 使用 aliyun CLI 查看当前账号下的 ACK 集群列表
aliyun cs GET /api/v1/clusters

# 查看单个集群详情 (替换为实际 cluster_id)
aliyun cs GET /clusters/<cluster_id>

# 关注返回字段:
# - cluster_type: 集群类型 (ManagedKubernetes / Kubernetes / Ask)
# - state: 集群状态
# - current_version: K8S 版本
# - meta_data: 管控面元数据
```

### 任务 2: 了解 ACR 实例 (45min)

```bash
# 查看 ACR 个人版仓库列表
aliyun cr GET /repos

# 查看 ACR 企业版实例列表
aliyun cr ListInstance

# 对比个人版和企业版的功能差异:
# - 镜像安全扫描
# - 多地域同步
# - 访问控制
# - P2P 加速分发
```

### 任务 3: 梳理管控层架构 (30min)

```bash
# 连接到 ACK 集群，查看管控组件
kubectl get pods -n kube-system

# 查看关键组件:
# - coredns: DNS 解析
# - metrics-server: 指标采集
# - cloud-controller-manager: 云资源管理
# - terway / flannel: CNI 网络插件
# - csi-plugin: 存储插件

# 查看组件版本
kubectl get pods -n kube-system -o custom-columns='NAME:.metadata.name,IMAGE:.spec.containers[0].image'
```

### 任务 4: 内部 SR 流程熟悉 (30min)

```bash
# SR 分类与优先级:
# P1 - 生产环境故障，影响业务
# P2 - 功能异常，有 workaround
# P3 - 使用咨询，功能建议
# P4 - 文档问题

# 常见 SR 场景:
# 1. 集群创建失败 -> 检查 VPC/vSwitch/安全组配置
# 2. 节点添加失败 -> 检查 ECS 库存、节点规格
# 3. 集群升级问题 -> 检查版本兼容性、组件状态
# 4. 镜像拉取失败 -> 检查 ACR 权限、网络策略
```

---

## 费曼复述 (0.5h)

用自己的语言回答以下问题:

1. **ACK 托管版和专有版的核心区别是什么？各自适合什么场景？**
   - 提示: 从管理责任、成本、灵活性三个角度

2. **当用户报告"集群创建失败"时，你的第一步排查思路是什么？**
   - 提示: 从资源依赖链路思考 (VPC -> vSwitch -> 安全组 -> ECS)

3. **ACR 企业版相比个人版多了哪些关键能力？**
   - 提示: 安全性、可用性、性能

---

## 今日检验

- [ ] 能说出 ACK 三种产品形态的区别
- [ ] 能说出 ACR 企业版和个人版的功能差异
- [ ] 能使用 aliyun CLI 查看集群和镜像仓库信息
- [ ] 能列出 ACK 集群中核心的 kube-system 组件

---

## 核心概念总结

| 概念 | 说明 | 生产注意事项 |
|------|------|--------------|
| ACK 托管版 | 管控面托管，用户管理 Worker | 推荐生产使用，降低运维负担 |
| ACK 专有版 | 完全自主管理 | 适合需要深度定制的场景 |
| ACK Serverless | 无节点，按 Pod 计费 | 适合突发流量、CI/CD 等场景 |
| ACR 企业版 | 独享实例，安全扫描 | 生产环境推荐使用 |
| 管控层 | 集群生命周期管理 | 理解管控层有助于排障定位 |

---

## 明日预告

Day 2 将学习 ACK SDK 和 API 的使用方式，掌握通过编程方式管理集群资源。
