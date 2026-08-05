---
title: 节点 NotReady：Terway ENI IP 耗尽
description: 专有云 ACK 集群因 Terway ENI 辅助 IP 耗尽导致节点 NotReady、Pod 无法分配沙箱网络的工单闭环样本。
summary: 专有云 ACK 集群因 Terway ENI 辅助 IP 耗尽导致节点 NotReady、Pod 无法分配沙箱网络的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- terway
- eni
- node-notready
- network
- p0
tier: peripheral
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T16:30:00+08:00'
incident_id: INC-2026-ACK-001
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-01
affected_namespace: kube-system
ticket_type: 节点故障
skill_ref:
- 节点 NotReady FTA
- Terway 网络诊断
fta_ref:
- 'FTA: Terway ENI 耗尽导致 NotReady'
last_updated: 2026-06-26 16:30:00+08:00
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 节点 NotReady：Terway ENI IP 耗尽 如何处理
trigger_keywords:
- 节点
prerequisites:
- kubectl-basics
- alicloud-basics
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
- target: '[[domain-11-production-operations/工单案例/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-039-rbac-api-access-denied.md]]'
  type: related_to
- target: '[[domain-11-production-operations/工单案例/ticket-case-010-networkpolicy-blocks-traffic.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户通过 ACK 专有云云监控告警发现节点 `cn-zhangjiakou.172.16.1.87` 状态变为 `NotReady`，该节点上运行的核心订单服务 Pod 全部处于 `ContainerCreating` 或 `Pending` 状态。客户描述如下：

> “生产环境 ACK 集群里有一台节点突然 NotReady，kubectl get node 看是 Unknown。所有新创建的 Pod 都起不来，describe pod 看到 `FailedCreatePodSandBox` 和 `network: assignPodIPv4 fail: no available IP` 之类的错误。集群用的是 Terway ENI 模式，麻烦尽快看一下。”

该集群为专有云 `ack-zyy-prod-01`，命名空间主要为 `order-service` 与 `kube-system`，当前业务峰值期，影响在线下单能力。

## 分类与优先级判定

- **工单类型**：节点故障 / 网络插件故障。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产环境集群出现节点级故障，直接造成业务 Pod 无法调度与启动。
2. 报错指向 Terway CNI 无法分配 Pod IP，属于网络基础设施层问题，影响面随时间扩大。
3. 处于业务高峰，符合“服务不可用”标准，需在 15 分钟内给出止血方案。

## 诊断步骤

按“先状态、后日志、再资源配额”的顺序排查：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认节点状态与容器运行时版本
kubectl get node cn-zhangjiakou.172.16.1.87 -o wide
kubectl describe node cn-zhangjiakou.172.16.1.87 | grep -A 20 Conditions

# 2. 查看节点事件，定位 NotReady 触发点
kubectl get events --field-selector involvedObject.name=cn-zhangjiakou.172.16.1.87 --sort-by='.lastTimestamp' -n default

# 3. 检查 Pod 创建失败原因
kubectl get events --field-selector reason=FailedCreatePodSandBox --all-namespaces | head -50

# 4. 采集 Terway 日志，寻找 IP 分配异常
kubectl logs -n kube-system -l app=terway -c terway --tail=300 --all-containers | grep -i "assignPodIPv4|no available IP|eni"

# 5. 查询该节点 ECS ENI 与辅助 IP 使用情况
aliyun ecs DescribeNetworkInterfaces \
  --RegionId cn-zhangjiakou \
  --InstanceId i-8vbdummy87 \
  --output cols=NetworkInterfaceId,PrivateIpAddress,SecondaryPrivateIpAddressCount rows=NetworkInterfaces.NetworkInterface[]

# 6. 使用 ACK 诊断工具查看节点网络配额
ack-cli node diagnose cn-zhangjiakou.172.16.1.87 --cluster ack-zyy-prod-01 --module network

# 7. 通过 ASO 检查该节点 ENI 关联的 PrivatePool 状态
kubectl get eni -n kube-system cn-zhangjiakou.172.16.1.87 -o yaml
```
## 根因分析

节点 `cn-zhangjiakou.172.16.1.87` 为 `ecs.c7.xlarge` 规格，单节点默认最多挂载 3 张 ENI，每张 ENI 最多分配 10 个辅助私网 IP。该节点承载了 28 个 Pod（含 DaemonSet 与业务 Pod），Terway 按 Pod 粒度独占 IP。当业务突发扩容时，Terway 请求分配新的辅助 IP 失败：

```
assignPodIPv4 fail: no available private ip for pod order-service/order-api-7d9c4f8b5-xk2z9
```

由于 Terway 健康检查依赖 IP 分配能力，持续失败后被 kubelet 判定为 CNI 插件异常，节点状态转为 `NotReady`。根本原因是节点 ENI 辅助 IP 配额不足，而非节点 CPU/内存资源不足。

## 修复命令

**第一步：隔离节点，避免新 Pod 继续调度到问题节点**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

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
kubectl cordon cn-zhangjiakou.172.16.1.87
```
**第二步：将可迁移业务 Pod 驱逐到其他节点，释放部分 ENI IP**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl drain cn-zhangjiakou.172.16.1.87 \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --force \
  --pod-selector='app notin (node-exporter,terway)' \
  --timeout=300s
```
**第三步：临时扩容节点池，增加可承载 Pod 的节点数**

```bash
aliyun cs POST /clusters/ack-zyy-prod-01/nodes \
  --body '{"count":2,"instance_type":"ecs.c7.2xlarge","image_id":"aliyun_3_x64_20G_alibase_20240618.vhd","nodepool_id":"np-zyy-compute"}'
