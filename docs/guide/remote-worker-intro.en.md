# Remote Worker and Control Plane

> Status: English draft, pending review.

Remote Worker support models a control plane, worker registry, leases, assignments, remote results, and output/artifact chunks.

## Transport Boundary

| Transport | Meaning |
| --- | --- |
| `XWORK_REMOTE_TRANSPORT_IN_PROCESS` | Control plane and worker share process memory. |
| `XWORK_REMOTE_TRANSPORT_HTTP_BOUNDARY` | Host product owns HTTP/socket/auth/retry; xwork receives decoded messages. |

## Minimal Control Plane

```c
xwork_control_plane_options tPlaneOptions;
xwork_control_plane *pPlane = NULL;

xwork_control_plane_options_init(&tPlaneOptions);
tPlaneOptions.sPlaneId = "plane-1";
tPlaneOptions.pRuntime = pRuntime;
tPlaneOptions.eTransport = XWORK_REMOTE_TRANSPORT_IN_PROCESS;

xwork_control_plane_create(&tPlaneOptions, &pPlane);
xwork_control_plane_start(pPlane);
```

## Boundary

xwork does not include a production cloud control plane. Worker auth, tenant isolation, network protocol implementation, and deployment are host responsibilities.

## Next

- [Remote Worker API](../api/api-remote-worker.en.md)
- [Remote Worker Agent example](../case/remote-worker-agent.en.md)
