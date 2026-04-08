# grafana v1.2 Release Notes

Source: [v1.2.0](https://github.com/grafana/grafana/releases/tag/v1.2.0)

New features:
#70 Grid Thresholds (warning and error regions or lines in graph)
#72 Added an example of a scripted dashboard and a short [wiki article](https://github.com/torkelo/grafana/wiki/Scripted-dashboards) documenting scripted dashboards.

Fixes:
#81 Grid min/max values are ignored bug
#80 "stacked as percent" graphs should always use "max" value of 100 bug
#73 Left Y format change did not work 
#42 Fixes to grid min/max auto scaling
#69 Fixes to lexer/parser for metrics segments like "10-20". 
#67 Allow decimal input for scale function 
#68 Bug when trying to open dashboard while in edit mode
