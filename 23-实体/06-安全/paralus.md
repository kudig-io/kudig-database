---
title: Paralus (entities)
description: '## 概述'
summary: 'Paralus 是一个 Kubernetes 零信任访问管理平台，为多集群环境提供统一的身份认证、授权和审计能力。它作为 kubectl 和 [[17-系统基础/06-知识字典/fundamentals/the-kubernetes-api.md|Kubernetes API]] 之间的安全代理层，'
category: entities
tags:
- k8s
- cncf
- security
- paralus
- istio
- opa
- crd
- operator
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Paralus 是什么
- 如何 Paralus
trigger_keywords:
- Paralus
prerequisites:
- kubectl-basics
- service-mesh-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# Paralus

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

Paralus 是一个 CNCF 沙箱项目，由 RackN 开发，是一个 Kubernetes 集群的零信任访问控制系统。它提供基于角色的访问控制（RBAC）、单点登录（SSO）和审计日志功能，让用户通过统一身份认证安全地访问多个 K8s 集群。Paralus 消除了共享 kubeconfig 和 SSH 访问的安全风险，通过短期 Token 和细粒度权限实现零信任 K8s 访问。

## Key Features（核心能力）

- **统一身份认证**：集成 OIDC/SAML/SSO 提供跨集群统一登录
- **细粒度 RBAC**：基于 Namespace、Role、Cluster 的多层访问控制
- **kubectl 代理**：通过 Paralus Proxy 代理 kubectl 命令，无需直接暴露 K8s API
- **SSO 集成**：支持 GitHub、Google、Azure AD、Okta 等身份提供商
- **审计日志**：记录所有 K8s API 操作，支持合规审计
- **多集群管理**：统一管理多个 K8s 集群的访问权限

## 架构与工作原理

Paralus 由多个组件构成：Paralus Controller 是核心管理平面，管理用户、角色、集群和策略；Paralus Connector（Adapter）部署在目标集群，作为 K8s API 的反向代理执行认证和授权；Paralus CLI（pctl）为用户提供本地 kubectl 代理。用户通过 OIDC SSO 认证后获取短期 Token，Token 通过 Paralus Proxy 转发到目标集群，Proxy 验证 Token 并根据 RBAC 策略授权或拒绝请求。

## K8s 集成

Paralus Controller 部署在管理集群上，通过 CRD 或数据库管理用户/角色/集群映射。Paralus Adapter 以 Deployment 部署在目标集群，作为 K8s API Server 前面的认证代理。用户配置 kubectl 使用 Paralus Proxy 地址而非直接连接 K8s API Server。通过 ValidatingWebhook 确保 Adapter 正确拦截所有 API 请求。

## 生产用例

- **多集群安全访问**：为团队提供统一的多集群 K8s 访问入口
- **合规审计**：满足金融/医疗行业对 K8s 操作审计的要求
- **零信任安全**：消除共享凭据，使用短期 Token 实现最小权限访问
- **外部协作者访问**：安全地为外包团队提供临时 K8s 访问

## 安装与配置

```bash
# 🟢 添加 Helm 仓库
helm repo add paralus https://paralus.github.io/helm-charts
helm repo update

# 🟢 安装 Paralus
helm install paralus paralus/paralus \
  -n paralus --create-namespace \
  --set fqdn.name=paralus.example.com \
  --set fqdn.apihost=api.paralus.example.com \
  --set oidc.enabled=true \
  --set oidc.provider=google

# 🟢 验证安装
kubectl get pods -n paralus
kubectl get svc -n paralus

# 🟢 下载 pctl CLI
curl -L https://github.com/paralus/cli/releases/latest/download/pctl-linux-amd64 -o pctl
chmod +x pctl && mv pctl /usr/local/bin/

# 🟢 登录并获取 kubeconfig
pctl login paralus.example.com
pctl kubeconfig --cluster production > ~/.kube/paralus-config
export KUBECONFIG=~/.kube/paralus-config
kubectl get nodes  # 通过 Paralus 代理访问
```

### 角色和权限配置

