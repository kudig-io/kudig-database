---
title: ACK集群运维
description: 阿里云ACK专有版集群管理、节点池运维、日志监控与安全配置完整指南
summary: ACK专有版与托管版集群的运维管理、日志监控及安全配置指南。
category: cloud-provider
tags:
- alibaba-cloud
- ack
- kubernetes
- cluster-management
- node-pool
- monitoring
- security
- autoscaling
tier: core
sources:
- 阿里云ACK运维手册
- ASCM 控制台操作指南
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
relationships:
- target: '[[实体/etcd.md]]'
  type: uses
- target: '[[实体/kubelet.md]]'
  type: uses
- target: '[[系统基础/知识字典/security/pod-security-policies.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# ACK集群运维

本文档覆盖阿里云 ACK 专有版集群的全生命周期运维：集群管理、节点池运维、日志监控体系与安全防护配置。面向远程顾问场景，所有命令均可通过工单指导客户在堡垒机执行。

---

## 1. ACK专有版集群管理

### 1.1 集群创建流程

ACK 专有版（Dedicated）与托管版（Managed）的核心区别：专有版需客户自管 Master 节点，托管版由阿里云托管 Master。

| 维度 | 专有版 ACK | 托管版 ACK | 专有云可用性 |
|------|------------|------------|--------------|
| Master 节点 | 客户自建/可见 | 阿里云托管 | 专有版为主 |
| [[实体/etcd.md|etcd]] | 客户自管 | 阿里云托管 | 专有版自管 |
| 适用场景 | 金融/强合规 | 通用互联网 | 专有云多专有版 |
| 运维复杂度 | 高 | 低 | 需掌握 etcd/管控面 |

```yaml
# 通过 ASCM OpenAPI 创建 ACK 专有版集群
# POST /api/v1/clusters
{
  "cluster_type": "Dedicated",
  "name": "prod-k8s-apsara",
  "region_id": "cn-apsara-local",
  "vpc_id": "vpc-apsara-xxx",
  "vswitch_ids": ["vsw-apsara-1", "vsw-apsara-2"],
  "master_instance_types": ["ecs.g7.2xlarge"],
  "num_of_masters": 3,
  "master_vswitch_ids": ["vsw-apsara-1", "vsw-apsara-2", "vsw-apsara-3"],
  "worker_instance_types": ["ecs.g7.2xlarge"],
  "num_of_nodes": 3,
  "container_cidr": "172.20.0.0/16",
  "service_cidr": "172.21.0.0/20",
  "addons": [
    {"name": "terway-eniip"},
    {"name": "csi-plugin"},
    {"name": "metrics-server"},
    {"name": "logtail-ds"}
  ],
  "kubernetes_version": "1.28.3-aliyun.1",
  "charge_type": "PostPaid",
  "key_pair": "apsara-k8s-key"
}
```

### 1.2 集群扩缩容

**垂直扩容（升降配节点规格）**：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl delete`：删除资源（可由声明式清单重建）

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 1. 查看当前节点规格
kubectl get nodes -L alibabacloud.com/ecs-instance-type

# 2. 通过 ASCM/OpenAPI 修改节点池实例规格
# 注意：垂直扩容需逐节点替换，先添加新规格节点，再驱逐旧节点

# 3. 添加新节点（命令行方式）
aliyun cs POST /clusters/<cluster-id>/nodes \
  --body '{
    "instances": ["i-apsara-new1", "i-apsara-new2"],
    "nodepool_id": "np-xxx"
  }'

# 4. 安全驱逐旧节点（远程顾问指导）
kubectl drain <old-node-name> \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --grace-period=120

# 5. 确认 Pod 迁移完成
kubectl get pods -A -o wide | grep <old-node-name>

# 6. 从集群移除节点
kubectl delete node <old-node-name>
```
**水平扩容（增减节点数）**：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 方法1: 通过 kubectl 直接调整节点池期望节点数
# 需先获取节点池 ID
aliyun cs GET /clusters/<cluster-id>/nodepools

# 方法2: 修改节点池的 scaling_group 配置
aliyun cs PUT /clusters/<cluster-id>/nodepools/<np-id> \
  --body '{
    "scaling_group": {
      "desired_size": 10,
      "min_size": 3,
      "max_size": 20
    }
  }'

