**<u>Theme:</u>** Medical ICU Bedside ECG Telemetry & Nurse Call System – Real- Time Systems Final Capstone 

A dual-core FreeRTOS bedside monitor that simulates ECG/HR/O2 telemetry through a producer, consumer, coordinator, responder chain built to demonstrate the detection of arrhythmias and hypoxemia in real-time with hardware nurse-call override and demonstrate the deterministic embedded IPC design for an embedded/firmware engineering role in relation to medical device software. 

# **<u>Demo:</u>** 

- **Video Link: https://youtu.be/sQ4TaH5XAuQ** 

- **Live Wokwi:** <u>https://wokwi.com/projects/470915869059743745</u> 

# **<u>Architecture:</u>** 

- The way that this application works is that sensor data is sampled every 50ms by task A and placed into a FreeRTOS queue. Then it's consumed by task B which then performs the arrhythmia detection and then updates the shared patient state. After each is produced and the processing is complete, then the event group synchronizes task C which notifies task D to dispatch the alarm or the nurse-call responses. All the health metrics including the WCET, queue depth, and fault counters are then exported to the core 0 monitoring 



<!-- Start of picture text -->
Final Integration Capstone — Production-Grade Concurrency Architecture GB 'rpiemented in cxrent emware<br>ICU Bedside ECG Telemetry & Nurse Call System — tasks- ISR - cores -IPC -guards -watchdogs - SOH {1 Recommended addition (guard/ watchdog)<br>CORE 1 — Real-Time IPC Pipeline (deterministic, no network I/O) CORE 0 — Observability & Supervision Plane<br>TASK A — Producer .<br>Telemetry Sampler - Prio 8 Monitor Task<br>20 Hz (50 ms) - hb_a, WCET_a Serial (1 Hz) or HTTP dashboard<br>sets EV_BIT_DATA PRODUCED ‘ask sates Sys PRODUCED bat drecily Prio 4-6 - non-real-time, WEFVHTTP<br>aae (Jp) AcquiresReads SOHG1 plane everybefore reading cycle<br>wert last_sample/last_verdict [ADD]<br>data_q— depth 10 a never blocks Core1 scheduling<br>typed FIFO, xQueueSend 10ms timeout —- dfop ae<br>wee ° IDLEO Watchdog<br>porn<br>TASK B — Consumer | G1—#ortMUXnepernenespinlock sees ec ene nc ec tees,| \ eeue gFy ESP-IDF default - Core 0 starvation guard<br>‘Arrhythmia Processor . Prio 8 : Guardsfast_sample / last_verdict ry 8<br>100 ms max wait - hb_b, WCET_b +—{shareg, Se ate H rc +)<br>sets EV_BIT_DATA_PROCESSED Sa eee Why this split matters (rubric Q1)<br>F W.FVHTTP work is unbounded Jatency —<br>: \ pinning it to Core 0 keeps Core 1's 50ms<br>evt_group‘AND-wait: PRODUCED & PROCESSED H:; omedseas \| SOHhb_a.d Telemetry Plane samplingstateimmuneacross to deadiine network cores and jitter.is why rendezvous Reading G1 exists: sharedtimingwithout<br>clear-on-exit, portMAX_DELAY 4 {must grow monotonically) it, the monitor can read a tom/half-wnitten<br>H WCET_max per task (us) last_sample mid-update from Task B<br>data_q depth / evt_group bits L J<br>TASK C — Coordinator<br>earsRendezvousxTaskNotifyGive(responder} both bits- Prio- hb_c,9  WCET_c oy!SS ullaskNotifyTakereadsalarm last_verdict/last_sampleDispaich Actuator- hb_d, WCET_d- Prio 12 dropped.etmanual_nurse_calls samples Fault* Queue posture full > sample summary dropped, counted :<br>H Girect notify from JSR H thisRead-onlyis the fauit/heaithexport to Coresurface 0 — ++ Task Cross-co sta r vatione race +—> caughtprevented by TWDT by G1 [ADD]<br>GP1I018 ISR — Nurse* Call :H an operator or dashboard checks. +  Idle-taskManual escalation lockup + —caughtnurse-call by IDLEa’tISR,  WOTalways<br>IRAM_ATTR, NEGEUGE, 200ms debounce ' live regardless of pipeline state<br>\VTaskNotifyGiveF om SR = Task D H . J<br>28010 each cyclo — allo A,B, C.D feed the TWOT v<br>! Task Watchdog Timer (TWDT) [ADD] 4 IDLE1 Watchdog<br>' esp_task_wdt_add(A,B,C,D) at init; each task calls Hl ESP-IDF default<br>' esp_task_wdt_reset() once per cycle — catches a stalled/deadlocked task Hl Core 1 starvation guard<br>Rubric coverage<br>Tasks (A-D, priorities, cores) ISR_(GPIO18, debounced, IRAM_ATTR) Cores (0 vs 1 spilt, rationale above)<br>IPC (queue depth-10, event-group AND-wait, direct task notification) SOH (heartbeats + WCET + fault counters, exported read-only to Core 0)<br>Guards + Watchdogs— flagged as [ADD] above: current code has no spinlock around last_sample/last_verdict and no esp_task_wdt registration. Add both for full production-grade credit.<br><!-- End of picture text -->



