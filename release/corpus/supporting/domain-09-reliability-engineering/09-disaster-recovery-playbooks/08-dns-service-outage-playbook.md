---
title: DNS 服务中断恢复
description: 'CoreDNS 全部不可用紧急恢复：Pod 状态排查、DNS 缓存检查、NodeLocal DNSCache 故障恢复及临时旁路方案'
summary: 'CoreDNS 全部不可用紧急恢复：Pod 状态排查、DNS 缓存检查、NodeLocal DNSCache 故障恢复及临时旁路方案'
category: reliability-engineering
tags:
- disaster-recovery
- k8s
- sre
- coredns
- dns
- nodelocaldns
- servicemesh
tier: critical
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- DNS 服务中断恢复 是什么
- 如何恢复 CoreDNS 不可用
- NodeLocal DNSCache 故障怎么处理
trigger_keywords:
- coredns
- dns
- nodelocaldns
- kube-dns
- dns-policy
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

# DNS 服务中断恢复

## 概述

CoreDNS 是 Kubernetes 集群的 DNS 服务核心，负责 Service 发现和外部域名解析。当所有 CoreDNS Pod 不可用时，集群内所有依赖域名解析的工作负载将出现连接超时、`NXDOMAIN` 错误或 `connection refused`。本手册覆盖 CoreDNS 故障排查、NodeLocal DNSCache 恢复、DNS 配置修复及紧急旁路方案。

---

## 1. CoreDNS Pod 状态排查

### 1.1 快速状态检查

```bash
# 检查 CoreDNS Pod 状态
kubectl -n kube-system get pod -l k8s-app=kube-dns -o wide

# 正常状态：Running，READY 1/1 或 2/2
# 异常状态：
#   CrashLoopBackOff → 配置错误或资源不足
#   Pending          → 调度失败（节点资源/PDB 限制）
#   ImagePullBackOff → 镜像拉取失败
#   Evicted          → 节点磁盘或内存压力
```

### 1.2 CrashLoopBackOff 排查

```bash
# 查看 Pod 事件和日志
kubectl -n kube-system describe pod -l k8s-app=kube-dns
kubectl -n kube-system logs -l k8s-app=kube-dns -p --tail=50  # 上一次崩溃日志
kubectl -n kube-system logs -l k8s-app=kube-dns --tail=50      # 当前日志

# 常见 CrashLoopBackOff 原因：
#   1. Corefile 配置语法错误
#   2. 内存不足（OOMKilled）
#   3. 端口 53 被占用

# 检查是否 OOMKilled
kubectl -n kube-system get pod -l k8s-app=kube-dns -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.containerStatuses[*].lastState.terminated.reason}{"\n"}{end}'
```

### 1.3 Pending Pod 排查

```bash
# 查看调度失败原因
kubectl -n kube-system describe pod -l k8s-app=kube-dns | grep -A5 "Events"

# 常见原因：
#   资源不足 → 检查 node allocatable
#   PDB 限制 → 检查 PodDisruptionBudget
#   污点/亲和性 → 检查节点 taint 和 CoreDNS tolerations

# 检查 PDB
kubectl -n kube-system get pdb -l k8s-app=kube-dns
```

---

## 2. CoreDNS Corefile 配置检查

### 2.1 查看当前配置

```bash
# Corefile 存储在 ConfigMap 中
kubectl -n kube-system get configmap coredns -o yaml

# 典型 Corefile 结构：
#   .:53 {
#       errors
#       health { lameduck 5s }
#       ready
#       kubernetes cluster.local in-addr.arpa ip6.arpa {
#           pods insecure
#           fallthrough in-addr.arpa ip6.arpa
#           ttl 30
#       }
#       prometheus :9153
#       forward . /etc/resolv.conf
#       cache 30
#       loop
#       reload
#       loadbalance
#   }
```

### 2.2 Corefile 语法验证

