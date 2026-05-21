---
title: minikube v0.17 Release Notes
description: minikube v0.17 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.17 Release Notes 是什么
- 如何 minikube v0.17 Release Notes
trigger_keywords:
- minikube
- v0.17
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# minikube v0.17 Release Notes

Source: [v0.17.1](https://github.com/kubernetes/minikube/releases/tag/v0.17.1)

# Minikube v0.17.1

Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.17.1/CHANGELOG.md).

## Distribution

Minikube is distrubuted in binary form for Linux, OSX, and Windows systems for the v0.17.1 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.17.1/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.17.1/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.17.1/minikube-windows-amd64.exe)

## Installation

### OSX

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.17.1/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Linux

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.17.1/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]

Download the `minikube_0.17-1.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]

Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path

### Windows Installer [Experimental]

Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage

Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.17.1/README.md).

## Checksums

Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
b175c355d377a6ce2fefdd19201c865a7e628581261ac949fffb725af459c389

==> out/minikube-linux-amd64.sha256 <==
54f9e24b5622f540a6d5edd7450ce546cf6f57f9feff21fd5d92d0d2f552ac31

==> out/minikube-windows-amd64.exe.sha256 <==
86a713ced29399c736d14bf19be7aead96a22b03374441d99a457a4c44df9d53
```

### ISO

``` shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
ea6777fc2782ce151c6e755193dca5ef2d08dee761d3f8c8466e2b8fe2da0e10
```
