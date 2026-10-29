---
title: etcd v2.0 Release Notes
description: etcd v2.0 Release Notes — Kubernetes 生产运维知识库
summary: etcd v2.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- docker
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- etcd v2.0 Release Notes 是什么
- 如何 etcd v2.0 Release Notes
trigger_keywords:
- etcd
- v2.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[etcd|etcd]] v2.0 Release Notes

Source: [v2.0.13](https://github.com/etcd-io/etcd/releases/tag/v2.0.13)

### Changelog
- [GH 3030] The -advertise-client-urls flag is no longer required if falling back to proxy mode when discovery, or using read-only proxy mode.

This bug only affected the initial cluster bootstrapping and incorrectly required the `-advertise-client-urls` flag to be set in certain circumstances. There is no problem for existing clusters that have already been started.

### Getting Started

#### OS X

To get started on OSX run the following in a terminal:

```
curl -L  https://github.com/coreos/etcd/releases/download/v2.0.13/etcd-v2.0.13-darwin-amd64.zip -o etcd-v2.0.13-darwin-amd64.zip
unzip etcd-v2.0.13-darwin-amd64.zip
cd etcd-v2.0.13-darwin-amd64
./etcd
```

Open another terminal:

```
# 🟢 低风险：只读/信息收集，通常无副作用
./etcdctl set mykey "this is awesome"
./etcdctl get mykey
```
#### Linux

To get started on Linux run the following in a terminal:

```
curl -L  https://github.com/coreos/etcd/releases/download/v2.0.13/etcd-v2.0.13-linux-amd64.tar.gz -o etcd-v2.0.13-linux-amd64.tar.gz
tar xzvf etcd-v2.0.13-linux-amd64.tar.gz
cd etcd-v2.0.13-linux-amd64
./etcd
```

Open another terminal:

```
# 🟢 低风险：只读/信息收集，通常无副作用
./etcdctl set mykey "this is awesome"
./etcdctl get mykey
```
#### Docker

To get started with Docker on Linux run the following in a terminal:

```
# 🟢 低风险：只读/信息收集，通常无副作用
docker run -p 4001:4001 -v /usr/share/ca-certificates/:/etc/ssl/certs quay.io/coreos/etcd:v2.0.13
```
#### ACI/Rocket

To get started with rkt on Linux run the following in a terminal:

```
rkt run coreos.com/etcd:v2.0.13
```


<!-- risk-assessed -->