```bash
# 备份当前 ConfigMap
kubectl -n kube-system get configmap coredns -o yaml > coredns-backup.yaml

# 在 CoreDNS Pod 内验证配置
kubectl -n kube-system exec deploy/coredns -- cat /etc/coredns/Corefile

# 检查上游 DNS 是否可达（CoreDNS forward 目标）
kubectl -n kube-system exec deploy/coredns -- nslookup google.com 8.8.8.8

# 如果上游 DNS 不可达，修改 forward 为可达的 DNS 服务器
kubectl -n kube-system edit configmap coredns
# 将 forward . /etc/resolv.conf 改为 forward . <reachable-dns-ip>
# 然后重启 CoreDNS
kubectl -n kube-system rollout restart deployment/coredns
```

### 2.3 CoreDNS 资源限制调优

```bash
# 查看当前资源限制
kubectl -n kube-system get deploy corefile -o jsonpath='{.spec.template.spec.containers[0].resources}'

# 如果频繁 OOM，增加内存限制
kubectl -n kube-system set resources deployment/coredns \
  --limits=memory=512Mi \
  --requests=cpu=250m,memory=256Mi
```

---

## 3. DNS 缓存与上游配置

### 3.1 CoreDNS 缓存配置

```
# Corefile 中缓存相关指令：
#   cache 30                    → 正向解析缓存 30 秒
#   cache 30 {
#       denial 10 0             → NXDOMAIN 缓存 10 秒
#       success 1000 30         → 成功缓存最多 1000 条，TTL 30 秒
#   }

# 如果怀疑缓存污染，重启 CoreDNS 清除内存缓存
kubectl -n kube-system rollout restart deployment/coredns
```

### 3.2 节点级 /etc/resolv.conf 检查

```bash
# CoreDNS 使用 Pod 内的 /etc/resolv.conf（继承自节点）
# 检查节点的 resolv.conf
kubectl -n kube-system exec deploy/coredns -- cat /etc/resolv.conf

# 常见问题：
#   nameserver 指向已下线的 DNS 服务器
#   search 域过长导致 DNS 查询超时
#   ndots 设置不合理（默认 5，应根据场景调低）
```

### 3.3 上游 DNS 可达性验证

```bash
# 从 CoreDNS Pod 内部测试上游 DNS
kubectl -n kube-system exec deploy/coredns -- nslookup kubernetes.default 10.96.0.10
kubectl -n kube-system exec deploy/coredns -- nslookup google.com

# 从普通 Pod 测试集群 DNS
kubectl run dns-test --image=busybox --rm -it --restart=Never -- nslookup kubernetes.default

# 测试外部 DNS
kubectl run dns-test --image=busybox --rm -it --restart=Never -- nslookup google.com
```

---

## 4. NodeLocal DNSCache 故障恢复

### 4.1 NodeLocal DNSCache 原理

NodeLocal DNSCache 在每个节点上运行一个 DNS 缓存 Pod，监听节点本地 IP（默认 169.254.20.10），避免 DNS 请求经过 kube-proxy iptables 规则，降低 CoreDNS 负载和 DNS 延迟。

### 4.2 状态检查

```bash
# 查看 NodeLocal DNSCache Pod
kubectl -n kube-system get pod -l k8s-app=node-local-dns -o wide

# 查看 DaemonSet 状态
kubectl -n kube-system get daemonset node-local-dns

# 检查节点本地 DNS 是否正常监听
# 在任意节点上执行
kubectl debug node/<node-name> -it --image=busybox -- nslookup kubernetes.default 169.254.20.10
```

### 4.3 NodeLocal DNSCache Pod 故障

```bash
# 查看日志
kubectl -n kube-system logs -l k8s-app=node-local-dns --tail=100

# 常见故障：
#   1. 端口 53 被占用 → 检查节点上是否有其他 DNS 服务
#   2. iptables 规则冲突 → 与 kube-proxy 模式冲突
#   3. 内存不足 → 调整资源限制

# 重启单个节点的 NodeLocal DNSCache
kubectl -n kube-system delete pod -l k8s-app=node-local-dns --field-selector spec.nodeName=<node-name>

# 全部重启
kubectl -n kube-system rollout restart daemonset/node-local-dns
```

### 4.4 iptables 规则修复

