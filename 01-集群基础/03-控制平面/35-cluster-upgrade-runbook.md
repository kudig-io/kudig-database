---
title: Kubernetes 集群升级生产运行手册
description: 面向生产环境的 Kubernetes 集群升级与回滚运行手册，覆盖预检、控制平面、工作节点、插件与回滚决策矩阵
summary: 面向生产环境的 Kubernetes 集群升级与回滚运行手册，覆盖预检、控制平面、工作节点、插件与回滚决策矩阵
category: control-plane
tags:
- production
- best-practices
- playbook
- cluster-lifecycle
- upgrade
- rollback
- kubeadm
- control-plane
- etcd
- kubelet
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 集群升级的标准流程是什么
- 如何安全地升级 kubeadm 集群并回滚
- 升级前需要检查哪些兼容性、废弃 API 和证书
trigger_keywords:
- cluster upgrade
- kubernetes upgrade
- kubeadm upgrade
- 集群升级
- 升级回滚
- 控制平面升级
- worker 节点升级
prerequisites:
- kubectl-basics
- kubeadm-basics
- etcd-basics
- linux-admin
- prometheus-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes 集群升级生产运行手册

> **文档类型**: 生产运行手册 | **适用版本**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07
> **使用场景**: 生产级 kubeadm 集群的小版本升级、控制平面滚动升级、工作节点滚动升级及升级失败回滚

---

## 1. 适用场景与范围

本运行手册适用于使用 **kubeadm 部署的 Kubernetes 生产集群** 进行小版本（Minor Version）升级，例如从 v1.30.x 升级到 v1.31.x。本文覆盖：

- 升级前的兼容性、废弃 API、证书与容量检查；
- etcd 与控制平面滚动升级顺序；
- 工作节点的安全驱逐与滚动升级；
- CoreDNS、kube-proxy、CNI、CSI 等集群插件的同步升级判断；
- 升级失败的回滚决策矩阵与具体操作；
- 升级后的关键验证命令与健康基线。

对于托管集群（EKS、GKE、ACK、AKS 等），控制平面升级通常由云厂商托管，SRE 主要负责工作节点与插件版本同步，具体路径应参考 [[18-云厂商/00-总览/99-production-readiness-operations-guide.md|云服务商 生产就绪运维指南]]。若使用 Cluster API、Kubespray 或自研 Operator 管理节点生命周期，可将本手册中的手动命令转化为对应工具的配置与阶段门控。

在执行生产升级前，必须在与生产环境同版本、同配置、同规模的预发或灰度集群完成一次完整演练，验证升级计划、回滚步骤与业务影响。演练报告应作为生产变更工单的附件，未通过演练的集群禁止进入生产升级窗口。

> **不在本手册范围内**: 大版本跳跃升级（如 v1.x → v2.x）、跨云厂商迁移、自定义二进制集群、非 kubeadm 部署（如 OpenShift、Rancher）的专有升级路径。这些情况应参考 [[01-集群基础/03-控制平面/07-plane-upgrade-migration.md|控制平面升级与迁移策略]] 或对应云厂商升级文档。

---

## 2. 前置条件与工具

### 2.1 角色与变更窗口

| 要求 | 说明 |
|------|------|
| 变更工单 | 所有升级操作必须关联变更工单，明确影响范围、回滚窗口、通知名单 |
| 双人复核 | 控制平面 `kubeadm upgrade apply`、etcd 快照恢复、节点批量 drain 必须双人复核 |
| 维护窗口 | 建议选择业务低峰期，控制平面升级预留 ≥ 30 分钟，工作节点按每节点 5–10 分钟估算 |
| 回滚包就绪 | 目标版本与旧版本的 `kubeadm`、`kubelet`、`kubectl` 二进制包已在仓库可获取 |
| 通知模板 | 升级前 30 分钟在值班群与业务方同步窗口，升级结束后 10 分钟内发出完成或异常通报 |
| 回滚窗口 | 生产变更工单必须明确回滚窗口截止时间，超时后视为新事件走事件响应流程 |

