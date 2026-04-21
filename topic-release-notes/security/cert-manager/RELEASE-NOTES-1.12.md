# cert-manager v1.12 Release Notes

Source: [v1.12.17](https://github.com/cert-manager/cert-manager/releases/tag/v1.12.17)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

This patch release addresses several vulnerabilities reported by the Trivy security scanner. It is built with the latest version of Go 1.23 and includes various dependency updates.

> 📖 Read the full [cert-manager 1.12 release notes](https://cert-manager.io/docs/releases/release-notes/release-notes-1.12), before installing or upgrading.

## Changes since `v1.12.16`

### Bug or Regression

- Bump Go to `v1.23.8` to fix `CVE-2025-22871` ([#7709](https://github.com/cert-manager/cert-manager/pull/7709), [`@wallrj`](https://github.com/wallrj))
- Bump `golang.org/x/net` to `v0.38.0` to fix `CVE-2025-22872` ([#7709](https://github.com/cert-manager/cert-manager/pull/7709), [`@wallrj`](https://github.com/wallrj))
- Bump `github.com/golang-jwt/jwt/v4` to `v4.5.2` to fix `CVE-2025-30204` ([#7709](https://github.com/cert-manager/cert-manager/pull/7709), [`@wallrj`](https://github.com/wallrj))
- Bump `go-jose` to address `CVE-2025-27144` ([#7597](https://github.com/cert-manager/cert-manager/pull/7597), [`@SgtCoDFish`](https://github.com/SgtCoDFish))
- Bump `golang.org/x/net` to address `CVE-2025-22870` reported by Trivy ([#7624](https://github.com/cert-manager/cert-manager/pull/7624), [`@SgtCoDFish`](https://github.com/SgtCoDFish))
- Bump `golang.org/x/net` to address `CVE-2025-22870` reported by Trivy ([#7623](https://github.com/cert-manager/cert-manager/pull/7623), [`@SgtCoDFish`](https://github.com/SgtCoDFish))