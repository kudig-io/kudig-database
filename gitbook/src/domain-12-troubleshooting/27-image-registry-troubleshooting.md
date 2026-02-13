# 27 - 镜像仓库故障排查 (Image Registry Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Images](https://kubernetes.io/docs/concepts/containers/images/)

---

## 1. 镜像仓库故障诊断总览 (Image Registry Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **镜像拉取失败** | ImagePullBackOff | 应用部署中断 | P0 - 紧急 |
| **认证失败** | 401 Unauthorized | 私有仓库访问拒绝 | P0 - 紧急 |
| **网络连接超时** | timeout/网络不可达 | 镜像下载失败 | P1 - 高 |
| **镜像不存在** | manifest unknown | 部署配置错误 | P1 - 高 |
| **镜像安全扫描失败** | 漏洞检测告警 | 安全合规风险 | P1 - 高 |

### 1.2 镜像仓库架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                   镜像仓库故障诊断架构                                        │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       应用部署层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Pod-A     │    │   Pod-B     │    │   Pod-C     │              │  │
│  │  │ (拉取镜像)   │    │ (拉取镜像)   │    │ (拉取镜像)   │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    镜像拉取代理层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   cri-o     │    │ containerd  │    │   docker    │              │  │
│  │  │  (CRI)      │    │  (CRI)      │    │  (CRI)      │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   认证管理   │   │   网络代理   │   │   缓存管理   │                   │
│  │ (Registry   │   │ (Proxy)     │   │ (Cache)     │                   │
│  │ Auth)       │   │   镜像      │   │   镜像      │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      镜像仓库层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Docker    │    │   Harbor    │    │   ACR/GCR   │              │  │
│  │  │   Hub       │    │   Registry  │    │   Registry  │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      存储后端层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   本地存储   │   │   对象存储   │   │   分布式存储  │              │  │
│  │  │ (Local)     │   │ (S3/OSS)    │   │ (GlusterFS) │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 镜像拉取失败问题排查 (Image Pull Failure Troubleshooting)

### 2.1 基础状态检查

```bash
# ========== 1. Pod状态检查 ==========
# 查看镜像拉取失败的Pod
kubectl get pods --all-namespaces --field-selector=status.phase=Pending -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .status.containerStatuses[*].state.waiting.reason
}{
        "\n"
}{
    end
}' | grep ImagePullBackOff

# 查看详细事件信息
kubectl describe pod <pod-name> -n <namespace>

# 检查容器状态
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{
    .status.containerStatuses[*].state.waiting.message
}'

# ========== 2. 镜像配置检查 ==========
# 查看Pod使用的镜像
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{
    .spec.containers[*].image
}'

# 检查镜像拉取策略
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{
    .spec.containers[*].imagePullPolicy
}'

# 验证镜像标签
kubectl get deployment <deployment-name> -n <namespace> -o jsonpath='{
    .spec.template.spec.containers[*].image
}'
```

### 2.2 认证和权限问题

```bash
# ========== 1. 镜像仓库认证检查 ==========
# 查看imagePullSecrets配置
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{
    .spec.imagePullSecrets[*].name
}'

# 检查Secret内容
SECRET_NAME=$(kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.imagePullSecrets[0].name}')
if [ -n "$SECRET_NAME" ]; then
    kubectl get secret $SECRET_NAME -n <namespace> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | jq
fi

# 验证认证配置格式
kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d

# ========== 2. 手动认证测试 ==========
# 创建测试Pod验证认证
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: registry-test
  namespace: <namespace>
spec:
  containers:
  - name: test
    image: <private-registry>/<image>:<tag>
    command: ["sleep", "3600"]
  imagePullSecrets:
  - name: <secret-name>
EOF

# 测试Docker登录
kubectl run docker-test --image=docker:dind -n <namespace> -it --rm --privileged -- sh
# 在容器内执行:
# docker login <registry-url> -u <username> -p <password>
# docker pull <private-registry>/<image>:<tag>

# ========== 3. 多仓库认证配置 ==========
# 创建包含多个仓库认证的Secret
cat <<EOF > multi-registry-auth.yaml
apiVersion: v1
kind: Secret
metadata:
  name: multi-registry-secret
  namespace: <namespace>
type: kubernetes.io/dockerconfigjson
data:
  .dockerconfigjson: $(echo '{
    "auths": {
      "https://index.docker.io/v1/": {
        "auth": "'$(echo -n "username:password" | base64)'"
      },
      "harbor.example.com": {
        "auth": "'$(echo -n "harbor-user:harbor-password" | base64)'"
      },
      "gcr.io": {
        "auth": "'$(echo -n "_json_key:{\"private_key\":\"...\"}" | base64)'"
      }
    }
  }' | base64)
EOF
```

