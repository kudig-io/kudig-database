---
title: 03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive)
description: 'title: 03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive)'
category: general
tags:
- terway
- networking
- cni
- cilium
- mysql
- statefulset
- job
- networkpolicy
- crd
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- 03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive) 是什么
- 如何 03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive)
- Kubernetes 03 networking traffic 最佳实践
trigger_keywords:
- 03b
- Terway
- CRD
- 深度操作指南
- CRD
- Operations
- Deep
- Dive
prerequisites:
- kubectl-basics
- networking-basics
- ebpf-basics
- cilium-basics
- mysql-basics
---

title: 03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive)
description: '# 03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive)'
category: terway
tags:
- k8s
- terway
- networking
- alicloud
- cilium
- mysql
- statefulset
- job
- networkpolicy
- crd
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
estimated_read_time: 5min
intent_queries:
- 03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive) 是什么
- 如何 03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive)
trigger_keywords:
- 03b
- Terway
- CRD
- 深度操作指南
- CRD
- Operations
- Deep
- Dive
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 03b - Terway CRD 深度操作指南 (CRD Operations Deep Dive)

> **适用版本**: 阿里云 ACK v1.25 - v1.32+ | **Terway 版本**: v1.5+ | **最后更新**: 2026-05

---

<!-- chunk: 1. CRD 资源架构 -->## 1. CRD 资源架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          Terway CRD 资源架构                                     │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│   ┌─────────────────────────────────────────────────────────────────────────┐   │
│   │                      Kubernetes API Server                               │   │
│   │                                                                          │   │
│   │   ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐   │   │
│   │   │   PodENI    │  │NodeNetworking│  │PodNetworking│  │ ReservedIP  │   │   │
│   │   │   (CRD)     │  │    (CRD)     │  │    (CRD)    │  │   IPInstance │   │   │
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

**数据流路径**:
1. 用户创建 Pod/StatefulSet, 通过 Annotation 或 PodNetworking 声明网络需求
2. Kubernetes API Server 将资源变更事件推送给 Terway Controller
3. Terway Controller 的 IPAM Manager 计算 IP 分配策略, ENI Allocator 调用阿里云 API
4. Alicloud VPC/ENI API 执行 ENI 创建/IP 分配/安全组绑定等操作
5. 结果回写至对应 CRD 的 status 字段

---

<!-- chunk: 2. CRD 全量清单 -->## 2. CRD 全量清单

| CRD 名称 | API 版本 | 作用域 | 功能描述 |
|:---|:---|:---|:---|
| **PodENI** | `network.alibabacloud.com/v1beta1` | Namespaced | Pod 弹性网卡资源配置, 管理 Pod 与 ENI 的绑定关系 |
| **NodeNetworking** | `network.alibabacloud.com/v1beta1` | Cluster | 节点网络资源配置, ENI 池管理与 IP 地址分配策略 |
| **PodNetworking** | `network.alibabacloud.com/v1beta1` | Cluster | Pod 级别网络策略, 实现 vSwitch/安全组隔离 |
| **ReservedIP** | `network.alibabacloud.com/v1beta1` | Cluster | 固定 IP 保留资源, 支持 IP 保留/回收/重新分配 |
| **IPInstance** | `network.alibabacloud.com/v1beta1` | Cluster | IP 实例管理, 跟踪所有已分配的 IP 地址 |

```bash
kubectl get crd | grep -E 'network.alibabacloud|terway'
```

```
ipinstances.network.alibabacloud.com           2025-01-15T08:30:00Z
nodenetworkings.network.alibabacloud.com       2025-01-15T08:30:00Z
podenis.network.alibabacloud.com               2025-01-15T08:30:00Z
podnetworkings.network.alibabacloud.com        2025-01-15T08:30:00Z
reservedips.network.alibabacloud.com           2025-01-15T08:30:00Z
```

---

<!-- chunk: 3. PodENI CRUD 操作 -->## 3. PodENI CRUD 操作

