---
title: minikube v0.10 Release Notes
description: minikube v0.10 Release Notes — Kubernetes 生产运维知识库
summary: minikube v0.10 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.10 Release Notes 是什么
- 如何 minikube v0.10 Release Notes
trigger_keywords:
- minikube
- v0.10
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




# minikube v0.10 Release Notes

Source: [v0.10.0](https://github.com/kubernetes/minikube/releases/tag/v0.10.0)

# Minikube v0.10.0

Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.10.0/CHANGELOG.md).

## [[Distribution|Distribution]]

Minikube is distrubuted in binary form for Linux, OSX, and Windows systems for the v0.10.0 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.10.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.10.0/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.10.0/minikube-windows-amd64.exe)

## Installation

### OSX

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.10.0/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Linux

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.10.0/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Windows [Experimental]

Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path

## Usage

Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.10.0/README.md).

## Checksums

Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
fc7552b5475d0c20ad96057fb88d686e226d09bd1034269d5078b33ba7d7cc8e

==> out/minikube-linux-amd64.sha256 <==
f905af9b2ef1e954d0633680ab5a8914d628b9104cce54de7e42509d89d2c541

==> out/minikube-windows-amd64.exe.sha256 <==
93b0e1fabeab79da2b61c9237076893dacf3cc147294298d50cb48ca7cd2a86f
```

### ISO

``` shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)= 
aadc8b6f5720d5a493a36e1f07f71bffb588780c76498d68cd761793d2ca344e
```


<!-- risk-assessed -->
