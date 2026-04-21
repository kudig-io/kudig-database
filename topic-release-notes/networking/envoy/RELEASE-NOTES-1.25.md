# envoy v1.25 Release Notes

Source: [v1.25.11](https://github.com/envoyproxy/envoy/releases/tag/v1.25.11)

repo: Release v1.25.11

Summary of changes:

* Fixed a bug where processing of deferred streams with the value of
  ``http.max_requests_per_io_cycle`` more than 1, can cause a crash.

**Docker images**:
    https://hub.docker.com/r/envoyproxy/envoy/tags?page=1&name=v1.25.11
**Docs**:
    https://www.envoyproxy.io/docs/envoy/v1.25.11/
**Release notes**:
    https://www.envoyproxy.io/docs/envoy/v1.25.11/version_history/v1.25/v1.25.0
**Full changelog**:
    https://github.com/envoyproxy/envoy/compare/v1.25.10...v1.25.11

Signed-off-by: Ryan Northey <ryan@synca.io>