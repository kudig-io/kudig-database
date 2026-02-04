# 29 - 云提供商集成故障排查 (Cloud Provider Integration Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [Kubernetes Cloud Providers](https://kubernetes.io/docs/concepts/cluster-administration/cloud-providers/)

---

## 1. 云提供商集成故障诊断总览 (Cloud Provider Integration Diagnosis Overview)

### 1.1 常见故障现象分类

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **认证凭证失效** | API调用401/403 | 云资源操作失败 | P0 - 紧急 |
| **网络配置错误** | LoadBalancer服务卡住 | 外部访问中断 | P0 - 紧急 |
| **存储卷挂载失败** | PVC Pending状态 | 数据持久化失败 | P0 - 紧急 |
| **实例元数据异常** | 节点信息不准确 | 调度决策错误 | P1 - 高 |
| **API速率限制** | 请求被节流 | 操作延迟/失败 | P1 - 高 |

### 1.2 云提供商集成架构回顾

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                 云提供商集成故障诊断架构                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                       Kubernetes控制面                                │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │  API Server │    │  Scheduler  │    │  Controller │              │  │
│  │  │             │    │             │    │  Manager    │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   Cloud     │    │   Storage   │    │   Network   │                   │
│  │  Controller │    │  Provisioner│    │  Controller │                   │
│  │  Manager    │    │             │    │             │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    云提供商API接口                                   │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   Compute   │    │   Storage   │    │   Network   │              │  │
│  │  │   API       │    │   API       │    │   API       │              │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│         ┌────────────────────┼────────────────────┐                       │
│         │                    │                    │                       │
│         ▼                    ▼                    ▼                       │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐                   │
│  │   实例管理   │   │   存储管理   │   │   网络管理   │                   │
│  │ (EC2/VM)    │   │ (EBS/Disk)  │   │ (ELB/VPC)   │                   │
│  │   创建/删除  │   │   挂载/卸载  │   │   配置/路由  │                   │
│  └─────────────┘    └─────────────┘    └─────────────┘                   │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                      云基础设施层                                     │  │
│  │  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐              │  │
│  │  │   虚拟机     │   │   存储卷     │   │   负载均衡器  │              │  │
│  │  │ (Instances) │   │ (Volumes)   │   │ (LoadBalancers)│             │  │
│  │  └─────────────┘    └─────────────┘    └─────────────┘              │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 云提供商认证和权限问题 (Cloud Provider Authentication and Permissions)

### 2.1 凭证状态检查

```bash
# ========== 1. 云提供商凭证验证 ==========
# AWS凭证检查
kubectl run aws-cli-test --image=amazon/aws-cli -n kube-system -it --rm -- sh -c "
echo '=== AWS Credentials Check ==='
aws sts get-caller-identity
aws ec2 describe-availability-zones --region us-west-2
"

# GCP凭证检查
kubectl run gcp-cli-test --image=gcr.io/google.com/cloudsdktool/cloud-sdk -n kube-system -it --rm -- sh -c "
echo '=== GCP Credentials Check ==='
gcloud auth list
gcloud compute zones list --filter='region:us-central1'
"

# Azure凭证检查
kubectl run azure-cli-test --image=mcr.microsoft.com/azure-cli -n kube-system -it --rm -- sh -c "
echo '=== Azure Credentials Check ==='
az account show
az account list-locations --query '[].name' -o table
"

# ========== 2. Kubernetes云控制器管理器检查 ==========
# 检查云控制器管理器状态
kubectl get pods -n kube-system | grep cloud-controller

# 查看云控制器日志
kubectl logs -n kube-system -l k8s-app=cloud-controller-manager --tail=100

# 验证云提供商配置
kubectl get configmap -n kube-system cloud-config -o yaml 2>/dev/null || echo "No cloud-config found"

# ========== 3. 节点云提供商标签检查 ==========
# 检查节点云提供商信息
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.providerID
}{
        "\t"
}{
        .metadata.labels."failure-domain\.beta\.kubernetes\.io/zone"
}{
        "\n"
}{
    end
}'

# 验证实例元数据服务可达性
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
    echo "=== Node: $node ==="
    kubectl debug node/$node --image=busybox -it -- sh -c "
        echo 'Testing metadata service:'
        wget -q -O - http://169.254.169.254/latest/meta-data/instance-id 2>/dev/null || echo 'Metadata service not accessible'
    "
done
```

### 2.2 权限不足问题排查

