# 157 - Pod 全面故障排查 (Pod Comprehensive Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-01

---

## 一、Pod 状态诊断总览 (Pod Status Overview)

### 1.1 Pod 生命周期状态

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Pod 生命周期状态图                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐                │
│   │ Pending │───▶│ Running │───▶│Succeeded│    │  Failed │                │
│   └────┬────┘    └────┬────┘    └─────────┘    └─────────┘                │
│        │              │                              ▲                      │
│        │              │         ┌─────────┐          │                      │
│        │              └────────▶│  Failed │──────────┘                      │
│        │                        └─────────┘                                 │
│        │                                                                    │
│        │         ┌─────────────────────────────────────────┐               │
│        └────────▶│            常见卡住状态                   │               │
│                  │  • Pending (调度/镜像/资源)              │               │
│                  │  • ContainerCreating (CNI/存储)          │               │
│                  │  • CrashLoopBackOff (应用错误)           │               │
│                  │  • ImagePullBackOff (镜像拉取)           │               │
│                  │  • Init:Error (初始化失败)               │               │
│                  │  • Terminating (删除卡住)                │               │
│                  └─────────────────────────────────────────┘               │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 Pod 状态速查表

| 状态 | 含义 | 常见原因 | 优先检查项 |
|:---|:---|:---|:---|
| **Pending** | 等待调度 | 资源不足/节点选择器/亲和性/污点 | `kubectl describe pod` Events |
| **ContainerCreating** | 创建容器中 | CNI/存储挂载/镜像拉取 | Events + kubelet日志 |
| **Running** | 运行中 | - | 正常状态 |
| **CrashLoopBackOff** | 反复崩溃 | 应用错误/配置错误/依赖缺失 | `kubectl logs --previous` |
| **ImagePullBackOff** | 镜像拉取失败 | 镜像不存在/凭证错误/网络问题 | 镜像名/Secret/网络 |
| **Init:Error** | 初始化失败 | InitContainer错误 | InitContainer日志 |
| **Terminating** | 删除中 | Finalizer/PV卸载/preStop阻塞 | Finalizers/Events |
| **Unknown** | 状态未知 | 节点失联/kubelet故障 | 节点状态/kubelet |
| **Evicted** | 被驱逐 | 资源压力/节点维护 | 节点状态/资源 |
| **OOMKilled** | 内存溢出 | 内存超限 | 容器limits/应用内存 |

---

## 二、Pending 状态排查 (Pending Troubleshooting)

### 2.1 排查流程图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         Pod Pending 排查流程                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────┐                                                           │
│   │ Pod Pending │                                                           │
│   └──────┬──────┘                                                           │
│          │                                                                  │
│          ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ kubectl describe pod <name> -n <ns> | grep -A20 Events│                 │
│   └──────────────────────────────────────────────────────┘                 │
│          │                                                                  │
│          ├─── "Insufficient cpu/memory" ──▶ 资源不足                        │
│          │         └─▶ 扩容节点/调整requests/检查LimitRange                  │
│          │                                                                  │
│          ├─── "node(s) didn't match Pod's node selector" ──▶ 节点选择器     │
│          │         └─▶ 检查nodeSelector/nodeAffinity                        │
│          │                                                                  │
│          ├─── "node(s) had taint ... that pod didn't tolerate" ──▶ 污点     │
│          │         └─▶ 添加tolerations或移除节点taint                       │
│          │                                                                  │
│          ├─── "persistentvolumeclaim not found" ──▶ PVC问题                 │
│          │         └─▶ 检查PVC状态/StorageClass                             │
│          │                                                                  │
│          ├─── "no nodes available to schedule" ──▶ 无可用节点               │
│          │         └─▶ 检查节点状态/cordoned                                │
│          │                                                                  │
│          └─── "pod has unbound PersistentVolumeClaims" ──▶ PVC未绑定        │
│                    └─▶ 检查PV/PVC绑定状态                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Pending 原因分类

| 原因类型 | 事件关键字 | 排查命令 | 解决方案 |
|:---|:---|:---|:---|
| **资源不足** | Insufficient cpu/memory | `kubectl describe nodes \| grep -A5 Allocated` | 扩容/调整requests |
| **节点选择** | didn't match node selector | `kubectl get nodes --show-labels` | 修改selector/添加标签 |
| **污点容忍** | had taint...didn't tolerate | `kubectl describe nodes \| grep Taints` | 添加tolerations |
| **亲和性** | didn't match pod affinity | `kubectl get pods -o wide` | 调整affinity规则 |
| **PVC绑定** | unbound PersistentVolumeClaims | `kubectl get pvc` | 创建PV/检查StorageClass |
| **配额限制** | exceeded quota | `kubectl describe quota -n <ns>` | 调整quota/清理资源 |
| **调度器** | no nodes available | `kubectl get nodes` | 检查节点状态 |

