# argo-cd v1.5 Release Notes

Source: [v1.5.8](https://github.com/argoproj/argo-cd/releases/tag/v1.5.8)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.5.8/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.5.8/manifests/ha/install.yaml
```

### Bug Fixes

* fix: upgrade awscli version (#3774)
* fix: html encode login error/description before rendering it (#3773)
* fix: oidc should set samesite cookie (#3632)
* fix: avoid panic in badge handler (#3741)