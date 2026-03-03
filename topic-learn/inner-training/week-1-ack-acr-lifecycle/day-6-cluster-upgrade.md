# Day 6: K8S 集群升级

> **学习时间**: 4-5 小时 | **主题**: 掌握集群版本升级策略与操作步骤

---

## 今日目标

- [ ] 理解 ACK 集群升级的两个阶段 (管控面 + 节点)
- [ ] 掌握升级前的兼容性检查方法
- [ ] 能通过控制台和 API 完成集群升级
- [ ] 了解升级回滚策略和风险控制

---

## 理论学习 (2h)

### 必读文档

1. **K8S 版本升级策略**
   - 文件: `../../../domain-1-architecture-fundamentals/07-upgrade-paths-strategy.md`
   - 重点: 版本兼容性、升级路径规划

2. **升级与迁移策略**
   - 文件: `../../../domain-1-architecture-fundamentals/18-upgrade-migration-strategy.md`
   - 重点: 升级风险评估、回滚方案

3. **ACK 集群管理**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/alicloud-ack-overview.md`
   - 重点: ACK 特有的升级流程和注意事项

### 阅读要点

- ACK 集群升级分两个阶段: 管控面升级 + 节点升级
- 管控面升级由阿里云自动完成 (托管版)，对业务无感知
- 节点升级支持两种方式: 原地升级、替换升级 (推荐)
- 版本跨度限制: 只能逐版本升级 (如 1.26 -> 1.28 需先升到 1.27)
- 升级前必须检查: API 废弃、组件兼容性、自定义配置

---

## 实践任务 (2.5h)

### 任务 1: 升级前检查 (45min)

```bash
# 1. 查看当前集群版本
kubectl version
aliyun cs GET /clusters/<cluster_id> | jq '.current_version'

# 2. 查看可升级的目标版本
aliyun cs GET /upgrade/cluster/<cluster_id>

# 3. 检查 API 废弃情况
# 使用 kubent (Kube No Trouble) 工具检查废弃 API
# 安装: brew install kubent (macOS) 或从 GitHub 下载
kubent

# 4. 检查集群组件兼容性
kubectl get pods -n kube-system -o wide
aliyun cs GET /clusters/<cluster_id>/components/upgradestatus

# 5. 检查自定义 webhook 和 admission
kubectl get validatingwebhookconfigurations
kubectl get mutatingwebhookconfigurations

# 6. 备份关键资源
kubectl get all -A -o yaml > cluster-backup.yaml
kubectl get configmaps -A -o yaml > configmaps-backup.yaml
kubectl get secrets -A -o yaml > secrets-backup.yaml
```

### 任务 2: 管控面升级 (30min)

```bash
# 通过 API 触发管控面升级 (托管版)
aliyun cs POST /api/v2/clusters/<cluster_id>/upgrade \
  --body '{
    "next_version": "1.28.9-aliyun.1"
  }'

# 查看升级进度
aliyun cs GET /clusters/<cluster_id>/logs

# 验证管控面升级完成
kubectl version --short
# Server Version 应该更新为目标版本

# 检查管控组件状态
kubectl get pods -n kube-system
kubectl get cs  # 检查组件状态 (如果适用)
```

### 任务 3: 节点升级 - 替换升级方式 (45min)

```bash
# 替换升级 (推荐): 创建新节点 -> 排水旧节点 -> 移除旧节点

# 1. 在节点池中扩容新节点
aliyun cs POST /clusters/<cluster_id>/nodepools/<nodepool_id> \
  --body '{"count": 1}'

# 2. 等待新节点 Ready
kubectl get nodes -w

# 3. 对旧节点执行排水 (cordon + drain)
kubectl cordon <old-node-name>
kubectl drain <old-node-name> --ignore-daemonsets --delete-emptydir-data

# 4. 确认业务 Pod 已迁移到新节点
kubectl get pods -A -o wide | grep <old-node-name>

# 5. 移除旧节点
aliyun cs POST /clusters/<cluster_id>/nodes \
  --body '{"nodes":["<old-node-id>"],"release_node":true}'

# 6. 验证集群状态
kubectl get nodes -o wide
kubectl get pods -A | grep -v Running
```

### 任务 4: 升级后验证 (30min)

```bash
# 1. 验证集群版本
kubectl version

# 2. 验证所有节点版本一致
kubectl get nodes -o custom-columns='NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion'

# 3. 验证核心组件状态
kubectl get pods -n kube-system | grep -v Running

# 4. 验证业务 Pod 状态
kubectl get pods -A | grep -v 'Running\|Completed'

# 5. 验证 Service 可用性
kubectl get svc -A | grep LoadBalancer
# 测试 SLB 可达性

# 6. 验证存储
kubectl get pvc -A
kubectl get pv
```

---

## 费曼复述 (0.5h)

用自己的语言回答以下问题:

1. **ACK 集群升级分哪两个阶段？各自的操作方式是什么？**
   - 提示: 管控面 (托管版自动) + 节点 (原地/替换)

2. **为什么推荐替换升级而不是原地升级？**
   - 提示: 风险可控、可回滚、不影响业务

3. **升级前使用 kubent 工具检查什么？为什么重要？**
   - 提示: 检查已废弃的 API 版本，避免升级后资源无法管理

---

## 今日检验

- [ ] 能说出 ACK 集群升级的两个阶段
- [ ] 能进行升级前的兼容性检查
- [ ] 能通过替换方式完成节点升级
- [ ] 能完成升级后的全面验证

---

## 核心概念总结

| 升级方式 | 优点 | 缺点 | 适用场景 |
|----------|------|------|---------|
| 管控面升级 | 托管版自动，无需操作 | 不可回滚 | 所有托管版集群 |
| 原地升级 | 操作简单 | 风险高，影响业务 | 测试环境 |
| 替换升级 | 风险可控，可回滚 | 需要额外资源 | 生产环境推荐 |

---

## 明日预告

Day 7 将学习集群证书管理，理解证书类型、过期处理和轮换机制。
