---
title: 升级迁移故障排查
description: '# 34 - 升级迁移故障排查 (Upgrade and Migration Troubleshooting)'
summary: '.status.nodeInfo.containerRuntimeVersion'
category: troubleshooting
tags:
- upgrade
- migration
- rollback
- kubeadm
- version
- etcd
- apiserver
- kubelet
- scheduler
- prometheus
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
- K8s 升级失败
- 版本不兼容
- 回滚
- 迁移
trigger_keywords:
- 升级迁移故障排查
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
- prometheus-basics
- monitoring-basics
- cilium-basics
- cni-basics
- etcd-basics
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
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 34 - 升级迁移故障排查 (Upgrade and Migration Troubleshooting)

---

<!-- chunk: 1. 升级迁移故障诊断总览 (Upgrade and Migration Diagnosis Overview) -->
## 1. 升级迁移故障诊断总览 (Upgrade and Migration Diagnosis Overview)

### 1.1 常见升级问题分类

| 问题类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **版本兼容性问题** | API废弃/变更 | 集群功能异常 | P0 - 紧急 |
| **组件升级失败** | Control Plane组件宕机 | 集群管控失效 | P0 - 紧急 |
| **工作负载中断** | Pod无法调度/运行 | 应用服务中断 | P0 - 紧急 |
| **存储数据丢失** | PV/PVC不兼容 | 数据持久化失败 | P0 - 紧急 |
| **网络插件失效** | CNI插件不兼容 | 网络通信中断 | P0 - 紧急 |

### 1.2 升级迁移架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                升级迁移故障诊断架构                                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       应用层兼容性                                   │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   工作负载   │   │   配置管理   │   │   服务发现   │              │  │
│  │  │ (Workloads) │   │ (Config)    │   │ (Discovery) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   API版本    │   │   存储兼容   │   │   网络兼容   │                   │
│  │ (API Version)│   │ (Storage)   │   │ (Network)   │                   │
│  │   兼容性     │   │   性       │   │   性       │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      Control Plane层                                │  │
│  │  ┌───────────────────────────────────────────────────────────────┐  │  │
│  │  │                   kube-apiserver                              │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐           │  │  │
│  │  │  │   etcd      │  │   控制器     │  │   调度器     │           │  │  │
│  │  │  │  (存储)     │  │  (Manager)  │  │  (Scheduler)│           │  │  │
│  │  │  └─────────────┘  └─────────────┘  └─────────────┘           │  │  │
│  │  └───────────────────────────────────────────────────────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   节点组件   │   │   网络插件   │   │   存储插件   │                   │
│  │ (Node Comp) │   │ (CNI)       │   │ (CSI)       │                   │
│  │   升级      │   │   兼容性     │   │   兼容性     │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      数据平面层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   kubelet   │    │   kube-proxy│   │   容器运行时  │              │  │
│  │  │   升级      │    │   升级      │   │  (Container) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 2. 升级前准备检查 (Pre-Upgrade Preparation Check) -->
## 2. 升级前准备检查 (Pre-Upgrade Preparation Check)

### 2.1 版本兼容性验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. 当前版本检查 ==========
# 检查集群当前版本
kubectl version --short

# 查看各组件版本
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.nodeInfo.kubeletVersion
}{
        "\t"
}{
        .status.nodeInfo.containerRuntimeVersion
}{
        "\n"
}{
    end
}'

# 检查Control Plane版本
kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{
    .items[*].spec.containers[*].image
}'

# ========== 2. 废弃API检查 ==========
# 检查将被废弃的API版本
cat <<'EOF' > deprecated-api-checker.sh
#!/bin/bash

TARGET_VERSION=${1:-v1.26.0}

echo "Checking deprecated APIs for upgrade to $TARGET_VERSION"

# 使用kubeval或类似的工具检查资源配置
for ns in $(kubectl get namespaces -o jsonpath='{.items[*].metadata.name}'); do
    echo "Checking namespace: $ns"
    
    # 检查Deployments
    kubectl get deployments -n $ns -o yaml | grep -E "apiVersion:.*extensions/v1beta1|apiVersion:.*apps/v1beta1" && \
        echo "  ⚠️  Deprecated API found in Deployments"
    
    # 检查Ingress
    kubectl get ingress -n $ns -o yaml | grep -E "apiVersion:.*extensions/v1beta1" && \
        echo "  ⚠️  Deprecated API found in Ingress"
    
    # 检查NetworkPolicy
    kubectl get networkpolicy -n $ns -o yaml | grep -E "apiVersion:.*extensions/v1beta1" && \
        echo "  ⚠️  Deprecated API found in NetworkPolicy"
done

echo "Deprecated API check completed"
EOF

chmod +x deprecated-api-checker.sh

# ========== 3. 升级路径验证 ==========
# 检查支持的升级路径
kubectl get nodes --no-headers | awk '{print $5}' | sort | uniq -c

