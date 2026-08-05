---
title: Kubeflow 平台故障排查指南 [topic-structural-trouble-shooting]
description: 'title: Kubeflow 平台故障排查指南'
summary: 'title: Kubeflow 平台故障排查指南'
category: structural-troubleshooting
tags:
- troubleshooting
- guide
- kubelet
- controller-manager
- prometheus
- istio
- docker
- opa
- minio
- mysql
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- Kubeflow 平台故障排查指南 是什么
- 如何 Kubeflow 平台故障排查指南
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- Kubeflow 平台故障排查指南 故障排查
- Kubeflow 平台故障排查指南 排障步骤
trigger_keywords:
- Kubeflow
- 平台故障排查指南
- troubleshooting
- diagnostics
- structural
- trouble
- shooting
prerequisites:
- kubectl-basics
- pod-lifecycle
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- mysql-basics
- gpu-scheduling-basics
- tls-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: [[Kubeflow|Kubeflow]] 平台故障排查指南
description: '# Kubeflow 平台故障排查指南'
category: structural-troubleshooting
tags:
- k8s
- troubleshooting
- decision-tree
- [[kubelet|kubelet]]
- controller-manager
- [[Prometheus|prometheus]]
- istio
- opa
- minio
- mysql
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Kubeflow 平台故障排查指南 是什么
- 如何 Kubeflow 平台故障排查指南
- Kubeflow 平台故障排查指南 故障排查
- Kubeflow 平台故障排查指南 排障步骤
trigger_keywords:
- Kubeflow
- 平台故障排查指南
- structural
- trouble
- shooting
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Kubeflow 平台故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32 | Kubeflow v1.8+ | **最后更新**: 2026-04 | **难度**: 高级

---

## 0. 10 分钟快速诊断

1. **核心组件存活**：`kubectl get pods -n kubeflow`，确认所有 Pod 状态为 Running，特别关注 `kubeflow-pipelines`、`katib-controller`、`kserve-controller-manager`。
2. **认证授权**：访问 Kubeflow Central Dashboard，确认身份认证服务（Dex/OIDC/Auth0）正常工作，无登录循环。
3. **Pipeline 状态**：`kubectl get workflows -A`，查看是否有失败的 Argo Workflow。
4. **KServe 推理服务**：`kubectl get inferenceservices -A`，确认模型服务状态为 `Ready`。
5. **Notebook 服务器**：`kubectl get notebooks -A`，检查 Jupyter Notebook Pod 状态。
6. **快速缓解**：
   - Pipeline 卡住：删除失败的 Workflow 并重新提交。
   - Notebook 无法启动：检查 PVC 和镜像拉取状态。
   - KServe 模型加载失败：检查模型存储 URI 和 Secret 权限。
7. **证据留存**：保存 Kubeflow 组件日志、Workflow YAML、InferenceService 状态、Notebook 事件。

---

## 1. 问题现象与影响分析

### 1.1 常见问题现象

#### 1.1.1 Kubeflow Pipelines 问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Pipeline 运行失败 | `workflow failed with exit code 1` | Argo Workflow | `kubectl get workflows -A` |
| Pipeline UI 无法加载 | `Failed to load pipelines` | Kubeflow UI | 浏览器控制台 |
| 组件之间无法通信 | `dns error: service 'ml-pipeline' not found` | Pipeline 组件日志 | Pod 日志 |
| 持久化代理失败 | `persistentagent failed to create client` | Persistence Agent | `kubectl logs -n kubeflow deployment/ml-pipeline-persistenceagent` |
| 定时调度未触发 | `cron workflow not triggered` | Argo Events | `kubectl get cronworkflows -A` |

#### 1.1.2 Katib 超参数调优问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Experiment 失败 | `Experiment failed: suggestion service unavailable` | Katib Controller | `kubectl get experiments -A` |
| Trial 持续 Pending | `trial is pending because ...` | Katib Trial | `kubectl describe trial` |
| 建议算法报错 | `algorithm service error: ...` | Katib Suggestion | Suggestion Pod 日志 |
| 指标收集失败 | `metrics collector cannot parse result` | Metrics Collector | Sidecar 日志 |

#### 1.1.3 KServe 模型服务问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| 模型加载失败 | `Model loading failed: storage error` | KServe Agent | `kubectl logs inference-pod -c storage-initializer` |
| 推理服务未就绪 | `InferenceService not ready` | KServe Controller | `kubectl get inferenceservice` |
| 自动扩缩容异常 | `HPA cannot get metrics` | KServe + HPA | `kubectl describe hpa` |
| 金丝雀发布失败 | `traffic split failed` | KServe Controller | Controller 日志 |
| 解释器服务报错 | `explainer container error` | KServe Explainer | Explainer Pod 日志 |

