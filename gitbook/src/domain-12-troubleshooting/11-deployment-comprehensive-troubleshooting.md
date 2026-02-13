# 11 - Deployment 全面故障排查 (Deployment Comprehensive Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **专家级别**: ⭐⭐⭐⭐ | **参考**: [Kubernetes Deployment官方文档](https://kubernetes.io/docs/concepts/workloads/controllers/deployment/)

## 🎯 本文档价值

本文档专注于生产环境Deployment故障的系统性诊断和处理，提供：
- **完整的问题分类体系**：从滚动更新失败到回滚异常的全流程覆盖
- **实战诊断方法**：基于真实生产案例的故障分析技巧
- **自动化工具集**：可直接使用的诊断脚本和检查清单
- **预防性最佳实践**：避免常见Deployment问题的配置建议

---

## 相关文档交叉引用

### 🔗 关联故障排查文档
- **[05-Pod Pending诊断](./05-pod-pending-diagnosis.md)** - Pod调度相关问题
- **[08-Pod综合故障排查](./08-pod-comprehensive-troubleshooting.md)** - Pod生命周期问题
- **[17-HPA/VPA故障排查](./17-hpa-vpa-troubleshooting.md)** - 自动扩缩容相关问题
- **[34-升级迁移故障排查](./34-upgrade-migration-troubleshooting.md)** - 版本升级兼容性问题

### 📚 扩展学习资料
- **[Kubernetes控制器模式](../domain-2-design-principles/03-controller-pattern.md)** - 理解Deployment控制器原理
- **[资源版本控制](../domain-2-design-principles/06-resource-version-control.md)** - 资源状态管理机制

---

## 1. Deployment 状态诊断 (Status Diagnosis)

### 1.1 Deployment 状态速查

| 状态 | 含义 | 检查项 |
|:---|:---|:---|
| **Available** | 可用副本数达标 | 正常 |
| **Progressing** | 正在更新中 | 等待或检查卡住原因 |
| **ReplicaFailure** | 副本创建失败 | Pod创建问题 |
| **Stalled** | 更新卡住 | progressDeadlineSeconds |

### 1.2 排查流程图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      Deployment 故障排查流程                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────────────┐                                                   │
│   │ Deployment异常      │                                                   │
│   └──────────┬──────────┘                                                   │
│              │                                                              │
│              ▼                                                              │
│   ┌────────────────────────────────────────────────────┐                   │
│   │ kubectl get deployment <name> -n <ns>              │                   │
│   │ kubectl describe deployment <name> -n <ns>         │                   │
│   └────────────────────────────────────────────────────┘                   │
│              │                                                              │
│              ├─── READY 0/N ──▶ Pod创建问题                                 │
│              │         └─▶ 检查ReplicaSet/Pod                              │
│              │                                                              │
│              ├─── 更新卡住 ──▶ 镜像/资源/调度问题                           │
│              │         └─▶ kubectl rollout status                          │
│              │                                                              │
│              ├─── 副本数不对 ──▶ HPA/资源配额/节点                          │
│              │         └─▶ kubectl get hpa/quota                           │
│              │                                                              │
│              └─── 回滚失败 ──▶ 检查rollout history                          │
│                       └─▶ kubectl rollout undo                             │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 副本数异常排查 (Replica Issues)

### 2.1 副本为0的原因

```bash
# === 检查Deployment ===
kubectl get deployment <name> -n <namespace>
kubectl describe deployment <name> -n <namespace>

# === 检查ReplicaSet ===
kubectl get rs -n <namespace> -l app=<app-label>
kubectl describe rs <rs-name> -n <namespace>

# === 常见原因 ===
# 1. replicas设置为0
kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.replicas}'

# 2. HPA缩到0
kubectl get hpa -n <namespace>

# 3. 暂停状态
kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.paused}'
```

### 2.2 副本数不足原因

