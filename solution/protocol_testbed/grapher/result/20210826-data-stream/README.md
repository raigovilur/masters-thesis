# Overview
I sent each record represented by each line in a file like the one shown below. This file is extracted from rosbag file (snow_problem.bag). 
<img src=https://gpsd-playground.s3.eu-north-1.amazonaws.com/vehicle_velocity_photo.png>

The records are sent from Iseauto to a server with the interval of 10 milliseconds. 
In this experiment, I assumed that the records are generated from ros node continuously with the interval of 10 milliseconds.
The number of record that was sent in this experiment is 10923.

Connection is established only before sending the first record to server and is not disconnected  until the last record (10923th record) is sent to server. 

The file used for this experiment is available at https://gpsd-playground.s3.eu-north-1.amazonaws.com/vehicle_velocity.csv.



## How Latency is measured
### Before experiment
synchronize system clock using ntp

### Latency
### At client
1. Read data from the file
2. Create record                
3. **Get system clock and add it to record as timestamp (t1)**
4. encrypt a record, send it to server

### At server
5. decrypt the received record
6. **Get system clock (t2)**
7. calculate t2 - t1, that is latency I measured


### How CPU usage is measured
by top command.
```
top -b -d1
```

# Comparison by ciphersuites
## TLSv1.3
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/latency_TLS.png>
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/top_TLS.png>

## DTLSv1.2
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/latency_DTLS.png>                       
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/top_DTLS.png>

## QUIC
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/latency_QUIC.png>                          
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/top_QUIC.png>

# Comparison by protocols
## TLS_AES_128_GCM_SHA256
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/latency_TLS_AES_128_GCM_SHA256.png>        
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/top_TLS_AES_128_GCM_SHA256.png>

## TLS_AES_256_GCM_SHA256
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/latency_TLS_AES_256_GCM_SHA256.png>        
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/top_TLS_AES_256_GCM_SHA256.png>

## TLS_CHACHA20_POLY1305_SHA256
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/latency_TLS_CHACHA20_POLY1305_SHA256.png>  
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/top_TLS_CHACHA20_POLY1305_SHA256.png>

## TLS_AES_128_CCM_SHA256
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/latency_TLS_AES_128_CCM_SHA256.png>        
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/top_TLS_AES_128_CCM_SHA256.png>

## TLS_AES_128_CCM_8_SHA256
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/latency_TLS_AES_128_CCM_8_SHA256.png>      
<img src=https://raw.githubusercontent.com/raigovilur/masters-thesis/streaming/solution/protocol_testbed/grapher/result/20210826-data-stream/top_TLS_AES_128_CCM_8_SHA256.png>
