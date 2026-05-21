---
title: 03 - 应用工作负载迁移
description: ③ 配置适配 ──────────┘    ⑤ 验证
category: migration
tags:
- k8s
- migration
- modernization
- docker
- harbor
- opa
- statefulset
- daemonset
- job
- cronjob
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 应用工作负载迁移 是什么
- 如何 应用工作负载迁移
trigger_keywords:
- 应用工作负载迁移
- migration
prerequisites:
- kubectl-basics
- gitops-basics
- tls-basics
- policy-basics
---

# 03 - 应用工作负载迁移

> **文档版本**: v1.0 | **适用场景**: 自建 K8s → 阿里云 ACK | **更新日期**: 2026-03 | **关键词**: Deployment, Service, Ingress, ConfigMap, Secret, YAML 适配, 镜像同步

---

## 目录

1. [迁移流程概览](#1-迁移流程概览)
2. [镜像仓库迁移](#2-镜像仓库迁移)
3. [资源导出与清洗](#3-资源导出与清洗)
4. [ACK 特有适配](#4-ack-特有适配)
5. [Namespace 与 RBAC 迁移](#5-namespace-与-rbac-迁移)
6. [ConfigMap 与 Secret 迁移](#6-configmap-与-secret-迁移)
7. [Deployment 迁移](#7-deployment-迁移)
8. [Service 迁移](#8-service-迁移)
9. [Ingress 迁移](#9-ingress-迁移)
10. [CronJob 与 Job 迁移](#10-cronjob-与-job-迁移)
11. [批量迁移自动化](#11-批量迁移自动化)
12. [迁移验证](#12-迁移验证)

---

## 1. 迁移流程概览

```
自建集群                                              ACK 集群
───────                                              ─────────

① 导出资源 YAML ─────┐
                     │    ② 清洗（去除集群特有字段）
② 镜像同步 ──────────┤    ③ 适配（ACK 注解/标签）
                     │    ④ 应用到 ACK
③ 配置适配 ──────────┘    ⑤ 验证

迁移顺序（依赖关系）:
  Namespace → RBAC → ConfigMap/Secret → PVC → Deployment/StatefulSet → Service → Ingress
```

---

## 2. 镜像仓库迁移

### 2.1 同步策略

| 方案 | 适用场景 | 优势 | 劣势 |
|------|---------|------|------|
| **ACR 企业版 + 镜像同步** | 自建 Harbor → ACR | 增量同步、触发器自动 | 需 ACR 企业版 |
| **skopeo 批量复制** | 任意 Registry → ACR | 无需 Docker Daemon | 全量复制 |
| **docker pull/tag/push** | 少量镜像 | 简单直接 | 效率低 |
| **ACR 镜像加速器** | 保留原仓库 | 零迁移成本 | 依赖外部仓库可用性 |

### 2.2 使用 skopeo 批量同步

```bash
#!/bin/bash
# sync-images-to-acr.sh
# 用途: 将自建集群使用的镜像批量同步到 ACR

SOURCE_REGISTRY="harbor.internal.com"
TARGET_REGISTRY="registry.cn-hangzhou.aliyuncs.com"
TARGET_NAMESPACE="your-acr-namespace"

# 从评估报告获取镜像列表
IMAGES_FILE="images-list.txt"

# 登录 ACR
docker login --username=<acr-user> $TARGET_REGISTRY

# 批量同步
while IFS= read -r image; do
  # 跳过公共镜像（可选）
  if echo "$image" | grep -qE "^(k8s.gcr.io|registry.k8s.io|docker.io/library)"; then
    echo "SKIP (public): $image"
    continue
  fi

  # 生成目标镜像名
  img_name=$(echo "$image" | sed "s|$SOURCE_REGISTRY/||" | tr '/' '-')
  target="$TARGET_REGISTRY/$TARGET_NAMESPACE/$img_name"

  echo "SYNC: $image → $target"
  skopeo copy \
    --src-tls-verify=false \
    --dest-tls-verify=true \
    "docker://$image" \
    "docker://$target" || echo "FAILED: $image"

done < "$IMAGES_FILE"

echo "镜像同步完成"
```

### 2.3 ACK 配置 ImagePullSecret

```bash
# 创建 ACR 拉取凭证（每个业务 Namespace 都需要）
kubectl create secret docker-registry acr-secret \
  --docker-server=registry.cn-hangzhou.aliyuncs.com \
  --docker-username=<acr-user> \
  --docker-password=<acr-password> \
  -n <namespace>

# 或通过 ServiceAccount 默认挂载
kubectl patch serviceaccount default \
  -n <namespace> \
  -p '{"imagePullSecrets": [{"name": "acr-secret"}]}'
```

---

## 3. 资源导出与清洗

### 3.1 导出资源

```bash
#!/bin/bash
# export-resources.sh
# 从自建集群导出所有业务资源

SOURCE_CONTEXT="source-cluster"
EXPORT_DIR="./migration-export"
mkdir -p $EXPORT_DIR

# 获取业务 Namespace（排除系统命名空间）
NAMESPACES=$(kubectl --context=$SOURCE_CONTEXT get ns --no-headers \
  -o custom-columns=:metadata.name | grep -vE "^(kube-|default$|ingress-|[[domain-19-landscape-references/01-cncf-landscape/graduated/cert-manager/cert-manager|cert-manager]])")

for ns in $NAMESPACES; do
  echo "=== 导出命名空间: $ns ==="
  mkdir -p $EXPORT_DIR/$ns

  # 导出各类资源
  for resource in configmaps secrets deployments statefulsets daemonsets \
                  services ingresses cronjobs jobs \
                  horizontalpodautoscalers poddisruptionbudgets \
                  serviceaccounts roles rolebindings \
                  networkpolicies resourcequotas limitranges; do
    kubectl --context=$SOURCE_CONTEXT get $resource -n $ns -o yaml \
      2>/dev/null > $EXPORT_DIR/$ns/$resource.yaml
    
    # 检查是否有实际资源
    if [ $(kubectl --context=$SOURCE_CONTEXT get $resource -n $ns --no-headers 2>/dev/null | wc -l) -eq 0 ]; then
      rm -f $EXPORT_DIR/$ns/$resource.yaml
    else
      echo "  ✓ $resource"
    fi
  done
done

# 导出 ClusterRole/ClusterRoleBinding（非系统）
kubectl --context=$SOURCE_CONTEXT get clusterroles -o yaml | \
  yq 'del(.items[] | select(.metadata.name | test("^system:")))' > $EXPORT_DIR/clusterroles.yaml
kubectl --context=$SOURCE_CONTEXT get clusterrolebindings -o yaml | \
  yq 'del(.items[] | select(.metadata.name | test("^system:")))' > $EXPORT_DIR/clusterrolebindings.yaml

echo "=== 导出完成: $EXPORT_DIR ==="
find $EXPORT_DIR -name "*.yaml" | wc -l
```

### 3.2 YAML 清洗脚本

> 导出的 YAML 包含集群特有字段（uid、resourceVersion、creationTimestamp 等），必须清理后才能导入 ACK。

```bash
#!/bin/bash
# clean-yaml.sh
# 清洗导出的 YAML，去除集群特有字段

EXPORT_DIR="./migration-export"
CLEAN_DIR="./migration-clean"

clean_yaml() {
  local input=$1
  local output=$2

  yq eval '
    # 删除集群特有元数据
    del(.metadata.uid) |
    del(.metadata.resourceVersion) |
    del(.metadata.creationTimestamp) |
    del(.metadata.generation) |
    del(.metadata.managedFields) |
    del(.metadata.selfLink) |
    del(.metadata.annotations["kubectl.kubernetes.io/last-applied-configuration"]) |
    del(.metadata.annotations["deployment.kubernetes.io/revision"]) |
    del(.metadata.annotations["field.cattle.io/*"]) |
    
    # 删除状态字段
    del(.status) |
    
    # 对于 List 类型，对每个 item 执行清洗
    (select(.kind == "*List") | .items[] |= (
      del(.metadata.uid) |
      del(.metadata.resourceVersion) |
      del(.metadata.creationTimestamp) |
      del(.metadata.generation) |
      del(.metadata.managedFields) |
      del(.metadata.selfLink) |
      del(.status)
    ))
  ' "$input" > "$output"
}

# 使用 kubectl-neat（更智能的清洗工具）
# 安装: kubectl krew install neat
clean_yaml_neat() {
  local input=$1
  local output=$2
  cat "$input" | kubectl neat > "$output"
}

# 批量清洗
find $EXPORT_DIR -name "*.yaml" | while read f; do
  relative=${f#$EXPORT_DIR/}
  mkdir -p "$CLEAN_DIR/$(dirname $relative)"
  output="$CLEAN_DIR/$relative"
  
  echo "清洗: $relative"
  clean_yaml_neat "$f" "$output"
done

echo "清洗完成: $CLEAN_DIR"
```

---

## 4. ACK 特有适配

### 4.1 镜像地址替换

```bash
# 将自建 Harbor 地址替换为 ACR 地址
find ./migration-clean -name "*.yaml" -exec sed -i '' \
  's|harbor\.internal\.com/|registry.cn-hangzhou.aliyuncs.com/your-namespace/|g' {} +

# 验证替换结果
grep -rn "harbor.internal.com" ./migration-clean/
# 预期: 无输出（已全部替换）

grep -rn "registry.cn-hangzhou" ./migration-clean/ | head -5
# 预期: 显示替换后的镜像地址
```

### 4.2 Service 注解适配

| 自建集群配置 | ACK 等效配置 | 说明 |
|------------|------------|------|
| `type: NodePort` | `type: LoadBalancer` (推荐) | ACK 使用 SLB/NLB 替代 NodePort |
| 无 LB 注解 | `service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec` | 指定 SLB 规格 |
| MetalLB 注解 | 阿里云 SLB 注解 | 完全不同，需重写 |
| `externalTrafficPolicy: Local` | 保持不变 | ACK 支持 |

```yaml
# 自建集群 Service（NodePort 方式）
apiVersion: v1
kind: Service
metadata:
  name: web-service
spec:
  type: NodePort
  ports:
  - port: 80
    targetPort: 8080
    nodePort: 30080
  selector:
    app: web

---
# ACK 适配后（LoadBalancer 方式）
apiVersion: v1
kind: Service
metadata:
  name: web-service
  annotations:
    # 使用内网 SLB
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-address-type: "intranet"
    # SLB 规格
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-spec: "slb.s2.small"
    # 指定 vSwitch（内网 SLB 必填）
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-vswitch-id: "<vsw-id>"
    # 健康检查
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-flag: "on"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-type: "http"
    service.beta.kubernetes.io/alibaba-cloud-loadbalancer-health-check-uri: "/health"
spec:
  type: LoadBalancer
  ports:
  - port: 80
    targetPort: 8080
  selector:
    app: web
```

### 4.3 Ingress 注解映射

| 自建 nginx-ingress 注解 | ACK nginx-ingress 注解 | 兼容性 |
|------------------------|----------------------|--------|
| `nginx.ingress.kubernetes.io/rewrite-target` | 相同 | 完全兼容 |
| `nginx.ingress.kubernetes.io/ssl-redirect` | 相同 | 完全兼容 |
| `nginx.ingress.kubernetes.io/proxy-body-size` | 相同 | 完全兼容 |
| `nginx.ingress.kubernetes.io/proxy-read-timeout` | 相同 | 完全兼容 |
| `nginx.ingress.kubernetes.io/cors-*` | 相同 | 完全兼容 |
| `nginx.ingress.kubernetes.io/auth-url` | 相同 | 完全兼容 |
| `nginx.ingress.kubernetes.io/configuration-snippet` | **需审查** | 自定义 Nginx 片段需逐条验证 |
| `cert-manager.io/cluster-issuer` | 相同（需安装 cert-manager） | 需在 ACK 安装 cert-manager |

> 详细 Ingress 迁移参考: `domain-03-networking-traffic/09-nginx-ingress-migration-guide.md`

---

## 5. Namespace 与 RBAC 迁移

```bash
# 1. 创建命名空间
for ns_file in ./migration-clean/*/; do
  ns=$(basename $ns_file)
  echo "创建命名空间: $ns"
  kubectl --context=ack-cluster create namespace $ns --dry-run=client -o yaml | kubectl apply -f -
done

# 2. 迁移 ResourceQuota
for ns in $(ls ./migration-clean/); do
  if [ -f "./migration-clean/$ns/resourcequotas.yaml" ]; then
    echo "应用 ResourceQuota: $ns"
    kubectl --context=ack-cluster apply -f ./migration-clean/$ns/resourcequotas.yaml
  fi
done

# 3. 迁移 LimitRange
for ns in $(ls ./migration-clean/); do
  if [ -f "./migration-clean/$ns/limitranges.yaml" ]; then
    echo "应用 LimitRange: $ns"
    kubectl --context=ack-cluster apply -f ./migration-clean/$ns/limitranges.yaml
  fi
done

# 4. 迁移 ServiceAccount
for ns in $(ls ./migration-clean/); do
  if [ -f "./migration-clean/$ns/serviceaccounts.yaml" ]; then
    echo "应用 ServiceAccount: $ns"
    kubectl --context=ack-cluster apply -f ./migration-clean/$ns/serviceaccounts.yaml
  fi
done

# 5. 迁移 Role/RoleBinding
for ns in $(ls ./migration-clean/); do
  for rbac in roles rolebindings; do
    if [ -f "./migration-clean/$ns/$rbac.yaml" ]; then
      echo "应用 $rbac: $ns"
      kubectl --context=ack-cluster apply -f ./migration-clean/$ns/$rbac.yaml
    fi
  done
done

# 6. 迁移 ClusterRole/ClusterRoleBinding
kubectl --context=ack-cluster apply -f ./migration-clean/clusterroles.yaml
kubectl --context=ack-cluster apply -f ./migration-clean/clusterrolebindings.yaml

# 7. ACK RAM 与 RBAC 集成（可选）
# ACK 支持将阿里云 RAM 用户/角色映射为 K8s RBAC 主体
# 通过 ACK 控制台: 集群 → 安全管理 → 授权管理
```

---

## 6. ConfigMap 与 Secret 迁移

```bash
# ConfigMap 迁移
for ns in $(ls ./migration-clean/); do
  if [ -f "./migration-clean/$ns/configmaps.yaml" ]; then
    echo "应用 ConfigMap: $ns"
    kubectl --context=ack-cluster apply -f ./migration-clean/$ns/configmaps.yaml
  fi
done

# Secret 迁移（注意: 需要特殊处理）
for ns in $(ls ./migration-clean/); do
  if [ -f "./migration-clean/$ns/secrets.yaml" ]; then
    echo "应用 Secret: $ns"
    # 过滤掉 ServiceAccount Token 类型（ACK 会自动创建）
    yq eval 'del(.items[] | select(.type == "kubernetes.io/service-account-token"))' \
      ./migration-clean/$ns/secrets.yaml | \
      kubectl --context=ack-cluster apply -f -
  fi
done

# 验证
kubectl --context=ack-cluster get configmaps -A --no-headers | grep -v kube- | wc -l
kubectl --context=ack-cluster get secrets -A --no-headers | grep -v kube- | wc -l
```

### 关键注意事项

```
Secret 迁移注意:
├── ✅ Opaque 类型 → 直接迁移
├── ✅ kubernetes.io/tls → 直接迁移（TLS 证书）
├── ✅ kubernetes.io/dockerconfigjson → 更新为 ACR 凭证
├── ❌ kubernetes.io/service-account-token → 不迁移（ACK 自动生成）
└── ⚠️  含环境特有值（DB 连接串等）→ 需手动更新为 ACK 环境值

ConfigMap 迁移注意:
├── ✅ 应用配置 → 直接迁移
├── ⚠️  含 IP/域名 → 需更新为 ACK 环境值
└── ⚠️  含自建集群特有路径 → 需适配
```

---

## 7. Deployment 迁移

```bash
# 批量迁移 Deployment
for ns in $(ls ./migration-clean/); do
  if [ -f "./migration-clean/$ns/deployments.yaml" ]; then
    echo "应用 Deployment: $ns"
    kubectl --context=ack-cluster apply -f ./migration-clean/$ns/deployments.yaml
  fi
done

# 验证 Deployment 状态
kubectl --context=ack-cluster get deployments -A | grep -v kube-
```

### 常见适配项

```yaml
# 适配项 1: nodeSelector / nodeAffinity
# 自建集群可能有自定义标签，需映射到 ACK 节点标签
# 自建:
#   nodeSelector:
#     disk: ssd
# ACK 适配:
spec:
  template:
    spec:
      nodeSelector:
        node-role: app                    # 映射到 ACK 节点池标签
      tolerations:                        # 如果需要调度到特定节点池
      - key: "workload-type"
        operator: "Equal"
        value: "stateful"
        effect: "NoSchedule"

---
# 适配项 2: hostPath → PVC
# 自建:
#   volumes:
#   - name: data
#     hostPath:
#       path: /data/app
# ACK 适配:
spec:
  template:
    spec:
      volumes:
      - name: data
        persistentVolumeClaim:
          claimName: app-data-pvc         # 使用云盘 PVC

---
# 适配项 3: 资源限制调整
# ACK Terway 模式下，每个 Pod 占用一个 ENI 辅助 IP
# 需要确保节点资源充足
spec:
  template:
    spec:
      containers:
      - name: app
        resources:
          requests:
            cpu: "250m"
            memory: "512Mi"
          limits:
            cpu: "1"
            memory: "1Gi"
```

---

## 8. Service 迁移

参考 [4.2 Service 注解适配](#42-service-注解适配)，核心转换规则:

```bash
# 批量将 NodePort 转换为 ClusterIP（内部服务）
yq eval '
  (.items[] | select(.spec.type == "NodePort") | .spec.type) = "ClusterIP" |
  del(.items[] | select(.spec.type == "ClusterIP") | .spec.ports[].nodePort)
' ./migration-clean/<ns>/services.yaml > ./migration-clean/<ns>/services-adapted.yaml

# 需要外部暴露的 Service 单独处理为 LoadBalancer
```

---

## 9. Ingress 迁移

```bash
# Ingress 迁移（确保 ingressClassName 正确）
for ns in $(ls ./migration-clean/); do
  if [ -f "./migration-clean/$ns/ingresses.yaml" ]; then
    # 确保 ingressClassName 设置正确
    yq eval '
      (.items[].spec.ingressClassName) = "nginx"
    ' ./migration-clean/$ns/ingresses.yaml | \
    kubectl --context=ack-cluster apply -f -
  fi
done

# 验证 Ingress
kubectl --context=ack-cluster get ingress -A
```

---

## 10. CronJob 与 Job 迁移

```bash
# CronJob 迁移
for ns in $(ls ./migration-clean/); do
  if [ -f "./migration-clean/$ns/cronjobs.yaml" ]; then
    echo "应用 CronJob: $ns"
    kubectl --context=ack-cluster apply -f ./migration-clean/$ns/cronjobs.yaml
  fi
done

# 注意: 迁移后建议先暂停 CronJob，待全部验证通过再启用
kubectl --context=ack-cluster get cronjobs -A -o name | \
  xargs -I {} kubectl --context=ack-cluster patch {} -p '{"spec":{"suspend":true}}'
```

---

## 11. 批量迁移自动化

```bash
#!/bin/bash
# migrate-workloads.sh
# 一键迁移所有工作负载到 ACK

set -euo pipefail

ACK_CONTEXT="ack-cluster"
CLEAN_DIR="./migration-clean"

echo "======================================"
echo "  工作负载批量迁移 → ACK"
echo "  时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "======================================"

# 迁移顺序：Namespace → RBAC → Config → Workload → Service → Ingress

NAMESPACES=$(ls $CLEAN_DIR | grep -vE "^(clusterroles|clusterrolebindings)")

echo ">>> Step 1: Namespace"
for ns in $NAMESPACES; do
  kubectl --context=$ACK_CONTEXT create namespace $ns --dry-run=client -o yaml | \
    kubectl --context=$ACK_CONTEXT apply -f - 2>/dev/null
done

echo ">>> Step 2: ClusterRole/ClusterRoleBinding"
kubectl --context=$ACK_CONTEXT apply -f $CLEAN_DIR/clusterroles.yaml 2>/dev/null || true
kubectl --context=$ACK_CONTEXT apply -f $CLEAN_DIR/clusterrolebindings.yaml 2>/dev/null || true

echo ">>> Step 3: Per-namespace resources"
for ns in $NAMESPACES; do
  echo "  --- $ns ---"
  for resource in resourcequotas limitranges serviceaccounts roles rolebindings \
                  configmaps secrets deployments statefulsets daemonsets \
                  services ingresses cronjobs horizontalpodautoscalers \
                  poddisruptionbudgets networkpolicies; do
    if [ -f "$CLEAN_DIR/$ns/$resource.yaml" ]; then
      echo "    ✓ $resource"
      kubectl --context=$ACK_CONTEXT apply -f "$CLEAN_DIR/$ns/$resource.yaml" 2>&1 | \
        grep -v "unchanged" || true
    fi
  done
done

echo ""
echo ">>> 迁移完成，开始验证..."
echo ""

# 验证
echo "=== Pod 状态 ==="
kubectl --context=$ACK_CONTEXT get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded \
  | grep -v "^NAMESPACE" || echo "所有 Pod 正常"

echo ""
echo "=== 未就绪 Deployment ==="
kubectl --context=$ACK_CONTEXT get deployments -A | awk '$3 != $4 {print}' | grep -v "^NAMESPACE" || echo "所有 Deployment 就绪"

echo ""
echo "======================================"
echo "  迁移执行完成"
echo "======================================"
```

---

## 12. 迁移验证

### 12.1 自动化验证脚本

```bash
#!/bin/bash
# verify-migration.sh

ACK_CONTEXT="ack-cluster"
SOURCE_CONTEXT="source-cluster"

echo "=== 工作负载数量对比 ==="
echo "资源类型           | 源集群 | ACK"
echo "-------------------|--------|-----"
for resource in deployments statefulsets daemonsets services ingresses cronjobs configmaps secrets; do
  src=$(kubectl --context=$SOURCE_CONTEXT get $resource -A --no-headers 2>/dev/null | \
    grep -vE "^kube-" | wc -l | xargs)
  ack=$(kubectl --context=$ACK_CONTEXT get $resource -A --no-headers 2>/dev/null | \
    grep -vE "^kube-" | wc -l | xargs)
  printf "%-19s| %-6s | %s\n" "$resource" "$src" "$ack"
done

echo ""
echo "=== ACK Pod 异常检查 ==="
kubectl --context=$ACK_CONTEXT get pods -A | grep -vE "(Running|Completed|kube-system)" || echo "无异常 Pod"

echo ""
echo "=== ACK 事件告警 ==="
kubectl --context=$ACK_CONTEXT get events -A --field-selector type=Warning --sort-by=.lastTimestamp | tail -20
```

### 12.2 检查清单

- [ ] 镜像已全部同步到 ACR，ACK Pod 可正常拉取
- [ ] 所有 Namespace 已创建
- [ ] RBAC（Role/RoleBinding/ClusterRole）已迁移
- [ ] ConfigMap/Secret 已迁移（环境特有值已更新）
- [ ] 所有 Deployment 已就绪（READY 列匹配）
- [ ] 所有 Service 已创建（LoadBalancer 类型已分配 IP）
- [ ] 所有 Ingress 已创建（可通过 ACK Ingress IP 访问）
- [ ] CronJob 已迁移（暂停状态）
- [ ] 无 Warning 事件
- [ ] Pod 日志无异常

---

**上一步**: ← [02-ACK 目标集群设计](./02-ack-target-cluster-design.md)
**下一步**: → [04-存储与数据迁移](./04-storage-data-migration.md)