#### 1.1.4 Notebook 服务器问题

| 现象 | 报错信息 | 报错来源 | 查看方式 |
|------|----------|----------|----------|
| Notebook 无法启动 | `notebook server failed to start` | Notebook Controller | `kubectl get notebooks -A` |
| PVC 挂载失败 | `MountVolume failed` | kubelet Events | `kubectl describe pod` |
| GPU 不可见 | `nvidia-smi not found` | Notebook Pod | Pod 内执行 |
| 镜像拉取失败 | `ImagePullBackOff` | kubelet | `kubectl get pod` |
| 内核连接断开 | `kernel connection lost` | Jupyter UI | 浏览器/Jupyter 日志 |

#### 1.1.5 生产环境典型场景

| 场景 | 典型现象 | 根本原因 | 解决方向 |
|------|----------|----------|----------|
| **大规模并行 Pipeline 崩溃** | 同时提交 100+ Pipeline 时 Argo Controller 卡死 | Argo Controller 并发限制默认过低 | 调大 `--workflow-workers` 和 `--pod-workers` |
| **Katib NAS 实验搜索空间过大** | Katib Suggestion Pod OOMKilled | 神经架构搜索内存消耗高 | 增加 Suggestion Pod 内存限制 |
| **KServe 大模型加载超时** | 部署 LLM（>10GB）时 InferenceService 始终未就绪 | Storage Initializer 默认超时 10 分钟不足 | 增加 `storage-initializer` 超时配置 |
| **Notebook 共享存储性能瓶颈** | 多用户 Notebook 同时训练时 I/O 延迟飙升 | 所有 Notebook 共享同一个 NFS/RWX PVC | 为每个用户配置独立的 PVC |

### 1.2 报错查看方式汇总

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Kubeflow 核心组件状态
kubectl get pods -n kubeflow -o wide

# Pipeline 工作流状态
kubectl get workflows -A -o wide
kubectl get cronworkflows -A

# Katib 实验状态
kubectl get experiments -A
kubectl get trials -A
kubectl get suggestions -A

# KServe 推理服务状态
kubectl get inferenceservices -A
kubectl get trainedmodels -A

# Notebook 状态
kubectl get notebooks -A
kubectl get pod -n kubeflow -l notebook-name

# 核心组件日志
kubectl logs -n kubeflow deployment/ml-pipeline-ui --tail=100
kubectl logs -n kubeflow deployment/katib-controller --tail=100
kubectl logs -n kubeflow deployment/kserve-controller-manager --tail=100
kubectl logs -n kubeflow deployment/notebook-controller-deployment --tail=100
```
---

## 2. 排查方法与步骤

### 2.1 诊断原理说明

Kubeflow 是一个复杂的 ML 平台，由多个独立组件组成：

```
┌─────────────────────────────────────────────────────────────────┐
│                    Kubeflow Central Dashboard                    │
│  (认证: Dex/OIDC + 授权: Istio AuthorizationPolicy)              │
├─────────────────────────────────────────────────────────────────┤
│  Kubeflow Pipelines         │  Katib Hyperparameter Tuning     │
│  - ml-pipeline (API)        │  - katib-controller              │
│  - ml-pipeline-ui           │  - katib-db-manager              │
│  - argo-workflow-controller │  - suggestion algorithms         │
│  - persistence-agent        │  - metrics-collector             │
├─────────────────────────────────────────────────────────────────┤
│  KServe Model Serving       │  Notebook Servers                │
│  - kserve-controller        │  - notebook-controller           │
│  - inference services       │  - jupyter-web-app               │
│  - storage-initializer      │  - pvcaccess-management          │
│  - transformers/explainers  │                                  │
├─────────────────────────────────────────────────────────────────┤
│  共享基础设施                                                   │
│  - MinIO/S3 (Artifact Store)  │  - MySQL (Pipeline DB)         │
│  - Istio Ingress Gateway      │  - Cert-manager (TLS)          │
└─────────────────────────────────────────────────────────────────┘
```

**关键依赖链**：
- Pipeline 依赖：Argo Workflows → MinIO/S3 → MySQL
- Katib 依赖：Katib Controller → Suggestion Services → Metrics Collector → PVC/ConfigMap
- KServe 依赖：KServe Controller → Knative/Istio → Storage Initializer → Model Storage
- Notebook 依赖：Notebook Controller → PVC → GPU Device Plugin（如使用 GPU）

### 2.2 排查逻辑决策树

```
Kubeflow 问题
    ├── Central Dashboard 无法访问
    │   ├── Istio Gateway 未就绪？──► 检查 istio-ingressgateway Pod
    │   ├── Dex/OIDC 认证失败？──► 检查 dex/auth 配置和 Secret
    │   └── 证书过期？──► 检查 cert-manager 和 TLS Secret
    ├── Pipeline 运行失败
    │   ├── Argo Workflow Controller 异常？──► 重启 controller，检查资源限制
    │   ├── 组件之间 DNS 不可达？──► 检查 kubeflow namespace DNS
    │   ├── Artifact 存储访问失败？──► 检查 MinIO/S3 凭证和连通性
    │   └── 容器镜像拉取失败？──► 检查 imagePullSecrets 和镜像仓库
    ├── Katib 实验失败
    │   ├── Suggestion Service 未就绪？──► 检查算法 Pod 状态
    │   ├── Trial Pod 资源不足？──► 调大 Trial 的 requests/limits
    │   ├── 指标解析失败？──► 检查 metrics collector 配置
    │   └── 数据库连接失败？──► 检查 katib-mysql Pod
    ├── KServe 模型服务失败
    │   ├── Storage Initializer 失败？──► 检查模型 URI 和访问权限
    │   ├── Predictor 容器启动失败？──► 检查模型格式和框架版本
    │   ├── 自动扩缩容不工作？──► 检查 HPA 和 metrics-server
    │   └── Transformer/Explainer 失败？──► 检查组件日志
    └── Notebook 无法启动
        ├── PVC 创建/挂载失败？──► 检查 StorageClass 和配额
        ├── GPU 不可见？──► 检查 Device Plugin 和节点标签
        ├── 镜像拉取失败？──► 检查镜像名称和 registry 认证
        └── 内核启动失败？──► 检查 Notebook 镜像中的 jupyter 安装
