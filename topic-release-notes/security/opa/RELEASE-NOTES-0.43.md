# opa v0.43 Release Notes

Source: [v0.43.1](https://github.com/open-policy-agent/opa/releases/tag/v0.43.1)

This is a security release fixing the following vulnerabilities:

- CVE-2022-36085: Respect unsafeBuiltinMap for 'with' replacements in the compiler

  See https://github.com/open-policy-agent/opa/security/advisories/GHSA-f524-rf33-2jjr for all details.

- CVE-2022-27664 and CVE-2022-32190.

  Fixed by updating the Go version used in our builds to 1.18.6,
  see https://groups.google.com/g/golang-announce/c/x49AQzIVX-s.
  Note that CVE-2022-32190 is most likely not relevant for OPA's usage of net/url.
  But since these CVEs tend to come up in security assessment tooling regardless,
  it's better to get it out of the way.