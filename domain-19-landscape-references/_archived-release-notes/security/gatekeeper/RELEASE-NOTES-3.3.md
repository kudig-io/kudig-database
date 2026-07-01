---
title: etcd v3.3 Release Notes
description: etcd v3.3 Release Notes — Kubernetes 生产运维知识库
summary: etcd v3.3 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- etcd
- docker
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- etcd v3.3 Release Notes 是什么
- 如何 etcd v3.3 Release Notes
trigger_keywords:
- etcd
- v3.3
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- etcd-basics
---



# [[etcd|etcd]] v3.3 Release Notes

Source: [v3.3.27](https://github.com/etcd-io/etcd/releases/tag/v3.3.27)

Please check out CHANGELOG](https://github.com/etcd-io/etcd/blob/master/CHANGELOG-3.3.md) for a full list of changes. And make sure to read [upgrade guide](https://etcd.io/docs/v3.4.0/upgrades/upgrade_3_3/) before upgrading etcd (there may be breaking changes).

For installation guides, please check out [play.etcd.io](http://play.etcd.io) and [operating etcd](https://github.com/etcd-io/etcd/tree/master/Documentation#operating-etcd-clusters). Latest support status for common architectures and operating systems can be found at [supported platforms](https://github.com/etcd-io/etcd/blob/master/Documentation/op-guide/supported-platform.md).

###### Linux

```bash
ETCD_VER=v3.3.27

# choose either URL
GOOGLE_URL=https://storage.googleapis.com/etcd
GITHUB_URL=https://github.com/etcd-io/etcd/releases/download
DOWNLOAD_URL=${GOOGLE_URL}

rm -f /tmp/etcd-${ETCD_VER}-linux-amd64.tar.gz
rm -rf /tmp/etcd-download-test && mkdir -p /tmp/etcd-download-test

curl -L ${DOWNLOAD_URL}/${ETCD_VER}/etcd-${ETCD_VER}-linux-amd64.tar.gz -o /tmp/etcd-${ETCD_VER}-linux-amd64.tar.gz
tar xzvf /tmp/etcd-${ETCD_VER}-linux-amd64.tar.gz -C /tmp/etcd-download-test --strip-components=1
rm -f /tmp/etcd-${ETCD_VER}-linux-amd64.tar.gz

/tmp/etcd-download-test/etcd --version
ETCDCTL_API=3 /tmp/etcd-download-test/etcdctl version
```

```bash
# start a local etcd server
/tmp/etcd-download-test/etcd

# write,read to etcd
ETCDCTL_API=3 /tmp/etcd-download-test/etcdctl --endpoints=localhost:2379 put foo bar
ETCDCTL_API=3 /tmp/etcd-download-test/etcdctl --endpoints=localhost:2379 get foo
```

###### macOS (Darwin)

```bash
ETCD_VER=v3.3.27

# choose either URL
GOOGLE_URL=https://storage.googleapis.com/etcd
GITHUB_URL=https://github.com/etcd-io/etcd/releases/download
DOWNLOAD_URL=${GOOGLE_URL}

rm -f /tmp/etcd-${ETCD_VER}-darwin-amd64.zip
rm -rf /tmp/etcd-download-test && mkdir -p /tmp/etcd-download-test

curl -L ${DOWNLOAD_URL}/${ETCD_VER}/etcd-${ETCD_VER}-darwin-amd64.zip -o /tmp/etcd-${ETCD_VER}-darwin-amd64.zip
unzip /tmp/etcd-${ETCD_VER}-darwin-amd64.zip -d /tmp && rm -f /tmp/etcd-${ETCD_VER}-darwin-amd64.zip
mv /tmp/etcd-${ETCD_VER}-darwin-amd64/* /tmp/etcd-download-test && rm -rf mv /tmp/etcd-${ETCD_VER}-darwin-amd64

/tmp/etcd-download-test/etcd --version
ETCDCTL_API=3 /tmp/etcd-download-test/etcdctl version
```

###### Docker

etcd uses [`gcr.io/etcd-development/etcd`](https://gcr.io/etcd-development/etcd) as a primary container registry, and [`quay.io/coreos/etcd`](https://quay.io/coreos/etcd) as secondary.

```bash
rm -rf /tmp/etcd-data.tmp && mkdir -p /tmp/etcd-data.tmp && \
  docker rmi gcr.io/etcd-development/etcd:v3.3.27 || true && \
  docker run \
  -p 2379:2379 \
  -p 2380:2380 \
  --mount type=bind,source=/tmp/etcd-data.tmp,destination=/etcd-data \
  --name etcd-gcr-v3.3.27 \
  gcr.io/etcd-development/etcd:v3.3.27 \
  /usr/local/bin/etcd \
  --name s1 \
  --data-dir /etcd-data \
  --listen-client-urls http://0.0.0.0:2379 \
  --advertise-client-urls http://0.0.0.0:2379 \
  --listen-peer-urls http://0.0.0.0:2380 \
  --initial-advertise-peer-urls http://0.0.0.0:2380 \
  --initial-cluster s1=http://0.0.0.0:2380 \
  --initial-cluster-token tkn \
  --initial-cluster-state new \
  --log-level info \
  --logger zap \
  --log-outputs stderr

docker exec etcd-gcr-v3.3.27 /bin/sh -c "/usr/local/bin/etcd --version"
docker exec etcd-gcr-v3.3.27 /bin/sh -c "ETCDCTL_API=3 /usr/local/bin/etcdctl version"
docker exec etcd-gcr-v3.3.27 /bin/sh -c "ETCDCTL_API=3 /usr/local/bin/etcdctl endpoint health"
docker exec etcd-gcr-v3.3.27 /bin/sh -c "ETCDCTL_API=3 /usr/local/bin/etcdctl put foo bar"
docker exec etcd-gcr-v3.3.27 /bin/sh -c "ETCDCTL_API=3 /usr/local/bin/etcdctl get foo"
```