---
title: 'Day 19: 故障排查方法论 (关键日)'
description: 'title: Day 19: 故障排查方法论 (关键日)'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- scheduler
- prometheus
- coredns
- containerd
- networkpolicy
- rag
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 19: 故障排查方法论 (关键日) 是什么'
- '如何 Day 19: 故障排查方法论 (关键日)'
- 'Day 19: 故障排查方法论 (关键日) 故障排查'
- 'Day 19: 故障排查方法论 (关键日) 排障步骤'
trigger_keywords:
- Day
- '19:'
- 故障排查方法论
- 关键日
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
---

---
title: Day 19: 故障排查方法论 (关键日)
last_updated: 2026-05-18
difficulty: intermediate
intent_queries:
  - kubernetes 故障排查方法论 FTA
  - FEBM 取证循证方法
  - k8s 故障树分析
  - 结构化排障流程
trigger_keywords:
  - FTA
  - FEBM
  - 故障树
  - 取证循证
  - 故障排查
  - 故障树分析
  - 根因分析
reading_level: intermediate
audience:
  - sre-engineer
  - devops-engineer
  - platform-engineer
estimated_read_time: 240min
related_domains:
  - domain-10-troubleshooting-diagnostics
  - topic-fta
  - topic-febm
related_topics:
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-20-troubleshooting-practice
  - domain-11-production-operations/topic-learn/public-training/one-month/week-3-operations/day-21-platform-ops
  - domain-11-production-operations/topic-learn/public-training/one-month/projects/p3-observability-fault-drill
---

# Day 19: 故障排查方法论 (关键日)

> **学习时间**: 4-5 小时 | **主题**: FTA/FEBM 结构化故障排查

---

## 概述

故障排查是运维工程师最核心的能力。传统的"试错法"效率低下且容易引入新问题。今天你将学习两种结构化的故障排查方法论：FTA（故障树分析）和 FEBM（取证循证方法）。FTA 帮助你构建全面的排查框架，FEBM 帮助你在每一步做出准确的判断。掌握这两种方法将使你的排障效率大幅提升。

---

## 今日目标

- [ ] 理解 FTA 故障树分析方法
- [ ] 掌握 FEBM 取证循证方法论
- [ ] 建立结构化排障思维框架

---

## 核心概念

### 1. 故障排查方法对比

| 方法 | 特点 | 优势 | 劣势 |
|------|------|------|------|
| 试错法 | 随机尝试 | 不需要学习 | 效率低、可能引入新问题 |
| 经验法 | 凭经验判断 | 快速 (经验丰富时) | 不可靠、难传承 |
| FTA | 自顶向下分解 | 系统化、不遗漏 | 需要构建故障树 |
| FEBM | 证据驱动 | 准确、可复现 | 需要数据支撑 |

### 2. FTA vs FEBM 定位

```
FTA: 解决"排查什么"的问题 (框架)
  └── 提供系统化的排查路径

FEBM: 解决"怎么判断"的问题 (方法)
  └── 提供证据驱动的决策依据

两者结合:
  用 FTA 构建排查框架
  用 FEBM 在框架内每一步做出准确判断
```

### 3. K8s 故障分类

| 故障层级 | 典型故障 | 排查入口 |
|----------|---------|---------|
| Pod 级 | Pending/CrashLoop/ImagePull | kubectl describe pod |
| Service 级 | 无 Endpoints/DNS 失败 | kubectl get endpoints |
| 节点级 | NotReady/DiskPressure | kubectl describe node |
| 控制面级 | API 不可用/调度失败 | kubectl get componentstatuses |
| 网络级 | Pod 间不通/外网不可达 | NetworkPolicy/iptables |

---

## 理论学习 (2h) - 方法论精读

### 必读文档

1. **结构化故障排查框架**
   - 文件: `../../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/[[domain-07-platform-engineering/topic-code-analysis/deployment-create/README|README]].md`
   - 重点: 排障框架总览

2. **FTA 核心原理**
   - 文件: `../../domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md`
   - 重点: 故障树构建、根因分析

3. **FEBM 理论基础**
   - 文件: `../../domain-10-troubleshooting-diagnostics/topic-febm/01-febm-theory-foundations.md`
   - 重点: 取证循证方法论

---

## 实战演练 (2.5h)

### 任务 1: 理解 FTA 故障树 (45min)

#### 1.1 FTA 核心概念

