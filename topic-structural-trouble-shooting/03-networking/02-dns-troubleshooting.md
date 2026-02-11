# CoreDNS/DNS 故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32, CoreDNS v1.9+ | **最后更新**: 2026-01 | **难度**: 中级-高级

---

## 目录

1. [问题现象与影响分析](#1-问题现象与影响分析)
2. [排查方法与步骤](#2-排查方法与步骤)
3. [解决方案与风险控制](#3-解决方案与风险控制)

---

## 0. 10 分钟快速诊断

1. **CoreDNS 存活与 Endpoints**：`kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide`，`kubectl get endpoints -n kube-system kube-dns`。
2. **Pod 内快速验证**：`kubectl run dnsutils --rm -it --image=registry.k8s.io/e2e-test-images/jessie-dnsutils:1.3 -- sh`，执行 `dig kubernetes.default` 与 `dig @<cluster-dns-ip> kubernetes.default.svc.cluster.local`。
3. **resolv.conf 校验**：`kubectl exec <pod> -- cat /etc/resolv.conf`，检查 `nameserver` 是否为集群 DNS，`search` 与 `ndots` 是否合理。
4. **Corefile 检查**：`kubectl get cm -n kube-system coredns -o yaml`，确认 `forward` 上游、`cache`、`loop` 插件配置合理。
5. **上游 DNS 健康**：`kubectl logs -n kube-system -l k8s-app=kube-dns | grep -E "SERVFAIL|timeout|forward" | tail`，排除上游抖动。
6. **网络路径**：`kubectl get svc -n kube-system kube-dns -o wide` 与 `kube-proxy` 规则，确认 Service/Endpoints 正确映射。
7. **快速缓解**：
   - CoreDNS 资源不足：扩副本或提高 CPU/内存 request。
   - 上游慢：启用 `cache` 并调整 `max_concurrent`；必要时引入 NodeLocal DNSCache。
8. **证据留存**：保存 DNS 测试结果、CoreDNS 日志、Corefile 配置与资源使用快照。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 CoreDNS 服务不可用

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| CoreDNS Pod 异常 | `CrashLoopBackOff` | kubectl | `kubectl get pods -n kube-system` |
| DNS Service 无 Endpoints | Endpoints 为空 | kubectl | `kubectl get endpoints -n kube-system kube-dns` |
| CoreDNS 配置错误 | `plugin/errors: 2 errors` | CoreDNS 日志 | CoreDNS Pod 日志 |
| 资源不足 | `OOMKilled` | kubectl | `kubectl describe pod` |

#### 1.1.2 DNS 解析失败

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 集群内域名解析失败 | `NXDOMAIN` | nslookup/dig | `nslookup <service>.<ns>.svc.cluster.local` |
| 外部域名解析失败 | `SERVFAIL` | nslookup/dig | `nslookup google.com` |
| 解析超时 | `connection timed out` | nslookup/dig | DNS 查询结果 |
| 间歇性解析失败 | 偶发失败 | 应用日志 | 应用日志 |

#### 1.1.3 DNS 性能问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 解析延迟高 | 响应时间长 | 应用监控 | 应用监控 |
| DNS 请求超时 | `i/o timeout` | 应用日志 | 应用日志 |
| CoreDNS 负载高 | CPU/内存高 | 监控 | `kubectl top pods` |
| 上游 DNS 慢 | `plugin/forward: unhealthy` | CoreDNS 日志 | CoreDNS 日志 |

### 1.2 报错查看方式汇总

```bash
# 查看 CoreDNS Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=200

# 查看 CoreDNS 配置
kubectl get configmap -n kube-system coredns -o yaml

# 查看 DNS Service
kubectl get svc -n kube-system kube-dns
kubectl get endpoints -n kube-system kube-dns

# 在 Pod 内测试 DNS
kubectl run test-dns --rm -it --image=busybox:1.28 -- sh
# 在 Pod 内执行
nslookup kubernetes.default
nslookup google.com

# 使用 dnsutils 镜像进行详细测试
kubectl run dnsutils --rm -it --image=registry.k8s.io/e2e-test-images/jessie-dnsutils:1.3 -- sh
# 在 Pod 内执行
dig kubernetes.default.svc.cluster.local
dig @10.96.0.10 kubernetes.default.svc.cluster.local

# 查看 Pod 的 DNS 配置
kubectl exec <pod-name> -- cat /etc/resolv.conf
```

### 1.3 影响面分析

#### 1.3.1 直接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **Service 发现** | 完全失效 | 通过 DNS 名称的服务发现不可用 |
| **外部域名解析** | 失效 | Pod 无法解析外部域名 |
| **Ingress** | 部分影响 | 基于名称的路由可能受影响 |
| **证书验证** | 可能失败 | HTTPS 证书验证需要 DNS |

#### 1.3.2 间接影响

| 影响范围 | 影响程度 | 影响描述 |
|----------|----------|----------|
| **服务间调用** | 高 | 使用 Service 名称的调用失败 |
| **数据库连接** | 可能失败 | 外部数据库连接可能失败 |
| **外部 API** | 失败 | 调用外部 API 失败 |
| **镜像拉取** | 可能失败 | 使用域名的镜像仓库不可用 |

---

## 2. 排查方法与步骤

### 2.1 排查原理

CoreDNS 是 Kubernetes 集群的 DNS 服务，负责服务发现和外部域名解析。排查需要从以下层面：

1. **服务层面**：CoreDNS Pod 是否正常运行
2. **配置层面**：CoreDNS 配置是否正确
3. **网络层面**：Pod 到 CoreDNS 的网络是否通畅
4. **上游层面**：上游 DNS 是否正常
5. **客户端层面**：Pod 的 DNS 配置是否正确

#### 2.1.1 CoreDNS 架构深度剖析

**核心插件链机制**

CoreDNS 采用插件化架构，每个 DNS 请求按照 Corefile 中定义的插件顺序依次处理：

```
┌─────────────┐
│ DNS 请求    │
└──────┬──────┘
       │
       v
┌─────────────┐
│  errors     │ ─── 错误日志记录
└──────┬──────┘
       │
       v
┌─────────────┐
│  cache      │ ─── 缓存查询（命中直接返回）
└──────┬──────┘
       │
       v
┌─────────────┐
│  kubernetes │ ─── 集群内域名解析（*.svc.cluster.local）
└──────┬──────┘
       │
       v
┌─────────────┐
│  forward    │ ─── 上游 DNS 转发（外部域名）
└──────┬──────┘
       │
       v
┌─────────────┐
│ DNS 响应    │
└─────────────┘
```

**关键插件功能详解**

| 插件名称 | 功能 | 关键参数 | 故障影响 |
|---------|------|---------|---------|
| **errors** | 记录错误到日志 | - | 禁用导致错误排查困难 |
| **health** | 健康检查端点 | `lameduck 5s` | 影响滚动更新平滑性 |
| **ready** | 就绪检查端点 `/ready` | - | 影响 Pod 就绪判断 |
| **kubernetes** | K8s 服务发现 | `pods insecure`<br>`fallthrough`<br>`ttl 30` | 集群内域名解析失败 |
| **prometheus** | 暴露指标 | `:9153` | 无监控数据 |
| **forward** | 上游 DNS 转发 | `max_concurrent 1000`<br>`policy sequential/random` | 外部域名解析失败 |
| **cache** | DNS 缓存 | `success 9984 30`<br>`denial 9984 5` | 无缓存导致性能差 |
| **loop** | 检测转发环路 | - | 环路导致无限递归 |
| **reload** | 热加载配置 | - | 需重启 Pod 更新配置 |
| **loadbalance** | 负载均衡 A 记录 | - | 多 IP 返回顺序固定 |

**kubernetes 插件深度解析**

```yaml
kubernetes cluster.local in-addr.arpa ip6.arpa {
    # pods insecure: 允许 Pod A 记录查询（非严格模式）
    # pods verified: 仅返回存在的 Pod（严格模式，性能差）
    # pods disabled: 禁用 Pod 记录
    pods insecure
    
    # fallthrough: 未匹配域名继续下一插件
    fallthrough in-addr.arpa ip6.arpa
    
    # ttl 30: DNS 记录 TTL（秒）
    ttl 30
    
    # endpoint_pod_names: 使用 Pod 名作为 Endpoint 域名
    endpoint_pod_names
    
    # 上游 API Server 配置（自动发现）
    # kubeconfig /etc/coredns/kubeconfig
}
```

**域名解析优先级规则**

1. **完整 FQDN**（如 `svc.namespace.svc.cluster.local.`）
   - 直接查询，不进行搜索域扩展
   - 性能最优，推荐生产使用

2. **短域名** + `search` 域扩展
   ```
   # /etc/resolv.conf
   nameserver 10.96.0.10
   search default.svc.cluster.local svc.cluster.local cluster.local
   options ndots:5
   ```
   
   查询 `mysql` 时的搜索顺序（`ndots:5`）：
   ```
   mysql.default.svc.cluster.local     (5 个点 >= ndots，直接查询)
   mysql.svc.cluster.local             (4 个点 < ndots，继续搜索)
   mysql.cluster.local
   mysql                               (最终查询原始名称)
   ```

3. **外部域名处理**
   - `google.com` 有 1 个点 < ndots(5)，先查询 `google.com.default.svc.cluster.local` 等（浪费 4 次查询）
   - **优化方案**：使用 `google.com.`（末尾加点）跳过搜索域

**forward 插件高级配置**

```
forward . 8.8.8.8 8.8.4.4 {
    # max_concurrent 1000: 最大并发上游查询数
    max_concurrent 1000
    
    # policy sequential: 顺序尝试上游（第一个失败再试第二个）
    # policy random: 随机选择上游（负载均衡）
    policy sequential
    
    # force_tcp: 强制使用 TCP（UDP 有问题时）
    # force_tcp
    
    # prefer_udp: 优先 UDP，大响应时自动切换 TCP
    prefer_udp
    
    # expire 10s: 上游健康检查失败后标记为不健康的时间
    expire 10s
    
    # health_check 5s: 健康检查间隔
    health_check 5s
}
```

**cache 插件缓存策略**

```
cache 30 {
    # success 9984 30: 成功响应缓存 30 秒，容量 9984 条
    success 9984 30
    
    # denial 9984 5: NXDOMAIN 等否定响应缓存 5 秒
    denial 9984 5
    
    # prefetch 10 60s: 预取机制（剩余 TTL < 10 秒时，后台刷新缓存）
    prefetch 10 60s
    
    # serve_stale: 上游失败时返回过期缓存
    serve_stale
}
```

**NodeLocal DNSCache 架构**

```
┌──────────────────────────────────────────────┐
│                    Node                       │
│  ┌──────────┐          ┌──────────────────┐ │
│  │   Pod    │          │  NodeLocal DNS   │ │
│  │          │          │  (169.254.20.10) │ │
│  │resolv.   │ ───────> │                  │ │
│  │conf:     │          │  [本地缓存]      │ │
│  │169.254.  │          │       │          │ │
│  │20.10     │          │       v          │ │
│  └──────────┘          │  [ClusterDNS]    │ │
│                        │  (10.96.0.10)    │ │
│                        └──────────────────┘ │
└──────────────────────────────────────────────┘
```

优势：
- **零跳转**：Pod 直连本地 DNS（无 iptables/IPVS 开销）
- **强缓存**：节点级缓存，减少 CoreDNS 负载
- **HA 提升**：CoreDNS 故障时本地缓存仍可服务

配置示例：
```yaml
# 每节点 DaemonSet
apiVersion: v1
kind: ConfigMap
metadata:
  name: node-local-dns
  namespace: kube-system
data:
  Corefile: |
    cluster.local:53 {
        errors
        cache {
            success 9984 30
            denial 9984 5
        }
        reload
        loop
        bind 169.254.20.10
        forward . 10.96.0.10  # 转发到 CoreDNS
        prometheus :9253
    }
```

#### 2.1.2 DNS 性能瓶颈分析

**常见性能问题根因**

| 问题现象 | 根本原因 | 关键指标 | 排查方向 |
|---------|---------|---------|---------|
| P99 延迟 >100ms | ndots 过高导致多次查询 | `coredns_dns_request_duration_seconds` | 检查 ndots 配置 |
| 外部域名慢 | 上游 DNS 延迟高 | `coredns_forward_request_duration_seconds` | 更换上游或启用缓存 |
| 缓存命中率低 | TTL 过短或缓存容量不足 | `coredns_cache_hits_total` / `coredns_cache_misses_total` | 调整缓存参数 |
| CPU 使用率高 | 并发查询过多 | `coredns_dns_requests_total` | 扩展副本或启用 NodeLocal DNS |
| SERVFAIL 错误 | 上游 DNS 故障或环路 | `coredns_dns_responses_total{rcode="SERVFAIL"}` | 检查 forward 与 loop 插件 |

**ndots 对性能的影响**

测试场景：查询 `api.example.com`（假设不存在于集群内）

| ndots | 查询次数 | 查询列表 | 延迟估算 |
|-------|---------|---------|---------|
| 5（默认）| 4 次 | `api.example.com.default.svc...`<br>`api.example.com.svc...`<br>`api.example.com.cluster.local`<br>`api.example.com` | ~100ms |
| 2 | 2 次 | `api.example.com.default.svc...`<br>`api.example.com` | ~50ms |
| 1 | 1 次 | `api.example.com` | ~25ms |

**优化建议**：
- 集群内服务间调用：使用完整 FQDN `service.namespace.svc.cluster.local`
- 外部 API 调用：末尾加点 `api.example.com.` 或设置 `ndots:1`

**CoreDNS 水平扩展策略**

```bash
# 根据集群规模设置副本数
# 小集群（<50 节点）: 2 副本
# 中型集群（50-200 节点）: 3-5 副本
# 大型集群（>200 节点）: 按 100 节点 : 1 副本

# 设置反亲和性避免单点故障
kubectl patch deployment -n kube-system coredns -p '{
  "spec": {
    "replicas": 5,
    "template": {
      "spec": {
        "affinity": {
          "podAntiAffinity": {
            "requiredDuringSchedulingIgnoredDuringExecution": [{
              "labelSelector": {
                "matchLabels": {"k8s-app": "kube-dns"}
              },
              "topologyKey": "kubernetes.io/hostname"
            }]
          }
        }
      }
    }
  }
}'

# 资源配置参考
resources:
  requests:
    cpu: 200m      # 按 QPS 调整（1000 QPS ≈ 100m CPU）
    memory: 128Mi  # 缓存占用主要内存
  limits:
    cpu: 2000m     # 留足突发容量
    memory: 512Mi
```

#### 2.1.3 生产环境最佳实践

**企业级 Corefile 配置模板**

```
.:53 {
    errors
    health {
       lameduck 5s  # 优雅关闭窗口
    }
    ready
    
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
       ttl 30
    }
    
    prometheus :9153
    
    # 主配置：外部域名解析
    forward . 10.0.0.53 10.0.0.54 {  # 企业内网 DNS
       max_concurrent 1000
       policy sequential
       health_check 5s
    }
    
    # 高级配置：缓存策略
    cache 60 {
       success 9984 60   # 成功响应缓存 1 分钟
       denial 9984 10    # 否定响应缓存 10 秒
       prefetch 10 60s   # 预取机制
       serve_stale       # 上游失败时返回过期缓存
    }
    
    loop           # 环路检测
    reload         # 热加载
    loadbalance    # 多 IP 负载均衡
}

# 专用域转发（可选）
example.com:53 {
    errors
    cache 30
    forward . 192.168.1.10 192.168.1.11  # 特定域名专用 DNS
}
```

**监控告警规则**

```yaml
# Prometheus 告警规则
groups:
- name: coredns
  interval: 30s
  rules:
  # DNS 请求延迟过高
  - alert: CoreDNS_HighLatency
    expr: histogram_quantile(0.99, rate(coredns_dns_request_duration_seconds_bucket[5m])) > 0.1
    for: 5m
    annotations:
      summary: "CoreDNS P99 延迟 >100ms（{{ $value }}s）"
      
  # 上游健康检查失败
  - alert: CoreDNS_ForwardHealthCheckFailed
    expr: rate(coredns_forward_healthcheck_failures_total[5m]) > 0
    annotations:
      summary: "上游 DNS {{ $labels.to }} 健康检查失败"
      
  # SERVFAIL 错误率过高
  - alert: CoreDNS_HighErrorRate
    expr: rate(coredns_dns_responses_total{rcode="SERVFAIL"}[5m]) / rate(coredns_dns_responses_total[5m]) > 0.01
    for: 5m
    annotations:
      summary: "SERVFAIL 错误率 >1%（{{ $value | humanizePercentage }}）"
      
  # 缓存命中率过低
  - alert: CoreDNS_LowCacheHitRate
    expr: rate(coredns_cache_hits_total[5m]) / (rate(coredns_cache_hits_total[5m]) + rate(coredns_cache_misses_total[5m])) < 0.5
    for: 10m
    annotations:
      summary: "缓存命中率 <50%（{{ $value | humanizePercentage }}）"
      
  # CoreDNS Pod 不足
  - alert: CoreDNS_InsufficientReplicas
    expr: kube_deployment_status_replicas_available{deployment="coredns"} < 2
    for: 5m
    annotations:
      summary: "CoreDNS 可用副本 <2（当前 {{ $value }}）"
```

### 2.2 排查步骤和具体命令

#### 2.2.1 第一步：检查 CoreDNS 状态

```bash
# 查看 CoreDNS Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

# 查看 Pod 详情
kubectl describe pod -n kube-system -l k8s-app=kube-dns

# 查看 CoreDNS Deployment
kubectl get deployment -n kube-system coredns

# 查看 DNS Service
kubectl get svc -n kube-system kube-dns -o yaml

# 验证 Endpoints 存在
kubectl get endpoints -n kube-system kube-dns
```

#### 2.2.2 第二步：检查 CoreDNS 配置

```bash
# 查看 CoreDNS ConfigMap
kubectl get configmap -n kube-system coredns -o yaml

# 检查 Corefile 语法
# CoreDNS 配置示例
# .:53 {
#     errors
#     health {
#        lameduck 5s
#     }
#     ready
#     kubernetes cluster.local in-addr.arpa ip6.arpa {
#        pods insecure
#        fallthrough in-addr.arpa ip6.arpa
#        ttl 30
#     }
#     prometheus :9153
#     forward . /etc/resolv.conf {
#        max_concurrent 1000
#     }
#     cache 30
#     loop
#     reload
#     loadbalance
# }

# 查看 CoreDNS 日志检查配置错误
kubectl logs -n kube-system -l k8s-app=kube-dns | grep -i "error"
```

#### 2.2.3 第三步：测试 DNS 解析

```bash
# 创建测试 Pod
kubectl run test-dns --rm -it --image=busybox:1.28 -- sh

# 在测试 Pod 内执行
# 测试集群内域名
nslookup kubernetes.default
nslookup kube-dns.kube-system.svc.cluster.local

# 测试外部域名
nslookup google.com

# 查看 DNS 配置
cat /etc/resolv.conf

# 使用 dig 进行详细测试（需要 dnsutils 镜像）
kubectl run dnsutils --rm -it --image=registry.k8s.io/e2e-test-images/jessie-dnsutils:1.3 -- sh
dig kubernetes.default.svc.cluster.local
dig +trace google.com
```

#### 2.2.4 第四步：检查网络连通性

```bash
# 在测试 Pod 内检查到 CoreDNS 的连通性
# 获取 kube-dns ClusterIP
kubectl get svc -n kube-system kube-dns -o jsonpath='{.spec.clusterIP}'

# 在 Pod 内测试
nc -zvu <kube-dns-ip> 53

# 检查 kube-proxy 规则
iptables -t nat -L -n | grep <kube-dns-ip>

# 检查 Pod 到 CoreDNS Pod 的连通性
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
# 直接 ping CoreDNS Pod IP
ping <coredns-pod-ip>
```

#### 2.2.5 第五步：检查上游 DNS

```bash
# 查看节点的 DNS 配置
cat /etc/resolv.conf

# 测试节点上的 DNS 解析
nslookup google.com
dig google.com

# 查看 CoreDNS 上游配置
kubectl get configmap -n kube-system coredns -o yaml | grep -A5 forward

# 检查 CoreDNS 日志中的上游错误
kubectl logs -n kube-system -l k8s-app=kube-dns | grep -i "forward"
```

#### 2.2.6 第六步：检查 DNS 性能

```bash
# 查看 CoreDNS 指标
kubectl port-forward -n kube-system svc/kube-dns 9153:9153 &
curl http://localhost:9153/metrics

# 关键指标
# coredns_dns_request_duration_seconds - 请求延迟
# coredns_dns_requests_total - 请求总数
# coredns_dns_responses_total - 响应总数
# coredns_forward_healthcheck_failures_total - 上游健康检查失败

# 查看 CoreDNS 资源使用
kubectl top pods -n kube-system -l k8s-app=kube-dns

# 检查是否有大量错误
kubectl logs -n kube-system -l k8s-app=kube-dns | grep -c "NXDOMAIN"
kubectl logs -n kube-system -l k8s-app=kube-dns | grep -c "SERVFAIL"
```

### 2.3 排查注意事项

| 注意项 | 说明 | 建议 |
|--------|------|------|
| **ndots 设置** | 影响解析行为 | 检查 ndots 配置 |
| **search 域** | 影响短域名解析 | 检查 search 配置 |
| **DNS 策略** | Pod 的 dnsPolicy | 检查 Pod spec |
| **缓存** | CoreDNS 缓存影响更新 | 考虑缓存时间 |

---

## 3. 解决方案与风险控制

### 3.1 CoreDNS Pod 异常

#### 3.1.1 解决步骤

```bash
# 步骤 1：检查 Pod 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl describe pod -n kube-system <coredns-pod>

# 步骤 2：查看错误日志
kubectl logs -n kube-system <coredns-pod> --previous

# 步骤 3：检查配置是否有语法错误
kubectl get configmap -n kube-system coredns -o yaml

# 步骤 4：如果是资源不足，增加资源限制
kubectl edit deployment -n kube-system coredns
# 修改 resources:
#   requests:
#     cpu: 100m
#     memory: 70Mi
#   limits:
#     cpu: 1000m
#     memory: 170Mi

# 步骤 5：如果是配置错误，修复配置
kubectl edit configmap -n kube-system coredns

# 步骤 6：重启 CoreDNS
kubectl rollout restart deployment -n kube-system coredns

# 步骤 7：验证恢复
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl run test --rm -it --image=busybox -- nslookup kubernetes
```

#### 3.1.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **高** | 重启期间 DNS 短暂不可用 | 确保多副本 |
| **中** | 配置错误导致无法启动 | 修改前备份 |

#### 3.1.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. CoreDNS 是关键服务，修改需谨慎
2. 确保至少有 2 个 CoreDNS 副本
3. 配置变更前先在测试环境验证
4. 修改后立即测试 DNS 解析
5. 考虑使用 PodDisruptionBudget
```

### 3.2 DNS 解析失败

#### 3.2.1 解决步骤

```bash
# 步骤 1：确认故障范围
# 测试集群内域名
kubectl run test --rm -it --image=busybox -- nslookup kubernetes.default

# 测试外部域名
kubectl run test --rm -it --image=busybox -- nslookup google.com

# 步骤 2：如果集群内域名失败
# 检查 kubernetes 插件配置
kubectl get configmap -n kube-system coredns -o yaml | grep -A10 "kubernetes"

# 检查 cluster.local 域是否正确
kubectl get configmap -n kube-system coredns -o yaml | grep "cluster.local"

# 步骤 3：如果外部域名失败
# 检查 forward 配置
kubectl get configmap -n kube-system coredns -o yaml | grep -A5 "forward"

# 检查上游 DNS 是否可达
kubectl exec -n kube-system <coredns-pod> -- nslookup google.com 8.8.8.8

# 步骤 4：修复配置（如果需要）
kubectl edit configmap -n kube-system coredns

# 常见修复：指定可靠的上游 DNS
# forward . 8.8.8.8 8.8.4.4 {
#    max_concurrent 1000
# }

# 步骤 5：重新加载配置
kubectl rollout restart deployment -n kube-system coredns

# 步骤 6：验证修复
kubectl run test --rm -it --image=busybox -- nslookup kubernetes.default
kubectl run test --rm -it --image=busybox -- nslookup google.com
```

#### 3.2.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | 上游 DNS 变更可能影响解析 | 使用可靠的公共 DNS |
| **低** | 测试解析无风险 | - |

#### 3.2.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 上游 DNS 应该可靠且低延迟
2. 考虑配置多个上游 DNS 备份
3. 企业环境考虑使用内部 DNS
4. 注意 DNS 流量的安全性
5. 监控 DNS 解析延迟
```

### 3.3 DNS 性能优化

#### 3.3.1 解决步骤

```bash
# 步骤 1：检查当前性能
kubectl top pods -n kube-system -l k8s-app=kube-dns

# 步骤 2：扩展 CoreDNS 副本数
kubectl scale deployment -n kube-system coredns --replicas=3

# 步骤 3：优化 CoreDNS 配置
kubectl edit configmap -n kube-system coredns
# 优化配置示例：
# .:53 {
#     errors
#     health {
#        lameduck 5s
#     }
#     ready
#     kubernetes cluster.local in-addr.arpa ip6.arpa {
#        pods insecure
#        fallthrough in-addr.arpa ip6.arpa
#        ttl 30
#     }
#     prometheus :9153
#     forward . 8.8.8.8 8.8.4.4 {
#        max_concurrent 1000
#        policy sequential
#     }
#     cache 60 {
#        success 9984 60
#        denial 9984 5
#     }
#     loop
#     reload
#     loadbalance
# }

# 步骤 4：考虑启用 NodeLocal DNSCache
# 部署 NodeLocal DNSCache
kubectl apply -f https://raw.githubusercontent.com/kubernetes/kubernetes/master/cluster/addons/dns/nodelocaldns/nodelocaldns.yaml

# 步骤 5：增加资源限制
kubectl edit deployment -n kube-system coredns
# resources:
#   requests:
#     cpu: 200m
#     memory: 128Mi
#   limits:
#     cpu: 2000m
#     memory: 512Mi

# 步骤 6：应用配置
kubectl rollout restart deployment -n kube-system coredns

# 步骤 7：验证性能改善
# 测试解析延迟
time kubectl run test --rm -it --image=busybox -- nslookup kubernetes.default
```

#### 3.3.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **中** | NodeLocal DNS 部署复杂 | 先在测试环境验证 |
| **低** | 扩展副本数一般无风险 | 确保资源充足 |
| **中** | 配置变更需要重启 | 确保多副本 |

#### 3.3.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. DNS 扩容需要评估节点资源
2. NodeLocal DNSCache 需要节点支持
3. 缓存时间过长可能导致更新延迟
4. 监控 DNS 请求量和延迟
5. 考虑 DNS 查询的 QPS 限制
```

### 3.4 Pod DNS 配置问题

#### 3.4.1 解决步骤

```bash
# 步骤 1：检查 Pod 的 DNS 配置
kubectl exec <pod-name> -- cat /etc/resolv.conf

# 步骤 2：检查 Pod 的 dnsPolicy
kubectl get pod <pod-name> -o yaml | grep -A10 dnsPolicy

# dnsPolicy 说明：
# - ClusterFirst: 优先使用集群 DNS（默认）
# - ClusterFirstWithHostNet: hostNetwork Pod 使用集群 DNS
# - Default: 使用节点 DNS 配置
# - None: 完全自定义 DNS

# 步骤 3：如果需要自定义 DNS，使用 dnsConfig
kubectl patch pod <pod-name> -p '{
  "spec": {
    "dnsPolicy": "None",
    "dnsConfig": {
      "nameservers": ["10.96.0.10"],
      "searches": ["default.svc.cluster.local", "svc.cluster.local", "cluster.local"],
      "options": [{"name": "ndots", "value": "5"}]
    }
  }
}'

# 步骤 4：调整 ndots（减少不必要的搜索）
# 对于频繁解析外部域名的应用
# dnsConfig:
#   options:
#     - name: ndots
#       value: "1"

# 步骤 5：使用 FQDN 减少搜索
# 应用中使用完整域名：google.com.（注意末尾的点）

# 步骤 6：验证 DNS 配置
kubectl exec <pod-name> -- cat /etc/resolv.conf
kubectl exec <pod-name> -- nslookup kubernetes.default.svc.cluster.local
```

#### 3.4.2 执行风险

| 风险等级 | 风险描述 | 缓解措施 |
|----------|----------|----------|
| **低** | DNS 策略变更影响解析行为 | 测试后再应用 |
| **低** | ndots 调整影响短域名解析 | 理解业务需求 |

#### 3.4.3 安全生产风险提示

```
⚠️  安全生产风险提示：
1. 不同 dnsPolicy 行为不同，理解后再配置
2. ndots 设置影响解析性能
3. hostNetwork Pod 默认不使用集群 DNS
4. 自定义 DNS 配置需要维护
5. 使用 FQDN 可以提高解析效率
```

---

## 附录

### A. CoreDNS 关键指标

| 指标名称 | 说明 | 告警阈值建议 |
|----------|------|--------------|
| `coredns_dns_request_duration_seconds` | 请求延迟 | P99 > 100ms |
| `coredns_dns_requests_total` | 请求总数 | 监控趋势 |
| `coredns_dns_responses_total{rcode="SERVFAIL"}` | 失败响应 | > 1% |
| `coredns_forward_healthcheck_failures_total` | 上游失败 | > 0 |

### B. 常见 DNS 策略

| 策略 | 说明 | 适用场景 |
|------|------|----------|
| ClusterFirst | 优先集群 DNS | 大多数 Pod |
| ClusterFirstWithHostNet | hostNetwork 用集群 DNS | hostNetwork Pod |
| Default | 继承节点 DNS | 需要节点 DNS |
| None | 完全自定义 | 特殊需求 |

### C. DNS 调试清单

- [ ] CoreDNS Pod 正常运行
- [ ] kube-dns Service 有 Endpoints
- [ ] Pod 到 CoreDNS 网络通畅
- [ ] CoreDNS 配置正确
- [ ] 上游 DNS 可用
- [ ] Pod resolv.conf 正确
- [ ] ndots 设置合理

---

## 4. 生产环境典型案例

### 案例 1：ndots 配置不当导致外部 API 调用延迟暴增

**故障现场**

- **现象**：业务团队反馈调用第三方支付 API（`pay.example.com`）延迟从 50ms 激增至 500ms
- **影响范围**：所有调用外部 API 的服务（~200 Pod）
- **业务影响**：支付订单处理耗时增加 10 倍，用户投诉量上升

**排查过程**

```bash
# 1. 抓取 Pod 内 DNS 查询日志
kubectl exec -it payment-api-7d8f4 -- tcpdump -i any -n port 53 -A
# 发现查询 pay.example.com 时进行了 5 次 DNS 查询：
# pay.example.com.default.svc.cluster.local  -> NXDOMAIN
# pay.example.com.svc.cluster.local          -> NXDOMAIN
# pay.example.com.cluster.local              -> NXDOMAIN
# pay.example.com                            -> 成功（耗时 125ms）
# 总耗时 = 3 × 25ms（失败查询）+ 125ms（成功查询）= 200ms

# 2. 检查 Pod DNS 配置
kubectl exec payment-api-7d8f4 -- cat /etc/resolv.conf
# nameserver 10.96.0.10
# search default.svc.cluster.local svc.cluster.local cluster.local
# options ndots:5  # ← 罪魁祸首

# 3. 查看 CoreDNS 指标
curl http://coredns-svc:9153/metrics | grep coredns_dns_requests_total
# coredns_dns_requests_total{rcode="NXDOMAIN"} 120000  # 大量 NXDOMAIN 查询
# coredns_dns_requests_total{rcode="NOERROR"} 30000

# 4. 分析：
# ndots:5 导致外部域名（点数 < 5）先进行 search 域扩展
# 每次查询 pay.example.com 浪费 3 次集群内域名查询
```

**应急措施**

```yaml
# 方案 A：调整 ndots 为 2（推荐）
apiVersion: v1
kind: Pod
metadata:
  name: payment-api
spec:
  dnsConfig:
    options:
    - name: ndots
      value: "2"  # 外部域名（2 个点）直接查询，集群内短名（0 个点）仍走搜索域
  containers:
  - name: app
    image: payment-api:v1

# 方案 B：应用层优化（临时）
# 在代码中使用 FQDN：pay.example.com.（末尾加点）
```

```bash
# 执行滚动更新
kubectl set env deployment/payment-api DNS_FQDN="pay.example.com."  # 应用层方案
kubectl patch deployment payment-api --patch "$(cat <<EOF
spec:
  template:
    spec:
      dnsConfig:
        options:
        - name: ndots
          value: "2"
EOF
)"  # 基础设施方案

# 验证效果
kubectl exec payment-api-new-pod -- time nslookup pay.example.com
# 查询次数降为 1 次，延迟 50ms
```

**长期优化**

```yaml
# 1. 全局默认 ndots 配置（kubelet）
apiVersion: v1
kind: ConfigMap
metadata:
  name: kubelet-config
  namespace: kube-system
data:
  kubelet: |
    apiVersion: kubelet.config.k8s.io/v1beta1
    kind: KubeletConfiguration
    clusterDNS:
    - "10.96.0.10"
    clusterDomain: cluster.local
    resolvConf: /run/systemd/resolve/resolv.conf
    # 自定义 DNS 策略（需 kubelet v1.26+）
    # dnsConfig:
    #   options:
    #   - name: ndots
    #     value: "2"

# 2. 启用 NodeLocal DNSCache 减少跨节点查询
kubectl apply -f https://raw.githubusercontent.com/kubernetes/kubernetes/master/cluster/addons/dns/nodelocaldns/nodelocaldns.yaml

# 3. 调优 CoreDNS 缓存
kubectl edit cm -n kube-system coredns
# cache 60 {
#     success 9984 60
#     denial 9984 10   # NXDOMAIN 缓存 10 秒（减少重复失败查询）
#     prefetch 10 60s
# }

# 4. 监控告警
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: coredns-nxdomain-alert
  namespace: kube-system
spec:
  groups:
  - name: coredns
    interval: 30s
    rules:
    - alert: HighNXDOMAINRate
      expr: rate(coredns_dns_responses_total{rcode="NXDOMAIN"}[5m]) / rate(coredns_dns_responses_total[5m]) > 0.3
      for: 10m
      annotations:
        summary: "NXDOMAIN 响应率 >30%（{{ \$value | humanizePercentage }}），检查 ndots 配置"
EOF
```

**效果评估**

| 指标 | 优化前 | 优化后 | 改善 |
|-----|--------|--------|------|
| 外部 API 延迟（P99） | 500ms | 50ms | ↓ 90% |
| DNS 查询次数/请求 | 5 次 | 1 次 | ↓ 80% |
| CoreDNS CPU 使用率 | 45% | 18% | ↓ 60% |
| NXDOMAIN 响应率 | 75% | 5% | ↓ 93% |

---

### 案例 2：CoreDNS 上游 DNS 故障导致集群级服务中断

**故障现场**

- **现象**：凌晨 2:35 所有 Pod 无法解析外部域名，集群内域名正常
- **影响范围**：全集群（500+ 节点，10000+ Pod）
- **业务影响**：
  - 拉取外部镜像失败（新 Pod 无法启动）
  - 调用外部 API 失败（支付、短信、邮件服务全部中断）
  - Ingress Controller 无法解析上游域名（CDN 回源失败）

**排查过程**

```bash
# 1. 验证故障范围
kubectl run test --rm -it --image=busybox -- nslookup google.com
# ;; connection timed out; no servers could be reached

kubectl run test --rm -it --image=busybox -- nslookup kubernetes.default
# Server:    10.96.0.10
# Address 1: 10.96.0.10 kube-dns.kube-system.svc.cluster.local
# Name:      kubernetes.default
# Address 1: 10.96.0.1 kubernetes.default.svc.cluster.local
# ✅ 集群内域名正常

# 2. 检查 CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
# NAME                      READY   STATUS    AGE
# coredns-7d8f4b6c9-5xqhz   1/1     Running   12d
# coredns-7d8f4b6c9-9lkjm   1/1     Running   12d
# ✅ Pod 正常

kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50 | grep -E "error|timeout|forward"
# [ERROR] plugin/forward: google.com. dial udp 10.0.0.53:53: i/o timeout
# [ERROR] plugin/forward: github.com. dial udp 10.0.0.53:53: i/o timeout
# ❌ 上游 DNS 10.0.0.53 超时

# 3. 测试上游 DNS 可达性
kubectl exec -n kube-system coredns-7d8f4b6c9-5xqhz -- nslookup google.com 10.0.0.53
# ;; connection timed out
# ❌ 上游 DNS 不可达

kubectl exec -n kube-system coredns-7d8f4b6c9-5xqhz -- nslookup google.com 8.8.8.8
# Server:    8.8.8.8
# Address 1: 8.8.8.8 dns.google
# Name:      google.com
# Address 1: 142.250.185.46
# ✅ 公网 DNS 正常

# 4. 检查 Corefile 配置
kubectl get cm -n kube-system coredns -o yaml | grep -A3 forward
# forward . 10.0.0.53 {  # ← 单点故障：仅配置一个上游 DNS
#     max_concurrent 1000
# }

# 5. 根因分析：
# - 企业内网 DNS 服务器 10.0.0.53 故障（网络团队排查中）
# - CoreDNS 未配置备用上游 DNS
# - 缓存机制无法覆盖未查询过的域名
```

**应急措施**

```bash
# 紧急切换到公网 DNS（3 分钟内恢复）
kubectl edit cm -n kube-system coredns
# 修改 forward 配置：
# forward . 8.8.8.8 8.8.4.4 {
#     max_concurrent 1000
#     policy sequential
#     health_check 5s
# }

# 热更新 CoreDNS（无需重启）
kubectl rollout restart deployment -n kube-system coredns

# 验证恢复
kubectl run test --rm -it --image=busybox -- nslookup google.com
# Server:    10.96.0.10
# Address 1: 10.96.0.10 kube-dns.kube-system.svc.cluster.local
# Name:      google.com
# Address 1: 142.250.185.46
# ✅ 外部域名解析恢复
```

**长期优化**

```yaml
# 1. 高可用上游 DNS 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health {
           lameduck 5s
        }
        ready
        kubernetes cluster.local in-addr.arpa ip6.arpa {
           pods insecure
           fallthrough in-addr.arpa ip6.arpa
           ttl 30
        }
        prometheus :9153
        
        # 高可用配置：主备 DNS + 公网 DNS 兜底
        forward . 10.0.0.53 10.0.0.54 8.8.8.8 {
            max_concurrent 1000
            policy sequential  # 顺序尝试，第一个失败立即切换
            health_check 5s    # 5 秒健康检查
            expire 10s         # 10 秒后标记为不健康
        }
        
        # 积极缓存策略
        cache 60 {
            success 9984 60   # 成功响应缓存 1 分钟
            denial 9984 10
            prefetch 10 60s
            serve_stale       # ← 关键：上游故障时返回过期缓存
        }
        
        loop
        reload
        loadbalance
    }

