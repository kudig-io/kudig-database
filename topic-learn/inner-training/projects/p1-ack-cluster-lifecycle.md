# P1: ACK 集群生命周期管理

> **对应周次**: Week 1 | **预计时间**: 3-4 小时 | **难度**: ⭐⭐

---

## 项目目标

独立完成 ACK 集群的全生命周期操作：从规划网络、创建集群、升级版本到最终删除清理，全程使用 aliyun CLI + 控制台双通道操作。

## 前置条件

- [ ] 完成 Week 1 全部教案 (Day 1-7)
- [ ] 已安装并配置 aliyun CLI
- [ ] 拥有测试账号的 RAM 权限 (cs:FullAccess)
- [ ] 了解 VPC/vSwitch 网络基础

---

## 实施步骤

### Step 1: 网络规划与 VPC 创建 (30min)

```bash
# 1.1 规划网络 CIDR
# VPC CIDR:      172.16.0.0/12
# Pod CIDR:      10.0.0.0/16 (Flannel) 或 Pod vSwitch (Terway)
# Service CIDR:  192.168.0.0/16

# 1.2 创建 VPC (如无可用 VPC)
aliyun vpc CreateVpc \
  --RegionId cn-hangzhou \
  --CidrBlock 172.16.0.0/12 \
  --VpcName "ack-training-vpc"

# 1.3 创建 vSwitch (至少 2 个可用区)
aliyun vpc CreateVSwitch \
  --RegionId cn-hangzhou \
  --ZoneId cn-hangzhou-h \
  --VpcId <vpc-id> \
  --CidrBlock 172.16.0.0/24 \
  --VSwitchName "ack-training-vsw-a"

aliyun vpc CreateVSwitch \
  --RegionId cn-hangzhou \
  --ZoneId cn-hangzhou-i \
  --VpcId <vpc-id> \
  --CidrBlock 172.16.1.0/24 \
  --VSwitchName "ack-training-vsw-b"
```

### Step 2: 创建 ACK 托管版集群 (30min)

```bash
# 2.1 通过 API 创建集群
aliyun cs POST /clusters --body '{
  "name": "training-cluster-01",
  "cluster_type": "ManagedKubernetes",
  "kubernetes_version": "1.28.9-aliyun.1",
  "region_id": "cn-hangzhou",
  "vpcid": "<vpc-id>",
  "container_cidr": "10.0.0.0/16",
  "service_cidr": "192.168.0.0/16",
  "vswitch_ids": ["<vsw-a-id>", "<vsw-b-id>"],
  "num_of_nodes": 0,
  "endpoint_public_access": true,
  "snat_entry": true,
  "addons": [
    {"name": "flannel"},
    {"name": "csi-plugin"},
    {"name": "csi-provisioner"},
    {"name": "nginx-ingress-controller"}
  ]
}'

# 2.2 查看集群创建进度
aliyun cs GET /clusters/<cluster_id>

# 2.3 获取 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config
# 保存到 ~/.kube/config

# 2.4 验证连接
kubectl cluster-info
kubectl get nodes
```

### Step 3: 添加节点池 (30min)

```bash
# 3.1 创建系统节点池
aliyun cs POST /clusters/<cluster_id>/nodepools --body '{
  "nodepool_info": {"name": "system-pool"},
  "scaling_group": {
    "vswitch_ids": ["<vsw-a-id>"],
    "instance_types": ["ecs.g6.xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "desired_size": 2
  },
  "kubernetes_config": {
    "labels": [{"key": "node-role", "value": "system"}],
    "taints": [{"key": "CriticalAddonsOnly", "value": "true", "effect": "NoSchedule"}]
  }
}'

# 3.2 创建业务节点池
aliyun cs POST /clusters/<cluster_id>/nodepools --body '{
  "nodepool_info": {"name": "app-pool"},
  "scaling_group": {
    "vswitch_ids": ["<vsw-a-id>", "<vsw-b-id>"],
    "instance_types": ["ecs.g6.xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "desired_size": 2
  },
  "kubernetes_config": {
    "labels": [{"key": "node-role", "value": "app"}]
  }
}'

# 3.3 等待节点就绪
kubectl get nodes -w
```

### Step 4: 集群升级 (30min)

```bash
# 4.1 查看当前版本
kubectl version

# 4.2 查看可升级版本
aliyun cs GET /clusters/<cluster_id>/upgradestatus

# 4.3 升级管控面 (如有可用升级)
# 注意: 先在测试环境验证
aliyun cs POST /clusters/<cluster_id>/upgrade \
  --body '{"version": "<target-version>"}'

# 4.4 查看升级状态
aliyun cs GET /clusters/<cluster_id>/upgradestatus

# 4.5 升级节点 (替换升级)
# 通过控制台: 集群 → 节点池 → 升级
```

### Step 5: 集群删除与清理 (30min)

```bash
# 5.1 删除业务资源
kubectl delete all --all -n default

# 5.2 删除集群 (保留 SLB 等关联资源需手动确认)
aliyun cs DELETE /clusters/<cluster_id> \
  --body '{"retain_all_resources": false}'

# 5.3 确认清理
aliyun cs GET /clusters/<cluster_id>

# 5.4 清理 VPC 资源 (可选)
# aliyun vpc DeleteVSwitch --VSwitchId <vsw-id>
# aliyun vpc DeleteVpc --VpcId <vpc-id>
```

---

## 验收清单

- [ ] 成功规划网络 CIDR 并创建 VPC/vSwitch
- [ ] 通过 API 创建 ACK 托管版集群
- [ ] 创建了系统节点池和业务节点池
- [ ] 完成管控面升级 (或了解升级流程)
- [ ] 正确删除集群并清理关联资源
- [ ] 全程记录了操作步骤和遇到的问题

---

## 注意事项

- 创建集群会产生费用 (ECS、SLB 等)，完成后及时删除
- 集群删除前确认已备份重要数据
- CIDR 规划要预留扩展空间
