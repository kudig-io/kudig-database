---
title: 'Skill: 蓝绿部署切换失败的诊断和修复'
summary: 'Skill: 蓝绿部署切换失败的诊断和修复：蓝绿部署切换后流量未正确路由到新版本（绿环境），或切换后业务指标异常需要回滚。远程顾问模式下需验证
  Service selector 变更、Pod 就绪状态及负载均衡器健康检查。'
category: skill
tags:
- skill
- domain-10
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




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
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get service <service-name> -n <namespace> -o jsonpath='{.spec.selector}'
kubectl get endpoints <service-name> -n <namespace>
```
> 确认 selector 中的 `version` 标签已指向目标环境（green/blue），且 Endpoints 列表中的 IP 与目标环境的 Pod IP 一致。
> 如果无法执行，替代方案：请用户提供 Service 的 YAML 截图，或描述当前流量表现（如返回的版本号、日志特征）。

### 步骤2: 检查目标环境 Pod 就绪状态
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n <namespace> -l version=green -o wide
kubectl describe pod <green-pod-name> -n <namespace>
```
> 确认绿环境所有 Pod 均为 Running 且 Ready，readinessProbe 通过，无 CrashLoopBackOff 或 ImagePullBackOff。

### 步骤3: 验证负载均衡器及外部访问路径
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

## 生产案例

### 案例 1：绿环境 Pod 未全部就绪即切换导致 502

**背景**：某 SaaS 平台执行蓝绿切换时，绿环境 10 个 Pod 中仅 6 个 Ready，切换后 40% 请求返回 502。

**时间线**：
| 时间 | 事件 | 操作 |
|------|------|------|
| 09:00 | 绿环境 Deployment 创建，10 副本 | 🟢 `kubectl get deploy -n prod -l version=green` |
| 09:03 | 6/10 Pod Ready，运维人员误判全部就绪 | 🟢 `kubectl get pods -n prod -l version=green \| grep Running` |
| 09:04 | 切换 Service selector 到 green | 🟡 `kubectl patch svc web -n prod -p '{"spec":{"selector":{"version":"green"}}}'` |
| 09:05 | 40% 请求 502，触发告警 | 🟢 `curl -I https://app.example.com` |
| 09:06 | 立即回滚 selector 到 blue | 🟡 `kubectl patch svc web -n prod -p '{"spec":{"selector":{"version":"blue"}}}'` |

**根因**：绿环境 Pod 启动需要连接数据库预热（~60s），运维未等待全部 Ready 即切换。

### 案例 2：数据库 Schema 不兼容导致回滚失败

**背景**：绿环境执行了数据库 migration（删除旧字段），切换后发现 bug 需回滚，但蓝环境代码依赖已删除字段。

**教训**：蓝绿部署的数据库变更必须向前兼容（expand-contract 模式），切换后至少保留蓝环境一个完整业务周期。

## 升级决策点

- **P0（立即回滚）**：切换后核心业务不可用，错误率 >5%
- **P1（评估回滚）**：切换后部分功能异常，错误率 1-5%，评估影响后决定
- **P2（观察）**：仅性能微降，无功能影响，继续观察

## 面试要点

1. **Q: 蓝绿部署如何处理数据库 Schema 变更？**
   A: 采用 expand-contract 模式：① expand 阶段添加新字段/表（兼容旧代码）；② 绿环境使用新 Schema；③ 确认绿环境稳定后，contract 阶段清理旧字段。绝不能在切换前执行破坏性 migration。

2. **Q: 蓝绿部署与滚动更新的区别和适用场景？**
   A: 滚动更新逐步替换 Pod，资源开销小但回滚慢；蓝绿需要双倍资源但切换/回滚瞬时完成。蓝绿适合：重大版本升级、需要完整验证的场景、对回滚速度要求极高的核心服务。

3. **Q: 如何确保蓝绿切换的原子性？**
   A: ① 使用单次 `kubectl patch` 修改 selector（原子操作）；② 切换前确认绿环境所有 Pod Ready；③ 使用自动化工具（Argo Rollouts）而非手动操作；④ 切换后立即验证 Endpoints 和流量；⑤ 保留蓝环境作为快速回滚窗口。

## 相关概念

- [[22-概念/09-平台与发布/blue-green-deployment.md|蓝绿部署]] — 蓝绿发布切换机制、流量路由与回滚策略
- [[22-概念/02-工作负载/deployment-controller-architecture.md|Deployment 控制器架构]] — Deployment 滚动更新与副本管理原理
- [[22-概念/09-平台与发布/canary-deployment.md|金丝雀部署]] — 渐进式发布策略与流量权重控制

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
