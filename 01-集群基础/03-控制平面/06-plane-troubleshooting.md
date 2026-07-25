---
title: 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
description: 'title: 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)'
summary: 'title: 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)'
category: general
tags:
- k8s
- control-plane
- deep-dive
- troubleshooting
- etcd
- apiserver
- scheduler
- controller-manager
- prometheus
- calico
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 06-plane-troubleshooting常见问题有哪些？
- 如何排查06-plane-troubleshooting相关问题？
- 06-plane-troubleshooting的故障处理方法
trigger_keywords:
- 控制平面故障排查手册
- Control
- Plane
- Troubleshooting
- Handbook
- cluster
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- prometheus-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
description: '# 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)'
category: control-plane
tags:
- k8s
- control-plane
- [[etcd|etcd]]
- apiserver
- scheduler
- controller-manager
- [[Prometheus|prometheus]]
- calico
- docker
- rbac
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook) 是什么
- 如何 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
- Kubernetes 3 control plane 最佳实践
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook) 故障排查
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook) 排障步骤
trigger_keywords:
- 控制平面故障排查手册
- Control
- Plane
- Troubleshooting
- Handbook
- control
- plane
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../工作负载/
  label: '相关知识域: 工作负载'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../存储/
  label: '相关知识域: 存储'
- type: domain
  path: ../安全/
  label: '相关知识域: 安全'
- type: skill
  path: ../故障诊断/topic-skills/11-control-plane-failure.md
  label: '运维技能: 11-control-plane-failure'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **文档类型**: 故障诊断指南

---

<!-- chunk: 目录 -->
## 目录

