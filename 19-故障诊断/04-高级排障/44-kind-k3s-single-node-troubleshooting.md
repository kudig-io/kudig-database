---
title: Kind / K3s 单机集群故障排查
description: '# Kind / K3s 单机集群故障排查'
summary: 'Kind（[[kubernetes|Kubernetes]] in Docker）用于本地开发/测试/CI，每个"节点"是一个 Docker 容器。'
category: troubleshooting
tags:
- k8s
- troubleshooting
- debugging
- fault-analysis
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- flannel
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- Kind / K3s 单机集群故障排查 是什么
- 如何 Kind / K3s 单机集群故障排查
- Kubernetes 12 troubleshooting 最佳实践
- Kind / K3s 单机集群故障排查 故障排查
- Kind / K3s 单机集群故障排查 排障步骤
trigger_keywords:
- Kind
- K3s
- 单机集群故障排查
- troubleshooting
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- cni-basics
- etcd-basics
- mysql-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: fta
  path: ../故障诊断/FTA故障树/list/node-fta.md
  label: '故障树: node'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kind / [[k3s|K3s]] 单机集群故障排查

> **文档类型**: 故障排查手册 | **适用版本**: Kind (K8s 1.28-1.33) / K3s | **最后更新**: 2026-05
> **使用场景**: Agent 处理开发/测试环境（Kind/K3s）的常见问题

---

<!-- chunk: 1. Kind 单机集群问题 -->
## 1. Kind 单机集群问题

### 1.1 Kind 简介与核心概念

Kind（[[kubernetes|Kubernetes]] in Docker）用于本地开发/测试/CI，每个"节点"是一个 Docker 容器。

**架构**：
```
# 🟢 低风险：只读/信息收集，通常无副作用
宿主机（物理/虚拟机）
  └── Docker Daemon
       ├── kind-control-plane (容器)
       │    ├── kube-apiserver
       │    ├── kube-scheduler
       │    ├── kube-controller-manager
       │    └── etcd
       ├── kind-worker-1 (容器, 可选)
       │    ├── kubelet
       │    ├── kube-proxy
       │    └── containerd
       └── kind-worker-2 (容器, 可选)
```
**kind 基本命令**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kind create cluster --name=dev  # 创建名为 dev 的集群
kind get clusters               # 列出所有集群
kind delete cluster --name=dev  # 删除集群
kind get kubeconfig --name=dev  # 获取 kubeconfig
kind load docker-image <image> --name=dev  # 加载镜像到节点
```
---

### 1.2 Kind 端口冲突导致创建失败

**问题现象**: `kind create cluster` 报错 "port 6443 is already in use"

**排查步骤**：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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
# 1. 查看占用端口的进程
ss -tlnp | grep 6443
# 或
lsof -i :6443

# 2. 常见冲突
# - 6443 被 kube-apiserver（已有集群）
# - 6443 被 kubelet（本地 K8s）

# 3. 解决方案 1: 停止冲突服务
sudo systemctl stop kubelet  # 本地 kubelet
sudo docker stop $(sudo docker ps -q --filter name=kind*)  # 停止其他 kind 容器

# 4. 解决方案 2: 指定 API Server 端口
cat > kind-config.yaml <<EOF
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
  extraPortMappings:
  - containerPort: 6443
    hostPort: 6443
EOF
kind create cluster --config kind-config.yaml --name=dev
```
---

### 1.3 Kind 镜像加载失败

**问题现象**: `kind load docker-image myapp:v1.0` 报错 "image not found" 或超时

**排查步骤**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 确认镜像存在
docker images | grep myapp

# 2. 确认节点容器运行中
docker ps | grep kind

# 3. 加载失败时查看 kind 日志
kind create cluster --name=dev 2>&1 | tail -50

# 4. 使用内网 registry（避免每次 load）
# 创建 kind 集群时配置 registry
cat > registry-config.yaml <<EOF
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
containerdConfigPatches:
- |-
  [plugins."io.containerd.grpc.v1.cri".registry.mirrors."my-registry.example.com"]
    endpoint = ["http://my-registry.example.com:5000"]
