# loki v2.4 Release Notes

Source: [v2.4.2](https://github.com/grafana/loki/releases/tag/v2.4.2)

## Loki 2.4.2

Loki 2.4.2 is a patch fix release on 2.4.x

### Defaults changes

2.4.2 makes the following changes to Loki defaults to improve usability, see [PR 5077](https://github.com/grafana/loki/pull/5077):

| config | new default | old default |
| --- | --- | --- |
| parallelise_shardable_queries | true | false |
| split_queries_by_interval | 30m | 0s |
| query_ingesters_within | 3h | 0s |
| max_chunk_age | 2h | 1h |
| max_concurrent | 10 | 20 |

### Bug fixes

2.4.2 fixes these bugs:

- [PR 4968](https://github.com/grafana/loki/pull/4968) **trevorwhitney**: Fixes a bug in which querying ingesters wrongly returns a ruler,
causing the internal server error `code = Unimplemented`.
- [PR 4875](https://github.com/grafana/loki/pull/4875) **trevorwhitney**: Honor the replication factor specified in the common configuration block when `memberlist` is the consistent hash ring store.
- [PR 4792](https://github.com/grafana/loki/pull/4792) **AndreZiviani**: Corrects the default values of configuration options in the documentation for:
    - `scheduler_dns_lookup_period` 
    - `min_ready_duration` 
    - `final_sleep` 
    - `max_transfer_retries` 
    - `chunk_retain_period` 
    - `chunk_target_size` 
    - `batch_size` 
    - `timeout` (for Redis requests) 

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
```bash
$ docker pull "grafana/loki:2.4.2"
$ docker pull "grafana/promtail:2.4.2"
```

#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v2.4.2/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```