```yaml
# 定义组织角色
apiVersion: rbac.paralus.io/v1alpha1
kind: Role
metadata:
  name: namespace-admin
  namespace: paralus
spec:
  permissions:
    - apiGroups: ["*"]
      resources: ["*"]
      verbs: ["*"]
  scope: namespace
---
# 定义只读角色
apiVersion: rbac.paralus.io/v1alpha1
kind: Role
metadata:
  name: namespace-viewer
  namespace: paralus
spec:
  permissions:
    - apiGroups: ["", "apps", "batch"]
      resources: ["pods", "deployments", "services", "jobs"]
      verbs: ["get", "list", "watch"]
  scope: namespace
---
# 用户-角色-集群绑定
apiVersion: rbac.paralus.io/v1alpha1
kind: UserRoleBinding
metadata:
  name: dev-team-prod-access
  namespace: paralus
spec:
  users:
    - email: dev1@company.com
    - email: dev2@company.com
  role: namespace-viewer
  clusters:
    - name: production
      namespaces:
        - app-frontend
        - app-backend
```

## 运维操作

```bash
# 🟢 查看已注册集群
pctl get clusters

# 🟢 查看用户和角色
pctl get users
pctl get roles

# 🟢 查看审计日志
kubectl logs -n paralus -l app=paralus-audit --tail=100

# 🟡 撤销用户访问
pctl delete userrolebinding dev-team-prod-access

# 🟡 注册新集群
pctl register cluster --name staging --kubeconfig ~/.kube/staging-config

# 🔴 删除集群注册（会移除目标集群上的 Adapter）
pctl delete cluster staging
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| kubectl 401 | Token 过期 | `pctl login` | 重新登录获取新 Token |
| 集群不可达 | Adapter 断连 | `pctl get clusters` | 检查目标集群 Adapter Pod |
| 权限被拒绝 | RBAC 配置错误 | `pctl get userrolebindings` | 检查角色绑定 |
| SSO 登录失败 | OIDC 配置错误 | 查看 Paralus Controller 日志 | 检查 OIDC Provider 配置 |

```bash
# 排查流程
# 1. 检查 Paralus 控制平面
kubectl get pods -n paralus
kubectl logs -n paralus -l app=paralus-controller --tail=50

# 2. 检查目标集群 Adapter
kubectl --context=production get pods -n paralus-system

# 3. 检查 Token 有效性
pctl whoami

# 4. 检查审计日志
kubectl logs -n paralus -l app=paralus-audit --tail=50 | grep "denied"
```

## 生产案例

### 案例1：多集群统一访问控制
- **场景**：50+ 开发者需要访问 5 个 K8s 集群，之前共享 kubeconfig 存在安全风险
- **方案**：Paralus + Google SSO；按团队分配 Namespace 级权限；短期 Token 自动过期；全操作审计
- **效果**：消除共享凭据风险，权限变更即时生效，审计覆盖率 100%

### 案例2：外部承包商临时访问
- **场景**：外包团队需要临时访问生产集群排查问题，要求可追溯且自动过期
- **方案**：创建临时用户 + 只读角色 + 7天过期；所有操作录制审计；到期自动撤销
- **效果**：外部访问安全可控，零凭据泄漏风险，审计合规

## 对比替代方案

| 维度 | Paralus | Teleport | K8s RBAC+OIDC | Rancher |
|------|---------|----------|--------------|--------|
| K8s 专注 | 是 | 否(通用) | 是 | 是 |
| 多集群 | 强 | 强 | 弱 | 强 |
| 审计 | 强 | 强 | 弱 | 中 |
| SSO | 支持 | 支持 | 支持 | 支持 |
| 短期 Token | 是 | 是 | 否 | 是 |
| 学习曲线 | 中 | 高 | 低 | 中 |

## 检查清单

- [ ] Paralus 控制平面已部署且 Pod Running
- [ ] OIDC/SSO 已配置且可登录
- [ ] 目标集群 Adapter 已部署且连接正常
- [ ] 角色和权限已按最小权限原则配置
- [ ] 审计日志已启用且存储已配置
- [ ] Token 过期策略已配置
- [ ] 共享 kubeconfig 已废除

## Related

- [[distribution]] — Distribution
- [[02-istio-security-hardening]] — [[istio|Istio]]io 安全加固|Istio 安全加固]]
- [[copa]] — Copa (Copacetic)
- [[nats]] — NATS
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- paralus
- [[23-实体/15-参考与索引/cncf-security.md|CNCF 安全与合规项目全景]] — Cross-reference
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
