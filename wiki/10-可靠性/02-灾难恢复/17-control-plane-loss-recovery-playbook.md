---
title: 控制面全部丢失的灾难恢复
description: '从 etcd snapshot 重建集群全流程、kubeadm init 恢复、证书重生成、kubeconfig 重建及 worker 节点重新加入'
summary: '从 etcd snapshot 重建集群全流程、kubeadm init 恢复、证书重生成、kubeconfig 重建及 worker 节点重新加入'
category: reliability-engineering
tags:
- disaster-recovery
- k8s
- sre
- control-plane
- etcd
- kubeadm
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- 控制面全部丢失的灾难恢复 是什么
- 如何 控制面全部丢失的灾难恢复
- 从 etcd snapshot 重建集群
trigger_keywords:
- control-plane
- disaster-recovery
- etcd-snapshot
- kubeadm-init
- cluster-rebuild
prerequisites:
- kubectl-basics
- etcd-basics
- sre-practices
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


# 控制面全部丢失的灾难恢复

## 概述

控制面全部丢失是 Kubernetes 集群最严重的灾难场景，通常由以下原因导致：

- 机房级故障（火灾、水灾、断电）
- 云平台区域级故障
- 存储系统全面故障
- 人为误操作删除控制面节点
- 安全攻击导致控制面被破坏

在这种场景下，API Server、Controller Manager、Scheduler、etcd 全部不可用，集群无法接受任何 API 请求。恢复的核心是从 etcd snapshot 重建控制面，然后重新加入 worker 节点。

### 恢复前提

| 条件 | 说明 | 必要性 |
|------|------|--------|
| etcd snapshot 备份 | 定期创建的 etcd 数据快照 | 必须 |
| 控制面配置备份 | kubeadm-config、静态 Pod 配置 | 强烈推荐 |
| 证书备份 | PKI 目录完整备份 | 强烈推荐 |
| Worker 节点可用 | 数据面节点 SSH 可达 | 必须 |
| 备份验证记录 | 定期恢复演练记录 | 推荐 |

### 恢复流程概览

```
准备阶段 → etcd 恢复 → kubeadm init → 证书重建 → kubeconfig 重建 → Worker 加入 → 验证
```

## 详细步骤

### 第一阶段：环境评估与准备

#### 1.1 确认灾难范围

```bash
# 尝试连接原控制面节点
for cp in 10.0.0.1 10.0.0.2 10.0.0.3; do
  echo "=== $cp ==="
  ssh -o ConnectTimeout=5 root@$cp "hostname && uptime" 2>&1
done

# 检查是否有任何控制面节点可恢复
# 如果有部分节点可恢复，优先使用增量恢复而非全量重建
```

#### 1.2 确认 Worker 节点状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 列出所有 worker 节点
for worker in 10.0.0.10 10.0.0.11 10.0.0.12; do
  echo "=== $worker ==="
  ssh -o ConnectTimeout=5 root@$worker "hostname && systemctl is-active kubelet containerd" 2>&1
done

# 检查 worker 节点上的 Pod 状态（kubelet 会进入自治模式）
for worker in 10.0.0.10 10.0.0.11 10.0.0.12; do
  echo "=== $worker ==="
  ssh root@$worker "crictl ps | head -10" 2>&1
done
```
#### 1.3 定位 etcd 备份

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查备份存储位置
# 常见位置：
# - 本地：/var/lib/etcd-snapshot/
# - NFS：/mnt/nfs/etcd-backup/
# - 对象存储：s3://bucket/etcd-backup/

# 查找最新的备份
ls -lt /var/lib/etcd-snapshot/ | head -5

# 如果备份在对象存储
aws s3 ls s3://my-k8s-backup/etcd/ --recursive | sort | tail -5

# 下载最新备份
aws s3 cp s3://my-k8s-backup/etcd/latest.db /tmp/etcd-snapshot.db
```
#### 1.4 验证备份完整性

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证 snapshot 文件
ETCDCTL_API=3 etcdctl snapshot status /tmp/etcd-snapshot.db --write-out=table

# 输出示例：
# +----------+----------+------------+------------+
# |   HASH   | REVISION | TOTAL KEYS | TOTAL SIZE |
# +----------+----------+------------+------------+
# | a1b2c3d4 |  1234567 |       5892 |     128 MB |
# +----------+----------+------------+------------+

