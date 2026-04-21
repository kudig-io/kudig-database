# helm v2.14 Release Notes

Source: [v2.14.3](https://github.com/helm/helm/releases/tag/v2.14.3)

Helm v2.14.3 is a patch release. Users are encouraged to upgrade for the best experience.

This release was signed with `92AA 783C BAAE 8E3B` and can be found at @bacongobbler's [keybase account](https://keybase.io/bacongobbler). Please use the attached signatures for verifying this release using `gpg`.

The community keeps growing, and we'd love to see you there!

- Join the discussion in [Kubernetes Slack](https://kubernetes.slack.com):
  - `#helm-users` for questions and just to hang out
  - `#helm-dev` for discussing PRs, code, and bugs
- Hang out at the Public Developer Call: Thursday, 9:30 Pacific via [Zoom](https://zoom.us/j/696660622)
- Test, debug, and contribute charts: [GitHub/helm/charts](https://github.com/helm/charts)

## Installation and Upgrading

Download Helm 2.14.3. The common platform binaries are here:

- [MacOS amd64](https://get.helm.sh/helm-v2.14.3-darwin-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.14.3-darwin-amd64.tar.gz.sha256))
- [Linux amd64](https://get.helm.sh/helm-v2.14.3-linux-amd64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.14.3-linux-amd64.tar.gz.sha256))
- [Linux arm](https://get.helm.sh/helm-v2.14.3-linux-arm.tar.gz) ([checksum](https://get.helm.sh/helm-v2.14.3-linux-arm.tar.gz.sha256))
- [Linux arm64](https://get.helm.sh/helm-v2.14.3-linux-arm64.tar.gz) ([checksum](https://get.helm.sh/helm-v2.14.3-linux-arm64.tar.gz.sha256))
- [Linux i386](https://get.helm.sh/helm-v2.14.3-linux-386.tar.gz) ([checksum](https://get.helm.sh/helm-v2.14.3-linux-386.tar.gz.sha256))
- [Linux ppc64le](https://get.helm.sh/helm-v2.14.3-linux-ppc64le.tar.gz) ([checksum](https://get.helm.sh/helm-v2.14.3-linux-ppc64le.tar.gz.sha256))
- [Linux s390x](https://get.helm.sh/helm-v2.14.3-linux-s390x.tar.gz) ([checksum](https://get.helm.sh/helm-v2.14.3-linux-s390x.tar.gz.sha256))
- [Windows amd64](https://get.helm.sh/helm-v2.14.3-windows-amd64.zip) ([checksum](https://get.helm.sh/helm-v2.14.3-windows-amd64.zip.sha256))

Once you have the client installed, upgrade Tiller with `helm init --upgrade`.

The [Quickstart Guide](https://docs.helm.sh/using_helm/#quickstart-guide) will get you going from there. For **upgrade instructions** or detailed installation notes, check the [install guide](https://docs.helm.sh/using_helm/#installing-helm). You can also use a [script to install](https://raw.githubusercontent.com/helm/helm/master/scripts/get) on any system with `bash`.

## What's Next

- v2.14.4 will contain only bug fixes.
- v2.15.0 is the next feature release.

## Changelog

- fix: upgrade with CRD changes 0e7f3b6637f7af8fcfddb3d2941fcc7cbebb0085 (Yusuke Kuoka)