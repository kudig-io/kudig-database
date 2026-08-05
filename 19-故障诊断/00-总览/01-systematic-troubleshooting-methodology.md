---
title: Systematic Kubernetes Troubleshooting Methodology — From Symptom to Root Cause
description: K8s 故障诊断方法论 — 系统化排障流程、分层诊断、决策树、工具链、根因分析框架
summary: 建立系统化的 Kubernetes 故障诊断方法论，从症状识别到根因定位的完整排障体系
category: practice
tags:
- troubleshooting
- methodology
- root-cause-analysis
- debugging
- decision-tree
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: troubleshooting
---
# 系统化故障诊断方法论

> 从症状到根因的结构化排障框架，避免盲目猜测。

## 诊断原则

```
┌─────────────────────────────────────────────────────────────┐
│  故障诊断黄金法则                                            │
│                                                             │
│  1. 观察先于行动 — 先收集证据，再执行变更                      │
│  2. 分层定位 — 网络/计算/存储/应用逐层排除                    │
│  3. 二分法缩小 — 每次排除一半可能性                           │
│  4. 变更即嫌疑 — 最近变更是首要怀疑对象                       │
│  5. 可复现优先 — 能复现的问题已解决一半                       │
│  6. 记录一切 — 时间线、命令输出、假设                         │
└─────────────────────────────────────────────────────────────┘
```

## 分层诊断模型

```
Layer 7: 应用逻辑    ← 业务错误、数据不一致
Layer 6: 服务通信    ← DNS、Service、Ingress、超时
Layer 5: 容器运行时  ← OOM、CrashLoop、镜像问题
Layer 4: 调度/资源   ← Pending、Eviction、CPU Throttle
Layer 3: 网络基础设施 ← CNI、路由、防火墙、MTU
Layer 2: 节点/OS     ← 磁盘满、内核错误、时钟偏移
Layer 1: 基础设施    ← 云 API、硬件故障、证书过期
```

## 通用诊断流程

### Phase 1: 症状收集（< 2 min）

```bash
# 快速全景扫描
kubectl get events -A --sort-by='.lastTimestamp' | tail -30
kubectl get pods -A --field-selector status.phase!=Running
kubectl get nodes -o wide
kubectl top nodes
kubectl top pods -A --sort-by=memory | head -20

# 最近变更
kubectl get events -A --field-selector reason=Killing --sort-by='.lastTimestamp'
kubectl get deploy -A -o json | jq -r '.items[] | 
  select(.metadata.annotations["kubectl.kubernetes.io/last-applied-configuration"] != null) |
  "\(.metadata.namespace)/\(.metadata.name)"'
```

### Phase 2: 范围界定（< 5 min）

```bash
# 确定影响范围
# 单 Pod？单 Deployment？单 Namespace？全集群？

# 单 Pod 问题
kubectl describe pod <pod> -n <ns>
kubectl logs <pod> -n <ns> --previous  # 上次崩溃日志
kubectl logs <pod> -n <ns> --all-containers

# 节点问题
kubectl describe node <node>
kubectl get events --field-selector involvedObject.name=<node>

# 集群级问题
kubectl get componentstatuses 2>/dev/null
kubectl -n kube-system get pods
etcdctl endpoint health --cluster
```

### Phase 3: 假设验证（< 15 min）

```bash
# 假设: DNS 问题
kubectl exec -it <pod> -- nslookup kubernetes.default
kubectl exec -it <pod> -- cat /etc/resolv.conf
kubectl -n kube-system logs -l k8s-app=kube-dns --tail=50

# 假设: 资源不足
kubectl describe pod <pod> | grep -A5 "Events"
kubectl get events --field-selector reason=FailedScheduling
kubectl describe node <node> | grep -A10 "Allocated resources"

# 假设: 网络策略阻断
kubectl get networkpolicy -n <ns>
kubectl exec -it <pod> -- wget -qO- --timeout=3 http://target-svc:8080/health

# 假设: 存储问题
kubectl get pvc -n <ns>
kubectl describe pvc <pvc> -n <ns>
kubectl get pv | grep <pvc>
```

### Phase 4: 根因确认与修复

```bash
# 确认根因后执行修复
# 修复 → 验证 → 监控 → 记录

# 验证修复
kubectl rollout status deploy/<name> -n <ns>
kubectl get pods -n <ns> -l app=<name> -w

# 持续监控
kubectl logs -f deploy/<name> -n <ns> --tail=100
```

## 常见故障决策树

### Pod CrashLoopBackOff

```
CrashLoopBackOff
├── kubectl logs --previous
│   ├── OOMKilled → 增加 memory limits / 排查内存泄漏
│   ├── 应用错误（panic/exception）→ 修复代码 / 回滚版本
│   ├── 配置错误（env/configmap）→ 检查配置
│   └── 依赖不可用（DB/Redis）→ 检查依赖服务
├── kubectl describe pod
│   ├── ImagePullBackOff → 镜像/Registry 问题
│   ├── CreateContainerConfigError → Secret/ConfigMap 缺失
│   └── 探针失败 → 调整探针参数
└── 节点问题
    ├── 磁盘压力 → 清理/扩容
    └── 内核错误 → dmesg 检查
```

### Pod Pending

