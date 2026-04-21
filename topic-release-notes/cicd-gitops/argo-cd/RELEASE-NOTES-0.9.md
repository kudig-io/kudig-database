# argo-cd v0.9 Release Notes

Source: [v0.9.2](https://github.com/argoproj/argo-cd/releases/tag/v0.9.2)

* Update to kustomize 1.0.8
- Fix issue where argocd-server logged credentials in plain text during repo add (issue #653)
- Credentials not being accepted for Google Source Repositories (issue #651)
- Azure Repos do not work as a repository (issue #643)
- Temporary ignore service catalog resources (issue #650)
- Normalize policies by always adding space after comma
