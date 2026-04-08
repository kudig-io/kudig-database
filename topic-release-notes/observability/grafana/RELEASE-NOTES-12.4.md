# grafana v12.4 Release Notes

Source: [v12.4.2](https://github.com/grafana/grafana/releases/tag/v12.4.2)

[Download page](https://grafana.com/grafana/download/12.4.2)
[What's new highlights](https://grafana.com/docs/grafana/latest/whatsnew/)

### Features and enhancements

- **Analytics tab:** Improve voice over accessibility (Enterprise)
- **Dashboards a11y:** Do not open time zonemenu on focus [#120388](https://github.com/grafana/grafana/pull/120388), [@idastambuk](https://github.com/idastambuk)
- **Dashboards:** Resolve display names by identity in version history [#120273](https://github.com/grafana/grafana/pull/120273), [@ivanortegaalba](https://github.com/ivanortegaalba)
- **Plugins:** Forward AWS SDK credential chain env vars to external AWS plugins [#120209](https://github.com/grafana/grafana/pull/120209), [@kevinwcyu](https://github.com/kevinwcyu)
- **Public Dashboards:** Prevent unintended CRUD operations from different orgs [#120457](https://github.com/grafana/grafana/pull/120457), [@mmandrus](https://github.com/mmandrus)

### Bug fixes

- **IAM:** Handle NULL team_member.external column to fix dashboard loading [#120179](https://github.com/grafana/grafana/pull/120179), [@difro](https://github.com/difro)
- **Plugins:** Fix installer IsDisabled condition [#120568](https://github.com/grafana/grafana/pull/120568), [@andresmgot](https://github.com/andresmgot)
- **Plugins:** Forward PLUGIN_UNIX_SOCKET_DIR to plugin processes to fix tmp dir in restricted environments [#120275](https://github.com/grafana/grafana/pull/120275), [@HarshadaGawas05](https://github.com/HarshadaGawas05)
- **Security:** Fixes CVE-2026-27876
- **Security:** Fixes CVE-2026-27877
- **Security:** Fixes CVE-2026-28375
- **Security:** Fixes CVE-2026-27879
- **Security:** Fixes CVE-2026-27880
- **Security:** Fixes CVE-2026-27876
- **Security:** Fixes CVE-2026-33375