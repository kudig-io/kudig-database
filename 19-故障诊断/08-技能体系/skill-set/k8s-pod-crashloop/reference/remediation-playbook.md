---
title: pod crashloop Remediation Playbook
summary: pod crashloop Remediation Playbook：诊断结果 ├── 退出码 137 (OOMKilled) │   ├── 内存
  limit 过低 → 增加 limit │   ├── 内存泄漏 → 修复应用代码 │   └── 节点内存不足 → 节点扩容/驱逐 ├── 退出码 1 (应用错误)
  │   ├── 配置错误 → 修正 ConfigMap/Secret │  ...
category: remediation
tags:
- reference
- remediation
- playbook
- visibility/public
tier: supporting
created: '2026-05-22'
updated: '2026-05-22'
skill_set: k8s-pod-crashloop
last_updated: 2026-05-22
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Pod CrashLoopBackOff / OOMKilled 修复手册

## 修复流程图

```
诊断结果
├── 退出码 137 (OOMKilled)
│   ├── 内存 limit 过低 → 增加 limit
│   ├── 内存泄漏 → 修复应用代码
│   └── 节点内存不足 → 节点扩容/驱逐
├── 退出码 1 (应用错误)
│   ├── 配置错误 → 修正 ConfigMap/Secret
│   ├── 依赖未就绪 → 添加 initContainer
│   └── 代码 Bug → 回滚版本
├── 退出码 143 (SIGTERM)
│   └── terminationGracePeriod 过短 → 延长并优化优雅停机
└── 健康检查失败
    ├── livenessProbe 过于敏感 → 放宽阈值
    └── startupProbe 缺失 → 添加 startupProbe
```

## 具体修复步骤

### 修复 1：增加内存限制（OOMKilled）

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查当前限制
kubectl get pod <pod> -o jsonpath='{.spec.containers[0].resources}'

# 编辑 Deployment 增加 limit
kubectl patch deployment <deployment> -p '{"spec":{"template":{"spec":{"containers":[{"name":"<container>","resources":{"limits":{"memory":"512Mi"},"requests":{"memory":"256Mi"}}}]}}}}'

# 验证
kubectl get pod <new-pod> -o jsonpath='{.spec.containers[0].resources}'
```
### 修复 2：回滚到上一个版本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看历史版本
kubectl rollout history deployment/<deployment>

# 回滚到上一个版本
kubectl rollout undo deployment/<deployment>

# 验证
kubectl rollout status deployment/<deployment>
```
### 修复 3：修正健康检查配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 编辑 Deployment
kubectl edit deployment <deployment>

# 添加/修改 startupProbe
startupProbe:
  httpGet:
    path: /health/startup
    port: 8080
  failureThreshold: 30
  periodSeconds: 10
```
## 回滚方案

所有修改均可通过 `kubectl rollout undo` 回滚。

## 验证清单

- [ ] Pod 状态 Running
- [ ] Restart Count 为 0 或不再增长
- [ ] 应用日志无 ERROR/FATAL
- [ ] 服务响应正常

## 升级决策点

- **P0（立即升级）**：核心服务所有 Pod CrashLoop，服务完全不可用
- **P1（30分钟内升级）**：部分 Pod CrashLoop，服务降级但可用
- **P2（观察）**：仅单个非关键 Pod 异常，有充分副本支撑

## 生产注意事项

1. 查看上一次崩溃日志：`kubectl logs <pod> --previous` 获取崩溃前的关键信息
2. 使用 `kubectl debug` 创建临时调试容器，避免修改生产 Pod
3. 修改 livenessProbe 时注意：过短的 `initialDelaySeconds` 会导致慢启动应用反复被杀
4. OOMKilled 时检查 `kubectl describe pod` 中的 Last State 确认退出码 137
5. 配置变更导致的 CrashLoop 可用 `kubectl rollout undo` 快速回滚

## 面试要点

1. **Q: CrashLoopBackOff 的退避机制是什么？**
   A: 容器每次崩溃后等待时间指数增长：10s、20s、40s、80s、160s、300s（封顶 5min）。状态显示为 CrashLoopBackOff 时实际是在等待下次重启。通过 `kubectl get pod -o jsonpath='{.status.containerStatuses[0].lastState}'` 查看上次崩溃原因。

2. **Q: 如何区分应用崩溃和探针失败导致的重启？**
   A: 查看 `kubectl describe pod` 的 Last State：Reason=Error 表示应用自己退出（非零退出码）；Reason=OOMKilled 表示内存超限；Events 中有 "Liveness probe failed" 表示探针失败被 kubelet 杀死。退出码 137=SIGKILL（OOM或探针），139=段错误，1=应用错误。

3. **Q: 生产环境如何快速定位 CrashLoop 根因？**
   A: ① `kubectl logs --previous` 查看崩溃前日志；② `kubectl describe pod` 查看 Events 和 Last State；③ 检查最近变更（`kubectl rollout history`）；④ 检查依赖服务状态（DB/Redis/MQ）；⑤ 使用 `kubectl debug` 创建临时容器检查文件系统/网络。

## Related

- [[reference|#reference Hub]] — tag hub

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
