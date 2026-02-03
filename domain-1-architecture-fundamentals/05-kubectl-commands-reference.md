# kubectl 命令完整参考 (kubectl Commands Complete Reference)

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-01 | **文档类型**: 生产运维参考

---

## 文档概述

kubectl 是 Kubernetes 集群的命令行工具，用于对集群执行命令和管理资源。本文档提供生产环境级别的完整命令参考，涵盖所有命令类别、高级用法、性能优化及故障排查技巧。

---

## 目录

- [1. kubectl 基础架构](#1-kubectl-基础架构)
- [2. 资源查看命令 (get/describe/explain)](#2-资源查看命令)
- [3. 资源创建与管理 (create/apply/delete)](#3-资源创建与管理)
- [4. Pod 调试与交互 (exec/logs/attach/cp)](#4-pod-调试与交互)
- [5. 资源编辑与补丁 (edit/patch/replace)](#5-资源编辑与补丁)
- [6. 部署管理 (rollout/scale/autoscale)](#6-部署管理)
- [7. 集群管理 (cluster-info/top/cordon/drain)](#7-集群管理)
- [8. 配置与上下文 (config/context)](#8-配置与上下文)
- [9. 高级调试 (debug/port-forward/proxy)](#9-高级调试)
- [10. 认证与授权 (auth/certificate)](#10-认证与授权)
- [11. 插件与扩展 (plugin/krew)](#11-插件与扩展)
- [12. 性能优化与最佳实践](#12-性能优化与最佳实践)
- [13. 生产环境运维脚本](#13-生产环境运维脚本)
- [14. 故障排查速查表](#14-故障排查速查表)

---

## 1. kubectl 基础架构

### 1.1 kubectl 工作原理

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           kubectl 架构                                   │
├─────────────────────────────────────────────────────────────────────────┤
│  ┌──────────────┐     ┌──────────────┐     ┌──────────────────────────┐ │
│  │   kubectl    │────▶│  kubeconfig  │────▶│  kube-apiserver          │ │
│  │  (CLI Tool)  │     │  (~/.kube/)  │     │  (REST API + Auth)       │ │
│  └──────────────┘     └──────────────┘     └──────────────────────────┘ │
│         │                    │                        │                  │
│         ▼                    ▼                        ▼                  │
│  ┌──────────────┐     ┌──────────────┐     ┌──────────────────────────┐ │
│  │ Command Parse│     │ Context/Auth │     │ Admission Controllers    │ │
│  │ + Validation │     │ Certificate  │     │ + etcd Storage           │ │
│  └──────────────┘     └──────────────┘     └──────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
```

### 1.2 版本兼容性矩阵

| kubectl 版本 | 支持的集群版本 | 重要特性变更 |
|:---|:---|:---|
| v1.32 | v1.31 - v1.33 | kubectl debug 增强、apply --server-side 默认 |
| v1.31 | v1.30 - v1.32 | kubectl events 命令 GA、--subresource 支持 |
| v1.30 | v1.29 - v1.31 | kubectl auth whoami、交互式删除确认 |
| v1.29 | v1.28 - v1.30 | kubectl debug profile 增强 |
| v1.28 | v1.27 - v1.29 | kubectl --subresource=status/scale |
| v1.27 | v1.26 - v1.28 | kubectl auth whoami (Beta) |
| v1.26 | v1.25 - v1.27 | kubectl events 命令 Beta |
| v1.25 | v1.24 - v1.26 | PSP 移除、kubectl create token |

> **重要**: kubectl 版本应与集群版本差异不超过一个次要版本 (n±1)

### 1.3 全局选项参考

| 选项 | 简写 | 说明 | 生产环境建议 |
|:---|:---|:---|:---|
| `--kubeconfig` | - | 指定配置文件路径 | 多集群场景必用 |
| `--context` | - | 指定上下文 | 避免误操作生产集群 |
| `--namespace` | `-n` | 指定命名空间 | 配合 kubens 使用 |
| `--all-namespaces` | `-A` | 所有命名空间 | 全局资源查看 |
| `--output` | `-o` | 输出格式 | json/yaml/wide/name/custom-columns |
| `--selector` | `-l` | 标签选择器 | 批量操作必用 |
| `--field-selector` | - | 字段选择器 | 状态过滤 |
| `--dry-run` | - | 模拟运行 | client/server/none |
| `--v` | - | 日志级别 (0-10) | 调试时用 6-9 |
| `--request-timeout` | - | 请求超时 | 默认 0 (无限) |
| `--as` | - | 用户模拟 | RBAC 测试 |
| `--as-group` | - | 组模拟 | RBAC 测试 |

### 1.4 输出格式详解

| 格式 | 说明 | 适用场景 | 示例 |
|:---|:---|:---|:---|
| `wide` | 扩展列输出 | 快速查看更多信息 | `-o wide` |
| `yaml` | YAML 格式 | 资源导出、备份 | `-o yaml` |
| `json` | JSON 格式 | 脚本处理、API 调试 | `-o json` |
| `name` | 仅输出名称 | 管道操作 | `-o name` |
| `jsonpath` | JSONPath 表达式 | 精确字段提取 | `-o jsonpath='{.spec}'` |
| `jsonpath-file` | 从文件读取表达式 | 复杂表达式 | `-o jsonpath-file=tpl.txt` |
| `custom-columns` | 自定义列 | 定制输出 | `-o custom-columns=NAME:.metadata.name` |
| `custom-columns-file` | 从文件读取列定义 | 复杂报表 | `-o custom-columns-file=cols.txt` |
| `go-template` | Go 模板 | 高级格式化 | `-o go-template='{{.metadata.name}}'` |
| `go-template-file` | 从文件读取模板 | 复杂报表 | `-o go-template-file=tpl.txt` |

---

## 2. 资源查看命令

### 2.1 kubectl get - 资源列表查询

#### 基础语法

```bash
kubectl get <resource> [name] [options]
```

#### 常用资源缩写

| 资源类型 | 缩写 | API 组 | 作用域 |
|:---|:---|:---|:---|
| pods | po | core/v1 | Namespaced |
| services | svc | core/v1 | Namespaced |
| deployments | deploy | apps/v1 | Namespaced |
| replicasets | rs | apps/v1 | Namespaced |
| statefulsets | sts | apps/v1 | Namespaced |
| daemonsets | ds | apps/v1 | Namespaced |
| jobs | - | batch/v1 | Namespaced |
| cronjobs | cj | batch/v1 | Namespaced |
| configmaps | cm | core/v1 | Namespaced |
| secrets | - | core/v1 | Namespaced |
| persistentvolumes | pv | core/v1 | Cluster |
| persistentvolumeclaims | pvc | core/v1 | Namespaced |
| nodes | no | core/v1 | Cluster |
| namespaces | ns | core/v1 | Cluster |
| ingresses | ing | networking.k8s.io/v1 | Namespaced |
| networkpolicies | netpol | networking.k8s.io/v1 | Namespaced |
| storageclasses | sc | storage.k8s.io/v1 | Cluster |
| serviceaccounts | sa | core/v1 | Namespaced |
| horizontalpodautoscalers | hpa | autoscaling/v2 | Namespaced |
| poddisruptionbudgets | pdb | policy/v1 | Namespaced |
| endpoints | ep | core/v1 | Namespaced |
| endpointslices | - | discovery.k8s.io/v1 | Namespaced |
| events | ev | events.k8s.io/v1 | Namespaced |
| leases | - | coordination.k8s.io/v1 | Namespaced |

#### 生产环境常用查询

```bash
# 查看所有 Pod 及其详细信息
kubectl get pods -A -o wide

# 按节点分组查看 Pod
kubectl get pods -A -o wide --sort-by='.spec.nodeName'

# 查看非 Running 状态的 Pod
kubectl get pods -A --field-selector=status.phase!=Running

# 查看 Pod 的资源请求和限制
kubectl get pods -o custom-columns=\
'NAME:.metadata.name,'\
'CPU_REQ:.spec.containers[*].resources.requests.cpu,'\
'CPU_LIM:.spec.containers[*].resources.limits.cpu,'\
'MEM_REQ:.spec.containers[*].resources.requests.memory,'\
'MEM_LIM:.spec.containers[*].resources.limits.memory'

# 查看所有镜像及版本
kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"\t"}{.metadata.name}{"\t"}{range .spec.containers[*]}{.image}{"\n"}{end}{end}'

# 查看节点资源分配
kubectl get nodes -o custom-columns=\
'NAME:.metadata.name,'\
'CPU:.status.allocatable.cpu,'\
'MEMORY:.status.allocatable.memory,'\
'PODS:.status.allocatable.pods'

# 查看 PVC 绑定状态
kubectl get pvc -A -o custom-columns=\
'NAMESPACE:.metadata.namespace,'\
'NAME:.metadata.name,'\
'STATUS:.status.phase,'\
'VOLUME:.spec.volumeName,'\
'CAPACITY:.status.capacity.storage,'\
'STORAGECLASS:.spec.storageClassName'

# 查看 Service 端点映射
kubectl get svc,ep -A -o wide

# 查看带特定标签的资源
kubectl get all -l app=nginx -A

# 监视资源变化
kubectl get pods -w --output-watch-events

# 查看资源及其 Owner
kubectl get pods -o custom-columns=\
'NAME:.metadata.name,'\
'OWNER_KIND:.metadata.ownerReferences[0].kind,'\
'OWNER_NAME:.metadata.ownerReferences[0].name'
```

#### 高级 JSONPath 表达式

```bash
# 获取所有节点 IP
kubectl get nodes -o jsonpath='{.items[*].status.addresses[?(@.type=="InternalIP")].address}'

# 获取所有 Secret 名称 (排除 service-account-token)
kubectl get secrets -A -o jsonpath='{range .items[?(@.type!="kubernetes.io/service-account-token")]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'

# 获取所有 Ready 节点
kubectl get nodes -o jsonpath='{range .items[?(@.status.conditions[?(@.type=="Ready")].status=="True")]}{.metadata.name}{"\n"}{end}'

# 获取 Pod 重启次数
kubectl get pods -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{range .status.containerStatuses[*]}{.restartCount}{" "}{end}{"\n"}{end}'

# 获取使用特定镜像的 Pod
kubectl get pods -A -o jsonpath='{range .items[?(@.spec.containers[*].image=="nginx:1.25")]}{.metadata.namespace}/{.metadata.name}{"\n"}{end}'
```

### 2.2 kubectl describe - 详细信息查看

```bash
# 基础语法
kubectl describe <resource> <name> [-n namespace]

# 查看 Pod 详情 (包含事件)
kubectl describe pod <pod-name>

# 查看节点详情
kubectl describe node <node-name>

# 查看 Service 详情
kubectl describe svc <service-name>

# 查看 PV/PVC 绑定关系
kubectl describe pv <pv-name>
kubectl describe pvc <pvc-name>

# 查看 Deployment 滚动更新状态
kubectl describe deploy <deployment-name>

# 查看 Ingress 规则和后端
kubectl describe ingress <ingress-name>
```

### 2.3 kubectl explain - API 文档查询

```bash
# 基础语法
kubectl explain <resource>[.field] [--api-version=<version>]

# 查看 Pod spec 字段
kubectl explain pod.spec

# 查看容器字段
kubectl explain pod.spec.containers

# 查看特定 API 版本
kubectl explain deployment --api-version=apps/v1

# 递归显示所有字段
kubectl explain pod.spec --recursive

# 查看 CRD 字段
kubectl explain prometheusrules.spec

# v1.30+ 支持 OpenAPI v3
kubectl explain pod.spec.containers --output=plaintext-openapiv2
```

### 2.4 kubectl api-resources - API 资源查询

```bash
# 列出所有 API 资源
kubectl api-resources

# 列出支持特定动作的资源
kubectl api-resources --verbs=list,get

# 列出 namespaced 资源
kubectl api-resources --namespaced=true

# 列出特定 API 组资源
kubectl api-resources --api-group=apps

# 按资源名排序
kubectl api-resources --sort-by=name

# 输出宽格式 (包含 SHORTNAMES、APIVERSION、NAMESPACED、KIND)
kubectl api-resources -o wide
```

### 2.5 kubectl events - 事件查询 (v1.26+ GA)

```bash
# 查看当前命名空间事件
kubectl events

# 查看所有命名空间事件
kubectl events -A

# 按时间排序
kubectl events --sort-by='.lastTimestamp'

# 只看警告事件
kubectl events --types=Warning

# 监视事件
kubectl events -w

# 查看特定资源的事件
kubectl events --for=pod/nginx

# 组合过滤
kubectl events -A --types=Warning --sort-by='.lastTimestamp' | head -50
```

---

## 3. 资源创建与管理

### 3.1 kubectl create - 命令式创建

#### 基础资源创建

| 命令 | 说明 | 生产环境注意事项 |
|:---|:---|:---|
| `kubectl create namespace <name>` | 创建命名空间 | 配合 ResourceQuota/LimitRange |
| `kubectl create deployment <name> --image=<image>` | 创建 Deployment | 建议用 YAML 定义完整配置 |
| `kubectl create service clusterip <name> --tcp=<port>:<targetPort>` | 创建 ClusterIP Service | 需指定正确的 selector |
| `kubectl create configmap <name> --from-file=<path>` | 创建 ConfigMap | 注意文件编码 |
| `kubectl create secret generic <name> --from-literal=<key>=<value>` | 创建 Secret | 建议用 External Secrets |
| `kubectl create serviceaccount <name>` | 创建 ServiceAccount | 配合 RBAC 使用 |
| `kubectl create job <name> --image=<image>` | 创建 Job | 设置合理的 backoffLimit |
| `kubectl create cronjob <name> --image=<image> --schedule=<cron>` | 创建 CronJob | 注意时区设置 |

#### ConfigMap 创建详解

```bash
# 从文件创建
kubectl create configmap nginx-conf --from-file=nginx.conf

# 从目录创建
kubectl create configmap app-config --from-file=./config/

# 从键值对创建
kubectl create configmap app-env \
  --from-literal=DB_HOST=mysql.default.svc \
  --from-literal=DB_PORT=3306

# 从 env 文件创建
kubectl create configmap app-env --from-env-file=.env

# 生成 YAML (不实际创建)
kubectl create configmap app-config \
  --from-literal=key=value \
  --dry-run=client -o yaml > configmap.yaml
```

#### Secret 创建详解

```bash
# 创建 generic Secret
kubectl create secret generic db-secret \
  --from-literal=username=admin \
  --from-literal=password='S3cur3P@ss!'

# 创建 TLS Secret
kubectl create secret tls my-tls-secret \
  --cert=path/to/tls.crt \
  --key=path/to/tls.key

# 创建 Docker Registry Secret
kubectl create secret docker-registry regcred \
  --docker-server=<registry-server> \
  --docker-username=<username> \
  --docker-password=<password> \
  --docker-email=<email>

# 创建 SSH 密钥 Secret
kubectl create secret generic ssh-key \
  --from-file=ssh-privatekey=~/.ssh/id_rsa \
  --from-file=ssh-publickey=~/.ssh/id_rsa.pub
```

#### Token 创建 (v1.24+)

```bash
# 创建 ServiceAccount Token (v1.24+ 推荐方式)
kubectl create token <service-account-name>

# 指定过期时间
kubectl create token <sa-name> --duration=24h

# 指定 audience
kubectl create token <sa-name> --audience=api

# 绑定到特定 Pod
kubectl create token <sa-name> --bound-object-kind=Pod --bound-object-name=<pod-name>
```

### 3.2 kubectl apply - 声明式管理

#### 基础用法

```bash
# 应用单个文件
kubectl apply -f deployment.yaml

# 应用目录下所有文件
kubectl apply -f ./manifests/

# 应用远程文件
kubectl apply -f https://raw.githubusercontent.com/kubernetes/dashboard/v2.7.0/aio/deploy/recommended.yaml

# 递归应用
kubectl apply -R -f ./manifests/

# 服务端 Apply (v1.22+ GA, v1.32+ 默认)
kubectl apply --server-side -f deployment.yaml

# 强制冲突处理
kubectl apply --server-side --force-conflicts -f deployment.yaml

# 查看差异但不应用
kubectl diff -f deployment.yaml

# 仅验证不应用
kubectl apply -f deployment.yaml --dry-run=server

# 记录变更原因
kubectl apply -f deployment.yaml --record  # 已废弃
# v1.24+ 使用 annotation
kubectl annotate deployment/nginx kubernetes.io/change-cause="Update nginx to 1.25"
```

#### 服务端 Apply (Server-Side Apply) 详解

```bash
# 服务端 Apply 优势:
# 1. 更精确的字段所有权管理
# 2. 支持多个管理器
# 3. 更好的冲突检测

# 指定字段管理器
kubectl apply --server-side --field-manager=my-controller -f deployment.yaml

# 查看字段所有权
kubectl get deployment nginx -o yaml | grep -A 20 'managedFields'

# 强制获取字段所有权
kubectl apply --server-side --force-conflicts -f deployment.yaml
```

### 3.3 kubectl delete - 资源删除

```bash
# 删除单个资源
kubectl delete pod nginx

# 删除多个资源
kubectl delete pod nginx1 nginx2

# 通过文件删除
kubectl delete -f deployment.yaml

# 通过标签删除
kubectl delete pods -l app=nginx

# 删除命名空间下所有 Pod
kubectl delete pods --all -n <namespace>

# 强制删除 (绕过优雅终止)
kubectl delete pod nginx --force --grace-period=0

# 级联删除策略 (v1.20+)
kubectl delete deployment nginx --cascade=foreground  # 等待所有依赖删除
kubectl delete deployment nginx --cascade=background  # 后台删除 (默认)
kubectl delete deployment nginx --cascade=orphan      # 孤儿依赖

# 删除并等待
kubectl delete pod nginx --wait=true

# 设置超时
kubectl delete pod nginx --timeout=60s

# 删除所有已完成的 Pod
kubectl delete pods --field-selector=status.phase==Succeeded -A

# 删除所有失败的 Pod
kubectl delete pods --field-selector=status.phase==Failed -A

# 删除 Evicted Pod
kubectl get pods -A | grep Evicted | awk '{print $2 " -n " $1}' | xargs -L1 kubectl delete pod
```

### 3.4 kubectl run - 快速创建 Pod

```bash
# 创建简单 Pod
kubectl run nginx --image=nginx:1.25

# 创建并暴露端口
kubectl run nginx --image=nginx:1.25 --port=80

# 创建临时调试 Pod
kubectl run debug --image=busybox --rm -it --restart=Never -- sh

# 使用自定义命令
kubectl run myapp --image=busybox --restart=Never -- sleep 3600

# 指定资源限制
kubectl run nginx --image=nginx:1.25 \
  --requests='cpu=100m,memory=128Mi' \
  --limits='cpu=200m,memory=256Mi'

# 指定环境变量
kubectl run myapp --image=myapp:latest \
  --env="DB_HOST=mysql" \
  --env="DB_PORT=3306"

# 指定标签
kubectl run nginx --image=nginx:1.25 --labels="app=nginx,env=prod"

# 生成 YAML
kubectl run nginx --image=nginx:1.25 --dry-run=client -o yaml > pod.yaml

# 覆盖 entrypoint
kubectl run debug --image=alpine --restart=Never --command -- tail -f /dev/null
```

### 3.5 kubectl expose - 服务暴露

```bash
# 暴露 Deployment 为 ClusterIP Service
kubectl expose deployment nginx --port=80 --target-port=80

# 暴露为 NodePort
kubectl expose deployment nginx --type=NodePort --port=80

# 暴露为 LoadBalancer
kubectl expose deployment nginx --type=LoadBalancer --port=80

# 指定服务名
kubectl expose deployment nginx --name=nginx-svc --port=80

# 暴露 Pod
kubectl expose pod nginx --port=80

# 指定 selector
kubectl expose deployment nginx --port=80 --selector='app=nginx,version=v1'

# 指定 cluster IP (不推荐)
kubectl expose deployment nginx --port=80 --cluster-ip=10.96.0.100

# 指定外部 IP
kubectl expose deployment nginx --port=80 --external-ip=192.168.1.100
```

---

## 4. Pod 调试与交互

### 4.1 kubectl exec - 容器命令执行

```bash
# 在 Pod 中执行命令
kubectl exec <pod-name> -- <command>

# 进入交互式 Shell
kubectl exec -it <pod-name> -- /bin/bash
kubectl exec -it <pod-name> -- /bin/sh

# 多容器 Pod 指定容器
kubectl exec -it <pod-name> -c <container-name> -- /bin/bash

# 执行复杂命令 (使用 sh -c)
kubectl exec <pod-name> -- sh -c 'cat /etc/hosts && echo "---" && cat /etc/resolv.conf'

# 查看环境变量
kubectl exec <pod-name> -- env

# 查看进程
kubectl exec <pod-name> -- ps aux

# 查看网络
kubectl exec <pod-name> -- netstat -tlnp

# 测试 DNS 解析
kubectl exec <pod-name> -- nslookup kubernetes.default

# 测试服务连通性
kubectl exec <pod-name> -- curl -s http://service-name:port/health

# 查看挂载点
kubectl exec <pod-name> -- df -h

# 批量执行
kubectl get pods -l app=nginx -o name | xargs -I {} kubectl exec {} -- nginx -v
```

### 4.2 kubectl logs - 日志查看

```bash
# 查看 Pod 日志
kubectl logs <pod-name>

# 查看指定容器日志
kubectl logs <pod-name> -c <container-name>

# 实时跟踪日志
kubectl logs -f <pod-name>

# 查看最近 N 行
kubectl logs --tail=100 <pod-name>

# 查看最近时间段
kubectl logs --since=1h <pod-name>
kubectl logs --since=30m <pod-name>
kubectl logs --since-time='2024-01-01T10:00:00Z' <pod-name>

# 查看前一个容器的日志 (重启后)
kubectl logs <pod-name> --previous

# 查看所有容器日志
kubectl logs <pod-name> --all-containers=true

# 查看 Init 容器日志
kubectl logs <pod-name> -c <init-container-name>

# 带时间戳
kubectl logs <pod-name> --timestamps=true

# 查看 Deployment 所有 Pod 日志
kubectl logs -l app=nginx --all-containers=true

# 限制输出字节数
kubectl logs <pod-name> --limit-bytes=1048576

# 组合使用
kubectl logs -f --tail=50 --timestamps <pod-name>

# 多 Pod 日志聚合 (需要 stern 工具)
# stern "nginx.*" --tail=10
```

### 4.3 kubectl cp - 文件复制

```bash
# 从 Pod 复制到本地
kubectl cp <namespace>/<pod-name>:<path> <local-path>
kubectl cp default/nginx:/etc/nginx/nginx.conf ./nginx.conf

# 从本地复制到 Pod
kubectl cp <local-path> <namespace>/<pod-name>:<path>
kubectl cp ./config.yaml default/nginx:/tmp/config.yaml

# 指定容器
kubectl cp <local-path> <pod-name>:<path> -c <container-name>

# 复制目录
kubectl cp default/nginx:/var/log/ ./logs/

# 注意事项:
# 1. 需要 Pod 中有 tar 命令
# 2. 大文件传输可能超时
# 3. 符号链接可能有问题
```

### 4.4 kubectl attach - 容器附加

```bash
# 附加到运行中的容器
kubectl attach <pod-name> -i

# 附加到指定容器
kubectl attach <pod-name> -c <container-name> -it

# 获取 TTY
kubectl attach <pod-name> -it
```

### 4.5 kubectl debug - 调试容器 (v1.25+ GA)

```bash
# 创建调试容器 (Ephemeral Container)
kubectl debug -it <pod-name> --image=busybox --target=<container-name>

# 使用调试 profile
kubectl debug -it <pod-name> --image=busybox --profile=general
kubectl debug -it <pod-name> --image=busybox --profile=baseline
kubectl debug -it <pod-name> --image=busybox --profile=restricted
kubectl debug -it <pod-name> --image=busybox --profile=netadmin
kubectl debug -it <pod-name> --image=busybox --profile=sysadmin

# 复制 Pod 进行调试 (修改命令)
kubectl debug <pod-name> -it --copy-to=debug-pod --container=app -- sh

# 复制 Pod 并修改镜像
kubectl debug <pod-name> -it --copy-to=debug-pod --set-image=*=busybox

# 共享进程命名空间
kubectl debug <pod-name> -it --image=busybox --share-processes

# 调试节点
kubectl debug node/<node-name> -it --image=ubuntu

# 常用调试镜像
# - busybox: 基础工具
# - nicolaka/netshoot: 网络调试
# - alpine: 轻量级
# - ubuntu: 完整工具链
# - gcr.io/kubernetes-e2e-test-images/jessie-dnsutils: DNS 调试
```

#### 调试 Profile 详解

| Profile | 用途 | 权限级别 |
|:---|:---|:---|
| `general` | 通用调试 | 标准权限 |
| `baseline` | 基线安全 | 符合 Pod Security Standards baseline |
| `restricted` | 严格安全 | 符合 Pod Security Standards restricted |
| `netadmin` | 网络调试 | NET_ADMIN capability |
| `sysadmin` | 系统调试 | 特权容器 |

---

## 5. 资源编辑与补丁

### 5.1 kubectl edit - 在线编辑

```bash
# 编辑资源
kubectl edit deployment nginx

# 指定编辑器
KUBE_EDITOR="vim" kubectl edit deployment nginx
KUBE_EDITOR="code --wait" kubectl edit deployment nginx

# 编辑特定命名空间资源
kubectl edit deployment nginx -n production

# 编辑子资源 (v1.28+)
kubectl edit deployment nginx --subresource=status

# 输出格式
kubectl edit deployment nginx -o json
```

### 5.2 kubectl patch - 资源补丁

#### 三种补丁类型

| 类型 | 说明 | 适用场景 |
|:---|:---|:---|
| `strategic` | 策略合并 (默认) | K8s 原生资源 |
| `merge` | JSON Merge Patch | 简单字段更新 |
| `json` | JSON Patch | 精确操作 (add/remove/replace) |

#### Strategic Merge Patch

```bash
# 更新镜像
kubectl patch deployment nginx -p '{"spec":{"template":{"spec":{"containers":[{"name":"nginx","image":"nginx:1.26"}]}}}}'

# 添加环境变量
kubectl patch deployment nginx -p '
spec:
  template:
    spec:
      containers:
      - name: nginx
        env:
        - name: NEW_VAR
          value: "new_value"
' --type=strategic

# 更新副本数
kubectl patch deployment nginx -p '{"spec":{"replicas":5}}'

# 添加标签
kubectl patch deployment nginx -p '{"metadata":{"labels":{"version":"v2"}}}'

# 添加注解
kubectl patch deployment nginx -p '{"metadata":{"annotations":{"description":"Updated deployment"}}}'

# 更新 NodeSelector
kubectl patch deployment nginx -p '{"spec":{"template":{"spec":{"nodeSelector":{"disktype":"ssd"}}}}}'
```

#### JSON Patch

```bash
# 替换镜像
kubectl patch deployment nginx --type='json' -p='[{"op":"replace","path":"/spec/template/spec/containers/0/image","value":"nginx:1.26"}]'

# 添加容器
kubectl patch deployment nginx --type='json' -p='[{"op":"add","path":"/spec/template/spec/containers/-","value":{"name":"sidecar","image":"busybox"}}]'

# 删除注解
kubectl patch deployment nginx --type='json' -p='[{"op":"remove","path":"/metadata/annotations/description"}]'

# 添加 init 容器
kubectl patch deployment nginx --type='json' -p='[{"op":"add","path":"/spec/template/spec/initContainers","value":[{"name":"init","image":"busybox","command":["sh","-c","echo init"]}]}]'
```

#### Merge Patch

```bash
# 简单字段更新
kubectl patch configmap mycm --type=merge -p '{"data":{"key":"new_value"}}'

# 删除字段 (设为 null)
kubectl patch deployment nginx --type=merge -p '{"spec":{"template":{"spec":{"nodeSelector":null}}}}'
```

#### 子资源补丁 (v1.28+)

```bash
# 更新 Deployment status
kubectl patch deployment nginx --subresource=status -p '{"status":{"readyReplicas":3}}'

# 更新 scale 子资源
kubectl patch deployment nginx --subresource=scale -p '{"spec":{"replicas":10}}'
```

### 5.3 kubectl replace - 资源替换

```bash
# 替换资源
kubectl replace -f deployment.yaml

# 强制替换 (先删后建)
kubectl replace --force -f deployment.yaml

# 从 stdin 替换
cat deployment.yaml | kubectl replace -f -
```

### 5.4 kubectl set - 快速设置

```bash
# 更新镜像
kubectl set image deployment/nginx nginx=nginx:1.26

# 更新多个容器镜像
kubectl set image deployment/app app=app:v2 sidecar=sidecar:v2

# 所有容器使用相同镜像
kubectl set image deployment/nginx *=nginx:1.26

# 设置环境变量
kubectl set env deployment/nginx DB_HOST=mysql
kubectl set env deployment/nginx DB_HOST=mysql DB_PORT=3306

# 从 ConfigMap 设置环境变量
kubectl set env deployment/nginx --from=configmap/app-config

# 从 Secret 设置环境变量
kubectl set env deployment/nginx --from=secret/app-secret

# 删除环境变量
kubectl set env deployment/nginx DB_HOST-

# 查看环境变量
kubectl set env deployment/nginx --list

# 设置资源限制
kubectl set resources deployment/nginx \
  --requests=cpu=100m,memory=128Mi \
  --limits=cpu=200m,memory=256Mi

# 设置 ServiceAccount
kubectl set serviceaccount deployment/nginx my-sa

# 设置 selector (慎用)
kubectl set selector service nginx 'app=nginx,version=v2'

# 设置 subject (RBAC)
kubectl set subject rolebinding admin --user=alice
```

### 5.5 kubectl label - 标签管理

```bash
# 添加标签
kubectl label pods nginx env=prod

# 更新标签
kubectl label pods nginx env=staging --overwrite

# 删除标签
kubectl label pods nginx env-

# 批量添加标签
kubectl label pods -l app=nginx tier=frontend

# 所有节点添加标签
kubectl label nodes --all zone=default

# 添加节点标签
kubectl label node node1 node-role.kubernetes.io/worker=

# 查看标签
kubectl get pods --show-labels

# 按标签筛选
kubectl get pods -l 'env in (prod,staging)'
kubectl get pods -l 'env notin (dev)'
kubectl get pods -l 'env'        # 有此标签
kubectl get pods -l '!env'       # 无此标签
```

### 5.6 kubectl annotate - 注解管理

```bash
# 添加注解
kubectl annotate pods nginx description="Web server"

# 更新注解
kubectl annotate pods nginx description="Updated web server" --overwrite

# 删除注解
kubectl annotate pods nginx description-

# 批量添加
kubectl annotate pods -l app=nginx team=platform

# 记录变更原因
kubectl annotate deployment nginx kubernetes.io/change-cause="Update to v1.26"
```

---

## 6. 部署管理

### 6.1 kubectl rollout - 滚动更新管理

#### 状态查看

```bash
# 查看部署状态
kubectl rollout status deployment/nginx

# 查看 DaemonSet 状态
kubectl rollout status daemonset/fluentd

# 查看 StatefulSet 状态
kubectl rollout status statefulset/mysql

# 带超时
kubectl rollout status deployment/nginx --timeout=5m
```

#### 历史与回滚

```bash
# 查看部署历史
kubectl rollout history deployment/nginx

# 查看特定版本详情
kubectl rollout history deployment/nginx --revision=2

# 回滚到上一版本
kubectl rollout undo deployment/nginx

# 回滚到特定版本
kubectl rollout undo deployment/nginx --to-revision=2

# 查看回滚预览
kubectl rollout undo deployment/nginx --dry-run=client -o yaml
```

#### 暂停与恢复

```bash
# 暂停部署 (批量修改时)
kubectl rollout pause deployment/nginx

# 执行多次修改...
kubectl set image deployment/nginx nginx=nginx:1.26
kubectl set env deployment/nginx LOG_LEVEL=debug
kubectl set resources deployment/nginx --limits=cpu=500m

# 恢复部署 (触发一次滚动更新)
kubectl rollout resume deployment/nginx
```

#### 重启

```bash
# 滚动重启 (v1.15+)
kubectl rollout restart deployment/nginx

# 重启 DaemonSet
kubectl rollout restart daemonset/fluentd

# 重启 StatefulSet
kubectl rollout restart statefulset/mysql

# 批量重启
kubectl rollout restart deployment -l app=myapp
```

### 6.2 kubectl scale - 扩缩容

```bash
# 扩展副本数
kubectl scale deployment nginx --replicas=5

# 缩容
kubectl scale deployment nginx --replicas=1

# 条件扩缩 (当前副本数匹配时)
kubectl scale deployment nginx --current-replicas=3 --replicas=5

# 批量扩缩
kubectl scale deployment nginx1 nginx2 nginx3 --replicas=3

# 按标签扩缩
kubectl scale deployment -l app=web --replicas=5

# 扩缩 StatefulSet
kubectl scale statefulset mysql --replicas=3

# 扩缩 ReplicaSet
kubectl scale rs nginx-abc123 --replicas=5
```

### 6.3 kubectl autoscale - 自动扩缩容

```bash
# 创建 HPA
kubectl autoscale deployment nginx --min=2 --max=10 --cpu-percent=80

# 查看 HPA
kubectl get hpa

# 查看 HPA 详情
kubectl describe hpa nginx

# 删除 HPA
kubectl delete hpa nginx

# 生成 HPA YAML
kubectl autoscale deployment nginx --min=2 --max=10 --cpu-percent=80 \
  --dry-run=client -o yaml
```

#### HPA v2 YAML 示例

```yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: nginx-hpa
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: nginx
  minReplicas: 2
  maxReplicas: 10
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 80
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 10
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
      - type: Percent
        value: 100
        periodSeconds: 15
      - type: Pods
        value: 4
        periodSeconds: 15
      selectPolicy: Max
```

---

## 7. 集群管理

### 7.1 kubectl cluster-info - 集群信息

```bash
# 显示集群信息
kubectl cluster-info

# 显示详细信息
kubectl cluster-info dump

# 导出到文件
kubectl cluster-info dump --output-directory=/path/to/dump/

# 指定命名空间
kubectl cluster-info dump --namespaces=kube-system,default
```

### 7.2 kubectl top - 资源使用情况

```bash
# 查看节点资源使用
kubectl top nodes

# 查看 Pod 资源使用
kubectl top pods

# 所有命名空间
kubectl top pods -A

# 按 CPU 排序
kubectl top pods --sort-by=cpu

# 按内存排序
kubectl top pods --sort-by=memory

# 查看容器级别
kubectl top pods --containers

# 指定标签
kubectl top pods -l app=nginx
```

### 7.3 kubectl cordon/uncordon - 节点调度控制

```bash
# 标记节点不可调度
kubectl cordon <node-name>

# 取消不可调度标记
kubectl uncordon <node-name>

# 批量操作
kubectl cordon node1 node2 node3

# 查看节点状态
kubectl get nodes
# 输出: node1   Ready,SchedulingDisabled   ...
```

### 7.4 kubectl drain - 节点排空

```bash
# 排空节点 (基础)
kubectl drain <node-name>

# 忽略 DaemonSet (通常需要)
kubectl drain <node-name> --ignore-daemonsets

# 强制删除本地数据 Pod
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 强制删除无控制器的 Pod
kubectl drain <node-name> --force

# 设置优雅终止时间
kubectl drain <node-name> --grace-period=60

# 设置超时
kubectl drain <node-name> --timeout=5m

# 忽略错误继续
kubectl drain <node-name> --ignore-daemonsets --force --delete-emptydir-data

# 生产环境推荐组合
kubectl drain <node-name> \
  --ignore-daemonsets \
  --delete-emptydir-data \
  --force \
  --grace-period=60 \
  --timeout=5m

# 排空后恢复调度
kubectl uncordon <node-name>
```

### 7.5 kubectl taint - 节点污点管理

```bash
# 添加污点
kubectl taint nodes <node-name> key=value:NoSchedule

# 常用效果
# NoSchedule: 不调度新 Pod
# PreferNoSchedule: 尽量不调度
# NoExecute: 驱逐现有 Pod + 不调度新 Pod

# 添加 NoExecute 污点
kubectl taint nodes <node-name> key=value:NoExecute

# 删除污点
kubectl taint nodes <node-name> key:NoSchedule-
kubectl taint nodes <node-name> key-  # 删除所有效果

# 生产环境常用污点
# 专用节点
kubectl taint nodes <node-name> dedicated=gpu:NoSchedule

# 维护中
kubectl taint nodes <node-name> node.kubernetes.io/maintenance:NoSchedule

# 查看节点污点
kubectl describe node <node-name> | grep Taints
```

---

## 8. 配置与上下文

### 8.1 kubeconfig 文件结构

```yaml
apiVersion: v1
kind: Config
preferences: {}

# 集群定义
clusters:
- name: prod-cluster
  cluster:
    server: https://k8s-api.prod.example.com:6443
    certificate-authority-data: <base64-encoded-ca-cert>
    # 或使用文件路径
    # certificate-authority: /path/to/ca.crt
- name: dev-cluster
  cluster:
    server: https://k8s-api.dev.example.com:6443
    certificate-authority-data: <base64-encoded-ca-cert>

# 用户凭证
users:
- name: prod-admin
  user:
    client-certificate-data: <base64-encoded-cert>
    client-key-data: <base64-encoded-key>
    # 或使用文件路径
    # client-certificate: /path/to/client.crt
    # client-key: /path/to/client.key
- name: dev-user
  user:
    token: <bearer-token>
    # 或使用 exec 插件
    # exec:
    #   apiVersion: client.authentication.k8s.io/v1beta1
    #   command: aws
    #   args:
    #   - eks
    #   - get-token
    #   - --cluster-name
    #   - my-cluster

# 上下文定义
contexts:
- name: prod-admin@prod
  context:
    cluster: prod-cluster
    user: prod-admin
    namespace: default
- name: dev-user@dev
  context:
    cluster: dev-cluster
    user: dev-user
    namespace: development

# 当前上下文
current-context: prod-admin@prod
```

### 8.2 kubectl config - 配置管理

```bash
# 查看当前配置
kubectl config view

# 查看合并后的配置
kubectl config view --flatten

# 查看最小化配置
kubectl config view --minify

# 查看当前上下文
kubectl config current-context

# 列出所有上下文
kubectl config get-contexts

# 切换上下文
kubectl config use-context dev-user@dev

# 设置默认命名空间
kubectl config set-context --current --namespace=production

# 创建新上下文
kubectl config set-context new-context \
  --cluster=prod-cluster \
  --user=prod-admin \
  --namespace=production

# 设置集群
kubectl config set-cluster new-cluster \
  --server=https://k8s-api.example.com:6443 \
  --certificate-authority=/path/to/ca.crt

# 设置用户凭证
kubectl config set-credentials new-user \
  --client-certificate=/path/to/client.crt \
  --client-key=/path/to/client.key

# 设置用户 Token
kubectl config set-credentials new-user --token=<token>

# 删除上下文
kubectl config delete-context dev-user@dev

# 删除集群
kubectl config delete-cluster dev-cluster

# 删除用户
kubectl config delete-user dev-user

# 使用多个 kubeconfig 文件
export KUBECONFIG=~/.kube/config:~/.kube/config-prod:~/.kube/config-dev
kubectl config view --flatten > ~/.kube/config-merged

# 指定配置文件
kubectl --kubeconfig=/path/to/config get pods
```

### 8.3 生产环境配置最佳实践

```bash
# 1. 使用独立的 kubeconfig 文件
mkdir -p ~/.kube/configs
# 每个集群一个文件

# 2. 设置别名快速切换
alias kprod='kubectl --kubeconfig=~/.kube/configs/prod.yaml'
alias kdev='kubectl --kubeconfig=~/.kube/configs/dev.yaml'

# 3. 使用 kubectx 和 kubens (推荐)
# brew install kubectx
kubectx prod-cluster    # 切换集群
kubens production       # 切换命名空间

# 4. Shell 提示符显示当前上下文
# 在 .bashrc 或 .zshrc 中添加:
# PS1='$(kubectl config current-context) $ '
# 或使用 kube-ps1

# 5. 防止误操作生产环境
# 使用 kubectl-safe 插件或设置提示
```

---

## 9. 高级调试

### 9.1 kubectl port-forward - 端口转发

```bash
# 转发 Pod 端口
kubectl port-forward pod/nginx 8080:80

# 转发 Service 端口
kubectl port-forward svc/nginx 8080:80

# 转发 Deployment
kubectl port-forward deployment/nginx 8080:80

# 监听所有接口
kubectl port-forward --address 0.0.0.0 pod/nginx 8080:80

# 后台运行
kubectl port-forward pod/nginx 8080:80 &

# 多端口转发
kubectl port-forward pod/nginx 8080:80 8443:443

# 随机本地端口
kubectl port-forward pod/nginx :80
```

### 9.2 kubectl proxy - API 代理

```bash
# 启动 API 代理
kubectl proxy

# 指定端口
kubectl proxy --port=8001

# 监听所有接口
kubectl proxy --address=0.0.0.0 --accept-hosts='.*'

# 后台运行
kubectl proxy &

# 访问 API
curl http://localhost:8001/api/v1/namespaces/default/pods

# 访问服务
curl http://localhost:8001/api/v1/namespaces/default/services/nginx/proxy/
```

### 9.3 kubectl wait - 等待条件

```bash
# 等待 Pod Ready
kubectl wait --for=condition=Ready pod/nginx --timeout=60s

# 等待所有 Pod Ready
kubectl wait --for=condition=Ready pod --all --timeout=120s

# 等待 Deployment 可用
kubectl wait --for=condition=Available deployment/nginx --timeout=120s

# 等待 Job 完成
kubectl wait --for=condition=Complete job/myjob --timeout=300s

# 等待删除完成
kubectl wait --for=delete pod/nginx --timeout=60s

# 使用标签选择
kubectl wait --for=condition=Ready pod -l app=nginx --timeout=60s

# 等待 Node Ready
kubectl wait --for=condition=Ready node --all --timeout=300s

# 等待 CRD 建立
kubectl wait --for=condition=Established crd/myresources.example.com

# 使用 JSONPath 条件 (v1.23+)
kubectl wait --for=jsonpath='{.status.phase}'=Running pod/nginx
```

### 9.4 kubectl debug 高级用法

```bash
# 创建节点调试 Pod
kubectl debug node/<node-name> -it --image=ubuntu

# 调试 Pod 网络
kubectl debug -it <pod-name> --image=nicolaka/netshoot --target=<container>

# 使用 chroot 访问节点文件系统
kubectl debug node/<node-name> -it --image=ubuntu
# 在 Pod 中: chroot /host

# 复制并修改 Pod 进行调试
kubectl debug <pod-name> -it --copy-to=debug-pod \
  --container=app \
  --image=busybox \
  -- sh

# 共享进程命名空间查看进程
kubectl debug <pod-name> -it --image=busybox --share-processes
# 在 Pod 中: ps aux
```

---

## 10. 认证与授权

### 10.1 kubectl auth - 认证授权检查

```bash
# 检查是否有权限
kubectl auth can-i create pods
kubectl auth can-i delete deployments
kubectl auth can-i '*' '*'  # 是否是超级管理员

# 指定命名空间
kubectl auth can-i create pods -n production

# 以其他用户身份检查
kubectl auth can-i create pods --as=alice
kubectl auth can-i create pods --as=system:serviceaccount:default:mysa

# 以组身份检查
kubectl auth can-i create pods --as-group=developers

# 列出用户所有权限
kubectl auth can-i --list
kubectl auth can-i --list --namespace=production

# 查看当前身份 (v1.27+ Beta, v1.30+ GA)
kubectl auth whoami

# 协调 RBAC 配置
kubectl auth reconcile -f rbac.yaml

# 检查特定资源
kubectl auth can-i get pods/nginx
kubectl auth can-i get pods --subresource=logs
```

### 10.2 kubectl certificate - 证书管理

```bash
# 批准 CSR
kubectl certificate approve <csr-name>

# 拒绝 CSR
kubectl certificate deny <csr-name>

# 查看 CSR
kubectl get csr

# 查看 CSR 详情
kubectl describe csr <csr-name>

# 创建 CSR (示例)
cat <<EOF | kubectl apply -f -
apiVersion: certificates.k8s.io/v1
kind: CertificateSigningRequest
metadata:
  name: myuser
spec:
  request: $(cat myuser.csr | base64 | tr -d '\n')
  signerName: kubernetes.io/kube-apiserver-client
  usages:
  - client auth
EOF

# 获取已签名证书
kubectl get csr myuser -o jsonpath='{.status.certificate}' | base64 -d > myuser.crt
```

### 10.3 RBAC 资源管理

```bash
# 创建 Role
kubectl create role pod-reader \
  --verb=get,list,watch \
  --resource=pods

# 创建 ClusterRole
kubectl create clusterrole pod-reader \
  --verb=get,list,watch \
  --resource=pods

# 创建 RoleBinding
kubectl create rolebinding pod-reader-binding \
  --role=pod-reader \
  --user=alice

# 创建 ClusterRoleBinding
kubectl create clusterrolebinding pod-reader-binding \
  --clusterrole=pod-reader \
  --user=alice

# 绑定到 ServiceAccount
kubectl create rolebinding myapp-binding \
  --clusterrole=view \
  --serviceaccount=default:myapp

# 绑定到组
kubectl create clusterrolebinding dev-binding \
  --clusterrole=edit \
  --group=developers

# 查看 Role
kubectl get roles
kubectl describe role pod-reader

# 查看 RoleBinding
kubectl get rolebindings
kubectl describe rolebinding pod-reader-binding
```

---

## 11. 插件与扩展

### 11.1 kubectl plugin - 插件机制

```bash
# 列出已安装插件
kubectl plugin list

# 插件搜索路径
# $PATH 中的 kubectl-<plugin-name> 可执行文件

# 运行插件
kubectl <plugin-name>

# 创建简单插件示例
cat > /usr/local/bin/kubectl-hello << 'EOF'
#!/bin/bash
echo "Hello, kubectl plugin!"
EOF
chmod +x /usr/local/bin/kubectl-hello

# 验证
kubectl hello
```

### 11.2 Krew - 插件管理器

```bash
# 安装 Krew (Linux/macOS)
(
  set -x; cd "$(mktemp -d)" &&
  OS="$(uname | tr '[:upper:]' '[:lower:]')" &&
  ARCH="$(uname -m | sed -e 's/x86_64/amd64/' -e 's/\(arm\)\(64\)\?.*/\1\2/' -e 's/aarch64$/arm64/')" &&
  KREW="krew-${OS}_${ARCH}" &&
  curl -fsSLO "https://github.com/kubernetes-sigs/krew/releases/latest/download/${KREW}.tar.gz" &&
  tar zxvf "${KREW}.tar.gz" &&
  ./"${KREW}" install krew
)

# 添加到 PATH
export PATH="${KREW_ROOT:-$HOME/.krew}/bin:$PATH"

# 更新索引
kubectl krew update

# 搜索插件
kubectl krew search

# 安装插件
kubectl krew install <plugin-name>

# 升级插件
kubectl krew upgrade <plugin-name>

# 列出已安装
kubectl krew list

# 卸载插件
kubectl krew uninstall <plugin-name>
```

### 11.3 推荐生产插件

| 插件 | 说明 | 安装命令 |
|:---|:---|:---|
| `ctx` | 快速切换上下文 | `kubectl krew install ctx` |
| `ns` | 快速切换命名空间 | `kubectl krew install ns` |
| `neat` | 清理 YAML 输出 | `kubectl krew install neat` |
| `tree` | 资源层级树视图 | `kubectl krew install tree` |
| `images` | 显示集群镜像 | `kubectl krew install images` |
| `access-matrix` | RBAC 权限矩阵 | `kubectl krew install access-matrix` |
| `whoami` | 显示当前用户 | `kubectl krew install whoami` |
| `sniff` | Pod 网络抓包 | `kubectl krew install sniff` |
| `node-shell` | 节点 Shell 访问 | `kubectl krew install node-shell` |
| `pod-logs` | 多 Pod 日志查看 | `kubectl krew install pod-logs` |
| `resource-capacity` | 集群资源容量 | `kubectl krew install resource-capacity` |
| `view-secret` | 解码 Secret | `kubectl krew install view-secret` |
| `view-cert` | 查看证书信息 | `kubectl krew install view-cert` |
| `deprecations` | 废弃 API 检查 | `kubectl krew install deprecations` |
| `score` | 资源配置评分 | `kubectl krew install score` |

```bash
# 使用示例
kubectl ctx           # 列出/切换上下文
kubectl ns            # 列出/切换命名空间
kubectl tree deploy nginx  # 显示 Deployment 资源树
kubectl images -A     # 显示所有镜像
kubectl access-matrix # RBAC 权限矩阵
kubectl resource-capacity  # 集群资源容量
kubectl view-secret <secret-name> -a  # 解码显示 Secret
kubectl deprecations  # 检查废弃 API
```

---

## 12. 性能优化与最佳实践

### 12.1 kubectl 性能优化

| 优化项 | 说明 | 配置方法 |
|:---|:---|:---|
| **启用压缩** | 减少网络传输 | `kubectl get pods --compress=true` |
| **限制输出** | 减少返回数据量 | `--field-selector`, `--label-selector` |
| **使用 --chunk-size** | 分页获取大量资源 | `kubectl get pods --chunk-size=500` |
| **缓存发现信息** | 减少 API 调用 | 设置 `--cache-dir` |
| **使用 name 输出** | 最小化输出 | `-o name` |
| **避免 -o yaml/json** | 大量资源时禁用 | 用 jsonpath 提取需要的字段 |

### 12.2 生产环境别名配置

```bash
# ~/.bashrc 或 ~/.zshrc

# 基础别名
alias k='kubectl'
alias kg='kubectl get'
alias kd='kubectl describe'
alias kdel='kubectl delete'
alias kl='kubectl logs'
alias ke='kubectl exec -it'
alias ka='kubectl apply -f'

# 资源查看
alias kgp='kubectl get pods'
alias kgpa='kubectl get pods -A'
alias kgpw='kubectl get pods -o wide'
alias kgd='kubectl get deployments'
alias kgs='kubectl get services'
alias kgn='kubectl get nodes'
alias kgns='kubectl get namespaces'
alias kgpv='kubectl get pv'
alias kgpvc='kubectl get pvc'
alias kgcm='kubectl get configmaps'
alias kgsec='kubectl get secrets'
alias kging='kubectl get ingress'

# 描述资源
alias kdp='kubectl describe pod'
alias kdd='kubectl describe deployment'
alias kds='kubectl describe service'
alias kdn='kubectl describe node'

# 日志
alias klf='kubectl logs -f'
alias klt='kubectl logs --tail=100'

# 执行
alias kexec='kubectl exec -it'

# 上下文
alias kctx='kubectl config current-context'
alias kns='kubectl config view --minify --output "jsonpath={..namespace}"'

# 快速操作
alias krun='kubectl run --rm -it --restart=Never --image=busybox debug -- sh'
alias kdebug='kubectl run --rm -it --restart=Never --image=nicolaka/netshoot debug -- sh'

# 清理
alias kdelpf='kubectl delete pods --field-selector=status.phase==Failed -A'
alias kdelpe='kubectl get pods -A | grep Evicted | awk '\''{print "kubectl delete pod " $2 " -n " $1}'\'' | sh'
```

### 12.3 kubectl 自动补全

```bash
# Bash
source <(kubectl completion bash)
echo 'source <(kubectl completion bash)' >> ~/.bashrc

# 使用别名
echo 'alias k=kubectl' >> ~/.bashrc
echo 'complete -o default -F __start_kubectl k' >> ~/.bashrc

# Zsh
source <(kubectl completion zsh)
echo 'source <(kubectl completion zsh)' >> ~/.zshrc

# PowerShell
kubectl completion powershell | Out-String | Invoke-Expression
kubectl completion powershell >> $PROFILE

# Fish
kubectl completion fish | source
kubectl completion fish > ~/.config/fish/completions/kubectl.fish
```

### 12.4 安全最佳实践

| 实践 | 说明 | 实施方法 |
|:---|:---|:---|
| **使用独立 kubeconfig** | 隔离凭证 | 每集群独立文件 |
| **限制 admin 访问** | 日常使用受限账号 | 创建只读账号 |
| **启用审计** | 追踪操作记录 | 配置 audit policy |
| **上下文确认** | 防止误操作 | 使用 kube-ps1 |
| **禁用 delete all** | 防止批量删除 | admission webhook |
| **使用 dry-run** | 预览变更 | `--dry-run=server` |
| **版本锁定** | 一致性操作 | 固定 kubectl 版本 |

---

## 13. 生产环境运维脚本

### 13.1 集群巡检脚本

```bash
#!/bin/bash
# cluster-health-check.sh - 集群健康检查脚本

set -euo pipefail

echo "=========================================="
echo "Kubernetes 集群健康检查报告"
echo "时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=========================================="

echo -e "\n[1] 集群基本信息"
kubectl cluster-info | head -2
echo "Kubernetes 版本: $(kubectl version --short 2>/dev/null | grep Server || kubectl version -o json | jq -r '.serverVersion.gitVersion')"
echo "节点数量: $(kubectl get nodes --no-headers | wc -l)"

echo -e "\n[2] 节点状态"
kubectl get nodes -o wide
echo ""
kubectl top nodes 2>/dev/null || echo "Metrics server 未安装"

echo -e "\n[3] 非 Running Pod"
NOT_RUNNING=$(kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded --no-headers 2>/dev/null | wc -l)
if [ "$NOT_RUNNING" -gt 0 ]; then
    echo "发现 $NOT_RUNNING 个非正常 Pod:"
    kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded
else
    echo "所有 Pod 状态正常"
fi

echo -e "\n[4] Pod 重启统计 (重启次数 > 5)"
kubectl get pods -A -o json | jq -r '
  .items[] | 
  select(.status.containerStatuses != null) |
  .status.containerStatuses[] | 
  select(.restartCount > 5) |
  "\(.name) in \(.state | keys[0]) - Restarts: \(.restartCount)"
' | head -20

echo -e "\n[5] 资源配额使用情况"
kubectl get resourcequota -A 2>/dev/null || echo "未配置 ResourceQuota"

echo -e "\n[6] PVC 状态"
PENDING_PVC=$(kubectl get pvc -A --field-selector=status.phase!=Bound --no-headers 2>/dev/null | wc -l)
if [ "$PENDING_PVC" -gt 0 ]; then
    echo "发现 $PENDING_PVC 个未绑定 PVC:"
    kubectl get pvc -A --field-selector=status.phase!=Bound
else
    echo "所有 PVC 已绑定"
fi

echo -e "\n[7] 最近 Warning 事件 (30分钟内)"
kubectl get events -A --field-selector=type=Warning --sort-by='.lastTimestamp' 2>/dev/null | tail -20

echo -e "\n[8] 控制平面组件状态"
kubectl get pods -n kube-system -l tier=control-plane -o wide 2>/dev/null || \
kubectl get pods -n kube-system | grep -E 'kube-apiserver|kube-controller|kube-scheduler|etcd'

echo -e "\n[9] 证书过期检查"
kubectl get csr 2>/dev/null | head -10

echo -e "\n=========================================="
echo "检查完成"
echo "=========================================="
```

### 13.2 Pod 批量诊断脚本

```bash
#!/bin/bash
# pod-diagnose.sh - Pod 诊断脚本

POD_NAME=${1:-}
NAMESPACE=${2:-default}

if [ -z "$POD_NAME" ]; then
    echo "Usage: $0 <pod-name> [namespace]"
    exit 1
fi

echo "=========================================="
echo "Pod 诊断报告: $POD_NAME"
echo "命名空间: $NAMESPACE"
echo "=========================================="

echo -e "\n[1] Pod 基本信息"
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o wide

echo -e "\n[2] Pod 详细状态"
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='
状态: {.status.phase}
节点: {.spec.nodeName}
IP: {.status.podIP}
QoS: {.status.qosClass}
启动时间: {.status.startTime}
'
echo ""

echo -e "\n[3] 容器状态"
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='
{range .status.containerStatuses[*]}
容器: {.name}
  镜像: {.image}
  就绪: {.ready}
  重启次数: {.restartCount}
  状态: {.state}
{end}
'

echo -e "\n[4] 资源配置"
kubectl get pod "$POD_NAME" -n "$NAMESPACE" -o jsonpath='
{range .spec.containers[*]}
容器: {.name}
  Requests: CPU={.resources.requests.cpu}, Memory={.resources.requests.memory}
  Limits: CPU={.resources.limits.cpu}, Memory={.resources.limits.memory}
{end}
'

echo -e "\n[5] 事件记录"
kubectl get events -n "$NAMESPACE" --field-selector=involvedObject.name="$POD_NAME" --sort-by='.lastTimestamp'

echo -e "\n[6] 日志 (最近 50 行)"
kubectl logs "$POD_NAME" -n "$NAMESPACE" --tail=50 2>/dev/null || echo "无法获取日志"

echo -e "\n[7] 描述信息"
kubectl describe pod "$POD_NAME" -n "$NAMESPACE"
```

### 13.3 资源清理脚本

```bash
#!/bin/bash
# cleanup-resources.sh - 资源清理脚本

echo "=========================================="
echo "Kubernetes 资源清理"
echo "=========================================="

# 清理已完成的 Job
echo -e "\n[1] 清理已完成的 Job..."
kubectl delete jobs --field-selector=status.successful=1 -A 2>/dev/null || echo "无已完成 Job"

# 清理 Failed Pod
echo -e "\n[2] 清理 Failed Pod..."
kubectl delete pods --field-selector=status.phase==Failed -A 2>/dev/null || echo "无 Failed Pod"

# 清理 Evicted Pod
echo -e "\n[3] 清理 Evicted Pod..."
kubectl get pods -A | grep Evicted | awk '{print $2 " -n " $1}' | while read line; do
    kubectl delete pod $line 2>/dev/null
done
echo "Evicted Pod 清理完成"

# 清理已完成的 Pod (Succeeded)
echo -e "\n[4] 清理已完成的 Pod..."
kubectl delete pods --field-selector=status.phase==Succeeded -A 2>/dev/null || echo "无已完成 Pod"

echo -e "\n=========================================="
echo "清理完成"
echo "=========================================="
```

### 13.4 备份脚本

```bash
#!/bin/bash
# backup-resources.sh - 资源备份脚本

BACKUP_DIR=${1:-./k8s-backup-$(date +%Y%m%d-%H%M%S)}
NAMESPACES=${2:-$(kubectl get ns -o jsonpath='{.items[*].metadata.name}')}

mkdir -p "$BACKUP_DIR"

echo "=========================================="
echo "Kubernetes 资源备份"
echo "备份目录: $BACKUP_DIR"
echo "=========================================="

# 备份集群级资源
echo -e "\n[1] 备份集群级资源..."
mkdir -p "$BACKUP_DIR/cluster"

for resource in nodes namespaces persistentvolumes storageclasses clusterroles clusterrolebindings; do
    echo "  - $resource"
    kubectl get "$resource" -o yaml > "$BACKUP_DIR/cluster/$resource.yaml" 2>/dev/null || true
done

# 备份命名空间级资源
echo -e "\n[2] 备份命名空间级资源..."

for ns in $NAMESPACES; do
    # 跳过系统命名空间
    if [[ "$ns" == "kube-system" || "$ns" == "kube-public" || "$ns" == "kube-node-lease" ]]; then
        continue
    fi
    
    echo "  处理命名空间: $ns"
    mkdir -p "$BACKUP_DIR/namespaces/$ns"
    
    for resource in deployments statefulsets daemonsets services configmaps secrets serviceaccounts roles rolebindings ingresses networkpolicies persistentvolumeclaims horizontalpodautoscalers poddisruptionbudgets; do
        kubectl get "$resource" -n "$ns" -o yaml > "$BACKUP_DIR/namespaces/$ns/$resource.yaml" 2>/dev/null || true
    done
done

echo -e "\n=========================================="
echo "备份完成"
echo "文件数: $(find "$BACKUP_DIR" -name "*.yaml" | wc -l)"
echo "总大小: $(du -sh "$BACKUP_DIR" | cut -f1)"
echo "=========================================="
```

---

## 14. 故障排查速查表

### 14.1 Pod 故障排查

| 状态 | 可能原因 | 排查命令 |
|:---|:---|:---|
| **Pending** | 资源不足/调度失败 | `kubectl describe pod <name>` 查看 Events |
| **ImagePullBackOff** | 镜像拉取失败 | 检查镜像名称、仓库凭证 |
| **CrashLoopBackOff** | 容器启动失败 | `kubectl logs <pod> --previous` |
| **OOMKilled** | 内存不足 | 增加内存 limit，检查内存泄漏 |
| **Evicted** | 节点资源不足 | 检查节点资源，调整 QoS |
| **Terminating** | 删除卡住 | 检查 finalizers，强制删除 |
| **Init:Error** | Init 容器失败 | `kubectl logs <pod> -c <init-container>` |
| **ContainerCreating** | Volume 挂载问题 | 检查 PVC 状态，CSI 日志 |

### 14.2 常用排查命令速查

```bash
# Pod 相关
kubectl get pods -A -o wide                    # 查看所有 Pod
kubectl describe pod <name>                    # 查看 Pod 详情
kubectl logs <pod> --previous                  # 查看上一个容器日志
kubectl logs <pod> -c <container>              # 指定容器日志
kubectl exec -it <pod> -- sh                   # 进入 Pod
kubectl debug -it <pod> --image=busybox        # 调试 Pod

# 节点相关
kubectl get nodes -o wide                      # 查看节点状态
kubectl describe node <name>                   # 查看节点详情
kubectl top nodes                              # 查看节点资源使用
kubectl debug node/<name> -it --image=ubuntu   # 调试节点

# 网络相关
kubectl get svc,ep -A                          # 查看服务和端点
kubectl get ingress -A                         # 查看 Ingress
kubectl exec -it <pod> -- curl <service>       # 测试服务连通性
kubectl exec -it <pod> -- nslookup <service>   # 测试 DNS 解析

# 存储相关
kubectl get pv,pvc -A                          # 查看存储卷
kubectl describe pvc <name>                    # 查看 PVC 详情

# 事件相关
kubectl get events -A --sort-by='.lastTimestamp'           # 所有事件
kubectl get events --types=Warning -A                       # 告警事件
kubectl get events --field-selector=involvedObject.name=<pod>  # 指定资源事件

# RBAC 相关
kubectl auth can-i --list                      # 列出当前用户权限
kubectl auth can-i <verb> <resource> --as=<user>  # 检查用户权限
```

### 14.3 紧急操作速查

```bash
# 紧急回滚
kubectl rollout undo deployment/<name>

# 紧急扩容
kubectl scale deployment/<name> --replicas=10

# 紧急停止 (缩容到 0)
kubectl scale deployment/<name> --replicas=0

# 强制删除卡住的 Pod
kubectl delete pod <name> --force --grace-period=0

# 紧急排空节点
kubectl drain <node> --ignore-daemonsets --delete-emptydir-data --force

# 紧急隔离节点
kubectl cordon <node>

# 查看 API Server 健康
kubectl get --raw='/healthz?verbose'

# 查看 etcd 健康 (需要访问权限)
kubectl get --raw='/healthz/etcd'
```

---

## 附录 A: kubectl 命令完整列表

### 基础命令

| 命令 | 说明 | 常用选项 |
|:---|:---|:---|
| `create` | 命令式创建资源 | `-f`, `--dry-run` |
| `expose` | 暴露服务 | `--port`, `--type` |
| `run` | 运行 Pod | `--image`, `--rm` |
| `set` | 设置资源属性 | `image`, `env`, `resources` |

### 应用管理

| 命令 | 说明 | 常用选项 |
|:---|:---|:---|
| `apply` | 声明式管理 | `-f`, `--server-side` |
| `get` | 获取资源 | `-o`, `-l`, `-A` |
| `describe` | 详细描述 | - |
| `edit` | 在线编辑 | `-o` |
| `delete` | 删除资源 | `--force`, `--cascade` |

### 部署管理

| 命令 | 说明 | 常用选项 |
|:---|:---|:---|
| `rollout` | 滚动更新管理 | `status`, `undo`, `restart` |
| `scale` | 扩缩容 | `--replicas` |
| `autoscale` | 自动扩缩 | `--min`, `--max`, `--cpu-percent` |

### 集群管理

| 命令 | 说明 | 常用选项 |
|:---|:---|:---|
| `cluster-info` | 集群信息 | `dump` |
| `top` | 资源使用情况 | `nodes`, `pods` |
| `cordon` | 标记不可调度 | - |
| `uncordon` | 取消不可调度 | - |
| `drain` | 排空节点 | `--ignore-daemonsets` |
| `taint` | 管理污点 | - |

### 调试诊断

| 命令 | 说明 | 常用选项 |
|:---|:---|:---|
| `logs` | 查看日志 | `-f`, `--tail`, `--previous` |
| `exec` | 执行命令 | `-it`, `-c` |
| `attach` | 附加到容器 | `-it` |
| `cp` | 复制文件 | - |
| `debug` | 调试容器 | `--image`, `--profile` |
| `port-forward` | 端口转发 | - |
| `proxy` | API 代理 | `--port` |

### 高级命令

| 命令 | 说明 | 常用选项 |
|:---|:---|:---|
| `diff` | 比较差异 | `-f` |
| `patch` | 补丁更新 | `--type` |
| `replace` | 替换资源 | `--force` |
| `wait` | 等待条件 | `--for`, `--timeout` |
| `label` | 管理标签 | `--overwrite` |
| `annotate` | 管理注解 | `--overwrite` |

### 配置管理

| 命令 | 说明 | 常用选项 |
|:---|:---|:---|
| `config` | kubeconfig 管理 | `view`, `use-context`, `set-context` |
| `api-resources` | 列出 API 资源 | `--verbs`, `--namespaced` |
| `api-versions` | 列出 API 版本 | - |
| `explain` | API 文档 | `--recursive` |

### 认证授权

| 命令 | 说明 | 常用选项 |
|:---|:---|:---|
| `auth` | 认证检查 | `can-i`, `whoami` |
| `certificate` | 证书管理 | `approve`, `deny` |

---

## 附录 B: 版本特性变更

### kubectl v1.32 (2024-12)

- Server-Side Apply 成为默认 (`--server-side` 默认启用)
- `kubectl debug` 增强，支持更多调试 profile
- 改进的资源打印格式

### kubectl v1.31 (2024-08)

- `kubectl events` 命令 GA
- `--subresource` 支持 GA
- 改进的 JSONPath 支持

### kubectl v1.30 (2024-04)

- `kubectl auth whoami` GA
- 交互式删除确认 (可配置)
- 改进的 Bash/Zsh 补全

### kubectl v1.29 (2023-12)

- `kubectl debug` profile 增强
- 改进的 Server-Side Apply 冲突处理

### kubectl v1.28 (2023-08)

- `--subresource` 支持 Beta
- `kubectl events` 改进

### kubectl v1.27 (2023-04)

- `kubectl auth whoami` Beta
- 改进的 kubeconfig 合并

### kubectl v1.26 (2022-12)

- `kubectl events` 命令 Beta
- 改进的资源监视

### kubectl v1.25 (2022-08)

- `kubectl debug` GA
- PSP 相关命令移除
- `kubectl create token` 添加

---

## 参考资源

| 资源 | 链接 |
|:---|:---|
| kubectl 官方文档 | https://kubernetes.io/docs/reference/kubectl/ |
| kubectl 命令参考 | https://kubernetes.io/docs/reference/generated/kubectl/kubectl-commands |
| kubectl Cheat Sheet | https://kubernetes.io/docs/reference/kubectl/cheatsheet/ |
| JSONPath 语法 | https://kubernetes.io/docs/reference/kubectl/jsonpath/ |
| Krew 插件列表 | https://krew.sigs.k8s.io/plugins/ |

---

> **文档维护**: 本文档随 Kubernetes 版本更新持续维护，当前适用于 v1.25 - v1.32