### 2.3 常用排查命令

```bash
# === 查看Pod详情和Events ===
kubectl describe pod <pod-name> -n <namespace>

# === 查看调度失败原因 ===
kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name>

# === 检查节点资源 ===
kubectl describe nodes | grep -A10 "Allocated resources"

# === 检查节点污点 ===
kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints

# === 检查节点标签 ===
kubectl get nodes --show-labels

# === 检查PVC状态 ===
kubectl get pvc -n <namespace>

# === 检查ResourceQuota ===
kubectl describe quota -n <namespace>

# === 模拟调度 (dry-run) ===
kubectl run test-pod --image=nginx --dry-run=server -o yaml
```

---

## 三、ContainerCreating 排查 (ContainerCreating Troubleshooting)

### 3.1 常见原因

| 原因 | 事件关键字 | 排查方法 |
|:---|:---|:---|
| **CNI错误** | network plugin, failed to set up pod network | 检查CNI Pod状态/配置 |
| **镜像拉取** | pulling image, ErrImagePull | 检查镜像名/凭证/网络 |
| **存储挂载** | Unable to attach/mount volume | 检查PV/PVC/CSI |
| **Secret不存在** | secret not found | 检查Secret是否存在 |
| **ConfigMap不存在** | configmap not found | 检查ConfigMap是否存在 |

### 3.2 CNI 问题排查

```bash
# === 检查CNI Pod状态 ===
kubectl get pods -n kube-system -l k8s-app=calico-node  # Calico
kubectl get pods -n kube-system -l app=flannel          # Flannel
kubectl get pods -n kube-system -l app=terway           # Terway

# === 检查CNI日志 ===
kubectl logs -n kube-system -l k8s-app=calico-node --tail=100

# === 检查节点CNI配置 ===
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/10-calico.conflist

# === 检查kubelet日志 ===
journalctl -u kubelet --since "10 minutes ago" | grep -i cni
```

### 3.3 存储挂载问题排查

```bash
# === 检查PVC状态 ===
kubectl get pvc -n <namespace>
kubectl describe pvc <pvc-name> -n <namespace>

# === 检查PV状态 ===
kubectl get pv
kubectl describe pv <pv-name>

# === 检查CSI驱动 ===
kubectl get csidrivers
kubectl get pods -n kube-system -l app=csi-provisioner

# === 检查节点存储设备 ===
lsblk
df -h
```

---

## 四、CrashLoopBackOff 排查 (CrashLoopBackOff Troubleshooting)

### 4.1 排查流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                     CrashLoopBackOff 排查流程                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌──────────────────┐                                                      │
│   │ CrashLoopBackOff │                                                      │
│   └────────┬─────────┘                                                      │
│            │                                                                │
│            ▼                                                                │
│   ┌─────────────────────────────────────────────────────┐                  │
│   │ kubectl logs <pod> --previous (查看上次崩溃日志)    │                  │
│   └─────────────────────────────────────────────────────┘                  │
│            │                                                                │
│            ├─── 有应用错误日志 ──▶ 分析应用错误                             │
│            │         └─▶ 修复代码/配置                                      │
│            │                                                                │
│            ├─── 无日志/启动即退出 ──▶ 检查启动命令                          │
│            │         └─▶ 检查command/args/entrypoint                        │
│            │                                                                │
│            ├─── "exec format error" ──▶ 架构不匹配                          │
│            │         └─▶ 检查镜像架构(amd64/arm64)                          │
│            │                                                                │
│            ├─── "permission denied" ──▶ 权限问题                            │
│            │         └─▶ 检查securityContext/文件权限                       │
│            │                                                                │
│            ├─── OOMKilled ──▶ 内存不足                                      │
│            │         └─▶ 增加memory limits                                  │
│            │                                                                │
│            └─── "cannot find..." ──▶ 依赖缺失                               │
│                      └─▶ 检查ConfigMap/Secret/环境变量                      │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 4.2 常用排查命令

```bash
# === 查看当前日志 ===
kubectl logs <pod-name> -n <namespace>

# === 查看上一次崩溃的日志 ===
kubectl logs <pod-name> -n <namespace> --previous

# === 查看所有容器日志 ===
kubectl logs <pod-name> -n <namespace> --all-containers

# === 实时跟踪日志 ===
kubectl logs <pod-name> -n <namespace> -f

# === 查看容器退出码 ===
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[0].lastState.terminated.exitCode}'

# === 进入运行中的容器调试 ===
kubectl exec -it <pod-name> -n <namespace> -- /bin/sh

# === 使用临时调试容器 (K8s 1.25+) ===
kubectl debug <pod-name> -n <namespace> -it --image=busybox --target=<container-name>
```

