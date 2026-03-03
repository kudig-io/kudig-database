# Day 27: 扩展生态 + 高级主题

> **学习时间**: 4-5 小时 | **主题**: CRD、Helm、Operator

---

## 今日目标

- [ ] 了解 CRD 开发基础
- [ ] 掌握 Helm 包管理
- [ ] 理解 Operator 模式

---

## 理论学习 (2h)

### 必读文档

1. **CRD 开发指南**
   - 文件: `../../domain-10-extensions/01-crd-development-guide.md`

2. **Helm Charts 管理**
   - 文件: `../../domain-10-extensions/06-helm-charts-management.md`

3. **Operator 开发入门**
   - 文件: `../../domain-9-platform-ops/20-crd-operator-development.md`

---

## 实践任务 (2.5h)

### 任务 1: Helm 实践 (1h)

```bash
# 添加常用 Helm repo
helm repo add bitnami https://charts.bitnami.com/bitnami
helm repo add prometheus-community https://prometheus-community.github.io/helm-charts
helm repo update

# 搜索 Chart
helm search repo nginx

# 查看 Chart 信息
helm show chart bitnami/nginx
helm show values bitnami/nginx

# 安装 Chart
helm install my-nginx bitnami/nginx \
  --set replicaCount=2 \
  --set service.type=ClusterIP

# 查看安装
helm list
helm status my-nginx

# 自定义 values.yaml
cat > nginx-values.yaml << 'EOF'
replicaCount: 3
service:
  type: ClusterIP
  port: 80
resources:
  requests:
    cpu: 100m
    memory: 128Mi
  limits:
    cpu: 200m
    memory: 256Mi
EOF

# 使用自定义配置升级
helm upgrade my-nginx bitnami/nginx -f nginx-values.yaml

# 回滚
helm rollback my-nginx 1

# 卸载
helm uninstall my-nginx
```

### 任务 2: 创建简单的 Helm Chart (1h)

```bash
# 创建 Chart 骨架
helm create my-app

# 查看结构
tree my-app

# 修改 values.yaml
cat > my-app/values.yaml << 'EOF'
replicaCount: 2
image:
  repository: nginx
  tag: alpine
  pullPolicy: IfNotPresent
service:
  type: ClusterIP
  port: 80
resources:
  requests:
    cpu: 50m
    memory: 64Mi
  limits:
    cpu: 100m
    memory: 128Mi
EOF

# 验证 Chart
helm lint my-app
helm template my-app my-app

# 打包
helm package my-app

# 本地安装
helm install test my-app/
```

### 任务 3: 了解 Operator 模式 (30min)

```bash
# Operator 核心概念
# CRD: Custom Resource Definition (自定义资源定义)
# CR: Custom Resource (自定义资源实例)
# Controller: 监听 CR 变化并执行操作

# 示例: 查看已安装的 CRD
kubectl get crd

# 查看 Prometheus Operator 的 CRD
kubectl get crd | grep monitoring.coreos.com

# 查看自定义资源
kubectl get servicemonitor -A
kubectl get prometheusrule -A

# Operator 工作流程:
# 1. 用户创建 CR (如 ServiceMonitor)
# 2. Operator Controller 监听到 CR 变化
# 3. Controller 执行相应操作 (如配置 Prometheus 抓取目标)
# 4. Controller 更新 CR 状态
```

---

## 费曼复述 (0.5h)

1. **Helm Chart 和 Kustomize 的区别和使用场景？**
2. **Operator 模式的核心思想是什么？**
3. **CRD 在 K8s 扩展中扮演什么角色？**

---

## 今日检验

- [ ] 能够使用 Helm 安装和管理应用
- [ ] 能够创建简单的 Helm Chart
- [ ] 理解 Operator 模式的工作原理