```
FTA (Fault Tree Analysis) 基本元素:

┌──────────────────────────────┐
│    顶事件 (Top Event)         │  ← 要分析的故障现象
└──────────┬───────────────────┘
           │
    ┌──────┴──────┐
    │  OR 门       │              ← 任一子事件发生即触发
    └──────┬──────┘
    ┌──────┴──────────────┐
    │                      │
┌───┴──────┐        ┌─────┴──────┐
│ 中间事件  │        │ 中间事件    │  ← 进一步分解的子故障
└───┬──────┘        └─────┬──────┘
    │                      │
┌───┴──┐              ┌───┴──┐
│基本  │              │基本  │      ← 不可再分的根因
│事件  │              │事件  │
└──────┘              └──────┘
```

逻辑门类型:

| 门类型 | 含义 | 示例 |
|--------|------|------|
| OR 门 | 任一子事件发生，父事件就发生 | CPU 高 OR 内存高 → 资源不足 |
| AND 门 | 所有子事件都发生，父事件才发生 | CPU 高 AND 内存高 → 资源耗尽 |

#### 1.2 示例: Pod 无法启动故障树

```
Pod 无法启动 (顶事件)
├── [OR] 调度失败
│   ├── [AND] 资源不足
│   │   ├── CPU 请求超过节点容量
│   │   └── Memory 请求超过节点容量
│   ├── [OR] 调度约束不满足
│   │   ├── nodeSelector 无匹配节点
│   │   ├── nodeAffinity 不满足
│   │   ├── podAntiAffility 冲突
│   │   └── Taints/Tolerations 阻止
│   └── PVC Pending
│       ├── StorageClass 不存在
│       └── 无可用 PV
├── [OR] 镜像问题
│   ├── 镜像不存在 (ImagePullBackOff)
│   ├── 拉取权限不足 (需要 imagePullSecrets)
│   └── 网络不通 (无法连接 Registry)
└── [OR] 启动失败
    ├── 配置错误 (ConfigMap/Secret 缺失)
    ├── 健康检查失败 (readiness/liveness probe)
    ├── OOMKilled (内存限制过小)
    └── 应用自身错误 (exit code != 0)
```

#### 1.3 示例: Service 不可达故障树

```
Service 不可达 (顶事件)
├── [OR] DNS 解析失败
│   ├── CoreDNS Pod 异常
│   ├── Service 名称错误
│   └── 跨 Namespace 访问未加后缀
├── [OR] Endpoints 为空
│   ├── Selector 与 Pod 标签不匹配
│   ├── Pod 未 Ready (readinessProbe 失败)
│   └── Pod 不存在 (Deployment 副本为 0)
├── [OR] 网络不通
│   ├── NetworkPolicy 阻止
│   ├── CNI 插件异常
│   └── iptables/IPVS 规则错误
└── [OR] Service 配置错误
    ├── 端口不匹配
    ├── targetPort 错误
    └── ClusterIP 冲突
```

#### 1.4 示例: Node NotReady 故障树

```
Node NotReady (顶事件)
├── [OR] kubelet 异常
│   ├── kubelet 进程崩溃
│   ├── kubelet 证书过期
│   └── kubelet 资源不足
├── [OR] 节点资源耗尽
│   ├── 磁盘满 (DiskPressure)
│   ├── 内存不足 (MemoryPressure)
│   └── PID 耗尽 (PIDPressure)
├── [OR] 网络异常
│   ├── 节点网络不通
│   ├── API Server 不可达
│   └── DNS 解析失败
└── [OR] 运行时异常
    ├── containerd 进程崩溃
    ├── 镜像存储满
    └── 内核异常 (OOM Killer)
```

---

### 任务 2: Pod Pending 完整排障 (45min)

参考 `../../domain-10-troubleshooting-diagnostics/05-pod-pending-diagnosis.md`

#### 2.1 创建会 Pending 的 Pod

```bash
cat > pending-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: pending-test
spec:
  containers:
  - name: app
    image: nginx:alpine
    resources:
      requests:
        cpu: "100"
        memory: "1000Gi"
EOF

kubectl apply -f pending-pod.yaml
```

#### 2.2 按 FEBM 方法论排查

```
Step 1: 收集证据
```