```bash
# ========== AWS IAM权限检查 ==========
# 检查必需的IAM策略
cat <<'EOF' > aws-permissions-check.sh
#!/bin/bash

echo "Checking AWS IAM permissions for Kubernetes"

REQUIRED_PERMISSIONS=(
    "ec2:DescribeInstances"
    "ec2:DescribeRegions"
    "ec2:DescribeRouteTables"
    "ec2:DescribeSecurityGroups"
    "ec2:DescribeSubnets"
    "ec2:DescribeVolumes"
    "ec2:CreateSecurityGroup"
    "ec2:CreateTags"
    "ec2:CreateVolume"
    "ec2:ModifyInstanceAttribute"
    "ec2:ModifyVolume"
    "ec2:AttachVolume"
    "ec2:AuthorizeSecurityGroupIngress"
    "ec2:DeleteSecurityGroup"
    "ec2:DeleteVolume"
    "ec2:DetachVolume"
    "ec2:RevokeSecurityGroupIngress"
    "ec2:DescribeVpcs"
    "elasticloadbalancing:AddTags"
    "elasticloadbalancing:AttachLoadBalancerToSubnets"
    "elasticloadbalancing:ApplySecurityGroupsToLoadBalancer"
    "elasticloadbalancing:CreateLoadBalancer"
    "elasticloadbalancing:CreateLoadBalancerPolicy"
    "elasticloadbalancing:CreateLoadBalancerListeners"
    "elasticloadbalancing:ConfigureHealthCheck"
    "elasticloadbalancing:DeleteLoadBalancer"
    "elasticloadbalancing:DeleteLoadBalancerListeners"
    "elasticloadbalancing:DescribeLoadBalancers"
    "elasticloadbalancing:DescribeLoadBalancerAttributes"
    "elasticloadbalancing:DetachLoadBalancerFromSubnets"
    "elasticloadbalancing:DeregisterInstancesFromLoadBalancer"
    "elasticloadbalancing:ModifyLoadBalancerAttributes"
    "elasticloadbalancing:RegisterInstancesWithLoadBalancer"
    "elasticloadbalancing:SetLoadBalancerPoliciesForBackendServer"
    "elasticloadbalancing:AddTags"
    "elasticloadbalancing:RemoveTags"
    "autoscaling:DescribeAutoScalingGroups"
    "autoscaling:DescribeLaunchConfigurations"
    "autoscaling:DescribeTags"
    "autoscaling:CreateOrUpdateTags"
    "autoscaling:UpdateAutoScalingGroup"
    "autoscaling:SetDesiredCapacity"
    "autoscaling:TerminateInstanceInAutoScalingGroup"
)

# 测试每个权限
for permission in "${REQUIRED_PERMISSIONS[@]}"; do
    echo "Testing: $permission"
    aws iam simulate-principal-policy \
        --policy-source-arn arn:aws:iam::$(aws sts get-caller-identity --query Account --output text):user/kubernetes-controller \
        --action-names $permission \
        --resource-arns "*" 2>/dev/null | jq -r '.EvaluationResults[0].EvalDecision'
done
EOF

chmod +x aws-permissions-check.sh

# ========== GCP IAM权限检查 ==========
cat <<'EOF' > gcp-permissions-check.sh
#!/bin/bash

echo "Checking GCP IAM permissions for Kubernetes"

PROJECT_ID=$(gcloud config get-value project)
SERVICE_ACCOUNT=$(gcloud config get-value account)

REQUIRED_ROLES=(
    "roles/compute.admin"
    "roles/container.admin"
    "roles/iam.serviceAccountUser"
    "roles/storage.admin"
)

for role in "${REQUIRED_ROLES[@]}"; do
    echo "Checking role: $role"
    gcloud projects get-iam-policy $PROJECT_ID \
        --flatten="bindings[].members" \
        --format="table(bindings.role)" \
        --filter="bindings.members:$SERVICE_ACCOUNT AND bindings.role:$role"
done
EOF

chmod +x gcp-permissions-check.sh

# ========== Azure权限检查 ==========
cat <<'EOF' > azure-permissions-check.sh
#!/bin/bash

echo "Checking Azure permissions for Kubernetes"

SUBSCRIPTION_ID=$(az account show --query id -o tsv)
SERVICE_PRINCIPAL=$(az account show --query user.name -o tsv)

REQUIRED_PERMISSIONS=(
    "Microsoft.Compute/virtualMachines/read"
    "Microsoft.Network/loadBalancers/read"
    "Microsoft.Storage/storageAccounts/read"
    "Microsoft.Compute/disks/read"
)

for permission in "${REQUIRED_PERMISSIONS[@]}"; do
    echo "Testing: $permission"
    az role assignment list \
        --assignee $SERVICE_PRINCIPAL \
        --scope /subscriptions/$SUBSCRIPTION_ID \
        --query "[?roleDefinitionName=='Contributor']" 2>/dev/null
done
EOF

chmod +x azure-permissions-check.sh
```