---

## 3. 网络连接问题排查 (Network Connectivity Issues)

### 3.1 网络连通性测试

```bash
# ========== 1. 基础网络测试 ==========
# 测试到镜像仓库的网络连通性
kubectl run network-test --image=busybox -n <namespace> -it --rm -- sh

# 在测试容器中执行网络测试:
# 测试DNS解析
nslookup <registry-domain>

# 测试端口连通性
nc -zv <registry-domain> 443
nc -zv <registry-domain> 80

# 测试HTTPS连接
openssl s_client -connect <registry-domain>:443 -servername <registry-domain>

# ========== 2. 镜像仓库API测试 ==========
# 测试Docker Registry API
REGISTRY_URL="https://<registry-domain>"
REPOSITORY="<repository-name>"
TAG="<tag>"

# 获取认证token
TOKEN=$(curl -s -u "<username>:<password>" "$REGISTRY_URL/v2/token?service=<registry-domain>&scope=repository:$REPOSITORY:pull" | jq -r .token)

# 测试镜像manifest
curl -s -H "Authorization: Bearer $TOKEN" "$REGISTRY_URL/v2/$REPOSITORY/manifests/$TAG" | jq

# 测试blob层
MANIFEST=$(curl -s -H "Authorization: Bearer $TOKEN" "$REGISTRY_URL/v2/$REPOSITORY/manifests/$TAG")
BLOB_DIGEST=$(echo $MANIFEST | jq -r '.layers[0].digest')
curl -s -H "Authorization: Bearer $TOKEN" -I "$REGISTRY_URL/v2/$REPOSITORY/blobs/$BLOB_DIGEST"

# ========== 3. 代理和防火墙检查 ==========
# 检查节点代理配置
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
    echo "=== Node: $node ==="
    kubectl debug node/$node --image=busybox -it -- sh -c "
        echo 'Checking HTTP proxy:'
        env | grep -i proxy
        echo 'Testing registry connectivity:'
        nc -zv <registry-domain> 443
    "
done

# 检查网络策略影响
kubectl get networkpolicy -n <namespace> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.egress[*].to[*].ipBlock.cidr
}{
        "\n"
}{
    end
}'
```

### 3.2 镜像拉取超时问题

```bash
# ========== 超时配置优化 ==========
# 调整kubelet镜像拉取超时设置
cat <<EOF > kubelet-config-patch.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
imagePullProgressDeadline: "5m0s"  # 增加到5分钟
registryPullQPS: 10                # 每秒拉取请求数
registryBurst: 20                  # 突发请求数
EOF

# 为特定节点应用配置
kubectl patch node <node-name> -p '{
    "spec": {
        "configSource": {
            "configMap": {
                "name": "kubelet-config",
                "namespace": "kube-system",
                "kubeletConfigKey": "kubelet"
            }
        }
    }
}'

# ========== 镜像预拉取策略 ==========
# 创建镜像预拉取DaemonSet
cat <<EOF | kubectl apply -f -
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: image-prepull
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: image-prepull
  template:
    metadata:
      labels:
        app: image-prepull
    spec:
      containers:
      - name: prepull
        image: <private-registry>/<image>:<tag>
        command: ["sleep", "infinity"]
        resources:
          requests:
            cpu: "100m"
            memory: "128Mi"
      imagePullSecrets:
      - name: <registry-secret>
      terminationGracePeriodSeconds: 30
EOF

# ========== 镜像缓存优化 ==========
# 配置镜像缓存代理
cat <<EOF > image-cache-proxy.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: registry-proxy
  namespace: kube-system
spec:
  replicas: 2
  selector:
    matchLabels:
      app: registry-proxy
  template:
    metadata:
      labels:
        app: registry-proxy
    spec:
      containers:
      - name: proxy
        image: registry:2.8
        ports:
        - containerPort: 5000
        env:
        - name: REGISTRY_PROXY_REMOTEURL
          value: "https://registry-1.docker.io"
        - name: REGISTRY_STORAGE_CACHE_BLOBDESCRIPTOR
          value: "inmemory"
        volumeMounts:
        - name: cache-volume
          mountPath: /var/lib/registry
      volumes:
      - name: cache-volume
        emptyDir: {}
---
apiVersion: v1
kind: Service
metadata:
  name: registry-proxy
  namespace: kube-system
spec:
  selector:
    app: registry-proxy
  ports:
  - port: 5000
    targetPort: 5000
EOF
```

