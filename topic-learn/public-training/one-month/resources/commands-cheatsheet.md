---
title: K8s 命令速查表
description: 'kubectl config view                           # 查看 kubeconfig 配置'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- docker
- hpa
- statefulset
- daemonset
- job
- cronjob
last_updated: 2026-05-18
difficulty: beginner
reading_level: beginner
audience:
- beginner-devops
- developer
- platform-engineer
estimated_read_time: 5min
intent_queries:
- kubectl 常用命令速查
- kubernetes 命令大全
- kubectl cheatsheet 常用操作
- k8s 日常运维命令
trigger_keywords:
- kubectl
- 命令速查
- cheatsheet
- 常用命令
- 运维
- Pod操作
- Deployment
- Service
- Namespace
- 资源管理
related_domains:
- domain-1-architecture-fundamentals
- domain-12-troubleshooting
related_topics:
- topic-learn/public-training/one-month/resources/reading-sequence
- topic-learn/public-training/one-month/resources/knowledge-map
---


# K8s 命令速查表

常用 kubectl 命令快速参考，按使用场景分类整理。

---

## 集群信息

```bash
# 集群基本信息
kubectl cluster-info                          # 显示集群 API 地址
kubectl version                               # 客户端和服务端版本
kubectl config view                           # 查看 kubeconfig 配置
kubectl config current-context                # 当前上下文
kubectl config get-contexts                   # 所有上下文列表

# 切换集群/上下文
kubectl config use-context <context-name>     # 切换上下文
kubectl config set-context --current --namespace=<ns>  # 切换默认 namespace

# 节点信息
kubectl get nodes                             # 节点列表
kubectl get nodes -o wide                     # 节点详情 (IP/OS/内核)
kubectl describe node <node-name>             # 节点完整信息
kubectl top nodes                             # 节点资源使用率

# 节点标签和污点
kubectl get nodes --show-labels               # 查看所有标签
kubectl label node <name> key=value           # 添加标签
kubectl label node <name> key-                # 删除标签
kubectl describe node <name> | grep Taints    # 查看污点
kubectl taint node <name> key=value:NoSchedule  # 添加污点
kubectl taint node <name> key:NoSchedule-     # 删除污点
```

---

## Pod 操作

```bash
# 查看 Pod
kubectl get pods                              # 当前 namespace Pod 列表
kubectl get pods -o wide                      # 详细信息 (IP/节点)
kubectl get pods -A                           # 所有 namespace
kubectl get pods -l app=nginx                 # 按标签筛选
kubectl get pods --show-labels                # 显示标签
kubectl get pods --sort-by='.metadata.creationTimestamp'  # 按时间排序

# Pod 详情
kubectl describe pod <pod-name>               # 完整详情和事件
kubectl get pod <name> -o yaml                # YAML 格式输出
kubectl get pod <name> -o json                # JSON 格式输出

# Pod 日志
kubectl logs <pod-name>                       # 当前日志
kubectl logs <pod-name> -c <container>        # 指定容器日志
kubectl logs <pod-name> --previous            # 上次崩溃日志
kubectl logs -f <pod-name>                    # 实时跟踪日志
kubectl logs <pod-name> --since=1h            # 最近 1 小时
kubectl logs <pod-name> --tail=100            # 最后 100 行
kubectl logs -l app=nginx --all-containers    # 所有容器日志

# 进入 Pod
kubectl exec -it <pod-name> -- /bin/sh        # 交互式终端
kubectl exec -it <pod-name> -c <container> -- /bin/sh  # 指定容器
kubectl exec <pod-name> -- <command>          # 执行单条命令

# 端口转发
kubectl port-forward <pod-name> 8080:80       # 转发 Pod 端口
kubectl port-forward svc/<service> 8080:80    # 转发 Service 端口
kubectl port-forward deploy/<name> 8080:80    # 转发 Deployment 端口
kubectl port-forward <pod> 8080:80 9090:9090  # 多端口转发

# 创建调试 Pod
kubectl run debug --image=busybox -it --rm -- sh          # busybox 调试
kubectl run curl --image=curlimages/curl -it --rm -- sh   # curl 调试
kubectl run netshoot --image=nicolaka/netshoot -it --rm -- bash  # 网络调试
```

---

## Deployment 操作