---

## 3. LoadBalancer服务问题排查 (LoadBalancer Service Issues)

### 3.1 LoadBalancer状态检查

```bash
# ========== 1. LoadBalancer服务状态 ==========
# 查看Pending状态的LoadBalancer服务
kubectl get services --all-namespaces --field-selector=spec.type=LoadBalancer | grep -E "(pending|Pending)"

# 查看LoadBalancer详细信息
kubectl describe service <service-name> -n <namespace>

# 检查LoadBalancer事件
kubectl get events --field-selector involvedObject.kind=Service,involvedObject.name=<service-name> --sort-by='.lastTimestamp'

# ========== 2. 云提供商LoadBalancer配置 ==========
# AWS ELB/ALB检查
aws elb describe-load-balancers --load-balancer-names <elb-name> 2>/dev/null || \
aws elbv2 describe-load-balancers --names <alb-name> 2>/dev/null

# GCP LoadBalancer检查
gcloud compute forwarding-rules list --filter="name~.*k8s.*"

# Azure LoadBalancer检查
az network lb list --query "[].name" -o table

# ========== 3. 安全组和网络ACL检查 ==========
# AWS安全组检查
kubectl get service <service-name> -n <namespace> -o jsonpath='{.metadata.annotations.service\.beta\.kubernetes\.io/aws-load-balancer-extra-security-groups}'

# 检查安全组规则
aws ec2 describe-security-groups --group-ids <sg-id> --query 'SecurityGroups[0].IpPermissions'

# 网络ACL检查
aws ec2 describe-network-acls --filters Name=vpc-id,Values=<vpc-id>
```

### 3.2 LoadBalancer配置问题

```bash
# ========== 注解配置检查 ==========
# 查看服务注解配置
kubectl get service <service-name> -n <namespace> -o jsonpath='{.metadata.annotations}'

# AWS特有注解检查
kubectl get service <service-name> -n <namespace> -o jsonpath='{
    .metadata.annotations."service\.beta\.kubernetes\.io/aws-load-balancer-type"
}{
        "\t"
}{
        .metadata.annotations."service\.beta\.kubernetes\.io/aws-load-balancer-cross-zone-load-balancing-enabled"
}{
        "\t"
}{
        .metadata.annotations."service\.beta\.kubernetes\.io/aws-load-balancer-ssl-cert"
}'

# GCP特有注解检查
kubectl get service <service-name> -n <namespace> -o jsonpath='{
    .metadata.annotations."cloud\.google\.com/load-balancer-type"
}{
        "\t"
}{
        .metadata.annotations."networking\.gke\.io/load-balancer-type"
}'

# ========== 健康检查配置 ==========
# 检查健康检查配置
kubectl get service <service-name> -n <namespace> -o jsonpath='{
    .spec.ports[0].targetPort
}{
        "\t"
}{
        .spec.ports[0].protocol
}'

# 验证后端Pod健康状态
kubectl get endpoints <service-name> -n <namespace> -o jsonpath='{.subsets[*].addresses[*].ip}'

# ========== 故障排除工具 ==========
# LoadBalancer诊断脚本
cat <<'EOF' > loadbalancer-diagnostic.sh
#!/bin/bash

SERVICE_NAME=$1
NAMESPACE=${2:-default}

if [ -z "$SERVICE_NAME" ]; then
    echo "Usage: $0 <service-name> [namespace]"
    exit 1
fi

echo "=== LoadBalancer Diagnostic for $SERVICE_NAME ==="

# 1. 服务基本信息
echo "1. Service Information:"
kubectl get service $SERVICE_NAME -n $NAMESPACE -o wide

# 2. 服务详细描述
echo "2. Service Details:"
kubectl describe service $SERVICE_NAME -n $NAMESPACE

# 3. Endpoints状态
echo "3. Endpoints Status:"
kubectl get endpoints $SERVICE_NAME -n $NAMESPACE

# 4. 相关事件
echo "4. Related Events:"
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$SERVICE_NAME --sort-by='.lastTimestamp'

# 5. 后端Pod状态
echo "5. Backend Pods:"
BACKEND_PODS=$(kubectl get endpoints $SERVICE_NAME -n $NAMESPACE -o jsonpath='{.subsets[*].addresses[*].targetRef.name}')
if [ -n "$BACKEND_PODS" ]; then
    for pod in $BACKEND_PODS; do
        echo "  Pod: $pod"
        kubectl get pod $pod -n $NAMESPACE -o jsonpath='{.status.phase} {.status.podIP}'
        echo ""
    done
fi

# 6. 云提供商特定检查 (根据环境调整)
echo "6. Cloud Provider Specific Checks:"
# 这里可以根据检测到的云提供商类型执行相应检查

echo "Diagnostic completed"
EOF

chmod +x loadbalancer-diagnostic.sh
```

