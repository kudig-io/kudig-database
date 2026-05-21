---
title: minikube v0.26 Release Notes
description: minikube v0.26 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.26 Release Notes 是什么
- 如何 minikube v0.26 Release Notes
trigger_keywords:
- minikube
- v0.26
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# minikube v0.26 Release Notes

Source: [v0.26.1](https://github.com/kubernetes/minikube/releases/tag/v0.26.1)

# Minikube v0.26.1
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.26.1/CHANGELOG.md).

## Distribution
Minikube is distributed in binary form for Linux, OSX, and Windows systems for the v0.26.1 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through GitHub or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.26.1/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.26.1/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.26.1/minikube-windows-amd64.exe)

## Installation
### OSX
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.26.1/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

### Linux
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.26.1/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]
Download the `minikube_0.26-1.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]
Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path.

### Windows Installer [Experimental]
Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage
Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.26.1/README.md).

## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
8cabcaa244a7d62697ad8f4393e3661c9e9cd598a75df79a8d1ffe803f80b209

==> out/minikube-linux-amd64.sha256 <==
3c956459ee8dad9452c97d54463f982d1d511a9afaa0f3851d88320bd5dcf58a

==> out/minikube-windows-amd64.exe.sha256 <==
498f9f090972c047836fcb1580bb6fcee53fe920955377569d903ba72a0c425e
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
f652fe00c2c83da2b10dda3314f071a1887ff222be63a26b211e3dd92df260c1
```
