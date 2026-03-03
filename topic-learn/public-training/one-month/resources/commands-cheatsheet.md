# K8s 命令速查表

常用 kubectl 命令快速参考。

---

## 集群信息

```bash
# 集群信息
kubectl cluster-info
kubectl version
kubectl config view

# 节点信息
kubectl get nodes
kubectl describe node <node-name>
kubectl top nodes
```

## Pod 操作

```bash
# 查看 Pod
kubectl get pods
kubectl get pods -o wide
kubectl get pods -A                    # 所有 namespace
kubectl get pods -l app=nginx          # 按标签筛选
kubectl get pods --show-labels

# Pod 详情
kubectl describe pod <pod-name>

# Pod 日志
kubectl logs <pod-name>
kubectl logs <pod-name> -c <container>  # 指定容器
kubectl logs <pod-name> --previous      # 上次崩溃日志
kubectl logs -f <pod-name>              # 实时日志

# 进入 Pod
kubectl exec -it <pod-name> -- /bin/sh
kubectl exec -it <pod-name> -c <container> -- /bin/sh

# 端口转发
kubectl port-forward <pod-name> 8080:80
kubectl port-forward svc/<service> 8080:80
```

## Deployment 操作

```bash
# 创建
kubectl create deployment nginx --image=nginx:alpine
kubectl apply -f deployment.yaml

# 查看
kubectl get deployments
kubectl describe deployment <name>

# 扩缩容
kubectl scale deployment <name> --replicas=3

# 滚动更新
kubectl set image deployment/<name> <container>=<image>
kubectl rollout status deployment/<name>
kubectl rollout history deployment/<name>

# 回滚
kubectl rollout undo deployment/<name>
kubectl rollout undo deployment/<name> --to-revision=2
```

## Service 操作

```bash
# 创建
kubectl expose deployment <name> --port=80 --type=ClusterIP
kubectl apply -f service.yaml

# 查看
kubectl get svc
kubectl describe svc <name>
kubectl get endpoints <name>
```

## Namespace 操作

```bash
# 创建
kubectl create namespace <name>

# 切换默认 namespace
kubectl config set-context --current --namespace=<name>

# 查看
kubectl get namespaces
```

## 资源管理

```bash
# 查看所有资源
kubectl get all
kubectl get all -n <namespace>

# 删除资源
kubectl delete pod <name>
kubectl delete deployment <name>
kubectl delete -f <file.yaml>

# 资源使用
kubectl top pods
kubectl top nodes
```

## 调试排查

```bash
# 事件
kubectl get events
kubectl get events --sort-by='.lastTimestamp'

# 权限检查
kubectl auth can-i <verb> <resource>
kubectl auth can-i --list

# API 资源
kubectl api-resources
kubectl api-versions
kubectl explain <resource>

# 调试 Pod
kubectl run debug --image=busybox -it --rm -- sh
kubectl run curl --image=curlimages/curl -it --rm -- sh
```

## YAML 生成

```bash
# dry-run 生成 YAML
kubectl create deployment nginx --image=nginx --dry-run=client -o yaml
kubectl expose deployment nginx --port=80 --dry-run=client -o yaml

# 导出现有资源
kubectl get deployment <name> -o yaml > deployment.yaml
```

## 标签和选择器

```bash
# 添加标签
kubectl label pod <name> app=web

# 删除标签
kubectl label pod <name> app-

# 按标签筛选
kubectl get pods -l app=web
kubectl get pods -l 'app in (web, api)'
```

## ConfigMap 和 Secret

```bash
# ConfigMap
kubectl create configmap <name> --from-literal=key=value
kubectl create configmap <name> --from-file=config.txt

# Secret
kubectl create secret generic <name> --from-literal=password=secret
kubectl get secret <name> -o jsonpath='{.data.password}' | base64 -d
```

---

## 常用缩写

| 全称 | 缩写 |
|------|------|
| pods | po |
| services | svc |
| deployments | deploy |
| replicasets | rs |
| configmaps | cm |
| namespaces | ns |
| nodes | no |
| persistentvolumes | pv |
| persistentvolumeclaims | pvc |
| statefulsets | sts |
| daemonsets | ds |
| ingresses | ing |
| networkpolicies | netpol |

---

## 输出格式

```bash
-o wide          # 详细信息
-o yaml          # YAML 格式
-o json          # JSON 格式
-o name          # 仅资源名称
-o jsonpath='{.metadata.name}'  # JSONPath 提取
```
