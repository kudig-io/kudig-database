# 03 - CNI 网络插件故障排查 (CNI Network Plugin Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-02 | **参考**: [CNI Specification](https://github.com/containernetworking/cni)

---

## 1. CNI 网络故障诊断总览 (CNI Diagnosis Overview)

### 1.1 常见网络故障类型

| 故障类型 | 症状表现 | 影响范围 | 紧急程度 |
|---------|---------|---------|---------|
| **Pod网络不通** | Pod无法ping通其他Pod/Service | 应用间通信中断 | P0 - 紧急 |
| **DNS解析失败** | nslookup失败、域名无法解析 | 服务发现异常 | P1 - 高 |
| **跨节点通信失败** | 同集群不同节点Pod无法互访 | 集群网络分割 | P1 - 高 |
| **网络策略失效** | NetworkPolicy规则不生效 | 安全边界破坏 | P2 - 中 |
| **IP地址耗尽** | CNI IP池分配完、Pod卡在ContainerCreating | 新Pod无法创建 | P1 - 高 |
| **MTU问题** | 数据包分片、连接超时 | 网络性能下降 | P2 - 中 |

### 1.2 CNI 网络架构概览

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      CNI 网络故障诊断架构                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                          应用Pod层面                                   │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │   Pod A     │  │   Pod B     │  │   Pod C     │  │   Pod D     │  │  │
│  │  │ 10.244.1.2  │  │ 10.244.2.3  │  │ 10.244.1.4  │  │ 10.244.3.5  │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │  │  │  │                                     │
│        ┌─────────────────────┘  │  │  └─────────────────────┐             │
│        │                        │  │                        │             │
│        ▼                        ▼  ▼                        ▼             │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    CNI插件实现层 (Calico/Flannel/Cilium等)           │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │   Calico    │  │  Flannel    │  │   Cilium    │  │    Terway   │  │  │
│  │  │ (BGP/IPAM)  │  │ (VXLAN)     │  │ (eBPF)      │  │ (阿里云CNI) │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    网络虚拟化层                                        │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │   veth pair │  │   VXLAN     │  │   eBPF      │  │   ENI       │  │  │
│  │  │ (容器接口)   │  │ (隧道封装)   │  │ (内核旁路)   │  │ (弹性网卡)   │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    主机网络栈                                          │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  │  │
│  │  │   iptables  │  │   ipvs      │  │   routing   │  │   policy    │  │  │
│  │  │ (NAT规则)   │  │ (负载均衡)   │  │ (路由表)    │  │ (路由策略)   │  │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘  │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                              │                                              │
│                              ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐  │
│  │                    物理网络层                                          │  │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐                   │  │
│  │  │   交换机     │  │   路由器     │  │   防火墙     │                   │  │
│  │  └─────────────┘  └─────────────┘  └─────────────┘                   │  │
│  └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 2. Pod 网络不通故障排查 (Pod Network Connectivity Issues)

### 2.1 故障诊断流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      Pod 网络不通诊断流程                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   Pod A 无法访问 Pod B                                                      │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 1: 检查Pod状态和网络配置                         │                 │
│   │ kubectl get pod -o wide                              │                 │
│   │ kubectl describe pod <pod-name>                      │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── Pod未Running ──▶ 转Pod故障排查                              │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 2: 检查Pod网络接口                               │                 │
│   │ kubectl exec -it <pod> -- ip addr show               │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── 无网络接口 ──▶ CNI插件故障                                  │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 3: 检查节点CNI组件                               │                 │
│   │ kubectl get pods -n kube-system -l k8s-app=calico-node│                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── CNI Pod异常 ──▶ 检查CNI Pod日志                             │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 4: 检查主机网络配置                              │                 │
│   │ ip route show                                        │                 │
│   │ iptables -t nat -L                                   │                 │
│   └──────────────────────────────────────────────────────┘                 │
│           │                                                                  │
│           ├─── 路由缺失 ──▶ 检查CNI路由配置                                │
│           │                                                                  │
│           ▼                                                                  │
│   ┌──────────────────────────────────────────────────────┐                 │
│   │ Step 5: 跨节点连通性测试                              │                 │
│   │ ping <other-node-pod-ip>                             │                 │
│   └──────────────────────────────────────────────────────┘                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 2.2 详细诊断命令

