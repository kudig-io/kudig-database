---
title: 命令输出解读语料 — Agent 诊断推理核心数据 [故障诊断]
description: kubectl/系统命令输出→诊断结论的结构化映射, 供 Agent 直接用于问题推理
summary: kubectl/系统命令输出→诊断结论的结构化映射, 供 Agent 直接用于问题推理
category: agent-corpus
source: fta-skills  # M3: 来源标注（由 08-技能体系 Skills 与 06-FTA故障树 FTA 自动提取生成）
tags:
- k8s
- troubleshooting
- command-output
- diagnosis
- agent
- corpus
- etcd
- apiserver
- kubelet
- scheduler
tier: supporting
created: '2026-05-23'
last_updated: '2026-05-19'
difficulty: advanced
reading_level: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 5min
intent_queries:
- kubectl get pods 输出异常怎么解读
- kubectl describe node NotReady 怎么排查
- Pod CrashLoopBackOff 日志怎么看
- kubectl top 显示资源不足怎么处理
- etcd 健康检查失败怎么诊断
trigger_keywords:
- 命令输出
- kubectl
- 诊断
- 故障排查
- 日志解读
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 命令输出解读语料 — Agent 诊断推理核心数据

> **用途**: Agent 接收命令输出后, 直接匹配诊断结论
> **格式**: 每条记录包含 command → output_pattern → diagnosis → action
> **最后更新**: 2026-05-19

---

## 1. Pod 状态异常诊断

### 1.1 CrashLoopBackOff

```yaml
command: "kubectl get pods -A"
output_pattern: |
  NAME                    READY   STATUS             RESTARTS      AGE
  myapp-xxxx-yyyy         0/1     CrashLoopBackOff   6 (32s ago)   5m
diagnosis:
  - "应用启动失败后反复重启, RESTARTS 持续增长"
  - "常见原因: 启动命令错误、依赖服务未就绪、配置文件缺失、内存不足 OOMKilled"
  - "32s ago 表示最后一次重启距今 32 秒, 指数退避中"
action:
  - "kubectl logs myapp-xxxx-yyyy --previous  # 查看上次崩溃日志"
  - "kubectl describe pod myapp-xxxx-yyyy     # 查看 Events 和 Last State"
  - "检查 livenessProbe 配置是否过于严格"
severity: high
```

### 1.2 ImagePullBackOff

```yaml
command: "kubectl get pods -A"
output_pattern: |
  NAME                    READY   STATUS             RESTARTS   AGE
  myapp-xxxx-yyyy         0/1     ImagePullBackOff   0          2m
diagnosis:
  - "镜像拉取失败, [[23-实体/02-K8s核心组件/kubernetes.md|k8s]] 正在退避重试"
  - "常见原因: 镜像名/tag 拼写错误、私有仓库认证失败、网络不通、镜像不存在"
action:
  - "kubectl describe pod myapp-xxxx-yyyy | grep -A5 Events"
  - "检查 imagePullSecrets 配置"
  - "手动 docker pull 验证镜像可达性"
severity: high
```

### 1.3 Pending (调度失败)

```yaml
command: "kubectl get pods -A"
output_pattern: |
  NAME                    READY   STATUS    RESTARTS   AGE
  myapp-xxxx-yyyy         0/1     Pending   0          10m
diagnosis:
  - "Pod 未被调度到任何节点"
  - "常见原因: 资源不足、nodeSelector/affinity 无匹配节点、PVC 未绑定、污点未容忍"
action:
  - "kubectl describe pod myapp-xxxx-yyyy | grep -A10 Events"
  - "kubectl get nodes -o wide  # 检查节点状态"
  - "kubectl describe node <node> | grep -A5 Allocatable"
  - "检查 resource requests 是否超过节点可分配资源"
severity: high
```

### 1.4 OOMKilled

