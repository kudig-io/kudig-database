---
title: StatefulSet 故障排查
description: '# 21 - StatefulSet 故障排查 (StatefulSet Troubleshooting)'
summary: 'kubectl get statefulsets --all-namespaces'
category: troubleshooting
tags:
- statefulset
- pvc
- ordinal
- network-identity
- stable-hostname
- controller-manager
- prometheus
- coredns
- mysql
- networkpolicy
tier: core
created: '2026-05-23'
last_updated: '2026-07-02'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- StatefulSet 启动顺序不对
- PVC 无法挂载
- Pod DNS 不稳定
trigger_keywords:
- StatefulSet
- 故障排查
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- cni-basics
- etcd-basics
- mysql-basics
k8s_versions:
- 1.25
- 1.26
- 1.27
- 1.28
- 1.29
- 1.3
- 1.31
- 1.32
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/FTA故障树/list/statefulset-fta.md
  label: '故障树: statefulset'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 21 - [[statefulset|StatefulSet]] 故障排查 (StatefulSet Troubleshooting)

---

<!-- chunk: 1. StatefulSet 故障诊断总览 (StatefulSet Diagnosis Overview) -->
## 1. StatefulSet 故障诊断总览 (StatefulSet Diagnosis Overview)

### 1.1 常见问题现象分类

| 问题类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **Pod启动顺序异常** | Pod未按序号启动 | 数据一致性风险 | P0 - 紧急 |
| **PVC绑定失败** | 存储卷无法挂载 | 数据持久化失败 | P0 - 紧急 |
| **网络标识异常** | Pod DNS解析失败 | 服务发现失效 | P1 - 高 |
| **更新策略问题** | 滚动更新卡住 | 服务版本不一致 | P1 - 高 |
| **脑裂问题** | 多个主节点选举 | 数据冲突风险 | P0 - 紧急 |

### 1.2 StatefulSet 架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    StatefulSet 故障诊断架构                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      有序Pod管理层                                    │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │  Pod-0      │    │  Pod-1      │    │  Pod-2      │              │  │
│  │  │ (mysql-0)   │    │ (mysql-1)   │    │ (mysql-2)   │              │  │
│  │  │ State: Ready│    │ State: Ready│    │ State: Ready│              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   PVC绑定    │   │   网络标识    │   │   启动顺序    │                   │
│  │ (Persistent  │   │ (Headless   │   │ (Ordinal    │                   │
│  │ VolumeClaim)│   │ Service)    │   │ Index)      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    StatefulSet控制器                                  │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                   kube-controller-manager                     │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │ StatefulSet │  │   Volume    │  │   Network   │           │  │  │
│  │  │  │ Controller  │  │ Controller  │  │ Controller  │           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   存储系统   │   │   DNS服务    │   │   更新策略   │                   │
│  │ (CSI/Cinder)│   │ (CoreDNS)   │   │ (Rolling    │                   │
│  │   提供PV    │   │   解析      │   │ Update)     │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 2. StatefulSet 基础状态检查 (Basic Status Check) -->
## 2. StatefulSet 基础状态检查 (Basic Status Check)

### 2.1 StatefulSet 资源状态验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 基础信息检查 ==========
# 查看所有StatefulSet
kubectl get statefulsets --all-namespaces

# 查看特定StatefulSet详细信息
kubectl describe statefulset <statefulset-name> -n <namespace>

# 检查StatefulSet配置
kubectl get statefulset <statefulset-name> -n <namespace> -o yaml

# ========== 2. Pod有序性检查 ==========
# 查看Pod创建顺序
kubectl get pods -n <namespace> -l <selector> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.creationTimestamp
}{
        "\n"
}{
    end
}' | sort -k2

# 验证Pod命名规范
kubectl get pods -n <namespace> -l <selector> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\n"
}{
    end
}' | grep -E "^<statefulset-name>-[0-9]+$"

# ========== 3. PVC绑定状态检查 ==========
# 查看相关PVC
kubectl get pvc -n <namespace> -l <selector>

# 检查PVC绑定状态
kubectl get pvc -n <namespace> | grep <statefulset-name>

# 验证PV绑定关系
kubectl get pv | grep -E "$(kubectl get pvc -n <namespace> -o jsonpath='{.items[*].spec.volumeName}')"
```
### 2.2 网络和服务检查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== Headless Service检查 ==========
# 检查Headless Service是否存在
kubectl get service <service-name> -n <namespace>

# 验证Service类型
kubectl get service <service-name> -n <namespace> -o jsonpath='{.spec.clusterIP}'

# 检查Endpoints
kubectl get endpoints <service-name> -n <namespace>

# ========== DNS解析验证 ==========
# 测试Pod DNS解析
kubectl run dns-test --image=busybox -n <namespace> -it --rm -- sh
# 在容器内执行:
# nslookup <statefulset-name>-0.<service-name>.<namespace>.svc.cluster.local
# nslookup <statefulset-name>-1.<service-name>.<namespace>.svc.cluster.local

# 检查CoreDNS状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
```
---