### 4.3 退出码含义

| 退出码 | 含义 | 常见原因 | 解决方案 |
|:---:|:---|:---|:---|
| **0** | 正常退出 | 任务完成/主进程结束 | 检查是否应该持续运行 |
| **1** | 通用错误 | 应用错误 | 查看日志分析 |
| **126** | 命令不可执行 | 权限问题 | 检查文件权限 |
| **127** | 命令未找到 | 路径错误/命令不存在 | 检查command配置 |
| **128+N** | 信号N终止 | 被信号终止 | 查看信号原因 |
| **137** | SIGKILL(128+9) | OOMKilled/强制终止 | 检查内存/手动删除 |
| **139** | SIGSEGV(128+11) | 段错误 | 应用bug |
| **143** | SIGTERM(128+15) | 优雅终止 | 正常关闭信号 |

---

## 五、ImagePullBackOff 排查 (ImagePullBackOff Troubleshooting)

### 5.1 常见原因及解决

| 原因 | 错误信息 | 排查方法 | 解决方案 |
|:---|:---|:---|:---|
| **镜像不存在** | manifest unknown | `docker pull <image>` | 检查镜像名/tag |
| **私有仓库认证** | unauthorized | 检查imagePullSecrets | 创建/更新Secret |
| **网络问题** | timeout/connection refused | 检查网络连通性 | 配置代理/镜像加速 |
| **Tag不存在** | tag not found | 检查tag是否存在 | 使用正确的tag |
| **仓库地址错误** | server not found | 检查registry地址 | 修正仓库地址 |

### 5.2 排查命令

```bash
# === 查看拉取错误详情 ===
kubectl describe pod <pod-name> -n <namespace> | grep -A10 "Events"

# === 检查imagePullSecrets ===
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.imagePullSecrets}'

# === 检查Secret内容 ===
kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d

# === 手动测试镜像拉取 ===
docker pull <image-name>
crictl pull <image-name>

# === 检查ServiceAccount的imagePullSecrets ===
kubectl get sa <sa-name> -n <namespace> -o jsonpath='{.imagePullSecrets}'
```

### 5.3 创建镜像拉取Secret

```bash
# === 创建docker-registry类型Secret ===
kubectl create secret docker-registry <secret-name> \
  --docker-server=<registry-server> \
  --docker-username=<username> \
  --docker-password=<password> \
  --docker-email=<email> \
  -n <namespace>

# === 在Pod中使用 ===
spec:
  imagePullSecrets:
  - name: <secret-name>
```

---

## 六、Terminating 状态排查 (Terminating Troubleshooting)

### 6.1 删除卡住的原因

| 原因 | 检查方法 | 解决方案 |
|:---|:---|:---|
| **Finalizers** | `kubectl get pod -o jsonpath='{.metadata.finalizers}'` | 移除finalizers |
| **PV卸载失败** | `kubectl describe pod` Events | 检查存储/手动卸载 |
| **preStop Hook阻塞** | 检查preStop配置 | 调整/移除preStop |
| **节点失联** | `kubectl get nodes` | 恢复节点/强制删除 |
| **容器无法停止** | kubelet日志 | 检查应用信号处理 |

### 6.2 强制删除命令

```bash
# === 标准删除 ===
kubectl delete pod <pod-name> -n <namespace>

# === 设置删除超时 ===
kubectl delete pod <pod-name> -n <namespace> --grace-period=30

# === 强制删除 (谨慎使用) ===
kubectl delete pod <pod-name> -n <namespace> --force --grace-period=0

# === 移除Finalizers后删除 ===
kubectl patch pod <pod-name> -n <namespace> -p '{"metadata":{"finalizers":null}}'
kubectl delete pod <pod-name> -n <namespace>

# === 检查是否有Finalizers ===
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.metadata.finalizers}'
```

---

## 七、Init Container 故障排查 (Init Container Troubleshooting)

### 7.1 Init 状态含义

| 状态 | 含义 | 排查方法 |
|:---|:---|:---|
| **Init:0/N** | 第一个InitContainer运行中 | 查看第一个init日志 |
| **Init:Error** | InitContainer失败 | 查看失败的init日志 |
| **Init:CrashLoopBackOff** | InitContainer反复失败 | 查看init --previous日志 |
| **PodInitializing** | Init完成,主容器启动中 | 等待或检查主容器 |

### 7.2 排查命令