```yaml
command: "kubectl describe pod myapp-xxxx-yyyy"
output_pattern: |
  Last State: Terminated
    Reason: OOMKilled
    Exit Code: 137
diagnosis:
  - "容器因内存超限被内核 OOM Killer 终止"
  - "Exit Code 137 = 128 + 9 (SIGKILL)"
  - "limits.memory 设置过低或应用内存泄漏"
action:
  - "检查 resources.limits.memory 配置"
  - "kubectl top pod myapp-xxxx-yyyy  # 查看实际内存使用"
  - "增大 limits.memory 或优化应用内存使用"
severity: high
```

### 1.5 Init:Error / Init:CrashLoopBackOff

```yaml
command: "kubectl get pods -A"
output_pattern: |
  NAME                    READY   STATUS             RESTARTS   AGE
  myapp-xxxx-yyyy         0/1     Init:Error         3          2m
diagnosis:
  - "Init Container 执行失败, 主容器未启动"
  - "Init Container 按顺序执行, 任一失败则阻塞"
action:
  - "kubectl logs myapp-xxxx-yyyy -c <init-container-name>  # 查看 init 容器日志"
  - "kubectl describe pod myapp-xxxx-yyyy  # 查看 init container 状态"
severity: high
```

---

## 2. Node 状态异常诊断

### 2.1 NotReady

```yaml
command: "kubectl get nodes"
output_pattern: |
  NAME       STATUS     ROLES           AGE   VERSION
  node-01    NotReady   worker          5d    v1.32.0
diagnosis:
  - "节点不健康, 可能原因: kubelet 挂掉、网络不通、磁盘压力、PID 压力"
  - "NotReady 超过 pod-eviction-timeout (默认5min) 后, Pod 会被驱逐"
action:
  - "kubectl describe node node-01 | grep -A10 Conditions"
  - "ssh node-01 'systemctl status kubelet'"
  - "ssh node-01 'journalctl -u kubelet --since 5min'"
  - "检查节点磁盘: ssh node-01 'df -h'"
  - "检查节点内存: ssh node-01 'free -h'"
severity: critical
```

### 2.2 DiskPressure

```yaml
command: "kubectl describe node node-01"
output_pattern: |
  Conditions:
    Type             Status  Reason
    DiskPressure     True    KubeletHasDiskPressure
diagnosis:
  - "节点磁盘空间不足 (默认阈值: 可用 < 15%)"
  - "kubelet 会自动驱逐 Pod 释放空间"
action:
  - "ssh node-01 'df -h /var/lib/docker /var/lib/kubelet'"
  - "ssh node-01 'du -sh /var/log/*'  # 检查日志占用"
  - "清理无用镜像: crictl rmi --prune"
  - "检查 emptyDir 卷是否过大"
severity: high
```

### 2.3 MemoryPressure

```yaml
command: "kubectl describe node node-01"
output_pattern: |
  Conditions:
    Type             Status  Reason
    MemoryPressure   True    KubeletHasMemoryPressure
diagnosis:
  - "节点可用内存不足 (默认阈值: 可用 < 100Mi)"
  - "kubelet 会触发 Pod 驱逐"
action:
  - "ssh node-01 'free -h'"
  - "kubectl top nodes  # 查看资源使用"
  - "检查是否有内存泄漏的 Pod"
  - "考虑增加节点或调整 Pod limits"
severity: high
```

### 2.4 PIDPressure

```yaml
command: "kubectl describe node node-01"
output_pattern: |
  Conditions:
    Type             Status  Reason
    PIDPressure      True    KubeletHasPIDPressure
diagnosis:
  - "节点 PID 数量接近上限 (默认阈值: 可用 < 1000)"
  - "常见原因: 应用创建过多线程/进程, fork bomb"
action:
  - "ssh node-01 'ps aux | wc -l'"
  - "ssh node-01 'ps aux --sort=-%mem | head -20'"
  - "检查 kubelet --pod-max-pids 配置"
severity: high
```

---

## 3. [[etcd|etcd]] 诊断

### 3.1 etcd 健康检查失败

