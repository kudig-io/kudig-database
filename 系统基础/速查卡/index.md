---
title: 速查卡总索引
description: 云原生全域速查卡总索引，覆盖 K8s、Linux、Docker、Git、Helm、网络、PromQL、SQL、TLS/PKI、GitOps、Gateway API 等核心工具命令速查
summary: 云原生速查卡总索引，覆盖 15+ 个技术领域，包含常用命令、参数速查、故障排查流程
category: index
tags:
- index
- cheat-sheet
- kubernetes
- linux
- docker
tier: core
created: '2026-07-02'
last_updated: 2026-07
difficulty: beginner
audience:
- 所有工程师
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 云原生速查卡总索引

> 本速查卡集合覆盖云原生工程师日常工作中最常用的工具和命令，是生产运维的快速参考。

## 速查卡目录

### 平台与编排

| 速查卡 | 内容 | 行数 | 难度 |
|--------|------|------|------|
| [[系统基础/速查卡/k8s.md|K8s 速查卡]] | K8s 全资源操作、集群管理、故障排查 | 1800+ | 入门→高级 |
| [[系统基础/速查卡/kubectl-scene-cheatsheet.md|kubectl 场景速查]] | 按场景分类的 kubectl 命令 | 650+ | 入门→中级 |
| [[系统基础/速查卡/helm.md|Helm 速查卡]] | Chart 管理、模板、仓库操作 | 290+ | 入门→中级 |
| [[系统基础/速查卡/gateway-api.md|Gateway API 速查]] | Gateway/HTTPRoute/TLS 配置 | 270+ | 中级→高级 |
| [[系统基础/速查卡/gitops.md|GitOps 速查卡]] | ArgoCD/Flux 操作命令 | 300+ | 中级→高级 |

### 系统与容器

| 速查卡 | 内容 | 行数 | 难度 |
|--------|------|------|------|
| [[系统基础/速查卡/linux.md|Linux 速查卡]] | 系统管理、性能、网络、存储 | 2200+ | 入门→高级 |
| [[系统基础/速查卡/docker.md|Docker 速查卡]] | 镜像、容器、网络、构建 | 520+ | 入门→中级 |
| [[系统基础/速查卡/networking.md|网络速查卡]] | TCP/IP、DNS、抓包、防火墙 | 600+ | 中级→高级 |
| [[系统基础/速查卡/tls-pki.md|TLS/PKI 速查卡]] | 证书、加密、PKI 操作 | 500+ | 中级→高级 |

### 可观测性

| 速查卡 | 内容 | 行数 | 难度 |
|--------|------|------|------|
| [[系统基础/速查卡/promql.md|PromQL 速查卡]] | 查询语言、函数、告警规则 | 560+ | 中级→高级 |
| [[系统基础/速查卡/perf-bpftrace-cheat-sheet.md|性能/bpftrace 速查]] | 性能分析、eBPF 追踪 | 730+ | 高级→专家 |

### 开发与工具

| 速查卡 | 内容 | 行数 | 难度 |
|--------|------|------|------|
| [[系统基础/速查卡/git.md|Git 速查卡]] | 版本控制、分支、合并、变基 | 650+ | 入门→高级 |
| [[系统基础/速查卡/go.md|Go 速查卡]] | Go 语言、并发、测试、工具链 | 2600+ | 入门→高级 |
| [[系统基础/速查卡/sql.md|SQL 速查卡]] | 查询、索引、优化、运维 | 770+ | 入门→高级 |

### 索引与导航

| 文档 | 内容 |
|------|------|
| [[系统基础/速查卡/MOC.md|内容地图 (MOC)]] | 速查卡导航与关联 |
| [[系统基础/速查卡/README.md|说明文档]] | 使用指南与贡献规范 |

## 快速参考：最常用命令 Top 20

### kubectl 最常用

```bash
kubectl get pods -A -o wide                    # 查看所有 Pod
kubectl describe pod <name> -n <ns>            # Pod 详情
kubectl logs <pod> -n <ns> --tail=100 -f       # 实时日志
kubectl exec -it <pod> -n <ns> -- /bin/sh      # 进入容器
kubectl apply -f manifest.yaml                 # 应用配置
kubectl delete pod <name> -n <ns>              # 删除 Pod
kubectl rollout status deployment/<name>       # 滚动更新状态
kubectl scale deployment/<name> --replicas=3   # 扩容
kubectl port-forward svc/<name> 8080:80        # 端口转发
kubectl get events -A --sort-by=.metadata.creationTimestamp  # 事件
```

### Linux 最常用

