# loki v2.6 Release Notes

Source: [v2.6.1](https://github.com/grafana/loki/releases/tag/v2.6.1)

Loki 2.6.1 is a patch fix release on [2.6.0](https://github.com/grafana/loki/releases/tag/v2.6.0)

### Notable changes:
- [PR 6658](https://github.com/grafana/loki/pull/6658) **JordanRushing**: Updated the versions of [dskit](https://github.com/grafana/dskit) and [memberlist](https://github.com/grafana/memberlist) to allow configuring cluster labels for memberlist. Cluster labels prevent mixing the members between two consistent hash rings of separate applications that are run on the same Kubernetes cluster.
- [PR 6681](https://github.com/grafana/loki/pull/6681) **MasslessParticle** Fixed an HTTP connection leak between the querier and the compactor when the log entry deletion feature is enabled.
- [PR 6583](https://github.com/grafana/loki/pull/6583) **MasslessParticle** Fixed noisy error messages when the log entry deletion feature is disabled for a tenant 



### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
```bash
$ docker pull "grafana/loki:2.6.1"
$ docker pull "grafana/promtail:2.6.1"
```

#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v2.6.1/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```