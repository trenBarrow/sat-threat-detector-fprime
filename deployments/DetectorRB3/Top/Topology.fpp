module DetectorRB3App {

  enum Ports_RateGroups {
    rg1
  }

  topology App {
    # Subtopology imports
    import CdhCore.Subtopology
    import ComFprime.Subtopology

    # Instances used in the topology
    instance posixTime
    instance linuxTimer
    instance rateGroupDriverComp
    instance rateGroup1Comp
    instance comDriver
    instance detector

    # Pattern graph specifiers
    command connections instance CdhCore.cmdDisp
    event connections instance CdhCore.events
    telemetry connections instance CdhCore.tlmSend
    text event connections instance CdhCore.textLogger
    time connections instance posixTime

    # Direct graph specifiers
    connections RateGroups {
      linuxTimer.CycleOut -> rateGroupDriverComp.CycleIn
      rateGroupDriverComp.CycleOut[Ports_RateGroups.rg1] -> rateGroup1Comp.CycleIn
      rateGroup1Comp.RateGroupMemberOut[0] -> CdhCore.tlmSend.Run
      rateGroup1Comp.RateGroupMemberOut[1] -> ComFprime.comQueue.run
    }

    connections Comms {
      # ComDriver buffer allocations
      comDriver.allocate   -> ComFprime.commsBufferManager.bufferGetCallee
      comDriver.deallocate -> ComFprime.commsBufferManager.bufferSendIn

      # ComDriver <-> ComStub (Uplink)
      comDriver.$recv                      -> ComFprime.comStub.drvReceiveIn
      ComFprime.comStub.drvReceiveReturnOut -> comDriver.recvReturnIn

      # ComStub <-> ComDriver (Downlink)
      ComFprime.comStub.drvSendOut -> comDriver.$send
      comDriver.ready              -> ComFprime.comStub.drvConnected
    }

    connections CdhCore_ComFprime {
      # Events and telemetry to comQueue
      CdhCore.events.PktSend    -> ComFprime.comQueue.comPacketQueueIn[ComFprime.Ports_ComPacketQueue.EVENTS]
      CdhCore.tlmSend.PktSend   -> ComFprime.comQueue.comPacketQueueIn[ComFprime.Ports_ComPacketQueue.TELEMETRY]
      # Command routing between the router and dispatcher
      ComFprime.fprimeRouter.commandOut -> CdhCore.cmdDisp.seqCmdBuff
      CdhCore.cmdDisp.seqCmdStatus      -> ComFprime.fprimeRouter.cmdResponseIn
    }
  }
}
