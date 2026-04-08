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