```

### 2.3 详细诊断命令

#### Kubeflow 全景诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# Kubeflow 全景诊断脚本

echo "=== Kubeflow 全景诊断 ==="

# 1. 核心命名空间 Pod 状态
echo "1. Kubeflow 命名空间 Pod 状态:"
kubectl get pods -n kubeflow -o json | jq -r '
  .items[] | select(.status.phase != "Running" or (.status.containerStatuses[]? | .ready == false)) |
  "  \(.metadata.name): phase=\(.status.phase), restarts=\(.status.containerStatuses[0].restartCount // 0)"
'

# 2. Pipeline 系统状态
echo ""
echo "2. Pipeline 系统状态:"
echo "  Workflows (最近 10 个):"
kubectl get workflows -A --sort-by='.metadata.creationTimestamp' | tail -10

echo ""
echo "  失败的 Workflows:"
kubectl get workflows -A -o json | jq -r '
  .items[] | select(.status.phase == "Failed") |
  "  \(.metadata.namespace)/\(.metadata.name): failedAt=\(.status.finishedAt // "unknown")"
' | tail -10

# 3. Katib 实验状态
echo ""
echo "3. Katib 实验状态:"
kubectl get experiments -A -o json | jq -r '
  .items[] |
  "  \(.metadata.namespace)/\(.metadata.name): type=\(.spec.type), status=\(.status.conditions[-1].type // "unknown")"
'

# 4. KServe 推理服务
echo ""
echo "4. KServe 推理服务状态:"
kubectl get inferenceservices -A -o json | jq -r '
  .items[] |
  "  \(.metadata.namespace)/\(.metadata.name): status=\(.status.conditions[-1].status // "unknown")"
'

# 5. Notebook 服务器
echo ""
echo "5. Notebook 服务器状态:"
kubectl get notebooks -A -o json | jq -r '
  .items[] |
  "  \(.metadata.namespace)/\(.metadata.name): ready=\(.status.readyReplicas // 0)/\(.spec.replicas // 1)"
'

# 6. 共享基础设施
echo ""
echo "6. 共享基础设施状态:"
echo "  MinIO/S3:"
kubectl get pods -n kubeflow -l app=minio -o jsonpath='{.items[*].status.phase}' 2>/dev/null || \
  echo "    未发现 MinIO Pod，可能使用外部 S3"

echo ""
echo "  MySQL:"
kubectl get pods -n kubeflow -l app=mysql -o jsonpath='{.items[*].status.phase}' 2>/dev/null || \
  echo "    未发现 MySQL Pod"

echo ""
echo "  Istio IngressGateway:"
kubectl get pods -n istio-system -l app=istio-ingressgateway -o jsonpath='{.items[*].status.phase}'
```
#### Pipeline 问题深度诊断

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# Pipeline 问题深度诊断脚本
# 用法: ./diagnose-pipeline.sh <workflow-name> <namespace>

WORKFLOW_NAME=${1:-""}
NAMESPACE=${2:-"kubeflow-user-example-com"}

if [ -z "$WORKFLOW_NAME" ]; then
  echo "用法: $0 <workflow-name> [namespace]"
  exit 1
fi