# 方法3: 使用 ACK 提供的 autoscaler（见2.2节）
```
### 1.3 集群升级

ACK 专有版集群升级路径需严格遵循版本阶梯（如 1.26 → 1.28 不可跨级）。

```bash
# 远程顾问检查集群可升级版本
aliyun cs GET /clusters/<cluster-id>/upgrade_status

# 典型输出：
# {
#   "upgradeable": true,
#   "current_version": "1.26.3-aliyun.1",
#   "available_versions": ["1.28.3-aliyun.1"],
#   "risk_level": "medium",
#   "pre_check_items": [...]
# }

# 升级前置检查（关键步骤）
aliyun cs POST /clusters/<cluster-id>/upgrade_precheck

# 客户确认后发起升级
aliyun cs POST /clusters/<cluster-id>/upgrade \
  --body '{
    "version": "1.28.3-aliyun.1",
    "next_version": "1.28.3-aliyun.1"
  }'

# 监控升级进度
aliyun cs GET /clusters/<cluster-id>/upgrade_status
```

**升级注意事项**：

| 检查项 | 命令/方法 | 风险等级 |
|--------|-----------|----------|
| API 废弃检查 | `kubectl get --raw=/api/v1` | 高 |
| [[系统基础/知识字典/security/pod-security-policies.md|Pod 安全策略]] | `kubectl get psp` | 高 |
| 节点镜像预热 | 确认新节点镜像可用 | 中 |
| CRD 兼容性 | `kubectl get crd` | 中 |

---

## 2. ACK节点池管理

### 2.1 节点池类型

ACK 专有版支持多种节点池类型，适应不同工作负载：

| 节点池类型 | 适用场景 | 节点规格 | 计费模式 |
|------------|----------|----------|----------|
| **系统节点池** | kube-system、addons | 高规格（4C8G+） | 包年包月 |
| **通用节点池** | 通用业务负载 | 标准规格 | 按量/包年包月 |
| **GPU节点池** | AI训练/推理 | V100/A10/T4 | 按量 |
| **裸金属节点池** | 高性能计算 | 物理机 | 包年包月 |
| **竞价节点池** | 容错性批处理 | 共享规格 | 竞价实例 |
| **虚拟节点** | Serverless Pod | ECI | 按Pod计费 |

```yaml
# 节点池配置示例
apiVersion: cs.alibaba-cloud.com/v1
kind: NodePool
metadata:
  name: prod-worker-pool
spec:
  cluster_id: "c-apsara-xxx"
  nodepool_info:
    name: "prod-worker"
  scaling_group:
    instance_types: ["ecs.g7.2xlarge", "ecs.g7.xlarge"]
    desired_size: 6
    min_size: 3
    max_size: 20
    vswitch_ids: ["vsw-apsara-1", "vsw-apsara-2"]
    system_disk_category: "cloud_essd"
    system_disk_size: 120
    data_disks:
      - category: "cloud_essd"
        size: 500
    key_pair: "apsara-k8s-key"
    security_group_ids: ["sg-apsara-xxx"]
  kubernetes_config:
    runtime: "containerd"
    runtime_version: "1.6.20"
    cgroup_driver: "systemd"
    user_data: ""
  labels:
    node-type: "general"
    env: "production"
  taints:
    - key: "dedicated"
      value: "general"
      effect: "NoSchedule"
```

### 2.2 弹性伸缩配置

ACK 专有版通过 cluster-autoscaler 实现节点池弹性伸缩：

```yaml
# Cluster Autoscaler 配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cluster-autoscaler
  namespace: kube-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: cluster-autoscaler
  template:
    metadata:
      labels:
        app: cluster-autoscaler
    spec:
      serviceAccountName: cluster-autoscaler
      containers:
        - name: cluster-autoscaler
          image: registry-vpc.cn-apsara-local.aliyuncs.com/acs/cluster-autoscaler:v1.28.0
          command:
            - ./cluster-autoscaler
            - --cloud-provider=alicloud
            - --node-group-auto-discovery=asg:tag=k8s.io/cluster-autoscaler/enabled,k8s.io/cluster-autoscaler/<cluster-id>
            - --scale-down-enabled=true
            - --scale-down-delay-after-add=10m
            - --scale-down-unneeded-time=10m
            - --skip-nodes-with-local-storage=false
            - --skip-nodes-with-system-pods=false
            - --expander=least-waste
          resources:
            limits:
              cpu: "1"
              memory: "1Gi"
            requests:
              cpu: "100m"
              memory: "300Mi"