```bash
# 创建
kubectl create deployment nginx --image=nginx:alpine           # 命令行创建
kubectl apply -f deployment.yaml                                # YAML 文件创建

# 查看
kubectl get deployments                                         # Deployment 列表
kubectl get deployments -o wide                                 # 详细信息
kubectl describe deployment <name>                              # 完整详情

# 扩缩容
kubectl scale deployment <name> --replicas=3                    # 手动扩缩容
kubectl autoscale deployment <name> --min=2 --max=10 --cpu-percent=70  # 创建 HPA

# 滚动更新
kubectl set image deployment/<name> <container>=<image>         # 更新镜像
kubectl rollout status deployment/<name>                        # 查看更新状态
kubectl rollout history deployment/<name>                       # 查看更新历史
kubectl rollout history deployment/<name> --revision=2          # 查看指定版本

# 回滚
kubectl rollout undo deployment/<name>                          # 回滚到上一版本
kubectl rollout undo deployment/<name> --to-revision=2          # 回滚到指定版本

# 暂停和恢复
kubectl rollout pause deployment/<name>                         # 暂停滚动更新
kubectl rollout resume deployment/<name>                        # 恢复滚动更新
```

---

## Service 操作

```bash
# 创建
kubectl expose deployment <name> --port=80 --target-port=80     # 命令行创建
kubectl expose deployment <name> --type=LoadBalancer --port=80   # LoadBalancer 类型
kubectl apply -f service.yaml                                    # YAML 文件创建

# 查看
kubectl get svc                                                  # Service 列表
kubectl get svc -o wide                                          # 详细信息
kubectl describe svc <name>                                      # 完整详情

# Endpoints
kubectl get endpoints <name>                                     # 查看后端 Pod IP
kubectl get endpointslices -l kubernetes.io/service-name=<name>  # EndpointSlice

# DNS 测试
kubectl run dns-test --image=busybox --rm -it -- nslookup <service>
kubectl run dns-test --image=busybox --rm -it -- nslookup <svc>.<ns>.svc.cluster.local
```

---

## Namespace 操作

```bash
# 创建和查看
kubectl create namespace <name>                                  # 创建
kubectl get namespaces                                           # 列表
kubectl describe namespace <name>                                # 详情

# 切换默认 namespace
kubectl config set-context --current --namespace=<name>

# 删除 (包含所有资源)
kubectl delete namespace <name>
```

---

## 资源管理

```bash
# 查看所有资源
kubectl get all                                                  # 当前 namespace
kubectl get all -n <namespace>                                   # 指定 namespace

# 删除资源
kubectl delete pod <name>                                        # 删除 Pod
kubectl delete deployment <name>                                 # 删除 Deployment
kubectl delete -f <file.yaml>                                    # 删除 YAML 定义的所有资源
kubectl delete all --all -n <namespace>                          # 删除 namespace 所有资源

# 资源使用
kubectl top pods                                                 # Pod 资源使用
kubectl top pods -A --sort-by=cpu                                # 按 CPU 排序
kubectl top pods -A --sort-by=memory                             # 按内存排序
kubectl top nodes                                                # 节点资源使用
```

---

## 调试排查

```bash
# 事件
kubectl get events                                               # 当前 namespace 事件
kubectl get events -A                                            # 所有 namespace 事件
kubectl get events --sort-by='.lastTimestamp'                    # 按时间排序
kubectl get events --field-selector type=Warning                 # 仅警告事件
kubectl get events --field-selector involvedObject.name=<name>   # 指定资源事件

# 权限检查
kubectl auth can-i <verb> <resource>                             # 检查当前用户权限
kubectl auth can-i <verb> <resource> --as=<user>                 # 模拟用户权限
kubectl auth can-i --list                                        # 列出所有权限
kubectl auth can-i --list --as=system:serviceaccount:<ns>:<sa>   # SA 权限

# API 资源
kubectl api-resources                                            # 所有资源类型
kubectl api-resources --namespaced=false                         # 集群级资源
kubectl api-versions                                             # 所有 API 版本
kubectl explain <resource>                                       # 资源字段说明
kubectl explain pod.spec.containers                              # 嵌套字段说明

# 调试 Pod
kubectl run debug --image=busybox -it --rm -- sh
kubectl run curl --image=curlimages/curl -it --rm -- sh
kubectl run netshoot --image=nicolaka/netshoot -it --rm -- bash

# 节点调试
kubectl debug node/<name> -it --image=busybox                    # 调试节点 (1.18+)
```

---

## YAML 生成

```bash
# dry-run 生成 YAML
kubectl create deployment nginx --image=nginx --dry-run=client -o yaml
kubectl expose deployment nginx --port=80 --dry-run=client -o yaml
kubectl create job test --image=busybox --dry-run=client -o yaml -- echo hello

# 导出现有资源
kubectl get deployment <name> -o yaml > deployment.yaml
kubectl get svc <name> -o yaml > service.yaml
kubectl get all -o yaml > all-resources.yaml

# diff (1.13+)
kubectl diff -f deployment.yaml                                  # 预览变更
```

---

## 标签和选择器

```bash
# 添加标签
kubectl label pod <name> app=web                                 # 添加标签
kubectl label pod <name> app=web --overwrite                     # 覆盖标签
kubectl label pod <name> app-                                    # 删除标签
kubectl label pods -l app=old env=staging --all                  # 批量添加

# 按标签筛选
kubectl get pods -l app=web                                      # 等于
kubectl get pods -l 'app in (web, api)'                          # 集合
kubectl get pods -l 'app notin (debug)'                          # 不在集合
kubectl get pods -l env!=prod                                    # 不等于
kubectl get pods -l 'version>1.0'                                # 大于
```

