# 142 - Flannel 完整指南 (Flannel Complete Guide)

> **适用版本**: Kubernetes v1.25 - v1.32 | **Flannel 版本**: v0.24+ | **最后更新**: 2026-01

---

## 1. Flannel 架构

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         Flannel 架构图                                   │
├─────────────────────────────────────────────────────────────────────────┤
│                                                                          │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │                    Control Plane                                 │   │
│   │  ┌───────────────────────────────────────────────────────────┐  │   │
│   │  │                   etcd / Kubernetes API                    │  │   │
│   │  │             (存储子网分配信息)                             │  │   │
│   │  └───────────────────────────────────────────────────────────┘  │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                    │                                     │
│                                    ▼                                     │
│   ┌─────────────────────────────────────────────────────────────────┐   │
│   │                      Node 1                                      │   │
│   │  ┌─────────────────────────────────────────────────────────┐    │   │
│   │  │  flanneld (DaemonSet)                                    │    │   │
│   │  │  - 监听子网变化                                          │    │   │
│   │  │  - 维护路由/FDB 表                                       │    │   │
│   │  │  - 配置 VXLAN/host-gw                                    │    │   │
│   │  └─────────────────────────────────────────────────────────┘    │   │
│   │                          │                                       │   │
│   │            ┌─────────────┼─────────────┐                         │   │
│   │            ▼             ▼             ▼                         │   │
│   │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐             │   │
│   │  │    Pod A     │ │    Pod B     │ │    Pod C     │             │   │
│   │  │ 10.244.1.2   │ │ 10.244.1.3   │ │ 10.244.1.4   │             │   │
│   │  └──────┬───────┘ └──────┬───────┘ └──────┬───────┘             │   │
│   │         │ veth           │ veth           │ veth                 │   │
│   │         └────────────────┼────────────────┘                      │   │
│   │                          ▼                                       │   │
│   │  ┌─────────────────────────────────────────────────────────┐    │   │
│   │  │              cni0 (Linux Bridge)                         │    │   │
│   │  │              IP: 10.244.1.1/24                           │    │   │
│   │  └─────────────────────────┬───────────────────────────────┘    │   │
│   │                            │                                     │   │
│   │                            ▼                                     │   │
│   │  ┌─────────────────────────────────────────────────────────┐    │   │
│   │  │           flannel.1 (VXLAN VTEP)                         │    │   │
│   │  │           或直接路由 (host-gw)                           │    │   │
│   │  └─────────────────────────┬───────────────────────────────┘    │   │
│   │                            │                                     │   │
│   │                            ▼                                     │   │
│   │  ┌─────────────────────────────────────────────────────────┐    │   │
│   │  │                     eth0                                 │    │   │
│   │  │                 IP: 192.168.1.10                         │    │   │
│   │  └─────────────────────────────────────────────────────────┘    │   │
│   └─────────────────────────────────────────────────────────────────┘   │
│                                                                          │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 2. 后端模式对比

| 后端 | 封装开销 | 性能 | 网络要求 | 适用场景 |
|:---|:---:|:---:|:---|:---|
| **VXLAN** | 50 bytes | 中 | 无特殊要求 | 云环境、默认选择 |
| **host-gw** | 0 | 高 | L2 互通 | 裸金属、同网段 |
| **UDP** | 较大 | 低 | 无特殊要求 | 调试、不推荐生产 |
| **IPIP** | 20 bytes | 中高 | 无特殊要求 | 跨网段、低开销 |
| **WireGuard** | 80 bytes | 中 | WireGuard 支持 | 需要加密 |

---

## 3. VXLAN 模式详解

### 3.1 VXLAN 工作流程

