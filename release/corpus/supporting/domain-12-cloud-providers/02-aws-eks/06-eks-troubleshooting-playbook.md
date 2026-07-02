---
title: EKS 故障排查手册
description: 'EKS 节点组问题、Addon 冲突、Fargate 调度失败、ALB Ingress 及常见错误码解析'
summary: 'EKS 节点组问题、Addon 冲突、Fargate 调度失败、ALB Ingress 及常见错误码解析'
category: cloud-providers
tags:
- cloud
- k8s
- aws
- eks
- troubleshooting
- debugging
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 支持工程师
estimated_read_time: 15min
intent_queries:
- EKS 故障排查 是什么
- 如何排查 EKS 常见问题
trigger_keywords:
- eks-troubleshooting
- node-group-issues
- addon-conflict
- fargate-scheduling
- alb-ingress
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

# EKS 故障排查手册

## 1. 节点组问题

### 1.1 节点 NotReady

```bash
# 快速诊断
kubectl get nodes -o wide
kubectl describe node <node-name> | grep -A 20 "Conditions:"

# 常见原因排查
# 1. Kubelet 进程异常
kubectl get nodes -o json | jq '.items[] | {
  name: .metadata.name,
  ready: (.status.conditions[] | select(.type=="Ready") | .status),
  reason: (.status.conditions[] | select(.type=="Ready") | .reason),
  message: (.status.conditions[] | select(.type=="Ready") | .message)
}'

# 2. 检查系统日志
# 通过 SSM 登录节点
aws ssm start-session --target <instance-id>

# 查看 kubelet 日志
journalctl -u kubelet -f --no-pager | tail -100

# 3. 检查资源压力
kubectl describe node <node-name> | grep -A 10 "Conditions:"
# 关注 MemoryPressure, DiskPressure, PIDPressure
```

### 1.2 节点无法加入集群

```bash
# 检查节点 IAM Role 权限
aws iam list-attached-role-policies --role-name eks-node-role
# 必须包含:
# - AmazonEKSWorkerNodePolicy
# - AmazonEKS_CNI_Policy
# - AmazonEC2ContainerRegistryReadOnly

# 检查 Security Group
aws ec2 describe-security-groups --group-ids <sg-id> \
  --query 'SecurityGroups[*].IpPermissions'

# 检查节点 bootstrap 日志
cat /var/log/cloud-init-output.log | grep -i error
cat /var/log/kubelet/kubelet.log | tail -50

# 常见错误
# "Unable to register node with API server" → IAM 权限不足
# "Failed to connect to apiserver" → Security Group 或网络问题
# "x509: certificate signed by unknown authority" → CA Bundle 问题
```

### 1.3 Managed Node Group 升级卡住

```bash
# 查看 ASG 状态
aws autoscaling describe-auto-scaling-groups \
  --auto-scaling-group-names <asg-name> \
  --query 'AutoScalingGroups[*].{Instances:Instances[*].{Id:InstanceId,LifecycleState:LifecycleState},Status:Status}'

# 手动取消卡住的更新
aws eks update-nodegroup-config \
  --cluster-name prod-cluster \
  --nodegroup-name <nodegroup> \
  --scaling-config minSize=3,maxSize=10,desiredSize=5

# 如果节点 Drain 卡住
kubectl uncordon <node-name>
kubectl describe node <node-name> | grep -A 5 "Allocated resources"
# 检查是否有 PDB 阻止驱逐
kubectl get pdb -A
```

### 1.4 Spot 实例中断处理

