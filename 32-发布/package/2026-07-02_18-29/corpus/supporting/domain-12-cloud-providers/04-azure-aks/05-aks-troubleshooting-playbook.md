---
title: AKS 故障排查手册
description: '节点池问题、SLB 配置错误、KMS etcd 加密、Azure CNI IP 耗尽、Container Insights 日志分析'
summary: '节点池问题、SLB 配置错误、KMS etcd 加密、Azure CNI IP 耗尽、Container Insights 日志分析'
category: cloud-providers
tags:
- cloud
- k8s
- aks
- azure
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
- 平台工程师
estimated_read_time: 15min
intent_queries:
- AKS 故障排查方法是什么
- 如何排查 SLB 配置错误
- 如何解决 Azure CNI IP 耗尽
trigger_keywords:
- AKS troubleshooting
- SLB
- KMS
- etcd encryption
- IP exhaustion
- Container Insights
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


# AKS 故障排查手册

## 1. 节点池问题

### 1.1 节点 NotReady

**症状**：`kubectl get nodes` 显示节点 NotReady

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看节点状态和条件
kubectl describe node <node-name> | grep -A20 "Conditions:"

# 2. 常见原因排查
# a) 磁盘压力
kubectl describe node <node-name> | grep -A5 "DiskPressure"
df -h /var/lib/kubelet

# b) 内存压力
kubectl describe node <node-name> | grep -A5 "MemoryPressure"

# c) PID 耗尽
kubectl describe node <node-name> | grep -A5 "PIDPressure"

# 3. 检查 kubelet 日志（SSH 到节点）
journalctl -u kubelet --since "1 hour ago" | grep -E "error|fail|panic"
```
**解决方案**：

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 磁盘压力 → 清理无用镜像
crictl rmi --prune

# 内存压力 → 驱逐 Pod 到其他节点
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data

# 节点池整体异常 → 重新创建节点池
az aks nodepool scale \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name worker \
  --node-count 0

# 等待缩为 0 后再扩回来
az aks nodepool scale \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name worker \
  --node-count 5
```
### 1.2 节点池扩缩容失败

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查配额限制
az vm list-usage --location eastasia --query "[?contains(name.value, 'standardDSv5Family')]"

# 检查节点池状态
az aks nodepool show \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name worker \
  --query provisioningState

# 检查 autoscaler 日志
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=200

# 常见错误：
# - "sku not available" → 该区域/可用区无此 VM SKU
# - "quota exceeded" → 订阅或区域配额不足
# - "subnet exhausted" → 子网 IP 不足
```
### 1.3 节点升级卡住

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看升级状态
az aks nodepool show \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name worker \
  --query "{state:provisioningState, version:currentOrchestratorVersion}"

# 检查是否有 Pod 无法驱逐
kubectl get pods --all-namespaces -o wide | grep <node-name>
# PDB 可能阻止驱逐
kubectl get pdb -A

# 手动解除卡住的升级
# 1. 删除有问题的 PDB（临时）
kubectl delete pdb <pdb-name> -n <namespace>
# 2. 重试升级
az aks nodepool upgrade \
  --resource-group rg-aks-prod \
  --cluster-name aks-prod-01 \
  --name worker \
  --kubernetes-version 1.31.0
```
## 2. SLB 配置错误

### 2.1 Service External IP Pending

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 症状：kubectl get svc 显示 EXTERNAL-IP 为 <pending>

# 1. 检查 SLB 状态
az network lb show \
  --resource-group MC_rg-aks-prod_aks-prod-01_eastasia \
  --name kubernetes \
  --query provisioningState

# 2. 检查 NSG 规则
az network nsg rule list \
  --resource-group MC_rg-aks-prod_aks-prod-01_eastasia \
  --nsg-name aks-prod-01-nsg \
  --output table

# 3. 检查 Service 事件
kubectl describe svc <service-name>

# 4. 检查 cloud-controller-manager 日志
kubectl logs -n kube-system -l component=cloud-controller-manager --tail=100
```
### 2.2 SLB 健康探测失败

```yaml
# 正确的 Health Probe 配置
apiVersion: v1
kind: Service
metadata:
  name: api-service
  annotations:
    # 健康探测路径
    service.beta.kubernetes.io/azure-load-balancer-health-probe-request-path: /healthz
    # 探测间隔
    service.beta.kubernetes.io/azure-load-balancer-health-probe-interval: "5"
    # 探测次数
    service.beta.kubernetes.io/azure-load-balancer-health-probe-num-of-probe: "2"
spec:
  type: LoadBalancer
  ports:
  - port: 80
    targetPort: 8080
    protocol: TCP
  selector:
    app: api