```
Pod A (10.244.1.10) ──▶ Pod B (10.244.2.20)

Node 1 (192.168.1.10)              Node 2 (192.168.1.20)
┌────────────────────┐             ┌────────────────────┐
│  1. Pod A 发包     │             │                    │
│  Dst: 10.244.2.20  │             │                    │
│         │          │             │                    │
│         ▼          │             │                    │
│  2. cni0 Bridge    │             │                    │
│  查路由表          │             │                    │
│  10.244.2.0/24     │             │                    │
│  via flannel.1     │             │                    │
│         │          │             │                    │
│         ▼          │             │                    │
│  3. flannel.1 VTEP │             │                    │
│  查 FDB 表         │             │                    │
│  10.244.2.0 →      │             │                    │
│  MAC: Node2-VTEP   │             │                    │
│  → 192.168.1.20    │             │                    │
│         │          │             │                    │
│         ▼          │             │                    │
│  4. VXLAN 封装     │             │  5. VXLAN 解封装   │
│  Outer IP:         │────UDP:8472─│  Dst: 10.244.2.20  │
│  192.168.1.10 →    │             │         │          │
│  192.168.1.20      │             │         ▼          │
│                    │             │  6. cni0 转发      │
│                    │             │         │          │
│                    │             │         ▼          │
│                    │             │  7. Pod B 收包     │
└────────────────────┘             └────────────────────┘
```

### 3.2 VXLAN 配置

```yaml
# ConfigMap 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-flannel-cfg
  namespace: kube-flannel
data:
  net-conf.json: |
    {
      "Network": "10.244.0.0/16",
      "Backend": {
        "Type": "vxlan",
        "VNI": 1,
        "Port": 8472,
        "DirectRouting": true
      }
    }
```

### 3.3 DirectRouting 混合模式

```json
{
  "Network": "10.244.0.0/16",
  "Backend": {
    "Type": "vxlan",
    "DirectRouting": true
  }
}
```

- **同子网**: 使用 host-gw (直接路由)
- **跨子网**: 使用 VXLAN 封装

---

## 4. host-gw 模式详解

### 4.1 host-gw 原理

```
Pod A (10.244.1.10) ──▶ Pod B (10.244.2.20)

Node 1 (192.168.1.10)              Node 2 (192.168.1.20)
┌────────────────────┐             ┌────────────────────┐
│                    │             │                    │
│  路由表:           │             │  路由表:           │
│  10.244.2.0/24     │             │  10.244.1.0/24     │
│  via 192.168.1.20  │             │  via 192.168.1.10  │
│  dev eth0          │             │  dev eth0          │
│                    │             │                    │
│  Pod A ──▶ cni0    │             │  cni0 ──▶ Pod B    │
│       ──▶ eth0 ────┼─────────────┼──▶ eth0            │
│                    │  直接路由   │                    │
│                    │  无封装     │                    │
└────────────────────┘             └────────────────────┘
```

### 4.2 host-gw 配置

```json
{
  "Network": "10.244.0.0/16",
  "Backend": {
    "Type": "host-gw"
  }
}
```

### 4.3 host-gw 限制

| 限制 | 说明 |
|:---|:---|
| L2 互通 | 所有节点必须在同一 L2 网络 |
| 不支持跨网段 | 无法跨越 L3 路由器 |
| 云环境限制 | 部分云环境不支持 |

---

## 5. 完整部署 YAML