| 原因 | 检查方法 | 解决方案 |
|:---|:---|:---|
| **资源不足** | `kubectl describe pod` | 扩容节点/调整requests |
| **节点不可调度** | `kubectl get nodes` | uncordon节点 |
| **PVC未绑定** | `kubectl get pvc` | 创建PV/检查StorageClass |
| **镜像拉取失败** | Pod Events | 检查镜像/凭证 |
| **ResourceQuota** | `kubectl describe quota` | 调整配额 |
| **LimitRange** | `kubectl get limitrange` | 调整限制 |

### 2.3 生产环境典型场景

#### 场景1：大促期间Deployment扩容失败
```yaml
# ❌ 问题现象
# Deployment在流量高峰时无法扩容到期望副本数

# 🔍 诊断步骤
# 1. 检查HPA配置和指标
kubectl get hpa -n production
kubectl get --raw "/apis/metrics.k8s.io/v1beta1/namespaces/production/pods" | jq

# 2. 检查节点资源
kubectl top nodes | sort -k4 -n  # 按内存使用率排序

# 3. 检查集群自动扩缩容状态
kubectl get configmap cluster-autoscaler-status -n kube-system -o yaml

# ✅ 解决方案
# 预先配置足够的节点池容量
# 优化HPA配置，避免过度敏感的扩缩策略
```

#### 场景2：金丝雀发布过程中回滚失败
```bash
# ❌ 问题现象
# Deployment更新卡在50%进度，无法完成也无法回滚

# 🔍 诊断步骤
# 1. 检查Deployment状态详情
kubectl rollout status deployment/my-app -n production --timeout=30s

# 2. 查看历史版本
kubectl rollout history deployment/my-app -n production

# 3. 检查Pod事件和日志
kubectl get events --sort-by=.lastTimestamp -n production | grep my-app
kubectl logs -l app=my-app -n production --tail=100

# ✅ 解决方案
# 强制重启：kubectl rollout restart deployment/my-app -n production
# 手动回滚：kubectl rollout undo deployment/my-app -n production --to-revision=3
```

### 2.4 副本数超出预期

```bash
# === 检查HPA ===
kubectl get hpa <name> -n <namespace>
kubectl describe hpa <name> -n <namespace>

# === 检查是否有多个Deployment ===
kubectl get deployment -n <namespace> -l app=<label>

# === 检查ReplicaSet ===
kubectl get rs -n <namespace> -l app=<label>
```

---

## 3. 滚动更新问题 (Rolling Update Issues)

### 3.1 更新卡住排查

```bash
# === 检查rollout状态 ===
kubectl rollout status deployment/<name> -n <namespace>

# === 查看更新进度 ===
kubectl get deployment <name> -n <namespace> -o jsonpath='{.status.conditions}'

# === 检查新旧ReplicaSet ===
kubectl get rs -n <namespace> -l app=<label>
kubectl describe rs <new-rs> -n <namespace>

# === 查看新Pod状态 ===
kubectl get pods -n <namespace> -l app=<label> --sort-by=.metadata.creationTimestamp
```

### 3.2 更新卡住的原因

| 原因 | 症状 | 解决方案 |
|:---|:---|:---|
| **镜像拉取失败** | ImagePullBackOff | 检查镜像名/凭证 |
| **资源不足** | Pending | 扩容/调整requests |
| **探针失败** | Pod不Ready | 调整探针配置 |
| **应用启动慢** | Pod启动中 | 调整initialDelaySeconds |
| **maxUnavailable=0** | 无法替换 | 调整策略参数 |

### 3.3 滚动更新参数

```yaml
spec:
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 25%        # 最多超出期望副本数
      maxUnavailable: 25%  # 最多不可用副本数
  minReadySeconds: 10      # Pod Ready后等待时间
  progressDeadlineSeconds: 600  # 更新超时时间
```

**参数影响**:

| 参数 | 设置 | 影响 |
|:---|:---|:---|
| maxSurge=0, maxUnavailable=1 | 逐个替换 | 慢但安全 |
| maxSurge=1, maxUnavailable=0 | 先创建再删除 | 需要额外资源 |
| maxSurge=25%, maxUnavailable=25% | 默认值 | 平衡速度和稳定 |

