# etcd v2.1 Release Notes

Source: [v2.1.3](https://github.com/etcd-io/etcd/releases/tag/v2.1.3)

### Changelog
- [[GH 3378](https://github.com/coreos/etcd/pull/3378)] when invalid TLS files are provided etcd logs a helpful error message and shuts down cleanly.

### Getting Started

#### OS X

To get started on OSX run the following in a terminal:

```
curl -L  https://github.com/coreos/etcd/releases/download/v2.1.3/etcd-v2.1.3-darwin-amd64.zip -o etcd-v2.1.3-darwin-amd64.zip
unzip etcd-v2.1.3-darwin-amd64.zip
cd etcd-v2.1.3-darwin-amd64
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
curl -L  https://github.com/coreos/etcd/releases/download/v2.1.3/etcd-v2.1.3-linux-amd64.tar.gz -o etcd-v2.1.3-linux-amd64.tar.gz
tar xzvf etcd-v2.1.3-linux-amd64.tar.gz
cd etcd-v2.1.3-linux-amd64
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
docker run -p 2379:2379 -v /usr/share/ca-certificates/:/etc/ssl/certs quay.io/coreos/etcd:v2.1.3
```

#### ACI/Rocket

To get started with Rocket on Linux run the following in a terminal:

```
rkt run coreos.com/etcd:v2.1.3
```