```bash
# 查看 Spot 中断事件
kubectl get events --field-selector reason=NodeNotReady

# 配置 Spot 中断处理
kubectl apply -f - <<EOF
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: node-termination-handler
  namespace: kube-system
spec:
  selector:
    matchLabels:
      app: node-termination-handler
  template:
    metadata:
      labels:
        app: node-termination-handler
    spec:
      serviceAccountName: node-termination-handler
      containers:
        - name: nth
          image: public.ecr.aws/aws-ec2/aws-node-termination-handler:v1.22.0
          env:
            - name: QUEUE_URL
              value: "https://sqs.ap-southeast-1.amazonaws.com/123456789012/spot-interrupt"
            - name: ENABLE_SPOT_INTERRUPTION_DRAINING
              value: "true"
            - name: ENABLE_REBALANCE_RECOMMENDATION
              value: "true"
EOF
```

## 2. Addon 版本冲突

### 2.1 Addon 版本兼容性

```bash
# 查看可用 addon 版本
aws eks describe-addon-versions \
  --addon-name vpc-cni \
  --kubernetes-version 1.31 \
  --query 'addons[*].addonVersions[*].{Version: addonVersion, K8sVersion: compatibility[*].clusterVersion}'

# 查看当前集群 addon 状态
aws eks list-addons --cluster-name prod-cluster \
  --query 'addons[*]' | while read addon; do
    aws eks describe-addon \
      --cluster-name prod-cluster \
      --addon-name "$addon" \
      --query '{name: addon.addonName, version: addon.addonVersion, status: addon.status}'
  done
```

### 2.2 Addon 升级失败

```bash
# 查看 addon 事件
aws eks describe-addon \
  --cluster-name prod-cluster \
  --addon-name vpc-cni \
  --query 'addon.health.issues'

# 强制覆盖（谨慎使用）
aws eks update-addon \
  --cluster-name prod-cluster \
  --addon-name vpc-cni \
  --addon-version v1.18.1-eksbuild.1 \
  --resolve-conflicts OVERWRITE

# 手动回滚 addon
kubectl rollout undo daemonset/aws-node -n kube-system
```

### 2.3 CoreDNS 问题

```bash
# 检查 CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# 常见问题: CoreDNS Pod 无法调度
kubectl describe pod -n kube-system -l k8s-app=kube-dns | grep -A 5 "Events:"

# 如果是 IP 地址不足
kubectl get pods -A -o json | jq '[.items[] | .status.podIP] | length'
# 解决: 启用 Prefix Delegation 或添加 Secondary CIDR

# CoreDNS 性能问题
# 检查 DNS 延迟
kubectl run dnsperf --image=infoblox/dnstools --rm -it -- \
  nslookup kubernetes.default.svc.cluster.local
```

## 3. Fargate 调度失败

### 3.1 Fargate Pod 一直处于 Pending

```bash
# 查看 Fargate Profile 状态
aws eks describe-fargate-profile \
  --cluster-name prod-cluster \
  --fargate-profile-name batch-jobs \
  --query 'fargateProfile.status'

# 检查 Pod 事件
kubectl describe pod <pod-name> -n <namespace>
# 常见错误:
# "Pod's node selector doesn't match Fargate profile selectors"
# "FailedScheduling: 0/N nodes are available"

# 验证 Profile Selector 匹配
kubectl get pod <pod-name> -n <namespace> -o json \
  | jq '{namespace: .metadata.namespace, labels: .metadata.labels}'
# 确认与 Profile 的 selectors 匹配

# 检查 Pod Execution Role
aws iam get-role --role-name eks-fargate-role \
  --query 'Role.AssumeRolePolicyDocument'
# 必须包含 ecs-tasks.amazonaws.com 的信任关系
```

### 3.2 Fargate Pod 启动缓慢

```bash
# Fargate 冷启动通常 30-60 秒
# 优化: 使用 init container 预热

# 查看 Fargate 启动时间
kubectl get pod <pod-name> -o json \
  | jq '{
    created: .metadata.creationTimestamp,
    started: .status.containerStatuses[0].state.running.startedAt,
    ready: (.status.conditions[] | select(.type=="Ready") | .lastTransitionTime)
  }'
```

### 3.3 Fargate 存储限制

