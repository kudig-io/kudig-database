# opa v0.46 Release Notes

Source: [v0.46.3](https://github.com/open-policy-agent/opa/releases/tag/v0.46.3)

This is a second security fix to address CVE-2022-41717/GO-2022-1144.

We previously believed that upgrading the Golang version and its stdlib would be sufficient
to address the problem. It turns out we also need to bump the x/net dependency to v0.4.0.,
a version that hadn't existed when v0.46.2 was released.

This release bumps the golang.org/x/net dependency to v0.4.0, and contains no other
changes over v0.46.2.

Note that the affected code is OPA's HTTP server. So if you're using OPA as a Golang library,
or if your confident that your OPA's HTTP interface is protected by other means (as it should
be -- not exposed to the public internet), you're OK.