---

## 4. 镜像安全和合规问题 (Image Security and Compliance)

### 4.1 镜像安全扫描

```bash
# ========== 1. 静态安全扫描 ==========
# 使用Trivy扫描镜像
trivy image <image-name>:<tag>

# 扫描严重漏洞
trivy image --severity HIGH,CRITICAL <image-name>:<tag>

# 生成安全报告
trivy image --format json --output scan-report.json <image-name>:<tag>

# ========== 2. 运行时安全检查 ==========
# 检查运行中的容器安全配置
kubectl get pods -n <namespace> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.containers[*].securityContext.privileged
}{
        "\n"
}{
    end
}' | grep true

# 检查镜像签名
cosign verify --key <public-key> <image-name>:<tag>

# 验证镜像完整性
docker inspect <image-name>:<tag> | jq '.[0].Id'

# ========== 3. 安全策略实施 ==========
# 配置镜像策略Webhook
cat <<EOF > image-policy-webhook.yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: image-policy
webhooks:
- name: image-policy.example.com
  clientConfig:
    service:
      name: image-policy-service
      namespace: kube-system
      path: "/validate"
    caBundle: <ca-bundle>
  rules:
  - operations: ["CREATE", "UPDATE"]
    apiGroups: [""]
    apiVersions: ["v1"]
    resources: ["pods"]
  admissionReviewVersions: ["v1"]
  sideEffects: None
  timeoutSeconds: 5
EOF

# 创建镜像白名单策略
cat <<EOF > image-whitelist-policy.yaml
apiVersion: policies.example.com/v1
kind: ImagePolicy
metadata:
  name: whitelist-policy
spec:
  allowedRegistries:
  - harbor.example.com
  - gcr.io
  - quay.io
  blockedImages:
  - "vulnerable-image:*"
  requiredSignatures: true
  minimumSeverity: "HIGH"
EOF
```

### 4.2 镜像漏洞管理

```bash
# ========== 漏洞监控告警 ==========
# 配置漏洞扫描CronJob
cat <<EOF | kubectl apply -f -
apiVersion: batch/v1
kind: CronJob
metadata:
  name: vulnerability-scan
  namespace: security
spec:
  schedule: "0 2 * * *"  # 每天凌晨2点执行
  jobTemplate:
    spec:
      template:
        spec:
          containers:
          - name: scanner
            image: aquasec/trivy:latest
            command:
            - /bin/sh
            - -c
            - |
              # 扫描集群中使用的所有镜像
              kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.spec.containers[*].image}{"\n"}{end}' | \
              sort | uniq | while read image; do
                echo "Scanning \$image"
                trivy image --severity HIGH,CRITICAL --exit-code 1 \$image || echo "Vulnerabilities found in \$image"
              done
            env:
            - name: TRIVY_CACHE_DIR
              value: /tmp/trivy-cache
            volumeMounts:
            - name: cache
              mountPath: /tmp/trivy-cache
          volumes:
          - name: cache
            emptyDir: {}
          restartPolicy: OnFailure
EOF

# ========== 漏洞修复流程 ==========
# 自动化漏洞修复脚本
cat <<'EOF' > auto-vulnerability-fix.sh
#!/bin/bash

NAMESPACE=${1:-default}
SEVERITY=${2:-HIGH}

echo "Starting automated vulnerability fix for namespace: $NAMESPACE"

# 获取需要扫描的镜像列表
IMAGES=$(kubectl get pods -n $NAMESPACE -o jsonpath='{range .items[*]}{.spec.containers[*].image}{"\n"}{end}' | sort | uniq)

echo "Found images to scan:"
echo "$IMAGES"

# 扫描每个镜像
echo "$IMAGES" | while read image; do
    echo "Scanning image: $image"
    
    # 执行漏洞扫描
    RESULT=$(trivy image --severity $SEVERITY --exit-code 0 $image 2>&1)
    EXIT_CODE=$?
    
    if [ $EXIT_CODE -ne 0 ]; then
        echo "⚠️  Vulnerabilities found in $image"
        echo "$RESULT"
        
        # 尝试自动修复 - 拉取最新版本
        if [[ $image == *:* ]]; then
            BASE_IMAGE=$(echo $image | cut -d: -f1)
            NEW_IMAGE="${BASE_IMAGE}:latest"
            
            echo "Attempting to update to: $NEW_IMAGE"
            if docker pull $NEW_IMAGE 2>/dev/null; then
                echo "✓ Successfully pulled newer image"
                
                # 更新Deployment
                DEPLOYMENTS=$(kubectl get deployments -n $NAMESPACE -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}')
                echo "$DEPLOYMENTS" | while read deployment; do
                    if kubectl get deployment $deployment -n $NAMESPACE -o jsonpath='{.spec.template.spec.containers[*].image}' | grep -q $image; then
                        echo "Updating deployment: $deployment"
                        kubectl set image deployment/$deployment -n $NAMESPACE *=${NEW_IMAGE}
                    fi
                done
            else
                echo "✗ Failed to pull newer image"
            fi
        fi
    else
        echo "✓ No $SEVERITY vulnerabilities found in $image"
    fi
done

echo "Vulnerability fix process completed"
EOF

chmod +x auto-vulnerability-fix.sh
```

