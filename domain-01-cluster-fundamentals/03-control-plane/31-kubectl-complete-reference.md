---
title: 31 - kubectl 完全命令参考 (kubectl Complete Reference)
description: 2. [资源创建与管理](#2-资源创建与管理-create-apply-delete-replace)
summary: 2. [资源创建与管理](#2-资源创建与管理-create-apply-delete-replace)
category: control-plane
tags:
- k8s
- control-plane
- etcd
- apiserver
- scheduler
- controller-manager
- kubelet
- helm
- docker
- mysql
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 30min
intent_queries:
- kubectl 完全命令参考 (kubectl Complete Reference) 是什么
- 如何 kubectl 完全命令参考 (kubectl Complete Reference)
- Kubernetes 3 control plane 最佳实践
trigger_keywords:
- kubectl
- 完全命令参考
- kubectl
- Complete
- Reference
- control
- plane
prerequisites:
- kubectl-basics
- kubernetes-concepts
- helm-basics
- etcd-basics
- mysql-basics
- gpu-scheduling-basics
- tls-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-02-workloads-applications/
  label: '相关知识域: domain-02-workloads-applications'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-04-storage-data/
  label: '相关知识域: domain-04-storage-data'
- type: domain
  path: ../domain-05-security-compliance/
  label: '相关知识域: domain-05-security-compliance'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---



# 31 - kubectl 完全命令参考 (kubectl Complete Reference)

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25 - v1.32+ | **最后更新**: 2026-04 | **文档类型**: 命令参考手册

---

<!-- chunk: 目录 -->
## 目录

1. [核心命令族概览](#1-核心命令族概览)
2. [资源创建与管理](#2-资源创建与管理-create-apply-delete-replace)
3. [资源查看与诊断](#3-资源查看与诊断-get-describe-explain-top-logs-events)
4. [工作负载运维](#4-工作负载运维-rollout-scale-autoscale-set)
5. [集群与配置管理](#5-集群与配置管理-config-cluster-info-version-api-resources)
6. [容器交互与调试](#6-容器交互与调试-exec-logs-port-forward-cp-attach-debug)
7. [网络与服务暴露](#7-网络与服务暴露-expose-port-forward-proxy)
8. [节点管理](#8-节点管理-cordon-uncordon-drain-taint)
9. [权限与安全](#9-权限与安全-auth-certificate-create-token)
10. [高级特性](#10-高级特性-diff-kustomize-patch-wait-annotate-label)
11. [Shell 自动补全与插件](#11-shell-自动补全与插件)
12. [生产环境速查表](#12-生产环境速查表)

---

<!-- chunk: 1. 核心命令族概览 -->
## 1. 核心命令族概览

### 1.1 kubectl 命令分类架构

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                          kubectl Command Families                                 │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐               │
│  │   资源管理        │  │   运维诊断        │  │   集群配置        │               │
│  │  create          │  │  get             │  │  config          │               │
│  │  apply           │  │  describe        │  │  cluster-info    │               │
│  │  delete          │  │  explain         │  │  version         │               │
│  │  replace         │  │  logs            │  │  api-resources   │               │
│  │  patch           │  │  top             │  │  api-versions    │               │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘               │
│                                                                                  │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐               │
│  │   工作负载        │  │   容器交互        │  │   权限安全        │               │
│  │  rollout         │  │  exec            │  │  auth            │               │
│  │  scale           │  │  port-forward    │  │  certificate     │               │
│  │  autoscale       │  │  cp              │  │  create token    │               │
│  │  set             │  │  attach          │  │  create role     │               │
│  │  wait            │  │  debug           │  │                  │               │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘               │
│                                                                                  │
│  ┌──────────────────┐  ┌──────────────────┐  ┌──────────────────┐               │
│  │   网络服务        │  │   节点管理        │  │   高级特性        │               │
│  │  expose          │  │  cordon          │  │  diff            │               │
│  │  proxy           │  │  uncordon        │  │  kustomize       │               │
│  │  port-forward    │  │  drain           │  │  annotate        │               │
│  │                  │  │  taint           │  │  label           │               │
│  │                  │  │                  │  │  plugin          │               │
│  └──────────────────┘  └──────────────────┘  └──────────────────┘               │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 全局选项速查

| 全局选项 | 说明 | 生产环境用法 |
|----------|------|--------------|
| `--context <name>` | 指定集群上下文 | `kubectl --context=prod get nodes` |
| `--namespace <ns>` / `-n` | 指定命名空间 | `kubectl -n kube-system get [[Pods|pods]]` |
| `--all-namespaces` / `-A` | 所有命名空间 | `kubectl get pods -A` |
| `--kubeconfig <path>` | 指定配置文件 | `kubectl --kubeconfig=/etc/k8s/admin.conf get nodes` |
| `--server <host:port>` | 直接指定 API Server | `kubectl --server=https://apiserver:6443 get nodes` |
| `--token <token>` | 使用 Bearer Token | `kubectl --token=$TOKEN get pods` |
| `--insecure-skip-tls-verify` | 跳过 TLS 验证 | **仅限测试/恢复场景** |
| `-v` / `-v=6` | 日志详细程度 | `-v=6` 显示 HTTP 请求详情 |
| `--dry-run=<mode>` | 模拟执行 | `client` 或 `server` |
| `-o <format>` | 输出格式 | `yaml`, `json`, `wide`, `name`, `custom-columns` |

### 1.3 输出格式详解

| 输出格式 | 说明 | 示例 |
|----------|------|------|
| `-o yaml` | YAML 完整资源定义 | `kubectl get pod nginx -o yaml` |
| `-o json` | JSON 完整资源定义 | `kubectl get pod nginx -o json` |
| `-o wide` | 额外信息(节点、IP) | `kubectl get nodes -o wide` |
| `-o name` | 仅输出资源名称 | `kubectl get pods -o name` |
| `-o jsonpath=...` | JSONPath 提取 | `kubectl get pod -o jsonpath={.status.phase}` |
| `-o custom-columns=...` | 自定义列 | 见下方示例 |
| `--show-labels` | 显示所有标签 | `kubectl get pods --show-labels` |

```bash
# 自定义列输出示例
kubectl get pods -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,NODE:.spec.nodeName,RESTARTS:.status.containerStatuses[0].restartCount
```

---

<!-- chunk: 2. 资源创建与管理 (create, apply, delete, replace) -->
## 2. 资源创建与管理 (create, apply, delete, replace)

### 2.1 kubectl create

**语法格式:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl create -f <filename|url> [options]
kubectl create <resource> <name> [flags]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-f <file>` | 从 YAML/JSON 文件创建 | `kubectl create -f deployment.yaml` |
| `--dry-run=client` | 本地模拟，不提交到集群 | `kubectl create -f deploy.yaml --dry-run=client` |
| `--dry-run=server` | Server 端模拟验证 | `kubectl create -f deploy.yaml --dry-run=server` |
| `-o yaml` | 输出生成的 YAML | `kubectl create deploy nginx --image=nginx -o yaml --dry-run=client` |
| `--save-config` | 保存配置到 annotation | `kubectl create -f deploy.yaml --save-config` |
| `--record` (已弃用) | 记录命令到 revision history | 改用 dry-run=server 方式 |

**生产环境示例:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 1. 从文件创建资源 (最常用)
kubectl create -f namespace.yaml
kubectl create -f deployment.yaml -f service.yaml

# 2. 从目录批量创建
kubectl create -f ./k8s-manifests/

# 3. 从 URL 创建 (GitOps 场景)
kubectl create -f https://raw.githubusercontent.com/org/repo/main/deploy.yaml

# 4. 命令式创建常见资源 (快速测试)
kubectl create namespace production
kubectl create deployment nginx --image=nginx:1.25 --replicas=3
kubectl create service clusterip nginx --tcp=80:80
kubectl create secret generic db-creds --from-literal=password=MyP@ssw0rd
kubectl create configmap app-config --from-file=config.properties
kubectl create serviceaccount app-sa --namespace production

# 5. 生成 YAML 但不提交 (模板生成)
kubectl create deployment nginx --image=nginx:1.25 --replicas=3 -o yaml --dry-run=client > deploy-template.yaml
```

> **注意事项:**
> - `create` 是命令式操作，如果资源已存在会报错 (AlreadyExists)
> - 生产环境推荐优先使用 `apply` 进行声明式管理
> - `--save-config` 可在 annotation 中保存配置，便于后续 `apply` 接管

### 2.2 kubectl apply

**语法格式:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl apply -f <filename|directory|url> [options]
kubectl apply -k <kustomization_directory> [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-f <file>` | 应用 YAML/JSON 文件 | `kubectl apply -f deployment.yaml` |
| `-f <dir>` | 应用目录下所有资源 | `kubectl apply -f ./manifests/` |
| `-k <dir>` | 应用 Kustomize 目录 | `kubectl apply -k ./overlays/production/` |
| `--dry-run=server` | Server 端模拟验证 | `kubectl apply -f deploy.yaml --dry-run=server` |
| `--dry-run=server -o yaml` | 模拟并输出最终 YAML | `kubectl apply -f deploy.yaml --dry-run=server -o yaml` |
| `--server-side` | Server-Side Apply (SSA) | `kubectl apply -f deploy.yaml --server-side` |
| `--prune` | 清理未在配置中声明的资源 | `kubectl apply -f ./manifests/ --prune -l app=myapp` |
| `--force` | 强制应用 (处理 conflicts) | `kubectl apply -f deploy.yaml --force` |
| `--force-conflicts` | 强制覆盖 field manager 冲突 | `kubectl apply -f deploy.yaml --server-side --force-conflicts` |
| `--validate=true/false` | 是否进行 schema 验证 | `kubectl apply -f deploy.yaml --validate=false` |

**生产环境示例:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 1. 基础应用 (最常用)
kubectl apply -f deployment.yaml

# 2. 应用整个目录 (CI/CD 管道)
kubectl apply -f ./k8s/base/

# 3. Server-Side Apply (v1.18+ 推荐，解决 field ownership 问题)
kubectl apply -f deployment.yaml --server-side
kubectl apply -f deployment.yaml --server-side --force-conflicts

# 4. Server-Side Dry Run (提交前验证，推荐用于生产发布前检查)
kubectl apply -f deployment.yaml --dry-run=server -o yaml

# 5. 带标签修剪的声明式管理 (GitOps 场景)
kubectl apply -f ./manifests/ --prune -l app=myapp --namespace production

# 6. 强制应用 (处理 finalizers 或其他阻塞情况)
kubectl apply -f deployment.yaml --force

# 7. 从 stdin 应用 (管道操作)
cat deployment.yaml | kubectl apply -f -

# 8. 应用 Kustomize 配置
kubectl apply -k ./overlays/production/
```

> **注意事项:**
> - `--server-side` 是 v1.18+ 的推荐方式，特别适合多控制器/CI 系统管理同一资源
> - `--dry-run=server` 会实际发送到 API Server 验证，比 `client` 更准确但会产生负载
> - `--prune` 非常危险，务必配合精确的标签选择器使用，避免误删
> - 使用 `--prune` 时建议先 `--dry-run=server` 确认影响范围

### 2.3 kubectl delete

**语法格式:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
kubectl delete <resource> <name> [options]
kubectl delete -f <filename> [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-f <file>` | 删除文件定义的资源 | `kubectl delete -f deployment.yaml` |
| `-l <selector>` | 按标签选择器删除 | `kubectl delete pods -l app=nginx` |
| `--all` | 删除命名空间下所有资源 | `kubectl delete pods --all` |
| `--grace-period=<seconds>` | 优雅终止等待秒数 | `kubectl delete pod nginx --grace-period=30` |
| `--grace-period=0 --force` | 强制立即删除 | `kubectl delete pod stuck-pod --grace-period=0 --force` |
| `--wait=false` | 不等待资源删除完成 | `kubectl delete -f ./manifests/ --wait=false` |
| `--timeout=<duration>` | 等待超时时间 | `kubectl delete pod nginx --timeout=60s` |
| `--cascade=background` | 后台级联删除 | `kubectl delete deployment nginx --cascade=background` |
| `--cascade=foreground` | 前台级联删除 | `kubectl delete deployment nginx --cascade=foreground` |
| `--cascade=orphan` | 孤儿化删除 (不删除子资源) | `kubectl delete deployment nginx --cascade=orphan` |

**生产环境示例:**

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete --all`：批量删除某类全部资源，波及面巨大
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# 1. 从文件删除资源
kubectl delete -f deployment.yaml

# 2. 按标签批量删除 (清理测试环境)
kubectl delete deployments,pods,services -l env=testing --namespace staging

# 3. 强制删除卡住的 Pod (关键 - 排查 Terminating 状态)
kubectl delete pod nginx --grace-period=0 --force --namespace production  # ⚠️ 跳过优雅终止，可能丢数据

# 4. 级联策略控制 (保留 ReplicaSet 进行调试)
kubectl delete deployment nginx --cascade=orphan

# 5. 清理命名空间下所有资源 (保留命名空间本身)
kubectl delete all --all --namespace temporary-namespace  # ⚠️ 批量删除，波及面大

# 6. 带超时的删除操作 (脚本中使用)
kubectl delete namespace old-project --timeout=300s --wait=true  # ⚠️ 不可逆：永久删除命名空间及全部资源

# 7. 删除并忽略不存在错误 (幂等脚本)
kubectl delete -f manifest.yaml --ignore-not-found=true
```

> **注意事项:**
> - `--grace-period=0 --force` 不执行 pre-stop hooks 和优雅终止，可能导致数据丢失
> - `--cascade=orphan` 会留下子资源，需要手动清理，适合调试场景
> - 删除 Namespace 会级联删除其下所有资源，操作前务必确认
> - 生产环境删除操作建议先 `get` 确认目标范围

### 2.4 kubectl replace

**语法格式:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl replace -f <filename> [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-f <file>` | 替换资源 | `kubectl replace -f deployment.yaml` |
| `--force` | 强制替换 (先删除再创建) | `kubectl replace -f deploy.yaml --force` |
| `--cascade` | 级联删除旧资源 | `kubectl replace -f deploy.yaml --cascade` |
| `--save-config` | 保存配置到 annotation | `kubectl replace -f deploy.yaml --save-config` |
| `--dry-run=server` | Server 端模拟 | `kubectl replace -f deploy.yaml --dry-run=server` |

**生产环境示例:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 1. 替换现有资源 (要求资源必须已存在)
kubectl replace -f deployment.yaml

# 2. 强制替换 (不可变字段修改)
kubectl replace -f deployment.yaml --force

# 3. 从 stdin 替换
cat deployment.yaml | kubectl replace -f -
```

> **注意事项:**
> - `replace` 是完整的 PUT 操作，会替换整个资源对象
> - 如果资源不存在，`replace` 会报错 (NotFound)，而 `apply` 会创建
> - `--force` 会先删除再重建，会导致服务中断，生产环境慎用
> - 推荐使用 `apply --server-side` 代替 `replace`
---

<!-- chunk: 3. 资源查看与诊断 (get, describe, explain, top, logs, events) -->
## 3. 资源查看与诊断 (get, describe, explain, top, logs, events)

### 3.1 kubectl get

**语法格式:**
```bash
kubectl get <resource> [<name>] [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-o yaml/json/wide/name` | 输出格式 | `kubectl get pod nginx -o yaml` |
| `-o jsonpath=...` | JSONPath 查询 | `kubectl get pod nginx -o jsonpath={.status.podIP}` |
| `-o custom-columns=...` | 自定义输出列 | `kubectl get pods -o custom-columns=POD:.metadata.name,IP:.status.podIP` |
| `--show-labels` | 显示所有标签 | `kubectl get pods --show-labels` |
| `-l <selector>` / `--selector` | 标签选择器过滤 | `kubectl get pods -l app=nginx,tier=frontend` |
| `-L <label>` | 按标签分组显示为列 | `kubectl get pods -L app,version` |
| `--sort-by=...` | 按字段排序 | `kubectl get pods --sort-by=.status.startTime` |
| `--field-selector=...` | 字段选择器 | `kubectl get pods --field-selector=status.phase!=Succeeded` |
| `-w` / `--watch` | 持续监听变化 | `kubectl get pods -w` |
| `--watch-only` | 仅监听，不输出当前状态 | `kubectl get pods --watch-only` |
| `-A` / `--all-namespaces` | 所有命名空间 | `kubectl get pods -A` |
| `--show-kind` | 显示资源类型 | `kubectl get pods --show-kind` |
| `--chunk-size=500` | 分页获取 (大集群) | `kubectl get pods -A --chunk-size=500` |

**生产环境示例:**

```bash
# 1. 查看所有命名空间的 Pod (带节点和 IP)
kubectl get pods -A -o wide

# 2. 查看特定标签的 Pod 并显示标签
kubectl get pods -l app=nginx,tier=frontend --show-labels -n production

# 3. 按重启次数排序查看 Pod (定位不稳定服务)
kubectl get pods --sort-by=.status.containerStatuses[0].restartCount -n production

# 4. 查看非运行状态的 Pod (排查问题)
kubectl get pods --field-selector=status.phase!=Running -A

# 5. 实时监听 Pod 状态变化 (发布时观察滚动更新)
kubectl get pods -l app=api-gateway -w -n production

# 6. 自定义列输出 - 资源使用概览
kubectl get pods -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,NODE:.spec.nodeName,IP:.status.podIP,RESTARTS:.status.containerStatuses[0].restartCount

# 7. 提取特定字段 (脚本中使用)
kubectl get deployment api-gateway -n production -o jsonpath={.spec.replicas}

# 8. 查看所有 Services 的 ClusterIP 和端口映射
kubectl get svc -A -o custom-columns=NAMESPACE:.metadata.namespace,NAME:.metadata.name,TYPE:.spec.type,CLUSTER-IP:.spec.clusterIP,PORTS:.spec.ports[0].port

# 9. 大集群分页获取 (避免 API Server OOM)
kubectl get pods -A --chunk-size=500 --no-headers | wc -l

# 10. 查看带标签列的节点资源
kubectl get nodes -L node-role.kubernetes.io/control-plane,kubernetes.io/arch
```

> **注意事项:**
> - `-w` 会持续占用连接，在脚本中使用时注意超时处理
> - `--chunk-size` 对大型集群非常重要，可减少 API Server 和 etcd 压力
> - `--sort-by` 支持 JSONPath 表达式，但排序在客户端完成，大列表可能较慢
> - `-o jsonpath` 返回空字符串时不会报错，脚本中需额外检查

### 3.2 kubectl describe

**语法格式:**
```bash
kubectl describe <resource> <name> [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-n <namespace>` | 指定命名空间 | `kubectl describe pod nginx -n production` |
| `--show-events=true/false` | 是否显示 Events | `kubectl describe pod nginx --show-events=true` |

**生产环境示例:**

```bash
# 1. 查看 Pod 详细状态 (排查 Pending/CrashLoopBackOff)
kubectl describe pod nginx-7d8c9b4f5-x2k9m -n production

# 2. 查看 Node 详细信息和资源压力
kubectl describe node worker-node-01

# 3. 查看 Deployment 的事件和滚动更新状态
kubectl describe deployment api-gateway -n production

# 4. 查看 PVC 绑定和挂载详情
kubectl describe pvc data-volume -n production

# 5. 快速定位最后一条 Warning 事件
kubectl describe pod nginx -n production | grep -A 5 "Events:"
```

> **注意事项:**
> - `describe` 会自动汇总相关 Events，是排查问题的首选命令
> - Events 有保留时间限制 (默认 1 小时)，过期事件会被清理
> - 对于大规模集群，频繁 `describe` 会产生较大 API 负载

### 3.3 kubectl explain

**语法格式:**
```bash
kubectl explain <resource>[.<fieldPath>] [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--recursive` | 递归显示所有字段 | `kubectl explain deployment.spec --recursive` |
| `--api-version=<version>` | 指定 API 版本 | `kubectl explain deployment --api-version=apps/v1` |

**生产环境示例:**

```bash
# 1. 查看资源顶级字段
kubectl explain deployment

# 2. 递归查看完整 spec 结构 (编写 YAML 时参考)
kubectl explain deployment.spec --recursive

# 3. 查看特定字段的详细说明
kubectl explain deployment.spec.strategy.rollingUpdate

# 4. 查看 Pod spec 中的容器资源限制字段
kubectl explain pod.spec.containers.resources --recursive

# 5. 查看不同 API 版本的定义差异
kubectl explain deployment --api-version=apps/v1
```

> **注意事项:**
> - `--recursive` 输出量很大，建议配合 `grep` 过滤使用
> - API 版本不同字段可能不同，跨版本迁移时需特别核对
> - 是编写和审查 YAML 的必备工具

### 3.4 kubectl top

**语法格式:**
```bash
kubectl top node [node-name] [options]
kubectl top pod [pod-name] [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--containers` | 显示容器级别指标 | `kubectl top pod nginx --containers` |
| `-n <namespace>` | 指定命名空间 | `kubectl top pod -n production` |
| `-l <selector>` | 标签选择器 | `kubectl top pod -l app=nginx` |
| `--sort-by=cpu/memory` | 按指标排序 | `kubectl top pod --sort-by=cpu -A` |
| `--no-headers` | 不输出表头 | `kubectl top node --no-headers` |
| `--use-protocol-buffers` | 使用 protobuf 协议 (高效) | `kubectl top node --use-protocol-buffers` |

**生产环境示例:**

```bash
# 1. 查看节点资源使用 (排查节点压力)
kubectl top node

# 2. 查看 Pod CPU/Memory 使用并按 CPU 排序
kubectl top pod -n production --sort-by=cpu

# 3. 查看容器级别资源使用 (排查高消耗容器)
kubectl top pod api-gateway-7d8c9b4f5-x2k9m --containers -n production

# 4. 查看特定标签的 Pod 资源使用
kubectl top pod -l app=api-gateway -n production

# 5. 查看所有命名空间资源使用 (Top 10 CPU)
kubectl top pod -A --sort-by=cpu | head -n 11
```

> **注意事项:**
> - 需要 Metrics Server 安装并正常运行
> - 如果显示 `<unknown>`，检查 metrics-server Pod 和 API 聚合层
> - 指标有一定延迟 (通常 15-60 秒)

### 3.5 kubectl logs

**语法格式:**
```bash
kubectl logs <pod-name> [container-name] [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--previous` / `-p` | 查看上一个容器的日志 (排查 CrashLoopBackOff) | `kubectl logs nginx --previous` |
| `--follow` / `-f` | 实时跟踪日志 | `kubectl logs nginx -f` |
| `--tail=<N>` | 显示最后 N 行 | `kubectl logs nginx --tail=100` |
| `--since=<duration>` | 显示最近一段时间的日志 | `kubectl logs nginx --since=1h` |
| `--since-time=<timestamp>` | 显示指定时间后的日志 | `kubectl logs nginx --since-time=2026-04-20T10:00:00Z` |
| `--timestamps` | 显示时间戳 | `kubectl logs nginx --timestamps` |
| `--prefix` | 显示 Pod/容器名称前缀 | `kubectl logs nginx --prefix` |
| `--all-containers` | 显示所有容器的日志 | `kubectl logs nginx --all-containers` |
| `--selector=<label>` | 按标签选择多个 Pod | `kubectl logs -l app=nginx --tail=50` |
| `--max-log-requests=<N>` | 最大并发日志请求 | `kubectl logs -l app=nginx --max-log-requests=10` |
| `--ignore-errors` | 忽略获取日志的错误 | `kubectl logs -l app=nginx --ignore-errors` |

**生产环境示例:**

```bash
# 1. 查看 Pod 最近 100 行日志 (最常用)
kubectl logs nginx-7d8c9b4f5-x2k9m --tail=100 -n production

# 2. 实时跟踪日志 (发布时观察启动过程)
kubectl logs nginx-7d8c9b4f5-x2k9m -f -n production

# 3. 查看 CrashLoopBackOff 的上一次容器日志 (关键排查命令)
kubectl logs nginx-7d8c9b4f5-x2k9m --previous -n production

# 4. 查看最近 1 小时的错误日志
kubectl logs nginx-7d8c9b4f5-x2k9m --since=1h -n production | grep ERROR

# 5. 查看多 Pod 应用的聚合日志 (按标签)
kubectl logs -l app=api-gateway --tail=50 --prefix --all-containers -n production

# 6. 查看 init 容器日志
kubectl logs nginx-7d8c9b4f5-x2k9m -c init-myservice -n production

# 7. 带时间戳的完整日志导出
kubectl logs nginx-7d8c9b4f5-x2k9m --timestamps --since=24h > nginx-logs.txt

# 8. 查看 Job Pod 的日志
kubectl logs job/batch-data-processing -n production
```

> **注意事项:**
> - `--previous` 是排查 CrashLoopBackOff 的必备命令，查看崩溃前的最后日志
> - `-f` 会保持连接打开，在脚本中使用时需要配合 `timeout` 命令
> - 多容器 Pod 必须指定容器名或使用 `--all-containers`
> - 日志量大的 Pod 建议配合 `--since` 或 `--tail` 限制范围
> - 默认日志保留策略由容器运行时决定，Docker 默认保留 100MB 或 10 个文件

### 3.6 kubectl events

**语法格式:**
```bash
kubectl events [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--all-namespaces` / `-A` | 所有命名空间 | `kubectl events -A` |
| `-n <namespace>` | 指定命名空间 | `kubectl events -n production` |
| `-l <selector>` | 按标签过滤 | `kubectl events -l app=nginx` |
| `--for=<resource>` | 针对特定资源 | `kubectl events --for=Pod/nginx-7d8c9b4f5-x2k9m` |
| `--field-selector=<selector>` | 字段选择器 | `kubectl events --field-selector=reason=FailedScheduling` |
| `--types=<types>` | 按事件类型过滤 | `kubectl events --types=Warning` |
| `--watch` / `-w` | 持续监听 | `kubectl events -w` |
| `--chunk-size=<N>` | 分页大小 | `kubectl events -A --chunk-size=500` |

**生产环境示例:**

```bash
# 1. 查看命名空间最近事件
kubectl events -n production

# 2. 查看所有 Warning 事件 (排查问题)
kubectl events -A --types=Warning

# 3. 查看特定 Pod 的事件
kubectl events --for=Pod/nginx-7d8c9b4f5-x2k9m -n production

# 4. 查看调度失败事件
kubectl events -A --field-selector=reason=FailedScheduling

# 5. 实时监听事件 (发布时观察)
kubectl events -w -n production

# 6. 查看特定 Deployment 相关事件
kubectl events --for=Deployment/api-gateway -n production
```

> **注意事项:**
> - v1.23+ 引入 `kubectl events` 命令，替代早期的 `kubectl get events`
> - Events 默认保留 1 小时 (由 event-ttl 参数控制)
> - Warning 事件是排查问题的关键线索
> - 大规模集群中事件量很大，建议配合过滤条件使用
---

<!-- chunk: 4. 工作负载运维 (rollout, scale, autoscale, set) -->
## 4. 工作负载运维 (rollout, scale, autoscale, set)

### 4.1 kubectl rollout

**语法格式:**


| 子命令 | 说明 | 示例 |
|----------|------|------|
| history | 查看滚动更新历史 | kubectl rollout history deployment/nginx |
| status | 查看滚动更新状态 | kubectl rollout status deployment/nginx |
| pause | 暂停滚动更新 | kubectl rollout pause deployment/nginx |
| resume | 恢复滚动更新 | kubectl rollout resume deployment/nginx |
| undo | 回滚到上一版本 | kubectl rollout undo deployment/nginx |
| restart | 重启工作负载 | kubectl rollout restart deployment/nginx |

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| --revision=<N> | 指定版本号 | kubectl rollout history deployment/nginx --revision=3 |
| --to-revision=<N> | 回滚到指定版本 | kubectl rollout undo deployment/nginx --to-revision=2 |
| --watch / -w | 等待完成 | kubectl rollout status deployment/nginx -w |
| --timeout=<duration> | 超时时间 | kubectl rollout status deployment/nginx --timeout=300s |

**生产环境示例:**


> **注意事项:**
> - restart 不会修改 Pod Template spec，仅添加 restartedAt annotation
> - pause 后已更新的 Pod 保持运行
> - StatefulSet 的 undo 需要谨慎
> - 生产环境发布建议配合 rollout status -w 监控

### 4.2 kubectl scale

**语法格式:**


| 常用选项 | 说明 | 示例 |
|----------|------|------|
| --replicas=<N> | 目标副本数 | kubectl scale deployment nginx --replicas=5 |
| --current-replicas=<N> | 当前副本数条件 | kubectl scale deployment nginx --current-replicas=3 --replicas=5 |
| --timeout=<duration> | 等待超时 | kubectl scale deployment nginx --replicas=10 --timeout=120s |
| --all | 缩放所有资源 | kubectl scale deployment --all --replicas=3 |

**生产环境示例:**


> **注意事项:**
> - 被 HPA 管理的 Deployment 不应手动 scale
> - 大规模扩容时注意节点资源

### 4.3 kubectl autoscale

**语法格式:**


| 常用选项 | 说明 | 示例 |
|----------|------|------|
| --min=<N> | 最小副本数 | kubectl autoscale deployment nginx --min=2 --max=10 |
| --max=<N> | 最大副本数 | kubectl autoscale deployment nginx --max=10 |
| --cpu-percent=<percent> | CPU 使用率阈值 | kubectl autoscale deployment nginx --cpu-percent=70 --max=10 |
| --memory-percent=<percent> | Memory 使用率阈值 | kubectl autoscale deployment nginx --memory-percent=80 --max=10 |

**生产环境示例:**


> **注意事项:**
> - 需要 Metrics Server 支持
> - 容器必须设置 resources.requests.cpu
> - 生产环境建议通过 YAML 定义 HPA

### 4.4 kubectl set

**语法格式:**


#### 4.4.1 kubectl set image

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| <container>=<image> | 设置容器镜像 | kubectl set image deployment/nginx nginx=nginx:1.26 |
| --all | 更新所有资源 | kubectl set image deployment --all *=nginx:1.26 |
| --local | 本地修改不提交 | kubectl set image deployment/nginx nginx=nginx:1.26 --local -o yaml |

**生产环境示例:**


#### 4.4.2 kubectl set resources

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| --limits | 设置资源限制 | kubectl set resources deployment/nginx --limits=cpu=500m,memory=512Mi |
| --requests | 设置资源请求 | kubectl set resources deployment/nginx --requests=cpu=200m,memory=256Mi |
| --containers | 指定容器 | kubectl set resources deployment/nginx --limits=cpu=1g --containers=app |

**生产环境示例:**


#### 4.4.3 kubectl set env

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| <key>=<value> | 设置环境变量 | kubectl set env deployment/nginx ENV=prod |
| --from=configmap/<name> | 从 ConfigMap 导入 | kubectl set env deployment/nginx --from=configmap/app-config |
| --from=secret/<name> | 从 Secret 导入 | kubectl set env deployment/nginx --from=secret/db-creds |
| --list | 列出环境变量 | kubectl set env deployment/nginx --list |

**生产环境示例:**


#### 4.4.4 kubectl set serviceaccount

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| <serviceaccount> | 指定 ServiceAccount | kubectl set serviceaccount deployment/nginx sa-name |
| --local | 本地输出 | kubectl set serviceaccount deployment/nginx sa-name --local -o yaml |

**生产环境示例:**


> **注意事项:**
> - set image 是最常用的生产命令，但要确保镜像 tag 正确
> - set resources 修改后不会自动重启 Pod，需要手动 rollout restart
> - set env 修改后 Deployment 会自动触发滚动更新
> - set serviceaccount 修改后需要 rollout restart 才生效
---

<!-- chunk: 5. 集群与配置管理 (config, cluster-info, version, api-resources) -->
## 5. 集群与配置管理 (config, cluster-info, version, api-resources)

### 5.1 kubectl config

**语法格式:**


| 子命令 | 说明 | 示例 |
|----------|------|------|
| view | 查看 kubeconfig 内容 | kubectl config view |
| current-context | 显示当前上下文 | kubectl config current-context |
| use-context | 切换上下文 | kubectl config use-context prod-cluster |
| set-context | 创建/修改上下文 | kubectl config set-context prod --cluster=prod |
| set-cluster | 配置集群信息 | kubectl config set-cluster prod --server=https://... |
| set-credentials | 配置用户凭证 | kubectl config set-credentials admin --token=... |
| unset | 删除配置项 | kubectl config unset contexts.prod |
| rename-context | 重命名上下文 | kubectl config rename-context old new |
| delete-context | 删除上下文 | kubectl config delete-context old |
| get-contexts | 列出所有上下文 | kubectl config get-contexts |
| get-clusters | 列出所有集群 | kubectl config get-clusters |

**生产环境示例:**

apiVersion: v1
clusters:
- cluster:
    certificate-authority-data: DATA+OMITTED
    server: https://127.0.0.1:57284
  name: kind-my-k8s
contexts:
- context:
    cluster: kind-my-k8s
    user: kind-my-k8s
  name: kind-my-k8s
current-context: kind-my-k8s
kind: Config
users:
- name: kind-my-k8s
  user:
    client-certificate-data: DATA+OMITTED
    client-key-data: DATA+OMITTED
kind-my-k8s
Context "prod" created.
User "prod-admin" set.
CURRENT   NAME          CLUSTER        AUTHINFO      NAMESPACE
*         kind-my-k8s   kind-my-k8s    kind-my-k8s   
          prod          prod-cluster   admin         production
apiVersion: v1
clusters:
- cluster:
    certificate-authority-data: DATA+OMITTED
    server: https://127.0.0.1:57284
  name: kind-my-k8s
contexts:
- context:
    cluster: kind-my-k8s
    user: kind-my-k8s
  name: kind-my-k8s
current-context: kind-my-k8s
kind: Config
users:
- name: kind-my-k8s
  user:
    client-certificate-data: DATA+OMITTED
    client-key-data: DATA+OMITTED

> **注意事项:**
> - 生产环境建议为每个集群使用独立的 kubeconfig 文件
> -  切换是全局的，会影响所有终端窗口
> -  只输出当前上下文相关的配置，便于分享
> - 敏感信息 (token/certs) 建议使用  引用外部文件

### 5.2 kubectl cluster-info

**语法格式:**

To further debug and diagnose cluster problems, use 'kubectl cluster-info dump'.

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| --dump | 导出集群诊断信息 | kubectl cluster-info dump |
| --namespaces | 指定导出的命名空间 | kubectl cluster-info dump --namespaces=kube-system,production |
| --output-directory | 输出目录 | kubectl cluster-info dump --output-directory=./cluster-dump |

**生产环境示例:**


To further debug and diagnose cluster problems, use 'kubectl cluster-info dump'.

> **注意事项:**
> -  会收集大量数据，注意磁盘空间
> - 导出的日志可能包含敏感信息，需妥善保管

### 5.3 kubectl version

**语法格式:**


| 常用选项 | 说明 | 示例 |
|----------|------|------|
| --short (已弃用) | 简短输出 | 使用 -o json 替代 |
| --client | 仅显示客户端版本 | kubectl version --client |
| -o yaml/json | 结构化输出 | kubectl version -o json |

**生产环境示例:**

{
  "clientVersion": {
    "major": "1",
    "minor": "35",
    "gitVersion": "v1.35.3",
    "gitCommit": "6c1cd99aef09161ddb07b8ade6c9564e9b9[[entities/bfe.md|bfe]]27",
    "gitTreeState": "clean",
    "buildDate": "2026-03-18T18:30:07Z",
    "goVersion": "go1.26.1",
    "compiler": "gc",
    "platform": "darwin/arm64"
  },
  "kustomizeVersion": "v5.7.1"
}
{
  "clientVersion": {
    "major": "1",
    "minor": "35",
    "gitVersion": "v1.35.3",
    "gitCommit": "6c1cd99aef09161ddb07b8ade6c9564e9b9bfe27",
    "gitTreeState": "clean",
    "buildDate": "2026-03-18T18:30:07Z",
    "goVersion": "go1.26.1",
    "compiler": "gc",
    "platform": "darwin/arm64"
  },
  "kustomizeVersion": "v5.7.1"
}
Client Version: v1.35.3

### 5.4 kubectl api-resources

**语法格式:**


| 常用选项 | 说明 | 示例 |
|----------|------|------|
| --verbs=<list> | 按支持的操作过滤 | kubectl api-resources --verbs=list,get |
| --namespaced=true/false | 按命名空间属性过滤 | kubectl api-resources --namespaced=true |
| --api-group=<group> | 按 API 组过滤 | kubectl api-resources --api-group=apps |
| -o wide | 显示额外信息 | kubectl api-resources -o wide |
| --cached | 使用缓存 (更快) | kubectl api-resources --cached |
| --sort-by=<field> | 排序 | kubectl api-resources --sort-by=name |

**生产环境示例:**


> **注意事项:**
> -  是编写 RBAC 规则时的常用组合
> -  可快速定位集群级资源 (如 Node, ClusterRole)
> - 插件或 CRD 安装后需要重新查询才能看到新资源

### 5.5 kubectl api-versions

**生产环境示例:**


---

<!-- chunk: 6. 容器交互与调试 (exec, logs, port-forward, cp, attach, debug) -->
## 6. 容器交互与调试 (exec, logs, port-forward, cp, attach, debug)

### 6.1 kubectl exec

**语法格式:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec <pod-name> [-c <container>] -- <command> [args...]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-c <container>` | 指定容器 | `kubectl exec nginx -c app -- ls /app` |
| `-i` / `--stdin` | 传递 stdin | `kubectl exec -i nginx -- sh` |
| `-t` / `--tty` | 分配 TTY | `kubectl exec -it nginx -- bash` |
| `--env=<key=value>` | 设置环境变量 | `kubectl exec nginx --env=DEBUG=1 -- env` |

**生产环境示例:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 1. 进入 Pod 的交互式 shell (最常用)
kubectl exec -it nginx-7d8c9b4f5-x2k9m -- bash
kubectl exec -it nginx-7d8c9b4f5-x2k9m -- sh

# 2. 在指定容器中执行命令 (多容器 Pod)
kubectl exec nginx-7d8c9b4f5-x2k9m -c sidecar -- curl localhost:8080/health

# 3. 查看容器内环境变量
kubectl exec nginx-7d8c9b4f5-x2k9m -- env | sort

# 4. 在容器内运行诊断命令
kubectl exec nginx-7d8c9b4f5-x2k9m -- netstat -tlnp
kubectl exec nginx-7d8c9b4f5-x2k9m -- ps aux
kubectl exec nginx-7d8c9b4f5-x2k9m -- df -h

# 5. 管道输入到容器命令
cat script.sql | kubectl exec -i mysql-0 -- mysql -u root -p$PASSWORD

# 6. 临时设置环境变量执行命令
kubectl exec nginx-7d8c9b4f5-x2k9m --env=DEBUG=true -- printenv DEBUG
```

> **注意事项:**
> - `-it` 组合是进入交互式 shell 的标准方式
> - 多容器 Pod 必须指定 `-c`，否则会报错
> - `kubectl exec` 依赖 API Server 到节点的网络连通性
> - 生产环境进入容器调试应避免修改容器内文件系统

### 6.2 kubectl logs

已在 [3.5 kubectl logs](#35-kubectl-logs) 中详细描述。

### 6.3 kubectl port-forward

**语法格式:**
```bash
kubectl port-forward <pod-name> [LOCAL_PORT:]REMOTE_PORT [...[LOCAL_PORT:]REMOTE_PORT]
kubectl port-forward <service/name> [LOCAL_PORT:]REMOTE_PORT
kubectl port-forward <deployment/name> [LOCAL_PORT:]REMOTE_PORT
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--address=<ip>` | 绑定地址 | `kubectl port-forward pod/nginx 8080:80 --address=0.0.0.0` |

**生产环境示例:**

```bash
# 1. 转发 Pod 端口到本地
kubectl port-forward pod/nginx-7d8c9b4f5-x2k9m 8080:80 -n production

# 2. 转发 Service 端口 (负载均衡到后端 Pod)
kubectl port-forward svc/api-gateway 8080:80 -n production

# 3. 转发 Deployment (自动选择 Pod)
kubectl port-forward deployment/api-gateway 8080:80 -n production

# 4. 后台运行并绑定所有接口 (团队成员共享)
kubectl port-forward svc/api-gateway 8080:80 --address=0.0.0.0 -n production &

# 5. 多端口转发
kubectl port-forward pod/db-0 5432:5432 6379:6379 -n production

# 6. 随机本地端口
kubectl port-forward svc/api-gateway :80 -n production
```

> **注意事项:**
> - `port-forward` 连接断开时转发停止，建议使用 `nohup` 或 `screen`
> - 通过 Service 转发时，每次连接可能路由到不同 Pod
> - 大量并发连接可能影响 API Server 性能
> - 生产环境长期端口转发建议使用 Ingress 或 VPN

### 6.4 kubectl cp

**语法格式:**
```bash
kubectl cp <file-spec-src> <file-spec-dest> [options]
# <file-spec> = [namespace/]pod-name:/path
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-c <container>` | 指定容器 | `kubectl cp ./file nginx:/tmp/file -c app` |
| `--retries=<N>` | 重试次数 | `kubectl cp ./large.zip nginx:/tmp/ --retries=10` |

**生产环境示例:**

```bash
# 1. 从本地复制文件到 Pod
kubectl cp ./app.conf nginx-7d8c9b4f5-x2k9m:/etc/nginx/conf.d/ -n production

# 2. 从 Pod 复制文件到本地
kubectl cp nginx-7d8c9b4f5-x2k9m:/var/log/nginx/access.log ./access.log -n production

# 3. 复制到多容器 Pod 的指定容器
kubectl cp ./script.py nginx-7d8c9b4f5-x2k9m:/tmp/script.py -c sidecar -n production

# 4. 跨命名空间复制
kubectl cp production/nginx-7d8c9b4f5-x2k9m:/data/backup.sql ./backup.sql

# 5. 复制目录
kubectl cp ./config nginx-7d8c9b4f5-x2k9m:/app/config -n production

# 6. 大文件复制增加重试
kubectl cp ./dump.sql mysql-0:/tmp/dump.sql --retries=10 -n production
```

> **注意事项:**
> - `kubectl cp` 依赖 `tar` 命令存在于容器镜像中
> - 最小化镜像 (如 distroless) 可能不支持 `cp`
> - 大文件传输建议使用 `kubectl exec` 配合 `curl` 或对象存储
> - 跨命名空间格式: `<namespace>/<pod>:<path>`

### 6.5 kubectl attach

**语法格式:**
```bash
kubectl attach <pod-name> [-c <container>] [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-c <container>` | 指定容器 | `kubectl attach nginx -c main` |
| `-i` / `--stdin` | 传递 stdin | `kubectl attach -i nginx` |
| `-t` / `--tty` | 分配 TTY | `kubectl attach -it nginx` |

**生产环境示例:**

```bash
# 1. 附加到运行中的容器 (查看前台进程输出)
kubectl attach nginx-7d8c9b4f5-x2k9m -n production

# 2. 附加到指定容器并传递输入
kubectl attach -it nginx-7d8c9b4f5-x2k9m -c debugger -n production

# 3. 附加到 Job Pod 查看实时输出
kubectl attach job/batch-processor -n production
```

> **注意事项:**
> - `attach` 附加到容器的 stdin/stdout/stderr，与 `exec` 不同
> - 如果容器没有前台进程保持运行，`attach` 会立即退出
> - 多容器 Pod 必须指定 `-c`
> - 与 `docker attach` 行为类似，Ctrl+C 可能终止容器进程

### 6.6 kubectl debug

**语法格式:**
```bash
kubectl debug <pod-name> [options]
kubectl debug <node-name> -it --image=<image> [options]  # v1.21+ Ephemeral Containers
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-it` | 交互式 | `kubectl debug nginx -it --image=busybox` |
| `--image=<image>` | 调试容器镜像 | `kubectl debug nginx --image=nicolaka/netshoot` |
| `--target=<container>` | 目标容器名 | `kubectl debug nginx --target=app` |
| `--copy-to=<new-pod>` | 复制 Pod 并调试 | `kubectl debug nginx --copy-to=nginx-debug` |
| `--share-processes` | 共享进程命名空间 | `kubectl debug nginx --share-processes` |
| `--env=<key=value>` | 环境变量 | `kubectl debug nginx --env=DEBUG=1` |

**生产环境示例:**

```bash
# 1. 在运行中的 Pod 启动临时调试容器 (Ephemeral Container)
kubectl debug nginx-7d8c9b4f5-x2k9m -it --image=nicolaka/netshoot --target=app -n production

# 2. 复制 Pod 进行调试 (不影响原 Pod)
kubectl debug nginx-7d8c9b4f5-x2k9m --copy-to=nginx-debug --share-processes -n production

# 3. 调试 distroless/最小化镜像 Pod
kubectl debug nginx-7d8c9b4f5-x2k9m -it --image=busybox:1.36 --target=app -n production

# 4. 调试 Node 问题 (在节点上启动 Pod)
kubectl debug node/worker-01 -it --image=mcr.microsoft.com/oss/nginx/nginx:1.21.6

# 5. 带环境变量的调试容器
kubectl debug nginx-7d8c9b4f5-x2k9m -it --image=alpine --env=HTTP_PROXY=http://proxy:3128 -n production
```

> **注意事项:**
> - Ephemeral Containers 需要 v1.23+ 且 `EphemeralContainers` feature gate 启用
> - 临时容器不会重启，退出后即消失
> - `--share-processes` 允许查看目标容器的进程和文件系统
> - Node 调试会创建特权 Pod，需要相应权限

---

<!-- chunk: 7. 网络与服务暴露 (expose, port-forward, proxy) -->
## 7. 网络与服务暴露 (expose, port-forward, proxy)

### 7.1 kubectl expose

**语法格式:**
```bash
kubectl expose <resource> <name> --port=<port> [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--port=<port>` | 服务端口 | `kubectl expose deployment nginx --port=80` |
| `--target-port=<port>` | 容器目标端口 | `kubectl expose deployment nginx --port=80 --target-port=8080` |
| `--type=<type>` | 服务类型 | `ClusterIP`, `NodePort`, `LoadBalancer`, `ExternalName` |
| `--name=<name>` | 服务名称 | `kubectl expose deployment nginx --name=nginx-svc` |
| `--selector=<labels>` | 选择器 | `kubectl expose deployment nginx --selector=app=nginx` |
| `--external-ip=<ip>` | 外部 IP | `kubectl expose deployment nginx --external-ip=10.0.0.1` |
| `--dry-run=client` | 本地模拟 | `kubectl expose deploy nginx --port=80 --dry-run=client -o yaml` |

**生产环境示例:**

```bash
# 1. 为 Deployment 创建 ClusterIP Service
kubectl expose deployment api-gateway --port=80 --target-port=8080 --name=api-gateway-svc -n production

# 2. 创建 NodePort Service (临时外部访问)
kubectl expose deployment api-gateway --port=80 --type=NodePort --name=api-gateway-np -n production

# 3. 为 Pod 创建 Service
kubectl expose pod nginx-7d8c9b4f5-x2k9m --port=80 --name=nginx-svc -n production

# 4. 生成 YAML 不提交
kubectl expose deployment api-gateway --port=80 --target-port=8080 --dry-run=client -o yaml -n production

# 5. 创建 Headless Service
kubectl expose deployment api-gateway --port=80 --cluster-ip=None --name=api-gateway-headless -n production
```

> **注意事项:**
> - `expose` 是命令式操作，生产环境建议通过 YAML 管理 Service
> - NodePort 会占用节点端口 (默认 30000-32767)
> - 修改现有 Service 应使用 `apply` 或 `patch`

### 7.2 kubectl port-forward

已在 [6.3 kubectl port-forward](#63-kubectl-port-forward) 中详细描述。

### 7.3 kubectl proxy

**语法格式:**
```bash
kubectl proxy [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--port=<port>` | 监听端口 | `kubectl proxy --port=8080` |
| `--address=<ip>` | 绑定地址 | `kubectl proxy --port=8080 --address=0.0.0.0` |
| `--accept-hosts=<regex>` | 允许的主机头 | `kubectl proxy --accept-hosts='^.*'` |
| `--accept-paths=<regex>` | 允许的路径 | `kubectl proxy --accept-paths='^.*'` |
| `--reject-paths=<regex>` | 拒绝的路径 | `kubectl proxy --reject-paths='^/api/./pods/./attach'` |
| `--api-prefix=<path>` | API 前缀 | `kubectl proxy --api-prefix=/k8s/` |

**生产环境示例:**

```bash
# 1. 启动本地 API Server 代理 (最常用)
kubectl proxy --port=8080

# 2. 后台运行并允许外部访问 (团队协作场景)
kubectl proxy --port=8080 --address=0.0.0.0 --accept-hosts='^.*' &

# 3. 通过代理访问 Kubernetes API
curl http://localhost:8080/api/v1/namespaces/production/pods

# 4. 代理到特定路径前缀
kubectl proxy --port=8080 --api-prefix=/k8s/
# 访问: curl http://localhost:8080/k8s/api/v1/nodes

# 5. 限制访问路径 (安全代理)
kubectl proxy --port=8080 --reject-paths='^/api/./pods/./exec,^/api/./pods/./attach'
```

> **注意事项:**
> - `kubectl proxy` 使用当前用户的凭证，权限与当前 kubeconfig 一致
> - 暴露到 `0.0.0.0` 时务必配合 `--accept-hosts` 限制
> - 代理不处理身份验证，适合内部开发/调试工具
> - 长期运行建议使用专门的 API 网关或 Ingress
---

<!-- chunk: 8. 节点管理 (cordon, uncordon, drain, taint) -->
## 8. 节点管理 (cordon, uncordon, drain, taint)

### 8.1 kubectl cordon / uncordon

**语法格式:**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

```bash
kubectl cordon <node-name>
kubectl uncordon <node-name>
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--dry-run=client` | 本地模拟 | `kubectl cordon worker-01 --dry-run=client` |
| `--selector=<labels>` | 按标签选择 | `kubectl cordon -l node-role.kubernetes.io/worker` |

**生产环境示例:**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl cordon`：标记节点不可调度

```bash
# 1. 标记节点不可调度 (维护前)
kubectl cordon worker-node-01

# 2. 批量标记节点组不可调度
kubectl cordon -l node-role.kubernetes.io/worker=worker-pool-a

# 3. 维护完成后恢复调度
kubectl uncordon worker-node-01

# 4. 查看节点调度状态
kubectl get nodes -o custom-columns=NAME:.metadata.name,SCHEDULABLE:.spec.unschedulable,STATUS:.status.conditions[-1].type
```

> **注意事项:**
> - `cordon` 仅阻止新 Pod 调度，不影响已运行的 Pod
> - `uncordon` 不会自动重新调度 Pod，需删除 Pod 触发重新调度
> - 集群升级前应先 cordon 再 drain

### 8.2 kubectl drain

**语法格式:**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
kubectl drain <node-name> [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--force` | 强制驱逐 (忽略 DaemonSet Pod) | `kubectl drain worker-01 --force` |
| `--ignore-daemonsets` | 忽略 DaemonSet Pod | `kubectl drain worker-01 --ignore-daemonsets` |
| `--delete-emptydir-data` | 删除 EmptyDir 数据 | `kubectl drain worker-01 --delete-emptydir-data` |
| `--grace-period=<seconds>` | 优雅终止时间 | `kubectl drain worker-01 --grace-period=120` |
| `--pod-selector=<selector>` | 仅驱逐匹配的 Pod | `kubectl drain worker-01 --pod-selector=app!=critical` |
| `--timeout=<duration>` | 超时时间 | `kubectl drain worker-01 --timeout=300s` |
| `--dry-run=client` | 本地模拟 | `kubectl drain worker-01 --dry-run=client` |
| `--skip-wait-for-delete-timeout=<seconds>` | 跳过等待 | `kubectl drain worker-01 --skip-wait-for-delete-timeout=60` |

**生产环境示例:**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响

```bash
# 1. 标准节点排空 (维护前)
kubectl drain worker-node-01 --ignore-daemonsets --delete-emptydir-data

# 2. 强制排空 (包含 DaemonSet 管理的 Pod)
kubectl drain worker-node-01 --ignore-daemonsets --force --delete-emptydir-data

# 3. 排除关键业务 Pod 的排空
kubectl drain worker-node-01 --ignore-daemonsets --pod-selector=app!=payment-core

# 4. 带超时的排空 (大型节点)
kubectl drain worker-node-01 --ignore-daemonsets --timeout=600s --delete-emptydir-data

# 5. 模拟排空 (评估影响)
kubectl drain worker-node-01 --ignore-daemonsets --dry-run=client

# 6. 批量排空节点组
for node in $(kubectl get nodes -l node-role.kubernetes.io/worker=pool-a -o name); do
  kubectl drain "$node" --ignore-daemonsets --delete-emptydir-data --timeout=300s
done
```

> **注意事项:**
> - `drain` 会驱逐 Pod，可能导致服务中断，确保 PDB (PodDisruptionBudget) 已配置
> - 使用 `--force` 会删除 DaemonSet Pod，它们会在其他节点重建
> - `--delete-emptydir-data` 会丢失 EmptyDir 卷中的临时数据
> - 大规模排空时建议分批进行，观察业务影响
> - 如果 drain 卡住，检查 Pod 的 terminationGracePeriodSeconds 和 PDB

### 8.3 kubectl taint

**语法格式:**
```bash
kubectl taint <node-name> <key>=<value>:<effect> [options]
kubectl taint <node-name> <key>:<effect>-  # 删除 taint
```

| Effect 类型 | 说明 |
|-------------|------|
| `NoSchedule` | 新 Pod 不能调度到该节点 (除非容忍) |
| `PreferNoSchedule` | 尽量不在该节点调度 |
| `NoExecute` | 不能调度，已运行的 Pod 也会被驱逐 (除非容忍) |

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--overwrite` | 覆盖现有 taint | `kubectl taint nodes worker-01 key=value:NoSchedule --overwrite` |

**生产环境示例:**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl taint nodes`：变更污点影响 Pod 调度

```bash
# 1. 为节点添加 NoSchedule Taint (专用节点)
kubectl taint nodes worker-gpu-01 gpu=true:NoSchedule

# 2. 添加 NoExecute Taint (立即驱逐非容忍 Pod)
kubectl taint nodes worker-01 maintenance=true:NoExecute

# 3. 删除特定 Taint
kubectl taint nodes worker-01 gpu:NoSchedule-

# 4. 删除所有 key 为 gpu 的 Taint
kubectl taint nodes worker-01 gpu-

# 5. 为控制平面节点添加 Taint (防止工作负载调度)
kubectl taint nodes master-01 node-role.kubernetes.io/control-plane=:NoSchedule

# 6. 查看节点所有 Taints
kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints
```

> **注意事项:**
> - Taint 和 Toleration 配合使用，单独的 Taint 会阻止所有 Pod 调度
> - `NoExecute` 会立即驱逐不匹配的 Pod，生产环境慎用
> - 控制平面节点默认带有 `control-plane` taint (v1.24+)
> - 修改 Taint 后不会重新调度已运行的 Pod

---

<!-- chunk: 9. 权限与安全 (auth, certificate, create token) -->
## 9. 权限与安全 (auth, certificate, create token)

### 9.1 kubectl auth

**语法格式:**
```bash
kubectl auth <subcommand> [options]
```

| 子命令 | 说明 | 示例 |
|----------|------|------|
| can-i | 检查权限 | `kubectl auth can-i create pods` |
| reconcile | 协调 RBAC 规则 | `kubectl auth reconcile -f rbac.yaml` |

#### 9.1.1 kubectl auth can-i

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--list` | 列出所有权限 | `kubectl auth can-i --list` |
| `--namespace=<ns>` | 指定命名空间 | `kubectl auth can-i create pods -n production` |
| `--as=<user>` | 模拟用户 | `kubectl auth can-i create pods --as=developer` |
| `--as-group=<group>` | 模拟组 | `kubectl auth can-i create pods --as=developer --as-group=dev-team` |
| `--subresource=<name>` | 检查子资源 | `kubectl auth can-i get pods/log` |

**生产环境示例:**

```bash
# 1. 检查当前用户是否有创建 Pod 权限
kubectl auth can-i create pods -n production

# 2. 检查特定用户的权限
kubectl auth can-i delete nodes --as=cluster-admin

# 3. 列出当前用户在命名空间的所有权限
kubectl auth can-i --list -n production

# 4. 检查子资源权限 (日志访问)
kubectl auth can-i get pods/log -n production

# 5. 检查跨命名空间权限
kubectl auth can-i list secrets --all-namespaces

# 6. 检查非资源 URL 权限
kubectl auth can-i get /healthz
```

#### 9.1.2 kubectl auth reconcile

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--dry-run=client` | 本地模拟 | `kubectl auth reconcile -f rbac.yaml --dry-run=client` |
| `--remove-extra-subjects` | 删除额外主体 | `kubectl auth reconcile -f rbac.yaml --remove-extra-subjects` |
| `--remove-extra-permissions` | 删除额外权限 | `kubectl auth reconcile -f rbac.yaml --remove-extra-permissions` |

**生产环境示例:**

```bash
# 1. 协调 RBAC 配置 (确保 YAML 与集群状态一致)
kubectl auth reconcile -f rbac-config.yaml

# 2. 协调并删除额外权限 (严格模式)
kubectl auth reconcile -f rbac-config.yaml --remove-extra-permissions --remove-extra-subjects

# 3. 模拟协调 (预览变更)
kubectl auth reconcile -f rbac-config.yaml --dry-run=server -o yaml
```

> **注意事项:**
> - `auth reconcile` 确保 RBAC 资源与声明式配置严格一致
> - `--remove-extra-permissions` 会删除配置中未声明的规则，需谨慎使用
> - 适合 GitOps 流程中同步 RBAC 配置

### 9.2 kubectl certificate

**语法格式:**
```bash
kubectl certificate <subcommand> <certificate-signing-request-name>
```

| 子命令 | 说明 | 示例 |
|----------|------|------|
| approve | 批准 CSR | `kubectl certificate approve csr-abc123` |
| deny | 拒绝 CSR | `kubectl certificate deny csr-abc123` |

**生产环境示例:**

```bash
# 1. 列出待处理的 CSR
kubectl get csr | grep Pending

# 2. 批准节点加入集群的 CSR
kubectl certificate approve csr-worker-01-abc123

# 3. 批量批准所有 Pending CSR (自动化场景)
kubectl get csr -o json | jq -r '.items[] | select(.status == {}) | .metadata.name' | xargs kubectl certificate approve

# 4. 查看 CSR 详情后批准
kubectl describe csr csr-worker-01-abc123
kubectl certificate approve csr-worker-01-abc123

# 5. 拒绝可疑的 CSR
kubectl certificate deny csr-suspicious-xyz789
```

> **注意事项:**
> - 批准 CSR 前务必验证请求者身份
> - 自动批准 CSR 存在安全风险，建议使用自动化审批控制器
> - 节点 CSR 通常由 kubelet 自动生成

### 9.3 kubectl create token

**语法格式:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl create token <serviceaccount-name> [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--namespace=<ns>` | 指定命名空间 | `kubectl create token app-sa -n production` |
| `--duration=<duration>` | Token 有效期 | `kubectl create token app-sa --duration=1h` |
| `--audience=<aud>` | 指定受众 | `kubectl create token app-sa --audience=https://vault` |
| `--bound-object-kind=<kind>` | 绑定对象类型 | `kubectl create token app-sa --bound-object-kind=Pod` |
| `--bound-object-name=<name>` | 绑定对象名称 | `kubectl create token app-sa --bound-object-name=pod-01` |

**生产环境示例:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 1. 为 ServiceAccount 创建临时 Token
kubectl create token app-sa -n production

# 2. 创建短期 Token (脚本使用)
kubectl create token ci-runner -n ci --duration=30m

# 3. 创建绑定到 Pod 的 Token (增强安全)
kubectl create token app-sa -n production --bound-object-kind=Pod --bound-object-name=app-01

# 4. 指定受众的 Token (外部系统集成)
kubectl create token vault-auth -n security --audience=https://vault.company.com

# 5. 生成 Token 并立即使用
TOKEN=$(kubectl create token app-sa -n production --duration=1h)
curl -H "Authorization: Bearer $TOKEN" https://kubernetes.default.svc/api/v1/namespaces/production/pods
```

> **注意事项:**
> - v1.24+ ServiceAccount 不再自动创建长期 Secret Token
> - `create token` 生成的是短期投影 Token (Projected Volume Token)
> - 默认有效期为 1 小时，最长由 API Server `--service-account-max-token-expiration` 控制
> - 长期 Token 应使用 `kubectl create secret` 创建传统 ServiceAccount Token
---

<!-- chunk: 10. 高级特性 (diff, kustomize, patch, wait, annotate, label) -->
## 10. 高级特性 (diff, kustomize, patch, wait, annotate, label)

### 10.1 kubectl diff

**语法格式:**
```bash
kubectl diff -f <filename> [options]
kubectl diff -k <kustomization_directory>
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-f <file>` | 对比文件与集群 | `kubectl diff -f deployment.yaml` |
| `-k <dir>` | 对比 Kustomize 与集群 | `kubectl diff -k ./overlays/production/` |
| `--prune` | 包含将被删除的资源 | `kubectl diff -f ./manifests/ --prune -l app=myapp` |
| `--server-side` | 使用 Server-Side Apply | `kubectl diff -f deployment.yaml --server-side` |
| `--force-conflicts` | 强制覆盖冲突 | `kubectl diff -f deployment.yaml --server-side --force-conflicts` |
| `--field-manager=<name>` | 指定 field manager | `kubectl diff -f deployment.yaml --field-manager=ci-system` |

**生产环境示例:**

```bash
# 1. 对比 YAML 文件与集群当前状态
kubectl diff -f deployment.yaml

# 2. 对比 Kustomize 配置与集群
kubectl diff -k ./overlays/production/

# 3. 对比并包含将被修剪的资源
kubectl diff -f ./manifests/ --prune -l app=myapp -n production

# 4. 使用 Server-Side Apply 方式对比
kubectl diff -f deployment.yaml --server-side

# 5. CI/CD 发布前检查差异
kubectl diff -f ./k8s/production/ | tee diff-output.txt
if [ ${PIPESTATUS[0]} -eq 0 ]; then
  echo "No changes detected"
elif [ ${PIPESTATUS[0]} -eq 1 ]; then
  echo "Changes detected, ready to apply"
else
  echo "Error during diff"
fi
```

> **注意事项:**
> - `diff` 返回码: 0=无差异, 1=有差异, >1=错误
> - 需要在环境中安装 `diff` 命令 (Linux/macOS 默认有)
> - `diff` 不会修改集群状态，可安全用于 CI/CD 检查
> - 敏感字段 (如 Secret) 可能显示明文，注意日志安全

### 10.2 kubectl kustomize

**语法格式:**
```bash
kubectl kustomize <kustomization_directory> [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `-o yaml/json` | 输出格式 | `kubectl kustomize ./base/ -o yaml` |
| `--enable-helm` | 启用 Helm 支持 | `kubectl kustomize ./base/ --enable-helm` |
| `--load-restrictor=LoadRestrictionsNone` | 禁用加载限制 | `kubectl kustomize ./base/ --load-restrictor=LoadRestrictionsNone` |
| `--reorder=legacy/none` | 资源排序方式 | `kubectl kustomize ./base/ --reorder=none` |

**生产环境示例:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 1. 渲染 Kustomize 配置为完整 YAML
kubectl kustomize ./overlays/production/ > production-manifests.yaml

# 2. 渲染并直接应用
kubectl kustomize ./overlays/production/ | kubectl apply -f -

# 3. 渲染带 Helm Chart 引用的配置
kubectl kustomize ./base/ --enable-helm

# 4. 查看渲染后的资源顺序
kubectl kustomize ./overlays/production/ | grep -E "^apiVersion:|^kind:|^  name:"

# 5. 对比不同环境的 Kustomize 输出
diff <(kubectl kustomize ./overlays/staging/) <(kubectl kustomize ./overlays/production/)
```

> **注意事项:**
> - `kubectl kustomize` 是内置的 Kustomize 渲染器
> - 与独立 `kustomize` 二进制版本可能有差异
> - `--enable-helm` 需要 v1.25+ 且 Kustomize v4.1.0+
> - 生产环境建议先渲染输出审查再应用

### 10.3 kubectl patch

**语法格式:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch <resource> <name> --type=<type> -p '<patch>' [options]
```

| Patch 类型 | 说明 | 适用场景 |
|------------|------|----------|
| `strategic` | 策略合并 (默认) | 列表字段有 merge 策略时 |
| `merge` | JSON Merge Patch | 简单字段覆盖 |
| `json` | JSON Patch (RFC 6902) | 精确操作特定字段 |

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--type=<type>` | Patch 类型 | `kubectl patch deploy nginx --type=strategic -p '{...}'` |
| `-p <patch>` | Patch 内容 | `kubectl patch deploy nginx -p '{"spec":{"replicas":5}}'` |
| `--patch-file=<file>` | 从文件读取 patch | `kubectl patch deploy nginx --patch-file=patch.json` |
| `--dry-run=server` | Server 端模拟 | `kubectl patch deploy nginx -p '...' --dry-run=server` |

**生产环境示例:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
# 1. Strategic Merge Patch - 更新副本数 (默认)
kubectl patch deployment api-gateway -n production -p '{"spec":{"replicas":10}}'

# 2. JSON Patch - 精确修改特定字段
kubectl patch deployment api-gateway -n production --type=json   -p='[{"op": "replace", "path": "/spec/template/spec/containers/0/image", "value": "nginx:1.26"}]'

# 3. Strategic Patch - 添加/更新环境变量
kubectl patch deployment api-gateway -n production --type=strategic   -p='{"spec":{"template":{"spec":{"containers":[{"name":"api-gateway","env":[{"name":"LOG_LEVEL","value":"debug"}]}]}}}}'

# 4. Merge Patch - 简单字段覆盖
kubectl patch node worker-01 --type=merge   -p='{"spec":{"unschedulable":true}}'

# 5. JSON Patch - 删除字段
kubectl patch deployment api-gateway -n production --type=json   -p='[{"op": "remove", "path": "/spec/template/spec/containers/0/livenessProbe"}]'

# 6. 从文件应用 patch
kubectl patch deployment api-gateway -n production --patch-file=./patches/add-sidecar.yaml

# 7. 批量 patch 所有 Deployment 的 resources
for deploy in $(kubectl get deploy -n production -o name); do
  kubectl patch "$deploy" -n production --type=strategic     -p='{"spec":{"template":{"spec":{"containers":[{"name":"app","resources":{"limits":{"cpu":"2"}}}]}}}}'
done
```

> **注意事项:**
> - `strategic` 是默认类型，对列表字段有智能合并行为
> - `json` 类型最精确但需要正确的 JSON Path
> - 容器列表 patch 时必须包含 `name` 字段 (strategic merge key)
> - Patch 语法错误可能导致意外结果，建议先用 `--dry-run=server` 验证
> - 复杂的 patch 建议通过 YAML 文件管理

### 10.4 kubectl wait

**语法格式:**
```bash
kubectl wait <resource> <name> --for=<condition> [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--for=condition=<condition>` | 等待条件满足 | `kubectl wait pod nginx --for=condition=Ready` |
| `--for=delete` | 等待删除完成 | `kubectl wait pod nginx --for=delete` |
| `--for=jsonpath='...'` | 等待 JSONPath 条件 | `kubectl wait pod nginx --for=jsonpath='{.status.phase}'=Running` |
| `--timeout=<duration>` | 超时时间 | `kubectl wait pod nginx --for=condition=Ready --timeout=120s` |
| `--selector=<labels>` | 按标签选择 | `kubectl wait pods -l app=nginx --for=condition=Ready` |

**生产环境示例:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
# 1. 等待 Pod 就绪
kubectl wait pod nginx-7d8c9b4f5-x2k9m --for=condition=Ready --timeout=120s -n production

# 2. 等待 Deployment 滚动更新完成
kubectl wait deployment/api-gateway --for=condition=Available --timeout=300s -n production

# 3. 等待 Job 完成
kubectl wait job/batch-processor --for=condition=Complete --timeout=600s -n production

# 4. 等待所有标签匹配的 Pod 就绪
kubectl wait pods -l app=api-gateway --for=condition=Ready --timeout=180s -n production

# 5. 等待资源删除完成
kubectl wait pod old-nginx --for=delete --timeout=60s -n production

# 6. 等待特定 JSONPath 条件
kubectl wait pod nginx-7d8c9b4f5-x2k9m --for=jsonpath='{.status.phase}'=Running --timeout=60s

# 7. CI/CD 发布等待脚本
kubectl apply -f deployment.yaml
kubectl wait deployment/api-gateway --for=condition=Available --timeout=300s
```

> **注意事项:**
> - 默认超时时间为 30 秒，生产环境建议显式设置
> - `--for=delete` 在资源不存在时返回成功 (幂等)
> - 多个资源等待时，任一失败即返回错误
> - 是 CI/CD 管道中实现发布等待的关键命令

### 10.5 kubectl annotate

**语法格式:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
kubectl annotate <resource> <name> <key>=<value> [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--overwrite` | 覆盖现有 annotation | `kubectl annotate pod nginx owner=team-a --overwrite` |
| `--all` | 更新所有资源 | `kubectl annotate pods --all team=platform` |
| `-l <selector>` | 按标签选择 | `kubectl annotate pods -l app=nginx cost-center=12345` |
| `--resource-version=<version>` | 乐观锁 | `kubectl annotate pod nginx note=test --resource-version=12345` |
| `--local` | 本地输出 | `kubectl annotate pod nginx note=test --local -o yaml` |

**生产环境示例:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 1. 添加 annotation 标记资源所有者
kubectl annotate deployment api-gateway owner=platform-team -n production

# 2. 添加变更记录 annotation
kubectl annotate deployment api-gateway changed-by=$(whoami) changed-at=$(date -Iseconds) --overwrite -n production

# 3. 批量标记 Pod
kubectl annotate pods -l app=api-gateway cost-center=CC-12345 --overwrite -n production

# 4. 删除 annotation
kubectl annotate deployment api-gateway owner- changed-by- -n production

# 5. 为 Node 添加注解
kubectl annotate node worker-01 topology.kubernetes.io/zone=zone-a --overwrite

# 6. 触发 Ingress 证书重新颁发 (cert-manager)
kubectl annotate ingress api-gateway cert-manager.io/issue-temporary-certificate- -n production
```

> **注意事项:**
> - 不加 `--overwrite` 时，如果 annotation 已存在会报错
> - annotation 值长度无严格限制，适合存储元数据
> - 某些 annotation 是控制器使用的 (如 `deployment.kubernetes.io/revision`)
> - 删除 annotation 使用 `key-` 语法

### 10.6 kubectl label

**语法格式:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
kubectl label <resource> <name> <key>=<value> [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--overwrite` | 覆盖现有标签 | `kubectl label node worker-01 env=prod --overwrite` |
| `--all` | 更新所有资源 | `kubectl label pods --all env=prod` |
| `-l <selector>` | 按标签选择 | `kubectl label pods -l app=nginx tier=frontend` |
| `--resource-version=<version>` | 乐观锁 | `kubectl label pod nginx env=prod --resource-version=123` |
| `--local` | 本地输出 | `kubectl label pod nginx env=prod --local -o yaml` |

**生产环境示例:**

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

```bash
# 1. 为 Pod 添加标签
kubectl label pod nginx-7d8c9b4f5-x2k9m env=production -n production

# 2. 覆盖现有标签
kubectl label deployment api-gateway version=v2.1.0 --overwrite -n production

# 3. 批量标记节点
kubectl label nodes -l node-role.kubernetes.io/worker pool=pool-a --overwrite

# 4. 删除标签
kubectl label deployment api-gateway version- env- -n production

# 5. 为 Namespace 添加标签
kubectl label namespace production team=platform cost-center=12345 --overwrite

# 6. 为 Service 添加标签 (用于选择器匹配)
kubectl label service api-gateway app=api-gateway tier=backend --overwrite -n production
```

> **注意事项:**
> - 标签是选择器的基础，修改可能影响 Service/Deployment 匹配
> - 某些系统标签受保护，不能修改 (如 `kubernetes.io/os`)
> - Node 标签修改可能影响 Pod 调度
> - 批量标签操作前建议先确认影响范围

---

<!-- chunk: 11. Shell 自动补全与插件 -->
## 11. Shell 自动补全与插件

### 11.1 kubectl completion

**语法格式:**
```bash
kubectl completion <shell>
```

| 支持的 Shell | 说明 | 示例 |
|--------------|------|------|
| `bash` | Bash 自动补全 | `source <(kubectl completion bash)` |
| `zsh` | Zsh 自动补全 | `source <(kubectl completion zsh)` |
| `fish` | Fish 自动补全 | `kubectl completion fish | source` |
| `powershell` | PowerShell 自动补全 | `kubectl completion powershell | Out-String | Invoke-Expression` |

**生产环境配置示例:**

```bash
# Bash - 添加到 ~/.bashrc
echo 'source <(kubectl completion bash)' >> ~/.bashrc

# Bash (macOS with Homebrew bash)
echo 'source <(kubectl completion bash)' >> ~/.bash_profile

# Zsh - 添加到 ~/.zshrc
echo 'source <(kubectl completion zsh)' >> ~/.zshrc

# 为 kubectl 别名启用补全 (如 k)
echo 'alias k=kubectl' >> ~/.bashrc
echo 'complete -o default -F __start_kubectl k' >> ~/.bashrc

# Fish
kubectl completion fish | source
kubectl completion fish > ~/.config/fish/completions/kubectl.fish
```

> **注意事项:**
> - 需要 `bash-completion` 包已安装
> - kubectl v1.26+ 使用 Cobra v1.6+ 生成补全脚本
> - 别名补全配置需要保持与 kubectl 版本同步

### 11.2 kubectl plugin list

**语法格式:**
```bash
kubectl plugin list [options]
```

| 常用选项 | 说明 | 示例 |
|----------|------|------|
| `--name-only` | 仅显示名称 | `kubectl plugin list --name-only` |

**生产环境示例:**

```bash
# 1. 列出所有已安装的插件
kubectl plugin list

# 2. 仅显示插件名称
kubectl plugin list --name-only

# 3. 常用插件示例
# kubectl krew (插件管理器)
# kubectl ctx / kubectx (上下文切换)
# kubectl ns / kubens (命名空间切换)
# kubectl stern (多 Pod 日志聚合)
# kubectl tree (资源层级树)
# kubectl resource-capacity (节点资源容量)
```

**常用插件推荐:**

| 插件 | 用途 | 安装 |
|------|------|------|
| `krew` | 插件管理器 | `kubectl krew install krew` |
| `ctx` | 快速切换上下文 | `kubectl krew install ctx` |
| `ns` | 快速切换命名空间 | `kubectl krew install ns` |
| `stern` | 多 Pod 日志聚合 | `kubectl krew install stern` |
| `tree` | 资源层级树 | `kubectl krew install tree` |
| `resource-capacity` | 节点资源容量 | `kubectl krew install resource-capacity` |
| `node-shell` | 节点 shell | `kubectl krew install node-shell` |
| `df-pv` | PVC 磁盘使用 | `kubectl krew install df-pv` |

> **注意事项:**
> - 插件是独立的二进制文件，命名格式为 `kubectl-<name>`
> - 插件存放在 `$PATH` 中即可被 kubectl 识别
> - Krew 是官方推荐的插件管理器
> - 生产环境使用插件前需评估其安全性和维护状态

---

<!-- chunk: 12. 生产环境速查表 -->
## 12. 生产环境速查表

### 12.1 高频命令速查

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘
> - `kubectl cordon`：标记节点不可调度
> - `kubectl drain`：驱逐节点所有 Pod，业务流量受影响
> - `kubectl taint nodes`：变更污点影响 Pod 调度

```
┌─────────────────────────────────────────────────────────────────────────────────┐
│                    kubectl Production Quick Reference                             │
├─────────────────────────────────────────────────────────────────────────────────┤
│                                                                                  │
│  发布部署                                                                        │
│  ─────────────────────────────────────────────────────────────────────────────   │
│  kubectl apply -f manifest.yaml --server-side                                    │
│  kubectl apply -f manifest.yaml --dry-run=server -o yaml                         │
│  kubectl rollout status deployment/app -w --timeout=300s                         │
│  kubectl rollout restart deployment/app                                          │
│  kubectl rollout undo deployment/app                                             │
│  kubectl diff -f manifest.yaml                                                   │
│                                                                                  │
│  排查问题                                                                        │
│  ─────────────────────────────────────────────────────────────────────────────   │
│  kubectl get pods -A -o wide                                                     │
│  kubectl describe pod <pod> -n <ns>                                              │
│  kubectl logs <pod> --previous -n <ns>              # CrashLoopBackOff          │
│  kubectl logs <pod> -f --tail=100 -n <ns>                                        │
│  kubectl get events -A --types=Warning                                           │
│  kubectl top pod --sort-by=cpu -A | head -n 11                                   │
│  kubectl delete pod <pod> --grace-period=0 --force -n <ns>  # 强制删除          │  # ⚠️ 跳过优雅终止，可能丢数据
│                                                                                  │
│  容器调试                                                                        │
│  ─────────────────────────────────────────────────────────────────────────────   │
│  kubectl exec -it <pod> -- bash                                                  │
│  kubectl debug <pod> -it --image=nicolaka/netshoot --target=<container>          │
│  kubectl port-forward svc/<svc> 8080:80 -n <ns>                                  │
│  kubectl cp ./file <pod>:/tmp/file -n <ns>                                       │
│                                                                                  │
│  权限检查                                                                        │
│  ─────────────────────────────────────────────────────────────────────────────   │
│  kubectl auth can-i --list -n <ns>                                               │
│  kubectl auth can-i create pods --as=<user> -n <ns>                              │
│  kubectl create token <sa> -n <ns> --duration=1h                                 │
│                                                                                  │
│  节点运维                                                                        │
│  ─────────────────────────────────────────────────────────────────────────────   │
│  kubectl cordon <node>                                                           │
│  kubectl drain <node> --ignore-daemonsets --delete-emptydir-data                 │
│  kubectl uncordon <node>                                                         │
│  kubectl taint nodes <node> key=value:NoSchedule                                 │
│                                                                                  │
└─────────────────────────────────────────────────────────────────────────────────┘
```

### 12.2 按场景分类速查表

| 场景 | 推荐命令 |
|------|----------|
| **发布前验证** | `kubectl apply -f manifest.yaml --dry-run=server -o yaml` |
| **灰度发布** | `kubectl rollout pause deployment/app` -> 验证 -> `kubectl rollout resume deployment/app` |
| **紧急回滚** | `kubectl rollout undo deployment/app --to-revision=<N>` |
| **服务扩容** | `kubectl scale deployment/app --replicas=<N>` |
| **ConfigMap 更新** | `kubectl apply -f configmap.yaml` + `kubectl rollout restart deployment/app` |
| **Secret 更新** | `kubectl apply -f secret.yaml` + `kubectl rollout restart deployment/app` |
| **Pod Crash 排查** | `kubectl describe pod <pod>` -> `kubectl logs <pod> --previous` |
| **Pod Pending 排查** | `kubectl describe pod <pod>` -> 检查资源/节点选择器/污点 |
| **OOMKilled 排查** | `kubectl describe pod <pod>` -> `kubectl top pod <pod> --containers` |
| **网络不通排查** | `kubectl exec -it <pod> -- curl <target>` -> `kubectl get svc,endpoints` |
| **性能问题排查** | `kubectl top pod/node` -> `kubectl logs` -> `kubectl describe node` |
| **权限拒绝排查** | `kubectl auth can-i <verb> <resource> --as=<user>` -> `kubectl get rolebinding,clusterrolebinding` |
| **证书过期处理** | `kubectl get csr` -> `kubectl certificate approve <csr>` |
| **节点维护** | `kubectl cordon <node>` -> `kubectl drain <node> --ignore-daemonsets --delete-emptydir-data` -> 维护 -> `kubectl uncordon <node>` |
| **清理测试资源** | `kubectl delete all -l env=testing --namespace=staging` |
| **强制删除资源** | `kubectl delete <resource> <name> --grace-period=0 --force` |

### 12.3 安全敏感命令速查

| 操作 | 命令 | 风险等级 |
|------|------|----------|
| 强制删除 Pod | `kubectl delete pod <pod> --grace-period=0 --force` | 高 |
| 跳过 TLS 验证 | `kubectl --insecure-skip-tls-verify get nodes` | 高 |
| 批量删除资源 | `kubectl delete all --all -n <ns>` | 高 |
| 节点 drain | `kubectl drain <node> --force --ignore-daemonsets` | 中 |
| 应用带 prune | `kubectl apply -f ./ --prune -l app=<name>` | 高 |
| 强制 replace | `kubectl replace -f manifest.yaml --force` | 中 |
| 暴露代理到全网 | `kubectl proxy --address=0.0.0.0 --accept-hosts='^.*'` | 高 |
| 创建长期 Token | `kubectl create token <sa> --duration=8760h` | 中 |
| 修改系统标签 | `kubectl label node <node> kubernetes.io/role=master --overwrite` | 中 |
| 调试 Node | `kubectl debug node/<node> -it --image=<image>` | 中 |

### 12.4 输出格式速查

| 需求 | 命令 |
|------|------|
| 查看 Pod IP 和节点 | `kubectl get pod -o wide` |
| 查看资源 YAML | `kubectl get <resource> <name> -o yaml` |
| 查看资源 JSON | `kubectl get <resource> <name> -o json` |
| 仅输出名称 | `kubectl get pods -o name` |
| 提取特定字段 | `kubectl get pod <pod> -o jsonpath={.status.podIP}` |
| 自定义输出列 | `kubectl get pods -o custom-columns=NAME:.metadata.name,IP:.status.podIP` |
| 显示标签 | `kubectl get pods --show-labels` |
| 按标签过滤 | `kubectl get pods -l app=nginx,tier=frontend` |
| 按字段过滤 | `kubectl get pods --field-selector=status.phase=Running` |
| 排序输出 | `kubectl get pods --sort-by=.status.startTime` |
| 监听变化 | `kubectl get pods -w` |
| 所有命名空间 | `kubectl get pods -A` |

---

> **文档维护说明:**
> - 本文档覆盖 Kubernetes v1.25 - v1.32+ 的 kubectl 核心命令
> - 部分命令在不同版本间存在差异，请以实际集群版本为准
> - 生产环境执行任何修改操作前，建议先使用 `--dry-run=server` 验证
> - 本文档最后更新于 2026-04，建议定期回顾更新

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-3: Kubernetes控制平面]]
- Domain-3 控制平面 — 开源项目索引
- Kubernetes 控制平面架构总览 (Control Plane Architecture Overview)
- 控制平面组件交互详解 (Control Plane Components Interaction Deep Dive)
- 控制平面高可用部署模式 (Control Plane High Availability Deployment Patt...
- 控制平面安全加固指南 (Control Plane Security Hardening Guide)
- 控制平面监控与可观测性 (Control Plane Monitoring & Observability)
- 控制平面故障排查手册 (Control Plane Troubleshooting Handbook)
- 控制平面升级与迁移策略 (Control Plane Upgrade & Migration Strategy)
- 控制平面性能基准测试 (Control Plane Performance Benchmarking)
- 控制平面扩缩容指南 (Control Plane Scalability Guide)

## See Also

- 29-in-place-pod-resize
- 30-dynamic-resource-allocation
- 32-kubeadm-cluster-lifecycle
- 32-kubeadm-upgrade-complete-guide

- [[domain-07-platform-engineering/topic-code-analysis/deployment-create/01-overview.md|01-overview]]
- [[domain-07-platform-engineering/topic-code-analysis/cluster-delete/01-overview.md|01-overview]]