### 2.2 必备工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 所有节点必须预装
kubeadm --version
kubelet --version
kubectl version --client
etcdctl version
containerd --version  # 或 cri-o 版本
crictl version
```
### 2.3 备份空间与网络

- `/var/lib/etcd` 所在分区剩余空间 ≥ 当前 etcd 数据大小的 2 倍；
- `/etc/kubernetes` 可完整复制到 `/backup/kube-config-$(date +%Y%m%d)`；
- 所有节点可访问镜像仓库或已预加载目标版本镜像；
- 若使用离线仓库，确认 `kubeadm config images list --kubernetes-version=v1.XX.Y` 所列镜像已同步；
- 升级前 7 天内应完成一次 etcd 快照恢复演练，确认备份文件可用且恢复命令正确。

---

## 3. 标准操作流程

### 3.1 升级前检查

> ⚠️ **🟡 中危操作** — 查询类命令，但仍建议在只读窗口执行，避免误操作

#### 3.1.1 版本偏斜检查

Kubernetes 要求控制平面与 kubelet 版本差 ≤ 2 个小版本，升级时最多升 1 个小版本。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 当前版本
kubectl version --short

# 节点版本
kubectl get nodes -o wide

# 版本偏斜统计
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kubeletVersion}{"\n"}{end}'
```
| 当前版本 | 允许目标版本 |
|----------|--------------|
| v1.28.x | v1.29.x |
| v1.29.x | v1.30.x |
| v1.30.x | v1.31.x |
| v1.31.x | v1.32.x |
| v1.32.x | v1.33.x |

> 严禁跳过小版本升级，例如 v1.29 → v1.31 必须分两次完成。

#### 3.1.2 废弃 API 扫描

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 kubectl 检查当前集群中仍在使用的废弃 API
kubectl get --raw=/metrics 2>/dev/null | grep -i deprecated || true

# 推荐通过 pluto 或 kubepug 扫描
pluto detect-helm --target-versions k8s=v1.33.0
pluto detect-api-resources --target-versions k8s=v1.33.0
```
若扫描结果存在 `removed-in: v1.XX` 且目标版本已移除的 API，必须先修改对应工作负载清单，否则升级后这些资源将无法被管理。

#### 3.1.3 etcd 健康检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
ETCDCTL_API=3 etcdctl endpoint health --cluster \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

ETCDCTL_API=3 etcdctl endpoint status --cluster -w table \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key
```
**通过标准**: 所有成员 `is healthy: true`；leader 稳定；DB 大小 < 8 GB（或平台基线）。

#### 3.1.4 证书有效期检查

```bash
kubeadm certs check-expiration
```

**通过标准**: 所有内部证书剩余有效期 ≥ 30 天。若低于 30 天，建议先执行证书续期再升级：

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
kubeadm certs renew all
systemctl restart kubelet
```
#### 3.1.5 容量与 PDB 检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 节点资源
kubectl top nodes

# 检查是否有 PodDisruptionBudget 会阻止 drain
kubectl get pdb --all-namespaces

# 检查是否有本地存储 Pod
kubectl get pv --all-namespaces | grep local

# 检查异常 Pod
kubectl get pods --all-namespaces --field-selector=status.phase!=Running,status.phase!=Succeeded
```
**通过标准**: 可调度资源余量 ≥ 1 个工作节点容量；关键工作负载已配置 PDB 且允许至少 1 个副本中断；无持续 CrashLoopBackOff。

### 3.2 备份与快照

> ⚠️ **🟠 高危操作** — 备份前确认磁盘空间与对象存储写入权限

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
BACKUP_DIR=/backup/upgrade-$(date +%Y%m%d-%H%M%S)
mkdir -p $BACKUP_DIR

# 1. etcd 快照
ETCDCTL_API=3 etcdctl snapshot save $BACKUP_DIR/etcd.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

ETCDCTL_API=3 etcdctl snapshot status $BACKUP_DIR/etcd.db --write-out=table

# 2. 控制平面配置备份
cp -a /etc/kubernetes $BACKUP_DIR/kubernetes

# 3. 静态 Pod manifest 单独备份
cp -a /etc/kubernetes/manifests $BACKUP_DIR/manifests-$(date +%s)

# 4. 备份验证（至少检查文件大小与校验和）
sha256sum $BACKUP_DIR/etcd.db > $BACKUP_DIR/etcd.db.sha256
ls -lh $BACKUP_DIR
```
备份完成后应立即上传至异地对象存储（如 S3、OSS、COS），并保留至少两个版本周期。

### 3.3 控制平面升级

> ⚠️ **🟠 高危操作** — 影响全集群 API 可用性，必须双人复核

#### 3.3.1 升级第一个控制平面节点

```bash
# 1. 升级 kubeadm 包（示例为 Debian/Ubuntu）
apt-mark unhold kubeadm
apt-get update && apt-get install -y kubeadm=1.XX.Y-1*
apt-mark hold kubeadm
kubeadm version

