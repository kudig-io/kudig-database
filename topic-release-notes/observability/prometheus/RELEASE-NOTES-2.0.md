# prometheus v2.0 Release Notes

Source: [v2.0.0](https://github.com/prometheus/prometheus/releases/tag/v2.0.0)

This release includes a completely rewritten storage, huge performance
improvements, but also many backwards incompatible changes. For more
information, read the announcement blog post and migration guide.

https://prometheus.io/blog/2017/11/08/announcing-prometheus-2-0/
https://prometheus.io/docs/prometheus/2.0/migration/

* [CHANGE] Completely rewritten storage layer, with WAL. This is not backwards compatible with 1.x storage, and many flags have changed/disappeared.
* [CHANGE] New staleness behavior. Series now marked stale after target scrapes no longer return them, and soon after targets disappear from service discovery.
* [CHANGE] Rules files use YAML syntax now. Conversion tool added to promtool.
* [CHANGE] Removed `count_scalar`, `drop_common_labels` functions and `keep_common` modifier from PromQL.
* [CHANGE] Rewritten exposition format parser with much higher performance. The Protobuf exposition format is no longer supported.
* [CHANGE] Example console templates updated for new storage and metrics names. Examples other than node exporter and Prometheus removed.
* [CHANGE] Admin and lifecycle APIs now disabled by default, can be reenabled via flags
* [CHANGE] Flags switched to using Kingpin, all flags are now --flagname rather than -flagname.
* [FEATURE/CHANGE] Remote read can be configured to not read data which is available locally. This is enabled by default.
* [FEATURE] Rules can be grouped now. Rules within a rule group are executed sequentially.
* [FEATURE] Added experimental GRPC apis
* [FEATURE] Add timestamp() function to PromQL.
* [ENHANCEMENT] Remove remote read from the query path if no remote storage is configured.
* [ENHANCEMENT] Bump Consul HTTP client timeout to not match the Consul SD watch timeout.
* [ENHANCEMENT] Go-conntrack added to provide HTTP connection metrics.
* [BUGFIX] Fix connection leak in Consul SD.
