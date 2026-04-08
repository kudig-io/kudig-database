# cert-manager v1.11 Release Notes

Source: [v1.11.5](https://github.com/cert-manager/cert-manager/releases/tag/v1.11.5)

v1.11.5 contains an important security fix that addresses [CVE-2023-29409](https://cve.report/CVE-2023-29409).

## Changes since v1.11.4

- Use Go 1.19.9 to fix a security issue in Go's `crypto/tls` library. (#6317, @maelvls)