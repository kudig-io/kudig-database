# cert-manager v1.14 Release Notes

Source: [v1.14.7](https://github.com/cert-manager/cert-manager/releases/tag/v1.14.7)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

## 📜 Changes since [v1.14.6](https://github.com/cert-manager/cert-manager/releases/tag/v1.14.6)

### Bugfixes

- BUGFIX: fix issue that caused Vault issuer to not retry signing when an error was encountered. (#7113, @cert-manager-bot)

### Other (Cleanup or Flake)

- Update github.com/Azure/azure-sdk-for-go/sdk/azidentity to address CVE-2024-35255 (#7093, @ThatsMrTalbot)