<!-- chunk: 3. 存储相关问题排查 (Storage储问题排查|Storage Issues]] Troubleshooting) -->
## 3. 存储相关问题排查 (Storage Issues Troubleshooting)

### 3.1 PVC绑定失败问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. PVC状态检查 ==========
# 查看PVC详细状态
kubectl describe pvc -n <namespace> <pvc-name>

# 检查PVC事件
kubectl get events -n <namespace> --field-selector involvedObject.name=<pvc-name>

# 验证StorageClass
kubectl get storageclass
kubectl get pvc -n <namespace> -o jsonpath='{.items[*].spec.storageClassName}'

# ========== 2. PV供应问题 ==========
# 检查动态供应状态
kubectl get pods -n kube-system | grep csi

# 查看PV创建状态
kubectl get pv | grep <pvc-name>

# 检查存储后端状态
# AWS EBS
aws ec2 describe-volumes --filters Name=tag-key,Values=kubernetes.io/created-for/pvc/name

# GCP PD
gcloud compute disks list --filter="labels.kubernetes-io-created-for-pvc-name:*"

# ========== 3. 存储容量和访问模式 ==========
# 验证存储请求
kubectl get pvc -n <namespace> -o jsonpath='{
    .items[*].spec.resources.requests.storage
}'

# 检查访问模式
kubectl get pvc -n <namespace> -o jsonpath='{
    .items[*].spec.accessModes
}'

# 验证PV容量
kubectl get pv -o jsonpath='{
    .items[?(@.spec.claimRef.name=="<pvc-name>")].spec.capacity.storage
}'
```
### 3.2 数据持久化验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 数据一致性检查 ==========
# 在Pod中写入测试数据
kubectl exec -n <namespace> <pod-name> -- sh -c "echo 'test-data-$(date)' > /data/test-file"

# 验证数据持久性
kubectl delete pod -n <namespace> <pod-name>
# 等待Pod重建后检查数据
kubectl exec -n <namespace> <pod-name> -- cat /data/test-file

# ========== 跨Pod数据隔离验证 ==========
# 在不同Pod中写入不同数据
for i in {0..2}; do
    kubectl exec -n <namespace> <statefulset-name>-$i -- sh -c "echo 'data-from-pod-$i' > /data/pod-$i-file"
done

# 验证数据隔离
for i in {0..2}; do
    kubectl exec -n <namespace> <statefulset-name>-$i -- cat /data/pod-$i-file
done
```
---

<!-- chunk: 4. 网络标识问题排查 (Network Identity Issues) -->
## 4. 网络标识问题排查 (Network Identity Issues)

### 4.1 DNS解析问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 1. DNS配置检查 ==========
# 检查Pod DNS策略
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.dnsPolicy}'

# 查看DNS配置
kubectl exec -n <namespace> <pod-name> -- cat /etc/resolv.conf

# 验证搜索域
kubectl exec -n <namespace> <pod-name> -- cat /etc/resolv.conf | grep search

# ========== 2. 网络连通性测试 ==========
# Pod间网络测试
kubectl exec -n <namespace> <statefulset-name>-0 -- ping -c 3 <statefulset-name>-1.<service-name>.<namespace>.svc.cluster.local

# 端口连通性测试
kubectl exec -n <namespace> <statefulset-name>-0 -- nc -zv <statefulset-name>-1.<service-name>.<namespace>.svc.cluster.local <port>

# ========== 3. Service配置验证 ==========
# 检查Headless Service配置
kubectl get service <service-name> -n <namespace> -o yaml

# 验证selector匹配
kubectl get service <service-name> -n <namespace> -o jsonpath='{.spec.selector}'

# 检查Endpoints更新
kubectl get endpoints <service-name> -n <namespace> -w
```
### 4.2 网络策略影响

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 网络策略检查 ==========
# 查看相关NetworkPolicy
kubectl get networkpolicy -n <namespace>

# 检查策略影响
kubectl describe networkpolicy -n <namespace> <policy-name>

# 验证网络连通性
kubectl run network-test --image=busybox -n <namespace> -it --rm -- sh
# 测试到StatefulSet Pod的连接
```
---

<!-- chunk: 5. 更新和扩缩容问题 (Update and Scale Issues) -->
## 5. 更新和扩缩容问题 (Update and Scale Issues)

### 5.1 滚动更新卡住

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 更新状态检查 ==========
# 查看更新进度
kubectl get statefulset <statefulset-name> -n <namespace> -o jsonpath='{
    .status.updatedReplicas,
    .status.readyReplicas,
    .status.replicas
}'