---

## 4. 回滚操作 (Rollback)

### 4.1 回滚命令

```bash
# === 查看历史版本 ===
kubectl rollout history deployment/<name> -n <namespace>

# === 查看特定版本详情 ===
kubectl rollout history deployment/<name> -n <namespace> --revision=2

# === 回滚到上一版本 ===
kubectl rollout undo deployment/<name> -n <namespace>

# === 回滚到指定版本 ===
kubectl rollout undo deployment/<name> -n <namespace> --to-revision=2

# === 检查回滚状态 ===
kubectl rollout status deployment/<name> -n <namespace>
```

### 4.2 回滚失败排查

```bash
# === 检查回滚是否成功 ===
kubectl rollout status deployment/<name> -n <namespace>

# === 如果回滚卡住 ===
# 1. 检查新Pod状态
kubectl get pods -n <namespace> -l app=<label>

# 2. 检查Events
kubectl describe deployment <name> -n <namespace> | grep -A10 Events

# 3. 可能需要手动干预
kubectl scale deployment <name> -n <namespace> --replicas=0
kubectl scale deployment <name> -n <namespace> --replicas=3
```

---

## 5. 探针问题排查 (Probe Issues)

### 5.1 探针失败诊断

```bash
# === 查看探针配置 ===
kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[0].livenessProbe}'
kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[0].readinessProbe}'

# === 检查Pod Events ===
kubectl describe pod <pod-name> -n <namespace> | grep -E "Liveness|Readiness"

# === 手动测试探针 ===
kubectl exec -it <pod-name> -n <namespace> -- curl -v http://localhost:<port>/<path>
kubectl exec -it <pod-name> -n <namespace> -- cat <file-path>  # exec探针
```

### 5.2 探针问题解决

| 问题 | 症状 | 解决方案 |
|:---|:---|:---|
| **启动慢导致失败** | 容器反复重启 | 增加initialDelaySeconds或使用startupProbe |
| **路径错误** | 404响应 | 检查httpGet.path |
| **端口错误** | Connection refused | 检查port配置 |
| **间隔太短** | 频繁失败 | 增加periodSeconds |
| **超时太短** | Timeout | 增加timeoutSeconds |

### 5.3 推荐探针配置

```yaml
spec:
  containers:
  - name: app
    livenessProbe:
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 30
      periodSeconds: 10
      timeoutSeconds: 5
      failureThreshold: 3
    readinessProbe:
      httpGet:
        path: /ready
        port: 8080
      initialDelaySeconds: 5
      periodSeconds: 5
      timeoutSeconds: 3
      failureThreshold: 3
    startupProbe:  # K8s 1.20+
      httpGet:
        path: /healthz
        port: 8080
      initialDelaySeconds: 10
      periodSeconds: 10
      failureThreshold: 30  # 允许5分钟启动
```

---

## 6. 资源配额问题 (Resource Quota Issues)

### 6.1 配额检查

```bash
# === 检查命名空间配额 ===
kubectl describe quota -n <namespace>

# === 检查LimitRange ===
kubectl describe limitrange -n <namespace>

# === 检查Deployment资源请求 ===
kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[0].resources}'
```

### 6.2 配额不足解决

```bash
# === 查看当前使用量 ===
kubectl describe quota -n <namespace>
# Used vs Hard

# === 解决方案 ===
# 1. 申请增加配额
# 2. 减少Deployment副本数
# 3. 降低资源requests
# 4. 清理未使用资源
```

---

## 7. 亲和性与调度问题 (Affinity & Scheduling Issues)

### 7.1 调度失败排查

```bash
# === 检查nodeSelector ===
kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.template.spec.nodeSelector}'

# === 检查nodeAffinity ===
kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.template.spec.affinity}'

# === 检查tolerations ===
kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.template.spec.tolerations}'

# === 检查节点标签和污点 ===
kubectl get nodes --show-labels
kubectl describe nodes | grep -E "Taints|Labels"
```

### 7.2 常见调度问题

