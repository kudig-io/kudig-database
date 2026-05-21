---
title: minikube v0.12 Release Notes
description: minikube v0.12 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.12 Release Notes 是什么
- 如何 minikube v0.12 Release Notes
trigger_keywords:
- minikube
- v0.12
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# minikube v0.12 Release Notes

Source: [v0.12.2](https://github.com/kubernetes/minikube/releases/tag/v0.12.2)

# Minikube v0.12.2

Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.12.2/CHANGELOG.md).

## Distribution

Minikube is distrubuted in binary form for Linux, OSX, and Windows systems for the v0.12.2 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.12.2/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.12.2/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.12.2/minikube-windows-amd64.exe)

## Installation

### OSX

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.12.2/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Linux

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.12.2/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]

Download the `minikube_0.12-2.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]

Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path

## Usage

Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.12.2/README.md).

## Checksums

Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
5f818be5235d606ec5241ac1eea0dc92e6328d56841617c51e4595a0abc4300c

==> out/minikube-linux-amd64.sha256 <==
ace8dcaf9418ed0e4eb07e7737b0af41036f6aaa05f784386725feb041463a74

==> out/minikube-windows-amd64.exe.sha256 <==
1768aea06220946f22e88d294f02984135a77799b88373fc6b1a98b3da6802ef
```

### ISO

``` shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)= 
aadc8b6f5720d5a493a36e1f07f71bffb588780c76498d68cd761793d2ca344e
```
