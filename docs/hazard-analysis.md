# Hazard Analysis & Standard Mapping

  ---------------------------------------------------------------------------------
  Hazard            Potential Effect   Mitigation Implemented    Standard Mapping
  ----------------- ------------------ ------------------------- ------------------
  **Telemetry queue Patient telemetry  Queue depth of 10         **IEC 62304 §5.5**
  overflow**        samples may be     provides a 500 ms buffer. -- Software
                    dropped, delaying  Queue failures increment  implementation and
                    arrhythmia         a dropped-sample counter  defensive error
                    detection.         and are monitored         handling
                                       continuously so overload  
                                       is visible during         
                                       testing.                  

  **Missed          Critical cardiac   Producer, consumer,       **IEC 62304 §5.5**
  arrhythmia        event may not      coordinator, and          -- Timing and
  detection due to  trigger an alarm   responder execute as      implementation
  delayed           in time.           deterministic FreeRTOS    controls
  processing**                         tasks with measured WCET, 
                                       fixed priorities, and     
                                       schedulability analysis   
                                       (RMS/EDF).                

  **ISR blocking or Nurse-call button  ISR performs only minimal **IEC 62304 §5.5**
  excessive         becomes            work, debounces the       -- Safe software
  interrupt         unresponsive       button, and wakes the     implementation
  latency**         during             responder task using      practices
                    emergencies.       Direct Task Notification  
                                       instead of performing     
                                       lengthy processing inside 
                                       the interrupt.            

  **Race conditions Incorrect patient  Queue transfers telemetry **IEC 62304 §5.5**
  between tasks**   state or           safely, Event Groups      -- Controlled
                    inconsistent alarm synchronize processing    inter-process
                    decisions.         stages, and Direct Task   communication
                                       Notifications provide     
                                       one-to-one signaling,     
                                       preventing unsafe         
                                       shared-state              
                                       communication.            

  **False alarm     Alarm fatigue may  Alarm is generated only   **IEC 62304 §5.7**
  generation**      reduce operator    after telemetry is        -- Software system
                    confidence.        processed and rhythm      testing and
                                       thresholds indicate       verification
                                       arrhythmia or hypoxemia.  
                                       Coordinator ensures the   
                                       complete processing       
                                       pipeline finishes before  
                                       responder activation.     

  **Loss of system  Software failures  Core 0 continuously       **IEC 62304 §5.8**
  observability**   become difficult   reports queue depth,      -- Verification
                    to diagnose during event bits, heartbeats,   and maintenance
                    operation.         WCET, dropped samples,    support
                                       and alert counters        
                                       without interfering with  
                                       real-time execution on    
                                       Core 1.                   

  **Task execution  Real-time pipeline Maximum WCET is measured  **IEC 62304 §5.5**
  exceeding         could become       for every task,           -- Real-time
  deadline**        nondeterministic   utilization is            implementation
                    and delay alarms.  calculated, and the task  verification
                                       set is verified as        
                                       schedulable under both    
                                       RMS and EDF before        
                                       deployment.               

  **Failure of      Loss of            Monitoring executes       **IEC 62304 §5.3**
  monitoring        diagnostics even   independently on Core 0   -- Software
  interface**       if the telemetry   while the medical         architectural
                    pipeline continues pipeline remains isolated design and
                    running.           on Core 1, preventing     separation of
                                       monitoring failures from  concerns
                                       affecting patient-data    
                                       processing.               
  ---------------------------------------------------------------------------------