```yaml
command: "etcdctl endpoint health --cluster"
output_pattern: |
  https://etcd-0:2379 is healthy: successfully committed proposal: took = 12.345ms
  https://etcd-1:2379 is unhealthy: Error on endpoint: context deadline exceeded
  https://etcd-2:2379 is healthy: successfully committed proposal: took = 15.678ms
diagnosis:
  - "etcd-1 不健康, 可能原因: 网络分区、磁盘 IO 慢、进程挂死"
  - "2/3 节点健康时集群仍可用, 但需尽快修复"
action:
  - "ssh etcd-1 'systemctl status etcd'"
  - "ssh etcd-1 'journalctl -u etcd --since 5min'"
  - "检查磁盘 IO: ssh etcd-1 'iostat -x 1 3'"
  - "检查网络: ping etcd-1, telnet etcd-1 2379"
severity: critical
```

### 3.2 etcd Leader 切换频繁

```yaml
command: "etcdctl endpoint status --write-out=table"
output_pattern: |
  +------------------+----+---------+---------+-----------+------------+
  |    ENDPOINT      | ID | VERSION | DB SIZE | IS LEADER | RAFT TERM  |
  +------------------+----+---------+---------+-----------+------------+
  | etcd-0:2379      |  1 |  3.5.x  |  2.1 GB |     true  |    15234   |
  | etcd-1:2379      |  2 |  3.5.x  |  2.1 GB |    false  |    15234   |
  | etcd-2:2379      |  3 |  3.5.x  |  2.1 GB |    false  |    15234   |
  # Raft Term 异常高 (> 1000 次/天)
diagnosis:
  - "Raft Term 过高说明 Leader 频繁切换"
  - "常见原因: 网络抖动、磁盘 IO 慢、CPU 不足"
  - "Leader 切换会导致 API Server 短暂不可用"
action:
  - "etcdctl --endpoints=https://etcd-0:2379 endpoint health --write-out=table"
  - "检查各节点磁盘延迟: etcd_disk_wal_fsync_duration_seconds"
  - "检查网络延迟: etcd_network_peer_round_trip_time_seconds"
severity: high
```

### 3.3 etcd DB 大小超限

```yaml
command: "etcdctl endpoint status --write-out=table"
output_pattern: |
  | etcd-0:2379 | 1 | 3.5.x | 8.2 GB | true | 1234 |
  # DB SIZE 接近 --quota-backend-bytes (默认 8GB)
diagnosis:
  - "etcd DB 接近配额上限, 可能导致写入失败"
  - "常见原因: 未配置 compaction、资源版本泄漏、ConfigMap/Secret 过多"
action:
  - "etcdctl compact $(etcdctl endpoint status --write-out=json | jq '.[0].Status.header.revision')"
  - "etcdctl defrag --endpoints=https://etcd-0:2379"
  - "检查 --quota-backend-bytes 配置, 生产建议 8-16GB"
  - "清理不必要的 CRD、ConfigMap、Secret"
severity: critical
```

---

## 4. 网络诊断

### 4.1 [[service|Service]] 无法访问

```yaml
command: "kubectl get svc myapp"
output_pattern: |
  NAME    TYPE        CLUSTER-IP     EXTERNAL-IP   PORT(S)   AGE
  myapp   ClusterIP   10.96.100.50   <none>        80/TCP    2d
# 从 Pod 内 curl http://myapp 无响应
diagnosis:
  - "Service 存在但后端 Pod 无响应"
  - "常见原因: selector 不匹配、Pod 未就绪、端口不匹配、NetworkPolicy 阻断"
action:
  - "kubectl get endpoints myapp  # 检查是否有 endpoint"
  - "kubectl describe svc myapp   # 检查 selector"
  - "kubectl get pods -l app=myapp  # 检查 Pod 标签"
  - "从 Pod 内 curl http://10.96.100.50:80  # 直接 IP 测试"
  - "检查 NetworkPolicy: kubectl get networkpolicy -A"
severity: high
```