<!-- Start of picture text -->
Task A — producer<br>20 Hz telemetry sampling<br>( ><br>Queue<br>depth 10, typed FIFO<br>XX /<br>Producer also sets<br>PRODUCED bit directly<br>Task B — consumer<br>arrhythmia threshold check<br>Event group<br>wait for both bits<br>oe)<br>Task C — coordinator Button ISR<br>rendezvous, clears bits nurse call, debounced<br>direct notify<br>direct notify<br>from ISR<br>Task D — responder<br>alert actuator, prio 12<br><!-- End of picture text -->



<!-- Start of picture text -->
Core 1 Core 0<br>Real-time IPC pipeline Observability plane<br>Task A — producer- 20 Hz- prio 8 Serial monitor (1 Hz)<br>Task B — consumer- prio. 8 or HTTP web server<br>Task C — coordinator - prio 9 repad:= eventReads queuebits, heartbeats depth,<br>Task D — responder- prio 12<br>Shared IPC: queue (depth 10),<br>event group<br>direct notify — Task D<br>Button ISR (GPIO 18)<br>debounced, nurse call<br><!-- End of picture text -->



<!-- Start of picture text -->
@ Task table & WCET utilization calculator<br>Enter each periodic task with its period T and measured worst-case execution time C (same time unit). This computes per-task utilization<br>| U;=C/T, total utilization, and checks it against the schedulability bounds. Export the finished table into your portfolio as your WCET evidence. |<br>TASK PERIOD T WCET C Uy = C/T DEL<br>A-> Telemetry Sampler 50ms Q.030ms 0.001 ~«<br>B-> Arrhythmia Processor 50ms 0.164ms 0.003 x<br>C->Pipeline Coordinator 50ms 0.079ms 0.002 <x<br>D-> Alarm Dispatch Actuator 50ms 0.008ms 0.000 ~<br>Reset example<br>0.01 TOTAL UTILIZATION U = XCz/Tz<br>JY U=0.006 < RM bound (0.757). Schedulable under Rate-Monotonic and EDF (independent periodic tasks, deadline = period).<br><!-- End of picture text -->

|**Task**|**Period T**|**WCET C**|**U = C/T**|**Priority **|**Deadline**|
|---|---|---|---|---|---|
|Task A -> Telemetry<br>Sampler|50 ms|0.030 ms|0.0006|8|50 ms|
|Task B -><br>Arrhythmia<br>Processor|50 ms|0.164 ms|0.00328|8|50 ms|
|Task C -> Pipeline<br>Coordinator|50 ms|0.079 ms|0.00158|9|50 ms|
|Task D -> Alarm<br>Dispatch Actuator|50 ms|0.008 ms|0.00016|12|50 ms|



- Total Utilization U = 0.00060 + 0.00328 + 0.00158 + 0.00016 = 0.00562 

- Rate Monotonic: U = 0.00562 < 0.757 -> schedulable! 

- Earliest Deadline: Since U < 1.0 then the task set is schedulable under EDF. 

# **<u>Task Table & WCET Utilization Calculator Values from Demo Run:</u>** 

== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline === 

Telemetry: Status=NORMAL | HR=60 BPM | SpO2=99% | ECG=-100 mV 

IPC State: Queue Depth=0/10 | EventBits=0x00 

