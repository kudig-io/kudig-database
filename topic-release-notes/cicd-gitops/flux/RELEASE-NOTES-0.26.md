# flux v0.26 Release Notes

Source: [v0.26.3](https://github.com/fluxcd/flux2/releases/tag/v0.26.3)

## Highlights

Flux v0.26.3 is a patch release that comes with fixes to bootstrap. Users are encouraged to upgrade for the best experience.

In addition, kustomize-controller was update to be on par with Kustomize [v4.5.2 release](https://github.com/kubernetes-sigs/kustomize/releases/tag/kustomize%2Fv4.5.2).

## Components changelog
- kustomize-controller [v0.20.2](https://github.com/fluxcd/kustomize-controller/blob/v0.20.2/CHANGELOG.md)

## CLI changelog
- PR #2418 - @stefanprodan - Fix bootstrap: Reset schema cache after applying CRDs
- PR #2416 - @fluxcdbot - Update kustomize-controller to v0.20.2
- PR #2415 - @stefanprodan - Add GitRepository namespace arg to `flux create image update`