```bash
# ========== 1. Pod网络状态检查 ==========

# 检查Pod网络状态
kubectl get pods -o wide --all-namespaces | grep -v "Running"

# 查看Pod网络接口
kubectl exec -it <pod-name> -n <namespace> -- ip addr show

# 检查Pod路由表
kubectl exec -it <pod-name> -n <namespace> -- ip route show

# ========== 2. CNI插件状态检查 ==========

# 检查CNI Pod状态 (以Calico为例)
kubectl get pods -n kube-system -l k8s-app=calico-node -o wide

# 检查CNI配置
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/*.conflist

# 检查CNI二进制文件
ls -la /opt/cni/bin/

# ========== 3. 节点网络检查 ==========

# 检查节点路由表
ip route show

# 检查ARP表
arp -a

# 检查网络接口状态
ip link show

# 检查iptables规则
iptables -t nat -L -n -v | head -20

# ========== 4. 跨节点连通性测试 ==========

# 测试Pod到Pod连通性
kubectl run debug-pod --rm -it --image=busybox -- sh
# 在Pod内执行: ping <other-pod-ip>

# 测试节点间连通性
ping <other-node-ip>

# 检查UDP端口连通性 (VXLAN常用4789端口)
nc -uzv <other-node-ip> 4789

# ========== 5. CNI日志分析 ==========

# 查看CNI Pod日志
kubectl logs -n kube-system -l k8s-app=calico-node --tail=100

# 查看kubelet CNI相关日志
journalctl -u kubelet | grep -i cni

# 查看CNI调用错误
grep "CNI failed" /var/log/messages
```

### 2.3 常见错误及解决方案

| 错误信息 | 可能原因 | 解决方案 |
|---------|---------|---------|
| `failed to set up sandbox container` | CNI插件未就绪 | 检查CNI Pod状态，重启CNI组件 |
| `no IP addresses available` | IP地址池耗尽 | 扩展IP池或清理未使用IP |
| `network plugin is not ready` | CNI初始化失败 | 检查CNI配置文件和权限 |
| `dial tcp: lookup xxx on 10.96.0.10:53` | DNS解析失败 | 检查CoreDNS和网络策略 |
| `connection refused` | 网络策略阻止 | 检查NetworkPolicy配置 |

---

## 3. DNS 解析故障排查 (DNS Resolution Issues)

### 3.1 DNS 故障诊断

```bash
# ========== 1. CoreDNS状态检查 ==========

# 检查CoreDNS Pod状态
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

# 检查CoreDNS服务
kubectl get svc -n kube-system kube-dns -o wide

# 检查CoreDNS配置
kubectl get configmap coredns -n kube-system -o yaml

# ========== 2. DNS解析测试 ==========

# 从Pod内测试DNS解析
kubectl run dns-test --rm -it --image=busybox -- sh
# 在Pod内执行:
# nslookup kubernetes.default
# nslookup google.com

# 测试集群内Service解析
kubectl exec -it <pod> -- nslookup <service>.<namespace>.svc.cluster.local

# ========== 3. DNS配置检查 ==========

# 检查Pod的resolv.conf
kubectl exec -it <pod> -- cat /etc/resolv.conf

# 检查kubelet DNS配置
cat /var/lib/kubelet/config.yaml | grep -A5 dns

# 检查集群DNS配置
kubectl cluster-info | grep dns
```

### 3.2 常见DNS问题

| 问题类型 | 症状 | 解决方案 |
|---------|------|---------|
| **CoreDNS Pod CrashLoopBackOff** | DNS服务不可用 | 检查CoreDNS日志，调整资源配置 |
| **解析超时** | DNS查询慢 | 检查上游DNS服务器，优化配置 |
| **解析失败** | 域名无法解析 | 检查NetworkPolicy，确认53端口开放 |
| **解析不一致** | 不同Pod解析结果不同 | 检查CoreDNS副本数和服务发现 |

---

## 4. 跨节点网络通信故障 (Cross-Node Network Issues)

### 4.1 网络互通性检查

```bash
# ========== 1. 隧道网络检查 (VXLAN/Overlay) ==========

# 检查VXLAN接口
ip link show type vxlan

# 检查VXLAN转发数据库
bridge fdb show | grep vxlan

# 检查UDP端口连通性
nc -uzv <node-ip> 4789  # VXLAN端口

# ========== 2. BGP路由检查 (Calico等) ==========

# 检查BGP邻居状态
calicoctl node status

# 检查路由表
ip route show | grep bird

# 查看BGP路由
calicoctl get workloadEndpoint -o wide

# ========== 3. 网络策略影响 ==========

# 检查NetworkPolicy
kubectl get networkpolicy --all-namespaces

# 检查特定命名空间的策略
kubectl get networkpolicy -n <namespace> -o yaml

# ========== 4. MTU问题排查 ==========

# 检查接口MTU
ip link show

# 测试不同大小的数据包
ping -M do -s 1400 <destination-ip>  # 不分片测试
ping -M do -s 1500 <destination-ip>  # 可能分片

# 检查iptables规则中的MTU处理
iptables -t mangle -L -n -v
```

---

## 5. CNI IP地址管理故障 (IPAM Issues)

### 5.1 IP地址耗尽问题

```bash
# ========== 1. IP使用情况检查 ==========

# 检查节点IP分配情况
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.pods}{"\n"}{end}'

# 检查Pod CIDR分配
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'

# ========== 2. CNI IP池状态 ==========

# Calico IP池检查
calicoctl get ippool -o wide

# 检查已分配IP
calicoctl ipam show

# 检查IP冲突
calicoctl ipam check

# ========== 3. IP回收操作 ==========

# 清理未使用的IP (谨慎操作)
calicoctl ipam release --ip=<ip-address>

# 扩展IP池
calicoctl create -f - <<EOF
apiVersion: projectcalico.org/v3
kind: IPPool
metadata:
  name: new-pool
spec:
  cidr: 10.245.0.0/16
  blockSize: 26
  natOutgoing: true
EOF
```

