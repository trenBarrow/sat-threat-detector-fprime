module DetectorRB3 {

  active component Detector {

    # Ports
    async input port FeatureIn: Fw.BufferSend

    # Special ports for events/time/telemetry
    event port Log
    text event port LogText
    time get port Time
    telemetry port Tlm

    # Telemetry
    telemetry RiskScore: F32 id 0x7000

    # Events
    event RiskAlert(Risk: F32, Reason: string) \
      severity warning high id 0x7100 \
      format "Risk {} {}"
  }
}
