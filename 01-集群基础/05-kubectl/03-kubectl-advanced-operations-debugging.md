---
title: kubectl Advanced Operations and Production Debugging Toolkit
description: kubectl 高级操作 — 调试技巧、临时容器、资源管理、批量操作、输出格式化、插件生态、自动化脚本
summary: 面向生产环境的 kubectl 高级操作技巧与调试工具链完整指南
category: practice
tags:
- kubectl
- debugging
- operations
- cli
- troubleshooting
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: intermediate
domain: cluster
---
# kubectl 高级操作与生产调试工具

> 面向生产环境的 kubectl 高级技巧、调试方法与效率提升。

## 调试技巧

### 临时容器（Ephemeral Containers）

```bash
# 向运行中的 Pod 注入调试容器（无需重启）
kubectl debug -it pod-name -n production \
  --image=nicolaka/netshoot \
  --target=app-container \
  -- bash

# 调试节点（创建特权 Pod 挂载节点文件系统）
kubectl debug node/worker-1 -it --image=ubuntu:22.04 -- bash
# 进入后: chroot /host

# 复制 Pod 并添加调试工具
kubectl debug pod-name -n production \
  --copy-to=debug-pod \
  --image=nicolaka/netshoot \
  --share-processes \
  -- bash
```

### 日志高级操作

```bash
# 多容器 Pod 指定容器
kubectl logs pod-name -c sidecar -n production

# 前一个容器实例的日志（CrashLoopBackOff）
kubectl logs pod-name -n production --previous

# 时间范围过滤
kubectl logs pod-name -n production --since=1h
kubectl logs pod-name -n production --since-time="2026-07-21T10:00:00Z"

# 带时间戳
kubectl logs pod-name -n production --timestamps

# 多 Pod 聚合（stern 替代）
kubectl logs -l app=api-server -n production --tail=100 --prefix

# 流式日志 + grep
kubectl logs -f pod-name -n production | grep --line-buffered "ERROR"
```

## 资源管理

### 批量操作

```bash
# 批量删除 Completed Jobs
kubectl delete jobs -n production --field-selector status.successful=1

# 批量重启 Deployment（滚动重启）
kubectl rollout restart deployment -n production

# 批量设置资源限制（LimitRange 替代）
kubectl set resources deployment -n production --all \
  --limits=cpu=2,memory=2Gi \
  --requests=cpu=100m,memory=128Mi

# 批量添加标签
kubectl label pods -n production -l app=api-server team=backend --overwrite

# 批量添加注解
kubectl annotate deployment -n production --all \
  kubectl.kubernetes.io/restartedAt="$(date -u +%Y-%m-%dT%H:%M:%SZ)"

# 按标签批量删除
kubectl delete pods -n production -l env=test --grace-period=0 --force
```

### 资源配额检查

```bash
# 命名空间资源使用
kubectl top pods -n production --sort-by=memory
kubectl top nodes --sort-by=cpu

# 查看 ResourceQuota 使用情况
kubectl describe resourcequota -n production

# 查看 LimitRange
kubectl get limitrange -n production -o yaml

# 节点资源分配
kubectl describe node worker-1 | grep -A 20 "Allocated resources"
```

## 输出格式化

### JSONPath 查询

```bash
# 获取所有 Pod IP
kubectl get pods -n production -o jsonpath='{.items[*].status.podIP}'

# 获取 Deployment 镜像
kubectl get deploy -n production -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.template.spec.containers[0].image}{"\n"}{end}'

# 获取 Secret 的 data keys（不解码）
kubectl get secret my-secret -o jsonpath='{.data}' | jq 'keys'

# 获取 PVC 容量
kubectl get pvc -n production -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.resources.requests.storage}{"\n"}{end}'

# 获取节点内核版本
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.kernelVersion}{"\n"}{end}'
```

### 自定义列输出

```bash
# Pod 状态概览
kubectl get pods -n production -o custom-columns=\
NAME:.metadata.name,\
STATUS:.status.phase,\
RESTARTS:.status.containerStatuses[0].restartCount,\
NODE:.spec.nodeName,\
AGE:.metadata.creationTimestamp,\
IP:.status.podIP

# Deployment 概览
kubectl get deploy -n production -o custom-columns=\
NAME:.metadata.name,\
READY:.status.readyReplicas,\
DESIRED:.spec.replicas,\
IMAGE:.spec.template.spec.containers[0].image,\
STRATEGY:.spec.strategy.type
```

## 生产操作

### 安全操作模式