# 2. 部署 NodeLocal DNSCache 提升容灾能力
kubectl apply -f - <<EOF
apiVersion: v1
kind: ConfigMap
metadata:
  name: node-local-dns
  namespace: kube-system
data:
  Corefile: |
    cluster.local:53 {
        errors
        cache {
            success 9984 600   # 本地缓存 10 分钟（更长时间）
            denial 9984 30
            serve_stale        # 上游故障时返回过期缓存
        }
        reload
        loop
        bind 169.254.20.10
        forward . 10.96.0.10 {
            force_tcp         # 使用 TCP 连接 CoreDNS（更可靠）
        }
        prometheus :9253
    }
    
    .:53 {
        errors
        cache {
            success 9984 600
            denial 9984 30
            serve_stale
        }
        reload
        loop
        bind 169.254.20.10
        forward . 10.0.0.53 10.0.0.54 8.8.8.8 {
            policy sequential
            health_check 5s
        }
        prometheus :9253
    }
---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-local-dns
  namespace: kube-system
  labels:
    k8s-app: node-local-dns
spec:
  selector:
    matchLabels:
      k8s-app: node-local-dns
  template:
    metadata:
      labels:
        k8s-app: node-local-dns
    spec:
      priorityClassName: system-node-critical
      serviceAccountName: node-local-dns
      hostNetwork: true
      dnsPolicy: Default
      tolerations:
      - effect: NoExecute
        operator: Exists
      - effect: NoSchedule
        operator: Exists
      containers:
      - name: node-cache
        image: registry.k8s.io/dns/k8s-dns-node-cache:1.22.20
        resources:
          requests:
            cpu: 25m
            memory: 25Mi
        args: [ "-localip", "169.254.20.10", "-conf", "/etc/coredns/Corefile", "-upstreamsvc", "kube-dns" ]
        securityContext:
          privileged: true
        volumeMounts:
        - name: config-volume
          mountPath: /etc/coredns
        - name: xtables-lock
          mountPath: /run/xtables.lock
      volumes:
      - name: config-volume
        configMap:
          name: node-local-dns
      - name: xtables-lock
        hostPath:
          path: /run/xtables.lock
          type: FileOrCreate
