---
title: CCE故障排查手册
description: 'CCE集群故障排查：节点纳管异常、Addon故障、CCE Turbo网络问题、日志采集与监控告警配置'
summary: 'CCE集群故障排查：节点纳管异常、Addon故障、CCE Turbo网络问题、日志采集与监控告警配置'
category: cloud-providers
tags:
- cloud
- k8s
- huawei-cce
- troubleshooting
- monitoring
- logging
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- CCE故障排查 是什么
- 如何排查CCE节点纳管问题
- CCE Addon异常怎么处理
trigger_keywords:
- CCE
- 故障排查
- 节点纳管
- Addon
- Turbo
- 日志采集
- 监控告警
prerequisites:
- kubectl-basics
- cloud-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# CCE故障排查手册

## 1. 节点纳管问题

### 1.1 节点纳管失败

**症状**：节点加入集群后状态为 `NotReady` 或 `Unknown`

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看节点状态
kubectl get nodes -o wide
# NAME          STATUS     ROLES    AGE   VERSION   INTERNAL-IP
# node-001      NotReady   <none>   5m    v1.28.x   10.0.1.10

# 查看节点 Conditions
kubectl describe node node-001 | grep -A 20 "Conditions:"
# Conditions:
#   Type             Status  Message
#   ----             ------  -------
#   Ready            False   Kubelet not ready: runtime network not ready
#   DiskPressure     False
#   MemoryPressure   False
```
**排查流程**：

```
节点纳管失败
├── 1. 网络不通
│   ├── 检查 VPC 安全组是否放行 10250/10255/6443 端口
│   ├── 检查节点与 API Server 的连通性
│   └── 命令: curl -k https://<api-server>:6443/healthz
│
├── 2. kubelet 启动失败
│   ├── 查看 kubelet 日志
│   └── 命令: journalctl -u kubelet --no-pager -n 100
│
├── 3. 容器运行时异常
│   ├── 检查 containerd/docker 状态
│   └── 命令: systemctl status containerd
│
├── 4. 证书问题
│   ├── 检查 kubelet 证书是否过期
│   └── 命令: openssl x509 -in /etc/kubernetes/pki/kubelet.crt -noout -dates
│
└── 5. 资源不足
    ├── 检查节点 CPU/内存是否耗尽
    └── 命令: free -h && df -h
```

**修复操作**：

```bash
# 场景 1: 安全组规则缺失
# 控制台 → VPC → 安全组 → 添加入站规则:
#   TCP 10250   (kubelet API)
#   TCP 10255   (kubelet 只读)
#   TCP 6443    (API Server)
#   TCP 2379-2380 (etcd)
#   UDP 8472    (VXLAN / Flannel)

# 场景 2: 重置节点
# 控制台 → 节点管理 → 重置节点
# 或命令行:
# cce reset node --cluster-id <id> --node-id <id>

# 场景 3: 移除并重新纳管
# 控制台 → 节点管理 → 移除节点 → 等待完成 → 重新添加
```

### 1.2 节点频繁 NotReady

**症状**：节点间歇性变为 NotReady，随后恢复

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 NotReady 事件历史
kubectl get events --field-selector involvedObject.name=node-001 | grep NotReady

# 常见原因:
# 1. kubelet 心跳超时 (CPU 高负载导致 kubelet 响应慢)
# 2. 网络抖动 (VPC 质量问题)
# 3. 磁盘压力 (容器日志/镜像撑满磁盘)
# 4. OOM (kubelet 被系统 OOM Killer 杀掉)

# 查看系统 OOM 记录
dmesg | grep -i oom | tail -20

# 查看 kubelet 最近重启
systemctl status kubelet
journalctl -u kubelet --since "1 hour ago" | grep -i "restart\|crash\|fail"
```
**修复操作**：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 磁盘清理
# 清理未使用的容器镜像
crictl rmi --prune

# 清理容器日志 (临时)
find /var/log/containers -name "*.log" -size +100M -exec truncate -s 50M {} \;

# 调整 kubelet 驱逐阈值
# /var/lib/kubelet/config.yaml
evictionHard:
  memory.available: "500Mi"
  nodefs.available: "10%"
  imagefs.available: "15%"
