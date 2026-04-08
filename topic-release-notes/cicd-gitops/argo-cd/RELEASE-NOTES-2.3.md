# argo-cd v2.3 Release Notes

Source: [v2.3.17](https://github.com/argoproj/argo-cd/releases/tag/v2.3.17)

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.3.17/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.3.17/manifests/ha/install.yaml
```

## Release signatures

All Argo CD container images and CLI binaries are signed by cosign. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets/) on how to verify the signatures.
```shell
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEesHEB7vX5Y2RxXypjMy1nI1z7iRG
JI9/gt/sYqzpsa65aaNP4npM43DDxoIy/MQBo9s/mxGxmA+8UXeDpVC9vw==
-----END PUBLIC KEY-----
```

## 2.3.x has reached EOL

This is the **last patch release in the 2.3 series**. Please upgrade to >=2.4 to continue to receive security updates. Read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation before upgrading.

## Upgrading

If upgrading from a different minor version, be sure to read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation.

## Changes

This release includes 4 contributions from 3 contributors with 0 features and 0 bug fixes.

### Security (1)

- CRITICAL: Users with any cluster secret update access may update out-of-bounds cluster secrets (GHSA-3jfq-742w-xg8j)

### Documentation (2)

- docs: Fix heading to not include a v for the second version (#12218)
- docs: add destination.name example (#12242)

### Other (1)

- chore: add dist to path to use our kustomize version (#12352)