```

**第四步：调整节点池伸缩组实例规格为支持更多 ENI 的机型（变更后滚动替换）**

在 ACK 控制台 → 节点池 → `np-zyy-compute` → 修改实例规格为 `ecs.c7.2xlarge`，并开启自动修复。对存量节点执行替换：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
kubectl delete node cn-zhangjiakou.172.16.1.87
# 由集群自动缩容/扩容完成替换
```
**第五步：恢复节点可调度**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl uncordon cn-zhangjiakou.172.16.1.87
```
## 验证命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 节点恢复 Ready
kubectl get node cn-zhangjiakou.172.16.1.87 -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'

# 2. 新建测试 Pod 验证 Terway IP 分配
kubectl run network-test --image=registry.aliyuncs.com/acs/busybox --restart=Never -n default -- sleep 600
kubectl get pod network-test -n default -o wide
kubectl logs -n kube-system -l app=terway -c terway --tail=50 | grep network-test

# 3. 业务 Pod 全部 Running
kubectl get pod -n order-service -o wide | grep -v Running

# 4. ENI 配额检查
aliyun ecs DescribeNetworkInterfaces --RegionId cn-zhangjiakou --InstanceId i-8vbdummy87
```
## 回复客户话术

> 您好，经排查，本次节点 NotReady 的根因是 **Terway ENI 辅助 IP 配额耗尽**。该节点上 Pod 数量接近 ENI IP 上限，扩容时 CNI 无法为新 Pod 分配 IP，进而触发节点状态异常。我们已完成以下处置：
>
> 1. 隔离问题节点并迁移可移动业务 Pod；
> 2. 临时扩容 2 台 `ecs.c7.2xlarge` 节点以承载新增 Pod；
> 3. 调整节点池规格并替换高负载节点，提升 ENI IP 容量。
>
> 当前节点已恢复 Ready，业务 Pod 全部 Running。建议后续：
> - 评估业务密度，避免单节点 Pod 数超过 ENI IP 上限；
> - 为节点池选择支持更多 ENI 的实例规格；
> - 配置 Terway ENI 使用率告警。
>
> 如有任何新异常，请随时联系。

## 复盘与沉淀

本次故障充分体现了专有云 ACK 在使用 Terway ENI 模式时，节点网络容量与计算容量并不完全等价。即使节点 CPU/内存仍有富余，ENI 辅助 IP 耗尽同样会导致节点失稳。`ecs.c7.xlarge` 单节点最多 3 张 ENI，每张 10 个辅助 IP，扣除系统保留后可用 IP 约为 28 个，业务密度一旦超过该阈值就会出现 IP 分配失败。

在排障过程中，需要注意区分 Terway 的三种模式：`Terway ENI`、`Terway ENIIP` 与 `Terway IPVLAN`。本例为 ENI 模式，每个 Pod 独占一个辅助 IP，因此对 ENI 配额最敏感。若业务场景 Pod 密度高，可优先考虑 `Terway ENIIP` 模式，使一个辅助 IP 可被多个 Pod 共享，显著提升单节点 Pod 容量。

另外，ASO（Alibaba Cloud Service Operator）侧的 PrivatePool 状态、`NetworkInterface` 绑定关系、VPC 子网剩余 IP 数也是排查时必须同步检查的项目。若子网本身 IP 耗尽，则单纯替换节点规格无法解决，需要同步扩容 VPC 网段或调整交换机。

建议将以下指标纳入日常监控：节点已分配 Pod IP 数、ENI 剩余辅助 IP 数、Terway 分配失败次数、节点 NotReady 持续时间。配合 容量规划 流程，在业务扩容前评估网络容量余量，避免再次触发同类故障。

后续 SOP 更新要点：
1. 节点池变更前，使用 `ack-cli node capacity` 计算 ENI IP 上限与当前 Pod 密度；
2. 高密业务节点池统一使用支持更多 ENI 的实例规格，如 `ecs.c7.2xlarge`（最多 4 张 ENI，每张 15 辅助 IP）；
3. 在 Prometheus 中配置告警规则：`terway_allocated_ips / terway_max_ips > 0.85` 持续 5 分钟触发 P2 告警；
4. 将本案例写入 节点 NotReady 回复模板，缩短后续同类工单响应时间。

同时，建议在节点池维度建立“网络容量看板”，按实例类型展示单节点 ENI IP 上限、当前分配率、最近 7 天峰值，便于在业务上线前发现潜在瓶颈。最后，在故障闭环报告中量化影响：记录节点 NotReady 持续时间、受影响 Pod 数量、业务失败请求数，以及因临时扩容产生的额外成本。这些数据不仅用于客户沟通，也为后续容量规划和 FinOps 分析提供依据，可参考 成本影响评估 模板进行统计。

## 是否需要升级及交接信息

- **是否升级**：已定位并止血，暂不需要升级；若替换节点后仍频繁出现 ENI 耗尽，需升级至 **网络基础设施团队** 与 **ACK 产品支持**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-001`
  - 根因：`Terway ENI 辅助 IP 配额耗尽`
  - 影响节点：`cn-zhangjiakou.172.16.1.87`
  - 临时修复：节点驱逐 + 节点池扩容
  - 长期方案：调整节点池实例规格并启用 ENI 使用率监控
  - 待跟进：确认节点池滚动替换完成，更新 SOP 与容量基线

## Related

- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- RBAC 权限不足导致应用无法访问 K8s API
- 阿里云专有云 NetworkPolicy 误拦截导致服务间调用 503


<!-- risk-assessed -->
