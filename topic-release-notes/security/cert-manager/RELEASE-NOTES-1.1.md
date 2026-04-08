# cert-manager v1.1 Release Notes

Source: [v1.1.1](https://github.com/cert-manager/cert-manager/releases/tag/v1.1.1)

This is a maintenance release that allows users who have installed a pre-v1.1 version of cert-manager using the Helm chart with `--set installCRDs=true` to upgrade to the `v1.1` release without hitting [a CRD validation issue](https://github.com/helm/helm/issues/5806) that causes `helm upgrade` to fail.

If you cannot upgrade to Kubernetes `v1.16` or later but wish to use the latest version of cert-manager that supports Kubernetes `v1.11` - `v1.15` you should upgrade to this release.

Most users should upgrade to the latest `v1.2.0` release below.

## Changes by Kind

### Bug or Regression

- Fix Helm chart type conversion bug ([#3647](https://github.com/jetstack/cert-manager/pull/3647), [@irbekrm](https://github.com/irbekrm))