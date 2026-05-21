---
title: P0 核心场景手工种子 I-O 对
description: 从核心 Skill 手工提取的高质量命令输出→诊断 I-O 对
category: agent-corpus
tags:
- k8s
- troubleshooting
- command-output
- diagnosis
- seed
- p0
---

# P0 核心场景手工种子 I-O 对

## Domain: NODE

```yaml
io_pair_id: IODIAG-NODE-0001
skill_ref: SKILL-01
scenario: 节点 NotReady - kubelet 异常
severity: critical
command: kubectl get nodes
output_pattern: |
  NAME       STATUS     ROLES           AGE   VERSION
  node-01    NotReady   worker          5d    v1.32.0
diagnosis:
  - 节点不健康，kubelet 可能挂掉或网络不通
  - NotReady 超过 pod-eviction-timeout (默认5min) 后 Pod 会被驱逐
  - 需立即排查 kubelet 状态和节点资源压力
action:
  - kubectl describe node node-01
  - ssh node-01 'systemctl status kubelet'
  - ssh node-01 'journalctl -u kubelet --since "5 min ago"'
  - ssh node-01 'df -h /var/lib/kubelet'
confidence: 0.95
tags: [node, status, notready, kubelet]
```

```yaml
io_pair_id: IODIAG-NODE-0002
skill_ref: SKILL-01
scenario: 节点 DiskPressure
severity: high
command: kubectl describe node node-01
output_pattern: |
  Conditions:
    Type             Status  Reason
    DiskPressure     True    KubeletHasDiskPressure
diagnosis:
  - 节点磁盘空间不足（默认阈值：可用 < 15%）
  - kubelet 会自动驱逐 Pod 释放空间
  - 可能原因：日志膨胀、镜像堆积、emptyDir 过大
action:
  - ssh node-01 'df -h /var/lib/docker /var/lib/kubelet'
  - ssh node-01 'du -sh /var/log/*'
  - crictl rmi --prune
  - 检查 emptyDir 卷大小
confidence: 0.95
tags: [node, diskpressure, kubelet, eviction]
```

```yaml
io_pair_id: IODIAG-NODE-0003
skill_ref: SKILL-01
scenario: 节点 MemoryPressure
severity: high
command: kubectl describe node node-01
output_pattern: |
  Conditions:
    Type             Status  Reason
    MemoryPressure   True    KubeletHasMemoryPressure
diagnosis:
  - 节点可用内存不足（默认阈值：可用 < 100Mi）
  - kubelet 会触发 Pod 驱逐
  - 可能原因：内存泄漏、未设置 limits、大页内存占用
action:
  - ssh node-01 'free -h'
  - kubectl top nodes
  - 检查是否有内存泄漏的 Pod
  - 考虑增加节点或调整 Pod limits
confidence: 0.95
tags: [node, memorypressure, kubelet, eviction]
```

```yaml
io_pair_id: IODIAG-NODE-0004
skill_ref: SKILL-01
scenario: 节点 PIDPressure
severity: high
command: kubectl describe node node-01
output_pattern: |
  Conditions:
    Type             Status  Reason
    PIDPressure      True    KubeletHasPIDPressure
diagnosis:
  - 节点 PID 数量接近上限（默认阈值：可用 < 1000）
  - 常见原因：应用创建过多线程/进程，fork bomb
  - 可能导致新 Pod 无法启动
action:
  - ssh node-01 'ps aux | wc -l'
  - ssh node-01 'ps aux --sort=-%mem | head -20'
  - 检查 kubelet --pod-max-pids 配置
confidence: 0.92
tags: [node, pidpressure, kubelet, processes]
```

