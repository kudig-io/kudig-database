# Terway ACK 实战技术指南

> **适用环境**: 阿里云专有云、公共云 ACK 集群  
> **目标读者**: DevOps 工程师、网络架构师、平台运维工程师  
> **文档版本**: v1.0 | 2026年1月  

---

## 一、生产级部署配置模板

### 1.1 标准 Terway DaemonSet 配置

```yaml
# terway-daemonset.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: terway-eniip
  namespace: kube-system
  labels:
    app: terway
    component: eniip
spec:
  selector:
    matchLabels:
      app: terway
      component: eniip
  template:
    metadata:
      labels:
        app: terway
        component: eniip
    spec:
      priorityClassName: system-node-critical
      serviceAccountName: terway
      hostNetwork: true
      dnsPolicy: ClusterFirstWithHostNet
      tolerations:
        - operator: Exists
          effect: NoSchedule
        - operator: Exists
          effect: NoExecute
      
      initContainers:
      - name: terway-init
        image: registry.cn-hangzhou.aliyuncs.com/acs/terway:v1.12.0
        command:
        - /bin/sh
        - -c
        - |
          sysctl -w net.ipv4.conf.all.rp_filter=0
          sysctl -w net.ipv4.conf.eth0.rp_filter=0
          modprobe ip_vs
          modprobe ip_vs_rr
          modprobe ip_vs_wrr
          modprobe ip_vs_sh
        securityContext:
          privileged: true
        volumeMounts:
        - name: host-proc
          mountPath: /proc
        - name: lib-modules
          mountPath: /lib/modules
          readOnly: true
      
      containers:
      - name: terway
        image: registry.cn-hangzhou.aliyuncs.com/acs/terway:v1.12.0
        imagePullPolicy: IfNotPresent
        command:
        - /usr/bin/terwayd
        - --config-path=/etc/eni/eni_conf
        - --daemon-mode=ENIIP
        - --log-level=info
        resources:
          requests:
            cpu: 100m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 1Gi
        securityContext:
          privileged: true
        env:
        - name: NODE_NAME
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
        - name: POD_NAMESPACE
          valueFrom:
            fieldRef:
              fieldPath: metadata.namespace
        - name: POD_NAME
          valueFrom:
            fieldRef:
              fieldPath: metadata.name
        - name: POD_IP
          valueFrom:
            fieldRef:
              fieldPath: status.podIP
        volumeMounts:
        - name: config
          mountPath: /etc/eni
          readOnly: true
        - name: host-proc
          mountPath: /proc
        - name: lib-modules
          mountPath: /lib/modules
          readOnly: true
        - name: cni-bin
          mountPath: /opt/cni/bin
        - name: cni-netd
          mountPath: /etc/cni/net.d
        - name: log
          mountPath: /var/log
        - name: run
          mountPath: /var/run
        - name: token
          mountPath: /var/addon
          readOnly: true
      
      volumes:
      - name: config
        configMap:
          name: eni-config
      - name: host-proc
        hostPath:
          path: /proc
      - name: lib-modules
        hostPath:
          path: /lib/modules
      - name: cni-bin
        hostPath:
          path: /opt/cni/bin
      - name: cni-netd
        hostPath:
          path: /etc/cni/net.d
      - name: log
        hostPath:
          path: /var/log
      - name: run
        hostPath:
          path: /var/run
      - name: token
        projected:
          sources:
          - serviceAccountToken:
              path: token-config
              expirationSeconds: 3600
              audience: acs
```

### 1.2 Terway ConfigMap 标准配置

