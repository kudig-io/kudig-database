---
title: 'Week 1 Checkpoint: 自测检验 [week-1-ack-acr-lifecycle]'
description: '## 概述'
summary: '本测验覆盖 Week 1 全部核心知识点，包括 ACK/ACR 服务架构、SDK/API 调用、集群创建/删除/升级流程和证书管理。测验分为四个部分，总计 80 分。答题时间限制 90 分钟。'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- controller-manager
- flannel
- coredns
- helm
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Week 1 Checkpoint: 自测检验 是什么'
- '如何 Week 1 Checkpoint: 自测检验'
trigger_keywords:
- Week
- 'Checkpoint:'
- 自测检验
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Week 1 Checkpoint: 自测检验
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ACK cluster lifecycle self-test quiz
  - [[Kubernetes|Kubernetes]] week 1 knowledge assessment
  - ACK ACR fundamental concepts test
  - Self-checkpoint quiz questions
  - Knowledge evaluation
trigger_keywords:
  - checkpoint
  - self-test
  - quiz
  - assessment
  - week 1
  - evaluation
  - 自我检验
  - 自测
reading_level: intermediate
audience:
  - Week 1 learners
  - ACK beginners
estimated_read_time: 30min
related_domains:
  - 云厂商
  - 集群基础
related_topics:
  - day-1-ack-acr-sr
  - day-2-ack-sdk-api
  - day-3-ack-acr-console
  - day-4-cluster-creation
  - day-5-cluster-deletion
  - day-6-cluster-upgrade
  - day-7-cluster-certificate
---

# Week 1 Checkpoint: 自测检验

> 完成本周学习后，请独立完成以下自测题，不要查阅资料。

---

## 概述

本测验覆盖 Week 1 全部核心知识点，包括 ACK/ACR 服务架构、SDK/API 调用、集群创建/删除/升级流程和证书管理。测验分为四个部分，总计 80 分。答题时间限制 90 分钟。

---

## 一、概念理解 (每题 3 分，共 30 分)

### 1. ACK 托管版、专有版、Serverless 三种集群类型的核心区别是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 维度 | 托管版 | 专有版 | Serverless |
|------|--------|--------|------------|
| 控制平面 | 阿里云托管 | 用户自建 | 阿里云托管 |
| 节点管理 | 用户管理 Worker | 用户管理全部 | 无需管理 (ECI) |
| 计费 | 管理费 + ECS | 仅 ECS | 按 Pod 计费 |
| 适用场景 | 大多数生产 | 合规定制 | 突发/间歇性 |
| 控制平面定制 | 不可以 | 可以 | 不可以 |

---

### 2. ACK 集群创建时，VPC CIDR、Pod CIDR、[[Service|Service]] CIDR 三者有什么关系和约束？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 网段 | 用途 | 约束 |
|------|------|------|
| VPC CIDR | 底层网络基础 | 需包含所有 vSwitch 网段 |
| Pod CIDR | Pod IP 分配范围 | 不能与 VPC/Service CIDR 重叠 |
| Service CIDR | ClusterIP 分配范围 | 创建后不可修改 |

示例规划:

```
VPC CIDR:      172.16.0.0/12  → 可容纳 1048576 个 IP
Pod CIDR:      10.0.0.0/16    → 可容纳 65536 个 Pod IP
Service CIDR:  192.168.0.0/16 → 可容纳 65536 个 Service
vSwitch A:     172.16.0.0/24  → 可容纳 256 个节点
vSwitch B:     172.16.1.0/24  → 可容纳 256 个节点
```

---

### 3. ACK 集群中有哪些核心的 kube-system 组件？各自的作用是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 组件 | 作用 | 必装 |
|------|------|------|
| [[CoreDNS|coredns]] | 韧群 DNS 服务 (域名解析) | 是 |
| cloud-controller-manager | 管理云资源 (SLB/路由/ENI) | 是 |
| terway / flannel | CNI 网络插件 (Pod 网络) | 二选一 |
| csi-plugin / csi-provisioner | CSI 存储插件 (云盘/NAS/OSS) | 是 |
| metrics-server | 指标采集 (kubectl top) | 是 |
| kube-proxy | Service 转发 (iptables/IPVS) | 是 |
| nginx-ingress-controller | Ingress 控制器 (L7 路由) | 推荐 |
| ack-node-problem-detector | 节点问题检测 | 推荐 |

---

### 4. ACK 集群升级分哪两个阶段？替换升级的操作流程是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

```
阶段一: 管控面升级
  └── 托管版由阿里云自动完成
  └── API Server 短暂不可用 (5-10 分钟)

阶段二: 节点升级
  └── 替换升级 (推荐):
      1. 扩容新版本节点
      2. cordon 旧节点
      3. drain 旧节点 (迁移 Pod)
      4. 确认 Pod 已迁移
      5. 移除旧版本节点
      6. 验证集群状态
```

