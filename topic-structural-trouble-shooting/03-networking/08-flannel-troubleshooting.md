---
title: Flannel 网络故障排查指南
description: '# Flannel 网络故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- etcd
- kubelet
- prometheus
- cilium
- flannel
- calico
- daemonset
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Flannel 网络故障排查指南 是什么
- 如何 Flannel 网络故障排查指南
- Flannel 网络故障排查指南 故障排查
- Flannel 网络故障排查指南 排障步骤
trigger_keywords:
- Flannel
- 网络故障排查指南
- structural
- trouble
- shooting
---


# Flannel 网络故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | Flannel v0.20+ | **最后更新**: 2026-04 | **难度**: 中级

---

## 0. 10 分钟快速诊断

1. **Flannel Pod 状态**：`kubectl get pods -n kube-system -l app=flannel`，确认所有 flannel Pod 为 Running 且运行时间较长（非频繁重启）。
2. **子网分配**：在节点上 `cat /run/flannel/subnet.env`，确认 `FLANNEL_SUBNET` 已正确分配。
3. **CNI 配置**：检查 `/etc/cni/net.d/10-flannel.conflist` 存在且格式正确。
4. **跨节点连通**：在不同节点 Pod 之间执行 `ping` 和 `curl`，确认 overlay 网络正常。
5. **VXLAN 检查**：`ip -d link show flannel.1`，确认 VTEP 和 MAC 地址正确；`bridge fdb show dev flannel.1` 查看远端节点学习状态。
6. **快速缓解**：
   - 子网未分配：删除 `/run/flannel/subnet.env` 并重启 flannel Pod，强制重新注册。
   - 跨节点不通：检查 UDP 4789（VXLAN）是否被防火墙阻断，或尝试切换为 host-gw 模式。
   - MTU 问题：将 Pod MTU 降至 1450（VXLAN 场景）。
7. **证据留存**：保存 flannel Pod 日志、`subnet.env`、节点路由表、FDB/ARP 表、`/etc/cni/net.d/` 目录内容。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 Pod IP 分配失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Pod 无 IP 地址 | `failed to read subnet.env` | flannel CNI | `kubectl describe pod` |
| CNI 插件未找到 | `failed to find plugin "flannel" in path` | kubelet | `/opt/cni/bin/` 目录检查 |
| 子网冲突 | `subnet collision detected` | flanneld | flannel Pod 日志 |
| 后端初始化失败 | `failed to initialize VXLAN backend` | flanneld | flannel Pod 日志 |
| IPAM 池耗尽 | `no IP addresses available in range set` | host-local IPAM | CNI 日志 |

#### 1.1.2 跨节点 Pod 通信失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 跨节点 Ping 不通 | `Destination Host Unreachable` | Pod 内 ping | Pod 内执行 |
| 跨节点连接超时 | `dial tcp: i/o timeout` | 应用日志 | 应用 Pod 日志 |
| VXLAN 隧道未建立 | `VTEP not found` | flanneld | `bridge fdb show` |
| 路由缺失 | `no route to host` | 应用日志 | `ip route` |
| ARP 学习失败 | `incomplete` ARP 条目 | 内核网络栈 | `ip neigh` |

#### 1.1.3 后端模式特定问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| host-gw 跨子网失败 | `Network unreachable` | 内核路由 | `ip route get` |
| UDP 模式性能差 | 网络延迟高、吞吐低 | 应用监控 | `iperf3` / `qperf` |
| 云扩展后端 API 错误 | `failed to allocate VPC route` | flanneld | 云厂商 API 日志 |
| MTU 不匹配 | `ping -M do -s 1472` 失败 | 网络测试 | Pod 内执行 |

---

## 2. 排查方法与步骤

### 2.1 Flannel 架构与后端模式

#### 2.1.1 三种核心后端对比

| 后端 | 封装方式 | 端口 | 适用场景 | 排查重点 |
|------|----------|------|----------|----------|
| **VXLAN** | UDP 封装 + VXLAN 头部 | 4789 | 默认模式，通用性最好 | VTEP、FDB、MTU |
| **host-gw** | 无封装，直接路由 | 无 | 同二层网络，性能最优 | 路由表、二层连通性 |
| **UDP** | UDP 封装（用户态） | 8285 | 已废弃，不推荐 | 性能极差，建议迁移 |