# 记录备份的 revision 和时间，用于恢复后验证
```
#### 1.5 准备恢复节点

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 选择一个节点作为新的主控制面节点
# 可以是：
# 1. 新创建的节点
# 2. 恢复后的原控制面节点
# 3. 升级一个 worker 节点为控制面

# 在新节点上安装必要组件
yum install -y kubeadm-<version> kubelet-<version> kubectl-<version> containerd.io
systemctl enable --now containerd kubelet
```
### 第二阶段：从 etcd Snapshot 恢复

#### 2.1 恢复 etcd 数据

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ⚠️ 以下操作在新的主控制面节点上执行

# 创建 etcd 数据目录
mkdir -p /var/lib/etcd

# 从 snapshot 恢复
ETCDCTL_API=3 etcdctl snapshot restore \
  /tmp/etcd-snapshot.db \
  --data-dir=/var/lib/etcd \
  --name=etcd-1 \
  --initial-cluster=etcd-1=https://10.0.0.1:2380 \
  --initial-advertise-peer-urls=https://10.0.0.1:2380 \
  --initial-cluster-token=k8s-recovery-$(date +%Y%m%d)

# 设置正确的文件权限
chown -R etcd:etcd /var/lib/etcd
chmod 700 /var/lib/etcd
```
#### 2.2 临时启动 etcd（用于数据验证）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建临时 etcd 配置
cat > /tmp/etcd-temp.conf << EOF
ETCD_NAME=etcd-1
ETCD_DATA_DIR=/var/lib/etcd
ETCD_LISTEN_CLIENT_URLS=https://127.0.0.1:2379
ETCD_ADVERTISE_CLIENT_URLS=https://127.0.0.1:2379
ETCD_LISTEN_PEER_URLS=https://127.0.0.1:2380
ETCD_INITIAL_ADVERTISE_PEER_URLS=https://127.0.0.1:2380
ETCD_INITIAL_CLUSTER=etcd-1=https://127.0.0.1:2380
ETCD_INITIAL_CLUSTER_STATE=new
ETCD_INITIAL_CLUSTER_TOKEN=k8s-recovery-temp
ETCD_CERT_FILE=/etc/kubernetes/pki/etcd/server.crt
ETCD_KEY_FILE=/etc/kubernetes/pki/etcd/server.key
ETCD_TRUSTED_CA_FILE=/etc/kubernetes/pki/etcd/ca.crt
ETCD_PEER_CERT_FILE=/etc/kubernetes/pki/etcd/peer.crt
ETCD_PEER_KEY_FILE=/etc/kubernetes/pki/etcd/peer.key
ETCD_PEER_TRUSTED_CA_FILE=/etc/kubernetes/pki/etcd/ca.crt
EOF

# 使用临时配置启动 etcd
# 如果使用 systemd，临时修改 service 文件
# 或直接运行 etcd 二进制
source /tmp/etcd-temp.conf && etcd &

# 验证数据完整性
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=table

# 检查关键数据是否存在
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  get /registry/namespaces --prefix --keys-only | head -10

# 停止临时 etcd
kill %1
```
### 第三阶段：kubeadm init 恢复

#### 3.1 准备 kubeadm 配置文件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 如果有备份的 kubeadm-config，直接使用
cp /path/to/backup/kubeadm-config.yaml /etc/kubernetes/kubeadm-config.yaml

# 如果没有备份，从 etcd 中恢复
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  get /registry/clusterstates/kubeadm-config --print-value-only > /etc/kubernetes/kubeadm-config.yaml

# 如果以上都没有，手动创建
cat > /etc/kubernetes/kubeadm-config.yaml << EOF
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
kubernetesVersion: v1.30.0
controlPlaneEndpoint: "10.0.0.100:6443"
networking:
  podSubnet: "10.244.0.0/16"
  serviceSubnet: "10.96.0.0/12"
etcd:
  local:
    dataDir: /var/lib/etcd
EOF
```
#### 3.2 执行 kubeadm init with etcd 恢复

```bash
# ⚠️ 这是关键步骤，使用 --upload-certs 标志

# 首先恢复 etcd（使用第二阶段的步骤）

# 执行 kubeadm init，使用已恢复的 etcd 数据
kubeadm init \
  --config=/etc/kubernetes/kubeadm-config.yaml \
  --upload-certs \
  --skip-phases=etcd

