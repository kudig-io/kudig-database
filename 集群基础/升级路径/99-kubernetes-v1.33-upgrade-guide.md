---
title: Kubernetes v1.33 升级实操指南
description: '# Kubernetes v1.33 升级实操指南'
summary: 'kubectl version -o json | jq '.serverVersion.gitVersion''
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- etcd
- apiserver
- kubelet
- scheduler
- istio
- docker
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubernetes v1.33 升级实操指南 是什么
- 如何 Kubernetes v1.33 升级实操指南
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Kubernetes
- v1.33
- 升级实操指南
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- service-mesh-basics
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
  path: ../容器运行时/
  label: '相关知识域: 容器运行时'
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] v1.33 升级实操指南

> **适用版本**: 从 v1.32 升级到 v1.33  
> **最后更新**: 2026-04-24  
> **难度**: 高级

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [一、升级前检查清单](#一升级前检查清单)
- [二、控制平面升级](#二控制平面升级)
- [三、工作节点升级](#三工作节点升级)
- [四、升级后验证](#四升级后验证)
- [五、启用 v1.33 新特性](#五启用-v1.33-新特性)
- [六、回滚预案](#六回滚预案)
- [七、常见问题排查](#七常见问题排查)

---

<!-- chunk: 一、升级前检查清单 -->
## 一、升级前检查清单

### 1.1 版本兼容性确认

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# upgrade-check.sh

echo "=== K8s v1.33 升级前检查 ==="

# 1. 当前版本
echo "当前版本:"
kubectl version -o json | jq '.serverVersion.gitVersion'

# 2. 检查已弃用 API
echo -e "\n已弃用 API 使用:"
kubectl get --raw /metrics 2>/dev/null | grep apiserver_requested_deprecated_apis || echo "无已弃用 API"

# 3. 检查 CSI 驱动
echo -e "\nCSI 驱动:"
kubectl get csidrivers

# 4. 检查 CCM
echo -e "\n云控制器管理器:"
kubectl get pods -n kube-system | grep cloud-controller || echo "未部署 CCM"

# 5. 检查 Feature Gates
echo -e "\nFeature Gates:"
kubectl get --raw /api/v1/nodes/$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')/proxy/configz 2>/dev/null | \
  jq '.kubeletconfig.featureGates' || echo "无法获取"

# 6. 检查 Pod 使用旧 API
echo -e "\n使用 restartPolicy 的 Init 容器 (Sidecar 兼容性):"
kubectl get pods -A -o json | jq -r '
  .items[] |
  select(.spec.initContainers? // [] | map(select(.restartPolicy == "Always")) | length > 0) |
  "\(.metadata.namespace)/\(.metadata.name)"
' | head -20

# 7. etcd 版本
echo -e "\netcd 版本:"
kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].spec.containers[0].image}'

# 8. 容器运行时
echo -e "\n容器运行时:"
kubectl get nodes -o jsonpath='{.items[0].status.nodeInfo.containerRuntimeVersion}'

echo -e "\n=== 检查完成 ==="
```
### 1.2 备份

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 备份 etcd
ETCD_POD=$(kubectl get pods -n kube-system -l component=etcd -o jsonpath='{.items[0].metadata.name}')
kubectl exec -n kube-system $ETCD_POD -- etcdctl \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  snapshot save /var/lib/etcd/snapshot-pre-upgrade.db

# 导出所有资源
mkdir -p /backup/k8s-$(date +%Y%m%d)
kubectl get all --all-namespaces -o yaml > /backup/k8s-$(date +%Y%m%d)/all-resources.yaml
kubectl get cm,secret --all-namespaces -o yaml > /backup/k8s-$(date +%Y%m%d)/configs.yaml
kubectl get crd -o yaml > /backup/k8s-$(date +%Y%m%d)/crds.yaml
```
---

<!-- chunk: 二、控制平面升级 -->
## 二、控制平面升级

### 2.1 kubeadm 升级

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
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
# 1. 升级 kubeadm
apt-mark unhold kubeadm && \
apt-get update && \
apt-get install -y kubeadm=1.33.0-1.1 && \
apt-mark hold kubeadm

# 2. 验证版本
kubeadm version

# 3. 升级计划 (干跑)
kubeadm upgrade plan v1.33.0

# 4. 执行升级
kubeadm upgrade apply v1.33.0 --yes

# 5. 升级 kubelet 和 kubectl
apt-mark unhold kubelet kubectl && \
apt-get install -y kubelet=1.33.0-1.1 kubectl=1.33.0-1.1 && \
apt-mark hold kubelet kubectl

# 6. 重启 kubelet
systemctl daemon-reload
systemctl restart kubelet
```
### 2.2 高可用控制平面

```bash
# 第一个控制节点
kubeadm upgrade apply v1.33.0 --yes

# 其他控制节点
kubeadm upgrade node
```

---

<!-- chunk: 三、工作节点升级 -->
## 三、工作节点升级

### 3.1 驱逐和升级

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
#!/bin/bash
# upgrade-node.sh NODE_NAME

NODE=$1

echo "升级节点: $NODE"

# 1. 驱逐 Pod
kubectl drain $NODE --ignore-daemonsets --delete-emptydir-data --force

# 2. 升级 kubeadm
apt-mark unhold kubeadm && \
apt-get update && \
apt-get install -y kubeadm=1.33.0-1.1 && \
apt-mark hold kubeadm

# 3. 升级节点配置
kubeadm upgrade node

# 4. 升级 kubelet
apt-mark unhold kubelet kubectl && \
apt-get install -y kubelet=1.33.0-1.1 kubectl=1.33.0-1.1 && \
apt-mark hold kubelet kubectl

# 5. 重启
systemctl daemon-reload
systemctl restart kubelet

# 6. 恢复调度
kubectl uncordon $NODE

echo "节点 $NODE 升级完成"
```
### 3.2 批量升级脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# upgrade-all-workers.sh

for node in $(kubectl get nodes -l '!node-role.kubernetes.io/control-plane' -o jsonpath='{.items[*].metadata.name}'); do
  echo "========================================="
  echo "升级节点: $node"
  echo "========================================="
  
  ./upgrade-node.sh $node
  
  # 等待节点就绪
  kubectl wait --for=condition=Ready node/$node --timeout=300s
  
  echo "节点 $node 就绪"
  sleep 30
done
```
---

<!-- chunk: 四、升级后验证 -->
## 四、升级后验证

### 4.1 版本验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "=== 升级后验证 ==="

# 集群版本
kubectl version -o json | jq '.serverVersion | {gitVersion, gitCommit, buildDate}'

# 节点状态
kubectl get nodes -o wide

# Pod 状态
kubectl get pods -A | grep -v Running | grep -v Completed || echo "所有 Pod 正常"

# 核心组件
kubectl get pods -n kube-system

# API Server 健康
kubectl get --raw /healthz

# etcd 健康
kubectl exec -n kube-system etcd-$(hostname) -- etcdctl \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health
```
### 4.2 功能验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. Sidecar 容器 (v1.33 GA)
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: sidecar-test
spec:
  initContainers:
  - name: sidecar
    image: busybox
    command: ["sh", "-c", "echo Sidecar running && sleep 3600"]
    restartPolicy: Always
  containers:
  - name: app
    image: nginx
EOF

kubectl wait --for=condition=Ready pod/sidecar-test --timeout=60s
kubectl get pod sidecar-test -o jsonpath='{.status.initContainerStatuses[0].restartCount}'
# 预期: 0 (正常运行)

# 2. ValidatingAdmissionPolicy (v1.30 GA, v1.33 确认)
kubectl get validatingadmissionpolicies

# 3. DRA (如启用)
kubectl get resourceslices 2>/dev/null || echo "DRA 未启用"

# 4. 检查新 API
echo "v1.33 新增 API:"
kubectl api-versions | grep -E "v1\.33|v1alpha3" | head -10
```
---

<!-- chunk: 五、启用 v1.33 新特性 -->
## 五、启用 v1.33 新特性

### 5.1 Sidecar 容器 (已默认启用)

```yaml
# 无需 Feature Gate，v1.33 GA 默认启用
# 直接在 Pod 中使用 restartPolicy: Always
apiVersion: v1
kind: Pod
metadata:
  name: app-with-sidecar
spec:
  initContainers:
  - name: istio-init
    image: istio/proxyv2:1.24.0
    restartPolicy: Always
  containers:
  - name: app
    image: myapp:v1.0
```

### 5.2 [[系统基础/知识字典/scheduling/dynamic-resource-allocation.md|Dynamic Resource Allocation]] (需显式启用)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
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
# 1. 修改 kube-apiserver 和 kube-scheduler 的 Feature Gate
# /etc/kubernetes/manifests/kube-apiserver.yaml
# - --feature-gates=DynamicResourceAllocation=true

# 2. 修改 kubelet 配置
# /var/lib/kubelet/config.yaml
featureGates:
  DynamicResourceAllocation: true

# 3. 重启组件
systemctl restart kubelet
```
### 5.3 Scheduler Queueing Hints (Beta, 默认启用 v1.33)

```bash
# v1.33 Beta 默认启用，无需操作
# 如需禁用:
# kube-scheduler --feature-gates=SchedulerQueueingHints=false
```

### 5.4 In-Place Pod Vertical Scaling (Alpha, 实验性)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器
> - `kubectl apply/create/replace`：创建/变更集群资源
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
# 1. 启用 Feature Gate
# /var/lib/kubelet/config.yaml
featureGates:
  InPlacePodVerticalScaling: true

# 2. 重启 kubelet
systemctl restart kubelet

# 3. 创建可调整 Pod
kubectl apply -f - <<EOF
apiVersion: v1
kind: Pod
metadata:
  name: resize-test
  annotations:
    resize.policy/container.app: "RestartNotRequired"
spec:
  containers:
  - name: app
    image: nginx
    resources:
      requests:
        cpu: "100m"
        memory: "128Mi"
      limits:
        cpu: "200m"
        memory: "256Mi"
EOF

# 4. 原地调整资源
kubectl patch pod resize-test --patch '
{
  "spec": {
    "containers": [{
      "name": "app",
      "resources": {
        "requests": {"cpu": "200m", "memory": "256Mi"},
        "limits": {"cpu": "400m", "memory": "512Mi"}
      }
    }]
  }
}'
```
### 5.5 nftables kube-proxy (Beta, 实验性)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

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
# 1. 修改 kube-proxy ConfigMap
kubectl edit cm kube-proxy -n kube-system

# 修改:
# mode: "nftables"

# 2. 重启 kube-proxy
kubectl rollout restart ds kube-proxy -n kube-system
```
---

<!-- chunk: 六、回滚预案 -->
## 六、回滚预案

### 6.1 控制平面回滚

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
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
# 如果升级失败，回滚到 v1.32
kubeadm upgrade apply v1.32.x --yes

# 回滚 kubelet
apt-mark unhold kubelet kubeadm kubectl
apt-get install -y kubelet=1.32.x-1.1 kubeadm=1.32.x-1.1 kubectl=1.32.x-1.1
apt-mark hold kubelet kubeadm kubectl
systemctl restart kubelet
```
### 6.2 etcd 回滚

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用升级前快照恢复
kubectl exec -n kube-system etcd-$(hostname) -- etcdctl \
  snapshot restore /var/lib/etcd/snapshot-pre-upgrade.db \
  --data-dir=/var/lib/etcd-restored

# 修改 etcd 挂载到新数据目录
# 编辑 /etc/kubernetes/manifests/etcd.yaml
# - --data-dir=/var/lib/etcd-restored
```
---

<!-- chunk: 七、常见问题排查 -->
## 七、常见问题排查

| 问题 | 原因 | 解决 |
|:---|:---|:---|
| kubeadm upgrade 失败 | API Server 未就绪 | 检查 `docker ps` 或 `crictl ps` |
| kubelet 无法启动 | 配置不兼容 | 检查 `/var/log/syslog` |
| Pod 无法调度 | 节点 NotReady | `kubectl describe node` |
| Sidecar 不重启 | 版本 < v1.33 | 确认集群版本 |
| DRA 资源不识别 | Feature Gate 未启用 | 检查 kube-apiserver 参数 |
| etcd 健康检查失败 | 数据不一致 | 使用快照恢复 |
| CNI 插件失败 | 版本不兼容 | 升级 CNI 到最新版 |

---

<!-- chunk: 参考链接 -->
## 参考链接

- [K8s 升级指南](https://kubernetes.io/docs/tasks/administer-cluster/cluster-upgrade/)
- [kubeadm 升级](https://kubernetes.io/docs/tasks/administer-cluster/kubeadm/kubeadm-upgrade/)
- [K8s v1.33 发布说明](https://kubernetes.io/blog/2025/04/23/kubernetes-v1-33-release/)
- [Feature Gates](https://kubernetes.io/docs/reference/command-line-tools-reference/feature-gates/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 MOC
- [[集群基础/README.md|Domain-1: Kubernetes架构基础]]
- Domain-1 架构基础 — 开源项目索引
- Kubernetes 架构全景图
- Kubernetes 核心组件深度剖析
- 03 - 功能和API表
- 04 - Kubernetes 源码结构深度解析
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)

## See Also

- 99-kubernetes-v1.33-production-best-practices
- 99-kubernetes-v1.33-quick-reference-card
- 99-kubernetes-version-lifecycle-support-policy
- 01-kubernetes-architecture-overview

```

<!-- risk-assessed -->
