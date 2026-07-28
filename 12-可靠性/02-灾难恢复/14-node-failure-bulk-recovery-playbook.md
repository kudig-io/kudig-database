---
title: 批量节点宕机恢复
description: '批量节点宕机时的故障分类诊断、kubelet/容器运行时排查、节点重新加入集群全流程'
summary: '批量节点宕机时的故障分类诊断、kubelet/容器运行时排查、节点重新加入集群全流程'
category: reliability-engineering
tags:
- disaster-recovery
- k8s
- sre
- node
- kubelet
- containerd
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
- 批量节点宕机恢复 是什么
- 如何 批量节点宕机恢复
- kubelet 无法启动排查
trigger_keywords:
- node
- failure
- kubelet
- containerd
- CNI
- bulk-recovery
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


# 批量节点宕机恢复

## 概述

批量节点宕机是 Kubernetes 集群中最严重的故障场景之一。当多个节点同时不可用时，可能导致大量 Pod 驱逐、服务中断，甚至触发控制面雪崩。常见诱因包括：

- 机房网络分区或交换机故障
- 批量内核升级/OOM 导致节点重启
- 存储系统故障导致 kubelet 挂起
- 容器运行时（containerd/docker）批量崩溃

本手册提供系统化的故障分类诊断方法，覆盖 kubelet、容器运行时、CNI 插件三大故障层，并给出节点安全恢复和重新加入集群的完整流程。

### 故障分类矩阵

| 故障层 | 症状 | 影响范围 |
|--------|------|----------|
| kubelet 层 | `NotReady`，kubelet 进程退出或 OOM | 单节点或多节点 |
| 容器运行时 | `NotReady`，Pod `ContainerCreating` 卡住 | 运行时版本相同的节点 |
| CNI 插件 | Pod 网络不通，`NetworkPluginNotReady` | 使用相同 CNI 的节点 |
| 系统层 | 节点 SSH 不通，硬件故障 | 物理机/机柜级别 |

## 详细步骤

### 第一阶段：节点状态诊断

#### 1.1 快速评估集群状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有节点状态
kubectl get nodes -o wide

# 统计 NotReady 节点数量
kubectl get nodes --no-headers | grep -c "NotReady"

# 查看 NotReady 节点列表
kubectl get nodes --no-headers | grep "NotReady" | awk '{print $1, $2, $5}'

# 查看节点 Conditions 详情
kubectl describe node <node-name> | grep -A 20 "Conditions:"
```
#### 1.2 节点 Conditions 解读

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看详细的 Conditions
kubectl get node <node-name> -o jsonpath='{.status.conditions}' | jq '.'

# 关键 Conditions：
# - Ready=False → 节点不可用
# - MemoryPressure=True → 内存不足
# - DiskPressure=True → 磁盘不足
# - PIDPressure=True → PID 耗尽
# - NetworkUnavailable=True → 网络插件未就绪
```
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点事件
kubectl get events --field-selector involvedObject.name=<node-name> --sort-by='.lastTimestamp'

# 查看节点上的 Pod 状态
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> --sort-by='.status.phase'
```
#### 1.3 SSH 登录检查

```bash
# 批量检查节点 SSH 可达性
for node in node-1 node-2 node-3; do
  echo "=== $node ==="
  ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no root@$node \
    "hostname && uptime" 2>&1
done

# 批量检查系统资源
for node in node-1 node-2 node-3; do
  echo "=== $node ==="
  ssh root@$node "free -h && df -h / && uptime" 2>&1
done
```

### 第二阶段：kubelet 故障排查

#### 2.1 kubelet 进程状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 kubelet 进程是否运行
ssh root@<node> "systemctl is-active kubelet"

# 查看 kubelet 服务状态
ssh root@<node> "systemctl status kubelet"

# 查看 kubelet 日志（最近 100 行）
ssh root@<node> "journalctl -u kubelet --no-pager -n 100"

# 查看 kubelet 启动错误
ssh root@<node> "journalctl -u kubelet --since '1 hour ago' | grep -iE 'error|fatal|panic'"
```
#### 2.2 kubelet 常见启动失败原因

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 原因 1：证书过期
ssh root@<node> "kubeadm certs check-expiration"
# 解决：kubeadm certs renew all

# 原因 2：配置文件损坏
ssh root@<node> "cat /var/lib/kubelet/config.yaml"
# 解决：从备份恢复或 kubeadm init phase kubelet-config 重新生成

