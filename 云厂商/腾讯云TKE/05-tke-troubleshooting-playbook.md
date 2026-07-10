---
title: TKE 故障排查手册
description: 'CLB 后端异常、镜像仓库 ACR 拉取失败、节点池弹性伸缩问题、TKE 日志采集配置全面排查指南'
summary: 'CLB 后端异常、镜像仓库 ACR 拉取失败、节点池弹性伸缩问题、TKE 日志采集配置全面排查指南'
category: cloud-providers
tags:
- cloud
- k8s
- tke
- tencent
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
- TKE 故障排查方法是什么
- 如何排查 CLB 后端异常
- 如何解决 ACR 镜像拉取失败
trigger_keywords:
- CLB
- ACR
- 弹性伸缩
- 日志采集
- TKE troubleshooting
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


# TKE 故障排查手册

## 1. CLB 后端异常

### 1.1 Service External IP Pending

**症状**：`kubectl get svc` 显示 EXTERNAL-IP 为 `<pending>`

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 Service 事件
kubectl describe svc <service-name>

# 2. 检查 CLB 配额
tccli clb DescribeLoadBalancers --LoadBalancerType 1

# 3. 检查子网 IP 是否充足
tccli vpc DescribeSubnets --SubnetIds '["subnet-xxxxxxxx"]'

# 4. 检查 TKE 组件日志
kubectl logs -n kube-system -l k8s-app=service-controller --tail=100

# 常见原因：
# - CLB 配额不足 → 申请配额提升
# - 子网 IP 耗尽 → 扩展子网
# - 安全组未放行 → 检查安全组规则
```
### 1.2 CLB 健康检查失败

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 验证后端 Pod 是否正常
kubectl get pods -l app=api -o wide
kubectl describe pod <pod-name>

# 2. 检查 Pod 端口监听
kubectl exec -it <pod-name> -- netstat -tlnp | grep 8080

# 3. 检查健康检查路径
kubectl exec -it <pod-name> -- curl -v http://localhost:8080/healthz

# 4. 检查 CLB 监听器配置
tccli clb DescribeListeners \
  --LoadBalancerId "lb-xxxxxxxx" \
  --ListenerIds '["lbl-xxxxxxxx"]'

# 5. 检查后端绑定
tccli clb DescribeTargets \
  --LoadBalancerId "lb-xxxxxxxx" \
  --ListenerId "lbl-xxxxxxxx"

# 常见原因：
# - Pod 未就绪（Readiness Probe 失败）
# - 健康检查路径返回非 200
# - 安全组阻止 CLB 到 Pod 的流量
# - 目标端口与 Pod 端口不一致
```
### 1.3 CLB 流量异常

```bash
# 1. 检查 CLB 监控指标
tccli clb DescribeLoadBalancerTraffic \
  --LoadBalancerId "lb-xxxxxxxx" \
  --StartTime "2026-07-01T00:00:00+08:00" \
  --EndTime "2026-07-02T00:00:00+08:00"

# 2. 检查后端连接数
tccli clb DescribeTargets \
  --LoadBalancerId "lb-xxxxxxxx" \
  --ListenerId "lbl-xxxxxxxx"

# 3. 抓包分析（在节点上）
tcpdump -i eth0 port 80 -nn -c 100

# 4. 检查 iptables 规则
iptables -t nat -L KUBE-SERVICES -n -v | grep <service-ip>

# 常见原因：
# - 后端 Pod 连接数满
# - SNAT 端口耗尽
# - 带宽打满
# - 后端 Pod 响应慢
```

### 1.4 共享 CLB 冲突

```yaml
# 多个 Service 共享同一个 CLB 时，监听器配置可能冲突
# 检查是否有端口冲突
kubectl get svc -A -o json | jq '.items[] | select(.spec.type=="LoadBalancer") | {name: .metadata.name, port: .spec.ports[0].port}'

# 解决方案：
# 1. 使用不同端口
# 2. 使用不同 CLB
# 3. 使用 ALB Ingress 统一管理
```

## 2. 镜像仓库 ACR 拉取失败

### 2.1 认证失败

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 症状：ImagePullBackOff

# 1. 检查 Secret 配置
kubectl get secret <image-pull-secret> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d

# 2. 测试手动拉取
kubectl run test-pull --image=ccr.ccs.tencentyun.com/namespace/image:tag --dry-run=client -o yaml

# 3. 检查 ACR 凭证
tccli tcr DescribeNamespaces --RegistryId "tcr-xxxxxxxx"

# 4. 重新创建 Secret
kubectl create secret docker-registry acr-secret \
  --docker-server=ccr.ccs.tencentyun.com \
  --docker-username=<tencent-cloud-id> \
  --docker-password=<tencent-cloud-key>

