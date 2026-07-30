# Tasks & Timing (WCET Evidence)

The following table summarizes the measured worst-case execution time (WCET), task periods, priorities, deadlines, and processor utilization for each real-time task in the ICU ECG Telemetry Monitor. WCET values were measured during execution using the built-in timing instrumentation.

| Task | Period T | WCET C | U = C/T | Priority | Deadline |
|------|:--------:|:------:|:-------:|:--------:|:--------:|
| Task A → Telemetry Sampler | 50 ms | 0.030 ms | 0.00060 | 8 | 50 ms |
| Task B → Arrhythmia Processor | 50 ms | 0.164 ms | 0.00328 | 8 | 50 ms |
| Task C → Pipeline Coordinator | 50 ms | 0.079 ms | 0.00158 | 9 | 50 ms |
| Task D → Alarm Dispatch Actuator | 50 ms | 0.008 ms | 0.00016 | 12 | 50 ms |

## Total Utilization

\[
U = 0.00060 + 0.00328 + 0.00158 + 0.00016 = 0.00562
\]

- **Total Utilization:** **U = 0.00562**
- **Rate Monotonic (RM):** \(U = 0.00562 < 0.757\) → **Schedulable**
- **Earliest Deadline First (EDF):** \(U < 1.0\) → **Schedulable**

The measured utilization is significantly below the Rate Monotonic utilization bound for four periodic tasks (0.757), indicating that all tasks can meet their deadlines under RMS scheduling. Since the total utilization is also less than 1.0, the task set is likewise schedulable under Earliest Deadline First (EDF).