#### 2.1.2 确认当前后端模式

```bash
# 查看 Flannel ConfigMap
kubectl get configmap -n kube-system kube-flannel-cfg -o yaml

# 查看节点上的子网环境变量
cat /run/flannel/subnet.env

# 查看网络接口（VXLAN 模式下会有 flannel.1）
ip link show | grep flannel
```

**VXLAN 模式典型配置**：
```json
{
  "Network": "10.244.0.0/16",
  "Backend": {
    "Type": "vxlan",
    "VNI": 1,
    "Port": 4789
  }
}
```

**host-gw 模式典型配置**：
```json
{
  "Network": "10.244.0.0/16",
  "Backend": {
    "Type": "host-gw"
  }
}
```

### 2.2 Pod IP 分配失败排查

#### 2.2.1 排查逻辑决策树

```
Pod 处于 ContainerCreating，无 IP 地址
    │
    ├─ 1. 检查 flannel Pod 状态
    │       ├─ flannel Pod 未 Running → 排查 DaemonSet
    │       └─ flannel Pod Running → 进入 2
    │
    ├─ 2. 检查 CNI 配置
    │       ├─ /etc/cni/net.d/ 无 flannel 配置 → CNI 初始化失败
    │       ├─ /opt/cni/bin/ 无 flannel 二进制 → 插件缺失
    │       └─ 配置正常 → 进入 3
    │
    ├─ 3. 检查子网分配
    │       ├─ /run/flannel/subnet.env 不存在 → flanneld 未正确注册子网
    │       ├─ 子网与其他节点冲突 → 子网分配冲突
    │       └─ 子网正常 → 进入 4
    │
    └─ 4. 检查 IPAM
            ├─ host-local IPAM 池耗尽 → 大量 Pod 创建/删除
            └─ 其他错误 → 查看 CNI 日志
```

#### 2.2.2 子网分配失败

```bash
# 查看所有节点的子网分配
kubectl get nodes -o json | jq -r '.items[] | 
  "\(.metadata.name): \(.spec.podCIDR // "未分配")"'

# 查看 flannel 子网分配记录
kubectl logs -n kube-system -l app=flannel | grep -i "subnet\|lease"

# 检查节点上的子网环境文件
cat /run/flannel/subnet.env
# 预期输出：
# FLANNEL_NETWORK=10.244.0.0/16
# FLANNEL_SUBNET=10.244.1.1/24
# FLANNEL_MTU=1450
# FLANNEL_IPMASQ=true
```

**子网冲突原因**：
- 使用 etcd 后端时，etcd 中存在脏数据（旧节点子网未清理）
- 手动修改了 `spec.podCIDR` 导致与 flannel 分配逻辑冲突
- 多个集群使用相同的 Pod CIDR 且共享 etcd

**修复**：
```bash
# 方法 1：清理 etcd 中的旧子网记录（使用 etcd 后端时）
ETCDCTL_API=3 etcdctl --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  del /coreos.com/network/subnets/ --prefix

# 重启所有 flannel Pod
kubectl delete pod -n kube-system -l app=flannel

# 方法 2：使用 Kubernetes API 后端时，直接重置节点 podCIDR
kubectl patch node <node-name> --type json -p '[{"op": "remove", "path": "/spec/podCIDR"}]'
```

#### 2.2.3 CNI 插件缺失

```bash
# 检查 CNI 二进制是否存在
ls -la /opt/cni/bin/ | grep flannel

# 检查 CNI 配置文件
ls -la /etc/cni/net.d/ | grep flannel

# 查看 CNI 配置内容
cat /etc/cni/net.d/10-flannel.conflist
```

**预期配置**：
```json
{
  "name": "cbr0",
  "cniVersion": "0.3.1",
  "plugins": [
    {
      "type": "flannel",
      "delegate": {
        "hairpinMode": true,
        "isDefaultGateway": true
      }
    },
    {
      "type": "portmap",
      "capabilities": {"portMappings": true}
    }
  ]
}
```