# 验证最小升级跳跃
CURRENT_VERSION=$(kubectl version --short | grep Server | awk '{print $3}')
echo "Current version: $CURRENT_VERSION"

# 检查是否有跳版本升级的风险
if $CURRENT_VERSION == *"v1.22"* && $TARGET_VERSION == *"v1.24"*; then
    echo "⚠️  Risk of skipping versions detected"
fi
```
### 2.2 备份和回滚准备

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== etcd备份 ==========
# 执行etcd完整备份
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
ETCDCTL_API=3 kubectl exec -n kube-system $ETCD_POD -- sh -c "
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key

echo 'Creating etcd snapshot...'
etcdctl snapshot save /tmp/etcd-backup-$(date +%Y%m%d-%H%M%S).db
echo 'Snapshot created successfully'

# 验证备份
etcdctl --write-out=table snapshot status /tmp/etcd-backup-*.db
"

# ========== 资源配置备份 ==========
# 备份所有命名空间的资源配置
cat <<'EOF' > full-cluster-backup.sh
#!/bin/bash

BACKUP_DIR="/tmp/cluster-backup-$(date +%Y%m%d-%H%M%S)"
mkdir -p $BACKUP_DIR

echo "Creating full cluster backup to: $BACKUP_DIR"

# 备份所有命名空间
for ns in $(kubectl get namespaces -o jsonpath='{.items[*].metadata.name}'); do
    echo "Backing up namespace: $ns"
    mkdir -p $BACKUP_DIR/$ns
    
    # 备份各种资源类型
    RESOURCES=("deployments" "services" "configmaps" "secrets" "daemonsets" "statefulsets" "jobs" "cronjobs" "ingresses" "networkpolicies")
    
    for resource in "${RESOURCES[@]}"; do
        kubectl get $resource -n $ns -o yaml > $BACKUP_DIR/$ns/$resource.yaml 2>/dev/null || echo "No $resource in $ns"
    done
    
    # 备份PVC
    kubectl get pvc -n $ns -o yaml > $BACKUP_DIR/$ns/pvc.yaml 2>/dev/null
    
    # 备份RBAC配置
    kubectl get role,rolebinding,clusterrole,clusterrolebinding -n $ns -o yaml > $BACKUP_DIR/$ns/rbac.yaml 2>/dev/null
done

# 备份集群级别资源
echo "Backing up cluster-level resources..."
mkdir -p $BACKUP_DIR/cluster

CLUSTER_RESOURCES=("namespaces" "nodes" "persistentvolumes" "storageclasses" "customresourcedefinitions")
for resource in "${CLUSTER_RESOURCES[@]}"; do
    kubectl get $resource -o yaml > $BACKUP_DIR/cluster/$resource.yaml 2>/dev/null
done

# 创建备份摘要
cat > $BACKUP_DIR/backup-summary.txt <<SUMMARY
Cluster Backup Summary
=====================
Backup Time: $(date)
Kubernetes Version: $(kubectl version --short | grep Server | awk '{print $3}')
Nodes Count: $(kubectl get nodes --no-headers | wc -l)
Namespaces Count: $(kubectl get namespaces --no-headers | wc -l)

Backup Location: $BACKUP_DIR
SUMMARY

echo "Full cluster backup completed"
echo "Backup location: $BACKUP_DIR"
ls -la $BACKUP_DIR
EOF

chmod +x full-cluster-backup.sh
```
---

<!-- chunk: 3. Control Plane升级故障排查 (Control Plane Upgrade Troubleshooting) -->
## 3. Control Plane升级故障排查 (Control Plane Upgrade Troubleshooting)

### 3.1 API Server升级问题

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 1. API Server升级状态检查 ==========
# 检查API Server Pod状态
kubectl get pods -n kube-system -l component=kube-apiserver

# 查看API Server详细信息
kubectl describe pods -n kube-system -l component=kube-apiserver

# 检查API Server日志
kubectl logs -n kube-system -l component=kube-apiserver --tail=100

# 验证API Server健康状态
kubectl get componentstatuses

# ========== 2. API版本兼容性问题 ==========
# 检查API资源版本支持
kubectl api-versions | grep -E "(v1beta1|v1alpha1)" && echo "⚠️  Deprecated API versions found"

# 验证API组可用性
kubectl api-resources --api-group=apps
kubectl api-resources --api-group=networking.k8s.io

# 检查自定义资源定义兼容性
kubectl get crds -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.versions[*].name
}{
        "\n"
}{
    end
}'

# ========== 3. 证书和认证问题 ==========
# 检查证书有效期
kubectl exec -n kube-system $ETCD_POD -- openssl x509 -in /etc/kubernetes/pki/apiserver.crt -text -noout | grep "Not After"

# 验证证书链
kubectl exec -n kube-system $ETCD_POD -- openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt

