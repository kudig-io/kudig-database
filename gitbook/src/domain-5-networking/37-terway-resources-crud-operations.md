# 37 - Terway 实例 CRUD 操作指南 (Terway Resources CRUD Operations)

> **适用版本**: 阿里云 ACK v1.24 - v1.32 | **Terway 版本**: v1.5+ | **最后更新**: 2026-02

---

## 概述

Terway 是阿里云开源的 Container Network Interface (CNI) 插件，专为阿里云 VPC/ENI 网络设计。本文档详细介绍 Terway 相关自定义资源 (CRD) 的增删改查操作，涵盖 PodENI、NodeNetworking、PodNetworking 等核心资源的管理。

### 核心架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          Terway CRD 资源架构                                     │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│   ┌─────────────────────────────────────────────────────────────────────────┐   │
│   │                      Kubernetes API Server                               │   │
│   │                                                                          │   │
│   │   ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐   │   │
│   │   │   PodENI    │  │NodeNetworking│  │PodNetworking│  │  ENIConfig  │   │   │
│   │   │   (CRD)     │  │    (CRD)     │  │    (CRD)    │  │  (ConfigMap)│   │   │
│   │   └──────┬──────┘  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘   │   │
│   │          │                │                │                │           │   │
│   └──────────┼────────────────┼────────────────┼────────────────┼───────────┘   │
│              │                │                │                │               │
│              ▼                ▼                ▼                ▼               │
│   ┌─────────────────────────────────────────────────────────────────────────┐   │
│   │                      Terway Controller                                   │   │
│   │                                                                          │   │
│   │   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                  │   │
│   │   │  IPAM Manager │  │ ENI Allocator│  │Policy Manager│                  │   │
│   │   └──────────────┘  └──────────────┘  └──────────────┘                  │   │
│   │                                                                          │   │
│   └──────────────────────────────┬───────────────────────────────────────────┘   │
│                                  │                                               │
│                                  ▼                                               │
│   ┌─────────────────────────────────────────────────────────────────────────┐   │
│   │                      Alicloud VPC/ENI API                                │   │
│   │                                                                          │   │
│   │   ┌──────────────┐  ┌──────────────┐  ┌──────────────┐                  │   │
│   │   │   ENI CRUD   │  │  IP Allocate │  │ SecurityGroup│                  │   │
│   │   └──────────────┘  └──────────────┘  └──────────────┘                  │   │
│   │                                                                          │   │
│   └─────────────────────────────────────────────────────────────────────────┘   │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

---

## 1. Terway CRD 资源概览

### 1.1 核心 CRD 资源列表

| CRD 名称 | API 版本 | 作用域 | 功能描述 |
|:---|:---|:---|:---|
| **PodENI** | network.alibabacloud.com/v1beta1 | Namespaced | Pod 弹性网卡资源配置 |
| **NodeNetworking** | network.alibabacloud.com/v1beta1 | Cluster | 节点网络资源配置 |
| **PodNetworking** | network.alibabacloud.com/v1beta1 | Cluster | Pod 网络策略配置 |
| **ReservedIP** | network.alibabacloud.com/v1beta1 | Cluster | 固定 IP 保留资源 |
| **IPInstance** | network.alibabacloud.com/v1beta1 | Cluster | IP 实例管理 |

### 1.2 查看集群 CRD 资源

```bash
# 查看 Terway 相关 CRD
kubectl get crd | grep -E 'network.alibabacloud|terway'

# 输出示例:
# ipinstances.network.alibabacloud.com           2025-01-15T08:30:00Z
# nodenetworkings.network.alibabacloud.com       2025-01-15T08:30:00Z
# podenis.network.alibabacloud.com               2025-01-15T08:30:00Z
# podnetworkings.network.alibabacloud.com        2025-01-15T08:30:00Z
# reservedips.network.alibabacloud.com           2025-01-15T08:30:00Z

# 查看 CRD 详细定义
kubectl explain podenis.network.alibabacloud.com
kubectl explain nodenetworkings.network.alibabacloud.com
```

---

## 2. PodENI 资源 CRUD 操作

PodENI 是 Terway 的核心 CRD，用于管理 Pod 与弹性网卡 (ENI) 的绑定关系。

### 2.1 PodENI 资源结构

```yaml
# PodENI CRD 结构定义
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: podenis.network.alibabacloud.com
spec:
  group: network.alibabacloud.com
  versions:
    - name: v1beta1
      served: true
      storage: true
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                pod:
                  type: object
                  properties:
                    name: { type: string }
                    namespace: { type: string }
                    uid: { type: string }
                eni:
                  type: object
                  properties:
                    id: { type: string }
                    mac: { type: string }
                    securityGroupIDs: { type: array, items: { type: string } }
                    vSwitchID: { type: string }
                allocation:
                  type: object
                  properties:
                    ipType: { type: string }
                    ipv4: { type: string }
                    ipv6: { type: string }
            status:
              type: object
              properties:
                phase: { type: string }
                eniIP: { type: string }
                message: { type: string }
```

### 2.2 Create - 创建 PodENI 资源

#### 2.2.1 通过 Pod Annotation 自动创建