#<!-- chunk: 3.1 OpenAPI v3 Schema -->## 3.1 OpenAPI v3 Schema

```yaml
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

**Phase 状态流转**: `Pending` -> `Binding` -> `Bound` / `Failed`

#<!-- chunk: 3.2 Create -->## 3.2 Create

##<!-- chunk: 通过 Pod Annotation 自动创建 -->## 通过 Pod Annotation 自动创建

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: app-with-eni
  namespace: production
  annotations:
    k8s.aliyun.com/eni: "true"
    k8s.aliyun.com/security-group: "sg-xxx,sg-yyy"
    k8s.aliyun.com/vswitch-ids: "vsw-xxx,vsw-yyy"
    k8s.aliyun.com/pod-ip-fixed: "true"
    k8s.aliyun.com/pod-ip-retain-hour: "48"
spec:
  containers:
    - name: app
      image: nginx:latest
      ports:
        - containerPort: 80
  resources:
    limits:
      aliyun/eni: "1"
    requests:
      aliyun/eni: "1"
```

##<!-- chunk: 手动创建 -->## 手动创建

```yaml
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
    ipType: "IPv4"
    ipv4: "172.16.1.100"
```

##<!-- chunk: StatefulSet 固定 IP 完整示例 -->## StatefulSet 固定 IP 完整示例

```yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql-stateful
  namespace: database
  annotations:
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
        k8s.aliyun.com/pod-ip-fixed: "true"
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

#<!-- chunk: 3.3 Read -->## 3.3 Read

```bash
kubectl get podenis -A

kubectl get podenis -n production

kubectl get podeni <podeni-name> -n <namespace> -o yaml

kubectl get podenis -A -o custom-columns=\
NAMESPACE:.metadata.namespace,\
NAME:.metadata.name,\
POD:.spec.pod.name,\
PHASE:.status.phase,\
ENI_IP:.status.eniIP

kubectl get podenis -l app=mysql -A

kubectl get podenis -A --field-selector status.phase=Bound

kubectl get podeni -n production \
  -o jsonpath='{.items[?(@.spec.pod.name=="app-with-eni")].metadata.name}'

kubectl get podenis -A -o yaml > podenis-backup.yaml

kubectl describe podeni <podeni-name> -n <namespace>
```

##<!-- chunk: 状态监控脚本 -->## 状态监控脚本

```bash
cat << 'SCRIPT' > check-podeni-status.sh
#!/bin/bash
echo "=== PodENI Status Report ==="
echo "Timestamp: $(date)"
echo ""

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
SCRIPT
chmod +x check-podeni-status.sh
./check-podeni-status.sh
```

#<!-- chunk: 3.4 Update -->## 3.4 Update

##<!-- chunk: 更新安全组 -->## 更新安全组

```bash
kubectl edit podeni <podeni-name> -n <namespace>

kubectl patch podeni <podeni-name> -n <namespace> --type='json' -p='[
  {
    "op": "replace",
    "path": "/spec/eni/securityGroupIDs",
    "value": ["sg-new-xxxxx", "sg-new-yyyyy"]
  }
]'

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

##<!-- chunk: 更新 vSwitch -->## 更新 vSwitch

```bash
kubectl patch podeni <podeni-name> -n <namespace> --type='merge' -p='
{
  "spec": {
    "eni": {
      "vSwitchID": "vsw-new-xxxxx"
    }
  }
}'
```

##<!-- chunk: 通过 Annotation 更新 Pod 网络配置 -->## 通过 Annotation 更新 Pod 网络配置

```bash
kubectl annotate pod <pod-name> -n <namespace> \
  k8s.aliyun.com/security-group="sg-xxxxx" \
  --overwrite

kubectl annotate pod <pod-name> -n <namespace> \
  k8s.aliyun.com/pod-ip-fixed- \
  --overwrite
```

#<!-- chunk: 3.5 Delete -->## 3.5 Delete