---

## 5. 镜像仓库性能优化 (Image Registry Performance Optimization)

### 5.1 镜像拉取性能监控

```bash
# ========== 性能指标收集 ==========
# 监控镜像拉取时间
cat <<'EOF' > image-pull-monitor.sh
#!/bin/bash

NAMESPACE=${1:-default}
INTERVAL=${2:-300}  # 5分钟间隔

echo "Starting image pull performance monitoring for namespace: $NAMESPACE"

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    # 获取正在拉取镜像的Pod
    kubectl get pods -n $NAMESPACE --field-selector=status.phase=Pending -o jsonpath='{
        range .items[?(@.status.containerStatuses[*].state.waiting.reason=="ContainerCreating")]
    }{
            .metadata.name
    }{
            "\t"
    }{
            .spec.containers[*].image
    }{
            "\n"
    }{
        end
    }' | while IFS=$'\t' read pod image; do
        # 记录开始时间
        START_TIME=$(kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.metadata.creationTimestamp}')
        CURRENT_TIME=$(date -u +"%Y-%m-%dT%H:%M:%SZ")
        
        # 计算等待时间
        START_SECONDS=$(date -d "$START_TIME" +%s)
        CURRENT_SECONDS=$(date -d "$CURRENT_TIME" +%s)
        WAIT_TIME=$((CURRENT_SECONDS - START_SECONDS))
        
        echo "$TIMESTAMP - Pod: $pod, Image: $image, Wait time: ${WAIT_TIME}s"
        
        # 告警阈值检查
        if [ $WAIT_TIME -gt 300 ]; then  # 超过5分钟
            echo "$TIMESTAMP - ALERT: Long image pull time for $pod ($WAIT_TIME seconds)"
        fi
    done
    
    sleep $INTERVAL
done
EOF

chmod +x image-pull-monitor.sh

# ========== 镜像大小分析 ==========
# 分析镜像层大小
cat <<'EOF' > image-size-analyzer.sh
#!/bin/bash

IMAGE=$1

if [ -z "$IMAGE" ]; then
    echo "Usage: $0 <image-name>"
    exit 1
fi

echo "Analyzing image: $IMAGE"

# 使用docker inspect分析镜像层次
docker pull $IMAGE
docker inspect $IMAGE | jq '.[0].RootFS.Layers' | wc -l

# 分析每层大小
docker history $IMAGE --format "table {{.Size}}\t{{.CreatedBy}}" | head -20

# 计算总大小
TOTAL_SIZE=$(docker images --format "{{.Size}}" $IMAGE)
echo "Total image size: $TOTAL_SIZE"

# 优化建议
if [[ $TOTAL_SIZE == *"GB"* ]] || [[ $(echo $TOTAL_SIZE | sed 's/GB//') -gt 1 ]]; then
    echo "⚠️  Image is quite large, consider optimization:"
    echo "  - Use multi-stage builds"
    echo "  - Remove unnecessary files"
    echo "  - Use smaller base images"
    echo "  - Combine RUN commands"
fi
EOF

chmod +x image-size-analyzer.sh
```

