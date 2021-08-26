# Results
## TODO
- [x] TLS_local
- [ ] DTLS_local
- [ ] QUIC_local
- [ ] TLS_vehicle
- [ ] DTLS_vehicle
- [ ] QUIC_vehicle

## Evaluation method
CPU usage and power consumption are obtained at client-side, while throughput is measured at server-side.
### Cpu Usage
```bash
top -b -d1 > logfile
```

### Power consumption
#### Solution 1
```bash
powerstat 1 > logfile
```
- Pros: 
  - data is automatically stored to log file
- Cons
  - `powerstat` can only get power consumption by whole system. It cannot get power consumption for each process or device.

#### Solution 2
Run powertop and record the results by video.
After that, calculate the sum of discharging rate caused by processes and devices related to the protocol_testbed program.

- Pros
  - `powertop` can get power consumption for each process or device.
- Cons
  - It requires us to input data to computer by hand watching the recorded video. 
  - It is difficult to determine which processes and devices are used by the program. 

<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/cscs/solution/protocol_testbed/grapher/result/example/powertop.png>


### Throughput
Embedded in `protocol_testbed_server`

## Example
TLS local: File transfer on TLS when both client and server does not move
### CPU Usage
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/cscs/solution/protocol_testbed/grapher/result/example/cpu_usage.png>

### Power consumption (Solution 1)
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/cscs/solution/protocol_testbed/grapher/result/example/power_consumption.png>

### Throughput
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/cscs/solution/protocol_testbed/grapher/result/example/throughput.png>
