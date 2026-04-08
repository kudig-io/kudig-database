# argo-cd v1.4 Release Notes

Source: [v1.4.3](https://github.com/argoproj/argo-cd/releases/tag/v1.4.3)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.4.3/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.4.3/manifests/ha/install.yaml
```

### Bug Fixes

* fix: evaluate attack vector of GHSA-qm7j-c969-7j4q on ArgoCD (CVE-2020-5260)