---
title: 'Day 9: RAM 账号管理'
description: 'title: Day 9: RAM 账号管理'
summary: 'title: Day 9: RAM 账号管理'
category: learning
tags:
- k8s
- training
- hands-on
- rbac
- operator
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 9: RAM 账号管理 是什么'
- '如何 Day 9: RAM 账号管理'
trigger_keywords:
- Day
- '9:'
- RAM
- 账号管理
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 9: RAM 账号管理
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - ACK RAM authorization [[23-实体/kubernetes.md|[[Kubernetes|kubernetes]]]] RBAC integration
  - aliyun cs grant_permissions RAM user cluster access
  - RAM role assume role Kubernetes
  - Multi-team RBAC namespace isolation
  - kubeconfig RAM user authentication
trigger_keywords:
  - RAM
  - RBAC
  -权限管理
  - grant_permissions
  - kubeconfig
  - 双层权限
  - 云平台权限
  - 集群权限
  - AssumeRole
reading_level: intermediate
audience:
  - ACK operators
  - DevOps engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - 安全
  - 云厂商
  - 故障诊断
related_topics:
  - rbac-configuration
  - ack-cluster-lifecycle
  - certificate-management
---

# Day 9: RAM 账号管理

> **学习时间**: 4-5 小时 | **主题**: RAM 账号与 K8S 集成方案

---

## 今日目标

- [ ] 理解阿里云 RAM 与 ACK 权限的映射关系
- [ ] 掌握 RAM 用户授权 ACK 集群的操作流程
- [ ] 了解 RAM 角色扮演在 ACK 中的应用
- [ ] 能够为不同团队配置分级权限

---

## 理论学习 (2h)

### 必读文档

1. **ACK RAM 授权**
   - 文件: `../../../云厂商/04-alicloud-ack/243-ack-ram-authorization.md`
   - 重点: RAM 策略类型、ACK 权限模型

2. **认证授权体系**
   - 文件: `../../../安全/01-authentication-authorization-system.md`
   - 重点: K8S 认证方式、与云平台的集成

### 阅读要点

- ACK 权限分两层: 云平台权限 (RAM) + 集群内权限 (RBAC)
- RAM 权限控制: 控制台访问、API 调用、集群管理操作
- RBAC 权限控制: 集群内 K8S 资源操作
- RAM 角色: 管理员、运维、开发、只读四种预置角色
- 跨账号访问: 通过 RAM 角色扮演 (AssumeRole) 实现

---

## 实践任务 (2.5h)

### 任务 1: RAM 用户与 ACK 权限关联 (45min)

```bash
# 1. 查看 RAM 用户列表
aliyun ram ListUsers

# 2. 查看 RAM 自定义策略
aliyun ram ListPolicies --PolicyType Custom

# 3. ACK 控制台授权 RAM 用户
# 控制台 -> ACK -> 授权管理 -> RAM 授权
# 选择 RAM 用户 -> 分配集群角色:
#   - 管理员: 集群所有操作
#   - 运维: 读写 Namespace 资源
#   - 开发: 读写指定 Namespace
#   - 只读: 查看所有资源

# 4. 查看 ACK 授权配置
aliyun cs GET /clusters/<cluster_id>/auth/users
```

### 任务 2: 创建 RAM 策略与角色 (45min)

```bash
# 创建 ACK 只读策略
cat > ack-readonly-policy.json << 'EOF'
{
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "cs:Get*",
        "cs:List*",
        "cs:Describe*"
      ],
      "Resource": "*"
    }
  ],
  "Version": "1"
}
EOF

aliyun ram CreatePolicy \
  --PolicyName ACKReadOnly \
  --PolicyDocument "$(cat ack-readonly-policy.json)"

# 创建 ACK 运维策略
cat > ack-ops-policy.json << 'EOF'
{
  "Statement": [
    {
      "Effect": "Allow",
      "Action": "cs:*",
      "Resource": "*"
    },
    {
      "Effect": "Deny",
      "Action": [
        "cs:DeleteCluster",
        "cs:CreateCluster"
      ],
      "Resource": "*"
    }
  ],
  "Version": "1"
}
EOF

aliyun ram CreatePolicy \
  --PolicyName ACKOpsEngineer \
  --PolicyDocument "$(cat ack-ops-policy.json)"
```

### 任务 3: 为 RAM 用户配置 kubeconfig (45min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. RAM 用户获取 kubeconfig (需要 RAM 用户自己的 AK/SK)
# 方式一: 通过控制台下载
# 方式二: 通过 API
aliyun cs GET /k8s/<cluster_id>/user_config

# 2. 验证 RAM 用户权限
# 使用 RAM 用户的 kubeconfig
export KUBECONFIG=~/ram-user-kubeconfig.yaml
kubectl get pods -A
kubectl create namespace test  # 根据授权角色决定是否成功

# 3. 查看 RAM 用户在集群中的身份
kubectl auth whoami  # K8S 1.27+
kubectl auth can-i --list
```
### 任务 4: 多团队权限管理方案 (30min)

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl apply/create/replace`：创建/变更集群资源

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
# 场景: 为开发、测试、运维三个团队配置不同权限

# 开发团队: 只能访问 dev namespace
# 测试团队: 只能访问 test namespace
# 运维团队: 可以访问所有 namespace (非 admin)

# 1. 创建 Namespace
kubectl create namespace dev
kubectl create namespace test

# 2. 在 ACK 控制台为各团队 RAM 用户授权:
# - 开发团队 RAM 用户 -> 开发角色 -> dev namespace
# - 测试团队 RAM 用户 -> 开发角色 -> test namespace
# - 运维团队 RAM 用户 -> 运维角色 -> 所有 namespace

# 3. 验证隔离效果
# 开发用户:
kubectl get pods -n dev    # 成功
kubectl get pods -n test   # 失败 (无权限)

# 清理
kubectl delete namespace dev test  # ⚠️ 不可逆：永久删除命名空间及全部资源
```
---

## 费曼复述 (0.5h)

1. **ACK 的两层权限模型是什么？它们分别控制什么？**
2. **RAM 用户如何获取 ACK 集群的 kubeconfig？**
3. **如何实现"开发团队只能操作自己的 Namespace"？**

---

## 今日检验

- [ ] 理解 RAM 权限与 RBAC 权限的区别和联系
- [ ] 能为 RAM 用户授权 ACK 集群访问
- [ ] 能创建自定义 RAM 策略
- [ ] 能设计多团队权限隔离方案

---

## 核心概念总结

| 权限层 | 控制范围 | 管理方式 |
|--------|----------|---------|
| RAM 权限 | 云平台操作 (控制台/API) | RAM 策略 |
| ACK 授权 | 集群角色分配 | ACK 授权管理 |
| RBAC 权限 | 集群内资源操作 | Role/ClusterRole |

---

## 明日预告

Day 10 将学习 ACK/ACR/K8S 常见漏洞类型与防护措施。


<!-- risk-assessed -->
