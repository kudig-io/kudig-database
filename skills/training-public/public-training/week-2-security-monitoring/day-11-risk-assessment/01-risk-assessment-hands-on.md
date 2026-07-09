---
title: 'Day 11: K8s 安全风险识别与防护实操'
description: '# Day 11: K8s 安全风险识别与防护实操'
summary: 'kubectl get clusterrolebindings -A | grep -i "system:masters"'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- calico
- helm
- docker
- falco
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 11: K8s 安全风险识别与防护实操 是什么'
- '如何 Day 11: K8s 安全风险识别与防护实操'
trigger_keywords:
- Day
- '11:'
- K8s
- 安全风险识别与防护实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- helm-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Day 11: K8s 安全风险识别与防护实操

> **日期**: Week 2 Day 4 | **主题**: 安全风险评估与最佳实践 | **版本**: K8s 1.28-1.33

---

## 1. 安全风险分类

### 1.1 K8s 安全层级模型

```
┌─────────────────────────────────────────┐
│           云厂商层（物理安全）           │
├─────────────────────────────────────────┤
│           集群层（RBAC/网络策略）        │
├─────────────────────────────────────────┤
│           节点层（OS/运行时/网络）       │
├─────────────────────────────────────────┤
│           Pod 层（SecurityContext/PSP）  │
├─────────────────────────────────────────┤
│           应用层（镜像/密钥/数据）       │
└─────────────────────────────────────────┘
```

### 1.2 五大攻击面

| 攻击面 | 风险 | 防护措施 |
|--------|------|---------|
| API Server | 未授权访问、提权 | RBAC + 审计日志 + 认证 |
| [[etcd|etcd]] | 数据泄露 | TLS + 网络隔离 + 加密 |
| [[kubelet|Kubelet]] | 容器逃逸 | RBAC + 静态 Pod + PSP |
| [[concepts/container-runtime.md|Container Runtime]] | 权限过大 | 最小化 capabilities |
| 网络 | 横向移动 | [[NetworkPolicy|NetworkPolicy]] + CNI 隔离 |

---

## 2. 身份与访问风险（RBAC）

### 2.1 高危权限组合检测

**检查 system:masters 组滥用**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查找所有 system:masters 绑定
kubectl get clusterrolebindings -A | grep -i "system:masters"

# 列出非 SA 的 system:masters 绑定（可能的安全风险）
kubectl get clusterrolebindings -A -o json | jq -r '.items[] |
  select(.subjects[].name == "system:masters") |
  {name: .metadata.name, subjects: .subjects}'
```
**检测 ServiceAccount 权限过大**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查找绑定到 system:authenticated 组的 ClusterRole
kubectl get clusterrolebindings -A | grep "system:authenticated"

# 检查 SA 是否有 create/patch/delete 权限
kubectl auth can-i create pods --as=system:serviceaccount:default:default
kubectl auth can-i delete pods --as=system:serviceaccount:default:default
```
### 2.2 最小权限检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
cat > check-rbac.sh <<'EOF'
#!/bin/bash
echo "=== RBAC 安全检查 ==="

# 检查匿名访问
echo "[1] 检查匿名访问 API Server 权限"
kubectl auth can-i --list --as=system:anonymous

# 检查 create pod 权限（可用于提权）
echo "[2] 检查 create pod 权限分布"
kubectl get clusterrolebindings -A -o json | jq -r '.items[] |
  select(.roleRef.name) |
  {name: .metadata.name, role: .roleRef.name, subjects: [.subjects[].name]}' | \
  while read binding; do
    name=$(echo "$binding" | jq -r '.name')
    kubectl auth can-i create pods --as="system:clusterrolebinding:$name" 2>/dev/null && \
      echo "  $name can create pods"
  done

# 检查 secrets 访问权限
echo "[3] 检查 secrets 全局读取权限"
kubectl get clusterrole --all-namespaces | grep -E "get.*secret|list.*secret" | head -10
EOF
chmod +x check-rbac.sh
./check-rbac.sh
```
---

## 3. Pod 安全风险

### 3.1 检测特权 Pod

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查找 privileged Pod
kubectl get pods -A -o yaml | grep -E "privileged: true" | head -20

# 或使用 kubectl-deepgrep 插件（krew install deepgrep）
kubectl deepgrep privileged=true

# 使用 Popeye 扫描 Pod 安全配置
helm repo add doktorlenz https://doktorlenz.github.io/charts
helm install popeye doktorlenz/popeye -n popeye --create-namespace
kubectl port-forward -n popeye svc/popeye 8080:80
```
### 3.2 常见高危 Pod 配置

