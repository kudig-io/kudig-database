---
title: etcd v0.4 Release Notes
description: etcd v0.4 Release Notes — Kubernetes 生产运维知识库
summary: etcd v0.4 Release Notes — Kubernetes 生产运维知识库
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
- etcd v0.4 Release Notes 是什么
- 如何 etcd v0.4 Release Notes
trigger_keywords:
- etcd
- v0.4
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




# [[etcd|etcd]] v0.4 Release Notes

Source: [v0.4.9](https://github.com/etcd-io/etcd/releases/tag/v0.4.9)

## Changelog
- new `/v2/migration/snapshot` endpoint to support creating point-in-time snapshot.
  the snapshot will be returned in HTTP body be default
  the snapshot will be saved under data-dir if `disk=true`
- documentation about default value of --bind-addr and --peer-bind-addr is fixed

### Getting Started

#### OS X

To get started on OSX run the following in a terminal:

```
curl -L  https://github.com/coreos/etcd/releases/download/v0.4.9/etcd-v0.4.9-darwin-amd64.zip -o etcd-v0.4.9-darwin-amd64.zip.
unzip etcd-v0.4.9-darwin-amd64.zip.
cd etcd-v0.4.9-darwin-amd64
./etcd
```

Open another terminal:

```
# 🟢 低风险：只读/信息收集，通常无副作用
# Press enter to background etcd
./etcdctl set mykey "this is awesome"
./etcdctl get mykey
```
#### Linux

To get started on Linux run the following in a terminal:

```
curl -L  https://github.com/coreos/etcd/releases/download/v0.4.9/etcd-v0.4.9-linux-amd64.tar.gz -o etcd-v0.4.9-linux-amd64.tar.gz
tar xzvf etcd-v0.4.9-linux-amd64.tar.gz
cd etcd-v0.4.9-linux-amd64
./etcd
```

Open another terminal:

```
# 🟢 低风险：只读/信息收集，通常无副作用
# Press enter to background etcd
./etcdctl set mykey "this is awesome"
./etcdctl get mykey
```
#### Docker

To get started with Docker on Linux run the following in a terminal:

```
# 🟢 低风险：只读/信息收集，通常无副作用
docker run -p 4001:4001 -v /etc/ssl/certs/:/etc/ssl/certs/  quay.io/coreos/etcd:v0.4.9
```
Open another terminal:

```
# 🟢 低风险：只读/信息收集，通常无副作用
docker run --net=host quay.io/coreos/etcd:v0.4.9 /etcdctl set mykey "this is awesome"
docker run --net=host quay.io/coreos/etcd:v0.4.9 /etcdctl get mykey
```

<!-- risk-assessed -->
