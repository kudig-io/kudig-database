---
title: minikube v0.22 Release Notes
description: minikube v0.22 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.22 Release Notes 是什么
- 如何 minikube v0.22 Release Notes
trigger_keywords:
- minikube
- v0.22
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# minikube v0.22 Release Notes

Source: [v0.22.3](https://github.com/kubernetes/minikube/releases/tag/v0.22.3)

# Minikube v0.22.3
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.22.3/CHANGELOG.md).

## Distribution
Minikube is distributed in binary form for Linux, OSX, and Windows systems for the v0.22.3 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through GitHub or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.22.3/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.22.3/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.22.3/minikube-windows-amd64.exe)

## Installation
### OSX
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.22.3/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

### Linux
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.22.3/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```
Feel free to leave off the ```sudo mv minikube /usr/local/bin``` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]
Download the `minikube_0.22-3.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]
Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path.

### Windows Installer [Experimental]
Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage
Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.22.3/README.md).

## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
62483f95b55fc14e6cd3898bbfdd637a0ad921e5f1a632b5e367d74af5e36f24

==> out/minikube-linux-amd64.sha256 <==
7d85e6ca06943376fe3235663857bf51b4d3fe0d59b6ef645821bf212301244b

==> out/minikube-windows-amd64.exe.sha256 <==
0a7037cb510bfbd9f78b85a80910a43bb8f6e7a9de89cd4323fea3ff52fce524
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
7f0766b20c237fbaa5bdbcb0a08dbc5cc839343b7862e882f2a9f79ac1c18826
```
