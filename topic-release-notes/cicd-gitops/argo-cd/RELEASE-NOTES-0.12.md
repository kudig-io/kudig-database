# argo-cd v0.12 Release Notes

Source: [v0.12.3](https://github.com/argoproj/argo-cd/releases/tag/v0.12.3)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v0.12.3/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v0.12.3/manifests/ha/install.yaml
```


## Changes since v0.12.2

- Application controller becomes unresponsive (#1476)
