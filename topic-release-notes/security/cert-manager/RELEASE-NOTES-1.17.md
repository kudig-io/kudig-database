# cert-manager v1.17 Release Notes

Source: [v1.17.4](https://github.com/cert-manager/cert-manager/releases/tag/v1.17.4)

cert-manager is the easiest way to automatically manage certificates in Kubernetes and OpenShift clusters.

We fixed a bug in the CSR's name constraints construction (only applies if you have enabled the `NameConstraints` feature gate).

Changes since `v1.17.3`:

### Bug or Regression

- BUGFIX: permitted URI domains were incorrectly used to set the excluded URI domains in the CSR's name constraints (#7832, @cert-manager-bot)