```yaml
# 方式一: 通过 Pod Annotation 自动触发创建
apiVersion: v1
kind: Pod
metadata:
  name: app-with-eni
  namespace: production
  annotations:
    # 启用独占 ENI 模式
    k8s.aliyun.com/eni: "true"
    
    # 指定安全组 (多个用逗号分隔)
    k8s.aliyun.com/security-group: "sg-xxx,sg-yyy"
    
    # 指定 vSwitch
    k8s.aliyun.com/vswitch-ids: "vsw-xxx,vsw-yyy"
    
    # 固定 IP 地址
    k8s.aliyun.com/pod-ip-fixed: "true"
    
    # IP 地址保留时间 (小时)
    k8s.aliyun.com/pod-ip-retain-hour: "48"
spec:
  containers:
    - name: app
      image: nginx:latest
      ports:
        - containerPort: 80
  # ENI 独占模式需要资源声明
  resources:
    limits:
      aliyun/eni: "1"
    requests:
      aliyun/eni: "1"
```

#### 2.2.2 手动创建 PodENI 资源

```yaml
# 方式二: 手动创建 PodENI (高级场景)
apiVersion: network.alibabacloud.com/v1beta1
kind: PodENI
metadata:
  name: podeni-app-with-eni
  namespace: production
  labels:
    app: myapp
    environment: production
spec:
  pod:
    name: app-with-eni
    namespace: production
    uid: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  eni:
    securityGroupIDs:
      - sg-xxxxxxxxxx
      - sg-yyyyyyyyyy
    vSwitchID: vsw-xxxxxxxxxx
  allocation:
    ipType: "IPv4"  # IPv4, IPv6, DualStack
    ipv4: "172.16.1.100"  # 可选: 指定 IP
```

#### 2.2.3 StatefulSet 固定 IP 配置

```yaml
# StatefulSet 固定 IP 完整示例
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-stateful
  namespace: database
  annotations:
    # 启用固定 IP
    k8s.aliyun.com/pod-ip-fixed: "true"
spec:
  serviceName: mysql
  replicas: 3
  selector:
    matchLabels:
      app: mysql
  template:
    metadata:
      labels:
        app: mysql
      annotations:
        # Pod 级别固定 IP 注解
        k8s.aliyun.com/pod-ip-fixed: "true"
        # 指定 vSwitch (可选)
        k8s.aliyun.com/vswitch-ids: "vsw-xxxxxx"
    spec:
      containers:
        - name: mysql
          image: mysql:8.0
          ports:
            - containerPort: 3306
          env:
            - name: MYSQL_ROOT_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: mysql-secret
                  key: password
          resources:
            requests:
              cpu: "500m"
              memory: "1Gi"
            limits:
              cpu: "2000m"
              memory: "4Gi"
          volumeMounts:
            - name: data
              mountPath: /var/lib/mysql
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        accessModes: ["ReadWriteOnce"]
        storageClassName: alicloud-disk-essd
        resources:
          requests:
            storage: 100Gi
```

### 2.3 Read - 查询 PodENI 资源

```bash
# ============================================
# 基础查询命令
# ============================================

# 列出所有 PodENI 资源
kubectl get podenis -A

# 列出指定命名空间的 PodENI
kubectl get podenis -n production

# 查看特定 PodENI 详情
kubectl get podeni <podeni-name> -n <namespace> -o yaml

# 以表格形式显示关键信息
kubectl get podenis -A -o custom-columns=\
NAMESPACE:.metadata.namespace,\
NAME:.metadata.name,\
POD:.spec.pod.name,\
PHASE:.status.phase,\
ENI_IP:.status.eniIP

# ============================================
# 高级查询命令
# ============================================

# 按 Label 筛选
kubectl get podenis -l app=mysql -A

# 按 Phase 筛选
kubectl get podenis -A --field-selector status.phase=Bound

# 查看特定 Pod 关联的 PodENI
kubectl get podeni -n production \
  -o jsonpath='{.items[?(@.spec.pod.name=="app-with-eni")].metadata.name}'

# 导出 PodENI 列表到文件
kubectl get podenis -A -o yaml > podenis-backup.yaml

# 查询 PodENI 事件
kubectl describe podeni <podeni-name> -n <namespace>

# ============================================
# 状态监控脚本
# ============================================

# 检查所有 PodENI 绑定状态
cat << 'EOF' > check-podeni-status.sh
#!/bin/bash
echo "=== PodENI Status Report ==="
echo "Timestamp: $(date)"
echo ""

# 统计各状态数量
echo "Status Distribution:"
kubectl get podenis -A -o json | jq -r '
  .items | group_by(.status.phase) | 
  .[] | "\(.[] | .status.phase): \(length)"' | sort | uniq -c

echo ""
echo "Failed PodENIs:"
kubectl get podenis -A -o json | jq -r '
  .items[] | select(.status.phase == "Failed") |
  "\(.metadata.namespace)/\(.metadata.name): \(.status.message)"'

echo ""
echo "Pending PodENIs:"
kubectl get podenis -A -o json | jq -r '
  .items[] | select(.status.phase == "Pending") |
  "\(.metadata.namespace)/\(.metadata.name)"'
EOF
chmod +x check-podeni-status.sh
./check-podeni-status.sh
```

### 2.4 Update - 更新 PodENI 资源

#### 2.4.1 更新安全组

```bash
# 方式一: 直接编辑
kubectl edit podeni <podeni-name> -n <namespace>

# 方式二: 使用 patch 更新安全组
kubectl patch podeni <podeni-name> -n <namespace> --type='json' -p='[
  {
    "op": "replace",
    "path": "/spec/eni/securityGroupIDs",
    "value": ["sg-new-xxxxx", "sg-new-yyyyy"]
  }
]'

# 方式三: 使用 YAML 文件更新
cat << 'EOF' > update-podeni.yaml
apiVersion: network.alibabacloud.com/v1beta1
kind: PodENI
metadata:
  name: podeni-app-with-eni
  namespace: production
spec:
  eni:
    securityGroupIDs:
      - sg-updated-xxxxx
      - sg-updated-yyyyy
EOF
kubectl apply -f update-podeni.yaml
```

