---
title: "Ingress Gateway Failure Remediation Playbook"
category: remediation
skill_set: "k8s-ingress-gateway"
created: "2026-05-22"
updated: "2026-05-22"
---

# 修复操作手册 / Remediation Playbook

> **来源**: SKILL-ING-001 v1.0 — [[Ingress|Ingress]] Gateway Failure 诊断与修复

## 目录

- [风险级别说明](#风险级别说明)
- [修复操作](#修复操作)
  - [🟢 低风险](#-低风险)
    - [REM-001 修正 Ingress 规则](#rem-001)
    - [REM-004 更新 TLS 证书](#rem-004)
    - [REM-006 修正 DNS](#rem-006)
  - [🟡 中风险](#-中风险)
    - [REM-002 重启/修复 Ingress Controller](#rem-002)
    - [REM-003 修复后端服务](#rem-003)
    - [REM-005 安装 Gateway API Controller](#rem-005)
- [验证确认](#验证确认)
- [升级协议](#升级协议)

## 风险级别说明

| 风险级别 | 标识 | 含义 | Agent 行为 |
|---------|------|------|-----------|
| 低风险 | 🟢 | 配置调整 | 可建议自动执行 |
| 中风险 | 🟡 | 组件重启或服务变更 | 建议操作并等待人工审批 |

## 修复操作

### 🟢 低风险

#### REM-001: 修正 Ingress 规则

- **适用根因**: RC-001
- **前置检查**:
  ```bash
  kubectl get ingress <name> -n <namespace> -o yaml
  # 检查 path、backend service name/port、host
  ```
- **执行命令**:
  ```bash
  # 修正 backend service
  kubectl patch ingress <name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/rules/0/http/paths/0/backend/service/name", "value": "<correct-service>"}]'

  # 修正 path
  kubectl patch ingress <name> -n <namespace> --type='json' -p='
  [{"op": "replace", "path": "/spec/rules/0/http/paths/0/path", "value": "/<correct-path>"}]'

  # 添加 ingressClassName
  kubectl patch ingress <name> -n <namespace> --type='json' -p='
  [{"op": "add", "path": "/spec/ingressClassName", "value": "nginx"}]'
  ```
- **后置验证**:
  ```bash
  kubectl get ingress <name> -n <namespace>
  kubectl describe ingress <name> -n <namespace>
  ```

#### REM-004: 更新 TLS 证书

- **适用根因**: RC-004
- **前置检查**:
  ```bash
  kubectl get secret <tls-secret> -n <namespace>
  openssl x509 -in <(kubectl get secret <tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d) -noout -enddate
  ```
- **执行命令**:
  ```bash
  # 从 cert-manager 重新签发
  kubectl cert-manager renew --namespace=<namespace> <certificate-name>

  # 或手动更新 secret
  kubectl create secret tls <tls-secret> \
    --cert=<path/to/cert.pem> \
    --key=<path/to/key.pem> \
    -n <namespace> --dry-run=client -o yaml | kubectl apply -f -
  ```
- **后置验证**:
  ```bash
  kubectl get secret <tls-secret> -n <namespace>
  curl -k -v https://<ingress-address>/ 2>&1 | grep -i "expire"
  ```

#### REM-006: 修正 DNS

- **适用根因**: RC-006
- **前置检查**:
  ```bash
  nslookup <ingress-host>
  dig <ingress-host>
  ```
- **执行命令**:
  ```bash
  # 更新 DNS 记录指向 Ingress 的 LoadBalancer IP
  # 通过域名提供商或内部 DNS 管理工具
  ```
- **后置验证**:
  ```bash
  nslookup <ingress-host>
  # 预期: 解析到正确的 LoadBalancer IP
  ```

### 🟡 中风险

#### REM-002: 重启/修复 Ingress Controller

- **适用根因**: RC-002
- **前置检查**:
  ```bash
  kubectl get pods -n ingress-nginx
  kubectl logs -n ingress-nginx -l app.kubernetes.io/name=ingress-nginx --tail=50
  ```
- **执行命令**:
  ```bash
  # 方案 A: 重启 Ingress Controller
  kubectl rollout restart deployment ingress-nginx-controller -n ingress-nginx

  # 方案 B: 如果配置导致问题，回滚
  kubectl rollout undo deployment ingress-nginx-controller -n ingress-nginx

  # 方案 C: 重新安装
  kubectl apply -f https://raw.githubusercontent.com/kubernetes/ingress-nginx/controller-v1.9.0/deploy/static/provider/cloud/deploy.yaml
  ```
- **后置验证**:
  ```bash
  kubectl get pods -n ingress-nginx
  kubectl get svc -n ingress-nginx
  ```

#### REM-003: 修复后端服务

- **适用根因**: RC-003
- **前置检查**:
  ```bash
  kubectl get endpoints <backend-service> -n <namespace>
  kubectl get pods -n <namespace> -l app=<backend-label>
  ```
- **执行命令**:
  ```bash
  # 根据后端服务具体问题修复
  # 可能涉及: Pod 重启、Deployment 修复、Service 端口修正
  kubectl rollout restart deployment <backend-deployment> -n <namespace>
  ```
- **后置验证**:
  ```bash
  kubectl get endpoints <backend-service> -n <namespace>
  # 预期: 有活跃的 endpoints
  ```

#### REM-005: 安装 Gateway API Controller

- **适用根因**: RC-005
- **前置检查**:
  ```bash
  kubectl get gatewayclass
  # 预期: 无输出（未安装）
  ```
- **执行命令**:
  ```bash
  # 安装 Gateway API CRD
  kubectl apply -f https://github.com/kubernetes-sigs/gateway-api/releases/download/v1.0.0/standard-install.yaml

  # 安装 Istio Gateway Controller（示例）
  istioctl install --set profile=minimal -y
  ```
- **后置验证**:
  ```bash
  kubectl get gatewayclass
  # 预期: 至少一个 GatewayClass
  ```

## 验证确认

### 即时验证

```bash
# V1: Ingress 有地址
kubectl get ingress <name> -n <namespace>

# V2: Controller Running
kubectl get pods -n ingress-nginx

# V3: 后端有 Endpoints
kubectl get endpoints <backend> -n <namespace>

# V4: HTTP 测试
# 从集群内部测试
curl -H "Host: <ingress-host>" http://<ingress-controller-ip>/

# V5: HTTPS 测试（如有 TLS）
curl -k https://<ingress-host>/
```

### 解决确认标准

- [ ] Ingress/Gateway 有分配的地址
- [ ] Ingress Controller Pod Running
- [ ] 后端 Service 有活跃的 Endpoints
- [ ] HTTP 请求返回 200（非 502/503）
- [ ] HTTPS 证书有效（如有 TLS）
- [ ] DNS 解析正确

## 升级协议

### 自动升级条件

| 条件 | 说明 |
|------|------|
| Ingress Controller 完全不可用 | 所有入口中断 |
| 证书链问题 | 需要 CA 介入 |
| DDoS 攻击 | 安全团队响应 |

### 升级消息模板

```
【{severity}】Ingress Gateway Failure - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: Ingress {namespace}/{ingress} 不通
- 影响范围: 
  - 受影响域名: {affected_hosts}
  - 外部访问: {access_status}
- 可能根因: {suspected_root_cause}
- 已尝试修复: {attempted_remediation}
- Skill 版本: SKILL-ING-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