# 2. 生成升级计划并确认组件版本
kubeadm upgrade plan v1.XX.Y

# 3. 执行升级（默认会自动升级 etcd 与证书续期）
kubeadm upgrade apply v1.XX.Y \
  --etcd-upgrade=true \
  --certificate-renewal=true \
  --yes
```

#### 3.3.2 升级其他控制平面节点

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
# 在其他控制平面节点依次执行
apt-mark unhold kubeadm kubelet kubectl
apt-get update && apt-get install -y kubeadm=1.XX.Y-1* kubelet=1.XX.Y-1* kubectl=1.XX.Y-1*
apt-mark hold kubeadm kubelet kubectl

# 升级节点配置
kubeadm upgrade node

# 重启 kubelet
systemctl daemon-reload
systemctl restart kubelet
```
#### 3.3.3 控制平面升级顺序

```text
1. 第一个控制平面：kubeadm upgrade apply
2. 其余控制平面：kubeadm upgrade node
3. 每个控制平面节点升级后验证 kubelet Ready
4. 全部控制平面完成后再升级工作节点
```

> 若 etcd 采用外部集群，应先在 etcd 侧完成滚动升级，再执行 `kubeadm upgrade apply --etcd-upgrade=false`。

### 3.4 工作节点滚动升级

> ⚠️ **🟠 高危操作** — drain 会驱逐业务 Pod，需确认 PDB 与容量

#### 3.4.1 单节点升级脚本

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
NODE=<node-name>
NEW_VERSION=1.XX.Y

# 1. 标记不可调度并排空
echo "==> Draining $NODE"
kubectl drain $NODE \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --force=false \
  --grace-period=30 \
  --timeout=300s

# 2. SSH 到目标节点升级 kubeadm / kubelet
ssh $NODE "
  set -e
  apt-mark unhold kubeadm kubelet kubectl
  apt-get update
  apt-get install -y kubeadm=${NEW_VERSION}-1* kubelet=${NEW_VERSION}-1* kubectl=${NEW_VERSION}-1*
  apt-mark hold kubeadm kubelet kubectl
  kubeadm upgrade node
  systemctl daemon-reload
  systemctl restart kubelet
"

# 3. 验证节点版本与状态
kubectl get nodes $NODE -o wide

# 4. 解除维护
kubectl uncordon $NODE
```
#### 3.4.2 批量滚动策略

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
# 获取工作节点列表
NODES=$(kubectl get nodes -l '!node-role.kubernetes.io/control-plane' -o jsonpath='{.items[*].metadata.name}')

# 每次升级 BATCH 个节点，默认为 1
BATCH=1
for node in $NODES; do
  echo "==> Upgrading $node"
  kubectl drain $node --ignore-daemonsets --delete-emptydir-data --timeout=300s
  ssh $node "kubeadm upgrade node && systemctl restart kubelet"
  sleep 30
  kubectl uncordon $node
  echo "==> $node done. Waiting for pods reschedule..."
  sleep 60
done
```
> 大规模集群建议使用 Cluster API、Kubespray 或自研升级 Operator 替代手动 SSH。

### 3.5 集群插件与工具链同步升级

控制平面与工作节点升级完成后，需评估以下插件是否需要同步升级：

| 组件 | 判断依据 | 示例操作 |
|------|----------|----------|
| CoreDNS | kubeadm 已自动升级，检查镜像版本 | `kubectl get deployment coredns -n kube-system -o jsonpath='{.spec.template.spec.containers[0].image}'` |
| kube-proxy | kubeadm 已自动升级 | `kubectl get daemonset kube-proxy -n kube-system -o jsonpath='{.spec.template.spec.containers[0].image}'` |
| CNI | 查看 CNI 官方版本矩阵 | `kubectl get pods -n kube-system -l k8s-app=calico-node -o jsonpath='{.items[*].spec.containers[0].image}'` |
| CSI | 查看 CSI 驱动兼容性 | 按厂商文档升级 controller/node 插件 |
| Metrics Server | 依赖聚合层与 API 版本 | `helm upgrade metrics-server stable/metrics-server` |
| cert-manager | 查看 release notes 中 K8s 兼容性 | `helm upgrade cert-manager jetstack/cert-manager` |

> 不要一次性升级所有插件。建议先升级 CNI/CSI，观察 15 分钟后再升级其他组件。

### 3.6 升级期间监控与告警