```
## 2. Addon 异常

### 2.1 Addon 状态检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看所有 Addon 状态
kubectl get deployments,daemonsets -n kube-system
# 重点关注:
# - coredns
# - everest-csi-controller / csi-nodeplugin
# - cce-agent
# - npd (Node Problem Detector)
# - metrics-server

# 查看 Addon 详情
kubectl describe deployment coredns -n kube-system

# 查看 Addon Pod 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100
```
### 2.2 CoreDNS 异常

**症状**：集群内 DNS 解析失败

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 测试 DNS 解析
kubectl run dns-test --image=busybox:1.36 --rm -it -- nslookup kubernetes.default

# 检查 CoreDNS Pod
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# 常见错误:
# - "no endpoints available" → CoreDNS 后端异常
# - "connection refused" → 上游 DNS 不通
# - "SERVFAIL" → CoreDNS 配置错误
```
**修复操作**：

```yaml
# 修复 1: 重置 CoreDNS 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health {
            lameduck 5s
        }
        ready
        kubernetes cluster.local in-addr.arpa ip6.arpa {
            pods insecure
            fallthrough in-addr.arpa ip6.arpa
            ttl 30
        }
        forward . /etc/resolv.conf {
            max_concurrent 1000
        }
        cache 30
        loop
        reload
        loadbalance
    }
```

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 修复 2: 重启 CoreDNS
kubectl rollout restart deployment coredns -n kube-system

# 修复 3: 扩容 CoreDNS (大规模集群)
kubectl scale deployment coredns -n kube-system --replicas=5
```
### 2.3 CSI 插件异常

**症状**：PVC 挂载失败或 PV 创建超时

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 CSI 控制器
kubectl get pods -n kube-system | grep csi
kubectl logs -n kube-system -l app=everest-csi-controller --tail=50

# 检查 CSI 节点插件
kubectl get pods -n kube-system -l app=csi-nodeplugin
kubectl logs -n kube-system -l app=csi-nodeplugin -c everest-csi-plugin --tail=50

# 常见错误:
# - "Volume not found" → EVS 卷被误删
# - "AttachVolume.Attach failed" → 可用区不匹配
# - "MountVolume.SetUp failed" → 文件系统损坏
```
**修复操作**：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 强制卸载残留卷 (节点上执行)
umount /var/lib/kubelet/pods/<pod-uid>/volumes/kubernetes.io~csi/<pv-id>

# 2. 重启 CSI 插件
kubectl rollout restart daemonset csi-nodeplugin -n kube-system

# 3. 清理 Finalizer (PV 删除卡住)
kubectl patch pv <pv-name> -p '{"metadata":{"finalizers":null}}'
```
### 2.4 CCE Agent 异常

**症状**：控制台与集群状态不同步、操作超时

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 CCE Agent
kubectl get pods -n kube-system -l app=cce-agent
kubectl logs -n kube-system -l app=cce-agent --tail=100

# 常见问题:
# - Agent 与 CCE 控制面连接断开
# - Agent 版本过旧
# - 节点资源不足导致 Agent 被驱逐

# 修复: 重启 CCE Agent
kubectl rollout restart daemonset cce-agent -n kube-system
```
## 3. CCE Turbo 网络问题

### 3.1 Pod 网络不通

**症状**：Pod 无法访问其他 Pod 或外部网络

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查 Pod 网络配置
kubectl get pod <pod-name> -o jsonpath='{.metadata.annotations}'

# 2. 检查 ENI 分配状态 (Cloud Native 2.0)
kubectl get node <node-name> -o jsonpath='{.status.capacity}'

# 3. 测试 Pod 内网络
kubectl exec -it <pod-name> -- ping -c 3 8.8.8.8
kubectl exec -it <pod-name> -- nslookup kubernetes.default

# 4. 检查 Pod 网卡配置
kubectl exec -it <pod-name> -- ip addr show
kubectl exec -it <pod-name> -- ip route show
```
### 3.2 ENI 分配失败

```
# 🟢 低风险：只读/信息收集，通常无副作用
排查流程:
├── 1. 节点 ENI 配额用尽
│   ├── 检查: kubectl describe node <node> | grep eni
│   └── 解决: 升级节点规格 (更多 ENI 配额)
│
├── 2. 安全组规则阻止
│   ├── 检查: 控制台 → VPC → 安全组
│   └── 解决: 放行容器 CIDR 流量
│
├── 3. 子网 IP 不足
│   ├── 检查: 控制台 → VPC → 子网 → 可用 IP 数
│   └── 解决: 扩展子网或添加新子网
│
└── 4. ENI 创建 API 限流
    ├── 检查: CCE 控制台 → 事件 → 限流相关
    └── 解决: 联系华为云提升 API 配额
```
### 3.3 Service 负载均衡异常

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Service Endpoints
kubectl get endpoints <service-name>