```
Pending
├── kubectl describe pod → Events
│   ├── Insufficient cpu/memory → 扩容节点 / 调整 requests
│   ├── node(s) had taint → 添加 toleration / 移除 taint
│   ├── pod has unbound PVC → 检查 StorageClass/PV
│   └── didn't match affinity → 检查亲和性规则
├── kubectl get nodes
│   ├── NotReady → 节点故障
│   └── 资源已满 → 驱逐低优先级 Pod / 扩容
└── 调度器问题
    └── kubectl -n kube-system logs kube-scheduler
```

### Service 不可达

```
Service 不可达
├── ClusterIP 不通
│   ├── Endpoints 为空 → 检查 selector 匹配
│   ├── kube-proxy 异常 → 检查 iptables/ipvs 规则
│   └── NetworkPolicy 阻断 → 检查策略
├── DNS 解析失败
│   ├── CoreDNS Pod 状态 → 重启/扩容
│   ├── ndots 配置 → 调整 dnsConfig
│   └── 上游 DNS 超时 → 检查 /etc/resolv.conf
└── Ingress 不通
    ├── Ingress Controller 状态
    ├── TLS 证书过期
    └── 后端 Service 健康
```

## 高级诊断工具

### 网络诊断

```bash
# 临时调试 Pod（带完整工具）
kubectl run debug --rm -it --image=nicolaka/netshoot -- bash

# 在调试 Pod 中:
# DNS 测试
dig kubernetes.default.svc.cluster.local
dig @10.96.0.10 my-service.my-ns.svc.cluster.local

# 连通性测试
curl -v http://my-service:8080/health
tcpdump -i eth0 -nn port 8080

# Service 规则检查（在节点上）
iptables-save | grep <service-cluster-ip>
ipvsadm -Ln | grep <cluster-ip>

# 跨节点连通性
ping <pod-ip-on-other-node>
traceroute <pod-ip>
```

### 性能诊断

```bash
# CPU 分析
kubectl top pods -n <ns> --sort-by=cpu
# 进入容器
kubectl exec -it <pod> -- top -H -p 1

# 内存分析
kubectl exec -it <pod> -- cat /proc/1/status | grep -i vm
kubectl exec -it <pod> -- cat /sys/fs/cgroup/memory.stat

# 网络延迟
kubectl exec -it <pod> -- ping -c 10 <target>
kubectl exec -it <pod> -- curl -o /dev/null -s -w '%{time_total}\n' http://target/

# 磁盘 IO
kubectl exec -it <pod> -- iostat -x 1 5
kubectl exec -it <pod> -- df -h
```

### 临时调试容器

```bash
# 为运行中的 Pod 添加调试容器
kubectl debug -it <pod> --image=nicolaka/netshoot --target=<container> -- bash

# 节点级调试
kubectl debug node/<node> -it --image=ubuntu -- bash
# 进入后:
chroot /host
journalctl -u kubelet --since "10 min ago"
dmesg | tail -50
```

## 根因分析框架（5-Why）

```markdown
## 事件: 生产 API 延迟飙升

### 时间线
- 14:00 告警触发 P99 > 2s
- 14:02 确认非流量突增（QPS 正常）
- 14:05 发现 DB 连接池耗尽
- 14:08 定位到慢查询（全表扫描）
- 14:10 确认是 14:00 上线的新功能引入

### 5-Why 分析
1. Why: API 延迟高 → DB 连接池耗尽
2. Why: 连接池耗尽 → 慢查询占用连接
3. Why: 慢查询 → 缺少索引的全表扫描
4. Why: 缺少索引 → 新功能 SQL 未走 Review
5. Why: 未 Review → CI 缺少 SQL 审计检查

### 修复
- 即时: 添加索引 + 回滚
- 长期: CI 添加 SQL 审计（pt-query-advisor）
```

## 诊断工具速查

| 场景 | 工具 | 命令 |
|------|------|------|
| Pod 状态 | kubectl | `describe`, `logs`, `get events` |
| 网络抓包 | tcpdump/netshoot | `tcpdump -i any -nn port 80` |
| DNS 诊断 | dig/nslookup | `dig @<dns-ip> <svc>` |
| 资源监控 | kubectl top / Prometheus | `top pods --sort-by=cpu` |
| 内核追踪 | bpftrace/perf | `bpftrace -e '...'` |
| 应用 Profiling | pprof/py-spy/async-profiler | `go tool pprof` |
| 日志分析 | kubectl logs + grep | `logs --since=1h | grep ERROR` |
| 集群审计 | kube-audit / Falco | 安全事件回溯 |
| 存储诊断 | fio / iostat | `fio --name=test --rw=randread` |
| 证书检查 | openssl | `openssl s_client -connect host:443` |

## 防复发机制

| 机制 | 实现 |
|------|------|
| 告警覆盖 | 每个故障模式有对应告警 |
| Runbook | 每类故障有标准处理流程 |
| 混沌工程 | 定期注入故障验证韧性 |
| 变更管理 | 金丝雀发布 + 自动回滚 |
| 事后复盘 | 48h 内完成 blameless postmortem |
| 知识沉淀 | 故障案例入库（本知识库） |

## Related

- [[19-故障诊断/index.md|故障诊断]]
- [[19-故障诊断/04-高级排障/index.md|高级排障]]
- [[12-可靠性/05-事后复盘/index.md|事后复盘]]
- [[09-可观测性/05-告警/index.md|告警管理]]