```

**弹性伸缩触发条件**：

| 场景 | 行为 | 配置参数 |
|------|------|----------|
| Pod 因资源不足 Pending | 扩容节点 | `--scale-up-enabled` |
| 节点利用率低于阈值 | 缩容节点 | `--scale-down-utilization-threshold=0.5` |
| 节点上有本地存储 Pod | 跳过缩容 | `--skip-nodes-with-local-storage` |
| 节点上有系统 Pod | 跳过缩容 | `--skip-nodes-with-system-pods` |
| 节点带特定标签 | 保护不缩容 | `cluster-autoscaler.kubernetes.io/scale-down-disabled: true` |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 远程诊断 CA 状态
kubectl get pods -n kube-system | grep cluster-autoscaler
kubectl logs -n kube-system deployment/cluster-autoscaler --tail=200

# 查看 CA 事件
kubectl get events -n kube-system | grep cluster-autoscaler

# 检查节点池伸缩组状态
aliyun cs GET /clusters/<cluster-id>/nodepools/<np-id>
```
### 2.3 节点池运维命令速查

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# === 节点池列表 ===
aliyun cs GET /clusters/<cluster-id>/nodepools

# === 节点池详情 ===
aliyun cs GET /clusters/<cluster-id>/nodepools/<np-id>

# === 手动伸缩节点池 ===
aliyun cs POST /clusters/<cluster-id>/nodepools/<np-id>/nodes \
  --body '{"count": 2}'

# === 移除节点 ===
aliyun cs DELETE /clusters/<cluster-id>/nodes \
  --body '{"nodes":["cn-apsara-xxxxx"],"release_node":true}'

# === 更新节点池配置 ===
aliyun cs PUT /clusters/<cluster-id>/nodepools/<np-id> \
  --body '{"scaling_group": {"desired_size": 8}}'

# === 节点打标签（kubectl）===
kubectl label nodes <node-name> workload-type=batch --overwrite

# === 节点污点管理 ===
kubectl taint nodes <node-name> gpu=true:NoSchedule
kubectl taint nodes <node-name> gpu=true:NoSchedule-
```
---

## 3. ACK日志与监控

### 3.1 SLS日志服务集成

专有云场景下，日志采集通过 Logtail DaemonSet 部署：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 远程诊断 Logtail 状态
kubectl get pods -n kube-system | grep logtail
kubectl logs -n kube-system -l k8s-app=logtail --tail=100
kubectl get aliyunlogconfigs -A
```
### 3.2 Prometheus监控

ACK 专有版内置 Prometheus 监控体系：

```yaml
# Prometheus 监控规则示例
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: ack-alerts
  namespace: monitoring
spec:
  groups:
    - name: ack-node-alerts
      rules:
        - alert: ACKNodeDiskPressure
          expr: node_filesystem_avail_bytes{mountpoint="/"} / node_filesystem_size_bytes{mountpoint="/"} < 0.1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "ACK节点磁盘压力警告"
            description: "节点 {{ $labels.instance }} 磁盘可用空间不足 10%"

        - alert: ACKNodeMemoryPressure
          expr: node_memory_MemAvailable_bytes / node_memory_MemTotal_bytes < 0.1
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "ACK节点内存压力告警"
            description: "节点 {{ $labels.instance }} 可用内存不足 10%"
```

**远程诊断监控数据**：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Prometheus 组件
kubectl get pods -n monitoring
kubectl logs -n monitoring prometheus-k8s-0 --tail=100

# 查询节点指标
kubectl top nodes
kubectl top pods -A

# 检查 metrics-server
kubectl get deployment metrics-server -n kube-system
```
---

## 4. ACK安全

### 4.1 RAM角色配置

ACK 专有版集群的 RAM 角色体系：

| 角色类型 | 用途 | 权限范围 |
|----------|------|----------|
| Master RAM Role | 管控面组件访问云资源 | 只读 ECS/SLB/VPC |
| Worker RAM Role | 节点访问云资源 | 根据功能分配 |
| ServiceAccount | Pod 级权限 | 通过RRSA/OIDC绑定RAM |

```yaml
# RRSA为Pod授权
apiVersion: v1
kind: ServiceAccount
metadata:
  name: oss-access-sa
  namespace: default
  annotations:
    ram.aliyuncs.com/role-name: "oss-readonly-role"
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: oss-reader
spec:
  template:
    spec:
      serviceAccountName: oss-access-sa
      containers:
        - name: app
          image: harbor.corp.internal/oss-reader:v1
