---
title: 'P5: 毕业综合项目'
description: 综合运用 4 周所学，独立完成一套完整的 ACK 集群运维方案：从集群规划、创建、安全加固、应用部署到监控告警，模拟真实生产环境的运维场景。
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- prometheus
- flannel
- helm
- opa
- mysql
- pdb
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'P5: 毕业综合项目 是什么'
- '如何 P5: 毕业综合项目'
trigger_keywords:
- 'P5:'
- 毕业综合项目
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- prometheus-basics
- mysql-basics
- policy-basics
---

---
title: P5: 毕业综合项目
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - ACK comprehensive cluster operation project
  - Kubernetes multi-tier architecture deployment
  - ACK end-to-end cluster lifecycle management
  - Production-grade cluster security hardening
  - Microservices deployment ACK best practices
trigger_keywords:
  - graduation
  - comprehensive
  - full-stack
  - project
  - ACK lifecycle
  - security hardening
  - monitoring
  - alerting
  - network
  - storage
reading_level: advanced
audience:
  - ACK learners (completion project)
  - DevOps engineers
  - Platform engineers
estimated_read_time: 60min
related_domains:
  - domain-12-cloud-providers
  - domain-05-security-compliance
  - domain-06-observability
  - domain-9-workload
related_topics:
  - ack-cluster-lifecycle
  - security-monitoring
  - node-workload-management
  - network-storage-practice
---

# P5: 毕业综合项目

> **对应周次**: 全部 4 周 | **预计时间**: 6-8 小时 | **难度**: ⭐⭐⭐⭐

---

## 项目目标

综合运用 4 周所学，独立完成一套完整的 ACK 集群运维方案：从集群规划、创建、安全加固、应用部署到监控告警，模拟真实生产环境的运维场景。

## 前置条件

- [ ] 完成 4 周全部教案和 4 次自测
- [ ] 完成 P1-P4 实操项目
- [ ] 准备好测试用的阿里云账号和资源

---

## 项目场景

> 你的团队需要为一个新业务搭建 ACK 集群环境。该业务包含 Web 前端、API 后端、数据库三层架构。
> 要求：高可用部署、权限隔离、监控告警、网络安全、存储持久化。

---

## 实施步骤

### Phase 1: 集群规划与创建 (1.5h)

#### 1.1 网络规划文档

```
填写以下规划表:

| 项目 | CIDR / 配置 |
|------|------------|
| VPC CIDR | |
| 节点 vSwitch-A (可用区) | |
| 节点 vSwitch-B (可用区) | |
| Pod vSwitch (Terway) 或 Pod CIDR (Flannel) | |
| Service CIDR | |
| CNI 方案选择 | Terway / Flannel |
| 选择理由 | |
```

#### 1.2 集群创建

```bash
# 使用 aliyun CLI 创建集群
# 要求:
# - 托管版 ACK
# - K8S 最新稳定版
# - 启用公网访问
# - 安装 Nginx Ingress Controller

cat > create-cluster.json << 'EOF'
{
  "name": "graduation-cluster",
  "region_id": "cn-hangzhou",
  "cluster_type": "ManagedKubernetes",
  "kubernetes_version": "1.28.3-aliyun.1",
  "vpcid": "<vpc-id>",
  "container_cidr": "10.96.0.0/16",
  "service_cidr": "172.21.0.0/20",
  "snat_entry": true,
  "public_slb": true,
  "node_cidr_mask": "26",
  "proxy_mode": "ipvs",
  "addons": [
    {"name": "nginx-ingress-controller", "config": "{}"},
    {"name": "csi-plugin", "config": "{}"},
    {"name": "csi-provisioner", "config": "{}"},
    {"name": "logtail-ds", "config": "{\"IngressDashboardEnabled\":\"true\"}"}
  ]
}
EOF

aliyun cs POST /clusters --body "$(cat create-cluster.json)"

# 等待集群创建完成
aliyun cs GET /clusters/<cluster_id> | jq '.state'

# 获取 kubeconfig
aliyun cs GET /k8s/<cluster_id>/user_config > ~/.kube/config-graduation
export KUBECONFIG=~/.kube/config-graduation

# 验证集群
kubectl cluster-info
kubectl get nodes
```

#### 1.3 节点池设计