# 5. 确认 Pod 使用了正确的 Secret
kubectl get pod <pod-name> -o jsonpath='{.spec.imagePullSecrets}'
```
### 2.2 网络不通

```bash
# 1. 检查节点到 ACR 的网络连通性
# SSH 到节点
curl -v https://ccr.ccs.tencentyun.com/v2/

# 2. 检查 VPC DNS 解析
nslookup ccr.ccs.tencentyun.com

# 3. 检查安全组规则
tccli vpc DescribeSecurityGroups --SecurityGroupIds '["sg-xxxxxxxx"]'

# 4. 检查 NAT 网关（如果没有公网访问）
tccli nat DescribeNatGateways --VpcId "vpc-xxxxxxxx"

# 5. 使用私有 ACR（如果需要）
# 在 TCR 中创建私有实例，通过 VPC 内网访问
tccli tcr CreateInstance --RegistryName "tcr-prod" --RegistryType "basic" --ChargeType "month"
```

### 2.3 ACR 配额限制

```bash
# 检查镜像仓库配额
tccli tcr DescribeImages --RegistryId "tcr-xxxxxxxx" --NamespaceName "prod" --RepositoryName "app"

# 检查拉取频率限制
# ACR 可能有 API 调用频率限制

# 解决方案：
# 1. 使用镜像预热（提前拉取到节点）
# 2. 配置本地镜像缓存
# 3. 升级 ACR 套餐
```

### 2.4 大镜像拉取超时

```yaml
# 对于大镜像（>1GB），增加超时配置
apiVersion: v1
kind: Pod
metadata:
  name: large-image-app
spec:
  # 增加 initContainer 预拉取
  initContainers:
  - name: pre-pull
    image: busybox
    command: ["sh", "-c", "echo 'Image pre-pull complete'"]
  containers:
  - name: app
    image: very-large-image:latest
    imagePullPolicy: Always

# 或使用 ChromFS 加速
# annotations:
#   chromfs.cloud.tencent.com/enabled: "true"
```

## 3. 节点池弹性伸缩问题

### 3.1 无法扩容

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 Cluster Autoscaler 日志
kubectl logs -n kube-system -l app=cluster-autoscaler --tail=200

# 2. 检查节点池状态
tccli tke DescribeClusterNodePools --ClusterId "cls-xxxxxxxx"

# 3. 检查 VM 配额
tccli cvm DescribeAccountQuota

# 4. 检查子网 IP
tccli vpc DescribeSubnets --SubnetIds '["subnet-xxxxxxxx"]'

# 常见错误：
# - "insufficient quota" → 配额不足
# - "no available zone" → 可用区资源不足
# - "subnet exhausted" → 子网 IP 耗尽
# - "launch template error" → 实例配置错误
```
### 3.2 扩容速度慢

```bash
# 1. 检查节点启动时间
# 从 CVM 创建到节点 Ready 的时间

# 2. 优化节点池配置
# - 使用更小的基础镜像
# - 预装常用容器镜像（自定义镜像）
# - 减少节点初始化脚本

# 3. 调整 Autoscaler 参数
# 在 TKE 控制台调整：
# - scale-down-delay-after-add: 10m → 5m
# - scale-down-unneeded-time: 10m → 5m
# - scan-interval: 10s → 5s
```

### 3.3 缩容失败

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
# 1. 检查 PDB（PodDisruptionBudget）
kubectl get pdb -A

# 2. 检查不可驱逐的 Pod
kubectl get pods -A -o wide | grep <node-name>
# DaemonSet Pod、本地存储的 Pod 不可驱逐

# 3. 手动驱逐节点
kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force

# 4. 检查节点是否被标记为不可缩容
kubectl describe node <node-name> | grep "cluster-autoscaler"
```
### 3.4 节点池配置不一致

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查节点池配置
tccli tke DescribeNodePool --ClusterId "cls-xxxxxxxx" --NodePoolId "np-xxxxxxxx"

# 检查节点实际配置
kubectl get nodes -o custom-columns=NAME:.metadata.name,CPU:.status.capacity.cpu,MEM:.status.capacity.memory

# 重新同步节点池
# 在 TKE 控制台执行"更新节点池"操作
```
## 4. TKE 日志采集配置

### 4.1 启用日志采集

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 TKE 日志采集组件
# 在 TKE 控制台启用，或通过 Helm 安装

# 检查日志采集组件状态
kubectl get pods -n kube-system -l k8s-app=tke-log-agent

# 检查采集配置
kubectl get clslogconfig -A
```
### 4.2 容器标准输出采集

```yaml
# TKE 内置采集配置
apiVersion: cls.cloud.tencent.com/v1
kind: LogConfig
metadata:
  name: app-stdout
  namespace: production
