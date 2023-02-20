# M80: An Ultrafast FaaS Implementation

Functions as a services (FaaS) is simple: Users provide a function, and the service provider executes it. Usually, FaaS is used to respond to HTTP events.

One big problem FaaS providers face is isolating different' users functions. Here are a few techniques used today:
- Amazon uses [Firecracker](https://firecracker-microvm.github.io/) to isolate functions. Firecracker runs functions in ultralight VMs.
- [Cloudflare uses V8 isolates](https://blog.cloudflare.com/cloud-computing-without-containers/). This means that a functions must be written in JavaScript or WebAssembly.

This approach is problematic. The `toy-lambda` example in this repository contains a `hello-world` serverless benchmark. With V8 isolates, here are the benchmark stats:
```
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency   224.44ms   38.06ms   1.07s    89.85%
    Req/Sec    70.53     16.39   130.00     66.75%
  8460 requests in 30.04s, 247.88KB read
```

Meanwhile, using a user-provided shared library, here are the benchmark stats:
```
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency    15.60ms   32.36ms 841.78ms   95.56%
    Req/Sec     1.61k   367.35     2.32k    84.48%
  188699 requests in 30.05s, 5.40MB read
```

The results are astounding: The shared library approach reduces latency by more than 14x, and requests/second increases by more than 22x.

M80's idea is to move closer to the second set of numbers by using *software fault isolation*. Software fault isolation allows users to execute untrusted third-party binaries with the confidence that the binaries don't do anything "bad".

The key challenge is how to have confidence the SFI mechanism is correct? M80's approach is to use F* to implement a formally verified SFI environment. Equipped with this SFI mechanism (and possibly some runtime monitoring), we obtain massive savings for FaaS providers.