```yaml
io_pair_id: IODIAG-NODE-0005
skill_ref: SKILL-01
scenario: 节点 kubelet 证书过期
severity: critical
command: ssh node-01 'journalctl -u kubelet --since "1 hour ago" | grep -i certificate'
output_pattern: |
  E0521 10:30:15.123456    1234 certificate_manager.go:123] Failed to rotate certificate: <...>
  E0521 10:30:15.123789    1234 kubelet.go:456] certificate rotation failed: x509: certificate has expired or is not yet valid
diagnosis:
  - kubelet 客户端证书已过期，无法与 apiserver 通信
  - 节点将在 5 分钟后被标记为 NotReady
  - 证书自动轮转可能因网络或 RBAC 问题失败
action:
  - kubeadm certs check-expiration
  - kubeadm certs renew all
  - systemctl restart kubelet
  - 检查证书轮转权限
confidence: 0.93
tags: [node, certificate, kubelet, notready, expired]
```

## Domain: POD

```yaml
io_pair_id: IODIAG-POD-0001
skill_ref: SKILL-02
scenario: Pod CrashLoopBackOff
severity: high
command: kubectl get pods -A
output_pattern: |
  NAME                    READY   STATUS             RESTARTS      AGE
  myapp-xxxx-yyyy         0/1     CrashLoopBackOff   6 (32s ago)   5m
diagnosis:
  - 应用启动失败后反复重启，RESTARTS 持续增长
  - 常见原因：启动命令错误、依赖服务未就绪、配置文件缺失、内存不足 OOMKilled
  - 32s ago 表示最后一次重启距今 32 秒，指数退避中
action:
  - kubectl logs myapp-xxxx-yyyy --previous
  - kubectl describe pod myapp-xxxx-yyyy
  - 检查 livenessProbe 配置是否过于严格
confidence: 0.96
tags: [pod, crashloopbackoff, container, restart]
```

```yaml
io_pair_id: IODIAG-POD-0002
skill_ref: SKILL-02
scenario: Pod OOMKilled
severity: high
command: kubectl describe pod myapp-xxxx-yyyy
output_pattern: |
  Last State: Terminated
    Reason: OOMKilled
    Exit Code: 137
diagnosis:
  - 容器因内存超限被内核 OOM Killer 终止
  - Exit Code 137 = 128 + 9 (SIGKILL)
  - limits.memory 设置过低或应用内存泄漏
action:
  - 检查 resources.limits.memory 配置
  - kubectl top pod myapp-xxxx-yyyy
  - 增大 limits.memory 或优化应用内存使用
confidence: 0.97
tags: [pod, oomkilled, memory, container, exit-code-137]
```

```yaml
io_pair_id: IODIAG-POD-0003
skill_ref: SKILL-03
scenario: Pod Pending - 调度失败
severity: high
command: kubectl get pods -A
output_pattern: |
  NAME                    READY   STATUS    RESTARTS   AGE
  myapp-xxxx-yyyy         0/1     Pending   0          10m
diagnosis:
  - Pod 未被调度到任何节点
  - 常见原因：资源不足、nodeSelector/affinity 无匹配节点、PVC 未绑定、污点未容忍
  - 10m 表示已等待 10 分钟
action:
  - kubectl describe pod myapp-xxxx-yyyy | grep -A10 Events
  - kubectl get nodes -o wide
  - kubectl describe node <node> | grep -A5 Allocatable
  - 检查 resource requests 是否超过节点可分配资源
confidence: 0.94
tags: [pod, pending, scheduling, scheduler]
```

```yaml
io_pair_id: IODIAG-POD-0004
skill_ref: SKILL-03
scenario: Pod Pending - Insufficient CPU
severity: high
command: kubectl describe pod myapp-xxxx-yyyy
output_pattern: |
  Events:
    Warning  FailedScheduling  1m  default-scheduler  0/3 nodes are available:
      3 Insufficient cpu.
diagnosis:
  - 所有节点 CPU 资源不足，无法调度新 Pod
  - Pod 的 resources.requests.cpu 超过节点可分配量
  - 可能原因：节点负载过高、requests 设置不合理
action:
  - kubectl top nodes
  - kubectl describe node <node> | grep -A5 Allocatable
  - 降低 Pod 的 resources.requests 或扩容节点
confidence: 0.95
tags: [pod, pending, scheduling, cpu, insufficient]
```