### 2.3 跨节点通信失败排查

#### 2.3.1 VXLAN 模式排查

```bash
# 1. 确认 VXLAN 接口存在且状态 UP
ip -d link show flannel.1
# 预期输出包含：vxlan id 1 local <node-ip> dev eth0 port 4789

# 2. 检查 VTEP MAC 地址
ip link show flannel.1 | grep link/ether

# 3. 检查 FDB（转发数据库）是否学习到了远端节点
bridge fdb show dev flannel.1
# 预期输出：
# <remote-vtep-mac> dev flannel.1 dst <remote-node-ip> self permanent

# 4. 检查路由表
ip route | grep flannel
# 预期输出：
# 10.244.2.0/24 via 10.244.2.0 dev flannel.1 onlink

# 5. 检查 ARP 表
ip neigh | grep flannel

# 6. 抓包验证 VXLAN 封装
tcpdump -i eth0 udp port 4789 -nn -e
```

**FDB 学习失败原因**：
- 节点间 UDP 4789 端口被防火墙阻断
- flanneld 未正确广播子网信息
- VTEP MAC 冲突

**修复 FDB**：
```bash
# 手动添加 FDB 条目（临时修复）
bridge fdb add <remote-vtep-mac> dev flannel.1 dst <remote-node-ip>

# 或重启 flannel 强制重新学习
kubectl delete pod -n kube-system -l app=flannel
```

#### 2.3.2 host-gw 模式排查

```bash
# 1. 确认路由已添加
ip route | grep "10.244"
# 预期输出：
# 10.244.2.0/24 via <remote-node-ip> dev eth0

# 2. 确认二层连通性
ping -c 3 <remote-node-ip>

# 3. 检查目标节点是否可达
ip route get <remote-node-ip>

# 4. 检查是否有iptables规则阻断
iptables -L -n -v | grep DROP
```

**host-gw 关键要求**：
- 所有节点必须在同一二层网络（同 VPC/子网）
- 如果节点跨子网，host-gw 模式无法工作，需切换到 VXLAN
- 云厂商环境中，需确保虚拟路由表允许节点间直接通信

#### 2.3.3 MTU 问题排查

```bash
# 测试大包连通性
kubectl exec -it <pod-a> -- ping -M do -s 1472 <pod-b-ip>
# 如果失败，说明存在 MTU 问题

# 查看当前 MTU 配置
cat /run/flannel/subnet.env | grep MTU
ip link show flannel.1 | grep mtu

# 计算正确 MTU
# 物理网卡 MTU - VXLAN 头部(50) = 推荐 Pod MTU
# 如 eth0 MTU = 1500，则 Pod MTU 应为 1450
```

**修复 MTU**：
```bash
# 修改 Flannel ConfigMap
kubectl edit configmap -n kube-system kube-flannel-cfg
# 在 net-conf.json 中添加："Backend": {"Type": "vxlan", "VNI": 1, "Port": 4789}
# 无需显式设置 MTU，flannel 会自动计算

# 如果手动设置了不合适的 MTU，删除后重启
kubectl patch configmap -n kube-system kube-flannel-cfg --type merge -p \
  '{"data":{"net-conf.json":"{\\"Network\\":\\"10.244.0.0/16\\",\\"Backend\\":{\\"Type\\":\\"vxlan\\"}}"}}'
kubectl rollout restart ds/kube-flannel-ds -n kube-system
```

### 2.4 后端数据存储问题

Flannel 支持两种数据存储后端：

#### 2.4.1 etcd 后端（旧版默认）

```bash
# 查看 etcd 中的子网分配
ETCDCTL_API=3 etcdctl --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  get /coreos.com/network/subnets --prefix

# 常见问题：etcd 中残留已删除节点的子网记录
# 清理方法：
etcdctl del /coreos.com/network/subnets/<old-subnet> \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```

**已知问题**：
- etcd 数据损坏或不可用时，flannel 无法分配新子网
- 多集群共享 etcd 时，子网可能冲突
- etcd 性能下降时，flannel 子网注册延迟增加

#### 2.4.2 Kubernetes API 后端（推荐）

