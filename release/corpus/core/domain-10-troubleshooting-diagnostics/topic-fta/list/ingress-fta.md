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



---
title: "[[Ingress|Ingress]] 异常故障树分析"
category: skills
summary: "<!-- condition: kubectl get [[Pods|pods]] -n ingress-nginx -l app=ingress-nginx -o jsonpath='{range .items[?(@.status.phase!=\'Running\')]} {.metadata.name}{\'\n\'}{end}' 显示 Ingress Cont..."
tags: ["k8s", "fta", "troubleshooting"]
sources: ["domain-10-troubleshooting-diagnostics/topic-fta/list/ingress-fta.md"]
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

## 相关链接

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|FTA 诊断执行引擎]]
- [[ts-networking|网络故障排查]]

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[domain-19-landscape-references/topic-index/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/topic-index/network-index.md|Network 网络知识图谱索引]]

---

> 📄 **完整版本**: 本文件为精简版故障树速查。完整的故障树分析（含详细根因推理和决策路径）请查阅：
> [[_archives/troubleshooting-diagnostics/topic-fta/list/ingress-fta.md|Ingress FTA 完整版]]
