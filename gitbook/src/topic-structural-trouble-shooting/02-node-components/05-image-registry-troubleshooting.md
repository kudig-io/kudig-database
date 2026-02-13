# 镜像与镜像仓库故障排查指南

> **适用版本**: Kubernetes v1.25 - v1.32, containerd v1.6+ | **最后更新**: 2026-01 | **难度**: 中级-高级
>
> **版本说明**:
> - v1.27+ imagePullPolicy IfNotPresent 一致性改进
> - v1.30+ 支持 OCI artifacts 镜像
> - containerd v1.7+ 支持 registry.config_path 多镜像源

## 🎯 本文档价值

| 读者对象 | 价值体现 |
| :--- | :--- |
| **初学者** | 搞清楚 `ImagePullBackOff` 的底层触发链路，学会正确配置 `imagePullSecrets`，掌握镜像拉取策略（Always vs IfNotPresent）的差异及其对生产环境的影响。 |
| **资深专家** | 深入理解 containerd 的镜像分层存储架构、多架构镜像（Manifest List）的解析机制、大规模拉取时的 P2P 加速与本地缓存治理，以及镜像签名校验（Cosign）的安全加固方案。 |

---

## 0. 10 分钟快速诊断

1. **快速定位失败**：在故障 Pod 上 `kubectl describe pod <name> | grep -A2 -E "Image|ErrImage|BackOff|429|unauthorized"`，记录错误码（DNS/TLS/401/429/空间）。
2. **连通性与 TLS**：`nslookup <registry>`、`curl -Iv https://<registry>/v2/`，若证书错误检查 CA/中间证书；云私有域注意 443/5000 安全组。
3. **认证与凭据**：`crictl pull <image> --creds user:pass` 验证，检查 `imagePullSecrets`、SA 绑定；`cat ~/.docker/config.json` 或 `/etc/containerd/config.toml` registry 配置。
4. **速率与并发**：观察 `toomanyrequests`/`rate limit exceeded`，临时切换私有镜像缓存/镜像加速器，或降低批量创建并开启预拉取。
5. **磁盘与缓存**：`df -h /var/lib/containerd`、`df -i`，空间/ inode 不足会导致拉取中断；必要时 `crictl rmi --prune` 清理未用镜像。
6. **镜像一致性**：确认是否使用 Digest（SHA256）而非可变 Tag；比对多架构 Manifest，避免架构不匹配导致 `exec format error`。
7. **证据留存**：保存 describe 输出、crictl pull 错误、TLS/CA 信息、registry 配置与监控（拉取时延/429 次数），便于复盘。

---

## 1. 核心原理解析：镜像分发链路

### 1.1 镜像的分层与解压逻辑

镜像不是一个大文件，而是由多个 `Layer`（层）组成的。containerd 在拉取时：
1. **并发拉取**：根据 `max_concurrent_downloads` 并发下载各层。
2. **内容寻址存储 (CAS)**：使用 SHA256 校验每一层，确保内容完整性。
3. **Snapshotter 挂载**：将下载的层解压，并通过 `overlayfs` 等快照插件挂载为容器的 `rootfs`。

### 1.2 生产环境典型“镜像坑”

1. **Tag 覆盖导致的一致性问题**：
   - **现象**：`imagePullPolicy: IfNotPresent` 且使用了固定标签（如 `v1.0`），但镜像仓库中的 `v1.0` 镜像被重新推送了。部分节点由于已有旧镜像，导致集群内 Pod 运行的代码版本不一致。
   - **对策**：**生产环境严禁重新推送同名 Tag**，必须通过增加版本号（如 `v1.0.1`）或使用镜像 `Digest`（SHA256 哈希）来确保唯一性。
2. **磁盘 IOPS 饱和导致的拉取超时**：
   - **现象**：在大规模扩容时，大量 Pod 同时拉取数百 MB 的镜像，导致节点磁盘 IOPS 爆表，从而触发 kubelet 的 CRI 响应超时。
   - **对策**：配置镜像预热（Image Pre-pulling）或使用 Dragonfly/Kraken 等 P2P 分发工具。

# 专家级观测工具链（Expert's Toolbox）

```bash
# 专家级：查看镜像各层的详细 Digest 和本地存储路径
crictl inspecti <image-id> | jq '.status.size, .status.repoDigests'

# 专家级：监控镜像拉取的内部指标
# containerd 暴露的镜像操作延迟
curl -s localhost:13387/metrics | grep containerd_image_pull_duration

# 专家级：手动验证镜像层完整性
ctr -n k8s.io images check
```