```

### 2.3 SLB SNAT 端口耗尽

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 症状：外部访问间歇性超时

# 1. 检查 SNAT 端口使用
az monitor metrics list \
  --resource $SLB_ID \
  --metric SnatConnectionCount \
  --aggregation Maximum \
  --interval PT5M

# 2. 解决方案
# a) 使用 NAT Gateway 替代 SLB SNAT
# b) 增加 SLB 前缀（更多公网 IP）
# c) 减少出站长连接（设置 keep-alive timeout）
```
### 2.4 Internal LB 无法访问

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Internal LB 配置
az network lb show \
  --resource-group MC_rg-aks-prod_aks-prod-01_eastasia \
  --name kubernetes-internal \
  --query frontendIpConfigurations

# 确认子网路由可达
az network route-table route list \
  --resource-group MC_rg-aks-prod_aks-prod-01_eastasia \
  --route-table-name aks-prod-01-routetable

# 常见问题：
# - Internal LB 的子网未被 VNet 对等连接覆盖
# - UDR 路由缺失或冲突
# - NSG 规则阻止了源地址
```
## 3. KMS etcd 加密问题

### 3.1 启用 KMS 加密

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建 Key Vault 和 Key
az keyvault create \
  --name kv-aks-prod \
  --resource-group rg-aks-prod \
  --location eastasia \
  --enable-rbac-authorization

az keyvault key create \
  --vault-name kv-aks-prod \
  --name etcd-encryption-key \
  --protection software \
  --kty RSA \
  --size 2048

# 为 AKS Managed Identity 赋权
KV_ID=$(az keyvault show --name kv-aks-prod --query id -o tsv)
az role assignment create \
  --assignee $AKS_MI_PRINCIPAL_ID \
  --role "Key Vault Crypto User" \
  --scope $KV_ID

# 启用 KMS
az aks update \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --enable-azure-keyvault-kms \
  --azure-keyvault-kms-key-vault-network-access "Private" \
  --azure-keyvault-kms-key-id $KEY_ID
```
### 3.2 KMS 加密故障排查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 检查 KMS Provider 状态
kubectl get pods -n kube-system -l component=kms

# 查看 KMS 日志
kubectl logs -n kube-system -l component=kms --tail=100

# 测试 Secret 创建/读取
kubectl create secret generic test-secret --from-literal=key=value
kubectl get secret test-secret -o jsonpath='{.data.key}' | base64 -d

# 常见错误：
# "failed to encrypt" → Key Vault 权限不足
# "connection refused" → Private Endpoint DNS 未解析
# "key not found" → Key 已被禁用或删除

# 验证 etcd 数据已加密
# 注意：需要 etcdctl 访问权限（AKS 中不直接暴露）
# 可通过 Azure Portal 查看 etcd 备份
```
## 4. Azure CNI IP 耗尽

### 4.1 诊断 IP 使用情况

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查子网 IP 使用
az network vnet subnet show \
  --resource-group rg-aks-net \
  --vnet-name vnet-aks-prod \
  --name aks-pod-subnet \
  --query "ipConfigurations[].{IP:id}"

# 2. 检查节点已分配 Pod 数量
kubectl get nodes -o custom-columns=NAME:.metadata.name,PODS:.status.capacity.pods

# 3. 检查各节点 IP 使用
for node in $(kubectl get nodes -o name); do
  echo "=== $node ==="
  kubectl describe $node | grep -A5 "Allocated resources"
  echo "Pod count: $(kubectl get pods --all-namespaces --field-selector spec.nodeName=$node --no-headers | wc -l)"
done

# 4. 检查 IP 分配详情
# SSH 到节点查看 Azure CNI 状态
cat /etc/cni/net.d/10-azure.conflist
```
### 4.2 解决方案

```
# 🟢 低风险：只读/信息收集，通常无副作用
IP 耗尽解决路径：

短期：
  ├── 清理 Evicted/Succeeded Pod（释放 IP）
  │   kubectl delete pods --field-selector status.phase=Succeeded -A
  │   kubectl delete pods --field-selector status.phase=Failed -A
  │
  ├── 减少 max-pods（如果当前值过高）
  │   → 重建节点池使用更小的 max-pods
  │
  └── 扩展子网（需要重建集群或添加新节点池到新子网）

长期：
  ├── 迁移到 Azure CNI Overlay 模式
  │   → Pod 使用独立 CIDR，不消耗子网 IP
  │
  ├── 使用更大的子网
  │   → /16 子网可提供 65,534 个 IP
  │
  └── 多子网策略
      → 不同节点池使用不同子网
```
### 4.3 Overlay 迁移

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 创建新集群使用 Overlay 模式（无法就地迁移）
az aks create \
  --resource-group rg-aks-overlay \
  --name aks-overlay-01 \
  --network-plugin azure \
  --network-plugin-mode overlay \
  --pod-cidr 100.64.0.0/10 \
  --vnet-subnet-id $SUBNET_ID