### 5.2 镜像缓存和分发优化

```bash
# ========== 镜像缓存配置 ==========
# 配置节点级镜像缓存
cat <<EOF > node-image-cache.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: image-cache
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: image-cache
  template:
    metadata:
      labels:
        app: image-cache
    spec:
      containers:
      - name: registry
        image: registry:2.8
        ports:
        - containerPort: 5000
        env:
        - name: REGISTRY_STORAGE_FILESYSTEM_ROOTDIRECTORY
          value: /var/lib/registry
        - name: REGISTRY_PROXY_REMOTEURL
          value: https://registry-1.docker.io
        volumeMounts:
        - name: registry-storage
          mountPath: /var/lib/registry
        - name: docker-config
          mountPath: /root/.docker
          readOnly: true
      volumes:
      - name: registry-storage
        hostPath:
          path: /var/lib/image-cache
          type: DirectoryOrCreate
      - name: docker-config
        secret:
          secretName: registry-auth
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: image-cache-config
  namespace: kube-system
data:
  config.yml: |
    version: 0.1
    log:
      fields:
        service: registry
    storage:
      cache:
        blobdescriptor: inmemory
      filesystem:
        rootdirectory: /var/lib/registry
    http:
      addr: :5000
      headers:
        X-Content-Type-Options: [nosniff]
    proxy:
      remoteurl: https://registry-1.docker.io
EOF

# ========== 镜像预热策略 ==========
# 创建镜像预热Job
cat <<EOF > image-preheat-job.yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: image-preheat
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: preheat
        image: docker:dind
        command:
        - /bin/sh
        - -c
        - |
          # 预拉取常用镜像列表
          IMAGES="
          nginx:latest
          redis:alpine
          postgres:13-alpine
          "
          
          for image in \$IMAGES; do
            echo "Preheating image: \$image"
            docker pull \$image
          done
          
          echo "Image preheating completed"
        securityContext:
          privileged: true
        volumeMounts:
        - name: dockersock
          mountPath: /var/run/docker.sock
      volumes:
      - name: dockersock
        hostPath:
          path: /var/run/docker.sock
      restartPolicy: Never
  backoffLimit: 3
EOF

# ========== 镜像分发优化 ==========
# 配置镜像分发网络策略
cat <<EOF > image-distribution-policy.yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: image-registry-access
  namespace: kube-system
spec:
  podSelector:
    matchLabels:
      app: registry-proxy
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector: {}
    ports:
    - protocol: TCP
      port: 5000
  egress:
  - to:
    - ipBlock:
        cidr: 0.0.0.0/0  # 允许访问外部镜像仓库
    ports:
    - protocol: TCP
      port: 443
    - protocol: TCP
      port: 80
EOF
```

---

## 6. 监控和告警配置 (Monitoring and Alerting)

### 6.1 镜像相关监控

```bash
# ========== 镜像拉取监控配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: image-pull-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      k8s-app: kubelet
  endpoints:
  - port: metrics
    path: /metrics
    interval: 30s
    metricRelabelings:
    - sourceLabels: [__name__]
      regex: 'container_image_pull.*'
      action: keep
EOF

# ========== 告警规则配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: image-registry-alerts
  namespace: monitoring
spec:
  groups:
  - name: image.rules
    rules:
    - alert: ImagePullFailure
      expr: kube_pod_container_status_waiting_reason{reason="ImagePullBackOff"} == 1
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Image pull failed for pod {{ \$labels.namespace }}/{{ \$labels.pod }}"
        
    - alert: RegistryUnreachable
      expr: rate(container_image_pull_failures_total[5m]) > 10
      for: 5m
      labels:
        severity: warning
      annotations:
        summary: "Image registry appears unreachable ({{ \$value }} failures/min)"
        
    - alert: ImageVulnerabilityDetected
      expr: security_scans_vulnerabilities{severity=~"HIGH|CRITICAL"} > 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "High severity vulnerabilities detected in scanned images"
        
    - alert: ImagePullTimeout
      expr: histogram_quantile(0.99, rate(container_image_pull_duration_seconds_bucket[5m])) > 300
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "Image pull operations taking longer than 5 minutes (99th percentile)"
        
    - alert: RegistryAuthenticationFailure
      expr: rate(container_image_pull_authentication_failures_total[5m]) > 5
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Image registry authentication failures ({{ \$value }}/min)"
EOF
```