---

## 目录

1. [镜像分发链路逻辑](#1-核心原理解析镜像分发链路)
2. [专家观测工具链](#专家级观测工具链experts-toolbox)
3. [故障现象与拉取策略解析](#12-常见问题现象)
4. [基础排查步骤（初学者）](#22-排查命令集)
5. [深度治理方案](#第三部分解决方案与风险控制)

---

## 第一部分：问题现象与影响分析

### 1.1 镜像拉取流程

```
┌─────────────────────────────────────────────────────────────────┐
│                      镜像拉取流程                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌──────────────┐                                               │
│  │   kubelet    │                                               │
│  └──────┬───────┘                                               │
│         │                                                        │
│         ▼                                                        │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              Container Runtime (containerd/CRI-O)         │   │
│  └──────────────────────────────────────────────────────────┘   │
│         │                                                        │
│         ├─── 1. 解析镜像名称 (registry/repo:tag)                │
│         │                                                        │
│         ├─── 2. 检查本地镜像缓存                                │
│         │         │                                              │
│         │         └─► 存在且符合拉取策略 → 使用本地镜像          │
│         │                                                        │
│         ├─── 3. 获取认证凭据                                    │
│         │         │                                              │
│         │         ├─► imagePullSecrets                          │
│         │         ├─► ServiceAccount 关联的 Secrets             │
│         │         └─► 节点配置的凭据 (/root/.docker/config.json) │
│         │                                                        │
│         ├─── 4. 连接镜像仓库                                    │
│         │         │                                              │
│         │         ├─► DNS 解析                                  │
│         │         ├─► TLS 握手                                  │
│         │         └─► 认证验证                                  │
│         │                                                        │
│         └─── 5. 拉取镜像层                                      │
│                   │                                              │
│                   └─► 存储到本地                                │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 1.2 常见问题现象

| 问题类型 | 现象描述 | 错误信息示例 | 查看方式 |
|---------|---------|-------------|---------|
| 镜像不存在 | 拉取失败 | `manifest unknown` / `not found` | `kubectl describe pod` |
| 认证失败 | 拉取被拒绝 | `unauthorized` / `authentication required` | `kubectl describe pod` |
| 网络问题 | 连接失败 | `connection refused` / `timeout` | `kubectl describe pod` |
| TLS 证书问题 | 握手失败 | `x509: certificate signed by unknown authority` | `kubectl describe pod` |
| 拉取策略问题 | 未使用最新镜像 | 无错误，但镜像版本不对 | 检查 imagePullPolicy |
| 磁盘空间不足 | 拉取中断 | `no space left on device` | 节点磁盘检查 |
| 速率限制 | 拉取被限制 | `toomanyrequests` / `rate limit exceeded` | `kubectl describe pod` |
| 镜像格式错误 | 运行失败 | `exec format error` | Pod 日志 |

### 1.3 ImagePullPolicy 说明

| 策略 | 行为 | 适用场景 |
|-----|-----|---------|
| Always | 每次都尝试拉取最新镜像 | 使用 latest 标签、需要确保最新 |
| IfNotPresent | 本地不存在时才拉取 | 使用固定版本标签 (推荐) |
| Never | 永不拉取，只使用本地镜像 | 预加载镜像、离线环境 |

**默认行为：**
- 使用 `:latest` 标签或无标签：默认 `Always`
- 使用其他标签：默认 `IfNotPresent`

### 1.4 影响分析

| 故障类型 | 直接影响 | 间接影响 | 影响范围 |
|---------|---------|---------|---------|
| 镜像拉取失败 | Pod 无法启动 | 服务不可用 | 使用该镜像的所有 Pod |
| 认证失败 | 私有镜像无法访问 | 依赖该镜像的服务中断 | 私有仓库的所有镜像 |
| 仓库不可用 | 新 Pod 无法启动 | 扩容失败、故障恢复受阻 | 依赖该仓库的所有服务 |
| 速率限制 | 拉取延迟或失败 | 部署变慢 | 公共仓库镜像 |

---

## 第二部分：排查方法（基础与进阶）

### 2.1 排查决策树

```
镜像拉取故障
      │
      ├─── ImagePullBackOff / ErrImagePull？
      │         │
      │         ├─ "not found" / "manifest unknown"
      │         │       └─► 检查镜像名称、标签是否正确
      │         │
      │         ├─ "unauthorized" / "authentication required"
      │         │       └─► 检查 imagePullSecrets 配置
      │         │
      │         ├─ "connection refused" / "timeout"
      │         │       └─► 检查网络连通性、DNS、防火墙
      │         │
      │         ├─ "x509: certificate" 错误
      │         │       └─► 检查 TLS 证书配置
      │         │
      │         └─ "toomanyrequests"
      │                 └─► 速率限制，使用镜像代理或认证
      │
      ├─── 镜像版本不对？
      │         │
      │         └─► 检查 imagePullPolicy 和标签
      │
      └─── 架构不匹配？
                │
                └─► 检查镜像支持的平台 (amd64/arm64)
```

### 2.2 排查命令集

#### 2.2.1 Pod 镜像状态检查

```bash
# 查看 Pod 事件
kubectl describe pod <pod-name> -n <namespace>

# 查看镜像拉取状态
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[*].state}'

# 查看 Pod 使用的镜像
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].image}'

# 查看 imagePullPolicy
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].imagePullPolicy}'

