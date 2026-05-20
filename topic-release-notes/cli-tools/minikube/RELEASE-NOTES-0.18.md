---
title: minikube v0.18 Release Notes
description: minikube v0.18 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.18 Release Notes 是什么
- 如何 minikube v0.18 Release Notes
trigger_keywords:
- minikube
- v0.18
- Release
- Notes
- release
- notes
---

# minikube v0.18 Release Notes

Source: [v0.18.0](https://github.com/kubernetes/minikube/releases/tag/v0.18.0)

# Minikube v0.18.0
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.18.0/CHANGELOG.md).

## Distribution
Minikube is distrubuted in binary form for Linux, OSX, and Windows systems for the v0.18.0 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.18.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.18.0/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.18.0/minikube-windows-amd64.exe)

## Installation
### OSX
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.18.0/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

### Linux
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.18.0/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]
Download the `minikube_0.18-0.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]
Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path

### Windows Installer [Experimental]
Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage
Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.18.0/README.md).


## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
301dc5f47cccface80769bd3a03d58ba80893f6ede75f2a7ec22f1bc56666deb

==> out/minikube-linux-amd64.sha256 <==
212fbf52a6d601deb383271e2af756982f9b72715e1f7a3d65770d04387d8c8d

==> out/minikube-windows-amd64.exe.sha256 <==
302bfe9747882291e686ecf6f337e78836f7bb0e1783fc6d8bf1f3482fb2badf
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
1d0bad32c3eaa76110ef0a8030aa3dcb1f5fe75e017f28ff93d468af505e04aa
```
