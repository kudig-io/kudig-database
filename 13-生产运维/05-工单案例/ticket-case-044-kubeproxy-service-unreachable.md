---
title: kube-proxy 异常导致 Service 不通
description: 专有云 ACK 集群因 kube-proxy 配置损坏导致 Pod 无法通过 ClusterIP 访问后端服务的工单闭环样本。
summary: 专有云 ACK 集群因 kube-proxy 配置损坏导致 Pod 无法通过 ClusterIP 访问后端服务的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- kube-proxy
- service
- iptables
- clusterip
- p0
tier: supporting
created: '2026-06-26T14:00:00+08:00'
updated: '2026-06-26T15:30:00+08:00'
incident_id: INC-2026-ACK-044
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-04
affected_namespace: kube-system
ticket_type: 网络故障
skill_ref:
- '[[05-网络/01-K8s网络核心/09-kube-proxy-modes-performance.md|kube-proxy
  模式与性能]]'
- '[[19-故障诊断/06-FTA故障树/list/service-fta.md|Service 异常故障树分析]]'
fta_ref:
- '[[19-故障诊断/06-FTA故障树/list/service-fta.md|FTA: Service
  异常]]'
last_updated: 2026-06-26 15:30:00+08:00
duplicate_of: INC-2026-ACK-019
status: duplicate
duplication_reason: 与 "INC-2026-ACK-019" 主题重复，内容角度相似，降低 RAG 权重
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- kube-proxy 异常导致 Service 不通 如何处理
trigger_keywords:
- kube-proxy
prerequisites:
- kubectl-basics
- k8s-networking
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
- target: '[[17-系统基础/06-知识字典/fundamentals/kube-proxy.md]]'
  type: related_to
- target: '[[22-概念/03-网络/service.md]]'
  type: related_to
- target: '[[13-生产运维/05-工单案例/ticket-case-041-ingress-controller-502.md]]'
  type: related_to
- target: '[[13-生产运维/05-工单案例/ticket-case-046-ingress-controller-404-502.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户在专有云 ACK 集群 `ack-zyy-prod-04` 的微服务平台进行发布验证时，发现多个服务之间无法通过 Service 域名互相访问，部分接口调用超时。客户描述如下：

> “我们服务 A 调用服务 B 的 ClusterIP 一直超时，但直接访问 Pod IP 是正常的。nslookup 域名也能解析。所有 Service 的 endpoints 都存在。怀疑是 kube-proxy 或者 iptables 出问题，麻烦紧急看一下。”

受影响命名空间包括 `order-service`、`pay-service`、`inventory-service` 等，几乎所有依赖 Service 通信的微服务均受影响。

## 分类与优先级判定

- **工单类型**：网络故障 / kube-proxy 故障。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产环境大量微服务通过 ClusterIP 通信失败，服务间调用大面积超时。
2. Pod IP 直接访问正常，域名解析正常，仅 Service 层转发异常，高度指向 kube-proxy/iptables。
3. 影响范围接近全集群，符合“服务不可用”标准，需立即止血。

## 诊断步骤

按“先 Service 状态、后 iptables 规则、再 kube-proxy 日志”的顺序排查：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 查看 Service 与 Endpoints 状态
kubectl get svc -n order-service
kubectl get endpoints -n order-service

# 2. 从问题 Pod 内测试 ClusterIP 与 Pod IP
kubectl exec -n order-service deploy/order-api -- sh -c 'nc -vz order-svc.order-service 8080'
kubectl exec -n order-service deploy/order-api -- sh -c 'nc -vz 172.16.4.101 8080'

# 3. 检查 kube-proxy Pod 状态与重启
kubectl get pod -n kube-system -l k8s-app=kube-proxy -o wide
kubectl describe pod -n kube-system -l k8s-app=kube-proxy

# 4. 查看 kube-proxy 日志中的错误与同步状态
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=200 | grep -iE "error|fail|sync|iptables"

# 5. 检查节点 iptables nat 表中的 KUBE-SERVICES 链
kubectl node-shell cn-zhangjiakou.172.16.4.15 -- iptables -t nat -L KUBE-SERVICES -n | head -30

# 6. 检查 kube-proxy ConfigMap 配置是否被异常修改
kubectl get configmap kube-proxy -n kube-system -o yaml | head -50