# --skip-phases=etcd 跳过 etcd 初始化，因为我们已经从 snapshot 恢复了
# --upload-certs 将控制面证书上传到集群，便于后续加入其他控制面节点

# 输出示例：
# Your Kubernetes control plane has initialized successfully!
#
# To start using your cluster, you need to run the following as a regular user:
#   mkdir -p $HOME/.kube
#   sudo cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
#   sudo chown $(id -u):$(id -g) $HOME/.kube/config
#
# Then you can join any number of control-plane nodes by running:
#   kubeadm join 10.0.0.100:6443 --token <token> --discovery-token-ca-cert-hash sha256:<hash> --control-plane --certificate-key <key>
#
# Then you can join any number of worker nodes by running:
#   kubeadm join 10.0.0.100:6443 --token <token> --discovery-token-ca-cert-hash sha256:<hash>
```

#### 3.3 初始化 kubectl 配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 设置 kubectl 配置
mkdir -p $HOME/.kube
cp -i /etc/kubernetes/admin.conf $HOME/.kube/config
chown $(id -u):$(id -g) $HOME/.kube/config

# 验证集群状态
kubectl get nodes
kubectl get pods -n kube-system
```
### 第四阶段：证书重新生成

#### 4.1 检查证书状态

```bash
# 查看证书过期时间
kubeadm certs check-expiration

# 检查所有证书文件
ls -la /etc/kubernetes/pki/
ls -la /etc/kubernetes/pki/etcd/
```

#### 4.2 重新生成证书

```bash
# 如果证书缺失或过期，重新生成
# 重新生成所有证书
kubeadm certs renew all

# 或逐个重新生成
kubeadm certs renew apiserver
kubeadm certs renew apiserver-kubelet-client
kubeadm certs renew front-proxy-client
kubeadm certs renew etcd-server
kubeadm certs renew etcd-peer
kubeadm certs renew etcd-healthcheck-client

# 验证证书
kubeadm certs check-expiration
```

#### 4.3 证书分发（如有多控制面）

```bash
# 如果使用 --upload-certs，证书加密后存储在 kubeadm-certs Secret 中
# 其他控制面节点加入时会自动获取

# 手动分发证书（如需要）
# 将 /etc/kubernetes/pki/ 目录复制到其他控制面节点
scp -r /etc/kubernetes/pki/ root@10.0.0.2:/etc/kubernetes/pki/
scp -r /etc/kubernetes/pki/ root@10.0.0.3:/etc/kubernetes/pki/
```

### 第五阶段：kubeconfig 重建

#### 5.1 重建管理员 kubeconfig

```bash
# 重新生成 admin.conf
kubeadm kubeconfig user --client-name=kubernetes-admin --org=system:masters

# 或使用 kubeadm init 自动生成的 admin.conf
cp /etc/kubernetes/admin.conf $HOME/.kube/config
```

#### 5.2 重建组件 kubeconfig

```bash
# Controller Manager kubeconfig
kubeadm kubeconfig user \
  --client-name=system:kube-controller-manager \
  --config=/etc/kubernetes/kubeadm-config.yaml

# Scheduler kubeconfig
kubeadm kubeconfig user \
  --client-name=system:kube-scheduler \
  --config=/etc/kubernetes/kubeadm-config.yaml

# kubelet kubeconfig（每个节点不同）
# 通常由 kubeadm join 自动生成
```

#### 5.3 分发 kubeconfig 到服务账号

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确保 ServiceAccount token secret 存在
kubectl get secrets -n kube-system | grep token

# 如果 ServiceAccount token 丢失，需要重新创建
# Kubernetes 1.24+ 使用 TokenRequest API，不再自动创建 secret
# 需要为需要的服务创建 long-lived token
kubectl create token <service-account-name> -n kube-system --duration=87600h
```
### 第六阶段：Worker 节点重新加入

#### 6.1 生成 Join Token

```bash
# 在控制面节点生成新的 join token
kubeadm token create --print-join-command

# 输出示例：
# kubeadm join 10.0.0.100:6443 --token abcdef.0123456789abcdef --discovery-token-ca-cert-hash sha256:xxxx
```

#### 6.2 Worker 节点加入集群

```bash
# 在每个 worker 节点上执行
# 如果节点是全新的
kubeadm join 10.0.0.100:6443 --token <token> --discovery-token-ca-cert-hash sha256:<hash>

