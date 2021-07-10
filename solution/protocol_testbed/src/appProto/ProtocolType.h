#ifndef PROTOCOL_TESTBED_PROTOCOLTYPE_H
#define PROTOCOL_TESTBED_PROTOCOLTYPE_H
namespace Protocol {
    enum ProtocolType {
        Dummy,
        OpenSSL_TLS,
        OpenSSL_DTLS1_2,
        MVFST_QUIC
    };

    class Options {
    public:
        unsigned int UDPtarget = 30; //Megabits per second
    };
}

#endif //PROTOCOL_TESTBED_PROTOCOLTYPE_H
