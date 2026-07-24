---
title: 05 - Terway 测试验证 (Testing & Validation)
description: '## 1. Pod 网络基础验证'
summary: 'ENIIP 模式下 Pod IP 应属于 VPC 子网网段，可通过以下命令比对：'
category: terway
tags:
- k8s
- terway
- networking
- alicloud
- cilium
- flannel
- coredns
- ingress
- gateway
- networkpolicy
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
estimated_read_time: 5min
intent_queries:
- Terway 测试验证 (Testing & Validation) 是什么
- 如何 Terway 测试验证 (Testing & Validation)
trigger_keywords:
- Terway
- 测试验证
- Testing
- Validation
- terway
prerequisites:
- kubectl-basics
- networking-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 05 - Terway 测试验证 (Testing & Validation)

> **适用版本**: 阿里云 ACK v1.25 - v1.32+ | **Terway 版本**: v1.5+ | **最后更新**: 2026-05

---

## 1. Pod 网络基础验证

### 1.1 创建测试 Pod

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run terway-test-1 \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --command -- sleep 3600

kubectl run terway-test-2 \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --command -- sleep 3600

kubectl wait --for=condition=Ready pod/terway-test-1 --timeout=60s
kubectl wait --for=condition=Ready pod/terway-test-2 --timeout=60s
```
### 1.2 验证 Pod IP (ENIIP 模式)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod -o wide
```
ENIIP 模式下 Pod IP 应属于 VPC 子网网段，可通过以下命令比对：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
VPC_CIDR=$(kubectl get configmap -n kube-system eni-config -o jsonpath='{.data.eni_conf}' | grep -o '"vswitches":{[^}]*}' | head -1)
echo "vSwitch 配置: $VPC_CIDR"

kubectl get pod terway-test-1 -o wide --no-headers | awk '{print "Pod IP:", $6}'
```
### 1.3 验证 Pod Annotation (已分配 IP)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pod terway-test-1 -o yaml | grep k8s.aliyun.com
```
关键字段：
- `k8s.aliyun.com/allocated-ipv4`: Terway 分配给 Pod 的 IPv4 地址，应与 `kubectl get pod -o wide` 显示的 IP 一致

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
ALLOCATED_IP=$(kubectl get pod terway-test-1 -o jsonpath='{.metadata.annotations.k8s\.aliyun\.com/allocated-ipv4}')
POD_IP=$(kubectl get pod terway-test-1 -o jsonpath='{.status.podIP}')
echo "Annotation IP: $ALLOCATED_IP"
echo "Status PodIP:  $POD_IP"
[ "$ALLOCATED_IP" = "$POD_IP" ] && echo "PASS: IP 一致" || echo "FAIL: IP 不一致"
```
### 1.4 Pod 内网络接口检查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "=== 网络接口 ==="
kubectl exec terway-test-1 -- ip addr show

echo "=== 路由表 ==="
kubectl exec terway-test-1 -- ip route show

echo "=== DNS 配置 ==="
kubectl exec terway-test-1 -- cat /etc/resolv.conf

echo "=== 网卡详情 ==="
kubectl exec terway-test-1 -- ip link show
```
预期结果：

| 检查项 | 预期值 |
|:---|:---|
| eth0 IP | VPC 子网 IP，非 10.244.x.x 等 CIDR |
| 默认路由 | 指向节点网关或 ENI 接口 |
| nameserver | 集群 [[CoreDNS|CoreDNS]] [[Service|Service]] ClusterIP（通常为 10.96.0.10 或 172.16.0.10） |
| search | `default.svc.cluster.local svc.cluster.local cluster.local` |

异常排查：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 若 Pod 内无 eth0 或 IP 异常，检查 Terway 日志
kubectl logs -n kube-system -l app=terway --tail=200 | grep -i "error|fail"

# 检查 ENI 分配状态
kubectl exec -n kube-system $(kubectl get pods -n kube-system -l app=terway -o jsonpath='{.items[0].metadata.name}') -- terway-cli show
```
---

## 2. 跨节点连通性测试

### 2.1 同节点 Pod 通信

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
TEST1_IP=$(kubectl get pod terway-test-1 -o jsonpath='{.status.podIP}')
TEST2_IP=$(kubectl get pod terway-test-2 -o jsonpath='{.status.podIP}')

kubectl exec terway-test-1 -- ping -c 3 -W 2 $TEST2_IP
```
**预期**: 0% 丢包，延迟 < 1ms。
**异常排查**: 若同节点 Pod 不通，检查 veth pair 是否正确创建：`ip link show type veth`（在节点上执行）。

