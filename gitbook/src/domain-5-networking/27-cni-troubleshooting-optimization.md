# 144 - CNI 故障排查与优化 (CNI Troubleshooting & Optimization)

> **适用版本**: Kubernetes v1.25 - v1.32 | **难度**: 高级 | **最后更新**: 2026-01

---

## 1. CNI 故障分级

| 级别 | 类型 | 影响 | 优先级 |
|:---|:---|:---|:---|
| **P0** | Pod 网络初始化失败 | Pod 无法启动 | 紧急 |
| **P1** | Pod 间通信故障 | 服务不可用 | 高 |
| **P2** | Service 访问异常 | 部分功能异常 | 中 |
| **P3** | 网络性能问题 | 延迟/丢包 | 低 |

---

## 2. 系统化排查流程

```
┌─────────────────────────────────────────────────────────────────────────┐
│                       CNI 故障排查流程图                                 │
└─────────────────────────────────────────────────────────────────────────┘

                    ┌───────────────┐
                    │  故障发现     │
                    └───────┬───────┘
                            │
                            ▼
              ┌─────────────────────────┐
              │ Step 1: CNI 组件检查    │
              │ - CNI Pod 状态          │
              │ - CNI 配置文件          │
              │ - CNI 二进制文件        │
              └───────────┬─────────────┘
                          │
            ┌─────────────┼─────────────┐
            │ 正常        │             │ 异常
            ▼             │             ▼
┌───────────────────┐     │    ┌───────────────────┐
│ Step 2: 节点网络  │     │    │ 修复 CNI 组件    │
│ - 网络接口        │     │    │ - 重启 Pod       │
│ - 路由表          │     │    │ - 重新部署       │
│ - iptables        │     │    │ - 检查权限       │
└─────────┬─────────┘     │    └───────────────────┘
          │               │
    ┌─────┼─────┐         │
    │正常 │     │异常      │
    ▼     │     ▼         │
┌─────────┴───────────┐   │
│ Step 3: Pod 网络    │   │
│ - 网络命名空间      │   │
│ - veth 接口         │   │
│ - IP 配置           │   │
└─────────┬───────────┘   │
          │               │
    ┌─────┼─────┐         │
    │正常 │     │异常      │
    ▼     │     ▼         │
┌─────────┴───────────┐   │
│ Step 4: 连通性测试  │   │
│ - 同节点 Pod        │   │
│ - 跨节点 Pod        │   │
│ - Service/DNS       │   │
└─────────────────────┘   │
```

---

## 3. 诊断命令速查表

| 检查项 | 命令 |
|:---|:---|
| **CNI Pod 状态** | `kubectl get pods -n kube-system -l k8s-app=calico-node` |
| **CNI 配置** | `cat /etc/cni/net.d/*.conflist` |
| **CNI 插件** | `ls -la /opt/cni/bin/` |
| **网络接口** | `ip link show` |
| **路由表** | `ip route show` |
| **iptables** | `iptables -t nat -L -n -v` |
| **IPVS** | `ipvsadm -Ln` |
| **Pod 网络** | `kubectl exec <pod> -- ip addr` |
| **DNS 测试** | `kubectl exec <pod> -- nslookup kubernetes` |
| **连通性** | `kubectl exec <pod> -- ping <target-ip>` |

---

## 4. Pod 网络初始化失败

### 4.1 现象

```
Events:
  Warning  FailedCreatePodSandBox  kubelet  Failed to create pod sandbox: 
    rpc error: code = Unknown desc = failed to setup network for sandbox: 
    plugin type="calico" failed: error getting ClusterInformation: 
    connection refused
```

### 4.2 排查步骤

```bash
# 1. 检查 CNI 配置文件
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/10-calico.conflist

# 2. 检查 CNI 插件
ls -la /opt/cni/bin/
/opt/cni/bin/calico --version

# 3. 检查 CNI Pod 状态
kubectl get pods -n kube-system -l k8s-app=calico-node
kubectl describe pod -n kube-system calico-node-xxxxx

# 4. 检查 CNI 日志
kubectl logs -n kube-system calico-node-xxxxx -c calico-node --tail=100
journalctl -u kubelet | grep -i cni

# 5. 检查 kubelet 日志
journalctl -u kubelet | grep -i "network plugin"
```

### 4.3 常见原因与解决

| 原因 | 解决方案 |
|:---|:---|
| CNI 配置文件缺失 | 重新部署 CNI |
| CNI 二进制缺失 | 复制或重新安装 CNI 插件 |
| CNI Pod 未就绪 | 检查 CNI Pod 状态和日志 |
| IPAM 分配失败 | 检查 IP 池配置和可用 IP |

---

## 5. IPAM 故障排查

### 5.1 Calico IPAM

```bash
# 查看 IP 池
kubectl get ippools -o yaml

# 查看 IP 分配
calicoctl ipam show

# 查看节点 IP 块
calicoctl ipam show --show-blocks

# 检查 IP 使用情况
calicoctl ipam check

# 释放未使用的 IP
calicoctl ipam release --ip=10.244.1.100
```

### 5.2 Flannel IPAM

