# prometheus v2.41 Release Notes

Source: [v2.41.0](https://github.com/prometheus/prometheus/releases/tag/v2.41.0)

* [FEATURE] Relabeling: Add `keepequal` and `dropequal` relabel actions. #11564
* [FEATURE] Add support for HTTP proxy headers. #11712
* [ENHANCEMENT] Reload private certificates when changed on disk. #11685
* [ENHANCEMENT] Add `max_version` to specify maximum TLS version in `tls_config`. #11685
* [ENHANCEMENT] Add `goos` and `goarch` labels to `prometheus_build_info`. #11685
* [ENHANCEMENT] SD: Add proxy support for EC2 and LightSail SDs #11611
* [ENHANCEMENT] SD: Add new metric `prometheus_sd_file_watcher_errors_total`. #11066
* [ENHANCEMENT] Remote Read: Use a pool to speed up marshalling. #11357
* [ENHANCEMENT] TSDB: Improve handling of tombstoned chunks in iterators. #11632
* [ENHANCEMENT] TSDB: Optimize postings offset table reading. #11535
* [BUGFIX] Scrape: Validate the metric name, label names, and label values after relabeling. #11074
* [BUGFIX] Remote Write receiver and rule manager: Fix error handling. #11727