```yaml
# terway-configmap.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
  annotations:
    config.version: "1.0.0"
    config.updated-at: "2026-01-15T10:00:00Z"
data:
  eni_conf: |
    {
      "version": "1",
      "access_key": "",
      "access_secret": "",
      "credential_path": "/var/addon/token-config",
      "region": "cn-hangzhou",
      "cluster_id": "cluster-xxxxxx",
      "vswitches": {
        "cn-hangzhou-h": ["vsw-xxx1", "vsw-xxx2"],
        "cn-hangzhou-i": ["vsw-yyy1", "vsw-yyy2"]
      },
      "security_groups": ["sg-xxx"],
      "eni_tags": {
        "k8s.aliyun.com/cluster-id": "cluster-xxxxxx"
      },
      "eni_max_pool_size": 25,
      "eni_min_pool_size": 10,
      "eni_pool_monitor": true,
      "eni_pool_decrease_threshold": 5,
      "eni_pool_increase_threshold": 20,
      "service_cidr": "172.21.0.0/20",
      "daemon_mode": "ENIIP",
      "ip_family": "ipv4",
      "ipam_type": "eniip",
      "eniip_max_pool_size": 50,
      "eniip_min_pool_size": 15,
      "eniip_pool_monitor": true,
      "eniip_pool_decrease_threshold": 10,
      "eniip_pool_increase_threshold": 30,
      "hot_plug": true,
      "eni_selection_policy": "least-ip",
      "eni_allocation_timeout": 60,
      "eni_release_timeout": 300,
      "eni_max_eni": 0,
      "eni_max_ips_per_eni": 0,
      "eni_trunking": false,
      "eni_trunk_prefix": "trunk-",
      "eni_trunk_vlan_range": "100-200",
      "log_level": "info",
      "log_output": "stdout",
      "log_format": "json",
      "metrics_port": 6060,
      "health_check_port": 6061,
      "pprof_port": 6062,
      "enable_debug": false,
      "enable_prometheus": true,
      "enable_health_check": true,
      "enable_pprof": false,
      "enable_ipv6": false,
      "ipv6_mode": "disabled",
      "ipv6_subnet_id": "",
      "ipv6_address_count": 0,
      "ipv6_nat_gateway_id": "",
      "ipv6_route_table_id": "",
      "ipv6_vswitch_id": "",
      "ipv6_security_group_id": "",
      "ipv6_bandwidth_package_id": "",
      "ipv6_bandwidth": 0,
      "ipv6_internet_charge_type": "PayByTraffic",
      "ipv6_delete_with_instance": true
    }
```

### 1.3 RBAC 权限配置

```yaml
# terway-rbac.yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: terway
  namespace: kube-system
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: terway
rules:
- apiGroups: [""]
  resources: ["nodes", "nodes/status", "pods", "pods/status", "services", "endpoints"]
  verbs: ["get", "list", "watch", "patch", "update"]
- apiGroups: [""]
  resources: ["events"]
  verbs: ["create", "patch", "update"]
- apiGroups: ["networking.k8s.io"]
  resources: ["networkpolicies"]
  verbs: ["get", "list", "watch"]
- apiGroups: ["crd.projectcalico.org"]
  resources: ["networkpolicies", "clusterinformations", "hostendpoints", "ippools"]
  verbs: ["get", "list", "watch", "create", "update", "delete"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: terway
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: terway
subjects:
- kind: ServiceAccount
  name: terway
  namespace: kube-system
```

## 二、阿里云 ACK 环境优化配置

### 2.1 多可用区网络规划

```yaml
# multi-az-terway-config.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config-multi-az
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "region": "cn-hangzhou",
      "vswitches": {
        # 可用区H - 主要业务区域
        "cn-hangzhou-h": [
          "vsw-h-xxx1",  # 主交换机
          "vsw-h-xxx2"   # 备用交换机
        ],
        # 可用区I - 灾备区域
        "cn-hangzhou-i": [
          "vsw-i-yyy1",  # 主交换机
          "vsw-i-yyy2"   # 备用交换机
        ],
        # 可用区J - 扩展区域
        "cn-hangzhou-j": [
          "vsw-j-zzz1",
          "vsw-j-zzz2"
        ]
      },
      "security_groups": [
        "sg-business",     # 业务安全组
        "sg-monitoring"    # 监控安全组
      ],
      "eni_tags": {
        "Environment": "Production",
        "Team": "Platform",
        "Owner": "DevOps"
      },
      # 跨可用区负载均衡配置
      "eni_selection_policy": "round-robin",
      "eni_max_pool_size": 30,
      "eni_min_pool_size": 15,
      "eni_pool_monitor": true,
      "eni_pool_decrease_threshold": 8,
      "eni_pool_increase_threshold": 25
    }
```

### 2.2 高性能网络配置

