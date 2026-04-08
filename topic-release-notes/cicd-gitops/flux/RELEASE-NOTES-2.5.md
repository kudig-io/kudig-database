# flux v2.5 Release Notes

Source: [v2.5.1](https://github.com/fluxcd/flux2/releases/tag/v2.5.1)

## Highlights

Flux v2.5.1 is a patch release which comes with various fixes. Users are encouraged to upgrade for the best experience.

Fixes:

- Fix a bug introduced in kustomize-controller v1.5.0 that was causing spurious logging for deprecated API versions and health check failures.
- Sanitize the kustomize-controller logs when encountering errors during SOPS decryption.

## Components changelog

- kustomize-controller [v1.5.1](https://github.com/fluxcd/kustomize-controller/blob/v1.5.1/CHANGELOG.md)

## CLI Changelog

- PR #5215 - @matheuscscp - Update backport labels for 2.5
- PR #5214 - @fluxcdbot - Update kustomize-controller to v1.5.1

