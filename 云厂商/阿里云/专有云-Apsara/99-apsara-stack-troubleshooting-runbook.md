---
title: 专有云（Apsara Stack）- 故障手册（Runbook）
description: 专有云飞天底座组件异常→K8s症状→排查命令→升级路径的可执行故障手册
summary: 阿里云专有云（Apsara Stack）专属故障手册，按飞天底座组件（伏羲/洛神/盘古/女娲/天基/ASO/RAM-RRSA/KMS/SLS/Prometheus）分章，提供「组件异常→K8s症状→排查命令→升级路径」的可执行 runbook。
category: cloud-provider
tags:
- alibaba-cloud
- apsara-stack
- private-cloud
- troubleshooting
- runbook
- fuxi
- luoshen
- pangu
- nuwa
- tianji
tier: core
sources:
- 阿里云专有云运维手册
- Apsara Stack 组件排障指南
created: 2026-07-23
last_updated: 2026-07-23
relationships:
- target: '[[云厂商/阿里云/apsara-stack-components.md]]'
  type: related_to
- target: '[[云厂商/阿里云/06-阿里云专有云远程顾问指南.md]]'
  type: related_to
- target: '[[云厂商/阿里云/专有云-Apsara/253-apsara-tianji-aso-operations.md]]'
  type: related_to
- target: '[[云厂商/阿里云/专有云-Apsara/256-apsara-pangu-storage-troubleshooting.md]]'
  type: related_to
difficulty: advanced
audience:
- SRE
- 远程顾问
- 驻场运维
- 平台工程师
estimated_read_time: 25min
intent_queries:
- 专有云故障怎么排查
- Pod Pending 是哪个底座组件问题
- 专有云网络异常怎么定位
- 专有云什么时候联系 TAM
trigger_keywords:
- 故障手册
- runbook
- Pending
- NotReady
- 底座组件
- 升级 TAM
prerequisites:
- alicloud-basics
- k8s-troubleshooting
- tianji-aso-operations
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 专有云（Apsara Stack）- 故障手册（Runbook）

本手册面向在客户数据中心运维 [[云厂商/阿里云/01-专有云架构概述.md|阿里云专有云（Apsara Stack）]] 的 SRE 与远程顾问，将常见 K8s 工单现象与飞天底座组件建立双向映射，提供「**组件异常 → K8s 症状 → 排查命令 → 升级路径**」的可执行 runbook。

> **核心原则**：专有云中很多 K8s 症状并非 ACK 自身 Bug，而是底座组件（伏羲/洛神/盘古/女娲/天基/ASO）异常在容器层的映射。排障时需**同时查看 ACK 组件日志与底座组件状态**。

---

## 0. 通用排障流程（先看这个）

无论什么症状，按此顺序快速定位：

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 第 0 步：集群基础健康
kubectl get nodes -o wide
kubectl get --raw=/livez && kubectl get --raw=/readyz
# 第 1 步：异常 Pod
kubectl get pods -A --field-selector status.phase!=Running,status.phase!=Succeeded
# 第 2 步：近期事件
kubectl get events -A --sort-by='.lastTimestamp' | tail -50
# 第 3 步：系统组件
kubectl get pods -n kube-system | grep -vE 'Running|Completed'
```

然后按症状在下方找到对应组件章节。

---

## 1. 伏羲（Fuxi）调度异常

| 项 | 内容 |
|----|------|
| **典型 K8s 症状** | Pod 长期 `Pending`；`FailedScheduling` 显示 `0/xx nodes available`；Cluster Autoscaler 无法弹节点；热迁移失败 |
| **底层含义** | 伏羲负责 ECS/神龙实例创建、迁移、释放；资源池不足或调度策略异常 |
| **排查命令** | `kubectl describe pod <pod>` 看 Events；ASO `底座运维 > 调度 > 伏羲` 查资源池水位 |
| **升级触发** | 全局资源池不足长期 Pending → 联系 TAM 释放/扩容资源 |

```bash
# 🟢 低风险：只读
kubectl describe pod <pod-name> | grep -A20 Events
# FailedScheduling 原因（资源不足/污点/亲和性）
kubectl get events -A --field-selector reason=FailedScheduling
# 节点资源水位
kubectl top nodes
kubectl describe node <node> | grep -A10 Allocatable
```

---

## 2. 洛神（Luoshen）网络异常

| 项 | 内容 |
|----|------|
| **典型 K8s 症状** | 跨节点 Pod 不通；Service ClusterIP 异常；DNS 解析失败；Ingress 502/504；全集群网络异常 |
| **底层含义** | 洛神提供 VPC、SLB、EIP、Terway 网络能力；路由/VXLAN/ENI 异常 |
| **排查命令** | `kubectl run debug --image=nicolaka/netshoot`；检查 VPC 路由表、VSwitch、安全组 |
| **升级触发** | 全集群网络异常/洛神控制面异常 → TAM 立即升级（P0） |

```bash
# 🟢 低风险：只读（debug pod 临时创建）
# 跨节点连通性测试
kubectl run debug --image=nicolaka/netshoot --rm -it --restart=Never -- \
  ping <target-pod-ip>