#### 2.4.2 更新 vSwitch 配置

```bash
# 更新 PodENI 的 vSwitch
kubectl patch podeni <podeni-name> -n <namespace> --type='merge' -p='
{
  "spec": {
    "eni": {
      "vSwitchID": "vsw-new-xxxxx"
    }
  }
}'
```

#### 2.4.3 通过 Annotation 更新 Pod 网络配置

```bash
# 为现有 Pod 添加安全组 Annotation (需要 Pod 重建)
kubectl annotate pod <pod-name> -n <namespace> \
  k8s.aliyun.com/security-group="sg-xxxxx" \
  --overwrite

# 移除固定 IP 配置
kubectl annotate pod <pod-name> -n <namespace> \
  k8s.aliyun.com/pod-ip-fixed- \
  --overwrite
```

### 2.5 Delete - 删除 PodENI 资源

```bash
# ============================================
# 基础删除命令
# ============================================

# 删除单个 PodENI
kubectl delete podeni <podeni-name> -n <namespace>

# 删除指定命名空间所有 PodENI
kubectl delete podenis -n <namespace> --all

# 强制删除 (解决 Finalizer 阻塞问题)
kubectl delete podeni <podeni-name> -n <namespace> \
  --force --grace-period=0

# ============================================
# 批量删除操作
# ============================================

# 按 Label 删除
kubectl delete podenis -l app=deprecated-app -A

# 删除特定状态的 PodENI
kubectl get podenis -A -o json | \
  jq -r '.items[] | select(.status.phase=="Failed") | 
  "-n \(.metadata.namespace) \(.metadata.name)"' | \
  xargs -r kubectl delete podeni

# ============================================
# 清理 Finalizer (高级操作)
# ============================================

# 当 PodENI 无法正常删除时，清理 Finalizer
kubectl patch podeni <podeni-name> -n <namespace> \
  -p '{"metadata":{"finalizers":[]}}' --type=merge

# 批量清理所有 Terminating 状态的 PodENI
for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}'); do
  for podeni in $(kubectl get podenis -n $ns --field-selector=metadata.deletionTimestamp!=nil -o jsonpath='{.items[*].metadata.name}'); do
    echo "Cleaning finalizer for $podeni in $ns"
    kubectl patch podeni $podeni -n $ns -p '{"metadata":{"finalizers":[]}}' --type=merge
  done
done
```

---

## 3. NodeNetworking 资源 CRUD 操作

NodeNetworking CRD 用于管理节点的网络资源配置，包括 ENI 池管理和 IP 地址分配策略。

### 3.1 NodeNetworking 资源结构

```yaml
# NodeNetworking CRD 结构定义
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: nodenetworkings.network.alibabacloud.com
spec:
  group: network.alibabacloud.com
  versions:
    - name: v1beta1
      served: true
      storage: true
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                nodeName:
                  type: string
                eniOptions:
                  type: object
                  properties:
                    maxENI: { type: integer }
                    minENI: { type: integer }
                    vSwitchIDs: { type: array, items: { type: string } }
                    securityGroupIDs: { type: array, items: { type: string } }
                ipam:
                  type: object
                  properties:
                    maxIPPerENI: { type: integer }
                    poolSize: { type: integer }
                    policy: { type: string }
            status:
              type: object
              properties:
                eniCount: { type: integer }
                availableIPs: { type: integer }
                allocatedIPs: { type: integer }
```

### 3.2 Create - 创建 NodeNetworking 资源

```yaml
# 创建 NodeNetworking 资源
apiVersion: network.alibabacloud.com/v1beta1
kind: NodeNetworking
metadata:
  name: node-networking-worker-01
  labels:
    node-type: worker
    zone: cn-hangzhou-h
spec:
  nodeName: worker-node-01
  eniOptions:
    # ENI 池配置
    maxENI: 8          # 最大 ENI 数量
    minENI: 2          # 最小 ENI 数量 (预热)
    vSwitchIDs:        # Pod 使用的 vSwitch
      - vsw-xxxxx-zone-h
      - vsw-yyyyy-zone-h
    securityGroupIDs:  # 安全组
      - sg-xxxxxxxxxx
  ipam:
    # IP 地址管理配置
    maxIPPerENI: 30    # 每个 ENI 最大 IP 数
    poolSize: 25       # IP 池大小
    policy: "ordered"  # 分配策略: ordered | random
```

### 3.3 Read - 查询 NodeNetworking 资源

