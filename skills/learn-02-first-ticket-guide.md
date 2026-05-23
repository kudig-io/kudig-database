---
title: 'Day 2: 第一个工单处理指南'
description: '- 第一个工单处理指南'
category: skills
tags:
- k8s
- learn
- quick-start
- etcd
- kubelet
- prometheus
- grafana
- coredns
- hpa
- daemonset
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 'Day 2: 第一个工单处理指南 是什么'
- '如何 Day 2: 第一个工单处理指南'
trigger_keywords:
- Day
- '2:'
- 第一个工单处理指南
prerequisites:
- kubectl-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
created: "2026-05-23"
---

trigger_keywords:
- Day
- '2:'
- 第一个工单处理指南
- learn  role: contributor---

# Day 2: 第一个工单处理指南

> **适用对象**: 第一次处理 K8s 工单的工程师 | **版本**: K8s 1.28-1.33

---

## 1. 工单处理流程

```
收到工单 → 分类 → 诊断 → 修复 → 验证 → 关闭
```

### 1.1 工单分类

| 工单类型 | 典型描述 | SLA | 处理优先级 | 示例场景 |
|---------|---------|-----|----------|---------|
| P0 | 集群不可用、大规模 Pod 问题 | 15min | 立即处理 | etcd 宕机、全部节点 NotReady |
| P1 | 单个服务问题、Pod 异常 | 30min | 快速处理 | 服务 503、Pod CrashLoopBackOff |
| P2 | 配置问题、非关键功能异常 | 2h | 计划处理 | ConfigMap 更新、HPA 未生效 |
| P3 | 咨询、文档建议 | 24h | 日常处理 | 最佳实践咨询、资源规划建议 |

### 1.2 分类问题清单

```bash
# 处理工单前，先快速确认
1. "哪个集群/命名空间？" → kubectl get namespaces
2. "哪个服务/Pod？" → kubectl get pods -n <ns>
3. "具体现象是什么？" → kubectl describe pod <pod> / kubectl logs
4. "从什么时候开始的？" → kubectl get events --sort-by='.lastTimestamp'
5. "有告警吗？" → 查看 Prometheus/Grafana
```

### 1.3 快速诊断命令集

```bash
# 集群级健康检查（30 秒完成）
echo "=== Cluster Info ==="
kubectl cluster-info
kubectl get nodes -o wide

echo "=== System Pods ==="
kubectl get pods -n kube-system | grep -v Running

echo "=== All Namespaces ==="
kubectl get namespaces

echo "=== Recent Events ==="
kubectl get events -A --sort-by='.lastTimestamp' | tail -20

echo "=== Resource Usage ==="
kubectl top nodes 2>/dev/null || echo "Metrics Server not available"
kubectl top pods -A --sort-by=memory 2>/dev/null | head -15 || echo "Metrics Server not available"
```

---

## 2. 场景一：Pod 问题（最常见）

### 场景描述
> "用户反映网站无法访问，检查发现 Pod 处于 CrashLoopBackOff 状态"

### 处理步骤

