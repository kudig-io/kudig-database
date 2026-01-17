# 问题诊断工具

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [kubectl-debug](https://github.com/aylei/kubectl-debug) | [Inspektor Gadget](https://www.inspektor-gadget.io/)

## 工具对比

| 工具 | 类型 | 特权需求 | 学习曲线 | 功能范围 | 实时性 | 生产推荐 |
|------|------|---------|---------|---------|--------|---------|
| **kubectl-debug** | 临时容器 | 低 | ⭐⭐⭐⭐⭐ | 容器调试 | 实时 | 强烈推荐 |
| **Inspektor Gadget** | eBPF追踪 | 需要 | ⭐⭐⭐ | 系统级 | 实时 | 深度诊断 |
| **kubectl-trace** | eBPF追踪 | 需要 | ⭐⭐⭐ | 内核追踪 | 实时 | 性能分析 |
| **Sysdig** | 系统监控 | 需要 | ⭐⭐⭐⭐ | 全栈 | 实时 | 企业级 |
| **kubectl-exec** | 内置 | 低 | ⭐⭐⭐⭐⭐ | 基础调试 | 实时 | 基础场景 |
| **ephemeral容器** | K8s原生 | 低 | ⭐⭐⭐⭐ | 容器调试 | 实时 | v1.25+ |

---

## kubectl-debug 快速调试

### 核心特性

```
kubectl-debug优势
├── 无需重启Pod
├── 支持Distroless镜像调试
├── 自动清理调试容器
├── 支持主机网络命名空间
└── 丰富的调试工具镜像
```

### 安装与使用

```bash
# 安装kubectl-debug
curl -Lo kubectl-debug.tar.gz https://github.com/aylei/kubectl-debug/releases/download/v0.1.1/kubectl-debug_0.1.1_linux_amd64.tar.gz
tar -zxvf kubectl-debug.tar.gz kubectl-debug
sudo mv kubectl-debug /usr/local/bin/

# 调试Pod(自动注入调试容器)
kubectl debug myapp-pod-xxxx

# 使用自定义镜像
kubectl debug myapp-pod-xxxx --image=nicolaka/netshoot

# 调试节点(hostNetwork)
kubectl debug node/node-1 --image=nicolaka/netshoot

# 指定容器
kubectl debug myapp-pod-xxxx -c myapp --image=busybox

# 共享进程命名空间
kubectl debug myapp-pod-xxxx --share-processes

# Fork模式(复制Pod)
kubectl debug myapp-pod-xxxx --copy-to=myapp-debug
```

### 调试镜像推荐

| 镜像 | 包含工具 | 适用场景 |
|------|---------|---------|
| **nicolaka/netshoot** | tcpdump, curl, netstat, dig | 网络问题 |
| **busybox** | 基础Shell工具 | 轻量调试 |
| **alpine** | apk包管理器 | 通用调试 |
| **debian:slim** | apt包管理器 | 需要安装工具 |
| **aylei/debug-agent** | strace, gdb, perf | 性能分析 |

---

## Ephemeral Containers (v1.25+ GA)

### Kubernetes原生调试容器

```bash
# 添加临时调试容器
kubectl debug -it myapp-pod-xxxx \
  --image=busybox:1.36 \
  --target=myapp \
  --share-processes

# 查看临时容器
kubectl describe pod myapp-pod-xxxx

# 清理(删除Pod后自动清理)
kubectl delete pod myapp-pod-xxxx
```

### YAML配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: myapp
spec:
  shareProcessNamespace: true  # 启用进程命名空间共享
  containers:
    - name: myapp
      image: myapp:latest
      ports:
        - containerPort: 8080
  # 临时容器定义(由kubectl debug自动添加)
  ephemeralContainers:
    - name: debugger
      image: nicolaka/netshoot
      command: ["/bin/sh"]
      stdin: true
      tty: true
      targetContainerName: myapp
```

---

## Inspektor Gadget - eBPF追踪

### 核心能力

```
Inspektor Gadget追踪范围
├── 网络追踪 (tcpconnect, dns, http)
├── 文件系统 (opensnoop, fsslower)
├── 进程追踪 (execsnoop, biosnoop)
├── 安全审计 (capabilities, seccomp)
└── 性能分析 (profile, biotop)
```

### 安装

```bash
# 安装kubectl-gadget插件
kubectl krew install gadget

# 部署DaemonSet
kubectl gadget deploy

# 验证安装
kubectl gadget version
```

### 实用Gadget示例

#### 1. 网络连接追踪

```bash
# 追踪TCP连接
kubectl gadget trace tcpconnect \
  --namespace production \
  --podname myapp-xxxx

# 输出示例
TIMESTAMP   NAMESPACE   POD          COMM    PID    IP  SADDR          SPORT  DADDR          DPORT
14:32:01    production  myapp-xxxx   curl    1234   4   10.0.1.5       54321  1.2.3.4        443

# DNS查询追踪
kubectl gadget trace dns \
  --namespace production

# HTTP请求追踪
kubectl gadget trace http \
  --namespace production
```

#### 2. 文件系统操作

```bash
# 文件打开追踪
kubectl gadget trace opensnoop \
  --namespace production \
  --podname myapp-xxxx

# 慢速I/O追踪
kubectl gadget trace fsslower \
  --namespace production \
  --min-latency 10ms
```

#### 3. 进程执行追踪

```bash
# 执行命令追踪
kubectl gadget trace execsnoop \
  --namespace production

# 输出示例
TIMESTAMP   NAMESPACE   POD          PCOMM   PID    PPID   RET  ARGS
14:35:12    production  myapp-xxxx   bash    5678   1234   0    /bin/sh -c curl api.example.com
```

#### 4. 性能分析

```bash
# CPU性能分析(30秒)
kubectl gadget profile cpu \
  --namespace production \
  --podname myapp-xxxx \
  --timeout 30s

# 块I/O分析
kubectl gadget top block-io \
  --namespace production

# 文件系统延迟分析
kubectl gadget top file \
  --namespace production
```

#### 5. 安全审计

```bash
# Capabilities使用追踪
kubectl gadget trace capabilities \
  --namespace production

# Seccomp违规追踪
kubectl gadget trace seccomp \
  --namespace production
```

---

## kubectl-trace - bpftrace集成

### 安装与使用

```bash
# 安装kubectl-trace
kubectl krew install trace

# 追踪系统调用
kubectl trace run node/node-1 \
  -e 'tracepoint:syscalls:sys_enter_* { @[probe] = count(); }'

# 追踪特定Pod
kubectl trace run pod/myapp-xxxx -n production \
  -e 'kprobe:tcp_sendmsg { @[comm] = count(); }'

# 追踪网络延迟
kubectl trace run node/node-1 \
  -e '
    kprobe:tcp_sendmsg {
      @start[tid] = nsecs;
    }
    kretprobe:tcp_sendmsg /@start[tid]/ {
      @latency_us = hist((nsecs - @start[tid]) / 1000);
      delete(@start[tid]);
    }
  '
```

### bpftrace脚本示例

```bash
# 追踪DNS查询(getaddrinfo)
kubectl trace run pod/myapp-xxxx -n production -e '
  uprobe:/lib/x86_64-linux-gnu/libc.so.6:getaddrinfo {
    printf("DNS query: %s\n", str(arg0));
  }
'

# 追踪HTTP请求(Go应用)
kubectl trace run pod/myapp-xxxx -n production -e '
  uprobe:/app:net/http.(*Request).Write {
    printf("HTTP request: %s %s\n", str(arg1), str(arg2));
  }
'
```

---

## Sysdig 企业级监控

### DaemonSet 部署

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: sysdig-agent
  namespace: sysdig
spec:
  selector:
    matchLabels:
      app: sysdig-agent
  template:
    metadata:
      labels:
        app: sysdig-agent
    spec:
      hostNetwork: true
      hostPID: true
      tolerations:
        - effect: NoSchedule
          key: node-role.kubernetes.io/master
      containers:
        - name: sysdig-agent
          image: sysdig/agent:latest
          securityContext:
            privileged: true
          env:
            - name: ACCESS_KEY
              valueFrom:
                secretKeyRef:
                  name: sysdig-secret
                  key: access-key
            - name: TAGS
              value: "env:production,cluster:k8s-prod"
          volumeMounts:
            - mountPath: /host/var/run/docker.sock
              name: docker-sock
            - mountPath: /host/dev
              name: dev-vol
              readOnly: true
            - mountPath: /host/proc
              name: proc-vol
              readOnly: true
            - mountPath: /host/boot
              name: boot-vol
              readOnly: true
            - mountPath: /host/lib/modules
              name: modules-vol
              readOnly: true
            - mountPath: /host/usr
              name: usr-vol
              readOnly: true
      volumes:
        - name: docker-sock
          hostPath:
            path: /var/run/docker.sock
        - name: dev-vol
          hostPath:
            path: /dev
        - name: proc-vol
          hostPath:
            path: /proc
        - name: boot-vol
          hostPath:
            path: /boot
        - name: modules-vol
          hostPath:
            path: /lib/modules
        - name: usr-vol
          hostPath:
            path: /usr
```

### 命令行使用

```bash
# 实时系统调用追踪
sysdig -k k8s.namespace.name=production and k8s.pod.name contains myapp

# 网络连接追踪
sysdig -k k8s.pod.name=myapp-xxxx and evt.type=connect

# 文件I/O追踪
sysdig -k k8s.pod.name=myapp-xxxx and evt.type=open

# 保存捕获文件
sysdig -k k8s.pod.name=myapp-xxxx -w capture.scap

# 回放分析
sysdig -r capture.scap
```

---

## 常见问题诊断场景

### 1. Pod启动失败

```bash
# 查看事件
kubectl describe pod myapp-xxxx

# 查看日志
kubectl logs myapp-xxxx --previous

# 调试Init容器
kubectl debug myapp-xxxx --copy-to=debug-pod \
  --container=init-myapp

# 检查镜像拉取
kubectl get events --field-selector involvedObject.name=myapp-xxxx
```

### 2. 网络连通性问题

```bash
# 进入调试容器
kubectl debug myapp-xxxx --image=nicolaka/netshoot

# 网络工具
ping target-service
nslookup target-service
traceroute target-service
curl -v http://target-service
tcpdump -i eth0 port 80

# 使用Inspektor Gadget追踪
kubectl gadget trace tcpconnect --namespace production
kubectl gadget trace dns --namespace production
```

### 3. 性能问题

```bash
# CPU性能分析
kubectl gadget profile cpu --namespace production --podname myapp-xxxx

# 内存分析
kubectl top pod myapp-xxxx
kubectl debug myapp-xxxx --image=aylei/debug-agent
# 在调试容器中
pmap -x $(pgrep myapp)

# I/O性能
kubectl gadget top block-io --namespace production
```

### 4. 应用崩溃

```bash
# 查看coredump
kubectl debug myapp-xxxx --image=aylei/debug-agent --share-processes
# 在调试容器中
gdb /app $(pidof app) /tmp/core

# 追踪系统调用
kubectl trace run pod/myapp-xxxx -e '
  tracepoint:signal:signal_deliver /comm == "myapp"/ {
    printf("Signal %d sent to PID %d\n", args->sig, pid);
  }
'
```

---

## 调试工具箱脚本

### debug-toolkit.sh

```bash
#!/bin/bash
# Kubernetes问题诊断工具箱

POD_NAME=$1
NAMESPACE=${2:-default}

echo "=== 基础信息 ==="
kubectl get pod $POD_NAME -n $NAMESPACE -o wide
kubectl describe pod $POD_NAME -n $NAMESPACE

echo -e "\n=== 资源使用 ==="
kubectl top pod $POD_NAME -n $NAMESPACE

echo -e "\n=== 事件 ==="
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$POD_NAME

echo -e "\n=== 日志(最近100行) ==="
kubectl logs $POD_NAME -n $NAMESPACE --tail=100

echo -e "\n=== 进入调试容器 ==="
kubectl debug $POD_NAME -n $NAMESPACE --image=nicolaka/netshoot -it
```

---

## 工具选型建议

| 场景 | 推荐工具 | 原因 |
|------|---------|------|
| **快速排查** | kubectl-debug + netshoot | 无需重启、工具丰富 |
| **网络问题** | Inspektor Gadget trace | 实时追踪、无侵入 |
| **性能分析** | kubectl-trace + bpftrace | 内核级追踪 |
| **安全审计** | Inspektor Gadget capabilities | eBPF审计 |
| **企业监控** | Sysdig | 完整商业支持 |

---

## 最佳实践

```yaml
# 1. 生产环境准备
prepare:
  - 安装kubectl-debug插件
  - 部署Inspektor Gadget
  - 准备调试镜像(netshoot)
  - 配置RBAC权限

# 2. 安全考虑
security:
  - 限制调试容器使用范围
  - 审计调试操作日志
  - 自动清理临时容器
  - 使用只读文件系统

# 3. 工具组合
toolchain:
  - 基础: kubectl + ephemeral containers
  - 网络: Inspektor Gadget trace
  - 性能: kubectl-trace + profile
  - 深度: Sysdig全栈分析
```

---

## 常见问题

**Q: 调试Distroless镜像如何操作?**  
A: 使用kubectl-debug或ephemeral containers注入调试工具。

**Q: eBPF工具需要特权容器吗?**  
A: 是的，Inspektor Gadget和kubectl-trace需要特权权限访问内核。

**Q: 生产环境使用调试工具安全吗?**  
A: kubectl-debug和ephemeral containers是安全的(隔离)，eBPF工具需要严格RBAC控制。