升级期间应保持核心告警通道畅通，并对预期内的噪音进行临时静音：

```bash
# 建议保留的核心告警
- apiserver_request_duration_seconds: P99 > 1s 持续 2 分钟
- etcd_server_leader_changes_seen_total: 在 5 分钟内发生变化
- kube_node_status_condition{condition="Ready",status="false"}: 控制平面节点 NotReady

# 建议临时静音的告警
- KubePodCrashLooping: 由 drain/uncordon 导致的短暂重启
- TargetDown: 被维护节点上的 node-exporter/kubelet 目标
```

每个控制平面节点升级后，至少观察 2 分钟 API Server 延迟与 etcd leader 状态；每个工作节点 drain 后，确认关键工作负载副本已重新调度并 Ready，再继续下一节点。

### 3.7 常见失败模式与预处理

| 失败现象 | 常见根因 | 预处理与缓解 |
|----------|----------|--------------|
| `kubeadm upgrade plan` 无法连接仓库 | 离线环境或镜像仓库权限异常 | 提前使用 `kubeadm config images list` 拉取镜像并导入私有仓库 |
| drain 被 PDB 阻塞 | 工作负载副本数不足或 PDB 配置过严 | 升级前检查 `kubectl get pdb`，必要时临时放宽或扩容副本 |
| `kubeadm upgrade apply` 提示 etcd 版本不兼容 | 目标 K8s 版本要求更高 etcd 版本 | 参考官方组件矩阵，先滚动升级 etcd 再执行 kubeadm |
| 节点升级后版本未变 | 包管理器缓存未刷新或 hold 状态未解除 | 确认 `apt-mark showhold` 输出为空，再执行安装 |
| 静态 Pod 镜像拉取失败 | 私有仓库未同步新镜像或网络策略限制 | 在每个节点预拉镜像：`crictl pull registry.example.com/kube-apiserver:v1.XX.Y` |

建议将上述失败模式纳入每次升级的桌面检查表（desk checklist），并在升级演练中至少模拟一次 drain 阻塞与镜像拉取失败场景。

---

## 4. 关键检查点与验证命令

### 4.1 升级过程中检查点

| 阶段 | 检查命令 | 通过标准 |
|------|----------|----------|
| 第一个控制平面升级后 | `kubectl get nodes` | 该节点 Ready，版本已更新 |
| etcd 升级后 | `etcdctl endpoint health --cluster` | 全部 healthy |
| 全部控制平面升级后 | `kubectl get pods -n kube-system -l tier=control-plane` | 所有 Pod Running/Ready |
| 每个工作节点升级后 | `kubectl get nodes <node>` | Ready，kubelet 版本更新 |
| 插件升级后 | `kubectl get pods --all-namespaces` | 无异常重启 |

### 4.2 升级后完整验证

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 版本一致性
kubectl version --short
kubectl get nodes -o wide

# 2. 控制平面组件版本
kubectl get pods -n kube-system -l component=kube-apiserver -o jsonpath='{.items[*].spec.containers[0].image}'
kubectl get pods -n kube-system -l component=kube-controller-manager -o jsonpath='{.items[*].spec.containers[0].image}'
kubectl get pods -n kube-system -l component=kube-scheduler -o jsonpath='{.items[*].spec.containers[0].image}'

# 3. etcd 健康
ETCDCTL_API=3 etcdctl endpoint health --cluster

# 4. API Server 响应
kubectl get --raw=/healthz
curl -sk https://localhost:6443/healthz

# 5. 核心功能测试
kubectl run test-nginx --image=nginx:1.25 --restart=Never --rm -it -- /bin/true
kubectl auth can-i create pods --all-namespaces

# 6. DNS 测试
kubectl run dns-test --image=registry.k8s.io/e2e-test-images/agnhost:2.45 --restart=Never --rm -it -- nslookup kubernetes.default.svc.cluster.local

# 7. 证书有效期
kubeadm certs check-expiration

# 8. 事件与异常 Pod
kubectl get events --all-namespaces --sort-by='.lastTimestamp' | tail -n 30
kubectl get pods --all-namespaces --field-selector=status.phase!=Running,status.phase!=Succeeded
```
> 上述验证命令应在升级完成后立即执行一次，并在 24 小时后执行一次复核，以捕获延迟出现的兼容性问题或证书异常。

### 4.3 监控基线核对

升级完成后 24 小时内，重点核对这些指标：

```promql
# API Server P99 延迟
histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket[5m])) by (le, verb))

