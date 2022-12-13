FlowPing
===========
## General info
https://flowping.fel.cvut.cz

## Versions
* ver. 3.x.x in Master branch (based on F-Tester edition flowping with basic icmp ping like client only output and rich client/server JSON Output)
  * server can handle multiple connection with multiple output JSON files.
  * When server is terminated it tries to correctly terminate client connections. 
* ver. 2.9.x Optimized edition with ipv6 support for [F-Tester](https://f-tester.fel.cvut.cz/en) in FT2-Edition-ipv6_archive branch (Only JSON output is supported
* ver. 1.5.3 Original application in flowping_v1_archive branch 

Note: sample length for JSON output is possible from 1 ms to 1 hour. 0 is reserved for per packet mode output.
Note: server connection limit is hardwired to 64 simultaneous connections
Note: if no packet arrives to server within 1 hour connection is considered as TIMEOUTed.
