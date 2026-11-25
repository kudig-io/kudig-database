---
title: minikube v0.1 Release Notes
description: minikube v0.1 Release Notes — Kubernetes 生产运维知识库
summary: minikube v0.1 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.1 Release Notes 是什么
- 如何 minikube v0.1 Release Notes
trigger_keywords:
- minikube
- v0.1
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




# minikube v0.1 Release Notes

Source: [v0.1.0](https://github.com/kubernetes/minikube/releases/tag/v0.1.0)

# Minikube v0.1.0

This is the initial release of Minikube. Minikube is still under active development, and features may change at any time.

## [[Distribution|Distribution]]

Minikube is only distributed in binary form for Linux and OSX systems for the v0.1.0 release. Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.1.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.1.0/minikube-linux-amd64)

## Installation

### OSX

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.1.0/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Linux

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.1.0/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

## Usage

Documentation is available [here](https://github.com/kubernetes/minikube/blob/master/README.md).

## Checksums

Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare SHA1 hashes with these values:

### OSX

``` shell
$ openssl sha1 minikube-darwin-amd64
SHA1(minikube-darwin-amd64)= 3bb14d8edbfce78a629a8fae2aa851222da5d2b6
```

### Linux

``` shell
$ openssl sha1 minikube-linux-amd64
SHA1(minikube-linux-amd64)= 232fab1b77aeeb49efc157811e79ce031b72a182
```

### ISO

``` shell
$ openssl sha1 minikube.iso
SHA1(minikube.iso)= b817e54b1ea44889dcacbb89aa68736b306017c5
```


<!-- risk-assessed -->