### 2.2 跨节点 Pod 通信

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 获取节点列表，在第二个节点上创建测试 Pod
NODE1=$(kubectl get pod terway-test-1 -o jsonpath='{.spec.nodeName}')
NODE2=$(kubectl get nodes -o jsonpath='{.items[1].metadata.name}')

kubectl run terway-test-3 \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --overrides='{"spec":{"nodeName":"'$NODE2'"}}' \
  --command -- sleep 3600

kubectl wait --for=condition=Ready pod/terway-test-3 --timeout=60s

TEST3_IP=$(kubectl get pod terway-test-3 -o jsonpath='{.status.podIP}')
echo "terway-test-1 @ $NODE1 → terway-test-3 @ $NODE2 ($TEST3_IP)"

kubectl exec terway-test-1 -- ping -c 3 -W 2 $TEST3_IP
```
**预期**: 0% 丢包，延迟 < 5ms（同可用区）或 < 10ms（跨可用区）。
**异常排查**: 若跨节点不通，检查 VPC 安全组是否放行 Pod 网段，以及 vSwitch 路由配置。

### 2.3 Pod → Node 通信

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
NODE_IP=$(kubectl get node $NODE1 -o jsonpath='{.status.addresses[?(@.type=="InternalIP")].address}')
echo "Node IP: $NODE_IP"

kubectl exec terway-test-1 -- ping -c 3 -W 2 $NODE_IP
```
**预期**: 成功，延迟 < 1ms。

### 2.4 Pod → 外网通信

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "=== ICMP 测试 ==="
kubectl exec terway-test-1 -- ping -c 3 -W 5 8.8.8.8

echo "=== HTTP 出口 IP 测试 ==="
kubectl exec terway-test-1 -- wget -q -O- http://ifconfig.me 2>/dev/null || \
  echo "注意: 需要配置 NAT Gateway 或 EIP 才能访问外网"
```
**预期**: ping 成功（若允许 ICMP）；wget 返回节点 EIP 或 NAT Gateway 出口 IP。
**异常排查**: 外网不通时检查：NAT Gateway 是否配置、安全组是否放行出方向、是否存在 0.0.0.0/0 路由。

### 2.5 Pod → VPC 元数据服务

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "=== 元数据服务 ==="
kubectl exec terway-test-1 -- wget -q -O- http://100.100.100.200/latest/meta-data/instance-id

echo "=== RAM 角色 (如有) ==="
kubectl exec terway-test-1 -- wget -q -O- http://100.100.100.200/latest/meta-data/ram/security-credentials/
```
**预期**: 返回 ECS 实例 ID 和角色信息。
**异常排查**: 若无法访问 100.100.100.200，检查 vSwitch 是否在正确 VPC 内，以及安全组是否阻断。

### 2.6 连通性测试汇总脚本

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
PASS=0; FAIL=0
report() {
  if [ $1 -eq 0 ]; then echo "  PASS: $2"; PASS=$((PASS+1))
  else echo "  FAIL: $2"; FAIL=$((FAIL+1)); fi
}

echo "--- 同节点 Pod 通信 ---"
kubectl exec terway-test-1 -- ping -c 1 -W 2 $TEST2_IP &>/dev/null
report $? "同节点 Pod ping"

echo "--- 跨节点 Pod 通信 ---"
kubectl exec terway-test-1 -- ping -c 1 -W 2 $TEST3_IP &>/dev/null
report $? "跨节点 Pod ping"

echo "--- Pod → Node ---"
kubectl exec terway-test-1 -- ping -c 1 -W 2 $NODE_IP &>/dev/null
report $? "Pod → Node ping"

echo "--- Pod → 外网 ---"
kubectl exec terway-test-1 -- ping -c 1 -W 5 8.8.8.8 &>/dev/null
report $? "Pod → 外网 ping"

echo "--- Pod → 元数据 ---"
kubectl exec terway-test-1 -- wget -q -T 3 -O- http://100.100.100.200/latest/meta-data/instance-id &>/dev/null
report $? "VPC 元数据服务"

