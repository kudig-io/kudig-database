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



# KUDIG 配置生成 Prompt 模板

> 用途: Agent 为用户生成生产级 [[entities/kubernetes.md|kubernetes]] YAML 配置

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
```bash
kubectl apply -f {file}.yaml
kubectl rollout status deployment/{name} -n {namespace}
```

参考文档: {related_docs}
```
