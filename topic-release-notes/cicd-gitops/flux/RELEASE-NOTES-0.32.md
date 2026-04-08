# flux v0.32 Release Notes

Source: [v0.32.0](https://github.com/fluxcd/flux2/releases/tag/v0.32.0)

## Highlights

Flux v0.32.0 comes with support for distributing Kubernetes manifests, Kustomize overlays and Terraform code as OCI artifacts.
For more information please see the [Flux OCI documentation](https://fluxcd.io/docs/cheatsheets/oci-artifacts/).

### New features

- New Flux CLI commands `flux push|pull|tag artifact` for publishing OCI Artifacts to container registries.
- New source type [OCIRepository](https://fluxcd.io/docs/components/source/ocirepositories/) for fetching OCI artifacts from container registries.
- Resolve Helm dependencies from OCI for charts defined in Git.

## Components changelog

- source-controller [v0.26.0](https://github.com/fluxcd/source-controller/blob/v0.26.0/CHANGELOG.md) [v0.26.1](https://github.com/fluxcd/source-controller/blob/v0.26.1/CHANGELOG.md) 
- kustomize-controller [v0.27.0](https://github.com/fluxcd/kustomize-controller/blob/v0.27.0/CHANGELOG.md)
- notification-controller [v0.25.0](https://github.com/fluxcd/notification-controller/blob/v0.25.0/CHANGELOG.md) [v0.25.1](https://github.com/fluxcd/notification-controller/blob/v0.25.1/CHANGELOG.md)
- image-reflector-controller [v0.20.0](https://github.com/fluxcd/image-reflector-controller/blob/v0.20.0/CHANGELOG.md)
- image-automation-controller [v0.24.1](https://github.com/fluxcd/image-automation-controller/blob/v0.24.1/CHANGELOG.md)

## CLI Changelog
- PR #2966 - @fluxcdbot - Update toolkit components
- PR #2964 - @pjbgf - Add validation to namespace flag
- PR #2955 - @somtochiama - fix log filter and add tests for `flux logs`
- PR #2951 - @stefanprodan - [RFC-0003] Add the provider field for OIDC auth
- PR #2940 - @hiddeco - AUR: further solve `.SRCINFO` issues
- PR #2937 - @hiddeco - AUR: ensure `pkgname` is bottom entry in .SRCINFO