# 如果节点之前属于旧集群，需要先重置
kubeadm reset -f
rm -rf /etc/kubernetes/
rm -rf /var/lib/kubelet/
rm -rf /etc/cni/net.d/
iptables -F -t nat
iptables -F
ip link delete cni0 2>/dev/null
ip link delete flannel.1 2>/dev/null

# 然后 join
kubeadm join 10.0.0.100:6443 --token <token> --discovery-token-ca-cert-hash sha256:<hash>
```

#### 6.3 批量加入脚本

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
#!/bin/bash
# bulk-worker-join.sh

WORKERS=("10.0.0.10" "10.0.0.11" "10.0.0.12")
JOIN_CMD="kubeadm join 10.0.0.100:6443 --token <token> --discovery-token-ca-cert-hash sha256:<hash>"

for worker in "${WORKERS[@]}"; do
  echo "=== Processing $worker ==="

  # SSH 可达性检查
  if ! ssh -o ConnectTimeout=5 root@$worker "true" 2>/dev/null; then
    echo "  SSH unreachable, skipping"
    continue
  fi

  # 重置节点
  echo "  Resetting node..."
  ssh root@$worker "kubeadm reset -f"
  ssh root@$worker "rm -rf /etc/kubernetes/ /var/lib/kubelet/ /etc/cni/net.d/"
  ssh root@$worker "iptables -F -t nat && iptables -F"

  # 加入集群
  echo "  Joining cluster..."
  ssh root@$worker "$JOIN_CMD"

  sleep 30

  # 验证状态
  NODE_NAME=$(ssh root@$worker "hostname")
  STATUS=$(kubectl get node $NODE_NAME --no-headers 2>/dev/null | awk '{print $2}')
  echo "  Node status: $STATUS"
  echo ""
done
```
#### 6.4 验证节点加入

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查所有节点状态
kubectl get nodes -o wide

# 等待所有节点 Ready
for node in $(kubectl get nodes --no-headers | awk '{print $1}'); do
  kubectl wait --for=condition=Ready node/$node --timeout=300s
done

# 检查节点上的系统 Pod
kubectl get pods -n kube-system -o wide
```
### 第七阶段：恢复后验证

#### 7.1 集群完整性验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 验证核心组件
kubectl get componentstatuses
kubectl get pods -n kube-system

# 验证 API Server 功能
kubectl api-resources
kubectl api-versions

# 验证 etcd 数据完整性
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health
```
#### 7.2 业务资源验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Namespace
kubectl get namespaces

# 检查核心工作负载
kubectl get deployments -A
kubectl get statefulsets -A
kubectl get daemonsets -A

# 检查 Service
kubectl get services -A

# 检查 ConfigMap 和 Secret
kubectl get configmaps -A | wc -l
kubectl get secrets -A | wc -l

# 检查 Ingress
kubectl get ingress -A
```
#### 7.3 网络功能验证

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 CNI 状态
kubectl get pods -n kube-system -l k8s-app=calico-node
# 或
kubectl get pods -n kube-system -l app=flannel

# 创建测试 Pod 验证网络
kubectl run test-pod --image=busybox -- sleep 3600
kubectl exec test-pod -- nslookup kubernetes.default
kubectl exec test-pod -- wget -qO- http://kubernetes.default.svc.cluster.local
kubectl delete pod test-pod

# 验证跨节点通信
kubectl run test-pod-1 --image=nginx --overrides='{"spec":{"nodeName":"<node-1>"}}'
kubectl run test-pod-2 --image=busybox --overrides='{"spec":{"nodeName":"<node-2>"}}'
kubectl exec test-pod-2 -- wget -qO- http://<test-pod-1-ip>
kubectl delete pod test-pod-1 test-pod-2
```
#### 7.4 数据一致性验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 对比恢复前后的资源数量（如果有记录）
# 恢复前应记录：
# - kubectl get all -A | wc -l
# - kubectl get secrets -A | wc -l
# - kubectl get configmaps -A | wc -l

# 检查是否有资源丢失
kubectl get all -A -o json | jq '.items | length'

# 检查 PV/PVC 状态
kubectl get pv
kubectl get pvc -A