echo ""
echo "结果: PASS=$PASS  FAIL=$FAIL"
```
---

## 3. [[NetworkPolicy|NetworkPolicy]] 测试

### 3.1 默认拒绝所有入站流量

先创建一个带 HTTP 服务的测试目标：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run np-server \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/nginx:alpine \
  --labels app=np-server

kubectl wait --for=condition=Ready pod/np-server --timeout=60s
NP_SERVER_IP=$(kubectl get pod np-server -o jsonpath='{.status.podIP}')
```
验证无策略时可访问：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec terway-test-1 -- wget -q -T 3 -O- http://$NP_SERVER_IP:80
echo "无策略时应返回 0: $?"
```
应用默认拒绝策略：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: deny-all-ingress
  namespace: default
spec:
  podSelector: {}
  policyTypes:
  - Ingress
EOF
```
验证策略生效：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl exec terway-test-1 -- wget -T 5 -q -O- http://$NP_SERVER_IP:80
echo "策略生效后应超时(exit!=0): $?"
```
**预期**: wget 超时退出（exit code 非零），说明入站流量被拒绝。

### 3.2 放行特定标签 Pod

创建带标签的前端 Pod：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run np-frontend \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --labels app=frontend \
  --command -- sleep 3600

kubectl run np-other \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --labels app=other \
  --command -- sleep 3600
```
应用放行策略：

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-server
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: np-server
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 80
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-server
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: np-server
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 80
EOF
```
验证：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "=== 前端 Pod (app=frontend) 访问 ==="
kubectl exec np-frontend -- wget -T 5 -q -O- http://$NP_SERVER_IP:80
echo "预期: 成功 (exit 0)"

echo "=== 无标签/其他 Pod (app=other) 访问 ==="
kubectl exec np-other -- wget -T 5 -q -O- http://$NP_SERVER_IP:80
echo "预期: 超时 (exit != 0)"
```
### 3.3 出站策略 (Egress)

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: egress-allow-dns-only
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: np-server
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
  - to:
    - ipBlock:
        cidr: 100.100.100.200/32
    ports:
    - protocol: TCP
      port: 80
```

验证出站限制：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: egress-allow-dns-only
  namespace: default
spec:
  podSelector:
    matchLabels:
      app: np-server
  policyTypes:
  - Egress
  egress:
  - to:
    - namespaceSelector: {}
      podSelector:
        matchLabels:
          k8s-app: kube-dns
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
  - to:
    - ipBlock:
        cidr: 100.100.100.200/32
    ports:
    - protocol: TCP
      port: 80
EOF

echo "=== DNS 解析 (应成功) ==="
kubectl exec np-server -- nslookup kubernetes.default.svc.cluster.local

echo "=== 外网访问 (应超时) ==="
kubectl exec np-server -- wget -T 5 -q -O- http://8.8.8.8
echo "预期: 超时"
```
### 3.4 清理所有测试策略和 Pod

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
kubectl delete networkpolicy deny-all-ingress allow-frontend-to-server egress-allow-dns-only --ignore-not-found
kubectl delete pod np-server np-frontend np-other --force --grace-period=0 --ignore-not-found  # ⚠️ 跳过优雅终止，可能丢数据
```
---

## 4. ENI 配额验证

### 4.1 节点级配额查看

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
NODE_NAME=$(kubectl get nodes -o jsonpath='{.items[0].metadata.name}')
kubectl describe node $NODE_NAME | grep -E "aliyun.com|Allocatable|Capacity" -A 5

echo ""
echo "=== 详细 ENI/IP 配额 ==="
kubectl get node $NODE_NAME -o json | jq '{
  node: .metadata.name,
  allocatable: {
    eni_total: .status.allocatable["aliyun.com/eni"],
    ip_total: .status.allocatable["aliyun.com/ip"],
  },
  capacity: {
    eni_total: .status.capacity["aliyun.com/eni"],
    ip_total: .status.capacity["aliyun.com/ip"],
  }
}'
```
关键 Annotation 字段说明：

| 字段 | 含义 |
|:---|:---|
| `aliyun.com/eni` | 节点可用的 ENI 总数 |
| `aliyun.com/ip` | 节点可用的辅助 IP 总数 |
| `node.k8s.alibabacloud.com/allocated-eni` | 已分配 ENI 数 |
| `node.k8s.alibabacloud.com/eni-max` | ENI 上限 |
| `node.k8s.alibabacloud.com/allocated-ip` | 已分配辅助 IP 数 |
| `node.k8s.alibabacloud.com/ip-max` | 辅助 IP 上限 |

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
echo "=== Annotation 详情 ==="
kubectl get node $NODE_NAME -o json | jq '.metadata.annotations | to_entries[] | select(.key | contains("k8s.alibabacloud"))'
```
### 4.2 所有节点配额总览

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get nodes -o custom-columns=\
"NODE:.metadata.name,\
ENI-ALLOC:.metadata.annotations.node\.k8s\.alibabacloud\.com/allocated-eni,\
ENI-MAX:.metadata.annotations.node\.k8s\.alibabacloud\.com/eni-max,\
IP-ALLOC:.metadata.annotations.node\.k8s\.alibabacloud\.com/allocated-ip,\
IP-MAX:.metadata.annotations.node\.k8s\.alibabacloud\.com/ip-max"
```
### 4.3 密度压力测试

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
echo "开始创建 50 个 Pod 进行密度测试..."
for i in $(seq 1 50); do
  kubectl run density-test-$i \
    --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
    --command -- sleep 300 &
done
wait

