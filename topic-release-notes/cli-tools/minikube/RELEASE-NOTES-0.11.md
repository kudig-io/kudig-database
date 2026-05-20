---
title: minikube v0.11 Release Notes
description: minikube v0.11 Release Notes — Kubernetes 生产运维知识库
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
- minikube v0.11 Release Notes 是什么
- 如何 minikube v0.11 Release Notes
trigger_keywords:
- minikube
- v0.11
- Release
- Notes
- release
- notes
---

# minikube v0.11 Release Notes

Source: [v0.11.0](https://github.com/kubernetes/minikube/releases/tag/v0.11.0)

# Minikube v0.11.0

Minikube is still under active development, and features may change at any time. Release notes are available [here](https://github.com/kubernetes/minikube/blob/v0.11.0/CHANGELOG.md).

## Distribution

Minikube is distrubuted in binary form for Linux, OSX, and Windows systems for the v0.11.0 release. Please note that Windows support is currently experimental and may have issues.  Binaries are available through Github or on Google Cloud Storage. The direct GCS links are:
[Darwin/amd64](https://storage.googleapis.com/minikube/releases/v0.11.0/minikube-darwin-amd64)
[Linux/amd64](https://storage.googleapis.com/minikube/releases/v0.11.0/minikube-linux-amd64)
[Windows/amd64](https://storage.googleapis.com/minikube/releases/v0.11.0/minikube-windows-amd64.exe)

## Installation

### OSX

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.11.0/minikube-darwin-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Linux

``` shell
curl -Lo minikube https://storage.googleapis.com/minikube/releases/v0.11.0/minikube-linux-amd64 && chmod +x minikube && sudo mv minikube /usr/local/bin/
```

Feel free to leave off the `sudo mv minikube /usr/local/bin` if you would like to add minikube to your path manually.

### Debian Package (.deb) [Experimental]

Download the `minikube_0.11-0.deb` file, and install it using `sudo dpkg -i minikube_.deb`

### Windows [Experimental]

Download the `minikube-windows-amd64.exe` file, rename it to `minikube.exe` and add it to your path

## Usage

Documentation is available [here](https://github.com/kubernetes/minikube/blob/v0.11.0/README.md).

## Checksums

Minikube consists of a binary executable and a VM image in ISO format. To verify the contents of your distribution, you can compare sha256 hashes with these values:

```
$ tail -n +1 -- out/*.sha256
==> out/minikube-darwin-amd64.sha256 <==
7fe7ce35eda959d91071d065141d040053b945d5af0d57f98eb18afb93a4c921

==> out/minikube-linux-amd64.sha256 <==
f654835e3610fc746060ffdcdba5df68ca39231a4c6d5c6c9d9caffb10c25da1

==> out/minikube-windows-amd64.exe.sha256 <==
4ba3e7a08cf3cc2b0f50e454a48d67022e8ca78b9363cd743644eb7b5f03a3b5
```

### ISO

``` shell
$ openssl sha256 minikube.iso
SHA256(minikube.iso)= 
aadc8b6f5720d5a493a36e1f07f71bffb588780c76498d68cd761793d2ca344e
```