```bash
systemctl status <service>                     # 服务状态
journalctl -u <service> --since="10 min ago"   # 服务日志
top -c                                         # 进程资源
df -h                                          # 磁盘使用
free -h                                        # 内存使用
ss -tlnp                                       # 监听端口
ip addr show                                   # 网络接口
curl -s http://localhost:8080/health           # 健康检查
tail -f /var/log/syslog                        # 实时日志
find / -name "*.log" -size +100M              # 大文件
```

### Docker 最常用

```bash
docker ps -a                                   # 所有容器
docker logs <container> --tail=100 -f          # 容器日志
docker exec -it <container> /bin/sh            # 进入容器
docker build -t myapp:v1 .                     # 构建镜像
docker run -d -p 8080:80 myapp:v1              # 运行容器
docker system df                               # 磁盘使用
docker system prune -a                         # 清理资源
docker images                                  # 镜像列表
docker network ls                              # 网络列表
docker stats                                   # 实时资源
```

## 故障排查快速流程

### Pod 异常状态速查

| 状态 | 可能原因 | 快速排查 |
|------|----------|----------|
| Pending | 资源不足/调度失败 | `kubectl describe pod` 查看 Events |
| CrashLoopBackOff | 应用崩溃/探针失败 | `kubectl logs <pod> --previous` |
| ImagePullBackOff | 镜像拉取失败 | 检查 imagePullSecrets、网络 |
| OOMKilled | 内存超限 | 增加 memory limit |
| Evicted | 节点资源压力 | `kubectl describe node` |
| Terminating | Finalizer/优雅关闭 | 检查 finalizers、preStop |
| CreateContainerConfigError | ConfigMap/Secret 缺失 | 检查引用的配置对象 |
| ErrImageNeverPull | imagePullPolicy=Never 但镜像不存在 | 加载镜像或改策略 |

### 节点异常状态速查

| 状态 | 可能原因 | 快速排查 |
|------|----------|----------|
| NotReady | kubelet 崩溃/网络断开 | `journalctl -u kubelet` |
| DiskPressure | 磁盘空间不足 | `df -h`、清理镜像 |
| MemoryPressure | 内存不足 | `free -h`、检查 Pod 内存 |
| PIDPressure | PID 耗尽 | `ps aux \| wc -l` |
| NetworkUnavailable | CNI 未就绪 | 检查 CNI DaemonSet |

### 服务访问故障速查

```
1. Pod 是否 Running?        → kubectl get pods
2. Endpoints 是否存在?      → kubectl get endpoints <svc>
3. Service 选择器是否匹配?  → kubectl get svc <name> -o yaml
4. Pod 端口是否正确?        → kubectl describe pod
5. NetworkPolicy 是否拦截?  → kubectl get networkpolicy
6. DNS 是否解析?            → nslookup <svc>.<ns>.svc.cluster.local
7. 从 Pod 内测试?           → kubectl exec -it <pod> -- curl <svc>:<port>
```

## 资源请求与限制速查

```yaml
# 推荐配置模板
resources:
  requests:
    cpu: "100m"      # 保证分配
    memory: "128Mi"  # 保证分配
  limits:
    cpu: "500m"      # 最大可用
    memory: "512Mi"  # 超过则 OOMKill
```

| 场景 | CPU Request | CPU Limit | Mem Request | Mem Limit |
|------|-------------|-----------|-------------|----------|
| 轻量 API | 50m | 200m | 64Mi | 256Mi |
| 普通 Web | 100m | 500m | 128Mi | 512Mi |
| 计算密集 | 500m | 2000m | 256Mi | 1Gi |
| 数据库 | 1000m | 4000m | 1Gi | 4Gi |
| AI 推理 | 2000m | 8000m | 4Gi | 16Gi |

## 常用端口速查

| 端口 | 服务 | 用途 |
|------|------|------|
| 6443 | kube-apiserver | API 访问 |
| 2379-2380 | etcd | 集群存储 |
| 10250 | kubelet | 节点 API |
| 10257 | kube-controller-manager | 控制器 |
| 10259 | kube-scheduler | 调度器 |
| 53 | CoreDNS | DNS 服务 |
| 80/443 | Ingress/Gateway | HTTP/HTTPS |
| 9090 | Prometheus | 监控 |
| 3000 | Grafana | 仪表盘 |
| 8080 | 应用默认 | 业务服务 |
| 5432 | PostgreSQL | 数据库 |
| 3306 | MySQL | 数据库 |
| 6379 | Redis | 缓存 |
| 9092 | Kafka | 消息队列 |
| 2375/2376 | Docker | 容器引擎 |

