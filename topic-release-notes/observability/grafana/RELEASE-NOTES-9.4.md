# grafana v9.4 Release Notes

Source: [v9.4.17](https://github.com/grafana/grafana/releases/tag/v9.4.17)

[Download page](https://grafana.com/grafana/download/9.4.17)
[What's new highlights](https://grafana.com/docs/grafana/latest/whatsnew/)

### Features and enhancements

- **Chore:** Upgrade Go to 1.20.10. [#76370](https://github.com/grafana/grafana/issues/76370), [@zerok](https://github.com/zerok)
- **SSE:** DSNode to update result with names to make each value identifiable by labels (only Graphite and TestData). [#74615](https://github.com/grafana/grafana/issues/74615), [@yuri-tceretian](https://github.com/yuri-tceretian)

### Bug fixes

- **BrowseDashboards:** Only remember the most recent expanded folder. [#74812](https://github.com/grafana/grafana/issues/74812), [@joshhunt](https://github.com/joshhunt)
- **SQL Datasources:** Fix variable throwing error if query returns no data. [#74609](https://github.com/grafana/grafana/issues/74609), [@mdvictor](https://github.com/mdvictor)
- **RBAC:** Chore fix hasPermissionInOrg. (Enterprise)