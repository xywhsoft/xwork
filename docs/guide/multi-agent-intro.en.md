# Multi-Agent Task Graph

> Status: English draft, pending review.

Multi-Agent support is built around an in-process agent pool and a task graph. It schedules dependencies, runs child tasks, records handoff metadata, and exposes recovery snapshots.

## Flow

```text
agent pool
  agents: planner / coder / tester / reviewer
task graph
  nodes
  dependencies
  handoff metadata
execute
  child runs
  aggregate report
```

## Minimal Agent Pool

```c
xwork_agent_pool_options tPoolOptions;
xwork_agent_options tAgentOptions;
xwork_agent_pool *pPool = NULL;

xwork_agent_pool_options_init(&tPoolOptions);
tPoolOptions.sPoolId = "pool-1";
tPoolOptions.pRuntime = pRuntime;
xwork_agent_pool_create(&tPoolOptions, &pPool);

xwork_agent_options_init(&tAgentOptions);
tAgentOptions.sAgentId = "coder";
tAgentOptions.eRole = XWORK_AGENT_ROLE_CODER;
xwork_agent_pool_add_agent(pPool, &tAgentOptions, NULL);
```

## Boundary

xwork schedules the task graph. It does not invent the product-level planning strategy; planner logic belongs in the product or model layer.

## Next

- [Multi-Agent API](../api/api-multi-agent.en.md)
- [Multi-Agent claw example](../case/multi-agent-claw.en.md)
