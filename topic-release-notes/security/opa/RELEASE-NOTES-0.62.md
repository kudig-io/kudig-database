# opa v0.62 Release Notes

Source: [v0.62.1](https://github.com/open-policy-agent/opa/releases/tag/v0.62.1)

This is a **security fix release** for the fixes published in [Go 1.22.1](https://groups.google.com/g/golang-announce/c/5pwGVUPoMbg).

OPA servers using `--authentication=tls` would be affected: crafted malicious client certificates could cause a panic in the server.

Also, crafted server certificates could panic OPA's HTTP clients, in bundle plugin, status and decision logs; and `http.send` calls that verify TLS.

This is CVE-2024-24783 (https://pkg.go.dev/vuln/GO-2024-2598).

Note that there are other security fixes in this Golang release, but whether or not OPA is affected is harder to assess. An update is advised.


### Miscellaneous

- Add Trino to OPA ecosystem (authored by @mosabua)
- update: ADOPTERS.md (#6608) (authored by @fredmaggiowski)


