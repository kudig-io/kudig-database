# kops v1.11 Release Notes

Source: [1.11.1](https://github.com/kubernetes/kops/releases/tag/1.11.1)

Release 1.11.1

## Highlights
* Fixes for [cve-2019-5736](https://kubernetes.io/blog/2019/02/11/runc-and-cve-2019-5736/), for more details please see the [kops advisory for cve-2019-5736](https://github.com/kubernetes/kops/blob/master/docs/advisories/cve_2019_5736.md)
* Fixes for RHEL & Centos distros, that may have caused instances to fail to start due to an issue around overlay2 detection.
* Reserve the nodeport range, to avoid potential port conflicts; see #6342 for details

For full details please see the [1.11 release notes](https://github.com/kubernetes/kops/blob/master/docs/releases/1.11-NOTES.md)

Many thanks to everyone that contributed to the release.  Thanks for code contributions to [@bcorijn](https://github.com/bcorijn), [@jjo](https://github.com/jjo), [@justinsb](https://github.com/justinsb), [@mikesplain](https://github.com/mikesplain), [@nareshku](https://github.com/nareshku), [@ricardo-larosa](https://github.com/ricardo-larosa), [@sp-joseluis-ledesma](https://github.com/sp-joseluis-ledesma)

