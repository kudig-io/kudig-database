# CoreDNS ACK 实战技术指南

> **适用环境**: 阿里云专有云、公共云 ACK 集群  
> **目标读者**: DevOps 工程师、平台运维工程师  
> **文档版本**: v1.0 | 2026年1月  

---

## 一、生产级部署配置模板

### 1.1 标准 CoreDNS Deployment 配置

```yaml
# coredns-deployment.yaml
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
      maxSurge: 1
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
        - key: "node-role.kubernetes.io/master"
          effect: "NoSchedule"
      
      # 反亲和性配置
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
      
      # 安全上下文
      securityContext:
        runAsNonRoot: true
        runAsUser: 1000
        fsGroup: 1000
        seccompProfile:
          type: RuntimeDefault
      
      containers:
      - name: coredns
        image: coredns/coredns:1.11.1
        imagePullPolicy: IfNotPresent
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 200m
            memory: 256Mi
        
        # 性能优化参数
        env:
        - name: GOGC
          value: "20"
        - name: GOMAXPROCS
          value: "2"
        
        # 安全配置
        securityContext:
          readOnlyRootFilesystem: true
          allowPrivilegeEscalation: false
          capabilities:
            drop:
            - ALL
            add:
            - NET_BIND_SERVICE
          privileged: false
        
        args: [ "-conf", "/etc/coredns/Corefile" ]
        volumeMounts:
        - name: config-volume
          mountPath: /etc/coredns
          readOnly: true
        - name: tmp
          mountPath: /tmp
        
        # 健康检查
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
            periodSeconds: 10
            timeoutSeconds: 5
            successThreshold: 1
            failureThreshold: 3
        
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

### 1.2 CoreDNS Service 配置

```yaml
# coredns-service.yaml
apiVersion: v1
kind: Service
metadata:
  name: kube-dns
  namespace: kube-system
  labels:
    k8s-app: kube-dns
    kubernetes.io/cluster-service: "true"
    kubernetes.io/name: "CoreDNS"
  annotations:
    service.alpha.kubernetes.io/tolerate-unready-endpoints: "true"
spec:
  selector:
    k8s-app: kube-dns
  clusterIP: 10.96.0.10
  ports:
  - name: dns
    port: 53
    protocol: UDP
    targetPort: 53
  - name: dns-tcp
    port: 53
    protocol: TCP
    targetPort: 53
  - name: metrics
    port: 9153
    protocol: TCP
    targetPort: 9153
```

### 1.3 RBAC 配置

```yaml
# coredns-rbac.yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: coredns
  namespace: kube-system
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  labels:
    kubernetes.io/bootstrapping: rbac-defaults
  name: system:coredns
rules:
- apiGroups:
  - ""
  resources:
  - endpoints
  - services
  - pods
  - namespaces
  verbs:
  - list
  - watch
- apiGroups:
  - discovery.k8s.io
  resources:
  - endpointslices
  verbs:
  - list
  - watch
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  annotations:
    rbac.authorization.kubernetes.io/autoupdate: "true"
  labels:
    kubernetes.io/bootstrapping: rbac-defaults
  name: system:coredns
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: system:coredns
subjects:
- kind: ServiceAccount
  name: coredns
  namespace: kube-system
```

## 二、阿里云 ACK 环境优化配置

### 2.1 ACK 标准 Corefile 配置

```yaml
# coredns-configmap.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
  annotations:
    config.version: "1.0.0"
    config.updated-at: "2026-01-15T10:00:00Z"
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
        forward . 223.5.5.5 223.6.6.6 {
            max_concurrent 1000
            max_fails 3
            health_check 5s
            policy round_robin
            prefer_udp
        }
        cache 30 {
            success 10000 3600 300
            denial 1000 60 30
            prefetch 10 1h 10%
            serve_stale 1h
        }
        loop
        reload
        loadbalance round_robin
    }

    # 阿里云 PrivateZone 集成
    internal.company.local:53 {
        errors
        cache 60
        forward . 100.100.2.136 100.100.2.138 {
            max_concurrent 100
            health_check 10s
            policy round_robin
        }
    }