Task                           Period   Priority  Heartbeats   WCET(us) 

A: Telemetry Sampler (Prod)    50 ms    8         1            19 

B: Arrhythmia Process (Cons)   Queue    8         1            4 

C: Pipeline Coordinator        Rendezvous 9         1 

D: Alarm Dispatch Actuator     Notif    12        0            0 

Stats: Auto Alerts=0 | Nurse Calls=0 | Drops=0 

=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline === 

Telemetry: Status=NORMAL | HR=79 BPM | SpO2=98% | ECG=90 mV 

IPC State: Queue Depth=0/10 | EventBits=0x00 

Task                           Period   Priority  Heartbeats   WCET(us) 

A: Telemetry Sampler (Prod)    50 ms    8         21           30 

B: Arrhythmia Process (Cons)   Queue    8         21           164 

C: Pipeline Coordinator        Rendezvous 9         21           79 

D: Alarm Dispatch Actuator     Notif    12        20           8 

Stats: Auto Alerts=3 | Nurse Calls=0 | Drops=0 

=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline === 

Telemetry: Status=NORMAL | HR=99 BPM | SpO2=96% | ECG=90 mV IPC State: Queue Depth=0/10 | EventBits=0x00 

Task                           Period   Priority  Heartbeats   WCET(us) 

A: Telemetry Sampler (Prod)    50 ms    8         41           30 

B: Arrhythmia Process (Cons)   Queue    8         41           164 

C: Pipeline Coordinator        Rendezvous 9         41           79 

D: Alarm Dispatch Actuator     Notif    12        41           8 

Stats: Auto Alerts=6 | Nurse Calls=1 | Drops=0 

=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline === 

Telemetry: Status=ARRHYTHMIA | HR=64 BPM | SpO2=94% | ECG=90 mV 

IPC State: Queue Depth=0/10 | EventBits=0x00 

Task                           Period   Priority  Heartbeats   WCET(us) 

A: Telemetry Sampler (Prod)    50 ms    8         61           30 

B: Arrhythmia Process (Cons)   Queue    8         61           164 

C: Pipeline Coordinator        Rendezvous 9         61           79 

D: Alarm Dispatch Actuator     Notif    12        62           8 

Stats: Auto Alerts=21 | Nurse Calls=2 | Drops=0 

=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline === 

Telemetry: Status=NORMAL | HR=84 BPM | SpO2=98% | ECG=90 mV 

IPC State: Queue Depth=0/10 | EventBits=0x00 

Task                           Period   Priority  Heartbeats   WCET(us) 

A: Telemetry Sampler (Prod)    50 ms    8         81           30 

B: Arrhythmia Process (Cons)   Queue    8         81           164 

C: Pipeline Coordinator        Rendezvous 9         81           79 

D: Alarm Dispatch Actuator     Notif    12        83           8 

Stats: Auto Alerts=24 | Nurse Calls=3 | Drops=0 

=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline === 

Telemetry: Status=ARRHYTHMIA | HR=104 BPM | SpO2=96% | ECG=90 mV IPC State: Queue Depth=0/10 | EventBits=0x00 

Task                           Period   Priority  Heartbeats   WCET(us) 

A: Telemetry Sampler (Prod)    50 ms    8         101          30 

B: Arrhythmia Process (Cons)   Queue    8         101          164 C: Pipeline Coordinator        Rendezvous 9         101          79 

D: Alarm Dispatch Actuator     Notif    12        103          8 Stats: Auto Alerts=32 | Nurse Calls=3 | Drops=0 

=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline === 

Telemetry: Status=ARRHYTHMIA | HR=69 BPM | SpO2=94% | ECG=90 mV 

IPC State: Queue Depth=0/10 | EventBits=0x00 

Task                           Period   Priority  Heartbeats   WCET(us) 

A: Telemetry Sampler (Prod)    50 ms    8         121          30 

B: Arrhythmia Process (Cons)   Queue    8         121          164 C: Pipeline Coordinator        Rendezvous 9         121          79 

D: Alarm Dispatch Actuator     Notif    12        124          8 

Stats: Auto Alerts=43 | Nurse Calls=4 | Drops=0 

=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline === Telemetry: Status=NORMAL | HR=89 BPM | SpO2=98% | ECG=90 mV IPC State: Queue Depth=0/10 | EventBits=0x00 

