module DetectorRB3App {

  module Default {
    constant QUEUE_DEPTH = 10
    constant STACK_SIZE = 64 * 1024
  }

  instance rateGroupDriverComp: Svc.RateGroupDriver base id 0x23000000

  instance rateGroup1Comp: Svc.ActiveRateGroup base id 0x23010000 \
    queue size Default.QUEUE_DEPTH \
    stack size Default.STACK_SIZE \
    priority 120

  instance posixTime: Svc.PosixTime base id 0x23020000

  instance linuxTimer: Svc.LinuxTimer base id 0x23030000

  instance comDriver: Drv.TcpServer base id 0x23040000

  instance detector: DetectorRB3.Detector base id 0x23050000 \
    queue size Default.QUEUE_DEPTH \
    stack size Default.STACK_SIZE \
    priority 110
}