nodes:
- role: control-plane
EOF
kind create cluster --config registry-config.yaml --name=dev
```
---

### 1.4 Kind 存储路径问题（Pod 数据丢失）

**问题现象**: Pod 重启后数据丢失，或 PV 无法持久化

**根因**: Kind 节点的 `/var/lib/kubelet` 等目录是容器内的匿名卷，重启后数据丢失

**排查步骤**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 kind 控制平面容器的挂载
docker inspect kind-control-plane --format '{{range .Mounts}}{{.Source}} -> {{.Destination}}{{"\n"}}{{end}}'

# 2. 使用持久化存储（将宿主机目录挂载到节点）
cat > kind-persistent.yaml <<EOF
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
  extraMounts:
  - hostPath: /tmp/kind-data
    containerPath: /var/lib/kubelet
EOF
kind create cluster --config kind-persistent.yaml --name=dev

# 3. 对于 PV 测试，使用 hostPath（仅开发环境）
cat > hostpath-pv.yaml <<EOF
apiVersion: v1
kind: PersistentVolume
metadata:
  name: hostpath-pv
spec:
  capacity:
    storage: 1Gi
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  hostPath:
    path: /tmp/kind-pv  # 在宿主机上
EOF
```
---

### 1.5 Kind 多节点集群网络问题

**问题现象**: Worker 节点无法与 Control Plane 通信，Pod 之间跨节点不通

**排查步骤**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查容器网络
docker network inspect kind

# 2. 检查节点间连通性（进入容器）
docker exec kind-control-plane ping -c 3 <worker-ip>
docker exec kind-worker-1 ping -c 3 <control-plane-ip>

# 3. 检查 CNI 插件状态
docker exec kind-control-plane crictl ps -a | grep cni
docker exec kind-worker-1 crictl ps -a | grep cni

# 4. 创建多节点 kind 集群
cat > kind-multi.yaml <<EOF
kind: Cluster
apiVersion: kind.x-k8s.io/v1alpha4
nodes:
- role: control-plane
- role: worker
- role: worker
EOF
kind create cluster --config kind-multi.yaml --name=dev

# 5. 查看节点状态
kubectl get nodes -o wide
```
---

### 1.6 Kind 集群无法删除（残留）

**问题现象**: `kind delete cluster` 卡住或报错，容器无法删除

**排查步骤**：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `docker prune/rm -f`：强制清理镜像/容器/卷，运行中容器会被杀
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据

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
# 1. 强制删除容器
docker rm -f $(docker ps -a | grep kind | awk '{print $1}')  # ⚠️ 强制清理，可能杀运行中容器

# 2. 清理残留网络
docker network rm $(docker network ls | grep kind | awk '{print $1}')

# 3. 手动清理
sudo rm -rf /tmp/kind-*  # ⚠️ 删除系统/数据文件
docker volume ls | grep kind
# 如有：docker volume rm $(docker volume ls -q | grep kind)  # ⚠️ 强制清理，可能杀运行中容器

# 4. 验证清理完成
kind get clusters  # 应无输出
docker ps -a | grep kind  # 应无输出
```
---

<!-- chunk: 2. K3s 单机集群问题 -->
## 2. K3s 单机集群问题

### 2.1 K3s 简介与架构

K3s 是轻量级 K8s，二进制运行，无需 Docker（默认使用 containerd）。

**架构特点**：
- 单二进制文件，包含所有 K8s 控制平面组件 + kubelet
- 默认使用 SQLite 作为数据存储（可选 etcd/外部数据库）
- 占用 < 512MB 内存
- 适合边缘/IoT/开发/测试

**关键路径**：
```bash
# K3s 二进制和配置
/usr/local/bin/k3s
/etc/rancher/k3s/k3s.yaml        # kubeconfig
/var/lib/rancher/k3s/             # 数据目录（包含 SQLite）
/var/lib/kubelet/pods/            # Pod 数据
/var/log/k3s.log                  # 日志
```

---

### 2.2 K3s 内置 SQLite 数据库损坏

**问题现象**: K3s 无法启动，报错 "database is locked" 或 "SQLite error"

**排查步骤**：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 查看 K3s 日志
journalctl -u k3s --since "5 minutes ago" | tail -50

# 2. 检查 SQLite 文件
ls -la /var/lib/rancher/k3s/server/db/
# 默认: /var/lib/rancher/k3s/server/db/state.db

# 3. 备份并重建（紧急）
sudo systemctl stop k3s
sudo mv /var/lib/rancher/k3s/server/db/state.db /var/lib/rancher/k3s/server/db/state.db.bak

