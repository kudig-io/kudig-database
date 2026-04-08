# grafana v11.5 Release Notes

Source: [v11.5.10](https://github.com/grafana/grafana/releases/tag/v11.5.10)

[Download page](https://grafana.com/grafana/download/11.5.10)
[What's new highlights](https://grafana.com/docs/grafana/latest/whatsnew/)

### Features and enhancements

- **Go:** Update to 1.25.2 + golangci-lint v2.5.0 + golang.org/x/net v0.45.0 [#112163](https://github.com/grafana/grafana/pull/112163), [@macabu](https://github.com/macabu)
- **Go:** Update to 1.25.3 [#112366](https://github.com/grafana/grafana/pull/112366), [@macabu](https://github.com/macabu)

### Bug fixes

- **Auth:** Fix render user OAuth passthrough [#112093](https://github.com/grafana/grafana/pull/112093), [@mgyongyosi](https://github.com/mgyongyosi)
- **LDAP Authentication:** Fix URL to propagate username context as parameter [#111845](https://github.com/grafana/grafana/pull/111845), [@bradleypettit](https://github.com/bradleypettit)
- **Plugins:** Dependencies do not inherit parent URL for preinstall [#111802](https://github.com/grafana/grafana/pull/111802), [@wbrowne](https://github.com/wbrowne)