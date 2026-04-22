# Multi-Agent claw Example

> Status: English draft, pending review.

This example maps to [`examples/multi_agent_claw.c`](../../examples/multi_agent_claw.c). It demonstrates planner/coder/tester/reviewer fan-out/fan-in through an agent pool and task graph.

## What It Demonstrates

- In-process agent pool.
- Task graph nodes and dependencies.
- Child run mapping.
- Aggregate report artifact.
- Task graph recovery.

## Key APIs

- `xwork_agent_pool_create()`
- `xwork_agent_pool_add_agent()`
- `xwork_task_graph_create()`
- `xwork_task_graph_add_node()`
- `xwork_task_graph_add_dependency()`
- `xwork_task_graph_execute()`

## Extension Points

- Generate task graphs from a planner model.
- Add product-specific handoff metadata.
- Persist and recover long-running multi-agent tasks.