```yaml
# high-performance-terway.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: terway-perf-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "region": "cn-hangzhou",
      "vswitches": {
        "cn-hangzhou-h": ["vsw-perf-xxx"]
      },
      "security_groups": ["sg-high-perf"],
      "daemon_mode": "ENI",
      "eni_max_pool_size": 5,
      "eni_min_pool_size": 2,
      "eni_pool_monitor": true,
      "eni_pool_decrease_threshold": 1,
      "eni_pool_increase_threshold": 4,
      "eni_selection_policy": "least-eni",
      "eni_allocation_timeout": 30,
      "eni_release_timeout": 120,
      "hot_plug": true,
      "eni_trunking": false,
      # 性能优化参数
      "eni_max_eni": 8,
      "eni_max_ips_per_eni": 20,
      "log_level": "warn",
      "enable_prometheus": true,
      "metrics_port": 6060
    }
```

### 2.3 大规模集群配置

```yaml
# large-scale-terway.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: terway-large-scale-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "region": "cn-hangzhou",
      "vswitches": {
        "cn-hangzhou-h": ["vsw-large-xxx1", "vsw-large-xxx2"],
        "cn-hangzhou-i": ["vsw-large-yyy1", "vsw-large-yyy2"]
      },
      "security_groups": ["sg-large-scale"],
      "daemon_mode": "ENIIP",
      "eni_max_pool_size": 50,
      "eni_min_pool_size": 20,
      "eni_pool_monitor": true,
      "eni_pool_decrease_threshold": 15,
      "eni_pool_increase_threshold": 40,
      "eni_selection_policy": "least-ip",
      "eni_allocation_timeout": 90,
      "eni_release_timeout": 600,
      "hot_plug": true,
      "eni_trunking": true,
      "eni_trunk_prefix": "trunk-large-",
      "eni_trunk_vlan_range": "1000-2000",
      # 大规模优化
      "eni_max_eni": 16,
      "eni_max_ips_per_eni": 50,
      "eniip_max_pool_size": 100,
      "eniip_min_pool_size": 30,
      "eniip_pool_monitor": true,
      "eniip_pool_decrease_threshold": 20,
      "eniip_pool_increase_threshold": 70,
      "log_level": "error",
      "enable_prometheus": true,
      "metrics_port": 6060,
      "enable_debug": false,
      "enable_health_check": true
    }
```

## 三、高级特性配置

### 3.1 固定 IP 配置 (StatefulSet)

```yaml
# statefulset-fixed-ip.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: database-cluster
  namespace: production
  annotations:
    # 启用 Terway 固定 IP 功能
    k8s.aliyun.com/pod-ip-fixed: "true"
spec:
  serviceName: database-cluster
  replicas: 3
  selector:
    matchLabels:
      app: database
  template:
    metadata:
      labels:
        app: database
    spec:
      containers:
      - name: mysql
        image: mysql:8.0
        ports:
        - containerPort: 3306
        env:
        - name: MYSQL_ROOT_PASSWORD
          value: "password123"
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: alicloud-disk-essd
      resources:
        requests:
          storage: 100Gi
---
# 对应的 Service 配置
apiVersion: v1
kind: Service
metadata:
  name: database-cluster
  namespace: production
spec:
  clusterIP: None
  selector:
    app: database
  ports:
  - port: 3306
    targetPort: 3306
```

### 3.2 NetworkPolicy 配置

```yaml
# networkpolicy-example.yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: backend-isolated
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend-api
  policyTypes:
  - Ingress
  - Egress
  ingress:
  # 允许前端服务访问
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
  # 允许监控组件访问
  - from:
    - namespaceSelector:
        matchLabels:
          name: monitoring
      podSelector:
        matchLabels:
          app: prometheus
    ports:
    - protocol: TCP
      port: 8080
  egress:
  # 允许访问数据库
  - to:
    - podSelector:
        matchLabels:
          app: database
    ports:
    - protocol: TCP
      port: 3306
  # 允许访问外部 API
  - to:
    - ipBlock:
        cidr: 0.0.0.0/0
        except:
        - 10.0.0.0/8
        - 172.16.0.0/12
        - 192.168.0.0/16
    ports:
    - protocol: TCP
      port: 443
---
# 默认拒绝策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
```

