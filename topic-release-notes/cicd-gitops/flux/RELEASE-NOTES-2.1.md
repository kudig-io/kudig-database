# flux v2.1 Release Notes

Source: [v2.1.2](https://github.com/fluxcd/flux2/releases/tag/v2.1.2)

## Highlights

Flux `v2.1.2` is a patch release which comes with various fixes. Users are encouraged to upgrade for the best experience. 

### Fixes

- Ensures faster recovery of `Kustomization` and `HelmRelease` resources when the source-controller has restarted and is working on restoring the storage.
- Prevent source-controller from failing to reconcile `OCIRepositories` when artifacts contain symlinks.
- Addresses issue with helm-controller miss-labeling Custom Resource Definitions.
- Detect immutable field errors in Google Cloud resources managed by Flux `Kustomizations`.
- Better error reporting for `flux bootstrap` when the owner doesn't match the identity associated with the given token.
- Allow `flux pull artifact` to fetch OCI artifacts produced by other tools.

## Components changelog

- source-controller [v1.1.2](https://github.com/fluxcd/source-controller/blob/v1.1.2/CHANGELOG.md)
- kustomize-controller [v1.1.1](https://github.com/fluxcd/kustomize-controller/blob/v1.1.1/CHANGELOG.md)
- helm-controller [v0.36.2](https://github.com/fluxcd/helm-controller/blob/v0.36.2/CHANGELOG.md)

## CLI Changelog

- PR #4324 - @somtochiama - bootstrap: Fix error msg when the Git token doesn't match the repo owner
- PR #4323 - @stefanprodan - e2e: Update Go dependencies
- PR #4313 - @fluxcdbot - Update toolkit components
- PR #4296 - @Skarlso - fix: only wait for changeset if the result is not empty
- PR #4285 - @matheuscscp - Add badge for SLSA Level 3
- PR #4284 - @errordeveloper - Make `flux pull` work for OCI artifacts produced by other tools

