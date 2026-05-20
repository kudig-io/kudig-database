---
title: minikube v1.0 Release Notes
description: minikube v1.0 Release Notes — Kubernetes 生产运维知识库
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
- minikube v1.0 Release Notes 是什么
- 如何 minikube v1.0 Release Notes
trigger_keywords:
- minikube
- v1.0
- Release
- Notes
- release
- notes
---

# minikube v1.0 Release Notes

Source: [v1.0.1](https://github.com/kubernetes/minikube/releases/tag/v1.0.1)

# Minikube v1.0.1
Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v1.0.1/CHANGELOG.md).

## Distribution
Minikube is distributed in binary form for Linux, OSX, and Windows systems for the v1.0.1 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through GitHub or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v1.0.1/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v1.0.1/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v1.0.1/minikube-windows-amd64.exe)

## Installation
### OSX
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v1.0.1/minikube-darwin-amd64 && chmod +x minikube && sudo cp minikube /usr/local/bin/ && rm minikube
```
Feel free to leave off ```sudo cp minikube /usr/local/bin/ && rm minikube``` if you would like to add minikube to your path manually.

Or you can install via homebrew with `brew cask install minikube`.

### Linux
```shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v1.0.1/minikube-linux-amd64 && chmod +x minikube && sudo cp minikube /usr/local/bin/ && rm minikube
```
Feel free to leave off ```sudo cp minikube /usr/local/bin/ && rm minikube``` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]
Download the `minikube_1.0.1.deb` file, and install it using `sudo dpkg -i minikube_1.0.1.deb`

### RPM Package (.rpm) [Experimental]
Download the `minikube-1.0.1.rpm` file, and install it using `sudo rpm -i minikube-1.0.1.rpm`

### Windows [Experimental]
Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path.

### Windows Installer [Experimental]
Download the `minikube-installer.exe` file, and execute the installer.  This will automatically add minikube.exe to your path with an uninstaller available as well.

## Usage
Documentation is available [here](https://github.com/kubernetes/minikube/blob/v1.0.1/README.md).

## Checksums
Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
0af8a3f582c9284ffe10e99444b60a75241325f2bc9ab43ec758802f2b89e1db

==> out/minikube-linux-amd64.sha256 <==
7b56374955990ef2dd0289e6ecb62cf2b4587cab2b481d95f58de5db56799868

==> out/minikube-windows-amd64.exe.sha256 <==
58abb5fb3e694a451102963e04ce13ea0cea46b7bf5c7947f40fdfc673282ac9
```

### ISO
```shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)=
72e6867d375ddad669629283b6506d77cca944eca145671b679322d9f0d8c395
```