# Terway Daemon 日志（CNI 层）
kubectl logs -n kube-system -l app=terway-daemon --tail=200
# Service/Ingress 状态
kubectl get svc,ingress -A
# 节点网络（驻场执行）
ip route
ip link | grep -E 'eth|eni'
```

---

## 3. 盘古（Pangu）存储异常

| 项 | 内容 |
|----|------|
| **典型 K8s 症状** | 所有 PVC `Pending`；ESSD IO 延迟飙升；NAS 无响应；快照/扩容全局失败 |
| **底层含义** | 盘古是 ESSD/NAS/OSS/CPFS 专有云底层实现；集群水位/副本/ChunkServer 异常 |
| **排查命令** | `kubectl get pvc,pv -A`；节点 `iostat -x`；天基 `盘古集群健康` |
| **升级触发** | 盘古集群异常 → 联系存储团队/TAM（禁止自行操作） |

> 详细排障见 [[云厂商/阿里云/专有云-Apsara/256-apsara-pangu-storage-troubleshooting.md|256 盘古存储排障]]。

```bash
# 🟢 低风险：只读
kubectl get pvc -A --field-selector status.phase!=Bound
kubectl get pv | grep -v Bound
# 节点 IO（驻场）
iostat -x 1 5
# CSI 插件日志
kubectl logs -n kube-system -l app=csi-plugin -c csi-plugin --tail=200
```

---

## 4. 女娲（Nuwa）一致性异常

| 项 | 内容 |
|----|------|
| **典型 K8s 症状** | APIServer 响应极慢/频繁超时；etcd Leader 频繁切换；集群状态不一致；控制台操作无响应 |
| **底层含义** | 女娲承载部分控制面元数据；一致性服务异常影响管控面 |
| **排查命令** | `kubectl get --raw=/healthz`；`etcdctl endpoint status`；天基 `一致性服务 > 女娲` |
| **升级触发** | 高危，**禁止自行重启** → 联系 TAM/驻场 |

```bash
# 🟢 低风险：只读
kubectl get --raw=/healthz
# etcd 状态（专有版自管 etcd）
ETCDCTL_API=3 etcdctl endpoint status \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key -w table
# 看 Leader 是否频繁切换（is_learner / raft_term / raft_index）
```

> ⚠️ 女娲异常是最高危场景之一，任何操作前必须联系 TAM。自行重启可能导致元数据损坏。

---

## 5. 天基（Tianji）/ ASO 异常

| 项 | 内容 |
|----|------|
| **典型 K8s 症状** | 控制台无法登录/操作无响应；Addon Pod 版本不一致；ACK 控制台功能缺失；升级任务卡死；告警丢失 |
| **底层含义** | 天基托管管控面、负责配置下发/自愈；ASO 是运维入口 |
| **排查命令** | `kubectl get pods -n tianji-system`；天基 `运维大盘`；ASO `变更中心` |
| **升级触发** | 天基/ASO 自身故障 → 驻场工程师（底座核心，禁止客户自行操作） |

> 详细流程见 [[云厂商/阿里云/专有云-Apsara/253-apsara-tianji-aso-operations.md|253 天基/ASO 运维流程]]。

```bash
# 🟢 低风险：只读
kubectl get pods -n tianji-system
kubectl get pods -n tianji-system -o wide | grep -iv running
# ASO 变更中心是否有卡住任务（控制台查看）
```

---

## 6. RAM / RRSA 异常

| 项 | 内容 |
|----|------|
| **典型 K8s 症状** | CCM 无法创建 SLB；CSI 无法创建云盘；Pod 访问 OSS/SLS/KMS 报 `InvalidAccessKeyId.NotFound`；凭证过期 |
| **底层含义** | RAM 角色/策略错误；RRSA（Pod 级 RAM Role）OIDC 联邦异常 |
| **排查命令** | `kubectl logs -n kube-system -l app=cloud-controller-manager`；Pod 内验证 STS |
| **升级触发** | 根证书/密钥问题 → 阿里云安全团队 |

```bash
# 🟢 低风险：只读
# CCM/CSI 权限错误日志
kubectl logs -n kube-system -l app=cloud-controller-manager --tail=100
kubectl logs -n kube-system -l app=csi-plugin -c csi-plugin --tail=100
# Pod 内验证 RRSA 身份（应用 Pod 内执行）
# curl -s "http://100.100.100.200/latest/meta-data/ram/security-credentials/<role>"
# aliyun sts GetCallerIdentity
# 检查 ServiceAccount RRSA 注解
kubectl get sa <sa> -n <ns> -o yaml | grep -i 'oidc\|ram'
```

---

## 7. KMS 异常

| 项 | 内容 |
|----|------|
| **典型 K8s 症状** | 加密 PVC 创建失败；Secret 解密报错；证书加解密失败 |
| **底层含义** | KMS 密钥不可用/过期；kms-plugin 异常 |
| **排查命令** | `kubectl describe pvc`；`kubectl logs kms-plugin` |
| **升级触发** | KMS 根证书/密钥问题 → 阿里云安全团队 |

```bash
# 🟢 低风险：只读
kubectl get pods -n kube-system | grep kms
kubectl logs -n kube-system -l app=kms-plugin --tail=100
# KMS 密钥状态（驻场 aliyun CLI）
aliyun kms DescribeKey --KeyId kms-xxx --endpoint kms.aliyuncs.com
```

---

## 8. SLS 日志异常

| 项 | 内容 |
|----|------|
| **典型 K8s 症状** | 容器标准输出日志缺失/延迟；审计日志查不到；Logtail OOM；`ShardReadQuotaExceed` |
| **底层含义** | SLS 服务端异常；Logtail Agent 异常；Shard 配额/索引问题 |
| **排查命令** | `kubectl logs logtail`；SLS Project/Logstore/Shard 状态 |
| **升级触发** | SLS 服务端异常 → TAM；索引重建 → TAM |

> 详细见 [[云厂商/阿里云/专有云-Apsara/251-apsara-stack-sls-logging.md|251 SLS 日志服务]]。

```bash
# 🟢 低风险：只读
kubectl get pods -n kube-system | grep logtail
kubectl logs -n kube-system -l app=logtail --tail=100
# SLS 状态（驻场 aliyun CLI）
aliyun log get_project --project_name <project> --endpoint <sls-endpoint>
```

---

## 9. Prometheus / Grafana 异常

| 项 | 内容 |
|----|------|
| **典型 K8s 症状** | Grafana 大盘无数据；告警规则不触发；Remote Write 失败；`DatasourceError` |
| **底层含义** | Prometheus 实例异常；Agent 抓取失败；Grafana 数据源配置错误 |
| **排查命令** | `kubectl get pods -n arms-prom`；Prometheus 实例状态 |
| **升级触发** | Prometheus 服务端异常 → TAM；Dashboard 变更可自助 |

```bash
# 🟢 低风险：只读
kubectl get pods -n arms-prom
kubectl logs -n arms-prom <prometheus-agent-pod> --tail=100
```

---

## 10. 综合决策表：症状 → 组件 → 升级

| K8s 工单现象 | 优先排查组件 | 升级触发条件 |
|--------------|--------------|--------------|
| Pod 一直 `Pending` | 伏羲、ECS、VSwitch、Terway | 全局资源池不足 |
| 节点 `NotReady` | 伏羲、ECS、神龙、洛神 | 宿主机/网络设备故障 |
| `LoadBalancer` Service 无 IP | SLB/NLB/ALB、CCM、RAM | 底层负载均衡调度异常 |
| Ingress 访问 502/504 | SLB/ALB、Terway、后端 Pod | ALB 控制面异常 |
| PVC 无法绑定 | ESSD/NAS/OSS CSI、盘古 | 盘古集群异常 |
| Pod 挂载云盘失败 | ESSD、CSI、伏羲 | 存储网关异常 |
| Pod 无法访问 OSS/SLS/KMS | RRSA、RAM、KMS | KMS/RRSA 服务端异常 |
| 容器标准输出丢失 | SLS、Logtail | SLS 服务端异常 |
| 监控大盘无数据 | Prometheus、Grafana、SLS | Prometheus 服务端异常 |
| 控制台无法登录/操作 | ASO、天基、女娲 | 天基/ASO 自身故障 |
| 全集群网络异常 | 洛神、VPC、Terway | 洛神控制面异常 |
| 全集群存储 IO 异常 | 盘古、ESSD CSI | 盘古集群异常 |
| APIServer 极慢/超时 | 女娲、etcd | 女娲/etcd 异常（高危） |

---

## 11. 何时联系 TAM / 驻场工程师（汇总）

| 操作类型 | 处理方 | 说明 |
|----------|--------|------|
| ACK 版本升级/补丁回滚 | TAM + 客户窗口 | 需评估业务影响 |
| 天基/ASO 自身升级或重启 | 驻场工程师 | 底座核心，禁止客户自行操作 |
| 盘古/女娲/伏羲/洛神集群变更 | 驻场工程师 | 影响范围大，需专家评估 |
| 神龙固件/MOC 驱动/BIOS 升级 | 驻场工程师 | 需进入机房或 BMC |
| 全局网络/存储中断 | TAM 立即升级 | 通常触发 P0 响应 |
| RAM/RRSA/KMS 根证书或密钥问题 | 阿里云安全团队 | 涉及身份信任根 |
| ASO 控制台无法登录且影响排障 | 驻场工程师 | 先通过天基 CLI 或后台恢复 |

---

## 12. 一键诊断包（客户执行，输出给顾问）

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 专有云 ACK 一键诊断（驻场/堡垒机执行，打包给远程顾问）
DIAG_DIR="/tmp/apsara-diag-$(date +%Y%m%d-%H%M%S)"
mkdir -p $DIAG_DIR/{cluster,nodes,network,storage,logs}

# 集群
kubectl cluster-info > $DIAG_DIR/cluster/cluster-info.txt 2>&1
kubectl version > $DIAG_DIR/cluster/version.txt 2>&1
kubectl get nodes -o wide > $DIAG_DIR/cluster/nodes.txt 2>&1
kubectl get pods -A -o wide > $DIAG_DIR/cluster/all-pods.txt 2>&1
kubectl get events -A --sort-by='.lastTimestamp' > $DIAG_DIR/cluster/events.txt 2>&1

# 节点
for n in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
  kubectl describe node $n > $DIAG_DIR/nodes/$n.txt 2>&1
done

# 网络
kubectl get svc,ingress,networkpolicy -A > $DIAG_DIR/network/net.txt 2>&1
kubectl logs -n kube-system -l app=terway-daemon --tail=200 > $DIAG_DIR/network/terway.log 2>&1

# 存储
kubectl get pv,pvc -A > $DIAG_DIR/storage/pv-pvc.txt 2>&1
kubectl get sc > $DIAG_DIR/storage/sc.txt 2>&1
kubectl logs -n kube-system -l app=csi-plugin -c csi-plugin --tail=200 > $DIAG_DIR/storage/csi.log 2>&1

# 管控面
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=200 > $DIAG_DIR/logs/coredns.log 2>&1
kubectl logs -n kube-system -l component=kube-apiserver --tail=200 > $DIAG_DIR/logs/apiserver.log 2>&1 2>/dev/null
kubectl get pods -n tianji-system -o wide > $DIAG_DIR/cluster/tianji.txt 2>&1

tar czf $DIAG_DIR.tar.gz -C $(dirname $DIAG_DIR) $(basename $DIAG_DIR)
echo "诊断包已生成: $DIAG_DIR.tar.gz — 请发送给远程顾问"
```

> **安全**：诊断包不含 AccessKey/SecretKey（绝不收集）；Pod 日志脱敏后再发送。详见 [[云厂商/阿里云/06-阿里云专有云远程顾问指南.md|06 远程顾问指南]]。

---

## 相关文档

- [[云厂商/阿里云/apsara-stack-components.md|Apsara Stack 组件索引]]
- [[云厂商/阿里云/06-阿里云专有云远程顾问指南.md|06 专有云远程顾问指南]]
- [[云厂商/阿里云/专有云-Apsara/253-apsara-tianji-aso-operations.md|253 天基/ASO 运维]]
- [[云厂商/阿里云/专有云-Apsara/254-apsara-upgrade-patch-management.md|254 升级与补丁管理]]
- [[云厂商/阿里云/专有云-Apsara/255-apsara-compliance-hardening.md|255 合规加固]]
- [[云厂商/阿里云/专有云-Apsara/256-apsara-pangu-storage-troubleshooting.md|256 盘古存储排障]]

## Related

- [[实体/coredns.md|CoreDNS]]
- [[实体/etcd.md|etcd]]
- [[故障诊断/README.md|故障诊断域]]

<!-- risk-assessed -->