# 查看 imagePullSecrets
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.imagePullSecrets}'
```

#### 2.2.2 镜像仓库连通性检查

```bash
# 在节点上测试仓库连通性
# 方式 1: 使用 curl
curl -v https://registry.example.com/v2/

# 方式 2: 使用 crictl (containerd)
crictl pull <image>

# 方式 3: 从 Pod 内测试
kubectl run test-registry --rm -it --image=curlimages/curl --restart=Never -- \
  curl -v https://registry.example.com/v2/

# 检查 DNS 解析
nslookup registry.example.com
dig registry.example.com
```

#### 2.2.3 认证配置检查

```bash
# 查看 imagePullSecret 内容
kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | jq

# 验证 Secret 格式
kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.type}'
# 应该是: kubernetes.io/dockerconfigjson

# 检查 ServiceAccount 的 imagePullSecrets
kubectl get sa <sa-name> -n <namespace> -o jsonpath='{.imagePullSecrets}'

# 查看节点上的 Docker 配置
cat /root/.docker/config.json
# 或 containerd
cat /etc/containerd/config.toml | grep -A10 registry
```

#### 2.2.4 节点镜像缓存检查

```bash
# 使用 crictl (containerd/CRI-O)
crictl images
crictl images | grep <image-name>

# 检查镜像详情
crictl inspecti <image-id>

# 清理未使用的镜像
crictl rmi --prune

# 检查磁盘空间
df -h /var/lib/containerd
```

### 2.3 排查注意事项

| 注意事项 | 说明 |
|---------|-----|
| 镜像名称格式 | `registry/namespace/image:tag`，省略 registry 默认 docker.io |
| 私有仓库 | 需要配置 imagePullSecrets 或节点级凭据 |
| 速率限制 | Docker Hub 匿名 100 次/6小时，认证 200 次/6小时 |
| 镜像架构 | 多架构镜像需要确认支持目标平台 |
| 缓存行为 | IfNotPresent 策略可能使用过期的本地镜像 |

---

## 第三部分：解决方案与风险控制

### 3.1 认证问题

#### 场景 1：创建 imagePullSecret

```bash
# 方式 1: 从命令行创建
kubectl create secret docker-registry my-registry-secret \
  --docker-server=registry.example.com \
  --docker-username=myuser \
  --docker-password=mypassword \
  --docker-email=user@example.com \
  -n <namespace>

# 方式 2: 从 Docker 配置文件创建
kubectl create secret generic my-registry-secret \
  --from-file=.dockerconfigjson=$HOME/.docker/config.json \
  --type=kubernetes.io/dockerconfigjson \
  -n <namespace>

# 方式 3: 使用 YAML
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Secret
metadata:
  name: my-registry-secret
  namespace: <namespace>
type: kubernetes.io/dockerconfigjson
data:
  .dockerconfigjson: $(echo -n '{"auths":{"registry.example.com":{"username":"myuser","password":"mypassword","auth":"'$(echo -n 'myuser:mypassword' | base64)'"}}}' | base64 -w0)
EOF
```

#### 场景 2：在 Pod 中使用 imagePullSecret

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: myapp
spec:
  containers:
  - name: app
    image: registry.example.com/myapp:v1
  imagePullSecrets:
  - name: my-registry-secret
```

