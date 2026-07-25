---
title: Service 访问异常：kube-proxy 未同步 Endpoint 导致 ClusterIP 不通
description: 专有云 ACK 集群因 kube-proxy 异常停止同步 Endpoint，导致 Service ClusterIP 与 NodePort
  均无法访问后端的工单闭环样本。
summary: 专有云 ACK 集群因 kube-proxy 异常停止同步 Endpoint，导致 Service ClusterIP 与 NodePort 均无法访问后端的工单闭环样本。
category: 生产运维/ticket-case
tags:
- ack
- zyy
- kube-proxy
- service
- clusterip
- nodeport
- p0
- network
tier: peripheral
created: '2026-06-26T15:30:00+08:00'
updated: '2026-06-26T18:00:00+08:00'
incident_id: INC-2026-ACK-019
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-07
affected_namespace: order-platform
ticket_type: 服务发现异常
skill_ref:
- Service 故障排查
- kube-proxy 原理
fta_ref:
- 'FTA: Service 访问不通'
last_updated: 2026-06-26 18:00:00+08:00
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- Service 访问异常：kube-proxy 未同步 Endpoint 导致 ClusterIP 不通 如何处理
trigger_keywords:
- Service
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
- target: '[[13-生产运维/05-工单案例/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
- target: '[[13-生产运维/05-工单案例/ticket-case-044-kubeproxy-service-unreachable.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户发现订单平台多个微服务之间调用超时，表现为 Pod 可以通过 DNS 解析到 Service 名称，但实际访问 ClusterIP 时连接超时。客户描述如下：

> “ACK 集群 ack-zyy-prod-07 的 order-platform 命名空间里，服务间调用突然大量超时。我们在一个 Pod 里 curl 另一个 Service 的 ClusterIP，直接连不上。但 Service 后端的 Pod 都是 Running 的，Endpoint 也存在。更奇怪的是同一节点上直接 curl Pod IP 是可以的，就是通过 Service IP 不行。麻烦尽快排查。”

受影响命名空间为 `order-platform`，核心 Service 包括 `order-service`、`payment-client`、`inventory-client`。受影响节点为 `cn-shanghai.172.20.4.11`、`cn-shanghai.172.20.4.12`，kube-proxy 模式为 `iptables`。

## 分类与优先级判定

- **工单类型**：服务发现异常 / kube-proxy 故障。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产环境服务间调用大面积失败，Pod IP 可达但 ClusterIP 不可达，指向 kube-proxy 或 Service 网络层问题。
2. 影响订单平台核心链路，可能导致下单失败。
3. 需在 15 分钟内定位受影响节点并恢复 Service 转发能力。

## 诊断步骤

按“先验证 Pod IP、再检查 Endpoint、再检查 iptables/nftables、最后看 kube-proxy 日志”的顺序排查：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 在异常 Pod 内测试 ClusterIP、Pod IP、DNS 解析
kubectl exec -n order-platform -it $(kubectl get pod -n order-platform -l app=debug -o jsonpath='{.items[0].metadata.name}') -- sh -c '
  nslookup order-service
  curl -m 5 -s -o /dev/null -w "clusterip:%{http_code}\n" http://order-service:8080/health
  curl -m 5 -s -o /dev/null -w "podip:%{http_code}\n" http://10.244.1.20:8080/health
'

# 2. 检查 Service 与 Endpoint 状态
kubectl get svc -n order-platform order-service -o yaml
kubectl get endpoints -n order-platform order-service -o yaml

# 3. 检查 kube-proxy Pod 状态
kubectl get pod -n kube-system -l k8s-app=kube-proxy -o wide

# 4. 查看 kube-proxy 日志，关注同步失败与 API Server 连接异常
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=300 | grep -iE "error|fail|endpoint|sync|reflector|watch"

# 5. SSH 到问题节点查看 iptables 规则是否包含 Service 链
ssh root@cn-shanghai.172.20.4.11 'iptables -t nat -L KUBE-SERVICES -n | grep order-service'
ssh root@cn-shanghai.172.20.4.11 'iptables -t nat -L KUBE-SVC-XXXX -n 2>/dev/null | head'

# 6. 检查 kube-proxy 健康检查与 metrics
kubectl get --raw /api/v1/namespaces/kube-system/pods/$(kubectl get pod -n kube-system -l k8s-app=kube-proxy -o jsonpath='{.items[0].metadata.name}'):10249/proxy/healthz

# 7. 检查 API Server 连接延迟与 kube-proxy 权限
kubectl auth can-i list endpoints --as=system:serviceaccount:kube-system:kube-proxy -n default