```

```bash
# 远程检查 RAM 角色绑定
aliyun ram GetRole --RoleName ack-master-role

# 检查节点 RAM 角色
aliyun ecs DescribeInstances \
  --RegionId cn-apsara-local \
  --InstanceIds '["i-apsara-xxx"]' \
  | jq '.Instances.Instance[0].RamRoleName'
```

### 4.2 KMS密钥管理

ACK 专有版通过客户内部 KMS 或 HSM 进行密钥管理。Pod 可通过 CSI 解密卷或 Secret 注解方式使用加密数据。

| 加密场景 | 实现方式 | 远程顾问关注点 |
|----------|----------|--------------|
| Secret 加密 | KMS Secret Provider | 确认 KMS 服务可达 |
| 云盘加密 | StorageClass encrypted参数 | 确认 kmsKeyId 有效 |
| 镜像签名 | Notary/Cosign | 验证签名策略 |
| 传输加密 | mTLS | 证书有效期 |

### 4.3 安全组配置

ACK 专有版安全组规则模板：

| 方向 | 协议 | 端口 | 源/目标 | 用途 |
|------|------|------|---------|------|
| 入站 | TCP | 22 | 堡垒机IP段 | SSH管理 |
| 入站 | TCP | 6443 | 运维网段 | API Server |
| 入站 | TCP | 10250 | 节点间 | [[实体/kubelet.md|Kubelet]] |
| 入站 | TCP | 8472 | 节点间 | Flannel VXLAN |
| 入站 | UDP | 4789 | 节点间 | Terway VXLAN |
| 入站 | TCP | 30000-32767 | 负载均衡 | NodePort |
| 出站 | ALL | ALL | 0.0.0.0/0 | 节点出向 |

```bash
# 远程检查安全组
aliyun ecs DescribeSecurityGroupAttribute \
  --SecurityGroupId sg-apsara-xxx --RegionId cn-apsara-local
```

---

## 5. 远程顾问运维检查清单

### 5.1 日常巡检命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# ACK专有版日常巡检脚本

echo "=== 集群基础状态 ==="
kubectl cluster-info
kubectl version --short
kubectl get nodes -o wide

echo "=== 组件健康 ==="
kubectl get pods -n kube-system

echo "=== 资源使用 ==="
kubectl top nodes
kubectl top pods -A --sort-by=cpu | head -20

echo "=== 事件检查 ==="
kubectl get events --sort-by='.lastTimestamp' | tail -50

echo "=== PVC/PV 状态 ==="
kubectl get pv,pvc -A

echo "=== etcd 健康 ==="
ETCDCTL_API=3 etcdctl endpoint health \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```
### 5.2 常见问题快速处理

| 症状 | 根因 | 远程处理步骤 |
|------|------|--------------|
| 节点 NotReady | Kubelet异常/网络断开 | 检查节点Kubelet日志、网络连通性 |
| Pod 持续 Pending | 资源不足/污点不匹配 | 检查节点资源、节点池容量、Pod调度约束 |
| 镜像拉取失败 | 仓库不可达/认证失败 | 检查imagePullSecrets、节点网络、仓库状态 |
| DNS解析异常 | CoreDNS配置/上游DNS | 检查CoreDNS Pod、ConfigMap、上游DNS |
| 磁盘IO高 | 存储后端压力/本地盘满 | 检查PV使用率、存储后端监控、清理日志 |
| API Server慢 | etcd性能/请求突增 | 检查etcd磁盘延迟、API Server日志、限速配置 |

---

## 相关文档

- [[云厂商/阿里云/01-专有云架构概述.md|专有云架构概述]]
- [[云厂商/阿里云/03-Terway-CNI网络.md|Terway-CNI网络]]
- [[云厂商/阿里云/04-阿里云存储集成.md|阿里云存储集成]]
- [[云厂商/阿里云/05-阿里云SLB与Ingress.md|阿里云SLB与Ingress]]
- [[云厂商/阿里云/06-阿里云专有云远程顾问指南.md|阿里云专有云远程顾问指南]]
- [[alicloud-ack-overview|阿里云ACK概述]]
- [[alicloud-apsara-ack-overview|阿里云专有版ACK概述]]
## Related

- [[实体/coredns.md|CoreDNS (entities)]]
- [[系统基础/知识字典/networking/ingress.md|Ingress]]


<!-- risk-assessed -->