#### 场景 3：为 ServiceAccount 配置默认 imagePullSecret

```bash
# 方式 1: 使用 kubectl patch
kubectl patch serviceaccount default -n <namespace> \
  -p '{"imagePullSecrets": [{"name": "my-registry-secret"}]}'

# 方式 2: 使用 YAML
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: ServiceAccount
metadata:
  name: default
  namespace: <namespace>
imagePullSecrets:
- name: my-registry-secret
EOF
```

### 3.2 网络和 TLS 问题

#### 场景 1：私有仓库使用自签名证书

```bash
# 方式 1: 配置 containerd 信任证书
# 在所有节点执行

# 创建证书目录
mkdir -p /etc/containerd/certs.d/registry.example.com

# 添加 CA 证书
cat > /etc/containerd/certs.d/registry.example.com/hosts.toml << EOF
server = "https://registry.example.com"

[host."https://registry.example.com"]
  ca = "/etc/containerd/certs.d/registry.example.com/ca.crt"
EOF

# 复制 CA 证书
cp ca.crt /etc/containerd/certs.d/registry.example.com/

# 重启 containerd
systemctl restart containerd

# 方式 2: 配置跳过 TLS 验证 (不推荐生产环境)
cat > /etc/containerd/certs.d/registry.example.com/hosts.toml << EOF
server = "https://registry.example.com"

[host."https://registry.example.com"]
  skip_verify = true
EOF
```

#### 场景 2：配置镜像仓库代理/镜像

```bash
# containerd 配置镜像加速
cat > /etc/containerd/certs.d/docker.io/hosts.toml << EOF
server = "https://docker.io"

[host."https://mirror.example.com"]
  capabilities = ["pull", "resolve"]

[host."https://registry-1.docker.io"]
  capabilities = ["pull", "resolve"]
EOF

# 重启 containerd
systemctl restart containerd

# 验证配置
crictl pull docker.io/library/nginx:latest
```

### 3.3 速率限制问题

#### 场景 1：Docker Hub 速率限制

**问题现象：**
```
toomanyrequests: You have reached your pull rate limit
```

**解决方案：**

```bash
# 方案 1: 使用认证账户 (提升限额)
kubectl create secret docker-registry dockerhub-secret \
  --docker-server=https://index.docker.io/v1/ \
  --docker-username=<username> \
  --docker-password=<password> \
  -n <namespace>

# 方案 2: 使用镜像代理/缓存
# 部署 Harbor 或其他镜像代理

# 方案 3: 将常用镜像同步到私有仓库
skopeo copy docker://nginx:latest docker://registry.example.com/library/nginx:latest

# 方案 4: 预拉取镜像到节点
# 在 DaemonSet 中预拉取
```

### 3.4 镜像版本和标签问题

#### 场景 1：确保使用最新镜像

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
spec:
  template:
    spec:
      containers:
      - name: app
        image: myapp:v1.2.3  # 使用具体版本标签
        imagePullPolicy: Always  # 强制每次拉取
```

#### 场景 2：强制更新镜像

```bash
# 方式 1: 删除 Pod 让其重建
kubectl delete pod <pod-name> -n <namespace>

# 方式 2: 更新 Deployment 触发滚动更新
kubectl rollout restart deployment <name> -n <namespace>

# 方式 3: 更新镜像触发更新
kubectl set image deployment/<name> <container>=<new-image>

# 方式 4: 在节点上删除本地镜像缓存
crictl rmi <image>
```

### 3.5 镜像架构问题

#### 场景 1：架构不匹配

**问题现象：**
```
exec format error
```

**解决步骤：**

```bash
# 1. 检查节点架构
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.architecture}{"\n"}{end}'

# 2. 检查镜像支持的架构
# 使用 skopeo
skopeo inspect --raw docker://nginx:latest | jq '.manifests[].platform'

# 使用 docker manifest
docker manifest inspect nginx:latest

# 3. 使用多架构镜像
# 或为特定架构构建镜像
# docker build --platform linux/amd64 -t myapp:v1-amd64 .
# docker build --platform linux/arm64 -t myapp:v1-arm64 .

# 4. 使用 nodeSelector 确保调度到正确架构的节点
# spec:
#   nodeSelector:
#     kubernetes.io/arch: amd64
```

### 3.6 磁盘空间问题

#### 场景 1：节点磁盘空间不足

```bash
# 1. 检查磁盘使用
df -h /var/lib/containerd