# 8. 对比正常节点与异常节点的 iptables 规则数量
ssh root@cn-shanghai.172.20.4.11 'iptables -t nat -L | wc -l'
ssh root@cn-shanghai.172.20.4.13 'iptables -t nat -L | wc -l'
```
## 根因分析

经过排查，发现异常节点 `cn-shanghai.172.20.4.11` 与 `cn-shanghai.172.20.4.12` 上的 kube-proxy Pod 处于 `Running` 状态，但日志中持续出现以下错误：

```
E0626 15:42:11.123456       1 reflector.go:138] k8s.io/client-go/informers/factory.go:134: Failed to watch *v1.Endpoints: unknown (get endpoints)
E0626 15:42:12.234567       1 proxier.go:1234] Failed to execute iptables-restore: exit status 4 (Another app is currently holding the xtables lock.)
```

根本原因为：
1. **RBAC 异常**：集群近期升级了 Kubernetes 版本并调整了 RBAC 策略，kube-proxy 使用的 ServiceAccount `kube-proxy` 对 `endpoints` 资源的 watch 权限被意外移除，导致 kube-proxy 无法实时感知 Endpoint 变化。
2. **iptables 锁竞争**：由于 Endpoint 无法同步，kube-proxy 反复尝试重载 iptables 规则，而节点上其他安全组件也频繁操作 iptables，导致 `xtables lock` 竞争，规则重载持续失败。
3. **旧规则残留**：新 Pod 扩容后，Service 的 Endpoint 列表已变化，但异常节点上的 iptables NAT 链仍指向已删除或已迁移的 Pod IP，因此 ClusterIP 访问失败，而直接访问 Pod IP 正常。

## 修复命令

**第一步：恢复 kube-proxy RBAC 权限**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: system:kube-proxy
rules:
- apiGroups: [""]
  resources: ["services", "endpoints", "nodes", "pods"]
  verbs: ["get", "list", "watch"]
EOF
```
**第二步：重启异常节点上的 kube-proxy Pod，重新建立 Endpoint watch**

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘

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
for pod in $(kubectl get pod -n kube-system -l k8s-app=kube-proxy -o jsonpath='{.items[?(@.spec.nodeName=="cn-shanghai.172.20.4.11")].metadata.name} {.items[?(@.spec.nodeName=="cn-shanghai.172.20.4.12")].metadata.name}'); do
  kubectl delete pod -n kube-system $pod --force --grace-period=0  # ⚠️ 跳过优雅终止，可能丢数据
done
```
**第三步：若 iptables 锁竞争严重，临时切换到 ipvs 模式（需评估后执行）**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 修改 kube-proxy ConfigMap
kubectl get configmap kube-proxy -n kube-system -o yaml | sed 's/mode: "iptables"/mode: "ipvs"/' | kubectl apply -f -
kubectl rollout restart daemonset kube-proxy -n kube-system
```
**第四步：临时在异常节点上手动清理冲突的 iptables 链并重新同步**

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
ssh root@cn-shanghai.172.20.4.11 '
  iptables-save > /tmp/iptables-backup-$(date +%s).txt
  iptables -t nat -F KUBE-SERVICES
  iptables -t nat -F KUBE-POSTROUTING
  iptables -t nat -F KUBE-FIREWALL
  systemctl restart kubelet
'
```
**第五步：隔离并驱逐异常节点上的关键业务 Pod，确保流量不再经过问题节点**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

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
for node in cn-shanghai.172.20.4.11 cn-shanghai.172.20.4.12; do
  kubectl cordon $node
  kubectl drain $node --ignore-daemonsets --delete-emptydir-data --force --timeout=300s
done
```
## 验证命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. kube-proxy Pod 全部 Running 且日志无 watch 失败
kubectl get pod -n kube-system -l k8s-app=kube-proxy -o wide
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=100 | grep -iE "error|fail|watch" || echo "no errors"

# 2. Service ClusterIP 可访问
kubectl exec -n order-platform -it $(kubectl get pod -n order-platform -l app=debug -o jsonpath='{.items[0].metadata.name}') -- \
  curl -m 5 -s -o /dev/null -w "%{http_code}\n" http://order-service:8080/health

# 3. iptables 规则包含最新 Endpoint
ssh root@cn-shanghai.172.20.4.11 'iptables -t nat -L KUBE-SERVICES -n | grep order-service'

# 4. Endpoint 列表与 iptables 后端一致
kubectl get endpoints -n order-platform order-service