EOF

# 3. 监控告警
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: coredns-upstream-alert
  namespace: kube-system
spec:
  groups:
  - name: coredns-upstream
    interval: 15s
    rules:
    # 上游健康检查失败
    - alert: CoreDNS_UpstreamUnhealthy
      expr: rate(coredns_forward_healthcheck_failures_total[1m]) > 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "CoreDNS 上游 DNS {{ \$labels.to }} 健康检查失败"
        description: "立即检查上游 DNS 服务器状态"
    
    # 外部域名解析失败率过高
    - alert: CoreDNS_ExternalQueryFailureRate
      expr: |
        rate(coredns_forward_requests_total{rcode="SERVFAIL"}[5m]) 
        / rate(coredns_forward_requests_total[5m]) > 0.05
      for: 3m
      labels:
        severity: critical
      annotations:
        summary: "外部域名解析失败率 >5%（{{ \$value | humanizePercentage }}）"
EOF
```

**事后复盘**

| 维度 | 问题 | 改进措施 |
|-----|------|---------|
| **架构** | 单点故障（仅 1 个上游 DNS） | 配置 3 个上游 DNS（主备 + 公网兜底） |
| **缓存** | 缓存时间短（30s） | 延长至 60s，启用 `serve_stale` |
| **监控** | 无上游健康监控 | 新增 `CoreDNS_UpstreamUnhealthy` 告警 |
| **容灾** | 依赖集群级 DNS | 部署 NodeLocal DNSCache（节点级缓存） |
| **变更** | 未做 DNS 容灾演练 | 每季度故障注入测试（Chaos Mesh） |

**故障时间线**

```
02:35:00 - 企业内网 DNS 10.0.0.53 故障
02:35:15 - CoreDNS 开始报 timeout 错误
02:36:00 - 业务告警：外部 API 调用失败
02:38:00 - 运维介入，确认 DNS 故障
02:41:00 - 切换到公网 DNS，滚动重启 CoreDNS
02:44:00 - 外部域名解析恢复
02:50:00 - 所有业务服务恢复正常
总中断时间：15 分钟
```

---

### 案例 3：CoreDNS 内存泄漏导致 OOMKilled

**故障现场**

- **现象**：CoreDNS Pod 每 2-3 小时重启一次（OOMKilled）
- **影响范围**：重启期间 DNS 解析延迟增加（5-10s）
- **业务影响**：间歇性服务调用超时，用户体验下降

**排查过程**

```bash
# 1. 确认 OOMKilled
kubectl describe pod -n kube-system coredns-7d8f4b6c9-5xqhz | grep -A5 "Last State"
# Last State:     Terminated
#   Reason:       OOMKilled
#   Exit Code:    137
#   Started:      Mon, 01 Jan 2024 10:00:00 +0000
#   Finished:     Mon, 01 Jan 2024 12:30:00 +0000