```bash
# Fargate 仅支持 EFS（不支持 EBS）
# 如果 PVC 使用 EBS StorageClass，Pod 会调度失败

# 检查 PVC 绑定状态
kubectl get pvc -n <namespace>
kubectl describe pvc <pvc-name> -n <namespace>

# 解决: 使用 EFS StorageClass
# 参考 04-eks-storage-efs-fsx.md
```

## 4. ALB Ingress 问题

### 4.1 ALB Controller 安装验证

```bash
# 检查 ALB Controller 状态
kubectl get pods -n kube-system -l app.kubernetes.io/name=aws-load-balancer-controller
kubectl logs -n kube-system -l app.kubernetes.io/name=aws-load-balancer-controller --tail=50

# 验证 IAM 权限
kubectl describe sa aws-load-balancer-controller -n kube-system
# 确认 eks.amazonaws.com/role-arn 注解正确
```

### 4.2 Ingress 不创建 ALB

```bash
# 检查 Ingress 注解
kubectl describe ingress <ingress-name> -n <namespace>

# 必需注解
# kubernetes.io/ingress.class: alb
# alb.ingress.kubernetes.io/scheme: internet-facing 或 internal
# alb.ingress.kubernetes.io/target-type: ip 或 instance

# 检查 Controller 日志中的错误
kubectl logs -n kube-system -l app.kubernetes.io/name=aws-load-balancer-controller \
  | grep -i error | tail -20

# 常见错误:
# "AuthConfig not found" → Service Account IRSA 未配置
# "AccessDenied" → IAM 权限不足
# "SubnetDiscoveryFailed" → 子网缺少 kubernetes.io/role/elb 标签
```

### 4.3 子网标签问题

```bash
# 检查子网标签
aws ec2 describe-subnets \
  --filters "Name=vpc-id,Values=vpc-0123456789abcdef0" \
  --query 'Subnets[*].{SubnetId:SubnetId,Tags:Tags}' | jq .

# 公有子网需要标签
# kubernetes.io/role/elb = 1
# 私有子网需要标签
# kubernetes.io/role/internal-elb = 1

# 自动标签
aws ec2 create-tags \
  --resources subnet-aaaa subnet-bbbb subnet-cccc \
  --tags Key=kubernetes.io/role/elb,Value=1
```

### 4.4 Target Group 健康检查失败

```bash
# 查看 Target Group 健康状态
aws elbv2 describe-target-health \
  --target-group-arn <tg-arn> \
  --query 'TargetHealthDescriptions[*].{Target:Target.Id,Health:TargetHealth.State,Reason:TargetHealth.Reason}'

# 常见原因:
# 1. Pod 端口不匹配
kubectl get svc <service-name> -n <namespace> -o jsonpath='{.spec.ports[*]}'

# 2. 健康检查路径返回非 200
kubectl exec -it <pod-name> -n <namespace> -- curl -s -o /dev/null -w "%{http_code}" /healthz

# 3. 安全组不允许 ALB → Pod 通信
# 检查 Pod Security Group
```

## 5. CloudWatch 日志配置

### 5.1 控制平面日志

```bash
# 启用日志
aws eks update-cluster-config \
  --name prod-cluster \
  --logging '{"clusterLogging":[{"types":["api","audit","authenticator","controllerManager","scheduler"],"enabled":true}]}'

# 查看日志
aws logs describe-log-streams \
  --log-group-name /aws/eks/prod-cluster/cluster \
  --order-by LastEventTime \
  --descending

# CloudWatch Insights 查询 — 查找认证失败
fields @timestamp, @message
| filter @message like /authentication/
| filter @message like /denied/
| sort @timestamp desc
| limit 20
```

### 5.2 节点和 Pod 日志