### 5.2 IPAM配置优化

```yaml
# Calico IP池配置示例
apiVersion: projectcalico.org/v3
kind: IPPool
metadata:
  name: default-ipv4-ippool
spec:
  cidr: 10.244.0.0/16
  blockSize: 26  # 每个块64个IP，适合大多数场景
  ipipMode: Never  # 根据网络环境选择
  natOutgoing: true
  disabled: false
```

---

## 6. 不同CNI插件特有故障 (CNI-Specific Issues)

### 6.1 Calico 故障排查

```bash
# ========== Calico专用诊断 ==========

# 安装calicoctl
curl -O -L https://github.com/projectcalico/calicoctl/releases/download/v3.26.0/calicoctl
chmod +x calicoctl

# 检查Calico节点状态
./calicoctl node status

# 检查BGP配置
./calicoctl get bgpconfig -o yaml

# 检查 Felix配置
./calicoctl get felixconfig -o yaml

# 查看路由信息
./calicoctl get workloadEndpoint -o wide

# 检查网络策略
./calicoctl get networkPolicy -o wide
```

### 6.2 Flannel 故障排查

```bash
# ========== Flannel专用诊断 ==========

# 检查Flannel Pod日志
kubectl logs -n kube-system -l app=flannel

# 检查Flannel子网配置
kubectl exec -n kube-system -l app=flannel -- cat /run/flannel/subnet.env

# 检查VXLAN接口
ip link show flannel.1

# 检查Flannel网络配置
cat /etc/kube-flannel/net-conf.json
```

---

## 7. 生产环境应急处理 (Production Emergency Response)

### 7.1 网络故障紧急诊断脚本

```bash
#!/bin/bash
# cni-network-emergency-check.sh

echo "=== CNI 网络紧急诊断 ==="
echo "时间: $(date)"
echo ""

# 1. 检查CNI组件状态
echo "1. CNI组件状态:"
kubectl get pods -n kube-system -l k8s-app=calico-node 2>/dev/null || \
kubectl get pods -n kube-system -l app=flannel 2>/dev/null || \
echo "❌ 未找到CNI组件"

# 2. 检查Pod网络状态
echo -e "\n2. 异常Pod统计:"
kubectl get pods --all-namespaces --field-selector=status.phase!=Running | wc -l

# 3. 检查CoreDNS
echo -e "\n3. CoreDNS状态:"
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide

# 4. 网络连通性测试
echo -e "\n4. 节点间连通性测试:"
for node in $(kubectl get nodes -o jsonpath='{.items[*].status.addresses[?(@.type=="InternalIP")].address}'); do
  echo -n "$node: "
  timeout 3 ping -c 1 $node >/dev/null 2>&1 && echo "✅" || echo "❌"
done

# 5. 检查IP使用情况
echo -e "\n5. IP使用概况:"
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.allocatable.pods}{"\n"}{end}' | head -10

echo -e "\n=== 诊断完成 ==="
```

### 7.2 故障处理优先级

| 故障类型 | 响应时间 | 处理步骤 |
|---------|---------|---------|
| **全集群网络中断** | 15分钟内 | 1. 确认CNI组件状态 2. 检查节点网络 3. 应急重启 |
| **DNS解析失败** | 30分钟内 | 1. 检查CoreDNS 2. 验证网络策略 3. 重启CoreDNS |
| **跨节点通信失败** | 1小时内 | 1. 检查隧道/BGP 2. 验证路由配置 3. 调整网络设置 |
| **IP地址耗尽** | 2小时内 | 1. 扩展IP池 2. 清理僵尸Pod 3. 优化IP回收 |

---

## 8. 预防措施与最佳实践 (Prevention & Best Practices)

### 8.1 监控告警配置

```yaml
# Prometheus网络监控告警
groups:
- name: network.rules
  rules:
  # CNI组件不可用
  - alert: CNIComponentDown
    expr: up{job=~"calico|flannel|cilium"} == 0
    for: 2m
    labels:
      severity: critical
    annotations:
      summary: "CNI组件 {{ $labels.job }} 不可用"
  
  # CoreDNS异常
  - alert: CoreDNSHealthError
    expr: coredns_health_request_duration_seconds_count{status!="success"} > 0
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "CoreDNS健康检查失败"
  
  # 网络延迟高
  - alert: NetworkLatencyHigh
    expr: histogram_quantile(0.99, rate(container_network_latency_seconds_bucket[5m])) > 0.1
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "网络延迟超过100ms"
```

### 8.2 运维检查清单

- [ ] 定期检查CNI组件健康状态
- [ ] 监控IP地址池使用率（阈值80%）
- [ ] 验证跨节点网络连通性
- [ ] 检查CoreDNS解析成功率
- [ ] 审查NetworkPolicy配置变更
- [ ] 测试网络故障恢复流程
- [ ] 保持CNI插件版本更新

---