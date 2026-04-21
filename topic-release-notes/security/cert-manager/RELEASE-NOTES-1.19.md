# cert-manager v1.19 Release Notes

Source: [v1.19.4](https://github.com/cert-manager/cert-manager/releases/tag/v1.19.4)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

v1.19.4 is a simple patch release to fix some reported vulnerabilities - notably CVE-2026-24051 and CVE-2025-68121. All users should upgrade.

## Changes by Kind

### Bug or Regression

- Bump go to address CVE-2025-68121 (#8526, @SgtCoDFish)
- Bump otel SDK to address GO-2026-4394 (#8531, @SgtCoDFish)