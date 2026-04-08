# argo-cd v2.12 Release Notes

Source: [v2.12.13](https://github.com/argoproj/argo-cd/releases/tag/v2.12.13)

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.12.13/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.12.13/manifests/ha/install.yaml
```

## Release Signatures and Provenance

All Argo CD container images are signed by cosign.  A Provenance is generated for container images and CLI binaries which meet the SLSA Level 3 specifications. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets) on how to verify.


## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changelog
### Bug fixes
* 3033ba1e88c1e4c0e7536a75aa4cda414cf9dcb8: fix(security): repository.GetDetailedProject exposes repo secrets (#24387) (#24461) (@alexmt)
### Dependency updates
* 46fcb4fb588b954fb711fcff8f8cc2919f9ebf7f: chore(deps): bump slsa-framework/slsa-github-generator from 2.0.0 to 2.1.0 (#23166) (#24471) (@alexmt)
* 94e7134b0b2ce6278f474d77ec4130218b80d81c: chore(deps): update github.com/antonmedv/expr v1.15.2 to github.com/expr-lang/expr v1.17.0 (#22611) (@aali309)

**Full Changelog**: https://github.com/argoproj/argo-cd/compare/v2.12.12...v2.12.13

<a href="https://argoproj.github.io/cd/"><img src="https://raw.githubusercontent.com/argoproj/argo-site/master/content/pages/cd/gitops-cd.png" width="25%" ></a>