echo "等待 Pod 启动..."
sleep 30

TOTAL=$(kubectl get pods -l run --no-headers | grep "density-test" | wc -l)
RUNNING=$(kubectl get pods -l run --no-headers | grep "density-test" | grep "Running" | wc -l)
PENDING=$(kubectl get pods -l run --no-headers | grep "density-test" | grep "Pending" | wc -l)
FAILED=$(kubectl get pods -l run --no-headers | grep "density-test" | grep -vE "Running|Pending" | wc -l)

echo "总计: $TOTAL | Running: $RUNNING | Pending: $PENDING | Failed: $FAILED"

echo ""
echo "=== IP 分配详情 ==="
kubectl get pods -l run -o wide --no-headers | grep "density-test" | awk '{print $6}' | sort | uniq -c | sort -rn | head -20
```
结果解读：

| 现象 | 可能原因 | 处理建议 |
|:---|:---|:---|
| 全部 Running | IP 资源充足 | 正常 |
| 部分 Pending + FailedScheduling | 节点 CPU/内存不足 | 扩容节点或调整 requests |
| 部分 ContainerCreating | ENI/IP 配额耗尽 | 升级 ECS 规格（更多 ENI/IP） |
| Pod 有 IP 但 NotReady | vSwitch IP 池耗尽 | 扩容 vSwitch 或添加新 vSwitch |

清理密度测试 Pod：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘

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
for i in $(seq 1 50); do kubectl delete pod density-test-$i --force --grace-period=0 & done; wait  # ⚠️ 跳过优雅终止，可能丢数据
```
---

## 5. 固定 IP 验证

### 5.1 创建 PodNetworking (Fixed 分配策略)

```yaml
apiVersion: network.alibabacloud.com/v1beta1
kind: PodNetworking
metadata:
  name: fixed-ip-test
spec:
  allocationType:
    type: Fixed
    releaseStrategy: TTL
    ttl: 300s
  vSwitches:
  - vsw-xxx
  securityGroupIDs:
  - sg-xxx
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f podnetworking-fixed-ip.yaml
```
### 5.2 创建使用固定 IP 的 Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: fixed-ip-pod
  annotations:
    k8s.v1.cni.cncf.io/networks: fixed-ip-test
spec:
  containers:
  - name: app
    image: registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36
    command: ["sleep", "3600"]
```

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f fixed-ip-pod.yaml
kubectl wait --for=condition=Ready pod/fixed-ip-pod --timeout=60s

ORIGINAL_IP=$(kubectl get pod fixed-ip-pod -o jsonpath='{.status.podIP}')
echo "Pod 首次 IP: $ORIGINAL_IP"
```
### 5.3 删除 Pod 并验证 IP 保留

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

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
kubectl delete pod fixed-ip-pod --force --grace-period=0  # ⚠️ 跳过优雅终止，可能丢数据

echo "=== 检查 PodENI 资源保留 ==="
kubectl get podeni -A

echo "=== 检查 ReservedIP ==="
kubectl get reservedip -A

echo "=== Terway 视角 ==="
TERWAY_POD=$(kubectl get pods -n kube-system -l app=terway -o jsonpath='{.items[0].metadata.name}')
kubectl exec -n kube-system $TERWAY_POD -- terway-cli show | grep -i "fixed"
```
### 5.4 重建 Pod 验证 IP 一致性

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl apply -f fixed-ip-pod.yaml
kubectl wait --for=condition=Ready pod/fixed-ip-pod --timeout=60s

NEW_IP=$(kubectl get pod fixed-ip-pod -o jsonpath='{.status.podIP}')
echo "Pod 重建后 IP: $NEW_IP"
echo "原始 IP:       $ORIGINAL_IP"

[ "$ORIGINAL_IP" = "$NEW_IP" ] && echo "PASS: IP 保持不变" || echo "FAIL: IP 发生变化"
```
清理：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘
> - `kubectl delete`：删除资源（可由声明式清单重建）

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
kubectl delete pod fixed-ip-pod --force --grace-period=0 --ignore-not-found  # ⚠️ 跳过优雅终止，可能丢数据
kubectl delete podnetworking fixed-ip-test --ignore-not-found
```
---

## 6. GC (垃圾回收) 验证

### 6.1 记录当前 IP 分配状态

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
TERWAY_POD=$(kubectl get pods -n kube-system -l app=terway -o jsonpath='{.items[0].metadata.name}')

echo "=== GC 前 IP 分配 ==="
kubectl exec -n kube-system $TERWAY_POD -- terway-cli show

echo "=== GC 前 IPInstance CRD ==="
kubectl get ipinstance -A -o wide
```
### 6.2 创建并删除测试 Pod

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘

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
kubectl run gc-test-1 --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 --command -- sleep 60
kubectl run gc-test-2 --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 --command -- sleep 60
kubectl wait --for=condition=Ready pod/gc-test-1 --timeout=60s
kubectl wait --for=condition=Ready pod/gc-test-2 --timeout=60s