```bash
# 创建 3 个节点池
# 1. system-pool: 系统组件专用 (2 节点)
cat > system-pool.json << 'EOF'
{
  "nodepool_info": {"name": "system-pool"},
  "scaling_group": {
    "vswitch_ids": ["<vsw-id-az-a>", "<vsw-id-az-b>"],
    "instance_types": ["ecs.g6.xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "key_pair": "<key-pair>"
  },
  "kubernetes_config": {
    "labels": [{"key": "node-role", "value": "system"}],
    "taints": [{"key": "CriticalAddonsOnly", "value": "true", "effect": "NoSchedule"}]
  },
  "management": {"auto_repair": true},
  "count": 2
}
EOF

aliyun cs POST /clusters/<cluster_id>/nodepools --body "$(cat system-pool.json)"

# 2. app-pool: 业务应用 (2-5 节点, 自动伸缩)
cat > app-pool.json << 'EOF'
{
  "nodepool_info": {"name": "app-pool"},
  "scaling_group": {
    "vswitch_ids": ["<vsw-id-az-a>", "<vsw-id-az-b>"],
    "instance_types": ["ecs.g6.xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "key_pair": "<key-pair>"
  },
  "kubernetes_config": {
    "labels": [{"key": "workload", "value": "app"}]
  },
  "management": {"auto_repair": true},
  "auto_scaling": {"enable": true, "min_instances": 2, "max_instances": 5},
  "count": 2
}
EOF

aliyun cs POST /clusters/<cluster_id>/nodepools --body "$(cat app-pool.json)"

# 3. data-pool: 数据库专用 (2 节点, 大内存规格)
cat > data-pool.json << 'EOF'
{
  "nodepool_info": {"name": "data-pool"},
  "scaling_group": {
    "vswitch_ids": ["<vsw-id-az-a>", "<vsw-id-az-b>"],
    "instance_types": ["ecs.r6.2xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 200,
    "key_pair": "<key-pair>"
  },
  "kubernetes_config": {
    "labels": [{"key": "workload", "value": "database"}],
    "taints": [{"key": "dedicated", "value": "database", "effect": "NoSchedule"}]
  },
  "management": {"auto_repair": true},
  "count": 2
}
EOF

aliyun cs POST /clusters/<cluster_id>/nodepools --body "$(cat data-pool.json)"
```

---

### Phase 2: 安全加固 (1h)

#### 2.1 RBAC 权限设计

```
设计权限矩阵:

| 角色 | Namespace | 权限 |
|------|-----------|------|
| 运维工程师 | 全集群 | 读写全部资源 |
| 开发工程师 | app-ns | Pod/Deployment/Service 读写 |
| 测试工程师 | app-ns | Pod/Service 只读 + Pod 日志 |
| 安全审计 | 全集群 | 只读 |
```

```bash
# 创建命名空间
kubectl create namespace app-ns

# 创建运维工程师 ClusterRole
cat > ops-engineer-role.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: ops-engineer
rules:
- apiGroups: ["*"]
  resources: ["*"]
  verbs: ["*"]
- nonResourceURLs: ["*"]
  verbs: ["get"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: ops-engineer-binding
subjects:
- kind: User
  name: ops@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: ops-engineer
  apiGroup: rbac.authorization.k8s.io
EOF

kubectl apply -f ops-engineer-role.yaml

# 创建开发工程师 Role（命名空间级别）
cat > dev-engineer-role.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: dev-engineer
  namespace: app-ns
rules:
- apiGroups: ["", "apps"]
  resources: ["pods", "deployments", "services", "configmaps"]
  verbs: ["get", "list", "watch", "create", "update", "patch", "delete"]
- apiGroups: [""]
  resources: ["pods/log"]
  verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: dev-engineer-binding
  namespace: app-ns
subjects:
- kind: User
  name: dev@example.com
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: Role
  name: dev-engineer
  apiGroup: rbac.authorization.k8s.io
EOF

kubectl apply -f dev-engineer-role.yaml

# 创建测试工程师 Role（只读 + 日志）
cat > test-engineer-role.yaml << 'EOF'
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: test-engineer
  namespace: app-ns
rules:
- apiGroups: ["", "apps"]
  resources: ["pods", "services"]
  verbs: ["get", "list", "watch"]
- apiGroups: [""]
  resources: ["pods/log"]
  verbs: ["get", "list"]
EOF

kubectl apply -f test-engineer-role.yaml

# 验证权限
kubectl auth can-i create pods -n app-ns --as=dev@example.com
kubectl auth can-i delete pods -n app-ns --as=dev@example.com
kubectl auth can-i create pods -n app-ns --as=test@example.com
```

