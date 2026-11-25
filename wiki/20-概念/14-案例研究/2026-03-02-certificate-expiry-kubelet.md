---
title: '[2026-03-02] [P0] 证书过期导致 kubelet 无法上报心跳'
summary: '[2026-03-02] [P0] 证书过期导致 kubelet 无法上报心跳：06:45，运维人员发现 prod-logistics namespace
  大量 Pod 进入 Terminating 状态，新 Pod 卡在 Pending。Grafana 显示 4 个节点同时变为 NotReady。'
category: case-study
tags:
- production
- incident
- security
- certificate
- kubelet
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-03-02'
severity: P0
mttr: 25min
status: resolved
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [2026-03-02] Kubelet 客户端证书过期导致 120+ Pod 被驱逐

## 工单信息
- **工单编号**: INC-2026-0302-005
- **发现时间**: 2026-03-02 06:45 UTC
- **恢复时间**: 2026-03-02 07:10 UTC
- **影响范围**: 4 个节点，126 个 Pod，覆盖 `prod-logistics` 和 `prod-warehouse`
- **业务影响**: 物流追踪接口不可用，仓库 WMS 系统宕机 25 分钟

## 问题现象
06:45，运维人员发现 `prod-logistics` namespace 大量 Pod 进入 `Terminating` 状态，新 Pod 卡在 `Pending`。Grafana 显示 4 个节点同时变为 `NotReady`。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get nodes
# NAME                        STATUS     ROLES    AGE   VERSION
# ip-10-0-7-21.ec2.internal   NotReady   <none>   89d   v1.29.4
# ip-10-0-7-22.ec2.internal   NotReady   <none>   89d   v1.29.4
# ip-10-0-7-23.ec2.internal   NotReady   <none>   89d   v1.29.4
# ip-10-0-7-24.ec2.internal   NotReady   <none>   89d   v1.29.4
```
## 诊断过程

**06:47** — 检查 kubelet 日志：
```bash
ssh ip-10-0-7-21 "journalctl -u kubelet -n 50 --no-pager"
# Mar 02 06:43:12 ip-10-0-7-21 kubelet[1823]: E0302 06:43:12.112233 ...
#   "Unable to rotate certificates" err="no valid client certificate"
# Mar 02 06:43:15 ip-10-0-7-21 kubelet[1823]: E0302 06:43:15.223344 ...
#   "Failed to connect to API server" err="x509: certificate has expired or is not yet valid"
```

**06:49** — 检查证书有效期：
```bash
ssh ip-10-0-7-21 "openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates"
# notAfter=Mar  2 06:42:00 2026 GMT

# 当前时间
ssh ip-10-0-7-21 "date -u"
# Mon Mar  2 06:45:00 UTC 2026
```

**06:50** — 检查证书轮换配置：
```bash
ssh ip-10-0-7-21 "cat /var/lib/kubelet/config.yaml | grep -A3 rotateCertificates"
# rotateCertificates: true

# 但 kubelet 启动参数中缺少 --rotate-server-certificates
ssh ip-10-0-7-21 "ps aux | grep kubelet | grep rotate-server-certificates"
# （无输出）
```

**06:52** — 检查 CSR 状态：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get csr | grep Pending
# NAME        AGE   SIGNERNAME                                    REQUESTOR            CONDITION
# csr-7a2bc   5m    kubernetes.io/kube-apiserver-client-kubelet   system:node:ip-10-0-7-21   Pending
# csr-9x3pq   5m    kubernetes.io/kube-apiserver-client-kubelet   system:node:ip-10-0-7-22   Pending
```
发现 kubelet 客户端证书已于 06:42 过期，kubelet 尝试申请新的 CSR，但由于 `kube-controller-manager` 的 `--cluster-signing-cert-file` 和 `--cluster-signing-key-file` 参数在 01-15 的升级中被误移除，导致 CSR 无法自动批准。

## 根因
1. kubelet 客户端证书有效期为 1 年（kubeadm 默认）
2. 证书于 2026-03-02 06:42 过期
3. kubelet 尝试自动轮换，但 CSR 因 controller-manager 缺少签名证书参数而无法批准
4. 4 个节点的 kubelet 同时失去与 API Server 的 TLS 连接，节点变为 NotReady
5. Pod 被驱逐，业务中断

## 修复动作

**06:55** — 临时手动批准 CSR：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
for csr in $(kubectl get csr | grep Pending | awk '{print $1}'); do
  kubectl certificate approve $csr
done

kubectl get csr | grep Approved
# csr-7a2bc   8m   ...   Approved,Issued
# csr-9x3pq   8m   ...   Approved,Issued
```
**06:58** — 节点恢复 Ready：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get nodes
# NAME                        STATUS   ROLES    AGE   VERSION
# ip-10-0-7-21.ec2.internal   Ready    <none>   89d   v1.29.4
# ...
```
**07:00** — 修复 controller-manager 配置，恢复自动签名：
```bash
# 编辑 kube-controller-manager 静态 Pod 清单
sudo vi /etc/kubernetes/manifests/kube-controller-manager.yaml
# 添加参数：
#   - --cluster-signing-cert-file=/etc/kubernetes/pki/ca.crt
#   - --cluster-signing-key-file=/etc/kubernetes/pki/ca.key
```

**07:05** — 验证 CSR 自动批准：

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
# 删除一个节点的 kubelet 客户端证书，触发轮换
ssh ip-10-0-7-21 "rm /var/lib/kubelet/pki/kubelet-client-current.pem && systemctl restart kubelet"

# 观察 CSR 自动批准
kubectl get csr | grep system:node:ip-10-0-7-21
# csr-xyz123   30s   ...   Approved,Issued
```
**07:08** — 为所有节点提前轮换证书：

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
for node in ip-10-0-7-21 ip-10-0-7-22 ip-10-0-7-23 ip-10-0-7-24; do
  ssh $node "rm /var/lib/kubelet/pki/kubelet-client-current.pem && systemctl restart kubelet"
  sleep 5
done
```
## 验证
- 07:09 — 所有节点 Ready，Pod 全部 Running
- 07:10 — 物流追踪和 WMS 系统恢复，业务指标正常

## 复盘
- **直接原因**: kubelet 客户端证书过期 → kubelet 无法连接 API Server → 节点 NotReady → Pod 驱逐
- **根本原因**: 
  1. controller-manager 升级时误移除签名参数，导致 CSR 自动批准失效
  2. 缺少证书过期提前告警
- **改进措施**:
  1. 部署证书过期监控：`certificate_expiry_time < 30d` 触发 P1 告警，`< 7d` 触发 P0
  2. 所有证书相关变更必须经过安全团队评审
  3. 每月执行 `kubeadm certs check-expiration` 并输出到 Slack
  4. 为 kubelet 启用 `--rotate-server-certificates` 和 `--rotate-certificates`
- **相关 Skill**: [[kubelet-certificate-rotation]]
- **相关 FTA**: [[certificate-fta]]


<!-- risk-assessed -->