```bash
kubectl delete podeni <podeni-name> -n <namespace>

kubectl delete podenis -n <namespace> --all

kubectl delete podeni <podeni-name> -n <namespace> \
  --force --grace-period=0

kubectl delete podenis -l app=deprecated-app -A

kubectl get podenis -A -o json | \
  jq -r '.items[] | select(.status.phase=="Failed") |
  "-n \(.metadata.namespace) \(.metadata.name)"' | \
  xargs -r kubectl delete podeni
```

##<!-- chunk: Finalizer 清理 -->## Finalizer 清理

```bash
kubectl patch podeni <podeni-name> -n <namespace> \
  -p '{"metadata":{"finalizers":[]}}' --type=merge

for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}'); do
  for podeni in $(kubectl get podenis -n $ns --field-selector=metadata.deletionTimestamp!=nil -o jsonpath='{.items[*].metadata.name}'); do
    echo "Cleaning finalizer for $podeni in $ns"
    kubectl patch podeni $podeni -n $ns -p '{"metadata":{"finalizers":[]}}' --type=merge
  done
done
```

---

<!-- chunk: 4. NodeNetworking CRUD 操作 -->## 4. NodeNetworking CRUD 操作

#<!-- chunk: 4.1 Schema -->## 4.1 Schema

```yaml
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

#<!-- chunk: 4.2 Create -->## 4.2 Create

```yaml
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
    maxENI: 8
    minENI: 2
    vSwitchIDs:
      - vsw-xxxxx-zone-h
      - vsw-yyyyy-zone-h
    securityGroupIDs:
      - sg-xxxxxxxxxx
  ipam:
    maxIPPerENI: 30
    poolSize: 25
    policy: "ordered"
```

#<!-- chunk: 4.3 Read -->## 4.3 Read

```bash
kubectl get nodenetworkings

kubectl get nodenetworking <node-name> -o yaml

kubectl get nodenetworkings -o wide

kubectl get nodenetworkings -o custom-columns=\
NAME:.metadata.name,\
NODE:.spec.nodeName,\
ENI_COUNT:.status.eniCount,\
AVAILABLE_IP:.status.availableIPs,\
ALLOCATED_IP:.status.allocatedIPs
```

##<!-- chunk: 节点 IP 利用率监控脚本 -->## 节点 IP 利用率监控脚本

```bash
cat << 'SCRIPT' > check-node-networking.sh
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
SCRIPT
chmod +x check-node-networking.sh
./check-node-networking.sh
```

#<!-- chunk: 4.4 Update -->## 4.4 Update

```bash
kubectl patch nodenetworking <node-name> --type='merge' -p='
{
  "spec": {
    "eniOptions": {
      "maxENI": 10,
      "minENI": 3
    }
  }
}'

kubectl patch nodenetworking <node-name> --type='json' -p='[
  {
    "op": "add",
    "path": "/spec/eniOptions/vSwitchIDs/-",
    "value": "vsw-new-xxxxx"
  }
]'

kubectl patch nodenetworking <node-name> --type='json' -p='[
  {
    "op": "replace",
    "path": "/spec/eniOptions/securityGroupIDs",
    "value": ["sg-new-xxx", "sg-new-yyy"]
  }
]'

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

#<!-- chunk: 4.5 Delete -->## 4.5 Delete

```bash
kubectl delete nodenetworking <node-name>

kubectl delete nodenetworkings --all
```

---

<!-- chunk: 5. PodNetworking CRUD 操作 -->## 5. PodNetworking CRUD 操作

#<!-- chunk: 5.1 Schema -->## 5.1 Schema

```yaml
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

#<!-- chunk: 5.2 Create -->## 5.2 Create

##<!-- chunk: 生产 API 网络策略 -->## 生产 API 网络策略

```yaml
apiVersion: network.alibabacloud.com/v1beta1
kind: PodNetworking
metadata:
  name: production-api-networking
spec:
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
  vSwitchOptions:
    vSwitchIDs:
      - vsw-api-zone-h
      - vsw-api-zone-i
    selectionPolicy: "ordered"
  securityGroupOptions:
    securityGroupIDs:
      - sg-api-production
      - sg-common-base
  ipOptions:
    allocationType: "ENIIP"
    ipType: "IPv4"
