# prometheus v2.5 Release Notes

Source: [v2.5.0](https://github.com/prometheus/prometheus/releases/tag/v2.5.0)

* [CHANGE] Group targets by scrape config instead of job name. #4806 #4526            
* [CHANGE] Marathon SD: Various changes to adapt to Marathon 1.5+. #4499
* [CHANGE] Discovery: Split `prometheus_sd_discovered_targets` metric by scrape and notify (Alertmanager SD) as well as by section in the respective configuration. #4753
* [FEATURE] Add OpenMetrics support for scraping (EXPERIMENTAL). #4700
* [FEATURE] Add unit testing for rules. #4350              
* [FEATURE] Make maximum number of samples per query configurable via `--query.max-samples` flag. #4513
* [FEATURE] Make maximum number of concurrent remote reads configurable via `--storage.remote.read-concurrent-limit` flag. #4656
* [ENHANCEMENT] Support s390x platform for Linux. #4605
* [ENHANCEMENT] API: Add `prometheus_api_remote_read_queries` metric tracking currently executed or waiting remote read API requests. #4699
* [ENHANCEMENT] Remote Read: Add `prometheus_remote_storage_remote_read_queries` metric tracking currently in-flight remote read queries. #4677
* [ENHANCEMENT] Remote Read: Reduced memory usage. #4655               
* [ENHANCEMENT] Discovery: Add `prometheus_sd_discovered_targets`, `prometheus_sd_received_updates_total`, `prometheus_sd_updates_delayed_total`, and `prometheus_sd_updates_total` metri
cs for discovery subsystem. #4667                              
* [ENHANCEMENT] Discovery: Improve performance of previously slow updates of changes of targets. #4526
* [ENHANCEMENT] Kubernetes SD: Add extended metrics. #4458
* [ENHANCEMENT] OpenStack SD: Support discovering instances from all projects. #4682
* [ENHANCEMENT] OpenStack SD: Discover all interfaces. #4649
* [ENHANCEMENT] OpenStack SD: Support `tls_config` for the used HTTP client. #4654
* [ENHANCEMENT] Triton SD: Add ability to filter triton_sd targets by pre-defined groups. #4701
* [ENHANCEMENT] Web UI: Avoid browser spell-checking in expression field. #4728
* [ENHANCEMENT] Web UI: Add scrape duration and last evaluation time in targets and rules pages. #4722
* [ENHANCEMENT] Web UI: Improve rule view by wrapping lines. #4702
* [ENHANCEMENT] Rules: Error out at load time for invalid templates, rather than at evaluation time. #4537                                                                               * [ENHANCEMENT] TSDB: Add metrics for WAL operations. #4692
* [BUGFIX] Change max/min over_time to handle NaNs properly. #4386
* [BUGFIX] Check label name for `count_values` PromQL function. #4585          
* [BUGFIX] Ensure that vectors and matrices do not contain identical label-sets. #4589