# 检查RBAC权限变更
kubectl auth can-i list pods --all-namespaces
kubectl auth can-i create deployments --all-namespaces
```
### 3.2 etcd升级问题

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退
> - `kubectl scale --replicas=0`：缩容到 0，立即停服
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== etcd升级状态检查 ==========
# 检查etcd集群健康状态
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
kubectl exec -n kube-system $ETCD_POD -- sh -c "
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key

echo '=== etcd Cluster Health ==='
etcdctl endpoint health

echo '=== etcd Cluster Status ==='
etcdctl endpoint status -w table

echo '=== etcd Version Info ==='
etcdctl version
"

# ========== etcd数据兼容性检查 ==========
# 检查etcd数据版本
kubectl exec -n kube-system $ETCD_POD -- sh -c "
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key

echo 'Checking etcd data version:'
etcdctl get /registry --prefix --keys-only | head -20
"

# 验证关键数据完整性
kubectl exec -n kube-system $ETCD_POD -- sh -c "
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key

echo 'Verifying critical data:'
etcdctl get /registry/namespaces/default
etcdctl get /registry/deployments --prefix | wc -l
"

# ========== etcd升级回滚准备 ==========
# 创建etcd回滚脚本
cat <<'EOF' > etcd-rollback.sh
#!/bin/bash

BACKUP_FILE=$1

if [ -z "$BACKUP_FILE" ]; then
    echo "Usage: $0 <etcd-backup-file.db>"
    exit 1
fi

echo "=== etcd Rollback Procedure ==="
echo "Backup file: $BACKUP_FILE"

# 1. 停止etcd
echo "1. Stopping etcd..."
kubectl scale deployment -n kube-system etcd-operator --replicas=0

# 2. 恢复备份
echo "2. Restoring backup..."
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
kubectl cp $BACKUP_FILE kube-system/$ETCD_POD:/tmp/

kubectl exec -n kube-system $ETCD_POD -- sh -c "
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key

# 停止etcd服务
pkill etcd

# 恢复数据
etcdctl snapshot restore /tmp/$(basename $BACKUP_FILE) \
    --data-dir=/var/lib/etcd-restored \
    --initial-cluster=default=https://127.0.0.1:2380 \
    --initial-advertise-peer-urls=https://127.0.0.1:2380

# 替换数据目录
mv /var/lib/etcd /var/lib/etcd.backup.$(date +%Y%m%d-%H%M%S)
mv /var/lib/etcd-restored /var/lib/etcd

# 重启etcd
systemctl start etcd
"

# 3. 验证恢复
echo "3. Verifying restoration..."
sleep 30
kubectl exec -n kube-system $ETCD_POD -- sh -c "
export ETCDCTL_ENDPOINTS=https://127.0.0.1:2379
export ETCDCTL_CACERT=/etc/kubernetes/pki/etcd/ca.crt
export ETCDCTL_CERT=/etc/kubernetes/pki/etcd/server.crt
export ETCDCTL_KEY=/etc/kubernetes/pki/etcd/server.key

etcdctl endpoint health
etcdctl get /registry/namespaces/default
"

echo "etcd rollback completed"
EOF

chmod +x etcd-rollback.sh
```
---

<!-- chunk: 4. 节点组件升级故障排查 (Node Component Upgrade Troubleshooting) -->
## 4. 节点组件升级故障排查 (Node Component Upgrade Troubleshooting)

### 4.1 kubelet升级问题

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 1. kubelet状态检查 ==========
# 检查节点kubelet版本
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.nodeInfo.kubeletVersion
}{
        "\t"
}{
        .status.conditions[?(@.type=="Ready")].status
}{
        "\n"
}{
    end
}'

# 查看节点详细状态
kubectl describe node <node-name>

# 检查kubelet配置
kubectl debug node/<node-name> --image=busybox -it -- sh -c "
cat /var/lib/kubelet/config.yaml
ps aux | grep kubelet
"

# ========== 2. 升级兼容性验证 ==========
# 检查节点污点和容忍度
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.taints
}{
        "\n"
}{
    end
}'

# 验证Pod驱逐策略
kubectl get pods -A -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.annotations."cluster-autoscaler\.kubernetes\.io/safe-to-evict"
}{
        "\n"
}{
    end
}' | grep -v "true"

# ========== 3. 升级过程监控 ==========
# 监控节点升级进度
cat <<'EOF' > node-upgrade-monitor.sh
#!/bin/bash

