---
title: P0 核心场景手工种子 I-O 对
description: 从核心 Skill 手工提取的高质量命令输出→诊断 I-O 对
summary: 从核心 Skill 手工提取的高质量命令输出→诊断 I-O 对
category: agent-corpus
tags:
- k8s
- troubleshooting
- command-output
- diagnosis
- seed
- p0
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# P0 核心场景手工种子 I-O 对

## Domain: NODE

```yaml
io_pair_id: IODIAG-NODE-0001
skill_ref: SKILL-01
scenario: 节点 NotReady - kubelet 异常
severity: critical
command: kubectl get nodes
output_pattern: 'NAME       STATUS     ROLES           AGE   VERSION

  node-01    NotReady   worker          5d    v1.32.0

  '
diagnosis:
- 节点不健康，kubelet 可能挂掉或网络不通
- NotReady 超过 pod-eviction-timeout (默认5min) 后 Pod 会被驱逐
- 需立即排查 kubelet 状态和节点资源压力
action:
- command: kubectl describe node node-01
  description: 执行修复/检查命令
  risk_level: low
- command: ssh node-01 'systemctl status kubelet'
  description: 执行修复/检查命令
  risk_level: low
- command: ssh node-01 'journalctl -u kubelet --since "5 min ago"'
  description: 执行修复/检查命令
  risk_level: low
- command: ssh node-01 'df -h /var/lib/kubelet'
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.95
tags:
- node
- status
- notready
- kubelet
```

```yaml
io_pair_id: IODIAG-NODE-0002
skill_ref: SKILL-01
scenario: 节点 DiskPressure
severity: high
command: kubectl describe node node-01
output_pattern: "Conditions:\n  Type             Status  Reason\n  DiskPressure  \
  \   True    KubeletHasDiskPressure\n"
diagnosis:
- 节点磁盘空间不足（默认阈值：可用 < 15%）
- kubelet 会自动驱逐 Pod 释放空间
- 可能原因：日志膨胀、镜像堆积、emptyDir 过大
action:
- command: ssh node-01 'df -h /var/lib/docker /var/lib/kubelet'
  description: 执行修复/检查命令
  risk_level: low
- command: ssh node-01 'du -sh /var/log/*'
  description: 执行修复/检查命令
  risk_level: low
- command: crictl rmi --prune
  description: 执行修复/检查命令
  risk_level: medium
- command: 检查 emptyDir 卷大小
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.95
tags:
- node
- diskpressure
- kubelet
- eviction
```

```yaml
io_pair_id: IODIAG-NODE-0003
skill_ref: SKILL-01
scenario: 节点 MemoryPressure
severity: high
command: kubectl describe node node-01
output_pattern: "Conditions:\n  Type             Status  Reason\n  MemoryPressure\
  \   True    KubeletHasMemoryPressure\n"
diagnosis:
- 节点可用内存不足（默认阈值：可用 < 100Mi）
- kubelet 会触发 Pod 驱逐
- 可能原因：内存泄漏、未设置 limits、大页内存占用
action:
- command: ssh node-01 'free -h'
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl top nodes
  description: 执行修复/检查命令
  risk_level: low
- command: 检查是否有内存泄漏的 Pod
  description: 执行修复/检查命令
  risk_level: low
- command: 考虑增加节点或调整 Pod limits
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.95
tags:
- node
- memorypressure
- kubelet
- eviction
```

```yaml
io_pair_id: IODIAG-NODE-0004
skill_ref: SKILL-01
scenario: 节点 PIDPressure
severity: high
command: kubectl describe node node-01
output_pattern: "Conditions:\n  Type             Status  Reason\n  PIDPressure   \
  \   True    KubeletHasPIDPressure\n"
diagnosis:
- 节点 PID 数量接近上限（默认阈值：可用 < 1000）
- 常见原因：应用创建过多线程/进程，fork bomb
- 可能导致新 Pod 无法启动
action:
- command: ssh node-01 'ps aux | wc -l'
  description: 执行修复/检查命令
  risk_level: low
- command: ssh node-01 'ps aux --sort=-%mem | head -20'
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 kubelet --pod-max-pids 配置
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.92
tags:
- node
- pidpressure
- kubelet
- processes
```

