---
title: 容器镜像（Images）
description: '## 概述'
category: dictionary
tags:
- k8s
- glossary
- terminology
- kubelet
- docker
- opa
- daemonset
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 容器镜像（Images） 是什么
- 如何 容器镜像（Images）
trigger_keywords:
- 容器镜像
- Images
- dictionary
prerequisites:
- kubectl-basics
- pod-lifecycle
- cloud-provider-basics
- policy-basics
created: "2026-05-23"
---

# 容器镜像（Images）

## 概述

容器镜像（Container Image）是封装了应用程序及其所有软件依赖项的二进制数据，是一个可独立运行的可执行软件包，并对其运行时环境做出非常明确的假设。在 [[entities/kubernetes|[[Kubernetes|kubernetes]]]] 中，通常需要先创建应用程序的容器镜像并推送到镜像仓库，然后在 Pod 中引用该镜像。

## 核心概念/原理

### 镜像名称格式

容器镜像通常包含以下组成部分：

- **镜像名**：如 `pause`、`example/mycontainer`
- **仓库主机名**：如 `fictional.registry.example/imagename`，可包含端口号 `fictional.registry.example:10443/imagename`
- **标签（Tag）**：用于标识同一镜像系列的不同版本，如 `:v1.42.0`。若未指定，Kubernetes 默认使用 `latest`
- **摘要（Digest）**：镜像内容的唯一不可变标识符，如 `@sha256:1ff6c18fbef2045af6b9c16bf034cc421a29027b800e4f9b68ae9b1cb3e9ae07`

> **注意**：如果未指定仓库主机名，Kubernetes 默认使用 Docker 公共仓库（`docker.io`）。

### 镜像拉取策略（imagePullPolicy）

`imagePullPolicy` 控制 [[kubelet|kubelet]] 何时尝试拉取镜像：

| 策略值 | 说明 |
|--------|------|
| `IfNotPresent` | 仅当本地不存在该镜像时才拉取（默认值，除 `:latest` 或无标签外） |
| `Always` | 每次启动容器时，kubelet 都会查询镜像仓库解析镜像摘要，若本地缓存的摘要匹配则使用缓存，否则重新拉取 |
| `Never` | kubelet 不尝试拉取镜像，仅使用本地已存在的镜像，否则启动失败 |

**默认策略规则**：
- 省略 `imagePullPolicy` 且指定了 digest → `IfNotPresent`
- 省略 `imagePullPolicy` 且标签为 `:latest` → `Always`
- 省略 `imagePullPolicy` 且未指定标签 → `Always`
- 省略 `imagePullPolicy` 且指定了非 `latest` 标签 → `IfNotPresent`

### 镜像拉取方式

- **串行拉取**（默认）：kubelet 每次只向镜像服务发送一个拉取请求
- **并行拉取**：在 kubelet 配置中将 `serializeImagePulls` 设为 `false`，可同时进行多个镜像拉取。从 Kubernetes v1.35 起，可通过 `maxParallelImagePulls` 限制最大并行拉取数量

### 私有镜像仓库认证

访问私有仓库的几种方式：

1. **`imagePullSecrets`**（推荐）：在 Pod 中指定 Secret 来提供仓库凭证
2. **节点级配置**：在所有节点上配置 `.docker/config.json` 或容器运行时的认证配置
3. **kubelet 凭证提供插件**：kubelet 调用外部插件动态获取私有仓库凭证
4. **预拉取镜像**：在节点上提前拉取镜像，配合 `imagePullPolicy: IfNotPresent` 或 `Never` 使用

### 多架构镜像

容器仓库可以提供镜像索引（image index），指向多个架构特定的镜像清单。Kubernetes 官方镜像通常以 `-$(ARCH)` 后缀命名，同时也提供多架构镜像（如 `pause` 包含所有支持架构的清单）。

### 镜像拉取凭证验证（KubeletEnsureSecretPulledImages）

从 Kubernetes v1.35 [beta] 起，启用 `KubeletEnsureSecretPulledImages` 特性门控后，即使镜像已存在于节点上，Kubernetes 也会验证镜像拉取凭证，防止未授权 Pod 使用预拉取的镜像。可通过 `imagePullCredentialsVerificationPolicy` 配置验证策略：