echo "=== Pipeline Workflow $NAMESPACE/$WORKFLOW_NAME 深度诊断 ==="

# 1. Workflow 总体状态
echo "1. Workflow 状态:"
kubectl get workflow $WORKFLOW_NAME -n $NAMESPACE -o json | jq -r '
  {
    phase: .status.phase,
    startedAt: .status.startedAt,
    finishedAt: .status.finishedAt,
    progress: .status.progress,
    message: .status.message
  }'

# 2. 各步骤状态
echo ""
echo "2. Workflow 步骤状态:"
kubectl get workflow $WORKFLOW_NAME -n $NAMESPACE -o json | jq -r '
  .status.nodes | to_entries[] |
  "  \(.value.displayName // .key): phase=\(.value.phase), type=\(.value.type)"
'

# 3. 失败步骤的日志
echo ""
echo "3. 失败步骤日志:"
FAILED_PODS=$(kubectl get workflow $WORKFLOW_NAME -n $NAMESPACE -o json | \
  jq -r '.status.nodes | to_entries[] | select(.value.phase == "Failed") | .value.id')

for pod_id in $FAILED_PODS; do
  echo "--- Pod: $pod_id ---"
  kubectl logs $pod_id -n $NAMESPACE --tail=50 2>/dev/null | tail -20
  echo ""
done

# 4. Artifact 存储连通性
echo ""
echo "4. Artifact 存储连通性:"
# 检查 ml-pipeline 是否有访问 MinIO/S3 的权限
kubectl exec -n kubeflow deployment/ml-pipeline -- \
  wget -qO- http://minio-service.kubeflow.svc.cluster.local:9000/minio/health/live 2>/dev/null || \
  echo "  ⚠ 无法连接到 MinIO"

# 5. Pipeline UI 可访问性
echo ""
echo "5. Pipeline UI 服务状态:"
kubectl get svc ml-pipeline-ui -n kubeflow -o json | jq -r '{clusterIP: .spec.clusterIP, ports: .spec.ports}'
```
#### KServe 推理服务诊断

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# KServe 推理服务诊断脚本
# 用法: ./diagnose-kserve.sh <inferenceservice-name> <namespace>

ISVC_NAME=${1:-""}
NAMESPACE=${2:-"kubeflow-user-example-com"}

if [ -z "$ISVC_NAME" ]; then
  echo "用法: $0 <inferenceservice-name> [namespace]"
  exit 1
fi

echo "=== KServe InferenceService $NAMESPACE/$ISVC_NAME 诊断 ==="

# 1. InferenceService 状态
echo "1. InferenceService 状态:"
kubectl get inferenceservice $ISVC_NAME -n $NAMESPACE -o json | jq -r '
  {
    url: .status.url,
    ready: .status.conditions[-1].status,
    reason: .status.conditions[-1].reason,
    message: .status.conditions[-1].message
  }'

# 2. 关联的 Knative Service/Revision
echo ""
echo "2. 关联 Knative 资源:"
kubectl get ksvc -n $NAMESPACE -l serving.kserve.io/inferenceservice=$ISVC_NAME
kubectl get revision -n $NAMESPACE -l serving.kserve.io/inferenceservice=$ISVC_NAME -o json | jq -r '
  .items[] | "  \(.metadata.name): ready=\(.status.conditions[-1].status)"
'

# 3. Predictor Pod 状态
echo ""
echo "3. Predictor Pod 状态:"
kubectl get pods -n $NAMESPACE -l serving.kserve.io/inferenceservice=$ISVC_NAME -o json | jq -r '
  .items[] | "  \(.metadata.name): phase=\(.status.phase), ready=\(.status.containerStatuses[0].ready)"
'

# 4. Storage Initializer 日志
echo ""
echo "4. Storage Initializer 日志:"
for pod in $(kubectl get pods -n $NAMESPACE -l serving.kserve.io/inferenceservice=$ISVC_NAME -o name); do
  echo "=== $pod storage-initializer ==="
  kubectl logs -n $NAMESPACE $pod -c storage-initializer --tail=50 2>/dev/null | tail -15
done

# 5. 模型加载后的 Predictor 日志
echo ""
echo "5. Predictor 容器日志:"
for pod in $(kubectl get pods -n $NAMESPACE -l serving.kserve.io/inferenceservice=$ISVC_NAME -o name); do
  echo "=== $pod predictor ==="
  kubectl logs -n $NAMESPACE $pod -c kserve-container --tail=50 2>/dev/null | tail -15
done

# 6. 推理测试
echo ""
echo "6. 推理请求测试:"
ISVC_URL=$(kubectl get inferenceservice $ISVC_NAME -n $NAMESPACE -o jsonpath='{.status.url}' 2>/dev/null)
if [ -n "$ISVC_URL" ]; then
  # 通过 Istio IngressGateway 测试
  INGRESS_HOST=$(kubectl get svc istio-ingressgateway -n istio-system -o jsonpath='{.status.loadBalancer.ingress[0].ip}' 2>/dev/null)
  if [ -n "$INGRESS_HOST" ]; then
    echo "  测试 URL: http://$INGRESS_HOST/v1/models/$ISVC_NAME:predict"
    curl -s -o /dev/null -w "HTTP Status: %{http_code}\n" \
      -H "Content-Type: application/json" \
      -H "Host: $ISVC_URL" \
      http://$INGRESS_HOST/v1/models/$ISVC_NAME:predict 2>/dev/null || \
      echo "  请求失败"
  fi
else
  echo "  InferenceService URL 未生成"
fi
```
---