```bash
# NodeLocal DNSCache 依赖 iptables 规则将 DNS 流量重定向到本地缓存
# 检查规则是否存在（在节点上执行）
iptables -t nat -L KUBE-SERVICES | grep 169.254.20.10

# 如果规则丢失，重启 NodeLocal DNSCache Pod 会自动重建
# 如果规则冲突，临时删除冲突规则
iptables -t nat -D KUBE-SERVICES -d <conflicting-ip> -p udp --dport 53 -j KUBE-SVC-<hash>
```

---

## 5. 集群内外 DNS 解析验证

### 5.1 Service 解析验证

```bash
# 验证 ClusterIP Service 解析
kubectl run dns-test --image=busybox --rm -it --restart=Never -- \
  nslookup kubernetes.default.svc.cluster.local

# 验证 Headless Service 解析
kubectl run dns-test --image=busybox --rm -it --restart=Never -- \
  nslookup <headless-svc>.<namespace>.svc.cluster.local

# 验证 Pod FQDN 解析
kubectl run dns-test --image=busybox --rm -it --restart=Never -- \
  nslookup <pod-ip-dashed>.<namespace>.pod.cluster.local
```

### 5.2 外部域名解析验证

```bash
# 验证外部域名
kubectl run dns-test --image=busybox --rm -it --restart=Never -- \
  nslookup google.com

# 验证带自定义域名的 Service
kubectl run dns-test --image=busybox --rm -it --restart=Never -- \
  nslookup <service>.<namespace>.svc.<custom-domain>
```

### 5.3 DNS 解析延迟测试

```bash
# 使用 dig 测试解析延迟
kubectl run dns-perf --image=busybox --rm -it --restart=Never -- sh -c '
  for i in $(seq 1 10); do
    start=$(date +%s%N)
    nslookup kubernetes.default > /dev/null 2>&1
    end=$(date +%s%N)
    echo "Query $i: $(( (end - start) / 1000000 ))ms"
  done
'

# 正常延迟应 < 5ms（有 NodeLocal DNSCache）或 < 20ms（无缓存）
```

---

## 6. 临时 DNS 旁路方案

### 6.1 使用 hostNetwork DNS

当 CoreDNS 完全不可用时，可临时将关键 Pod 的 DNS 策略切换到节点 DNS：

```bash
# 修改 Pod 的 dnsPolicy
kubectl patch deployment <app-name> -n <namespace> --type=json -p='[
  {"op":"add","path":"/spec/template/spec/dnsPolicy","value":"ClusterFirstWithHostNet"}
]'
```

### 6.2 使用 dnsConfig 指定自定义 DNS

```bash
# 为 Pod 指定外部 DNS 服务器
kubectl patch deployment <app-name> -n <namespace> --type=strategic --patch='
spec:
  template:
    spec:
      dnsConfig:
        nameservers:
        - 8.8.8.8
        - 114.114.114.114
        searches:
        - <namespace>.svc.cluster.local
        - svc.cluster.local
        - cluster.local
        options:
        - name: ndots
          value: "2"
      dnsPolicy: None
'
```

### 6.3 临时部署外部 DNS Pod

```bash
# 快速部署一个 dnsmasq 作为临时 DNS
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: emergency-dns
  namespace: kube-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: emergency-dns
  template:
    metadata:
      labels:
        app: emergency-dns
    spec:
      containers:
      - name: dnsmasq
        image: janeczku/go-dnsmasq:release-1.0.7
        args:
        - --listen=0.0.0.0:53
        - --default-resolver
        - --append-search-domains
        - --hostsfile=/etc/hosts
        - --verbose
        ports:
        - containerPort: 53
          protocol: UDP
        resources:
          limits:
            memory: 64Mi
            cpu: 100m
---
apiVersion: v1
kind: Service
metadata:
  name: emergency-dns
  namespace: kube-system
spec:
  selector:
    app: emergency-dns
  ports:
  - port: 53
    targetPort: 53
    protocol: UDP
  type: ClusterIP
EOF

# 然后将 Pod 的 dnsPolicy 改为指向这个临时 DNS
```

### 6.4 通过 /etc/hosts 硬编码

```bash
# 仅适用于极少数关键服务的紧急方案
kubectl patch deployment <app-name> -n <namespace> --type=strategic --patch='
spec:
  template:
    spec:
      hostAliases:
      - ip: "10.96.100.1"
        hostnames:
        - "critical-svc.default.svc.cluster.local"
'
```