# 5. 跨节点 Service 访问正常
kubectl run netshoot-$(date +%s) -n order-platform --rm -i --restart=Never --image nicolaka/netshoot -- \
  curl -m 5 -s -o /dev/null -w "%{http_code}\n" http://order-service.order-platform.svc.cluster.local:8080/health

# 6. RBAC 权限校验通过
kubectl auth can-i watch endpoints --as=system:serviceaccount:kube-system:kube-proxy -n default
```
## 回复客户话术

> 您好，经排查，本次 Service 访问异常的根因是 **部分节点上的 kube-proxy 因 RBAC 权限被意外移除，无法 watch Endpoints 变化，导致 iptables NAT 规则未同步最新后端 Pod IP**。因此 Pod IP 直接访问正常，但通过 ClusterIP/Service 域名访问失败。我们已完成以下处置：
>
> 1. 恢复 `system:kube-proxy` ClusterRole 对 services、endpoints、nodes、pods 的 get/list/watch 权限；
> 2. 重启异常节点上的 kube-proxy Pod，重新建立 Endpoint 监听；
> 3. 清理异常节点上的旧 iptables NAT 链，强制重新同步规则；
> 4. 隔离并驱逐异常节点上的业务 Pod，待 kube-proxy 稳定后恢复调度。
>
> 当前 Service ClusterIP 与 NodePort 访问已恢复正常，业务调用超时下降。建议后续：
> - 在集群升级或 RBAC 变更前使用 `kubectl auth can-i` 复核核心组件权限；
> - 配置 kube-proxy 同步延迟告警；
> - 考虑将 kube-proxy 模式从 iptables 迁移到 ipvs，降低大规模集群下的规则同步压力。
>
> 如有新异常，请随时联系。

## 复盘与沉淀

本次故障的隐蔽性在于 kube-proxy Pod 本身处于 `Running` 状态，且没有因 CrashLoopBackOff 而明显告警，但实际已停止同步 Endpoint。这类“静默失败”需要依赖 Service 层健康探测与跨 Pod 调用监控才能及时发现。

关键经验教训：
1. **RBAC 变更影响面大**：kube-proxy、CoreDNS、metrics-server 等核心组件的权限变更必须严格走变更评审，避免遗漏；
2. **iptables 模式在大规模集群下存在瓶颈**：规则数量随 Service/Endpoint 增加而增长，重载时容易触发 xtables lock 竞争；
3. **Service 健康探测不可或缺**：仅探测 Pod IP 无法发现 kube-proxy 转发异常，必须在 Pod 内通过 Service DNS/ClusterIP 进行探测。

后续 SOP 更新要点：
1. 集群升级后执行核心组件 RBAC 校验脚本，确保 kube-proxy、CoreDNS、scheduler、controller-manager 权限完整；
2. 配置告警：`kubeproxy_sync_proxy_rules_duration_seconds` P99 > 5s 或 `iptables_restore_failures_total` 增长触发 P1；
3. 在 Prometheus 中监控 Service 层可用性，使用 blackbox_exporter 或 Pod 内探测脚本；
4. 将本案例写入 Service 不通回复模板；
5. 评估 ipvs 模式迁移可行性，制定灰度切换方案。

最后，建议在专有云 ACK 环境中，将 kube-proxy 的日志级别调整为 `--v=4` 并集中采集到 SLS，便于在权限问题或同步延迟时快速定位。同时，将核心组件的 RBAC 纳入 GitOps 管理，禁止手动修改，所有变更通过 PR 与 diff 审核。

## 是否需要升级及交接信息

- **是否升级**：已止血并恢复，暂不需要升级；若 RBAC 权限被反复覆盖或集群升级脚本存在 Bug，需升级至 **Kubernetes 平台团队** 与 **ACK 产品支持**。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-019`
  - 根因：`system:kube-proxy` ClusterRole 缺少 endpoints watch 权限，导致 iptables 规则未同步
  - 影响集群：`ack-zyy-prod-07`
  - 影响命名空间：`order-platform`
  - 影响节点：`cn-shanghai.172.20.4.11`、`cn-shanghai.172.20.4.12`
  - 临时修复：恢复 RBAC、重启 kube-proxy、清理 iptables、驱逐业务 Pod
  - 长期方案：核心组件 RBAC 变更评审、Service 层探测、ipvs 模式评估
  - 待跟进：确认 RBAC 固化到 GitOps、排查权限被覆盖原因

## Related

- kube-proxy
- Service
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- kube-proxy 异常导致 Service 不通


<!-- risk-assessed -->