```yaml
io_pair_id: IODIAG-NODE-0005
skill_ref: SKILL-01
scenario: 节点 kubelet 证书过期
severity: critical
command: ssh node-01 'journalctl -u kubelet --since "1 hour ago" | grep -i certificate'
output_pattern: 'E0521 10:30:15.123456    1234 certificate_manager.go:123] Failed
  to rotate certificate: <...>

  E0521 10:30:15.123789    1234 kubelet.go:456] certificate rotation failed: x509:
  certificate has expired or is not yet valid

  '
diagnosis:
- kubelet 客户端证书已过期，无法与 apiserver 通信
- 节点将在 5 分钟后被标记为 NotReady
- 证书自动轮转可能因网络或 RBAC 问题失败
action:
- command: kubeadm certs check-expiration
  description: 执行修复/检查命令
  risk_level: low
- command: kubeadm certs renew all
  description: 执行修复/检查命令
  risk_level: low
- command: systemctl restart kubelet
  description: 执行修复/检查命令
  risk_level: medium
- command: 检查证书轮转权限
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.93
tags:
- node
- certificate
- kubelet
- notready
- expired
```

## Domain: POD

```yaml
io_pair_id: IODIAG-POD-0001
skill_ref: SKILL-02
scenario: Pod CrashLoopBackOff
severity: high
command: kubectl get pods -A
output_pattern: 'NAME                    READY   STATUS             RESTARTS      AGE

  myapp-xxxx-yyyy         0/1     CrashLoopBackOff   6 (32s ago)   5m

  '
diagnosis:
- 应用启动失败后反复重启，RESTARTS 持续增长
- 常见原因：启动命令错误、依赖服务未就绪、配置文件缺失、内存不足 OOMKilled
- 32s ago 表示最后一次重启距今 32 秒，指数退避中
action:
- command: kubectl logs myapp-xxxx-yyyy --previous
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl describe pod myapp-xxxx-yyyy
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 livenessProbe 配置是否过于严格
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.96
tags:
- pod
- crashloopbackoff
- container
- restart
```

```yaml
io_pair_id: IODIAG-POD-0002
skill_ref: SKILL-02
scenario: Pod OOMKilled
severity: high
command: kubectl describe pod myapp-xxxx-yyyy
output_pattern: "Last State: Terminated\n  Reason: OOMKilled\n  Exit Code: 137\n"
diagnosis:
- 容器因内存超限被内核 OOM Killer 终止
- Exit Code 137 = 128 + 9 (SIGKILL)
- limits.memory 设置过低或应用内存泄漏
action:
- command: 检查 resources.limits.memory 配置
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl top pod myapp-xxxx-yyyy
  description: 执行修复/检查命令
  risk_level: low
- command: 增大 limits.memory 或优化应用内存使用
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.97
tags:
- pod
- oomkilled
- memory
- container
- exit-code-137
```

```yaml
io_pair_id: IODIAG-POD-0003
skill_ref: SKILL-03
scenario: Pod Pending - 调度失败
severity: high
command: kubectl get pods -A
output_pattern: 'NAME                    READY   STATUS    RESTARTS   AGE

  myapp-xxxx-yyyy         0/1     Pending   0          10m

  '
diagnosis:
- Pod 未被调度到任何节点
- 常见原因：资源不足、nodeSelector/affinity 无匹配节点、PVC 未绑定、污点未容忍
- 10m 表示已等待 10 分钟
action:
- command: kubectl describe pod myapp-xxxx-yyyy | grep -A10 Events
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl get nodes -o wide
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl describe node <node> | grep -A5 Allocatable
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 resource requests 是否超过节点可分配资源
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.94
tags:
- pod
- pending
- scheduling
- scheduler
```