---

## 4. 持久化存储问题排查 (Persistent Storage Issues)

### 4.1 存储卷状态检查

```bash
# ========== 1. PVC状态验证 ==========
# 查看Pending状态的PVC
kubectl get pvc --all-namespaces | grep -E "(pending|Pending)"

# 查看PVC详细信息
kubectl describe pvc <pvc-name> -n <namespace>

# 检查PV绑定状态
kubectl get pv | grep <pv-name>

# ========== 2. 存储类配置检查 ==========
# 查看存储类配置
kubectl get storageclass

# 检查默认存储类
kubectl get storageclass | grep "(default)"

# 查看存储类详细配置
kubectl describe storageclass <storageclass-name>

# ========== 3. CSI驱动状态检查 ==========
# 检查CSI驱动Pod状态
kubectl get pods -n kube-system | grep csi

# 查看CSI驱动日志
kubectl logs -n kube-system -l app=csi-driver --tail=100

# 验证CSI节点插件
kubectl get daemonset -n kube-system | grep csi
```

### 4.2 存储卷挂载问题

```bash
# ========== 挂载失败诊断 ==========
# 查看挂载失败的Pod
kubectl get pods --all-namespaces --field-selector=status.phase=Pending -o jsonpath='{
    range .items[?(@.status.containerStatuses[*].state.waiting.reason=="ContainerCreating")]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\n"
}{
    end
}'

# 检查Pod挂载事件
kubectl get events --field-selector involvedObject.kind=Pod,reason=FailedMount --sort-by='.lastTimestamp'

# 分析挂载失败原因
kubectl describe pod <pod-name> -n <namespace> | grep -A10 "Events:"

# ========== 存储容量和配额检查 ==========
# 检查存储配额
kubectl describe quota -n <namespace>

# 查看存储使用情况
kubectl get pvc -n <namespace> -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .status.capacity.storage
}{
        "\t"
}{
        .spec.resources.requests.storage
}{
        "\n"
}{
    end
}'

# ========== 故障排除工具 ==========
# 存储诊断脚本
cat <<'EOF' > storage-diagnostic.sh
#!/bin/bash

NAMESPACE=${1:-default}

echo "=== Storage Diagnostic for namespace: $NAMESPACE ==="

# 1. PVC状态检查
echo "1. PVC Status:"
kubectl get pvc -n $NAMESPACE -o wide

# 2. PV绑定状态
echo "2. PV Binding Status:"
kubectl get pv -o jsonpath='{
    range .items[?(@.spec.claimRef.namespace=="'$NAMESPACE'")]
}{
        .metadata.name
}{
        "\t"
}{
        .spec.claimRef.name
}{
        "\t"
}{
        .status.phase
}{
        "\n"
}{
    end
}'

# 3. 存储类配置
echo "3. Storage Classes:"
kubectl get storageclass -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .provisioner
}{
        "\t"
}{
        .metadata.annotations."storageclass\.kubernetes\.io/is-default-class"
}{
        "\n"
}{
    end
}'

# 4. 挂载失败事件
echo "4. Mount Failures:"
kubectl get events -n $NAMESPACE --field-selector reason=FailedMount --sort-by='.lastTimestamp'

# 5. CSI驱动状态
echo "5. CSI Driver Status:"
kubectl get pods -n kube-system | grep -E "(csi|ebs|disk)"

echo "Storage diagnostic completed"
EOF

chmod +x storage-diagnostic.sh

# ========== 存储性能测试 ==========
# 创建存储性能测试Pod
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Pod
metadata:
  name: storage-benchmark
  namespace: <namespace>
spec:
  containers:
  - name: benchmark
    image: ljishen/fio
    command: ["fio", "--name=test", "--rw=randrw", "--bs=4k", "--iodepth=16", "--size=1g", "--runtime=60", "--direct=1"]
    volumeMounts:
    - name: test-volume
      mountPath: /data
  volumes:
  - name: test-volume
    persistentVolumeClaim:
      claimName: <pvc-name>
EOF
```

