---
title: minikube v0.9 Release Notes
description: minikube v0.9 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.9 Release Notes 是什么
- 如何 minikube v0.9 Release Notes
trigger_keywords:
- minikube
- v0.9
- Release
- Notes
- release
- notes
---

# minikube v0.9 Release Notes

Source: [v0.9.0](https://github.com/kubernetes/minikube/releases/tag/v0.9.0)

# Minikube v0.9.0

Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.9.0/CHANGELOG.md).

## Distribution

Minikube is distrubuted in binary form for Linux, OSX, and Windows systems for the v0.9.0 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.9.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.9.0/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.9.0/minikube-windows-amd64.exe)

## Installation

### OSX

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.9.0/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Linux

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.9.0/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Windows [Experimental]

Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path

## Usage

Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.9.0/README.md).

## Checksums

Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
4415e7f3a30ace7cf0e5ad023f979509e6b25ded1bfb55368f918fc25098307e

==> out/minikube-linux-amd64.sha256 <==
76b341aa377c274c7147e203483f117ef01151a8e33bb232e29872ad20f6effc

==> out/minikube-windows-amd64.exe.sha256 <==
98cb96fef863c9a4acac210eea910c2d239a87b13a6e2ac10ffe35244880e203
```

### ISO

``` shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)= 
65dd4fda45f4a13f14e1f81f8ea8491d6ed07468da77d34f78e4b6cd6a3cc95e
```