#### 2.2 资源配额

```bash
cat > resource-quota.yaml << 'EOF'
apiVersion: v1
kind: ResourceQuota
metadata:
  name: app-quota
  namespace: app-ns
spec:
  hard:
    requests.cpu: "8"
    requests.memory: 16Gi
    limits.cpu: "16"
    limits.memory: 32Gi
    persistentvolumeclaims: "10"
    pods: "50"
    services: "20"
    configmaps: "30"
    secrets: "30"
---
apiVersion: v1
kind: LimitRange
metadata:
  name: app-limits
  namespace: app-ns
spec:
  limits:
  - default:
      cpu: 500m
      memory: 512Mi
    defaultRequest:
      cpu: 100m
      memory: 128Mi
    max:
      cpu: "4"
      memory: 4Gi
    min:
      cpu: 50m
      memory: 64Mi
    type: Container
EOF

kubectl apply -f resource-quota.yaml
kubectl get quota,limitrange -n app-ns
```

#### 2.3 NetworkPolicy (如使用 Terway)

```bash
cat > network-policy.yaml << 'EOF'
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: app-ns
spec:
  podSelector: {}
  policyTypes:
  - Ingress
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-backend
  namespace: app-ns
spec:
  podSelector:
    matchLabels:
      tier: backend
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          tier: frontend
    ports:
    - port: 8080
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-backend-to-db
  namespace: app-ns
spec:
  podSelector:
    matchLabels:
      tier: database
  policyTypes:
  - Ingress
  ingress:
  - from:
    - podSelector:
        matchLabels:
          tier: backend
    ports:
    - port: 3306
EOF

kubectl apply -f network-policy.yaml
kubectl get networkpolicy -n app-ns
```

---

### Phase 3: 应用部署 (2h)

#### 3.1 数据库层

```bash
cat > database.yaml << 'EOF'
apiVersion: v1
kind: Secret
metadata:
  name: db-credentials
  namespace: app-ns
type: Opaque
stringData:
  MYSQL_ROOT_PASSWORD: "Graduation2024!"
  MYSQL_DATABASE: "appdb"
---
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: app-ns
spec:
  serviceName: mysql-headless
  replicas: 1
  selector:
    matchLabels:
      app: mysql
      tier: database
  template:
    metadata:
      labels:
        app: mysql
        tier: database
    spec:
      nodeSelector:
        workload: database
      tolerations:
      - key: dedicated
        value: database
        effect: NoSchedule
      containers:
      - name: mysql
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
        command: ['sh', '-c', 'echo "MySQL Simulator running on $(hostname)" && sleep 86400']
        ports:
        - containerPort: 3306
        env:
        - name: MYSQL_ROOT_PASSWORD
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: MYSQL_ROOT_PASSWORD
        - name: MYSQL_DATABASE
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: MYSQL_DATABASE
        volumeMounts:
        - name: data
          mountPath: /var/lib/mysql
        resources:
          requests:
            cpu: 500m
            memory: 512Mi
          limits:
            cpu: "1"
            memory: 1Gi
        livenessProbe:
          exec:
            command: ['sh', '-c', 'echo "health check"']
          periodSeconds: 30
        readinessProbe:
          exec:
            command: ['sh', '-c', 'echo "ready"']
          periodSeconds: 10
  volumeClaimTemplates:
  - metadata:
      name: data
    spec:
      accessModes: [ReadWriteOnce]
      storageClassName: alicloud-disk-ssd
      resources:
        requests:
          storage: 40Gi
---
apiVersion: v1
kind: Service
metadata:
  name: mysql-headless
  namespace: app-ns
spec:
  clusterIP: None
  selector:
    app: mysql
  ports:
  - port: 3306
---
apiVersion: v1
kind: Service
metadata:
  name: mysql-svc
  namespace: app-ns
spec:
  selector:
    app: mysql
  ports:
  - port: 3306
    targetPort: 3306
EOF

kubectl apply -f database.yaml
kubectl get pods,svc,pvc -n app-ns -l tier=database
```

