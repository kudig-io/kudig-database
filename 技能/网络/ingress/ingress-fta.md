---
title: Ingress 异常故障树分析 (skills)
description: '| CTRL3 | 配置重载失败 | `kubectl logs -n ${INGRESS_NS} -l app.kubernetes.io/name=ingress-nginx
  --tail=100 | grep -iE "reload.*fail|error.*config"` | `reload.*fail|error` |
  确认配置重载问题 |'
summary: '| CTRL3 | 配置重载失败 | `kubectl logs -n ${INGRESS_NS} -l app.kubernetes.io/name=ingress-nginx
  --tail=100 | grep -iE "reload.*fail|error.*config"` | `reload.*fail|error` |
  确认配置重载问题 |'
category: general
tags:
- k8s
- ingress
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Ingress 异常故障树分析 是什么
- 如何 Ingress 异常故障树分析
trigger_keywords:
- Ingress
- 异常故障树分析
prerequisites:
- kubectl-basics
fta_id: FTA-INGRESS-001
component: Ingress
severity: critical
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: "[[Ingress|Ingress]] 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get [[Pods|pods]] -n ingress-nginx -l app=ingress-nginx -o jsonpath='{range .items[?(@.status.phase!=\'Running\')]} {.metadata.name}{\'\n\'}{end}' 显示 Ingress Cont..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["故障诊断/FTA故障树/list/ingress-fta.md"]
created: 2026-05-21
updated: 2026-05-21
lifecycle: reviewed
lifecycle_changed: "2026-05-21"
tier: supporting
base_confidence: 0.7
---

# Ingress 异常故障树分析

### 诊断命令快速参考表

### 1. Ingress Controller 诊断

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|----------|--------------|------|
| CTRL1A | OOMKilled | `kubectl get pods -n ${INGRESS_NS} -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{range .items[*]}{.status.containerStatuses[*].lastState.terminated.reason}{"\n"}{end}'` | `OOMKilled` | 确认内存溢出 |
| CTRL1B | CrashLoopBackOff | `kubectl get pods -n ${INGRESS_NS} -l app.kubernetes.io/name=ingress-nginx -o wide` | `CrashLoopBackOff` | 确认容器崩溃 |
| CTRL1C | 镜像拉取失败 | `kubectl get pods -n ${INGRESS_NS} -l app.kubernetes.io/name=ingress-nginx -o jsonpath='{.items[*].status.containerStatuses[*].state.waiting.reason}'` | `ImagePullBackOff|ErrImagePull` | 确认镜像问题 |
| CTRL3 | 配置重载失败 | `kubectl logs -n ${INGRESS_NS} -l app.kubernetes.io/name=ingress-nginx --tail=100 | grep -iE "reload.*fail|error.*config"` | `reload.*fail|error` | 确认配置重载问题 |
| CTRL4 | 资源压力 | `kubectl top pods -n ${INGRESS_NS} -l app.kubernetes.io/name=ingress-nginx` | CPU/内存使用 | 检查资源消耗 |

### 2. 规则/路由诊断

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|----------|--------------|------|
| RULE1A | Host 不匹配 | `kubectl get ingress -n ${NAMESPACE} ${INGRESS_NAME} -o jsonpath='{.spec.rules[*].host}'` | Host 列表 | 验证 Host 配置 |
| RULE1B | Path 正则错误 | `kubectl get ingress -n ${NAMESPACE} ${INGRESS_NAME} -o jsonpath='{.spec.rules[*].http.paths[*].path}'` | Path 列表 | 检查 Path 配置 |
| RULE2 | Backend 端口错误 | `kubectl get ingress -n ${NAMESPACE} ${INGRESS_NAME} -o jsonpath='{.spec.rules[*].http.paths[*].backend}'` | Backend 配置 | 验证端口映射 |
| RULE3 | Annotation 配置 | `kubectl get ingress -n ${NAMESPACE} ${INGRESS_NAME} -o jsonpath='{.metadata.annotations}'` | Annotations | 检查注解配置 |
| RULE4 | IngressClass | `kubectl get ingress -n ${NAMESPACE} ${INGRESS_NAME} -o jsonpath='{.spec.ingressClassName}'` | IngressClass 名称 | 验证 Class 匹配 |

### 3. TLS 证书诊断

| 节点 ID | 名称 | 诊断命令 | 预期输出模式 | 判定 |
|---------|------|----------|--------------|------|
| TLS1A | 证书过期 | `kubectl get secret ${TLS_SECR
...(截断)

## 生产案例

### 案例 1: Ingress 证书过期导致 HTTPS 全量 502

| 时间 | 事件 |
|------|------|
| 08:00 | 用户报告网站无法访问，浏览器显示证书错误 |
| 08:05 | `kubectl get secret tls-secret -n prod -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates` 确认已过期 |
| 08:10 | 🔴 更新 TLS Secret，Ingress Controller 自动重载 |
| 08:12 | HTTPS 恢复正常 |

**根因**: 证书 1 年有效期到期，未配置 cert-manager 自动续期。

### 案例 2: Ingress 后端 Service 端口不匹配导致 503

**现象**: Ingress 规则配置正确但访问返回 503 Service Unavailable。

**诊断**: `kubectl describe ingress` 后端端口 8080，但 Service targetPort 实际为 80

**修复**: 🟢 修正 Ingress backend port 与 Service port 一致

## 升级决策点

| 级别 | 条件 | 动作 |
|------|------|------|
| P0 | 全部域名不可访问 | 检查 Ingress Controller Pod 状态 |
| P1 | 单域名/路径异常 | 检查 Ingress 规则和后端 Service |
| P2 | 证书即将过期 | 配置 cert-manager 自动续期 |

## 面试要点

1. **Q: Ingress Controller 的工作原理是什么？**
   A: Ingress Controller watch Ingress 资源变更，将规则转换为反向代理配置(nginx.conf)，通过 reload 或动态更新生效。流量路径: Client → LB → Ingress Pod → Service → Pod。

2. **Q: Ingress 与 Gateway API 的主要区别？**
   A: Ingress: 简单 HTTP 路由，扩展性差(annotations)；Gateway API: 角色分离(Infra Provider/Cluster Operator/App Developer)，支持 TCP/UDP/gRPC，原生流量分割、Header 匹配。

3. **Q: Ingress TLS 证书管理的最佳实践？**
   A: 使用 cert-manager + Let's Encrypt/ACME 自动签发续期，配置 ClusterIssuer，Secret 存储在对应 namespace，启用 HSTS 和自动重定向。

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[ts-networking|网络故障排查]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]


<!-- risk-assessed -->