```yaml
# Fluent Bit DaemonSet（EKS 最佳实践）
apiVersion: v1
kind: ConfigMap
metadata:
  name: fluent-bit-config
  namespace: amazon-cloudwatch
data:
  fluent-bit.conf: |
    [SERVICE]
        Flush 5
        Log_Level info
        Daemon off
        Parsers_File parsers.conf

    [INPUT]
        Name tail
        Path /var/log/containers/*.log
        multiline.parser docker, cri
        Tag kube.*
        Mem_Buf_Limit 5MB
        Skip_Long_Lines On

    [FILTER]
        Name kubernetes
        Match kube.*
        Merge_Log On
        Keep_Log Off
        K8S-Logging.Parser On
        K8S-Logging.Exclude On

    [OUTPUT]
        Name cloudwatch_logs
        Match kube.*
        region ap-southeast-1
        log_group_name /eks/containers
        log_stream_prefix from-fluent-bit-
        auto_create_group true
```

## 6. 常见错误码解析

### 6.1 API Server 错误

| 错误码 | 含义 | 解决方案 |
|--------|------|---------|
| 403 Forbidden | RBAC 或 IAM 权限不足 | 检查 ClusterRoleBinding 和 IAM Policy |
| 429 Too Many Requests | API 限流 | 减少请求频率，使用 informer 缓存 |
| 500 Internal Server Error | 控制平面内部错误 | 检查控制平面日志，联系 AWS 支持 |
| 503 Service Unavailable | 控制平面正在维护 | 等待或检查集群升级状态 |

### 6.2 常见 Kubernetes 错误

```bash
# CrashLoopBackOff
kubectl describe pod <pod-name> | grep -A 10 "Last State"
kubectl logs <pod-name> --previous
# 常见原因: 启动命令错误、依赖服务不可用、内存不足

# ImagePullBackOff
kubectl describe pod <pod-name> | grep -A 5 "Events:"
# 常见原因: 镜像名错误、ECR 认证过期、网络不通
# ECR 认证:
aws ecr get-login-password --region ap-southeast-1 | \
  docker login --username AWS --password-stdin 123456789012.dkr.ecr.ap-southeast-1.amazonaws.com

# OOMKilled
kubectl describe pod <pod-name> | grep -A 5 "Last State"
kubectl top pod <pod-name>
# 解决: 增加 memory limits 或优化应用

# Evicted
kubectl get events --field-selector reason=Evicted
# 常见原因: 节点磁盘压力、内存压力
# 解决: 清理节点磁盘或增加节点容量
```

### 6.3 网络相关错误

```bash
# DNS 解析失败
kubectl run dnstest --image=busybox --rm -it -- nslookup kubernetes.default
# 如果失败: 检查 CoreDNS Pod 状态和日志

# Service 无法访问
kubectl get endpoints <service-name>
# 如果 Endpoints 为空: 检查 selector 匹配和 Pod 状态

# Pod 跨节点通信失败
# 检查 VPC CNI 日志
kubectl logs -n kube-system -l k8s-app=aws-node --tail=50
# 检查 Security Group 规则是否允许 Pod IP 段通信
```

## 7. 诊断快速命令集

```bash
# 集群健康快速检查
echo "=== Cluster Info ===" && \
kubectl cluster-info && \
echo "=== Nodes ===" && \
kubectl get nodes -o wide && \
echo "=== System Pods ===" && \
kubectl get pods -n kube-system && \
echo "=== Events (last 5min) ===" && \
kubectl get events --sort-by='.lastTimestamp' | tail -20 && \
echo "=== Resource Usage ===" && \
kubectl top nodes

# 节点资源使用详情
kubectl describe nodes | grep -A 5 "Allocated resources"

# 所有非 Running Pod
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded

# 查找 OOMKilled Pod
kubectl get pods -A -o json | jq '.items[] | select(.status.containerStatuses[]?.lastState.terminated.reason=="OOMKilled") | {name: .metadata.name, namespace: .metadata.namespace}'
```

## Related

- [[02-eks-cluster-lifecycle-management]]
- [[03-eks-networking-vpc-cni]]

## See Also

- EKS Troubleshooting Guide
- Kubernetes Troubleshooting
