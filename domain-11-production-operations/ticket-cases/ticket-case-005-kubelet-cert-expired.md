---
title: 证书过期导致 kubelet 无法连接 apiserver
description: 专有云 ACK 集群节点 kubelet 客户端证书过期，导致节点 NotReady、Pod 无法被调度的工单闭环样本。
summary: 专有云 ACK 集群节点 kubelet 客户端证书过期，导致节点 NotReady、Pod 无法被调度的工单闭环样本。
category: domain-11-production-operations/ticket-case
tags:
- ack
- zyy
- kubelet
- certificate
- apiserver
- notready
- control-plane
- p0
tier: supporting
created: '2026-06-26T07:00:00+08:00'
updated: '2026-06-26T09:45:00+08:00'
incident_id: INC-2026-ACK-005
priority: P0
severity: critical
affected_cluster: ack-zyy-prod-05
affected_namespace: kube-system
ticket_type: 控制面故障
skill_ref:
- kubelet 证书轮转
- K8s 证书管理
fta_ref:
- 'FTA: kubelet 证书过期'
last_updated: 2026-06-26 09:45:00+08:00
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 8min
intent_queries:
- 证书过期导致 kubelet 无法连接 apiserver 如何处理
trigger_keywords:
- 证书过期导致
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
relationships:
- target: '[[entities/kubelet.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-001-terway-eni-exhaustion.md]]'
  type: related_to
- target: '[[domain-11-production-operations/ticket-cases/ticket-case-002-java-oom-essd-iohang.md]]'
  type: related_to
- target: '[[skills/kubelet-certificate-rotation.md]]'
  type: related_to
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 工单描述

客户早上巡检发现集群 `ack-zyy-prod-05` 中 3 台工作节点同时变为 `NotReady`，控制台上看到节点状态为 `Unknown`，节点上的 Pod 虽未立即终止，但新 Pod 无法调度。客户描述：

> “早上 7 点监控报警，3 个节点突然 NotReady。ssh 到节点看 kubelet 日志，里面一直报 `x509: certificate has expired or is not yet valid`。我们没改过证书，是不是自动续期没生效？”

受影响集群 `ack-zyy-prod-05`，命名空间 `kube-system`，节点 `cn-zhangjiakou.172.16.5.31/32/33`。

## 分类与优先级判定

- **工单类型**：控制面故障 / 证书安全故障。
- **优先级**：P0。
- **严重级别**：critical。

判定依据：
1. 生产集群多节点同时 NotReady，影响调度能力，存在业务中断风险。
2. 根因为 kubelet 客户端证书过期，属于集群安全基础设施失效。
3. 需立即恢复节点证书并排查证书轮转机制，防止复发。

## 诊断步骤

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看节点状态
kubectl get nodes -l kubernetes.io/hostname~cn-zhangjiakou.172.16.5.3 -o wide

# 2. 查看 kubelet 日志关键报错
kubectl debug node/cn-zhangjiakou.172.16.5.31 -it --image=registry.aliyuncs.com/acs/busybox -- chroot /host journalctl -u kubelet -n 100 --no-pager | grep -i "x509|certificate|expire"

# 3. 检查 kubelet 客户端证书有效期
kubectl debug node/cn-zhangjiakou.172.16.5.31 -it --image=registry.aliyuncs.com/acs/busybox -- chroot /host openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates -subject

# 4. 检查 apiserver 证书有效期
kubectl debug node/cn-zhangjiakou.172.16.5.31 -it --image=registry.aliyuncs.com/acs/busybox -- chroot /host openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates -subject

# 5. 检查 kubelet 配置中证书轮转开关
kubectl debug node/cn-zhangjiakou.172.16.5.31 -it --image=registry.aliyuncs.com/acs/busybox -- chroot /host cat /var/lib/kubelet/config.yaml | grep -i rotate

# 6. 检查集群证书管理组件
kubectl get pod -n kube-system -l k8s-app=cert-manager
kubectl get csr --all-namespaces | grep -i kubelet | head -20