echo "Monitoring node upgrade progress..."

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    echo "$TIMESTAMP === Node Status ==="
    kubectl get nodes -o jsonpath='{
        range .items[*]
    }{
            .metadata.name
    }{
            "\t"
    }{
            .status.nodeInfo.kubeletVersion
    }{
            "\t"
    }{
            .status.conditions[?(@.type=="Ready")].status
    }{
            "\n"
    }{
        end
    }'
    
    # 检查升级相关的事件
    echo "$TIMESTAMP === Recent Upgrade Events ==="
    kubectl get events --field-selector reason=NodeReady,reason=NodeNotReady --sort-by='.lastTimestamp' | tail -5
    
    # 检查DaemonSet状态
    echo "$TIMESTAMP === DaemonSet Status ==="
    kubectl get daemonsets -A -o jsonpath='{
        range .items[*]
    }{
            .metadata.namespace
    }{
            "/"
    }{
            .metadata.name
    }{
            "\t"
    }{
            .status.numberReady
    }{
            "/"
    }{
            .status.desiredNumberScheduled
    }{
            "\n"
    }{
        end
    }'
    
    echo "---"
    sleep 30
done
EOF

chmod +x node-upgrade-monitor.sh
```
### 4.2 容器运行时兼容性

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 容器运行时状态检查 ==========
# 检查容器运行时版本
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.nodeInfo.containerRuntimeVersion
}{
        "\n"
}{
    end
}'

# 验证CRI兼容性
kubectl debug node/<node-name> --image=busybox -it -- crictl info

# 检查镜像兼容性
kubectl debug node/<node-name> --image=busybox -it -- crictl images

# ========== 运行时升级验证 ==========
# 创建容器运行时兼容性测试
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: runtime-compatibility-test
  namespace: default
spec:
  containers:
  - name: test-container
    image: busybox
    command: ["sh", "-c", "echo 'Runtime test successful' && sleep 3600"]
    # 测试不同的安全上下文
    securityContext:
      runAsNonRoot: true
      runAsUser: 1000
  # 测试特权容器
  - name: privileged-test
    image: busybox
    command: ["sh", "-c", "mount && sleep 3600"]
    securityContext:
      privileged: true
  # 测试主机网络
  hostNetwork: true
EOF

# 验证测试结果
kubectl logs runtime-compatibility-test -c test-container
kubectl logs runtime-compatibility-test -c privileged-test
```
---

<!-- chunk: 5. 工作负载迁移问题排查 (Workload Migration Troubleshooting) -->
## 5. 工作负载迁移问题排查 (Workload Migration Troubleshooting)

### 5.1 应用兼容性验证

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ========== 1. API兼容性检查 ==========
# 检查应用使用的API版本
kubectl get deployments --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .spec.template.spec.containers[*].env[*].valueFrom.fieldRef.apiVersion
}{
        "\n"
}{
    end
}' | sort | uniq -c

# 验证Ingress API版本
kubectl get ingress --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .apiVersion
}{
        "\n"
}{
    end
}'

# 检查NetworkPolicy兼容性
kubectl get networkpolicy --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .apiVersion
}{
        "\n"
}{
    end
}'

# ========== 2. 存储兼容性检查 ==========
# 检查PV/PVC配置
kubectl get pv -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.csi.driver
}{
        "\t"
}{
        .spec.accessModes
}{
        "\n"
}{
    end
}'

# 验证存储类兼容性
kubectl get storageclass -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .provisioner
}{
        "\t"
}{
        .parameters
}{
        "\n"
}{
    end
}'

# ========== 3. 网络策略验证 ==========
# 检查CNI插件兼容性
kubectl get pods -n kube-system -l k8s-app -o jsonpath='{
    range .items[*]
}{
        .metadata.labels.k8s-app
}{
        "\t"
}{
        .spec.containers[*].image
}{
        "\n"
}{
    end
}' | grep -E "(calico|flannel|cilium|weave)"

# 验证网络策略功能
kubectl run network-test --image=busybox -n test -it --rm -- sh -c "
ping -c 3 8.8.8.8
nslookup kubernetes.default
"
```
### 5.2 滚动升级策略优化

```bash
# ========== 优雅升级配置 ==========
# 配置滚动升级策略
cat <<EOF > graceful-upgrade-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: graceful-upgrade-app
  namespace: production
spec:
  replicas: 6
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1      # 最大不可用Pod数
      maxSurge: 1            # 最大额外Pod数
  selector:
    matchLabels:
      app: graceful-app
  template:
    metadata:
      labels:
        app: graceful-app
    spec:
      containers:
      - name: app
        image: myapp:latest
        ports:
        - containerPort: 8080
        readinessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
        # 优雅终止配置
        lifecycle:
          preStop:
            exec:
              command: ["/bin/sh", "-c", "sleep 15"]
        terminationGracePeriodSeconds: 30
      # 升级期间的容忍度
      tolerations:
      - key: "node.kubernetes.io/not-ready"
        operator: "Exists"
        effect: "NoExecute"
        tolerationSeconds: 300
      - key: "node.kubernetes.io/unreachable"
        operator: "Exists"
        effect: "NoExecute"
        tolerationSeconds: 300
EOF

# ========== 蓝绿部署策略 ==========
# 创建蓝绿部署配置
cat <<EOF > blue-green-deployment.yaml
# 蓝色环境 (当前版本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-blue
  namespace: production
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
      version: blue
  template:
    metadata:
      labels:
        app: myapp
        version: blue
    spec:
      containers:
      - name: app
        image: myapp:v1.0