```bash
# ============================================
# 基础查询命令
# ============================================

# 列出所有 NodeNetworking
kubectl get nodenetworkings

# 查看特定节点的网络配置
kubectl get nodenetworking <node-name> -o yaml

# 表格形式显示关键信息
kubectl get nodenetworkings -o wide

# 自定义列显示
kubectl get nodenetworkings -o custom-columns=\
NAME:.metadata.name,\
NODE:.spec.nodeName,\
ENI_COUNT:.status.eniCount,\
AVAILABLE_IP:.status.availableIPs,\
ALLOCATED_IP:.status.allocatedIPs

# ============================================
# 节点网络状态监控
# ============================================

# 查看所有节点的 ENI 和 IP 使用情况
cat << 'EOF' > check-node-networking.sh
#!/bin/bash
echo "=== Node Networking Status Report ==="
echo "Timestamp: $(date)"
echo ""

printf "%-30s %-10s %-12s %-12s %-12s\n" \
  "NODE" "ENI COUNT" "AVAILABLE" "ALLOCATED" "UTILIZATION"
echo "------------------------------------------------------------------------"

kubectl get nodenetworkings -o json | jq -r '
  .items[] | 
  "\(.spec.nodeName)|\(.status.eniCount)|\(.status.availableIPs)|\(.status.allocatedIPs)"' | \
while IFS='|' read node eni available allocated; do
  if [ -n "$available" ] && [ "$available" != "null" ] && [ "$available" -gt 0 ]; then
    utilization=$(echo "scale=2; $allocated * 100 / ($available + $allocated)" | bc)
  else
    utilization="0"
  fi
  printf "%-30s %-10s %-12s %-12s %-12s\n" "$node" "$eni" "$available" "$allocated" "${utilization}%"
done

echo ""
echo "=== Low IP Capacity Nodes (< 10 IPs) ==="
kubectl get nodenetworkings -o json | jq -r '
  .items[] | select(.status.availableIPs < 10) |
  "\(.spec.nodeName): \(.status.availableIPs) IPs available"'
EOF
chmod +x check-node-networking.sh
./check-node-networking.sh
```

### 3.4 Update - 更新 NodeNetworking 资源

```bash
# ============================================
# 更新 ENI 配置
# ============================================

# 增加 ENI 池大小
kubectl patch nodenetworking <node-name> --type='merge' -p='
{
  "spec": {
    "eniOptions": {
      "maxENI": 10,
      "minENI": 3
    }
  }
}'

# 更新 vSwitch 列表 (添加新的 vSwitch)
kubectl patch nodenetworking <node-name> --type='json' -p='[
  {
    "op": "add",
    "path": "/spec/eniOptions/vSwitchIDs/-",
    "value": "vsw-new-xxxxx"
  }
]'

# 更新安全组
kubectl patch nodenetworking <node-name> --type='json' -p='[
  {
    "op": "replace",
    "path": "/spec/eniOptions/securityGroupIDs",
    "value": ["sg-new-xxx", "sg-new-yyy"]
  }
]'

# ============================================
# 更新 IPAM 配置
# ============================================

# 更改 IP 分配策略
kubectl patch nodenetworking <node-name> --type='merge' -p='
{
  "spec": {
    "ipam": {
      "policy": "random",
      "poolSize": 30
    }
  }
}'
```

### 3.5 Delete - 删除 NodeNetworking 资源

```bash
# 删除 NodeNetworking (通常不需要手动删除)
kubectl delete nodenetworking <node-name>

# 删除所有 NodeNetworking (危险操作)
kubectl delete nodenetworkings --all
```

---

## 4. PodNetworking 资源 CRUD 操作

PodNetworking CRD 用于定义 Pod 级别的网络策略配置，实现 Pod 级别的 vSwitch、安全组隔离。

### 4.1 PodNetworking 资源结构

```yaml
# PodNetworking CRD 结构定义
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: podnetworkings.network.alibabacloud.com
spec:
  group: network.alibabacloud.com
  versions:
    - name: v1beta1
      served: true
      storage: true
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                selector:
                  type: object
                  properties:
                    matchLabels: { type: object }
                    matchExpressions: { type: array }
                vSwitchOptions:
                  type: object
                  properties:
                    vSwitchIDs: { type: array, items: { type: string } }
                    selectionPolicy: { type: string }
                securityGroupOptions:
                  type: object
                  properties:
                    securityGroupIDs: { type: array, items: { type: string } }
                ipOptions:
                  type: object
                  properties:
                    allocationType: { type: string }
                    ipType: { type: string }
```

### 4.2 Create - 创建 PodNetworking 资源

```yaml
# 示例 1: 为特定应用创建独立网络策略
apiVersion: network.alibabacloud.com/v1beta1
kind: PodNetworking
metadata:
  name: production-api-networking
spec:
  # Pod 选择器
  selector:
    matchLabels:
      app: api-server
      tier: backend
    matchExpressions:
      - key: environment
        operator: In
        values:
          - production
          - staging
  # vSwitch 配置
  vSwitchOptions:
    vSwitchIDs:
      - vsw-api-zone-h
      - vsw-api-zone-i
    selectionPolicy: "ordered"  # ordered | random
  # 安全组配置
  securityGroupOptions:
    securityGroupIDs:
      - sg-api-production
      - sg-common-base
  # IP 配置
  ipOptions:
    allocationType: "ENIIP"  # ENIIP | ENI
    ipType: "IPv4"           # IPv4 | IPv6 | DualStack
---
# 示例 2: 数据库 Pod 网络隔离
apiVersion: network.alibabacloud.com/v1beta1
kind: PodNetworking
metadata:
  name: database-isolated-networking
spec:
  selector:
    matchLabels:
      app: mysql
      tier: database
  vSwitchOptions:
    vSwitchIDs:
      - vsw-database-zone-h
  securityGroupOptions:
    securityGroupIDs:
      - sg-database-isolated
      - sg-database-backup
  ipOptions:
    allocationType: "ENIIP"
    ipType: "IPv4"
```

### 4.3 Read - 查询 PodNetworking 资源

