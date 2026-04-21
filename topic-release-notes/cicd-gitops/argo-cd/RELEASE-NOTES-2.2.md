# argo-cd v2.2 Release Notes

Source: [v2.2.16](https://github.com/argoproj/argo-cd/releases/tag/v2.2.16)

## Quick Start

### Non-HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.2.16/manifests/install.yaml
```

### HA:

```shell
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.2.16/manifests/ha/install.yaml
```

## 2.2.x has reached EOL

This is the **last patch release in the 2.2 series**. Please upgrade to >=2.3 to continue to receive security updates. Read the [upgrading](https://argo-cd.readthedocs.io/en/stable/operator-manual/upgrading/overview/) documentation before upgrading.

## Release signatures

All Argo CD container images and CLI binaries are signed by cosign. See the [documentation](https://argo-cd.readthedocs.io/en/stable/operator-manual/signed-release-assets/) on how to verify the signatures.
```shell
-----BEGIN PUBLIC KEY-----
MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEesHEB7vX5Y2RxXypjMy1nI1z7iRG
JI9/gt/sYqzpsa65aaNP4npM43DDxoIy/MQBo9s/mxGxmA+8UXeDpVC9vw==
-----END PUBLIC KEY-----
```

## Changes

This release includes 6 contributions from 4 contributors (3 of them new) with 0 features and 1 bug fixes.

A special thanks goes to the 3 new contributors:
- Alex Eftimie
- Allex
- Chromo-residuum-opec

### Security fixes

This release includes openssl version [3.0.2-0ubuntu1.7](https://launchpad.net/ubuntu/+source/openssl/3.0.2-0ubuntu1.7), which patches [high-severity vulnerabilities](https://www.openssl.org/blog/blog/2022/11/01/email-address-overflows/).

### Bug fixes

- fix: Update custom health check for kiali.io/Kiali (#10995)

### Documentation

- docs: fix 'bellow' typos (#11038)
- docs: mention that OCI helm does not support version ranges (#11026)

### Other

- chore: fix CI (#11022)
- chore: fix e2e (#11005)
- chore: upgrade actions/checkout to v3, i.e. Node.js 16 (#10947)