### 3.3 安全组集成配置

```yaml
# security-group-integration.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: terway-security-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "region": "cn-hangzhou",
      "vswitches": {
        "cn-hangzhou-h": ["vsw-xxx"]
      },
      # 多层级安全组配置
      "security_groups": [
        "sg-base",        # 基础安全组
        "sg-business",    # 业务安全组
        "sg-database"     # 数据库安全组
      ],
      "pod_security_group_mapping": {
        "namespace:production": "sg-business",
        "namespace:database": "sg-database",
        "label:app=frontend": "sg-frontend",
        "label:app=backend": "sg-backend",
        "label:app=database": "sg-database"
      },
      "default_security_group": "sg-base",
      "eni_max_pool_size": 25,
      "eni_min_pool_size": 10
    }
```

## 四、监控告警配置

### 4.1 Prometheus 监控配置

```yaml
# terway-servicemonitor.yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: terway
  namespace: monitoring
  labels:
    app: terway
spec:
  selector:
    matchLabels:
      app: terway
  namespaceSelector:
    matchNames:
    - kube-system
  endpoints:
  - port: metrics
    interval: 30s
    path: /metrics
    relabelings:
    - sourceLabels: [__meta_kubernetes_pod_node_name]
      targetLabel: node
    - sourceLabels: [__meta_kubernetes_pod_name]
      targetLabel: instance
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: (terway_.+)
      targetLabel: __name__
---
# terway-service.yaml (为监控暴露端点)
apiVersion: v1
kind: Service
metadata:
  name: terway-metrics
  namespace: kube-system
  labels:
    app: terway
spec:
  selector:
    app: terway
  ports:
  - name: metrics
    port: 6060
    targetPort: 6060
    protocol: TCP
```

### 4.2 关键告警规则

```yaml
# terway-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: terway-rules
  namespace: monitoring
spec:
  groups:
  - name: terway.rules
    rules:
    # ENI 池水位告警
    - alert: TerwayENIPoolLow
      expr: |
        terway_eni_pool_available / terway_eni_pool_total * 100 < 20
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Terway ENI 池水位过低 (instance {{ $labels.instance }})"
        description: "可用 ENI 数量占比: {{ $value | printf \"%.2f\" }}%"

    # IP 池水位告警
    - alert: TerwayIPPoolLow
      expr: |
        terway_ip_pool_available / terway_ip_pool_total * 100 < 15
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Terway IP 池水位严重不足 (instance {{ $labels.instance }})"
        description: "可用 IP 数量占比: {{ $value | printf \"%.2f\" }}%"

    # 网络分配失败率告警
    - alert: TerwayAllocationFailureRateHigh
      expr: |
        rate(terway_allocation_failures_total[5m]) / 
        rate(terway_allocation_attempts_total[5m]) > 0.1
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Terway 网络分配失败率过高 (instance {{ $labels.instance }})"
        description: "失败率: {{ $value | printf \"%.2f\" }}%"

    # 组件不可用告警
    - alert: TerwayComponentDown
      expr: up{job="terway"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Terway 组件不可用 (instance {{ $labels.instance }})"
        description: "Terway DaemonSet 组件已下线"

    # 网络延迟告警
    - alert: TerwayNetworkLatencyHigh
      expr: |
        histogram_quantile(0.99, 
          sum(rate(terway_network_operation_duration_seconds_bucket[5m])) by (le, operation)
        ) > 2
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Terway 网络操作延迟过高 (operation {{ $labels.operation }})"
        description: "P99 延迟: {{ $value | printf \"%.3f\" }}s"
```

## 五、自动化运维脚本

### 5.1 Terway 健康检查脚本