```bash
# 干运行（不实际执行）
kubectl apply -f deployment.yaml --dry-run=server -o yaml
kubectl delete pod test --dry-run=client

# 编辑前备份
kubectl get deployment api-server -n production -o yaml > backup-$(date +%s).yaml
kubectl edit deployment api-server -n production

# 等待条件满足
kubectl wait --for=condition=available deployment/api-server -n production --timeout=300s
kubectl wait --for=condition=ready pod -l app=api-server -n production --timeout=120s

# 原子性 patch
kubectl patch deployment api-server -n production \
  --type='json' \
  -p='[{"op":"replace","path":"/spec/replicas","value":5}]'

# 回滚到指定版本
kubectl rollout history deployment/api-server -n production
kubectl rollout undo deployment/api-server -n production --to-revision=3
```

### 上下文管理

```bash
# 查看所有上下文
kubectl config get-contexts

# 切换上下文
kubectl config use-context production-cluster

# 临时指定上下文（不切换）
kubectl get pods --context=staging-cluster -n default

# 设置默认命名空间
kubectl config set-context --current --namespace=production

# 查看当前配置
kubectl config view --minify
```

## 插件生态（krew）

```bash
# 安装 krew
kubectl krew install krew

# 推荐插件
kubectl krew install neat       # 清理 YAML 输出
kubectl krew install ctx        # 快速切换 context
kubectl krew install ns         # 快速切换 namespace
kubectl krew install sniff      # Pod 抓包（Wireshark）
kubectl krew install view-utilization  # 资源利用率
kubectl krew install df         # 节点磁盘使用
kubectl krew install resource-capacity  # 调度容量分析
kubectl krew install who-can    # RBAC 权限查询
kubectl krew install access-matrix  # RBAC 矩阵
kubectl krew install images     # 列出所有镜像
kubectl krew install unused-volumes  # 未使用的 PVC
kubectl krew install node-shell # 节点 shell
```

### 常用插件示例

```bash
# RBAC 权限查询
kubectl who-can get secrets -n production
kubectl access-matrix -n production

# 资源利用率
kubectl view-utilization -n production
kubectl resource-capacity --pods -n production

# 列出所有镜像
kubectl images -n production

# 节点 shell
kubectl node-shell worker-1
```

## 自动化脚本模式

### 健康检查脚本

```bash
#!/bin/bash
# cluster-health-check.sh
echo "=== 集群健康检查 $(date) ==="

echo "--- 节点状态 ---"
kubectl get nodes -o wide | awk '{print $1, $2, $5}'

echo "--- 异常 Pod ---"
kubectl get pods -A --field-selector 'status.phase!=Running,status.phase!=Succeeded' \
  -o custom-columns=NS:.metadata.namespace,NAME:.metadata.name,STATUS:.status.phase

echo "--- 重启次数 > 5 的 Pod ---"
kubectl get pods -A -o json | jq -r '.items[] | 
  select(.status.containerStatuses[]?.restartCount > 5) | 
  "\(.metadata.namespace)/\(.metadata.name) restarts=\(.status.containerStatuses[0].restartCount)"'

echo "--- PVC 未绑定 ---"
kubectl get pvc -A --field-selector status.phase!=Bound

echo "--- 证书过期检查 ---"
kubectl get certificates -A -o json | jq -r '.items[] | 
  select(.status.notAfter != null) |
  "\(.metadata.namespace)/\(.metadata.name) expires=\(.status.notAfter)"'

echo "--- 资源使用 Top 5 ---"
kubectl top pods -A --sort-by=memory | head -6
```

## 性能优化

| 场景 | 优化方法 |
|------|----------|
| 大量资源列表 | `--chunk-size=500` 分页 |
| 频繁 API 调用 | 使用 `--cache-dir` 本地缓存 |
| 大集群 get | 添加 `-l` 标签过滤 |
| 批量操作 | `xargs -P` 并行 |
| 脚本中 | 使用 `--output=json` + `jq` 替代多次调用 |

```bash
# 并行操作示例
kubectl get pods -n production -o name | \
  xargs -P 10 -I {} kubectl delete {} -n production --grace-period=0
```

---

## 生产事故诊断 Runbook

### Pod CrashLoopBackOff 诊断

