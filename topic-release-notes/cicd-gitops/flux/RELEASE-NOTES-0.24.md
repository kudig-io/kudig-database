# flux v0.24 Release Notes

Source: [v0.24.1](https://github.com/fluxcd/flux2/releases/tag/v0.24.1)

## Highlights

This version comes with a change to the length of the SHA hex added to the SemVer metadata composed for a `HelmChart` from `GitRepository` and `Bucket` resources with a `Revision` reconcile strategy. Refer to the source-controller changelog for more information.

## Components changelog

- [source-controller v0.19.2](https://github.com/fluxcd/source-controller/blob/v0.19.2/CHANGELOG.md)
- [kustomize-controller v0.18.2](https://github.com/fluxcd/kustomize-controller/blob/v0.18.2/CHANGELOG.md)
- [helm-controller v0.14.1](https://github.com/fluxcd/helm-controller/blob/v0.14.1/CHANGELOG.md)

## CLI changelog

- PR #2195 - @Nalum - Removing Kubernetes API Request Duration Graph
- PR #2194 - @kingdonb - monitoring: Pin kube-prometheus-stack  to v19.3.0
- PR #2191 - @stefanprodan - Run the ARM64 e2e tests on Equinix hardware
- PR #2178 - @fluxcdbot - Update toolkit components
- PR #2159 - @hiddeco - cmd: start trace short description with T
- PR #2153 - @stefanprodan - e2e: Update Calico to v3.20

## Docker images

- `docker pull fluxcd/flux-cli:v0.24.1`
- `docker pull ghcr.io/fluxcd/flux-cli:v0.24.1`