#### 3.2 API 后端

```bash
cat > backend.yaml << 'EOF'
apiVersion: v1
kind: ConfigMap
metadata:
  name: backend-config
  namespace: app-ns
data:
  DB_HOST: "mysql-svc"
  DB_PORT: "3306"
  APP_ENV: "production"
  LOG_LEVEL: "info"
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: api-backend
  namespace: app-ns
spec:
  replicas: 3
  selector:
    matchLabels:
      app: api-backend
      tier: backend
  template:
    metadata:
      labels:
        app: api-backend
        tier: backend
    spec:
      nodeSelector:
        workload: app
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values: [api-backend]
              topologyKey: kubernetes.io/hostname
      containers:
      - name: api
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
        ports:
        - containerPort: 8080
        envFrom:
        - configMapRef:
            name: backend-config
        env:
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: MYSQL_ROOT_PASSWORD
        resources:
          requests:
            cpu: 200m
            memory: 256Mi
          limits:
            cpu: 500m
            memory: 512Mi
        startupProbe:
          httpGet:
            path: /
            port: 80
          failureThreshold: 30
          periodSeconds: 2
        livenessProbe:
          httpGet:
            path: /
            port: 80
          periodSeconds: 15
        readinessProbe:
          httpGet:
            path: /
            port: 80
          periodSeconds: 5
---
apiVersion: v1
kind: Service
metadata:
  name: api-backend-svc
  namespace: app-ns
spec:
  selector:
    app: api-backend
  ports:
  - port: 80
    targetPort: 80
EOF

kubectl apply -f backend.yaml
```

#### 3.3 Web 前端

```bash
cat > frontend.yaml << 'EOF'
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-frontend
  namespace: app-ns
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-frontend
      tier: frontend
  template:
    metadata:
      labels:
        app: web-frontend
        tier: frontend
    spec:
      nodeSelector:
        workload: app
      affinity:
        podAntiAffinity:
          preferredDuringSchedulingIgnoredDuringExecution:
          - weight: 100
            podAffinityTerm:
              labelSelector:
                matchExpressions:
                - key: app
                  operator: In
                  values: [web-frontend]
              topologyKey: kubernetes.io/hostname
      containers:
      - name: nginx
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
        ports:
        - containerPort: 80
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 300m
            memory: 256Mi
        livenessProbe:
          httpGet:
            path: /
            port: 80
          periodSeconds: 15
        readinessProbe:
          httpGet:
            path: /
            port: 80
          periodSeconds: 5
---
apiVersion: v1
kind: Service
metadata:
  name: web-frontend-svc
  namespace: app-ns
spec:
  selector:
    app: web-frontend
  ports:
  - port: 80
    targetPort: 80
  type: ClusterIP
---
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: app-ingress
  namespace: app-ns
  annotations:
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - app.graduation.local
    secretName: app-tls
  rules:
  - host: app.graduation.local
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: web-frontend-svc
            port:
              number: 80
      - path: /api
        pathType: Prefix
        backend:
          service:
            name: api-backend-svc
            port:
              number: 80
EOF

kubectl apply -f frontend.yaml

# 创建自签名 TLS 证书
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout /tmp/tls.key -out /tmp/tls.crt \
  -subj "/CN=app.graduation.local"
kubectl create secret tls app-tls --key /tmp/tls.key --cert /tmp/tls.crt -n app-ns
```

#### 3.4 架构验证

```bash
# 验证清单:
echo "=== 架构总览 ==="
kubectl get all -n app-ns -o wide

echo "=== 调度验证 ==="
kubectl get pods -n app-ns -o custom-columns='NAME:.metadata.name,NODE:.spec.nodeName,STATUS:.status.phase'

echo "=== 网络验证 ==="
kubectl get svc,ingress -n app-ns

echo "=== 存储 ==="
kubectl get pvc -n app-ns

echo "=== 安全 ==="
kubectl get networkpolicy -n app-ns
kubectl get quota,limitrange -n app-ns
kubectl auth can-i create pods -n app-ns --as=dev@example.com

echo "=== 端到端测试 ==="
INGRESS_IP=$(kubectl get svc -n kube-system nginx-ingress-lb -o jsonpath='{.status.loadBalancer.ingress[0].ip}')
curl -k -H "Host: app.graduation.local" https://${INGRESS_IP}/
```

