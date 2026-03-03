# Day 5: K8S 集群删除

> **学习时间**: 4-5 小时 | **主题**: 理解集群删除流程与注意事项

---

## 今日目标

- [ ] 掌握集群删除的完整流程和先决条件
- [ ] 理解删除集群时的资源清理逻辑
- [ ] 了解"保留资源"与"完全删除"的区别
- [ ] 掌握删除失败的排查方法

---

## 理论学习 (2h)

### 必读文档

1. **ACK 集群管理**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/alicloud-ack-overview.md`
   - 重点: 集群删除相关的注意事项

2. **K8S 集群架构**
   - 文件: `../../../domain-1-architecture-fundamentals/01-kubernetes-architecture-overview.md`
   - 重点: 理解删除集群涉及的组件和资源

### 阅读要点

- 删除集群前必须评估的资源依赖
- 集群删除会清理: ECS 节点、SLB、ENI、安全组规则
- 可选保留的资源: SLB、NAT 网关、EIP
- 删除顺序: 先清理业务资源 -> 再删除集群
- 常见删除失败原因: 资源被其他服务引用

---

## 实践任务 (2.5h)

### 任务 1: 删除前检查清单 (45min)

```bash
# 1. 检查集群中的业务工作负载
kubectl get deployments -A
kubectl get statefulsets -A
kubectl get daemonsets -A

# 2. 检查 Service (特别是 LoadBalancer 类型)
kubectl get svc -A | grep LoadBalancer
# LoadBalancer 类型会创建 SLB 实例，需要确认是否保留

# 3. 检查 PVC 和 PV (持久化存储)
kubectl get pvc -A
kubectl get pv
# 注意: 删除集群可能导致 PV 数据丢失

# 4. 检查 Ingress (可能关联 SLB/ALB)
kubectl get ingress -A

# 5. 检查是否有外部依赖
kubectl get svc -A -o jsonpath='{range .items[?(@.spec.type=="LoadBalancer")]}{.metadata.namespace}/{.metadata.name}: {.status.loadBalancer.ingress[0].ip}{"\n"}{end}'
```

### 任务 2: 业务资源清理 (45min)

```bash
# 清理业务 Namespace (保留系统 Namespace)
kubectl get namespaces

# 逐个清理业务 Namespace
kubectl delete namespace <business-ns-1>
kubectl delete namespace <business-ns-2>

# 等待所有 Pod 终止
kubectl get pods -A | grep -v 'kube-system\|Running\|Completed'

# 检查 LoadBalancer 类型 Service 是否已清理
kubectl get svc -A | grep LoadBalancer

# 检查 PVC 是否已清理
kubectl get pvc -A

# 确认所有业务资源已清理完毕
kubectl get all -A --no-headers | grep -v kube-system
```

### 任务 3: 通过控制台删除集群 (30min)

```
# ACK 控制台 -> 集群列表 -> 选择目标集群 -> 更多 -> 删除集群

# 删除选项:
# 1. "删除集群并释放所有资源"
#    - 删除所有节点 ECS
#    - 释放 SLB 实例
#    - 释放 ENI 网卡
#    - 清理安全组规则
#
# 2. "删除集群但保留部分资源"
#    - 可选保留: SLB、NAT 网关、EIP
#    - 已有的 ECS 可选保留 (仅移除 K8S 组件)

# 注意:
# - 删除操作不可逆！
# - 确认输入集群名称才能执行
# - 删除过程约 5-10 分钟
```

### 任务 4: 通过 API 删除集群 (30min)

```bash
# 删除集群 (释放所有资源)
aliyun cs DELETE /clusters/<cluster_id>

# 删除集群 (保留 SLB)
aliyun cs DELETE /clusters/<cluster_id> \
  --body '{"retain_resources":["SLB"]}'

# 查看删除进度
aliyun cs GET /clusters/<cluster_id>/logs

# 验证删除完成
aliyun cs GET /clusters/<cluster_id>
# 应返回 404 或 state=deleting/deleted
```

### 任务 5: 删除失败排查 (30min)

```bash
# 常见删除失败原因:

# 1. SLB 被其他服务引用
# 检查: 查看 SLB 控制台，确认是否有非 K8S 的监听
aliyun slb DescribeLoadBalancers --VpcId <vpc_id>

# 2. ENI 无法释放
# 检查: 查看 ENI 状态，确认是否有安全组规则引用
aliyun ecs DescribeNetworkInterfaces --VpcId <vpc_id>

# 3. 安全组被引用
# 检查: 查看安全组关联的资源
aliyun ecs DescribeSecurityGroupReferences --SecurityGroupId.1 <sg_id>

# 4. 残留资源手动清理
# 如果集群删除失败后重试仍失败:
# - 手动释放残留 SLB
# - 手动释放残留 ENI
# - 联系内部 oncall 协助清理管控面残留
```

---

## 费曼复述 (0.5h)

用自己的语言回答以下问题:

1. **删除 ACK 集群前，必须做哪些检查？为什么？**
   - 提示: 业务负载、SLB、存储、外部依赖

2. **"保留资源删除"和"完全删除"有什么区别？什么场景用哪种？**
   - 提示: 保留 SLB/NAT 适合迁移场景

3. **集群删除失败最常见的原因是什么？如何处理？**
   - 提示: 资源被引用无法释放

---

## 今日检验

- [ ] 能列出删除集群前的完整检查清单
- [ ] 能通过控制台和 API 两种方式删除集群
- [ ] 理解保留资源和完全删除的区别
- [ ] 能排查集群删除失败的常见原因

---

## 核心概念总结

| 场景 | 操作 | 注意事项 |
|------|------|---------|
| 测试集群清理 | 完全删除 | 确认无业务数据 |
| 集群迁移 | 保留 SLB/NAT | 新集群复用网络资源 |
| 删除失败 | 检查残留资源 | SLB/ENI/安全组被引用 |
| 数据保护 | 先备份 PV 数据 | 删除后无法恢复 |

---

## 明日预告

Day 6 将学习集群升级策略，掌握版本升级的操作步骤和风险控制。