## 常用环境变量速查

| 变量 | 用途 | 示例 |
|------|------|------|
| KUBECONFIG | kubeconfig 路径 | ~/.kube/config |
| KUBECTL_EXTERNAL_DIFF | diff 工具 | difft |
| DOCKER_HOST | Docker 守护进程 | tcp://host:2376 |
| REGISTRY_AUTH_FILE | 镜像仓库认证 | ~/.docker/config.json |
| HELM_CACHE_HOME | Helm 缓存 | ~/.cache/helm |
| GOMAXPROCS | Go 并发数 | 4 |
| TZ | 时区 | Asia/Shanghai |

## 学习路径

```
入门: kubectl 场景速查 → Docker → Git
中级: K8s 全资源 → Helm → Linux → 网络
高级: PromQL → GitOps → Gateway API → TLS/PKI
专家: bpftrace/eBPF → Go → 性能调优
```

## Git 常用命令速查

```bash
# 基础操作
git status
git add .
git commit -m "feat: add new feature"
git push origin main
git pull --rebase origin main

# 分支管理
git checkout -b feature/new-feature
git merge --no-ff feature/new-feature
git rebase main
git cherry-pick <commit-hash>

# 查看历史
git log --oneline --graph -20
git diff HEAD~1
git show <commit-hash>
git blame <file>

# 撤销操作
git reset --soft HEAD~1     # 保留修改
git reset --hard HEAD~1     # 🔴 丢弃修改
git revert <commit-hash>    # 安全撤销
git stash && git stash pop  # 暂存/恢复

# 远程操作
git remote -v
git fetch --all --prune
git tag -a v1.0.0 -m "Release 1.0.0"
git push origin --tags
```

## Helm 常用命令速查

```bash
# 仓库管理
helm repo add bitnami https://charts.bitnami.com/bitnami
helm repo update
helm search repo nginx
helm search hub wordpress

# 安装/升级
helm install myapp bitnami/nginx -n production --create-namespace
helm upgrade myapp bitnami/nginx -f values-prod.yaml -n production
helm upgrade --install myapp ./chart  # 幂等安装
helm uninstall myapp -n production

# 查看/调试
helm list -A
helm status myapp -n production
helm history myapp -n production
helm get values myapp -n production
helm template myapp ./chart --debug

# 回滚
helm rollback myapp 2 -n production

# Chart 开发
helm create mychart
helm lint ./mychart
helm package ./mychart
helm show values bitnami/redis
```

## PromQL 常用查询速查

```promql
# CPU 使用率
rate(container_cpu_usage_seconds_total{namespace="prod"}[5m]) * 100

# 内存使用率
container_memory_working_set_bytes / container_spec_memory_limit_bytes * 100

# Pod 重启次数
increase(kube_pod_container_status_restarts_total[1h])

# API Server 延迟
histogram_quantile(0.99, rate(apiserver_request_duration_seconds_bucket[5m]))

# 节点状态
kube_node_status_condition{condition="Ready",status="true"}

# 磁盘使用率
(node_filesystem_size_bytes - node_filesystem_avail_bytes) / node_filesystem_size_bytes * 100

# Pod 就绪状态
kube_pod_status_ready{condition="true",namespace="prod"}

# Deployment 副本数
kube_deployment_status_replicas_available / kube_deployment_spec_replicas
```

## 网络排查命令速查

```bash
# 连通性测试
curl -sv http://service:port/health
wget -qO- http://service:port/
nc -zv host port
telnet host port

# DNS 解析
nslookup service.namespace.svc.cluster.local
dig @10.96.0.10 service.namespace.svc.cluster.local
host service.namespace.svc.cluster.local

# 路由与接口
ip route show
ip addr show
ip neigh show  # ARP 表
traceroute target-host

# 抓包
tcpdump -i eth0 -nn port 8080 -w capture.pcap
tcpdump -i any host 10.0.0.1 and port 443

# 连接状态
ss -tlnp          # 监听端口
ss -s             # 连接统计
netstat -an | grep ESTABLISHED | wc -l

# 防火墙
iptables -L -n -v
iptables -t nat -L -n
nft list ruleset
```

## 常用 YAML 模板

### Deployment 模板

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
  labels:
    app: myapp