---

## 5. 网络和路由问题排查 (Network and Routing Issues)

### 5.1 VPC和子网配置检查

```bash
# ========== 1. 网络配置验证 ==========
# AWS VPC检查
aws ec2 describe-vpcs --vpc-ids <vpc-id> --query 'Vpcs[0].[VpcId,CidrBlock,IsDefault]'

# 子网配置检查
aws ec2 describe-subnets --filters Name=vpc-id,Values=<vpc-id> --query 'Subnets[*].[SubnetId,CidrBlock,AvailabilityZone,State]'

# 路由表检查
aws ec2 describe-route-tables --filters Name=vpc-id,Values=<vpc-id>

# ========== 2. 安全组和网络策略 ==========
# 安全组规则检查
aws ec2 describe-security-groups --group-ids <sg-id> --query 'SecurityGroups[0].IpPermissions'

# 网络ACL检查
aws ec2 describe-network-acls --filters Name=vpc-id,Values=<vpc-id> --query 'NetworkAcls[*].Entries'

# Kubernetes网络策略检查
kubectl get networkpolicy --all-namespaces

# ========== 3. 路由和连通性测试 ==========
# 节点网络配置检查
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
    echo "=== Node: $node ==="
    kubectl debug node/$node --image=nicolaka/netshoot -it -- sh -c "
        echo 'Node network interfaces:'
        ip addr show
        echo 'Routing table:'
        ip route show
        echo 'Testing connectivity:'
        ping -c 3 8.8.8.8
    "
done
```

### 5.2 DNS和域名解析问题

```bash
# ========== 云DNS服务检查 ==========
# AWS Route53检查
aws route53 list-hosted-zones --query 'HostedZones[*].[Id,Name]'

# 检查DNS记录
aws route53 list-resource-record-sets --hosted-zone-id <zone-id>

# GCP Cloud DNS检查
gcloud dns managed-zones list

gcloud dns record-sets list --zone=<zone-name>

# Azure DNS检查
az network dns zone list --query "[].name" -o table

az network dns record-set list --zone-name <zone-name> --resource-group <rg-name>

# ========== 内部DNS配置 ==========
# 检查集群DNS配置
kubectl get configmap -n kube-system coredns -o yaml

# 验证DNS解析
kubectl run dns-test --image=busybox -n <namespace> -it --rm -- sh -c "
nslookup kubernetes.default
nslookup <external-domain>
"
```

---

## 6. 监控和告警配置 (Monitoring and Alerting)

### 6.1 云资源监控配置

```bash
# ========== 云监控集成 ==========
# AWS CloudWatch监控
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: aws-cloudwatch-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: cloudwatch-exporter
  endpoints:
  - port: metrics
    path: /metrics
    interval: 60s
EOF

# GCP Monitoring集成
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: gcp-monitoring-exporter
  namespace: monitoring
spec:
  selector:
    matchLabels:
      app: stackdriver-exporter
  endpoints:
  - port: metrics
    path: /metrics
    interval: 60s
EOF

# ========== 告警规则配置 ==========
cat <<EOF | kubectl apply -f -
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: cloud-provider-alerts
  namespace: monitoring
spec:
  groups:
  - name: cloud.rules
    rules:
    - alert: CloudProviderAPIDown
      expr: up{job=~"cloud-controller-manager|csi-driver"} == 0
      for: 2m
      labels:
        severity: critical
      annotations:
        summary: "Cloud provider API is down"
        
    - alert: LoadBalancerProvisioningFailed
      expr: kube_service_status_load_balancer_ingress == 0
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "LoadBalancer provisioning failed for service {{ \$labels.service }}"
        
    - alert: PersistentVolumeClaimPending
      expr: kube_persistentvolumeclaim_status_phase{phase="Pending"} == 1
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "PersistentVolumeClaim pending for {{ \$labels.persistentvolumeclaim }}"
        
    - alert: CloudAPIRateLimited
      expr: rate(cloudprovider_api_rate_limited_total[5m]) > 10
      for: 2m
      labels:
        severity: warning
      annotations:
        summary: "Cloud provider API rate limiting detected ({{ \$value }}/sec)"
        
    - alert: NodeCloudProviderDisconnected
      expr: kube_node_status_condition{condition="CloudProviderDisconnected",status="true"} == 1
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "Node {{ \$labels.node }} disconnected from cloud provider"
EOF
```