# 2. 查看内存使用趋势
kubectl top pod -n kube-system coredns-7d8f4b6c9-5xqhz --containers
# NAME        CPU     MEMORY
# coredns     120m    380Mi  # ← 接近 limit (512Mi)

# 3. 查看资源配置
kubectl get deployment -n kube-system coredns -o yaml | grep -A10 resources
# resources:
#   limits:
#     memory: 512Mi
#   requests:
#     memory: 70Mi  # ← request 过低，未反映真实使用

# 4. 分析 CoreDNS 指标
curl http://coredns-svc:9153/metrics | grep -E "cache|memory"
# coredns_cache_entries{type="success"} 9984   # 缓存已满
# coredns_cache_entries{type="denial"} 9984   # 缓存已满
# process_resident_memory_bytes 398458880      # ~380MB

# 5. 检查 Corefile 配置
kubectl get cm -n kube-system coredns -o yaml | grep -A5 cache
# cache 30 {
#     success 9984 30  # ← 默认容量，未调整
#     denial 9984 5
# }

# 6. 分析日志发现大量唯一域名查询
kubectl logs -n kube-system coredns-7d8f4b6c9-5xqhz | awk '{print $NF}' | sort | uniq -c | sort -rn | head -20
# 500 random-string-1.attacker.com
# 480 random-string-2.attacker.com
# ...
# ❌ 发现大量随机域名查询（疑似 DDoS 攻击）