# etcd leader 变更次数
etcd_server_leader_changes_seen_total

# 节点 NotReady 状态
kube_node_status_condition{condition="Ready",status="false"}

# Pod 重启率
rate(kube_pod_container_status_restarts_total[10m])
```

### 4.4 升级后收尾

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 清理旧版本镜像（仅确认备份完成后执行）
ctr -n k8s.io images prune

# 2. 更新 kubectl 客户端（如操作机未随集群升级）
apt-get install -y kubectl=1.XX.Y-1*

# 3. 将 kubeadm 配置迁移到最新 API 版本
kubeadm config migrate --old-config /etc/kubernetes/kubeadm.conf --new-config /etc/kubernetes/kubeadm.conf.new

# 4. 更新集群版本台账
kubectl get nodes -o wide > /var/lib/kudig/cluster-version-$(date +%Y%m%d).txt
```
收尾完成后，应在变更平台回填实际升级耗时、是否触发回滚、发现的异常与验证结果，并将升级窗口内的监控截图归档。

---

## 5. 回滚/应急方案

### 5.1 回滚决策矩阵

| 异常现象 | 影响范围 | 推荐回滚动作 | 风险等级 |
|----------|----------|--------------|----------|
| `kubeadm upgrade apply` 失败，API Server 无法启动 | 全集群 | 恢复旧版本静态 Pod manifest，回退 kubeadm/kubelet 包 | 高 |
| etcd 升级后集群失去 quorum | 全集群 | 从升级前快照恢复 etcd 数据目录 | 极高 |
| 单个控制平面节点 kubelet 无法启动 | 单节点 | 降级该节点 kubeadm/kubelet 包，恢复 manifest | 中 |
| 工作节点 drain 后无法重新 Ready | 单节点 | 降级 kubelet 包并 uncordon | 中 |
| 应用出现兼容性问题 | 业务负载 | 优先回滚应用版本；集群版本一般不回滚 | 低 |
| 监控指标异常但未影响业务 | 可观测 | 继续观察，不执行集群回滚 | 低 |

### 5.2 控制平面快速回滚

> ⚠️ **🟠 高危操作** — 必须在维护窗口内执行，事前确认 etcd 数据未损坏

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
# 1. 停止 kubelet
systemctl stop kubelet

# 2. 恢复升级前备份的静态 Pod manifest
cp /backup/upgrade-*/manifests-*/kube-apiserver.yaml /etc/kubernetes/manifests/
cp /backup/upgrade-*/manifests-*/kube-controller-manager.yaml /etc/kubernetes/manifests/
cp /backup/upgrade-*/manifests-*/kube-scheduler.yaml /etc/kubernetes/manifests/
cp /backup/upgrade-*/manifests-*/etcd.yaml /etc/kubernetes/manifests/

# 3. 降级 kubeadm / kubelet 包
apt-mark unhold kubeadm kubelet kubectl
apt-get install -y kubeadm=1.OLD.Y-1* kubelet=1.OLD.Y-1* kubectl=1.OLD.Y-1*
apt-mark hold kubeadm kubelet kubectl

# 4. 重启 kubelet
systemctl daemon-reload
systemctl start kubelet

# 5. 验证
kubectl version --short
kubectl get nodes
```
### 5.3 etcd 灾难回滚

> ⚠️ **🔴 灾难性操作** — 会丢失升级后写入的所有 etcd 数据，必须双人复核 + 变更审批

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
# 1. 停止所有控制平面节点 kubelet（停止 etcd 静态 Pod）
systemctl stop kubelet

# 2. 在每个控制平面节点执行恢复
ETCDCTL_API=3 etcdctl snapshot restore /backup/upgrade-*/etcd.db \
  --name=$(hostname) \
  --initial-cluster="cp-1=https://10.0.0.1:2380,cp-2=https://10.0.0.2:2380,cp-3=https://10.0.0.3:2380" \
  --initial-cluster-token=k8s-etcd-cluster \
  --initial-advertise-peer-urls=https://$(hostname -i):2380 \
  --data-dir=/var/lib/etcd

# 3. 启动 kubelet
systemctl start kubelet

# 4. 验证 etcd 与 API Server
ETCDCTL_API=3 etcdctl endpoint health --cluster
kubectl get nodes
```
### 5.4 升级后无法回滚的情况

以下情况发生后通常无法直接回滚集群版本，需要重建集群或从完整灾备恢复：