# 7. 检查 ACK 证书到期告警
aliyun cms DescribeMetricList --Namespace acs_k8s --MetricName k8s_cert_expire_days --RegionId cn-zhangjiakou --Dimensions "{\"cluster_id\":\"ack-zyy-prod-05\"}"
```
## 根因分析

kubelet 启动时使用 `/var/lib/kubelet/pki/kubelet-client-current.pem` 作为客户端证书与 apiserver 通信。该证书由 apiserver 的 `certificates.k8s.io` API 签发，默认有效期 1 年。受影响的 3 台节点证书已于 2026-06-25 23:59 过期。

正常情况下，kubelet 配置中 `rotateCertificates: true` 会在证书接近过期时自动发起 CSR 请求。但本次由于节点上的 `kube-controller-manager` 在 6 月初因一次配置变更被重启后，证书审批控制器 `csrapproving` 的自动批准规则被误关闭，导致 CSR 堆积未批，kubelet 未能自动续期。

根因：
1. kubelet 客户端证书过期；
2. 自动证书审批控制器未正常工作，导致 CSR 未批准；
3. 缺乏证书到期前的主动告警。

## 修复命令

**第一步：在节点上手动触发 kubelet 证书签名请求**

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
for node in cn-zhangjiakou.172.16.5.31 cn-zhangjiakou.172.16.5.32 cn-zhangjiakou.172.16.5.33; do
  kubectl debug node/$node -it --image=registry.aliyuncs.com/acs/busybox -- chroot /host rm -f /var/lib/kubelet/pki/kubelet-client-current.pem
  kubectl debug node/$node -it --image=registry.aliyuncs.com/acs/busybox -- chroot /host systemctl restart kubelet
done
```
**第二步：批量批准 pending 的 kubelet CSR**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get csr --sort-by='.metadata.creationTimestamp' | grep Pending | awk '{print $1}' | xargs -I {} kubectl certificate approve {}
```
**第三步：验证证书已更新**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
for node in cn-zhangjiakou.172.16.5.31 cn-zhangjiakou.172.16.5.32 cn-zhangjiakou.172.16.5.33; do
  echo "=== $node ==="
  kubectl debug node/$node -it --image=registry.aliyuncs.com/acs/busybox -- chroot /host openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates -subject
done
```
**第四步：恢复自动证书审批控制器**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl patch deployment kube-controller-manager -n kube-system --type='json' -p='[
  {"op": "replace", "path": "/spec/template/spec/containers/0/command", "value": ["kube-controller-manager","--cluster-name=ack-zyy-prod-05","--controllers=*,bootstrapsigner,tokencleaner,csrapproving,csrcleaner","--allocate-node-cidrs=true","--cluster-cidr=10.244.0.0/16"]}
]'
kubectl rollout status deployment/kube-controller-manager -n kube-system --timeout=300s
```
**第五步：配置证书到期告警**

```bash
aliyun cms PutContactGroup --ContactGroupName k8s-cert-oncall --ContactList "sre-aliyun"
aliyun cms PutResourceMetricRule --Name k8s_cert_expire_alert --Namespace acs_k8s --MetricName k8s_cert_expire_days --Threshold 30 --ComparisonOperator LessThanOrEqualTo
```

## 验证命令

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 节点恢复 Ready
kubectl get nodes -l kubernetes.io/hostname~cn-zhangjiakou.172.16.5.3 -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.conditions[?(@.type=="Ready")].status}{"\n"}{end}'

# 2. CSR 无堆积
kubectl get csr | grep Pending | wc -l

# 3. 新 Pod 可调度
kubectl run cert-test --image=registry.aliyuncs.com/acs/busybox --restart=Never -n default -- sleep 300
kubectl get pod cert-test -n default -o wide

# 4. kubelet 日志无证书报错
kubectl debug node/cn-zhangjiakou.172.16.5.31 -it --image=registry.aliyuncs.com/acs/busybox -- chroot /host journalctl -u kubelet -n 50 --no-pager | grep -i "x509|certificate" || echo "no cert error"

# 5. 证书有效期大于 300 天
kubectl debug node/cn-zhangjiakou.172.16.5.31 -it --image=registry.aliyuncs.com/acs/busybox -- chroot /host openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -text | grep "Not After"
```
## 回复客户话术