```bash
# 列出所有 PodNetworking
kubectl get podnetworkings

# 查看特定 PodNetworking 详情
kubectl get podnetworking <name> -o yaml

# 查看 PodNetworking 关联的 Pod
kubectl get pods -l app=api-server,tier=backend

# 验证 Pod 是否使用了正确的网络配置
kubectl get pod <pod-name> -o jsonpath='{.metadata.annotations.k8s\.aliyun\.com/allocated-podnetworking}'
```

### 4.4 Update - 更新 PodNetworking 资源

```bash
# 更新 PodNetworking 的 vSwitch
kubectl patch podnetworking <name> --type='json' -p='[
  {
    "op": "add",
    "path": "/spec/vSwitchOptions/vSwitchIDs/-",
    "value": "vsw-new-xxxxx"
  }
]'

# 更新安全组
kubectl patch podnetworking <name> --type='merge' -p='
{
  "spec": {
    "securityGroupOptions": {
      "securityGroupIDs": ["sg-updated-xxx"]
    }
  }
}'

# 编辑完整配置
kubectl edit podnetworking <name>
```

### 4.5 Delete - 删除 PodNetworking 资源

```bash
# 删除 PodNetworking
kubectl delete podnetworking <name>

# 删除多个 PodNetworking
kubectl delete podnetworking <name1> <name2>
```

---

## 5. ReservedIP 资源 CRUD 操作 (固定 IP 管理)

ReservedIP CRD 用于管理固定 IP 地址，支持 IP 地址的保留、回收和重新分配。

### 5.1 ReservedIP 资源结构

```yaml
# ReservedIP CRD 结构定义
apiVersion: network.alibabacloud.com/v1beta1
kind: ReservedIP
metadata:
  name: mysql-fixed-ip-0
spec:
  # IP 地址
  ip:
    ipv4: "172.16.10.100"
    # ipv6: "2001:db8::100"  # IPv6 地址
  
  # 关联信息
  association:
    podName: mysql-0
    namespace: database
    
  # 保留策略
  retention:
    enabled: true
    duration: "72h"  # IP 保留时长 (Pod 删除后)
    
  # 回收策略
  reclaimPolicy: "Retain"  # Retain | Delete
```

### 5.2 Create - 创建 ReservedIP 资源

```yaml
# 示例 1: 创建固定 IP 用于 StatefulSet
apiVersion: network.alibabacloud.com/v1beta1
kind: ReservedIP
metadata:
  name: mysql-fixed-ip-0
  namespace: database
  labels:
    app: mysql
    statefulset: mysql
    ordinal: "0"
spec:
  ip:
    ipv4: "172.16.10.100"
  association:
    podName: mysql-0
    namespace: database
  retention:
    enabled: true
    duration: "72h"
  reclaimPolicy: "Retain"
---
# 示例 2: 批量创建固定 IP
apiVersion: batch/v1
kind: Job
metadata:
  name: create-reserved-ips
  namespace: kube-system
spec:
  template:
    spec:
      serviceAccountName: terway-controller
      containers:
        - name: create-ips
          image: bitnami/kubectl:latest
          command:
            - /bin/sh
            - -c
            - |
              for i in $(seq 0 2); do
                cat <<EOF | kubectl apply -f -
              apiVersion: network.alibabacloud.com/v1beta1
              kind: ReservedIP
              metadata:
                name: mysql-fixed-ip-${i}
                namespace: database
              spec:
                ip:
                  ipv4: "172.16.10.10${i}"
                association:
                  podName: mysql-${i}
                  namespace: database
                retention:
                  enabled: true
                  duration: "72h"
                reclaimPolicy: "Retain"
              EOF
              done
      restartPolicy: OnFailure
```

### 5.3 Read - 查询 ReservedIP 资源

```bash
# ============================================
# 基础查询命令
# ============================================

# 列出所有固定 IP
kubectl get reservedips -A

# 查看特定固定 IP 详情
kubectl get reservedip <name> -n <namespace> -o yaml

# 查看特定应用的固定 IP
kubectl get reservedips -l app=mysql -A

# ============================================
# 固定 IP 状态监控
# ============================================

cat << 'EOF' > check-reserved-ips.sh
#!/bin/bash
echo "=== ReservedIP Status Report ==="
echo "Timestamp: $(date)"
echo ""

# 统计各命名空间的固定 IP 数量
echo "Fixed IPs by Namespace:"
kubectl get reservedips -A -o json | jq -r '
  .items | group_by(.metadata.namespace) |
  .[] | "\(.[] | .metadata.namespace): \(length)"' | sort

echo ""
echo "Orphaned Reserved IPs (no associated pod):"
kubectl get reservedips -A -o json | jq -r '
  .items[] | select(.spec.association.podName == null) |
  "\(.metadata.namespace)/\(.metadata.name): \(.spec.ip.ipv4)"'

echo ""
echo "Reserved IPs about to expire:"
kubectl get reservedips -A -o json | jq -r '
  .items[] | select(.status.expirationTimestamp != null) |
  "\(.metadata.namespace)/\(.metadata.name): expires at \(.status.expirationTimestamp)"'
EOF
chmod +x check-reserved-ips.sh
./check-reserved-ips.sh
```

### 5.4 Update - 更新 ReservedIP 资源

