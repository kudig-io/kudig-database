---
title: "[2026-04-10] [P1] Ingress 配置错误导致 502 Bad Gateway"
category: case-study
tags: [production, incident, networking, ingress, gateway]
date: "2026-04-10"
severity: P1
mttr: "20min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
---

# [2026-04-10] Nginx Ingress 配置错误导致全站 502 Bad Gateway

## 工单信息
- **工单编号**: INC-2026-0410-008
- **发现时间**: 2026-04-10 10:15 UTC
- **恢复时间**: 2026-04-10 10:35 UTC
- **影响范围**: 所有通过 Ingress 暴露的服务（12 个域名，8 个 namespace）
- **业务影响**: 官网、API 网关、管理后台全部返回 502，用户无法访问

## 问题现象
10:15，监控告警 `Ingress5xxRate > 5%`，随后迅速升至 100%。用户反馈：
- 官网打开显示 "502 Bad Gateway"
- 移动端 App 所有接口报错
- 管理后台无法登录

## 诊断过程

**10:16** — 检查 Ingress 状态：
```bash
kubectl get ingress -A
# NAMESPACE     NAME            CLASS    HOSTS              ADDRESS
# prod-web      main-ingress    nginx    www.example.com    203.0.113.45
# ...

kubectl describe ingress main-ingress -n prod-web
# Events:
#   Warning  Sync    5m    nginx-ingress-controller  
#     "Configuration error: upstream server not found"
```

**10:18** — 检查 Nginx Ingress Controller Pod：
```bash
kubectl get pods -n ingress-nginx
# NAME                                        READY   STATUS    RESTARTS   AGE
# ingress-nginx-controller-7d9f4b8c5a-abc12  0/1     Error     0          5m
# ingress-nginx-controller-7d9f4b8c5a-def34  0/1     Error     0          5m
```

**10:19** — 查看 Ingress Controller 日志：
```bash
kubectl logs -n ingress-nginx ingress-nginx-controller-7d9f4b8c5a-abc12
# 2026/04/10 10:14:45 [emerg] 1234#1234: 
#   invalid number of arguments in "proxy_pass" directive in /etc/nginx/nginx.conf:456
# nginx: [emerg] invalid number of arguments in "proxy_pass" directive
```

**10:21** — 检查近期 Ingress 变更：
```bash
# 查看 Git 提交记录
# Commit: 10:10 UTC, author: dev-zhang
# "feat: add WebSocket support for real-time notification"

git show a1b2c3d --stat
#  ingress-nginx/values.yaml | 4 ++++
#  ingress-nginx/configmap.yaml | 2 ++
```

**10:23** — 检查新添加的配置：
```bash
kubectl get cm ingress-nginx-controller -n ingress-nginx -o yaml | grep -A5 proxy-pass
# proxy-pass-headers: "Upgrade Connection"
```

问题定位：`proxy-pass-headers` 是 Nginx 配置项，但在 Ingress ConfigMap 中应使用 `proxy-pass-headers`（Ingress Nginx 特定注解），不应直接注入到 nginx.conf。实际上，Ingress Nginx Controller 的 ConfigMap 正确键应为 `proxy-pass-headers`，但本例中的错误是运维人员在 ConfigMap 中添加了非法的 `proxy-pass` 值（包含空格的无效参数）。

更准确地说，经过排查，是以下变更导致：
```yaml
# ingress-nginx/configmap.yaml
data:
  proxy-pass: "http://upstream"  # 非法！这不是有效的 ConfigMap 键值
```

Nginx Ingress Controller 将 ConfigMap 的数据作为 nginx 配置模板变量注入，错误的 `proxy-pass` 键值生成了无效的 nginx 配置。

## 根因
运维人员在 10:10 为支持 WebSocket 修改 Ingress Nginx Controller ConfigMap，误添加了无效的 `proxy-pass` 配置项。Ingress Controller 重新加载配置时 nginx 语法检查失败，Controller 进入 `CrashLoopBackOff` 状态，所有 Ingress 规则无法加载，外部请求全部返回 502。

## 修复动作

**10:25** — 回滚 ConfigMap：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 从 Git 回滚到上一个版本
git checkout HEAD~1 -- ingress-nginx/configmap.yaml
kubectl apply -f ingress-nginx/configmap.yaml
```

**10:28** — 重启 Ingress Controller：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart deployment ingress-nginx-controller -n ingress-nginx
kubectl get pods -n ingress-nginx -w
# ingress-nginx-controller-xxx  1/1   Running   0   30s
```

**10:30** — 验证 Nginx 配置：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n ingress-nginx ingress-nginx-controller-xxx -- nginx -t
# nginx: the configuration file /etc/nginx/nginx.conf syntax is ok
# nginx: configuration file /etc/nginx/nginx.conf test is successful
```

**10:32** — 验证 Ingress 规则加载：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n ingress-nginx ingress-nginx-controller-xxx -- \
  cat /etc/nginx/nginx.conf | grep -c "server_name www.example.com"
# 1
```

**10:33** — 外部验证：
```bash
curl -I https://www.example.com
# HTTP/2 200
# ...
```

## 验证
- 10:34 — 官网、App、管理后台全部恢复 200
- 10:35 — 502 错误率归零，业务恢复正常

## 复盘
- **直接原因**: Ingress Nginx ConfigMap 添加无效 `proxy-pass` 配置 → nginx 语法错误 → Controller 崩溃 → 全部 Ingress 规则失效 → 502
- **根本原因**: 配置变更未经过 nginx 语法校验，直接应用到生产环境
- **改进措施**:
  1. CI/CD Pipeline 中添加 `nginx -t` 语法检查步骤
  2. Ingress Controller ConfigMap 变更必须经过金丝雀环境验证（≥30min）
  3. 禁止直接修改 `ingress-nginx` namespace 的 ConfigMap，所有变更通过 Helm/ArgoCD 提交
  4. 为 Ingress Controller 添加 `nginx_config_test_fail` 告警，触发 P0
- **相关 Skill**: [[k8s-network-configuration-guide]]
- **相关 FTA**: [[ingress-fta]]
