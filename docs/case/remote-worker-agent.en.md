# Remote Worker Agent Example

> Status: English draft, pending review.

This example maps to [`examples/remote_worker_agent.c`](../../examples/remote_worker_agent.c). It demonstrates in-process remote worker control-plane behavior.

## What It Demonstrates

- Worker registration.
- Task enqueue/claim/complete.
- Assignment execution.
- Output/artifact chunk recording.
- Control-plane recovery.

## Key APIs

- `xwork_control_plane_create()`
- `xwork_control_plane_register_worker()`
- `xwork_control_plane_enqueue_task()`
- `xwork_control_plane_claim_task()`
- `xwork_control_plane_complete_task()`
- `xwork_control_plane_execute_next_local()`

## Boundary

The example uses in-process transport. Production networking, authentication, retries, and deployment are host responsibilities.