Task                           Period   Priority  Heartbeats   WCET(us) 

A: Telemetry Sampler (Prod)    50 ms    8         141          30 B: Arrhythmia Process (Cons)   Queue    8         141          164 C: Pipeline Coordinator        Rendezvous 9         141          79 

D: Alarm Dispatch Actuator     Notif    12        144          8 

Stats: Auto Alerts=46 | Nurse Calls=4 | Drops=0 

=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline === 

Telemetry: Status=ARRHYTHMIA | HR=109 BPM | SpO2=96% | ECG=90 mV 

IPC State: Queue Depth=0/10 | EventBits=0x00 

Task                           Period   Priority  Heartbeats   WCET(us) 

A: Telemetry Sampler (Prod)    50 ms    8         161          30 

B: Arrhythmia Process (Cons)   Queue    8         161          164 

C: Pipeline Coordinator        Rendezvous 9         161          79 

D: Alarm Dispatch Actuator     Notif    12        165          8 Stats: Auto Alerts=58 | Nurse Calls=5 | Drops=0 

=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline === 

Telemetry: Status=ARRHYTHMIA | HR=74 BPM | SpO2=94% | ECG=90 mV IPC State: Queue Depth=0/10 | EventBits=0x00 

Task                           Period   Priority  Heartbeats   WCET(us) 

A: Telemetry Sampler (Prod)    50 ms    8         181          30 

B: Arrhythmia Process (Cons)   Queue    8         181          164 

C: Pipeline Coordinator        Rendezvous 9         181          79 

D: Alarm Dispatch Actuator     Notif    12        185          8 

Stats: Auto Alerts=65 | Nurse Calls=5 | Drops=0 

=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline === 

Telemetry: Status=NORMAL | HR=94 BPM | SpO2=98% | ECG=90 mV 

IPC State: Queue Depth=0/10 | EventBits=0x00 

Task                           Period   Priority  Heartbeats   WCET(us) 

A: Telemetry Sampler (Prod)    50 ms    8         201          30 

B: Arrhythmia Process (Cons)   Queue    8         201          164 C: Pipeline Coordinator        Rendezvous 9         201          79 

D: Alarm Dispatch Actuator     Notif    12        205          8 

Stats: Auto Alerts=68 | Nurse Calls=5 | Drops=0 

# **<u>Induced-failure Prediction vs. Observation Analysis:</u>** 

I decided to test what rapidly pressing the button does to the outputs to the terminal. I predicted initially that the GPIO ISR would remain responsive since it should perform minimal work and it should also wake up task D whiles using direct task notification. Whiles I tested it, I noticed that the nurse-call counter increased every time I pressed the button as it should, and I noticed that the telemetry pipeline continued to properly update normally. All other measurements and readings that were displayed to the terminal normally as well. This helped me to conclude that 

my initial hypothesis was right, and the GPIO ISR does remain responsive. 

# **<u>Hazard Analysis & Standard Mapping:</u>** 

|<br>**Hazard**|<br>**Potential Effect**|<br>**Mitigation Implemented**|**Standard Mapping**|
|---|---|---|---|
|Telemetry queue<br>overflow|Patient telemetry<br>samples may be<br>dropped -><br>delays the<br>arrhythmia<br>detection|Queue depth of 10 provides a<br>500 ms buffer. Queue failures<br>increment a dropped-sample<br>counter and are monitored<br>continuously, so overload is<br>visible during testing.|IE**IEC 62304 §5.5**–<br>Software<br>implementation and<br>defensive error<br>handling|
|Missed<br>arrhythmia<br>detection due to<br>delayed<br>processing|Critical cardiac<br>events may not<br>trigger an alarm<br>in time.|Producer, consumer,<br>coordinator, and responder<br>execute deterministic<br>FreeRTOS tasks with<br>measured WCET, fixed<br>priorities, and schedulability<br>analysis (RMS/EDF).|**IEC 62304 Timing**<br>and implementation<br>controls|
|ISR blocking or<br>excessive<br>interrupt latency|Nurse-call button<br>becomes<br>unresponsive<br>during<br>emergencies|<br>ISR performs only minimal<br>work, debounces the button,<br>and wakes the responder task<br>using Direct Task Notification<br>instead of performing lengthy<br>processing inside the interrupt.|**IEC 62304 §5.5**–<br>Safe software<br>implementation<br>practices|
|Race conditions<br>between tasks|Incorrect patient<br>state or<br>inconsistent<br>alarm decisions.|<br>Queue transfers telemetry<br>safely, Event Groups<br>synchronize processing stages,<br>and Direct Task Notifications<br>provide one-to-one signaling,<br>preventing unsafe shared-state<br>communication.|**IEC 62304 §5.5**–<br>Controlled inter-<br>process<br>communication|
|False alarm<br>generation|Alarm fatigue<br>may reduce<br>operator<br>confidence.|Alarm is generated only after<br>telemetry is processed, and<br>rhythm thresholds indicate<br>arrhythmia or hypoxemia.<br>Coordinator ensures the<br>complete processing pipeline<br>finishes before responder<br>activation.|**IEC 62304 §5.7**–<br>Software system<br>testing and<br>verification|
|Loss of system<br>observability|Software failures<br>become difficult<br>to diagnose<br>during operation.|Core 0 continuously reports<br>queue depth, event bits,<br>heartbeats, WCET, dropped<br>samples, and alert counters<br>without interfering with real-<br>time executiononCore1.|**IEC 62304 §5.8**–<br>Verification and<br>maintenance support|



