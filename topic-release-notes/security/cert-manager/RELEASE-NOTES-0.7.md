# cert-manager v0.7 Release Notes

Source: [v0.7.2](https://github.com/cert-manager/cert-manager/releases/tag/v0.7.2)

This is a bugfix release for v0.7 and it is recommended all v0.7 users upgrade as soon as possible.

Notably, the newly introduced CAA record check has been *disabled* by default whilst we investigate issues with certain DNS resolvers that could cause the self-check to fail despite having passed in previous versions.

The new CAA check behaviour can be re-enabled by setting the `--feature-gates=ValidateCAA=true` flag on the cert-manager controller pod (or via `--set extraArgs='[--feature-gates=ValidateCAA=true]'` flag when running `helm install`).

## Changelog since v0.7.1

* Fix update loop in certificates controller and add additional debug logging  (#1602, @munnerz)
* Fixes additionalPrinterColumn formatting for Certificate resources (#1616, @munnerz)
* Disable the CAA check by default, and introduce a new `--feature-gates=ValidateCAA=true` option to enable it (#1585, @munnerz)
* Fix issues running the cainjector controller on Kubernetes 1.9 (#1579, @munnerz)