# 原因 3：磁盘空间不足
ssh root@<node> "df -h /var/lib/kubelet"
# 解决：清理镜像 crictl rmi --prune，清理日志

# 原因 4：PID 耗尽
ssh root@<node> "ps aux | wc -l"
ssh root@<node> "cat /proc/sys/kernel/pid_max"
# 解决：kill 僵尸进程或提高 pid_max
```
#### 2.3 kubelet 配置恢复

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
# 查看当前 kubelet 配置
ssh root@<node> "cat /var/lib/kubelet/config.yaml"

# 从控制面重新生成 kubelet 配置
# 在控制面节点执行
kubeadm init phase kubelet-config --node-name <node-name>

# 将生成的配置复制到目标节点
scp /etc/kubernetes/kubelet.conf root@<node>:/etc/kubernetes/kubelet.conf

# 重启 kubelet
ssh root@<node> "systemctl daemon-reload && systemctl restart kubelet"
```
#### 2.4 批量 kubelet 恢复脚本

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
# bulk-kubelet-recovery.sh
# 批量恢复 NotReady 节点的 kubelet

NODES=("node-1" "node-2" "node-3")

for node in "${NODES[@]}"; do
  echo "=== Processing $node ==="

  # 检查 SSH 可达
  if ! ssh -o ConnectTimeout=5 root@$node "true" 2>/dev/null; then
    echo "  SSH unreachable, skipping"
    continue
  fi

  # 检查 kubelet 状态
  STATUS=$(ssh root@$node "systemctl is-active kubelet" 2>/dev/null)
  echo "  kubelet status: $STATUS"

  if [ "$STATUS" != "active" ]; then
    echo "  Attempting kubelet restart..."
    ssh root@<node> "systemctl daemon-reload && systemctl restart kubelet"
    sleep 10
    STATUS=$(ssh root@$node "systemctl is-active kubelet" 2>/dev/null)
    echo "  kubelet status after restart: $STATUS"
  fi

  # 验证节点 Ready
  NODE_STATUS=$(kubectl get node $node --no-headers 2>/dev/null | awk '{print $2}')
  echo "  Node status: $NODE_STATUS"
  echo ""
done
```
### 第三阶段：容器运行时重置

#### 3.1 containerd 故障排查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 containerd 状态
ssh root@<node> "systemctl is-active containerd"

# 查看 containerd 日志
ssh root@<node> "journalctl -u containerd --no-pager -n 100"

# 检查 containerd 配置
ssh root@<node> "cat /etc/containerd/config.toml"

# 测试 containerd 响应
ssh root@<node> "crictl info"
```
#### 3.2 containerd 常见问题

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
# 问题 1：containerd 配置错误导致启动失败
ssh root@<node> "containerd config default > /etc/containerd/config.toml"
ssh root@<node> "systemctl restart containerd"

# 问题 2：镜像存储损坏
ssh root@<node> "crictl rmi --prune"
ssh root@<node> "systemctl restart containerd"

# 问题 3：sandbox/container 积压导致资源耗尽
ssh root@<node> "crictl pods | wc -l"
ssh root@<node> "crictl ps -a | wc -l"
# 清理已退出的容器
ssh root@<node> "crictl rm $(crictl ps -q --state exited 2>/dev/null)"
# 清理无用的 sandbox
ssh root@<node> "crictl rmp $(crictl pods -q --no-trunc 2>/dev/null | head -50)"
```
#### 3.3 containerd 完全重置

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
# ⚠️ 以下操作会清理该节点上所有容器和镜像
# 仅在 containerd 无法修复时使用

# 停止 kubelet 和 containerd
ssh root@<node> "systemctl stop kubelet containerd"

# 清理数据目录
ssh root@<node> "rm -rf /var/lib/containerd/io.containerd.grpc.v1.cri/containers/*"
ssh root@<node> "rm -rf /var/lib/containerd/io.containerd.grpc.v1.cri/sandboxes/*"
ssh root@<node> "rm -rf /var/lib/containerd/io.containerd.snapshotter.v1.overlayfs/*"

# 恢复默认配置
ssh root@<node> "containerd config default > /etc/containerd/config.toml"

# 启动服务
ssh root@<node> "systemctl start containerd && systemctl start kubelet"
```
#### 3.4 Docker 运行时排查（如使用 Docker）

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
# 检查 Docker 状态
ssh root@<node> "systemctl is-active docker"

# 查看 Docker 日志
ssh root@<node> "journalctl -u docker --no-pager -n 100"

# 清理 Docker 资源
ssh root@<node> "docker system prune -af"

