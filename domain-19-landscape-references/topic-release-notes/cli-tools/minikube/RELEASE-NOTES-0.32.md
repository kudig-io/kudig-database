---
title: minikube v0.32 Release Notes
description: minikube v0.32 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.32 Release Notes 是什么
- 如何 minikube v0.32 Release Notes
trigger_keywords:
- minikube
- v0.32
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# minikube v0.32 Release Notes

Source: [v0.32.0](https://github.com/kubernetes/minikube/releases/tag/v0.32.0)

# Minikube v0.32.0
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.32.0/CHANGELOG.md).

## Distribution
Minikube is distributed in binary form for Linux, OSX, and Windows systems for the v0.32.0 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through GitHub or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.32.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.32.0/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.32.0/minikube-windows-amd64.exe)

## Installation
### OSX
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.32.0/minikube-darwin-amd64 && chmod +x minikube && sudo cp minikube /usr/local/bin/ && rm minikube
```
Feel free to leave off ```sudo cp minikube /usr/local/bin/ && rm minikube``` if you would like to add minikube to your path manually.

Or you can install via homebrew with `brew cask install minikube`.

### Linux
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.32.0/minikube-linux-amd64 && chmod +x minikube && sudo cp minikube /usr/local/bin/ && rm minikube
```
Feel free to leave off ```sudo cp minikube /usr/local/bin/ && rm minikube``` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]
Download the `minikube_0.32-0.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]
Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path.

### Windows Installer [Experimental]
Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage
Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.32.0/README.md).

## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
d5b21adacd4b6bad9006816e4bdb29f39318a60919e7a6bb5e388a6299fffd0f

==> out/minikube-linux-amd64.sha256 <==
3298d3183deacd9ddd3032dab113a64d863df7648d6d24693284ba4193e95b49

==> out/minikube-windows-amd64.exe.sha256 <==
31668f64f6be25644b22d0465e0f72d745a027804b2c8685e040b8147bacfba1
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
0cf854dbf6a131f5fa9139ddebc2e1593367c869c94cdd267dfe0184cb3213df
```