```bash
# Step 1: 确认问题
kubectl get pods -n <namespace> | grep -v Running

# 示例输出:
# NAME                           READY   STATUS             RESTARTS   AGE
# frontend-app-7d9f8c6b5-xk2lm   0/1     CrashLoopBackOff   5          10m
# backend-api-5c8f7d6b4-pq8rs    0/1     ImagePullBackOff   0          5m

# Step 2: 收集信息
kubectl describe pod <pod-name> -n <namespace>    # 查看 Events 和 Conditions
kubectl logs <pod-name> -n <namespace> --previous  # 查看上一个容器日志
kubectl get events -n <namespace> --sort-by='.lastTimestamp' | tail -20

# Step 3: 诊断根因
# 常见原因：
# - 应用配置错误（检查 logs）
# - 资源不足 OOM（kubectl top pods）
# - 依赖服务不可达（检查 upstream）
# - 镜像拉取失败（describe pod 看 ImagePull）

# CrashLoopBackOff 常见根因对照表:
# ┌────────────────────┬─────────────────────────┬──────────────────────┐
# │ 现象               │ 可能原因                 │ 排查命令              │
# ├────────────────────┼─────────────────────────┼──────────────────────┤
# │ Exit Code 1        │ 应用启动失败             │ kubectl logs <pod>    │
# │ Exit Code 137      │ OOMKilled               │ kubectl describe pod  │
# │ Exit Code 139      │ Segmentation Fault      │ kubectl logs <pod>    │
# │ Exit Code 143      │ SIGTERM 正常退出         │ 检查探针配置          │
# │ ImagePullBackOff   │ 镜像拉取失败             │ kubectl describe pod  │
# │ CrashLoopBackOff   │ 多种原因                 │ kubectl logs --prev   │
# └────────────────────┴─────────────────────────┴──────────────────────┘

# Step 4: 修复（先备份再操作）
# 如果是配置问题：kubectl edit deployment <name> -n <namespace>
# 如果是资源问题：
kubectl patch deployment <name> -n <namespace> -p '{"spec":{"template":{"spec":{"containers":[{"name":"app","resources":{"limits":{"memory":"2Gi"}}}]}}}}'
# 如果需要重启：kubectl rollout restart deployment <name> -n <namespace>
# 如果需要回滚：kubectl rollout undo deployment <name> -n <namespace>

# Step 5: 验证
kubectl get pods -n <namespace> -l app=<app-name>  # 确认 Pod Running
kubectl logs <pod-name> -n <namespace>             # 确认无新错误
curl -s http://<service-name>.<namespace>.svc.cluster.local/health  # 确认服务恢复
```

### 常用修复命令

```bash
# 重启 Deployment（最常用）
kubectl rollout restart deployment <name> -n <namespace>

# 删除 Pod（让 Deployment 重建）
kubectl delete pod <pod-name> -n <namespace>

# 更新镜像版本
kubectl set image deployment/<name> <container>=<image>:<tag> -n <namespace>

# 回滚到上一个版本
kubectl rollout undo deployment <name> -n <namespace>

# 回滚到指定版本
kubectl rollout undo deployment <name> -n <namespace> --to-revision=2

# 查看回滚历史
kubectl rollout history deployment <name> -n <namespace>

# 查看更新状态
kubectl rollout status deployment <name> -n <namespace>

# 扩缩容
kubectl scale deployment <name> -n <namespace> --replicas=5
```

---

## 3. 场景二：[[Service|Service]] 无法访问

### 场景描述
> "前端 Pod 无法调用后端 API，返回 503"

### 处理步骤

```bash
# Step 1: 确认问题
kubectl get svc -n <namespace>                    # 查看 Service
kubectl get endpoints -n <namespace> <svc-name>   # 查看 Endpoints（关键！）

# 示例输出（正常）:
# NAME           ENDPOINTS                           AGE
# backend-api    10.244.1.15:8080,10.244.1.16:8080   5d

# 示例输出（异常）:
# NAME           ENDPOINTS   AGE
# backend-api    <none>      5d

# Step 2: 如果 Endpoints 为空
kubectl get pods -n <namespace> -l <selector>      # 确认有 Pod
kubectl get pods -n <namespace> | grep Running     # 确认 Pod 在运行

# 常见原因:
# 1. Service selector 与 Pod labels 不匹配
# 2. Pod 没有 Running（CrashLoopBackOff 等）
# 3. Pod 的 readinessProbe 未通过

# 检查 selector 匹配
kubectl get svc <svc-name> -n <namespace> -o jsonpath='{.spec.selector}'
kubectl get pods -n <namespace> --show-labels

# Step 3: 测试 Service 连通性
kubectl run test --image=curlimages/curl --restart=Never -it -- sh
# curl http://backend-svc:80/health
# curl -v http://backend-svc.<namespace>.svc.cluster.local:80/health

# Step 4: 修复
# 如果 selector 不匹配: kubectl edit svc <svc-name> -n <namespace>
# 如果 Pod 不 Running: 按场景一处理
# 如果 readinessProbe 未通过: 检查探针配置

# Step 5: 验证
kubectl get endpoints <svc-name> -n <namespace>    # 确认有 Endpoints
curl -s http://<svc-name>.<namespace>.svc.cluster.local/health

# DNS 排查
kubectl run dns-test --rm -it --restart=Never --image=busybox:1.36 -- sh -c '
  echo "=== Testing DNS ==="
  nslookup backend-svc
  nslookup backend-svc.<namespace>.svc.cluster.local
  nslookup kubernetes.default
  echo "=== Testing Connectivity ==="
  wget -qO- --timeout=5 http://backend-svc:80/health || echo "Connection failed"
'
```

