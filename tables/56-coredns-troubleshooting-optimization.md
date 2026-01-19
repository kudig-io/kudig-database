# 56 - CoreDNS 故障排查与性能优化 (Troubleshooting & Optimization)

> **适用版本**: CoreDNS 1.8.0+ / Kubernetes v1.25-v1.32 | **最后更新**: 2026-01

---

## 一、故障排查流程 (Troubleshooting Workflow)

### 1.1 排查流程图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      CoreDNS 故障排查流程                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌───────────────┐                                                         │
│   │  DNS解析失败   │                                                         │
│   └───────┬───────┘                                                         │
│           │                                                                 │
│           ▼                                                                 │
│   ┌───────────────────────────────────────────────────────────────────┐    │
│   │ Step 1: 确认CoreDNS Pod状态                                        │    │
│   │ kubectl get pods -n kube-system -l k8s-app=kube-dns               │    │
│   └───────────────────────────────────────────────────────────────────┘    │
│           │                                                                 │
│           ├─── Pod不存在/CrashLoopBackOff ──▶ 检查Deployment/日志          │
│           │                                                                 │
│           ▼                                                                 │
│   ┌───────────────────────────────────────────────────────────────────┐    │
│   │ Step 2: 检查kube-dns Service                                       │    │
│   │ kubectl get svc kube-dns -n kube-system                           │    │
│   └───────────────────────────────────────────────────────────────────┘    │
│           │                                                                 │
│           ├─── Service不存在/Endpoints为空 ──▶ 重建Service                  │
│           │                                                                 │
│           ▼                                                                 │
│   ┌───────────────────────────────────────────────────────────────────┐    │
│   │ Step 3: 从Pod内测试DNS解析                                         │    │
│   │ kubectl exec -it <pod> -- nslookup kubernetes.default             │    │
│   └───────────────────────────────────────────────────────────────────┘    │
│           │                                                                 │
│           ├─── 超时 ──▶ 检查网络策略/CNI                                    │
│           ├─── NXDOMAIN ──▶ 检查Corefile配置                               │
│           ├─── SERVFAIL ──▶ 检查上游DNS/Corefile语法                       │
│           │                                                                 │
│           ▼                                                                 │
│   ┌───────────────────────────────────────────────────────────────────┐    │
│   │ Step 4: 直接查询CoreDNS Pod                                        │    │
│   │ kubectl exec -n kube-system coredns-xxx -- dig @127.0.0.1 ...     │    │
│   └───────────────────────────────────────────────────────────────────┘    │
│           │                                                                 │
│           ├─── 成功 ──▶ 问题在网络层(Service/kube-proxy)                    │
│           ├─── 失败 ──▶ CoreDNS配置或上游问题                               │
│           │                                                                 │
│           ▼                                                                 │
│   ┌───────────────────────────────────────────────────────────────────┐    │
│   │ Step 5: 检查CoreDNS日志                                            │    │
│   │ kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100        │    │
│   └───────────────────────────────────────────────────────────────────┘    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 快速诊断命令集

```bash
# === CoreDNS Pod状态 ===
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

# === Service和Endpoints ===
kubectl get svc,ep kube-dns -n kube-system

# === CoreDNS日志 (最近100行) ===
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100

# === CoreDNS日志 (实时跟踪) ===
kubectl logs -n kube-system -l k8s-app=kube-dns -f

# === ConfigMap配置 ===
kubectl get configmap coredns -n kube-system -o yaml

# === 从测试Pod解析 ===
kubectl run dnstest --rm -it --image=busybox:1.36 --restart=Never -- nslookup kubernetes.default

# === 详细DNS测试 ===
kubectl run dnstest --rm -it --image=nicolaka/netshoot --restart=Never -- \
  dig @10.96.0.10 kubernetes.default.svc.cluster.local +short

# === 检查resolv.conf ===
kubectl exec -it <pod-name> -- cat /etc/resolv.conf

# === CoreDNS指标 ===
kubectl exec -n kube-system deploy/coredns -- wget -qO- http://localhost:9153/metrics | head -50
```

---

## 二、常见故障场景 (Common Issues)

### 2.1 DNS解析超时

**症状**:
```
;; connection timed out; no servers could be reached
```

**排查步骤**:

