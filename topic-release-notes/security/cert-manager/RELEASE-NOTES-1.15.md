# cert-manager v1.15 Release Notes

Source: [v1.15.5](https://github.com/cert-manager/cert-manager/releases/tag/v1.15.5)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

cert-manager v1.15.5 contains simple dependency bumps to address reported CVEs (CVE-2024-45337 and CVE-2024-45338).

We don't believe that cert-manager is actually vulnerable; this release is instead intended to satisfy vulnerability scanners.

## Changes

### Bug or Regression

- Bump golang.org/x/net to address CVE-2024-45337 and CVE-2024-45338 (#7496, @wallrj)

### Other (Cleanup or Flake)

- Bump to go 1.22.10 (#7507, @SgtCoDFish)