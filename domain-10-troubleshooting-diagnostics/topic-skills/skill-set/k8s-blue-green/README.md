---
title: "Skill: 蓝绿部署切换失败的诊断和修复"
category: skill
tags: ["skill", "domain-10", "visibility/public"]
sources: ["KUDIG Gap Analysis 2026-05-21"]
created: 2026-05-21
updated: 2026-05-21
status: reviewed
---

# Skill: 蓝绿部署切换失败的诊断和修复

## 问题描述
蓝绿部署切换后流量未正确路由到新版本（绿环境），或切换后业务指标异常需要回滚。远程顾问模式下需验证 Service selector 变更、Pod 就绪状态及负载均衡器健康检查。

## 常见症状
- 切换 Service selector 后，请求仍被路由到旧版本（蓝环境）
- 绿环境 Pod 未全部就绪，但已切换流量导致部分请求失败
- 切换后错误率上升、响应延迟增大
- 回滚时修改 selector 但流量未立即切回蓝环境
- 外部负载均衡器健康检查失败，导致服务整体不可用

## 诊断步骤

### 步骤1: 验证 Service selector 是否已更新
```bash
kubectl get service <service-name> -n <namespace> -o jsonpath='{.spec.selector}'
kubectl get endpoints <service-name> -n <namespace>
```
> 确认 selector 中的 `version` 标签已指向目标环境（green/blue），且 Endpoints 列表中的 IP 与目标环境的 Pod IP 一致。
> 如果无法执行，替代方案：请用户提供 Service 的 YAML 截图，或描述当前流量表现（如返回的版本号、日志特征）。

### 步骤2: 检查目标环境 Pod 就绪状态
```bash
kubectl get pods -n <namespace> -l version=green -o wide
kubectl describe pod <green-pod-name> -n <namespace>
```
> 确认绿环境所有 Pod 均为 Running 且 Ready，readinessProbe 通过，无 CrashLoopBackOff 或 ImagePullBackOff。

### 步骤3: 验证负载均衡器及外部访问路径
```bash
kubectl get service <service-name> -n <namespace> -o jsonpath='{.status.loadBalancer.ingress}'
curl -H "Host: <domain>" http://<lb-ip>/version
```
> 检查云提供商负载均衡器或 MetalLB 的健康检查状态，确认外部访问路径已指向正确的后端节点和端口。

## 修复措施
- **Selector 未生效**：确认 selector 修改已保存，等待 kube-proxy 同步规则（通常秒级），必要时重启 kube-proxy Pod
- **Pod 未就绪**：暂停切换，排查绿环境 Pod 未就绪原因（健康检查失败、启动慢、依赖未就绪）
- **会话中断**：在切换前优雅终止长连接，或使用支持连接耗尽（connection draining）的负载均衡器
- **指标异常回滚**：立即将 Service selector 切回蓝环境，保留绿环境用于问题排查
- **缓存/CDN 干扰**：清除 CDN 缓存，确认客户端未因本地缓存而访问旧版本
- **数据库兼容性**：确认蓝绿环境共享的数据库 schema 已向前兼容，必要时执行 schema 变更回滚

## 预防性措施
- 切换前在绿环境执行完整的冒烟测试和集成验证，确保所有 Pod 就绪后再切流量
- 保留蓝环境至少一个业务周期（如 30 分钟）后再清理，确保回滚窗口可用
- 使用自动化脚本执行 selector 切换，减少手动修改导致的拼写错误

## 相关概念

- [[concepts/blue-green-deployment|蓝绿部署]] — 蓝绿发布切换机制、流量路由与回滚策略
- [[concepts/deployment-controller-architecture|Deployment 控制器架构]] — Deployment 滚动更新与副本管理原理
- [[concepts/canary-deployment|金丝雀部署]] — 渐进式发布策略与流量权重控制