```

##<!-- chunk: 数据库隔离网络策略 -->## 数据库隔离网络策略

```yaml
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

#<!-- chunk: 5.3 Read -->## 5.3 Read

```bash
kubectl get podnetworkings

kubectl get podnetworking <name> -o yaml

kubectl get pods -l app=api-server,tier=backend

kubectl get pod <pod-name> -o jsonpath='{.metadata.annotations.k8s\.aliyun\.com/allocated-podnetworking}'
```

#<!-- chunk: 5.4 Update -->## 5.4 Update

```bash
kubectl patch podnetworking <name> --type='json' -p='[
  {
    "op": "add",
    "path": "/spec/vSwitchOptions/vSwitchIDs/-",
    "value": "vsw-new-xxxxx"
  }
]'

kubectl patch podnetworking <name> --type='merge' -p='
{
  "spec": {
    "securityGroupOptions": {
      "securityGroupIDs": ["sg-updated-xxx"]
    }
  }
}'

kubectl edit podnetworking <name>
```

#<!-- chunk: 5.5 Delete -->## 5.5 Delete

```bash
kubectl delete podnetworking <name>

kubectl delete podnetworking <name1> <name2>
```

---

<!-- chunk: 6. ReservedIP CRUD 操作 -->## 6. ReservedIP CRUD 操作

#<!-- chunk: 6.1 Schema -->## 6.1 Schema

```yaml
apiVersion: network.alibabacloud.com/v1beta1
kind: ReservedIP
metadata:
  name: mysql-fixed-ip-0
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
```

| 字段 | 可选值 | 说明 |
|:---|:---|:---|
| `spec.ip.ipType` | IPv4, IPv6, DualStack | IP 类型 |
| `spec.retention.duration` | 如 `72h`, `168h` | Pod 删除后 IP 保留时长 |
| `spec.reclaimPolicy` | Retain, Delete | 回收策略 |

#<!-- chunk: 6.2 Create -->## 6.2 Create

##<!-- chunk: 单个 YAML -->## 单个 YAML

```yaml
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
```

##<!-- chunk: 批量创建 (Job) -->## 批量创建 (Job)

```yaml
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

#<!-- chunk: 6.3 Read -->## 6.3 Read

```bash
kubectl get reservedips -A

kubectl get reservedip <name> -n <namespace> -o yaml

kubectl get reservedips -l app=mysql -A
```

##<!-- chunk: 孤儿检测与过期监控 -->## 孤儿检测与过期监控

```bash
cat << 'SCRIPT' > check-reserved-ips.sh
#!/bin/bash
echo "=== ReservedIP Status Report ==="
echo "Timestamp: $(date)"
echo ""

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
SCRIPT
chmod +x check-reserved-ips.sh
./check-reserved-ips.sh
```

#<!-- chunk: 6.4 Update -->## 6.4 Update

```bash
kubectl patch reservedip <name> -n <namespace> --type='merge' -p='
{
  "spec": {
    "retention": {
      "duration": "168h"
    }
  }
}'

kubectl patch reservedip <name> -n <namespace> --type='merge' -p='
{
  "spec": {
    "association": {
      "podName": "mysql-0-new"
    }
  }
}'
```

#<!-- chunk: 6.5 Delete -->## 6.5 Delete

```bash
kubectl delete reservedip <name> -n <namespace>

kubectl get reservedips -A -o json | \
  jq -r '.items[] | select(.status.expired == true) |
  "-n \(.metadata.namespace) \(.metadata.name)"' | \
  xargs -r kubectl delete reservedip

kubectl delete reservedip <name> -n <namespace> --force
```

---

<!-- chunk: 7. IPInstance CRUD 操作 -->## 7. IPInstance CRUD 操作

#<!-- chunk: 7.1 Schema -->## 7.1 Schema

```yaml
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

#<!-- chunk: 7.2 Read -->## 7.2 Read

