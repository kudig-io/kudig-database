# flux v0.37 Release Notes

Source: [v0.37.0](https://github.com/fluxcd/flux2/releases/tag/v0.37.0)

## Highlights

Flux v0.37.0 comes with new features and improvements. Users are encouraged to upgrade for the best experience.

### Breaking changes

#### Deprecation of `gitImplementation`

The interpretation of the `gitImplementation` field of `GitRepository` by source-controller and image-automation-controller has been deprecated, and will effectively always use `go-git`. This now supports all Git servers, including Azure DevOps and AWS CodeCommit, which previously were only supported by `libgit2`.

To opt-out from this behaviour, and get the controller to honour the field `.spec.gitImplementation`, start the controller with: `--feature-gates=ForceGoGitImplementation=false`.

For more information on this change, refer to the controllers's respective changelogs [listed below](#components-changelog).

#### Automatic force-push of `ImageUpdateAutomation`

Starting from this version, `ImageUpdateAutomation` objects with a `.spec.PushBranch` specified will have the push branch refreshed automatically via force push. To opt-out from this behaviour, start the controller with: `--feature-gates=GitForcePushBranch=false.`

### Features and improvements

- Support for bootstrapping Azure DevOps and AWS CodeCommit repositories using `flux bootstrap git`.
- Support cloning of Git v2 protocol (Azure DevOps and AWS CodeCommit) for `go-git` Git provider.
- Support force-pushing `ImageUpdateAutomation` repositories.
- Allow a dry-run of `flux build kustomization` with `--dry-run` and `--kustomization-file ./path/to/local/my-app.yaml`. Using these flags, variable substitutions from Secrets and ConfigMaps are skipped, and no connection to the cluster is made.
- Use signed OCI Helm chart for [kube-prometheus-stack](https://fluxcd.io/flux/guides/monitoring/).

### New documentation

- Guide: [AWS CodeCommit bootstrap](https://fluxcd.io/flux/use-cases/aws-codecommit)
- Guide: [Azure DevOps bootstrap](https://fluxcd.io/flux/use-cases/azure/#flux-installation-for-azure-devops)

## Components changelog

- source-controller [v0.32.1](https://github.com/fluxcd/source-controller/blob/v0.32.1/CHANGELOG.md)
- kustomize-controller [v0.31.0](https://github.com/fluxcd/kustomize-controller/blob/v0.31.0/CHANGELOG.md)
- helm-controller [v0.27.0](https://github.com/fluxcd/helm-controller/blob/v0.27.0/CHANGELOG.md)
- notification-controller [v0.29.0](https://github.com/fluxcd/notification-controller/blob/v0.29.0/CHANGELOG.md)
- image-reflector-controller [v0.23.0](https://github.com/fluxcd/image-reflector-controller/blob/v0.23.0/CHANGELOG.md)
- image-automation-controller [v0.27.0](https://github.com/fluxcd/image-automation-controller/blob/v0.27.0/CHANGELOG.md)

## CLI Changelog

- PR #3339 - @hiddeco - Update dependencies
- PR #3326 - @fluxcdbot - Update toolkit components
- PR #3324 - @stefanprodan - Update kubectl and remove nsswitch.conf in flux-cli image
- PR #3323 - @pjbgf - build: Pin GitHub Actions
- PR #3317 - @souleb - Add a dry-run mode to flux build kustomization
- PR #3303 - @stefanprodan - monitoring: Use kube-prometheus-stack signed OCI Helm chart
- PR #3299 - @aryan9600 - Refactor bootstrap process to use `fluxcd/pkg/git`
- PR #3294 - @phillebaba - Aggregate errors in uninstall functions
- PR #3288 - @dependabot[bot] - Bump hashicorp/setup-terraform from 2.0.2 to 2.0.3
- PR #3281 - @stefanprodan - Refactor ARM64 e2e test suite
- PR #3269 - @dependabot[bot] - Bump actions/setup-go from 2 to 3
- PR #3249 - @phillebaba - Remove file reading from bootstrap package