# 4. 从备份恢复（如有）
sudo cp /var/lib/rancher/k3s/server/db/state.db.bak /var/lib/rancher/k3s/server/db/state.db

# 5. 重启 K3s
sudo systemctl start k3s
```
**预防措施**：
```bash
# 使用外部数据库替代 SQLite（生产级）
# K3s 支持 etcd/MySQL/PostgreSQL
# 启动时指定: k3s server --datastore-endpoint="mysql://..."
```

---

### 2.3 K3s agent 无法连接 server

**问题现象**: K3s agent 节点无法注册到 server，日志报 "connection refused" 或 "node not found"

**排查步骤**：
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 在 agent 节点查看日志
journalctl -u k3s-agent --since "5 minutes ago" | tail -50

# 2. 确认 server IP 和端口
# agent 启动参数: --server https://<server-ip>:6443

# 3. 测试网络连通性
curl -sf https://<server-ip>:6443/healthz  # server 是否可达

# 4. 检查 server 上的 agent secret
# 在 server 上:
kubectl get nodes  # 查看已注册节点

# 5. 检查 TOKEN 是否有效
# 在 server 上:
cat /var/lib/rancher/k3s/agent/node-token  # agent 注册 token

# 6. 重新注册 agent
# agent 节点上:
sudo k3s agent --server https://<server-ip>:6443 \
  --token-file=/var/lib/rancher/k3s/agent/node-token \
  --node-name=<hostname>
```
---

### 2.4 K3s 单机多容器网络问题

**问题现象**: Pod 之间无法通信，[[service|Service]] ClusterIP 无法访问

**排查步骤**：

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 CNI 配置
cat /var/lib/rancher/k3s/agent/etc/cni/net.d/10-flannel.conflist

# 2. 检查 flannel 状态
kubectl get pods -n kube-system -l app=flannel

# 3. 检查 iptables 规则
iptables -L -n -t nat | grep KUBE

# 4. 检查 pod CIDR
kubectl get nodes -o jsonpath='{.items[*].spec.podCIDR}'
# 或
cat /var/lib/rancher/k3s/agent/cni/net.d/flannel.conflist

# 5. 重启 K3s（清理网络状态）
sudo systemctl restart k3s

# 6. 如网络仍异常，手动重建 flannel
kubectl delete pods -n kube-system -l app=flannel
# K3s 会自动重建 flannel pod
```
---

### 2.5 K3s 资源不足导致组件崩溃

**问题现象**: K3s 所有组件（apiserver/scheduler/controller）同时崩溃

**排查步骤**：
```bash
# 1. 查看系统资源
free -h
df -h

# 2. K3s 最低要求：1C2G（建议 2C4G）
# 如内存 < 1G，K3s 会OOM

# 3. 查看 OOM 事件
dmesg | grep -i "out of memory"
journalctl -u k3s --since "10 minutes ago" | grep -i oom

# 4. 解决方案：增加资源或减少 K3s 功能
# 减少内存占用：禁用 traefik/servicelb
sudo k3s server --no-deploy traefik --no-deploy servicelb

# 5. 监控资源使用
top
htop
```

---

### 2.6 K3s 卸载/重装

**问题现象**: K3s 残留导致重装失败

**排查步骤**：

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `rm -rf (系统/数据路径)`：删除系统或数据文件，可能摧毁节点或丢失全部数据
> - `iptables -F/-P DROP`：清空/改防火墙规则，可能立即断网(含SSH)
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

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
# 1. 停止 K3s
sudo systemctl stop k3s
sudo systemctl disable k3s

# 2. 清理数据
sudo rm -rf /etc/rancher/k3s/  # ⚠️ 删除系统/数据文件
sudo rm -rf /var/lib/rancher/k3s/  # ⚠️ 删除系统/数据文件
sudo rm -rf /var/lib/kubelet  # ⚠️ 删除系统/数据文件
sudo rm -rf ~/.kube  # ⚠️ 删除系统/数据文件
sudo rm -f /usr/local/bin/k3s

# 3. 清理 iptables 残留
iptables -F
iptables -t nat -F
ipvsadm -C

# 4. 重新安装
curl -sfL https://get.k3s.io | sh -
```
---

