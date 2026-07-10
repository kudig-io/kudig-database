---
title: Flannel 多集群场景与子网冲突处理
description: Flannel 多集群组网、子网冲突检测与处理、etcd 脏数据清理的完整指南
summary: Flannel 多集群组网、子网冲突检测与处理、etcd 脏数据清理的完整指南
category: networking
tags:
- k8s
- networking
- flannel
- multi-cluster
- etcd
- subnet
- conflict
- kubelet
- controller-manager
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- Flannel 多集群
- 子网冲突
- etcd 清理
trigger_keywords:
- Flannel
- multi-cluster
- 子网冲突
- etcd
prerequisites:
- kubectl-basics
- networking-basics
- etcd-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
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
  path: ../故障诊断/topic-fta/list/flannel-fta.md
  label: '故障树: flannel'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Flannel 多集群场景与子网冲突处理

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25+ | Flannel v0.20+ | **最后更新**: 2026-05

---

<!-- chunk: 1. 概述 -->
## 1. 概述

多集群场景下使用 Flannel 可能遇到子网冲突、[[etcd|etcd]] 脏数据等问题。本文档涵盖常见场景及处理方案。

### 1.1 常见多集群场景

| 场景 | 描述 | 风险 |
|:-----|:-----|:----:|
| 共享 etcd | 多集群共用同一个 etcd | 子网冲突 |
| 独立 etcd | 每集群独立 etcd | 低风险 |
| 集群迁移 | Pod CIDR 变更 | 路由残留 |
| 集群合并 | 两个集群合并 | 严重冲突 |

---

<!-- chunk: 2. 子网冲突检测 -->
## 2. 子网冲突检测

### 2.1 症状识别

```
# Flannel 日志中出现以下错误
subnet collision detected: 10.244.1.0/24 already allocated

# 或
failed to allocate subnet: lease already exists for subnet
```

### 2.2 检测方法

#### 2.2.1 检查 etcd 中的子网记录

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 etcd 后端时
ETCDCTL_API=3 etcdctl \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  get /coreos.com/network/subnets --prefix

# 查看所有子网分配
# 格式：/coreos.com/network/subnets/<subnet-cidr>
```
#### 2.2.2 使用 flannel kubectl 插件

```bash
# 安装 flannelctl
curl -s -L https://github.com/flannel-io/flannel/releases/latest/download/flannelctl-linux-amd64 -o /usr/local/bin/flannelctl
chmod +x /usr/local/bin/flannelctl

# 查看子网分配
flannelctl ipam show

# 查看所有节点子网
flannelctl subnet list
```

#### 2.2.3 检测跨集群冲突

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# check-subnet-conflict.sh

ETCD_HOST=${1:-127.0.0.1:2379}
SUBNET_PREFIX="10.244"

echo "=== 检查 etcd 中的子网分配 ==="

SUBNETS=$(ETCDCTL_API=3 etcdctl --endpoints=$ETCD_HOST \
  get /coreos.com/network/subnets --prefix \
  --keys-only 2>/dev/null)

declare -A COUNT
for subnet in $SUBNETS; do
  # 提取 /24 子网
  base=$(echo $subnet | cut -d'/' -f1 | cut -d'.' -f1-3)
  ((COUNT[$base]++))
done

echo "子网分布统计："
for base in "${!COUNT[@]}"; do
  count=${COUNT[$base]}
  if [ $count -gt 1 ]; then
    echo "  ⚠️  $base.x/24 出现 $count 次 - 冲突!"
  else
    echo "  ✓ $base.x/24 正常"
  fi
done
```
---

<!-- chunk: 3. etcd 脏数据清理 -->
## 3. etcd 脏数据清理

### 3.1 识别脏数据

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有子网记录
ETCDCTL_API=3 etcdctl --endpoints=$ETCD_HOST \
  get /coreos.com/network/subnets --prefix

# 识别孤立子网（节点已删除但记录残留）
# 检查节点是否仍存在
kubectl get nodes

# 对比 etcd 中的子网与实际节点
```
### 3.2 清理孤立子网

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# cleanup-orphan-subnets.sh

ETCD_HOST=${1:-127.0.0.1:2379}
STALE_SUBNETS=(
  "/coreos.com/network/subnets/10.244.5.0-24"  # 已删除节点的子网
)

for subnet in "${STALE_SUBNETS[@]}"; do
  echo "删除孤立子网: $subnet"
  ETCDCTL_API=3 etcdctl \
    --endpoints=$ETCD_HOST \
    del "$subnet"
done
```
### 3.3 完全重置子网分配

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大

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
# reset-all-subnets.sh
# ⚠️ 警告：此操作会导致所有 Pod 网络中断，仅在紧急情况下使用

