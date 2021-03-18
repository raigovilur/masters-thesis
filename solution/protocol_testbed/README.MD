# Protocol implementations

## Use
See ClientMain.cpp. To change implementation, change the Protocol::ProtocolType enum to your desired protocol.
File sent is currently hard-coded in ClientMain.cpp

## Dependencies
OpenSSL 1.1.x  
Facebook implementation of IETF QUIC (MVFST) https://github.com/facebookincubator/mvfst  
Add MVFST CMake dir to CMAKE_MODULE_PATH  

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