|Task execution<br>exceeding<br>deadline|Real-time<br>pipeline could<br>become<br>nondeterministic<br>and delay alarms.|Maximum WCET is measured<br>for every task, utilization is<br>calculated, and the task set is<br>verified as schedulable under<br>both RMS and EDF before<br>deployment.|**IEC 62304 §5.5**–<br>Real-time<br>implementation<br>verification|
|---|---|---|---|
|Failure of<br>monitoring<br>interface|Loss of<br>diagnostics even<br>if the telemetry<br>pipeline<br>continues<br>running.|Monitoring executes<br>independently on Core 0 while<br>the medical pipeline remains<br>isolated on Core 1, preventing<br>monitoring failures from<br>affecting patient data<br>processing.|**EC 62304 §5.3**–<br>Software<br>architectural design<br>and separation of<br>concerns|



# **<u>Graceful Degradation:</u>** 

If the telemetry processor can't keep up with the incoming data, then the queue will buffer up to 10 samples before additional samples become dropped. The system will detect if this happens through the queue depth as well as the dropped-sample counter whiles all the other real-time tasks are still running. This allows all the other telemetry information, alarms, and the nurse-call functionality to remain intact instead of causing a system wide failure. 

# **<u>Build & Run:</u>** 

For this project, the tool chain is the ESP-IDF with FreeRTOS, and the board is ESP32-S3 which I ran via the WOKWI simulation. In order to run this simulation, you can run it via WOWKI or clone all the information that will be dropped in this repositor to build it using ESP-IDF. When you run this simulation, you should see the serial monitor view with the live telemetry, task heartb3eats, WCET measurements, and all other appropriate readings. Pressing the GPIO button 

simulates a nurse-call interrupt, and then you can observe that notification updated on the serial monitor. 

# **<u>Tailored For:</u>** 

This particular application is mainly for an embedded/real-time system. This project shows all of the skills we've acquired through the duration of this class. It focuses on core skills in embedded and firmware engineering including FreeRTOS task scheduling, dual-core programming, interprocess communication, interrupt handling, WCET analysis, watchdog monitoring, and the most important a deterministic real-time system design. All of these techniques are used to help us further understand the importance of a real-world example, in this case, it's the medical world. 

# **<u>What I copied from APP 1/2:</u>** 

```
#if USE_WEBSERVER
```

```
/* ============================================================
```

```
 *  WEB MONITOR (Core 0) [USE_WEBSERVER = 1]
```

```
 * ============================================================ */
static esp_err_t handle_root(httpd_req_t *req)
{
```

```
=
    UBaseType_t depth  uxQueueMessagesWaiting(data_q);
```

```
=
    EventBits_t bits   xEventGroupGetBits(evt_group);
```

```
char buf[2048];
int n = snprintf(buf,sizeof(buf),
"<!DOCTYPE html>"
"<html lang=\"en\"><head>"
"<meta charset=\"utf-8\"><meta http-equiv=\"refresh\" content=\"1\">"
"<title>ICU ECG Monitor · Dual-Core IPC Pipeline</title>"
"<style>"
"  body { font-family: -apple-system, sans-serif; background: #FAFAF5; "
"         color: #1A1A1A; padding: 1.5rem; }"
```