```bash
# 查看节点 podCIDR 分配
kubectl get nodes -o json | jq -r '.items[] | 
  "\(.metadata.name): \(.spec.podCIDR)"'

# 查看 flannel 使用 Kubernetes API 的配置
kubectl get ds -n kube-system kube-flannel-ds -o yaml | grep -A 5 "args:"
# 应包含：- --kube-subnet-mgr
```

**优势**：
- 无需直接访问 etcd，权限更简单
- 与 Kubernetes 节点生命周期绑定，节点删除时自动清理子网
- 更易于在托管 Kubernetes 环境中使用

### 2.5 与 NetworkPolicy 的兼容性

**重要**：纯 Flannel 不支持 NetworkPolicy。如果需要网络策略，有两种方案：

1. **Canal** = Flannel + Calico（使用 Calico 处理策略，Flannel 处理网络）
2. **迁移到 Calico 或 Cilium**

```bash
# 检查是否使用 Canal
kubectl get pods -n kube-system | grep -E "flannel|calico"

# 如果同时存在 flannel 和 calico-node，说明是 Canal 方案
# 此时 NetworkPolicy 由 Calico 处理，排查需参考 Calico 文档
```

---

## 3. 解决方案与风险控制

### 3.1 后端模式切换

#### VXLAN → host-gw（同二层网络）

```bash
# 1. 备份当前配置
kubectl get configmap -n kube-system kube-flannel-cfg -o yaml > flannel-config-backup.yaml

# 2. 修改 ConfigMap
kubectl edit configmap -n kube-system kube-flannel-cfg
# 修改 net-conf.json：
# {"Network": "10.244.0.0/16", "Backend": {"Type": "host-gw"}}

# 3. 滚动重启 flannel
kubectl rollout restart ds/kube-flannel-ds -n kube-system

# 4. 验证
watch kubectl get pods -n kube-system -l app=flannel
```

**风险**：
- 切换期间会有短暂网络中断（Pod 间通信可能受影响 10-30 秒）
- host-gw 要求所有节点同二层，跨子网节点将失去网络连通性
- **回滚**：恢复备份的 ConfigMap 并重启 DaemonSet

### 3.2 子网分配冲突修复

```bash
# 方法 1：重置所有子网（Kubernetes API 后端）
# 注意：此操作会导致所有 Pod 网络中断，需谨慎

# 删除所有节点的 podCIDR
for node in $(kubectl get nodes -o name); do
  kubectl patch $node --type json -p '[{"op": "remove", "path": "/spec/podCIDR"}]'
done

# 重启 flannel，强制重新分配
kubectl delete pod -n kube-system -l app=flannel

# 方法 2：仅修复冲突节点
kubectl patch node <conflict-node> --type json -p '[{"op": "remove", "path": "/spec/podCIDR"}]'
kubectl delete pod -n kube-system -l app=flannel --field-selector spec.nodeName=<conflict-node>
```

### 3.3 CNI 配置恢复

```bash
# 如果 CNI 配置被误删除，从 ConfigMap 恢复
kubectl get configmap -n kube-system kube-flannel-cfg -o jsonpath='{.data.cni-conf\.json}' | \
  tee /etc/cni/net.d/10-flannel.conflist

# 确保 CNI 二进制存在
ls /opt/cni/bin/flannel || {
  echo "CNI 二进制缺失，需重新部署 Flannel"
  kubectl apply -f https://github.com/flannel-io/flannel/releases/latest/download/kube-flannel.yml
}
```

### 3.4 防火墙与端口

```bash
# VXLAN 模式需要放通 UDP 4789
iptables -A INPUT -p udp --dport 4789 -j ACCEPT
iptables -A INPUT -p udp --dport 8472 -j ACCEPT  # host-gw 模式下部分内核使用

# 如果使用 IPIP 模式（Calico 常见，Flannel 较少）
iptables -A INPUT -p 4 -j ACCEPT

# 保存规则
iptables-save > /etc/iptables/rules.v4  # Debian/Ubuntu
service iptables save  # CentOS/RHEL
```

---

## 4. 预防与最佳实践

### 4.1 监控告警配置