# 使用 ArgoCD 或 Flux 迁移工作负载
# 或使用 Blue-Green 集群切换
```
## 5. Container Insights 日志分析

### 5.1 启用 Container Insights

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 启用监控
az aks enable-addons \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --addons monitoring \
  --workspace-resource-id $LOG_ANALYTICS_ID

# 或使用 DCR（Data Collection Rule）精细化控制
az monitor data-collection rule create \
  --resource-group rg-aks-prod \
  --name dcr-aks-prod \
  --location eastasia \
  --data-flows '[{
    "streams": ["Microsoft-Syslog", "Microsoft-KubeEvents", "Microsoft-KubePodInventory"],
    "destinations": ["la-prod"]
  }]' \
  --destinations '[{
    "logAnalytics": [{"workspaceResourceId": "'$LOG_ANALYTICS_ID'", "name": "la-prod"}]
  }]'
```
### 5.2 常用 KQL 查询

```kql
// Pod 重启次数 Top 10
KubePodInventory
| where TimeGenerated > ago(24h)
| summarize RestartCount = max(RestartCount) by PodName, Namespace
| top 10 by RestartCount desc

// 节点资源使用率
InsightsMetrics
| where TimeGenerated > ago(1h)
| where Namespace == "node.14m.io.15m.pod"
| extend NodeName = tostring(parse_json(Tags).hostName)
| summarize avg(Val) by NodeName, Name
| render timechart

// OOMKilled 事件
KubeEvents
| where TimeGenerated > ago(24h)
| where Message contains "OOMKilling"
| project TimeGenerated, Namespace, PodName, Message

// CrashLoopBackOff 容器
ContainerInventory
| where TimeGenerated > ago(1h)
| where ContainerState == "Waiting"
| where ExitCode != 0
| project TimeGenerated, Namespace, ContainerName, Image, ExitCode

// DNS 解析延迟
InsightsMetrics
| where Name == "dns_request_duration_seconds"
| summarize p99 = percentile(Val, 99) by bin(TimeGenerated, 5m)
| render timechart

// 网络丢包
InsightsMetrics
| where Name == "transmit_packets_dropped_total"
| summarize total_dropped = sum(Val) by bin(TimeGenerated, 5m)
| render timechart
```

### 5.3 自定义告警

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Pod 重启过于频繁告警
az monitor scheduled-query create \
  --resource-group rg-aks-prod \
  --name "PodRestartAlert" \
  --scopes $LOG_ANALYTICS_ID \
  --condition "query='KubePodInventory | where RestartCount > 5 | summarize count() by PodName' > 0" \
  --window-size 15m \
  --evaluation-frequency 5m \
  --severity 2 \
  --action-group ag-sre-oncall

# 节点内存使用率 > 90% 告警
az monitor metrics alert create \
  --name "NodeMemoryHigh" \
  --resource-group rg-aks-prod \
  --scopes $AKS_ID \
  --condition "avg node_memory_working_set_bytes / node_memory_capacity_bytes > 0.9" \
  --window-size 10m \
  --severity 1
```
## 6. 常见故障速查表

| 故障现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|---------|
| Pod Pending | 资源不足 | `kubectl describe pod` | 扩容节点池 |
| Pod ImagePullBackOff | 镜像拉取失败 | `kubectl describe pod` | 检查 ACR 权限 |
| Service 无外部 IP | SLB 异常 | `kubectl describe svc` | 检查 SLB 配置 |
| DNS 解析失败 | CoreDNS 异常 | `kubectl logs -n kube-system -l k8s-app=kube-dns` | 重启 CoreDNS |
| 节点 NotReady | kubelet 异常 | SSH → `journalctl -u kubelet` | 重启节点 |
| 升级卡住 | PDB 阻止驱逐 | `kubectl get pdb` | 调整 PDB |

## 7. 诊断工具

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# AKS 连接诊断
az aks command invoke \
  --resource-group rg-aks-prod \
  --name aks-prod-01 \
  --command "kubectl get pods -A"

# 节点 Shell（无需 SSH）
kubectl debug node/<node-name> -it --image=busybox

# 网络诊断 Pod
kubectl run netshoot --rm -it --image=nicolaka/netshoot -- bash
# 在 Pod 内：
# nslookup kubernetes.default
# curl -v https://kubernetes.default/healthz
# traceroute <service-ip>
```
## Related

- [[02-aks-networking-azure-cni|AKS 网络与 Azure CNI]]
- [[04-aks-identity-workload-identity|AKS 身份认证与 Workload Identity]]

## See Also

- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-12-cloud-providers/04-azure-aks/01-azure-aks-production-runbook|AKS 生产环境运行手册]]
- Container Insights KQL 参考


<!-- risk-assessed -->
