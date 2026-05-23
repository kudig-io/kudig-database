---
title: etcd v3.0 Release Notes
description: etcd v3.0 Release Notes — Kubernetes 生产运维知识库
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
- etcd v3.0 Release Notes 是什么
- 如何 etcd v3.0 Release Notes
trigger_keywords:
- etcd
- v3.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
created: "2026-05-23"
---

# [[etcd|etcd]] v3.0 Release Notes

Source: [v3.0.17](https://github.com/etcd-io/etcd/releases/tag/v3.0.17)

Today we're announcing etcd v3.0.17. This is primarily a bug fix release, backward-compatible with all previous v3.0.0+ releases. Please read [NEWS](https://github.com/coreos/etcd/blob/master/NEWS) for highlighted changes.

Release signing key can be found at [coreos.com/security/app-signing-key](https://coreos.com/security/app-signing-key/).

##### Bug fixes
- [GH6085](https://github.com/coreos/etcd/pull/6085): etcdserver, lease: tie lease min ttl to election timeout
- [GH7203](https://github.com/coreos/etcd/pull/7203): etcdctlv3: snapshot restore works with lease key

##### Getting started

###### Linux

```
ETCD_VER=v3.0.17
DOWNLOAD_URL=https://github.com/coreos/etcd/releases/download
curl -L ${DOWNLOAD_URL}/${ETCD_VER}/etcd-${ETCD_VER}-linux-amd64.tar.gz -o /tmp/etcd-${ETCD_VER}-linux-amd64.tar.gz
mkdir -p /tmp/test-etcd && tar xzvf /tmp/etcd-${ETCD_VER}-linux-amd64.tar.gz -C /tmp/test-etcd --strip-components=1

/tmp/test-etcd/etcd --version

Git SHA: cc198e2
Go Version: go1.6.4
Go OS/Arch: linux/amd64
```

```
# start a local etcd server
/tmp/test-etcd/etcd

# write,read to etcd
ETCDCTL_API=3 /tmp/test-etcd/etcdctl --endpoints=localhost:2379 put foo "bar"
ETCDCTL_API=3 /tmp/test-etcd/etcdctl --endpoints=localhost:2379 get foo
```

###### Mac OS (Darwin)

```
ETCD_VER=v3.0.17
DOWNLOAD_URL=https://github.com/coreos/etcd/releases/download
curl -L ${DOWNLOAD_URL}/${ETCD_VER}/etcd-${ETCD_VER}-darwin-amd64.zip -o /tmp/etcd-${ETCD_VER}-darwin-amd64.zip
mkdir -p /tmp/test-etcd && unzip /tmp/etcd-${ETCD_VER}-darwin-amd64.zip -d /tmp && mv /tmp/etcd-${ETCD_VER}-darwin-amd64/* /tmp/test-etcd

/tmp/test-etcd/etcd --version
```

##### Run in containers

###### rkt

```
RKT_VERSION=v1.23.0

GITHUB_URL=https://github.com/coreos/rkt/releases/download

DOWNLOAD_URL=${GITHUB_URL}

rm -f /tmp/rkt-${RKT_VERSION}.tar.gz
rm -rf /tmp/test-rkt-${RKT_VERSION} && mkdir -p /tmp/test-rkt-${RKT_VERSION}

curl -L ${DOWNLOAD_URL}/${RKT_VERSION}/rkt-${RKT_VERSION}.tar.gz -o /tmp/rkt-${RKT_VERSION}.tar.gz
tar xzvf /tmp/rkt-${RKT_VERSION}.tar.gz -C /tmp/test-rkt-${RKT_VERSION} --strip-components=1

# sudo cp /tmp/test-rkt-${RKT_VERSION}/rkt /usr/local/bin
sudo cp /tmp/test-rkt-${RKT_VERSION}/rkt /

/rkt version


sudo /rkt \
    --trust-keys-from-https \
    run \
    --stage1-name coreos.com/rkt/stage1-fly:1.23.0 \
    quay.io/coreos/etcd:v3.0.17 \
    --exec=/bin/sh -- -c "export ETCDCTL_API=3 && /usr/local/bin/etcdctl version"

sudo rm -rf /tmp/etcd-data
sudo mkdir -p /tmp/etcd-data
sudo chown -R root:$(whoami) /tmp/etcd-data
sudo chmod -R a+rw /tmp/etcd-data

sudo /rkt \
    --trust-keys-from-https \
    run \
    --stage1-name coreos.com/rkt/stage1-fly:1.23.0 \
    --net=host \
    --volume etcd-data-dir,kind=host,source=/tmp/etcd-data \
    --mount volume=etcd-data-dir,target=/tmp/etcd-data \
    quay.io/coreos/etcd:v3.0.17 -- \
    --name my-etcd-1 \
    --data-dir /tmp/etcd-data \
    --listen-client-urls http://localhost:2379 \
    --advertise-client-urls http://localhost:2379 \
    --listen-peer-urls http://localhost:2380 \
    --initial-advertise-peer-urls http://localhost:2380 \
    --initial-cluster my-etcd-1=http://localhost:2380 \
    --initial-cluster-token my-etcd-token \
    --initial-cluster-state new \
    --auto-compaction-retention 1

sudo /rkt \
    --trust-keys-from-https \
    run \
    --stage1-name coreos.com/rkt/stage1-fly:1.23.0 \
    quay.io/coreos/etcd:v3.0.17 \
    --exec=/bin/sh -- -c "export ETCDCTL_API=3 && /usr/local/bin/etcdctl put foo bar"
```

For more details, please check [rkt commands](https://github.com/coreos/rkt/blob/master/Documentation/commands.md#rkt-run).

###### Docker

```
docker run --name etcd quay.io/coreos/etcd:v3.0.17
```

For more details, please check [Docker guide](https://github.com/coreos/etcd/blob/master/Documentation/op-guide/container.md#docker).