# 检查 ELB 状态
kubectl describe service <service-name> | grep -A 5 "Annotations"

# 常见问题:
# - Endpoints 为空 → 选择器不匹配 或 Pod 未就绪
# - ELB 创建失败 → 配额不足或子网不可用
# - 健康检查失败 → targetPort 配置错误

# 验证后端健康 (通过 ELB 控制台)
# 控制台 → 弹性负载均衡 → 后端服务器组 → 健康检查状态
```
### 3.4 网络延迟与丢包

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Pod 间延迟测试
kubectl exec -it <pod-a> -- ping -c 100 <pod-b-ip> | tail -3
# rtt min/avg/max/mdev = 0.1/0.3/1.2/0.1 ms

# 丢包检测
kubectl exec -it <pod-a> -- ping -c 1000 <pod-b-ip> | tail -5
# 1000 packets transmitted, 998 received, 0.2% packet loss

# 节点上抓包分析
tcpdump -i eth0 host <pod-b-ip> -nn -c 100

# 检查网卡丢包统计
cat /proc/net/dev
ethtool -S eth0 | grep -i drop
```
## 4. 日志采集配置

### 4.1 CCE 日志采集架构

```
Pod 容器日志 → /var/log/containers/*.log
    │
    ▼
日志采集 Agent (icagent / Fluent Bit)
    │
    ├──► LTS (云日志服务) ← 推荐
    ├──► Elasticsearch
    └──► Kafka
```

### 4.2 启用日志采集

```yaml
# 通过 CCE 控制台开启日志采集
# 集群 → 插件管理 → 日志采集 → 安装 icagent

# 或通过 DaemonSet 部署 Fluent Bit
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: fluent-bit
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: fluent-bit
  template:
    metadata:
      labels:
        app: fluent-bit
    spec:
      containers:
        - name: fluent-bit
          image: fluent/fluent-bit:2.2
          volumeMounts:
            - name: varlog
              mountPath: /var/log
              readOnly: true
            - name: containers
              mountPath: /var/lib/docker/containers
              readOnly: true
            - name: config
              mountPath: /fluent-bit/etc/
          resources:
            requests:
              cpu: 50m
              memory: 64Mi
            limits:
              cpu: 200m
              memory: 256Mi
      volumes:
        - name: varlog
          hostPath:
            path: /var/log
        - name: containers
          hostPath:
            path: /var/lib/docker/containers
        - name: config
          configMap:
            name: fluent-bit-config
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-config
  namespace: kube-system
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush         5
        Log_Level     info
        Daemon        off
        Parsers_File  parsers.conf

    [INPUT]
        Name              tail
        Tag               kube.*
        Path              /var/log/containers/*.log
        Parser            cri
        Mem_Buf_Limit     5MB
        Refresh_Interval  10

    [FILTER]
        Name                kubernetes
        Match               kube.*
        Kube_URL            https://kubernetes.default.svc:443
        Kube_CA_File        /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
        Kube_Token_File     /var/run/secrets/kubernetes.io/serviceaccount/token
        Merge_Log           On
        K8S-Logging.Parser  On

    [OUTPUT]
        Name            es
        Match           *
        Host            elasticsearch.logging.svc
        Port            9200
        Index           k8s-logs
        Type            _doc
```

### 4.3 日志查询 (LTS)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 通过 CCE 控制台查看日志
# 集群 → 工作负载 → Pod → 日志

# 通过 kubectl 查看日志
kubectl logs <pod-name> -n <namespace> --tail=100 -f

# 查看上一个崩溃容器的日志
kubectl logs <pod-name> --previous

# LTS 日志查询 (通过 API)
curl -X POST "https://lts.myhuaweicloud.com/<project-id>/groups/<group-id>/streams/<stream-id>/content/query" \
  -H "Content-Type: application/json" \
  -H "X-Auth-Token: <token>" \
  -d '{
    "start_time": "1719900000000",
    "end_time": "1719903600000",
    "query": "level:error AND namespace:production",
    "limit": 100
  }'
