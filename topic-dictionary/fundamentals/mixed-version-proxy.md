# Mixed Version Proxy（混合版本代理）

## 概述

Mixed Version Proxy 是 Kubernetes 1.28 引入的 Alpha 特性（默认关闭），它允许 API 服务器将资源请求代理给其他对等（peer）API 服务器，同时使客户端能够通过发现机制获得整个集群资源的完整视图。这在集群中运行多个不同版本的 Kubernetes API 服务器时非常有用（例如在进行长时间的滚动升级期间）。

## 核心概念/原理

- **Peer-aggregated Discovery（对等聚合发现）**：确保依赖发现机制的控制器始终能看到集群中所有资源的完整列表，即使不同资源由不同版本的 API 服务器提供。
- **Mixed Version Proxying（混合版本代理）**：在升级期间，将资源请求定向到能够正确提供该 API 的对等 API 服务器，防止用户因升级过程而遇到意外的 404 Not Found 错误。

## 关键机制或特性

### 启用条件

需要在启动 API 服务器时启用 `UnknownVersionInteroperabilityProxy` 特性门控，并配置以下参数：

- `--peer-ca-file`：用于验证对等 API 服务器服务证书的 CA 证书
- `--proxy-client-cert-file` / `--proxy-client-key-file`：源 API 服务器向目标 API 服务器证明身份时使用的客户端证书
- `--requestheader-client-ca-file`：目标 API 服务器用于验证对等连接的 CA 证书
- `--requestheader-allowed-names`：验证代理客户端证书时允许的 Common Name（可设为空以允许任意 CN）
- `--peer-advertise-ip` / `--peer-advertise-port`（可选）：指定对等 API 服务器用于代理请求的网络地址

### 代理传输与身份验证

- 源 API 服务器复用现有的聚合层代理客户端证书标志（`--proxy-client-cert-file` 和 `--proxy-client-key-file`）向对等 API 服务器证明身份。
- 目标 API 服务器通过 `--requestheader-client-ca-file` 验证对等连接。
- 源 API 服务器通过 `--peer-ca-file` 验证目标 API 服务器的服务证书。

### 对等聚合发现

启用该特性后，发现请求默认会自动提供聚合后的完整发现文档（列出集群中所有 API 服务器提供的资源）。如果客户端需要非聚合的发现文档，可在请求 Accept 头中添加：

```
application/json;g=apidiscovery.k8s.io;v=v2;as=APIGroupDiscoveryList;profile=nopeer
```

### 混合版本代理的工作流程

1. API 服务器收到资源请求后，首先检查本地非聚合发现文档。
2. 如果请求的资源在本地发现文档中存在（如 `GET /api/v1/pods/some-pod`），则由本地处理。
3. 如果资源不存在于本地发现文档中（如某个在新版本 Kubernetes 中才引入的 API），处理请求的 API 服务器会查询所有对等 API 服务器的非聚合发现文档，找到能够提供该资源的对等 API 服务器，然后将请求代理过去。
4. 如果没有已知的对等 API 服务器能处理该请求，则由本地处理链返回 404 Not Found。
5. 如果找到了对等 API 服务器但无法建立连接（如网络故障或数据竞争），则返回 503 Service Unavailable。

## 使用场景

- 长时间滚动升级 Kubernetes 控制平面期间，确保旧版本 API 服务器能够透明地将新 API 请求转发到新版本 API 服务器
- 高可用集群中，不同 API 服务器运行不同版本时的平滑过渡
- 控制器和客户端通过发现机制获得完整、一致的集群资源视图

## 最佳实践/注意事项

- 该特性目前为 Alpha 状态，仅在充分测试的非生产环境或愿意承担风险的生产环境中使用
- 正确配置所有 TLS/CA 证书和请求头验证参数，确保对等 API 服务器之间的安全通信
- 若未指定 `--peer-advertise-ip` 和 `--peer-advertise-port`，对等 API 服务器会回退使用 `--advertise-address` 或 `--bind-address`，最终使用主机默认接口
- 监控升级期间的 503 错误，排查网络连通性或发现信息同步延迟问题

## 参考链接

- https://kubernetes.io/docs/concepts/architecture/mixed-version-proxy/