| 步骤 | 检查项 | 命令 | 可能原因 |
|:---:|:---|:---|:---|
| 1 | CoreDNS Pod运行状态 | `kubectl get pods -n kube-system -l k8s-app=kube-dns` | Pod未运行 |
| 2 | kube-dns Service | `kubectl get svc,ep kube-dns -n kube-system` | Service无Endpoints |
| 3 | 网络连通性 | `kubectl exec <pod> -- nc -zv 10.96.0.10 53` | 网络策略阻断 |
| 4 | kube-proxy规则 | `iptables -t nat -L KUBE-SERVICES \| grep dns` | iptables规则缺失 |
| 5 | CNI状态 | `kubectl get pods -n kube-system -l k8s-app=<cni>` | CNI故障 |

**解决方案**:
```bash
# 重启CoreDNS
kubectl rollout restart deployment/coredns -n kube-system

# 重建kube-dns Service (如果丢失)
kubectl delete svc kube-dns -n kube-system
kubectl expose deployment coredns -n kube-system --name=kube-dns --port=53 --target-port=53 --protocol=UDP

# 检查NetworkPolicy
kubectl get networkpolicy -A | grep -E "kube-system|default"
```

### 2.2 NXDOMAIN错误

**症状**:
```
** server can't find nginx.default.svc.cluster.local: NXDOMAIN
```

**排查矩阵**:

| 查询类型 | 预期结果 | NXDOMAIN原因 |
|:---|:---|:---|
| `kubernetes.default.svc.cluster.local` | 成功 | Corefile kubernetes插件配置错误 |
| `nginx.default.svc.cluster.local` | 成功 | Service不存在或命名空间错误 |
| `www.google.com` | 成功 | forward插件配置错误 |
| `nginx` (短名称) | 成功 | resolv.conf search域错误 |

**排查命令**:
```bash
# 检查Service是否存在
kubectl get svc nginx -n default

# 检查kubernetes插件区域配置
kubectl get configmap coredns -n kube-system -o jsonpath='{.data.Corefile}' | grep kubernetes

# 检查Pod的resolv.conf
kubectl exec <pod> -- cat /etc/resolv.conf
```

### 2.3 SERVFAIL错误

**症状**:
```
** server can't find example.com: SERVFAIL
```

**常见原因**:

| 原因 | 检查方法 | 解决方案 |
|:---|:---|:---|
| Corefile语法错误 | 查看CoreDNS启动日志 | 修复配置语法 |
| 上游DNS不可达 | `dig @8.8.8.8 example.com` | 更换上游DNS |
| 转发循环 | CoreDNS日志出现loop检测 | 检查forward配置 |
| kubernetes插件错误 | 日志中kubernetes相关错误 | 检查RBAC权限 |

**诊断Corefile语法**:
```bash
# 验证Corefile语法
kubectl exec -n kube-system deploy/coredns -- coredns -conf /etc/coredns/Corefile -validate

# 查看启动错误
kubectl logs -n kube-system deploy/coredns | grep -i error
```

### 2.4 DNS解析慢

**症状**: DNS查询延迟高(>100ms)

**排查清单**:

| 检查项 | 正常值 | 检查命令 |
|:---|:---|:---|
| CoreDNS延迟 | <10ms | Prometheus: `coredns_dns_request_duration_seconds` |
| 缓存命中率 | >60% | Prometheus: `coredns_cache_hits_total / (hits+misses)` |
| 上游延迟 | <50ms | `dig @8.8.8.8 example.com +stats` |
| ndots配置 | 合理 | 外部域名查询次数过多 |

**优化方案**:
```yaml
# 1. 增加缓存大小和TTL
cache {
    success 50000 3600 60
    denial 5000 600
    prefetch 10 1h 10%
}

# 2. 对于外部域名,使用FQDN
# 错误: www.google.com (会触发多次search域查询)
# 正确: www.google.com. (直接查询)

# 3. 减少ndots值 (需要kubelet配置)
# /var/lib/kubelet/config.yaml
clusterDNS:
  - 10.96.0.10
clusterDomain: cluster.local
resolvConf: ""
# 添加Pod DNS Config
```

### 2.5 CoreDNS CrashLoopBackOff