# 检查 ServiceAccount
kubectl get serviceaccounts -A
```
#### 7.5 恢复 CronJob 和定时任务

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 CronJob 状态
kubectl get cronjobs -A

# 验证 CronJob 是否正常调度
kubectl get jobs -A --sort-by='.metadata.creationTimestamp' | tail -10
```
#### 7.6 监控和日志系统恢复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查监控组件
kubectl get pods -n monitoring
# Prometheus
# Grafana
# AlertManager

# 检查日志组件
kubectl get pods -n logging
# Elasticsearch
# Fluentd/Fluent Bit
# Kibana

# 验证 metrics-server
kubectl top nodes
kubectl top pods -A
```
## 生产最佳实践

### 备份策略

- **etcd 备份频率**：每小时自动备份，保留最近 72 小时
- **异地备份**：至少一份备份存储在不同区域/可用区
- **备份验证**：每周执行一次恢复演练
- **备份加密**：备份文件加密存储

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 示例：etcd 备份 CronJob
# 使用 etcdctl snapshot save + 对象存储上传
```
### 灾难恢复演练

- 每季度执行一次完整的控制面恢复演练
- 记录恢复时间目标（RTO）和恢复点目标（RPO）
- 演练内容包括：etcd 恢复、kubeadm init、worker 加入、业务验证

### 架构冗余

- 控制面节点分布在不同的可用区
- 使用负载均衡器暴露 API Server
- etcd 使用 3 或 5 节点集群，容忍 (n-1)/2 故障
- 考虑使用托管 Kubernetes 服务减少控制面管理负担

### 文档化

- 维护最新的集群配置文档
- 记录所有自定义配置和修改
- 保存 kubeadm-config 的版本控制历史
- 记录网络配置（CNI、Pod CIDR、Service CIDR）

## 故障排查

### 场景 1：kubeadm init 失败报 etcd 错误

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 etcd 数据目录权限
ls -la /var/lib/etcd/

# 检查 etcd 日志
journalctl -u etcd --no-pager -n 50

# 确认 etcd snapshot 恢复是否成功
ETCDCTL_API=3 etcdctl snapshot status /tmp/etcd-snapshot.db

# 重新执行 etcd 恢复
ETCDCTL_API=3 etcdctl snapshot restore /tmp/etcd-snapshot.db --data-dir=/var/lib/etcd ...
```
### 场景 2：Worker 节点 join 失败

```bash
# 检查 token 是否有效
kubeadm token list

# 如果 token 过期，重新生成
kubeadm token create

# 检查 CA 证书 hash
openssl x509 -pubkey -in /etc/kubernetes/pki/ca.crt | \
  openssl rsa -pubin -outform der 2>/dev/null | \
  openssl dgst -sha256 -hex | sed 's/^.* //'

# 检查网络连通性
ssh root@<worker> "curl -k https://10.0.0.100:6443/healthz"
```

### 场景 3：恢复后 Pod 无法调度

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查调度器日志
kubectl logs -n kube-system -l component=kube-scheduler

# 检查节点 taint
kubectl describe node <node-name> | grep Taints

# 检查资源容量
kubectl describe node <node-name> | grep -A 10 "Allocated resources"
```
### 场景 4：恢复后 Service 访问异常

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 kube-proxy
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy

# 检查 iptables 规则
iptables -t nat -L KUBE-SERVICES

# 重启 kube-proxy
kubectl rollout restart daemonset kube-proxy -n kube-system
```
### 场景 5：恢复后 Ingress 不工作

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Ingress Controller
kubectl get pods -n ingress-nginx
kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx

# 检查 Ingress 资源
kubectl get ingress -A
kubectl describe ingress <ingress-name>

# 检查 LoadBalancer Service
kubectl get svc -n ingress-nginx
```
## 参考链接

- [Kubernetes 官方文档 - 灾难恢复](https://kubernetes.io/docs/tasks/administer-cluster/cluster-management/#recovering-your-cluster)
- [kubeadm 初始化文档](https://kubernetes.io/docs/reference/setup-tools/kubeadm/kubeadm-init/)
- [etcd 灾难恢复指南](https://etcd.io/docs/latest/op-guide/recovery/)
- [Kubernetes 证书管理](https://kubernetes.io/docs/tasks/administer-cluster/kubeadm/kubeadm-certs/)
- [kubeadm 高可用集群](https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/high-availability/)

---

*本手册适用于 Kubernetes 1.28-1.32 版本。执行灾难恢复前，请确保已备份所有关键数据。*


<!-- risk-assessed -->
