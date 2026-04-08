# cert-manager v1.18 Release Notes

Source: [v1.18.6](https://github.com/cert-manager/cert-manager/releases/tag/v1.18.6)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

v1.18.6 is a simple patch release to fix some reported vulnerabilities, most notably [CVE-2025-68121](https://nvd.nist.gov/vuln/detail/CVE-2025-68121).

NB: We didn't attempt to patch [CVE-2026-24051](https://nvd.nist.gov/vuln/detail/CVE-2026-24051) but that vulnerability affects macOS only, so cert-manager will be unaffected. 

## Changes by Kind

### Bug or Regression

- Bump Go to address CVE-2025-68121 (#8525, @SgtCoDFish)