# 7. 根因分析：
# - 缓存容量固定（9984 条），大量唯一域名查询填满缓存
# - 内存持续增长，最终触发 OOM
# - 可能原因：应用 bug 或恶意攻击
```

**应急措施**

```bash
# 1. 临时提高内存 limit
kubectl patch deployment -n kube-system coredns -p '{
  "spec": {
    "template": {
      "spec": {
        "containers": [{
          "name": "coredns",
          "resources": {
            "requests": {"cpu": "200m", "memory": "256Mi"},
            "limits": {"cpu": "2000m", "memory": "1Gi"}
          }
        }]
      }
    }
  }
}'

# 2. 限制缓存容量并启用驱逐策略
kubectl edit cm -n kube-system coredns
# cache 30 {
#     success 5000 30   # 减半容量
#     denial 2000 5     # 减少否定缓存
#     prefetch 10 60s
# }

# 3. 阻止异常域名查询（临时）
kubectl edit cm -n kube-system coredns
# 在 Corefile 开头添加 blacklist 插件（需 CoreDNS 编译支持）
# .:53 {
#     # 使用 template 插件返回 NXDOMAIN（临时方案）
#     template IN A attacker.com {
#         rcode NXDOMAIN
#     }
#     ...
# }

# 4. 扩展副本应对负载
kubectl scale deployment -n kube-system coredns --replicas=5
```

**长期优化**

```yaml
# 1. 启用 ratelimit 插件（需自定义 CoreDNS 镜像）
# 下载 CoreDNS 源码并编译包含 ratelimit 插件的镜像
git clone https://github.com/coredns/coredns.git
cd coredns
# 在 plugin.cfg 中添加 ratelimit
echo "ratelimit:github.com/milgradesec/ratelimit" >> plugin.cfg
go generate && go build