### 4.2 [[coredns|CoreDNS]] 解析失败

```yaml
command: "kubectl get pods -n kube-system -l k8s-app=kube-dns"
output_pattern: |
  NAME                       READY   STATUS             RESTARTS   AGE
  coredns-xxxx-yyyy          0/1     CrashLoopBackOff   5          3m
diagnosis:
  - "CoreDNS 不可用, 所有 Service DNS 解析失败"
  - "影响: 所有依赖 Service 名称的通信中断"
action:
  - "kubectl logs -n kube-system coredns-xxxx-yyyy"
  - "kubectl describe pod -n kube-system coredns-xxxx-yyyy"
  - "检查 CoreDNS ConfigMap: kubectl get cm -n kube-system coredns -o yaml"
  - "临时测试: 从 Pod 内直接 curl http://<ClusterIP>:<port>"
severity: critical
```

### 4.3 [[ingress|Ingress]] 502 Bad Gateway

```yaml
command: "curl -I https://myapp.example.com"
output_pattern: |
  HTTP/2 502
  server: nginx
diagnosis:
  - "Ingress Controller 能接收请求但后端无响应"
  - "常见原因: 后端 Pod 未就绪、Service 端口错误、健康检查失败"
action:
  - "kubectl get ingress myapp -o yaml  # 检查 backend 配置"
  - "kubectl get endpoints <service-name>  # 检查 endpoint"
  - "kubectl logs -n ingress-nginx <ingress-controller-pod>  # 查看 Ingress 日志"
  - "检查 backend 的 readinessProbe"
severity: high
```

---

## 5. 存储诊断

### 5.1 PVC Pending

```yaml
command: "kubectl get pvc"
output_pattern: |
  NAME       STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS   AGE
  my-pvc     Pending                                      gp3            5m
diagnosis:
  - "PVC 未绑定 PV, 无可用存储资源"
  - "常见原因: StorageClass 不存在、PV 不足、zone 不匹配、CSI 驱动异常"
action:
  - "kubectl describe pvc my-pvc  # 查看 Events"
  - "kubectl get sc  # 检查 StorageClass"
  - "kubectl get pv  # 检查可用 PV"
  - "检查 CSI 驱动: kubectl get pods -n kube-system | grep csi"
severity: high
```

### 5.2 Pod 挂载 Volume 失败

```yaml
command: "kubectl describe pod myapp-xxxx-yyyy"
output_pattern: |
  Events:
    Warning  FailedMount  2m (x8 over 5m)  kubelet  MountVolume.SetUp failed for volume "pvc-xxx" : ...
diagnosis:
  - "Volume 挂载失败, Pod 无法启动"
  - "常见原因: CSI 驱动异常、节点网络不通存储后端、NFS server 不可达"
action:
  - "kubectl get pods -n kube-system | grep csi  # 检查 CSI 驱动"
  - "ssh <node> 'mount | grep <volume>'  # 检查节点挂载状态"
  - "检查存储后端连通性"
severity: high
```

---

## 6. 资源诊断

### 6.1 资源不足 (Insufficient CPU/Memory)

```yaml
command: "kubectl describe pod myapp-xxxx-yyyy"
output_pattern: |
  Events:
    Warning  FailedScheduling  1m  default-scheduler  0/3 nodes are available:
      3 Insufficient cpu.
diagnosis:
  - "所有节点 CPU 资源不足, 无法调度新 Pod"
  - "Pod 的 resources.requests.cpu 超过节点可分配量"
action:
  - "kubectl top nodes  # 查看节点资源使用"
  - "kubectl describe node <node> | grep -A5 Allocatable"
  - "降低 Pod 的 resources.requests 或扩容节点"
severity: high
```

### 6.2 ResourceQuota 超限

