---
title: minikube v0.28 Release Notes
description: minikube v0.28 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.28 Release Notes 是什么
- 如何 minikube v0.28 Release Notes
trigger_keywords:
- minikube
- v0.28
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# minikube v0.28 Release Notes

Source: [v0.28.2](https://github.com/kubernetes/minikube/releases/tag/v0.28.2)

# Minikube v0.28.2
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.28.2/CHANGELOG.md).

## [[Distribution|Distribution]]
Minikube is distributed in binary form for Linux, OSX, and Windows systems for the v0.28.2 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through GitHub or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.28.2/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.28.2/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.28.2/minikube-windows-amd64.exe)

## Installation
### OSX
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.28.2/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

Or you can install via homebrew with `brew cask install minikube`.

### Linux
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.28.2/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]
Download the `minikube_0.28-2.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]
Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path.

### Windows Installer [Experimental]
Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage
Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.28.2/README.md).

## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
dca43053510f5e8987ff89abf601594eaf58bc5d447d74f9a08e300f3d92133f

==> out/minikube-linux-amd64.sha256 <==
3c84872ffa5ddbce472062fb548f9b3a25af72587d35243e12f18d86aaa6a085

==> out/minikube-windows-amd64.exe.sha256 <==
1f9840a3a54d793b60ff1b9c7ef1c87269ee24d118f7ef47667d9f7a0ef7861a
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
c23cb6487b8be853c3579ee00508de40d3a56521d7355d13e91ad8bdbd7ce130
```
