---
title: 阿里云 ACK 生产运行手册
description: 面向 SRE 的阿里云 ACK 集群全生命周期生产运维、Terway、RAM/RRSA、自动伸缩、升级、灾备、SLS 监控、成本治理与故障排查 Runbook
category: cloud-provider
tags:
- production
- best-practices
- playbook
- cloud-provider
- alicloud-ack
- ack
- aliyun
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 云架构师
estimated_read_time: 25min
intent_queries:
- 阿里云 ACK 生产运行手册是什么
- 如何运维阿里云 ACK 生产集群
- ACK Terway RAM RRSA 升级与灾备怎么做
trigger_keywords:
- 阿里云 ACK
- ACK
- Terway
- RAM
- RRSA
- 自动伸缩
- SLS
- 成本治理
prerequisites:
- kubectl-basics
- aliyun-cli-basics
- ack-basics
- networking-basics
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

# 阿里云 ACK 生产运行手册

> **适用范围**: 阿里云 ACK（托管版/专有版）上运行生产负载的集群，覆盖集群生命周期、Terway 网络、RAM/RRSA、自动伸缩、升级、灾备、SLS 监控与成本治理。  
> **目标读者**: SRE、平台工程师、阿里云云架构师。  
> **最后更新**: 2026-07-01

本手册聚焦阿里云 ACK 生产运维的可执行命令与标准流程。建议与 [[domain-12-cloud-providers/99-production-readiness-operations-guide.md|云厂商托管 Kubernetes 生产就绪运维指南]] 配套使用。

---

## 1. 适用场景与范围

- **适用场景**:
  - ACK 生产集群创建、网络规划与基线加固。
  - Terway ENI/IP 管理、NetworkPolicy 与多可用区部署。
  - RAM/RRSA 工作负载身份配置与审计。
  - 节点池自动伸缩、集群升级、节点维护。
  - 灾备、备份、跨区域恢复。
  - SLS 日志、ARMS/Prometheus 监控与 FinOps。
- **不适用场景**: 其他云厂商 Kubernetes 或自建 Kubeadm 集群。

---

## 2. 前置条件与工具

| 工具/资源 | 版本/要求 | 用途 |
|---|---|---|
| 阿里云 CLI | ≥ 3.0 | 集群、VPC、RAM 管理 |
| kubectl | 与 ACK 版本匹配 | K8s 资源管理 |
| Helm 3 | ≥ 3.12 | 组件部署 |
| Velero | 已部署 | 跨集群备份恢复 |
| 日志服务 SLS | 已接入 | 日志采集与分析 |
| ARMS Prometheus / Grafana | 已接入 | 指标监控 |

```bash
# 验证工具
aliyun --version
kubectl version --client
```

---

## 3. 核心概念

### 3.1 ACK 集群形态

- **ACK 托管版**: 阿里云管理 Master 节点，用户管理 Worker 节点，推荐生产使用。
- **ACK 专有版**: 用户独占 Master 节点，适用于强合规场景。

### 3.2 Terway 网络

- Terway 为 Pod 分配 VPC 子网 IP 或独立 ENI。
- 相比 Flannel，Terway 支持 NetworkPolicy、固定 IP、更高网络性能。

### 3.3 RAM/RRSA

- **RRSA (RAM Roles for Service Accounts)**: 为 Pod 绑定 RAM Role，实现工作负载级最小权限，替代节点 RAM Role。

---

## 4. 标准操作流程

### 4.1 集群创建（推荐参数）

```bash
# 通过 aliyun CLI 创建 ACK 托管版集群
aliyun cs POST /clusters \
  --body '{
    "name": "prod-ack-shanghai",
    "region_id": "cn-shanghai",
    "cluster_type": "ManagedKubernetes",
    "kubernetes_version": "1.32.1-aliyun-1",
    "vpc_id": "vpc-xxx",
    "vswitch_ids": ["vsw-xxx1","vsw-xxx2","vsw-xxx3"],
    "num_of_nodes": 3,
    "node_cidr_mask": 25,
    "service_cidr": "172.21.0.0/20",
    "pod_vswitch_ids": ["vsw-yyy1","vsw-yyy2","vsw-yyy3"],
    "timezone": "Asia/Shanghai",
    "tags": [{"key":"Environment","value":"production"},{"key":"Team","value":"platform"}]
  }'
```

### 4.2 Terway 网络检查与 IP 管理