---

## ConfigMap 和 Secret

```bash
# ConfigMap
kubectl create configmap <name> --from-literal=key=value         # 键值对
kubectl create configmap <name> --from-file=config.txt           # 文件
kubectl create configmap <name> --from-env-file=.env             # 环境变量文件
kubectl get configmap <name> -o yaml                             # 查看

# Secret
kubectl create secret generic <name> --from-literal=password=secret  # 键值对
kubectl create secret generic <name> --from-file=ssh-key=~/.ssh/id_rsa  # 文件
kubectl create secret docker-registry regcred \
  --docker-server=registry.cn-hangzhou.aliyuncs.com \
  --docker-username=<user> \
  --docker-password=<pass>                                       # 镜像仓库凭证
kubectl get secret <name> -o jsonpath='{.data.password}' | base64 -d  # 解码查看
```

---

## 存储操作

```bash
# StorageClass
kubectl get storageclass                                         # 列表
kubectl describe storageclass <name>                             # 详情

# PV
kubectl get pv                                                   # 列表
kubectl describe pv <name>                                       # 详情

# PVC
kubectl get pvc                                                  # 列表
kubectl get pvc -A                                               # 所有 namespace
kubectl describe pvc <name>                                      # 详情
```

---

## 常用缩写

| 全称 | 缩写 | 全称 | 缩写 |
|------|------|------|------|
| pods | po | services | svc |
| deployments | deploy | replicasets | rs |
| configmaps | cm | namespaces | ns |
| nodes | no | persistentvolumes | pv |
| persistentvolumeclaims | pvc | statefulsets | sts |
| daemonsets | ds | ingresses | ing |
| networkpolicies | netpol | serviceaccounts | sa |
| clusterroles | cr | clusterrolebindings | crb |
| roles | ro | rolebindings | rb |
| horizontalpodautoscalers | hpa | cronjobs | cj |
| storageclass | sc | endpoints | ep |

---

## 输出格式

```bash
-o wide                              # 列表模式，显示更多信息
-o yaml                              # YAML 格式输出
-o json                              # JSON 格式输出
-o name                              # 仅资源名称
-o custom-columns='NAME:.metadata.name,STATUS:.status.phase'  # 自定义列
-o jsonpath='{.items[*].metadata.name}'                        # JSONPath 提取
-o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}'    # JSONPath 循环
```

### 常用 JSONPath 示例

```bash
# 获取所有 Pod IP
kubectl get pods -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.podIP}{"\n"}{end}'

# 获取节点容量
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.capacity.cpu}{"\t"}{.status.capacity.memory}{"\n"}{end}'

# 获取 Pod 的镜像
kubectl get pods -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.containers[*].image}{"\n"}{end}'

# 获取 Service 的 ClusterIP
kubectl get svc -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.clusterIP}{"\n"}{end}'
```

---

## 高级操作

```bash
# 批量操作
kubectl get pods -l app=old -o name | xargs -I {} kubectl delete {}

# 等待条件
kubectl wait --for=condition=ready pod/<name> --timeout=60s
kubectl wait --for=condition=available deployment/<name> --timeout=120s

# 资源使用分析
kubectl get pods -A -o json | jq '.items[] | {name: .metadata.name, ns: .metadata.namespace, cpu: .spec.containers[].resources.requests.cpu}'

# 集群资源汇总
kubectl get nodes -o json | jq '.items[] | {name: .metadata.name, cpu: .status.capacity.cpu, memory: .status.capacity.memory, pods: .status.capacity.pods}'
```

---

## 常用组合命令

| 场景 | 命令 |
|------|------|
| 查看 Pod 重启次数 Top 10 | `kubectl get pods -A --sort-by='.status.containerStatuses[0].restartCount' \| head -11` |
| 查看所有节点版本 | `kubectl get nodes -o custom-columns='NAME:.metadata.name,VERSION:.status.nodeInfo.kubeletVersion'` |
| 查看 Pod QoS 等级 | `kubectl get pods -o custom-columns='NAME:.metadata.name,QOS:.status.qosClass'` |
| 查看 PVC 绑定状态 | `kubectl get pvc -A -o custom-columns='NAME:.metadata.name,STATUS:.status.phase,VOLUME:.spec.volumeName,CAPACITY:.status.capacity.storage'` |
| 统计每个 NS 的 Pod 数 | `kubectl get pods -A --no-headers \| awk '{print $1}' \| sort \| uniq -c \| sort -rn` |
| 查看所有镜像版本 | `kubectl get pods -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {.spec.containers[*].image}{"\n"}{end}'` |
