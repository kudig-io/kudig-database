---
title: PVC 挂载失败：云盘 CSI 插件缺失
description: 专有云 ACK 集群节点池扩容后，新节点未安装云盘 CSI 插件，导致 Pod PVC 挂载失败的工单闭环样本。
summary: 专有云 ACK 集群节点池扩容后，新节点未安装云盘 CSI 插件，导致 Pod PVC 挂载失败的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- csi
- cloud-disk
- pvc
- storage
- nodepool
- p1
tier: peripheral
created: '2026-06-26T08:30:00+08:00'
updated: '2026-06-26T10:10:00+08:00'
incident_id: INC-2026-ACK-004
priority: P1
severity: high
affected_cluster: ack-zyy-prod-04
affected_namespace: app-data
ticket_type: 存储故障
skill_ref:
- 云盘 CSI 排障
- PVC 挂载失败排查
fta_ref:
- 'FTA: PVC 挂载失败-CSI 插件缺失'
last_updated: 2026-06-26 10:10:00+08:00
difficulty: intermediate
reading_level: intermediate
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- PVC 挂载失败：云盘 CSI 插件缺失 如何处理
trigger_keywords:
- PVC
prerequisites:
- kubectl-basics
- k8s-storage
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
relationships:
- target: '[[系统基础/知识字典/storage/csi.md]]'
  type: related_to
- target: '[[生产运维/工单案例/ticket-case-043-statefulset-pvc-unbound.md]]'
  type: related_to
- target: '[[生产运维/工单案例/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
- target: '[[生产运维/工单案例/ticket-case-028-statefulset-pvc-unbound.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户反馈 `app-data` 命名空间下的 MySQL 主从 StatefulSet 新扩容出来的 Pod 一直处于 `ContainerCreating`，describe pod 提示无法挂载 PVC。客户描述：

> “我们昨天晚上对节点池做了扩容，今天早上 MySQL 从库起不来，报错 `Unable to attach or mount volumes: unmount vol-xxx is not mounted` 还有 `csi-plugin xxx not found`。老节点上的 Pod 都正常，就新扩容的节点有问题。是不是新节点没装 CSI 插件？”

受影响集群 `ack-zyy-prod-04`，节点池 `np-data-ssd`，新节点 `cn-zhangjiakou.172.16.4.21`、`cn-zhangjiakou.172.16.4.22`。

## 分类与优先级判定

- **工单类型**：存储故障 / CSI 插件部署异常。
- **优先级**：P1。
- **严重级别**：high。

判定依据：
1. 生产环境有状态应用无法启动，存在数据服务单点风险。
2. 问题仅出现在新扩容节点，老节点正常，根因高度指向 CSI 插件未覆盖新节点池。
3. 修复明确，属于可控变更，无需全集群紧急止血，但需在 30 分钟内完成。

## 诊断步骤

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 Pod 状态与事件
kubectl get pod -n app-data -l app=mysql -o wide
kubectl describe pod -n app-data mysql-slave-0 | tail -60

# 2. 查看 PVC/PV 状态
kubectl get pvc -n app-data
kubectl get pv | grep mysql

# 3. 检查新节点是否有 CSI 插件 Pod
kubectl get pod -n kube-system -l app=csi-plugin -o wide
kubectl get pod -n kube-system -l app=csi-provisioner -o wide

# 4. 查看 CSI 插件 DaemonSet 的 nodeSelector 与容忍
kubectl get ds csi-plugin -n kube-system -o yaml | grep -A 20 nodeSelector
kubectl get ds csi-plugin -n kube-system -o yaml | grep -A 30 tolerations

# 5. 对比新老节点标签
kubectl get node cn-zhangjiakou.172.16.4.21 --show-labels
kubectl get node cn-zhangjiakou.172.16.4.03 --show-labels

# 6. 查看节点池信息
ack-cli nodepool list --cluster ack-zyy-prod-04
aliyun cs GET /clusters/ack-zyy-prod-04/nodepools/np-data-ssd