```bash
#!/bin/bash
# 🟢 diagnose-crashloop.sh <pod-name> <namespace>
POD="${1:?用法: $0 <pod-name> <namespace>}"
NS="${2:-default}"

echo "════ CrashLoopBackOff 诊断: $NS/$POD ════"

# 1. Pod 状态概览
echo -e "\n[1] Pod 状态"
kubectl get pod $POD -n $NS -o wide

# 2. 事件
echo -e "\n[2] 最近事件"
kubectl get events -n $NS --field-selector involvedObject.name=$POD --sort-by='.lastTimestamp' | tail -10

# 3. 前一次容器日志
echo -e "\n[3] 前一次容器日志 (last 50 lines)"
kubectl logs $POD -n $NS --previous --tail=50 2>/dev/null || echo "  无前期日志"

# 4. 资源使用
echo -e "\n[4] 资源使用"
kubectl top pod $POD -n $NS 2>/dev/null || echo "  metrics 不可用"

# 5. 容器状态详情
echo -e "\n[5] 容器状态"
kubectl get pod $POD -n $NS -o jsonpath='{range .status.containerStatuses[*]}{.name}: {.state}{"\n"}{end}'

# 6. 常见原因检查
echo -e "\n[6] 常见原因检查"
# OOMKilled?
kubectl get pod $POD -n $NS -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}' 2>/dev/null
echo ""
# 镜像拉取失败?
kubectl get pod $POD -n $NS -o jsonpath='{.status.conditions[?(@.type=="ContainersReady")].message}'
echo ""
```

### 节点 NotReady 诊断

```bash
#!/bin/bash
# 🟢 diagnose-node.sh <node-name>
NODE="${1:?用法: $0 <node-name>}"

echo "════ 节点诊断: $NODE ════"

# 1. 节点状态与条件
echo -e "\n[1] 节点条件"
kubectl get node $NODE -o jsonpath='{range .status.conditions[*]}{.type}: {.status} ({.reason}){"\n"}{end}'

# 2. kubelet 状态（通过节点日志）
echo -e "\n[2] 节点事件"
kubectl get events -A --field-selector involvedObject.name=$NODE --sort-by='.lastTimestamp' | tail -10

# 3. 节点上的 Pod
echo -e "\n[3] 节点上的 Pod 状态"
kubectl get pods -A --field-selector spec.nodeName=$NODE | grep -v Running | grep -v Completed

# 4. 节点资源
echo -e "\n[4] 节点资源"
kubectl describe node $NODE | grep -A 15 "Conditions:"

# 5. 网络检查
echo -e "\n[5] CNI Pod 状态"
kubectl get pods -n kube-system -l k8s-app=calico-node -o wide | grep $NODE
kubectl get pods -n kube-system -l app.kubernetes.io/name=cilium -o wide | grep $NODE
```

### 服务连接失败诊断

```bash
#!/bin/bash
# 🟢 diagnose-service.sh <service-name> <namespace>
SVC="${1:?用法: $0 <service-name> <namespace>}"
NS="${2:-default}"

echo "════ 服务连接诊断: $NS/$SVC ════"

# 1. Service 定义
echo -e "\n[1] Service 定义"
kubectl get svc $SVC -n $NS -o wide

# 2. Endpoints
 echo -e "\n[2] Endpoints"
kubectl get endpoints $SVC -n $NS

# 3. 后端 Pod 状态
echo -e "\n[3] 后端 Pod"
SELECTOR=$(kubectl get svc $SVC -n $NS -o jsonpath='{.spec.selector}' | jq -r 'to_entries | map("\(.key)=\(.value)") | join(",")')
kubectl get pods -n $NS -l $SELECTOR -o wide

# 4. DNS 解析
echo -e "\n[4] DNS 解析测试"
kubectl run dns-test --rm -it --restart=Never --image=busybox:1.36 -n $NS -- \
  nslookup $SVC.$NS.svc.cluster.local 2>/dev/null

# 5. 从集群内测试连接
echo -e "\n[5] 连接测试"
CLUSTER_IP=$(kubectl get svc $SVC -n $NS -o jsonpath='{.spec.clusterIP}')
PORT=$(kubectl get svc $SVC -n $NS -o jsonpath='{.spec.ports[0].port}')
kubectl run curl-test --rm -it --restart=Never --image=curlimages/curl -n $NS -- \
  curl -s -o /dev/null -w "%{http_code}" http://$CLUSTER_IP:$PORT/ 2>/dev/null

# 6. NetworkPolicy 检查
echo -e "\n[6] NetworkPolicy"
kubectl get networkpolicy -n $NS
```

---

## kubectl 安全操作规范

### 操作风险分级

| 风险等级 | 操作类型 | 示例 | 要求 |
|----------|----------|------|------|
| 🟢 低 | 只读查询 | get, describe, logs, top | 无特殊要求 |
| 🟡 中 | 状态变更 | apply, scale, label, annotate | 确认目标集群/NS |
| 🔴 高 | 删除/强制 | delete --force, drain, cordon | 双人确认 + 备份 |
| ☠️ 极高 | 不可逆 | delete namespace, delete pv | 主管审批 |

### 生产操作安全检查单

