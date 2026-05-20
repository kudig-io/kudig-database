---
title: etcd v0.3 Release Notes
description: etcd v0.3 Release Notes — Kubernetes 生产运维知识库
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
- etcd v0.3 Release Notes 是什么
- 如何 etcd v0.3 Release Notes
trigger_keywords:
- etcd
- v0.3
- Release
- Notes
- release
- notes
---

# etcd v0.3 Release Notes

Source: [v0.3.0](https://github.com/etcd-io/etcd/releases/tag/v0.3.0)

### Changelog

For full details see the [0.3.0 blog post](http://coreos.com/blog/etcd-0.3.0-released/).

### Getting Started

#### CoreOS / Docker

To run it it in a docker container on CoreOS:

``` sh
docker run -i -t -p 4002:4001 coreos/etcd
```

```
curl -L http://127.0.0.1:4002/v2/keys/mykey -XPUT -d value="this is awesome"
curl -L http://127.0.0.1:4002/v2/keys/mykey
```

#### OS X

To get started on OSX run the following in a terminal:

```
curl -L  https://github.com/coreos/etcd/releases/download/v0.3.0/etcd-v0.3.0-darwin-amd64.zip -o etcd-v0.3.0-darwin-amd64.zip 
unzip etcd-v0.3.0-darwin-amd64.zip 
cd etcd-v0.3.0-darwin-amd64
./etcd
```

Open another terminal:

```
# Press enter to background etcd
./etcdctl set mykey "this is awesome"
./etcdctl get mykey
```