---
# 绿色环境 (新版本)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-green
  namespace: production
spec:
  replicas: 0  # 初始为0，升级时增加
  selector:
    matchLabels:
      app: myapp
      version: green
  template:
    metadata:
      labels:
        app: myapp
        version: green
    spec:
      containers:
      - name: app
        image: myapp:v2.0
---
# 服务指向蓝色环境
apiVersion: v1
kind: Service
metadata:
  name: myapp-service
  namespace: production
spec:
  selector:
    app: myapp
    version: blue  # 升级时切换到green
  ports:
  - port: 80
    targetPort: 8080
EOF
```

---

<!-- chunk: 6. 回滚和恢复策略 (Rollback and Recovery Strategies) -->
## 6. 回滚和恢复策略 (Rollback and Recovery Strategies)

### 6.1 自动回滚机制

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 健康检查和回滚 ==========
# 配置带有健康检查的Deployment
cat <<EOF > self-healing-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: self-healing-app
  namespace: production
spec:
  replicas: 3
  revisionHistoryLimit: 10  # 保留10个历史版本
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
      maxSurge: 1
  selector:
    matchLabels:
      app: self-healing-app
  template:
    metadata:
      labels:
        app: self-healing-app
    spec:
      containers:
      - name: app
        image: myapp:latest
        ports:
        - containerPort: 8080
        readinessProbe:
          httpGet:
            path: /health
            port: 8080
          failureThreshold: 3
          periodSeconds: 10
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          failureThreshold: 5
          periodSeconds: 15
---
# 配置Pod Disruption Budget
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: self-healing-pdb
  namespace: production
spec:
  minAvailable: 2
  selector:
    matchLabels:
      app: self-healing-app
EOF

# ========== 回滚检测脚本 ==========
cat <<'EOF' > rollback-detector.sh
#!/bin/bash

DEPLOYMENT_NAME=$1
NAMESPACE=${2:-default}
FAILURE_THRESHOLD=${3:-5}

if [ -z "$DEPLOYMENT_NAME" ]; then
    echo "Usage: $0 <deployment-name> [namespace] [failure-threshold]"
    exit 1
fi

echo "Monitoring deployment: $DEPLOYMENT_NAME in namespace: $NAMESPACE"

CONSECUTIVE_FAILURES=0
LAST_REVISION=$(kubectl rollout history deployment/$DEPLOYMENT_NAME -n $NAMESPACE --revision=0 2>/dev/null | tail -1 | awk '{print $1}')

while true; do
    # 检查部署状态
    READY_REPLICAS=$(kubectl get deployment $DEPLOYMENT_NAME -n $NAMESPACE -o jsonpath='{.status.readyReplicas}')
    AVAILABLE_REPLICAS=$(kubectl get deployment $DEPLOYMENT_NAME -n $NAMESPACE -o jsonpath='{.status.availableReplicas}')
    UPDATED_REPLICAS=$(kubectl get deployment $DEPLOYMENT_NAME -n $NAMESPACE -o jsonpath='{.status.updatedReplicas}')
    REPLICAS=$(kubectl get deployment $DEPLOYMENT_NAME -n $NAMESPACE -o jsonpath='{.spec.replicas}')
    
    echo "$(date): Ready: $READY_REPLICAS/$REPLICAS, Available: $AVAILABLE_REPLICAS/$REPLICAS, Updated: $UPDATED_REPLICAS/$REPLICAS"
    
    # 检查健康状况
    if [ "$READY_REPLICAS" -lt "$REPLICAS" ] || [ "$AVAILABLE_REPLICAS" -lt "$REPLICAS" ]; then
        CONSECUTIVE_FAILURES=$((CONSECUTIVE_FAILURES + 1))
        echo "⚠️  Health check failed ($CONSECUTIVE_FAILURES/$FAILURE_THRESHOLD)"
        
        if [ $CONSECUTIVE_FAILURES -ge $FAILURE_THRESHOLD ]; then
            echo "🚨 Initiating rollback due to consecutive failures"
            
            # 执行回滚
            kubectl rollout undo deployment/$DEPLOYMENT_NAME -n $NAMESPACE
            
            # 等待回滚完成
            kubectl rollout status deployment/$DEPLOYMENT_NAME -n $NAMESPACE --timeout=300s
            
            echo "✅ Rollback completed"
            break
        fi
    else
        CONSECUTIVE_FAILURES=0
        echo "✓ Health check passed"
    fi
    
    sleep 30
done
EOF

chmod +x rollback-detector.sh
```
### 6.2 灾难恢复流程

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl apply/create/replace`：创建/变更集群资源

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
# ========== 完整集群恢复 ==========
# 创建灾难恢复脚本
cat <<'EOF' > disaster-recovery.sh
#!/bin/bash

BACKUP_LOCATION=$1
RESTORE_NAMESPACE=${2:-all}

if [ -z "$BACKUP_LOCATION" ]; then
    echo "Usage: $0 <backup-location> [namespace]"
    exit 1
fi

echo "=== Kubernetes Disaster Recovery ==="
echo "Backup location: $BACKUP_LOCATION"
echo "Restore namespace: $RESTORE_NAMESPACE"

# 1. 环境准备
echo "1. Preparing recovery environment..."
kubectl create namespace recovery-temp 2>/dev/null || echo "Namespace already exists"

# 2. 恢复集群资源
echo "2. Restoring cluster resources..."

if [ "$RESTORE_NAMESPACE" = "all" ]; then
    # 恢复所有命名空间
    for ns_dir in $BACKUP_LOCATION/*/; do
        NS_NAME=$(basename $ns_dir)
        echo "Restoring namespace: $NS_NAME"
        
        # 创建命名空间
        kubectl create namespace $NS_NAME 2>/dev/null || echo "Namespace $NS_NAME already exists"
        
        # 恢复资源
        for resource_file in $ns_dir/*.yaml; do
            if [ -f "$resource_file" ]; then
                echo "  Applying $resource_file"
                kubectl apply -f $resource_file -n $NS_NAME 2>/dev/null || echo "    Failed to apply $resource_file"
            fi
        done
    done
else
    # 恢复特定命名空间
    echo "Restoring namespace: $RESTORE_NAMESPACE"
    kubectl create namespace $RESTORE_NAMESPACE 2>/dev/null || echo "Namespace already exists"
    
    for resource_file in $BACKUP_LOCATION/$RESTORE_NAMESPACE/*.yaml; do
        if [ -f "$resource_file" ]; then
            echo "  Applying $resource_file"
            kubectl apply -f $resource_file -n $RESTORE_NAMESPACE 2>/dev/null || echo "    Failed to apply $resource_file"
        fi
    done
fi

# 3. 验证恢复状态
echo "3. Verifying recovery status..."

if [ "$RESTORE_NAMESPACE" = "all" ]; then
    kubectl get namespaces
    for ns in $(kubectl get namespaces -o jsonpath='{.items[*].metadata.name}' | xargs -n1 | grep -v -E "(kube-system|kube-public|kube-node-lease)"); do
        echo "Checking namespace: $ns"
        kubectl get deployments,services -n $ns
    done
else
    echo "Checking namespace: $RESTORE_NAMESPACE"
    kubectl get deployments,services -n $RESTORE_NAMESPACE
fi

# 4. 应用健康检查
echo "4. Performing health checks..."
HEALTHY_DEPLOYMENTS=0
TOTAL_DEPLOYMENTS=0

for deploy in $(kubectl get deployments --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'); do
    NS=$(echo $deploy | cut -d/ -f1)
    DEPLOY_NAME=$(echo $deploy | cut -d/ -f2)
    
    READY=$(kubectl get deployment $DEPLOY_NAME -n $NS -o jsonpath='{.status.readyReplicas}')
    DESIRED=$(kubectl get deployment $DEPLOY_NAME -n $NS -o jsonpath='{.status.replicas}')
    
    TOTAL_DEPLOYMENTS=$((TOTAL_DEPLOYMENTS + 1))
    if [ "$READY" = "$DESIRED" ] && [ -n "$READY" ]; then
        HEALTHY_DEPLOYMENTS=$((HEALTHY_DEPLOYMENTS + 1))
        echo "  ✓ $deploy: $READY/$DESIRED ready"
    else
        echo "  ✗ $deploy: $READY/$DESIRED ready"
    fi
done

echo "Recovery Summary:"
echo "  Healthy deployments: $HEALTHY_DEPLOYMENTS/$TOTAL_DEPLOYMENTS"
echo "  Success rate: $((HEALTHY_DEPLOYMENTS * 100 / TOTAL_DEPLOYMENTS))%"

# 5. 清理临时资源
kubectl delete namespace recovery-temp  # ⚠️ 不可逆：永久删除命名空间及全部资源

echo "Disaster recovery process completed"
EOF

chmod +x disaster-recovery.sh

# ========== 恢复验证清单 ==========
cat <<'EOF' > recovery-validation-checklist.md
# Kubernetes 恢复验证清单

<!-- chunk: 基础设施验证 -->
## 基础设施验证
- [ ] Control Plane组件运行正常
- [ ] etcd集群健康状态
- [ ] 所有节点处于Ready状态
- [ ] 网络插件功能正常
- [ ] 存储插件功能正常

<!-- chunk: 应用验证 -->
## 应用验证
- [ ] 核心应用Pod运行正常
- [ ] 服务端点可达
- [ ] Ingress路由功能正常
- [ ] 数据库连接正常
- [ ] 外部依赖服务可达

<!-- chunk: 监控和告警 -->
## 监控和告警
- [ ] Prometheus指标收集正常
- [ ] Alertmanager告警功能正常
- [ ] Grafana仪表板显示正常
- [ ] 日志收集系统运行正常

<!-- chunk: 安全验证 -->
## 安全验证
- [ ] RBAC权限配置正确
- [ ] 网络策略生效
- [ ] Secret和ConfigMap加载正常
- [ ] TLS证书有效

<!-- chunk: 性能验证 -->
## 性能验证
- [ ] 应用响应时间在可接受范围内
- [ ] 资源使用率正常
- [ ] 没有明显的性能退化

<!-- chunk: 业务验证 -->
## 业务验证
- [ ] 核心业务流程可执行
- [ ] 用户可以正常访问服务
- [ ] 数据一致性和完整性验证
- [ ] 第三方集成服务正常
EOF
```
---

