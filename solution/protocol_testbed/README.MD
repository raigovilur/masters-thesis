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


# Protocol
The application level protocol (sitting on top of all the network protocols) is intentionally kept simple.
In essence the system is made for sending one large file per connection.
Since connection start and end are only a tiny part of the total file sending time, it doesn't have a big effect on the overall
transmission time.  
The total package is:  
| Protocol ID (2 bytes) | Protocol version (2 bytes) | checksum for the file (64 bytes) | file size (64 bytes) | file name (1024 bytes, enough for 256 UTF-8 characters and the ending 0 bytes) | file itself... |
|---|---|---|---|---|---|
|   |   |   |   |   |   |

Protocol ID is fixed at 0x00. It's for future additions.  
Version Field is fixed at 0x01.
Checksum for the file contains a hash calculated from the entire file (on disk). It's calculated with SHA-256.  
File size is the byte size expected. 64 Bytes is the current largest file size supported by common Linux file systems and is more than enough for the foreseeable future.  
File name is UTF-8 encoded and will fit 255 UTF-8 characters and trailing four 0 bytes. It can fit more with certain character combinations, but it's not guaranteed to fit. 255 was chosen, because it's the file name length limit on Linux platforms.
