# thanos v0.2 Release Notes

Source: [v0.2.1](https://github.com/thanos-io/thanos/releases/tag/v0.2.1)

Xmas patch to release 2 critical fixes (Azure, DNS SD) and awesome, new store UI page.

This also includes first mitigation for https://github.com/improbable-eng/thanos/issues/335

Changelog also available [here](./CHANGELOG.md). 

### Added

- Relabel drop for Thanos Ruler to enable replica label drop and alert deduplication on AM side.
- Query: Stores UI page available at `/stores`.

![](./docs/img/query_ui_stores.png)

### Fixed

- Thanos Rule Alertmanager DNS SD bug.
- DNS SD bug when having SRV results with different ports.
- Move handling of HA alertmanagers to be the same as Prometheus.
- Azure iteration implementation flaw.