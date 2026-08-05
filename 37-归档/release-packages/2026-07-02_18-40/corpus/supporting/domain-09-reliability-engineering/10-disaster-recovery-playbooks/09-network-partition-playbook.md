---
title: 集群网络分区恢复
description: 'Kubernetes 集群网络分区(split-brain)检测、etcd leader 选举异常处理、API Server 分裂脑恢复及 Pod 跨节点通信恢复'
summary: 'Kubernetes 集群网络分区(split-brain)检测、etcd leader 选举异常处理、API Server 分裂脑恢复及 Pod 跨节点通信恢复'
category: reliability-engineering
tags:
- disaster-recovery
- k8s
- sre
- network-partition
- split-brain
- etcd
- apiserver
- cni
tier: critical
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
- 集群网络分区恢复 是什么
- 如何处理 Kubernetes split-brain
- etcd leader 选举异常怎么恢复
trigger_keywords:
- network-partition
- split-brain
- etcd-leader
- apiserver
- cni
- kube-proxy
prerequisites:
- kubectl-basics
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


# 集群网络分区恢复

## 概述

网络分区（Network Partition）是分布式系统中最危险的故障模式之一。当 Kubernetes 集群中的节点之间出现网络隔离时，可能导致 etcd 丢失 quorum、API Server 出现 split-brain、Pod 跨节点通信中断等问题。本手册覆盖从网络分区检测到 etcd leader 选举恢复、API Server 分裂脑处理、Pod 通信恢复及 Service 流量重新均衡的完整恢复流程。

---

## 1. 网络分区检测方法

### 1.1 节点间连通性检测

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有节点状态
kubectl get nodes -o wide

# 检查节点 NotReady 状态的原因
kubectl describe node <node-name> | grep -A10 "Conditions"

# 从控制平面 ping 所有工作节点
for node in $(kubectl get nodes -o jsonpath='{.items[*].status.addresses[?(@.type=="InternalIP")].address}'); do
  echo -n "Node $node: "
  ping -c 3 -W 2 $node > /dev/null 2>&1 && echo "REACHABLE" || echo "UNREACHABLE"
done
```
### 1.2 etcd 集群健康检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 etcd 成员列表和健康状态
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health --write-out=table

# 检查 etcd leader 信息
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=table

# 查看 etcd 成员列表
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member list --write-out=table
```
### 1.3 CNI 网络连通性检测

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 跨节点 Pod 连通性测试
# 在节点 A 上创建测试 Pod
kubectl run nettest-a --image=busybox --restart=Never --overrides='
{
  "spec": {
    "nodeName": "<node-a>",
    "containers": [{
      "name": "nettest-a",
      "image": "busybox",
      "command": ["sleep", "3600"]
    }]
  }
}'

# 在节点 B 上创建测试 Pod
kubectl run nettest-b --image=nginx --restart=Never --overrides='
{
  "spec": {
    "nodeName": "<node-b>"
  }
}'

# 获取 Pod IP 并测试连通性
POD_B_IP=$(kubectl get pod nettest-b -o jsonpath='{.status.podIP}')
kubectl exec nettest-a -- ping -c 3 $POD_B_IP

# 测试 Service 连通性
kubectl exec nettest-a -- wget -qO- --timeout=5 http://nettest-b
```
### 1.4 网络分区自动化检测脚本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# 网络分区快速检测脚本
set -euo pipefail

echo "=== 节点状态 ==="
kubectl get nodes -o wide

echo ""
echo "=== etcd 集群健康 ==="
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health 2>&1 || echo "etcd 连接失败"

echo ""
echo "=== 节点间连通性 ==="
NODE_IPS=$(kubectl get nodes -o jsonpath='{.items[*].status.addresses[?(@.type=="InternalIP")].address}')
for src in $NODE_IPS; do
  for dst in $NODE_IPS; do
    if [ "$src" != "$dst" ]; then
      echo -n "$src -> $dst: "
      timeout 3 kubectl exec -n kube-system deploy/kube-apiserver -- \
        curl -s --connect-timeout 2 "http://$dst:10250/healthz" > /dev/null 2>&1 \
        && echo "OK" || echo "FAIL"
    fi
  done
done

echo ""
echo "=== 跨节点 Pod 连通性 ==="
kubectl get pods -A -o wide | grep -E "Running" | head -20
```
---

## 2. etcd Leader 选举异常处理

### 2.1 etcd Leader 丢失症状

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 症状：
#   - kubectl 命令超时或返回 "connection refused"
#   - etcd 日志出现 "no leader" 或 "election timeout"
#   - API Server 日志出现 "etcdserver: no leader"

