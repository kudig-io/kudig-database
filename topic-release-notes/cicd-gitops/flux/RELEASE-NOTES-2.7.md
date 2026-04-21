# flux v2.7 Release Notes

Source: [v2.7.5](https://github.com/fluxcd/flux2/releases/tag/v2.7.5)

## Highlights

Flux v2.7.5 is a patch release that comes with fixes to helm-controller. Users are encouraged to upgrade for the best experience.

ℹ️ Please follow the [Upgrade Procedure for Flux v2.7+](https://github.com/fluxcd/flux2/discussions/5572) for a smooth upgrade from Flux v2.6 to the latest version.

Fixes:

- Fix HelmRelease history truncation when using the `RetryOnFailure` strategy.

:warning: Note that signature verification for OCI artifacts in source-controller is not compatible with Cosign v3.
Flux users are advised to use [Cosign v2.6](https://fluxcd.io/flux/flux-gh-action/#push-and-sign-kubernetes-manifests-to-container-registries) for signing Flux OCI artifacts and Helm charts, until support for Cosign v3 is added in Flux v2.8.

## Components changelog

- helm-controller [v1.4.5](https://github.com/fluxcd/helm-controller/blob/v1.4.5/CHANGELOG.md)

## CLI changelog

* [release/v2.7.x] Update toolkit components by @fluxcdbot in https://github.com/fluxcd/flux2/pull/5649


**Full Changelog**: https://github.com/fluxcd/flux2/compare/v2.7.4...v2.7.5

