---
title: K8S 运维诊断技能库
description: Kubernetes 运维诊断全栈技能库，涵盖 Pod/Node/Network/Storage/Performance 五大故障域的结构化 SOP
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- etcd
- apiserver
- kubelet
- prometheus
- coredns
last_updated: 2026-04
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- K8S 运维诊断技能库 是什么
- 如何 K8S 运维诊断技能库
trigger_keywords:
- K8S
- 运维诊断技能库
- ai
- agent
prerequisites:
- kubectl-basics
- prometheus-basics
- etcd-basics
---

# K8S 运维诊断技能库

## 1. 技能覆盖范围

```
技能域全景:

├── Pod 故障域
│   ├── Pending（调度失败）
│   ├── CrashLoopBackOff（崩溃循环）
│   ├── OOMKilled（内存溢出）
│   ├── ImagePullBackOff（镜像拉取失败）
│   ├── Error / Unknown（其他异常）
│   └── Evicted（被驱逐）
│
├── Node 故障域
│   ├── NotReady（节点不就绪）
│   ├── MemoryPressure / DiskPressure / PIDPressure
│   ├── NetworkUnavailable
│   └── SchedulingDisabled
│
├── Network 故障域
│   ├── Service 不通
│   ├── DNS 解析失败
│   ├── Pod 间通信异常
│   ├── Ingress 不可达
│   └── NetworkPolicy 拦截
│
├── Storage 故障域
│   ├── PVC Pending
│   ├── 挂载失败
│   └── CSI 驱动异常
│
└── Performance 故障域
    ├── API Server 延迟高
    ├── etcd 延迟高
    ├── 调度延迟
    └── 网络延迟
```

## 2. Pod 故障诊断 SOP

### 2.1 Pod Pending

**触发条件**: Pod 状态为 Pending 超过 30 秒

```bash
# Step 1: 确认状态
kubectl get pod <pod> -n <ns> -o wide

# Step 2: 查看事件（关键！事件中有调度失败原因）
kubectl describe pod <pod> -n <ns> | tail -20
kubectl get events -n <ns> --field-selector involvedObject.name=<pod> --sort-by=.lastTimestamp

# Step 3: 根据事件分支诊断
```

| 事件关键词 | 根因 | 修复方向 |
|-----------|------|---------|
| `Insufficient cpu/memory` | 节点资源不足 | 扩容节点 / 调整 requests |
| `node(s) didn't match selector` | NodeSelector 不匹配 | 检查标签 / 修改选择器 |
| `node(s) had taint` | Taint/Toleration 不匹配 | 添加 Toleration / 移除 Taint |
| `persistentvolumeclaim not found` | PVC 未绑定 | 检查 PVC 状态和 StorageClass |
| `Unschedulable` | 节点不可调度 | 检查节点 SchedulingDisabled |
| `pod has unbound immediate PVC` | PVC 立即绑定未就绪 | 等待 PVC Ready 或检查 PV |

### 2.2 CrashLoopBackOff

**触发条件**: Pod 状态为 CrashLoopBackOff

```bash
# Step 1: 查看上一次日志（关键！）
kubectl logs <pod> -n <ns> --previous --tail=100

# Step 2: 检查容器退出码
kubectl get pod <pod> -n <ns> -o jsonpath='{.status.containerStatuses[*].lastState.terminated.exitCode}'

# Step 3: 检查探针配置
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.containers[*].livenessProbe}'
```

| 退出码 | 含义 | 常见原因 |
|--------|------|---------|
| 0 | 正常退出 | 应用主动退出，检查 restartPolicy |
| 1 | 应用错误 | 代码异常、配置错误、依赖不可达 |
| 137 | SIGKILL (OOM) | 内存超限，被 cgroup OOM Killer 终止 |
| 139 | SIGSEGV | 段错误，应用 bug |
| 143 | SIGTERM | 被 K8S 终止（如 preStop hook 超时） |

### 2.3 OOMKilled

**触发条件**: Pod 因 OOM 被终止

```bash
# Step 1: 确认 OOM
kubectl describe pod <pod> -n <ns> | grep -A 5 "Last State:"
kubectl get events -n <ns> --field-selector reason=OOMKilling

# Step 2: 对比 limits vs 实际使用
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.containers[*].resources}'
kubectl top pod <pod> -n <ns> --containers

# Step 3: 分析内存趋势（Prometheus）
# sum(container_memory_working_set_bytes{namespace="<ns>",pod="<pod>"}) by (container)
```

**修复决策树**:
```
实际使用 > limits?
  ├── 是 → limits 设置过低
  │   ├── 调整 limits（推荐为实际峰值的 1.5 倍）
  │   └── 同步调整 requests（推荐为实际均值的 1.2 倍）
  └── 否 → 应用内存泄漏
      ├── 分析内存增长趋势
      ├── 检查 JVM heap / Go GC / Python 内存管理
      └── 建议应用团队排查内存泄漏
```

### 2.4 ImagePullBackOff

```bash
# Step 1: 检查镜像名称和 tag
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.containers[*].image}'

# Step 2: 检查 imagePullSecrets
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.imagePullSecrets}'
kubectl get secret <secret> -n <ns> -o jsonpath='{.type}'

# Step 3: 检查事件中的详细错误
kubectl describe pod <pod> -n <ns> | grep -A 5 "Failed"
```

| 错误信息 | 根因 | 修复 |
|---------|------|------|
| `repository does not exist` | 镜像名称错误 | 核实镜像地址 |
| `unauthorized` | 认证失败 | 检查 imagePullSecret 是否正确 |
| `manifest unknown` | Tag 不存在 | 核实 Tag 是否已推送 |
| `timeout` | 网络不通 | 检查节点到 Registry 的网络连通性 |

