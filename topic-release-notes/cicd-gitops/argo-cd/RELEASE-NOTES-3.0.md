# argo-cd v3.0 Release Notes

Source: [v3.0.23](https://github.com/argoproj/argo-cd/releases/tag/v3.0.23)

> [!IMPORTANT]
> **END OF LIFE NOTICE**
> 
> This is the final release of the `v3.0` release series. As of 02 Feb 2026, this version has reached end of life and will no longer receive bug fixes or security updates.
> 
> **Action Required**: Please upgrade to a [supported version](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) (v3.1 or higher).

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v3.0.23/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v3.0.23/manifests/ha/install.yaml
```

## Release Signatures and Provenance

All Argo CD container images are signed by cosign.  A Provenance is generated for container images and CLI binaries which meet the SLSA Level 3 specifications. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets) on how to verify.

## Release Notes Blog Post
For a detailed breakdown of the key changes and improvements in this release, check out the [official blog post](https://blog.argoproj.io/argo-cd-v2-14-release-candidate-57a664791e2a)

## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changelog
### Bug fixes
* 6c9b1cb046d1faf9fbb85f84ee79501db5f27355: fix(hydrator): empty links for failed operation (#25025) (cherry-pick #26014 for 3.0) (#26015) (@argo-cd-cherry-pick-bot[bot])
* 5f9dc64088a9d51423556890883adee73ed7bb5d: fix: cherry pick #25516 to release-3.0 (#26107) (@FourFifthsCode)
* 9a495627ce136ca948555738359094ab08cd5ccf: fix: invalid error message on health check failure (#26040) (cherry pick #26039 for 3.0) (#26073) (@dudinea)

**Full Changelog**: https://github.com/argoproj/argo-cd/compare/v3.0.22...v3.0.23

<a href="https://argoproj.github.io/cd/"><img src="https://raw.githubusercontent.com/argoproj/argo-site/master/content/pages/cd/gitops-cd.png" width="25%" ></a>

