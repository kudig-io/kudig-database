# prometheus v2.10 Release Notes

Source: [v2.10.0](https://github.com/prometheus/prometheus/releases/tag/v2.10.0)

* [CHANGE/BUGFIX] API: Encode alert values as string to correctly represent Inf/NaN. #5582
* [FEATURE] Template expansion: Make external labels available as `$externalLabels` in alert and console template expansion. #5463
* [FEATURE] TSDB: Add `prometheus_tsdb_wal_segment_current` metric for the WAL segment index that TSDB is currently writing to. tsdb#601
* [FEATURE] Scrape: Add `scrape_series_added` per-scrape metric. #5546
* [ENHANCEMENT] Discovery/kubernetes: Add labels `__meta_kubernetes_endpoint_node_name` and `__meta_kubernetes_endpoint_hostname`. #5571
* [ENHANCEMENT] Discovery/azure: Add label `__meta_azure_machine_public_ip`. #5475
* [ENHANCEMENT] TSDB: Simplify mergedPostings.Seek, resulting in better performance if there are many posting lists. tsdb#595
* [ENHANCEMENT] Log filesystem type on startup. #5558
* [ENHANCEMENT] Cmd/promtool: Use POST requests for Query and QueryRange. client_golang#557
* [ENHANCEMENT] Web: Sort alerts by group name. #5448
* [ENHANCEMENT] Console templates: Add convenience variables `$rawParams`, `$params`, `$path`. #5463
* [BUGFIX] TSDB: Don't panic when running out of disk space and recover nicely from the condition. tsdb#582
* [BUGFIX] TSDB: Correctly handle empty labels. tsdb#594
* [BUGFIX] TSDB: Don't crash on an unknown tombstone reference. tsdb#604
* [BUGFIX] Storage/remote: Remove queue-manager specific metrics if queue no longer exists. #5445 #5485 #5555
* [BUGFIX] PromQL: Correctly display `{__name__="a"}`. #5552
* [BUGFIX] Discovery/kubernetes: Use `service` rather than `ingress` as the name for the service workqueue. #5520
* [BUGFIX] Discovery/azure: Don't panic on a VM with a public IP. #5587
* [BUGFIX] Discovery/triton: Always read HTTP body to completion. #5596
* [BUGFIX] Web: Fixed Content-Type for js and css instead of using `/etc/mime.types`. #5551