```yaml
io_pair_id: IODIAG-POD-0005
skill_ref: SKILL-10
scenario: Pod ImagePullBackOff
severity: high
command: kubectl get pods -A
output_pattern: |
  NAME                    READY   STATUS             RESTARTS   AGE
  myapp-xxxx-yyyy         0/1     ImagePullBackOff   0          2m
diagnosis:
  - 镜像拉取失败，K8s 正在退避重试
  - 常见原因：镜像名/tag 拼写错误、私有仓库认证失败、网络不通、镜像不存在
action:
  - kubectl describe pod myapp-xxxx-yyyy | grep -A5 Events
  - 检查 imagePullSecrets 配置
  - 手动 docker pull 验证镜像可达性
confidence: 0.95
tags: [pod, imagepullbackoff, image, registry, pull]
```

```yaml
io_pair_id: IODIAG-POD-0006
skill_ref: SKILL-02
scenario: Pod Init:Error
severity: high
command: kubectl get pods -A
output_pattern: |
  NAME                    READY   STATUS             RESTARTS   AGE
  myapp-xxxx-yyyy         0/1     Init:Error         3          2m
diagnosis:
  - Init Container 执行失败，主容器未启动
  - Init Container 按顺序执行，任一失败则阻塞
  - RESTARTS 计数为 Init Container 的重启次数
action:
  - kubectl logs myapp-xxxx-yyyy -c <init-container-name>
  - kubectl describe pod myapp-xxxx-yyyy
confidence: 0.93
tags: [pod, init, init-container, error, startup]
```

## Domain: DNS

```yaml
io_pair_id: IODIAG-DNS-0001
skill_ref: SKILL-04
scenario: CoreDNS CrashLoopBackOff
severity: critical
command: kubectl get pods -n kube-system -l k8s-app=kube-dns
output_pattern: |
  NAME                       READY   STATUS             RESTARTS   AGE
  coredns-xxxx-yyyy          0/1     CrashLoopBackOff   5          3m
diagnosis:
  - CoreDNS 不可用，所有 Service DNS 解析失败
  - 影响：所有依赖 Service 名称的通信中断
  - 可能原因：配置错误、上游 DNS 不可达、资源不足
action:
  - kubectl logs -n kube-system coredns-xxxx-yyyy
  - kubectl describe pod -n kube-system coredns-xxxx-yyyy
  - 检查 CoreDNS ConfigMap
  - 临时测试：从 Pod 内直接 curl http://<ClusterIP>:<port>
confidence: 0.95
tags: [dns, coredns, crashloopbackoff, kube-system, critical]
```

```yaml
io_pair_id: IODIAG-DNS-0002
skill_ref: SKILL-04
scenario: DNS 解析失败 - ndots 问题
severity: medium
command: kubectl run -it --rm debug --image=busybox:1.28 -- nslookup kubernetes.default
output_pattern: |
  Server:    10.96.0.10
  Address 1: 10.96.0.10 kube-dns.kube-system.svc.cluster.local
  
  nslookup: can't resolve 'kubernetes.default'
diagnosis:
  - Pod 内 DNS 解析失败
  - 可能原因：ndots:5 导致外部域名解析异常、CoreDNS 配置错误、NetworkPolicy 阻断 53 端口
action:
  - 检查 Pod 的 /etc/resolv.conf
  - 测试集群内解析：nslookup kubernetes.default.svc.cluster.local
  - 检查 CoreDNS 日志
  - 检查 NetworkPolicy 是否允许 UDP 53
confidence: 0.88
tags: [dns, resolution, coredns, ndots, networkpolicy]
```

## Domain: NET