```yaml
io_pair_id: IODIAG-POD-0004
skill_ref: SKILL-03
scenario: Pod Pending - Insufficient CPU
severity: high
command: kubectl describe pod myapp-xxxx-yyyy
output_pattern: "Events:\n  Warning  FailedScheduling  1m  default-scheduler  0/3\
  \ nodes are available:\n    3 Insufficient cpu.\n"
diagnosis:
- 所有节点 CPU 资源不足，无法调度新 Pod
- Pod 的 resources.requests.cpu 超过节点可分配量
- 可能原因：节点负载过高、requests 设置不合理
action:
- command: kubectl top nodes
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl describe node <node> | grep -A5 Allocatable
  description: 执行修复/检查命令
  risk_level: low
- command: 降低 Pod 的 resources.requests 或扩容节点
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.95
tags:
- pod
- pending
- scheduling
- cpu
- insufficient
```

```yaml
io_pair_id: IODIAG-POD-0005
skill_ref: SKILL-10
scenario: Pod ImagePullBackOff
severity: high
command: kubectl get pods -A
output_pattern: 'NAME                    READY   STATUS             RESTARTS   AGE

  myapp-xxxx-yyyy         0/1     ImagePullBackOff   0          2m

  '
diagnosis:
- 镜像拉取失败，K8s 正在退避重试
- 常见原因：镜像名/tag 拼写错误、私有仓库认证失败、网络不通、镜像不存在
action:
- command: kubectl describe pod myapp-xxxx-yyyy | grep -A5 Events
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 imagePullSecrets 配置
  description: 执行修复/检查命令
  risk_level: low
- command: 手动 docker pull 验证镜像可达性
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.95
tags:
- pod
- imagepullbackoff
- image
- registry
- pull
```

```yaml
io_pair_id: IODIAG-POD-0006
skill_ref: SKILL-02
scenario: Pod Init:Error
severity: high
command: kubectl get pods -A
output_pattern: 'NAME                    READY   STATUS             RESTARTS   AGE

  myapp-xxxx-yyyy         0/1     Init:Error         3          2m

  '
diagnosis:
- Init Container 执行失败，主容器未启动
- Init Container 按顺序执行，任一失败则阻塞
- RESTARTS 计数为 Init Container 的重启次数
action:
- command: kubectl logs myapp-xxxx-yyyy -c <init-container-name>
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl describe pod myapp-xxxx-yyyy
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.93
tags:
- pod
- init
- init-container
- error
- startup
```

## Domain: DNS

```yaml
io_pair_id: IODIAG-DNS-0001
skill_ref: SKILL-04
scenario: CoreDNS CrashLoopBackOff
severity: critical
command: kubectl get pods -n kube-system -l k8s-app=kube-dns
output_pattern: 'NAME                       READY   STATUS             RESTARTS   AGE

  coredns-xxxx-yyyy          0/1     CrashLoopBackOff   5          3m

  '
diagnosis:
- CoreDNS 不可用，所有 Service DNS 解析失败
- 影响：所有依赖 Service 名称的通信中断
- 可能原因：配置错误、上游 DNS 不可达、资源不足
action:
- command: kubectl logs -n kube-system coredns-xxxx-yyyy
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl describe pod -n kube-system coredns-xxxx-yyyy
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 CoreDNS ConfigMap
  description: 执行修复/检查命令
  risk_level: low
- command: 临时测试：从 Pod 内直接 curl http://<ClusterIP>:<port>
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.95
tags:
- dns
- coredns
- crashloopbackoff
- kube-system
- critical
```

```yaml
io_pair_id: IODIAG-DNS-0002
skill_ref: SKILL-04
scenario: DNS 解析失败 - ndots 问题
severity: medium
command: kubectl run -it --rm debug --image=busybox:1.28 -- nslookup kubernetes.default
output_pattern: 'Server:    10.96.0.10

  Address 1: 10.96.0.10 kube-dns.kube-system.svc.cluster.local


  nslookup: can''t resolve ''kubernetes.default''

  '
diagnosis:
- Pod 内 DNS 解析失败
- 可能原因：ndots:5 导致外部域名解析异常、CoreDNS 配置错误、NetworkPolicy 阻断 53 端口
action:
- command: 检查 Pod 的 /etc/resolv.conf
  description: 执行修复/检查命令
  risk_level: low
- command: 测试集群内解析：nslookup kubernetes.default.svc.cluster.local
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 CoreDNS 日志
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 NetworkPolicy 是否允许 UDP 53
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.88
tags:
- dns
- resolution
- coredns
- ndots
- networkpolicy
```