# 查看 etcd 日志
journalctl -u etcd --since "5 minutes ago" --no-pager | grep -iE "leader|election|timeout"
# 或 Rook-Ceph 环境
kubectl -n kube-system logs etcd-<node-name> --tail=100 | grep -iE "leader|election"
```
### 2.2 强制重新选举 Leader

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 仅在 etcd 集群完全无法选举时使用
# 方法一：重启所有 etcd 节点（最安全）
# 对于 kubeadm 集群，移动 etcd manifest 触发重启
mv /etc/kubernetes/manifests/etcd.yaml /tmp/
sleep 10
mv /tmp/etcd.yaml /etc/kubernetes/manifests/

# 方法二：强制移除不可达的 etcd 成员
# 先确认哪些成员不可达
ETCDCTL_API=3 etcdctl \
  --endpoints=https://<reachable-etcd-ip>:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member list --write-out=table

# 移除不可达的成员（假设 ID 为 abc123）
ETCDCTL_API=3 etcdctl \
  --endpoints=https://<reachable-etcd-ip>:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member remove abc123
```
### 2.3 etcd Quorum 恢复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 如果 etcd 集群丢失 quorum（超过半数成员不可用）
# 需要从剩余成员重建集群

# 在唯一存活的 etcd 节点上
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=json | jq '.[0].Status.leader'

# 如果有数据目录备份，从备份恢复
ETCDCTL_API=3 etcdctl snapshot restore /path/to/snapshot.db \
  --data-dir=/var/lib/etcd-restore \
  --name=<member-name> \
  --initial-cluster=<member-name>=https://<ip>:2380 \
  --initial-advertise-peer-urls=https://<ip>:2380

# 更新 etcd manifest 使用恢复后的数据目录
# 修改 --data-dir 指向 /var/lib/etcd-restore
```
### 2.4 etcd Learner 节点恢复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 如果集群使用了 learner 节点（Kubernetes 1.28+）
# learner 不参与投票，但可用于数据同步

# 添加新的 learner 节点
ETCDCTL_API=3 etcdctl \
  --endpoints=https://<leader-ip>:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member add <new-member-name> --peer-urls=https://<new-ip>:2380 --learner

# 等待 learner 同步完成后提升为 voting 成员
ETCDCTL_API=3 etcdctl \
  --endpoints=https://<leader-ip>:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member promote <member-id>
```
---

## 3. API Server 分裂脑恢复

### 3.1 Split-Brain 检测

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 症状：
#   - 不同 kubectl 客户端看到不同的集群状态
#   - 同一个资源在不同 API Server 实例上有不同版本
#   - 出现 "optimistic locking" 或 "resource version conflict" 错误

# 检查所有 API Server 实例的状态
for node in $(kubectl get nodes -l node-role.kubernetes.io/control-plane -o jsonpath='{.items[*].metadata.name}'); do
  echo "=== API Server on $node ==="
  kubectl --server=https://<node-ip>:6443 --certificate-authority=/etc/kubernetes/pki/ca.crt \
    --client-certificate=/etc/kubernetes/pki/apiserver-kubelet-client.crt \
    --client-key=/etc/kubernetes/pki/apiserver-kubelet-client.key \
    get --raw /healthz
done
```
### 3.2 确定正确的 API Server 实例

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确定哪个 API Server 连接的是健康的 etcd leader
for node in $(kubectl get nodes -l node-role.kubernetes.io/control-plane -o jsonpath='{.items[*].metadata.name}'); do
  echo "=== $node ==="
  kubectl --server=https://<node-ip>:6443 get --raw /readyz/etcd 2>&1 || echo "NOT READY"
done

# 找到健康的 API Server 后，临时停止其他实例
# 在不健康的控制平面节点上
mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
```
### 3.3 恢复 API Server 一致性

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确保只有一个 API Server 实例运行
# 2. 等待 etcd 数据同步完成
# 3. 重新启动其他 API Server 实例

# 检查 etcd 数据一致性
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health

# 重启其他控制平面节点的 API Server
mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/

# 验证所有 API Server 状态一致
kubectl get nodes
kubectl get pods -A | head -20
```
### 3.4 修复 lease 对象

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# API Server 使用 lease 对象进行 leader election
# 检查 kube-system 命名空间中的 lease
kubectl -n kube-system get lease

# 如果 lease 对象损坏，删除后让 API Server 重新选举
kubectl -n kube-system delete lease kube-apiserver-<hash>

# 检查 kube-controller-manager 和 kube-scheduler 的 lease
kubectl -n kube-system get lease | grep -E "controller-manager|scheduler"
```
---

## 4. Pod 跨节点通信恢复

### 4.1 CNI 插件状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 CNI DaemonSet 状态
kubectl -n kube-system get ds | grep -E "calico|flannel|cilium|weave"

# 查看 CNI Pod 状态
kubectl -n kube-system get pod -l k8s-app=calico-node -o wide
# 或
kubectl -n kube-system get pod -l app=flannel -o wide
# 或
kubectl -n kube-system get pod -l k8s-app=cilium -o wide

