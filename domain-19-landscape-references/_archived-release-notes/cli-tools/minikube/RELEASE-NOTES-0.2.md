---
title: minikube v0.2 Release Notes
description: minikube v0.2 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.2 Release Notes 是什么
- 如何 minikube v0.2 Release Notes
trigger_keywords:
- minikube
- v0.2
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
created: "2026-05-23"
---

# minikube v0.2 Release Notes

Source: [v0.2.0](https://github.com/kubernetes/minikube/releases/tag/v0.2.0)

# Minikube v0.2.0

This is the second release of Minikube. Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/master/CHANGELOG.md).

## [[Distribution|Distribution]]

Minikube is only distributed in binary form for Linux and OSX systems for the v0.2.0 release. Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.2.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.2.0/minikube-linux-amd64)

## Installation

### OSX

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.2.0/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Linux

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.2.0/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

## Usage

Documentation is available [here](https://github.com/kubernetes/minikube/blob/master/README.md).

## Checksums

Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare SHA1 hashes with these values:

### OSX

``` shell
$ openssl sha1 out/minikube-darwin-amd64
SHA1(out/minikube-darwin-amd64)= cadd06c75f6f25ef392d63f9141989cce5652d39
```

### Linux

``` shell
$ openssl sha1 out/minikube-linux-amd64
SHA1(out/minikube-linux-amd64)= 2d0461353700f8fc4583b7503386e0dada6aaf33
```

### ISO

``` shell
$ openssl sha1 deploy/iso/minikube.iso
SHA1(deploy/iso/minikube.iso)= c8d8a7399ae8f0bee9481d51250565e291833211
```
