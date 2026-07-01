---
title: minikube v0.14 Release Notes
description: minikube v0.14 Release Notes — Kubernetes 生产运维知识库
summary: minikube v0.14 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.14 Release Notes 是什么
- 如何 minikube v0.14 Release Notes
trigger_keywords:
- minikube
- v0.14
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---



# minikube v0.14 Release Notes

Source: [v0.14.0](https://github.com/kubernetes/minikube/releases/tag/v0.14.0)

# Minikube v0.14.0

Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.14.0/CHANGELOG.md).

## [[Distribution|Distribution]]

Minikube is distrubuted in binary form for Linux, OSX, and Windows systems for the v0.14.0 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.14.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.14.0/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.14.0/minikube-windows-amd64.exe)

## Installation

### OSX

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.14.0/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Linux

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.14.0/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]

Download the `minikube_0.14-0.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]

Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path

### Windows Installer [Experimental]

Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage

Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.14.0/README.md).

## Checksums

Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
950805a1e1929dc41ef27698a0325b981f99f4a45df121521861fcd696e35f28

==> out/minikube-linux-amd64.sha256 <==
a9e8423474de6046c672db5b035475fc034223652b081d1accba89af7da5a5e0

==> out/minikube-windows-amd64.exe.sha256 <==
5d6ef16842ea17aeaa715e5966edbc4f5ad220eaca15c070c1a191bb2360677b
```

### ISO

``` shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)= 
aadc8b6f5720d5a493a36e1f07f71bffb588780c76498d68cd761793d2ca344e
```
