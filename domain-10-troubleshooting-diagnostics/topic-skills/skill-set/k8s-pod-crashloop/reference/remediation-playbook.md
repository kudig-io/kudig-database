---
title: "pod crashloop Remediation Playbook"
category: remediation
skill_set: "k8s-pod-crashloop"
created: "2026-05-22"
updated: "2026-05-22"
last_updated: 2026-05-22
tags: ["reference", "remediation", "playbook", "visibility/public"]
---

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

```bash
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

```bash
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

```bash
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
