# Protocol testbed
The protocol testbed is a client and a server application that can send a file via different protocols. These applications were deveolped as part of a Masters' thesis to evaluate QUIC, TLS 1.3 and DTLS 1.2 for use on autonomous vehicle telematics. 

# Supported network protocols
Currently the testbed supports  
QUIC (--protocol QUIC)  
TLS 1.3 (--protocol TLS)  
DTLS 1.2 (--protocol DTLS)  
TCP (--protocol TCP)  
UDP (--protocol UDP)  

## Use
Run either the protocol_testbed_client or protocol_testbed_server with the argument --help to see all arguments.

Run the server, then the client and the client will send the file specified by the --file argument to the server via the protocol specified by --protocol.

## Run on Docker
### Build
**Note:** Docker image is available at DockerHub repository, but currently it is configured as private. So, Build the image on your local environment.
```
docker build -t camsenec/protocol_testbed:`cat .env | grep TAG | cut -d "=" -f2` .
```

### Server
```
docker-compose down
docker-compose run --service-ports --rm server
```

In Docker Container
```
./run.sh server TLS|DTLS|QUIC
```

### Client
```
docker-compose down
docker-compose run --rm client
```

In Docker Container
```
./run.sh client TLS|DTLS|QUIC
```

### Configuration
Set the server's host ip address to `PROTOCOL_TESTBED_ADDRESS` in `docker-compose.yml`.

### Evaluation
For evaluation, you can excecute `run.sh` with options.
```
$ ./run.sh -h
Options:
  -p                   Enable power consumption log
  -c                   Enable cpu usage log
  -o                   Enable log
```

#### Client
```
./run.sh -p -c -o client TLS|DTLS|QUIC
```
**Note**: To run powerstat is prompted by the program when you enable p option.

#### Server
```
./run.sh -o server TLS|DTLS|QUIC
```
Test results are stored in `out` directory for client, while they are stored in `out/server` directory for server. 


## Dependencies
CMake
OpenSSL 1.1.x  
Facebook implementation of IETF QUIC (MVFST) https://github.com/facebookincubator/mvfst  
Add MVFST cmake directory to CMAKE_MODULE_PATH  
BOOST

## Compiling
Clone and build MVFST. Project has been compiled and tested on X64 and ARM64 architectures.
If dependencies have been resolved, run the cmake generator. Add the required dependencies to CMAKE_MODULE_PATH and CMAKE_PREFIX_PATH (MVFST and dependencies, <MVFST_REPO_DIR>/cmake, MVFST, FMT, FIZZ, FOLLY cmake directories from the MVFST repo build directory.

## Testing
### Client testing
Client implementations can be tested with open source tools
#### TLS
`ncat -lvnp 12345 --ssl`
#### DTLS
Generate a certificate  
`openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365`  
And then run  
`openssl s_server -dtls -accept 12345 -key key.pem -cert cert.pem` 


# Transfer statistics
Transfer statistics are printed to std out. Use unix command line to direct the output to a file if needed.

# File sending protocol
The application level protocol (sitting on top of all the network protocols) is intentionally kept simple.
In essence the system is made for sending one large file per connection.
Since connection start and end are only a tiny part of the total file sending time, it doesn't have a big effect on the overall
transmission time.  
The total package is:  
| Protocol ID (2 bytes) | Protocol version (2 bytes) | checksum for the file (32 bytes) | file size (8 bytes) | file name (1024 bytes, enough for 256 UTF-8 characters and the ending 0 bytes) | file itself... |
|---|---|---|---|---|---|
|   |   |   |   |   |   |

Protocol ID is fixed at 0x00. It's for future additions.  
Version Field is fixed at 0x01.
Checksum for the file contains a hash calculated from the entire file (on disk). It's calculated with SHA-256.  
File size is the byte size expected. 64 Bytes is the current largest file size supported by common Linux file systems and is more than enough for the foreseeable future.  
File name is UTF-8 encoded and will fit 255 UTF-8 characters and trailing four 0 bytes. It can fit more with certain character combinations, but it's not guaranteed to fit. 255 was chosen, because it's the file name length limit on Linux platforms.

# Source code organisation
Client code main file is ClientMain.cpp and all client protocol implementation and interface source files are at src/protocol. The server program entry is ServerMain.cpp and server protocol interface and implementation files are located at src/ServerProtocol. The src/appProto folder has code that is used by both the server and the client (header files only). src/dtlsCookieVault folder has the cookieVault component that is used in the DTLS server cookie validation process.