```bash
# 🟢 每次生产操作前执行

# 1. 确认当前上下文
echo "当前集群: $(kubectl config current-context)"
echo "当前 NS: $(kubectl config view --minify -o jsonpath='{.contexts[0].context.namespace}')"

# 2. 确认目标资源存在
kubectl get deployment api-server -n production

# 3. 干运行
kubectl apply -f change.yaml --dry-run=server

# 4. 备份当前状态
kubectl get deployment api-server -n production -o yaml > backup-$(date +%Y%m%d-%H%M%S).yaml

# 5. 执行变更
kubectl apply -f change.yaml

# 6. 验证
kubectl rollout status deployment/api-server -n production --timeout=120s
```

### 危险操作防护

```bash
# 🔴 绝对禁止在生产环境直接执行的操作

# ✘ 禁止: 强制删除带 finalizer 的资源
# kubectl delete crd xxx  # 可能导致所有 CR 被删除

# ✘ 禁止: 删除命名空间（会级联删除所有资源）
# kubectl delete ns production

# ✘ 禁止: 未确认的批量删除
# kubectl delete pods --all -n production

# ✔ 正确: 使用标签选择器精确匹配
kubectl delete pods -n production -l app=old-version,version=v1

# ✔ 正确: 先 cordon 再 drain
kubectl cordon worker-1
kubectl drain worker-1 --ignore-daemonsets --delete-emptydir-data --timeout=300s
```

---

## 大规模集群操作技巧

### 高效查询模式

```bash
# 大集群避免全量 list（使用 field-selector）
kubectl get pods -A --field-selector=status.phase=Failed
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded

# 使用 server-side 分页
kubectl get pods -A --chunk-size=500

# 指定资源版本避免转换开销
kubectl get deployments.v1.apps -n production

# 仅获取名称（减少传输量）
kubectl get pods -n production -o name

# 使用 label selector 缩小范围
kubectl get pods -n production -l 'app in (api,web,worker)'
```

### 批量操作安全模式

```bash
# 安全批量操作模板
# 1. 先统计影响范围
COUNT=$(kubectl get pods -n production -l version=old -o name | wc -l)
echo "将影响 $COUNT 个 Pod"

# 2. 干运行确认
kubectl delete pods -n production -l version=old --dry-run=server

# 3. 分批执行（避免一次性大规模变更）
kubectl get pods -n production -l version=old -o name | head -10 | \
  xargs -I {} kubectl delete {} -n production

# 4. 等待稳定后继续
kubectl wait --for=condition=ready pods -l app=api -n production --timeout=120s
```

### 多集群操作

```bash
# 多集群批量执行
CLUSTERS=("prod-cn" "prod-us" "staging")

for ctx in "${CLUSTERS[@]}"; do
  echo "=== [$ctx] ==="
  kubectl --context=$ctx get nodes --no-headers | wc -l
  kubectl --context=$ctx get pods -A --field-selector=status.phase=Failed --no-headers | wc -l
done

# 使用 kubectx 快速切换
kubectx prod-cn
kubens monitoring
```

---

## 调试工具箱完整清单

| 工具 | 用途 | 安装 | 典型命令 |
|------|------|------|----------|
| **stern** | 多 Pod 日志聚合 | `brew install stern` | `stern -l app=api -n prod` |
| **k9s** | TUI 集群管理 | `brew install k9s` | `k9s --context prod` |
| **kubectx/kubens** | 快速切换上下文/NS | `brew install kubectx` | `kubectx prod-cn` |
| **kube-ps1** | Shell 提示符显示集群 | `brew install kube-ps1` | 自动显示 |
| **kubectl-sniff** | Pod 网络抓包 | `kubectl krew install sniff` | `kubectl sniff pod -n prod` |
| **kubectl-neat** | 清理 YAML 输出 | `kubectl krew install neat` | `kubectl get pod x -o yaml \| kubectl neat` |
| **kubectl-tree** | 资源层级树 | `kubectl krew install tree` | `kubectl tree deploy api -n prod` |
| **kubectl-explore** | CRD 探索 | `kubectl krew install explore` | `kubectl explore prometheusrules` |
| **datree** | 策略检查 | `brew install datree` | `datree test manifest.yaml` |
| **kubectl-debug** | 网络调试 | `kubectl krew install debug` | 集成到 kubectl debug |

### 网络调试工具集

```bash
# 使用 netshoot 镜像进行网络诊断
kubectl run netshoot --rm -it --restart=Never \
  --image=nicolaka/netshoot -n production -- bash

# 内部常用命令:
# curl -v http://service.namespace.svc:port/
# dig service.namespace.svc.cluster.local
# tcpdump -i eth0 -nn port 8080
# ip route show
# ss -tlnp
# mtr --report target-host
# nmap -sT -p 80,443 target-host
```

## Related

- [[01-集群基础/05-kubectl/index.md|kubectl]]
- [[01-集群基础/05-kubectl/01-kubectl-debug-ephemeral-containers.md|临时容器调试]]
- [[19-故障诊断/index.md|故障诊断]]