### Service 排障流程图

```
Service 无法访问
├── Endpoints 为空？
│   ├── Yes → Pod 不存在或不 Running
│   │   ├── Pod 不存在 → 检查 Deployment
│   │   ├── Pod Pending → 资源不足
│   │   ├── Pod CrashLoopBackOff → 场景一
│   │   └── Pod Running 但无 Endpoints → readinessProbe 失败
│   └── No → 有 Endpoints 但无法访问
│       ├── ClusterIP 可达 → 应用层问题
│       ├── ClusterIP 不可达 → kube-proxy / iptables 问题
│       └── DNS 解析失败 → CoreDNS 问题
└── Service 配置错误？
    ├── selector 不匹配
    ├── targetPort 错误
    └── type 配置错误
```

---

## 4. 场景三：节点问题

### 场景描述
> "某个节点变为 NotReady，节点上的 Pod 无法正常提供服务"

### 处理步骤

```bash
# Step 1: 确认问题
kubectl get nodes | grep -v Ready

# 示例输出:
# NAME            STATUS     ROLES    AGE   VERSION
# node-192-168-0-3   NotReady   <none>   30d   v1.28.3-aliyun.1

# Step 2: 收集信息
kubectl describe node <node-name>                # 查看 Conditions 和事件
kubectl get events --field-selector involvedObject.name=<node-name> --sort-by='.lastTimestamp'

# Step 3: 如果是临时问题
kubectl uncordon <node-name>                     # 等待自动恢复后解封

# Step 4: 如果需要维护节点
kubectl cordon <node-name>                        # 封禁节点
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 示例输出:
# node/node-192-168-0-3 cordoned
# node/node-192-168-0-3 drained

# Step 5: 修复节点后
kubectl uncordon <node-name>                     # 解封节点

# Step 6: 验证
kubectl get nodes | grep <node-name>             # 确认 Ready
kubectl get pods -o wide | grep <node-name>      # 确认 Pod 已调度回来
```

### 节点 NotReady 排障命令集

```bash
# 1. 检查节点 Conditions
kubectl describe node <node-name> | grep -A 5 "Conditions"

# 2. 通过云控制台检查 ECS 实例状态
# 运行中 / 已停止 / 其他状态

# 3. SSH 登录检查 kubelet（如果 ECS 在运行）
systemctl status kubelet
journalctl -u kubelet --no-pager -n 50
systemctl restart kubelet  # 必要时重启

# 4. 检查节点资源
df -h          # 磁盘空间
free -m        # 内存
top -bn1       # CPU

# 5. 检查网络
ping <api-server-ip>
curl -k https://<api-server-ip>:6443/healthz
```

---

## 5. 工单处理记录模板