---

### Phase 4: 监控与运维 (1h)

#### 4.1 监控配置

```bash
# 确认 Prometheus 监控可用
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update
helm install prometheus prometheus-community/kube-prometheus-stack \
  --namespace monitoring --create-namespace \
  --set prometheus.prometheusSpec.retention=7d

# 创建自定义告警规则
cat > alert-rules.yaml << 'EOF'
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: graduation-alerts
  namespace: monitoring
spec:
  groups:
  - name: app-alerts
    rules:
    - alert: PodRestartTooMany
      expr: rate(kube_pod_container_status_restarts_total{namespace="app-ns"}[15m]) * 60 * 5 > 0
      for: 15m
      labels:
        severity: warning
      annotations:
        summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} restarting too often"
    - alert: NodeCPUHigh
      expr: sum(rate(node_cpu_seconds_total{mode!="idle"}[5m])) by (instance) / sum(rate(node_cpu_seconds_total[5m])) by (instance) > 0.8
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Node {{ $labels.instance }} CPU > 80%"
    - alert: PVCUsageHigh
      expr: (1 - kubelet_volume_stats_available_bytes / kubelet_volume_stats_capacity_bytes) > 0.9
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "PVC {{ $labels.persistentvolumeclaim }} usage > 90%"
EOF

kubectl apply -f alert-rules.yaml
kubectl get prometheusrule -n monitoring
```

#### 4.2 故障演练

```bash
# 演练 1: 模拟 Pod 故障
kubectl delete pod <api-pod-name> -n app-ns
# 观察: 自动恢复、readinessProbe 生效
kubectl get pods -n app-ns -w

# 演练 2: 模拟节点故障
kubectl cordon <node-name>
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data
# 观察: Pod 迁移、Service 自动更新 Endpoints
kubectl get pods -n app-ns -o wide -w
kubectl get endpoints api-backend-svc -n app-ns

# 演练 3: 模拟 DNS 故障排查
kubectl exec <pod-name> -n app-ns -- nslookup mysql-svc
kubectl exec <pod-name> -n app-ns -- nslookup api-backend-svc
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=10

# 演练 4: 模拟 PVC 问题
kubectl describe pvc -n app-ns
```

---

### Phase 5: 文档输出 (0.5h)

完成以下文档 (在自己的笔记中记录):

1. **集群架构图**: 画出集群的网络拓扑、节点池架构、应用部署图
2. **运维手册**: 记录日常运维操作 (扩容、升级、故障排查)
3. **经验总结**: 遇到的问题和解决方案

---

## 评分标准

| 评估项 | 满分 | 得分 |
|--------|:----:|:----:|
| 网络规划合理性 | 10 | |
| 集群创建与节点池配置 | 10 | |
| RBAC + 配额配置 | 10 | |
| 应用部署完整性 (三层架构) | 15 | |
| 调度策略正确性 | 10 | |
| 网络暴露 (Service + Ingress + TLS) | 10 | |
| 存储配置 (PVC + 持久化) | 10 | |
| 监控与告警 | 10 | |
| 故障演练与恢复 | 10 | |
| 文档输出质量 | 5 | |
| **合计** | **100** | |

**通过标准**: 80 分及以上

---

## 清理资源

```bash
# 删除应用
kubectl delete namespace app-ns

# 删除监控
helm uninstall prometheus -n monitoring
kubectl delete namespace monitoring

# 删除集群 (如不再需要)
aliyun cs DELETE /clusters/<cluster_id> --body '{"retain_all_resources": false}'

# 清理 VPC 资源
# aliyun vpc DeleteVSwitch --VSwitchId <vsw-id>
# aliyun vpc DeleteVpc --VpcId <vpc-id>
```

---

## 恭喜毕业！

完成本项目标志着你已具备 ACK 集群的独立运维能力。建议:

1. 将此项目的实操经验整理为团队文档
2. 在实际工作中持续应用所学
3. 关注 ACK 产品更新，持续学习新特性
4. 参与团队知识分享，教是最好的学

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