```
## 5. 监控告警配置

### 5.1 CCE 监控体系

```
CCE 监控架构:
├── 指标采集 (Prometheus / 云监控 CES)
│   ├── 节点指标: CPU/内存/磁盘/网络
│   ├── Pod 指标: CPU/内存/重启次数
│   ├── 容器指标: 资源使用率
│   └── 自定义指标: 业务指标
│
├── 日志采集 (LTS / Fluent Bit)
│   ├── 容器 stdout/stderr
│   ├── 容器日志文件
│   └── 节点系统日志
│
└── 告警 (CES 告警服务)
    ├── 阈值告警
    ├── 事件告警
    └── 组合告警
```

### 5.2 Prometheus 监控配置

```yaml
# CCE 内置 Prometheus 插件配置
# 集群 → 插件管理 → 云原生监控插件 → 安装

# ServiceMonitor 示例 (自定义指标采集)
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: my-app-monitor
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: my-app
  endpoints:
    - port: metrics
      interval: 15s
      path: /metrics
  namespaceSelector:
    matchNames:
      - production
```

### 5.3 云监控告警规则

```yaml
# 通过 CCE 控制台 → 集群 → 告警规则 创建

# 推荐告警规则:
# 1. 节点 CPU 使用率 > 85% 持续 5 分钟
# 2. 节点内存使用率 > 90% 持续 5 分钟
# 3. 节点磁盘使用率 > 85%
# 4. Pod 重启次数 > 5 次 / 10 分钟
# 5. Pod 处于 Pending 状态 > 10 分钟
# 6. Node NotReady 持续 3 分钟
# 7. PVC 使用率 > 80%

# 通过 API 创建告警规则
curl -X POST "https://ces.myhuaweicloud.com/V1.0/<project-id>/alarms" \
  -H "Content-Type: application/json" \
  -H "X-Auth-Token: <token>" \
  -d '{
    "alarm_name": "Node-High-CPU",
    "namespace": "SYS.CCE",
    "dimensions": [
      {
        "name": "cluster_id",
        "value": "<cluster-id>"
      }
    ],
    "metric_name": "cpu_utilization",
    "condition": {
      "operator": ">",
      "value": 85,
      "period": 300,
      "count": 3
    },
    "action_enabled": true,
    "alarm_actions": [
      {
        "type": "notification",
        "notification_list": ["urn:smn:cn-north-4:<project-id>:ops-alerts"]
      }
    ]
  }'
```

### 5.4 Grafana 集成

```yaml
# 部署 Grafana (如未使用 CCE 内置监控)
apiVersion: apps/v1
kind: Deployment
metadata:
  name: grafana
  namespace: monitoring
spec:
  replicas: 1
  selector:
    matchLabels:
      app: grafana
  template:
    metadata:
      labels:
        app: grafana
    spec:
      containers:
        - name: grafana
          image: grafana/grafana:10.2
          ports:
            - containerPort: 3000
          env:
            - name: GF_SECURITY_ADMIN_PASSWORD
              valueFrom:
                secretKeyRef:
                  name: grafana-secret
                  key: admin-password
          volumeMounts:
            - name: grafana-storage
              mountPath: /var/lib/grafana
      volumes:
        - name: grafana-storage
          persistentVolumeClaim:
            claimName: grafana-pvc
```

## 6. 常见故障速查表

| 症状 | 可能原因 | 排查命令 | 修复操作 |
|------|---------|---------|---------|
| Pod Pending | 资源不足 | `kubectl describe pod` | 扩容节点或调整 requests |
| Pod CrashLoop | 应用崩溃 | `kubectl logs --previous` | 修复应用或调整资源 limits |
| Node NotReady | kubelet 异常 | `journalctl -u kubelet` | 重启 kubelet 或重置节点 |
| DNS 解析失败 | CoreDNS 异常 | `nslookup kubernetes.default` | 重启/扩容 CoreDNS |
| PVC Pending | 存储配额不足 | `kubectl describe pvc` | 扩容或更换可用区 |
| Service 不通 | Endpoints 为空 | `kubectl get endpoints` | 检查 selector 匹配 |
| 节点磁盘满 | 日志/镜像堆积 | `df -h` | 清理或调整驱逐阈值 |
| API Server 超时 | 控制面过载 | CCE 控制台状态 | 扩容控制面或联系支持 |

---

*本文档描述 CCE 常见故障的排查方法。具体操作以华为云官方文档为准。*

## See Also

- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-12-cloud-providers/07-huawei-cce/01-huawei-cce-production-runbook|华为云 CCE 生产运行手册]]


<!-- risk-assessed -->