## 3. Node 故障诊断 SOP

### 3.1 Node NotReady

```bash
# Step 1: 确认状态和 Conditions
kubectl get nodes -o wide
kubectl get node <node> -o jsonpath='{.status.conditions}' | python3 -m json.tool

# Step 2: 检查 kubelet
kubectl get --raw /api/v1/nodes/<node>/proxy/healthz 2>/dev/null || echo "kubelet 不可达"

# Step 3: 检查节点事件
kubectl get events --field-selector involvedObject.name=<node> --sort-by=.lastTimestamp

# Step 4: 资源压力检查
kubectl top node <node>
kubectl describe node <node> | grep -A 5 "Allocated resources:"
```

**NotReady 根因决策树**:
```
Node NotReady
├── kubelet 不响应
│   ├── kubelet 进程挂了 → 重启 kubelet
│   ├── 节点 SSH 不通 → 物理/VM 层面故障
│   └── 证书过期 → 更新 kubelet 证书
├── MemoryPressure = True
│   ├── 系统内存不足 → 清理内存 / 扩容
│   └── 单个 Pod 内存泄漏 → 定位并重启 Pod
├── DiskPressure = True
│   ├── 容器日志占满 → 清理日志
│   ├── 镜像缓存过多 → docker/containerd image prune
│   └── 数据卷满 → 扩容磁盘
└── NetworkUnavailable = True
    ├── CNI 插件异常 → 检查 CNI Pod 状态
    └── 网络配置错误 → 检查路由和 iptables
```

## 4. Network 故障诊断 SOP

### 4.1 Service 不通

```bash
# Step 1: 确认 Service 和 Endpoints
kubectl get svc <svc> -n <ns>
kubectl get endpoints <svc> -n <ns>

# Step 2: Endpoints 为空？
kubectl get pods -n <ns> -l <selector> --show-labels

# Step 3: DNS 测试
kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup <svc>.<ns>.svc.cluster.local

# Step 4: 连通性测试
kubectl run net-test --image=busybox:1.36 --rm -it --restart=Never -- wget -qO- --timeout=5 http://<svc>.<ns>:<port>

# Step 5: 检查 NetworkPolicy
kubectl get networkpolicy -n <ns>
```

### 4.2 DNS 解析失败

```bash
# Step 1: CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# Step 2: CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml

# Step 3: DNS 测试
kubectl run dns-debug --image=busybox:1.36 --rm -it --restart=Never -- nslookup kubernetes.default
```

## 5. Storage 故障诊断 SOP

### 5.1 PVC Pending

```bash
# Step 1: 查看 PVC 状态
kubectl get pvc -n <ns>
kubectl describe pvc <pvc> -n <ns>

# Step 2: 检查 StorageClass
kubectl get sc
kubectl describe sc <sc-name>

# Step 3: 检查 PV
kubectl get pv | grep <pvc>

# Step 4: CSI 驱动状态
kubectl get pods -n kube-system -l app=csi-*
```

## 6. Performance 诊断 SOP

### 6.1 API Server 延迟高

```bash
# Step 1: 确认延迟
# histogram_quantile(0.99, sum(rate(apiserver_request_duration_seconds_bucket[5m])) by (le, verb))

# Step 2: 检查请求量
# sum(rate(apiserver_request_total[5m])) by (verb, resource)

# Step 3: 检查 etcd 延迟
# histogram_quantile(0.99, sum(rate(etcd_disk_wal_fsync_duration_seconds_bucket[5m])) by (le))

# Step 4: 检查审计日志量
kubectl logs -n kube-system -l component=kube-apiserver --tail=20
```

## 7. 输出格式模板

所有诊断结果必须按以下格式输出：

```markdown
## 诊断结果

### 1. 现象
[一句话描述异常状态]

### 2. 根因
[基于数据的根本原因分析，标注置信度]
- 置信度: 高/中/低
- 数据来源: [具体的命令或查询]

### 3. 修复方案
[可直接执行的命令和步骤]
- 风险等级: 低/中/高
- 影响范围: [受影响的资源]
- 回滚方案: [回滚命令]

### 4. 验证方法
[修复后确认问题已解决的命令]

### 5. 预防建议
[避免再次发生的措施]
```

## 8. 知识库关联

| 故障域 | kudig-database 参考文档 |
|--------|------------------------|
| Pod 故障 | `domain-10-troubleshooting-diagnostics/05-pod-pending-diagnosis.md` ~ `08-pod-comprehensive-troubleshooting.md` |
| Node 故障 | `domain-10-troubleshooting-diagnostics/06-node-notready-diagnosis.md`, `09-node-comprehensive-troubleshooting.md` |
| Network 故障 | `domain-10-troubleshooting-diagnostics/25-network-connectivity-troubleshooting.md`, `26-dns-troubleshooting.md` |
| Storage 故障 | `domain-10-troubleshooting-diagnostics/14-pvc-storage-troubleshooting.md`, `[[domain-10-troubleshooting-diagnostics/04-storage-csi-troubleshooting|04-storage-csi-troubleshooting]].md` |
| 性能问题 | `domain-10-troubleshooting-diagnostics/33-performance-bottleneck-troubleshooting.md` |
| 故障树 | `domain-10-troubleshooting-diagnostics/topic-fta/` 完整故障树分析模型 |

---

*本文件定义 Agent 的领域知识和操作流程。更新 SOP 时请同步更新 kudig-database 对应文档。*

## Related

- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/observability-index|Observability 可观测性知识图谱索引]]
- [[domain-19-landscape-references/topic-index/node-index|Node 知识图谱索引]]
