---
title: GKE 故障排查手册
description: 'GKE Node Auto-Repair、升级异常、Control Plane 不可达、GKE Connect Gateway 及日志分析'
summary: 'GKE Node Auto-Repair、升级异常、Control Plane 不可达、GKE Connect Gateway 及日志分析'
category: cloud-providers
tags:
- cloud
- k8s
- gcp
- gke
- troubleshooting
- debugging
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 支持工程师
estimated_read_time: 15min
intent_queries:
- GKE 故障排查 是什么
- 如何排查 GKE 常见问题
trigger_keywords:
- gke-troubleshooting
- node-auto-repair
- upgrade-issues
- control-plane
- connect-gateway
prerequisites:
- kubectl-basics
- cloud-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# GKE 故障排查手册

## 1. Node Auto-Repair 问题

### 1.1 Auto-Repair 触发条件

GKE 自动修复节点当满足以下条件之一：
- 节点 NotReady 超过一定时间（默认约 10 分钟）
- 节点运行但 kubelet 未响应
- 节点磁盘压力严重
- 节点运行时异常

### 1.2 排查 Auto-Repair 卡住

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点池状态
gcloud container node-pools describe default-pool \
  --cluster=prod-cluster \
  --region=asia-southeast1 \
  --format="table(status, management)"

# 查看操作状态
gcloud container operations list \
  --filter="operationType=REPAIR_NODES AND status=RUNNING" \
  --format="table(name, operationType, status, statusMessage)"

# 查看节点状态
kubectl get nodes -o wide
kubectl describe node <node-name> | grep -A 20 "Conditions:"
```
### 1.3 Auto-Repair 无法完成

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
# 常见原因:
# 1. 实例组中没有足够的健康节点
# 2. PDB 阻止节点驱逐
kubectl get pdb -A

# 3. 节点上有 DaemonSet Pod 无法驱逐
kubectl get pods --field-selector spec.nodeName=<node-name> -A

# 手动修复
# 取消自动修复
gcloud container node-pools update default-pool \
  --cluster=prod-cluster \
  --region=asia-southeast1 \
  --no-enable-autorepair

# 手动删除问题节点（触发重新创建）
kubectl delete node <node-name>

# 重新启用自动修复
gcloud container node-pools update default-pool \
  --cluster=prod-cluster \
  --region=asia-southeast1 \
  --enable-autorepair
```
### 1.4 节点自动升级问题

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
# 查看节点版本
kubectl get nodes -o custom-columns=NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion

# 查看升级状态
gcloud container operations list \
  --filter="operationType=UPGRADE_NODES AND status=RUNNING" \
  --format="table(name, operationType, progress)"

# 升级卡住时
# 检查 PDB 配置
kubectl get pdb -A -o wide

# 临时删除 PDB（谨慎）
kubectl delete pdb <pdb-name> -n <namespace>

# 手动 drain 节点
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force
```
## 2. 控制平面不可达

### 2.1 诊断控制平面

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查控制平面状态
gcloud container clusters describe prod-cluster \
  --region=asia-southeast1 \
  --format="table(status, currentMasterVersion, endpoint)"

# 检查 API Server 连通性
kubectl cluster-info

# 测试 API Server
curl -k https://<endpoint>/healthz
curl -k https://<endpoint>/version
```
### 2.2 Private Cluster 连接问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Master Authorized Networks
gcloud container clusters describe prod-cluster \
  --region=asia-southeast1 \
  --format="json(masterAuthorizedNetworksConfig)"

# 检查是否从正确网络访问
# 方式一: 通过 VPN
# 方式二: 通过 IAP 隧道
gcloud compute ssh bastion \
  --zone=asia-southeast1-a \
  -- -L 8443:<master-ip>:443

# 方式三: 通过 Cloud NAT 出站
gcloud compute routers get-nat-mapping-info cloud-router \
  --region=asia-southeast1
```
### 2.3 证书和认证问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取新凭证
gcloud container clusters get-credentials prod-cluster \
  --region=asia-southeast1

# 检查 kubeconfig
kubectl config view

# 验证 Token
kubectl auth whoami

# 常见错误:
# "Unable to connect to the server: x509: certificate signed by unknown authority"
# 解决: 重新获取凭证

# "Unauthorized"
# 解决: 检查 IAM 权限
gcloud projects get-iam-policy my-project \
  --filter="bindings.members:user:your-email@domain.com"
```
## 3. GKE 升级问题

