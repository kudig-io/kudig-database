---
title: minikube v0.23 Release Notes
description: minikube v0.23 Release Notes — Kubernetes 生产运维知识库
summary: minikube v0.23 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.23 Release Notes 是什么
- 如何 minikube v0.23 Release Notes
trigger_keywords:
- minikube
- v0.23
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




# minikube v0.23 Release Notes

Source: [v0.23.0](https://github.com/kubernetes/minikube/releases/tag/v0.23.0)

# Minikube v0.23.0
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.23.0/CHANGELOG.md).

## [[Distribution|Distribution]]
Minikube is distributed in binary form for Linux, OSX, and Windows systems for the v0.23.0 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through GitHub or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.23.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.23.0/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.23.0/minikube-windows-amd64.exe)

## Installation
### OSX
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.23.0/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

### Linux
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.23.0/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]
Download the `minikube_0.23-0.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]
Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path.

### Windows Installer [Experimental]
Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage
Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.23.0/README.md).

## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
3d0c5581cd14f85637fb888c1e2e124152c4c9643a257ba90c8cd929d2c2b8b3

==> out/minikube-linux-amd64.sha256 <==
cd9c6c640a1632e8c44d9b335e68db869da28442b6ab0642a2b7adbc1e4ef334

==> out/minikube-windows-amd64.exe.sha256 <==
ddee80b2505447197994377f40e574061e1d59203019b587361be2b28762fd61
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=65d7e43ebc92c7a8b1911a842d06f4f995d87e0bbee519b49110926d3ee902f7

```


<!-- risk-assessed -->