# 2. 清理未使用的镜像
crictl rmi --prune

# 3. 清理已停止的容器
crictl rm $(crictl ps -a -q --state exited)

# 4. 配置镜像垃圾回收
# kubelet 配置
# imageGCHighThresholdPercent: 85
# imageGCLowThresholdPercent: 80

# 5. 设置镜像大小限制
# 在 LimitRange 中设置
```

### 3.7 完整配置示例

```yaml
# 1. 创建 imagePullSecret
apiVersion: v1
kind: Secret
metadata:
  name: registry-credentials
  namespace: production
type: kubernetes.io/dockerconfigjson
stringData:
  .dockerconfigjson: |
    {
      "auths": {
        "registry.example.com": {
          "username": "myuser",
          "password": "mypassword"
        },
        "https://index.docker.io/v1/": {
          "username": "dockerhub-user",
          "password": "dockerhub-token"
        }
      }
    }
---
# 2. 配置 ServiceAccount
apiVersion: v1
kind: ServiceAccount
metadata:
  name: app-sa
  namespace: production
imagePullSecrets:
- name: registry-credentials
---
# 3. 使用 ServiceAccount 的 Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: myapp
  namespace: production
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
      serviceAccountName: app-sa
      containers:
      - name: app
        image: registry.example.com/myapp:v1.2.3
        imagePullPolicy: IfNotPresent
        resources:
          requests:
            cpu: 100m
            memory: 128Mi
          limits:
            cpu: 500m
            memory: 512Mi
```

---

### 3.8 镜像仓库最佳实践

```bash
# 1. 使用私有镜像仓库
# - Harbor: 企业级镜像仓库
# - Nexus: 多格式仓库
# - AWS ECR / GCR / ACR: 云厂商托管

# 2. 镜像命名规范
# registry.example.com/project/app:version
# 避免使用 latest 标签

# 3. 镜像扫描
# 集成 Trivy/Clair 进行漏洞扫描

# 4. 镜像签名
# 使用 cosign 签名验证

# 5. 预热镜像
# 使用 DaemonSet 预拉取常用镜像到所有节点
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: image-puller
spec:
  selector:
    matchLabels:
      app: image-puller
  template:
    metadata:
      labels:
        app: image-puller
    spec:
      containers:
      - name: pause
        image: registry.example.com/pause:3.9
        resources:
          requests:
            cpu: 1m
            memory: 1Mi
      initContainers:
      - name: pull-images
        image: registry.example.com/myapp:v1.2.3
        command: ["echo", "Image pulled"]
```

---

### 3.9 安全生产风险提示

| 操作 | 风险等级 | 风险说明 | 建议 |
|-----|---------|---------|-----|
| 使用 latest 标签 | 中 | 版本不可控，可能引入破坏性变更 | 使用具体版本标签 |
| 跳过 TLS 验证 | 高 | 中间人攻击风险 | 配置正确的 CA 证书 |
| 明文存储凭据 | 高 | 凭据泄露风险 | 使用 Secret 管理，限制访问 |
| 清理镜像缓存 | 低 | 下次拉取需要时间 | 低峰期执行 |
| 修改 containerd 配置 | 中 | 需要重启服务 | 逐节点滚动更新 |

---

## 附录

### 常用命令速查

```bash
# Pod 镜像检查
kubectl describe pod <pod>
kubectl get pod <pod> -o jsonpath='{.spec.containers[*].image}'
kubectl get pod <pod> -o jsonpath='{.status.containerStatuses[*].imageID}'

# Secret 管理
kubectl create secret docker-registry <name> --docker-server=<server> --docker-username=<user> --docker-password=<pass>
kubectl get secret <name> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d

# 节点镜像操作
crictl images
crictl pull <image>
crictl rmi <image>
crictl rmi --prune

# 调试
kubectl run debug --rm -it --image=curlimages/curl --restart=Never -- sh
```

### 相关文档

- [kubelet 故障排查](./01-kubelet-troubleshooting.md)
- [容器运行时故障排查](./03-container-runtime-troubleshooting.md)
- [ConfigMap/Secret 故障排查](../05-workloads/06-configmap-secret-troubleshooting.md)
- [Pod 故障排查](../05-workloads/01-pod-troubleshooting.md)