ETCD_HOST=${1:-127.0.0.1:2379}
read -p "确认删除所有子网? (yes/no): " confirm

if [ "$confirm" != "yes" ]; then
  echo "操作取消"
  exit 1
fi

echo "删除所有子网..."
ETCDCTL_API=3 etcdctl --endpoints=$ETCD_HOST \
  del /coreos.com/network/subnets --prefix

echo "删除网络配置..."
ETCDCTL_API=3 etcdctl --endpoints=$ETCD_HOST \
  del /coreos.com/network/config

echo "重启所有 flannel Pod..."
kubectl delete pod -n kube-system -l app=flannel --all  # ⚠️ 批量删除，波及面大

echo "完成，等待子网重新分配..."
sleep 30
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}'
```
---

<!-- chunk: 4. 多集群 etcd 配置最佳实践 -->
## 4. 多集群 etcd 配置最佳实践

### 4.1 独立 etcd 集群（推荐）

每个 Kubernetes 集群使用独立的 etcd 实例：

```yaml
# 集群 A
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-flannel-cfg
  namespace: kube-flannel
data:
  net-conf.json: |
    {
      "Network": "10.244.0.0/16",
      "Backend": {
        "Type": "vxlan"
      }
    }

---
# 集群 B 使用不同的 Pod CIDR
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-flannel-cfg
  namespace: kube-flannel
data:
  net-conf.json: |
    {
      "Network": "10.245.0.0/16",
      "Backend": {
        "Type": "vxlan"
      }
    }
```

### 4.2 共享 etcd 但隔离前缀

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用不同的 etcd 键前缀
# 集群 A
kubectl patch configmap -n kube-flannel kube-flannel-cfg --type merge -p \
  '{"data":{"net-conf.json":"{\"Network\":\"10.244.0.0/16\",\"Backend\":{\"Type\":\"vxlan\"},\"EtcdPrefix\":\"/cluster-a/network\"}"}}'

# 集群 B
kubectl patch configmap -n kube-flannel kube-flannel-cfg --type merge -p \
  '{"data":{"net-conf.json":"{\"Network\":\"10.245.0.0/16\",\"Backend\":{\"Type\":\"vxlan\"},\"EtcdPrefix\":\"/cluster-b/network\"}"}}'
```
### 4.3 使用 [[系统基础/知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 后端

**推荐方案**：避免使用 etcd 后端，改用 Kubernetes API 后端

```yaml
# Flannel 0.20+ 使用 --kube-subnet-mgr
args:
  - --ip-masq
  - --kube-subnet-mgr
  - --iface=eth0
```

**优势**：
- 无需直接访问 etcd
- 子网信息存储在 Kubernetes Node annotations 中
- 节点删除时自动清理

---

<!-- chunk: 5. 子网冲突预防 -->
## 5. 子网冲突预防

### 5.1 初始化集群时指定 Pod CIDR

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

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
# kubeadm 初始化时指定
kubeadm init --pod-network-cidr=10.244.0.0/16

# 如果已有集群，修改 kube-controller-manager
# 方式一：直接编辑
kubectl edit cm -n kube-system kubelet-config

# 方式二：重启 controller-manager 使配置生效
```
### 5.2 使用 PodCIDR 策略

```yaml
# Kubernetes 1.27+ 可以使用 NodeCIDRMaskSize
apiVersion: v1
kind: Node
metadata:
  name: worker-node-1
spec:
  podCIDR: 10.244.1.0/24
  podCIDRs:
    - 10.244.1.0/24
```

### 5.3 定期巡检脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# flannel-subnet-audit.sh
# 建议每日运行

LOG_FILE="/var/log/flannel-audit.log"
ALERT_EMAIL="sre@example.com"

echo "=== Flannel 子网巡检 $(date) ===" >> $LOG_FILE

# 检查子网分配
SUBNET_COUNT=$(ETCDCTL_API=3 etcdctl \
  --endpoints=$ETCD_HOST \
  get /coreos.com/network/subnets --prefix \
  --keys-only 2>/dev/null | wc -l)

NODE_COUNT=$(kubectl get nodes --no-headers 2>/dev/null | wc -l)

echo "etcd 子网记录数: $SUBNET_COUNT" >> $LOG_FILE
echo "实际节点数: $NODE_COUNT" >> $LOG_FILE

# 检测差异
DIFF=$((SUBNET_COUNT - NODE_COUNT))
if [ $DIFF -gt 5 ]; then
  echo "⚠️  警告：etcd 子网记录数 ($SUBNET_COUNT) 远超节点数 ($NODE_COUNT)" >> $LOG_FILE
  echo "可能存在孤立子网，请检查" | mail -s "[Alert] Flannel 子网异常" $ALERT_EMAIL
fi

# 检查冲突
COLLISIONS=$(ETCDCTL_API=3 etcdctl \
  --endpoints=$ETCD_HOST \
  get /coreos.com/network/subnets --prefix 2>/dev/null | grep -i collision | wc -l)

if [ $COLLISIONS -gt 0 ]; then
  echo "⚠️  检测到 $COLLISIONS 个子网冲突" >> $LOG_FILE
  echo "⚠️  子网冲突检测!" | mail -s "[Critical] Flannel 子网冲突" $ALERT_EMAIL
fi

echo "巡检完成" >> $LOG_FILE
```
---

