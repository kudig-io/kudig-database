---
title: minikube v1.3 Release Notes
description: minikube v1.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- minikube v1.3 Release Notes 是什么
- 如何 minikube v1.3 Release Notes
trigger_keywords:
- minikube
- v1.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# minikube v1.3 Release Notes

Source: [v1.3.1](https://github.com/kubernetes/minikube/releases/tag/v1.3.1)

# Minikube v1.3.1
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v1.3.1/CHANGELOG.md).

## [[Distribution|Distribution]]
Minikube is distributed in binary form for Linux, OSX, and Windows systems for the v1.3.1 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through GitHub or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v1.3.1/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v1.3.1/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v1.3.1/minikube-windows-amd64.exe)

## Installation

See https://minikube.sigs.k8s.io/docs/start/

## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
5947abe57fa390fcfd96849ac87fc9319f026d4b13f944b6beecc3615e3668be

==> out/minikube-linux-amd64.sha256 <==
057a4f4ca36ff51ecad59509d94d3694543b874949e805e2b79792ceef21f983

==> out/minikube-windows-amd64.exe.sha256 <==
be1999452b166de72f946aac17ca7c40d53b8a5c8f59dad912c2413f7cc42563
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
784595860ee65645bf53074aea90cd03ba697d27f5093594b732c3cd4f1956da
```
