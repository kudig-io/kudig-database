# envoy v1.33 Release Notes

Source: [v1.33.14](https://github.com/envoyproxy/envoy/releases/tag/v1.33.14)

**Summary of changes**:

* Security updates:

  Resolve dependency CVEs:
  - c-ares/CVE-2025-0913:
      Use after free can crash Envoy due to malfunctioning or compromised DNS.

While a potentially severe bug in some cloud environments, this has limited exploitability
as any attacker would require control of DNS.

Envoy advisory is here https://github.com/envoyproxy/envoy/security/advisories/GHSA-fg9g-pvc4-776f

**Docker images**:
    https://hub.docker.com/r/envoyproxy/envoy/tags?page=1&name=v1.33.14
**Docs**:
    https://www.envoyproxy.io/docs/envoy/v1.33.14/
**Release notes**:
    https://www.envoyproxy.io/docs/envoy/v1.33.14/version_history/v1.33/v1.33.14
**Full changelog**:
    https://github.com/envoyproxy/envoy/compare/v1.33.13...v1.33.14

Signed-off-by: Ryan Northey <ryan@synca.io>
Signed-off-by: Boteng Yao <boteng@google.com>