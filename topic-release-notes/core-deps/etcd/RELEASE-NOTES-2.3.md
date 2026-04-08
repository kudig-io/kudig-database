# etcd v2.3 Release Notes

Source: [v2.3.8](https://github.com/etcd-io/etcd/releases/tag/v2.3.8)

Today we're announcing etcd v2.3.8. This is primarily a bug fix release, backward-compatible with all previous v2.3.0+ releases. Please read [NEWS](https://github.com/coreos/etcd/blob/master/NEWS) for highlighted changes.

Release signing key can be found at [coreos.com/security/app-signing-key](https://coreos.com/security/app-signing-key/).

##### Bug fixes
- [GH7282](https://github.com/coreos/etcd/pull/7282): snap: fix write snap

##### Getting started

###### Linux

```
ETCD_VER=v2.3.8
DOWNLOAD_URL=https://github.com/coreos/etcd/releases/download
curl -L ${DOWNLOAD_URL}/${ETCD_VER}/etcd-${ETCD_VER}-linux-amd64.tar.gz -o /tmp/etcd-${ETCD_VER}-linux-amd64.tar.gz
mkdir -p /tmp/test-etcd && tar xzvf /tmp/etcd-${ETCD_VER}-linux-amd64.tar.gz -C /tmp/test-etcd --strip-components=1

/tmp/test-etcd/etcd --version

Git SHA: 7e4fc7e
Go Version: go1.7.5
Go OS/Arch: linux/amd64
```

```
# start a local etcd server
/tmp/test-etcd/etcd

# write,read to etcd
/tmp/test-etcd/etcdctl --endpoints=localhost:2379 set foo "bar"
/tmp/test-etcd/etcdctl --endpoints=localhost:2379 get foo
```

###### Mac OS (Darwin)

```
ETCD_VER=v2.3.8
DOWNLOAD_URL=https://github.com/coreos/etcd/releases/download
curl -L ${DOWNLOAD_URL}/${ETCD_VER}/etcd-${ETCD_VER}-darwin-amd64.zip -o /tmp/etcd-${ETCD_VER}-darwin-amd64.zip
mkdir -p /tmp/test-etcd && unzip /tmp/etcd-${ETCD_VER}-darwin-amd64.zip -d /tmp && mv /tmp/etcd-${ETCD_VER}-darwin-amd64/* /tmp/test-etcd

/tmp/test-etcd/etcd --version
```

###### Docker

```
docker run --net=host \
    --name etcd-v2.3.8 \
    --volume=/tmp/etcd-data:/etcd-data \
    quay.io/coreos/etcd:v2.3.8 \
    /usr/local/bin/etcd \
    --name my-etcd-1 \
    --data-dir /etcd-data \
    --listen-client-urls http://0.0.0.0:2379 \
    --advertise-client-urls http://0.0.0.0:2379 \
    --listen-peer-urls http://0.0.0.0:2380 \
    --initial-advertise-peer-urls http://0.0.0.0:2380 \
    --initial-cluster my-etcd-1=http://0.0.0.0:2380 \
    --initial-cluster-token my-etcd-token \
    --initial-cluster-state new

docker exec etcd-v2.3.8 /bin/sh -c "/usr/local/bin/etcd -version"
docker exec etcd-v2.3.8 /bin/sh -c "/usr/local/bin/etcdctl version"

docker exec etcd-v2.3.8 /bin/sh -c "/usr/local/bin/etcdctl set foo bar"
docker exec etcd-v2.3.8 /bin/sh -c "/usr/local/bin/etcdctl get foo"
```

For more details, please check [Docker guide](https://github.com/coreos/etcd/blob/master/Documentation/op-guide/container.md#docker).