spec:
  replicas: 3
  selector:
    matchLabels:
      app: myapp
  template:
    metadata:
      labels:
        app: myapp
    spec:
      containers:
        - name: myapp
          image: registry/myapp:v1.0.0
          ports:
            - containerPort: 8080
          resources:
            requests:
              cpu: 100m
              memory: 128Mi
            limits:
              cpu: 500m
              memory: 512Mi
          livenessProbe:
            httpGet:
              path: /health
              port: 8080
            initialDelaySeconds: 10
            periodSeconds: 10
          readinessProbe:
            httpGet:
              path: /ready
              port: 8080
            initialDelaySeconds: 5
            periodSeconds: 5
```

### Service 模板

```yaml
apiVersion: v1
kind: Service
metadata:
  name: myapp
spec:
  selector:
    app: myapp
  ports:
    - port: 80
      targetPort: 8080
      protocol: TCP
  type: ClusterIP
```

### ConfigMap 模板

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: myapp-config
data:
  APP_ENV: "production"
  LOG_LEVEL: "info"
  config.yaml: |
    server:
      port: 8080
      timeout: 30s
```

## 单位换算速查

| K8s 单位 | 等价 | 说明 |
|----------|------|------|
| 1 CPU | 1000m | 1 核 CPU |
| 100m | 0.1 CPU | 十分之一核 |
| 1Gi | 1024Mi | 二进制 |
| 1G | 1000M | 十进制 |
| 1Mi | 1024Ki | 二进制 |
| 1M | 1000K | 十进制 |

> ❗ K8s 中建议用 Mi/Gi（二进制），避免混淆。

## 常用工具安装速查

```bash
# kubectl
curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl"
chmod +x kubectl && sudo mv kubectl /usr/local/bin/

# helm
curl https://raw.githubusercontent.com/helm/helm/main/scripts/get-helm-3 | bash

# kind
go install sigs.k8s.io/kind@latest
# 或
curl -Lo ./kind https://kind.sigs.k8s.io/dl/latest/kind-linux-amd64

# k9s
curl -sS https://webi.sh/k9s | sh

# stern
go install github.com/stern/stern@latest

# kubectx/kubens
git clone https://github.com/ahmetb/kubectx /opt/kubectx
ln -s /opt/kubectx/kubectx /usr/local/bin/kubectx
ln -s /opt/kubectx/kubens /usr/local/bin/kubens

# trivy
curl -sfL https://raw.githubusercontent.com/aquasecurity/trivy/main/contrib/install.sh | sh -s -- -b /usr/local/bin

# kustomize
curl -s "https://raw.githubusercontent.com/kubernetes-sigs/kustomize/master/hack/install_kustomize.sh" | bash
```

## 常见错误代码速查

| 错误 | 含义 | 解决 |
|------|------|------|
| 401 Unauthorized | 认证失败 | 检查 kubeconfig/token |
| 403 Forbidden | 权限不足 | 检查 RBAC |
| 404 Not Found | 资源不存在 | 检查名称/命名空间 |
| 409 Conflict | 资源冲突 | 重新获取后重试 |
| 422 Unprocessable | 验证失败 | 检查 YAML 格式 |
| 429 Too Many Requests | 限流 | 减少请求频率 |
| 500 Internal Error | 服务端错误 | 检查 API Server 日志 |
| 503 Service Unavailable | 服务不可用 | 检查 etcd/API Server |
| context deadline exceeded | 超时 | 检查网络/负载 |
| connection refused | 连接拒绝 | 检查服务是否运行 |

## 快捷别名配置

```bash
# ~/.bashrc 或 ~/.zshrc
alias k='kubectl'
alias kgp='kubectl get pods'
alias kgs='kubectl get svc'
alias kgd='kubectl get deployment'
alias kdp='kubectl describe pod'
alias kl='kubectl logs -f'
alias ke='kubectl exec -it'
alias ka='kubectl apply -f'
alias kdel='kubectl delete'
alias kns='kubectl config set-context --current --namespace'
alias kctx='kubectl config use-context'

# 自动补全
source <(kubectl completion bash)
complete -o default -F __start_kubectl k
```

## 参考链接

- https://kubernetes.io/docs/reference/kubectl/quick-reference/
- https://cheatsheet.dennyzhang.com/
- https://www.docker.com/wp-content/uploads/2022/03/docker-cheat-sheet.pdf
- https://helm.sh/docs/helm/
- https://prometheus.io/docs/prometheus/latest/querying/basics/
- https://git-scm.com/docs
- https://www.brendangregg.com/BPF/bpftrace-cheat-sheet.html
- https://kubernetes.io/docs/reference/kubectl/generated/

## Related

- [[系统基础/知识字典/index.md|知识字典总索引]]
- [[系统基础/K8s事件/index.md|K8s 事件]]
- [[系统基础/Linux/index.md|Linux 知识]]

<!-- risk-assessed -->