## 3. 解决方案与风险控制

### 3.1 Kubeflow Pipelines 优化

#### 方案一：Argo Workflow Controller 性能调优

```yaml
# Argo Workflow Controller 性能调优
apiVersion: v1
kind: ConfigMap
metadata:
  name: workflow-controller-configmap
  namespace: kubeflow
data:
  config: |
    {
      "executorImage": "gcr.io/ml-pipeline/argoexec:v3.4.16",
      "containerRuntimeExecutor": "emissary",
      "artifactRepository": {
        "s3": {
          "bucket": "mlpipeline",
          "endpoint": "minio-service.kubeflow.svc.cluster.local:9000",
          "insecure": true,
          "accessKeySecret": {
            "name": "mlpipeline-minio-artifact",
            "key": "accesskey"
          },
          "secretKeySecret": {
            "name": "mlpipeline-minio-artifact",
            "key": "secretkey"
          }
        }
      },
      "parallelism": 100,           # 全局最大并发 Workflow 数
      "namespaceParallelism": 50,   # 每个 namespace 最大并发数
      "nodeEvents": {
        "enabled": true
      }
    }
---
# Workflow Controller Deployment 资源调优
apiVersion: apps/v1
kind: Deployment
metadata:
  name: workflow-controller
  namespace: kubeflow
spec:
  template:
    spec:
      containers:
      - name: workflow-controller
        image: gcr.io/ml-pipeline/workflow-controller:v3.4.16
        args:
        - --configmap=workflow-controller-configmap
        - --executor-image=gcr.io/ml-pipeline/argoexec:v3.4.16
        - --namespaced
        - --workflow-workers=64        # 默认 32，调大处理更多并发
        - --pod-workers=32             # 默认 8，调大加速 Pod 处理
        - --qps=20                     # API Server QPS
        - --burst=30                   # API Server Burst
        resources:
          limits:
            cpu: "2"
            memory: "4Gi"
          requests:
            cpu: "1"
            memory: "2Gi"
```

#### 方案二：Pipeline 组件高可用配置

```yaml
# ML Pipeline API Server 高可用配置
apiVersion: apps/v1
kind: Deployment
metadata:
  name: ml-pipeline
  namespace: kubeflow
spec:
  replicas: 2                    # 多副本高可用
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  template:
    spec:
      containers:
      - name: ml-pipeline-api-server
        image: gcr.io/ml-pipeline/api-server:2.0.5
        args:
        - --config=/config
        env:
        - name: OBJECTSTORECONFIG_SECURE
          value: "false"
        - name: DBCONFIG_HOST
          value: "mysql.kubeflow.svc.cluster.local"
        - name: DBCONFIG_PORT
          value: "3306"
        resources:
          limits:
            cpu: "1"
            memory: "2Gi"
          requests:
            cpu: "500m"
            memory: "1Gi"
---
# MySQL 高可用（如使用自建 MySQL）
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: mysql
  namespace: kubeflow
spec:
  serviceName: mysql
  replicas: 1  # 生产环境建议使用云托管 RDS 或部署 MySQL 主从
  template:
    spec:
      containers:
      - name: mysql
        image: mysql:8.0.33
        env:
        - name: MYSQL_ALLOW_EMPTY_PASSWORD
          value: "true"
        resources:
          limits:
            cpu: "2"
            memory: "4Gi"
          requests:
            cpu: "1"
            memory: "2Gi"
        volumeMounts:
        - name: mysql-persistent-storage
          mountPath: /var/lib/mysql
  volumeClaimTemplates:
  - metadata:
      name: mysql-persistent-storage
    spec:
      accessModes: ["ReadWriteOnce"]
      storageClassName: fast-ssd
      resources:
        requests:
          storage: 50Gi
```

### 3.2 Katib 优化配置