```bash
kubectl get ipinstances -A

kubectl get ipinstance <name> -o yaml

kubectl get ipinstances -A --field-selector spec.nodeName=<node-name>
```

##<!-- chunk: IP 分布分析脚本 -->## IP 分布分析脚本

```bash
cat << 'SCRIPT' > analyze-ip-usage.sh
#!/bin/bash
echo "=== IP Instance Usage Analysis ==="
echo "Timestamp: $(date)"
echo ""

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
SCRIPT
chmod +x analyze-ip-usage.sh
./analyze-ip-usage.sh
```

---

<!-- chunk: 8. ConfigMap 配置管理 -->## 8. ConfigMap 配置管理

#<!-- chunk: 8.1 eni-config 完整示例 -->## 8.1 eni-config 完整示例

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "max_pool_size": 25,
      "min_pool_size": 10,
      "credential_path": "/var/addon/token-config",
      "vswitches": {
        "cn-hangzhou-h": ["vsw-xxx1", "vsw-xxx2"],
        "cn-hangzhou-i": ["vsw-yyy1", "vsw-yyy2"]
      },
      "security_groups": ["sg-xxxxxxxxxx"],
      "service_cidr": "172.21.0.0/20",
      "vswitch_selection_policy": "ordered",
      "eni_binding_mode": "ENIIP",
      "ip_type": "IPv4"
    }
```

#<!-- chunk: 8.2 terway-config 完整示例 -->## 8.2 terway-config 完整示例

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: terway-config
  namespace: kube-system
data:
  terway_config: |
    {
      "network_policy": "cilium",
      "enable_ebpf": true,
      "kube_proxy_replacement": "strict",
      "enable_bandwidth_manager": true,
      "enable_bpf_masquerade": true
    }
```

#<!-- chunk: 8.3 ConfigMap CRUD -->## 8.3 ConfigMap CRUD

```bash
kubectl get cm eni-config -n kube-system -o yaml

kubectl get cm terway-config -n kube-system -o yaml

kubectl get cm eni-config -n kube-system -o jsonpath='{.data.eni_conf}' | jq .
```

##<!-- chunk: 用 jq 添加 vSwitch -->## 用 jq 添加 vSwitch

```bash
kubectl get cm eni-config -n kube-system -o json | \
  jq '.data.eni_conf = (.data.eni_conf | fromjson |
  .vswitches["cn-hangzhou-h"] += ["vsw-new-xxx"] | tojson)' | \
  kubectl apply -f -
```

##<!-- chunk: 用 jq 更新 IP 池大小 -->## 用 jq 更新 IP 池大小

```bash
kubectl patch cm eni-config -n kube-system --type='json' -p='[
  {
    "op": "replace",
    "path": "/data/eni_conf",
    "value": "{\"version\":\"1\",\"max_pool_size\":30,\"min_pool_size\":15}"
  }
]'
```

#<!-- chunk: 8.4 配置重载流程 -->## 8.4 配置重载流程

```bash
kubectl edit cm eni-config -n kube-system

kubectl delete pods -n kube-system -l app=terway-eniip

kubectl logs -n kube-system -l app=terway-eniip -c terway --tail=50 | grep -i config
```

---

<!-- chunk: 9. 综合诊断脚本 -->## 9. 综合诊断脚本

