---
title: loki v1.5 Release Notes
description: loki v1.5 Release Notes — Kubernetes 生产运维知识库
summary: loki v1.5 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- grafana
- helm
- docker
- mysql
- job
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 10min
intent_queries:
- loki v1.5 Release Notes 是什么
- 如何 loki v1.5 Release Notes
trigger_keywords:
- loki
- v1.5
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- monitoring-basics
- mysql-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# loki v1.5 Release Notes

Source: [v1.5.0](https://github.com/grafana/loki/releases/tag/v1.5.0)

It's been a busy month and a half since 1.4.0 was released, and a lot of new improvements have been added to Loki since!

Be prepared for some configuration changes that may cause some bumps when upgrading, 
we apologize for this but are always striving to reach the right compromise of code simplicity and user/operating experience. 

In this case we opted to keep a simplified configuration inline with [[Cortex|Cortex]] rather than a more complicated and error prone internal config mapping or difficult to implement support for multiple config names for the same feature.

This does result in breaking config changes for some configurations, however, these will fail fast and with the [list of diffs](https://cortexmetrics.io/docs/changelog/#config-file-breaking-changes) from the Cortex project should be quick to fix.

### Important Notes

**Be prepared for breaking config changes.**  Loki 1.5.0 vendors cortex [v1.0.1-0.20200430170006-3462eb63f324](https://github.com/cortexproject/cortex/commit/3462eb63f324c649bbaa122933bc591b710f4e48), 
there were substantial breaking config changes in Cortex 1.0 which standardized config options, and fixed typos.

**The Loki docker image user has changed to no longer be root**

Check the [upgrade guide](https://github.com/grafana/loki/blob/master/docs/operations/upgrade.md#150) for more detailed information on these changes.

### Notable Features and Fixes

There are quite a few we want to mention listed in order they were merged (mostly)

* [1837](https://github.com/grafana/loki/pull/1837) **sandeepsukhani**: flush boltdb to object store

This is perhaps the most exciting feature of 1.5.0, the first steps in removing a dependency on a separate index store!  This feature is still very new and experimental, however, we want this to be the future for Loki.  Only requiring just an object store.

If you want to test this new feature, and help us find any bugs, check out the [docs](文档/operations/storage/boltdb-shipper.md) to learn more and get started.

* [2073](https://github.com/grafana/loki/pull/2073) **slim-bean**: Loki: Allow configuring query_store_max_look_back_period when running a filesystem store and boltdb-shipper

This is even more experimental than the previous feature mentioned however also pretty exciting for Loki users who use the filesystem storage. We can leverage changes made in [1837](https://github.com/grafana/loki/pull/1837) to now allow Loki to run in a clustered mode with individual filesystem stores!

Please check out the last section in the [filesystem docs](文档/operations/storage/filesystem.md) for more details on how this works and how to use it!

* [2095](https://github.com/grafana/loki/pull/2095) **cyriltovena**: Adds backtick for the quoted string token lexer.

This will come as a big win to anyone who is writing complicated reqular expressions in either their Label matchers or Filter Expressions.  Starting now you can use the backtick to encapsulate your regex **and not have to do any escaping of special characters!!**

Examples:

```
{name="cassandra"} |~ `error=\w+`
{name!~`mysql-\d+`}
```

* [2055](https://github.com/grafana/loki/pull/2055) **aknuds1**: Chore: Fix spelling of per second in code

This is technically a breaking change for anyone who wrote code to processes the new statistics output in the query result added in 1.4.0, we apologize to anyone in this situation but if we don't fix this kind of error now it will be there forever.
And at the same time we didn't feel it was appropriate to make any major api revision changes for such a new feature and simple change.  We are always trying to use our best judgement in cases like this.

* [2031](https://github.com/grafana/loki/pull/2031) **cyriltovena**: Improve protobuf serialization

Thanks @cyriltovena for another big performance improvement in Loki, this time around protbuf's!

* [2021](https://github.com/grafana/loki/pull/2021) **slim-bean**: Loki: refactor validation and improve error messages
* [2012](https://github.com/grafana/loki/pull/2012) **slim-bean**: Loki: Improve logging and add metrics to streams dropped by stream limit

These two changes standardize the metrics used to report when a tenant hits a limit, now all discarded samples should be reported under `loki_discarded_samples_total` and you no longer need to also reference `cortex_discarded_samples_total`.
Additionally error messages were improved to help clients take better action when hitting limits.

* [1970](https://github.com/grafana/loki/pull/1970) **cyriltovena**: Allow to aggregate binary operations.

Another nice improvement to the query language which allows queries like this to work now:

```
sum by (job) (count_over_time({namespace="tns"}[5m] |= "level=error") / count_over_time({namespace="tns"}[5m]))
```

* [1713](https://github.com/grafana/loki/pull/1713) **adityacs**: Log error message for invalid checksum

In the event something went wrong with a stored chunk, rather than fail the query we ignore the chunk and return the rest.

* [2066](https://github.com/grafana/loki/pull/2066) **slim-bean**: Promtail: metrics stage can also count line bytes

This is a nice extension to a previous feature which let you add a metric to count log lines per stream, you can now count log bytes per stream.

Check out [this example](文档/clients/promtail/configuration.md#counter) to configure this in your promtail pipelines.

* [1935](https://github.com/grafana/loki/pull/1935) **cyriltovena**: Support stdin target via flag instead of automatic detection.

Third times a charm!  With 1.4.0 we allowed sending logs directly to promtail via stdin, with 1.4.1 we released a patch for this feature which wasn't detecting stdin correctly on some operating systems.
Unfortunately after a few more bug reports it seems this change caused some more undesired side effects so we decided to not try to autodetect stdin at all, instead now you must pass the `--stdin` flag if you want Promtail to listen for logs on stdin.

* [2076](https://github.com/grafana/loki/pull/2076) **cyriltovena**: Allows to pass inlined pipeline stages to the docker driver.
* [1906](https://github.com/grafana/loki/pull/1906) **cyriltovena**: Add no-file and keep-file log option for docker driver.

The docker logging driver received a couple very nice updates, it's always been challenging to configure pipeline stages for the docker driver, with the first PR there are now a few easier ways to do this!
In the second PR we added config options to control keeping any log files on the host when using the docker logging driver, allowing you to run with no disk access if you would like, as well as allowing you to control keeping log files available after container restarts.

* [1864](https://github.com/grafana/loki/pull/1864) **cyriltovena**: Sign helm package with GPG.

We now GPG sign helm packages!

### All Changes

Too many to list here, see the [CHANGELOG](https://github.com/grafana/loki/blob/v1.5.0/CHANGELOG.md#all-changes) for a full list of changes!

### Installation:
The components of Loki are currently distributed in plain binary form and as Docker container images. Choose what fits your use-case best.

#### Docker container:
* https://hub.docker.com/r/grafana/loki
* https://hub.docker.com/r/grafana/promtail
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
$ docker pull "grafana/loki:1.5.0"
$ docker pull "grafana/promtail:1.5.0"
```
#### Binary
We provide pre-compiled binary executables for the most common operating systems and architectures.
Choose from the assets below for the application and architecture matching your system.
Example for `Loki` on the `linux` operating system and `amd64` architecture:

```bash
$ curl -O -L "https://github.com/grafana/loki/releases/download/v1.5.0/loki-linux-amd64.zip"
# extract the binary
$ unzip "loki-linux-amd64.zip"
# make sure it is executable
$ chmod a+x "loki-linux-amd64"
```

<!-- risk-assessed -->