## Domain: NET

```yaml
io_pair_id: IODIAG-NET-0001
skill_ref: SKILL-05
scenario: Service 无 Endpoints
severity: high
command: kubectl get endpoints myapp
output_pattern: 'NAME    ENDPOINTS   AGE

  myapp   <none>      2d

  '
diagnosis:
- Service 存在但无后端 Pod 关联
- 常见原因：selector 不匹配、Pod 未就绪、所有 Pod 被删除
- 流量无法到达后端
action:
- command: kubectl get svc myapp -o yaml | grep selector
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl get pods -l app=myapp
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 Pod 的 labels 是否与 Service selector 匹配
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 Pod readinessProbe 是否通过
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.94
tags:
- net
- service
- endpoints
- selector
- connectivity
```

```yaml
io_pair_id: IODIAG-NET-0002
skill_ref: SKILL-05
scenario: Ingress 502 Bad Gateway
severity: high
command: curl -I https://myapp.example.com
output_pattern: 'HTTP/2 502

  server: nginx

  '
diagnosis:
- Ingress Controller 能接收请求但后端无响应
- 常见原因：后端 Pod 未就绪、Service 端口错误、健康检查失败
action:
- command: kubectl get ingress myapp -o yaml
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl get endpoints <service-name>
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl logs -n ingress-nginx <ingress-controller-pod>
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 backend 的 readinessProbe
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.92
tags:
- net
- ingress
- 502
- bad-gateway
- nginx
```

## Domain: CERT

```yaml
io_pair_id: IODIAG-CERT-0001
skill_ref: SKILL-06
scenario: API Server 证书过期
severity: critical
command: kubeadm certs check-expiration
output_pattern: 'CERTIFICATE                EXPIRES                  RESIDUAL TIME   CERTIFICATE
  AUTHORITY   EXTERNALLY MANAGED

  admin.conf                 May 20, 2026 10:00 UTC   0h              ca                      no

  apiserver                  May 20, 2026 10:00 UTC   0h              ca                      no

  '
diagnosis:
- API Server 证书已过期，集群控制平面不可用
- kubectl 无法连接集群
- 所有依赖 API Server 的组件停止工作
action:
- command: kubeadm certs renew all
  description: 执行修复/检查命令
  risk_level: low
- command: systemctl restart kubelet
  description: 执行修复/检查命令
  risk_level: medium
- command: 更新 ~/.kube/config
  description: 执行修复/检查命令
  risk_level: low
- command: 验证：kubectl get nodes
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.97
tags:
- cert
- certificate
- apiserver
- expired
- critical
- kubeadm
```

## Domain: CP

```yaml
io_pair_id: IODIAG-CP-0001
skill_ref: SKILL-11
scenario: API Server 响应慢
severity: critical
command: kubectl get --raw /metrics | grep apiserver_request_duration
output_pattern: 'apiserver_request_duration_seconds{verb="GET",resource="pods",quantile="0.99"}
  5.2

  # P99 延迟 > 5s (正常 < 1s)

  '
diagnosis:
- API Server 响应严重延迟
- 常见原因：etcd 延迟高、webhook 响应慢、内存不足、请求量过大
- 影响：所有 kubectl 操作变慢，控制器同步滞后
action:
- command: 检查 etcd 延迟：etcdctl endpoint health --write-out=table
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 webhook：kubectl get validatingwebhookconfigurations
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 API Server 资源：kubectl top pods -n kube-system -l component=kube-apiserver
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 --max-requests-inflight 配置
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.91
tags:
- cp
- apiserver
- latency
- p99
- performance
- critical
```

```yaml
io_pair_id: IODIAG-CP-0002
skill_ref: SKILL-11
scenario: Controller Manager Leader 选举失败
severity: critical
command: kubectl get leases -n kube-system
output_pattern: 'NAME                      HOLDER                    AGE

  kube-controller-manager   <none>                    5m

  # HOLDER 为空说明无实例持有 Leader

  '
diagnosis:
- Controller Manager 无法选举 Leader，可能全部挂死
- 影响：Deployment/ReplicaSet/DaemonSet 等控制器停止工作
- 新 Pod 不会创建，旧 Pod 不会缩容
action:
- command: kubectl get pods -n kube-system -l component=kube-controller-manager
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl logs -n kube-system <controller-manager-pod>
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 --leader-elect 配置
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 kube-controller-manager 的 RBAC 权限
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.94
tags:
- cp
- controller-manager
- leader-election
- lease
- critical
```

