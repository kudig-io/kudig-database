---
title: minikube v0.35 Release Notes
description: minikube v0.35 Release Notes — Kubernetes 生产运维知识库
summary: minikube v0.35 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v0.35 Release Notes 是什么
- 如何 minikube v0.35 Release Notes
trigger_keywords:
- minikube
- v0.35
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# minikube v0.35 Release Notes

Source: [v0.35.0](https://github.com/kubernetes/minikube/releases/tag/v0.35.0)

# Minikube v0.35.0
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.35.0/CHANGELOG.md).

## [[Distribution|Distribution]]
Minikube is distributed in binary form for Linux, OSX, and Windows systems for the v0.35.0 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through GitHub or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.35.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.35.0/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.35.0/minikube-windows-amd64.exe)

## Installation
### OSX
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.35.0/minikube-darwin-amd64 && chmod +x minikube && sudo cp minikube /usr/local/bin/ && rm minikube
```
Feel free to leave off ```sudo cp minikube /usr/local/bin/ && rm minikube``` if you would like to add minikube to your path manually.

Or you can install via homebrew with `brew cask install minikube`.

### Linux
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.35.0/minikube-linux-amd64 && chmod +x minikube && sudo cp minikube /usr/local/bin/ && rm minikube
```
Feel free to leave off ```sudo cp minikube /usr/local/bin/ && rm minikube``` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]
Download the `minikube_0.35.0.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]
Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path.

### Windows Installer [Experimental]
Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage
Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.35.0/README.md).

## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
9fb0ce4decb0f40b1b2ee11cad0aa89e13e2d5c46ff0d35875a0a65788dee6b4

==> out/minikube-linux-amd64.sha256 <==
e161995604c42c37a797fd11fac5d545f8b75f0796afc3b10679253bf229ff3d

==> out/minikube-windows-amd64.exe.sha256 <==
748c9087e876491332897d8dc937eeea92f9fda9003443ef668dc75f5638ed5b
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
b63ec2bca577d3612bc6dc9085946b396d55f82cd782c22ac9588d751f95e7bd
```


<!-- risk-assessed -->
