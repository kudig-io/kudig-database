# prometheus v0.20 Release Notes

Source: [0.20.0](https://github.com/prometheus/prometheus/releases/tag/0.20.0)

This release contains multiple breaking changes to the configuration schema.
- [FEATURE] Allow configuring multiple Alertmanagers
- [FEATURE] Add server name to TLS configuration
- [FEATURE] Add labels for all node addresses and discover node port if available in Kubernetes SD
- [ENHANCEMENT] More meaningful configuration errors
- [ENHANCEMENT] Round scraping timestamps to milliseconds in web UI
- [ENHANCEMENT] Make number of storage fingerprint locks configurable
- [BUGFIX] Fix date parsing in console template graphs
- [BUGFIX] Fix static console files in Docker images
- [BUGFIX] Fix console JS XHR requests for IE11
- [BUGFIX] Add missing path prefix in new status page
- [CHANGE] Rename `target_groups` to `static_configs` in config files
- [CHANGE] Rename `names` to `files` in file SD configuration
- [CHANGE] Remove kubelet port config option in Kubernetes SD configuration