| 问题 | 原因 | 解决方案 |
|:---|:---|:---|
| **节点标签不匹配** | nodeSelector | 添加节点标签或修改selector |
| **污点未容忍** | Taints | 添加tolerations |
| **反亲和冲突** | podAntiAffinity | 调整规则或增加节点 |
| **拓扑分布不满足** | topologySpreadConstraints | 调整约束 |

---

## 8. 镜像问题排查 (Image Issues)

### 8.1 镜像拉取失败

```bash
# === 检查镜像配置 ===
kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[0].image}'

# === 检查imagePullPolicy ===
kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.template.spec.containers[0].imagePullPolicy}'

# === 检查imagePullSecrets ===
kubectl get deployment <name> -n <namespace> -o jsonpath='{.spec.template.spec.imagePullSecrets}'

# === 手动测试拉取 ===
crictl pull <image>
```

### 8.2 镜像问题解决

| 问题 | 解决方案 |
|:---|:---|
| 镜像不存在 | 检查镜像名和tag |
| 私有仓库认证失败 | 创建/更新imagePullSecret |
| 网络不通 | 配置镜像加速/代理 |
| tag变化 | 使用imagePullPolicy: Always |

---

## 9. 实用诊断命令 (Diagnostic Commands)

```bash
# === Deployment状态 ===
kubectl get deployment <name> -n <namespace> -o wide
kubectl describe deployment <name> -n <namespace>

# === 查看所有关联资源 ===
kubectl get all -n <namespace> -l app=<label>

# === ReplicaSet状态 ===
kubectl get rs -n <namespace> -l app=<label>

# === Pod状态 ===
kubectl get pods -n <namespace> -l app=<label> -o wide

# === 事件 ===
kubectl get events -n <namespace> --sort-by='.lastTimestamp' | tail -20

# === 回滚历史 ===
kubectl rollout history deployment/<name> -n <namespace>

# === 暂停/恢复更新 ===
kubectl rollout pause deployment/<name> -n <namespace>
kubectl rollout resume deployment/<name> -n <namespace>

# === 重启Deployment ===
kubectl rollout restart deployment/<name> -n <namespace>

# === 扩缩容 ===
kubectl scale deployment <name> -n <namespace> --replicas=5
```

---

## 10. 一键诊断脚本 (Diagnostic Script)

```bash
#!/bin/bash
DEPLOY=$1
NS=${2:-default}

echo "=== Deployment Status ==="
kubectl get deployment $DEPLOY -n $NS -o wide

echo -e "\n=== Deployment Conditions ==="
kubectl get deployment $DEPLOY -n $NS -o jsonpath='{range .status.conditions[*]}{.type}{"\t"}{.status}{"\t"}{.message}{"\n"}{end}'

echo -e "\n=== ReplicaSets ==="
kubectl get rs -n $NS -l $(kubectl get deployment $DEPLOY -n $NS -o jsonpath='{.spec.selector.matchLabels}' | sed 's/map\[\(.*\)\]/\1/' | tr ' ' ',')

echo -e "\n=== Pods ==="
kubectl get pods -n $NS -l $(kubectl get deployment $DEPLOY -n $NS -o jsonpath='{.spec.selector.matchLabels}' | sed 's/map\[\(.*\)\]/\1/' | tr ' ' ',') -o wide

echo -e "\n=== Events ==="
kubectl describe deployment $DEPLOY -n $NS | grep -A15 "Events:"

echo -e "\n=== Rollout Status ==="
kubectl rollout status deployment/$DEPLOY -n $NS --timeout=5s 2>&1 || true
```

---

## 4. 常见问题解决方案 (Common Solutions)

### 4.1 Deployment 创建失败解决方案

当遇到 Deployment 创建失败时，按以下步骤排查：

1. **检查 YAML 语法**
   ```bash
   kubectl apply -f deployment.yaml --dry-run=client
   ```

2. **验证资源配置**
   ```bash
   # 检查 ResourceQuota
   kubectl describe quota -n <namespace>
   
   # 检查 LimitRange
   kubectl get limitrange -n <namespace>
   ```