```bash
# 查看子网分配
cat /run/flannel/subnet.env

# 检查节点子网注解
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.podCIDR}{"\n"}{end}'
```

### 5.3 Terway IPAM

```bash
# 查看 ENI IP 分配
kubectl exec -n kube-system <terway-pod> -c terway -- terway-cli show

# 查看节点 ENI 状态
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.annotations.k8s\.aliyun\.com/allocated-eniips}{"\n"}{end}'
```

---

## 6. 同节点 Pod 通信故障

### 6.1 排查路径

```
Pod A ──▶ veth ──▶ Bridge/路由 ──▶ veth ──▶ Pod B
         │              │              │
         检查点1       检查点2        检查点3
```

### 6.2 诊断命令

```bash
# 1. 查看 Pod 网络配置
kubectl exec pod-a -- ip addr
kubectl exec pod-a -- ip route

# 2. 查看 veth pair
ip link show | grep veth
bridge link show

# 3. 检查 bridge/cni0
ip addr show cni0
bridge fdb show br cni0

# 4. 抓包分析
tcpdump -i cni0 -n host <pod-a-ip> and host <pod-b-ip>

# 5. 检查 iptables
iptables -t filter -L FORWARD -n -v
```

---

## 7. 跨节点 Pod 通信故障

### 7.1 VXLAN 模式排查

```bash
# 1. 检查 VXLAN 接口
ip -d link show flannel.1

# 2. 检查 FDB 表
bridge fdb show dev flannel.1

# 3. 检查路由
ip route | grep <target-pod-cidr>

# 4. 检查防火墙 (UDP 8472/4789)
iptables -t filter -L INPUT -n | grep -E "8472|4789"
ss -ulnp | grep -E "8472|4789"

# 5. 抓包 (外层封装)
tcpdump -i eth0 -n udp port 8472

# 6. 验证 MTU
ping -M do -s 1422 <target-pod-ip>  # VXLAN: 1500-78=1422
```

### 7.2 BGP/路由模式排查

```bash
# 1. 检查 BGP 状态 (Calico)
calicoctl node status

# 2. 检查路由表
ip route | grep <target-node-ip>

# 3. 检查 BGP 邻居
calicoctl get bgpPeer -o yaml

# 4. 路由跟踪
traceroute <target-pod-ip>
```

---

## 8. DNS 解析故障

### 8.1 排查步骤

```bash
# 1. 检查 CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 2. 检查 DNS Service
kubectl get svc -n kube-system kube-dns

# 3. 检查 Pod DNS 配置
kubectl exec <pod> -- cat /etc/resolv.conf

# 4. DNS 解析测试
kubectl exec <pod> -- nslookup kubernetes.default
kubectl exec <pod> -- nslookup <service-name>.<namespace>

# 5. CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=100
```

### 8.2 常见问题

| 问题 | 原因 | 解决方案 |
|:---|:---|:---|
| DNS 超时 | CoreDNS Pod 异常 | 重启 CoreDNS |
| NXDOMAIN | Service 不存在 | 检查 Service 名称和命名空间 |
| 解析到错误 IP | DNS 缓存 | 清理 CoreDNS 缓存 |

---

## 9. 网络性能优化

### 9.1 MTU 优化

```yaml
# Calico MTU 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: calico-config
  namespace: kube-system
data:
  veth_mtu: "1440"  # VXLAN: 1500-60=1440
```

### 9.2 启用 eBPF (Cilium)

```yaml
# cilium-config ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: cilium-config
  namespace: kube-system
data:
  enable-bpf-masquerade: "true"
  kube-proxy-replacement: "strict"
  enable-bandwidth-manager: "true"
```

### 9.3 IPVS 模式

```yaml
# kube-proxy ConfigMap
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    mode: "ipvs"
    ipvs:
      scheduler: "rr"
      strictARP: true
```

---

## 10. 监控告警

### 10.1 关键指标

```yaml
# Prometheus 告警规则
groups:
  - name: cni-alerts
    rules:
      - alert: CNIPodNotReady
        expr: |
          kube_pod_status_ready{namespace="kube-system", pod=~"calico.*|flannel.*|cilium.*"} == 0
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "CNI Pod 未就绪"
          
      - alert: PodNetworkLatencyHigh
        expr: |
          histogram_quantile(0.99, rate(container_network_receive_packets_total[5m])) > 100
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Pod 网络延迟过高"
          
      - alert: IPPoolExhausted
        expr: |
          (calico_ipam_blocks_per_node - calico_ipam_blocks_per_node_free) / calico_ipam_blocks_per_node > 0.9
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "IP 池即将耗尽"
```

---

## 11. 最佳实践清单

| 类别 | 建议 |
|:---|:---|
| **预防** | 定期检查 CNI Pod 状态和日志 |
| **监控** | 配置网络延迟、丢包、IP 池告警 |
| **文档** | 记录网络架构和 CNI 配置 |
| **测试** | 定期执行网络连通性测试 |
| **备份** | 备份 CNI 配置和 IPAM 数据 |
| **升级** | CNI 升级前在测试环境验证 |

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)