```yaml
# Katib Experiment 资源优化示例
apiVersion: kubeflow.org/v1beta1
kind: Experiment
metadata:
  namespace: kubeflow-user-example-com
  name: gpu-hpo-experiment
spec:
  parallelTrialCount: 4          # 并行 Trial 数
  maxTrialCount: 20              # 最大 Trial 数
  maxFailedTrialCount: 3         # 允许失败次数
  objective:
    type: maximize
    goal: 0.99
    objectiveMetricName: accuracy
  algorithm:
    algorithmName: random
  metricsCollectorSpec:
    collector:
      kind: StdOut
  trialTemplate:
    primaryContainerName: training-container
    trialParameters:
    - name: learningRate
      description: Learning rate for the training model
      reference: lr
    - name: batchSize
      description: Batch size for the training model
      reference: batch-size
    trialSpec:
      apiVersion: batch/v1
      kind: Job
      spec:
        template:
          spec:
            containers:
            - name: training-container
              image: kubeflowkatib/pytorch-mnist:v1beta1-45c5727
              command:
              - "python3"
              - "/opt/pytorch-mnist/mnist.py"
              - "--epochs=1"
              - "--lr=${trialParameters.learningRate}"
              - "--batch-size=${trialParameters.batchSize}"
              resources:
                limits:
                  nvidia.com/gpu: "1"
                  memory: "8Gi"
                  cpu: "4"
                requests:
                  nvidia.com/gpu: "1"
                  memory: "4Gi"
                  cpu: "2"
            restartPolicy: Never
```

### 3.3 KServe 模型服务优化

```yaml
# KServe InferenceService 高级配置
apiVersion: serving.kserve.io/v1beta1
kind: InferenceService
metadata:
  name: llm-service
  namespace: kubeflow-user-example-com
  annotations:
    # 自定义 Storage Initializer 超时（大模型需要更长时间下载）
    serving.kserve.io/deploymentMode: Serverless
spec:
  predictor:
    model:
      modelFormat:
        name: huggingface
      storageUri: s3://model-bucket/llm-model/
      resources:
        limits:
          nvidia.com/gpu: "1"
          memory: "24Gi"
          cpu: "8"
        requests:
          nvidia.com/gpu: "1"
          memory: "16Gi"
          cpu: "4"
      # Storage Initializer 环境变量
      env:
      - name: STORAGE_URI
        value: s3://model-bucket/llm-model/
      - name: STORAGE_INITIALIZER_TIMEOUT
        value: "1800"  # 30 分钟超时
    # 自动扩缩容配置
    minReplicas: 1
    maxReplicas: 5
    containerConcurrency: 1  # 每个容器同时处理 1 个请求（LLM 场景）
    scaleMetric: concurrency
    scaleTarget: 1
  transformer:
    containers:
    - name: transformer
      image: my-registry/llm-transformer:v1.0
      resources:
        limits:
          memory: "4Gi"
          cpu: "2"
  explainer:
    containers:
    - name: explainer
      image: my-registry/llm-explainer:v1.0
      resources:
        limits:
          memory: "8Gi"
          cpu: "4"
---
# KServe 大模型加载优化 Secret
apiVersion: v1
kind: Secret
metadata:
  name: s3-model-credentials
  namespace: kubeflow-user-example-com
  annotations:
    serving.kserve.io/s3-endpoint: s3.amazonaws.com
    serving.kserve.io/s3-region: us-east-1
    serving.kserve.io/s3-usehttps: "1"
type: Opaque
stringData:
  AWS_ACCESS_KEY_ID: "your-access-key"
  AWS_SECRET_ACCESS_KEY: "your-secret-key"
```

### 3.4 Notebook 服务器优化

```yaml
# GPU Notebook 服务器配置
apiVersion: kubeflow.org/v1
kind: Notebook
metadata:
  name: gpu-notebook
  namespace: kubeflow-user-example-com
spec:
  template:
    spec:
      containers:
      - name: notebook
        image: jupyter/datascience-notebook:cuda-latest
        resources:
          limits:
            nvidia.com/gpu: "1"
            memory: "16Gi"
            cpu: "8"
          requests:
            nvidia.com/gpu: "1"
            memory: "8Gi"
            cpu: "4"
        volumeMounts:
        - name: workspace
          mountPath: /home/jovyan/workspace
        - name: data
          mountPath: /home/jovyan/data
      volumes:
      - name: workspace
        persistentVolumeClaim:
          claimName: workspace-pvc
      - name: data
        persistentVolumeClaim:
          claimName: data-pvc
      affinity:
        nodeAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
            nodeSelectorTerms:
            - matchExpressions:
              - key: nvidia.com/gpu.product
                operator: In
                values:
                - "NVIDIA-A100-SXM4-80GB"
      tolerations:
      - key: nvidia.com/gpu
        operator: Exists
        effect: NoSchedule
---
# 用户 PVC 配置
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: workspace-pvc
  namespace: kubeflow-user-example-com
spec:
  accessModes:
  - ReadWriteOnce
  storageClassName: fast-ssd
  resources:
    requests:
      storage: 50Gi
```

