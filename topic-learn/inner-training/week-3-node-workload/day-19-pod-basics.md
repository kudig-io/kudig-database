# Day 19: Pod 容器组基础

> **学习时间**: 4-5 小时 | **主题**: Pod 生命周期与基本操作

---

## 今日目标

- [ ] 理解 Pod 在 ACK 集群中的核心地位
- [ ] 掌握 Pod 生命周期各阶段 (Pending → Running → Succeeded/Failed)
- [ ] 能够创建、查看、删除 Pod 并排查基本问题
- [ ] 了解 ACK 控制台中 Pod 管理视图

---

## 理论学习 (2h)

### 必读文档

1. **K8S Pod 基础概念**
   - 文件: `../../../domain-09-workload/01-pod-overview.md`
   - 重点: Pod 定义、多容器模型、共享网络/存储

2. **Pod 生命周期**
   - 文件: `../../../domain-09-workload/02-pod-lifecycle.md`
   - 重点: Pod Phase、Container States、重启策略

3. **ACK 工作负载管理**
   - 文件: `../../../domain-17-cloud-provider/04-alicloud-ack/250-ack-workload.md`
   - 重点: ACK 控制台中 Pod/Deployment 管理入口

### 阅读要点

- Pod 是 K8S 调度的最小单元，一个 Pod 包含 1 个或多个容器
- Pod Phase: `Pending` → `Running` → `Succeeded`/`Failed`
- Container States: `Waiting` / `Running` / `Terminated`
- restartPolicy: `Always`(默认) / `OnFailure` / `Never`
- Init Container 在主容器启动前执行，用于初始化

---

## 实践任务 (2.5h)

### 任务 1: Pod 创建与基本操作 (40min)

```bash
# 创建简单 Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: nginx-demo
  labels:
    app: nginx
    env: training
spec:
  containers:
  - name: nginx
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
    ports:
    - containerPort: 80
    resources:
      requests:
        cpu: 100m
        memory: 128Mi
      limits:
        cpu: 200m
        memory: 256Mi
EOF

# 查看 Pod 状态
kubectl get pod nginx-demo -o wide

# 查看 Pod 详细信息
kubectl describe pod nginx-demo

# 查看 Pod YAML
kubectl get pod nginx-demo -o yaml
```

### 任务 2: Pod 日志与调试 (40min)

```bash
# 查看 Pod 日志
kubectl logs nginx-demo

# 实时跟踪日志
kubectl logs nginx-demo -f

# 进入 Pod 容器执行命令
kubectl exec -it nginx-demo -- /bin/bash

# 在容器内检查
curl localhost:80
cat /etc/nginx/nginx.conf
exit

# 端口转发到本地
kubectl port-forward pod/nginx-demo 8080:80
# 新终端验证: curl localhost:8080
```

### 任务 3: 多容器 Pod (Sidecar 模式) (40min)

```bash
# 创建带 Sidecar 的 Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: sidecar-demo
spec:
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
    volumeMounts:
    - name: shared-logs
      mountPath: /var/log/nginx
  - name: log-collector
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
    command: ['sh', '-c', 'tail -f /var/log/nginx/access.log']
    volumeMounts:
    - name: shared-logs
      mountPath: /var/log/nginx
  volumes:
  - name: shared-logs
    emptyDir: {}
EOF

# 查看各容器状态
kubectl get pod sidecar-demo
kubectl describe pod sidecar-demo

# 分别查看容器日志
kubectl logs sidecar-demo -c app
kubectl logs sidecar-demo -c log-collector
```

### 任务 4: Pod 生命周期观察 (30min)

```bash
# 创建带 Init Container 的 Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: init-demo
spec:
  initContainers:
  - name: init-check
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
    command: ['sh', '-c', 'echo "Init completed at $(date)" && sleep 5']
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:1.24
EOF

# 观察 Pod 从 Init 到 Running 的过程
kubectl get pod init-demo -w

# 查看 Events 了解调度与拉镜像过程
kubectl describe pod init-demo | grep -A 20 "Events:"

# 清理
kubectl delete pod nginx-demo sidecar-demo init-demo
```

---

## 费曼复述 (0.5h)

1. **Pod 为什么是 K8S 的最小调度单元，而不是容器？**
2. **Pod 中多个容器如何共享网络和存储？**
3. **Init Container 与普通容器有什么区别，适用于哪些场景？**

---

## 今日检验

- [ ] 能使用 kubectl 创建、查看、删除 Pod
- [ ] 能查看 Pod 日志和进入容器调试
- [ ] 理解多容器 Pod (Sidecar) 模式
- [ ] 了解 Init Container 和 Pod 生命周期阶段

---

## 核心概念总结

| 概念 | 说明 | 常用命令 |
|------|------|---------|
| Pod Phase | 描述 Pod 整体状态 | `kubectl get pod` |
| Container State | 描述容器运行状态 | `kubectl describe pod` |
| Init Container | 在主容器前运行的初始化容器 | 在 spec.initContainers 定义 |
| Sidecar | 辅助容器，与主容器共享资源 | 在 spec.containers 中追加 |
| restartPolicy | Pod 容器重启策略 | Always / OnFailure / Never |

---

## 明日预告

Day 20 将学习 Pod 调度策略、健康探针配置与资源管理进阶。