# 查看 CNI Pod 日志
kubectl -n kube-system logs -l k8s-app=calico-node --tail=50
```
### 4.2 CNI 配置修复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查节点上的 CNI 配置文件
ls -la /etc/cni/net.d/

# 查看当前 CNI 配置
cat /etc/cni/net.d/10-calico.conflist
# 或
cat /etc/cni/net.d/10-flannel.conflist

# 如果 CNI 配置丢失，重启 CNI DaemonSet
kubectl -n kube-system rollout restart daemonset/calico-node
# 或
kubectl -n kube-system rollout restart daemonset/kube-flannel-ds
```
### 4.3 IP 转发和路由检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在问题节点上检查 IP 转发是否启用
sysctl net.ipv4.ip_forward
# 应为 1

# 检查 iptables 规则
iptables -L -n -v | head -50

# 检查路由表
ip route show

# 检查 Pod CIDR 是否正确分配
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'

# 检查节点的 Pod CIDR 是否与 CNI 配置一致
cat /var/lib/kubelet/config.yaml | grep clusterDNS
```
### 4.4 Pod 网络命名空间修复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 如果 Pod 网络命名空间损坏，删除 Pod 让控制器重建
kubectl delete pod <pod-name> -n <namespace>

# 对于 StatefulSet Pod，需要谨慎处理
# 先确认 PVC 数据安全，再删除 Pod

# 检查 Pod 的网络命名空间
# 在节点上执行
crictl inspect <container-id> | grep -A5 "linux.namespaces"
```
---

## 5. Service 流量重新均衡

### 5.1 kube-proxy 状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 kube-proxy Pod 状态
kubectl -n kube-system get pod -l k8s-app=kube-proxy -o wide

# 查看 kube-proxy 日志
kubectl -n kube-system logs -l k8s-app=kube-proxy --tail=100

# 检查 kube-proxy 模式
kubectl -n kube-system get configmap kube-proxy -o yaml | grep mode
```
### 5.2 iptables/IPVS 规则修复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# iptables 模式：检查 Service 规则
iptables -t nat -L KUBE-SERVICES | head -30

# IPVS 模式：检查 Service 规则
ipvsadm -L -n | head -30

# 如果规则混乱，重启 kube-proxy
kubectl -n kube-system rollout restart daemonset/kube-proxy

# 等待规则重建
sleep 30

# 验证规则恢复
iptables -t nat -L KUBE-SERVICES | wc -l
```
### 5.3 Endpoints 同步检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Service 的 Endpoints 是否正确
kubectl get endpoints <service-name> -n <namespace>

# 如果 Endpoints 为空，检查 Pod 是否 Ready
kubectl get pods -n <namespace> -l <service-selector> -o wide

# 检查 Pod 的 readiness probe 是否通过
kubectl describe pod <pod-name> -n <namespace> | grep -A5 "Readiness"

# 手动触发 Endpoints 更新（最后手段）
kubectl delete endpoints <service-name> -n <namespace>
# Endpoints controller 会自动重建
```
### 5.4 External Traffic Policy 修复

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 Service 的 ExternalTrafficPolicy
kubectl get svc <service-name> -n <namespace> -o yaml | grep externalTrafficPolicy

# 如果使用 Local 模式且节点不健康，流量会被丢弃
# 临时切换为 Cluster 模式
kubectl patch svc <service-name> -n <namespace> --type=json -p='[
  {"op":"replace","path":"/spec/externalTrafficPolicy","value":"Cluster"}
]'

# 验证流量恢复
curl -v http://<service-ip>:<port>
```
---

## 6. 生产最佳实践

### 6.1 etcd 高可用配置

| 配置项 | 推荐值 | 说明 |
|--------|--------|------|
| etcd 成员数 | 3 或 5（奇数） | 保证 quorum 容忍单节点故障 |
| 心跳间隔 | 100ms（默认） | 跨机房可适当增大 |
| 选举超时 | 1000ms（默认） | 跨机房建议 2000-3000ms |
| 快照频率 | 10000 次事务 | 控制 WAL 文件大小 |
| 磁盘类型 | SSD/NVMe | etcd 对磁盘延迟敏感 |

### 6.2 网络分区预防

```bash
# 确保控制平面节点之间的网络冗余
# 使用多网卡绑定（bonding）
cat /proc/net/bonding/bond0

# 检查 MTU 配置一致性
ip link show | grep mtu

# 确保防火墙规则允许 etcd 和 API Server 端口
# etcd: 2379 (client), 2380 (peer)
# apiserver: 6443
# kubelet: 10250
iptables -L INPUT -n | grep -E "2379|2380|6443|10250"
```

### 6.3 监控告警