# 7. 检查节点 conntrack 表是否溢出
kubectl node-shell cn-zhangjiakou.172.16.4.15 -- conntrack -L | wc -l
kubectl node-shell cn-zhangjiakou.172.16.4.15 -- sysctl net.netfilter.nf_conntrack_count net.netfilter.nf_conntrack_max
```
## 根因分析

节点 `cn-zhangjiakou.172.16.4.15` 上的 kube-proxy 在 14:05 被运维人员手动修改 ConfigMap 时触发了滚动更新，但新的 kube-proxy ConfigMap 中 `iptables.minSyncPeriod` 被误设置为 `0s`，同时 `mode` 字段被从 `iptables` 改为 `ipvs` 后未清理旧的 iptables 规则。部分节点上的 kube-proxy 使用 ipvs 模式启动失败，回退到 iptables 模式但规则损坏：

```
iptables: Failed to execute iptables-restore: exit status 4 (Resource temporarily unavailable)
```

由于 iptables 规则不完整，`KUBE-SERVICES` 链中缺失大量 Service 的 DNAT 规则，导致 Pod 访问 ClusterIP 时数据包无法被正确转发到后端 Pod。根本原因是 kube-proxy 配置变更未经过验证，且新旧模式规则未清理。Pod IP 直接访问正常、域名解析正常而 Service 访问异常，是 kube-proxy/iptables 故障的典型特征，可以快速缩小排查范围。

## 修复命令

**第一步：恢复 kube-proxy ConfigMap 为正确配置**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 备份当前 ConfigMap
kubectl get configmap kube-proxy -n kube-system -o yaml > /tmp/kube-proxy-config-backup.yaml

# 恢复 mode 为 iptables 并设置合理的 minSyncPeriod
kubectl patch configmap kube-proxy -n kube-system --type merge -p '{"data":{"config.conf":"apiVersion: kubeproxy.config.k8s.io/v1alpha1\nkind: KubeProxyConfiguration\nmode: iptables\niptables:\n  minSyncPeriod: 10s\n  syncPeriod: 30s\n"}}'
```
**第二步：清理节点上残留的 ipvs 规则与损坏的 iptables 规则**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 在异常节点上执行
kubectl node-shell cn-zhangjiakou.172.16.4.15 -- ipvsadm --clear
kubectl node-shell cn-zhangjiakou.172.16.4.15 -- iptables -t nat -F KUBE-SERVICES
kubectl node-shell cn-zhangjiakou.172.16.4.15 -- iptables -t nat -F KUBE-POSTROUTING
kubectl node-shell cn-zhangjiakou.172.16.4.15 -- iptables -t filter -F KUBE-FORWARD
```
**第三步：重启所有 kube-proxy Pod 重新生成规则**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart daemonset/kube-proxy -n kube-system
kubectl rollout status daemonset/kube-proxy -n kube-system --timeout=180s
```
**第四步：验证 iptables 规则恢复完整**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl node-shell cn-zhangjiakou.172.16.4.15 -- iptables -t nat -L KUBE-SERVICES -n | grep -c "order-service"
```
若规则恢复后仍有部分 Service 不通，可针对异常 Service 单独检查其 Endpoints 与 `KUBE-SEP-*` 链，必要时清理该 Service 对应的 iptables 子链并重启 kube-proxy，避免全量规则 flush 造成二次影响。

## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. kube-proxy Pod 全部 Running
kubectl get pod -n kube-system -l k8s-app=kube-proxy -o wide

# 2. Service ClusterIP 访问恢复
kubectl exec -n order-service deploy/order-api -- sh -c 'nc -vz order-svc.order-service 8080'

# 3. 跨命名空间 Service 访问正常
kubectl exec -n pay-service deploy/pay-api -- sh -c 'nc -vz inventory-svc.inventory-service 8080'

# 4. iptables nat 表 KUBE-SERVICES 链包含业务 Service
kubectl node-shell cn-zhangjiakou.172.16.4.15 -- iptables -t nat -L KUBE-SERVICES -n | grep order-service | head -10

# 5. 业务接口端到端调用成功
kubectl exec -n order-service deploy/order-api -- wget -qO- --timeout=10 http://order-svc.order-service:8080/actuator/health
```
验证时不能只检查单一 Service，而应在多个命名空间与多个节点上交叉验证，因为 kube-proxy 规则是按节点生成的，可能存在部分节点规则生成失败而其他节点正常的情况。建议从每个节点上随机抽取一个 Pod 执行 ClusterIP 连通性测试，确保全集群规则一致。

