# Arborcore Core Retrofit E — Runtime / Event / Server Composition

Reference checkpoint: `e9b69ab5205033dac15128ff7e3fd6d627548cb2`.

## Mandatory runtime repairs

- E1 accept is a prepare/register/commit transaction; post-init failure rolls back to CLOSED.
- E2 epoll duration is a total monotonic deadline across EINTR.
- E3 strict incremental `http_frame_scan` plus the parser frame-length hint removes repeated whole-request parsing during fragmentation while preserving immediate rejection of bare LF/interior bare CR.
- E4 pipelined buffered/kernel work is drained before returning to epoll.
- E5 reusable connection storage is reset before publication and request-local framing state is reset at completion.
- E6 listener NONBLOCK/CLOEXEC is atomic at socket creation; accept4 already supplies atomic accepted-fd flags.
- E7 writes are attempted immediately; EPOLLOUT is armed only on actual backpressure.
- E9 each `server_handle_http_once` invocation is capped at eight completed requests and returns `SERVER_MORE_WORK` when immediate continuation is required; an eight-ready-event epoll test also qualifies batch delivery.

## E8 scatter/gather experiment

A status-200 iovec candidate is byte-equivalence tested and exercised through real Linux `writev(2)` emission. The experimental writer mutates iovec entries as resumable progress state across partial writes/EAGAIN. Production remains on the contiguous response buffer because promotion would add persistent per-connection iovec and digit-scratch lifetime state; the full state/size/backpressure trade-off belongs to the post-ABI response layer rather than being inferred from a syscall microbenchmark alone.

## E10 gate

The final gate requires warning-clean reconstruction, all prior tests, E-specific runtime contracts, ABI/dependency closure, bounded code-size growth, syscall evidence when `strace` is available, E8 evidence, and environment-qualified historical-or-paired server performance.