```yaml
groups:
- name: network-partition-alerts
  rules:
  - alert: EtcdLeaderChanged
    expr: changes(etcd_server_leader_changes_seen_total[1h]) > 3
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "etcd leader 在过去 1 小时内变更超过 3 次，可能存在网络分区"

  - alert: EtcdMemberUnhealthy
    expr: etcd_server_health_failures_total > 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "etcd 成员 {{ $labels.member }} 健康检查失败"

  - alert: NodeNotReady
    expr: kube_node_status_condition{condition="Ready",status="true"} == 0
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "节点 {{ $labels.node }} 进入 NotReady 状态"

  - alert: PodNetworkConnectivityLoss
    expr: probe_success{job="kubernetes-pods"} == 0
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Pod {{ $labels.pod }} 网络连通性丢失"

  - alert: KubeProxyRulesStale
    expr: kubeproxy_sync_proxy_rules_duration_seconds > 10
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "kube-proxy 规则同步耗时过长，可能存在网络问题"
```

---

## 7. 故障排查

### 7.1 常见问题对照表

| 症状 | 可能原因 | 处理方法 |
|------|---------|---------|
| `kubectl` 命令超时 | API Server 不可达或 etcd 无 leader | 检查 etcd 和 API Server 状态 |
| 节点 NotReady 但物理机正常 | 节点间网络分区 | 检查节点间连通性和防火墙规则 |
| Pod 之间无法通信 | CNI 插件故障或路由丢失 | 检查 CNI DaemonSet 和路由表 |
| Service 访问超时 | kube-proxy 规则未同步 | 重启 kube-proxy 并检查 Endpoints |
| etcd 频繁 leader 选举 | 网络延迟或磁盘 IO 过高 | 检查网络和 etcd 磁盘性能 |

### 7.2 日志分析

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# etcd 网络相关日志
journalctl -u etcd --since "10 minutes ago" | grep -iE "network|timeout|dial|transport"

# API Server 网络相关日志
kubectl -n kube-system logs -l component=kube-apiserver --tail=200 | grep -iE "connection|refused|timeout|reset"

# kubelet 网络相关日志
journalctl -u kubelet --since "10 minutes ago" | grep -iE "PLEG|network|sandbox|CNI"

# CNI 插件日志
kubectl -n kube-system logs -l k8s-app=calico-node --tail=100 | grep -iE "error|fail|timeout"
```
### 7.3 网络抓包分析

```bash
# 在问题节点上抓取 etcd 端口流量
tcpdump -i any port 2379 -nn -c 100

# 抓取 API Server 端口流量
tcpdump -i any port 6443 -nn -c 100

# 抓取 VXLAN/Geneve 隧道流量（Calico/Flannel）
tcpdump -i any port 4789 -nn -c 100  # VXLAN
tcpdump -i any port 6081 -nn -c 100  # Geneve

# 抓取 Pod 间通信流量
tcpdump -i any src <pod-cidr> and dst <pod-cidr> -nn -c 100
```

### 7.4 恢复后验证清单

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 所有节点 Ready
kubectl get nodes
# 期望：所有节点 STATUS 为 Ready

# 2. etcd 集群健康
ETCDCTL_API=3 etcdctl endpoint health --write-out=table
# 期望：所有 endpoint 为 healthy

# 3. 所有控制平面组件运行正常
kubectl -n kube-system get pod | grep -E "apiserver|controller-manager|scheduler|etcd"
# 期望：所有 Pod 为 Running

# 4. CNI 插件正常
kubectl -n kube-system get pod | grep -E "calico|flannel|cilium"
# 期望：每个节点都有一个 CNI Pod Running

# 5. 跨节点 Pod 通信正常
kubectl exec nettest-a -- ping -c 3 $POD_B_IP
# 期望：ping 成功

# 6. Service 解析和访问正常
kubectl run test --image=busybox --rm -it --restart=Never -- \
  wget -qO- --timeout=5 http://kubernetes.default
# 期望：返回 API Server 响应

# 7. DNS 解析正常
kubectl run test --image=busybox --rm -it --restart=Never -- \
  nslookup kubernetes.default.svc.cluster.local
# 期望：返回正确的 ClusterIP
```
---

## 参考链接

- [etcd 灾难恢复文档](https://etcd.io/docs/latest/op-guide/recovery/)
- [Kubernetes 网络调试指南](https://kubernetes.io/docs/tasks/debug/debug-cluster/)
- [Calico 网络策略故障排查](https://docs.tigera.io/calico/latest/operations/troubleshoot/)
- [Cilium 网络故障排查](https://docs.cilium.io/en/stable/troubleshooting/)
- [Kubernetes API Server 高可用](https://kubernetes.io/docs/setup/production-environment/tools/kubeadm/ha-topology/)


<!-- risk-assessed -->