# 检查更新策略
kubectl get statefulset <statefulset-name> -n <namespace> -o jsonpath='{.spec.updateStrategy}'

# 查看Pod更新状态
kubectl get pods -n <namespace> -l <selector> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.labels.controller-revision-hash
}{
        "\n"
}{
    end
}'

# ========== 2. 卡住原因分析 ==========
# 检查未就绪Pod
kubectl get pods -n <namespace> -l <selector> --field-selector=status.phase=Running -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.conditions[?(@.type=="Ready")].status
}{
        "\n"
}{
    end
}' | grep False

# 查看Pod事件
kubectl get events -n <namespace> --field-selector involvedObject.kind=Pod,reason=Unhealthy

# 检查存储挂载问题
kubectl describe pod -n <namespace> <pod-name> | grep -A10 "Volumes:"
```
### 5.2 扩缩容问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 扩容验证 ==========
# 执行扩容
kubectl scale statefulset <statefulset-name> -n <namespace> --replicas=5

# 监控扩容过程
kubectl get statefulset <statefulset-name> -n <namespace> -w

# 验证新Pod状态
kubectl get pods -n <namespace> -l <selector> --sort-by=.metadata.creationTimestamp

# ========== 缩容风险评估 ==========
# 检查数据分布
kubectl exec -n <namespace> <pod-name> -- sh -c "df -h /data"

# 验证主从关系
kubectl exec -n <namespace> <pod-name> -- sh -c "mysql -e 'SHOW MASTER STATUS;'"

# 确认数据同步状态
kubectl exec -n <namespace> <pod-name> -- sh -c "mysql -e 'SHOW SLAVE STATUS\G'"
```
---

<!-- chunk: 6. 监控和告警配置 (Monitoring and Alerting) -->
## 6. 监控和告警配置 (Monitoring and Alerting)

### 6.1 关键指标监控

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== StatefulSet状态监控 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: statefulset-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      k8s-app: kube-state-metrics
  endpoints:
  - port: http-metrics
    path: /metrics
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'kube_statefulset.*'
      action: keep
EOF

# ========== 告警规则配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: statefulset-alerts
  namespace: monitoring
spec:
  groups:
  - name: statefulset.rules
    rules:
    - alert: StatefulSetReplicasMismatch
      expr: kube_statefulset_status_replicas != kube_statefulset_status_replicas_available
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "StatefulSet replica count mismatch (namespace {{ \$labels.namespace }} statefulset {{ \$labels.statefulset }})"
        
    - alert: StatefulSetUpdateStuck
      expr: kube_statefulset_status_replicas_updated != kube_statefulset_replicas
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "StatefulSet update is stuck (namespace {{ \$labels.namespace }} statefulset {{ \$labels.statefulset }})"
        
    - alert: StatefulSetPodDegraded
      expr: kube_pod_status_ready{condition="false"} * on(pod) group_left(statefulset) kube_pod_owner{owner_kind="StatefulSet"} == 1
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "StatefulSet pod is degraded (namespace {{ \$labels.namespace }} statefulset {{ \$labels.statefulset }} pod {{ \$labels.pod }})"
        
    - alert: StatefulSetPVCUnbound
      expr: kube_persistentvolumeclaim_status_phase{phase!="Bound"} * on(persistentvolumeclaim) group_left(owner_name) kube_pod_spec_volumes_persistentvolumeclaims_info{owner_kind="StatefulSet"} == 1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "StatefulSet PVC is not bound (namespace {{ \$labels.namespace }} pvc {{ \$labels.persistentvolumeclaim }})"