# 重启 Docker
ssh root@<node> "systemctl restart docker"
```
### 第四阶段：CNI 插件重装

#### 4.1 CNI 状态检查

```bash
# 检查 CNI 配置目录
ssh root@<node> "ls -la /etc/cni/net.d/"

# 检查 CNI 二进制文件
ssh root@<node> "ls -la /opt/cni/bin/"

# 查看 CNI 相关日志
ssh root@<node> "journalctl -u kubelet | grep -i cni"

# 检查 NetworkPlugin 状态
ssh root@<node> "journalctl -u kubelet | grep -i 'network plugin'"
```

#### 4.2 Calico CNI 重装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Calico Pod 状态
kubectl get pods -n kube-system -l k8s-app=calico-node

# 重启 Calico 节点 Pod（会自动修复 CNI 配置）
kubectl delete pod -n kube-system -l k8s-app=calico-node --field-selector spec.nodeName=<node-name>

# 手动修复 CNI 配置
ssh root@<node> "cat > /etc/cni/net.d/10-calico.conflist << 'EOF'
{
  \"cniVersion\": \"0.3.1\",
  \"name\": \"k8s-pod-network\",
  \"plugins\": [
    {
      \"type\": \"calico\",
      \"log_level\": \"info\",
      \"datastore_type\": \"kubernetes\",
      \"mtu\": 1440,
      \"ipam\": {
        \"type\": \"calico-ipam"
      }
    }
  ]
}
EOF"

# 清理 CNI 网络命名空间残留
ssh root@<node> "ip link | grep cali"
# 删除异常的 veth pair
# ssh root@<node> "ip link delete <veth-name>"
```
#### 4.3 Flannel CNI 重装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Flannel Pod 状态
kubectl get pods -n kube-system -l app=flannel

# 重启 Flannel Pod
kubectl delete pod -n kube-system -l app=flannel --field-selector spec.nodeName=<node-name>

# 手动清理 Flannel 网络
ssh root@<node> "ip link delete flannel.1 2>/dev/null"
ssh root@<node> "ip link delete cni0 2>/dev/null"

# 清理残留的 iptables 规则
ssh root@<node> "iptables -F -t nat"
ssh root@<node> "iptables -F"
```
#### 4.4 CNI 完全重装

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
# 删除节点上的 CNI 配置和二进制
ssh root@<node> "rm -rf /etc/cni/net.d/*"
ssh root@<node> "rm -rf /opt/cni/bin/*"

# 重启 kubelet
ssh root@<node> "systemctl restart kubelet"

# 通过 DaemonSet 自动重装 CNI
# Calico：
kubectl rollout restart daemonset calico-node -n kube-system
# Flannel：
kubectl rollout restart daemonset kube-flannel-ds -n kube-system
```
### 第五阶段：节点重新加入集群

#### 5.1 节点清理

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
# ⚠️ 仅在节点需要完全重置时执行

# 在控制面节点删除旧节点
kubectl delete node <node-name>

# 在目标节点执行重置
ssh root@<node> "kubeadm reset -f"

# 清理残留配置
ssh root@<node> "rm -rf /etc/kubernetes/"
ssh root@<node> "rm -rf /var/lib/kubelet/"
ssh root@<node> "rm -rf /var/lib/etcd/"
ssh root@<node> "rm -rf /etc/cni/net.d/"
ssh root@<node> "iptables -F -t nat"
ssh root@<node> "iptables -F"
ssh root@<node> "ip link delete cni0 2>/dev/null"
ssh root@<node> "ip link delete flannel.1 2>/dev/null"
```
#### 5.2 重新加入集群

```bash
# 在控制面节点生成新的 join token
kubeadm token create --print-join-command

# 在目标节点执行 join
ssh root@<node> "<join-command-from-above>"

# 如果是控制面节点，需要加入 control-plane
kubeadm token create --print-join-command --control-plane
```

#### 5.3 验证节点恢复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查节点状态
kubectl get node <node-name> -o wide

# 等待节点 Ready
kubectl wait --for=condition=Ready node/<node-name> --timeout=300s

# 检查节点上的 Pod 是否正常运行
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name>

# 验证 CNI 就绪
kubectl get node <node-name> -o jsonpath='{.status.conditions[?(@.type=="NetworkUnavailable")].status}'
# 应输出 "False"
```
#### 5.4 批量恢复脚本

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
# bulk-node-recovery.sh
# 批量恢复 NotReady 节点