---

### 5. K8S 集群中的证书体系包含哪些类型？kubeconfig 过期后如何处理？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 证书类型 | 用途 | 有效期 | 轮换方式 |
|----------|------|--------|---------|
| CA 根证书 | 签发所有组件证书 | 10 年 | 手动 |
| API Server 证书 | 服务端认证 | 1 年 | 自动 |
| kubelet 证书 | 节点身份认证 | 1 年 | 自动 |
| etcd 证书 | etcd 通信加密 | 1 年 | 自动 |
| kubeconfig | 用户访问凭证 | 3 年 | API 重新获取 |

kubeconfig 过期处理:

```bash
# 方法 1: 通过 API 重新获取
aliyun cs GET /k8s/<cluster_id>/user_config | jq -r '.config' > ~/.kube/config

# 方法 2: 通过 ACK 控制台下载
# 控制台 → 集群详情 → 连接信息 → 复制 kubeconfig
```

---

### 6. ACK API 的认证方式有哪几种？各适合什么场景？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 认证方式 | 安全等级 | 适用场景 | 有效期 |
|----------|---------|---------|--------|
| AK/SK | 中 | 服务端程序、脚本 | 永久 |
| STS Token | 高 | 临时授权、跨账号 | 15min-12h |
| RAM 角色 (ECS) | 高 | ECS 上运行的应用 | 自动轮换 |

---

### 7. ACR 企业版与个人版的主要区别是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 特性 | 个人版 | 企业版 |
|------|--------|--------|
| 安全扫描 | 基础 | 深度 CVE |
| 镜像签名 | 不支持 | 支持 |
| 跨地域同步 | 不支持 | 支持 |
| Helm Chart | 不支持 | 支持 |
| SLA | 无 | 99.99% |

---

### 8. 集群删除时哪些资源会被自动清理？哪些需要手动处理？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

| 资源 | 自动清理 | 手动处理 |
|------|---------|---------|
| ECS Worker 节点 | 是 | - |
| SLB | 是 | 可能残留 |
| ENI | 是 | 可能残留 |
| 安全组 | 是 | - |
| PV 云盘 (Reclaim=Delete) | 是 | - |
| PV 云盘 (Reclaim=Retain) | - | 需手动释放 |
| NAT 网关 | 可选 | - |
| ACR 镜像 | 不涉及 | 需手动清理 |

---

### 9. 为什么 K8s 版本升级不能跨版本（如从 1.26 直接升到 1.28）？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

- 每个小版本可能有 API 变更和废弃
- 跨版本升级可能导致 API 不兼容
- 数据格式变更可能无法直接迁移
- 必须逐版本升级: 1.26 → 1.27 → 1.28

---

### 10. aliyun CLI 调用 ACK API 时的基本命令格式是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

```bash
# GET 请求
aliyun cs GET /api/v1/clusters
aliyun cs GET /clusters/<cluster_id>

# POST 请求
aliyun cs POST /clusters --body '{...}'

# DELETE 请求
aliyun cs DELETE /clusters/<cluster_id>
```

---

## 二、命令实操 (每题 2 分，共 16 分)

### 11. 如何使用 aliyun CLI 查看当前账号下所有 ACK 集群？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:** `aliyun cs GET /api/v1/clusters`

---

### 12. 如何检查 kubeconfig 中客户端证书的过期时间？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d | openssl x509 -noout -dates
```
---

### 13. 如何对一个节点执行排水操作 (drain)？需要哪些参数？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
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
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
```
---

### 14. 如何查看 ACK 集群中所有 LoadBalancer 类型的 Service？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get svc -A | grep LoadBalancer
kubectl get svc -A -o jsonpath='{range .items[?(@.spec.type=="LoadBalancer")]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'
```
---

### 15. 如何查看集群节点的 kubelet 版本信息？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get nodes -o custom-columns='NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion'
```
---

### 16. 如何通过 API 获取集群的 kubeconfig？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:**

```bash
aliyun cs GET /k8s/<cluster_id>/user_config
```

---

### 17. 如何查看集群组件的升级状态？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:**

```bash
aliyun cs GET /clusters/<cluster_id>/components/upgradestatus
```

---

### 18. 如何触发集群证书轮换？

**你的回答:**

```
(在此写下你的答案)
```

**参考答案:**

```bash
aliyun cs POST /clusters/<cluster_id>/certrenew
```

---

## 三、场景分析 (每题 5 分，共 20 分)

### 19. 用户报告集群创建失败，你的排查步骤是什么？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

