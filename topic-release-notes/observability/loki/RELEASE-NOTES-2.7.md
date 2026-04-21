# loki v2.7 Release Notes

Source: [v2.7.7](https://github.com/grafana/loki/releases/tag/v2.7.7)

This is release `v2.7.7` of Loki.

### Notable changes:
This release fixes a few vulnerabilities in Loki and our published images.

* Fix CVE-2023-1255, CVE-2023-2650, CVE-2023-2975, CVE-2023-3446, CVE-2023-3817, and CVE-2022-41721
* Upgrade base alpine image used in loki image to 3.18.2

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
```bash
$ docker pull "grafana/loki:2.7.7"
$ docker pull "grafana/promtail:2.7.7"
```

#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v2.7.7/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```