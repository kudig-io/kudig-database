# Pod Hostname

## 概述
本页解释 Pod 主机名的设置方式、配置后的潜在副作用以及底层机制。Pod 内部观察到的主机名默认来自 `metadata.name`。

## 核心概念/原理
- **默认主机名**：Pod 创建时，其主机名和完全限定域名（FQDN）均默认为 `metadata.name` 的值。
- **自定义主机名（`spec.hostname`）**：设置该字段后，其值优先于 `metadata.name` 作为 Pod 内部的主机名。
- **子域（`spec.subdomain`）**：
  - 若设置了 `spec.hostname=foo` 和 `spec.subdomain=bar`，则主机名为 `foo`，FQDN 为 `foo.bar.<namespace>.svc.<cluster-domain>`。
  - 同时设置 `hostname` 和 `subdomain` 时，集群 DNS 服务器会为 Pod 创建 A/AAAA 记录。
- **FQDN 作为主机名（`setHostnameAsFQDN`）**：
  - 默认情况下，`hostname` 命令返回短主机名。
  - 设置 `setHostnameAsFQDN: true` 后，kubelet 会将 FQDN 写入 Pod 的 hostname 命名空间，`hostname` 和 `hostname --fqdn` 均返回 FQDN。
  - Linux 内核的 hostname 字段限制为 64 个字符；若 FQDN 超过此长度，Pod 将无法启动（停留在 `ContainerCreating`）。
- **主机名覆盖（`hostnameOverride`）**：
  - Beta 特性（v1.35 默认启用）。
  - 无条件将 Pod 内部的主机名和 FQDN 都设置为 `hostnameOverride` 的值。
  - 长度限制 64 字符，遵循 RFC 1123 DNS 子域名标准。
  - **注意**：`hostnameOverride` 不影响集群 DNS 中的 A/AAAA 记录；若同时设置了 `hostname` 和 `subdomain`，DNS 记录仍基于后者生成。
  - 不能与 `hostNetwork` 和 `setHostnameAsFQDN` 同时设置。

## 关键机制或特性
- 主机名配置仅影响 Pod 内部进程看到的名称。
- DNS 记录的生成取决于 `hostname` + `subdomain`，而非 `hostnameOverride`。
- `hostnameOverride` 适用于需要 Pod 内部进程看到特定主机名，但不想改变 DNS 记录的场景。

## 使用场景
- 应用依赖特定主机名进行许可证验证或集群成员识别。
- 需要为 StatefulSet Pod 提供稳定且可预测的网络标识。
- 在 Pod 内部模拟特定的域名环境。

## 最佳实践/注意事项
- 确保 `metadata.name` 或 `spec.hostname` 与 `subdomain` 组合后的 FQDN 不超过 64 字符（若启用 `setHostnameAsFQDN`）。
- 使用 `hostnameOverride` 时，注意其不会修改 DNS 记录，且不能与 `hostNetwork` 同时使用。
- Pod 名称应符合 DNS Label 规则，以获得最佳兼容性。

## 参考链接
- https://kubernetes.io/docs/concepts/workloads/pods/pod-hostname/