```yaml
io_pair_id: IODIAG-NET-0001
skill_ref: SKILL-05
scenario: Service 无 Endpoints
severity: high
command: kubectl get endpoints myapp
output_pattern: |
  NAME    ENDPOINTS   AGE
  myapp   <none>      2d
diagnosis:
  - Service 存在但无后端 Pod 关联
  - 常见原因：selector 不匹配、Pod 未就绪、所有 Pod 被删除
  - 流量无法到达后端
action:
  - kubectl get svc myapp -o yaml | grep selector
  - kubectl get pods -l app=myapp
  - 检查 Pod 的 labels 是否与 Service selector 匹配
  - 检查 Pod readinessProbe 是否通过
confidence: 0.94
tags: [net, service, endpoints, selector, connectivity]
```

```yaml
io_pair_id: IODIAG-NET-0002
skill_ref: SKILL-05
scenario: Ingress 502 Bad Gateway
severity: high
command: curl -I https://myapp.example.com
output_pattern: |
  HTTP/2 502
  server: nginx
diagnosis:
  - Ingress Controller 能接收请求但后端无响应
  - 常见原因：后端 Pod 未就绪、Service 端口错误、健康检查失败
action:
  - kubectl get ingress myapp -o yaml
  - kubectl get endpoints <service-name>
  - kubectl logs -n ingress-nginx <ingress-controller-pod>
  - 检查 backend 的 readinessProbe
confidence: 0.92
tags: [net, ingress, 502, bad-gateway, nginx]
```

## Domain: CERT

```yaml
io_pair_id: IODIAG-CERT-0001
skill_ref: SKILL-06
scenario: API Server 证书过期
severity: critical
command: kubeadm certs check-expiration
output_pattern: |
  CERTIFICATE                EXPIRES                  RESIDUAL TIME   CERTIFICATE AUTHORITY   EXTERNALLY MANAGED
  admin.conf                 May 20, 2026 10:00 UTC   0h              ca                      no
  apiserver                  May 20, 2026 10:00 UTC   0h              ca                      no
diagnosis:
  - API Server 证书已过期，集群控制平面不可用
  - kubectl 无法连接集群
  - 所有依赖 API Server 的组件停止工作
action:
  - kubeadm certs renew all
  - systemctl restart kubelet
  - 更新 ~/.kube/config
  - 验证：kubectl get nodes
confidence: 0.97
tags: [cert, certificate, apiserver, expired, critical, kubeadm]
```

## Domain: CP

```yaml
io_pair_id: IODIAG-CP-0001
skill_ref: SKILL-11
scenario: API Server 响应慢
severity: critical
command: kubectl get --raw /metrics | grep apiserver_request_duration
output_pattern: |
  apiserver_request_duration_seconds{verb="GET",resource="pods",quantile="0.99"} 5.2
  # P99 延迟 > 5s (正常 < 1s)
diagnosis:
  - API Server 响应严重延迟
  - 常见原因：etcd 延迟高、webhook 响应慢、内存不足、请求量过大
  - 影响：所有 kubectl 操作变慢，控制器同步滞后
action:
  - 检查 etcd 延迟：etcdctl endpoint health --write-out=table
  - 检查 webhook：kubectl get validatingwebhookconfigurations
  - 检查 API Server 资源：kubectl top pods -n kube-system -l component=kube-apiserver
  - 检查 --max-requests-inflight 配置
confidence: 0.91
tags: [cp, apiserver, latency, p99, performance, critical]
```