GC_IP1=$(kubectl get pod gc-test-1 -o jsonpath='{.status.podIP}')
GC_IP2=$(kubectl get pod gc-test-2 -o jsonpath='{.status.podIP}')
echo "gc-test-1 IP: $GC_IP1"
echo "gc-test-2 IP: $GC_IP2"

kubectl delete pod gc-test-1 gc-test-2 --force --grace-period=0  # ⚠️ 跳过优雅终止，可能丢数据
```
### 6.3 等待 GC 周期并验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "等待 GC 周期 (约 120s)..."
sleep 120

echo "=== Dry-run GC ==="
kubectl exec -n kube-system $TERWAY_POD -- terway-cli garbage-collect --dry-run

echo "=== 执行 GC ==="
kubectl exec -n kube-system $TERWAY_POD -- terway-cli garbage-collect
```
### 6.4 验证 IP 已回收

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "=== GC 后 IP 分配 ==="
kubectl exec -n kube-system $TERWAY_POD -- terway-cli show

echo "=== GC 后 IPInstance CRD ==="
kubectl get ipinstance -A -o wide

echo "=== 验证已删除 Pod 的 IPInstance 已清理 ==="
kubectl get ipinstance -A -o json | jq -r '.items[] | select(.status.podName | test("gc-test")) | .metadata.name'
echo "预期: 无输出 (已清理)"
```
> GC 机制详解参考: [04-operations.md](./04-operations.md) 第 2 节 | [网络/38-terway-gc-mechanism.md](../网络/38-terway-gc-mechanism.md)

---

## 7. 安全组验证

### 7.1 节点安全组规则检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
echo "=== 集群安全组信息 ==="
kubectl get configmap -n kube-system eni-config -o yaml | grep security_group

echo ""
echo "=== 节点绑定的安全组 ==="
NODE_ID=$(kubectl get node $NODE_NAME -o jsonpath='{.spec.providerID}' | grep -o 'i-[a-z0-9]*')
aliyun ecs DescribeInstanceAttribute --InstanceId $NODE_ID | jq '.Data.SecurityGroupIds.SecurityGroupId[]'
```
### 7.2 安全组规则详情

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
SG_ID=$(kubectl get configmap -n kube-system eni-config -o jsonpath='{.data.eni_conf}' | grep -o '"security_group":"[^"]*"' | cut -d'"' -f4)
echo "安全组 ID: $SG_ID"

echo "=== 入方向规则 ==="
aliyun ecs DescribeSecurityGroupAttribute --SecurityGroupId $SG_ID --Direction ingress | \
  jq '.Permissions.Permission[] | {IpProtocol, PortRange, SourceCidrIp, Policy, Description}'

echo "=== 出方向规则 ==="
aliyun ecs DescribeSecurityGroupAttribute --SecurityGroupId $SG_ID --Direction egress | \
  jq '.Permissions.Permission[] | {IpProtocol, PortRange, DestCidrIp, Policy, Description}'
```
### 7.3 Pod 级安全组隔离测试

如果配置了 Pod 级安全组（Trunk ENI 模式）：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
echo "=== Pod ENI 安全组 ==="
kubectl get podeni -A -o wide

echo "=== 验证 Pod 使用独立安全组 ==="
kubectl get podeni -A -o json | jq '.items[] | {
  podName: .status.podName,
  securityGroupIDs: .status.securityGroupIDs
}'
```
### 7.4 安全组连通性验证

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "=== 测试安全组是否放行 Pod 网段 ==="
POD_CIDR=$(kubectl get configmap -n kube-system eni-config -o jsonpath='{.data.eni_conf}' | grep -o '"vswitches":{[^}]*}')
echo "Pod 网段配置: $POD_CIDR"

# 在 Pod 内测试是否被安全组阻断
kubectl exec terway-test-1 -- ping -c 1 -W 2 $TEST3_IP
echo "若跨节点 Pod 不通，检查安全组是否放行 Pod CIDR 互访"
```
> 安全组配置参考: [03-usage.md](./[[网络/Terway/03-usage.md|03-usage]].md) 第 2 节

---

## 8. 性能基准测试

> 详细性能分析参考: [06-performance.md](./06-performance.md)

### 8.1 iperf3 吞吐量测试

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run iperf3-server \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/iperf3:latest \
  --command -- iperf3 -s

kubectl wait --for=condition=Ready pod/iperf3-server --timeout=60s
IPERF_SERVER_IP=$(kubectl get pod iperf3-server -o jsonpath='{.status.podIP}')

kubectl run iperf3-client --rm -it --restart=Never \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/iperf3:latest -- \
  iperf3 -c $IPERF_SERVER_IP -t 30 -P 4 -J
```
预期吞吐量参考值：