spec:
  # 采集类型：标准输出
  type: stdout
  # 日志集和主题
  logsetId: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  topicId: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  # 采集配置
  config:
    # 选择器：匹配特定 Pod
    podSelector:
      matchLabels:
        app: api
    # 提取规则
    extractRule:
      # 时间格式
      timeKey: "timestamp"
      timeFormat: "%Y-%m-%dT%H:%M:%S.%fZ"
      # 日志级别
      logRegex: "(?P<level>INFO|WARN|ERROR) (?P<message>.*)"
      # 过滤
      filterRegex: "health"
      filterKeyRegex:
        message: "^health"
```

### 4.3 文件日志采集

```yaml
apiVersion: cls.cloud.tencent.com/v1
kind: LogConfig
metadata:
  name: app-file-logs
  namespace: production
spec:
  type: "container_file"
  logsetId: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  topicId: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  config:
    # 容器内日志文件路径
    containerFile:
      - namespace: production
        container: app
        logPath: /var/log/app/*.log
        filePattern: "*.log"
    # 提取规则
    extractRule:
      timeKey: "time"
      timeFormat: "%Y-%m-%d %H:%M:%S"
      logRegex: "\\[(?P<time>[^\\]]+)\\] (?P<level>\\w+) (?P<message>.*)"
```

### 4.4 节点文件采集

```yaml
apiVersion: cls.cloud.tencent.com/v1
kind: LogConfig
metadata:
  name: node-syslog
spec:
  type: "host_file"
  logsetId: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  topicId: "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
  config:
    hostFile:
      logPath: /var/log
      filePattern: "messages"
    extractRule:
      timeKey: "time"
      timeFormat: "%b %d %H:%M:%S"
```

### 4.5 日志采集故障排查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 检查日志采集组件
kubectl get pods -n kube-system -l k8s-app=tke-log-agent
kubectl logs -n kube-system -l k8s-app=tke-log-agent --tail=100

# 2. 检查采集配置
kubectl get clslogconfig -A -o yaml

# 3. 检查日志文件是否存在
kubectl exec -it <pod-name> -- ls -la /var/log/app/

# 4. 检查 CLS 日志主题
tccli cls DescribeTopics --LogsetId "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"

# 5. 检查日志是否到达 CLS
tccli cls SearchLog \
  --TopicId "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx" \
  --From "2026-07-02 00:00:00" \
  --To "2026-07-02 23:59:59" \
  --Query "* | limit 10"

# 常见问题：
# - 采集组件未安装 → 在控制台启用
# - 文件路径不匹配 → 检查 logPath
# - 权限不足 → 检查 ServiceAccount
# - CLS 配额满 → 清理旧日志
```
## 5. 常见故障速查表

| 故障现象 | 可能原因 | 排查命令 | 解决方案 |
|---------|---------|---------|---------|
| Pod Pending | 资源不足 | `kubectl describe pod` | 扩容节点池 |
| ImagePullBackOff | 镜像拉取失败 | `kubectl describe pod` | 检查 ACR 凭证 |
| CLB 无外部 IP | CLB 创建失败 | `kubectl describe svc` | 检查配额 |
| 节点 NotReady | kubelet 异常 | `journalctl -u kubelet` | 重启节点 |
| DNS 解析失败 | CoreDNS 异常 | `kubectl logs -n kube-system -l k8s-app=kube-dns` | 重启 CoreDNS |
| 日志未采集 | 配置错误 | `kubectl get clslogconfig` | 检查采集配置 |
| 弹性伸缩失效 | 配额不足 | Autoscaler 日志 | 申请配额 |

## 6. 诊断工具

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# TKE 集群诊断
tccli tke DescribeClusterInspectionResults --ClusterId "cls-xxxxxxxx"

# 节点健康检查
kubectl get nodes -o wide
kubectl describe node <node-name>

# 网络诊断
kubectl run netshoot --rm -it --image=nicolaka/netshoot -- bash
# nslookup kubernetes.default
# curl -v http://<service-ip>:<port>

# 存储诊断
kubectl get pvc -A
kubectl describe pvc <pvc-name>

# 日志诊断
kubectl logs <pod-name> --tail=100 -p  # 上一个容器的日志
kubectl logs <pod-name> --tail=100 --previous
```
## Related

- [[04-tke-iam-cam-integration|TKE 身份认证与 CAM 集成]]
- [[02-tke-networking-vpc-cni|TKE 网络与 VPC-CNI]]

## See Also

- [[云厂商/腾讯云TKE/99-tencent-tke-production-runbook.md|TKE 生产运维 Runbook]]
- CLB 配置指南
- ACR 镜像仓库文档


<!-- risk-assessed -->
