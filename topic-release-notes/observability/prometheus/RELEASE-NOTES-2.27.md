# prometheus v2.27 Release Notes

Source: [v2.27.1](https://github.com/prometheus/prometheus/releases/tag/v2.27.1)

This release contains a bug fix for a security issue in the API endpoint. An
attacker can craft a special URL that redirects a user to any endpoint via an
HTTP 302 response. See the [security advisory][GHSA-vx57-7f4q-fpc7] for more details.

[GHSA-vx57-7f4q-fpc7]:https://github.com/prometheus/prometheus/security/advisories/GHSA-vx57-7f4q-fpc7

This vulnerability has been reported by Aaron Devaney from MDSec.

* [BUGFIX] SECURITY: Fix arbitrary redirects under the /new endpoint (CVE-2021-29622)
