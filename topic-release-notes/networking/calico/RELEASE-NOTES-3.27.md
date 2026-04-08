# calico v3.27 Release Notes

Source: [v3.27.5](https://github.com/projectcalico/calico/releases/tag/v3.27.5)

Release notes can be found [on GitHub](https://github.com/projectcalico/calico/blob/v3.27.5/release-notes/v3.27.5-release-notes.md)

> [!IMPORTANT]
> Breaking change: On upgrade, the UID of `projectcalico.org/v3` resources will change. It is recommended that you restart any controllers that may care about this after upgrading Calico, including the kube-controller-manager.

> [!WARNING]
> The v3.27 release Calico uses libraries from Kubernetes version v1.27.16 which is end-of-life and is not receiving security updates. There may be unfixed security issues in these Kubernetes libraries. Please consider upgrading to a newer version of Calico to receive the latest security fixes and mitigations.

Attached to this release are the following artifacts:

- `release-v3.27.5.tgz`: container images, binaries, and kubernetes manifests.
- `calico-windows-v3.27.5.zip`: Calico for Windows.
- `tigera-operator-v3.27.5.tgz`: Calico Helm v3 chart.
- ocp.tgz: Manifest bundle for OpenShift.

Additional links:

- [VPP data plane release information](https://github.com/projectcalico/vpp-dataplane/blob/master/RELEASE_NOTES.md)