```yaml
---
apiVersion: v1
kind: Namespace
metadata:
  name: kube-flannel
  labels:
    pod-security.kubernetes.io/enforce: privileged
---
apiVersion: v1
kind: ServiceAccount
metadata:
  name: flannel
  namespace: kube-flannel
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRole
metadata:
  name: flannel
rules:
  - apiGroups: [""]
    resources: ["pods"]
    verbs: ["get"]
  - apiGroups: [""]
    resources: ["nodes"]
    verbs: ["get", "list", "watch"]
  - apiGroups: [""]
    resources: ["nodes/status"]
    verbs: ["patch"]
  - apiGroups: ["networking.k8s.io"]
    resources: ["clustercidrs"]
    verbs: ["list", "watch"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: flannel
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: ClusterRole
  name: flannel
subjects:
  - kind: ServiceAccount
    name: flannel
    namespace: kube-flannel
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-flannel-cfg
  namespace: kube-flannel
data:
  cni-conf.json: |
    {
      "name": "cbr0",
      "cniVersion": "0.3.1",
      "plugins": [
        {
          "type": "flannel",
          "delegate": {
            "hairpinMode": true,
            "isDefaultGateway": true
          }
        },
        {
          "type": "portmap",
          "capabilities": {
            "portMappings": true
          }
        }
      ]
    }
  net-conf.json: |
    {
      "Network": "10.244.0.0/16",
      "Backend": {
        "Type": "vxlan",
        "DirectRouting": true
      }
    }
---
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: kube-flannel-ds
  namespace: kube-flannel
spec:
  selector:
    matchLabels:
      app: flannel
  template:
    metadata:
      labels:
        app: flannel
    spec:
      serviceAccountName: flannel
      priorityClassName: system-node-critical
      hostNetwork: true
      tolerations:
        - operator: Exists
      initContainers:
        - name: install-cni-plugin
          image: docker.io/flannel/flannel-cni-plugin:v1.4.0-flannel1
          command: ["cp"]
          args: ["-f", "/flannel", "/opt/cni/bin/flannel"]
          volumeMounts:
            - name: cni-plugin
              mountPath: /opt/cni/bin
        - name: install-cni
          image: docker.io/flannel/flannel:v0.24.2
          command: ["cp"]
          args: ["-f", "/etc/kube-flannel/cni-conf.json", "/etc/cni/net.d/10-flannel.conflist"]
          volumeMounts:
            - name: cni
              mountPath: /etc/cni/net.d
            - name: flannel-cfg
              mountPath: /etc/kube-flannel/
      containers:
        - name: kube-flannel
          image: docker.io/flannel/flannel:v0.24.2
          command: ["/opt/bin/flanneld"]
          args:
            - --ip-masq
            - --kube-subnet-mgr
            - --iface=eth0
          env:
            - name: POD_NAME
              valueFrom:
                fieldRef:
                  fieldPath: metadata.name
            - name: POD_NAMESPACE
              valueFrom:
                fieldRef:
                  fieldPath: metadata.namespace
            - name: EVENT_QUEUE_DEPTH
              value: "5000"
          securityContext:
            privileged: false
            capabilities:
              add: ["NET_ADMIN", "NET_RAW"]
          volumeMounts:
            - name: run
              mountPath: /run/flannel
            - name: flannel-cfg
              mountPath: /etc/kube-flannel/
            - name: xtables-lock
              mountPath: /run/xtables.lock
      volumes:
        - name: run
          hostPath:
            path: /run/flannel
        - name: cni-plugin
          hostPath:
            path: /opt/cni/bin
        - name: cni
          hostPath:
            path: /etc/cni/net.d
        - name: flannel-cfg
          configMap:
            name: kube-flannel-cfg
        - name: xtables-lock
          hostPath:
            path: /run/xtables.lock
            type: FileOrCreate
```

---

## 6. 故障排查

### 6.1 诊断命令

```bash
# 查看 Flannel Pod 状态
kubectl get pods -n kube-flannel -o wide

# 查看 Flannel 日志
kubectl logs -n kube-flannel -l app=flannel --tail=100

# 检查 VXLAN 接口
ip -d link show flannel.1

# 查看 FDB 表
bridge fdb show dev flannel.1

# 检查路由表
ip route | grep -E "10.244|flannel"

# 检查 ARP 表
ip neigh show dev flannel.1

# 检查 CNI 配置
cat /etc/cni/net.d/10-flannel.conflist

# 抓包调试
tcpdump -i flannel.1 -n
tcpdump -i eth0 -n port 8472  # VXLAN UDP 端口
```

### 6.2 常见问题

| 问题 | 原因 | 解决方案 |
|:---|:---|:---|
| Pod 无法获取 IP | CNI 配置错误 | 检查 /etc/cni/net.d/ |
| 跨节点不通 | 防火墙阻止 UDP 8472 | 开放防火墙端口 |
| 网络性能差 | MTU 配置不当 | 设置 MTU=1450 |
| FDB 表为空 | flanneld 未同步 | 重启 flanneld |
| 路由丢失 | flanneld 异常 | 检查 flanneld 日志 |

---

## 7. 性能优化

| 优化项 | 建议 |
|:---|:---|
| **后端选择** | 同子网用 host-gw，跨子网用 DirectRouting |
| **MTU** | VXLAN 设置 1450，host-gw 用默认 1500 |
| **接口绑定** | 指定 --iface 避免选错网卡 |
| **EVENT_QUEUE_DEPTH** | 大规模集群增加到 5000+ |

---

## 8. 与 NetworkPolicy 集成

Flannel 本身不支持 NetworkPolicy，需配合 Calico:

```bash
# 安装 Canal (Flannel + Calico NetworkPolicy)
kubectl apply -f https://raw.githubusercontent.com/projectcalico/calico/v3.27.0/manifests/canal.yaml
```

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)