```bash
# 查看 Terway DaemonSet
kubectl get ds -n kube-system terway-daemon

# 进入 terway Pod 查看 ENI/IP 余量
kubectl exec -n kube-system ds/terway-daemon -- terway-cli show

# 查看 Pod 使用的 IP
kubectl get pods -A -o wide

# 监控 ENI 余量告警（PromQL 示例）
# terway_eni_available < 20
```

### 4.3 RAM/RRSA 配置

```bash
# 1. 在 ACK 控制台或 CLI 开启 RRSA
aliyun cs POST /clusters/<cluster-id>/rrsa \
  --body '{"enabled": true}'

# 2. 创建 RAM OIDC Provider 与 Role（通常在控制台完成）
# 3. 为 ServiceAccount 添加注解
kubectl annotate sa app-sa -n production \
  ram.aliyun.com/role-arn=acs:ram::<account-id>:role/app-role

# 4. 验证注解
kubectl get sa app-sa -n production -o jsonpath='{.metadata.annotations.ram\.aliyun\.com/role-arn}'
```

### 4.4 自动伸缩配置

```bash
# 查看节点池
aliyun cs GET /clusters/<cluster-id>/nodepools

# 启用节点池自动伸缩
aliyun cs POST /clusters/<cluster-id>/nodepools/<np-id> \
  --body '{
    "auto_scaling": {
      "enable": true,
      "min_instances": 3,
      "max_instances": 50,
      "type": "cpu"
    }
  }'

# 安装 cluster-autoscaler（如未自动安装）
helm upgrade --install cluster-autoscaler autoscaler/cluster-autoscaler \
  --set autoDiscovery.clusterName=prod-ack-shanghai \
  --set cloudProvider=alicloud
```

### 4.5 集群升级

```bash
# 查询可升级版本
aliyun cs GET /clusters/<cluster-id>

# 升级控制平面
aliyun cs POST /clusters/<cluster-id>/upgrade \
  --body '{"version":"1.33.1-aliyun-1","next_version":""}'

# 升级节点池镜像
aliyun cs POST /clusters/<cluster-id>/nodepools/<np-id>/upgrade \
  --body '{"image_id":"aliyun_2_1903_x64_20G_alibase_20260101.vhd"}'
```

### 4.6 灾备与备份

```bash
# 1. ACK 自动备份（控制台开启后）
aliyun cs GET /clusters/<cluster-id>/backups

# 2. Velero 备份到 OSS
velero backup create prod-ack-daily \
  --include-namespaces production,monitoring \
  --storage-location alibabacloud-oss \
  --ttl 720h0m0s

# 3. 跨区域复制 OSS 备份桶
aliyun oss replication --method put \
  --bucket kudig-velero-shanghai \
  --rule file://oss-replication-rule.json

# 4. 恢复演练
velero restore create --from-backup prod-ack-daily \
  --namespace-mappings production:production-drill
```

### 4.7 SLS 日志与 ARMS 监控

```bash
# 查看 Logtail DaemonSet
kubectl get ds -n kube-system logtail-ds

# 查看 Project 与 Logstore
aliyun log GetProject --projectName=kudig-prod-ack

# 查看 ARMS Prometheus 抓取任务
kubectl get servicemonitor -A

# 查看告警规则
aliyun cms DescribeMetricRuleList --Namespace acs_k8s
```

### 4.8 成本治理

```bash
# 查看按标签分账账单
aliyun bssopenapi QueryAccountBill \
  --BillingCycle $(date +%Y-%m) \
  --ProductCode ecs

# 为节点打标签
kubectl label nodes -l nodepool=spot-ng cost-center=platform team=backend env=prod

# 查看各命名空间资源使用
kubectl top pods -A --containers | sort -k4 -nr | head -n 20
```

---

## 5. 关键检查点与验证命令

| 检查项 | 命令 | 通过标准 |
|---|---|---|
| 集群状态 | `aliyun cs GET /clusters/<cluster-id>` | state = running |
| 节点就绪 | `kubectl get nodes -o wide` | 所有节点 Ready，版本一致 |
| Terway 健康 | `kubectl get ds terway-daemon -n kube-system` | 全节点 Desired=Ready |
| ENI/IP 余量 | `terway-cli show` | 余量 > 20% |
| RRSA 配置 | `kubectl get sa -A -o json \| jq` | 业务 SA 已绑定 RAM Role |
| 自动伸缩 | `aliyun cs GET /clusters/<cluster-id>/nodepools` | enable=true，min/max 合理 |
| 备份成功 | `aliyun cs GET /clusters/<cluster-id>/backups` / `velero backup get` | 最近 24h 成功 |