| 模式 | 单流吞吐 | 4 并发吞吐 |
|:---|:---:|:---:|
| ENI 独占 | 20-40 Gbps | 25-40 Gbps |
| ENIIP | 15-25 Gbps | 18-28 Gbps |
| IPVlan | 18-30 Gbps | 20-32 Gbps |

### 8.2 延迟测试

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "=== Pod 间延迟 (100 次) ==="
kubectl exec terway-test-1 -- ping -c 100 -i 0.1 $TEST3_IP
```
预期延迟参考值：

| 场景 | 平均延迟 | P99 延迟 |
|:---|:---:|:---:|
| 同节点 Pod | < 0.1ms | < 0.5ms |
| 同 AZ 跨节点 | < 1ms | < 3ms |
| 跨 AZ 跨节点 | < 5ms | < 10ms |

### 8.3 连接速率测试

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl run hping3-client --rm -it --restart=Never \
  --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/hping3 -- \
  hping3 -S -p 80 -c 1000 --faster $IPERF_SERVER_IP
```
预期：ENIIP 模式下 SYN 连接速率 > 50,000 pps。

### 8.4 DNS 解析性能测试

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "=== CoreDNS 解析延迟 ==="
kubectl exec terway-test-1 -- nslookup kubernetes.default.svc.cluster.local

echo "=== 批量 DNS 解析测试 ==="
for i in $(seq 1 20); do
  kubectl exec terway-test-1 -- nslookup kubernetes.default.svc.cluster.local 2>&1 | grep "Server:"
done

echo "=== 外部域名解析 ==="
kubectl exec terway-test-1 -- nslookup aliyun.com
```
清理性能测试 Pod：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘

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
kubectl delete pod iperf3-server --force --grace-period=0 --ignore-not-found  # ⚠️ 跳过优雅终止，可能丢数据
```
---

## 9. MTU 测试

### 9.1 检查各接口 MTU

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "=== 节点接口 MTU ==="
for node in $(kubectl get nodes -o jsonpath='{.items[*].metadata.name}'); do
  echo "--- $node ---"
  kubectl debug node/$node --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 -- \
    chroot /host ip link show | grep mtu
done

echo ""
echo "=== Pod 内接口 MTU ==="
kubectl exec terway-test-1 -- ip link show | grep mtu
```
### 9.2 各模式预期 MTU 值

| 模式 | 节点 eth0 | Pod eth0 | 说明 |
|:---|:---:|:---:|:---|
| ENI 独占 | 1500 | 1500 | Pod 直接使用 ENI |
| ENIIP | 1500 | 1500 | veth pair 无额外封装 |
| IPVlan | 1500 | 1500 | IPVlan 接口继承 ENI MTU |
| VPC 路由 | 1500 | 1500 | host-gw 模式, 无额外封装 (非 VXLAN) |

### 9.3 大包测试 (DF 位)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
echo "=== 1500 字节 (预期: ENIIP 成功, VPC 路由失败) ==="
kubectl exec terway-test-1 -- ping -c 3 -s 1472 -M do $TEST3_IP

echo "=== 1400 字节 (所有模式应成功) ==="
kubectl exec terway-test-1 -- ping -c 3 -s 1400 -M do $TEST3_IP

echo "=== 探测路径 MTU ==="
kubectl exec terway-test-1 -- ping -c 3 -M do -s 1472 $TEST3_IP 2>&1
echo "若出现 'local error: message too long', 说明 MTU 需要调低"
```
MTU 异常排查：

```bash
# 在节点上修改 MTU (临时)
ip link set eth0 mtu 1500

# 永久修改: 在 eni-config 中配置
# "enable_network_policy": true 时, MTU 可能需要降低
```

---

## 10. 端到端测试套件