## Domain: ETCD

```yaml
io_pair_id: IODIAG-ETCD-0001
skill_ref: SKILL-11
scenario: etcd 节点不健康
severity: critical
command: etcdctl endpoint health --cluster
output_pattern: 'https://etcd-0:2379 is healthy: successfully committed proposal:
  took = 12.345ms

  https://etcd-1:2379 is unhealthy: Error on endpoint: context deadline exceeded

  https://etcd-2:2379 is healthy: successfully committed proposal: took = 15.678ms

  '
diagnosis:
- etcd-1 不健康，可能原因：网络分区、磁盘 IO 慢、进程挂死
- 2/3 节点健康时集群仍可用，但需尽快修复
- 若再失一个节点，集群将不可用
action:
- command: ssh etcd-1 'systemctl status etcd'
  description: 执行修复/检查命令
  risk_level: low
- command: ssh etcd-1 'journalctl -u etcd --since 5min'
  description: 执行修复/检查命令
  risk_level: low
- command: 检查磁盘 IO：ssh etcd-1 'iostat -x 1 3'
  description: 执行修复/检查命令
  risk_level: low
- command: 检查网络：ping etcd-1, telnet etcd-1 2379
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.95
tags:
- etcd
- health
- unhealthy
- cluster
- quorum
- critical
```

```yaml
io_pair_id: IODIAG-ETCD-0002
skill_ref: SKILL-11
scenario: etcd DB 大小超限
severity: critical
command: etcdctl endpoint status --write-out=table
output_pattern: '| etcd-0:2379 | 1 | 3.5.x | 8.2 GB | true | 1234 |

  # DB SIZE 接近 --quota-backend-bytes (默认 8GB)

  '
diagnosis:
- etcd DB 接近配额上限，可能导致写入失败
- 常见原因：未配置 compaction、资源版本泄漏、ConfigMap/Secret 过多
- 集群可能进入只读模式
action:
- command: etcdctl compact $(etcdctl endpoint status --write-out=json | jq '.[0].Status.header.revision')
  description: 执行修复/检查命令
  risk_level: low
- command: etcdctl defrag --endpoints=https://etcd-0:2379
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 --quota-backend-bytes 配置，生产建议 8-16GB
  description: 执行修复/检查命令
  risk_level: low
- command: 清理不必要的 CRD、ConfigMap、Secret
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.93
tags:
- etcd
- db-size
- quota
- compaction
- defrag
- critical
```

## Domain: STORAGE

```yaml
io_pair_id: IODIAG-STORAGE-0001
skill_ref: SKILL-07
scenario: PVC Pending
severity: high
command: kubectl get pvc
output_pattern: 'NAME       STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS   AGE

  my-pvc     Pending                                      gp3            5m

  '
diagnosis:
- PVC 未绑定 PV，无可用存储资源
- 常见原因：StorageClass 不存在、PV 不足、zone 不匹配、CSI 驱动异常
action:
- command: kubectl describe pvc my-pvc
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl get sc
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl get pv
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 CSI 驱动：kubectl get pods -n kube-system | grep csi
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.94
tags:
- storage
- pvc
- pending
- pv
- storageclass
- csi
```

```yaml
io_pair_id: IODIAG-STORAGE-0002
skill_ref: SKILL-07
scenario: Pod MountVolume Failed
severity: high
command: kubectl describe pod myapp-xxxx-yyyy
output_pattern: "Events:\n  Warning  FailedMount  2m (x8 over 5m)  kubelet  MountVolume.SetUp\
  \ failed for volume \"pvc-xxx\" : ...\n"
diagnosis:
- Volume 挂载失败，Pod 无法启动
- 常见原因：CSI 驱动异常、节点网络不通存储后端、NFS server 不可达
action:
- command: kubectl get pods -n kube-system | grep csi
  description: 执行修复/检查命令
  risk_level: low
- command: ssh <node> 'mount | grep <volume>'
  description: 执行修复/检查命令
  risk_level: low
- command: 检查存储后端连通性
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.9
tags:
- storage
- mount
- failedmount
- volume
- csi
- kubelet
```