## 回复客户话术

> 您好，经排查，本次 Service 不通的根因是 **kube-proxy 配置被异常修改导致 iptables 规则损坏**。ConfigMap 中的 mode 被改为 `ipvs` 但节点未清理旧 iptables 规则，部分 kube-proxy 启动失败后规则生成不完整，ClusterIP 流量无法 DNAT 到后端 Pod。我们已完成以下处置：
>
> 1. 恢复 kube-proxy ConfigMap 为 `iptables` 模式并设置合理同步周期；
> 2. 清理异常节点上的 ipvs 规则与损坏的 iptables 规则；
> 3. 全量重启 kube-proxy DaemonSet，重新生成 Service 转发规则。
>
> 当前各服务间 ClusterIP 访问已恢复，业务健康检查通过。建议后续：
> - kube-proxy 配置变更必须通过 GitOps 审批与灰度发布；
> - 禁止手动修改 kube-system 下的关键 ConfigMap；
> - 配置 kube-proxy 同步失败告警。
>
> 如有新异常请随时联系。

## 复盘与沉淀

本次故障是典型的人为配置变更引发的网络平面故障。kube-proxy 作为 Kubernetes Service 网络的核心组件，其模式切换与参数调整必须谨慎。ipvs 模式与 iptables 模式在规则存储位置与机制上完全不同，直接切换而不清理旧规则会导致双份规则冲突或缺失。

在专有云 ACK 中，kube-proxy 通常以 DaemonSet 部署，配置集中管理在 `kube-system/kube-proxy` ConfigMap。任何对该 ConfigMap 的修改都会触发全集群滚动更新，风险极高。建议将此类变更纳入变更管理流程，先在测试集群验证，再按节点池灰度执行。对于大规模集群，建议评估迁移到 ipvs 模式的可行性，ipvs 在大规模 Service 场景下具有更好的转发性能与规则管理效率，但迁移前必须制定完整的规则清理与回滚方案。

此外，节点的 `conntrack` 表容量也是 Service 通信稳定性的关键因素。大流量场景下 conntrack 表溢出会导致连接被丢弃，表现为间歇性超时。建议根据节点规格调整 `net.netfilter.nf_conntrack_max`，并监控 `node_nf_conntrack_entries_limit` 与 `node_nf_conntrack_entries` 的差距。对于高并发业务，还可以考虑开启 `nf_conntrack_tcp_timeout_established` 调优，避免长连接过早老化。

后续 SOP 更新要点：
1. 禁止直接 edit kube-proxy ConfigMap，所有变更通过 GitOps PR；
2. 切换 kube-proxy 模式前必须先清理节点上的旧规则；
3. 监控 `kubeproxy_sync_proxy_rules_duration_seconds` 与 `kubeproxy_sync_proxy_rules_iptables_restore_failures_total`；
4. 根据集群规模调整节点 conntrack 容量，避免连接表溢出；
5. 将本案例写入 Service 不通回复模板。

## 是否需要升级及交接信息

- **是否升级**：已定位并止血，暂不需要升级；若全集群 iptables 规则多次异常或集群规模超过 5000 个 Service，需升级至 **网络基础设施团队** 评估迁移至 ipvs 模式的方案，并制定详细的规则清理与回滚计划。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-044`
  - 根因：kube-proxy 配置异常修改导致 iptables 规则损坏
  - 影响范围：`order-service`、`pay-service`、`inventory-service` 等全集群微服务通信
  - 临时修复：恢复 ConfigMap + 清理规则 + 重启 kube-proxy
  - 长期方案：GitOps 管控关键 ConfigMap + 灰度发布 + kube-proxy 监控
  - 待跟进：审计 kube-system 配置变更权限，更新变更管理 SOP

## Related

- kube-proxy
- Service
- Ingress 控制器 Pod 异常导致 404/502
- Ingress 控制器 Pod 异常导致业务访问 404/502


<!-- risk-assessed -->