```yaml
io_pair_id: IODIAG-CP-0002
skill_ref: SKILL-11
scenario: Controller Manager Leader 选举失败
severity: critical
command: kubectl get leases -n kube-system
output_pattern: |
  NAME                      HOLDER                    AGE
  kube-controller-manager   <none>                    5m
  # HOLDER 为空说明无实例持有 Leader
diagnosis:
  - Controller Manager 无法选举 Leader，可能全部挂死
  - 影响：Deployment/ReplicaSet/DaemonSet 等控制器停止工作
  - 新 Pod 不会创建，旧 Pod 不会缩容
action:
  - kubectl get pods -n kube-system -l component=kube-controller-manager
  - kubectl logs -n kube-system <controller-manager-pod>
  - 检查 --leader-elect 配置
  - 检查 kube-controller-manager 的 RBAC 权限
confidence: 0.94
tags: [cp, controller-manager, leader-election, lease, critical]
```

## Domain: ETCD

```yaml
io_pair_id: IODIAG-ETCD-0001
skill_ref: SKILL-11
scenario: etcd 节点不健康
severity: critical
command: etcdctl endpoint health --cluster
output_pattern: |
  https://etcd-0:2379 is healthy: successfully committed proposal: took = 12.345ms
  https://etcd-1:2379 is unhealthy: Error on endpoint: context deadline exceeded
  https://etcd-2:2379 is healthy: successfully committed proposal: took = 15.678ms
diagnosis:
  - etcd-1 不健康，可能原因：网络分区、磁盘 IO 慢、进程挂死
  - 2/3 节点健康时集群仍可用，但需尽快修复
  - 若再失一个节点，集群将不可用
action:
  - ssh etcd-1 'systemctl status etcd'
  - ssh etcd-1 'journalctl -u etcd --since 5min'
  - 检查磁盘 IO：ssh etcd-1 'iostat -x 1 3'
  - 检查网络：ping etcd-1, telnet etcd-1 2379
confidence: 0.95
tags: [etcd, health, unhealthy, cluster, quorum, critical]
```

```yaml
io_pair_id: IODIAG-ETCD-0002
skill_ref: SKILL-11
scenario: etcd DB 大小超限
severity: critical
command: etcdctl endpoint status --write-out=table
output_pattern: |
  | etcd-0:2379 | 1 | 3.5.x | 8.2 GB | true | 1234 |
  # DB SIZE 接近 --quota-backend-bytes (默认 8GB)
diagnosis:
  - etcd DB 接近配额上限，可能导致写入失败
  - 常见原因：未配置 compaction、资源版本泄漏、ConfigMap/Secret 过多
  - 集群可能进入只读模式
action:
  - etcdctl compact $(etcdctl endpoint status --write-out=json | jq '.[0].Status.header.revision')
  - etcdctl defrag --endpoints=https://etcd-0:2379
  - 检查 --quota-backend-bytes 配置，生产建议 8-16GB
  - 清理不必要的 CRD、ConfigMap、Secret
confidence: 0.93
tags: [etcd, db-size, quota, compaction, defrag, critical]
```

## Domain: STORAGE

```yaml
io_pair_id: IODIAG-STORAGE-0001
skill_ref: SKILL-07
scenario: PVC Pending
severity: high
command: kubectl get pvc
output_pattern: |
  NAME       STATUS    VOLUME   CAPACITY   ACCESS MODES   STORAGECLASS   AGE
  my-pvc     Pending                                      gp3            5m
diagnosis:
  - PVC 未绑定 PV，无可用存储资源
  - 常见原因：StorageClass 不存在、PV 不足、zone 不匹配、CSI 驱动异常
action:
  - kubectl describe pvc my-pvc
  - kubectl get sc
  - kubectl get pv
  - 检查 CSI 驱动：kubectl get pods -n kube-system | grep csi
confidence: 0.94
tags: [storage, pvc, pending, pv, storageclass, csi]
```

