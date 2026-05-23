---
title: "Skill: 金丝雀发布异常的诊断和修复"
category: skill
tags: ["skill", "domain-10", "visibility/public"]
sources: ["KUDIG Gap Analysis 2026-05-21"]
created: 2026-05-21
updated: 2026-05-21
status: reviewed
---

# Skill: 金丝雀发布异常的诊断和修复

## 问题描述
金丝雀发布过程中新版本（金丝雀）表现异常，如错误率升高、延迟增大或业务指标下降，需要快速判断是继续观察、暂停推广还是立即回滚。远程顾问模式下需基于用户提供的监控数据和日志给出决策建议。

## 常见症状
- 金丝雀版本的错误率（5xx）较基线上升超过 0.1%
- P99 延迟较基线上升超过 20%
- 金丝雀 Pod 的 CPU/内存使用率显著高于稳定版本
- 特定请求头或用户群体的流量路由异常
- Ingress/Service Mesh 权重设置未生效，流量比例不符合预期

## 诊断步骤

### 步骤1: 确认金丝雀流量比例与路由规则
```bash
kubectl get pods -n <namespace> -l version=canary
kubectl get ingress <ingress-name> -n <namespace> -o yaml | grep -A5 canary
kubectl get virtualservice <vs-name> -n <namespace> -o yaml
```
> 确认金丝雀 Deployment 的副本数、Ingress 的 `canary-weight` annotation 或 Istio VirtualService 的 weight 配置与预期一致。
> 如果无法执行，替代方案：请用户提供当前金丝雀与稳定版本的 Pod 数量，以及 Ingress/Service Mesh 控制台中的权重截图。

### 步骤2: 对比金丝雀与稳定版本的关键指标
```bash
kubectl logs -l version=canary -n <namespace> --tail=100 | grep -i error
kubectl top pods -n <namespace> -l version=canary
kubectl top pods -n <namespace> -l version=stable
```
> 收集金丝雀版本的错误日志、资源消耗，并与稳定版本进行对比，确认异常是资源相关还是代码逻辑相关。

### 步骤3: 检查金丝雀 Pod 健康与配置差异
```bash
kubectl describe pod <canary-pod> -n <namespace>
kubectl get deployment <canary-deployment> -n <namespace> -o yaml | grep -A10 env
```
> 对比金丝雀与稳定版本的环境变量、ConfigMap、Secret 挂载是否一致，确认无配置漂移或缺失。

## 修复措施
- **流量比例过高**：降低金丝雀权重至 1%-5%，缩小影响面后继续观察
- **健康检查未通过**：检查 readinessProbe 配置，确保金丝雀 Pod 完全就绪后再接收流量
- **资源不足**：为金丝雀版本提高 request/limit，或扩容金丝雀副本数以分散负载
- **配置漂移**：对比稳定与金丝雀版本的环境变量和配置挂载，修正差异后重新部署
- **一键回滚**：将 Ingress/VirtualService 权重归零，或直接缩容金丝雀 Deployment 至 0 副本
- **渐进推广节奏异常**：暂停自动推进，按 5% → 10% → 25% → 50% → 100% 手动控制节奏，每阶段观察 15-30 分钟

## 相关概念
- [[canary-deployment]]
- [[deployment-troubleshooting]]
- [[autoscaling-strategies]]
