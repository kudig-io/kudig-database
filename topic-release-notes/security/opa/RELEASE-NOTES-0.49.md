# opa v0.49 Release Notes

Source: [v0.49.2](https://github.com/open-policy-agent/opa/releases/tag/v0.49.2)

This release migrates the [ORAS Go library](oras.land/oras-go/v2) from v1.2.2 to v2.
The earlier version of the library had a dependency on the [docker](github.com/docker/docker)
package. That version of the docker package had some reported vulnerabilities such as
CVE-2022-41716, CVE-2022-41720. The ORAS Go library v2 removes the dependency on the docker package.