```bash
# 更新 IP 保留时长
kubectl patch reservedip <name> -n <namespace> --type='merge' -p='
{
  "spec": {
    "retention": {
      "duration": "168h"
    }
  }
}'

# 更新关联的 Pod
kubectl patch reservedip <name> -n <namespace> --type='merge' -p='
{
  "spec": {
    "association": {
      "podName": "mysql-0-new"
    }
  }
}'
```

### 5.5 Delete - 删除 ReservedIP 资源

```bash
# 删除固定 IP (会释放 IP 地址)
kubectl delete reservedip <name> -n <namespace>

# 批量删除过期固定 IP
kubectl get reservedips -A -o json | \
  jq -r '.items[] | select(.status.expired == true) | 
  "-n \(.metadata.namespace) \(.metadata.name)"' | \
  xargs -r kubectl delete reservedip

# 强制删除 (忽略 reclaimPolicy)
kubectl delete reservedip <name> -n <namespace> --force
```

---

## 6. IPInstance 资源 CRUD 操作

IPInstance CRD 用于跟踪和管理已分配的 IP 地址实例。

### 6.1 IPInstance 资源结构

```yaml
# IPInstance CRD 结构定义
apiVersion: apiextensions.k8s.io/v1
kind: CustomResourceDefinition
metadata:
  name: ipinstances.network.alibabacloud.com
spec:
  group: network.alibabacloud.com
  versions:
    - name: v1beta1
      served: true
      storage: true
      schema:
        openAPIV3Schema:
          type: object
          properties:
            spec:
              type: object
              properties:
                ip:
                  type: object
                  properties:
                    ipv4: { type: string }
                    ipv6: { type: string }
                    ipType: { type: string }
                eni:
                  type: object
                  properties:
                    id: { type: string }
                    mac: { type: string }
                pod:
                  type: object
                  properties:
                    name: { type: string }
                    namespace: { type: string }
                    uid: { type: string }
            status:
              type: object
              properties:
                phase: { type: string }
                nodeName: { type: string }
                vSwitchID: { type: string }
                securityGroupIDs: { type: array, items: { type: string } }
```

### 6.2 Read - 查询 IPInstance 资源

```bash
# ============================================
# 基础查询命令
# ============================================

# 列出所有 IP 实例
kubectl get ipinstances -A

# 查看特定 IP 实例详情
kubectl get ipinstance <name> -o yaml

# 按节点筛选 IP 实例
kubectl get ipinstances -A --field-selector spec.nodeName=<node-name>

# ============================================
# IP 使用情况分析
# ============================================

cat << 'EOF' > analyze-ip-usage.sh
#!/bin/bash
echo "=== IP Instance Usage Analysis ==="
echo "Timestamp: $(date)"
echo ""

# 统计各节点 IP 使用情况
echo "IP Distribution by Node:"
kubectl get ipinstances -A -o json | jq -r '
  .items | group_by(.status.nodeName) |
  .[] | "\(.[] | .status.nodeName): \(length) IPs"'

echo ""
echo "IP Distribution by Namespace:"
kubectl get ipinstances -A -o json | jq -r '
  .items | group_by(.spec.pod.namespace) |
  .[] | "\(.[] | .spec.pod.namespace): \(length) IPs"' | sort -t: -k2 -nr

echo ""
echo "IP Distribution by vSwitch:"
kubectl get ipinstances -A -o json | jq -r '
  .items | group_by(.status.vSwitchID) |
  .[] | "\(.[] | .status.vSwitchID): \(length) IPs"'

echo ""
echo "Unassigned IPs:"
kubectl get ipinstances -A -o json | jq -r '
  .items[] | select(.spec.pod.name == null or .spec.pod.name == "") |
  "\(.metadata.name): \(.spec.ip.ipv4)"'
EOF
chmod +x analyze-ip-usage.sh
./analyze-ip-usage.sh
```

---

## 7. ConfigMap 配置管理

### 7.1 eni-config ConfigMap

```yaml
# eni-config 配置示例
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      
      # IP 池配置
      "max_pool_size": 25,
      "min_pool_size": 10,
      
      # 认证配置
      "credential_path": "/var/addon/token-config",
      
      # vSwitch 配置 (多可用区)
      "vswitches": {
        "cn-hangzhou-h": ["vsw-xxx1", "vsw-xxx2"],
        "cn-hangzhou-i": ["vsw-yyy1", "vsw-yyy2"]
      },
      
      # 安全组配置
      "security_groups": ["sg-xxxxxxxxxx"],
      
      # Service CIDR
      "service_cidr": "172.21.0.0/20",
      
      # vSwitch 选择策略
      "vswitch_selection_policy": "ordered",
      
      # ENI 绑定模式
      "eni_binding_mode": "ENIIP",
      
      # IP 类型
      "ip_type": "IPv4"
    }

---
# terway-config 配置示例
apiVersion: v1
kind: ConfigMap
metadata:
  name: terway-config
  namespace: kube-system
data:
  terway_config: |
    {
      # NetworkPolicy 实现
      "network_policy": "cilium",
      
      # eBPF 加速
      "enable_ebpf": true,
      
      # kube-proxy 替换
      "kube_proxy_replacement": "strict",
      
      # 带宽管理
      "enable_bandwidth_manager": true,
      
      # 伪装模式
      "enable_bpf_masquerade": true
    }
```

### 7.2 ConfigMap CRUD 操作

