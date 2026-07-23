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
tier: core
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
sources: ["故障诊断/topic-fta/list/ingress-fta.md"]
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

### 案例1: Ingress TLS 证书过期导致 HTTPS 不可用

**时间线**:
- 00:00 凌晨告警: 多个域名 HTTPS 访问返回证书错误
- 00:05 检查发现 cert-manager 未自动续期（ACME challenge 失败）
- 00:10 确认根因: DNS01 challenge 的 CloudDNS 凭证过期
- 00:20 更新凭证后手动触发续期，证书恢复

**根因链**:
```
DNS提供商凭证过期 → cert-manager ACME challenge失败
→ 证书未续期 → 过期后HTTPS不可用 → 用户看到证书错误
```

**修复**:
```bash
# 🟢 检查证书状态
kubectl get certificates -A -o wide | grep -v "True"
# 🟡 手动触发续期
kubectl delete certificaterequest -n ${NS} ${CERT_NAME}-xxxxx
# 🟢 验证
curl -vI https://${DOMAIN} 2>&1 | grep "expire date"
```

### 案例2: Ingress Controller 过载导致 502

**现象**: 高峰期大量 502 错误，nginx-ingress Pod CPU 超过 90%

**根因**: 单副本 nginx-ingress 无法承载峰值流量，且未配置 HPA

**修复**:
```bash
# 🟡 扩容 Ingress Controller
kubectl scale deployment ingress-nginx-controller -n ingress-nginx --replicas=3
# 🟡 配置 HPA
kubectl autoscale deployment ingress-nginx-controller -n ingress-nginx --min=2 --max=10 --cpu-percent=70
```

## 预防与监控

### 告警规则

```yaml
groups:
- name: ingress-alerts
  rules:
  - alert: IngressCertExpiringSoon
    expr: certmanager_certificate_expiration_timestamp_seconds - time() < 14 * 24 * 3600
    for: 1h
    labels:
      severity: warning
  - alert: IngressHigh5xxRate
    expr: rate(nginx_ingress_controller_requests{status=~"5.."}[5m]) / rate(nginx_ingress_controller_requests[5m]) > 0.05
    for: 5m
    labels:
      severity: critical
```

### 预防措施

| 措施 | 说明 | 优先级 |
|------|------|--------|
| 证书自动续期 | cert-manager + 提前30天告警 | P0 |
| Ingress Controller 高可用 | 至少 2 副本 + 反亲和 | P0 |
| HPA 自动扩容 | 基于 CPU/连接数自动扩展 | P1 |
| 后端健康检查 | 配置 upstream health check | P1 |

## 面试要点

1. **Q: Ingress 返回 502 的排查路径？**
   A: 检查后端 Service/Endpoint 是否有可用 Pod → 查看 Ingress Controller 日志 → 确认后端响应超时 → 检查 NetworkPolicy → 验证 Pod 端口配置

2. **Q: Ingress 与 Gateway API 的区别？**
   A: Ingress 功能有限(仅HTTP路由)；Gateway API 支持 TCP/UDP/gRPC、角色分离、跨命名空间引用、更丰富的流量管理

3. **Q: TLS 证书管理最佳实践？**
   A: cert-manager 自动续期 + DNS01/HTTP01 challenge + 提前告警 + 多 CA 容灾 + 定期验证证书链完整性

## 相关链接

- [[技能/fta-方法论/methodology/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[技能/fta-方法论/execution-engine/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[ts-networking|网络故障排查]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]

---

> 📄 **完整版本**: 本文件为精简版故障树速查。完整的故障树分析（含详细根因推理和决策路径）请查阅：
> [[归档/troubleshooting-diagnostics/FTA故障树/list/ingress-fta.md|Ingress FTA 完整版]]


<!-- risk-assessed -->