- etcd 已完成 major 版本升级且数据格式不兼容旧版本；
- 已使用新版本独有 API 创建了工作负载，旧版本无法识别；
- 证书已自动续期并覆盖了旧证书，但未备份旧证书（可用 `kubeadm certs renew` 重新生成，不影响版本回滚）。

---

## 6. 风险与注意事项

1. **版本跳跃风险**: Kubernetes 控制平面最多只能比 kubelet 高 2 个小版本，升级必须逐小版本进行。跳跃升级会导致 API Server 拒绝连接旧 kubelet。
2. **废弃 API 风险**: 升级前务必使用 `pluto` 或 `kubepug` 扫描，目标版本若已移除某 API，相关资源会在升级后不可管理。
3. **证书续期**: `kubeadm upgrade apply` 默认会续期证书。若不希望续期，显式指定 `--certificate-renewal=false`，但需确保证书有效期足够。
4. **etcd 数据不可逆**: etcd 跨 major 版本升级后数据目录格式会变化，回滚前必须确认有对应旧版本的快照。
5. **插件兼容性**: CNI、CSI、Ingress Controller 的版本矩阵必须单独核对，kubeadm 不会自动处理这些组件。
6. **节点维护模式**: drain 时会触发 Pod 驱逐，务必确认 PDB 与资源余量；对 StatefulSet 与本地存储 Pod 需提前评估。
7. **监控告警屏蔽**: 升级期间建议将预期的节点 NotReady、Pod 重建告警临时静音，但保留 API Server 延迟、etcd leader 变更等核心告警。
8. **文档与审计**: 升级完成后 24 小时内更新集群版本台账，并在变更平台回填实际耗时、异常与验证结果。
9. **kube-proxy 模式变化**: 升级后若默认 kube-proxy 模式从 iptables 切换为 ipvs 或 nftables，需提前在 ConfigMap 中显式指定并验证后端连接性。
10. **CRD 与 Helm Release 兼容性**: 自定义资源定义与 Helm chart 可能依赖特定 Kubernetes API 版本，升级前应执行 `helm list` 与 `kubectl get crd` 检查，并在预发环境验证 chart 渲染结果。
11. **节点镜像与容器运行时版本**: kubelet 升级后可能要求更高版本的 containerd 或 cri-o，若运行时版本过旧会导致 Pod 无法创建或 CNI 初始化失败。升级前应核对官方组件支持矩阵，必要时先滚动升级容器运行时。

---

## 7. 相关 Runbook / 推荐阅读

### 同域参考

- [[01-集群基础/03-控制平面/32-kubeadm-upgrade-complete-guide.md|kubeadm 升级完整路径指南（含 rollback）]]
- [[01-集群基础/03-控制平面/07-plane-upgrade-migration.md|控制平面升级与迁移策略]]
- [[01-集群基础/03-控制平面/10-plane-backup-disaster-recovery.md|控制平面备份与灾备方案]]
- [[01-集群基础/03-控制平面/11-etcd-deep-dive.md|etcd 深度解析]]
- [[01-集群基础/03-控制平面/12-apiserver-deep-dive.md|API Server 深度解析]]
- [[01-集群基础/00-总览/99-production-readiness-operations-guide.md|集群基础 生产就绪运维指南]]

### 跨域参考

- [[11-发布变更/00-总览/99-production-readiness-operations-guide.md|变更与发布管理 生产就绪运维指南]] — 变更窗口、发布门控、回滚审批
- [[12-可靠性/00-总览/99-production-readiness-operations-guide.md|可靠性工程 生产就绪运维指南]] — RTO/RPO、DR 演练、PDB 设计
- [[13-生产运维/00-总览/99-production-readiness-operations-guide.md|生产运维 生产就绪运维指南]] — 值班、事件响应、变更台账
- [[18-云厂商/00-总览/99-production-readiness-operations-guide.md|云服务商 生产就绪运维指南]] — 托管集群（EKS/GKE/ACK）升级路径
- [[08-安全/00-总览/99-production-readiness-operations-guide.md|安全合规 生产就绪运维指南]] — 证书生命周期、审计策略
- [[09-可观测性/01-总览/99-production-readiness-operations-guide.md|可观测性 生产就绪运维指南]] — 升级期间监控基线与告警配置

---

*本运行手册基于 KUDIG 集群基础域 gap 分析编写，重点补齐生产环境集群升级预检、版本偏斜控制、滚动升级顺序、回滚决策矩阵与升级后验证等关键缺口。*


<!-- risk-assessed -->