```bash
# ============================================
# 查询配置
# ============================================

# 查看 eni-config
kubectl get cm eni-config -n kube-system -o yaml

# 查看 terway-config
kubectl get cm terway-config -n kube-system -o yaml

# 提取 JSON 配置
kubectl get cm eni-config -n kube-system -o jsonpath='{.data.eni_conf}' | jq .

# ============================================
# 更新配置
# ============================================

# 编辑配置
kubectl edit cm eni-config -n kube-system

# 添加 vSwitch
kubectl get cm eni-config -n kube-system -o json | \
  jq '.data.eni_conf = (.data.eni_conf | fromjson | 
  .vswitches["cn-hangzhou-h"] += ["vsw-new-xxx"] | tojson)' | \
  kubectl apply -f -

# 更新 IP 池大小
kubectl patch cm eni-config -n kube-system --type='json' -p='[
  {
    "op": "replace",
    "path": "/data/eni_conf",
    "value": "{\"version\":\"1\",\"max_pool_size\":30,\"min_pool_size\":15}"
  }
]'

# ============================================
# 配置生效操作
# ============================================

# 更新配置后重启 Terway Pod 使配置生效
kubectl delete pods -n kube-system -l app=terway-eniip

# 验证配置是否生效
kubectl logs -n kube-system -l app=terway-eniip -c terway --tail=50 | grep -i config
```

---

## 8. 常用 Annotation 速查表

### 8.1 Pod 级别 Annotation

| Annotation | 值 | 说明 |
|:---|:---|:---|
| `k8s.aliyun.com/eni` | "true" | 启用独占 ENI 模式 |
| `k8s.aliyun.com/pod-ip-fixed` | "true" | 启用固定 IP |
| `k8s.aliyun.com/pod-ip-retain-hour` | "48" | IP 保留时间 (小时) |
| `k8s.aliyun.com/security-group` | "sg-xxx" | 指定安全组 |
| `k8s.aliyun.com/vswitch-ids` | "vsw-xxx,vsw-yyy" | 指定 vSwitch |
| `k8s.aliyun.com/ignore-insecure` | "true" | 忽略镜像 insecure |

### 8.2 StatefulSet 级别 Annotation

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  annotations:
    # 固定 IP 配置
    k8s.aliyun.com/pod-ip-fixed: "true"
    # IP 保留时间
    k8s.aliyun.com/pod-ip-retain-hour: "72"
    # 指定 vSwitch
    k8s.aliyun.com/vswitch-ids: "vsw-xxxxx"
spec:
  # ... StatefulSet 配置
```

### 8.3 Node 级别 Annotation

```bash
# 查看节点 ENI 信息
kubectl get node <node-name> -o jsonpath='{.metadata.annotations}'

# 关键 Annotation:
# k8s.aliyun.com/allocated-eniips: 已分配的 ENI IP 列表
# k8s.aliyun.com/enipool-ip: ENI 池 IP 信息
# k8s.aliyun.com/node-network-policy: 节点网络策略
```

---

## 9. 故障排查指南

### 9.1 常见问题诊断

```bash
# ============================================
# Pod 网络问题诊断
# ============================================

# 1. 检查 Pod 创建状态
kubectl describe pod <pod-name> -n <namespace>

# 2. 检查 Terway 日志
kubectl logs -n kube-system -l app=terway-eniip -c terway --tail=200 | grep -i error

# 3. 检查 PodENI 状态
kubectl get podeni -n <namespace> -o wide

# 4. 检查节点 ENI 配额
kubectl get node <node-name> -o jsonpath='{.metadata.annotations.k8s\.aliyun\.com/allocated-eniips}'

# ============================================
# IP 分配问题诊断
# ============================================

# 检查 vSwitch IP 使用情况
aliyun ecs DescribeVSwitches --VSwitchId vsw-xxxxx

# 检查 ENI 配额
aliyun ecs DescribeInstanceTypes --InstanceTypes ecs.g7.xlarge

# 检查 IPInstance 状态
kubectl get ipinstances -A -o wide

# ============================================
# 固定 IP 问题诊断
# ============================================

# 检查 ReservedIP 状态
kubectl get reservedips -A -o wide

# 检查 IP 是否被正确保留
kubectl get reservedip <name> -n <namespace> -o yaml

# 检查 StatefulSet Pod 的 IP 绑定
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.metadata.annotations}'
```

### 9.2 故障排查脚本

```bash
cat << 'EOF' > terway-diagnose.sh
#!/bin/bash

echo "=========================================="
echo "  Terway Network Diagnostics Tool"
echo "  Timestamp: $(date)"
echo "=========================================="
echo ""

# 1. 检查 Terway 组件状态
echo "=== [1] Terway Component Status ==="
kubectl get pods -n kube-system -l app=terway-eniip -o wide
echo ""

# 2. 检查 CRD 资源统计
echo "=== [2] CRD Resource Statistics ==="
echo "PodENIs: $(kubectl get podenis -A --no-headers | wc -l)"
echo "NodeNetworkings: $(kubectl get nodenetworkings --no-headers | wc -l)"
echo "PodNetworkings: $(kubectl get podnetworkings --no-headers | wc -l)"
echo "ReservedIPs: $(kubectl get reservedips -A --no-headers | wc -l)"
echo "IPInstances: $(kubectl get ipinstances -A --no-headers | wc -l)"
echo ""

# 3. 检查 Pending/Failed 状态的资源
echo "=== [3] Abnormal Status Resources ==="
echo "Pending PodENIs:"
kubectl get podenis -A --field-selector status.phase=Pending 2>/dev/null || echo "None"
echo ""
echo "Failed PodENIs:"
kubectl get podenis -A --field-selector status.phase=Failed 2>/dev/null || echo "None"
echo ""