```bash
#!/bin/bash
# terway-health-check.sh

set -e

NAMESPACE="kube-system"
TERWAY_LABEL="app=terway"

echo "=== Terway 健康检查 ==="
echo "检查时间: $(date)"

# 1. 检查 DaemonSet 状态
echo "1. 检查 Terway DaemonSet 状态..."
DAEMONSET_STATUS=$(kubectl get daemonset terway-eniip -n ${NAMESPACE} -o jsonpath='{.status}')
DESIRED=$(echo ${DAEMONSET_STATUS} | jq -r '.desiredNumberScheduled')
READY=$(echo ${DAEMONSET_STATUS} | jq -r '.numberReady')
MISSING=$((DESIRED - READY))

if [ ${MISSING} -eq 0 ]; then
    echo "✅ DaemonSet 状态正常 (${READY}/${DESIRED})"
else
    echo "❌ DaemonSet 状态异常，缺失 ${MISSING} 个 Pod"
fi

# 2. 检查 Pod 状态
echo "2. 检查 Terway Pod 状态..."
kubectl get pods -n ${NAMESPACE} -l ${TERWAY_LABEL} -o wide

# 3. 检查节点网络配置
echo "3. 检查节点 ENI 配置..."
NODE_ENI_INFO=$(kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations.k8s\.aliyun\.com/allocated-eniips}{"\n"}{end}')
echo "${NODE_ENI_INFO}"

# 4. 检查 Terway 配置
echo "4. 检查 Terway 配置..."
CONFIG_CHECK=$(kubectl get configmap eni-config -n ${NAMESPACE} -o jsonpath='{.data.eni_conf}' | jq '.')
echo "配置摘要:"
echo "${CONFIG_CHECK}" | jq '{
  region: .region,
  daemon_mode: .daemon_mode,
  eni_max_pool_size: .eni_max_pool_size,
  eni_min_pool_size: .eni_min_pool_size
}'

# 5. 性能指标检查
echo "5. 性能指标检查..."
for POD in $(kubectl get pods -n ${NAMESPACE} -l ${TERWAY_LABEL} -o jsonpath='{.items[*].metadata.name}'); do
    echo "检查 Pod: ${POD}"
    METRICS=$(kubectl exec -n ${NAMESPACE} ${POD} -c terway -- curl -s http://localhost:6060/metrics 2>/dev/null | \
              grep -E "terway_eni_pool|terway_ip_pool|terway_allocation" | head -10)
    if [ -n "$METRICS" ]; then
        echo "✅ 能够获取监控指标"
        echo "${METRICS}"
    else
        echo "⚠️  无法获取监控指标"
    fi
    echo "---"
done

echo "=== 健康检查完成 ==="
```

### 5.2 Terway 网络诊断脚本

```bash
#!/bin/bash
# terway-network-diagnosis.sh

NODE_NAME=${1:-$(hostname)}

echo "=== Terway 网络诊断 ==="
echo "节点: ${NODE_NAME}"
echo "诊断时间: $(date)"

# 1. 基础网络信息
echo "1. 基础网络信息"
echo "节点 IP:"
hostname -I

echo "网络接口:"
ip addr show | grep -E "^[0-9]+:|inet "

# 2. Terway 组件检查
echo "2. Terway 组件检查"
echo "Terway 进程:"
ps aux | grep terwayd | grep -v grep

echo "Terway 端口监听:"
netstat -tlnp | grep -E "(6060|6061)"

# 3. CNI 配置检查
echo "3. CNI 配置检查"
if [ -f /etc/cni/net.d/10-terway.conf ]; then
    echo "✅ Terway CNI 配置文件存在"
    cat /etc/cni/net.d/10-terway.conf | jq .
else
    echo "❌ Terway CNI 配置文件不存在"
fi

# 4. 网络连通性测试
echo "4. 网络连通性测试"
echo "测试到 DNS 服务器:"
ping -c 3 100.100.2.136

echo "测试到 VPC 网关:"
ping -c 3 $(ip route | grep default | awk '{print $3}')

# 5. Pod 网络测试
echo "5. Pod 网络测试"
kubectl run network-test --rm -it --image=busybox:1.36 \
  --overrides='{"spec": {"nodeSelector": {"kubernetes.io/hostname": "'${NODE_NAME}'"}}}' \
  -- ping -c 4 8.8.8.8

echo "=== 网络诊断完成 ==="
```

### 5.3 Terway 配置备份脚本