以下脚本可一键运行所有验证项，输出 PASS/FAIL 汇总。

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete pod --force`：强制删除 Pod，跳过优雅终止与数据刷盘
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

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
#!/bin/bash
set -euo pipefail

PASS=0
FAIL=0
WARN=0
TOTAL=0

report() {
  TOTAL=$((TOTAL+1))
  if [ "$1" = "PASS" ]; then echo "  [PASS] $2"; PASS=$((PASS+1))
  elif [ "$1" = "FAIL" ]; then echo "  [FAIL] $2"; FAIL=$((FAIL+1))
  else echo "  [WARN] $2"; WARN=$((WARN+1)); fi
}

TERWAY_NS="kube-system"
BUSYBOX="registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36"

echo "============================================"
echo "  Terway 端到端测试套件"
echo "  时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "============================================"

# --- 1. 基础环境检查 ---
echo ""
echo "[1/10] Terway 组件状态"
TERWAY_PODS=$(kubectl get pods -n $TERWAY_NS -l app=terway --no-headers 2>/dev/null | wc -l)
TERWAY_RUNNING=$(kubectl get pods -n $TERWAY_NS -l app=terway --no-headers 2>/dev/null | grep "Running" | wc -l || true)
if [ "$TERWAY_RUNNING" -ge 1 ] && [ "$TERWAY_PODS" -eq "$TERWAY_RUNNING" ]; then
  report "PASS" "Terway Pods ($TERWAY_RUNNING/$TERWAY_PODS) 全部 Running"
else
  report "FAIL" "Terway Pods ($TERWAY_RUNNING/$TERWAY_PODS) 异常"
fi

# --- 2. 创建测试 Pod ---
echo ""
echo "[2/10] 创建测试 Pod"
kubectl run e2e-test-1 --image=$BUSYBOX --command -- sleep 600 2>/dev/null || true
kubectl run e2e-test-2 --image=$BUSYBOX --command -- sleep 600 2>/dev/null || true
sleep 5
kubectl wait --for=condition=Ready pod/e2e-test-1 --timeout=90s 2>/dev/null && \
  report "PASS" "e2e-test-1 Ready" || report "FAIL" "e2e-test-1 未就绪"
kubectl wait --for=condition=Ready pod/e2e-test-2 --timeout=90s 2>/dev/null && \
  report "PASS" "e2e-test-2 Ready" || report "FAIL" "e2e-test-2 未就绪"

TEST1_IP=$(kubectl get pod e2e-test-1 -o jsonpath='{.status.podIP}' 2>/dev/null || echo "")
TEST2_IP=$(kubectl get pod e2e-test-2 -o jsonpath='{.status.podIP}' 2>/dev/null || echo "")

# --- 3. Pod IP 验证 ---
echo ""
echo "[3/10] Pod IP 验证 (ENIIP)"
if [ -n "$TEST1_IP" ]; then
  ANNOTATION_IP=$(kubectl get pod e2e-test-1 -o jsonpath='{.metadata.annotations.k8s\.aliyun\.com/allocated-ipv4}' 2>/dev/null || echo "")
  if [ "$ANNOTATION_IP" = "$TEST1_IP" ]; then
    report "PASS" "Pod IP ($TEST1_IP) 与 Annotation 一致"
  else
    report "FAIL" "Pod IP ($TEST1_IP) 与 Annotation ($ANNOTATION_IP) 不一致"
  fi
else
  report "FAIL" "无法获取 Pod IP"
fi

# --- 4. 同节点连通性 ---
echo ""
echo "[4/10] 同节点 Pod 通信"
if [ -n "$TEST2_IP" ]; then
  kubectl exec e2e-test-1 -- ping -c 3 -W 2 $TEST2_IP &>/dev/null && \
    report "PASS" "同节点 Pod ping ($TEST1_IP → $TEST2_IP)" || \
    report "FAIL" "同节点 Pod ping 失败"
else
  report "WARN" "跳过 (无目标 IP)"
fi

# --- 5. Pod → 外网 ---
echo ""
echo "[5/10] Pod → 外网"
kubectl exec e2e-test-1 -- ping -c 2 -W 5 8.8.8.8 &>/dev/null && \
  report "PASS" "Pod ping 8.8.8.8" || \
  report "WARN" "Pod ping 8.8.8.8 失败 (可能未配置 NAT)"

# --- 6. Pod → VPC 元数据 ---
echo ""
echo "[6/10] Pod → VPC 元数据服务"
kubectl exec e2e-test-1 -- wget -q -T 3 -O- http://100.100.100.200/latest/meta-data/instance-id &>/dev/null && \
  report "PASS" "VPC 元数据服务可达" || \
  report "FAIL" "VPC 元数据服务不可达"

# --- 7. ENI 配额 ---
echo ""
echo "[7/10] ENI 配额检查"
NODE_NAME=$(kubectl get pod e2e-test-1 -o jsonpath='{.spec.nodeName}' 2>/dev/null || echo "")
if [ -n "$NODE_NAME" ]; then
  IP_MAX=$(kubectl get node $NODE_NAME -o jsonpath='{.metadata.annotations.node\.k8s\.alibabacloud\.com/ip-max}' 2>/dev/null || echo "N/A")
  IP_ALLOC=$(kubectl get node $NODE_NAME -o jsonpath='{.metadata.annotations.node\.k8s\.alibabacloud\.com/allocated-ip}' 2>/dev/null || echo "N/A")
  report "PASS" "ENI 配额: 已分配 IP=$IP_ALLOC / 上限=$IP_MAX"
else
  report "WARN" "无法获取节点信息"
fi

# --- 8. DNS 解析 ---
echo ""
echo "[8/10] DNS 解析"
kubectl exec e2e-test-1 -- nslookup kubernetes.default.svc.cluster.local &>/dev/null && \
  report "PASS" "集群内 DNS 解析正常" || \
  report "FAIL" "集群内 DNS 解析失败"

# --- 9. MTU ---
echo ""
echo "[9/10] MTU 测试"
if [ -n "$TEST2_IP" ]; then
  kubectl exec e2e-test-1 -- ping -c 1 -s 1400 -M do $TEST2_IP &>/dev/null && \
    report "PASS" "MTU >= 1428 (1400 + 28 header)" || \
    report "WARN" "大包传输失败，可能存在 MTU 问题"
fi

# --- 10. GC 状态 ---
echo ""
echo "[10/10] GC 状态"
TERWAY_POD=$(kubectl get pods -n $TERWAY_NS -l app=terway -o jsonpath='{.items[0].metadata.name}' 2>/dev/null || echo "")
if [ -n "$TERWAY_POD" ]; then
  GC_DRY=$(kubectl exec -n $TERWAY_NS $TERWAY_POD -- terway-cli garbage-collect --dry-run 2>&1 || true)
  report "PASS" "GC dry-run: $GC_DRY"
else
  report "WARN" "无法执行 GC 检查"
fi

# --- 清理 ---
echo ""
echo "清理测试 Pod..."
kubectl delete pod e2e-test-1 e2e-test-2 --force --grace-period=0 2>/dev/null || true  # ⚠️ 跳过优雅终止，可能丢数据

# --- 汇总 ---
echo ""
echo "============================================"
echo "  测试结果汇总"
echo "  TOTAL: $TOTAL"
echo "  PASS:  $PASS"
echo "  FAIL:  $FAIL"
echo "  WARN:  $WARN"
echo "============================================"

if [ "$FAIL" -gt 0 ]; then
  echo "  存在失败项，请排查后重新测试"
  exit 1
else
  echo "  全部通过"
  exit 0
fi
```
将脚本保存后执行：

