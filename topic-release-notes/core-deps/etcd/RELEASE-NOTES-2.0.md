# etcd v2.0 Release Notes

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
./etcdctl set mykey "this is awesome"
./etcdctl get mykey
```

#### Docker

To get started with Docker on Linux run the following in a terminal:

```
docker run -p 4001:4001 -v /usr/share/ca-certificates/:/etc/ssl/certs quay.io/coreos/etcd:v2.0.13
```

#### ACI/Rocket

To get started with rkt on Linux run the following in a terminal:

```
rkt run coreos.com/etcd:v2.0.13
```