```yaml
# PrometheusRule: Flannel 关键指标告警
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: flannel-alerts
  namespace: monitoring
spec:
  groups:
    - name: flannel
      rules:
        - alert: FlannelPodNotRunning
          expr: |
            kube_pod_status_phase{namespace="kube-system",pod=~"kube-flannel.*",phase!="Running"} == 1
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "Flannel Pod 未正常运行"
            description: "节点 {{ $labels.node }} 上的 Flannel Pod 状态异常"

        - alert: FlannelSubnetNotAllocated
          expr: |
            kube_node_spec_pod_cidr == ""
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "节点未分配 Pod CIDR"
            description: "节点 {{ $labels.node }} 未分配 Flannel 子网"

        - alert: FlannelVXLANErrors
          expr: |
            rate(node_network_receive_errs_total{device="flannel.1"}[5m]) > 0.1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "Flannel VXLAN 接口错误率高"
            description: "flannel.1 接口错误率 {{ $value }}/s"
```

### 4.2 日常巡检清单

- [ ] **Flannel Pod 健康**：所有节点上的 flannel Pod 均为 Running
- [ ] **子网分配**：所有节点 `spec.podCIDR` 已正确分配且无冲突
- [ ] **CNI 配置**：`/etc/cni/net.d/` 包含有效的 flannel 配置
- [ ] **跨节点连通**：定期在不同节点 Pod 间执行连通性测试
- [ ] **MTU 配置**：Pod MTU 已根据封装类型正确设置
- [ ] **防火墙规则**：VXLAN 端口（4789）未被阻断
- [ ] **后端选择**：确认当前后端模式适合网络拓扑（VXLAN 通用，host-gw 需同二层）

### 4.3 容量规划建议

| 指标 | 警戒线 | 建议 |
|------|--------|------|
| 单节点 Pod 密度 | 110 个/节点 | Flannel 默认 /24 子网支持 254 个 IP，但需预留系统 Pod 空间 |
| 集群节点数 | 500+ | VXLAN 模式下 FDB 表规模可控；更大规模建议评估 Cilium |
| Pod 创建速率 | 100/分钟 | 大量 Pod 同时创建可能导致 IPAM 竞争，建议平滑扩容 |

### 4.4 自动化诊断脚本