## Domain: WORK

```yaml
io_pair_id: IODIAG-WORK-0001
skill_ref: SKILL-08
scenario: Deployment Rollout 失败
severity: high
command: kubectl rollout status deployment/myapp
output_pattern: 'error: deployment "myapp" exceeded its progress deadline

  '
diagnosis:
- Deployment 更新进度超时
- 常见原因：新 Pod 无法启动、镜像拉取失败、健康检查不通过、资源不足
action:
- command: kubectl get pods -l app=myapp
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl describe deployment myapp
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl rollout history deployment/myapp
  description: 执行修复/检查命令
  risk_level: low
- command: 检查新 ReplicaSet 的 Pod 状态和 Events
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.92
tags:
- work
- deployment
- rollout
- progress-deadline
- update
```

## Domain: SEC

```yaml
io_pair_id: IODIAG-SEC-0001
skill_ref: SKILL-09
scenario: RBAC 权限不足
severity: medium
command: kubectl auth can-i get pods --as=system:serviceaccount:default:myapp
output_pattern: 'no

  '
diagnosis:
- ServiceAccount myapp 无权 get pods
- 缺少对应的 ClusterRole/RoleBinding
action:
- command: kubectl auth can-i --list --as=system:serviceaccount:default:myapp
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 RoleBinding：kubectl get rolebindings -A -o yaml | grep myapp
  description: 执行修复/检查命令
  risk_level: low
- command: 创建适当的 Role 和 RoleBinding
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.95
tags:
- sec
- rbac
- permission
- serviceaccount
- authorization
```

```yaml
io_pair_id: IODIAG-SEC-0002
skill_ref: SKILL-09
scenario: ResourceQuota 超限
severity: medium
command: kubectl describe pod myapp-xxxx-yyyy
output_pattern: "Events:\n  Warning  FailedCreate  1m  replicaset-controller  Error\
  \ creating: pods \"myapp-xxx\" is forbidden:\n    exceeded quota: my-quota, requested:\
  \ cpu=2, used: cpu=8, limited: cpu=10\n"
diagnosis:
- 命名空间 ResourceQuota 已用尽
- 当前已用 8 CPU，请求 2 CPU，上限 10 CPU
action:
- command: kubectl get resourcequota -n <namespace>
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl describe resourcequota my-quota -n <namespace>
  description: 执行修复/检查命令
  risk_level: low
- command: 清理不用的 Pod 或申请提高配额
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.94
tags:
- sec
- quota
- resourcequota
- limits
- namespace
```

## Domain: INGRESS

```yaml
io_pair_id: IODIAG-INGRESS-0001
skill_ref: SKILL-13
scenario: Ingress Controller Pod 未就绪
severity: high
command: kubectl get pods -n ingress-nginx
output_pattern: 'NAME                                        READY   STATUS    RESTARTS   AGE

  ingress-nginx-controller-xxxx-yyyy          0/1     Pending   0          5m

  '
diagnosis:
- Ingress Controller 未运行，所有 Ingress 规则不生效
- 可能原因：资源不足、节点污点、镜像拉取失败
action:
- command: kubectl describe pod -n ingress-nginx ingress-nginx-controller-xxxx-yyyy
  description: 执行修复/检查命令
  risk_level: low
- command: kubectl get nodes -o wide
  description: 执行修复/检查命令
  risk_level: low
- command: 检查 Ingress Controller 的 nodeSelector 和 tolerations
  description: 执行修复/检查命令
  risk_level: low
confidence: 0.9
tags:
- ingress
- nginx
- controller
- pending
- gateway
```

## 统计

| 指标 | 数值 |
|------|------|
| 手工种子 I-O 对总数 | 22 |
| 覆盖 Domain | 10 |
| 覆盖 Severity | critical: 5, high: 13, medium: 4 |
| 关联 Skills | 9 个 |


<!-- risk-assessed -->