```bash
#!/bin/bash
# terway-config-backup.sh

BACKUP_DIR="/backup/terway"
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
NAMESPACE="kube-system"

mkdir -p ${BACKUP_DIR}

echo "=== Terway 配置备份 ==="
echo "备份时间: $(date)"
echo "备份目录: ${BACKUP_DIR}"

# 备份 DaemonSet
echo "1. 备份 DaemonSet 配置..."
kubectl get daemonset terway-eniip -n ${NAMESPACE} -o yaml > \
  ${BACKUP_DIR}/terway-daemonset-${TIMESTAMP}.yaml

# 备份 ConfigMap
echo "2. 备份 ConfigMap 配置..."
kubectl get configmap eni-config -n ${NAMESPACE} -o yaml > \
  ${BACKUP_DIR}/terway-configmap-${TIMESTAMP}.yaml

# 备份 RBAC
echo "3. 备份 RBAC 配置..."
kubectl get serviceaccount terway -n ${NAMESPACE} -o yaml > \
  ${BACKUP_DIR}/terway-sa-${TIMESTAMP}.yaml

kubectl get clusterrole terway -o yaml > \
  ${BACKUP_DIR}/terway-clusterrole-${TIMESTAMP}.yaml

kubectl get clusterrolebinding terway -o yaml > \
  ${BACKUP_DIR}/terway-crb-${TIMESTAMP}.yaml

# 备份节点网络状态
echo "4. 备份节点网络状态..."
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations.k8s\.aliyun\.com/allocated-eniips}{"\n"}{end}' > \
  ${BACKUP_DIR}/node-eni-status-${TIMESTAMP}.txt

# 创建版本信息文件
cat > ${BACKUP_DIR}/VERSION-${TIMESTAMP} << EOF
Terway Backup Version Info
==========================
Backup Time: $(date)
Kubernetes Version: $(kubectl version --short | grep Server | awk '{print $3}')
Terway Image: $(kubectl get daemonset terway-eniip -n ${NAMESPACE} -o jsonpath='{.spec.template.spec.containers[0].image}')
Node Count: $(kubectl get nodes --no-headers | wc -l)
Region: $(kubectl get configmap eni-config -n ${NAMESPACE} -o jsonpath='{.data.eni_conf}' | jq -r '.region')
EOF

echo "备份完成!"
echo "备份文件列表:"
ls -la ${BACKUP_DIR}/terway-*${TIMESTAMP}*
```

## 六、故障排除手册

### 6.1 常见问题诊断矩阵

| 问题现象 | 可能原因 | 诊断命令 | 解决方案 |
|----------|----------|----------|----------|
| Pod 无法获取 IP | ENI/IP 配额耗尽 | `kubectl get nodes -o jsonpath='{.metadata.annotations.k8s\.aliyun\.com/allocated-eniips}'` | 增加配额或扩容节点 |
| Pod 网络不通 | 安全组规则限制 | `aliyun ecs DescribeInstanceAttribute --InstanceId <instance-id>` | 检查并修正安全组规则 |
| 跨节点通信失败 | VPC 路由配置错误 | `aliyun vpc DescribeRouteTableList` | 检查路由表配置 |
| ENI 创建失败 | RAM 权限不足 | `kubectl logs -n kube-system -l app=terway` | 检查并授予权限 |
| 网络延迟高 | 网络模式不当 | `kubectl exec -n kube-system <terway-pod> -- terway-cli show` | 考虑使用 ENI 模式 |

### 6.2 紧急恢复流程

```bash
#!/bin/bash
# terway-emergency-recovery.sh

echo "=== Terway 紧急恢复 ==="

# 1. 检查当前状态
echo "1. 检查当前 Terway 状态..."
kubectl get daemonset terway-eniip -n kube-system
kubectl get pods -n kube-system -l app=terway

# 2. 如果 Pod 异常，尝试重启
echo "2. 重启 Terway DaemonSet..."
kubectl rollout restart daemonset/terway-eniip -n kube-system

# 3. 等待恢复
echo "3. 等待 Pod 恢复..."
kubectl rollout status daemonset/terway-eniip -n kube-system --timeout=300s

# 4. 验证恢复
echo "4. 验证网络功能..."
kubectl run recovery-test --rm -it --image=busybox:1.36 \
  -- ping -c 4 8.8.8.8

echo "紧急恢复流程完成!"
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)