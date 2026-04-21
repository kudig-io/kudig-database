# argo-cd v1.2 Release Notes

Source: [v1.2.5](https://github.com/argoproj/argo-cd/releases/tag/v1.2.5)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.2.5/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.2.5/manifests/ha/install.yaml
```

### Changes since v1.2.4

- Issue #2339 - Don't update 'status.reconciledAt' unless compared with latest git version