# 7. 查看 CSI 相关事件
kubectl get events -n app-data --field-selector reason=FailedMount --sort-by='.lastTimestamp' | tail -30
```
## 根因分析

节点池 `np-data-ssd` 昨晚扩容时使用了新镜像 `aliyun_3_x64_20G_alibase_20240618.vhd`，但该镜像模板中未预装 ACK 云盘 CSI 插件。现有 `csi-plugin` DaemonSet 的 nodeSelector 为 `alibabacloud.com/csi-plugin: "true"`，扩容脚本仅给老节点打了该标签，新节点缺少标签，导致 CSI DaemonSet 未调度到新节点。

同时，新节点的 kubelet 配置中 `--enable-controller-attach-detach` 与 CSI 驱动注册路径正常，但无 csi-plugin 容器，因此 kubelet 无法调用 `ControllerPublishVolume`，PVC 挂载失败。

根因：
1. 新节点镜像未包含 CSI 插件；
2. 节点标签缺失导致 DaemonSet 未覆盖；
3. 节点池扩容后缺少 CSI 就绪检查。

## 修复命令

**第一步：给新节点补打 CSI 插件标签**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl label node cn-zhangjiakou.172.16.4.21 alibabacloud.com/csi-plugin=true --overwrite
kubectl label node cn-zhangjiakou.172.16.4.22 alibabacloud.com/csi-plugin=true --overwrite
```
**第二步：检查 CSI 插件 Pod 调度到新节点**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod -n kube-system -l app=csi-plugin -o wide -w
kubectl wait --for=condition=Ready pod -l app=csi-plugin -n kube-system --timeout=120s
```
**第三步：若仍未调度，检查并放宽 DaemonSet 的 nodeSelector/tolerations**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch ds csi-plugin -n kube-system --type='json' -p='[
  {"op": "replace", "path": "/spec/template/spec/nodeSelector", "value": {"kubernetes.io/os":"linux"}}
]'
```
**第四步：重启 kubelet 以重新触发卷挂载（必要时）**

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
kubectl debug node/cn-zhangjiakou.172.16.4.21 -it --image=registry.aliyuncs.com/acs/busybox -- chroot /host systemctl restart kubelet
```
**第五步：对节点池进行修复，确保后续扩容自动安装 CSI**

在 ACK 控制台 → 节点池 `np-data-ssd` → 节点配置 → 开启 **自动安装 CSI 插件**，或执行：

```bash
aliyun cs POST /clusters/ack-zyy-prod-04/nodepools/np-data-ssd/operation/install_csi_plugin \
  --body '{"plugin_type":"disk"}'
```

## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. CSI 插件 Pod 在新节点 Running
kubectl get pod -n kube-system -l app=csi-plugin -o wide | grep -E "4.21|4.22"

# 2. Pod 事件中的挂载失败消失
kubectl describe pod -n app-data mysql-slave-0 | grep -i "mounted|attached"

# 3. MySQL Pod 恢复 Running
kubectl get pod -n app-data -l app=mysql

# 4. PVC/PV 状态正常
kubectl get pvc -n app-data
kubectl get pv

# 5. 在 Pod 内验证挂载点
kubectl exec -it -n app-data mysql-slave-0 -- df -h | grep mysql-data
```
## 回复客户话术

> 您好，`app-data/mysql-slave-0` PVC 挂载失败的根因已确认：**新扩容节点未安装云盘 CSI 插件**。
>
> 具体是节点池 `np-data-ssd` 昨晚扩容的新节点缺少 `alibabacloud.com/csi-plugin=true` 标签，导致 CSI DaemonSet 未调度到新节点，kubelet 无法挂载云盘。
>
> 已执行修复：
>
> - 为新节点补打 CSI 标签，CSI 插件 Pod 已成功 Running；
> - 在 ACK 控制台为节点池开启自动安装 CSI 插件，避免后续扩容再出现同类问题；
> - 触发 Pod 重新调度并验证挂载成功。
>
> 当前 MySQL 从库已恢复 Running，数据卷挂载正常。后续建议：
>
> - 节点池扩容后增加 **CSI 插件就绪检查** 作为准入条件；
> - 配置 CSI 插件未就绪告警；
> - 将 CSI 标签与节点初始化脚本绑定，避免人工遗漏。