```bash
# Step 1: 查看创建日志
aliyun cs GET /clusters/<cluster_id>/logs

# Step 2: 检查 VPC/vSwitch
aliyun vpc DescribeVpcAttribute --VpcId <vpc_id>
aliyun vpc DescribeVSwitchAttributes --VSwitchId <vsw_id>

# Step 3: 检查 ECS 配额
aliyun ecs DescribeAvailableResource --RegionId cn-hangzhou

# Step 4: 检查 CIDR 冲突

# Step 5: 检查 RAM 权限
aliyun ram ListPoliciesForUser --UserName <user>

# 常见原因:
# - vSwitch IP 耗尽
# - ECS 实例库存不足
# - CIDR 冲突
# - RAM 权限不足
# - 账户余额不足
```

---

### 20. 用户需要将集群从 1.26 版本升级到 1.28 版本，你如何制定升级计划？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

```
升级计划:

1. 版本路径规划
   1.26 → 1.27 → 1.28 (逐版本升级)

2. 每次升级前
   ├── 使用 kubent 检查 API 废弃
   ├── 备份集群资源
   ├── 检查组件兼容性
   └── 检查 webhook 配置

3. 升级执行
   ├── 管控面升级 (等待完成)
   ├── 节点替换升级 (逐节点)
   └── 每步升级后验证

4. 升级后验证
   ├── 版本确认
   ├── 组件状态检查
   └── 业务可用性验证

5. 回滚方案
   └── 替换升级方式下保留旧节点
```

---

### 21. 删除集群时提示"部分资源无法释放"，如何处理？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

```bash
# Step 1: 查看删除日志
aliyun cs GET /clusters/<cluster_id>/logs | tail -20

# Step 2: 检查残留 SLB
aliyun slb DescribeLoadBalancers --VpcId <vpc_id>

# Step 3: 检查残留 ENI
aliyun ecs DescribeNetworkInterfaces --VpcId <vpc_id>

# Step 4: 手动释放残留资源
aliyun slb DeleteLoadBalancer --LoadBalancerId <slb-id>
aliyun ecs DeleteNetworkInterface --NetworkInterfaceId <eni-id>

# Step 5: 重试删除
aliyun cs DELETE /clusters/<cluster_id>

# Step 6: 如仍失败，联系阿里云 oncall
```

---

### 22. 用户反馈 kubectl 命令突然返回 "x509: certificate has expired"，你的排查和处理步骤？

**你的回答:**

```
(在此写下你的答案)


```

**参考要点:**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 确认证书过期
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d | openssl x509 -noout -dates

# Step 2: 如果是 kubeconfig 过期
aliyun cs GET /k8s/<cluster_id>/user_config | jq -r '.config' > ~/.kube/config

# Step 3: 如果是集群内部证书
aliyun cs POST /clusters/<cluster_id>/certrenew

# Step 4: 重新获取 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config | jq -r '.config' > ~/.kube/config

# Step 5: 验证
kubectl cluster-info
kubectl get nodes
```
---

## 四、评分统计

| 部分 | 得分 | 满分 |
|------|------|------|
| 概念理解 | __ | 30 |
| 命令实操 | __ | 16 |
| 场景分析 | __ | 20 |
| **总分** | __ | **66** |

### 评估标准

- **60-66 分**: 优秀，完全掌握本周内容
- **46-59 分**: 良好，基本掌握，部分细节需加强
- **33-45 分**: 及格，核心概念理解，需要复习
- **< 33 分**: 不及格，建议重新学习本周内容

---

## 五、薄弱点记录

记录自测中暴露的薄弱点，下周重点复习:

```
1.


2.


3.


```

---

## 六、下周计划调整

基于自测结果，调整下周学习重点:

```
需要加强的领域:


下周额外复习:


```

---

## 七、知识点速查表

| 知识点 | 关键命令/概念 | 对应测验题 |
|--------|-------------|-----------|
| 集群类型 | 托管版/专有版/Serverless 区别 | Q1 |
| 网络规划 | VPC/Pod/Service CIDR 不重叠 | Q2 |
| 核心组件 | coredns/CCM/CNI/CSI | Q3 |
| 升级流程 | 管控面 + 替换升级 | Q4 |
| 证书管理 | CA/kubelet/kubeconfig 有效期 | Q5 |
| API 认证 | AK/SK/STS/RAM 角色 | Q6 |
| ACR 版本 | 个人版 vs 企业版 | Q7 |
| 资源清理 | 自动 vs 手动清理 | Q8 |
| 版本策略 | 逐版本升级 | Q9 |
| CLI 格式 | cs GET/POST/DELETE | Q10 |

---

## 延伸阅读

- [ACK 服务总览](../../云厂商/04-alicloud-ack/alicloud-ack-overview.md)
- [K8s 架构总览](../../集群基础/01-kubernetes-architecture-overview.md)
- [K8s 版本升级策略](../../集群基础/07-upgrade-paths-strategy.md)
- [集群生命周期管理](../../平台工程/02-cluster-lifecycle-management.md)

## Related

- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