### 3.5 风险控制与回滚

| 操作 | 风险等级 | 影响评估 | 回滚方案 |
|------|---------|---------|---------|
| 升级 Kubeflow 版本 | ⭐⭐⭐ 高 | 可能导致组件不兼容 | 使用 kustomize overlay 备份原配置 |
| 修改 Argo Controller 参数 | ⭐⭐ 中 | 可能影响运行中的 Workflow | 恢复原始 ConfigMap |
| 调整 KServe 模型版本 | ⭐⭐ 中 | 推理服务可能短暂中断 | 使用 KServe 金丝雀回滚 |
| 删除失败的 Workflow | ⭐ 低 | 释放资源，不可恢复 | 无需回滚，可重新提交 |
| 调整 Katib 搜索空间 | ⭐ 低 | 仅影响新 Experiment | 恢复原始 Experiment YAML |
| 修改 Notebook 镜像 | ⭐ 低 | 新启动的 Notebook 使用新镜像 | 恢复原始 Notebook CR |

### 3.6 验证与监控

#### Kubeflow 健康检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# Kubeflow 健康检查脚本

REPORT_FILE="/var/log/kubernetes/kubeflow-health-$(date +%Y%m%d-%H%M%S).log"

echo "=== Kubeflow 健康检查 $(date) ===" | tee $REPORT_FILE

# 1. 核心组件健康
COMPONENTS=("ml-pipeline" "ml-pipeline-ui" "katib-controller" "kserve-controller-manager" "notebook-controller-deployment")
for comp in "${COMPONENTS[@]}"; do
  STATUS=$(kubectl get deployment $comp -n kubeflow -o jsonpath='{.status.conditions[?(@.type=="Available")].status}' 2>/dev/null)
  if [ "$STATUS" = "True" ]; then
    echo "✓ $comp: Available" | tee -a $REPORT_FILE
  else
    echo "✗ $comp: Not Available" | tee -a $REPORT_FILE
  fi
done

# 2. Pipeline 成功率（最近 24 小时）
echo "" | tee -a $REPORT_FILE
echo "2. Pipeline Workflow 成功率:" | tee -a $REPORT_FILE
TOTAL_WORKFLOWS=$(kubectl get workflows -A --field-selector=status.phase!=Pending -o json 2>/dev/null | jq '.items | length')
SUCCEEDED_WORKFLOWS=$(kubectl get workflows -A --field-selector=status.phase=Succeeded -o json 2>/dev/null | jq '.items | length')
if [ "$TOTAL_WORKFLOWS" -gt 0 ] 2>/dev/null; then
  SUCCESS_RATE=$(echo "scale=2; $SUCCEEDED_WORKFLOWS / $TOTAL_WORKFLOWS * 100" | bc)
  echo "  成功: $SUCCEEDED_WORKFLOWS / $TOTAL_WORKFLOWS (${SUCCESS_RATE}%)" | tee -a $REPORT_FILE
else
  echo "  近期无 Workflow" | tee -a $REPORT_FILE
fi

# 3. KServe 推理服务健康
echo "" | tee -a $REPORT_FILE
echo "3. KServe 推理服务:" | tee -a $REPORT_FILE
kubectl get inferenceservices -A -o json 2>/dev/null | jq -r '
  .items[] | "  \(.metadata.namespace)/\(.metadata.name): ready=\(.status.conditions[-1].status // "unknown")"
' | tee -a $REPORT_FILE

# 4. Notebook 运行状态
echo "" | tee -a $REPORT_FILE
echo "4. Notebook 服务器:" | tee -a $REPORT_FILE
kubectl get notebooks -A -o json 2>/dev/null | jq -r '
  .items[] | "  \(.metadata.namespace)/\(.metadata.name): \(.status.readyReplicas // 0)/\(.spec.replicas // 1) ready"
' | tee -a $REPORT_FILE

