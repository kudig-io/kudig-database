---
title: minikube v1.2 Release Notes
description: minikube v1.2 Release Notes — Kubernetes 生产运维知识库
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
- minikube v1.2 Release Notes 是什么
- 如何 minikube v1.2 Release Notes
trigger_keywords:
- minikube
- v1.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# minikube v1.2 Release Notes

Source: [v1.2.0](https://github.com/kubernetes/minikube/releases/tag/v1.2.0)

# Minikube v1.2.0
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v1.2.0/CHANGELOG.md).

## Distribution
Minikube is distributed in binary form for Linux, OSX, and Windows systems for the v1.2.0 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through GitHub or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v1.2.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v1.2.0/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v1.2.0/minikube-windows-amd64.exe)

## Installation
### OSX
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v1.2.0/minikube-darwin-amd64 && chmod +x minikube && sudo cp minikube /usr/local/bin/ && rm minikube
```
Feel free to leave off ```sudo cp minikube /usr/local/bin/ && rm minikube``` if you would like to add minikube to your path manually.

Or you can install via homebrew with `brew cask install minikube`.

### Linux
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v1.2.0/minikube-linux-amd64 && chmod +x minikube && sudo cp minikube /usr/local/bin/ && rm minikube
```
Feel free to leave off ```sudo cp minikube /usr/local/bin/ && rm minikube``` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]
Download the `minikube_1.2.0.deb` file, and install it using `sudo dpkg -i minikube_1.2.0.deb`

### RPM Package (.rpm) [Experimental]
Download the `minikube-1.2.0.rpm` file, and install it using `sudo rpm -i minikube-1.2.0.rpm`

### Windows [Experimental]
Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path.

### Windows Installer [Experimental]
Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage
Documentation is available [here](https://github.com/kubernetes/minikube/blob/v1.2.0/README.md).

## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
183d017d094b7783c938dc709dbdfc9a48f92299178234f89047dfbb083a592c

==> out/minikube-linux-amd64.sha256 <==
123fc9f5656333fb2927cf91666a91cd5b28ef97503418ac2a90a2109e518ed9

==> out/minikube-windows-amd64.exe.sha256 <==
f6c30cb88ec61bc6fe17532a3ef56e4f1fcef2473e3d73fc56f352b44784490d
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
ce7106ba22f5e6afc10595c5eac899780952ca402ba62fbeda0519283901b75c
```
