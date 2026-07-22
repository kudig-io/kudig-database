---
title: K8s 架构基础与核心组件原理
description: '# K8s 架构基础与核心组件原理'
summary: 'KUDIG-DATABASE（Kubernetes Universal Database & Intelligence Gateway）是一个开源的云原生技术全域知识库，覆盖 950+ 篇文档、41 个知识领域、4300 万+ 字符。以 **Domain（知识域）× Topic（专题）** 二维矩阵组织，具有明确依赖关系和学习路径。'
category: reference
tags:
- k8s
- architecture
- core-components
- apiserver
- etcd
- scheduler
- controller-manager
- kubelet
- docker
- gateway
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- K8s 架构基础与核心组件原理 是什么
- 如何 K8s 架构基础与核心组件原理
trigger_keywords:
- K8s
- 架构基础与核心组件原理
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s 架构基础与核心组件原理

## 项目全景

KUDIG-DATABASE（Kubernetes Universal Database & Intelligence Gateway）是一个开源的云原生技术全域知识库，覆盖 950+ 篇文档、41 个知识领域、4300 万+ 字符。以 **Domain（知识域）× Topic（专题）** 二维矩阵组织，具有明确依赖关系和学习路径。

三大差异化定位：
- **生产级**：所有 YAML/Shell 示例经过万级节点生产环境验证
- **AI-Ready**：文档结构天然适配 NotebookLM、RAG 和 Agent 训练场景
- **方法论独创**：内置 FTA 故障树分析、FEBM 取证循证、Skill 诊断-修复闭环

## 分层架构模型

Kubernetes 将系统职责拆解为 7 个正交层次：

| 层次 | 名称 | 职责 | 关键组件 |
|------|------|------|----------|
| Layer 1 | 编排层 | 调度、编排、自动化 | Scheduler, Controllers |
| Layer 2 | API 层 | 统一入口、认证授权、准入控制 | API Server, Admission Controllers |
| Layer 3 | 数据层 | 持久化存储 | etcd |
| Layer 4 | 运行时层 | 容器运行环境 | kubelet, Container Runtime |
| Layer 5 | 网络层 | Pod 网络、Service 负载均衡 | CNI, kube-proxy |
| Layer 6 | 存储层 | 持久化卷管理 | CSI, Volume Plugin |
| Layer 7 | 扩展层 | 自定义功能扩展 | CRD, Operator, Webhook |

**核心推论**：任何请求都必须从 Layer 2（API Server）进入。Scheduler 不直接与 kubelet 通信，Controller Manager 也不直接读写 etcd。这种星型拓扑实现真正松耦合。

## 控制平面核心组件

### API Server
- 唯一的状态入口，所有组件通过 API Server 交互
- 默认端口：:6443（HTTPS）
- 职责：认证、授权、准入控制、API 对象 CRUD

### etcd
- 唯一的持久化后端，默认端口 :2379/:2380
- 基于 Raft 共识协议的分布式 KV 存储
- 生产环境推荐 3 或 5 节点集群

### Scheduler（kube-scheduler）
- 默认端口：:10259
- 两阶段调度：过滤（Filtering）→ 打分（Scoring）
- 支持自定义调度器和调度框架扩展

### Controller Manager（kube-controller-manager）
- 默认端口：:10257
- 核心控制循环：观察实际状态 → 比较期望状态 → 执行调谐动作
- 内置控制器：Deployment、ReplicaSet、Node、Service Account 等

## 控制器模式（Controller Pattern）

Kubernetes 的核心设计模式是**声明式 API + 控制器调谐**：
1. 用户通过 YAML 声明期望状态
2. API Server 将声明持久化到 etcd
3. 控制器 Watch 变更，执行调谐使实际状态匹配期望状态
4. 如果实际状态偏离期望，控制器自动修复

这一模式贯穿 Kubernetes 所有组件，是理解整个系统的基础。

## 学习路径建议

1. Linux 基础 → Docker 容器 → K8s 架构概览
2. 核心组件原理 → API 对象 → 控制器模式
3. 网络模型 → 存储模型 → 安全模型
4. 高级主题：调度、扩缩容、多集群、服务网格

## 运维操作

