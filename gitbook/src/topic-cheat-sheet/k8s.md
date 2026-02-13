# Kubernetes 生产环境速查卡

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02  
> **目标**: 涵盖生产环境 90% 以上常用命令，支持快速查阅和故障排查

---

## 📋 目录

- [kubectl 基础操作](#kubectl-基础操作)
- [集群信息与版本](#集群信息与版本)
- [资源查询与筛选](#资源查询与筛选)
- [Pod 操作](#pod-操作)
- [Deployment 管理](#deployment-管理)
- [Service 与网络](#service-与网络)
- [ConfigMap & Secret](#configmap--secret)
- [存储管理](#存储管理)
- [调度与亲和性](#调度与亲和性)
- [RBAC 权限管理](#rbac-权限管理)
- [故障排查](#故障排查)
- [资源监控](#资源监控)
- [高级操作](#高级操作)
- [etcd 操作](#etcd-操作)
- [API Server 管理](#api-server-管理)
- [集群维护](#集群维护)

---

## kubectl 基础操作

### 版本与上下文

```bash
# 查看 kubectl 版本 (适用于所有版本)
kubectl version --short
kubectl version --client --output=yaml  # v1.25+ 推荐格式

# 查看客户端和服务端详细版本
kubectl version --output=json | jq '.serverVersion'

# 查看当前上下文
kubectl config current-context

# 列出所有上下文
kubectl config get-contexts

# 切换上下文
kubectl config use-context <context-name>

# 设置默认命名空间
kubectl config set-context --current --namespace=<namespace>

# 查看配置文件路径
kubectl config view --minify | grep "current-context:" -A 3
```

**版本说明**:
- `--short` 标志在 v1.28+ 已弃用，推荐使用 `--output=yaml|json`
- `kubectl version` 在 v1.25+ 默认不显示服务端版本（除非添加 `--request-timeout=5s`）

---

## 集群信息与版本

```bash
# 查看集群信息
kubectl cluster-info
kubectl cluster-info dump  # 导出完整集群诊断信息

# 查看节点列表 (适用 v1.25-v1.32)
kubectl get nodes
kubectl get nodes -o wide  # 显示 IP、内核版本、容器运行时

# 查看节点详细信息
kubectl describe node <node-name>

# 查看节点容量和已分配资源
kubectl top nodes  # 需要安装 metrics-server (v0.6.0+)

# 查看节点标签
kubectl get nodes --show-labels

# 查看 API 资源版本
kubectl api-resources
kubectl api-resources --namespaced=true  # 仅命名空间级别资源
kubectl api-resources --api-group=apps  # 指定 API 组

# 查看 API 版本
kubectl api-versions

# 检查集群健康状态 (v1.25+)
kubectl get --raw='/readyz?verbose' | jq
kubectl get --raw='/livez?verbose' | jq
kubectl get componentstatuses  # ⚠️ v1.19+ 已弃用
```

**版本兼容性**:
- `kubectl top` 需要部署 metrics-server v0.6.0+ (兼容 K8s v1.25-v1.32)
- `kubectl get componentstatuses` 在 v1.19+ 已弃用，使用 `/livez` `/readyz` API 代替

---

## 资源查询与筛选

### 基础查询

```bash
# 查看所有命名空间的资源
kubectl get all -A  # -A 等同于 --all-namespaces

# 查看特定命名空间
kubectl get pods -n <namespace>
kubectl get pods --namespace=<namespace>

# 查看特定资源类型
kubectl get pods,svc,deploy -n <namespace>

# 多输出格式 (v1.25+)
kubectl get pods -o wide       # 宽表格
kubectl get pods -o yaml       # YAML 格式
kubectl get pods -o json       # JSON 格式
kubectl get pods -o name       # 仅名称
kubectl get pods -o jsonpath='{.items[*].metadata.name}'  # JSONPath

# 自定义列输出 (v1.25+)
kubectl get pods -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,NODE:.spec.nodeName

# 排序 (v1.25+)
kubectl get pods --sort-by=.metadata.creationTimestamp
kubectl get pods --sort-by=.status.startTime
```

### 标签与选择器

```bash
# 按标签筛选
kubectl get pods -l app=nginx
kubectl get pods -l 'env in (prod,staging)'
kubectl get pods -l 'tier!=frontend'

# 查看资源标签
kubectl get pods --show-labels

# 添加标签
kubectl label pods <pod-name> env=prod

# 修改标签 (需要 --overwrite)
kubectl label pods <pod-name> env=staging --overwrite

# 删除标签
kubectl label pods <pod-name> env-

# 多标签选择器
kubectl get pods -l 'app=nginx,tier=backend'

# 集合选择器 (v1.25+)
kubectl get pods -l 'app in (nginx,apache)'
kubectl get pods -l 'tier notin (frontend,cache)'
```

### 字段选择器

```bash
# 按字段筛选 (v1.25+)
kubectl get pods --field-selector status.phase=Running
kubectl get pods --field-selector status.phase!=Running,spec.restartPolicy=Always

# 查看特定节点上的 Pod
kubectl get pods --field-selector spec.nodeName=<node-name>

# 查看 Pending 状态的 Pod
kubectl get pods -A --field-selector status.phase=Pending

# 组合标签和字段选择器
kubectl get pods -l app=nginx --field-selector status.phase=Running
```

---

## Pod 操作

### Pod 创建与删除

```bash
# 从 YAML 创建
kubectl apply -f pod.yaml
kubectl create -f pod.yaml  # 不存在才创建

# 从命令行创建 (v1.25+)
kubectl run nginx --image=nginx:1.25 --port=80
kubectl run nginx --image=nginx:1.25 --dry-run=client -o yaml > pod.yaml

# 创建临时测试 Pod (v1.25+)
kubectl run test --image=busybox:1.36 --rm -it -- sh
kubectl run test --image=curlimages/curl:8.5.0 --rm -it -- sh

# 删除 Pod
kubectl delete pod <pod-name>
kubectl delete pod <pod-name> --grace-period=0 --force  # 强制删除
kubectl delete pod <pod-name> --wait=false  # 异步删除

# 批量删除
kubectl delete pods -l app=nginx
kubectl delete pods --all -n <namespace>

# 删除并重建 (v1.25+)
kubectl replace --force -f pod.yaml
```

**镜像版本说明**:
- `nginx:1.25` - 适用于生产环境 (2024+ 推荐)
- `busybox:1.36` - 故障排查工具 (2024 稳定版)
- `curlimages/curl:8.5.0` - 网络测试工具 (Alpine 精简版)

### Pod 状态查询

```bash
# 查看 Pod 状态
kubectl get pods
kubectl get pods -o wide  # 显示节点和 IP

# 查看 Pod 详细信息
kubectl describe pod <pod-name>

# 查看 Pod YAML 配置
kubectl get pod <pod-name> -o yaml

# 查看 Pod 事件 (最近 1 小时)
kubectl get events --field-selector involvedObject.name=<pod-name> --sort-by='.lastTimestamp'

# 查看 Pod 容器状态
kubectl get pod <pod-name> -o jsonpath='{.status.containerStatuses[*].state}'

# 查看 Pod 重启次数
kubectl get pods -o custom-columns=NAME:.metadata.name,RESTARTS:.status.containerStatuses[*].restartCount

# 监控 Pod 状态变化 (v1.25+)
kubectl get pods --watch
kubectl get pods -w --output-watch-events  # 显示事件类型
```

### Pod 日志

```bash
# 查看 Pod 日志 (适用 v1.25-v1.32)
kubectl logs <pod-name>
kubectl logs <pod-name> -c <container-name>  # 多容器 Pod

# 实时跟踪日志
kubectl logs -f <pod-name>

# 查看最近 N 行日志
kubectl logs <pod-name> --tail=100

# 查看过去 N 小时的日志
kubectl logs <pod-name> --since=1h
kubectl logs <pod-name> --since-time='2026-02-11T10:00:00Z'

# 查看前一个容器的日志 (重启后)
kubectl logs <pod-name> --previous

# 多 Pod 日志聚合 (v1.25+)
kubectl logs -l app=nginx --all-containers=true --prefix=true

# 导出日志到文件
kubectl logs <pod-name> > pod.log

# 查看 Init Container 日志
kubectl logs <pod-name> -c <init-container-name>
```

### Pod 执行命令

```bash
# 进入 Pod (交互式 shell)
kubectl exec -it <pod-name> -- /bin/bash
kubectl exec -it <pod-name> -- /bin/sh  # 如果没有 bash

# 指定容器 (多容器 Pod)
kubectl exec -it <pod-name> -c <container-name> -- /bin/bash

# 执行单条命令
kubectl exec <pod-name> -- ls /app
kubectl exec <pod-name> -- env  # 查看环境变量

# 复制文件到 Pod (v1.25+)
kubectl cp /local/file.txt <pod-name>:/remote/path/
kubectl cp <pod-name>:/remote/file.txt /local/path/

# 多容器 Pod 复制文件
kubectl cp /local/file.txt <pod-name>:/remote/path/ -c <container-name>
```

### Pod 调试

```bash
# 创建调试容器 (v1.25+, 需要 EphemeralContainers 特性)
kubectl debug <pod-name> -it --image=busybox:1.36

# 在新 Pod 中调试 (复制原 Pod 配置)
kubectl debug <pod-name> -it --copy-to=<new-pod-name> --container=debug -- sh

# 在节点上创建特权调试 Pod (v1.26+)
kubectl debug node/<node-name> -it --image=ubuntu:22.04

# 查看 Pod 资源使用 (需要 metrics-server)
kubectl top pod <pod-name>
kubectl top pod <pod-name> --containers  # 查看容器级别

# 端口转发到本地 (v1.25+)
kubectl port-forward <pod-name> 8080:80
kubectl port-forward <pod-name> 8080:80 --address=0.0.0.0  # 监听所有接口

# 查看 Pod 挂载的 ConfigMap/Secret
kubectl get pod <pod-name> -o jsonpath='{.spec.volumes[*]}'
```

---

## Deployment 管理

### Deployment 创建与更新

```bash
# 创建 Deployment (v1.25+)
kubectl create deployment nginx --image=nginx:1.25 --replicas=3
kubectl create deployment nginx --image=nginx:1.25 --dry-run=client -o yaml > deploy.yaml

# 应用 YAML
kubectl apply -f deployment.yaml

# 扩缩容
kubectl scale deployment <deployment-name> --replicas=5

# 自动扩缩容 (HPA, v1.25+ 使用 autoscaling/v2)
kubectl autoscale deployment <deployment-name> --min=2 --max=10 --cpu-percent=80

# 更新镜像
kubectl set image deployment/<deployment-name> <container-name>=<new-image>:<tag>

# 编辑 Deployment
kubectl edit deployment <deployment-name>

# 查看 Deployment
kubectl get deployment
kubectl get deploy -o wide

# 查看 Deployment 详情
kubectl describe deployment <deployment-name>

# 查看 Deployment YAML
kubectl get deployment <deployment-name> -o yaml
```

### Deployment 滚动更新

```bash
# 查看滚动更新状态
kubectl rollout status deployment/<deployment-name>

# 查看滚动更新历史 (v1.25+)
kubectl rollout history deployment/<deployment-name>

# 查看特定 revision 详情
kubectl rollout history deployment/<deployment-name> --revision=2

# 暂停滚动更新 (金丝雀发布)
kubectl rollout pause deployment/<deployment-name>

# 恢复滚动更新
kubectl rollout resume deployment/<deployment-name>

# 回滚到上一版本
kubectl rollout undo deployment/<deployment-name>

# 回滚到指定版本
kubectl rollout undo deployment/<deployment-name> --to-revision=3

# 重启所有 Pod (v1.15+)
kubectl rollout restart deployment/<deployment-name>
```

### ReplicaSet 管理

```bash
# 查看 ReplicaSet
kubectl get rs
kubectl get replicaset

# 查看 Deployment 关联的 ReplicaSet
kubectl get rs -l app=<deployment-label>

# 查看 ReplicaSet 详情
kubectl describe rs <rs-name>

# 删除 ReplicaSet (会自动重建)
kubectl delete rs <rs-name>
```

---

## Service 与网络

### Service 管理

```bash
# 创建 Service (v1.25+)
kubectl expose deployment <deployment-name> --port=80 --target-port=8080
kubectl expose deployment <deployment-name> --port=80 --type=NodePort
kubectl expose deployment <deployment-name> --port=80 --type=LoadBalancer

# 从 YAML 创建
kubectl apply -f service.yaml

# 查看 Service
kubectl get svc
kubectl get service -o wide

# 查看 Service 详情
kubectl describe svc <service-name>

# 查看 Service Endpoints
kubectl get endpoints <service-name>
kubectl get ep <service-name>  # 缩写

# 查看 Service 关联的 Pod
kubectl get pods -l <service-selector>

# 删除 Service
kubectl delete svc <service-name>
```

### Service 类型

```yaml
# ClusterIP (默认, 仅集群内访问)
apiVersion: v1
kind: Service
spec:
  type: ClusterIP
  clusterIP: None  # Headless Service

# NodePort (通过节点 IP:Port 访问)
spec:
  type: NodePort
  ports:
  - port: 80
    nodePort: 30080  # 30000-32767

# LoadBalancer (云厂商负载均衡器)
spec:
  type: LoadBalancer
  loadBalancerIP: 1.2.3.4  # 可选

# ExternalName (CNAME 别名)
spec:
  type: ExternalName
  externalName: example.com
```

### Ingress 管理

```bash
# 查看 Ingress (v1.25+ 使用 networking.k8s.io/v1)
kubectl get ingress
kubectl get ing -o wide

# 查看 Ingress 详情
kubectl describe ingress <ingress-name>

# 创建 Ingress
kubectl apply -f ingress.yaml

# 查看 Ingress 控制器 Pod
kubectl get pods -n ingress-nginx  # NGINX Ingress
kubectl get pods -n projectcontour  # Contour
kubectl get pods -n istio-system -l app=istio-ingressgateway  # Istio

# 查看 Ingress Class (v1.19+)
kubectl get ingressclass
```

**Ingress 控制器版本**:
- **NGINX Ingress Controller**: v1.9.0+ (兼容 K8s v1.25-v1.32)
- **Traefik**: v2.10+ (兼容 K8s v1.25-v1.32)
- **Istio Ingress Gateway**: v1.19+ (兼容 K8s v1.25-v1.32)

### NetworkPolicy

```bash
# 查看 NetworkPolicy (v1.25+ 使用 networking.k8s.io/v1)
kubectl get networkpolicy
kubectl get netpol  # 缩写

# 查看详情
kubectl describe networkpolicy <policy-name>

# 应用 NetworkPolicy
kubectl apply -f networkpolicy.yaml

# 删除 NetworkPolicy
kubectl delete networkpolicy <policy-name>

# 测试网络连通性 (需要 CNI 支持)
kubectl run test --image=busybox:1.36 --rm -it -- wget -O- http://<service-name>
```

**CNI 插件 NetworkPolicy 支持**:
- **Calico** v3.26+ ✅ 完整支持
- **Cilium** v1.14+ ✅ 完整支持 + eBPF 加速
- **Weave Net** v2.8+ ✅ 支持
- **Flannel** ❌ 不支持 (需要配合 Calico)

---

## ConfigMap & Secret

### ConfigMap 管理

```bash
# 从字面量创建 ConfigMap (v1.25+)
kubectl create configmap <cm-name> --from-literal=key1=value1 --from-literal=key2=value2

# 从文件创建
kubectl create configmap <cm-name> --from-file=config.txt
kubectl create configmap <cm-name> --from-file=app-config=/path/to/config.json

# 从目录创建
kubectl create configmap <cm-name> --from-file=/path/to/config-dir/

# 从 YAML 创建
kubectl apply -f configmap.yaml

# 查看 ConfigMap
kubectl get configmap
kubectl get cm  # 缩写

# 查看 ConfigMap 内容
kubectl describe cm <cm-name>
kubectl get cm <cm-name> -o yaml

# 编辑 ConfigMap
kubectl edit cm <cm-name>

# 删除 ConfigMap
kubectl delete cm <cm-name>
```

### Secret 管理

```bash
# 创建 Generic Secret (v1.25+)
kubectl create secret generic <secret-name> --from-literal=username=admin --from-literal=password=pass123

# 从文件创建
kubectl create secret generic <secret-name> --from-file=ssh-privatekey=/path/to/.ssh/id_rsa

# 创建 Docker 镜像拉取凭证 (v1.25+)
kubectl create secret docker-registry <secret-name> \
  --docker-server=registry.example.com \
  --docker-username=user \
  --docker-password=pass \
  --docker-email=user@example.com

# 创建 TLS Secret (v1.25+)
kubectl create secret tls <secret-name> \
  --cert=/path/to/tls.crt \
  --key=/path/to/tls.key

# 查看 Secret (值会被隐藏)
kubectl get secret
kubectl get secret <secret-name> -o yaml

# 解码 Secret 值
kubectl get secret <secret-name> -o jsonpath='{.data.password}' | base64 -d

# 删除 Secret
kubectl delete secret <secret-name>
```

**Secret 类型** (v1.25+):
- `Opaque` - 默认类型 (通用)
- `kubernetes.io/service-account-token` - ServiceAccount Token
- `kubernetes.io/dockercfg` - Docker 配置 (已弃用)
- `kubernetes.io/dockerconfigjson` - Docker 配置 (推荐)
- `kubernetes.io/tls` - TLS 证书

---

## 存储管理

### PersistentVolume (PV)

```bash
# 查看 PV (cluster-scoped)
kubectl get pv
kubectl get persistentvolume -o wide

# 查看 PV 详情
kubectl describe pv <pv-name>

# 查看 PV 状态
kubectl get pv -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,CLAIM:.spec.claimRef.name

# 删除 PV
kubectl delete pv <pv-name>
```

### PersistentVolumeClaim (PVC)

```bash
# 查看 PVC
kubectl get pvc
kubectl get persistentvolumeclaim

# 查看 PVC 详情
kubectl describe pvc <pvc-name>

# 创建 PVC
kubectl apply -f pvc.yaml

# 查看 PVC 绑定状态
kubectl get pvc -o custom-columns=NAME:.metadata.name,STATUS:.status.phase,VOLUME:.spec.volumeName

# 删除 PVC
kubectl delete pvc <pvc-name>

# 扩容 PVC (v1.25+ 需要 StorageClass 支持 allowVolumeExpansion)
kubectl edit pvc <pvc-name>  # 修改 spec.resources.requests.storage
```

### StorageClass

```bash
# 查看 StorageClass (v1.25+)
kubectl get storageclass
kubectl get sc  # 缩写

# 查看默认 StorageClass
kubectl get sc -o jsonpath='{.items[?(@.metadata.annotations.storageclass\.kubernetes\.io/is-default-class=="true")].metadata.name}'

# 设置默认 StorageClass
kubectl patch storageclass <sc-name> -p '{"metadata": {"annotations":{"storageclass.kubernetes.io/is-default-class":"true"}}}'

# 查看 StorageClass 详情
kubectl describe sc <sc-name>
```

**主流 CSI 驱动版本** (兼容 K8s v1.25-v1.32):
- **AWS EBS CSI**: v1.26+
- **GCE PD CSI**: v1.12+
- **Azure Disk CSI**: v1.28+
- **Longhorn**: v1.5+
- **Rook Ceph**: v1.12+ (Ceph Pacific/Quincy)
- **OpenEBS**: v3.9+

### VolumeSnapshot (v1.25+)

```bash
# 查看 VolumeSnapshot (需要 CSI 驱动支持)
kubectl get volumesnapshot
kubectl get volumesnapshotclass

# 查看 VolumeSnapshotContent
kubectl get volumesnapshotcontent

# 创建快照
kubectl apply -f volumesnapshot.yaml

# 从快照恢复 PVC
kubectl apply -f pvc-from-snapshot.yaml
```

---

## 调度与亲和性

### 节点调度

```bash
# 节点标签操作
kubectl label nodes <node-name> disktype=ssd
kubectl label nodes <node-name> disktype-  # 删除标签

# 节点污点操作 (v1.25+)
kubectl taint nodes <node-name> key=value:NoSchedule
kubectl taint nodes <node-name> key=value:NoExecute
kubectl taint nodes <node-name> key:NoSchedule-  # 删除污点

# 查看节点污点
kubectl get nodes -o jsonpath='{.items[*].spec.taints}'

# 标记节点不可调度 (维护模式)
kubectl cordon <node-name>
kubectl uncordon <node-name>  # 恢复调度

# 驱逐节点上的 Pod (v1.25+)
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 驱逐节点 (保留 DaemonSet 和 本地数据)
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force
```

### Pod 调度策略

```yaml
# NodeSelector (简单节点选择)
spec:
  nodeSelector:
    disktype: ssd
    kubernetes.io/arch: amd64

# NodeAffinity (节点亲和性, v1.25+)
spec:
  affinity:
    nodeAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
        nodeSelectorTerms:
        - matchExpressions:
          - key: kubernetes.io/hostname
            operator: In
            values: [node1, node2]
      preferredDuringSchedulingIgnoredDuringExecution:
      - weight: 100
        preference:
          matchExpressions:
          - key: disktype
            operator: In
            values: [ssd]

# PodAffinity (Pod 亲和性)
spec:
  affinity:
    podAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchLabels:
            app: cache
        topologyKey: kubernetes.io/hostname

# PodAntiAffinity (Pod 反亲和性)
spec:
  affinity:
    podAntiAffinity:
      requiredDuringSchedulingIgnoredDuringExecution:
      - labelSelector:
          matchLabels:
            app: web
        topologyKey: kubernetes.io/hostname

# Toleration (容忍污点)
spec:
  tolerations:
  - key: "key"
    operator: "Equal"
    value: "value"
    effect: "NoSchedule"
```

### PriorityClass (v1.25+)

```bash
# 查看 PriorityClass
kubectl get priorityclass
kubectl get pc  # 缩写

# 创建 PriorityClass
kubectl apply -f priorityclass.yaml

# Pod 中使用
# spec:
#   priorityClassName: high-priority
```

---

## RBAC 权限管理

### ServiceAccount

```bash
# 创建 ServiceAccount (v1.25+)
kubectl create serviceaccount <sa-name>

# 查看 ServiceAccount
kubectl get serviceaccount
kubectl get sa  # 缩写

# 查看 ServiceAccount Token (v1.25+ 需要手动创建 Secret)
kubectl create token <sa-name>  # 临时 Token (1 小时过期)
kubectl create token <sa-name> --duration=24h  # 自定义过期时间

# 绑定到 Pod
# spec:
#   serviceAccountName: <sa-name>
```

**Token 变更** (v1.25+):
- ServiceAccount 不再自动创建 Secret
- 使用 `kubectl create token` 生成临时 Token (推荐)
- 或手动创建 `kubernetes.io/service-account-token` 类型 Secret (长期 Token)

### Role & RoleBinding (命名空间级别)

```bash
# 创建 Role (v1.25+)
kubectl create role <role-name> --verb=get,list,watch --resource=pods

# 查看 Role
kubectl get role
kubectl describe role <role-name>

# 创建 RoleBinding
kubectl create rolebinding <binding-name> --role=<role-name> --serviceaccount=<namespace>:<sa-name>
kubectl create rolebinding <binding-name> --role=<role-name> --user=<username>
kubectl create rolebinding <binding-name> --role=<role-name> --group=<group-name>

# 查看 RoleBinding
kubectl get rolebinding
kubectl describe rolebinding <binding-name>

# 查看用户权限
kubectl auth can-i get pods --as=<username>
kubectl auth can-i create deployments --as=system:serviceaccount:<namespace>:<sa-name>

# 查看所有权限
kubectl auth can-i --list --as=<username>
```

### ClusterRole & ClusterRoleBinding (集群级别)

```bash
# 创建 ClusterRole
kubectl create clusterrole <role-name> --verb=get,list,watch --resource=nodes

# 查看 ClusterRole
kubectl get clusterrole
kubectl describe clusterrole <role-name>

# 创建 ClusterRoleBinding
kubectl create clusterrolebinding <binding-name> --clusterrole=<role-name> --serviceaccount=<namespace>:<sa-name>

# 查看 ClusterRoleBinding
kubectl get clusterrolebinding
kubectl describe clusterrolebinding <binding-name>

# 常用内置 ClusterRole
kubectl get clusterrole | grep -E "^(cluster-admin|admin|edit|view)"
# - cluster-admin: 完全管理员权限
# - admin: 命名空间管理员权限
# - edit: 可编辑资源 (不含 RBAC)
# - view: 只读权限
```

---

## 故障排查

### 事件查询

```bash
# 查看所有事件 (按时间排序)
kubectl get events --sort-by='.lastTimestamp'
kubectl get events --sort-by='.metadata.creationTimestamp'

# 查看最近事件
kubectl get events --watch

# 查看特定命名空间事件
kubectl get events -n <namespace>

# 查看特定资源的事件
kubectl get events --field-selector involvedObject.name=<pod-name>
kubectl get events --field-selector involvedObject.kind=Deployment

# 过滤警告事件 (v1.25+)
kubectl get events --field-selector type=Warning

# 查看事件详细信息
kubectl describe event <event-name>
```

### Pod 故障排查

```bash
# 查看 Pod 状态原因
kubectl get pod <pod-name> -o jsonpath='{.status.conditions[*].message}'

# 查看容器退出码
kubectl get pod <pod-name> -o jsonpath='{.status.containerStatuses[*].state.terminated.exitCode}'

# 查看容器重启原因
kubectl get pod <pod-name> -o jsonpath='{.status.containerStatuses[*].lastState.terminated.reason}'

# 查看 Pod 调度失败原因
kubectl describe pod <pod-name> | grep -A 10 "Events:"

# 查看所有 Pending 的 Pod
kubectl get pods -A --field-selector status.phase=Pending

# 查看所有 Failed 的 Pod
kubectl get pods -A --field-selector status.phase=Failed

# 查看 CrashLoopBackOff 的 Pod
kubectl get pods -A | grep CrashLoopBackOff

# 查看 ImagePullBackOff 的 Pod
kubectl get pods -A | grep ImagePullBackOff
```

### 节点故障排查

```bash
# 查看节点状态
kubectl get nodes -o wide
kubectl describe node <node-name>

# 查看节点 Conditions
kubectl get node <node-name> -o jsonpath='{.status.conditions[*]}'

# 查看节点容量和已分配
kubectl describe node <node-name> | grep -A 5 "Allocated resources:"

# 查看节点 Pod 列表
kubectl get pods -A --field-selector spec.nodeName=<node-name>

# 查看 NotReady 节点
kubectl get nodes | grep NotReady

# 查看节点事件
kubectl get events --field-selector involvedObject.kind=Node,involvedObject.name=<node-name>
```

### 网络故障排查

```bash
# 测试 Service 连通性
kubectl run test --image=busybox:1.36 --rm -it -- wget -O- http://<service-name>.<namespace>.svc.cluster.local

# 测试 DNS 解析
kubectl run test --image=busybox:1.36 --rm -it -- nslookup <service-name>

# 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns -f

# 查看 kube-proxy 日志
kubectl logs -n kube-system -l k8s-app=kube-proxy

# 查看 Service Endpoints
kubectl get endpoints <service-name>

# 查看 Pod 网络接口
kubectl exec <pod-name> -- ip addr show

# 查看 Pod 路由表
kubectl exec <pod-name> -- ip route

# 测试 Pod 间连通性
kubectl exec <pod1> -- ping <pod2-ip>
```

### 存储故障排查

```bash
# 查看 PVC 绑定状态
kubectl get pvc -A

# 查看 PVC 事件
kubectl describe pvc <pvc-name>

# 查看 PV 回收策略
kubectl get pv -o custom-columns=NAME:.metadata.name,RECLAIM:.spec.persistentVolumeReclaimPolicy

# 查看 StorageClass Provisioner
kubectl get sc -o custom-columns=NAME:.metadata.name,PROVISIONER:.provisioner

# 查看 CSI Driver
kubectl get csidrivers
kubectl get csinodes

# 查看 Volume Attachment
kubectl get volumeattachment
```

---

## 资源监控

### Metrics Server (v0.6.0+)

```bash
# 安装 Metrics Server (适用 K8s v1.25-v1.32)
kubectl apply -f https://github.com/kubernetes-sigs/metrics-server/releases/latest/download/components.yaml

# 查看节点资源使用
kubectl top nodes
kubectl top nodes --sort-by=cpu
kubectl top nodes --sort-by=memory

# 查看 Pod 资源使用
kubectl top pods
kubectl top pods -A --sort-by=cpu
kubectl top pods --containers  # 查看容器级别

# 查看特定命名空间
kubectl top pods -n <namespace>
```

### 资源配额

```bash
# 查看 ResourceQuota
kubectl get resourcequota
kubectl get quota  # 缩写

# 查看详情
kubectl describe quota <quota-name>

# 查看 LimitRange
kubectl get limitrange
kubectl describe limitrange <limitrange-name>
```

### HorizontalPodAutoscaler (HPA, v1.25+ 使用 autoscaling/v2)

```bash
# 创建 HPA
kubectl autoscale deployment <deployment-name> --min=2 --max=10 --cpu-percent=80

# 查看 HPA
kubectl get hpa
kubectl get horizontalpodautoscaler

# 查看 HPA 详情
kubectl describe hpa <hpa-name>

# 查看 HPA 状态
kubectl get hpa -o custom-columns=NAME:.metadata.name,REPLICAS:.status.currentReplicas,TARGET:.status.desiredReplicas

# 删除 HPA
kubectl delete hpa <hpa-name>
```

**HPA v2 特性** (v1.25+):
- 支持多指标 (CPU、内存、自定义指标、外部指标)
- 支持 `behavior` 字段控制扩缩容速率
- 支持 `ContainerResource` 指标类型 (v1.27+)

### VerticalPodAutoscaler (VPA, 需单独安装)

```bash
# VPA 版本: v1.0+ (兼容 K8s v1.25-v1.32)
# 安装: https://github.com/kubernetes/autoscaler/tree/master/vertical-pod-autoscaler

# 查看 VPA
kubectl get vpa

# 查看 VPA 推荐值
kubectl describe vpa <vpa-name>
```

---

## 高级操作

### 批量操作

```bash
# 批量删除 Pod
kubectl delete pods -l app=nginx
kubectl delete pods --all -n <namespace>

# 批量重启 Deployment
for deploy in $(kubectl get deploy -o name); do kubectl rollout restart $deploy; done

# 批量导出资源 YAML
kubectl get pods -o yaml > all-pods.yaml
kubectl get all -A -o yaml > cluster-backup.yaml

# 批量应用配置
kubectl apply -f ./manifests/  # 应用目录下所有 YAML
kubectl apply -f manifest.yaml -R  # 递归应用子目录
```

### Patch 操作 (v1.25+)

```bash
# JSON Patch
kubectl patch deployment <deployment-name> --type='json' -p='[{"op": "replace", "path": "/spec/replicas", "value": 5}]'

# Merge Patch (默认)
kubectl patch deployment <deployment-name> -p '{"spec":{"replicas":5}}'

# Strategic Merge Patch
kubectl patch deployment <deployment-name> --type='strategic' -p '{"spec":{"template":{"metadata":{"labels":{"version":"v2"}}}}}'

# 删除字段 (设置为 null)
kubectl patch deployment <deployment-name> -p '{"spec":{"template":{"spec":{"nodeSelector":null}}}}'
```

### 资源预留与限制

```yaml
# Pod 资源请求和限制 (v1.25+)
spec:
  containers:
  - name: app
    resources:
      requests:
        cpu: 100m      # 0.1 CPU
        memory: 128Mi
      limits:
        cpu: 500m
        memory: 512Mi

# QoS 等级:
# - Guaranteed: requests == limits
# - Burstable: requests < limits
# - BestEffort: 未设置 requests/limits
```

### Admission Webhook (v1.25+)

```bash
# 查看 ValidatingWebhookConfiguration
kubectl get validatingwebhookconfigurations

# 查看 MutatingWebhookConfiguration
kubectl get mutatingwebhookconfigurations

# 查看 Webhook 详情
kubectl describe validatingwebhookconfiguration <webhook-name>

# 临时禁用 Webhook (调试用)
kubectl delete validatingwebhookconfiguration <webhook-name>
```

---

## etcd 操作

### etcd 版本兼容性
- **K8s v1.25-v1.27**: etcd v3.5.0+
- **K8s v1.28-v1.30**: etcd v3.5.9+
- **K8s v1.31-v1.32**: etcd v3.5.13+

### etcd 命令 (etcdctl v3)

```bash
# 设置 etcdctl API 版本
export ETCDCTL_API=3

# etcd 健康检查
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health

# 查看 etcd 成员
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  member list

# 查看 etcd 状态
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=table

# 备份 etcd (生产环境必备)
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  snapshot save /backup/etcd-snapshot-$(date +%Y%m%d-%H%M%S).db

# 恢复 etcd
etcdctl snapshot restore /backup/etcd-snapshot.db \
  --data-dir=/var/lib/etcd-restore

# 查看 etcd 数据库大小
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint status --write-out=table | grep "DB SIZE"

# 压缩 etcd 历史版本 (生产环境定期执行)
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  compact $(etcdctl endpoint status --write-out="json" | jq -r '.[0].Status.header.revision')

# 碎片整理 (compact 后执行)
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  defrag
```

### 通过 kubectl 访问 etcd 数据

```bash
# 查看所有 API 资源在 etcd 中的路径
kubectl get --raw /

# 查看特定资源
kubectl get --raw /api/v1/namespaces/default/pods

# 查看所有命名空间
kubectl get --raw /api/v1/namespaces | jq '.items[].metadata.name'
```

---

## API Server 管理

### API Server 版本

```bash
# 查看 API Server 版本
kubectl version --short
curl -k https://<api-server>:6443/version

# 查看支持的 API 版本
kubectl api-versions

# 查看所有 API 资源
kubectl api-resources --sort-by=name
kubectl api-resources --namespaced=true
kubectl api-resources --namespaced=false

# 查看 API 资源详细信息 (v1.25+)
kubectl explain pod
kubectl explain pod.spec
kubectl explain pod.spec.containers
kubectl explain deployment.spec.strategy.rollingUpdate
```

### API 请求

```bash
# 查看 API Server 地址
kubectl cluster-info | grep "Kubernetes control plane"

# 原始 API 请求
kubectl get --raw /api/v1/nodes
kubectl get --raw /apis/apps/v1/deployments
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes  # Metrics API

# 查看 API Server 审计日志位置
# /var/log/kubernetes/audit.log (默认)

# 启用审计策略 (需要在 kube-apiserver 参数中配置)
# --audit-policy-file=/etc/kubernetes/audit-policy.yaml
# --audit-log-path=/var/log/kubernetes/audit.log
# --audit-log-maxage=30
# --audit-log-maxbackup=10
# --audit-log-maxsize=100
```

### API Priority and Fairness (v1.25+ 默认启用)

```bash
# 查看 FlowSchema
kubectl get flowschemas

# 查看 PriorityLevelConfiguration
kubectl get prioritylevelconfigurations

# 查看 API 请求队列状态 (v1.26+)
kubectl get --raw /metrics | grep apiserver_flowcontrol
```

---

## 集群维护

### 证书管理 (kubeadm)

```bash
# 查看证书过期时间 (适用 kubeadm 集群)
kubeadm certs check-expiration

# 续期所有证书 (kubeadm v1.25+)
kubeadm certs renew all

# 续期单个证书
kubeadm certs renew apiserver
kubeadm certs renew apiserver-kubelet-client

# 生成新的 kubeconfig
kubeadm init phase kubeconfig admin
```

**证书路径** (kubeadm):
- `/etc/kubernetes/pki/` - 证书目录
- `/etc/kubernetes/admin.conf` - 管理员 kubeconfig

### 升级集群 (kubeadm)

```bash
# 查看升级计划 (K8s v1.25+)
kubeadm upgrade plan

# 升级控制平面 (第一个 master 节点)
kubeadm upgrade apply v1.31.0

# 升级控制平面 (其他 master 节点)
kubeadm upgrade node

# 升级 kubelet 和 kubectl
apt-mark unhold kubelet kubectl && \
apt-get update && apt-get install -y kubelet=1.31.0-00 kubectl=1.31.0-00 && \
apt-mark hold kubelet kubectl

systemctl daemon-reload
systemctl restart kubelet

# 升级工作节点
kubectl drain <node-name> --ignore-daemonsets
kubeadm upgrade node
apt-get update && apt-get install -y kubelet=1.31.0-00
systemctl daemon-reload && systemctl restart kubelet
kubectl uncordon <node-name>
```

**升级路径**:
- ⚠️ 每次只能升级一个小版本 (v1.30 → v1.31 ✅, v1.29 → v1.31 ❌)
- 先升级 kubeadm → 再升级控制平面 → 最后升级 kubelet

### 节点维护

```bash
# 节点维护流程
kubectl cordon <node-name>  # 1. 标记不可调度
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data  # 2. 驱逐 Pod
# 3. 执行维护操作 (重启、升级等)
kubectl uncordon <node-name>  # 4. 恢复调度

# 删除节点
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force
kubectl delete node <node-name>

# 在节点上执行 (删除前)
kubeadm reset
systemctl stop kubelet
```

### 清理资源

```bash
# 清理 Completed 状态的 Pod
kubectl delete pods --field-selector status.phase=Succeeded -A

# 清理 Evicted 状态的 Pod
kubectl get pods -A --field-selector status.phase=Failed | grep Evicted | awk '{print $1, $2}' | xargs -n2 kubectl delete pod -n

# 清理未使用的 PV (Released 状态)
kubectl get pv | grep Released | awk '{print $1}' | xargs kubectl delete pv

# 清理未绑定的 PVC
kubectl get pvc -A | grep Pending

# 清理孤儿 ReplicaSet (replicas=0)
kubectl get rs -A | awk '$3+$4+$5 == 0 {print $1, $2}' | xargs -n2 kubectl delete rs -n

# 清理老旧镜像 (在节点上执行)
crictl rmi --prune  # containerd
docker image prune -a  # Docker
```

### 备份与恢复

```bash
# 备份关键资源 (生产环境推荐定期执行)
kubectl get all --all-namespaces -o yaml > cluster-backup.yaml
kubectl get pv,pvc --all-namespaces -o yaml > storage-backup.yaml
kubectl get configmap,secret --all-namespaces -o yaml > config-backup.yaml
kubectl get crd -o yaml > crd-backup.yaml

# 备份 etcd (最重要)
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd-$(date +%Y%m%d).db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key

# 恢复资源
kubectl apply -f cluster-backup.yaml
```

---

## 常见故障速查

| 问题 | 快速排查命令 | 常见原因 |
|------|--------------|----------|
| Pod Pending | `kubectl describe pod <pod>` | 资源不足、节点污点、PVC 未绑定 |
| Pod CrashLoopBackOff | `kubectl logs <pod> --previous` | 应用崩溃、配置错误、依赖不可用 |
| ImagePullBackOff | `kubectl describe pod <pod>` | 镜像不存在、凭证错误、网络问题 |
| Service 无法访问 | `kubectl get endpoints <svc>` | Pod 未 Ready、标签不匹配 |
| Node NotReady | `kubectl describe node <node>` | kubelet 故障、网络问题、资源耗尽 |
| PVC Pending | `kubectl describe pvc <pvc>` | StorageClass 不存在、Provisioner 故障 |
| DNS 解析失败 | `kubectl logs -n kube-system -l k8s-app=kube-dns` | CoreDNS Pod 故障、NetworkPolicy 阻塞 |
| 滚动更新卡住 | `kubectl rollout status deploy/<deploy>` | 健康检查失败、资源不足、PDB 阻塞 |

---

## 生产环境最佳实践

### 资源配置

```yaml
# ✅ 推荐配置
resources:
  requests:
    cpu: 100m      # 保证调度
    memory: 128Mi
  limits:
    cpu: 500m      # 防止资源耗尽
    memory: 512Mi  # 触发 OOMKilled

# ❌ 避免
resources: {}  # 未设置资源限制

# ⚠️ 谨慎使用
resources:
  limits:
    cpu: 2000m  # 可能导致 CPU 限流
```

### 健康检查

```yaml
# ✅ 推荐配置
livenessProbe:
  httpGet:
    path: /health
    port: 8080
  initialDelaySeconds: 30  # 等待应用启动
  periodSeconds: 10
  timeoutSeconds: 5
  failureThreshold: 3

readinessProbe:
  httpGet:
    path: /ready
    port: 8080
  initialDelaySeconds: 5
  periodSeconds: 5
  timeoutSeconds: 3
  failureThreshold: 3

# v1.25+ 新增 startupProbe (慢启动应用)
startupProbe:
  httpGet:
    path: /health
    port: 8080
  initialDelaySeconds: 0
  periodSeconds: 10
  failureThreshold: 30  # 最多等待 300s
```

### 滚动更新策略

```yaml
# ✅ 推荐配置 (平衡速度和稳定性)
strategy:
  type: RollingUpdate
  rollingUpdate:
    maxUnavailable: 25%  # 允许 25% Pod 不可用
    maxSurge: 25%        # 允许超出 25% Pod

# minReadySeconds (防止快速失败)
minReadySeconds: 10

# progressDeadlineSeconds (超时检测)
progressDeadlineSeconds: 600  # 10 分钟
```

### 命名规范

```bash
# ✅ 推荐命名规范
# 资源名称: <app>-<component>-<env>
# 示例:
#   myapp-web-prod
#   myapp-cache-staging
#   myapp-db-dev

# 标签规范
labels:
  app: myapp
  component: web
  env: prod
  version: v1.2.3
  managed-by: kubectl
```

---

## 附录: 常用镜像版本

### 官方镜像

| 镜像 | 版本 | 用途 | 架构 |
|------|------|------|------|
| `nginx` | 1.25 | Web 服务器 | amd64, arm64 |
| `redis` | 7.2 | 缓存数据库 | amd64, arm64 |
| `postgres` | 16 | 关系数据库 | amd64, arm64 |
| `mysql` | 8.2 | 关系数据库 | amd64, arm64 |
| `mongo` | 7.0 | 文档数据库 | amd64, arm64 |

### 调试工具

| 镜像 | 版本 | 工具 |
|------|------|------|
| `busybox` | 1.36 | wget, ping, nslookup, vi |
| `curlimages/curl` | 8.5.0 | curl |
| `nicolaka/netshoot` | latest | tcpdump, iperf, nmap, dig |
| `alpine` | 3.19 | 轻量级 Linux |
| `ubuntu` | 22.04 | 完整 Linux 环境 |

---

**文档维护**: 建议每季度更新一次，确保版本兼容性  
**反馈渠道**: 如有错误或建议，请提交 Issue
