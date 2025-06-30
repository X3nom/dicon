# DiCoN - Distributed Computing Network
DiCoN is a low-level C library/framework for LAN-based distributed computing clusters.
It enables remote procedure calls (RPC) across networked nodes using a custom lightweight protocol and POSIX multithreading.

Drawing inspiration from CUDA, it provides manual remote memory management (similar to `cudaMalloc` / `cudaMemcpy`).
Functions are dispatched similarly to POSIX `pthread`: you allocate and copy arguments remotely, then invoke the function; it runs in a thread context on a remote machine.
When done, the thread can be joined, sending returned data back to the caller machine.

## Requirements
- all cluster nodes should be on single LAN with stable ethernet connection *(connection drops are highly undesirable since library does NOT handle them at the moment)*
- networks with NAT may cause problems since client connects directly to nodes over TCP sockets
- `IPV4` *(IPV6 not yet supported by* `dicon_sockets.h`*)*

## Workings
Dicon has 3 parts:
1) **diconlib** - the library itself; used by the client program
2) **node** - process running on the cluster nodes; recieves commands over network from client who connects to it
3) **main-server** - retains informatin about online/aviable nodes that client can request; both nodes and client connect to it