### 3.1 控制平面升级失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看升级操作
gcloud container operations list \
  --filter="operationType=UPGRADE_MASTER AND status!=DONE" \
  --format="table(name, operationType, status, statusMessage)"

# 检查可用版本
gcloud container get-server-config \
  --region=asia-southeast1 \
  --format="yaml(channels)"

# 强制升级（谨慎使用）
gcloud container clusters upgrade prod-cluster \
  --region=asia-southeast1 \
  --master \
  --cluster-version=1.31
```
### 3.2 节点池升级问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点池升级状态
gcloud container operations list \
  --filter="operationType=UPGRADE_NODES" \
  --format="table(name, operationType, status, progress)"

# 手动触发节点池升级
gcloud container clusters upgrade prod-cluster \
  --region=asia-southeast1 \
  --node-pool=default-pool

# 如果升级卡住
# 检查是否有节点无法 drain
kubectl get nodes -o wide
kubectl describe node <stuck-node> | grep -A 10 "Events:"

# 强制取消操作
gcloud container operations cancel <operation-id> \
  --region=asia-southeast1
```
### 3.3 版本兼容性问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 API 弃用警告
kubectl api-versions | grep -i deprecated

# 使用 kubectl 插件检查
kubectl deprecations --cluster-version=1.31

# 检查不兼容资源
kubectl get all -A -o json | jq '.items[] | select(.apiVersion | contains("extensions/v1beta1"))'
```
## 4. Pod 调度问题

### 4.1 Pending Pod 诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Pod 事件
kubectl describe pod <pod-name> -n <namespace>

# 常见原因:
# 1. 资源不足
kubectl describe nodes | grep -A 5 "Allocated resources"
kubectl top nodes

# 2. 节点选择器/亲和性不匹配
kubectl get pod <pod-name> -o jsonpath='{.spec.nodeSelector}'
kubectl get nodes --show-labels

# 3. 污点不容忍
kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints
```
### 4.2 GPU Pod 调度

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 GPU 节点池
gcloud container node-pools describe gpu-pool \
  --cluster=prod-cluster \
  --region=asia-southeast1

# 验证 GPU 可用性
kubectl get nodes -o json | jq '.items[] | select(.status.allocatable["nvidia.com/gpu"]) | .metadata.name'

# 检查 GPU 驱动
kubectl get pods -n kube-system | grep nvidia
```
```yaml
# GPU Pod 配置
apiVersion: v1
kind: Pod
metadata:
  name: gpu-training
  namespace: ml-training
spec:
  nodeSelector:
    cloud.google.com/gke-accelerator: nvidia-tesla-t4
  tolerations:
    - key: nvidia.com/gpu
      operator: Exists
      effect: NoSchedule
  containers:
    - name: trainer
      image: gcr.io/my-project/trainer:latest
      resources:
        limits:
          nvidia.com/gpu: 1
          cpu: "4"
          memory: "16Gi"
```

## 5. 网络问题排查

### 5.1 Service 连通性

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Endpoints
kubectl get endpoints <service-name> -n <namespace>

# 测试 ClusterIP 连通性
kubectl run test --image=busybox --rm -it -- wget -qO- http://<service-name>:<port>/healthz

# 检查 DNS 解析
kubectl run dnstest --image=busybox --rm -it -- nslookup <service-name>.<namespace>.svc.cluster.local

# 检查 Network Policy
kubectl get networkpolicy -A -o wide
```
### 5.2 Ingress 问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Ingress 状态
kubectl describe ingress <ingress-name> -n <namespace>

# 检查 Backend Service 健康
gcloud compute backend-services list --format="table(name, backends)"
gcloud compute backend-services get-health <backend-service-name> --region=asia-southeast1

# 检查负载均衡器
gcloud compute forwarding-rules list --format="table(name, IPAddress, target)"
```
### 5.3 跨命名空间通信

```yaml
# 允许跨命名空间访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-cross-ns
  namespace: backend