- `NeverVerify`：禁用验证
- `NeverVerifyPreloadedImages`：不验证 kubelet 外预拉的镜像（默认）
- `NeverVerifyAllowListedImages`：白名单中的预拉镜像不验证
- `AlwaysVerify`：所有镜像都验证

## 关键机制或特性

- `ImagePullBackOff`：当镜像拉取失败时，容器会进入 Waiting 状态，Kubernetes 会以递增的退避延迟重试，最大延迟为 300 秒
- 按 RuntimeClass 拉取镜像（v1.29 [alpha]）：启用 `RuntimeClassInImageCriApi` 后，kubelet 以（镜像名, runtime handler）元组引用镜像，适用于 VM 类容器（如 Windows Hyper-V）
- 始终强制拉取：设置 `imagePullPolicy: Always`、使用 `:latest` 标签，或启用 `AlwaysPullImages` 准入控制器

## 使用场景

| 场景 | 建议方案 |
|------|----------|
| 仅使用开源/公共镜像 | 直接使用公共镜像仓库，无需额外配置 |
| 需要隐藏专有二进制镜像，但对集群用户可见 | 使用私有镜像仓库 + `imagePullSecrets` 或节点级认证 |
| 部分敏感镜像需要更严格的访问控制 | 启用 `AlwaysPullImages` 准入控制器，将敏感数据放入 Secret 而非镜像中 |
| 多租户集群，每个租户有自己的私有仓库 | 启用 `AlwaysPullImages`，为每个租户生成独立凭证并分发到对应命名空间 |

## 最佳实践/注意事项

- **生产环境避免使用 `:latest` 标签**：难以追踪当前运行版本，也不利于回滚。建议指定有意义的标签（如 `v1.42.0`）或镜像 digest
- **使用 digest 确保版本一致性**：`<image-name>@<digest>` 可以固定代码版本，防止仓库端标签变动导致运行不同版本的混合 Pod
- **为每个命名空间单独配置 `imagePullSecrets`**：Pod 只能引用同命名空间内的 Secret
- **合理配置并行拉取限制**：启用并行拉取时，设置 `maxParallelImagePulls` 防止过度消耗网络带宽和磁盘 I/O
- **升级 Kubernetes 时注意旧版凭证机制**：从 v1.26 起，内置的 ACR/ECR/GCR 凭证提供机制已移除，需改用 kubelet 凭证提供插件或 `imagePullSecrets`

## 生产 YAML 示例

### 私有仓库认证配置

```yaml
# 1. 创建 imagePullSecret
# kubectl create secret docker-registry regcred \
#   --docker-server=registry.example.com \
#   --docker-username=deploy-bot \
#   --docker-password=$REGISTRY_TOKEN \
#   --docker-email=deploy@example.com \
#   -n production

# 2. Pod 中引用 imagePullSecret
apiVersion: v1
kind: Pod
metadata:
  name: private-app
  namespace: production
spec:
  imagePullSecrets:
  - name: regcred
  containers:
  - name: app
    # 使用 digest 锁定镜像版本（推荐生产环境）
    image: registry.example.com/apps/api-server@sha256:a1b2c3d4e5f6...
    imagePullPolicy: IfNotPresent
    resources:
      requests:
        cpu: "500m"
        memory: "512Mi"
```

### 为 ServiceAccount 绑定 imagePullSecret（命名空间级别）

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: default
  namespace: production
imagePullSecrets:
- name: regcred
# 该命名空间下所有使用 default SA 的 Pod 自动获得 regcred
```

### 多架构镜像构建（参考 Dockerfile）

```dockerfile
# 使用 buildx 构建多架构镜像
# docker buildx build --platform linux/amd64,linux/arm64 \
#   -t registry.example.com/apps/api-server:v3.0 --push .

FROM --platform=$TARGETPLATFORM golang:1.22 AS builder
ARG TARGETARCH
WORKDIR /app
COPY . .
RUN CGO_ENABLED=0 GOARCH=$TARGETARCH go build -o server .

FROM --platform=$TARGETPLATFORM gcr.io/distroless/static:nonroot
COPY --from=builder /app/server /server
USER 65534:65534
ENTRYPOINT ["/server"]
```

## 镜像拉取策略决策树

```
是否指定了 imagePullPolicy？
  │
  ├─ 是 → 直接使用指定策略（Always/IfNotPresent/Never）
  │
  └─ 否（自动推断）
        │
        ├─ 指定了 digest (@sha256:...) → IfNotPresent
        ├─ 标签为 :latest → Always
        ├─ 未指定标签 → Always
        └─ 指定了非 latest 标签 → IfNotPresent