```
"  h1 { color: #6B4F09; border-bottom: 3px solid #FFC904; "
"       display: inline-block; padding-bottom: 4px; }"
```

```
"  table { border-collapse: collapse; margin: 1rem 0; width: 100%%; max-
width: 800px; }"
"  th { background: #1A1A1A; color: #FFC904; padding: 8px 14px; "
"       text-align: left; font-size: 12px; text-transform: uppercase; }"
"  td { padding: 8px 14px; border-bottom: 1px solid #ddd; }"
```

```
"  td.num { font-variant-numeric: tabular-nums; font-weight: 700; color:
#6B4F09; }"
"  .card { background: white; padding: 15px; margin-bottom: 15px; border-
radius: 6px; "
"          box-shadow: 0 2px 5px rgba(0,0,0,0.08); max-width: 800px; }"
"  .alert { color: #e74c3c; font-weight: bold; }"
```

```
"  .normal { color: #2ecc71; font-weight: bold; }"
```

```
"</style></head>"
```

```
"<body>"
```

- <mark>`"<h1>ICU ECG Telemetry &mdash; Dual-Core IPC Pipeline</h1>"`</mark> 

```
"<div class=\"card\">"
```

```
"  <h3>Current Telemetry Payload (Bed %lu)</h3>"
```

```
"  <p>Status: <span class=\"%s\">%s</span></p>"
```

```
"  <p><b>Heart Rate:</b> %u BPM | <b>SpO2:</b> %u%% | <b>ECG Wave:</b> %d
mV</p>"
```

- <mark>`"  <p><b>IPC Queue Depth:</b> %u / %d | <b>Event Bits:</b> 0x%02X</p>" "</div>"`</mark> 

```
"<table>"
```

   - <mark>`"<thead><tr><th>Task</th><th>Role</th><th>Period / Trigger</th>"`</mark> 

   - <mark>`"<th>Priority</th><th>Heartbeats</th><th>WCET`</mark> 

- <mark>`(&micro;s)</th></tr></thead>"`</mark> 

```
"<tbody>"
```

```
"<tr><td>Task A &mdash; Telemetry Sampler</td><td>Producer</td>"
"<td>50 ms</td><td>8</td>"
"<td class=\"num\">%lu</td><td class=\"num\">%llu</td></tr>"
"<tr><td>Task B &mdash; Arrhythmia Process</td><td>Consumer</td>"
"<td>Queue Event</td><td>8</td>"
"<td class=\"num\">%lu</td><td class=\"num\">%llu</td></tr>"
"<tr><td>Task C &mdash; Pipeline Coordinator</td><td>Rendezvous</td>"
"<td>Event Group</td><td>9</td>"
"<td class=\"num\">%lu</td><td class=\"num\">%llu</td></tr>"
"<tr><td>Task D &mdash; Alarm Dispatcher</td><td>Actuator</td>"
"<td>Notification</td><td>12</td>"
"<td class=\"num\">%lu</td><td class=\"num\">%llu</td></tr>"
"</tbody></table>"
```

```
"<div class=\"card\">"
```

```
"  <p><b>Auto Arrhythmia Alerts:</b> %lu | <b>Manual Nurse Calls:</b> %lu
```

```
| <b>Dropped Samples:</b> %lu</p>"
```

```
"</div>"
```

```
"<p style=\"font-size: 0.85rem; color: #666;\">Auto-refreshing every 1 s
```

```
&mdash; heartbeats must grow monotonically.</p>"
```

```
"</body></html>",
```

```
(unsignedlong)last_sample.patient_id,
```

```
(last_verdict == RHYTHM_ARRHYTHMIA)?"alert":"normal",
```

```
(last_verdict == RHYTHM_ARRHYTHMIA)?"ARRHYTHMIA DETECTED":"RHYTHM
```

```
NORMAL",
```

```
.heart_rate_bpm, last_sample.spo2_percent, last_sample.ecg_mv
(unsigned)depth, QUEUE_DEPTH,(unsigned)bits,
```

```
(unsignedlong)hb_a,(unsignedlonglong)wcet_a_max_us,
```