EOF
```
### 6.2 健康检查脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== StatefulSet健康检查脚本 ==========
cat <<'EOF' > statefulset-health-check.sh
#!/bin/bash

NAMESPACE=${1:-default}
STATEFULSET_NAME=$2

if [ -z "$STATEFULSET_NAME" ]; then
    echo "Usage: $0 <namespace> <statefulset-name>"
    exit 1
fi

echo "Performing health check for StatefulSet: $STATEFULSET_NAME in namespace: $NAMESPACE"

# 检查基本状态
DESIRED=$(kubectl get statefulset $STATEFULSET_NAME -n $NAMESPACE -o jsonpath='{.spec.replicas}')
CURRENT=$(kubectl get statefulset $STATEFULSET_NAME -n $NAMESPACE -o jsonpath='{.status.replicas}')
READY=$(kubectl get statefulset $STATEFULSET_NAME -n $NAMESPACE -o jsonpath='{.status.readyReplicas}')
UPDATED=$(kubectl get statefulset $STATEFULSET_NAME -n $NAMESPACE -o jsonpath='{.status.updatedReplicas}')

echo "Desired: $DESIRED, Current: $CURRENT, Ready: $READY, Updated: $UPDATED"

# 检查Pod状态
echo "Checking individual pod status:"
kubectl get pods -n $NAMESPACE -l app=$STATEFULSET_NAME -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.phase
}{
        "\t"
}{
        .status.containerStatuses[*].ready
}{
        "\n"
}{
    end
}'

# 检查PVC状态
echo "Checking PVC status:"
kubectl get pvc -n $NAMESPACE -l app=$STATEFULSET_NAME -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.phase
}{
        "\n"
}{
    end
}'

# 检查网络连通性
echo "Checking network connectivity:"
for i in $(seq 0 $((DESIRED-1))); do
    POD_NAME="${STATEFULSET_NAME}-${i}"
    if kubectl exec -n $NAMESPACE $POD_NAME -- hostname >/dev/null 2>&1; then
        echo "  ✓ $POD_NAME: reachable"
    else
        echo "  ✗ $POD_NAME: unreachable"
    fi
done

# 健康评估
if [ "$CURRENT" = "$DESIRED" ] && [ "$READY" = "$DESIRED" ] && [ "$UPDATED" = "$DESIRED" ]; then
    echo "✓ StatefulSet is healthy"
    exit 0
else
    echo "✗ StatefulSet has issues"
    exit 1
fi
EOF

chmod +x statefulset-health-check.sh

# ========== 数据一致性检查 ==========
cat <<'EOF' > data-consistency-check.sh
#!/bin/bash

STATEFULSET_NAME=$1
NAMESPACE=${2:-default}
DATA_PATH=${3:-/data}

echo "Checking data consistency for StatefulSet: $STATEFULSET_NAME"

# 获取Pod列表
PODS=$(kubectl get pods -n $NAMESPACE -l app=$STATEFULSET_NAME -o jsonpath='{.items[*].metadata.name}')

# 收集每个Pod的数据摘要
for pod in $PODS; do
    echo "Collecting data from $pod..."
    kubectl exec -n $NAMESPACE $pod -- find $DATA_PATH -type f -exec md5sum {} \; > /tmp/${pod}_checksums.txt
done

# 比较校验和
echo "Comparing data consistency..."
REFERENCE_POD=$(echo $PODS | cut -d' ' -f1)
REFERENCE_CHECKSUMS="/tmp/${REFERENCE_POD}_checksums.txt"

for pod in $PODS; do
    if [ "$pod" != "$REFERENCE_POD" ]; then
        CURRENT_CHECKSUMS="/tmp/${pod}_checksums.txt"
        if diff $REFERENCE_CHECKSUMS $CURRENT_CHECKSUMS >/dev/null; then
            echo "✓ Data consistent between $REFERENCE_POD and $pod"
        else
            echo "✗ Data inconsistency detected between $REFERENCE_POD and $pod"
            diff $REFERENCE_CHECKSUMS $CURRENT_CHECKSUMS | head -10
        fi
    fi
done

# 清理临时文件
rm -f /tmp/*_checksums.txt
EOF

chmod +x data-consistency-check.sh
```
---

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 故障诊断 MOC
- [[19-故障诊断/README.md|Domain-12 故障排查 (Troubleshooting)]]
- Domain-12 故障排查 — 开源项目索引
- [[19-故障诊断/01-核心排障/01-control-plane-apiserver-troubleshooting.md|API Server 故障排查]]
- [[19-故障诊断/01-核心排障/02-control-plane-etcd-troubleshooting.md|etcd 故障排查]]
- [[19-故障诊断/01-核心排障/03-networking-cni-troubleshooting.md|CNI 网络插件故障排查]]
- [[19-故障诊断/01-核心排障/04-storage-csi-troubleshooting.md|CSI 存储驱动故障排查]]
- [[19-故障诊断/01-核心排障/05-pod-pending-diagnosis.md|Pod Pending 状态深度诊断]]
- [[19-故障诊断/01-核心排障/06-node-notready-diagnosis.md|Node NotReady 状态深度诊断]]
- [[19-故障诊断/01-核心排障/07-oom-memory-diagnosis.md|OOM 和内存问题诊断]]
- [[19-故障诊断/01-核心排障/08-pod-comprehensive-troubleshooting.md|Pod 全面故障排查]]
- [[19-故障诊断/02-资源排障/09-node-comprehensive-troubleshooting.md|Node 全面故障排查]]
- [[19-故障诊断/06-FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- [[19-故障诊断/02-资源排障/19-configmap-secret-troubleshooting.md|19-configmap-secret-troubleshooting]]
- [[19-故障诊断/02-资源排障/20-daemonset-troubleshooting.md|20-daemonset-troubleshooting]]
- [[19-故障诊断/02-资源排障/22-job-troubleshooting.md|22-job-troubleshooting]]
- [[19-故障诊断/02-资源排障/23-namespace-troubleshooting.md|23-namespace-troubleshooting]]

## Related

- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]


<!-- risk-assessed -->