<!-- chunk: 3. Kind vs K3s 问题对比 -->
## 3. Kind vs K3s 问题对比

| 问题场景 | Kind 特征 | K3s 特征 |
|---------|----------|---------|
| API Server 无响应 | Docker 容器崩溃（`docker ps` 查看） | K3s 进程崩溃（`journalctl` 查看） |
| 网络不通 | 容器网络 vs 宿主机网络桥接问题 | CNI (flannel) vs iptables 问题 |
| 存储丢失 | 容器重启后匿名卷数据丢失 | SQLite 损坏 vs 数据目录持久化 |
| 端口冲突 | 宿主机端口占用 | K3s 默认端口 6443/UDP 8472 |
| 无法删除 | Docker 容器残留 | K3s 数据目录残留 |
| 资源不足 | Docker Desktop 内存限制 | 宿主机资源不足 |

---

<!-- chunk: 4. 故障排查命令速查 -->
## 4. 故障排查命令速查

| 场景 | Kind 命令 | K3s 命令 |
|------|----------|---------|
| 查看日志 | `docker logs kind-control-plane` | `journalctl -u k3s --since "10m"` |
| 进入节点 | `docker exec -it kind-control-plane bash` | N/A（直接在节点上） |
| 检查容器 | `docker ps -a | grep kind` | `systemctl status k3s` |
| 检查存储 | `docker inspect kind-control-plane | jq '.[0].Mounts'` | `ls /var/lib/rancher/k3s/` |
| 删除集群 | `kind delete cluster --name=dev` | `k3s-uninstall.sh` |
| 加载镜像 | `kind load docker-image img --name=dev` | `crictl pull img` |

---

```yaml
---
id: KIND-K3S-TROUBLESHOOTING-001
domain: troubleshooting
type: troubleshooting-guide
tags: [kind, k3s, local-cluster, single-node, development, agent-corpus, k8s-1.28-1.33]
intent_queries:
  - "Kind 端口冲突怎么解决"
  - "K3s SQLite 数据库损坏怎么办"
  - "Kind 镜像加载失败"
  - "K3s agent 无法连接 server"
  - "Kind 集群无法删除残留"
difficulty: intermediate
target_roles: [sre, ops-engineer, developer]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
related:
  - 故障诊断/01-control-plane-apiserver-troubleshooting.md
  - 故障诊断/FTA故障树/list/apiserver-fta.md
---
```

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 故障诊断 MOC
- [[19-故障诊断/README.md|Domain-12 故障排查 (Troubleshooting)]]
- Domain-12 故障排查 — 开源项目索引
- [[19-故障诊断/01-核心排障/01-control-plane-apiserver-troubleshooting.md|API Server 故障排查]]
- [[19-故障诊断/01-核心排障/02-control-plane-etcd-troubleshooting.md|etcd 故障排查]]
- [[19-故障诊断/01-核心排障/03-networking-cni-troubleshooting.md|CNI 网络插件故障排查]]
- [[19-故障诊断/01-核心排障/04-storage-csi-troubleshooting.md|CSI 存储驱动故障排查]]
- [[19-故障诊断/01-核心排障/05-pod-pending-diagnosis.md|Pod Pending 状态深度诊断]]
- [[19-故障诊断/01-核心排障/06-node-notready-diagnosis.md|Node NotReady 状态深度诊断]]
- [[19-故障诊断/01-核心排障/07-oom-memory-diagnosis.md|OOM 和内存问题诊断]]
- [[19-故障诊断/01-核心排障/08-pod-comprehensive-troubleshooting.md|Pod 全面故障排查]]
- [[19-故障诊断/02-资源排障/09-node-comprehensive-troubleshooting.md|Node 全面故障排查]]
- [[19-故障诊断/06-FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## See Also

- [[19-故障诊断/04-高级排障/42-chaos-engineering-fault-injection-testing.md|42-chaos-engineering-fault-injection-testing]]
- [[19-故障诊断/04-高级排障/43-symptom-sop-mapping.md|43-symptom-sop-mapping]]
- [[19-故障诊断/05-JVM调优/99-java-performance-resource-sizing-guide.md|99-java-performance-resource-sizing-guide]]
- [[19-故障诊断/05-JVM调优/99-jvm-gc-container-tuning-guide.md|99-jvm-gc-container-tuning-guide]]

```

<!-- risk-assessed -->
