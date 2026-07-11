---
title: 容器运行时演进
summary: 容器运行时演进：containerd 是 CNCF 毕业项目，Kubernetes 最广泛采用的容器运行时。
category: concepts
tags:
- runtime
- containerd
- wasm
- confidential-containers
- k8s
tier: core
created: 2026-05-24
updated: 2026-07
last_updated: 2026-07
---



# 容器运行时演进

## containerd 2.x

[[containerd]] 是 CNCF 毕业项目，Kubernetes 最广泛采用的容器运行时。

- **v2.0 LTS**：2024 年发布，移除 CRI v1alpha2，仅保留 CRI v1；引入 Sandbox API 支持可插拔沙箱运行时
- **v2.3 LTS**：保持约 4 个月发布节奏，持续优化稳定性与性能
- **CRI v1 唯一**：彻底废弃 v1alpha2 接口，简化代码路径
- **Sandbox API**：将 Pod sandbox 管理从 kubelet 解耦，支持 Kata Containers、WASM 等替代沙箱实现

## CRI-O

- 严格跟随 Kubernetes 版本发布（1.29、1.30…）
- 从 v1.29 起 crun 成为默认 OCI 运行时（替代 runc）
- 面向生产优化，Red Hat OpenShift 默认运行时

## crun vs runc

| 维度 | crun | runc |
|------|------|------|
| 语言 | C | Go |
| 启动速度 | 2-5x 更快 | 基准 |
| 内存占用 | ~1MB | ~10MB |
| WASM 支持 | 原生 | 无 |

crun 由 Red Hat 维护，适合低延迟与资源受限场景。

## User Namespaces GA（Kubernetes v1.36）

- 容器内 root 用户映射到宿主机非特权用户，大幅降低逃逸风险
- 支持 **idmap mounts**：文件权限正确映射，无需 chown
- 无需修改镜像或应用代码即可启用

参见 User Namespaces。

## WASM 容器

WebAssembly 作为新型容器运行时正在崛起：

- **runwasi**：containerd 的 WASM shim，使 containerd 可调度 WASM 工作负载
- **[[SpinKube]]** v0.24：CNCF Sandbox，将 Fermyon Spin 应用编排为 K8S 原生工作负载
- **sub-ms 冷启动**：WASM 模块启动延迟在亚毫秒级，远快于传统容器
- 适用于边缘计算、函数即服务、插件系统等场景

## 机密容器（Confidential Containers）

- **CoCo**：CNCF 沙箱项目，提供跨 TEE 厂商的统一机密容器标准
- **Kata Containers** 3.x：支持 Intel TDX、AMD SEV-SNP、ARM CCA 等 TEE 环境
- **GPU passthrough**：机密容器支持 GPU 直通，使 AI/ML 推理在 TEE 内安全执行
- 数据在使用中始终加密，解决多租户与公有云场景的信任根问题

参见 confidential-containers、TEE attestation。

## 镜像懒加载（Lazy Pulling）

传统容器启动需要完整拉取镜像，懒加载技术按需获取数据块：

- **Nydus** RAFS：CNCF 子项目，阿里开源，基于 FUSE/virtio-fs 的按需加载
- **eStargz**：Google 提出，兼容 OCI 标准的可寻址压缩格式
- **OverlayBD**：阿里云提出的块设备级懒加载方案

**效果**：冷启动时间缩短 50-80%，尤其在大镜像（>1GB）场景效果显著。

参见 OCI image spec、image lifecycle。

## 技术深度解析

### CRI 架构演进

容器运行时接口（CRI）的演进反映了 Kubernetes 解耦运行时的持续努力：

```
K8s 1.20 之前: Docker (dockershim) → 内置于 kubelet
K8s 1.20-1.24: dockershim 废弃警告 → CRI 过渡期
K8s 1.24+:     移除 dockershim → containerd / CRI-O 直接对接
K8s 1.32+:     CRI v1 唯一 → 移除 v1alpha2
```

### RuntimeClass 与多运行时共存

生产集群可以同时运行多种容器运行时，通过 RuntimeClass 按工作负载选择：

```yaml
# 标准容器（默认 runc）
spec:
  runtimeClassName: runc          # 高性能、低开销

# 安全隔离（Kata Containers）
spec:
  runtimeClassName: kata          # VM 级隔离，多租户场景

# WASM 工作负载
spec:
  runtimeClassName: wasm          # 亚毫秒启动、极小体积
```

### containerd 2.0 Sandbox API

containerd 2.0 引入的 Sandbox API 将 Pod sandbox 管理从 kubelet CRI 中解耦：

```
传统 CRI: kubelet → CRI RunPodSandbox → containerd 内部处理
Sandbox API: kubelet → CRI → containerd Sandbox Controller
  → 支持可插拔 sandbox 实现（runc / kata / wasm）
  → sandbox 生命周期独立于容器管理
```

## 最佳实践

- **containerd 作为默认运行时**：生产环境默认使用 containerd 2.x（CRI v1），无需 Docker——更轻量、更安全
- **安全敏感场景考虑 Kata Containers**：多租户或强隔离需求场景使用 Kata Containers 提供虚拟机级隔离，配合 Pod Overhead 计量资源
- **大镜像场景启用懒加载**：AI/ML 场景镜像通常 >1GB，启用 Nydus 或 eStargz 懒加载可将冷启动从分钟级降至秒级
- **ARM 节点使用 crun 替代 runc**：crun 在 ARM 平台上启动更快、内存占用更小——CRI-O 已默认使用 crun
- **为 WASM 工作负载准备独立节点池**：WASM shim 与传统容器行为不同，通过 RuntimeClass 和 taint 隔离 WASM 节点

## 常见陷阱

- **containerd 版本与 K8s 不兼容**：K8s 1.32+ 要求 containerd 2.0+，升级 K8s 前必须先升级运行时——否则 Pod 无法创建
- **镜像懒加载的首次访问延迟**：懒加载将镜像拉取延迟到运行时，首次文件访问可能变慢——不适合启动时加载大量文件的应用
- **机密容器性能开销**：TEE 加密/解密有 5-15% 性能损耗——需要评估是否值得安全收益

## 相关页面

- [[概念/specialized-k8s-technologies.md|K8S 专项技术]] — WASM 与边缘计算
- [[概念/k8s-security-compliance.md|K8S 安全与合规]] — 机密容器与 TEE
- [[概念/k8s-ai-ml-infrastructure.md|K8S AI/ML 基础设施]] — 大镜像优化
- [[概念/pod-overhead.md|Pod Overhead]] — 安全运行时的资源计量
- [[概念/kubernetes-containerd-integration.md|K8s 与 containerd 集成]] — CRI 通信架构