```

### 2.2 NodeLocal DNSCache 部署

```yaml
# nodelocaldns-daemonset.yaml
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
      - operator: Exists
      containers:
      - name: node-cache
        image: registry.k8s.io/dns/k8s-dns-node-cache:1.22.13
        resources:
          requests:
            cpu: 25m
            memory: 50Mi
          limits:
            cpu: 100m
            memory: 100Mi
        args:
        - --localip=169.254.20.10
        - --conf=/etc/Corefile
        - --upstreamsvc=kube-dns
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
      volumes:
      - name: config-volume
        configMap:
          name: node-local-dns-config
      - name: kube-dns-config
        configMap:
          name: kube-dns
          optional: true
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: node-local-dns-config
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        cache {
            success 9984 30
            denial 9984 5
            prefetch 10 1m 10%
        }
        forward . __PILLAR__CLUSTER__DNS__ {
            force_tcp
            prefer_udp
            max_concurrent 100
            health_check 5s
        }
        prometheus :9253
        loop
        reload
        loadbalance
    }
    
    # 阿里云特定优化
    aliyuncs.com:53 {
        forward . 223.5.5.5 223.6.6.6
        cache 300
    }
```

## 三、监控告警配置

### 3.1 PrometheusRule 配置

```yaml
# coredns-prometheus-rules.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: coredns-rules
  namespace: monitoring