**排查步骤**:
```bash
# 1. 查看Pod状态
kubectl describe pod -n kube-system -l k8s-app=kube-dns

# 2. 查看日志
kubectl logs -n kube-system -l k8s-app=kube-dns --previous

# 3. 常见原因
# - Corefile语法错误
# - ConfigMap未挂载
# - 端口冲突 (53端口被占用)
# - 内存不足 (OOM)
```

**常见错误及解决**:

| 错误日志 | 原因 | 解决方案 |
|:---|:---|:---|
| `plugin/loop: Loop detected` | DNS转发循环 | 检查forward配置,确保不转发到自己 |
| `Failed to list *v1.Service` | RBAC权限不足 | 检查ClusterRole/ClusterRoleBinding |
| `listen tcp :53: bind: address already in use` | 端口冲突 | 检查hostNetwork配置或其他DNS进程 |
| `OOMKilled` | 内存不足 | 增加memory limits |

---

## 三、DNS调试工具 (Debug Tools)

### 3.1 dig 命令详解

```bash
# 基本查询
dig @10.96.0.10 nginx.default.svc.cluster.local

# 指定查询类型
dig @10.96.0.10 nginx.default.svc.cluster.local A
dig @10.96.0.10 nginx.default.svc.cluster.local AAAA
dig @10.96.0.10 _http._tcp.nginx.default.svc.cluster.local SRV

# 简短输出
dig @10.96.0.10 nginx.default.svc.cluster.local +short

# 详细输出 (含统计)
dig @10.96.0.10 nginx.default.svc.cluster.local +stats

# 追踪解析路径
dig @10.96.0.10 nginx.default.svc.cluster.local +trace

# TCP查询 (绕过UDP问题)
dig @10.96.0.10 nginx.default.svc.cluster.local +tcp

# 反向解析
dig @10.96.0.10 -x 10.96.0.1
```

### 3.2 nslookup 命令

```bash
# 基本查询
nslookup nginx.default.svc.cluster.local 10.96.0.10

# 指定类型
nslookup -type=SRV _http._tcp.nginx.default.svc.cluster.local 10.96.0.10

# 调试模式
nslookup -debug nginx.default.svc.cluster.local 10.96.0.10
```

### 3.3 dnsperf 性能测试

```bash
# 安装
apt-get install dnsperf

# 创建查询文件
cat > queries.txt << EOF
nginx.default.svc.cluster.local A
kubernetes.default.svc.cluster.local A
www.google.com A
EOF

# 运行测试
dnsperf -s 10.96.0.10 -d queries.txt -l 30 -c 10 -Q 1000

# 参数说明:
# -s: DNS服务器
# -d: 查询文件
# -l: 测试时长(秒)
# -c: 并发客户端数
# -Q: 最大QPS
```

### 3.4 测试Pod模板

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: dns-debug
  namespace: default
spec:
  containers:
  - name: debug
    image: nicolaka/netshoot
    command: ["sleep", "infinity"]
    securityContext:
      capabilities:
        add: ["NET_ADMIN", "NET_RAW"]
  dnsPolicy: ClusterFirst
  restartPolicy: Never
---
# 使用方法:
# kubectl exec -it dns-debug -- bash
# dig @10.96.0.10 kubernetes.default.svc.cluster.local
# nslookup nginx.default 10.96.0.10
# tcpdump -i any port 53 -nn
```

---

## 四、性能优化 (Performance Optimization)

### 4.1 缓存优化

**默认 vs 优化配置对比**:

| 配置项 | 默认值 | 小型集群 | 大型集群 |
|:---|:---|:---|:---|
| cache TTL | 30s | 60s | 300s |
| success capacity | 9984 | 10000 | 50000 |
| denial capacity | 9984 | 1000 | 5000 |
| prefetch | 禁用 | `10 1h 10%` | `10 1h 20%` |
| serve_stale | 禁用 | 禁用 | `1h` |

**优化配置示例**:
```
cache {
    success 50000 300 60     # 5万条,最大5分钟,最小1分钟
    denial 5000 60 30        # 5千条否定缓存
    prefetch 10 1h 20%       # 剩余20%TTL时预取
    serve_stale 1h           # 上游故障时服务1小时过期缓存
}
```

### 4.2 forward优化

```
forward . 8.8.8.8 8.8.4.4 1.1.1.1 {
    max_concurrent 2000      # 增加并发数(大型集群)
    policy round_robin       # 负载均衡策略
    health_check 5s          # 健康检查间隔
    max_fails 3              # 失败阈值
    expire 10s               # 连接过期时间
    prefer_udp               # 优先UDP(更快)
}
```

### 4.3 kubernetes插件优化

```
kubernetes cluster.local in-addr.arpa ip6.arpa {
    pods insecure
    fallthrough in-addr.arpa ip6.arpa
    ttl 60                   # 增加TTL(减少API Server压力)
    noendpoints              # 如果不需要endpoint解析
}
```

### 4.4 资源配置优化

| 集群规模 | CPU Request | CPU Limit | Memory Request | Memory Limit |
|:---|:---|:---|:---|:---|
| <50节点 | 100m | 200m | 70Mi | 170Mi |
| 50-200节点 | 200m | 500m | 128Mi | 256Mi |
| 200-500节点 | 500m | 1000m | 256Mi | 512Mi |
| >500节点 | 1000m | 2000m | 512Mi | 1Gi |

**资源配置示例**:
```yaml
resources:
  requests:
    cpu: 200m
    memory: 128Mi
  limits:
    cpu: 1000m
    memory: 512Mi