```yaml
command: "kubectl describe pod myapp-xxxx-yyyy"
output_pattern: |
  Events:
    Warning  FailedCreate  1m  replicaset-controller  Error creating: pods "myapp-xxx" is forbidden:
      exceeded quota: my-quota, requested: cpu=2, used: cpu=8, limited: cpu=10
diagnosis:
  - "命名空间 ResourceQuota 已用尽"
  - "当前已用 8 CPU, 请求 2 CPU, 上限 10 CPU"
action:
  - "kubectl get resourcequota -n <namespace>  # 查看配额"
  - "kubectl describe resourcequota my-quota -n <namespace>"
  - "清理不用的 Pod 或申请提高配额"
severity: medium
```

---

## 7. 控制平面诊断

### 7.1 API Server 响应慢

```yaml
command: "kubectl get --raw /metrics | grep apiserver_request_duration"
output_pattern: |
  apiserver_request_duration_seconds{verb="GET",resource="pods",quantile="0.99"} 5.2
  # P99 延迟 > 5s (正常 < 1s)
diagnosis:
  - "API Server 响应严重延迟"
  - "常见原因: etcd 延迟高、webhook 响应慢、内存不足、请求量过大"
action:
  - "检查 etcd 延迟: etcdctl endpoint health --write-out=table"
  - "检查 webhook: kubectl get validatingwebhookconfigurations"
  - "检查 API Server 资源: kubectl top pods -n kube-system -l component=kube-apiserver"
  - "检查 --max-requests-inflight 配置"
severity: critical
```

### 7.2 Controller Manager 选举失败

```yaml
command: "kubectl get leases -n kube-system"
output_pattern: |
  NAME                      HOLDER                    AGE
  kube-controller-manager   <none>                    5m
  # HOLDER 为空说明无实例持有 Leader
diagnosis:
  - "Controller Manager 无法选举 Leader, 可能全部挂死"
  - "影响: Deployment/ReplicaSet/DaemonSet 等控制器停止工作"
action:
  - "kubectl get pods -n kube-system -l component=kube-controller-manager"
  - "kubectl logs -n kube-system <controller-manager-pod>"
  - "检查 --leader-elect 配置"
severity: critical
```

---

## 8. 安全诊断

### 8.1 RBAC 权限不足

```yaml
command: "kubectl auth can-i get pods --as=system:serviceaccount:default:myapp"
output_pattern: |
  no
diagnosis:
  - "ServiceAccount myapp 无权 get pods"
  - "缺少对应的 ClusterRole/RoleBinding"
action:
  - "kubectl auth can-i --list --as=system:serviceaccount:default:myapp"
  - "检查 RoleBinding: kubectl get rolebindings -A -o yaml | grep myapp"
  - "创建适当的 Role 和 RoleBinding"
severity: medium
```

### 8.2 Pod Security Standards 违规

```yaml
command: "kubectl describe pod myapp-xxxx-yyyy"
output_pattern: |
  Events:
    Warning  FailedCreate  1m  replicaset-controller  Error creating: admission webhook "pod-security-webhook" denied the request:
      violates PodSecurity "restricted:latest": ...
diagnosis:
  - "Pod 违反了命名空间的 Pod Security Standards"
  - "常见原因: 运行特权容器、使用 hostPath、不设 runAsNonRoot"
action:
  - "检查命名空间标签: kubectl get ns <ns> --show-labels"
  - "修改 Pod spec 以满足 restricted 级别要求"
  - "或降低 namespace 的 PSS 级别 (不推荐)"
severity: medium
```

---

## 使用说明

本语料库供 AI Agent 直接检索使用。每条记录包含:

| 字段 | 说明 |
|------|------|
| `command` | 执行的命令 |
| `output_pattern` | 典型输出模式 (可含通配符) |
| `diagnosis` | 诊断结论列表 |
| `action` | 建议的后续操作 |
| `severity` | 严重程度 (critical/high/medium/low) |

Agent 工作流:
1. 用户报告异常 → Agent 执行相应 kubectl 命令
2. 匹配 output_pattern → 获取 diagnosis
3. 按 action 列表逐步执行 → 定位根因 → 给出修复方案


<!-- risk-assessed -->