```

## 故障排查

| 症状 | 可能原因 | 排查步骤 |
|------|----------|----------|
| ImagePullBackOff | 镜像不存在、认证失败或网络问题 | `kubectl describe pod` 查看 Events；`crictl pull` 手动测试 |
| 同一标签运行了不同版本 | 仓库端标签被覆盖写入新镜像 | 改用 digest 锁定；`kubectl get pod -o jsonpath='{.status.containerStatuses[0].imageID}'` |
| 私有镜像拉取 401 | imagePullSecret 不存在或凭证过期 | `kubectl get secret regcred -o jsonpath='{.data.\.dockerconfigjson}' \| base64 -d` |
| 预拉取镜像仍触发拉取 | `imagePullPolicy: Always` 或使用了 `:latest` | 明确设置 `imagePullPolicy: IfNotPresent` 并使用具体标签 |
| 镜像拉取缓慢 | 镜像体积过大或未配置 mirror | 使用多阶段构建减小镜像；配置仓库 mirror/proxy |
| 节点磁盘空间不足 | 旧镜像未被 GC | 检查 kubelet 的 `imageGCHighThresholdPercent` 配置 |

## 镜像安全最佳实践

| 实践 | 说明 |
|------|------|
| 使用 digest 而非 tag | `image@sha256:...` 保证版本不可变 |
| 扫描漏洞 | CI 中集成 Trivy/Grype 扫描 |
| 最小化基础镜像 | 使用 distroless/scratch/alpine |
| 非 root 运行 | Dockerfile 中添加 `USER nonroot` |
| 签名验证 | 使用 cosign/Notary 签名 + 准入策略验证 |
| 禁止 :latest | 通过 OPA/Kyverno 策略强制 |

## 生产检查清单

- [ ] 生产镜像使用语义化版本标签或 digest，禁止 `:latest`
- [ ] 私有仓库配置 `imagePullSecrets` 或 ServiceAccount 级别绑定
- [ ] CI/CD 中集成镜像漏洞扫描
- [ ] 基础镜像使用 distroless 或 scratch（最小攻击面）
- [ ] 配置镜像仓库 mirror 加速拉取（特别是跨区域场景）
- [ ] kubelet 的 `imageGCHighThresholdPercent` 和 `imageGCLowThresholdPercent` 合理设置
- [ ] 多架构集群使用多架构镜像（image index）
- [ ] 启用 `KubeletEnsureSecretPulledImages` 防止未授权使用预拉取镜像

## 命令快速参考

```bash
# 查看节点上的镜像列表
crictl images
kubectl get nodes -o json | jq '.items[].status.images[:5]'

# 查看 Pod 实际使用的镜像 ID（含 digest）
kubectl get pod <name> -o jsonpath='{.status.containerStatuses[*].imageID}'

# 测试私有仓库认证
kubectl create secret docker-registry test-regcred \
  --docker-server=registry.example.com \
  --docker-username=user --docker-password=pass \
  --dry-run=client -o yaml

# 查看 imagePullSecret 内容
kubectl get secret regcred -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d | jq .

# 预拉取镜像到所有节点（DaemonSet 方式）
# 创建一个使用目标镜像的 DaemonSet，等待完成后删除

# 检查镜像拉取策略
kubectl get pod <name> -o jsonpath='{.spec.containers[*].imagePullPolicy}'

# 节点磁盘使用（镜像占用）
crictl imagefsinfo
```

## 交叉引用

- [容器运行时接口](container-runtime-interface-cri.md) — CRI ImageService 的底层拉取机制
- [RuntimeClass](runtime-class.md) — 按运行时拉取镜像的特性
- [容器环境](container-environment.md) — 镜像层文件系统对容器环境的影响
- [高级 Pod 配置](advanced-pod-configuration.md) — 镜像安全与 SecurityContext

## 参考链接

- [Kubernetes 官方文档：容器镜像](https://kubernetes.io/docs/concepts/containers/images/)
- [OCI Image Manifest Specification](https://github.com/opencontainers/image-spec/blob/main/manifest.md)
- [OCI Distribution Specification](https://github.com/opencontainers/distribution-spec)
