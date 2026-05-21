---
title: 04 - DNS 服务发现与 CoreDNS 调优
description: 1. [CoreDNS 架构深度解析](#1-coredns-架构深度解析)
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- kubelet
- prometheus
- grafana
- coredns
- daemonset
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- DNS 服务发现与 CoreDNS 调优 是什么
- 如何 DNS 服务发现与 CoreDNS 调优
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- DNS
- 服务发现与
- CoreDNS
- 调优
- networking
prerequisites:
- kubectl-basics
- networking-basics
- prometheus-basics
- monitoring-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: fta
  path: ../domain-10-troubleshooting-diagnostics/topic-fta/list/dns-fta.md
  label: '故障树: dns'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

# 04 - DNS 服务发现与 CoreDNS 调优

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **难度**: 高级

---

<!-- chunk: 目录 -->
## 目录

1. [CoreDNS 架构深度解析](#1-coredns-架构深度解析)
2. [高性能配置优化](#2-高性能配置优化)
3. [服务发现机制详解](#3-服务发现机制详解)
4. [故障诊断与排错](#4-故障诊断与排错)
5. [监控与性能分析](#5-监控与性能分析)
6. [安全加固配置](#6-安全加固配置)
7. [多集群 DNS 管理](#7-多集群-dns-管理)
8. [生产环境最佳实践](#8-生产环境最佳实践)

---

<!-- chunk: 1. CoreDNS 架构深度解析 -->
## 1. CoreDNS 架构深度解析

### 1.1 CoreDNS 组件架构

```yaml
# CoreDNS 部署架构图
apiVersion: apps/v1
kind: Deployment
metadata:
  name: coredns
  namespace: kube-system
  labels:
    k8s-app: kube-dns
spec:
  replicas: 2
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 1
      maxSurge: 25%
  selector:
    matchLabels:
      k8s-app: kube-dns
  template:
    metadata:
      labels:
        k8s-app: kube-dns
    spec:
      priorityClassName: system-cluster-critical
      serviceAccountName: coredns
      tolerations:
        - key: "CriticalAddonsOnly"
          operator: "Exists"
      nodeSelector:
        kubernetes.io/os: linux
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: k8s-app
                  operator: In
                  values: ["kube-dns"]
              topologyKey: kubernetes.io/hostname
      containers:
      - name: coredns
        image: registry.k8s.io/coredns/coredns:v1.11.1
        imagePullPolicy: IfNotPresent
        resources:
          limits:
            memory: 170Mi
          requests:
            cpu: 100m
            memory: 70Mi
        args: [ "-conf", "/etc/coredns/Corefile" ]
        volumeMounts:
        - name: config-volume
          mountPath: /etc/coredns
          readOnly: true
        - name: tmp
          mountPath: /tmp
        ports:
        - containerPort: 53
          name: dns
          protocol: UDP
        - containerPort: 53
          name: dns-tcp
          protocol: TCP
        - containerPort: 9153
          name: metrics
          protocol: TCP
        securityContext:
          allowPrivilegeEscalation: false
          capabilities:
            add:
            - NET_BIND_SERVICE
            drop:
            - ALL
          readOnlyRootFilesystem: true
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
            scheme: HTTP
          initialDelaySeconds: 60
          timeoutSeconds: 5
          successThreshold: 1
          failureThreshold: 5
        readinessProbe:
          httpGet:
            path: /ready
            port: 8181
            scheme: HTTP
          initialDelaySeconds: 30
          timeoutSeconds: 5
          successThreshold: 1
          failureThreshold: 3
      volumes:
        - name: config-volume
          configMap:
            name: coredns
            items:
            - key: Corefile
              path: Corefile
        - name: tmp
          emptyDir: {}
```

### 1.2 Corefile 配置详解

```yaml
# 生产级 Corefile 配置
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
        
        # Kubernetes 服务发现插件
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
            ttl 30
        }
        
        # 联邦服务发现
        federation cluster.local {
            east us-east-1.cluster.example.com
            west us-west-1.cluster.example.com
        }
        
        # Prometheus 监控指标
        prometheus :9153
        
        # 上游 DNS 转发
        forward . /etc/resolv.conf {
            max_concurrent 1000
            health_check 5s
            expire 90s
        }
        
        # 缓存配置
        cache 30 {
            success 9984
            denial 9984
            prefetch 1 10m 10%
        }
        
        # 循环检测防止
        loop
        
        # 配置重载
        reload
        
        # 负载均衡
        loadbalance round_robin
        
        # 日志记录（生产环境建议关闭）
        # log
    }
    
    # 特定域的自定义配置
    example.com:53 {
        errors
        cache 300
        forward . 10.0.0.10 10.0.0.11 {
            health_check 5s
        }
    }
```

### 1.3 插件工作机制

```bash
#!/bin/bash
# CoreDNS 插件链分析脚本

echo "=== CoreDNS 插件工作机制分析 ==="
echo

# 获取 CoreDNS Pod 信息
COREDNS_POD=$(kubectl get pods -n kube-system -l k8s-app=kube-dns -o jsonpath='{.items[0].metadata.name}')

echo "1. CoreDNS 插件链顺序:"
kubectl exec -n kube-system $COREDNS_POD -- cat /etc/coredns/Corefile | \
  grep -E "^[[:space:]]*[a-z]" | sed 's/{.*//' | nl

echo -e "\n2. 插件配置详情:"
kubectl exec -n kube-system $COREDNS_POD -- cat /etc/coredns/Corefile

echo -e "\n3. 当前 DNS 解析统计:"
kubectl exec -n kube-system $COREDNS_POD -- curl -s http://localhost:9153/metrics | \
  grep -E "(coredns_dns_requests_total|coredns_dns_responses_total)" | head -10

echo -e "\n4. 缓存命中率:"
kubectl exec -n kube-system $COREDNS_POD -- curl -s http://localhost:9153/metrics | \
  grep coredns_cache_hits_total
```

---

<!-- chunk: 2. 高性能配置优化 -->
## 2. 高性能配置优化

### 2.1 资源配额优化

```yaml
# CoreDNS 性能优化资源配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: coredns-optimized
  namespace: kube-system
spec:
  replicas: 4  # 根据集群规模调整
  template:
    spec:
      containers:
      - name: coredns
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: 1000m
            memory: 512Mi
        env:
        - name: GOGC
          value: "20"  # 垃圾回收优化
        - name: GOMAXPROCS
          valueFrom:
            resourceFieldRef:
              resource: limits.cpu
```

### 2.2 缓存策略优化

```yaml
# 高性能缓存配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns-cache-optimized
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health
        
        # 优化的缓存配置
        cache 60 {                    # 增加缓存时间
            success 9984              # 成功响应缓存条目
            denial 9984               # 否定响应缓存条目
            prefetch 1 10m 10%        # 预取机制
        }
        
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
            ttl 30
        }
        
        # 负载均衡优化
        loadbalance round_robin {
            response_count 5
        }
        
        forward . /etc/resolv.conf {
            max_concurrent 2000       # 增加并发连接数
            health_check 3s           # 缩短健康检查间隔
            expire 30s                # 缩短过期时间
        }
        
        prometheus :9153
        reload
    }
```

### 2.3 NodeLocal DNSCache

```yaml
# NodeLocal DNSCache 部署
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: nodelocaldns
  namespace: kube-system
  labels:
    k8s-app: nodelocaldns
spec:
  selector:
    matchLabels:
      k8s-app: nodelocaldns
  template:
    metadata:
      labels:
        k8s-app: nodelocaldns
    spec:
      priorityClassName: system-node-critical
      serviceAccountName: nodelocaldns
      hostNetwork: true
      dnsPolicy: Default  # 绕过 Kubelet DNS 配置
      containers:
      - name: node-cache
        image: registry.k8s.io/dns/k8s-dns-node-cache:1.22.28
        resources:
          requests:
            cpu: 25m
            memory: 50Mi
          limits:
            cpu: 100m
            memory: 200Mi
        args:
        - -localip
        - 169.254.20.10,10.96.0.10    # 本地 DNS IP
        - -conf
        - /etc/Corefile
        - -upstreamsvc
        - kube-dns
        ports:
        - containerPort: 53
          name: dns
          protocol: UDP
        - containerPort: 53
          name: dns-tcp
          protocol: TCP
        - containerPort: 9253
          name: metrics
          protocol: TCP
        volumeMounts:
        - name: config-volume
          mountPath: /etc/coredns
        - name: kube-dns-config
          mountPath: /etc/kube-dns
        livenessProbe:
          httpGet:
            host: 169.254.20.10
            path: /health
            port: 8080
          initialDelaySeconds: 60
          timeoutSeconds: 5
        readinessProbe:
          httpGet:
            host: 169.254.20.10
            path: /health
            port: 8080
          initialDelaySeconds: 30
          timeoutSeconds: 5
      volumes:
      - name: config-volume
        configMap:
          name: nodelocaldns-config
          items:
          - key: Corefile
            path: Corefile
      - name: kube-dns-config
        configMap:
          name: kube-dns
          optional: true
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: nodelocaldns-config
  namespace: kube-system
data:
  Corefile: |
    cluster.local:53 {
        errors
        cache {
            success 9984
            denial 9984
            prefetch 1 1h 10%
        }
        reload
        loop
        bind 169.254.20.10 10.96.0.10
        forward . __PILLAR__CLUSTER__DNS__ {
            force_tcp
            prefer_udp
        }
        prometheus :9253
        health 169.254.20.10:8080
        }
    in-addr.arpa:53 {
        errors
        cache 30
        reload
        loop
        bind 169.254.20.10 10.96.0.10
        forward . __PILLAR__CLUSTER__DNS__ {
            force_tcp
            prefer_udp
        }
        prometheus :9253
        }
    ip6.arpa:53 {
        errors
        cache 30
        reload
        loop
        bind 169.254.20.10 10.96.0.10
        forward . __PILLAR__CLUSTER__DNS__ {
            force_tcp
            prefer_udp
        }
        prometheus :9253
        }
    .:53 {
        errors
        cache 30
        reload
        loop
        bind 169.254.20.10 10.96.0.10
        forward . /etc/resolv.conf
        prometheus :9253
        }
```

---

<!-- chunk: 3. 服务发现机制详解 -->
## 3. 服务发现机制详解

### 3.1 A/AAAA 记录生成

```bash
#!/bin/bash
# DNS 服务发现机制验证脚本

echo "=== Kubernetes DNS 服务发现验证 ==="
echo

# 测试 Service DNS 记录
echo "1. Service DNS 记录测试:"
SERVICES=$(kubectl get svc --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}')

for svc in $SERVICES; do
  ns=$(echo $svc | cut -d'/' -f1)
  name=$(echo $svc | cut -d'/' -f2)
  
  echo "  测试 $svc:"
  
  # A 记录查询
  A_RECORD=$(nslookup $name.$ns.svc.cluster.local 2>/dev/null | grep "Address:" | tail -1 | awk '{print $2}')
  if [ ! -z "$A_RECORD" ]; then
    echo "    A记录: ✅ $A_RECORD"
  else
    echo "    A记录: ❌ 未找到"
  fi
  
  # SRV 记录查询
  SRV_RECORD=$(nslookup -type=SRV _$name._tcp.$ns.svc.cluster.local 2>/dev/null | grep "SRV" | head -1)
  if [ ! -z "$SRV_RECORD" ]; then
    echo "    SRV记录: ✅ $SRV_RECORD"
  else
    echo "    SRV记录: ⚠️  无端口信息"
  fi
done

# 测试 Headless Service
echo -e "\n2. Headless Service 测试:"
HEADLESS_SVCS=$(kubectl get svc --all-namespaces -o jsonpath='{range .items[?(@.spec.clusterIP=="None")]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}')

for svc in $HEADLESS_SVCS; do
  ns=$(echo $svc | cut -d'/' -f1)
  name=$(echo $svc | cut -d'/' -f2)
  
  echo "  测试 Headless Service $svc:"
  POD_RECORDS=$(nslookup $name.$ns.svc.cluster.local 2>/dev/null | grep "Address:" | grep -v "#53" | wc -l)
  echo "    解析到 $POD_RECORDS 个 Pod IP"
done
```

### 3.2 Pod DNS 记录

```yaml
# Pod DNS 配置验证
apiVersion: v1
kind: Pod
metadata:
  name: dns-test-pod
  namespace: default
  labels:
    app: dns-test
spec:
  containers:
  - name: test
    image: busybox:1.35
    command: ["sleep", "3600"]
    env:
    - name: POD_NAME
      valueFrom:
        fieldRef:
          fieldPath: metadata.name
    - name: POD_NAMESPACE
      valueFrom:
        fieldRef:
          fieldPath: metadata.namespace
    - name: POD_IP
      valueFrom:
        fieldRef:
          fieldPath: status.podIP
  dnsConfig:
    options:
    - name: ndots
      value: "5"
    - name: timeout
      value: "2"
    - name: attempts
      value: "3"
```

```bash
# Pod DNS 测试脚本
kubectl exec -it dns-test-pod -- sh -c '
echo "=== Pod DNS 测试 ==="
echo

# 测试各种 DNS 查询格式
echo "1. 基本服务查询:"
nslookup kubernetes.default

echo -e "\n2. 完整域名查询:"
nslookup kubernetes.default.svc.cluster.local

echo -e "\n3. 跨命名空间查询:"
nslookup kube-dns.kube-system.svc.cluster.local

echo -e "\n4. Pod DNS 查询:"
# 获取其他 Pod 的 DNS 名称
OTHER_POD=$(kubectl get pods -n default -l app!=dns-test -o jsonpath="{.items[0].metadata.name}")
if [ ! -z "$OTHER_POD" ]; then
  nslookup $OTHER_POD.default.pod.cluster.local
fi

echo -e "\n5. 反向 DNS 查询:"
MY_IP=$(hostname -i)
nslookup $MY_IP
'
```

---

<!-- chunk: 4. 故障诊断与排错 -->
## 4. 故障诊断与排错

### 4.1 常见故障场景

```bash
#!/bin/bash
# CoreDNS 故障诊断脚本

echo "=== CoreDNS 故障诊断工具 ==="
echo

# 1. 基础健康检查
echo "1. CoreDNS 健康状态检查:"
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

echo -e "\n2. CoreDNS 日志分析:"
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50 | \
  grep -E "(ERROR|WARN|panic)" || echo "未发现明显错误"

# 3. DNS 解析测试
echo -e "\n3. DNS 解析连通性测试:"
kubectl run dns-debug --rm -it --image=busybox -- sh -c "
  echo '测试 Kubernetes 服务:'
  nslookup kubernetes.default 2>&1
  
  echo -e '\n测试外部域名:'
  nslookup google.com 2>&1
  
  echo -e '\n测试 CoreDNS 服务:'
  nslookup kube-dns.kube-system.svc.cluster.local 2>&1
"

# 4. 配置验证
echo -e "\n4. Corefile 配置验证:"
kubectl get configmap coredns -n kube-system -o yaml | \
  yq '.data.Corefile' 2>/dev/null || \
  kubectl get configmap coredns -n kube-system -o jsonpath='{.data.Corefile}'

# 5. 性能指标检查
echo -e "\n5. CoreDNS 性能指标:"
if kubectl get pods -n monitoring -l app=prometheus >/dev/null 2>&1; then
  kubectl exec -n monitoring prometheus-0 -- curl -s http://coredns.kube-system:9153/metrics | \
    grep -E "(coredns_dns_request_duration_seconds|coredns_dns_responses_total|coredns_cache_hits_total)" | \
    head -10
else
  echo "未检测到 Prometheus 监控"
fi
```

### 4.2 性能瓶颈诊断

```yaml
# DNS 性能测试工具
apiVersion: v1
kind: Pod
metadata:
  name: dns-perf-test
  namespace: default
spec:
  containers:
  - name: dnsperf
    image: quay.io/sscaling/dnsperf:latest
    command: ["dnsperf"]
    args:
    - "-s"
    - "10.96.0.10"  # CoreDNS Service IP
    - "-d"
    - "/queries.txt"
    - "-l"
    - "60"
    - "-Q"
    - "1000"
    volumeMounts:
    - name: queries
      mountPath: /queries.txt
      subPath: queries.txt
  volumes:
  - name: queries
    configMap:
      name: dns-queries
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: dns-queries
  namespace: default
data:
  queries.txt: |
    kubernetes.default A
    kube-dns.kube-system.svc.cluster.local A
    google.com A
    github.com A
    www.baidu.com A
```

### 4.3 故障排除清单

```markdown
<!-- chunk: CoreDNS 故障排除 checklist -->
## CoreDNS 故障排除 checklist

### 🔍 基础检查
- [ ] CoreDNS Pod 状态正常
- [ ] CoreDNS Service 配置正确
- [ ] kubelet DNS 配置指向正确
- [ ] NodeLocal DNSCache 部署状态

### 📊 性能检查
- [ ] DNS 查询延迟 < 50ms
- [ ] 缓存命中率 > 90%
- [ ] CPU/Memory 使用率正常
- [ ] 连接数未达到上限

### 🔧 配置检查
- [ ] Corefile 语法正确
- [ ] 插件配置符合需求
- [ ] 上游 DNS 服务器可达
- [ ] 网络策略允许 DNS 流量

### 🛡️ 安全检查
- [ ] DNSSEC 配置正确
- [ ] 访问控制策略生效
- [ ] 日志记录完整
- [ ] 监控告警配置完善
```

---

<!-- chunk: 5. 监控与性能分析 -->
## 5. 监控与性能分析

### 5.1 Prometheus 监控配置

```yaml
# CoreDNS 监控配置
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: coredns
  namespace: monitoring
  labels:
    app: coredns
spec:
  jobLabel: k8s-app
  selector:
    matchLabels:
      k8s-app: kube-dns
  namespaceSelector:
    matchNames:
    - kube-system
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
    relabelings:
    - sourceLabels: [__meta_kubernetes_pod_name]
      targetLabel: pod
    - sourceLabels: [__meta_kubernetes_namespace]
      targetLabel: namespace

---
# CoreDNS Grafana Dashboard
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns-dashboard
  namespace: monitoring
data:
  coredns-dashboard.json: |
    {
      "dashboard": {
        "title": "CoreDNS Monitoring",
        "panels": [
          {
            "title": "DNS 查询率",
            "type": "graph",
            "targets": [
              {
                "expr": "rate(coredns_dns_requests_total[5m])",
                "legendFormat": "{{type}}"
              }
            ]
          },
          {
            "title": "DNS 响应时间 P99",
            "type": "graph",
            "targets": [
              {
                "expr": "histogram_quantile(0.99, sum(rate(coredns_dns_request_duration_seconds_bucket[5m])) by (le))",
                "legendFormat": "P99 延迟"
              }
            ]
          },
          {
            "title": "缓存命中率",
            "type": "graph",
            "targets": [
              {
                "expr": "rate(coredns_cache_hits_total[5m]) / (rate(coredns_cache_hits_total[5m]) + rate(coredns_cache_misses_total[5m]))",
                "legendFormat": "缓存命中率"
              }
            ]
          },
          {
            "title": "错误率",
            "type": "graph",
            "targets": [
              {
                "expr": "rate(coredns_dns_responses_total{rcode!=\"NOERROR\"}[5m])",
                "legendFormat": "{{rcode}}"
              }
            ]
          }
        ]
      }
    }
```

### 5.2 关键指标监控

```bash
#!/bin/bash
# CoreDNS 关键指标监控脚本

echo "=== CoreDNS 关键性能指标 ==="
echo

# 获取 CoreDNS 指标
METRICS_URL="http://coredns.kube-system:9153/metrics"

echo "1. 查询统计:"
curl -s $METRICS_URL | grep coredns_dns_requests_total | head -5

echo -e "\n2. 响应统计:"
curl -s $METRICS_URL | grep coredns_dns_responses_total | head -5

echo -e "\n3. 缓存统计:"
curl -s $METRICS_URL | grep coredns_cache_ | head -10

echo -e "\n4. 延迟统计:"
curl -s $METRICS_URL | grep coredns_dns_request_duration_seconds_sum | head -3

# 计算关键比率
echo -e "\n5. 性能比率计算:"
TOTAL_REQUESTS=$(curl -s $METRICS_URL | grep 'coredns_dns_requests_total' | awk '{sum+=$2} END {print sum}')
CACHE_HITS=$(curl -s $METRICS_URL | grep 'coredns_cache_hits_total' | awk '{sum+=$2} END {print sum}')
ERROR_RESPONSES=$(curl -s $METRICS_URL | grep 'coredns_dns_responses_total{rcode!="NOERROR"}' | awk '{sum+=$2} END {print sum}')

if [ $TOTAL_REQUESTS -gt 0 ]; then
  CACHE_HIT_RATE=$(echo "scale=2; $CACHE_HITS * 100 / $TOTAL_REQUESTS" | bc)
  ERROR_RATE=$(echo "scale=2; $ERROR_RESPONSES * 100 / $TOTAL_REQUESTS" | bc)
  
  echo "  缓存命中率: ${CACHE_HIT_RATE}%"
  echo "  错误率: ${ERROR_RATE}%"
fi
```

---

<!-- chunk: 6. 安全加固配置 -->
## 6. 安全加固配置

### 6.1 DNS 安全策略

```yaml
# DNS 安全加固配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns-security
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        # 安全日志记录
        log . {
            class error
            class denial
        }
        
        # 访问控制
        acl {
            allow net 10.0.0.0/8
            allow net 172.16.0.0/12
            allow net 192.168.0.0/16
            block
        }
        
        errors
        health
        
        # 限制递归查询
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
            ttl 30
        }
        
        # 安全的上游转发
        forward . 8.8.8.8 8.8.4.4 {
            max_fails 3
            tls_servername dns.google
            health_check 5s
        }
        
        # 防止 DNS 放大攻击
        bufsize 512
        
        cache 30
        loop
        reload
        loadbalance
    }

---
# NetworkPolicy 限制 DNS 访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: restrict-dns-access
  namespace: kube-system
spec:
  podSelector:
    matchLabels:
      k8s-app: kube-dns
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector: {}
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
  - from:
    - podSelector:
        matchLabels:
          k8s-app: nodelocaldns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
```

### 6.2 DNSSEC 配置

```yaml
# DNSSEC 启用配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns-dnssec
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health
        
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
            ttl 30
        }
        
        # 启用 DNSSEC 验证
        forward . 8.8.8.8 8.8.4.4 {
            tls_servername dns.google
            except cluster.local
            health_check 5s
        }
        
        # 本地权威区域启用 DNSSEC
        file /etc/coredns/db.cluster.local cluster.local {
            transfer to *
            reload
        }
        
        cache 30
        loop
        reload
        loadbalance
    }
```

---

<!-- chunk: 7. 多集群 DNS 管理 -->
## 7. 多集群 DNS 管理

### 7.1 跨集群 DNS 配置

```yaml
# 多集群 DNS 联邦配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: multicloud-dns
  namespace: kube-system
data:
  Corefile: |
    # 本地集群配置
    .:53 {
        errors
        health
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
        }
        prometheus :9153
        forward . /etc/resolv.conf
        cache 30
        loop
        reload
        loadbalance
    }
    
    # 远程集群联邦配置
    cluster-east.example.com:53 {
        errors
        cache 300
        forward . 10.10.0.10 10.10.0.11 {
            health_check 5s
        }
    }
    
    cluster-west.example.com:53 {
        errors
        cache 300
        forward . 10.20.0.10 10.20.0.11 {
            health_check 5s
        }
    }
```

### 7.2 ExternalDNS 集成

```yaml
# ExternalDNS 部署配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: external-dns
  namespace: kube-system
spec:
  strategy:
    type: Recreate
  selector:
    matchLabels:
      app: external-dns
  template:
    metadata:
      labels:
        app: external-dns
    spec:
      serviceAccountName: external-dns
      containers:
      - name: external-dns
        image: registry.k8s.io/external-dns/external-dns:v0.13.4
        args:
        - --source=service
        - --source=ingress
        - --provider=aws
        - --aws-zone-type=public
        - --registry=txt
        - --txt-owner-id=cluster-identifier
        - --domain-filter=example.com
        - --policy=sync
        - --events
        resources:
          requests:
            memory: 50Mi
            cpu: 10m
          limits:
            memory: 50Mi
            cpu: 100m
```

---

<!-- chunk: 8. 生产环境最佳实践 -->
## 8. 生产环境最佳实践

### 8.1 部署检查清单

```yaml
# CoreDNS 生产部署检查清单
apiVersion: batch/v1
kind: Job
metadata:
  name: coredns-deployment-check
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: validator
        image: busybox:1.35
        command:
        - /bin/sh
        - -c
        - |
          echo "=== CoreDNS 生产部署验证 ==="
          
          # 1. 基础连通性测试
          echo "1. DNS 基础连通性测试:"
          nslookup kubernetes.default || exit 1
          nslookup google.com || exit 1
          
          # 2. 性能测试
          echo "2. DNS 性能测试:"
          for i in $(seq 1 100); do
            nslookup kubernetes.default >/dev/null 2>&1
          done
          
          # 3. 配置验证
          echo "3. 配置验证:"
          dig @10.96.0.10 kubernetes.default | grep -q "ANSWER SECTION" || exit 1
          
          # 4. 缓存测试
          echo "4. 缓存效果测试:"
          time nslookup kubernetes.default
          time nslookup kubernetes.default  # 第二次应该更快
          
          echo "✅ 所有验证通过"
        env:
        - name: DNS_SERVER
          value: "10.96.0.10"
      restartPolicy: Never
```

### 8.2 滚动更新策略

```yaml
# CoreDNS 安全滚动更新配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: coredns-canary
  namespace: kube-system
spec:
  replicas: 1
  selector:
    matchLabels:
      k8s-app: coredns-canary
  template:
    metadata:
      labels:
        k8s-app: coredns-canary
    spec:
      containers:
      - name: coredns
        image: registry.k8s.io/coredns/coredns:v1.11.1
        args: [ "-conf", "/etc/coredns/Corefile" ]
        ports:
        - containerPort: 53
          name: dns
          protocol: UDP
        readinessProbe:
          httpGet:
            path: /ready
            port: 8181
          initialDelaySeconds: 5
          periodSeconds: 2
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxUnavailable: 0
      maxSurge: 1
```

通过以上全面的 CoreDNS 配置和优化方案，可以确保 Kubernetes 集群的 DNS 服务具备高性能、高可用和高安全性。

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-03-networking-traffic/MOC.md|domain-03-networking-traffic MOC]]
- [[domain-03-networking-traffic/README.md|Domain 5: Networking 网络]]
- [[domain-03-networking-traffic/00-network-in-nutshell.md|Kubernetes 网络基础 Network in a Nutshell]]
- [[domain-03-networking-traffic/00-open-source-projects-index.md|Domain-5 网络 — 开源项目索引]]
- [[domain-03-networking-traffic/01-network-architecture-overview-faq.md|FAQ 文档]]
- [[domain-03-networking-traffic/01-network-architecture-overview.md|网络核心组件]]
- [[domain-03-networking-traffic/02-cni-architecture-fundamentals.md|CNI 架构与核心原理]]
- [[domain-03-networking-traffic/03-cni-plugins-comparison.md|76 - CNI插件深度对比]]
- [[domain-03-networking-traffic/04-flannel-complete-guide.md|142 - Flannel 完整指南 (Flannel Complete Guide)]]
- [[domain-03-networking-traffic/04a-flannel-wireguard-backend.md|Flannel WireGuard 加密后端配置]]
- [[domain-03-networking-traffic/04b-flannel-ipv6-dual-stack.md|Flannel IPv6 Dual Stack 支持]]
- [[domain-03-networking-traffic/04c-flannel-windows-support.md|Flannel Windows 节点支持]]

## See Also

- [[domain-03-networking-traffic/09-kube-proxy-modes-performance.md|09-kube-proxy-modes-performance]]
- [[domain-03-networking-traffic/10-service-advanced-features.md|10-service-advanced-features]]
- [[domain-03-networking-traffic/12-dns-service-discovery.md|12-dns-service-discovery]]
- [[domain-03-networking-traffic/13-coredns-architecture-principles.md|13-coredns-architecture-principles]]

## Related

- [[domain-19-landscape-references/topic-index/network-index|Network 网络知识图谱索引]]
- [[domain-19-landscape-references/topic-index/dns-index|DNS 知识图谱索引]]
