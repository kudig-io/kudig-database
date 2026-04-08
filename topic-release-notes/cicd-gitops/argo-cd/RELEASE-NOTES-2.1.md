# argo-cd v2.1 Release Notes

Source: [v2.1.16](https://github.com/argoproj/argo-cd/releases/tag/v2.1.16)

## Quick Start

### Non-HA:

```bash
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.1.16/manifests/install.yaml
```

#### HA:

```bash
kubectl create namespace argocd
kubectl apply -n argocd -f https://raw.githubusercontent.com/argoproj/argo-cd/v2.1.16/manifests/ha/install.yaml
```

### Security fixes

* CRITICAL: External URLs for Deployments can include javascript ([GHSA-h4w9-6x78-8vrj](https://github.com/argoproj/argo-cd/security/advisories/GHSA-h4w9-6x78-8vrj))
* HIGH: Insecure entropy in PKCE/Oauth2/OIDC params ([GHSA-2m7h-86qq-fp4v](https://github.com/argoproj/argo-cd/security/advisories/GHSA-2m7h-86qq-fp4v))
* MODERATE: DoS through large directory app manifest files ([GHSA-jhqp-vf4w-rpwq](https://github.com/argoproj/argo-cd/security/advisories/GHSA-jhqp-vf4w-rpwq))
* MODERATE: Symlink following allows leaking out-of-bounds YAML files from Argo CD repo-server ([GHSA-q4w5-4gq2-98vm](https://github.com/argoproj/argo-cd/security/advisories/GHSA-q4w5-4gq2-98vm))

**Note:** This will be the last security fix release in the 2.1.x series. Please [upgrade to a newer minor version](https://argo-cd.readthedocs.io/en/latest/operator-manual/upgrading/overview/) to continue to get security fixes.

### Potentially-breaking changes

From the [GHSA-2m7h-86qq-fp4v](https://github.com/argoproj/argo-cd/security/advisories/GHSA-2m7h-86qq-fp4v) description:

> The patch introduces a new `reposerver.max.combined.directory.manifests.size` config parameter, which you should tune before upgrading in production. It caps the maximum total file size of .yaml/.yml/.json files in directory-type (raw manifest) Applications. The default max is 10M per Application. This max is designed to keep any single app from consuming more than 3G of memory in the repo-server (manifests consume more space in memory than on disk). The 300x ratio assumes a maliciously-crafted manifest file. If you only want to protect against accidental excessive memory use, it is probably safe to use a smaller ratio.
>
> If your organization uses directory-type Applications with very many manifests or very large manifests then check the size of those manifests and tune the config parameter before deploying this change to production. When testing, make sure to do a "hard refresh" in either the CLI or UI to test your directory-type App. That will make sure you're using the new max logic instead of relying on cached manifest responses from Redis.

### Bug fixes

* fix: missing Helm params (#9565) (#9566)

### Other

* test: directory app manifest generation (#9503)
* test: fix erroneous test change
* chore: eliminate go-mpatch dependency (#9045)
* chore: Make unit tests run on platforms other than amd64 (#8995)
* chore: remove obsolete repo-server unit test (#9559)
* chore: upgrade golangci-lint to v1.46.2 (#9448)
* chore: update golangci-lint (#8988)
* test: fix ErrorContains (#9445)