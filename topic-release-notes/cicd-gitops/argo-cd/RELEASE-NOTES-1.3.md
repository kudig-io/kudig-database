# argo-cd v1.3 Release Notes

Source: [v1.3.6](https://github.com/argoproj/argo-cd/releases/tag/v1.3.6)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.3.6/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.3.6/manifests/ha/install.yaml
```

### Bug Fixes 

* Add support for hidden directories with directory enforcer (#2821)

### Contributors

* Simon Behar

See also [milestone v1.3](https://github.com/argoproj/argo-cd/milestone/15?closed=1)
