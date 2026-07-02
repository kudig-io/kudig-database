---
title: 'Day 1: 新人首日检查清单 [quick-start]'
description: '# stern - 日志实时跟踪'
summary: '2. `domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md` — kubectl 场景速查'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
- rbac
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 1: 新人首日检查清单 是什么'
- '如何 Day 1: 新人首日检查清单'
trigger_keywords:
- Day
- '1:'
- 新人首日检查清单
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 1: 新人首日检查清单

> **适用对象**: 入职第一天 SRE/Ops 工程师 | **版本**: K8s 1.28-1.33

---

## 1. 环境准备

### 1.1 工具安装清单

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubectl - Kubernetes CLI
curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl"
sudo install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl
kubectl version --client  # 验证安装

# kubectx - 快速切换集群上下文（krew 插件）
kubectl krew install ctx
kubectl krew install switch

# k9s - 终端 UI（可选，推荐）
# https://github.com/derailed/k9s/releases

# stern - 日志实时跟踪
# https://github.com/stern/stern/releases

# jq - JSON 处理
sudo apt-get install -y jq  # Ubuntu
brew install jq              # macOS
```
### 1.2 配置 kubeconfig

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取集群 kubeconfig（联系组长提供）
mkdir -p ~/.kube
cp /path/to/cluster-config ~/.kube/config
chmod 600 ~/.kube/config

# 验证集群连接
kubectl cluster-info
kubectl get nodes

# 切换上下文
kubectl ctx production
kubectl ctx staging
```
---

## 2. 集群概览

### 2.1 快速了解集群状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看集群基本信息
kubectl cluster-info
kubectl version --short

# 查看所有节点
kubectl get nodes -o wide

# 查看系统组件
kubectl get pods -n kube-system

# 查看所有命名空间
kubectl get namespaces
```
### 2.2 理解集群架构

| 组件 | 查看命令 | 说明 |
|------|---------|------|
| API Server | `curl -sk https://localhost:6443/healthz` | 控制平面入口 |
| Scheduler | `kubectl get pods -n kube-system -l component=kube-scheduler` | Pod 调度器 |
| Controller Manager | `kubectl get pods -n kube-system -l component=kube-controller-manager` | 控制器循环 |
| [[etcd|etcd]] | `kubectl get pods -n kube-system -l component=etcd` | 数据存储 |
| [[kubelet|kubelet]] | `systemctl status kubelet` | 节点代理 |

---

## 3. 值班环境验证

### 3.1 验证 oncall 工具访问

```bash
# 1. 确认监控平台可访问（Prometheus/Grafana）
# 联系组长获取 URL 和账号

# 2. 确认告警平台可访问（AlertManager/PagerDuty）
# 确认手机告警能收到

# 3. 确认工单系统可访问（GitHub Issues/JIRA）
# 确认能创建和更新工单

# 4. 确认文档平台可访问
# 本知识库：http://kudig.example.com
```

### 3.2 验证基本操作权限

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 测试常用权限（应该全部返回 allowed）
kubectl auth can-i get pods --namespace=default
kubectl auth can-i create pods --namespace=default
kubectl auth can-i delete pods --namespace=default
kubectl auth can-i get nodes
kubectl auth can-i get services --namespace=kube-system

# 如果有 forbidden，联系组长补充权限
```
---

## 4. Day 1 自检清单

### 4.1 基础设施检查

- [ ] kubectl 已安装并能连接集群
- [ ] kubeconfig 已配置，切换到正确上下文
- [ ] 监控平台（Prometheus/Grafana）可访问
- [ ] 告警平台能收到测试告警
- [ ] 工单系统能创建和更新工单
- [ ] 文档平台（知识库）可访问

### 4.2 权限验证

- [ ] 能查看所有命名空间的 Pod
- [ ] 能查看所有节点
- [ ] 能查看 kube-system 组件状态
- [ ] 能创建/删除测试 Pod（需清理）
- [ ] 能查看 events 和 logs

### 4.3 知识储备自测

- [ ] 理解 kubectl get/describe/logs 命令
- [ ] 理解 Pod/Deployment/Service 关系
- [ ] 理解 Node/Pod 的调度流程
- [ ] 知道如何找到 oncall 联系人

---

## 5. 第一周学习任务

### Week 1 目标

| Day | 任务 | 完成 |
|-----|------|------|
| Day 1 | 工具安装 + 环境验证 | [ ] |
| Day 2 | 熟悉 kubectl 基本命令 | [ ] |
| Day 3 | 阅读故障排查手册（P1-5） | [ ] |
| Day 4 | 了解 oncall 流程和升级路径 | [ ] |
| Day 5 | 尝试处理一个简单工单 | [ ] |

### 推荐阅读顺序

1. `P1-5-oncall-quick-reference-card.md` — oncall 速查卡
2. `domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md` — kubectl 场景速查
3. `domain-10-troubleshooting-diagnostics/00-troubleshooting-overview.md` — 故障排查总览
4. `domain-11-production-operations/topic-learn/public-training/[[domain-04-storage-data/README.md|[[KUDIG Database]]]].md` — Week 1 培训

---

## 6. 常见问题

### Q: kubectl 连接超时
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 kubeconfig 配置
kubectl config view

# 检查 API Server 地址是否正确
grep server ~/.kube/config

# 测试 API Server 连通性
curl -sk https://<api-server-ip>:6443/healthz
```
### Q: 没有集群访问权限
- 联系组长/导师提供 kubeconfig
- 确认 IAM 角色配置正确

### Q: 权限不足（Forbidden）
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前用户
kubectl auth whoami

# 联系组长补充 RBAC 权限
```
---

```yaml
---
title: Day 1: 新人首日检查清单
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - "新人第一天要做什么"
  - "K8s环境怎么准备"
  - "oncall工具怎么验证"
  - "kubectl安装配置"
  - "ACK集群访问"
trigger_keywords:
  - "Day1检查清单"
  - "新人入职"
  - "kubectl配置"
  - "kubeconfig"
  - "监控平台"
  - "告警验证"
  - "权限验证"
reading_level: beginner
audience:
  - sre工程师
  - ops工程师
  - 新入职员工
estimated_read_time: 30min
related_domains:
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/quick-start/02-first-ticket-guide
  - domain-11-production-operations/topic-learn/quick-start/03-oncall-handoff
  - P1-5-oncall-quick-reference-card
id: QUICKSTART-DAY1
topic: onboarding
type: checklist
tags: [onboarding, day-1, setup, new-engineer, quick-start, k8s-1.28-1.33]
---
```

<!-- risk-assessed -->