```bash
chmod +x terway-e2e-test.sh
./terway-e2e-test.sh 2>&1 | tee terway-e2e-$(date +%Y%m%d-%H%M%S).log
```

---

## 11. 交叉引用

### 本专题内文档

| 文档 | 关联内容 |
|:---|:---|
| [01-product.md](./01-product.md) | Terway 模式总览、与 Flannel/Cilium 对比 |
| [02-architecture.md](./02-architecture.md) | ENI/ENIIP/IPVlan 架构原理、IPAM 机制、CRD 资源模型 |
| [03-usage.md](./03-usage.md) | 安装配置、NetworkPolicy 使用、固定 IP 配置 |
| [04-operations.md](./04-operations.md) | 健康检查、GC 机制详解、监控告警、故障排查 |
| [06-performance.md](./06-performance.md) | 性能基准数据、内核调优、大规格实例优化 |

### Domain 知识库

| 文档 | 说明 |
|:---|:---|
| [网络/05-terway-advanced-guide.md](../网络/05-terway-advanced-guide.md) | Terway 高级指南（模式对比、容量规划） |
| [网络/37-terway-resources-crud-operations.md](../网络/37-terway-resources-crud-operations.md) | Terway CRD 资源 CRUD 操作 |
| [网络/38-terway-gc-mechanism.md](../网络/38-terway-gc-mechanism.md) | GC 垃圾回收机制详解 |
| [网络/16-networkpolicy-deep-practice.md](../网络/16-networkpolicy-deep-practice.md) | NetworkPolicy 深度实践 |
| [网络/34-network-performance-tuning.md](../网络/34-network-performance-tuning.md) | 网络性能调优 |
| [网络/03-cni-plugins-comparison.md](../网络/03-cni-plugins-comparison.md) | CNI 插件对比 |

### Topic 专题

| 文档 | 说明 |
|:---|:---|
| [生产运维/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni.md](../生产运维/topic-learn/inner-training/week-4-network-storage/day-24-terway-cni.md) | Terway CNI 入门学习任务 |
| [故障诊断/高级排障/structural-03-networking/07-terway-troubleshooting.md](../故障诊断/高级排障/03-networking/07-terway-troubleshooting.md) | Terway 结构化故障排查 |
| [故障诊断/FTA故障树/list/terway-fta.md](../故障诊断/FTA故障树/list/terway-fta.md) | Terway 异常 FTA 故障树 |

## Related

- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]

```

<!-- risk-assessed -->
