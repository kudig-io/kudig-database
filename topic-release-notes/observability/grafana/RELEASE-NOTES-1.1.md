# grafana v1.1 Release Notes

Source: [v1.1.0](https://github.com/grafana/grafana/releases/tag/v1.1.0)

New features: 

#22 Support for native graphite png renderer, does not support click and select zoom yet
#60 Support for legend values (cactiStyle, min, max, current, total, avg). The options for these are found in the new "Axes & Grid" tab for now.
#62 There is now a "New" button in the search/open dashboard view to quickly open a clean empty dashboard. 
#55 Basic auth is now supported for elastic search as well 

some new function definitions added (will focus more on this for next release). 

Fixes
#45 zero values from graphite was handled as null. 
#63 Kibana / Grafana on same host would use same localStorage keys, now fixed
#46 Impossible to edit graph without a name fixed.
#24 fix for dashboard search when elastic search is configured to disable _all field. 
#38 Improvement to lexer / parser to support pure numeric literals in metric segments

Thanks to everyone who contributed fixes and provided feedback :+1: 