# 2. 配置限流策略
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health
        ready
        
        # 限流插件（需自定义镜像）
        ratelimit 100 {  # 每 IP 每秒 100 次查询
            window 1s
            ipv4-mask 24  # 按 /24 网段限流
        }
        
        kubernetes cluster.local in-addr.arpa ip6.arpa {
           pods insecure
           fallthrough
           ttl 30
        }
        
        prometheus :9153
        
        forward . 8.8.8.8 8.8.4.4 {
            max_concurrent 1000
            policy sequential
        }
        
        # 优化缓存配置
        cache 60 {
            success 5000 60   # 降低容量，延长 TTL
            denial 2000 10
            prefetch 10 60s
        }
        
        loop
        reload
        loadbalance
    }

# 3. 部署网络策略限制 DNS 访问（防护层）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: restrict-coredns-access
  namespace: kube-system
spec:
  podSelector:
    matchLabels:
      k8s-app: kube-dns
  policyTypes:
  - Ingress
  ingress:
  # 仅允许集群内 Pod 访问
  - from:
    - podSelector: {}
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53

# 4. 监控内存使用
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: coredns-memory-alert
  namespace: kube-system
spec:
  groups:
  - name: coredns-memory
    interval: 30s
    rules:
    - alert: CoreDNS_HighMemoryUsage
      expr: container_memory_working_set_bytes{pod=~"coredns-.*"} / container_spec_memory_limit_bytes{pod=~"coredns-.*"} > 0.8
      for: 5m
      annotations:
        summary: "CoreDNS 内存使用率 >80%（{{ \$value | humanizePercentage }}）"
    
    - alert: CoreDNS_CacheFull
      expr: coredns_cache_entries{type="success"} >= 4500
      for: 10m
      annotations:
        summary: "CoreDNS 缓存接近满载（{{ \$value }} / 5000）"
EOF
```

**效果评估**

| 指标 | 优化前 | 优化后 | 改善 |
|-----|--------|--------|------|
| OOMKilled 频率 | 每 2-3 小时 | 0 次/月 | ✅ 完全消除 |
| 稳定内存使用 | 380Mi（峰值 500Mi+） | 220Mi（峰值 280Mi） | ↓ 42% |
| 缓存命中率 | 45%（频繁驱逐） | 82% | ↑ 82% |
| DNS 查询 QPS | 8000 | 12000（限流后） | - |