---

## 7. 生产最佳实践

### 7.1 CoreDNS 高可用配置

| 配置项 | 推荐值 | 说明 |
|--------|--------|------|
| 副本数 | ≥ 3 | 至少 2 个，推荐 3+ |
| PDB | minAvailable: 1 | 保证滚动更新时始终有 DNS 可用 |
| Pod 反亲和性 | requiredDuringScheduling | 分散到不同节点 |
| HPA | CPU > 70% 触发 | 根据负载自动扩缩 |
| 资源请求 | cpu: 250m, mem: 256Mi | 避免被优先级驱逐 |

### 7.2 PodDisruptionBudget 配置

```yaml
apiVersion: policy/v1
kind: PodDisruptionBudget
metadata:
  name: coredns-pdb
  namespace: kube-system
spec:
  minAvailable: 1
  selector:
    matchLabels:
      k8s-app: kube-dns
```

### 7.3 监控告警

```yaml
groups:
- name: dns-alerts
  rules:
  - alert: CoreDNSDown
    expr: up{job="kubernetes-pods",pod=~"coredns.*"} == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "CoreDNS Pod {{ $labels.pod }} 不可用"

  - alert: CoreDNSErrorsHigh
    expr: rate(coredns_dns_responses_total{rcode="SERVFAIL"}[5m]) > 10
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS SERVFAIL 错误率过高"

  - alert: DNSLatencyHigh
    expr: histogram_quantile(0.99, rate(coredns_dns_request_duration_seconds_bucket[5m])) > 0.5
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "DNS P99 延迟超过 500ms"
```

---

## 8. 故障排查

### 8.1 常见故障对照表

| 症状 | 可能原因 | 处理方法 |
|------|---------|---------|
| 所有 DNS 查询超时 | CoreDNS Pod 全部不可用 | 检查 Pod 状态和节点健康 |
| 间歇性 NXDOMAIN | CoreDNS 缓存或 upstream 问题 | 检查 Corefile 和上游 DNS |
| 解析延迟 > 1s | NodeLocal DNSCache 不工作 | 检查 DaemonSet 和 iptables 规则 |
| 部分 Service 解析失败 | API Server 异常导致 endpoints 不同步 | 检查 API Server 和 endpoints |
| `dial tcp: lookup kubernetes: no such host` | Pod dnsPolicy 配置错误 | 检查 dnsPolicy 设置 |

### 8.2 DNS 日志分析

```bash
# 启用 CoreDNS query log（临时调试用）
# 在 Corefile 中添加 log 指令
kubectl -n kube-system edit configmap coredns
# 在 server block 中添加: log

# 查看查询日志
kubectl -n kube-system logs -l k8s-app=kube-dns --tail=200 | grep -E "A\b|AAAA\b|NXDOMAIN\b"

# 分析高频查询（找出 DNS 热点）
kubectl -n kube-system logs -l k8s-app=kube-dns --tail=1000 | \
  awk '{print $NF}' | sort | uniq -c | sort -rn | head -20
```

### 8.3 网络层排查

```bash
# 检查 CoreDNS Service 的 endpoints
kubectl -n kube-system get endpoints kube-dns

# 检查 kube-proxy 是否正确配置了 DNS Service 的 iptables/ipvs 规则
# iptables 模式
iptables -t nat -L KUBE-SERVICES | grep :53

# ipvs 模式
ipvsadm -L -n | grep :53

# 检查 Service 的 ClusterIP 是否可达
kubectl run nettest --image=busybox --rm -it --restart=Never -- \
  nc -zvu 10.96.0.10 53
```

---

## 参考链接

- [CoreDNS 官方文档](https://coredns.io/manual/toc/)
- [Kubernetes DNS 调试指南](https://kubernetes.io/docs/tasks/administer-cluster/dns-debugging-resolution/)
- [NodeLocal DNSCache 配置](https://kubernetes.io/docs/tasks/administer-cluster/nodelocaldns/)
- [Kubernetes dnsPolicy 详解](https://kubernetes.io/docs/concepts/services-networking/dns-pod-service/)
