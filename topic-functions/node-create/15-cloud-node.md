# 云厂商节点集成

## 源码路径

`pkg/kubelet/cloudresource/`
`pkg/cloudprovider/`

---

## 云厂商节点元数据

```bash
# AWS EC2 元数据
curl http://169.254.169.254/latest/meta-data/
# AMI ID, Instance ID, Instance Type, Availability Zone

# GCP GCE 元数据
curl http://metadata.google.internal/computeMetadata/v1/
# instance/name, instance-type, zone, machine-type

# Azure 元数据
curl -H Metadata:true http://169.254.169.254/metadata/instance
```

---

## provider-id

```bash
# kubelet 自动设置 provider-id
# AWS: aws:///us-east-1a/i-0abc123
# GCP: gce://project-zone/instance-name
# Azure: azure:///subscriptions/xxx/resourceGroups/xxx/providers/Microsoft.Compute/virtualMachines/xxx

# 查看节点 provider-id
kubectl get node <node> -o jsonpath='{.spec.providerID}'
```

---

## 云厂商节点标签

```yaml
# kubelet 自动添加标签:
kubernetes.io/arch: amd64
kubernetes.io/os: linux
node.kubernetes.io/instance-type: t3.medium
topology.kubernetes.io/region: us-east-1
topology.kubernetes.io/zone: us-east-1a
# AWS 特有:
node.kubernetes.io/instance-type: t3.medium
# GCP 特有:
cloud.google.com/gke-nodepool: default-pool
# Azure 特有:
node.kubernetes.io/agentpool: default
```

---

## 节点污点

```bash
# 云厂商自动添加污点 (防止调度到不支持的节点)
# AWS:
node.kubernetes.io/aws-source: NoSchedule

# GCE:
cloud.google.com/gke-provisioner: NoSchedule

# Azure:
cloud.google.com/agentpool: NoSchedule
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 节点无法注册 | provider-id 冲突 | 使用唯一 instance-id |
| 元数据获取失败 | 网络隔离 | 配置 NAT/Proxy |