# 4. 检查节点 IP 容量
echo "=== [4] Node IP Capacity ==="
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
  allocated=$(kubectl get pods -A --field-selector spec.nodeName=$node -o json | \
    jq -r '.items | length')
  max_pod=$(kubectl get node $node -o jsonpath='{.status.capacity.pods}')
  echo "$node: $allocated / $max_pod pods"
done
echo ""

# 5. 最近的 Terway 错误日志
echo "=== [5] Recent Terway Errors (last 10) ==="
kubectl logs -n kube-system -l app=terway-eniip -c terway --tail=500 2>/dev/null | \
  grep -i "error\|warn\|fail" | tail -10
echo ""

# 6. 网络策略检查
echo "=== [6] NetworkPolicy Status ==="
echo "NetworkPolicy mode: $(kubectl get cm terway-config -n kube-system -o jsonpath='{.data.terway_config}' 2>/dev/null | jq -r '.network_policy // "iptables"' 2>/dev/null || echo "iptables")"
echo ""

echo "=========================================="
echo "  Diagnostics Complete"
echo "=========================================="
EOF
chmod +x terway-diagnose.sh
./terway-diagnose.sh
```

---

## 10. 最佳实践

### 10.1 生产环境推荐配置

```yaml
# 生产环境 eni-config 推荐配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      
      # IP 池: 根据节点规格调整
      "max_pool_size": 25,
      "min_pool_size": 10,
      
      # 多可用区 vSwitch (高可用)
      "vswitches": {
        "cn-hangzhou-h": ["vsw-prod-h1", "vsw-prod-h2"],
        "cn-hangzhou-i": ["vsw-prod-i1", "vsw-prod-i2"]
      },
      
      # 生产安全组
      "security_groups": ["sg-prod-base"],
      
      # 负载均衡策略
      "vswitch_selection_policy": "random",
      
      # 推荐模式
      "eni_binding_mode": "ENIIP"
    }

---
# 生产环境 terway-config 推荐配置  
apiVersion: v1
kind: ConfigMap
metadata:
  name: terway-config
  namespace: kube-system
data:
  terway_config: |
    {
      # 使用 Cilium eBPF 实现 NetworkPolicy
      "network_policy": "cilium",
      "enable_ebpf": true,
      "kube_proxy_replacement": "strict",
      "enable_bandwidth_manager": true,
      "enable_bpf_masquerade": true
    }
```

### 10.2 运维检查清单

```markdown
## 日常运维检查清单

### 每日检查
- [ ] Terway Pod 运行状态正常
- [ ] 无 Pending/Failed 状态的 PodENI
- [ ] 各节点 IP 容量充足 (>20%)
- [ ] vSwitch IP 资源充足

### 每周检查
- [ ] 固定 IP 清理 (过期/孤儿 IP)
- [ ] Terway 组件版本检查
- [ ] 网络策略合规审计
- [ ] IP 使用趋势分析

### 每月检查
- [ ] 容量规划评估
- [ ] 安全组规则审计
- [ ] 备份配置和 CRD 资源
- [ ] 性能基准测试
```

### 10.3 容量规划参考

| 集群规模 | 节点数量 | 建议 vSwitch CIDR | 说明 |
|:---|:---|:---|:---|
| 小型 | <50 | /20 (4096 IP) | 每个 AZ 一个 vSwitch |
| 中型 | 50-200 | /19 (8192 IP) | 每个 AZ 2个 vSwitch |
| 大型 | 200-500 | /18 (16384 IP) | 每个 AZ 多个 vSwitch |
| 超大型 | >500 | /16 (65536 IP) | 分层 vSwitch 设计 |

---

## 11. 命令速查表

```bash
# ============================================
# PodENI 操作
# ============================================
kubectl get podenis -A                           # 列出所有 PodENI
kubectl get podeni <name> -n <ns> -o yaml        # 查看 PodENI 详情
kubectl delete podeni <name> -n <ns>             # 删除 PodENI
kubectl patch podeni <name> -n <ns> -p '...'     # 更新 PodENI

# ============================================
# NodeNetworking 操作
# ============================================
kubectl get nodenetworkings                      # 列出所有 NodeNetworking
kubectl get nodenetworking <name> -o yaml        # 查看 NodeNetworking 详情
kubectl patch nodenetworking <name> -p '...'     # 更新 NodeNetworking

# ============================================
# ReservedIP 操作
# ============================================
kubectl get reservedips -A                       # 列出所有固定 IP
kubectl apply -f reservedip.yaml                 # 创建固定 IP
kubectl delete reservedip <name> -n <ns>         # 删除固定 IP

# ============================================
# IPInstance 操作
# ============================================
kubectl get ipinstances -A                       # 列出所有 IP 实例
kubectl get ipinstances -A --field-selector spec.nodeName=<node>  # 按节点筛选

# ============================================
# ConfigMap 操作
# ============================================
kubectl get cm eni-config -n kube-system -o yaml # 查看 ENI 配置
kubectl edit cm eni-config -n kube-system        # 编辑 ENI 配置
kubectl delete pods -n kube-system -l app=terway-eniip  # 重启 Terway

# ============================================
# 故障排查
# ============================================
kubectl logs -n kube-system -l app=terway-eniip -c terway --tail=200  # 查看日志
kubectl describe podeni <name> -n <ns>           # 查看 PodENI 事件
```

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)
