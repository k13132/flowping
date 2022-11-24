FlowPing
===========
## General info
https://flowping.fel.cvut.cz

## Versions
* ver. 3.x.x in Master branch (based on F-Tester edition flowping with basic icmp ping like client only output and rich client/server JSON Output)
* ver. 2.9.x Optimized edition with ipv6 support for [F-Tester](https://f-tester.fel.cvut.cz/en) in FT2-Edition-ipv6_archive branch (Only JSON output is supported
* ver. 1.5.3 Original application in flowping_v1_archive branch 

Note: sample length for JSON output is possible from 1 ms to 1 hour. 0 is reserved for per packet mode output.



ToDo v3:
* signal to all client when server receives SIGQUIT
* close all output files after SIGQUIT
* set connection limit
* dead connection collector