| 配置 | 风险 | 正确做法 |
|------|------|---------|
| `privileged: true` | 完全权限，可逃逸 | 使用 `runAsNonRoot: true` + 限制 capabilities |
| `hostNetwork: true` | 访问宿主机网络 | 删除或使用 `hostPort: 0` |
| `hostPID: true` | 访问宿主机进程 | 删除 |
| `hostIPC: true` | 访问宿主机 IPC | 删除 |
| `securityContext.allowPrivilegeEscalation: true` | 可提权 | 设为 `false` |
| 未设 `runAsNonRoot: true` | 可能以 root 运行 | 设为 `true` |

### 3.3 Pod Security Policy 配置

```yaml
# 限制 Pod 安全上下文
apiVersion: policy/v1beta1

kind: PodSecurityPolicy
metadata:
  name: restricted-psp
spec:
  privileged: false
  allowPrivilegeEscalation: false
  requiredDropCapabilities:
    - ALL
  runAsUser:
    rule: MustRunAsNonRoot
  seLinux:
    rule: RunAsAny
  supplementalGroups:
    rule: RunAsAny
  fsGroup:
    rule: RunAsAny
  volumes:
    - 'configMap'
    - 'emptyDir'
    - 'secret'
    - 'persistentVolumeClaim'
```

---

## 4. 网络安全风险

### 4.1 检测未隔离的命名空间

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查命名空间是否有 NetworkPolicy
kubectl get networkpolicy -A

# 显示没有 NetworkPolicy 的 namespace
for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}'); do
  if ! kubectl get networkpolicy -n $ns &>/dev/null; then
    echo "WARNING: Namespace $ns has no NetworkPolicy"
  fi
done

# 对所有 namespace 应用默认拒绝（危险！先在测试环境验证）
kubectl label namespace default 'kubernetes.io/metadata.name=default'
```
### 4.2 检测不安全的 Service 类型

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查找 NodePort/LoadBalancer 类型 Service（暴露风险）
kubectl get svc -A | grep -E "NodePort|LoadBalancer" | grep -v "ingress"

# 检测 ClusterIP 是否泄漏到外部
kubectl get svc -A | awk '{print $1, $2, $4}' | grep -v "ClusterIP" | head -20
```
### 4.3 安全 NetworkPolicy 模板

```yaml
# 最小权限：默认拒绝所有 Ingress/Egress
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
# 白名单：仅允许 DNS 和特定服务访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-specific
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: api-server
  ingress:
    - from:
        - namespaceSelector:
            matchLabels:
              name: frontend
      ports:
        - protocol: TCP
          port: 8080
  egress:
    - to:
        - namespaceSelector: {}
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - protocol: UDP
          port: 53
```

---

## 5. 镜像安全

### 5.1 检测使用 latest 标签

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查找所有使用 latest 标签的 Pod
kubectl get pods -A -o json | jq -r '.items[] |
  select(.spec.containers[].image | contains(":latest")) |
  "\(.metadata.namespace)/\(.metadata.name): \(.spec.containers[].image)"'

# 使用 Trivy 扫描镜像漏洞
docker run --rm aquasec/trivy image nginx:latest
docker run --rm aquasec/trivy image --severity HIGH,CRITICAL myapp:v1.0
```
### 5.2 使用 ImagePolicyWebhook 强制签名验证

```yaml
# 在 kube-apiserver 配置 ImagePolicyWebhook
--enable-admission-plugins=ImagePolicyWebhook
--admission-config=/etc/kubernetes/admission-config.yaml
```

---

## 6. 密钥安全

### 6.1 检测直接挂载 secrets 到 Pod

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查找挂载了 secret 的 Pod
kubectl get pods -A -o json | jq -r '.items[] |
  select(.spec.volumes[].secret != null) |
  "\(.metadata.namespace)/\(.metadata.name)"

# 检查是否使用了 secret 作为环境变量（不推荐）
kubectl get pods -A -o json | jq -r '.items[] |
  select(.spec.containers[].env[] | select(.valueFrom?.secretKeyRef != null)) |
  "\(.metadata.namespace)/\(.metadata.name) in env vars"
```
### 6.2 使用 Vault 管理密钥