```bash
# 🟢 查看控制平面组件状态
kubectl get componentstatuses
kubectl get --raw /healthz?verbose

# 🟢 查看 API Server 状态
kubectl get --raw /version
kubectl get --raw /metrics | grep apiserver_request_total

# 🟢 查看 etcd 健康状态
etcdctl --endpoints=https://etcd:2379 endpoint health
etcdctl --endpoints=https://etcd:2379 endpoint status --write-out=table

# 🟢 查看调度器状态
kubectl get --raw /metrics | grep scheduler_scheduling_attempt_total
kubectl get events --field-selector reason=Scheduled -A

# 🟢 查看控制器状态
kubectl get --raw /metrics | grep workqueue_depth

# 🟢 查看 kubelet 状态
kubectl get nodes -o wide
kubectl describe node <node-name>

# 🟡 查看 API 请求延迟
kubectl get --raw /metrics | grep apiserver_request_duration_seconds

# 🟢 查看集群事件
kubectl get events -A --sort-by=.lastTimestamp | tail -20
```

## 故障排查

| 症状 | 可能原因 | 排查命令 | 修复方案 |
|------|----------|----------|----------|
| API Server 无响应 | 证书过期/资源不足 | `kubectl get --raw /healthz` | 检查证书有效期、资源使用 |
| etcd 延迟高 | 磁盘 I/O 不足 | `etcdctl endpoint status` | 使用 SSD、检查 fsync 延迟 |
| 节点 NotReady | kubelet 异常 | `systemctl status kubelet` | 检查 kubelet 日志和证书 |
| 调度延迟 | 大量 Pending Pod | `kubectl get pods --field-selector status.phase=Pending` | 检查调度器资源和配置 |
| 控制器积压 | workqueue 深度大 | 检查 controller metrics | 增加 controller-manager 资源 |
| 证书即将过期 | 未轮换 | `kubeadm certs check-expiration` | `kubeadm certs renew all` |

### 排查流程

```
集群异常 → 检查控制平面健康
  ├─ API Server 不可用 → 检查证书/资源/etcd 连接
  ├─ etcd 异常 → 检查磁盘/网络/Raft 状态
  ├─ 调度异常 → 检查节点资源/污点/亲和性
  └─ 节点异常 → 检查 kubelet/容器运行时/网络
      ├─ kubelet 停止 → 检查证书轮换、磁盘空间
      └─ 容器运行时异常 → 检查 containerd/CRI-O 状态
```

## 生产案例

### 案例1: etcd 磁盘延迟导致集群不稳定

**场景**: 集群周期性出现 API 请求超时，影响所有工作负载  
**排查**: etcd metrics 显示 fsync 延迟 > 100ms，磁盘 I/O 争用  
**方案**: 迁移 etcd 到独立 SSD，调整 --quota-backend-bytes  
**效果**: API 延迟稳定在 < 50ms，消除超时事件  

### 案例2: 证书过期导致节点失联

**场景**: 多个节点同时变为 NotReady  
**排查**: kubelet 日志显示证书验证失败，kubeadm 证书已过期  
**方案**: `kubeadm certs renew all` + 重启 kubelet，配置证书过期监控告警  
**效果**: 节点恢复，建立证书自动轮换机制  

## 检查清单

- [ ] 控制平面组件配置健康检查监控
- [ ] etcd 使用独立 SSD 存储
- [ ] 配置证书过期告警（提前 30 天）
- [ ] 定期备份 etcd 数据
- [ ] API Server 配置审计日志
- [ ] 监控 workqueue 深度和调度延迟
- [ ] 配置 API Server 请求限流
- [ ] 生产环境控制平面 3+ 节点高可用

---

> 来源：.zread/wiki/drafts/1-xiang-mu-zong-lan-kudig-database-quan-yu-zhi-shi-ku.md, .zread/wiki/drafts/5-jia-gou-ji-chu-yu-he-xin-zu-jian-yuan-li.md

## Related

- [[实体/kubelet.md|kubelet]] — kubelet
- [[cni]] — CNI (Container Network Interface)
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/controller-pattern.md|controller-pattern]] — Controller Pattern (Reconciliation Loop)
- [[实体/kube-scheduler.md|kube-scheduler]] — K8s 调度器


<!-- risk-assessed -->
