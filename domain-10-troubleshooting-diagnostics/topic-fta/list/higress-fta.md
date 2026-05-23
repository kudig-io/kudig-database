---
title: Higress 网关异常故障树分析 (skills)
description: '### 故障排查命令速查'
category: skills
tags:
- k8s
- fta
- troubleshooting
- envoy
- ingress
- gateway
- wasm
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Higress 网关异常故障树分析 是什么
- 如何 Higress 网关异常故障树分析
trigger_keywords:
- Higress
- 网关异常故障树分析
prerequisites:
- kubectl-basics
fta_id: FTA-HIGRESS-001
component: Higress
severity: high
created: "2026-05-23"
---

# Higress 网关异常故障树分析

### 故障排查命令速查

```bash
# 1. 检查 Higress 系统组件状态
kubectl get pods -n higress-system

# 2. 检查 Higress 网关日志
kubectl logs -n higress-system -l app=higress-gateway --tail=200 -f

# 3. 检查 Ingress 配置
kubectl get ingress -A
kubectl describe ingress <name> -n <namespace>

# 4. 查看 Envoy 配置
kubectl exec -it <higress-gateway-pod> -c envoy -- curl localhost:15000/config_dump

# 5. 检查 xDS 同步状态
kubectl exec -it <higress-gateway-pod> -c envoy -- curl localhost:15000/clusters

# 6. 检查 McpBridge 配置
kubectl get mcphbridge -A
kubectl describe mcphbridge <name> -n <namespace>

# 7. 检查 WasmPlugin 配置
kubectl get wasmplugin -A

# 8. 测试路由
kubectl exec -it <test-pod> -- curl -H "Host: app.example.com" http://<higress-gateway>:80/

# 9. 检查 Nacos 连接
kubectl exec -it <higress-gateway-pod> -- curl nacos:8848/v1/ns/instance/list?serviceName=<svc>

# 10. 检查 TLS 证书
kubectl get secret -n higress-system | grep -E "tls|cert"
openssl s_client -connect <gateway>:443 -servername <sni>
```

---

## 相关链接

- [[skills/FTA Methodology and Core Principles|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]

## Related

- [[skills/ts-cluster-operations|ts-cluster-operations]] — 集群运维故障排查
- storage.md|ts-storage]] — 存储故障排查
- [[skills/skill-19-node-resource-pressure|skill-19-node-resource-pressure]] — 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation
- [[certificate-fta]] — 证书异常故障树分析
- [[envoy]] — Envoy

- [[domain-10-troubleshooting-diagnostics/topic-fta/list/higress-fta|Higress 网关异常故障树分析]]
- [[skills/skill-README|topic-skills — 工单智能体 Kubernetes 诊断 Skill 库]] — Cross-reference
- [[skills/FTA-Driven Runbook Automation|FTA-Driven Runbook Automation]] — Cross-reference
- [[domain-19-landscape-references/topic-index/higress-index|Higress 知识图谱索引]]