### 6.2 云成本监控

```bash
# ========== 成本监控配置 ==========
# AWS Cost Explorer集成
cat <<'EOF' > aws-cost-monitor.sh
#!/bin/bash

echo "=== AWS Cost Analysis ==="

# 获取本月至今的成本
aws ce get-cost-and-usage \
    --time-period Start=$(date +%Y-%m-01),End=$(date +%Y-%m-%d) \
    --granularity MONTHLY \
    --metrics "BlendedCost" "UnblendedCost" \
    --group-by Type=DIMENSION,Key=SERVICE

# 分析Kubernetes相关服务成本
echo "Kubernetes-related costs:"
aws ce get-cost-and-usage \
    --time-period Start=$(date +%Y-%m-01),End=$(date +%Y-%m-%d) \
    --granularity DAILY \
    --metrics "UnblendedCost" \
    --filter file://k8s-cost-filter.json \
    --group-by Type=DIMENSION,Key=USAGE_TYPE

# 成本异常检测
CURRENT_COST=$(aws ce get-cost-and-usage \
    --time-period Start=$(date -d yesterday +%Y-%m-%d),End=$(date +%Y-%m-%d) \
    --granularity DAILY \
    --metrics "UnblendedCost" \
    --query 'ResultsByTime[0].Total.UnblendedCost.Amount' \
    --output text)

echo "Yesterday's cost: \$${CURRENT_COST}"

# 与上周同期比较
LAST_WEEK_COST=$(aws ce get-cost-and-usage \
    --time-period Start=$(date -d '7 days ago' +%Y-%m-%d),End=$(date -d '6 days ago' +%Y-%m-%d) \
    --granularity DAILY \
    --metrics "UnblendedCost" \
    --query 'ResultsByTime[0].Total.UnblendedCost.Amount' \
    --output text)

echo "Same day last week cost: \$${LAST_WEEK_COST}"

COST_DIFFERENCE=$(echo "$CURRENT_COST - $LAST_WEEK_COST" | bc)
if (( $(echo "$COST_DIFFERENCE > 0" | bc -l) )); then
    echo "⚠️  Cost increased by \$${COST_DIFFERENCE}"
else
    echo "✓ Cost decreased by \$${COST_DIFFERENCE#-}"
fi
EOF

chmod +x aws-cost-monitor.sh

# ========== 资源优化建议 ==========
cat <<'EOF' > resource-optimization-recommender.sh
#!/bin/bash

echo "=== Cloud Resource Optimization Recommendations ==="

# 1. 未使用的资源检测
echo "1. Unused Resources:"
echo "  - Idle LoadBalancers:"
kubectl get service --all-namespaces --field-selector=spec.type=LoadBalancer -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | \
while read svc; do
    ENDPOINTS=$(kubectl get endpoints $svc --all-namespaces -o jsonpath='{.subsets[*].addresses[*]}' 2>/dev/null)
    if [ -z "$ENDPOINTS" ]; then
        echo "    ⚠️  Service $svc has no backend endpoints"
    fi
done

# 2. 存储优化
echo "2. Storage Optimization:"
kubectl get pvc --all-namespaces -o jsonpath='{
    range .items[*]
}{
        .metadata.namespace
}{
        "/"
}{
        .metadata.name
}{
        "\t"
}{
        .status.capacity.storage
}{
        "\t"
}{
        .spec.resources.requests.storage
}{
        "\n"
}{
    end
}' | while read ns_pvc capacity request; do
    if [ "$capacity" != "$request" ]; then
        echo "    ⚠️  PVC $ns_pvc: allocated $capacity but requested $request"
    fi
done

# 3. 实例类型优化建议
echo "3. Instance Type Recommendations:"
kubectl get nodes -o jsonpath='{
    range .items[*]
}{
        .metadata.name
}{
        "\t"
}{
        .metadata.labels."node\.kubernetes\.io/instance-type"
}{
        "\t"
}{
        .status.capacity.cpu
}{
        "\t"
}{
        .status.capacity.memory
}{
        "\n"
}{
    end
}' | while read node instance_type cpu mem; do
    # 基于使用情况给出建议
    echo "    Node $node ($instance_type): $cpu CPU, $mem Memory"
done

echo "Optimization analysis completed"
EOF

chmod +x resource-optimization-recommender.sh
```

---