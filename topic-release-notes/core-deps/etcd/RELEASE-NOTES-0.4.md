# etcd v0.4 Release Notes

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
# Press enter to background etcd
./etcdctl set mykey "this is awesome"
./etcdctl get mykey
```

#### Docker

To get started with Docker on Linux run the following in a terminal:

```
docker run -p 4001:4001 -v /etc/ssl/certs/:/etc/ssl/certs/  quay.io/coreos/etcd:v0.4.9
```

Open another terminal:

```
docker run --net=host quay.io/coreos/etcd:v0.4.9 /etcdctl set mykey "this is awesome"
docker run --net=host quay.io/coreos/etcd:v0.4.9 /etcdctl get mykey
```
