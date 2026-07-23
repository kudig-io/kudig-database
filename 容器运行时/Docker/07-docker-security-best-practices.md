---
title: Docker 安全最佳实践
description: '# Docker 安全最佳实践'
summary: 'RUN pip install --user --no-cache-dir -r requirements.txt'
category: docker
tags:
- docker
- container
- image
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 开发工程师
- 运维工程师
- SRE
estimated_read_time: 5min
intent_queries:
- Docker 安全最佳实践 是什么
- 如何 Docker 安全最佳实践
- Kubernetes 13 docker 最佳实践
trigger_keywords:
- Docker
- 安全最佳实践
- docker
prerequisites:
- kubectl-basics
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
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/docker.md
  label: '速查卡: docker'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Docker 安全最佳实践

> **适用版本**: Docker 20.10+ / Docker 24.0+ / Docker 25.0+ | **最后更新**: 2026-01
> 
> **生产环境运维专家注**: 全面覆盖容器安全加固、漏洞扫描、权限管控、网络安全隔离、合规审计等企业级安全防护体系，满足等保2.0、GDPR等合规要求。

---

## 目录

- [容器安全基础](#容器安全基础)
- [镜像安全](#镜像安全)
- [容器运行时安全](#容器运行时安全)
- Linux 安全机制](#linux-安全机制)
- [Docker Daemon 安全](#docker-daemon-安全)
- [安全检查清单](#安全检查清单)

---

## 容器安全基础

### 容器隔离机制

| 层次 | 机制 | 作用 |
|:---|:---|:---|
| **应用层** | 代码安全、依赖安全 | 防止应用漏洞 |
| **运行时层** | 非 root、只读 FS、能力删除 | 限制容器权限 |
| **内核层** | Namespaces、Cgroups、Seccomp | 隔离与限制 |
| **主机层** | SELinux/AppArmor、审计 | 强制访问控制 |

### 攻击面与防护

| 攻击面 | 风险 | 防护措施 |
|:---|:---|:---|
| **镜像漏洞** | 已知 CVE | 镜像扫描、可信源 |
| **权限提升** | root 逃逸 | 非 root、删除能力 |
| **容器逃逸** | 挂载敏感路径 | 只读 FS、禁止特权 |
| **资源耗尽** | DoS 攻击 | 资源限制 |

---

## 镜像安全

### 可信基础镜像

| 镜像类型 | 安全性 | 推荐场景 |
|:---|:---|:---|
| **Distroless** | 最高 | 生产环境 |
| **Alpine** | 高 (精简) | 减少攻击面 |
| **官方镜像** | 高 | 通用场景 |

### 安全 Dockerfile

```dockerfile
FROM python:3.12-slim-bookworm AS builder
WORKDIR /app
COPY requirements.txt .
RUN pip install --user --no-cache-dir -r requirements.txt

FROM gcr.io/distroless/python3-debian12
COPY --from=builder /root/.local /root/.local
COPY --chown=nonroot:nonroot app.py .
USER nonroot:nonroot
EXPOSE 8080
ENTRYPOINT ["python", "app.py"]
```

### 镜像扫描

```bash
# Trivy 扫描
trivy image --severity HIGH,CRITICAL myapp:latest

# Grype 扫描
grype myapp:latest
```

---

## 容器运行时安全

### 非 root 用户

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker run --user 1000:1000 myapp
```
### 只读文件系统

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker run --read-only \
  --tmpfs /tmp:size=100m \
  --tmpfs /run:size=10m \
  myapp
```
### 能力管理

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker run \
  --cap-drop ALL \
  --cap-add NET_BIND_SERVICE \
  myapp
```
| 能力 | 风险 | 建议 |
|:---|:---:|:---|
| `CAP_SYS_ADMIN` | 极高 | 禁止 |
| `CAP_NET_ADMIN` | 高 | 按需 |
| `CAP_NET_BIND_SERVICE` | 低 | 允许 |

### 禁止权限提升

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker run --security-opt no-new-privileges:true myapp
```
### 资源限制

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker run \
  --memory 512m \
  --cpus 1.0 \
  --pids-limit 100 \
  myapp
```
---

## Linux 安全机制

### Seccomp

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker run --security-opt seccomp=./profile.json myapp
```
### AppArmor

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker run --security-opt apparmor=myprofile myapp
```
---

## Docker Daemon 安全

### 安全配置

```json
{
  "icc": false,
  "live-restore": true,
  "no-new-privileges": true,
  "userns-remap": "default"
}
```

### Rootless Docker

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
curl -fsSL https://get.docker.com/rootless | sh
```
---

## 安全检查清单

### 镜像安全

| 项目 | 检查内容 |
|:---|:---|
| ☐ 特定标签 | 避免 :latest |
| ☐ 可信来源 | 官方或已验证 |
| ☐ 漏洞扫描 | 无高危漏洞 |
| ☐ 非 root | 定义 USER |

### 运行时安全

| 项目 | 检查内容 |
|:---|:---|
| ☐ 非特权 | privileged=false |
| ☐ 非 root | user != 0 |
| ☐ 只读 FS | read_only=true |
| ☐ 能力限制 | cap_drop ALL |

### Docker Bench

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
docker run --rm --net host --pid host \
  -v /var/run/docker.sock:/var/run/docker.sock:ro \
  docker/docker-bench-security
```
---

## 相关文档

- [200-docker-architecture-overview](./200-docker-architecture-overview.md)
- [217-linux-container-fundamentals](./217-linux-container-fundamentals.md)

## 故障排查

| 问题 | 可能原因 | 诊断命令 | 解决方案 |
|------|----------|----------|----------|
| 容器以 root 运行 | 未指定 USER | `docker inspect <id> | jq .Config.User` | Dockerfile 添加 USER 指令 |
| 镜像含漏洞 | 基础镜像过旧 | `trivy image <image>` | 更新基础镜像并重新构建 |
| 特权容器 | --privileged 滥用 | `docker inspect <id> | jq .HostConfig.Privileged` | 移除特权，使用 capabilities |
| 敏感信息泄露 | 环境变量含密钥 | `docker inspect <id> | jq .Config.Env` | 使用 Docker secrets |
| 容器逃逸风险 | 危险挂载 | `docker inspect <id> | jq .HostConfig.Binds` | 移除 /var/run/docker.sock 挂载 |
| 网络暴露 | 端口映射过多 | `docker port <id>` | 仅暴露必要端口 |
| 日志含敏感数据 | 应用日志未脱敏 | `docker logs <id>` | 应用层脱敏处理 |
| 镜像未签名 | 缺少供应链验证 | `cosign verify <image>` | 启用 cosign/notation 签名 |

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| 用户 | 始终以非 root 运行 | USER 指令 + --user 参数 |
| 文件系统 | readOnlyRootFilesystem | 防止容器内写入 |
| 能力 | 最小化 Linux capabilities | --cap-drop ALL + 按需添加 |
| 网络 | 使用专用网络，限制端口 | 避免 --network host |
| 镜像 | 使用 distroless/alpine 基础 | 减小攻击面 |
| 扫描 | CI 强制漏洞扫描 | Trivy/Grype 集成 |
| 签名 | 启用镜像签名验证 | cosign + admission webhook |
| 资源 | 设置 CPU/内存限制 | 避免资源耗尽 |

## 相关工具

| 工具 | 用途 | 安装/使用 |
|------|------|----------|
| Trivy | 镜像漏洞扫描 | `brew install trivy` |
| Grype | 替代扫描工具 | `brew install grype` |
| cosign | 镜像签名 | `go install github.com/sigstore/cosign/v2/cmd/cosign@latest` |
| docker-bench | 安全基线检查 | `docker run docker/docker-bench-security` |
| dive | 镜像层分析 | `brew install dive` |
| hadolint | Dockerfile lint | `brew install hadolint` |

## 常见问题 FAQ

| 问题 | 解答 |
|------|------|
| 如何检查容器安全配置？ | `docker inspect` + docker-bench-security |
| 为什么不能用 --privileged？ | 完全放弃隔离，等同 root 宿主机 |
| 如何安全地挂载 Docker socket？ | 不应挂载，使用 rootless 或远程 API |
| 多阶段构建如何提升安全？ | 最终镜像不含编译工具，减小攻击面 |
| 如何限制容器网络访问？ | 使用 network policy 或 iptables |
| secrets 如何安全传递？ | Docker secrets 或 K8s Secrets + 加密 |
| 如何审计容器操作？ | 启用 auditd + containerd 日志 |
| 镜像签名如何验证？ | cosign verify + admission controller |

## 安全加固检查清单

| 检查项 | 命令/方法 | 期望结果 |
|--------|----------|----------|
| 非 root 运行 | `docker inspect <id> | jq .Config.User` | 非空且非 root |
| 只读文件系统 | `docker inspect <id> | jq .HostConfig.ReadonlyRootfs` | true |
| 无特权模式 | `docker inspect <id> | jq .HostConfig.Privileged` | false |
| 最小 capabilities | `docker inspect <id> | jq .HostConfig.CapDrop` | ["ALL"] |
| 无危险挂载 | `docker inspect <id> | jq .HostConfig.Binds` | 无 docker.sock |
| 资源限制 | `docker inspect <id> | jq .HostConfig.Memory` | > 0 |
| 网络隔离 | `docker network ls` | 专用网络 |
| 镜像扫描 | `trivy image <image>` | 无 HIGH/CRITICAL |

## 版本兼容性

| Docker 版本 | 安全特性 | 说明 |
|------------|----------|------|
| 20.10+ | rootless 模式稳定 | 推荐生产使用 |
| 23.0+ | BuildKit 默认 | 更安全构建 |
| 24.0+ | 改进的 capabilities | 更细粒度控制 |
| 25.0+ | 增强 seccomp | 默认配置文件更新 |

## 迁移指南

| 从 | 到 | 关键步骤 |
|------|------|----------|
| root 容器 | nonroot | 添加 USER 指令 |
| 特权模式 | capabilities | --cap-drop ALL + 按需添加 |
| 无扫描 | CI 扫描 | 集成 Trivy |
| 无签名 | cosign | 启用签名 + admission |

## 架构对比

```text
Docker 安全层次：

镜像层：签名 + 扫描 + 最小化
运行时：非 root + 只读 + 最小 capabilities
网络层：专用网络 + 端口限制
内核层：seccomp + AppArmor + SELinux
```

## See Also

- volumes.md|05-docker-storage-volumes]]
- 06-docker-compose-orchestration
- 08-docker-troubleshooting-guide
- 09-docker-performance-monitoring


<!-- risk-assessed -->