```markdown
## 工单处理记录

**工单编号**: INC-2026-XXXX
**标题**: [简短描述]
**优先级**: P0/P1/P2/P3
**处理人**: [你的名字]
**开始时间**: [时间]
**结束时间**: [时间]

### 现象描述
[用户报告的现象]

### 诊断过程
1. [步骤1] kubectl get pods -n xxx
   - 发现：Pod 处于 CrashLoopBackOff

2. [步骤2] kubectl logs xxx --previous
   - 发现：错误信息 "connection refused to database"

3. [步骤3] 检查数据库 Service
   - 发现：数据库 Pod 不在 Running 状态

### 根因
[根本原因]

### 修复措施
1. [措施1]
2. [措施2]

### 验证
- [ ] Pod 状态正常
- [ ] 服务可访问
- [ ] 无相关告警

### 后续行动
- [ ] 通知相关团队
- [ ] 更新文档
- [ ] 预防措施

### 耗时
[处理时长]
```

---

## 6. 工单升级标准

### 立即升级（联系 SRE 值班）

- 控制平面节点 NotReady
- etcd 不健康或数据损坏
- 多节点同时 NotReady
- 证书全部过期
- 安全事件（入侵/数据泄露）
- 存储卷数据不可用

### 升级前准备

```bash
# 收集信息以便快速交接
kubectl get nodes -o wide > /tmp/nodes.txt
kubectl get pods -A > /tmp/pods.txt
kubectl get events -A --sort-by='.lastTimestamp' | tail -50 > /tmp/events.txt
kubectl get svc -A > /tmp/services.txt
kubectl top nodes > /tmp/top-nodes.txt 2>/dev/null
kubectl top pods -A > /tmp/top-pods.txt 2>/dev/null

# 打包
tar czf /tmp/k8s-debug-$(date +%Y%m%d%H%M).tar.gz /tmp/nodes.txt /tmp/pods.txt /tmp/events.txt /tmp/services.txt /tmp/top-nodes.txt /tmp/top-pods.txt
```

---

## 7. 高频场景速查

### 7.1 Pod 常见异常状态速查

| 状态 | 含义 | 快速排查 | 常见修复 |
|------|------|---------|---------|
| Pending | 等待调度 | `kubectl describe pod` 看 Events | 检查资源/selector/taint |
| ContainerCreating | 创建中 | `kubectl describe pod` 看 Events | 检查镜像/ConfigMap/Secret |
| CrashLoopBackOff | 崩溃循环 | `kubectl logs --previous` | 检查应用日志/配置 |
| ImagePullBackOff | 镜像拉取失败 | `kubectl describe pod` | 检查镜像名/凭证 |
| OOMKilled | 内存溢出 | `kubectl describe pod` | 增加内存 limits |
| Completed | 正常退出 | 检查是否为 Job | 按需处理 |
| Error | 异常退出 | `kubectl logs` | 检查退出码和日志 |

### 7.2 常用参考文档

| 场景 | 参考文档 |
|------|---------|
| Pod 问题 | `domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md` |
| Service 问题 | `P1-5-oncall-quick-reference-card.md` |
| 节点问题 | `domain-10-troubleshooting-diagnostics/02-node-notready-troubleshooting.md` |
| 网络问题 | `domain-10-troubleshooting-diagnostics/03-service-endpoints-troubleshooting.md` |

---

```yaml
---  - "第一个工单怎么处理"
  - "Pod CrashLoopBackOff怎么处理"
  - "Service无法访问怎么办"
  - "工单处理记录模板"
  - "K8s工单处理流程"  - "CrashLoopBackOff"
  - "ImagePullBackOff"
  - "Pod故障排查"
  - "Service Endpoints"
  - "工单处理"
  - "值班SOP"
  - "oncall入门"  - sre工程师
  - ops工程师
  - 运维新人
related_domains:
  - domain-02-workloads-applications
  - domain-03-networking-traffic
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/quick-start/01-day-one-checklist
  - domain-11-production-operations/topic-learn/quick-start/03-oncall-handoff
  - P1-5-oncall-quick-reference-card
id: QUICKSTART-DAY2
topic: onboarding
type: hands-on-guide
tags: [onboarding, first-ticket, troubleshooting, sre, ops-engineer, k8s-1.28-1.33]
---
```

## Related

- [[entities/kubelet|kubelet]] — kubelet
- [[coredns]] — CoreDNS
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)