spec:
  groups:
  - name: coredns.rules
    rules:
    # DNS高延迟告警
    - alert: CoreDNSHighLatency
      expr: |
        histogram_quantile(0.99, 
          sum(rate(coredns_dns_request_duration_seconds_bucket[5m])) by (le, server)
        ) > 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "CoreDNS请求延迟过高 (instance {{ $labels.instance }})"
        description: "P99延迟: {{ $value | printf \"%.3f\" }}s"

    # DNS错误率告警
    - alert: CoreDNSErrorsHigh
      expr: |
        sum(rate(coredns_dns_responses_total{rcode="SERVFAIL"}[5m])) 
        / 
        sum(rate(coredns_dns_responses_total[5m])) > 0.01
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "CoreDNS错误率过高 (instance {{ $labels.instance }})"
        description: "SERVFAIL错误率: {{ $value | printf \"%.2f\" }}%"

    # 缓存命中率低告警
    - alert: CoreDNSCacheHitRateLow
      expr: |
        sum(rate(coredns_cache_hits_total[5m])) 
        / 
        (sum(rate(coredns_cache_hits_total[5m])) + sum(rate(coredns_cache_misses_total[5m]))) 
        < 0.5
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "CoreDNS缓存命中率过低 (instance {{ $labels.instance }})"
        description: "缓存命中率: {{ $value | printf \"%.2f\" }}%"

    # CoreDNS实例宕机
    - alert: CoreDNSDown
      expr: up{job="coredns"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "CoreDNS实例不可用 (instance {{ $labels.instance }})"
        description: "CoreDNS实例已下线超过2分钟"

    # 转发错误告警
    - alert: CoreDNSForwardErrors
      expr: |
        sum(rate(coredns_forward_responses_total{rcode="SERVFAIL"}[5m])) > 10
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "CoreDNS转发错误增加 (instance {{ $labels.instance }})"
        description: "上游DNS转发SERVFAIL错误较多"
```

### 3.2 ServiceMonitor 配置

```yaml
# coredns-servicemonitor.yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: coredns
  namespace: monitoring
  labels:
    app: coredns
spec:
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
      targetLabel: instance
    - sourceLabels: [__meta_kubernetes_pod_label_k8s_app]
      targetLabel: job
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: (coredns_.+)
      targetLabel: __name__
```

## 四、自动化运维脚本

### 4.1 CoreDNS 健康检查脚本

```bash
#!/bin/bash
# coredns-health-check.sh

set -e

NAMESPACE="kube-system"
COREDNS_LABEL="k8s-app=kube-dns"

echo "=== CoreDNS 健康检查 ==="
echo "检查时间: $(date)"

# 1. 检查Pod状态
echo "1. 检查CoreDNS Pod状态..."
POD_STATUS=$(kubectl get pods -n ${NAMESPACE} -l ${COREDNS_LABEL} -o jsonpath='{range .items[*]}{.metadata.name}: {.status.phase}{"\n"}{end}')
echo "${POD_STATUS}"

# 2. 检查Service状态
echo "2. 检查CoreDNS Service..."
SERVICE_STATUS=$(kubectl get svc -n ${NAMESPACE} kube-dns -o wide)
echo "${SERVICE_STATUS}"

# 3. DNS解析测试
echo "3. DNS解析测试..."
TEST_RESULT=$(kubectl run dns-test-$(date +%s) --rm -it --image=busybox:1.36 \
  --restart=Never --timeout=30s -- \
  nslookup kubernetes.default 2>&1 || echo "DNS测试失败")

if [[ $TEST_RESULT == *"Address"* ]]; then
    echo "✅ DNS解析正常"
else
    echo "❌ DNS解析失败"
    echo "详细信息: ${TEST_RESULT}"
fi

# 4. 检查配置
echo "4. 检查CoreDNS配置..."
CONFIG_CHECK=$(kubectl get configmap coredns -n ${NAMESPACE} -o jsonpath='{.data.Corefile}' | head -10)
echo "Corefile头部:"
echo "${CONFIG_CHECK}"

# 5. 性能指标检查
echo "5. 性能指标检查..."
METRICS=$(kubectl exec -n ${NAMESPACE} deploy/coredns -- \
  wget -qO- http://localhost:9153/metrics 2>/dev/null | \
  grep -E "coredns_dns_requests_total|coredns_cache_hits_total" | head -5)

if [ -n "$METRICS" ]; then
    echo "✅ 能够获取监控指标"
    echo "${METRICS}"
else
    echo "⚠️  无法获取监控指标"
fi

echo "=== 健康检查完成 ==="
```

### 4.2 CoreDNS 性能基准测试脚本

```bash
#!/bin/bash
# coredns-performance-benchmark.sh

set -e

TEST_DURATION=${1:-60}  # 测试持续时间，默认60秒
CONCURRENT_QUERIES=${2:-10}  # 并发查询数，默认10

echo "=== CoreDNS 性能基准测试 ==="
echo "测试时长: ${TEST_DURATION}秒"
echo "并发查询: ${CONCURRENT_QUERIES}个"

# 创建测试Pod
TEST_POD="dns-bench-$(date +%s)"
echo "创建测试环境..."

kubectl run ${TEST_POD} --image=nicolaka/netshoot --restart=Never -- \
  sleep 3600 > /dev/null 2>&1

# 等待Pod就绪
kubectl wait --for=condition=Ready pod/${TEST_POD} --timeout=60s

# 执行性能测试
echo "开始DNS性能测试..."

RESULTS=$(kubectl exec ${TEST_POD} -- \
  bash -c "
    echo '测试开始时间: \$(date)'
    
    # 并发DNS查询测试
    for i in \$(seq 1 ${CONCURRENT_QUERIES}); do
      (
        for j in \$(seq 1 \$(( ${TEST_DURATION} / ${CONCURRENT_QUERIES} ))); do
          start_time=\$(date +%s.%N)
          dig @10.96.0.10 kubernetes.default.svc.cluster.local +short > /dev/null 2>&1
          end_time=\$(date +%s.%N)
          echo \"\$end_time - \$start_time\" | bc -l
        done
      ) &
    done
    
    wait
    
    echo '测试结束时间: \$(date)'
  ")

echo "测试结果:"
echo "${RESULTS}"

# 清理测试Pod
kubectl delete pod ${TEST_POD} --force --grace-period=0

echo "=== 性能测试完成 ==="
```

### 4.3 CoreDNS 配置备份脚本

```bash
#!/bin/bash
# coredns-config-backup.sh

BACKUP_DIR="/backup/coredns"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
NAMESPACE="kube-system"

mkdir -p ${BACKUP_DIR}

echo "=== CoreDNS 配置备份 ==="
echo "备份时间: $(date)"
echo "备份目录: ${BACKUP_DIR}"

# 备份Deployment
echo "1. 备份Deployment配置..."
kubectl get deployment coredns -n ${NAMESPACE} -o yaml > \
  ${BACKUP_DIR}/coredns-deployment-${TIMESTAMP}.yaml

# 备份Service
echo "2. 备份Service配置..."
kubectl get service kube-dns -n ${NAMESPACE} -o yaml > \
  ${BACKUP_DIR}/coredns-service-${TIMESTAMP}.yaml

# 备份ConfigMap
echo "3. 备份ConfigMap配置..."
kubectl get configmap coredns -n ${NAMESPACE} -o yaml > \
  ${BACKUP_DIR}/coredns-configmap-${TIMESTAMP}.yaml

# 备份RBAC
echo "4. 备份RBAC配置..."
kubectl get serviceaccount coredns -n ${NAMESPACE} -o yaml > \
  ${BACKUP_DIR}/coredns-sa-${TIMESTAMP}.yaml

kubectl get clusterrole system:coredns -o yaml > \
  ${BACKUP_DIR}/coredns-clusterrole-${TIMESTAMP}.yaml

kubectl get clusterrolebinding system:coredns -o yaml > \
  ${BACKUP_DIR}/coredns-crb-${TIMESTAMP}.yaml

# 创建版本信息文件
cat > ${BACKUP_DIR}/VERSION-${TIMESTAMP} << EOF
CoreDNS Backup Version Info
===========================
Backup Time: $(date)
Kubernetes Version: $(kubectl version --short | grep Server | awk '{print $3}')
CoreDNS Image: $(kubectl get deployment coredns -n ${NAMESPACE} -o jsonpath='{.spec.template.spec.containers[0].image}')
Node Count: $(kubectl get nodes --no-headers | wc -l)
EOF

echo "备份完成!"
echo "备份文件列表:"
ls -la ${BACKUP_DIR}/coredns-*${TIMESTAMP}*
```

## 五、故障排除手册

### 5.1 常见问题诊断矩阵

| 问题现象 | 可能原因 | 诊断命令 | 解决方案 |
|----------|----------|----------|----------|
| DNS解析超时 | CoreDNS Pod异常 | `kubectl get pods -n kube-system -l k8s-app=kube-dns` | 重启Pod或检查资源 |
| NXDOMAIN错误 | Service不存在 | `kubectl get svc <service-name>` | 创建缺失的Service |
| SERVFAIL错误 | Corefile配置错误 | `kubectl logs -n kube-system -l k8s-app=kube-dns` | 修正Corefile语法 |
| 缓存命中率低 | TTL设置过短 | `kubectl exec -n kube-system deploy/coredns -- wget -qO- http://localhost:9153/metrics` | 调整cache配置 |
| 上游DNS不可达 | 网络策略阻断 | `kubectl exec <pod> -- nc -zv 223.5.5.5 53` | 检查NetworkPolicy |

### 5.2 紧急恢复流程

```bash
#!/bin/bash
# coredns-emergency-recovery.sh

echo "=== CoreDNS 紧急恢复 ==="

# 1. 检查当前状态
echo "1. 检查当前CoreDNS状态..."
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 2. 如果Pod异常，尝试重启
echo "2. 重启CoreDNS Deployment..."
kubectl rollout restart deployment/coredns -n kube-system

# 3. 等待恢复
echo "3. 等待Pod恢复..."
kubectl rollout status deployment/coredns -n kube-system --timeout=300s

# 4. 验证恢复
echo "4. 验证DNS功能..."
kubectl run recovery-test --rm -it --image=busybox:1.36 \
  -- nslookup kubernetes.default

echo "紧急恢复流程完成!"
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)