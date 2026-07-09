---
title: Higress 网关异常故障树分析 (skills)
description: '### 故障排查命令速查'
summary: '### 故障排查命令速查'
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
tier: peripheral
created: '2026-05-23'
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Higress 网关异常故障树分析

### 故障排查命令速查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
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

- [[skills/FTA Methodology and Core Principles.md|FTA 方法论]]
- [[skills/FTA Diagnostic Execution Engine.md|[[FTA 诊断执行引擎|FTA 诊断执行引擎]]]]

## Related

- [[skills/ts-cluster-operations.md|ts-cluster-operations]] — 集群运维故障排查
- storage.md|ts-storage]] — 存储故障排查
- [[skills/skill-19-node-resource-pressure.md|skill-19-node-resource-pressure]] — 节点资源压力诊断与修复 / Node Resource Pressure Diagnosis & Remediation
- [[certificate-fta]] — 证书异常故障树分析
- [[envoy]] — Envoy

- [[故障诊断/topic-fta/list/higress-fta.md|Higress 网关异常故障树分析]]
- [[skills/skill-README.md|topic-skills — 工单智能体 Kubernetes 诊断 Skill 库]] — Cross-reference
- [[skills/FTA-Driven Runbook Automation.md|FTA-Driven Runbook Automation]] — Cross-reference
- [[生态参考/topic-index/higress-index.md|Higress 知识图谱索引]]


<!-- risk-assessed -->
