# P3: 节点与工作负载管理实践

> **对应周次**: Week 3 | **预计时间**: 3-4 小时 | **难度**: ⭐⭐⭐

---

## 项目目标

设计和实施多节点池架构，完成节点运维操作 (扩缩容、维护、升级)，部署多种工作负载并配置调度策略与健康检查。

## 前置条件

- [ ] 完成 Week 3 全部教案 (Day 15-21)
- [ ] 有运行中的 ACK 集群 (至少 3 个节点)
- [ ] 了解节点池和 Pod 调度概念

---

## 实施步骤

### Step 1: 多节点池架构设计与创建 (40min)

```bash
# 1.1 设计节点池架构
# ┌─────────────────────────────────────────┐
# │           ACK 集群                       │
# ├──────────────┬──────────────┬───────────┤
# │ system-pool  │ app-pool     │ spot-pool │
# │ 4C16G × 2   │ 8C32G × 2-5 │ 8C32G × 0-3│
# │ 系统组件     │ 业务应用      │ 弹性任务   │
# └──────────────┴──────────────┴───────────┘

# 1.2 创建 spot (抢占式) 节点池
aliyun cs POST /clusters/<cluster_id>/nodepools --body '{
  "nodepool_info": {"name": "spot-pool"},
  "scaling_group": {
    "vswitch_ids": ["<vsw-id>"],
    "instance_types": ["ecs.g6.2xlarge"],
    "system_disk_category": "cloud_essd",
    "system_disk_size": 120,
    "desired_size": 1,
    "spot_strategy": "SpotWithPriceLimit",
    "spot_price_limit": [{"instance_type": "ecs.g6.2xlarge", "price_limit": "0.5"}]
  },
  "kubernetes_config": {
    "labels": [{"key": "node-type", "value": "spot"}],
    "taints": [{"key": "spot-instance", "value": "true", "effect": "PreferNoSchedule"}]
  },
  "auto_scaling": {
    "enable": true,
    "min_instances": 0,
    "max_instances": 3
  }
}'

# 1.3 查看所有节点池
aliyun cs GET /clusters/<cluster_id>/nodepools

# 1.4 查看节点标签分布
kubectl get nodes --show-labels | grep node-type
```

### Step 2: 节点运维操作 (40min)

```bash
# 2.1 节点维护模式 (cordon + drain)
NODE_NAME=$(kubectl get nodes -l node-type=spot -o jsonpath='{.items[0].metadata.name}' 2>/dev/null)

# 标记不可调度
kubectl cordon ${NODE_NAME}
kubectl get nodes

# 驱逐 Pod (优雅迁移)
kubectl drain ${NODE_NAME} --ignore-daemonsets --delete-emptydir-data --timeout=120s

# 模拟维护完成，恢复调度
kubectl uncordon ${NODE_NAME}

# 2.2 节点标签管理
kubectl label nodes ${NODE_NAME} environment=staging
kubectl label nodes ${NODE_NAME} team=backend

# 2.3 节点污点管理
kubectl taint nodes ${NODE_NAME} maintenance=true:NoExecute
kubectl taint nodes ${NODE_NAME} maintenance=true:NoExecute-  # 移除
```

### Step 3: 工作负载部署与调度 (40min)

```bash
# 3.1 部署 Deployment (调度到 app-pool)
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: Deployment
metadata:
  name: web-app
spec:
  replicas: 3
  selector:
    matchLabels:
      app: web-app
  template:
    metadata:
      labels:
        app: web-app
    spec:
      nodeSelector:
        node-role: app
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchExpressions:
              - key: app
                operator: In
                values: [web-app]
            topologyKey: kubernetes.io/hostname
      containers:
      - name: web
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
        ports:
        - containerPort: 80
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
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /
            port: 80
          periodSeconds: 5
EOF

# 3.2 部署可调度到 spot 节点的批处理任务
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: Job
metadata:
  name: batch-job
spec:
  completions: 5
  parallelism: 3
  template:
    spec:
      tolerations:
      - key: spot-instance
        operator: Equal
        value: "true"
        effect: PreferNoSchedule
      nodeSelector:
        node-type: spot
      containers:
      - name: worker
        image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
        command: ['sh', '-c', 'echo "Processing task $(hostname)..." && sleep 30 && echo "Done"']
        resources:
          requests:
            cpu: 100m
            memory: 64Mi
      restartPolicy: Never
EOF

# 3.3 查看调度结果
kubectl get pods -o wide
kubectl get pods -l job-name=batch-job -o wide
```

### Step 4: 组件健康检查 (20min)

```bash
# 4.1 全面组件检查
echo "=== kube-system 组件 ==="
kubectl get pods -n kube-system --sort-by='.status.phase'

echo "=== CoreDNS ==="
kubectl get pods -n kube-system -l k8s-app=kube-dns

echo "=== kube-proxy ==="
kubectl get ds -n kube-system kube-proxy

echo "=== CNI ==="
kubectl get ds -n kube-system | grep -E "terway|flannel"

echo "=== apiserver 健康 ==="
kubectl get --raw /healthz

# 4.2 DNS 测试
kubectl run dns-check --rm -it --restart=Never \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  -- nslookup kubernetes.default
```

---

## 验收清单

- [ ] 成功创建多节点池架构 (system + app + spot)
- [ ] 完成节点 cordon/drain/uncordon 操作
- [ ] Deployment 正确调度到指定节点池
- [ ] Job 使用 tolerations 调度到 spot 节点
- [ ] 三种探针均配置正确且工作正常
- [ ] 所有 kube-system 组件运行正常

---

## 清理资源

```bash
kubectl delete deploy web-app
kubectl delete job batch-job
# 删除 spot 节点池 (可选)
aliyun cs DELETE /clusters/<cluster_id>/nodepools/<spot-pool-id>
```