<!-- chunk: 7. 升级最佳实践 (Upgrade Best Practices) -->
## 7. 升级最佳实践 (Upgrade Best Practices)

### 7.1 分阶段升级策略

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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
# ========== 金丝雀升级配置 ==========
# 创建金丝雀升级部署
cat <<EOF > canary-upgrade.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: app-canary
  namespace: production
  labels:
    app: myapp
    track: canary
spec:
  replicas: 1  # 少量副本用于测试
  selector:
    matchLabels:
      app: myapp
      track: canary
  template:
    metadata:
      labels:
        app: myapp
        track: canary
    spec:
      containers:
      - name: app
        image: myapp:new-version
        ports:
        - containerPort: 8080
        env:
        - name: CANARY_ENABLED
          value: "true"
        resources:
          requests:
            cpu: "100m"
            memory: "128Mi"
          limits:
            cpu: "200m"
            memory: "256Mi"
---
# 金丝雀服务
apiVersion: v1
kind: Service
metadata:
  name: app-canary-service
  namespace: production
spec:
  selector:
    app: myapp
    track: canary
  ports:
  - port: 80
    targetPort: 8080
---
# 金丝雀Ingress（可选）
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-canary-ingress
  namespace: production
  annotations:
    nginx.ingress.kubernetes.io/canary: "true"
    nginx.ingress.kubernetes.io/canary-weight: "10"  # 10%流量