```bash
cat << 'SCRIPT' > terway-diagnose.sh
#!/bin/bash

echo "=========================================="
echo "  Terway Network Diagnostics Tool"
echo "  Timestamp: $(date)"
echo "=========================================="
echo ""

echo "=== [1] Terway Component Status ==="
kubectl get pods -n kube-system -l app=terway-eniip -o wide
echo ""

echo "=== [2] CRD Resource Statistics ==="
echo "PodENIs: $(kubectl get podenis -A --no-headers | wc -l)"
echo "NodeNetworkings: $(kubectl get nodenetworkings --no-headers | wc -l)"
echo "PodNetworkings: $(kubectl get podnetworkings --no-headers | wc -l)"
echo "ReservedIPs: $(kubectl get reservedips -A --no-headers | wc -l)"
echo "IPInstances: $(kubectl get ipinstances -A --no-headers | wc -l)"
echo ""

echo "=== [3] Abnormal Status Resources ==="
echo "Pending PodENIs:"
kubectl get podenis -A --field-selector status.phase=Pending 2>/dev/null || echo "None"
echo ""
echo "Failed PodENIs:"
kubectl get podenis -A --field-selector status.phase=Failed 2>/dev/null || echo "None"
echo ""

echo "=== [4] Node IP Capacity ==="
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
  allocated=$(kubectl get pods -A --field-selector spec.nodeName=$node -o json | \
    jq -r '.items | length')
  max_pod=$(kubectl get node $node -o jsonpath='{.status.capacity.pods}')
  echo "$node: $allocated / $max_pod pods"
done
echo ""

echo "=== [5] Recent Terway Errors (last 10) ==="
kubectl logs -n kube-system -l app=terway-eniip -c terway --tail=500 2>/dev/null | \
  grep -i "error\|warn\|fail" | tail -10
echo ""

echo "=== [6] NetworkPolicy Status ==="
echo "NetworkPolicy mode: $(kubectl get cm terway-config -n kube-system -o jsonpath='{.data.terway_config}' 2>/dev/null | jq -r '.network_policy // "iptables"' 2>/dev/null || echo "iptables")"
echo ""

echo "=========================================="
echo "  Diagnostics Complete"
echo "=========================================="
SCRIPT
chmod +x terway-diagnose.sh
./terway-diagnose.sh
```

**诊断步骤说明**:

| 步骤 | 检查项 | 异常判断标准 |
|:---|:---|:---|
| 1 | Terway Pod 状态 | 非 Running 或 Restart > 3 |
| 2 | CRD 资源统计 | ReservedIP/IPInstance 数量异常增长 |
| 3 | 异常状态资源 | 存在 Pending 或 Failed 状态 |
| 4 | 节点 IP 容量 | 已分配接近 max_pod (>80%) |
| 5 | Terway 错误日志 | 存在 error/warn 关键字 |
| 6 | NetworkPolicy 模式 | 生产环境应使用 cilium |

---

<!-- chunk: 10. 命令速查表 -->## 10. 命令速查表

#<!-- chunk: PodENI -->## PodENI

```bash
kubectl get podenis -A                           # 列出所有
kubectl get podeni <name> -n <ns> -o yaml        # 查看详情
kubectl delete podeni <name> -n <ns>             # 删除单个
kubectl delete podenis -l app=deprecated-app -A   # 按 Label 删除
kubectl patch podeni <name> -n <ns> -p '...'     # 更新
kubectl patch podeni <name> -n <ns> -p '{"metadata":{"finalizers":[]}}' --type=merge  # 清理 Finalizer
```

#<!-- chunk: NodeNetworking -->## NodeNetworking

```bash
kubectl get nodenetworkings                      # 列出所有
kubectl get nodenetworking <name> -o yaml        # 查看详情
kubectl patch nodenetworking <name> -p '...'     # 更新 ENI/IPAM 配置
kubectl delete nodenetworking <name>             # 删除
```

#<!-- chunk: PodNetworking -->## PodNetworking

```bash
kubectl get podnetworkings                       # 列出所有
kubectl get podnetworking <name> -o yaml         # 查看详情
kubectl patch podnetworking <name> -p '...'      # 更新 vSwitch/安全组
kubectl delete podnetworking <name>              # 删除
```

#<!-- chunk: ReservedIP -->## ReservedIP

```bash
kubectl get reservedips -A                       # 列出所有固定 IP
kubectl apply -f reservedip.yaml                 # 创建
kubectl patch reservedip <name> -n <ns> -p '...' # 更新保留时长/关联
kubectl delete reservedip <name> -n <ns>         # 删除
kubectl delete reservedip <name> -n <ns> --force # 强制删除
```

#<!-- chunk: IPInstance -->## IPInstance

```bash
kubectl get ipinstances -A                       # 列出所有 IP 实例
kubectl get ipinstances -A --field-selector spec.nodeName=<node>  # 按节点筛选
```