> 您好，集群 `ack-zyy-prod-05` 多节点 NotReady 的根因已确认：**kubelet 客户端证书过期，且自动 CSR 审批控制器被误关闭，导致证书未能自动轮转**。
>
> 已执行修复：
>
> - 删除过期证书并重启 kubelet，触发新的 CSR；
> - 批量批准 pending CSR，kubelet 获取新证书；
> - 恢复 `kube-controller-manager` 的 `csrapproving` 控制器；
> - 配置证书到期 30 天告警。
>
> 当前 3 台节点均已恢复 Ready，新 Pod 可正常调度。后续建议：
>
> - 每月检查一次集群 CSR 与证书有效期；
> - 将证书到期监控纳入 集群证书告警 基线；
> - 对控制面配置变更增加影响评估，避免误关闭自动审批控制器。

## 复盘与沉淀

Kubernetes 节点证书体系包括 kubelet 客户端证书、kubelet 服务器证书、apiserver 证书、etcd 证书以及 CA 证书等。kubelet 客户端证书由 `certificates.k8s.io` API 动态签发，默认有效期 1 年，依赖 `kube-controller-manager` 中的 `csrapproving` 控制器自动批准。若该控制器被误关闭，kubelet 会在证书过期前发起 CSR，但无法获得新证书，最终出现 `x509: certificate has expired or is not yet valid`。

本次故障的触发点是 6 月初控制面配置变更时，运维同学为排查其他问题修改了 `kube-controller-manager` 的启动参数，将 `--controllers` 写死为部分控制器，遗漏了 `csrapproving` 与 `csrcleaner`。虽然短期内集群运行正常，但随着证书到期，问题集中爆发。此类“延时故障”最难排查，因此所有控制面参数变更都必须经过变更评审，并验证控制器列表完整性。

在专有云 ACK 环境中，还可以利用 ACK 提供的证书管理功能：在 ACK 控制台“集群运维 → 证书管理”中查看证书到期时间，或调用 OpenAPI `aliyun cs GET /clusters/{ClusterId}/certificates` 获取集群证书状态。对于使用 kubeadm 部署的集群，可使用 `kubeadm certs check-expiration` 进行巡检。

建议建立以下机制：
1. 每月执行一次证书有效期巡检，关注 30 天内到期的证书；
2. 配置 `k8s_cert_expire_days <= 30` 的 P1 告警；
3. 控制面变更前使用 `kubectl get pod -n kube-system` 与 `kubectl get csr` 基线对比，确保 CSR 自动审批正常；
4. 将本案例写入 kubelet 证书过期回复模板。

后续整改清单：
1. 审查所有 ACK 集群 `kube-controller-manager` 启动参数，确保 `csrapproving` 启用；
2. 在变更窗口后 24 小时内检查 CSR 审批成功率；
3. 对证书过期告警配置短信 + 电话双重通知；
4. 建立控制面配置变更审批与回滚流程。

建议在 ACK 控制台启用“证书到期自动修复”实验性功能（如可用），或在集群中部署 cert-manager 等外部证书管理组件，作为原生 CSR 审批机制的补充。同时，将证书健康检查纳入 集群健康评分 体系，使证书风险与节点、网络、存储风险并列展示，避免安全类隐患被忽视。

此外，建议在控制面配置变更时使用 GitOps 管理 `kube-controller-manager` 静态 Pod 清单或 Deployment 参数，所有变更通过 PR 审批并保留审计日志，避免口头或临时命令导致参数遗漏。该实践可参考 控制面 GitOps 变更管理。

最后，建议在每次重大版本升级后执行证书有效期巡检，确保升级过程未对证书链造成意外影响。同时，将 CSR 自动审批状态纳入每日巡检日报，形成长效防护机制。

## 是否需要升级及交接信息

- **是否升级**：已止血并修复。因涉及控制面配置变更，已同步 **集群基础设施团队** 进行根因复盘。
- **交接信息**：
  - 故障单号：`INC-2026-ACK-005`
  - 根因：`kubelet 客户端证书过期 + CSR 自动审批关闭`
  - 影响节点：`cn-zhangjiakou.172.16.5.31/32/33`
  - 修复方式：证书清理与重启、CSR 批准、恢复控制器、配置告警
  - 待跟进：审查 6 月初控制面配置变更记录，完善变更评审 checklist

## Related

- kubelet
- 节点 NotReady：Terway ENI IP 耗尽
- Pod 持续 CrashLoopBackOff：Java OOM + ESSD IO hang
- [[skills/kubelet-certificate-rotation.md|kubelet 证书轮换机制]]


<!-- risk-assessed -->