```bash
# 证据 1: Pod 状态
kubectl get pod pending-test
# NAME            STATUS    RESTARTS   AGE
# pending-test    Pending   0          30s

# 证据 2: Pod 详情和事件
kubectl describe pod pending-test | tail -20
# Events:
#   Type     Reason            From        Message
#   Warning  FailedScheduling  default-scheduler  0/3 nodes available:
#     Insufficient cpu (3), Insufficient memory (3).

# 证据 3: 节点资源
kubectl get nodes -o custom-columns='NAME:.metadata.name,CPU:.status.allocatable.cpu,MEMORY:.status.allocatable.memory'
```

```
Step 2: 分析证据
- 状态: Pending
- 原因: FailedScheduling
- 细节: CPU 和 Memory 不足

Step 3: 形成假设
- 假设: Pod 请求的资源超过了所有节点的可用资源

Step 4: 验证假设
```

```bash
# 查看节点实际可用资源
kubectl describe node <node-name> | grep -A 5 Allocatable
# Allocatable:
#   cpu:     4          ← Pod 请求 100 CPU
#   memory: 16384Mi     ← Pod 请求 1000Gi

# 假设验证: 确认 Pod 请求远超节点容量
```

```
Step 5: 修复
```

```bash
kubectl delete pod pending-test

# 修改资源请求后重新创建
cat > fixed-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: fixed-test
spec:
  containers:
  - name: app
    image: nginx:alpine
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
EOF

kubectl apply -f fixed-pod.yaml
kubectl get pod fixed-test
# NAME         STATUS   RESTARTS   AGE
# fixed-test   Running  0          10s
```

---

### 任务 3: Node NotReady 排障 (45min)

参考 `../../domain-10-troubleshooting-diagnostics/06-node-notready-diagnosis.md`

按故障树路径逐层排查:

```bash
echo "========== Node NotReady 排查 =========="

# 检查 1: 确认 NotReady 节点
echo "--- Step 1: 确认节点状态 ---"
kubectl get nodes
kubectl get nodes | grep -v Ready

# 检查 2: 查看节点 Conditions
echo "--- Step 2: 节点 Conditions ---"
kubectl describe node <node-name> | grep -A 20 Conditions
# 关注: Ready, DiskPressure, MemoryPressure, PIDPressure, NetworkUnavailable

# 检查 3: kubelet 状态 (在节点上 SSH 执行)
echo "--- Step 3: kubelet 状态 ---"
systemctl status kubelet
journalctl -u kubelet -n 100 --no-pager

# 检查 4: 容器运行时
echo "--- Step 4: 容器运行时 ---"
systemctl status containerd
crictl ps | wc -l
crictl pods | wc -l

# 检查 5: 网络连通性
echo "--- Step 5: 网络连通性 ---"
ping -c 3 <api-server-ip>
curl -sk https://<api-server>:6443/healthz
# 应返回: ok

# 检查 6: 磁盘空间
echo "--- Step 6: 磁盘空间 ---"
df -h
du -sh /var/lib/kubelet/*
du -sh /var/lib/containerd/*

# 检查 7: 内存
echo "--- Step 7: 内存 ---"
free -h
cat /proc/meminfo | grep -E 'MemTotal|MemFree|MemAvailable'

# 检查 8: 系统日志 (检查 OOM Killer)
echo "--- Step 8: 系统日志 ---"
dmesg | grep -i "oom" | tail -10
dmesg | grep -i "kill" | tail -10

echo "========== 排查完毕 =========="
```

节点 Conditions 说明:

| Condition | True 含义 | 影响 |
|-----------|----------|------|
| Ready | 节点不健康 | Pod 会被驱逐 |
| DiskPressure | 磁盘空间不足 | 不再调度新 Pod |
| MemoryPressure | 内存不足 | 不再调度新 Pod |
| PIDPressure | PID 耗尽 | 不再调度新 Pod |
| NetworkUnavailable | 网络未正确配置 | 节点网络异常 |

---

### 任务 4: FEBM 实践 (30min)

#### 4.1 FEBM 核心流程

```
┌─────────────────────────────────────┐
│ 1. 收集证据                          │
│    - kubectl describe / get / logs   │
│    - Prometheus 指标                  │
│    - 系统日志 (dmesg/journalctl)      │
│    - 网络测试 (ping/curl/traceroute)  │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│ 2. 分析证据                          │
│    - 时间线重建                       │
│    - 因果关系分析                     │
│    - 排除法 (排除不可能的原因)         │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│ 3. 形成假设                          │
│    - 基于证据推理                     │
│    - 列出可能的根因                   │
│    - 按可能性排序                     │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│ 4. 验证假设                          │
│    - 设计验证实验                     │
│    - 执行验证命令                     │
│    - 确认或否定假设                   │
└──────────────┬──────────────────────┘
               ↓
┌─────────────────────────────────────┐
│ 5. 记录结论                          │
│    - 根因                            │
│    - 修复方案                        │
│    - 预防措施                        │
└─────────────────────────────────────┘
```

