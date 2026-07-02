---
title: '[2026-01-15] [P0] Node NotReady 导致大规模 Pod 驱逐'
summary: '[2026-01-15] [P0] Node NotReady 导致大规模 Pod 驱逐：09:23，PagerDuty 连续触发 3 条高优告警：'
category: case-study
tags:
- production
- incident
- cluster-fundamentals
- node
- pod
- eviction
tier: core
created: '2026-05-23'
updated: '2026-05-23'
date: '2026-01-15'
severity: P0
mttr: 18min
status: resolved
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [2026-01-15] Node NotReady 导致 47 个 Pod 被驱逐

## 工单信息
- **工单编号**: INC-2026-0115-001
- **发现时间**: 2026-01-15 09:23 UTC
- **恢复时间**: 2026-01-15 09:41 UTC
- **影响范围**: 3 个 namespace (`prod-order`, `prod-payment`, `prod-inventory`), 47 个 Pod
- **业务影响**: 订单服务降级 18 分钟，支付成功率跌至 12%

## 问题现象
09:23，PagerDuty 连续触发 3 条高优告警：
- `K8sNodeNotReady` — node `ip-10-0-4-17.ec2.internal`
- `PodEvictionRateHigh` — 47 pods evicted in 120s
- `OrderServiceErrorRate` — 5xx rate > 15%

用户反馈：下单页面转圈，支付回调超时，库存查询返回 503。

## 诊断过程

**09:24** — 值班工程师登录集群，先执行快速诊断：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get nodes
# NAME                        STATUS     ROLES    AGE   VERSION
# ip-10-0-4-17.ec2.internal   NotReady   <none>   45d   v1.29.4
# ... 其余节点 Ready

kubectl describe node ip-10-0-4-17.ec2.internal | grep -A5 Conditions
# Conditions:
#   Ready                False   ...  KubeletNotReady
```
**09:25** — 查看 kubelet 日志：
```bash
journalctl -u kubelet -n 200 --no-pager
# Jan 15 09:22:12 ip-10-0-4-17 kubelet[1823]: E0115 09:22:12.334182 ...
#   "node runtime is down" err="failed to get sandbox container info: ..."
# Jan 15 09:22:15 ip-10-0-4-17 kubelet[1823]: E0115 09:22:15.112033 ...
#   "container runtime status check may not have completed yet"
```

**09:27** — 检查 containerd 状态：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
systemctl status containerd
# ● containerd.service - containerd container runtime
#    Active: failed (Result: exit-code) since Mon 2026-01-15 09:22:08 UTC
# ...

journalctl -u containerd -n 100 --no-pager
# Jan 15 09:22:08 ip-10-0-4-17 containerd[1024]: ...
#   "exec: \"runc\": executable file not found in $PATH"
```
**09:28** — 发现 `/usr/local/sbin/runc` 被意外删除（前一日安全扫描脚本误清理）。containerd 重启失败，kubelet 报 NotReady，节点进入 `NotReady` 状态后触发 Pod 驱逐。

## 根因
安全合规扫描脚本 `/opt/security/cleanup-old-binaries.sh` 在 01-14 23:00 运行时将 `runc` 判定为"未签名二进制"并删除。该脚本缺少白名单机制，误删了 containerd 依赖的 `runc` 运行时。

## 修复动作

**09:29** — 从镜像仓库恢复 runc：
```bash
# 从已知正常节点复制
scp root@ip-10-0-4-18:/usr/local/sbin/runc /usr/local/sbin/runc
chmod +x /usr/local/sbin/runc

# 验证版本
runc --version
# runc version 1.1.12
```

**09:30** — 重启 containerd 和 kubelet：

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
systemctl restart containerd
systemctl restart kubelet

# 验证
kubectl get node ip-10-0-4-17.ec2.internal
# NAME                        STATUS   ROLES    AGE   VERSION
# ip-10-0-4-17.ec2.internal   Ready    <none>   45d   v1.29.4
```
**09:35** — 被驱逐的 Pod 由 Deployment/StatefulSet 控制器自动重建：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n prod-order | grep -c Running
# 15
kubectl get pods -n prod-payment | grep -c Running
# 12
```
## 验证
- 09:38 — Prometheus 查询 `order_service_5xx_rate` 恢复至 0.02%
- 09:39 — 支付成功率回升至 99.7%
- 09:41 — 全部业务指标恢复正常

## 复盘
- **直接原因**: 安全脚本误删 `runc` 二进制 → containerd 崩溃 → kubelet NotReady → Pod 驱逐
- **根本原因**: 安全脚本缺少容器运行时组件白名单，变更未经过集群 SRE 评审
- **改进措施**:
  1. 安全脚本添加 `/usr/local/sbin/runc`、`/usr/local/bin/containerd*` 白名单
  2. 所有节点级变更必须经过金丝雀节点验证（≥24h）
  3. 为 containerd 添加 `systemd` 健康检查：`ExecStartPost=/usr/bin/containerd config dump | grep runc`
- **相关 Skill**: [[skill-k8s-node-notready-SKILL]]
- **相关 FTA**: [[node-fta]]


<!-- risk-assessed -->