## 复盘与沉淀

CSI 插件是 Kubernetes 与底层云存储之间的桥梁。在 ACK 专有云中，云盘 CSI 通常由 `csi-plugin` DaemonSet（节点侧）与 `csi-provisioner` Deployment（控制面侧）组成。节点侧负责 `NodeStageVolume`、`NodePublishVolume`，控制面侧负责 `CreateVolume`、`ControllerPublishVolume`。任何一侧缺失或版本不匹配，都会导致 PVC 无法挂载或卸载。

节点池扩容后未安装 CSI 的根本原因是镜像模板与节点初始化脚本不同步。新镜像虽然满足 kubelet 启动条件，但缺少 CSI 标签与 CSI 容器。此类问题在以下场景也容易复现：自定义镜像、私有镜像仓库、离线环境、节点池跨可用区扩容、使用 ACK 边缘节点池。因此，节点上线 checklist 中必须包含“CSI 插件已调度且 Ready”这一项。

排查时可通过对比新老节点的标签、Annotation、kubelet 配置快速定位。若 DaemonSet 已调度但 CSI Pod 处于 `CrashLoopBackOff`，则应优先检查 `csi-plugin` 日志中的驱动注册错误、Secret 权限、云凭据有效性。本例中 CSI Pod 完全未调度，属于覆盖性问题，补打标签即可解决。

另外，在修复后若 Pod 仍报 `Multi-Attach error`，需检查 `VolumeAttachment` 对象是否仍绑定在旧节点上，可执行 `kubectl get volumeattachment | grep <pv-name>` 并删除残留对象，必要时重启 kubelet 刷新挂载状态。

为降低复发概率，建议：
1. 将 CSI 标签与节点初始化脚本绑定，作为节点池扩容的必填参数；
2. 在 ACK 控制台开启“自动安装存储插件”，确保新节点自动安装最新版本 CSI；
3. 配置 CSI Pod NotReady 告警，并在节点加入集群后执行挂载冒烟测试；
4. 将本案例写入 PVC CSI 插件缺失回复模板。

建议在存储插件管理中引入“版本一致性检查”：每次节点池扩容后，验证新节点 CSI 镜像版本与存量节点一致，避免因版本差异导致挂载协议不兼容。同时，将 CSI 就绪检查纳入 节点上线检查清单，确保存储、网络、监控三大插件全部 Ready 后再接入生产负载。

另外，建议在节点池扩容后执行一次“存储挂载冒烟测试”：创建一个带 PVC 的测试 Pod 并写入数据，验证 attach、mount、读写全链路正常后再接入生产 StatefulSet。该测试可自动化为 ACK 节点池扩容后的后置钩子。

最后，将 CSI 插件缺失排查步骤固化为 runbook，确保夜班值班同学也能按图索骥快速恢复。

## 是否需要升级及交接信息

- **是否升级**：已修复，无需升级；若 CSI 插件频繁因镜像问题缺失，需升级至 **ACK 产品支持** 与 **镜像基线团队**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-004`
  - 根因：`节点池扩容后新节点未安装云盘 CSI 插件`
  - 影响应用：`app-data/mysql-slave-0`
  - 修复方式：节点补标签 + 节点池开启自动安装 CSI
  - 待跟进：更新节点池镜像基线，纳入节点上线检查清单

## Related

- 容器存储接口
- StatefulSet Pod 启动失败：PVC 未绑定
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- StatefulSet Pod 启动失败：PVC 未绑定


<!-- risk-assessed -->
