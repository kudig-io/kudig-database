# Day 17: 节点池基础

> **学习时间**: 4-5 小时 | **主题**: 节点池概念与创建配置

---

## 今日目标

- [ ] 理解 ACK 节点池的概念和价值
- [ ] 掌握节点池的创建与配置
- [ ] 了解托管节点池与自管理节点池的区别
- [ ] 能通过控制台和 API 管理节点池

---

## 理论学习 (2h)

### 必读文档

1. **ACK 服务总览**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/alicloud-ack-overview.md`
   - 重点: 节点池功能与配置

2. **ECS 计算资源**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/240-ack-ecs-compute.md`
   - 重点: 实例规格选择

---

## 实践任务 (2.5h)

### 任务 1: 查看现有节点池 (30min)

```bash
# 通过 API 查看节点池
aliyun cs GET /clusters/<cluster_id>/nodepools

# 通过 kubectl 查看节点标签中的节点池信息
kubectl get nodes -o custom-columns='NAME:.metadata.name,POOL:.metadata.labels.alibabacloud\.com/nodepool-id'

# 查看节点池详情
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id>
```

### 任务 2: 创建新节点池 (45min)

```bash
# 通过 API 创建节点池
cat > create-nodepool.json << 'EOF'
{
  "nodepool_info": {
    "name": "app-pool"
  },
  "scaling_group": {
    "vswitch_ids": ["<vsw_id>"],
    "instance_types": ["ecs.g6.xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "data_disks": [
      {
        "category": "cloud_essd",
        "size": 200
      }
    ]
  },
  "kubernetes_config": {
    "node_name_mode": "customized,app-,5,suffix",
    "labels": [
      {"key": "workload", "value": "app"}
    ],
    "taints": []
  },
  "auto_scaling": {
    "enable": false
  },
  "count": 2
}
EOF

aliyun cs POST /clusters/<cluster_id>/nodepools \
  --body "$(cat create-nodepool.json)"
```

### 任务 3: 节点池配置对比 (45min)

```bash
# 托管节点池 vs 自管理节点池:
# 托管节点池:
# - 节点自动修复 (NotReady 自动替换)
# - 节点自动升级
# - CVE 自动修复
# - 推荐生产使用

# 自管理节点池:
# - 用户完全控制
# - 适合特殊配置需求

# 查看节点池管理模式
aliyun cs GET /clusters/<cluster_id>/nodepools/<nodepool_id> | jq '.management'
```

### 任务 4: 控制台节点池操作 (30min)

```
# ACK 控制台 -> 集群 -> 节点管理 -> 节点池
# 1. 查看节点池列表和状态
# 2. 创建新节点池 (通过表单)
# 3. 编辑节点池配置
# 4. 查看节点池中的节点列表
```

---

## 费曼复述 (0.5h)

1. **为什么需要节点池？它解决了什么问题？**
2. **托管节点池的"自动修复"是如何工作的？**
3. **如何为不同业务设计不同的节点池？**

---

## 今日检验

- [ ] 理解节点池的概念和价值
- [ ] 能通过 API 和控制台创建节点池
- [ ] 了解托管节点池和自管理节点池的区别
- [ ] 能查看节点池详情和节点列表

---

## 核心概念总结

| 节点池类型 | 特点 | 适用场景 |
|-----------|------|---------|
| 托管节点池 | 自动修复、自动升级 | 生产环境推荐 |
| 自管理节点池 | 完全控制 | 特殊配置需求 |

---

## 明日预告

Day 18 将学习节点池的扩缩容与生命周期管理。