```bash
# === 查看Init Container日志 ===
kubectl logs <pod-name> -c <init-container-name> -n <namespace>

# === 查看上次Init失败日志 ===
kubectl logs <pod-name> -c <init-container-name> -n <namespace> --previous

# === 查看所有Init Container ===
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.initContainers[*].name}'

# === 查看Init Container状态 ===
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.initContainerStatuses}'
```

---

## 八、资源相关问题 (Resource Issues)

### 8.1 OOMKilled 排查

```bash
# === 检查是否OOMKilled ===
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[0].lastState.terminated.reason}'

# === 查看当前内存使用 ===
kubectl top pod <pod-name> -n <namespace>

# === 查看资源限制 ===
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[0].resources}'

# === 查看节点内存压力 ===
kubectl describe node <node-name> | grep -A5 "Conditions"
```

### 8.2 CPU Throttling 排查

```bash
# === 查看CPU使用 ===
kubectl top pod <pod-name> -n <namespace>

# === 检查容器CFS配额 ===
# 进入节点
cat /sys/fs/cgroup/cpu/kubepods/pod<pod-uid>/<container-id>/cpu.cfs_quota_us
cat /sys/fs/cgroup/cpu/kubepods/pod<pod-uid>/<container-id>/cpu.cfs_period_us

# === 查看throttle统计 ===
cat /sys/fs/cgroup/cpu/kubepods/pod<pod-uid>/<container-id>/cpu.stat
```

### 8.3 资源问题解决方案

| 问题 | 症状 | 解决方案 |
|:---|:---|:---|
| **OOMKilled** | 容器被杀,exit code 137 | 增加memory limits |
| **CPU Throttling** | 响应慢但不被杀 | 增加cpu limits或改为burstable |
| **Evicted** | Pod被驱逐 | 检查节点资源压力,设置priority |
| **资源不足Pending** | 无法调度 | 扩容节点或优化requests |

---

## 九、网络相关问题 (Network Issues)

### 9.1 Pod网络排查

```bash
# === 检查Pod IP ===
kubectl get pod <pod-name> -n <namespace> -o wide

# === 进入Pod测试网络 ===
kubectl exec -it <pod-name> -n <namespace> -- /bin/sh

# === 测试DNS解析 ===
kubectl exec -it <pod-name> -n <namespace> -- nslookup kubernetes.default

# === 测试Service连通性 ===
kubectl exec -it <pod-name> -n <namespace> -- wget -qO- http://<service-name>:<port>

# === 检查NetworkPolicy ===
kubectl get networkpolicy -n <namespace>
```

### 9.2 常见网络问题

| 问题 | 症状 | 排查方法 |
|:---|:---|:---|
| **Pod无IP** | IP为空/ContainerCreating | 检查CNI |
| **DNS失败** | 无法解析服务名 | 检查CoreDNS |
| **Service不通** | 连接超时 | 检查Endpoints/kube-proxy |
| **跨节点不通** | 跨节点Pod无法通信 | 检查CNI网络/防火墙 |
| **NetworkPolicy阻断** | 部分流量被阻断 | 检查NetworkPolicy规则 |

---

## 十、调试技巧 (Debugging Tips)

### 10.1 临时调试容器

```yaml
# K8s 1.25+ Ephemeral Container
kubectl debug <pod-name> -it --image=nicolaka/netshoot --target=<container-name>

# 复制Pod进行调试
kubectl debug <pod-name> -it --copy-to=debug-pod --container=debug --image=busybox
```

### 10.2 常用调试镜像

| 镜像 | 用途 | 工具 |
|:---|:---|:---|
| `busybox` | 基础调试 | sh, wget, nc |
| `nicolaka/netshoot` | 网络调试 | tcpdump, dig, curl, iperf |
| `alpine` | 轻量调试 | sh, apk |
| `ubuntu` | 完整调试 | apt, 各种工具 |

### 10.3 一键诊断脚本

```bash
#!/bin/bash
POD=$1
NS=${2:-default}

echo "=== Pod Status ==="
kubectl get pod $POD -n $NS -o wide

echo -e "\n=== Pod Events ==="
kubectl describe pod $POD -n $NS | grep -A20 "Events:"

echo -e "\n=== Container Status ==="
kubectl get pod $POD -n $NS -o jsonpath='{range .status.containerStatuses[*]}{.name}{"\t"}{.state}{"\n"}{end}'

echo -e "\n=== Recent Logs (last 50 lines) ==="
kubectl logs $POD -n $NS --tail=50 2>/dev/null || echo "No logs available"

echo -e "\n=== Resource Usage ==="
kubectl top pod $POD -n $NS 2>/dev/null || echo "Metrics not available"
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