```yaml
# 使用 Vault 动态密钥
apiVersion: v1
kind: Pod
metadata:
  name: app
spec:
  containers:
    - name: app
      image: myapp:v1
      env:
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: db-credentials
              key: password
  volumes:
    - name: vault-token
      projected:
        sources:
          - serviceAccountToken:
              audience: vault
              expirationSeconds: 3600
              path: token
```

---

## 7. 安全检查清单

### 7.1 快速安全评估脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
cat > security-check.sh <<'EOF'
#!/bin/bash
echo "=== K8s 安全快速评估 ==="

# 1. 检查匿名访问
echo "[1] 匿名访问 API"
kubectl auth can-i --list --as=system:anonymous 2>/dev/null

# 2. 检查 system:masters 绑定
echo "[2] system:masters 绑定"
kubectl get clusterrolebindings 2>/dev/null | grep -i system:masters

# 3. 检查 privileged Pod
echo "[3] 特权 Pod"
kubectl get pods -A 2>/dev/null | grep -E "Privileged|RUNNING" | awk '{print $2}' | xargs -I{} kubectl get pod {} -A -o jsonpath='{.spec.containers[0].securityContext.privileged}' 2>/dev/null | grep -v "true"

# 4. 检查未加密的 Secret
echo "[4] Secret 加密状态"
kubectl get secret -A -o json 2>/dev/null | jq -r '.items[0].encryptedData' 2>/dev/null | head -1

# 5. 检查默认 ServiceAccount
echo "[5] 默认 SA 是否自动挂载"
kubectl get pod -A -o json 2>/dev/null | jq -r '.items[] | select(.spec.serviceAccountName == "default") | .metadata.name' | head -5

# 6. 检查网络隔离
echo "[6] 网络策略"
kubectl get networkpolicy -A 2>/dev/null | tail -n +2

echo "=== 检查完成 ==="
EOF
chmod +x security-check.sh
./security-check.sh
```
### 7.2 安全修复优先级

| 优先级 | 问题 | 修复方案 |
|--------|------|---------|
| P0 | 匿名访问 enabled | 禁用 `--anonymous-auth=false` |
| P0 | system:masters 权限过大 | 移除不必要的绑定 |
| P1 | Pod 无 SecurityContext | 添加 `runAsNonRoot: true` |
| P1 | 无 NetworkPolicy | 部署 Calico/Tigera 并配置默认拒绝 |
| P2 | 镜像使用 latest | 固定版本标签 |
| P2 | Secret 直接挂载 | 迁移到 Vault |

---

## 8. 实战练习

**练习 1**: 使用 `kubectl auth can-i` 验证所有 ServiceAccount 的最小权限

**练习 2**: 配置 PSP 限制所有 Pod 必须以非 root 运行，检测现有违规 Pod

**练习 3**: 在 `production` namespace 部署默认拒绝的 NetworkPolicy，放行 DNS 和必要服务

**练习 4**: 使用 Trivy 扫描所有生产镜像，生成漏洞报告

---

```yaml
---
id: LEARN-WEEK2-DAY11
title: Day 11 - K8s 安全风险识别与防护实操
topic: security-monitoring
type: hands-on-guide
tags: [security, rbac, networkpolicy, pod-security, vulnerability, hands-on, k8s-1.28-1.33]
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - "K8s 安全风险怎么识别"
  - "RBAC 权限怎么检查"
  - "Pod SecurityContext 怎么配置"
  - "NetworkPolicy 默认拒绝怎么写"
  - "特权容器怎么检测"
trigger_keywords:
  - 安全风险
  - RBAC
  - NetworkPolicy
  - PodSecurityPolicy
  - SecurityContext
  - 特权容器
  - 安全检查
  - 漏洞扫描
  - Trivy
  - 审计日志
  - 默认拒绝
  - 最小权限
reading_level: advanced
audience:
  - sre
  - security-engineer
  - ops-engineer
estimated_read_time: 50min
related_domains:
  - 安全
  - 故障诊断
  - domain-25-[[系统基础/topic-dictionary/security/cloud-native-security.md|cloud-native-security]]
related_topics:
  - security
  - rbac
  - networkpolicy
  - pod-security
  - vulnerability
related:
  - 生产运维/topic-learn/public-training/week-2-security-monitoring/day-08-rbac/01-rbac-hands-on.md
  - 安全/05-pod-security-standards.md
  - 安全/01-falco-cloud-native-security.md
---
```
```

<!-- risk-assessed -->