```
(unsignedlong)hb_b,(unsignedlonglong)wcet_b_max_us,
```

```
(unsignedlong)hb_c,(unsignedlonglong)wcet_c_max_us,
```

```
(unsignedlong)hb_d,(unsignedlonglong)wcet_d_max_us,
```

```
(unsignedlong)arrhythmia_alerts,(unsignedlong)manual_nurse_calls,
```

```
(unsignedlong)dropped_samples);
```

```
    httpd_resp_set_type(req,"text/html");
    httpd_resp_send(req, buf, n);
return ESP_OK;
```

```
}
```

```
static httpd_handle_t start_webserver(void)
{
```

```
    httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
    cfg.server_port   = HTTP_PORT;
    cfg.core_id       =0;
    cfg.task_priority =5;
    cfg.stack_size    =8192;
    httpd_handle_t s   = NULL;
if(httpd_start(&s,&cfg)== ESP_OK){
```

```
={.uri="/",.method=HTTP_GET,
```

```
.handler=handle_root,.user_ctx=NULL };
```

```
(s,&root);
```

```
        ESP_LOGI(TAG,"[WEBMON] HTTP server started on port %d", HTTP_PORT);
}
return s;
}
staticvoid wifi_event_handler(void*arg, esp_event_base_t base,
int32_t id,void*data)
```

```
{
```

```
if(base == WIFI_EVENT && id == WIFI_EVENT_STA_START)
esp_wifi_connect();
```

```
elseif(base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
esp_wifi_connect();
```

```
elseif(base == IP_EVENT   && id == IP_EVENT_STA_GOT_IP){
```

```
*e =(ip_event_got_ip_t *)data;
```

```
        ESP_LOGI(TAG,"[WEBMON] IP Address assigned: " IPSTR, IP2STR(&e-
>ip_info.ip));
```

```
();
```

```
}
```

```
}
```

```
staticvoid wifi_init_sta(void)
```

```
{
```

```
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
```

```
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
```

```
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
wifi_event_handler, NULL);
```

```
    esp_event_handler_register(IP_EVENT,   IP_EVENT_STA_GOT_IP,
wifi_event_handler, NULL);
```

```
    wifi_config_t cfg ={.sta ={.ssid = WIFI_SSID,.password = WIFI_PASS,
.threshold.authmode = WIFI_AUTH_OPEN }};
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA,&cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
}
```

```
staticvoid webmonitor_task(void*arg)
{
```

```
    wifi_init_sta();
for(;;){
        vTaskDelay(pdMS_TO_TICKS(1000));
}
}
#else/* USE_WEBSERVER == 0 */
```

```
/* ============================================================
```

```
 *  TERMINAL MONITOR (Core 0) [USE_WEBSERVER = 0]
 * ============================================================ */
staticvoid task_monitor(void*arg)
{
```

```
    TickType_t last = xTaskGetTickCount();
```

```
const TickType_t period = pdMS_TO_TICKS(1000);
```

```
for(;;){
```

```
=
        UBaseType_t depth  uxQueueMessagesWaiting(data_q);
```

```
=
        EventBits_t bits   xEventGroupGetBits(evt_group);
```

```
        printf("\n=== ICU ECG Telemetry Monitor · Dual-Core IPC Pipeline ===\n"
        printf("Telemetry: Status=%s | HR=%u BPM | SpO2=%u%% | ECG=%d mV\n",
(last_verdict == RHYTHM_ARRHYTHMIA)?"ARRHYTHMIA":"NORMAL",
               last_sample.heart_rate_bpm, last_sample.spo2_percent,
last_sample.ecg_mv);
        printf("IPC State: Queue Depth=%u/%d | EventBits=0x%02X\n",
(unsigned)depth, QUEUE_DEPTH,(unsigned)bits);
        printf("%-30s %-8s %-9s %-12s %-10s\n",
```

```
"Task","Period","Priority","Heartbeats","WCET(us)");
        printf("%-30s %-8s %-9d %-12lu %-10llu\n",
"A: Telemetry Sampler (Prod)","50 ms",8,(unsignedlong)hb_a,
(unsignedlonglong)wcet_a_max_us);
        printf("%-30s %-8s %-9d %-12lu %-10llu\n",
"B: Arrhythmia Process (Cons)","Queue",8,(unsignedlong)hb_b,
(unsignedlonglong)wcet_b_max_us);
        printf("%-30s %-8s %-9d %-12lu %-10llu\n",
```

