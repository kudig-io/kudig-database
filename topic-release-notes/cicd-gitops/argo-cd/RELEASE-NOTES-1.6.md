# argo-cd v1.6 Release Notes

Source: [v1.6.2](https://github.com/argoproj/argo-cd/releases/tag/v1.6.2)

## Quick Start

### Non-HA:

```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.6.2/manifests/install.yaml
```

### HA:

```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.6.2/manifests/ha/install.yaml
```

## Changes

- feat: adding validate for app create and app set (#4016)
- fix: use glob matcher in casbin built-in model (#3966)
- fix: Normalize Helm chart path when chart name contains a slash (#3987)
- fix: allow duplicates when using generateName (#3878)
- fix: nil pointer dereference while syncing an app (#3915)