NODES=("node-1" "node-2" "node-3")
CONTROL_PLANE="10.0.0.1"

# 获取 join 命令
JOIN_CMD=$(ssh root@$CONTROL_PLANE "kubeadm token create --print-join-command")
echo "Join command: $JOIN_CMD"

for node in "${NODES[@]}"; do
  echo "=== Processing $node ==="

  # SSH 可达性检查
  if ! ssh -o ConnectTimeout=5 root@$node "true" 2>/dev/null; then
    echo "  SSH unreachable, skipping"
    continue
  fi

  # 尝试重启 kubelet
  echo "  Restarting kubelet..."
  ssh root@$node "systemctl daemon-reload && systemctl restart kubelet"
  sleep 15

  # 检查节点状态
  STATUS=$(kubectl get node $node --no-headers 2>/dev/null | awk '{print $2}')
  echo "  Node status: $STATUS"

  if [ "$STATUS" != "Ready" ]; then
    echo "  Node still NotReady, attempting full reset..."
    ssh root@$node "kubeadm reset -f"
    ssh root@$node "$JOIN_CMD"
    sleep 30
    STATUS=$(kubectl get node $node --no-headers 2>/dev/null | awk '{print $2}')
    echo "  Node status after rejoin: $STATUS"
  fi

  echo ""
done

# 最终状态
echo "=== Final Status ==="
kubectl get nodes -o wide
```
## 生产最佳实践

### 节点健康监控

- 监控 `kube_node_status_condition{condition="Ready",status="true"}` 为 0 超过 2 分钟告警
- 监控节点 `MemoryPressure`、`DiskPressure`、`PIDPressure` 条件
- 监控 kubelet 进程 CPU/内存使用，OOM 告警

### 自动化恢复

- 使用 Node Problem Detector（NPD）自动检测常见节点故障
- 配置自动修复策略：kubelet OOM → 自动重启，磁盘满 → 自动清理
- 考虑使用 Cluster Autoscaler 替换不可恢复的节点

### 容量规划

- 保持 20% 的节点余量，应对突发宕机
- 使用 PodDisruptionBudget（PDB）防止批量驱逐
- 关键服务使用 PodAntiAffinity 分散到不同节点

## 故障排查

### 场景 1：kubelet 启动后立即 OOM

```bash
# 检查 kubelet 内存使用
ssh root@<node> "cat /proc/$(pgrep kubelet)/status | grep VmRSS"

# 检查是否被 OOM Killer 终止
ssh root@<node> "dmesg | grep -i oom"

# 解决：增加 kubelet 内存限制或减少节点 Pod 数量
```

### 场景 2：节点反复 NotReady → Ready → NotReady

```bash
# 检查网络连通性
ssh root@<node> "ping -c 3 <api-server-ip>"

# 检查 kubelet 到 API Server 的连接
ssh root@<node> "curl -k https://<api-server-ip>:6443/healthz"

# 检查是否有网络抖动
ssh root@<node> "journalctl -u kubelet | grep 'connection refused'"
```

### 场景 3：containerd 版本升级后节点 NotReady

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
# 检查 containerd 版本
ssh root@<node> "containerd --version"

# 检查 CRI 兼容性
ssh root@<node> "crictl --version"

# 回滚 containerd
ssh root@<node> "yum downgrade containerd.io-<previous-version>"
ssh root@<node> "systemctl restart containerd kubelet"
```
### 场景 4：节点恢复后 Pod 无法调度

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查节点 taint
kubectl get node <node-name> -o jsonpath='{.spec.taints}' | jq '.'

# 检查节点 allocatable 资源
kubectl describe node <node-name> | grep -A 10 "Allocated resources"

# 验证 scheduler 可以调度到该节点
kubectl get node <node-name> -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'
```
## 参考链接

- [Kubernetes 官方文档 - 节点管理](https://kubernetes.io/docs/concepts/architecture/nodes/)
- [Kubernetes 官方文档 - 节点问题检测](https://kubernetes.io/docs/tasks/debug/debug-cluster/monitor-node-health/)
- [containerd 故障排查](https://github.com/containerd/containerd/blob/main/docs/getting-started.md)
- [kubeadm reset 文档](https://kubernetes.io/docs/reference/setup-tools/kubeadm/kubeadm-reset/)
- [Calico 故障排查](https://docs.tigera.io/calico/latest/operations/troubleshoot/)

---

*本手册适用于 Kubernetes 1.28-1.32 版本。批量恢复前建议先在单节点验证流程。*


<!-- risk-assessed -->