```
"C: Pipeline Coordinator","Rendezvous",9,(unsigned
long)hb_c,(unsignedlonglong)wcet_c_max_us);
        printf("%-30s %-8s %-9d %-12lu %-10llu\n",
"D: Alarm Dispatch Actuator","Notif",12,(unsignedlong)hb_d,
(unsignedlonglong)wcet_d_max_us);
        printf("Stats: Auto Alerts=%lu | Nurse Calls=%lu | Drops=%lu\n",
(unsignedlong)arrhythmia_alerts,(unsigned
long)manual_nurse_calls,(unsignedlong)dropped_samples);
        vTaskDelayUntil(&last, period);
}
}
#endif/* USE_WEBSERVER */
```

**<u>What I copied from APP 3:</u>** 

```
staticvolatileint64_t last_edge_us =0;
```

```
staticvoid IRAM_ATTR button_isr(void*arg)
```

```
{
```

```
int64_t now = esp_timer_get_time();
```

```
if(now - last_edge_us <200000)return;/* 200 ms software debounce */
    last_edge_us = now;
```

```
    manual_nurse_calls++;
```

```
if(responder_handle != NULL){
        BaseType_t woken = pdFALSE;
```

```
        vTaskNotifyGiveFromISR(responder_handle,&woken);
        portYIELD_FROM_ISR(woken);
}
}
```

```
/* 4. Hardware Trigger ISR (Nurse Call Button) */
=
    gpio_config_t cfg {
```

```
.pin_bit_mask =1ULL<< BUTTON_GPIO,
.mode         = GPIO_MODE_INPUT,
.pull_up_en   = GPIO_PULLUP_ENABLE,
.intr_type    = GPIO_INTR_NEGEDGE,
```

```
};
    gpio_config(&cfg);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_GPIO, button_isr, NULL);
```

```
staticvoid responder_task(void*arg)
{
```

```
for(;;){
uint32_t n = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
if(n ==0)continue;
```

```
=
uint64_t t_start  esp_timer_get_time();
```

```
if(last_verdict == RHYTHM_ARRHYTHMIA){
            ESP_LOGW(TAG,"[ACTUATOR] CRITICAL VITAL ALARM! Bed: %lu | HR: %u BPM
| SpO2: %u%% | Triggers: %lu",
```

```
(unsignedlong)last_sample.patient_id,
                     last_sample.heart_rate_bpm,
                     last_sample.spo2_percent,
(unsignedlong)n);
}else{
            ESP_LOGI(TAG,"[ACTUATOR] Routine cycle check normal. Bed: %lu |
HR: %u BPM | SpO2: %u%%",
```

```
(unsignedlong)last_sample.patient_id,
```

```
                     last_sample.heart_rate_bpm,
                     last_sample.spo2_percent);
}
```

```
        hb_d++;
        MEASURE_WCET(t_start, wcet_d_max_us);
}
}
```

```
/* Utility macro for WCET measurement */
```

```
#define MEASURE_WCET(start_time, max_var)do{                            \
uint64_t elapsed = esp_timer_get_time()- start_time;                 \
if(elapsed > max_var){ max_var = elapsed;}                         \
}while(0)
```

# **<u>AI Disclosure:</u>** 

- For this final assignment, I used my application 5 to complete this portfolio. Due to this, 

here is the link for all the AI that was used in creating Application 5: 

   - https://claude.ai/share/f894576d-e2f5-4796-92c5-66ab4d012b8b 

- I also used AI to help me build concurrency diagrams and to ensure that I correctly have all the information required by the rubric. : 

   - https://claude.ai/share/4937d5fe-8733-46cd-bfa3-cbf9383fc040 

- I also used ChatGPT to help me build my hazard analysis and standard mapping table 

based on all the information I already possessed and then convert that information to 

my .md file. 

   - https://chatgpt.com/share/6a6bef51-6c5c-83ea-8b15-94c92d89099d 

- I also used AI to help me build my index.html file so that everything gets displayed 

correctly. 

- https://claude.ai/share/0c509195-ef29-4210-ab5a-2b9be6cc7f66 

