---
title: Kubernetes 集群可视化组件部署与安全指南
description: 对比 Kubernetes Dashboard 与 Headlamp 两种主流集群可视化工具，提供生产级部署、RBAC 授权、访问控制与安全加固指南。
summary: 对比 Kubernetes Dashboard 与 Headlamp 两种主流集群可视化组件，提供生产级部署、RBAC 授权、访问控制与安全加固指南。
category: 集群基础
tags:
- k8s
- dashboard
- headlamp
- visualization
- rbac
- oauth
- security
- ingress
- authentication
tier: supporting
created: '2026-07-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes Dashboard 部署
- Headlamp 部署
- 集群可视化工具安全加固
trigger_keywords:
- dashboard
- headlamp
- kubernetes visualization
prerequisites:
- kubectl-basics
- rbac-basics
- ingress-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubernetes 集群可视化组件部署与安全指南

> Kubernetes Dashboard 与 Headlamp 是两种主流的集群可视化工具。Dashboard 是官方 Web UI，Headlamp 是 CNCF 沙箱项目，提供更现代的插件化体验。

---

## 1. 组件对比

| 维度 | Kubernetes Dashboard | Headlamp |
|------|---------------------|----------|
| **项目归属** | Kubernetes 官方 | CNCF Sandbox |
| **部署方式** | Helm / 官方 YAML | Helm / 官方 YAML |
| **认证方式** | Bearer Token / Kubeconfig / OIDC | OIDC / Helm 插件 / ServiceAccount Token |
| **权限模型** | 登录用户权限受限于 ServiceAccount | 支持多集群、RBAC、OIDC 角色映射 |
| **插件生态** | 有限 | 丰富（Prometheus、Flux、ArgoCD 等） |
| **多集群** | 需单独部署 | 原生支持多集群 |
| **UI 体验** | 功能全面但较传统 | 现代化、可扩展 |

---

## 2. Kubernetes Dashboard

### 2.1 安装

```bash
# 🟡 添加 Helm repo
helm repo add kubernetes-dashboard https://kubernetes.github.io/dashboard/
helm repo update

# 🟡 安装 Dashboard（推荐命名空间 kubernetes-dashboard）
helm upgrade --install kubernetes-dashboard kubernetes-dashboard/kubernetes-dashboard \
  --create-namespace --namespace kubernetes-dashboard

# 🟢 查看 Pod
kubectl get pods -n kubernetes-dashboard
```

### 2.2 创建管理员用户（仅测试环境）

```yaml
# 🟡 中风险：创建具有 cluster-admin 权限的服务账户
apiVersion: v1
kind: ServiceAccount
metadata:
  name: admin-user
  namespace: kubernetes-dashboard
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: admin-user
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: cluster-admin
subjects:
- kind: ServiceAccount
  name: admin-user
  namespace: kubernetes-dashboard
```

```bash
# 🟢 获取登录 token
kubectl -n kubernetes-dashboard create token admin-user
```

### 2.3 生产环境安全加固

| 风险 | 加固措施 |
|------|---------|
| 默认使用 cluster-admin | 为不同团队创建最小权限 ServiceAccount |
| 暴露到公网无保护 | 仅通过内网 Ingress + OIDC / VPN 访问 |
| Token 长期有效 | 使用 short-lived token 或 OIDC |
| 跳过登录 | 禁用 `--enable-skip-login` |
| 未启用审计 | 开启 Dashboard 访问审计日志 |

```yaml
# 生产推荐：最小权限 ServiceAccount 示例
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  namespace: default
  name: dashboard-readonly
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps"]
  verbs: ["get", "list", "watch"]
```

---

## 3. Headlamp

### 3.1 安装

```bash
# 🟡 添加 Helm repo
helm repo add headlamp https://headlamp-k8s.github.io/headlamp/
helm repo update

# 🟡 安装 Headlamp
helm upgrade --install headlamp headlamp/headlamp \
  --create-namespace --namespace headlamp

# 🟢 查看 Pod
kubectl get pods -n headlamp
```

### 3.2 OIDC 集成示例

```yaml
# Helm values 片段
config:
  oidc:
    clientID: "headlamp"
    clientSecret: "<oidc-client-secret>"
    issuerURL: "https://auth.example.com/realms/production"
    scopes: "openid,profile,email,groups"
```

### 3.3 访问方式

```bash
# 🟢 端口转发（本地测试）
kubectl port-forward -n headlamp svc/headlamp 4466:80

# 🟡 生产环境：配置 Ingress + TLS
# 示例 Ingress 配置见下文
```

---

## 4. Ingress + TLS 访问配置

```yaml
apiVersion: networking.k8s.io/v1
kind: Ingress
metadata:
  name: k8s-dashboard
  namespace: kubernetes-dashboard
  annotations:
    nginx.ingress.kubernetes.io/backend-protocol: "HTTPS"
    nginx.ingress.kubernetes.io/ssl-redirect: "true"
spec:
  ingressClassName: nginx
  tls:
  - hosts:
    - dashboard.example.com
    secretName: dashboard-tls
  rules:
  - host: dashboard.example.com
    http:
      paths:
      - path: /
        pathType: Prefix
        backend:
          service:
            name: kubernetes-dashboard
            port:
              number: 443
```

> **注意**：Dashboard 后端使用 HTTPS，需要 `nginx.ingress.kubernetes.io/backend-protocol: "HTTPS"`。

---

## 5. 监控与审计

| 对象 | 指标/日志 | 工具 |
|------|----------|------|
| Dashboard 访问日志 | HTTP 请求、登录事件 | Ingress Controller 日志 |
| RBAC 变更审计 | RoleBinding/ClusterRoleBinding 变更 | Kubernetes Audit Log |
| Token 使用 | ServiceAccount Token 审计 | kube-apiserver audit |
| 会话监控 | 异常登录、敏感操作 | SIEM / 云平台审计 |

---

## 6. 检查清单

- [ ] 选择符合团队需求的可视化工具（Dashboard vs Headlamp）
- [ ] 使用最小权限原则配置 RBAC
- [ ] 生产环境禁用跳过登录（Dashboard）
- [ ] 通过 Ingress + TLS / VPN 限制访问
- [ ] 集成 OIDC 或短期 Token 认证
- [ ] 开启访问审计与异常告警
- [ ] 定期审查 ServiceAccount 权限
- [ ] 为多集群场景优先评估 Headlamp

---

## Related

- [[实体/rbac.md|RBAC]]
- [[实体/ingress-controller.md|Ingress Controller]]
- [[概念/security-defense-depth.md|Defense-in-Depth Security]]
- [[故障诊断/FTA故障树/list/rbac-fta.md|RBAC 异常故障树分析]]
- [[故障诊断/FTA故障树/list/webhook-admission-fta.md|Webhook 准入异常故障树分析]]


<!-- risk-assessed -->