```yaml
io_pair_id: IODIAG-STORAGE-0002
skill_ref: SKILL-07
scenario: Pod MountVolume Failed
severity: high
command: kubectl describe pod myapp-xxxx-yyyy
output_pattern: |
  Events:
    Warning  FailedMount  2m (x8 over 5m)  kubelet  MountVolume.SetUp failed for volume "pvc-xxx" : ...
diagnosis:
  - Volume 挂载失败，Pod 无法启动
  - 常见原因：CSI 驱动异常、节点网络不通存储后端、NFS server 不可达
action:
  - kubectl get pods -n kube-system | grep csi
  - ssh <node> 'mount | grep <volume>'
  - 检查存储后端连通性
confidence: 0.90
tags: [storage, mount, failedmount, volume, csi, kubelet]
```

## Domain: WORK

```yaml
io_pair_id: IODIAG-WORK-0001
skill_ref: SKILL-08
scenario: Deployment Rollout 失败
severity: high
command: kubectl rollout status deployment/myapp
output_pattern: |
  error: deployment "myapp" exceeded its progress deadline
diagnosis:
  - Deployment 更新进度超时
  - 常见原因：新 Pod 无法启动、镜像拉取失败、健康检查不通过、资源不足
action:
  - kubectl get pods -l app=myapp
  - kubectl describe deployment myapp
  - kubectl rollout history deployment/myapp
  - 检查新 ReplicaSet 的 Pod 状态和 Events
confidence: 0.92
tags: [work, deployment, rollout, progress-deadline, update]
```

## Domain: SEC

```yaml
io_pair_id: IODIAG-SEC-0001
skill_ref: SKILL-09
scenario: RBAC 权限不足
severity: medium
command: kubectl auth can-i get pods --as=system:serviceaccount:default:myapp
output_pattern: |
  no
diagnosis:
  - ServiceAccount myapp 无权 get pods
  - 缺少对应的 ClusterRole/RoleBinding
action:
  - kubectl auth can-i --list --as=system:serviceaccount:default:myapp
  - 检查 RoleBinding：kubectl get rolebindings -A -o yaml | grep myapp
  - 创建适当的 Role 和 RoleBinding
confidence: 0.95
tags: [sec, rbac, permission, serviceaccount, authorization]
```

```yaml
io_pair_id: IODIAG-SEC-0002
skill_ref: SKILL-09
scenario: ResourceQuota 超限
severity: medium
command: kubectl describe pod myapp-xxxx-yyyy
output_pattern: |
  Events:
    Warning  FailedCreate  1m  replicaset-controller  Error creating: pods "myapp-xxx" is forbidden:
      exceeded quota: my-quota, requested: cpu=2, used: cpu=8, limited: cpu=10
diagnosis:
  - 命名空间 ResourceQuota 已用尽
  - 当前已用 8 CPU，请求 2 CPU，上限 10 CPU
action:
  - kubectl get resourcequota -n <namespace>
  - kubectl describe resourcequota my-quota -n <namespace>
  - 清理不用的 Pod 或申请提高配额
confidence: 0.94
tags: [sec, quota, resourcequota, limits, namespace]
```

## Domain: INGRESS

```yaml
io_pair_id: IODIAG-INGRESS-0001
skill_ref: SKILL-13
scenario: Ingress Controller Pod 未就绪
severity: high
command: kubectl get pods -n ingress-nginx
output_pattern: |
  NAME                                        READY   STATUS    RESTARTS   AGE
  ingress-nginx-controller-xxxx-yyyy          0/1     Pending   0          5m
diagnosis:
  - Ingress Controller 未运行，所有 Ingress 规则不生效
  - 可能原因：资源不足、节点污点、镜像拉取失败
action:
  - kubectl describe pod -n ingress-nginx ingress-nginx-controller-xxxx-yyyy
  - kubectl get nodes -o wide
  - 检查 Ingress Controller 的 nodeSelector 和 tolerations
confidence: 0.90
tags: [ingress, nginx, controller, pending, gateway]
```

## 统计

| 指标 | 数值 |
|------|------|
| 手工种子 I-O 对总数 | 22 |
| 覆盖 Domain | 10 |
| 覆盖 Severity | critical: 5, high: 13, medium: 4 |
| 关联 Skills | 9 个 |
