# envoy v1.24 Release Notes

Source: [v1.24.12](https://github.com/envoyproxy/envoy/releases/tag/v1.24.12)



repo: Release `1.24.12`

Summary of changes:

* Fixed a bug where processing of deferred streams with the value of
    ``http.max_requests_per_io_cycle`` more than 1, can cause a crash.

**Docker images**:
    https://hub.docker.com/r/envoyproxy/envoy/tags?page=1&name=v1.24.12
**Docs**:
    https://www.envoyproxy.io/docs/envoy/v1.24.12/
**Release notes**:
    https://www.envoyproxy.io/docs/envoy/v1.24.12/version_history/v1.24/v1.24.12
**Full changelog**:
    https://github.com/envoyproxy/envoy/compare/v1.24.11...v1.24.12

Signed-off-by: Ryan Northey <ryan@synca.io>