<!-- chunk: 6. 集群迁移场景 -->
## 6. 集群迁移场景

### 6.1 Pod CIDR 变更流程

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 备份当前配置
kubectl get configmap -n kube-system kube-flannel-cfg -o yaml > flannel-config-backup.yaml

# 2. 确认新 CIDR 范围
NEW_CIDR="10.245.0.0/16"

# 3. 清理旧子网（Kubernetes API 后端）
for node in $(kubectl get nodes -o name); do
  kubectl patch $node --type json -p '[{"op": "remove", "path": "/spec/podCIDR"}]'
done

# 4. 更新 Flannel ConfigMap
kubectl patch configmap -n kube-system kube-flannel-cfg --type merge -p \
  "{\"data\":{\"net-conf.json\":\"{\\\"Network\\\":\\\"${NEW_CIDR}\\\",\\\"Backend\\\":{\\\"Type\\\":\\\"vxlan\\\"}}\"}}}"

# 5. 重启所有 flannel Pod
kubectl delete pod -n kube-system -l app=flannel

# 6. 等待子网重新分配
sleep 30

# 7. 验证
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}'
```
### 6.2 集群合并流程

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源

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
# ⚠️ 警告：集群合并是高风险操作

# 假设集群 A (10.244.0.0/16) 和集群 B (10.244.0.0/16) 需合并

# 方案：重新分配集群 B 的 CIDR
# 1. 备份集群 B
kubectl get all -A -o yaml > cluster-b-backup.yaml

# 2. 清理集群 B 的 flannel 子网
for node in $(kubectl get nodes -o name); do
  kubectl patch $node --type json -p '[{"op": "remove", "path": "/spec/podCIDR"}]'
done

# 3. 更新集群 B CIDR 为 10.245.0.0/16
kubectl patch configmap -n kube-system kube-flannel-cfg --type merge -p \
  "{\"data\":{\"net-conf.json\":\"{\\\"Network\\\":\\\"10.245.0.0/16\\\",\\\"Backend\\\":{\\\"Type\\\":\\\"vxlan\\\"}}\"}}}"

# 4. 重启集群 B 的 flannel
kubectl delete pod -n kube-system -l app=flannel

# 5. 重建所有 Pod（Pod IP 会变更）
kubectl delete pod -A --all  # ⚠️ 批量删除，波及面大
```
---

<!-- chunk: 7. 故障排查命令速查 -->
## 7. 故障排查命令速查

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
# 1. 查看 etcd 中的所有子网
ETCDCTL_API=3 etcdctl get /coreos.com/network/subnets --prefix

# 2. 检查特定节点子网
ETCDCTL_API=3 etcdctl get /coreos.com/network/subnets/10.244.1.0-24

# 3. 查看网络配置
ETCDCTL_API=3 etcdctl get /coreos.com/network/config

# 4. 删除孤立子网
ETCDCTL_API=3 etcdctl del /coreos.com/network/subnets/10.244.5.0-24

# 5. 检查 flannel 日志
kubectl logs -n kube-system -l app=flannel --since=10m | grep -iE "subnet|collision|error"

# 6. 验证节点 podCIDR
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}'

# 7. 使用 flannelctl 检查
flannelctl subnet list
```
---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 网络 KUDIG Database — Global MOC
- [[网络/README.md|Domain 03: Networking 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 76 - CNI插件深度对比
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持

## See Also

- 04b-flannel-ipv6-dual-stack
- 04c-flannel-windows-support
- 04e-flannel-command-reference
- 05-terway-advanced-guide

## Related

- [[生态参考/领域索引/flannel-index.md|Flannel 知识图谱索引]]

```

<!-- risk-assessed -->
