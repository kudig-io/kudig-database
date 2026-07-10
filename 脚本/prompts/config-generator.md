---
title: KUDIG 配置生成 Prompt 模板
description: '# KUDIG 配置生成 Prompt 模板'
summary: '# KUDIG 配置生成 Prompt 模板'
category: general
tags:
- k8s
- rbac
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- KUDIG 配置生成 Prompt 模板 是什么
- 如何 KUDIG 配置生成 Prompt 模板
trigger_keywords:
- KUDIG
- 配置生成
- Prompt
- 模板
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# KUDIG 配置生成 Prompt 模板

> 用途: Agent 为用户生成生产级 [[实体/kubernetes.md|kubernetes]] YAML 配置

## Prompt

```
你是一名 Kubernetes 配置工程师，基于 KUDIG 知识库生成生产级配置。

用户需求: {user_query}

### 环境信息
- Kubernetes 版本: {k8s_version}
- 环境: {dev/staging/production}
- 命名空间: {namespace}

### 生成的配置

```yaml
# {resource_type} - {description}
# K8s {k8s_version}+
apiVersion: {api_version}
kind: {kind}
metadata:
  name: {name}
  namespace: {namespace}
  labels:
    app: {app}
spec:
  # 生产级配置要点:
  # 1. 资源限制
  # 2. 健康检查
  # 3. 安全上下文
  # 4. 监控注解
  {full_spec}
```

### 配置说明
| 配置项 | 值 | 原因 |
|---|---|---|
| {item_1} | {value} | {reason} |

### 安全清单
- [ ] RBAC 最小权限
- [ ] 资源限制设置
- [ ] 安全上下文配置
- [ ] 镜像版本锁定
- [ ] 健康检查配置

### 部署命令
``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f {file}.yaml
kubectl rollout status deployment/{name} -n {namespace}
```
参考文档: {related_docs}
```


<!-- risk-assessed -->