```

### 4.5 副本数与HPA

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: coredns
  namespace: kube-system
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: coredns
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 60    # CPU超过60%扩容
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 70    # 内存超过70%扩容
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300    # 缩容稳定期5分钟
      policies:
      - type: Pods
        value: 1
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 0      # 立即扩容
      policies:
      - type: Pods
        value: 2
        periodSeconds: 60
```

### 4.6 ndots优化

**问题**: 默认ndots=5导致外部域名查询次数过多

**场景分析**:
```
查询: www.google.com (2个点 < 5)
实际查询序列:
1. www.google.com.default.svc.cluster.local  → NXDOMAIN
2. www.google.com.svc.cluster.local          → NXDOMAIN
3. www.google.com.cluster.local              → NXDOMAIN
4. www.google.com                            → 成功
总计: 4次查询 (3次浪费)
```

**优化方案**:

| 方案 | 配置 | 优点 | 缺点 |
|:---|:---|:---|:---|
| **使用FQDN** | `www.google.com.` | 无需改配置 | 需修改应用 |
| **减少ndots** | `ndots: 2` | 减少查询 | 可能影响K8s服务发现 |
| **Pod DNS Config** | 见下方 | 精确控制 | 需要配置每个Pod |
| **autopath插件** | 见下方 | 自动优化 | 增加CoreDNS复杂度 |

**Pod级别DNS配置**:
```yaml
apiVersion: v1
kind: Pod
metadata:
  name: myapp
spec:
  containers:
  - name: app
    image: myapp:latest
  dnsPolicy: ClusterFirst
  dnsConfig:
    options:
    - name: ndots
      value: "2"           # 减少ndots
    - name: timeout
      value: "2"           # 减少超时
    - name: attempts
      value: "2"           # 减少重试
```

**autopath插件配置**:
```
.:53 {
    errors
    health
    
    autopath @kubernetes    # 自动优化DNS查询路径
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
        pods insecure
        fallthrough in-addr.arpa ip6.arpa
    }
    
    forward . /etc/resolv.conf
    cache 30
    loop
    reload
    loadbalance
}
```

---

## 五、监控告警配置 (Monitoring & Alerting)

### 5.1 关键指标

| 指标 | 正常范围 | 告警阈值 | 说明 |
|:---|:---|:---|:---|
| QPS | 视集群规模 | 突增3倍 | DNS查询速率 |
| 延迟P99 | <50ms | >100ms | 请求处理延迟 |
| 错误率 | <0.1% | >1% | SERVFAIL/REFUSED比例 |
| 缓存命中率 | >60% | <50% | 缓存效率 |
| 上游延迟 | <100ms | >500ms | forward延迟 |
| 内存使用 | <80% limits | >90% limits | 内存压力 |

### 5.2 Prometheus告警规则