```bash
#!/bin/bash
# flannel-health-check.sh - Flannel 健康检查脚本

FAILED=0
NAMESPACE="kube-system"

echo "=== Flannel 健康检查 ==="

# 1. 检查 Flannel Pod 状态
echo "[1/6] 检查 Flannel Pod 状态..."
NOT_RUNNING=$(kubectl get pods -n $NAMESPACE -l app=flannel -o json | \
  jq -r '.items[] | select(.status.phase != "Running") | .metadata.name')
if [ -n "$NOT_RUNNING" ]; then
  echo "  ✗ 异常 Pod: $NOT_RUNNING"
  FAILED=1
else
  echo "  ✓ 所有 Flannel Pod 运行正常"
fi

# 2. 检查节点子网分配
echo "[2/6] 检查节点子网分配..."
NO_CIDR=$(kubectl get nodes -o json | \
  jq -r '.items[] | select(.spec.podCIDR == null or .spec.podCIDR == "") | .metadata.name')
if [ -n "$NO_CIDR" ]; then
  echo "  ✗ 未分配子网的节点: $NO_CIDR"
  FAILED=1
else
  echo "  ✓ 所有节点已分配 Pod CIDR"
fi

# 3. 检查 CNI 配置
echo "[3/6] 检查 CNI 配置..."
for node in $(kubectl get nodes -o name | cut -d/ -f2); do
  # 使用 debug pod 或节点 exec 检查
  CONFIG=$(kubectl debug node/$node -it --image=alpine -- sh -c \
    "test -f /host/etc/cni/net.d/10-flannel.conflist && echo 'EXISTS' || echo 'MISSING'" 2>/dev/null)
  if [ "$CONFIG" != "EXISTS" ]; then
    echo "  ✗ 节点 $node CNI 配置缺失"
    FAILED=1
  fi
done
if [ $FAILED -eq 0 ]; then
  echo "  ✓ CNI 配置检查完成"
fi

# 4. 检查跨节点连通性（简化版）
echo "[4/6] 检查跨节点连通性..."
# 获取两个不同节点的 Pod
POD_A=$(kubectl get pods --all-namespaces -o wide --field-selector status.phase=Running | \
  awk 'NR==2{print $1"/"$2" "$7}')
POD_B=$(kubectl get pods --all-namespaces -o wide --field-selector status.phase=Running | \
  awk 'NR==3{print $1"/"$2" "$7}')
if [ -n "$POD_A" ] && [ -n "$POD_B" ]; then
  NODE_A=$(echo $POD_A | awk '{print $2}')
  NODE_B=$(echo $POD_B | awk '{print $2}')
  if [ "$NODE_A" != "$NODE_B" ]; then
    IP_A=$(echo $POD_A | awk '{print $1}')
    IP_B=$(echo $POD_B | awk '{print $1}')
    # 实际执行 ping 测试（需要目标 Pod 支持 ping）
    echo "  ℹ 节点 $NODE_A 和 $NODE_B 上存在 Pod，建议手动执行跨节点 ping 测试"
  fi
fi

# 5. 检查 VXLAN 接口（在 VXLAN 模式下）
echo "[5/6] 检查 VXLAN 接口..."
FLANNEL_IFACE=$(kubectl get pods -n $NAMESPACE -l app=flannel -o json | \
  jq -r '.items[0].spec.nodeName')
VXLAN_CHECK=$(kubectl debug node/$FLANNEL_IFACE -it --image=alpine -- sh -c \
  "ip link show flannel.1 2>/dev/null | grep -c 'state UP'" 2>/dev/null)
if [ "$VXLAN_CHECK" == "1" ]; then
  echo "  ✓ VXLAN 接口状态正常"
else
  echo "  ⚠ 未检测到 VXLAN 接口（可能使用 host-gw 模式）"
fi

# 6. 检查最近错误日志
echo "[6/6] 检查错误日志..."
ERRORS=$(kubectl logs -n $NAMESPACE -l app=flannel --since=10m 2>/dev/null | \
  grep -icE "error|fail|unable|collision")
if [ "$ERRORS" -gt 0 ]; then
  echo "  ✗ 最近 10 分钟发现 $ERRORS 条错误日志"
  FAILED=1
else
  echo "  ✓ 最近 10 分钟无错误日志"
fi

echo ""
if [ $FAILED -eq 1 ]; then
  echo "检查结果: 存在异常，请进一步排查"
  exit 1
else
  echo "检查结果: 健康"
  exit 0
fi
```

---

## 附录 A: Flannel 与 CNI 版本兼容性

| Flannel 版本 | CNI 版本 | Kubernetes 版本 | 备注 |
|--------------|----------|-----------------|------|
| v0.20+ | v1.0+ | v1.25+ | 推荐版本，支持 IPv6 Dual Stack |
| v0.19 | v0.9+ | v1.24+ | 稳定版本 |
| v0.15-v0.18 | v0.8+ | v1.20-v1.24 | 需升级 |
| v0.14 及以下 | v0.7 | v1.19 及以下 | 已废弃，强烈建议升级 |

## 附录 B: 常见问题速查

| 问题 | 快速判断 | 解决方案 |
|------|----------|----------|
| Pod 无 IP | `kubectl describe pod` 显示 CNI 错误 | 检查 `/run/flannel/subnet.env` 和 CNI 插件 |
| 同节点通，跨节点不通 | `ping` 同节点 OK，跨节点失败 | 检查 VXLAN（UDP 4789）或 host-gw 路由 |
| 大包不通，小包通 | `ping -M do -s 1472` 失败 | 调整 MTU 至 1450（VXLAN）或物理网卡 MTU |
| 子网冲突 | flannel 日志显示 `collision` | 清理 etcd/节点 podCIDR 后重启 flannel |
| 网络策略不生效 | 纯 Flannel 不支持策略 | 切换到 Canal（Flannel+Calico）或迁移到 Calico/Cilium |
| flanneld 频繁重启 | `kubectl get pods` 显示重启次数高 | 检查 ConfigMap 配置、etcd 连接、资源限制 |