---

## 6. 常见故障与 remediation

| 现象 | 可能根因 | 确认命令 | 修复措施 |
|---|---|---|---|
| Pod 处于 `Pending` | ENI/IP 耗尽或节点池无容量 | `terway-cli show` / `kubectl describe pod` | 扩容节点池或调整 Terway 配置 |
| `ImagePullBackOff` | ACR 权限或镜像不存在 | `kubectl describe pod` / `aliyun cr GetRepo` | 更新 RRSA/ACR 授权；校验 tag |
| 应用无法访问 OSS/RDS | RAM Role 权限不足 | `aliyun sts AssumeRole` | 修复 RAM Policy 与 Trust Policy |
| 节点 NotReady | 磁盘压力、kubelet 异常、实例被回收 | `kubectl describe node` / 控制台 | 替换节点；排查系统日志 |
| Terway Pod CrashLoop | Terway 版本与节点镜像不兼容 | `kubectl logs -n kube-system terway-daemon-xxx` | 升级 Terway；回滚节点镜像 |
| 自动伸缩未触发 | cluster-autoscaler 未运行或伸缩组配置错误 | `kubectl logs -n kube-system deploy/cluster-autoscaler` | 检查节点池标签与资源余量 |
| SLS 日志缺失 | Logtail 未采集或 Project 配置错误 | `kubectl logs -n kube-system ds/logtail-ds` | 检查机器组与采集配置 |
| 跨地域恢复失败 | 备份未跨区域复制或版本不兼容 | `velero backup get` / `velero restore logs` | 启用 OSS 跨区域复制 |

---

## 7. 风险与注意事项

1. **Terway IP 规划**: Pod 子网需按节点数 × 每节点 Pod 数 × 1.5 预留，避免 IP 耗尽导致调度失败。
2. **RRSA 与节点 RAM Role 并用风险**: 避免业务 Pod 继承节点 RAM Role，坚持每个应用独立 ServiceAccount。
3. **控制平面升级**: ACK 托管版控制平面升级由阿里云维护，但需关注版本兼容与 API 废弃。
4. **节点镜像升级**: 升级前确认 CSI/CNI/日志组件版本兼容，建议在非生产节点池验证。
5. **成本标签**: 节点创建时即打上标签，后续通过阿里云分账功能按团队/环境统计。
6. **灾备 RPO/RTO**: 明确 ACK 自动备份与 Velero 备份的恢复粒度，定期演练 Namespace 级恢复。

---

## 8. 相关 Runbook / 推荐阅读

### 本域资料

- [[domain-12-cloud-providers/99-production-readiness-operations-guide.md|云厂商托管 Kubernetes 生产就绪运维指南]]
- [[domain-12-cloud-providers/05-alicloud-ack/alicloud-ack-overview.md|阿里云 ACK 概述]]
- [[domain-12-cloud-providers/05-alicloud-ack/242-ack-vpc-network.md|ACK VPC 网络]]
- [[domain-12-cloud-providers/05-alicloud-ack/243-ack-ram-authorization.md|ACK RAM 授权]]
- [[domain-12-cloud-providers/05-alicloud-ack/240-ack-ecs-compute.md|ACK ECS 计算]]
- [[domain-12-cloud-providers/05-alicloud-ack/241-ack-slb-nlb-alb.md|ACK SLB/NLB/ALB]]
- [[domain-12-cloud-providers/05-alicloud-ack/245-ack-ebs-storage.md|ACK EBS 存储]]
- [[domain-12-cloud-providers/05-alicloud-ack/service-ack-practical-guide.md|ACK 实战指南]]

### 跨域参考

- [[domain-05-security-compliance/README.md|安全合规域]] — RAM/RRSA、NetworkPolicy、Secret 管理
- [[domain-06-observability/README.md|可观测性域]] — SLS、ARMS、Prometheus、SLO
- [[domain-09-reliability-engineering/README.md|可靠性工程域]] — 灾备、RTO/RPO
- [[domain-11-production-operations/README.md|生产运维域]] — 事件响应与变更管理

---

*阿里云 ACK 生产运维需要充分理解 Terway 网络、RRSA 身份与阿里云产品生态。建议将常用 aliyun CLI 命令封装为脚本，并纳入 GitOps 流水线版本化管理。*