```yaml
groups:
- name: coredns.rules
  rules:
  # CoreDNS不可用
  - alert: CoreDNSDown
    expr: absent(up{job="coredns"} == 1)
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "CoreDNS is down"
      description: "CoreDNS has been down for more than 5 minutes"

  # 高错误率
  - alert: CoreDNSHighErrorRate
    expr: |
      (
        sum(rate(coredns_dns_responses_total{rcode=~"SERVFAIL|REFUSED"}[5m]))
        /
        sum(rate(coredns_dns_responses_total[5m]))
      ) > 0.01
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS error rate > 1%"
      
  # 高延迟
  - alert: CoreDNSHighLatency
    expr: |
      histogram_quantile(0.99, 
        sum(rate(coredns_dns_request_duration_seconds_bucket[5m])) by (le)
      ) > 0.1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS P99 latency > 100ms"
      
  # 低缓存命中率
  - alert: CoreDNSLowCacheHitRate
    expr: |
      (
        sum(rate(coredns_cache_hits_total[10m]))
        /
        (sum(rate(coredns_cache_hits_total[10m])) + sum(rate(coredns_cache_misses_total[10m])))
      ) < 0.5
    for: 15m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS cache hit rate < 50%"
      
  # 上游DNS延迟高
  - alert: CoreDNSForwardLatencyHigh
    expr: |
      histogram_quantile(0.99, 
        sum(rate(coredns_forward_request_duration_seconds_bucket[5m])) by (le)
      ) > 0.5
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS forward P99 latency > 500ms"
      
  # 内存使用过高
  - alert: CoreDNSMemoryHigh
    expr: |
      (
        container_memory_working_set_bytes{container="coredns"}
        /
        container_spec_memory_limit_bytes{container="coredns"}
      ) > 0.9
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS memory usage > 90%"
      
  # Pod数量低于预期
  - alert: CoreDNSPodsLow
    expr: |
      count(up{job="coredns"} == 1) < 2
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS running pods < 2"
```

### 5.3 日志分析

**启用详细日志**:
```
.:53 {
    log . "{remote}:{port} - {type} {class} {name} {rcode} {rsize} {duration}"
    errors
    # ... 其他配置
}
```

**日志关键字分析**:

| 关键字 | 含义 | 处理方式 |
|:---|:---|:---|
| `SERVFAIL` | 上游错误或配置错误 | 检查forward配置和上游DNS |
| `NXDOMAIN` | 域名不存在 | 检查服务/配置 |
| `REFUSED` | 拒绝查询 | 检查ACL配置 |
| `loop detected` | 转发循环 | 检查forward目标 |
| `i/o timeout` | 上游超时 | 检查网络/上游DNS |
| `plugin/kubernetes` | K8s插件错误 | 检查RBAC/API Server |

---

## 六、最佳实践清单 (Best Practices Checklist)

### 6.1 部署最佳实践

| 类别 | 检查项 | 推荐配置 |
|:---|:---|:---|
| **高可用** | 副本数 | ≥2 (生产环境≥3) |
| **高可用** | PodDisruptionBudget | minAvailable: 1 |
| **高可用** | 反亲和 | 分散到不同节点 |
| **资源** | CPU Request | 100m-500m |
| **资源** | Memory Request | 70Mi-256Mi |
| **资源** | Memory Limit | Request的2倍 |
| **监控** | prometheus插件 | 必须启用 |
| **监控** | 告警规则 | 覆盖关键指标 |
| **健康检查** | livenessProbe | /health:8080 |
| **健康检查** | readinessProbe | /ready:8181 |

### 6.2 配置最佳实践

| 类别 | 检查项 | 推荐配置 |
|:---|:---|:---|
| **缓存** | cache插件 | 必须启用 |
| **缓存** | success capacity | 10000-50000 |
| **缓存** | TTL | 30-300秒 |
| **转发** | max_concurrent | 1000-2000 |
| **转发** | health_check | 5-10秒 |
| **转发** | 多上游 | 至少2个 |
| **安全** | loop插件 | 必须启用 |
| **安全** | reload插件 | 推荐启用 |
| **kubernetes** | fallthrough | 反向解析区域 |
| **kubernetes** | ttl | 30-60秒 |

### 6.3 运维最佳实践

| 操作 | 最佳实践 |
|:---|:---|
| **配置变更** | 先验证语法,再应用,观察日志 |
| **升级** | 滚动更新,先测试环境 |
| **扩容** | 基于监控指标,使用HPA |
| **备份** | 定期备份ConfigMap |
| **文档** | 记录所有定制配置 |

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
