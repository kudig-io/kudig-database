# grafana v7.2 Release Notes

Source: [v7.2.2](https://github.com/grafana/grafana/releases/tag/v7.2.2)

[Download Page](https://grafana.com/grafana/download/7.2.2)
[What's New Highlights](https://grafana.com/docs/grafana/latest/guides/whats-new-in-v7-2/)
[Release Notes](https://community.grafana.com/t/release-notes-v7-2-x/36321)

### Features / Enhancements
**Caution:** Please do not use/enable the `database_metrics` feature flag. It will corrupt MySQL database tables. See [#28440](https://github.com/grafana/grafana/issues/28440) for more information.

~~**Instrumentation**: Add counters and histograms for database queries. [#28236](https://github.com/grafana/grafana/pull/28236), [@bergquist](https://github.com/bergquist)~~
* **Instrumentation**: Add histogram for request duration. [#28364](https://github.com/grafana/grafana/pull/28364), [@bergquist](https://github.com/bergquist)
* **Instrumentation**: Adds environment_info metric. [#28355](https://github.com/grafana/grafana/pull/28355), [@bergquist](https://github.com/bergquist)

### Bug Fixes
* **CloudWatch**: Fix custom metrics. [#28391](https://github.com/grafana/grafana/pull/28391), [@aknuds1](https://github.com/aknuds1)