### 6.2 镜像仓库健康检查

```bash
# ========== 仓库健康检查脚本 ==========
cat <<'EOF' > registry-health-check.sh
#!/bin/bash

REGISTRY_URL=${1:-https://registry-1.docker.io}
CHECK_INTERVAL=${2:-300}  # 5分钟检查一次

echo "Starting registry health check for: $REGISTRY_URL"

while true; do
    TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
    
    # 测试基础连通性
    if curl -s -o /dev/null -w "%{http_code}" $REGISTRY_URL/v2/ | grep -q "200"; then
        echo "$TIMESTAMP - Registry connectivity: ✓"
        
        # 测试认证端点
        if curl -s -o /dev/null -w "%{http_code}" $REGISTRY_URL/v2/token 2>/dev/null | grep -q "401\|200"; then
            echo "$TIMESTAMP - Authentication endpoint: ✓"
        else
            echo "$TIMESTAMP - Authentication endpoint: ✗"
        fi
        
        # 测试镜像清单API
        TEST_IMAGE="library/nginx"
        TEST_TAG="latest"
        if curl -s -H "Accept: application/vnd.docker.distribution.manifest.v2+json" \
           -o /dev/null -w "%{http_code}" \
           $REGISTRY_URL/v2/$TEST_IMAGE/manifests/$TEST_TAG 2>/dev/null | grep -q "401\|200"; then
            echo "$TIMESTAMP - Manifest API: ✓"
        else
            echo "$TIMESTAMP - Manifest API: ✗"
        fi
        
    else
        echo "$TIMESTAMP - Registry connectivity: ✗"
        echo "$TIMESTAMP - CRITICAL: Registry appears to be down"
    fi
    
    echo "---"
    sleep $CHECK_INTERVAL
done
EOF

chmod +x registry-health-check.sh

# ========== 镜像可用性验证 ==========
cat <<'EOF' > image-availability-check.sh
#!/bin/bash

IMAGE_LIST_FILE=${1:-images.txt}
NAMESPACE=${2:-default}

if [ ! -f "$IMAGE_LIST_FILE" ]; then
    echo "Image list file not found: $IMAGE_LIST_FILE"
    echo "Please create a file with one image per line"
    exit 1
fi

echo "Checking image availability for namespace: $NAMESPACE"

while IFS= read -r image; do
    echo "Checking image: $image"
    
    # 创建测试Pod
    cat <<TESTPOD | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: image-test-$(echo $image | tr '/' '-' | tr ':' '-')
  namespace: $NAMESPACE
spec:
  containers:
  - name: test
    image: $image
    command: ["sleep", "1"]
  restartPolicy: Never
TESTPOD

    # 等待Pod状态
    for i in {1..30}; do
        STATUS=$(kubectl get pod image-test-$(echo $image | tr '/' '-' | tr ':' '-') -n $NAMESPACE -o jsonpath='{.status.phase}' 2>/dev/null)
        if [ "$STATUS" = "Succeeded" ]; then
            echo "  ✓ Image available and runnable"
            break
        elif [ "$STATUS" = "Failed" ]; then
            REASON=$(kubectl get pod image-test-$(echo $image | tr '/' '-' | tr ':' '-') -n $NAMESPACE -o jsonpath='{.status.containerStatuses[0].state.terminated.reason}' 2>/dev/null)
            echo "  ✗ Image pull failed: $REASON"
            break
        fi
        sleep 10
    done
    
    # 清理测试Pod
    kubectl delete pod image-test-$(echo $image | tr '/' '-' | tr ':' '-') -n $NAMESPACE --ignore-not-found
    
done < "$IMAGE_LIST_FILE"

echo "Image availability check completed"
EOF

chmod +x image-availability-check.sh
```

---