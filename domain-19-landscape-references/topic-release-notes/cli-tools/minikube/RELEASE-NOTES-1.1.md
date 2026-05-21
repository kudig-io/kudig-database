---
title: minikube v1.1 Release Notes
description: minikube v1.1 Release Notes — Kubernetes 生产运维知识库
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
- minikube v1.1 Release Notes 是什么
- 如何 minikube v1.1 Release Notes
trigger_keywords:
- minikube
- v1.1
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

# minikube v1.1 Release Notes

Source: [v1.1.1](https://github.com/kubernetes/minikube/releases/tag/v1.1.1)

# Minikube v1.1.1
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v1.1.1/CHANGELOG.md).

## Distribution
Minikube is distributed in binary form for Linux, OSX, and Windows systems for the v1.1.1 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through GitHub or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v1.1.1/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v1.1.1/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v1.1.1/minikube-windows-amd64.exe)

## Installation
### OSX
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v1.1.1/minikube-darwin-amd64 && chmod +x minikube && sudo cp minikube /usr/local/bin/ && rm minikube
```
Feel free to leave off ```sudo cp minikube /usr/local/bin/ && rm minikube``` if you would like to add minikube to your path manually.

Or you can install via homebrew with `brew cask install minikube`.

### Linux
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v1.1.1/minikube-linux-amd64 && chmod +x minikube && sudo cp minikube /usr/local/bin/ && rm minikube
```
Feel free to leave off ```sudo cp minikube /usr/local/bin/ && rm minikube``` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]
Download the `minikube_1.1.1.deb` file, and install it using `sudo dpkg -i minikube_1.1.1.deb`

### RPM Package (.rpm) [Experimental]
Download the `minikube-1.1.1.rpm` file, and install it using `sudo rpm -i minikube-1.1.1.rpm`

### Windows [Experimental]
Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path.

### Windows Installer [Experimental]
Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage
Documentation is available [here](https://github.com/kubernetes/minikube/blob/v1.1.1/README.md).

## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
cf6b2c0397147fa998a5c4e36ed7ad045d67f929daa3e46a1d01656b5ab7cac5

==> out/minikube-linux-amd64.sha256 <==
4596c0daabfe637912e2372c41cc11116ca053a1f7045af8a731265e45f9ca83

==> out/minikube-windows-amd64.exe.sha256 <==
e86e9c7c1c25cfd251f010b62e83fdf2ee2eefc3e73b4b1f7fa18971577150be
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
c619754dc33591833be6cd4499fc53cb8364df7dc9d4cea3901c12a642d1fd59
```