1. [故障排查方法论](#1-故障排查方法论)
2. [API Server故障诊断](#2-api-server故障诊断)
3. [etcd集群故障处理](#3-etcd集群故障处理)
4. [控制器管理器问题](#4-控制器管理器问题)
5. [调度器问题排除](#5-调度器问题排除)
6. [网络连通性问题](#6-网络连通性问题)
7. [认证授权问题](#7-认证授权问题)
8. [性能问题诊断](#8-性能问题诊断)
9. [日志分析技巧](#9-日志分析技巧)
10. [应急恢复流程](#10-应急恢复流程)

---

<!-- chunk: 1. 故障排查方法论 -->
## 1. 故障排查方法论

### 1.1 系统性排查框架

```
# 🟢 低风险：只读/信息收集，通常无副作用
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          Troubleshooting Framework                              │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  1. 问题识别阶段 (Problem Identification)                                       │
│     ├── 症状收集: 错误信息、日志、监控告警                                     │
│     ├── 影响评估: 业务影响范围、严重程度                                       │
│     └── 优先级确定: P0(紧急) → P3(低优先级)                                    │
│                                                                                  │
│  2. 信息收集阶段 (Information Gathering)                                        │
│     ├── 状态检查: kubectl get/describe                                        │
│     ├── 日志收集: Pod日志、系统日志、审计日志                                  │
│     ├── 指标分析: Prometheus指标、性能数据                                    │
│     └── 配置审查: YAML配置、环境变量、启动参数                                 │
│                                                                                  │
│  3. 假设验证阶段 (Hypothesis Testing)                                           │
│     ├── 假设形成: 基于症状和信息提出可能原因                                   │
│     ├── 验证实验: 设计测试验证假设                                             │
│     ├── 结果分析: 分析测试结果确认或排除假设                                   │
│     └── 迭代优化: 根据结果调整假设继续验证                                     │
│                                                                                  │
│  4. 根因定位阶段 (Root Cause Analysis)                                          │
│     ├── 链路追踪: 从现象追溯到根本原因                                         │
│     ├── 依赖分析: 检查组件间依赖关系                                           │
│     ├── 时间线重建: 构建问题发生的时间序列                                     │
│     └── 模式识别: 识别重复出现的问题模式                                       │
│                                                                                  │
│  5. 解决方案实施 (Solution Implementation)                                      │
│     ├── 修复计划: 制定详细的修复步骤                                           │
│     ├── 风险评估: 评估修复操作的风险                                           │
│     ├── 执行修复: 按计划执行修复操作                                           │
│     └── 验证确认: 验证问题是否真正解决                                         │
│                                                                                  │
│  6. 预防改进阶段 (Prevention & Improvement)                                     │
│     ├── 经验总结: 记录故障处理过程和经验                                       │
│     ├── 流程优化: 改进监控告警和预防措施                                       ││     ├── 知识分享: 团队内部知识传递                                             │
│     └── 自动化提升: 将手动操作转化为自动化                                     │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```
### 1.2 问题分类矩阵

| 问题类型 | 常见症状 | 影响范围 | 恢复时间 | 复杂度 |
|----------|----------|----------|----------|--------|
| **配置错误** | Pod无法启动、API调用失败 | 单个应用 | 分钟级 | 低 |
| **资源不足** | 调度失败、OOMKilled | 部分工作负载 | 分钟级 | 中 |
| **网络问题** | 连接超时、DNS解析失败 | 跨组件通信 | 分钟到小时 | 中 |
| **存储问题** | PV挂载失败、数据丢失 | 持久化应用 | 小时级 | 高 |
| **认证授权** | 401/403错误、权限拒绝 | 访问控制 | 分钟级 | 中 |
| **组件问题** | 控制平面组件CrashLoop | 整个集群 | 分钟到小时 | 高 |
| **数据不一致** | 状态不同步、脑裂 | 集群状态 | 小时级 | 高 |
| **性能瓶颈** | 响应缓慢、超时 | 用户体验 | 持续优化 | 高 |

### 1.3 快速诊断命令集

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# Kubernetes快速诊断脚本

echo "=== Kubernetes Control Plane Quick Diagnosis ==="

# 1. 集群状态检查
echo "1. Checking cluster status..."
kubectl cluster-info
kubectl get nodes -o wide
kubectl get componentstatuses

# 2. 控制平面Pod状态
echo "2. Checking control plane pods..."
kubectl get pods -n kube-system

# 3. API Server可达性测试
echo "3. Testing API Server connectivity..."
kubectl get --raw=/healthz
kubectl get --raw=/livez
kubectl get --raw=/readyz

# 4. etcd集群健康检查
echo "4. Checking etcd health..."
ETCDCTL_API=3 etcdctl endpoint health --cluster
ETCDCTL_API=3 etcdctl endpoint status --cluster -w table

# 5. 关键组件日志检查
echo "5. Checking recent error logs..."
kubectl logs -n kube-system -l component=kube-apiserver --tail=50 | grep -i error
kubectl logs -n kube-system -l component=etcd --tail=50 | grep -i error
kubectl logs -n kube-system -l component=kube-controller-manager --tail=50 | grep -i error
kubectl logs -n kube-system -l component=kube-scheduler --tail=50 | grep -i error

# 6. 资源使用情况
echo "6. Checking resource usage..."
kubectl top nodes
kubectl top pods -n kube-system

# 7. 网络连通性检查
echo "7. Checking network connectivity..."
kubectl get svc --all-namespaces
kubectl get endpoints --all-namespaces

echo "=== Diagnosis completed ==="
```
---

<!-- chunk: 2. API Server故障诊断 -->
## 2. API Server故障诊断

### 2.1 常见问题类型

```
API Server问题分类:

┌─────────────────────────────────────────────────────────────────────────┐
│                           API Server Issues                              │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  启动失败类问题                                                         │
│  ├── 端口冲突 (Port already in use)                                     │
│  ├── 证书问题 (Certificate errors)                                      │
│  ├── 配置错误 (Invalid configuration)                                   │
│  └── 依赖组件不可达 (Dependent components unreachable)                  │
│                                                                          │
│  运行时问题                                                             │
│  ├── 内存泄漏 (Memory leak)                                             │
│  ├── CPU使用过高 (High CPU usage)                                       │
│  ├── 连接数超限 (Connection limits exceeded)                            │
│  └── etcd连接问题 (etcd connectivity issues)                            │
│                                                                          │
│  功能性问题                                                             │
│  ├── 认证失败 (Authentication failures)                                 │
│  ├── 授权拒绝 (Authorization denials)                                   │
│  ├── 准入控制阻塞 (Admission control blocks)                            │
│  └── API响应异常 (API response anomalies)                               │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 诊断步骤详解

#### 步骤1: 基础状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查API Server Pod状态
kubectl get pods -n kube-system -l component=kube-apiserver -o wide

# 2. 检查Pod详细信息
kubectl describe pod -n kube-system -l component=kube-apiserver

# 3. 检查API Server进程
ps aux | grep kube-apiserver

# 4. 检查端口监听状态
netstat -tlnp | grep :6443
ss -tlnp | grep :6443
```
#### 步骤2: 健康检查端点

```bash
# 1. 基础健康检查
curl -k https://localhost:6443/healthz

# 2. 详细健康检查
curl -k https://localhost:6443/livez?verbose

# 3. 就绪检查
curl -k https://localhost:6443/readyz?verbose

# 4. 指标端点检查
curl -k https://localhost:6443/metrics | head -20
```

#### 步骤3: 日志分析

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 实时日志查看
kubectl logs -n kube-system -l component=kube-apiserver -f

# 2. 错误日志筛选
kubectl logs -n kube-system -l component=kube-apiserver | grep -i "error|fatal|panic"

# 3. 特定时间段日志
kubectl logs -n kube-system -l component=kube-apiserver --since=1h

# 4. 日志模式分析
kubectl logs -n kube-system -l component=kube-apiserver | \
  awk '{print $NF}' | sort | uniq -c | sort -nr | head -10
```
#### 步骤4: 配置验证

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查API Server配置
kubectl get pod -n kube-system -l component=kube-apiserver -o yaml

# 2. 验证证书有效性
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -text -noout

# 3. 检查etcd连接配置
grep -A 5 -B 5 etcd /etc/kubernetes/manifests/kube-apiserver.yaml

# 4. 验证RBAC配置
kubectl auth can-i list pods --as=system:anonymous
```
### 2.3 典型故障处理

#### 问题1: API Server无法启动

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断步骤:
# 1. 检查Pod状态和事件
kubectl describe pod -n kube-system kube-apiserver-control-plane

# 2. 查看容器日志
docker logs $(docker ps -aq -f name=k8s_kube-apiserver) | tail -50

# 3. 检查配置文件语法
kube-apiserver --help 2>&1 | head -10

# 4. 验证端口可用性
lsof -i :6443
netstat -tlnp | grep 6443

# 常见解决方案:
# - 端口冲突: 修改端口或终止占用进程
# - 证书问题: 重新生成证书
# - 配置错误: 修正配置文件语法
# - 权限问题: 检查文件权限和SELinux/AppArmor
```
#### 问题2: etcd连接失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断命令:
# 1. 检查etcd端点可达性
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/apiserver-etcd-client.crt \
  --key=/etc/kubernetes/pki/apiserver-etcd-client.key \
  endpoint health

# 2. 测试网络连通性
telnet control-plane 2379
nc -zv control-plane 2379

# 3. 检查证书权限
ls -la /etc/kubernetes/pki/apiserver-etcd-client.*

# 解决方案:
# - 修复etcd集群状态
# - 检查网络策略和防火墙
# - 重新生成客户端证书
# - 验证etcd配置参数
```
#### 问题3: 认证授权失败

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断步骤:
# 1. 测试匿名访问
curl -k https://localhost:6443/api/v1/namespaces/default/pods

# 2. 验证ServiceAccount
kubectl get sa -n kube-system
kubectl describe sa default -n kube-system

# 3. 检查RBAC规则
kubectl get clusterroles | grep -i admin
kubectl get clusterrolebindings | grep -i admin

# 4. 测试Token有效性
TOKEN=$(kubectl create token default -n default)
curl -k -H "Authorization: Bearer $TOKEN" https://localhost:6443/api/v1/namespaces

# 解决方案:
# - 重新生成ServiceAccount Token
# - 修复RBAC配置
# - 检查认证Webhook配置
# - 验证证书颁发机构
```
---

<!-- chunk: 3. etcd集群故障处理 -->
## 3. etcd集群故障处理

### 3.1 etcd问题类型识别

```
etcd问题分类:

┌─────────────────────────────────────────────────────────────────────────┐
│                           etcd Failure Types                              │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  数据层面问题                                                           │
│  ├── 数据不一致 (Data inconsistency)                                    │
│  ├── 数据丢失 (Data loss)                                               │
│  ├── 数据损坏 (Data corruption)                                         │
│  └── 版本冲突 (Version conflicts)                                       │
│                                                                          │
│  集群层面问题                                                           │
│  ├── Leader选举失败 (Leader election failure)                           │
│  ├── 集群分裂 (Cluster partition)                                       │
│  ├── 成员变更失败 (Member change failure)                               │
│  └── 配额超限 (Quota exceeded)                                          │
│                                                                          │
│  性能层面问题                                                           │
│  ├── 写入延迟高 (High write latency)                                    │
│  ├── 读取超时 (Read timeouts)                                           │
│  ├── 磁盘I/O瓶颈 (Disk I/O bottleneck)                                  │
│  └── 内存使用过高 (High memory usage)                                   │
│                                                                          │
│  网络层面问题                                                           │
│  ├── 网络分区 (Network partition)                                       │
│  ├── 连接超时 (Connection timeouts)                                     │
│  ├── 包丢失 (Packet loss)                                               │
│  └── 带宽不足 (Insufficient bandwidth)                                  │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 3.2 etcd诊断工具集

#### 基础健康检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# etcd健康检查脚本

ETCDCTL="ETCDCTL_API=3 etcdctl"
ENDPOINTS="https://etcd-0.etcd:2379,https://etcd-1.etcd:2379,https://etcd-2.etcd:2379"
CERTS="--cacert=/etc/kubernetes/pki/etcd/ca.crt --cert=/etc/kubernetes/pki/etcd/server.crt --key=/etc/kubernetes/pki/etcd/server.key"

echo "=== etcd Health Check ==="

# 1. 集群健康状态
echo "1. Cluster Health:"
$ETCDCTL --endpoints=$ENDPOINTS $CERTS endpoint health

# 2. 集群状态详情
echo "2. Cluster Status:"
$ETCDCTL --endpoints=$ENDPOINTS $CERTS endpoint status -w table

# 3. 成员列表
echo "3. Member List:"
$ETCDCTL --endpoints=$ENDPOINTS $CERTS member list -w table

# 4. 告警检查
echo "4. Active Alarms:"
$ETCDCTL --endpoints=$ENDPOINTS $CERTS alarm list

# 5. 性能测试
echo "5. Performance Test:"
$ETCDCTL --endpoints=$ENDPOINTS $CERTS check perf --load="s"

# 6. 磁盘使用情况
echo "6. Disk Usage:"
$ETCDCTL --endpoints=$ENDPOINTS $CERTS endpoint status --write-out="json" | \
  jq '.[].Status.dbSize'

echo "=== Health Check Complete ==="
```
#### 详细诊断命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查etcd日志
kubectl logs -n kube-system -l component=etcd --tail=100

# 2. 监控关键指标
watch -n 5 'ETCDCTL_API=3 etcdctl endpoint status --cluster -w table'

# 3. 检查网络延迟
for i in {1..10}; do
  ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS endpoint health --cluster
  sleep 1
done

# 4. 验证数据一致性
ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS get /registry --prefix --keys-only | \
  head -10

# 5. 检查磁盘性能
dd if=/dev/zero of=/var/lib/etcd/test bs=1M count=1000 conv=fdatasync
rm /var/lib/etcd/test
```
### 3.3 常见故障处理

#### 问题1: Leader丢失

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 症状识别:
# - etcdctl endpoint status 显示没有leader
# - API Server返回"etcd cluster has no leader"错误
# - 集群写操作失败

# 诊断步骤:
# 1. 检查所有etcd成员状态
ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS member list

# 2. 检查网络连通性
for endpoint in etcd-0.etcd:2379 etcd-1.etcd:2379 etcd-2.etcd:2379; do
  nc -zv $endpoint 2379
done

# 3. 检查磁盘空间
df -h /var/lib/etcd

# 恢复方案:
# 方案1: 重启问题节点
kubectl delete pod -n kube-system etcd-control-plane-node

# 方案2: 强制重新选举
ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS member remove <member-id>
ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS member add etcd-new --peer-urls=https://new-node:2380

# 方案3: 从快照恢复
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-snapshot.db \
  --data-dir=/var/lib/etcd-restored \
  --name=etcd-0 \
  --initial-cluster=etcd-0=https://etcd-0:2380 \
  --initial-cluster-token=etcd-cluster-1 \
  --initial-advertise-peer-urls=https://etcd-0:2380
```
#### 问题2: 数据库配额超限

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 症状:
# - 错误信息:"etcdserver: mvcc: database space exceeded"
# - API Server写入操作失败
# - etcd报警:NOSPACE

# 诊断命令:
# 1. 检查数据库大小
ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS endpoint status --write-out="json" | \
  jq '.[].Status.dbSize'

# 2. 检查配额设置
ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS endpoint status --write-out="json" | \
  jq '.[].Status.capacity'

# 3. 查看告警状态
ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS alarm list

# 解决方案:
# 1. 执行压缩操作
rev=$(ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS endpoint status --write-out="json" | \
  jq -r '.[].Status.header.revision')
ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS compact $rev

# 2. 执行碎片整理
ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS defrag --cluster

# 3. 清除告警
ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS alarm disarm

# 4. 增加配额(临时方案)
# 修改etcd启动参数: --quota-backend-bytes=8589934592 (8GB)
```
#### 问题3: 网络分区

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 症状:
# - 集群成员间无法通信
# - 出现多个leader(candidate)
# - 写入操作hang住或超时

# 诊断步骤:
# 1. 检查网络连通性
ping etcd-0.etcd
telnet etcd-0.etcd 2379
telnet etcd-0.etcd 2380

# 2. 检查防火墙规则
iptables -L -n | grep 2379
iptables -L -n | grep 2380

# 3. 检查网络策略
kubectl get networkpolicies -n kube-system

# 4. 验证SSL/TLS连接
openssl s_client -connect etcd-0.etcd:2379 -cert client.crt -key client.key -CAfile ca.crt

# 恢复步骤:
# 1. 修复网络连通性
# 2. 重启网络服务
systemctl restart networking

# 3. 重新配置etcd成员
ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS member update <member-id> \
  --peer-urls=https://fixed-ip:2380

# 4. 监控集群恢复
watch -n 2 'ETCDCTL_API=3 etcdctl --endpoints=$ENDPOINTS $CERTS endpoint status -w table'
```
---

<!-- chunk: 4. 控制器管理器问题 -->
## 4. 控制器管理器问题

### 4.1 控制器故障模式

```
控制器管理器问题类型:

┌─────────────────────────────────────────────────────────────────────────┐
│                      Controller Manager Failures                          │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  Leader选举问题                                                         │
│  ├── 无法获得leadership                                                 │
│  ├── leadership频繁切换                                                 │
│  └── 多个实例同时认为自己是leader                                       │
│                                                                          │
│  控制器工作问题                                                         │
│  ├── 工作队列积压 (Work queue backlog)                                  │
│  ├── 控制器处理速度慢                                                   │
│  ├── Reconcile循环失败                                                  │
│  └── 资源版本冲突                                                       │
│                                                                          │
│  资源访问问题                                                           │
│  ├── API Server连接失败                                                 │
│  ├── 权限不足 (Insufficient permissions)                                │
│  ├── 资源不存在 (Resource not found)                                    │
│  └── 并发访问冲突                                                       │
│                                                                          │
│  配置相关问题                                                           │
│  ├── 启动参数错误                                                       │
│  ├── 配置文件语法错误                                                   │
│  ├── 特性门控配置不当                                                   │
│  └── 资源配额限制                                                       │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 4.2 诊断命令集合

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查控制器管理器状态
kubectl get pods -n kube-system -l component=kube-controller-manager

# 2. 查看控制器日志
kubectl logs -n kube-system -l component=kube-controller-manager --tail=100

# 3. 检查leadership状态
kubectl get leases -n kube-system

# 4. 监控工作队列
kubectl get --raw="/metrics" | grep workqueue

# 5. 检查控制器指标
kubectl get --raw="/metrics" | grep controller_manager

# 6. 验证RBAC权限
kubectl auth can-i list pods --as=system:kube-controller-manager
```
### 4.3 常见问题处理

#### 问题1: Deployment控制器不工作

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断步骤:
# 1. 检查Deployment控制器状态
kubectl get deployments --all-namespaces
kubectl describe deployment <deployment-name>

# 2. 查看控制器日志中的相关错误
kubectl logs -n kube-system -l component=kube-controller-manager | \
  grep -i "deployment|replicaset"

# 3. 检查工作队列状态
kubectl get --raw="/metrics" | grep "workqueue_depth.*deployment"

# 4. 验证相关资源是否存在
kubectl get rs -n <namespace>
kubectl get pods -n <namespace> -l app=<app-name>

# 解决方案:
# - 检查资源配额限制
# - 验证节点资源充足性
# - 检查网络策略是否阻止Pod创建
# - 重启控制器管理器Pod
```
#### 问题2: Node控制器驱逐Pod异常

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断命令:
# 1. 检查节点状态
kubectl describe node <node-name>

# 2. 查看节点条件
kubectl get nodes -o jsonpath='{.items[*].status.conditions}' | jq .

# 3. 检查污点和容忍度
kubectl describe node <node-name> | grep -A 10 Taints

# 4. 查看驱逐相关日志
kubectl logs -n kube-system -l component=kube-controller-manager | \
  grep -i "evict|taint|notready"

# 解决方案:
# - 调整节点监控参数
# - 检查节点资源使用情况
# - 验证网络连通性
# - 调整PodDisruptionBudget设置
```
---

<!-- chunk: 5. 调度器问题排除 -->
## 5. 调度器问题排除

### 5.1 调度器问题识别

```
调度器故障模式:

┌─────────────────────────────────────────────────────────────────────────┐
│                         Scheduler Failures                                │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│  调度失败类型                                                           │
│  ├── 资源不足无法调度 (Insufficient resources)                          │
│  ├── 约束条件不满足 (Constraint violations)                             │
│  ├── 亲和性/反亲和性冲突 (Affinity conflicts)                           │
│  └── 污点不容忍 (Taints not tolerated)                                  │
│                                                                          │
│  性能问题                                                               │
│  ├── 调度延迟过高 (High scheduling latency)                             │
│  ├── 批量调度缓慢 (Slow batch scheduling)                               │
│  ├── 预选/优选阶段耗时长                                                │
│  └── 调度器CPU使用率过高                                                │
│                                                                          │
│  配置问题                                                               │
│  ├── 调度策略配置错误                                                   │
│  ├── 插件配置不当                                                       │
│  ├── 评分算法不合理                                                     │
│  └── 资源配额设置错误                                                   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

### 5.2 调度器诊断工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查调度器状态
kubectl get pods -n kube-system -l component=kube-scheduler

# 2. 查看调度器日志
kubectl logs -n kube-system -l component=kube-scheduler --tail=100

# 3. 监控调度指标
kubectl get --raw="/metrics" | grep scheduler

# 4. 检查未调度Pod
kubectl get pods --all-namespaces --field-selector=status.phase=Pending

# 5. 分析调度失败原因
kubectl describe pod <pending-pod-name>
```
### 5.3 典型调度问题解决

#### 问题1: Pod持续Pending状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 诊断步骤:
# 1. 检查Pod调度状态
kubectl describe pod <pod-name> | grep -A 20 "Events:"

# 2. 查看节点资源情况
kubectl top nodes
kubectl describe nodes

# 3. 检查资源请求和限制
kubectl get pod <pod-name> -o yaml | grep -A 10 resources

# 4. 验证节点选择器
kubectl get pod <pod-name> -o yaml | grep -A 5 nodeSelector

# 5. 检查污点和容忍度
kubectl get pod <pod-name> -o yaml | grep -A 10 tolerations

# 常见解决方案:
# - 调整资源请求值
# - 添加节点容忍度
# - 清理不需要的Pod释放资源
# - 增加集群节点
```
#### 问题2: 调度器性能下降

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 性能诊断:
# 1. 监控调度延迟
kubectl get --raw="/metrics" | grep "scheduler_e2e_scheduling_duration"

# 2. 检查调度队列深度
kubectl get --raw="/metrics" | grep "scheduler_pending_pods"

# 3. 分析调度器CPU使用
kubectl top pod -n kube-system -l component=kube-scheduler

# 优化措施:
# - 调整调度器参数
# - 优化调度策略
# - 增加调度器实例
# - 升级到更高版本
```
---

<!-- chunk: 6. 网络连通性问题 -->
## 6. 网络连通性问题

### 6.1 网络故障诊断矩阵

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 网络连通性检查脚本
#!/bin/bash

echo "=== Network Connectivity Diagnosis ==="

# 1. 控制平面网络检查
echo "1. Control Plane Network:"
for component in kube-apiserver etcd kube-controller-manager kube-scheduler; do
  echo "Checking $component connectivity:"
  kubectl get pods -n kube-system -l component=$component -o wide
done

# 2. Service网络检查
echo "2. Service Network:"
kubectl get svc --all-namespaces | head -10

# 3. DNS解析检查
echo "3. DNS Resolution:"
kubectl run -it --rm debug-dns --image=busybox:1.28 -- nslookup kubernetes.default

# 4. 网络策略检查
echo "4. Network Policies:"
kubectl get networkpolicies --all-namespaces

# 5. CNI插件状态
echo "5. CNI Status:"
kubectl get pods -n kube-system -l k8s-app=calico-node

echo "=== Network Diagnosis Complete ==="
```
### 6.2 常见网络问题处理

#### 问题1: Pod无法访问Service

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 诊断步骤:
# 1. 检查Service配置
kubectl get svc <service-name> -o yaml

# 2. 验证Endpoints
kubectl get endpoints <service-name>

# 3. 测试Service连通性
kubectl run -it --rm debug-pod --image=busybox -- wget -qO- <service-ip>:<port>

# 4. 检查网络策略
kubectl get networkpolicies -n <namespace>

# 解决方案:
# - 修正Service配置
# - 检查Pod标签选择器
# - 调整网络策略规则
# - 验证CNI插件状态
```
---

<!-- chunk: 总结 -->
## 总结

这份故障排查手册涵盖了Kubernetes控制平面的主要问题类型和处理方法。通过系统性的诊断方法和具体的命令示例，可以帮助运维人员快速定位和解决各类问题。

关键要点:
1. 建立标准化的故障排查流程
2. 准备完善的诊断工具集
3. 保持监控系统的有效性
4. 定期进行故障演练
5. 持续优化预防措施

掌握这些故障排查技能将大大提高Kubernetes集群的稳定性和可靠性。
---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 集群基础 MOC
- [[01-集群基础/README.md|Domain-3: Kubernetes控制平面]]
- Domain-3 控制平面 — 开源项目索引
- Kubernetes 控制平面架构总览 (Control Plane Architecture Overview)
- 控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)
- 控制平面高可用部署模式 (Control Plane High Availability Deployment Patt...
- 控制平面安全加固指南 (Control Plane Security Hardening Guide)
- 控制平面监控与可观测性 (Control Plane Monitoring & Observability)
- 控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)
- 控制平面性能基准测试 (Control Plane Performance Benchmarking)
- 控制平面扩缩容指南 (Control Plane Scalability Guide)
- 控制平面备份与灾备方案 (Control Plane Backup & Disaster Recovery)
- [[19-故障诊断/06-FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## Related

- [[deep-dive|#deep-dive Hub]] — tag hub

- [[kudig-prompts-catalog]]

## See Also

- 04-plane-security-hardening
- 05-plane-monitoring-observability
- 07-plane-upgrade-migration
- 08-plane-performance-benchmarking


<!-- risk-assessed -->