3. **确认镜像可访问**
   ```bash
   # 测试镜像拉取
   docker pull <image-name>:<tag>
   
   # 检查镜像仓库凭证
   kubectl get secret -n <namespace> | grep image-pull
   ```

### 4.2 ReplicaSet 不创建解决方案

如果 Deployment 已创建但没有 ReplicaSet：

1. **检查 Deployment 状态**
   ```bash
   kubectl describe deployment <name> -n <namespace>
   ```

2. **查看控制器管理器日志**
   ```bash
   kubectl logs -n kube-system -l component=kube-controller-manager
   ```

3. **验证 RBAC 权限**
   ```bash
   kubectl auth can-i create replicaset --as=system:serviceaccount:kube-system:deployment-controller
   ```

### 4.3 滚动更新失败解决方案

更新卡住时的处理方法：

1. **暂停更新**
   ```bash
   kubectl rollout pause deployment/<name> -n <namespace>
   ```

2. **检查具体问题**
   ```bash
   # 查看新 Pod 事件
   kubectl describe pod <new-pod> -n <namespace>
   
   # 检查容器日志
   kubectl logs <new-pod> -n <namespace> --previous
   ```

3. **强制回滚**
   ```bash
   kubectl rollout undo deployment/<name> -n <namespace>
   ```

4. **调整更新策略**
   ```yaml
   spec:
     strategy:
       type: RollingUpdate
       rollingUpdate:
         maxSurge: 1
         maxUnavailable: 0
   ```

### 4.4 预防措施和最佳实践

1. **设置合适的资源请求**
   ```yaml
   resources:
     requests:
       cpu: "100m"
       memory: "128Mi"
     limits:
       cpu: "500m"
       memory: "512Mi"
   ```

2. **配置健康检查探针**
   ```yaml
   livenessProbe:
     httpGet:
       path: /health
       port: 8080
     initialDelaySeconds: 30
     periodSeconds: 10
   
   readinessProbe:
     httpGet:
       path: /ready
       port: 8080
     initialDelaySeconds: 5
     periodSeconds: 5
   ```

3. **启用滚动更新监控**
   ```bash
   # 监控更新进度
   watch kubectl get deployment <name> -n <namespace>
   
   # 设置更新超时
   kubectl patch deployment <name> -n <namespace> -p '{"spec":{"progressDeadlineSeconds":600}}'
   ```

---

## 5. 自动化诊断脚本

### 5.1 Deployment 健康检查脚本

```bash
#!/bin/bash
# deployment_health_check.sh

DEPLOYMENT_NAME=$1
NAMESPACE=${2:-default}

echo "=== Deployment Health Check for $DEPLOYMENT_NAME ==="

# 基本状态检查
kubectl get deployment $DEPLOYMENT_NAME -n $NAMESPACE -o wide

# 详细描述
echo -e "\n--- Detailed Status ---"
kubectl describe deployment $DEPLOYMENT_NAME -n $NAMESPACE

# ReplicaSet 状态
echo -e "\n--- ReplicaSets ---"
kubectl get rs -n $NAMESPACE -l app=$DEPLOYMENT_NAME

# Pod 状态
echo -e "\n--- Pods ---"
kubectl get pods -n $NAMESPACE -l app=$DEPLOYMENT_NAME --show-labels

# 事件检查
echo -e "\n--- Recent Events ---"
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$DEPLOYMENT_NAME

# 资源使用情况
echo -e "\n--- Resource Usage ---"
kubectl top pods -n $NAMESPACE -l app=$DEPLOYMENT_NAME 2>/dev/null || echo "Metrics server not available"
```

### 5.2 快速故障诊断命令集合

```bash
# Deployment 故障诊断一键命令
alias deploy_debug='
  echo "=== Deployment Debug Commands ===";
  kubectl get deployment -o wide;
  kubectl get rs -o wide;
  kubectl get pods -o wide;
  kubectl get events --sort-by=".lastTimestamp"
'
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