spec:
  podSelector:
    matchLabels:
      app: api
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              name: frontend
      ports:
        - port: 8080
```

## 6. 存储问题排查

### 6.1 PVC Pending

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 PVC 状态
kubectl describe pvc <pvc-name> -n <namespace>

# 常见原因:
# 1. StorageClass 不存在
kubectl get storageclass

# 2. 区域不匹配
# 检查 PV 可用区
gcloud compute disks list --filter="name:<pv-name>"

# 3. 配额不足
gcloud compute project-info describe --format="table(quotas)"
```
### 6.2 挂载失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查挂载错误
kubectl describe pod <pod-name> | grep -A 10 "Events:"
kubectl logs <pod-name> | grep -i mount

# 检查 CSI Driver 状态
kubectl get pods -n kube-system -l app=gce-pd-csi-driver

# 手动分离磁盘（紧急情况）
gcloud compute instances detach-disk <instance-name> --disk=<disk-name> --zone=<zone>
```
## 7. 日志分析

### 7.1 Cloud Logging 查询

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看集群事件
gcloud logging read \
  'resource.type="k8s_cluster" AND resource.labels.cluster_name="prod-cluster"' \
  --limit=50 \
  --format="table(timestamp, jsonPayload.message)"

# 查找 OOMKilled 容器
gcloud logging read \
  'resource.type="k8s_container" AND jsonPayload.reason="OOMKilled"' \
  --limit=20

# 查找 CrashLoopBackOff
gcloud logging read \
  'resource.type="k8s_container" AND textPayload=~"CrashLoopBackOff"' \
  --limit=20

# 查找调度失败
gcloud logging read \
  'resource.type="k8s_cluster" AND jsonPayload.message=~"FailedScheduling"' \
  --limit=20
```
### 7.2 Cloud Monitoring 查询

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点 CPU 使用率
gcloud monitoring time-series list \
  --filter='metric.type="kubernetes.io/node/cpu/allocatable_utilization"' \
  --interval="PT1H" \
  --format="table(points[].value.doubleValue, resource.labels.node_name)"

# 查看 Pod 内存使用
gcloud monitoring time-series list \
  --filter='metric.type="kubernetes.io/container/memory/used_bytes"' \
  --interval="PT1H"
```
## 8. 常见错误码

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| `ImagePullBackOff` | 镜像拉取失败 | 检查镜像名、认证、网络 |
| `CrashLoopBackOff` | 容器反复崩溃 | 查看容器日志、资源限制 |
| `OOMKilled` | 内存超限 | 增加 memory limit 或优化应用 |
| `Pending` | 无法调度 | 检查资源、选择器、污点 |
| `Evicted` | 节点压力驱逐 | 清理节点或增加节点 |
| `FailedMount` | 存储挂载失败 | 检查 PVC 状态、CSI Driver |
| `NetworkPolicy` | 策略阻止流量 | 检查 NetworkPolicy 规则 |

## 9. 快速诊断脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# gke-diagnostic.sh

CLUSTER=$1
REGION=$2

echo "=== Cluster Status ==="
gcloud container clusters describe $CLUSTER --region=$REGION \
  --format="table(status, currentMasterVersion, currentNodeVersion)"

echo "=== Node Status ==="
kubectl get nodes -o wide

echo "=== System Pods ==="
kubectl get pods -n kube-system

echo "=== Problem Pods ==="
kubectl get pods -A --field-selector='status.phase!=Running,status.phase!=Succeeded'

echo "=== Recent Events ==="
kubectl get events --sort-by='.lastTimestamp' | tail -20

echo "=== Resource Usage ==="
kubectl top nodes 2>/dev/null || echo "Metrics API not available"

echo "=== PVC Status ==="
kubectl get pvc -A

echo "=== Network Policies ==="
kubectl get networkpolicy -A
```
## Related

- [[02-gke-autopilot-serverless]]
- [[03-gke-networking-dataplane-v2]]

## See Also

- [[domain-12-cloud-providers/Google-GKE/99-gke-production-runbook.md|GKE 生产环境运行手册]]
- Kubernetes Troubleshooting


<!-- risk-assessed -->
