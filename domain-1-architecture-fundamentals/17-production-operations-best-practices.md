# 17 - 生产环境运维最佳实践 (Production Operations Best Practices)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **专家级别**: ⭐⭐⭐⭐⭐ | **参考**: [Kubernetes Production Guide](https://kubernetes.io/docs/setup/production-environment/), CNCF Production Readiness

---

## 相关文档交叉引用

### 🔗 关联架构文档
- **[01-K8s架构全景图](./01-kubernetes-architecture-overview.md)** - 理解整体架构是运维的基础
- **[02-控制平面详解](./02-core-components-deep-dive.md)** - 掌握核心组件工作原理
- **[14-安全架构](./14-security-architecture.md)** - 安全运维实践基础
- **[15-可观测性体系](./15-observability-architecture.md)** - 监控告警体系建设

### 📚 扩展学习资料
- **[Domain-12: 故障排查](../domain-12-troubleshooting)** - 系统性故障诊断方法论
- **[Domain-9: 平台运维](../domain-9-platform-ops)** - 日常运维操作指南
- **[CNCF Production Readiness](https://www.cncf.io/blog/2020/08/12/kubernetes-production-readiness-checklist/)** - CNCF官方生产就绪检查清单

---

## 1. 生产环境架构设计原则 (Production Architecture Principles)

### 1.1 高可用性设计 (High Availability Design)

#### 控制平面高可用部署
```yaml
# HA控制平面部署配置示例
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
metadata:
  name: prod-cluster
spec:
  # etcd高可用配置
  etcd:
    local:
      extraArgs:
        initial-cluster-state: new
        listen-client-urls: https://0.0.0.0:2379
        listen-peer-urls: https://0.0.0.0:2380
        advertise-client-urls: https://ETCD_IP:2379
        initial-advertise-peer-urls: https://ETCD_IP:2380
      serverCertSANs:
        - "etcd1.prod.internal"
        - "etcd2.prod.internal" 
        - "etcd3.prod.internal"
  
  # API Server高可用配置
  apiServer:
    certSANs:
      - "k8s-api.prod.internal"
      - "k8s-api-vip.prod.internal"
      - "10.100.0.100"  # VIP地址
    extraArgs:
      audit-log-path: "/var/log/kubernetes/audit.log"
      audit-policy-file: "/etc/kubernetes/policies/audit-policy.yaml"
      profiling: "false"
      service-account-issuer: "https://k8s-api.prod.internal"
      tls-cipher-suites: "TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256"
      
  # Controller Manager配置
  controllerManager:
    extraArgs:
      cluster-signing-cert-file: "/etc/kubernetes/pki/ca.crt"
      cluster-signing-key-file: "/etc/kubernetes/pki/ca.key"
      concurrent-deployment-syncs: "10"
      concurrent-endpoint-syncs: "10"
      horizontal-pod-autoscaler-use-rest-clients: "true"
      
  # Scheduler配置
  scheduler:
    extraArgs:
      profiling: "false"
      bind-timeout: "600s"
```

#### 负载均衡和VIP配置
```bash
# HAProxy配置示例
cat > /etc/haproxy/haproxy.cfg << EOF
global
    log /dev/log local0
    log /dev/log local1 notice
    chroot /var/lib/haproxy
    stats socket /run/haproxy/admin.sock mode 660 level admin expose-fd listeners
    stats timeout 30s
    user haproxy
    group haproxy
    daemon

defaults
    log global
    mode tcp
    option tcplog
    option dontlognull
    timeout connect 10s
    timeout client 30s
    timeout server 30s

frontend k8s-api
    bind *:6443
    default_backend k8s-api-backend

backend k8s-api-backend
    balance roundrobin
    server master1 10.100.1.10:6443 check fall 3 rise 2
    server master2 10.100.1.11:6443 check fall 3 rise 2  
    server master3 10.100.1.12:6443 check fall 3 rise 2
EOF
```

### 1.2 安全加固配置 (Security Hardening)

#### Pod安全策略配置
```yaml
# PodSecurityPolicy示例 (已废弃，推荐使用Pod Security Admission)
apiVersion: policy/v1beta1
kind: PodSecurityPolicy
metadata:
  name: restricted-psp
spec:
  privileged: false
  allowPrivilegeEscalation: false
  requiredDropCapabilities:
    - ALL
  volumes:
    - 'configMap'
    - 'emptyDir'
    - 'projected'
    - 'secret'
    - 'downwardAPI'
    - 'persistentVolumeClaim'
  hostNetwork: false
  hostIPC: false
  hostPID: false
  runAsUser:
    rule: 'MustRunAsNonRoot'
  seLinux:
    rule: 'RunAsAny'
  supplementalGroups:
    rule: 'MustRunAs'
    ranges:
      - min: 1
        max: 65535
  fsGroup:
    rule: 'MustRunAs'
    ranges:
      - min: 1
        max: 65535
  readOnlyRootFilesystem: true
```

#### 网络策略实施
```yaml
# 默认拒绝所有流量的网络策略
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

---
# 允许特定服务间通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-app-traffic
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: my-app
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: database
    ports:
    - protocol: TCP
      port: 5432
```

### 1.3 资源管理和调度优化 (Resource Management & Scheduling)

#### 节点资源预留配置
```yaml
# kubelet资源配置
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
systemReserved:
  cpu: 500m
  memory: 1Gi
  ephemeral-storage: 10Gi
kubeReserved:
  cpu: 200m
  memory: 512Mi
  ephemeral-storage: 2Gi
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"
evictionSoft:
  memory.available: "1Gi"
  nodefs.available: "15%"
  nodefs.inodesFree: "10%"
  imagefs.available: "20%"
evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "1m30s"
  nodefs.inodesFree: "1m30s"
  imagefs.available: "1m30s"
```

#### 服务质量等级配置
```yaml
# Pod QoS配置示例
apiVersion: v1
kind: Pod
metadata:
  name: qos-example
spec:
  containers:
  - name: high-priority
    image: nginx
    resources:
      requests:
        memory: "256Mi"
        cpu: "250m"
      limits:
        memory: "512Mi" 
        cpu: "500m"
    # Guaranteed QoS - 最高优先级
    
  - name: burstable
    image: busybox
    resources:
      requests:
        memory: "128Mi"
        cpu: "100m"
      # Burstable QoS - 可突发
      
  - name: best-effort
    image: alpine
    # BestEffort QoS - 最低优先级
```

## 2. 监控告警体系建设 (Monitoring & Alerting System)

### 2.1 核心指标监控配置

#### Prometheus监控规则
```yaml
# 核心组件监控规则
groups:
- name: kubernetes.system.rules
  rules:
  # API Server监控
  - alert: APIServerDown
    expr: absent(up{job="apiserver"}) == 1
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "API Server不可用"
      description: "Kubernetes API Server在{{ $labels.instance }}上已停止响应超过5分钟"
      
  - alert: APIServerLatencyHigh
    expr: histogram_quantile(0.99, apiserver_request_duration_seconds_bucket{verb=~"LIST|GET"}) > 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "API Server响应延迟过高"
      description: "{{ $labels.verb }}请求99th百分位延迟为{{ $value }}秒，超过阈值1秒"
      
  # etcd监控
  - alert: EtcdMembersDown
    expr: (count(etcd_server_has_leader) - sum(etcd_server_has_leader)) > 1
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "etcd成员故障"
      description: "etcd集群中有{{ $value }}个成员失去领导者，影响集群可用性"
      
  # 节点监控
  - alert: NodeNotReady
    expr: kube_node_status_condition{condition="Ready",status!="true"} == 1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "节点NotReady状态"
      description: "节点{{ $labels.node }}处于NotReady状态超过10分钟"
      
  # Pod监控
  - alert: PodCrashLooping
    expr: rate(kube_pod_container_status_restarts_total[5m]) * 60 * 5 > 0
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "Pod持续重启"
      description: "Pod {{ $labels.pod }} 在命名空间 {{ $labels.namespace }} 中在过去5分钟内重启次数过多"
```

#### Grafana仪表板配置
```json
{
  "dashboard": {
    "title": "Kubernetes Production Overview",
    "panels": [
      {
        "title": "集群健康状态",
        "type": "stat",
        "targets": [
          {
            "expr": "sum(kube_node_status_condition{condition=\"Ready\",status=\"true\"})",
            "legendFormat": "Ready Nodes"
          },
          {
            "expr": "count(kube_pod_status_ready{condition=\"true\"})",
            "legendFormat": "Ready Pods"
          }
        ]
      },
      {
        "title": "API Server性能",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(apiserver_request_total[5m])",
            "legendFormat": "{{ verb }} requests/sec"
          },
          {
            "expr": "histogram_quantile(0.99, apiserver_request_duration_seconds_bucket)",
            "legendFormat": "99th percentile latency"
          }
        ]
      }
    ]
  }
}
```

### 2.2 日志收集和分析系统

#### Fluentd配置示例
```xml
<!-- Fluentd配置文件 -->
<source>
  @type tail
  path /var/log/containers/*.log
  pos_file /var/log/fluentd-containers.log.pos
  tag kubernetes.*
  read_from_head true
  <parse>
    @type json
    time_key time
    time_format %Y-%m-%dT%H:%M:%S.%NZ
  </parse>
</source>

<filter kubernetes.**>
  @type kubernetes_metadata
  @id filter_kube_metadata
  kubernetes_url "#{ENV['FLUENT_FILTER_KUBERNETES_URL'] || 'https://kubernetes.default.svc:443/api'}"
  bearer_token_file /var/run/secrets/kubernetes.io/serviceaccount/token
  ca_file /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
  skip_labels false
  skip_master_url false
  skip_container_metadata false
</filter>

<match kubernetes.var.log.containers.**_production_**.log>
  @type elasticsearch
  host elasticsearch.prod.svc.cluster.local
  port 9200
  logstash_format true
  logstash_prefix production-logs
  include_tag_key true
  type_name container_log
  <buffer>
    @type file
    path /var/log/fluentd-buffers/kubernetes.system.buffer
    flush_mode interval
    retry_type exponential_backoff
    flush_thread_count 2
    flush_interval 5s
    retry_forever
    retry_max_interval 30
    chunk_limit_size 2M
    queue_limit_length 8
    overflow_action block
  </buffer>
</match>
```

## 3. 备份恢复策略 (Backup & Recovery Strategy)

### 3.1 etcd备份配置

#### 自动备份脚本
```bash
#!/bin/bash
# etcd-backup.sh - etcd自动备份脚本

set -euo pipefail

BACKUP_DIR="/backup/etcd"
DATE=$(date +%Y%m%d_%H%M%S)
ETCDCTL_API=3

# 创建备份目录
mkdir -p ${BACKUP_DIR}/${DATE}

# 执行etcd快照
etcdctl snapshot save ${BACKUP_DIR}/${DATE}/snapshot.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 验证快照完整性
etcdctl snapshot status ${BACKUP_DIR}/${DATE}/snapshot.db \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 压缩备份文件
tar -czf ${BACKUP_DIR}/${DATE}.tar.gz -C ${BACKUP_DIR} ${DATE}

# 删除旧备份（保留最近7天）
find ${BACKUP_DIR} -name "*.tar.gz" -mtime +7 -delete
find ${BACKUP_DIR} -mindepth 1 -maxdepth 1 -type d -mtime +7 -exec rm -rf {} \;

# 上传到远程存储（可选）
if [[ "${UPLOAD_TO_S3:-false}" == "true" ]]; then
  aws s3 cp ${BACKUP_DIR}/${DATE}.tar.gz s3://${S3_BUCKET}/etcd-backups/
fi

echo "etcd backup completed: ${BACKUP_DIR}/${DATE}.tar.gz"
```

#### 备份验证脚本
```bash
#!/bin/bash
# etcd-restore-test.sh - 备份恢复测试脚本

set -euo pipefail

BACKUP_FILE=$1
TEST_NAMESPACE="backup-test-$(date +%s)"

# 恢复到临时集群进行验证
docker run --rm -d \
  --name etcd-restore-test \
  -p 2379:2379 \
  -v $(pwd)/${BACKUP_FILE}:/backup/snapshot.db \
  quay.io/coreos/etcd:v3.5.0 \
  etcd \
  --data-dir=/tmp/etcd-data \
  --listen-client-urls=http://0.0.0.0:2379 \
  --advertise-client-urls=http://0.0.0.0:2379

sleep 10

# 验证数据完整性
docker exec etcd-restore-test etcdctl snapshot status /backup/snapshot.db
docker exec etcd-restore-test etcdctl get / --prefix --keys-only | wc -l

# 清理测试环境
docker stop etcd-restore-test

echo "Backup validation completed successfully"
```

### 3.2 应用配置备份

#### Helm Release备份
```bash
#!/bin/bash
# helm-backup.sh - Helm Release备份脚本

BACKUP_DIR="/backup/helm"
DATE=$(date +%Y%m%d_%H%M%S)

mkdir -p ${BACKUP_DIR}/${DATE}

# 备份所有Helm Release
helm list --all-namespaces -o json | jq -r '.[].name + " " + .[].namespace' | while read release namespace; do
  mkdir -p ${BACKUP_DIR}/${DATE}/${namespace}
  helm get values ${release} --namespace ${namespace} > ${BACKUP_DIR}/${DATE}/${namespace}/${release}-values.yaml
  helm get manifest ${release} --namespace ${namespace} > ${BACKUP_DIR}/${DATE}/${namespace}/${release}-manifest.yaml
done

# 压缩备份
tar -czf ${BACKUP_DIR}/${DATE}-helm.tar.gz -C ${BACKUP_DIR} ${DATE}
rm -rf ${BACKUP_DIR}/${DATE}

echo "Helm releases backup completed: ${BACKUP_DIR}/${DATE}-helm.tar.gz"
```

## 4. 灾难恢复计划 (Disaster Recovery Plan)

### 4.1 DR演练流程

#### 集群重建脚本
```bash
#!/bin/bash
# cluster-restore.sh - 集群灾难恢复脚本

set -euo pipefail

BACKUP_FILE=${1:-"/backup/latest.tar.gz"}
NEW_CLUSTER_CIDR=${2:-"10.244.0.0/16"}

# 1. 初始化新集群
kubeadm init \
  --pod-network-cidr=${NEW_CLUSTER_CIDR} \
  --control-plane-endpoint="k8s-api.new-cluster.internal:6443" \
  --upload-certs

# 2. 恢复etcd数据
mkdir -p /tmp/etcd-restore
tar -xzf ${BACKUP_FILE} -C /tmp/etcd-restore

# 停止当前etcd
systemctl stop etcd

# 恢复快照
ETCDCTL_API=3 etcdctl snapshot restore /tmp/etcd-restore/snapshot.db \
  --data-dir=/var/lib/etcd \
  --initial-cluster="etcd1=https://10.100.1.10:2380" \
  --initial-cluster-token="etcd-cluster-1" \
  --initial-advertise-peer-urls="https://10.100.1.10:2380"

# 启动etcd
systemctl start etcd

# 3. 重新应用配置
kubectl apply -f /tmp/etcd-restore/manifests/

echo "Cluster restoration completed"
```

### 4.2 多区域部署策略

#### 跨区域联邦配置
```yaml
# Kubefed配置示例
apiVersion: types.kubefed.io/v1beta1
kind: KubeFedCluster
metadata:
  name: prod-us-east
  namespace: kube-federation-system
spec:
  apiEndpoint: https://k8s-api.us-east.prod.internal:6443
  secretRef:
    name: us-east-cluster-secret
    
---
apiVersion: types.kubefed.io/v1beta1
kind: KubeFedCluster
metadata:
  name: prod-us-west
  namespace: kube-federation-system
spec:
  apiEndpoint: https://k8s-api.us-west.prod.internal:6443
  secretRef:
    name: us-west-cluster-secret
```

## 5. 性能优化和容量规划 (Performance Optimization & Capacity Planning)

### 5.1 集群容量评估

#### 容量规划计算器
```bash
#!/bin/bash
# capacity-planner.sh - 集群容量规划工具

NODE_COUNT=${1:-10}
POD_PER_NODE=${2:-110}
CPU_PER_NODE=${3:-8}
MEMORY_PER_NODE=${4:-32}

echo "=== Kubernetes集群容量规划报告 ==="
echo "节点数量: ${NODE_COUNT}"
echo "每节点Pod数: ${POD_PER_NODE}"
echo "每节点CPU: ${CPU_PER_NODE}核"
echo "每节点内存: ${MEMORY_PER_NODE}GB"
echo ""

# 计算总容量
TOTAL_PODS=$((NODE_COUNT * POD_PER_NODE))
TOTAL_CPU=$((NODE_COUNT * CPU_PER_NODE))
TOTAL_MEMORY=$((NODE_COUNT * MEMORY_PER_NODE))

echo "总Pod容量: ${TOTAL_PODS}个"
echo "总CPU容量: ${TOTAL_CPU}核"
echo "总内存容量: ${TOTAL_MEMORY}GB"

# API Server压力估算
API_SERVER_QPS=$((TOTAL_PODS / 100))  # 假设每个Pod产生0.01 QPS
echo "预估API Server QPS: ${API_SERVER_QPS}"

# etcd存储需求估算
ETCD_STORAGE_GB=$((TOTAL_PODS * 2 / 1024))  # 假设每个对象2KB
echo "预估etcd存储需求: ${ETCD_STORAGE_GB}GB"

# 建议配置
echo ""
echo "=== 建议配置 ==="
if [ $NODE_COUNT -gt 50 ]; then
  echo "✓ 建议启用API Server水平扩展"
  echo "✓ 建议etcd集群使用SSD存储"
  echo "✓ 建议增加监控采样间隔"
fi
```

### 5.2 性能调优参数

#### 内核参数优化
```bash
# /etc/sysctl.d/k8s.conf - Kubernetes内核优化参数
net.bridge.bridge-nf-call-ip6tables = 1
net.bridge.bridge-nf-call-iptables = 1
net.ipv4.ip_forward = 1
net.ipv4.tcp_keepalive_time = 600
net.ipv4.tcp_keepalive_intvl = 60
net.ipv4.tcp_keepalive_probes = 3
net.core.somaxconn = 32768
net.ipv4.tcp_max_syn_backlog = 8096
net.ipv4.tcp_fin_timeout = 30
vm.max_map_count = 262144
fs.file-max = 1000000
fs.inotify.max_user_watches = 1048576
```

#### kubelet性能调优
```yaml
# kubelet性能优化配置
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
maxPods: 250
podPidsLimit: 4096
serializeImagePulls: false
imageGCHighThresholdPercent: 85
imageGCLowThresholdPercent: 80
evictionPressureTransitionPeriod: "5m0s"
containerLogMaxSize: "100Mi"
containerLogMaxFiles: 10
cpuManagerPolicy: "static"
topologyManagerPolicy: "best-effort"
```

## 6. 安全合规和审计 (Security Compliance & Auditing)

### 6.1 CIS基准符合性检查

#### 自动化合规检查脚本
```bash
#!/bin/bash
# cis-benchmark-check.sh - CIS Kubernetes基准检查

echo "=== CIS Kubernetes Benchmark Check ==="

# 检查API Server配置
echo "1. API Server安全配置检查:"
if systemctl is-active kube-apiserver >/dev/null 2>&1; then
  ps aux | grep kube-apiserver | grep -q "profiling=false" && echo "✓ Profiling已禁用" || echo "✗ Profiling未禁用"
  ps aux | grep kube-apiserver | grep -q "anonymous-auth=false" && echo "✓ 匿名认证已禁用" || echo "✗ 匿名认证未禁用"
  ps aux | grep kube-apiserver | grep -q "authorization-mode.*RBAC" && echo "✓ RBAC已启用" || echo "✗ RBAC未启用"
else
  echo "✗ API Server未运行"
fi

# 检查etcd配置
echo -e "\n2. etcd安全配置检查:"
if systemctl is-active etcd >/dev/null 2>&1; then
  ps aux | grep etcd | grep -q "client-cert-auth=true" && echo "✓ 客户端证书认证已启用" || echo "✗ 客户端证书认证未启用"
  ps aux | grep etcd | grep -q "auto-tls=false" && echo "✓ 自动TLS已禁用" || echo "✗ 自动TLS未禁用"
else
  echo "✗ etcd未运行"
fi

# 检查网络策略
echo -e "\n3. 网络策略检查:"
default_deny_count=$(kubectl get networkpolicy --all-namespaces 2>/dev/null | grep -c "default-deny" || echo "0")
if [ "$default_deny_count" -gt 0 ]; then
  echo "✓ 发现默认拒绝策略"
else
  echo "✗ 未发现默认拒绝策略"
fi

echo -e "\n=== 检查完成 ==="
```

### 6.2 审计日志配置

#### 审计策略配置
```yaml
# /etc/kubernetes/policies/audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
# 不记录watch请求
- level: None
  verbs: ["watch"]
  
# 不记录读取相关的请求
- level: None
  resources:
  - group: ""
    resources: ["events"]
    
# 记录Pod变更的元数据
- level: Metadata
  resources:
  - group: ""
    resources: ["pods"]
  verbs: ["create", "update", "patch", "delete"]
  
# 记录Secret和ConfigMap的变更
- level: RequestResponse
  resources:
  - group: ""
    resources: ["secrets", "configmaps"]
  verbs: ["create", "update", "patch", "delete"]
  
# 记录认证相关操作
- level: RequestResponse
  resources:
  - group: "rbac.authorization.k8s.io"
    resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
  verbs: ["create", "update", "patch", "delete"]
  
# 记录所有其他请求的基本信息
- level: Metadata
```

## 7. 运维自动化和GitOps (Operations Automation & GitOps)

### 7.1 GitOps流水线配置

#### ArgoCD应用配置
```yaml
# argocd-application.yaml
apiVersion: argoproj.io/v1alpha1
kind: Application
metadata:
  name: production-app
  namespace: argocd
spec:
  project: production
  source:
    repoURL: https://github.com/company/production-manifests.git
    targetRevision: HEAD
    path: apps/production
    helm:
      valueFiles:
      - values-prod.yaml
  destination:
    server: https://kubernetes.default.svc
    namespace: production
  syncPolicy:
    automated:
      prune: true
      selfHeal: true
    syncOptions:
    - CreateNamespace=true
    - ApplyOutOfSyncOnly=true
  ignoreDifferences:
  - group: apps
    kind: Deployment
    jsonPointers:
    - /spec/replicas
  info:
  - name: url
    value: https://grafana.prod.company.com/d/cluster-overview
```

### 7.2 自动化运维脚本

#### 健康检查脚本
```bash
#!/bin/bash
# health-check.sh - 集群健康检查脚本

set -euo pipefail

echo "=== Kubernetes集群健康检查 ==="
echo "检查时间: $(date)"
echo "集群版本: $(kubectl version --short | grep Server | awk '{print $3}')"
echo ""

# 1. 节点健康检查
echo "1. 节点状态检查:"
kubectl get nodes -o wide | grep -E "(NotReady|SchedulingDisabled)" && {
  echo "⚠️  发现不健康的节点"
  exit 1
} || echo "✓ 所有节点状态正常"

# 2. 核心组件检查
echo -e "\n2. 核心组件检查:"
for component in kube-apiserver kube-controller-manager kube-scheduler; do
  pod_count=$(kubectl get pods -n kube-system -l tier=control-plane,component=${component} --no-headers | wc -l)
  ready_count=$(kubectl get pods -n kube-system -l tier=control-plane,component=${component} -o jsonpath='{.items[*].status.containerStatuses[?(@.ready==true)].ready}' | wc -w)
  if [ "$pod_count" -eq "$ready_count" ] && [ "$pod_count" -gt 0 ]; then
    echo "✓ ${component}: ${ready_count}/${pod_count} 就绪"
  else
    echo "✗ ${component}: ${ready_count}/${pod_count} 就绪"
    exit 1
  fi
done

# 3. 系统资源检查
echo -e "\n3. 系统资源检查:"
low_resource_pods=$(kubectl top pods --all-namespaces 2>/dev/null | awk '$3+0 > 80 || $4+0 > 80 {print $1"/"$2}' | head -10)
if [ -n "$low_resource_pods" ]; then
  echo "⚠️  发现资源使用率高的Pod:"
  echo "$low_resource_pods"
else
  echo "✓ 资源使用率正常"
fi

echo -e "\n=== 健康检查完成 ==="
```

## 8. 成本优化和FinOps (Cost Optimization & FinOps)

### 8.1 资源使用分析

#### 成本分析脚本
```bash
#!/bin/bash
# cost-analyzer.sh - Kubernetes成本分析工具

echo "=== Kubernetes成本分析报告 ==="
echo "分析时间: $(date)"
echo ""

# 获取节点规格和成本
echo "1. 节点成本分析:"
kubectl get nodes -o json | jq -r '
  .items[] | 
  {
    name: .metadata.name,
    instance_type: .metadata.labels."node.kubernetes.io/instance-type",
    capacity_cpu: .status.capacity.cpu,
    capacity_memory: .status.capacity.memory,
    allocatable_cpu: .status.allocatable.cpu,
    allocatable_memory: .status.allocatable.memory
  } | 
  "节点: \(.name)\n实例类型: \(.instance_type)\nCPU容量: \(.capacity_cpu)核\n内存容量: \(.capacity_memory)\n可分配CPU: \(.allocatable_cpu)核\n可分配内存: \(.allocatable_memory)\n---"
'

# Pod资源请求分析
echo -e "\n2. Pod资源请求分析:"
kubectl get pods --all-namespaces -o json | jq -r '
  .items[] |
  .spec.containers[] |
  {
    namespace: .metadata.namespace,
    pod: .metadata.name,
    container: .name,
    cpu_request: .resources.requests.cpu,
    memory_request: .resources.requests.memory,
    cpu_limit: .resources.limits.cpu,
    memory_limit: .resources.limits.memory
  } |
  select(.cpu_request != null or .memory_request != null) |
  "NS:\(.namespace) Pod:\(.pod) Container:\(.container)\n  CPU请求: \(.cpu_request) 限制: \(.cpu_limit)\n  内存请求: \(.memory_request) 限制: \(.memory_limit)\n---"
' | head -20

# 资源利用率统计
echo -e "\n3. 资源利用率统计:"
echo "CPU请求总量: $(kubectl get pods --all-namespaces -o json | jq '[.items[].spec.containers[].resources.requests.cpu | tonumber] | add') 核"
echo "内存请求总量: $(kubectl get pods --all-namespaces -o json | jq '[.items[].spec.containers[].resources.requests.memory | sub("Gi$"; "") | tonumber] | add') Gi"
```

### 8.2 自动伸缩配置

#### HPA配置示例
```yaml
# HorizontalPodAutoscaler配置
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: app-hpa
  namespace: production
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: app-deployment
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 60
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
      - type: Pods
        value: 4
        periodSeconds: 60
      selectPolicy: Max
```

---

## 💡 专家提示 (Expert Tips)

### 关键成功因素
1. **渐进式部署** - 采用蓝绿部署或金丝雀发布策略
2. **监控先行** - 在变更前确保有足够的监控覆盖
3. **文档驱动** - 所有操作都应该有详细的文档记录
4. **定期演练** - 定期进行DR演练和故障恢复训练
5. **持续改进** - 基于监控数据和故障经验不断优化

### 常见陷阱避免
- ❌ 忽视etcd性能调优
- ❌ 缺乏适当的备份策略  
- ❌ 没有实施网络策略
- ❌ 忽略安全补丁更新
- ❌ 缺乏容量规划

### 最佳实践总结
- ✅ 实施多层次监控告警体系
- ✅ 建立完善的备份恢复机制
- ✅ 采用GitOps进行配置管理
- ✅ 实施严格的访问控制策略
- ✅ 定期进行安全合规检查

---