#<!-- chunk: ConfigMap -->## ConfigMap

```bash
kubectl get cm eni-config -n kube-system -o yaml # 查看 ENI 配置
kubectl get cm terway-config -n kube-system -o yaml # 查看 Terway 配置
kubectl edit cm eni-config -n kube-system        # 编辑配置
kubectl delete pods -n kube-system -l app=terway-eniip  # 重载配置
```

#<!-- chunk: 故障排查 -->## 故障排查

```bash
kubectl logs -n kube-system -l app=terway-eniip -c terway --tail=200  # 查看日志
kubectl describe podeni <name> -n <ns>           # 查看 PodENI 事件
```

#<!-- chunk: Annotation 速查 -->## Annotation 速查

| Annotation | 值 | 说明 |
|:---|:---|:---|
| `k8s.aliyun.com/eni` | "true" | 启用独占 ENI 模式 |
| `k8s.aliyun.com/pod-ip-fixed` | "true" | 启用固定 IP |
| `k8s.aliyun.com/pod-ip-retain-hour` | "48" | IP 保留时间 (小时) |
| `k8s.aliyun.com/security-group` | "sg-xxx" | 指定安全组 |
| `k8s.aliyun.com/vswitch-ids` | "vsw-xxx,vsw-yyy" | 指定 vSwitch |

---

<!-- chunk: 11. 交叉引用 -->## 11. 交叉引用

| 文档 | 说明 |
|:---|:---|
| [03-usage.md](./03-usage.md) | Terway 使用指南, 包含基础操作与场景示例 |
| [02-architecture.md](./02-architecture.md) | Terway 架构设计, CNI 插件原理与组件交互 |
| [04-operations.md](./04-operations.md) | Terway 运维手册, 日常运维与故障处理 |
| [domain-03-networking-traffic/37](../domain-03-networking-traffic/37-terway-resources-crud-operations.md) | 原始 CRUD 操作指南 (本文档提取源) |

---

**容量规划参考**:

| 集群规模 | 节点数量 | 建议 vSwitch CIDR | 说明 |
|:---|:---|:---|:---|
| 小型 | <50 | /20 (4096 IP) | 每个 AZ 一个 vSwitch |
| 中型 | 50-200 | /19 (8192 IP) | 每个 AZ 2 个 vSwitch |
| 大型 | 200-500 | /18 (16384 IP) | 每个 AZ 多个 vSwitch |
| 超大型 | >500 | /16 (65536 IP) | 分层 vSwitch 设计 |

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[README.md|Domain 5 Networking MOC]]
- [[README.md|Topic: Terway 专题 — 阿里云容器网络 (CNI) 全栈知识体系]]
- [[40-terway-product-overview.md|01 - Terway 产品概览 (Product Overview)]]
- [[41-terway-architecture-deep-dive.md|02 - Terway 架构原理 (Architecture Deep Dive)]]
- [[42-terway-usage-guide.md|03 - Terway 使用指南 (Usage Guide)]]
- [[44-terway-operations-manual.md|04 - Terway 运维手册 (Operations Manual)]]
- [[45-terway-testing-validation.md|05 - Terway 测试验证 (Testing & Validation)]]
- [[46-terway-performance-tuning.md|06 - Terway 性能调优 (Performance Tuning)]]
- [[47-terway-troubleshooting-fta.md|07 - Terway 故障树速查 (FTA Troubleshooting Quick Reference)]]
- [[../domain-19-landscape-references/topic-index/terway-index.md|Terway 全项目资源索引]]

## See Also

- [[domain-03-networking-traffic/41-terway-architecture-deep-dive.md|41-terway-architecture-deep-dive]]
- [[domain-03-networking-traffic/42-terway-usage-guide.md|42-terway-usage-guide]]
- [[domain-03-networking-traffic/44-terway-operations-manual.md|44-terway-operations-manual]]
- [[domain-03-networking-traffic/45-terway-testing-validation.md|45-terway-testing-validation]]