spec:
  rules:
  - host: myapp.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: app-canary-service
            port:
              number: 80
EOF

# ========== 分区升级脚本 ==========
cat <<'EOF' > phased-upgrade.sh
#!/bin/bash

UPGRADE_VERSION=$1
NODE_GROUPS=("control-plane" "worker-pool-1" "worker-pool-2")

if [ -z "$UPGRADE_VERSION" ]; then
    echo "Usage: $0 <target-version>"
    exit 1
fi

echo "=== Phased Upgrade to $UPGRADE_VERSION ==="

# 1. 升级Control Plane
echo "Phase 1: Upgrading Control Plane"
for node in $(kubectl get nodes -l node-role.kubernetes.io/control-plane -o jsonpath='{.items[*].metadata.name}'); do
    echo "Upgrading control plane node: $node"
    # 这里添加具体的升级命令
    # kubeadm upgrade node ...
done

# 等待Control Plane稳定
echo "Waiting for Control Plane stabilization..."
sleep 120

# 2. 验证Control Plane
echo "Verifying Control Plane..."
kubectl get componentstatuses
kubectl get nodes -l node-role.kubernetes.io/control-plane

# 3. 分批升级Worker节点
for group in "${NODE_GROUPS[@]:1}"; do
    echo "Phase: Upgrading $group"
    
    NODES=$(kubectl get nodes -l node-group=$group -o jsonpath='{.items[*].metadata.name}')
    NODE_ARRAY=($NODES)
    
    # 分批处理（每次2个节点）
    for ((i=0; i<${#NODE_ARRAY[@]}; i+=2)); do
        BATCH_NODES=("${NODE_ARRAY[@]:i:2}")
        echo "Upgrading batch: ${BATCH_NODES[*]}"
        
        # 驱逐Pod
        for node in "${BATCH_NODES[@]}"; do
            echo "Draining node: $node"
            kubectl drain $node --ignore-daemonsets --delete-emptydir-data
        done
        
        # 执行升级
        for node in "${BATCH_NODES[@]}"; do
            echo "Upgrading node: $node"
            # ssh $node "yum update kubelet kubeadm kubectl"
            # systemctl restart kubelet
        done
        
        # 验证节点状态
        sleep 60
        for node in "${BATCH_NODES[@]}"; do
            kubectl uncordon $node
            kubectl get node $node
        done
        
        # 验证工作负载
        sleep 120
        kubectl get pods -A --field-selector=spec.nodeName==$node
    done
done

echo "Phased upgrade completed"
EOF

chmod +x phased-upgrade.sh
```
### 7.2 升级验证和监控

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ========== 升级验证工具 ==========
cat <<'EOF' > upgrade-validator.sh
#!/bin/bash

TARGET_VERSION=$1

if [ -z "$TARGET_VERSION" ]; then
    echo "Usage: $0 <target-version>"
    exit 1
fi

echo "=== Kubernetes Upgrade Validator ==="
echo "Target version: $TARGET_VERSION"

# 1. 预升级检查
echo "1. Pre-upgrade validation..."

# 检查当前版本
CURRENT_VERSION=$(kubectl version --short | grep Server | awk '{print $3}')
echo "Current version: $CURRENT_VERSION"

# 检查升级路径
if "$CURRENT_VERSION" != *"$TARGET_VERSION"*; then
    echo "✓ Version upgrade path valid"
else
    echo "⚠️  Same version detected"
fi

# 2. 资源兼容性检查
echo "2. Resource compatibility check..."
./deprecated-api-checker.sh $TARGET_VERSION

# 3. 组件健康检查
echo "3. Component health check..."
kubectl get componentstatuses -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .conditions[?(@.type=="Healthy")].status
}{
        "\n"
}{
    end
}'

# 4. 节点状态检查
echo "4. Node status check..."
kubectl get nodes -o jsonpath='{
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
}' | grep -v "True" && echo "⚠️  Some nodes not ready"

# 5. 工作负载检查
echo "5. Workload status check..."
kubectl get deployments --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .status.readyReplicas
}{
        "/"
}{
        .status.replicas
}{
        "\n"
}{
    end
}' | awk '$3 != $4 {print "⚠️  " $0}'

echo "Upgrade validation completed"
EOF

chmod +x upgrade-validator.sh

# ========== 升级监控仪表板 ==========
cat <<'EOF' > upgrade-monitoring-dashboard.json
{
  "dashboard": {
    "title": "Kubernetes Upgrade Monitoring",
    "panels": [
      {
        "title": "Upgrade Progress",
        "type": "stat",
        "targets": [
          {
            "expr": "count(kube_node_info{version=\"$target_version\"})",
            "legendFormat": "Upgraded Nodes"
          },
          {
            "expr": "count(kube_node_info)",
            "legendFormat": "Total Nodes"
          }
        ]
      },
      {
        "title": "Component Health",
        "type": "graph",
        "targets": [
          {
            "expr": "up{job=\"kubernetes-apiservers\"}",
            "legendFormat": "API Server"
          },
          {
            "expr": "up{job=\"kubernetes-nodes\"}",
            "legendFormat": "Kubelet"
          }
        ]
      },
      {
        "title": "Workload Availability",
        "type": "graph",
        "targets": [
          {
            "expr": "kube_deployment_status_replicas_available",
            "legendFormat": "{{deployment}}"
          }
        ]
      },
      {
        "title": "Upgrade Events",
        "type": "table",
        "targets": [
          {
            "expr": "kube_event_count{reason=~\"NodeReady|NodeNotReady|Upgrade\"}",
            "legendFormat": "{{reason}}"
          }
        ]
      }
    ]
  }
}
EOF
```
---

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-10-troubleshooting-diagnostics KUDIG Database — Global MOC
- [[domain-10-troubleshooting-diagnostics/README.md|Domain-12 故障排查 (Troubleshooting)]]
- index.md|Domain-12 故障排查 — 开源项目索引]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/01-control-plane-apiserver-troubleshooting.md|[[API Server 故障排查|API Server 故障排查]]]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/02-control-plane-etcd-troubleshooting.md|[[etcd 故障排查|etcd 故障排查]]]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/03-networking-cni-troubleshooting.md|CNI 网络插件故障排查]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/04-storage-csi-troubleshooting.md|CSI 存储驱动故障排查]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/05-pod-pending-diagnosis.md|Pod Pending 状态深度诊断]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/06-node-notready-diagnosis.md|Node NotReady 状态深度诊断]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/07-oom-memory-diagnosis.md|OOM 和内存问题诊断]]
- [[domain-10-troubleshooting-diagnostics/00-core-troubleshooting/08-pod-comprehensive-troubleshooting.md|Pod 全面故障排查]]
- [[domain-10-troubleshooting-diagnostics/01-resource-troubleshooting/09-node-comprehensive-troubleshooting.md|Node 全面故障排查]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- [[domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/32-security-troubleshooting.md|32-security-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/02-infrastructure-troubleshooting/33-performance-bottleneck-troubleshooting.md|33-performance-bottleneck-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/03-advanced-troubleshooting/35-node-component-troubleshooting.md|35-node-component-troubleshooting]]
- [[domain-10-troubleshooting-diagnostics/03-advanced-troubleshooting/36-helm-chart-troubleshooting.md|36-helm-chart-troubleshooting]]

## Related

- [[domain-19-landscape-references/topic-index/cluster-index.md|Cluster 集群知识图谱索引]]


<!-- risk-assessed -->