#### 4.2 故障报告模板

```markdown
## 故障报告

**时间**: YYYY-MM-DD HH:MM
**报告人**: [名字]
**影响**: [影响范围和程度]

### 1. 现象
- 观察到的症状 (具体、可量化)

### 2. 证据收集

| 证据 | 来源 | 内容 |
|------|------|------|
| Pod 状态 | kubectl get pod | CrashLoopBackOff |
| 日志 | kubectl logs --previous | java.lang.OutOfMemoryError |
| 事件 | kubectl describe pod | OOMKilled |
| 指标 | Prometheus | 内存使用率峰值 98% |

### 3. 分析过程

| 假设 | 验证方法 | 结果 |
|------|---------|------|
| 假设 1: 内存泄漏 | 查看内存趋势图 | 确认，内存持续增长 |
| 假设 2: limits 过小 | 对比实际使用量 | 否定，limits 已足够 |
| 假设 3: 配置错误 | 检查 JVM 参数 | 确认，-Xmx 设置不合理 |

### 4. 根因
[确定的根本原因]

### 5. 修复方案

| 方案 | 类型 | 描述 |
|------|------|------|
| 临时缓解 | 热修复 | 增大 limits.memory |
| 永久修复 | 代码修复 | 修复内存泄漏 |

### 6. 预防措施
- [ ] 添加内存使用告警 (> 85%)
- [ ] 添加 CI 内存泄漏检测
- [ ] 代码 review 关注资源释放
```

---

## 费曼复述 (0.5h)

1. **FTA 故障树的 AND 门和 OR 门分别在什么场景使用？**
   - AND: 多个条件同时满足才触发 (如: CPU 高 AND 内存高)
   - OR: 任一条件触发 (如: CPU 高 OR 内存高)

2. **FEBM 方法论的核心步骤是什么？**
   - 收集证据 → 分析证据 → 形成假设 → 验证假设 → 记录结论

3. **为什么结构化排障比"经验排障"更可靠？**
   - 不遗漏排查路径
   - 可复现、可传承
   - 证据驱动，避免主观臆断

---

## 今日检验

- [ ] 能够画出简单故障的 FTA 故障树
- [ ] 能够按照结构化流程排查 Pod 问题
- [ ] 能够使用 FEBM 方法记录排障过程

---

## 核心排障命令速查

| 场景 | 命令 | 用途 |
|------|------|------|
| Pod 状态 | `kubectl get pods -o wide` | 查看 Pod 状态和所在节点 |
| Pod 详情 | `kubectl describe pod <name>` | Events、Conditions、资源 |
| Pod 日志 | `kubectl logs <name> --previous` | 上次崩溃的日志 |
| Pod 进入 | `kubectl exec -it <name> -- sh` | 进入容器内部检查 |
| 事件 | `kubectl get events --sort-by='.lastTimestamp'` | 最近事件 |
| 节点状态 | `kubectl describe node <name>` | Conditions、资源、Pod 列表 |
| API 健康 | `kubectl get --raw /healthz` | API Server 健康检查 |
| DNS 测试 | `kubectl run test --image=busybox --rm -it -- nslookup <svc>` | DNS 解析验证 |
| 端口转发 | `kubectl port-forward svc/<name> 8080:80` | 本地测试 Service |
| 资源使用 | `kubectl top pods/nodes` | CPU/内存使用率 |

---

## 延伸阅读

- [结构化故障排查框架](../../domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/README.md)
- [FTA 核心原理](../../domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md)
- [FEBM 理论基础](../../domain-10-troubleshooting-diagnostics/topic-febm/01-febm-theory-foundations.md)
- [Pod Pending 诊断](../../domain-10-troubleshooting-diagnostics/05-pod-pending-diagnosis.md)
- [Node NotReady 诊断](../../domain-10-troubleshooting-diagnostics/06-node-notready-diagnosis.md)
- [Pod 综合排障](../../domain-10-troubleshooting-diagnostics/08-pod-comprehensive-troubleshooting.md)
