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
updated: 2026-05-24
last_updated: 2026-05-24
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

## Related

- [[concepts/specialized-k8s-technologies.md|specialized k8s technologies]] — 特殊化 K8S 技术
- [[concepts/k8s-security-compliance.md|k8s security compliance]] — K8S 安全与合规
- [[concepts/k8s-ai-ml-infrastructure.md|k8s ai ml infrastructure]] — K8S AI/ML 基础设施