echo "" | tee -a $REPORT_FILE
echo "报告已保存: $REPORT_FILE" | tee -a $REPORT_FILE
```
#### Prometheus 监控告警

```yaml
# Kubeflow 监控告警
groups:
- name: kubeflow
  rules:
  - alert: KubeflowComponentDown
    expr: |
      kube_deployment_status_replicas_available{namespace="kubeflow"} < 1
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "Kubeflow 组件不可用"
      description: "Deployment {{ $labels.deployment }} 在 kubeflow 命名空间中没有可用副本"

  - alert: KubeflowPipelineFailed
    expr: |
      argo_workflows_count{status="Failed", namespace=~"kubeflow.*"} > 0
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Kubeflow Pipeline 运行失败"
      description: "命名空间 {{ $labels.namespace }} 中有失败的 Pipeline Workflow"

  - alert: KatibExperimentFailed
    expr: |
      katib_experiment_status{status="Failed"} == 1
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Katib 实验失败"
      description: "Katib Experiment {{ $labels.experiment_name }} 运行失败"

  - alert: KServeInferenceServiceNotReady
    expr: |
      kserve_inference_service_ready{namespace=~"kubeflow.*"} == 0
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "KServe 推理服务未就绪"
      description: "InferenceService {{ $labels.name }} 在 {{ $labels.namespace }} 中未就绪"

  - alert: NotebookServerDown
    expr: |
      kube_statefulset_status_replicas_ready{namespace=~"kubeflow.*", statefulset=~".*notebook.*"} <
      kube_statefulset_replicas{namespace=~"kubeflow.*", statefulset=~".*notebook.*"}
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Notebook 服务器未就绪"
      description: "Notebook StatefulSet {{ $labels.statefulset }} 的副本未全部就绪"
```

### 3.7 最佳实践

1. **存储分离**：将 Artifact Store（MinIO/S3）、模型存储、Notebook 工作目录使用不同的存储后端和 StorageClass
2. **Namespace 隔离**：为每个团队/用户分配独立的 Kubeflow Profile/Namespace，实现资源隔离和配额管理
3. **Pipeline 版本管理**：使用 Pipeline 版本功能，避免直接修改生产 Pipeline，保留回滚能力
4. **KServe 预热**：对大模型部署，使用 `minReplicas: 1` 保持常驻实例，避免冷启动延迟
5. **GPU 配额管理**：为 Namespace 配置 ResourceQuota 限制 GPU 使用量，防止单个用户耗尽集群 GPU
6. **定期清理**：建立 Workflow、过期 Trial、旧模型版本的自动清理策略
7. **Istio 调优**：大规模 Kubeflow 部署时，调大 Istio Proxy 的 `concurrency` 和连接池参数

### 典型问题案例

#### 案例一：Pipeline 并发过高导致 Argo Controller 崩溃

**问题描述**：同时提交 200+ Pipeline 后，Argo Workflow Controller Pod 频繁 OOMKilled。

**根本原因**：Controller 默认 `workflow-workers=32`，大量并发 Workflow 导致内存耗尽。

**解决方案**：
1. 将 Controller 内存限制从 2Gi 提升到 8Gi
2. 调大 `workflow-workers=64` 和 `pod-workers=32`
3. 在 Pipeline 层面限制并发数，使用 `parallelism` 参数

#### 案例二：KServe 部署 LLM 时 Storage Initializer 反复失败

**问题描述**：部署 20GB 的 HuggingFace 模型时，InferenceService 始终处于 `Pending`。

**根本原因**：KServe Storage Initializer 默认使用 `s3cmd` 下载，大模型下载时间超过默认超时。

**解决方案**：
1. 在 InferenceService 注解中增加 `storage-initializer` 超时
2. 使用模型预热 Job 预先下载模型到共享 PVC
3. 配置 KServe 使用 `huggingface` runtime 的缓存功能

#### 案例三：Notebook 内核频繁断开

**问题描述**：用户在 Jupyter Notebook 中运行训练时，内核连接反复断开。

**根本原因**：Notebook Pod 的内存限制（4Gi）不足，训练过程中触发 OOMKilled。

**解决方案**：
1. 将 Notebook 内存限制提升到 16Gi
2. 在 Notebook 中配置 `PYTORCH_CUDA_ALLOC_CONF` 限制 GPU 内存使用
3. 启用 kubeflow-profile 级别的 ResourceQuota 监控告警

## Related

- 08-docker-troubleshooting-guide
- 16-troubleshooting-guide
- [[domain-17-system-foundation/速查卡/go.md|go]]
- [[domain-17-system-foundation/速查卡/sql.md|sql]]
- [[domain-17-system-foundation/速查卡/k8s.md|k8s]]
- [[domain-19-landscape-references/领域索引/ai-gpu-index.md|AI / GPU 基础设施知识图谱索引]]
- [[domain-19-landscape-references/领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]

## See Also

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-ai-ml-workloads/03-mpi-operator-troubleshooting|03-mpi-operator-troubleshooting]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-ai-ml-workloads/01-ai-ml-workloads-troubleshooting|01-ai-ml-workloads-troubleshooting]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-ai-ml-workloads/03-mpi-operator-troubleshooting|03-mpi-operator-troubleshooting]]
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/12-ai-ml-workloads/01-ai-ml-workloads-troubleshooting|01-ai-ml-workloads-troubleshooting]]

```

<!-- risk-assessed -->
