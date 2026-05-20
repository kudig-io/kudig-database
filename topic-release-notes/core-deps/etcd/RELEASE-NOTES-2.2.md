---
title: etcd v2.2 Release Notes
description: etcd v2.2 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- etcd v2.2 Release Notes 是什么
- 如何 etcd v2.2 Release Notes
trigger_keywords:
- etcd
- v2.2
- Release
- Notes
- release
- notes
---

# etcd v2.2 Release Notes

Source: [v2.2.5](https://github.com/etcd-io/etcd/releases/tag/v2.2.5)

### Changelog
- [[GH 3830](https://github.com/coreos/etcd/pull/3830)] Godeps: update boltdb to fix arm64 build
- [[GH 4215](https://github.com/coreos/etcd/pull/4215)] etcdmain: fix proxy srv lookup
- [[GH 4254](https://github.com/coreos/etcd/pull/4254)] client: do not timeout when wait is true
- [[GH 4281](https://github.com/coreos/etcd/pull/4281)] etcdserver, auth: not cache a flag of auth status
- update gRPC dependencies (git SHA [`e29d659177655e589850ba7d3d83f7ce12ef23dd`](https://github.com/grpc/grpc-go/commit/e29d659177655e589850ba7d3d83f7ce12ef23dd))

### Getting Started

#### OS X

To get started on OSX run the following in a terminal:

```
curl -L  https://github.com/coreos/etcd/releases/download/v2.2.5/etcd-v2.2.5-darwin-amd64.zip -o etcd-v2.2.5-darwin-amd64.zip
unzip etcd-v2.2.5-darwin-amd64.zip
cd etcd-v2.2.5-darwin-amd64
./etcd
```

Open another terminal:

```
./etcdctl set mykey "this is awesome"
./etcdctl get mykey
```

#### Linux

To get started on Linux run the following in a terminal:

```
curl -L  https://github.com/coreos/etcd/releases/download/v2.2.5/etcd-v2.2.5-linux-amd64.tar.gz -o etcd-v2.2.5-linux-amd64.tar.gz
tar xzvf etcd-v2.2.5-linux-amd64.tar.gz
cd etcd-v2.2.5-linux-amd64
./etcd
```

Open another terminal:

```
./etcdctl set mykey "this is awesome"
./etcdctl get mykey
```

#### Docker

To get started with Docker on Linux run the following in a terminal:

```
docker run --name etcd quay.io/coreos/etcd:v2.2.5
docker exec etcd /etcdctl set foo bar
```

For advanced usage, please check [our docker guide](https://github.com/coreos/etcd/blob/master/Documentation/docker_guide.md).

#### ACI/rkt

To get started with rkt on Linux run the following in a terminal:

```
# for more info about rkt command line, see related doc at https://github.com/coreos/rkt/blob/master/Documentation/commands.md#rkt-run
rkt run --volume data-dir,kind=host,source=/tmp --mds-register=false coreos.com/etcd:v2.2.5
```
