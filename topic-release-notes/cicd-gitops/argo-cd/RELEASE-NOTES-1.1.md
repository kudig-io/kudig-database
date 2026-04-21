# argo-cd v1.1 Release Notes

Source: [v1.1.2](https://github.com/argoproj/argo-cd/releases/tag/v1.1.2)

## Quick Start
### Non-HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.1.2/manifests/install.yaml
```
### HA:
```
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v1.1.2/manifests/ha/install.yaml
```

### Changes since v1.1.1

-  'argocd app wait' should print correct sync status (#2049)
- Check that TLS is enabled when registering DEX Handlers (#2047)
- Do not ignore Argo hooks when there is a Helm hook. (#1952)