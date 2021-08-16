#!/bin/bash

print_usage() {
  echo "Usage: $0 [client|server] [TLS|DTLS|QUIC]"
}

dtls_run() {
  case $mode in
  "client")
    ./build_/protocol_testbed_client --protocol $protocol \
      --address $PROTOCOL_TESTBED_ADDRESS \
      --port $PROTOCOL_TESTBED_PORT \
      --stport $PROTOCOL_TESTBED_STPORT \
      --buffer-size $PROTOCOL_TESTBED_BUFSIZE \
      --udpTarget $PROTOCOL_TESTBED_UDP_TARGET \
      --stportBW $PROTOCOL_TESTBED_STPORTBW \
      --file $filepath
    ;;
  "server")
    if [ ! -e "cert.pem" -o ! -e "key.pem" ] ; then
      openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365
    fi
    
    if [ $(eval ps aux | grep iperf3 | grep -v grep | wc -l) -lt 1 ] ; then
      iperf3 -s -p $PROTOCOL_TESTBED_STPORT &
    fi

    ./build_/protocol_testbed_server --protocol $protocol \
      --port $PROTOCOL_TESTBED_PORT \
      --cert ./cert.pem \
      --pkey ./key.pem \
      --path $filepath
    ;;
  *)
    print_usage
    ;;
  esac
}

run() {

  case $mode in
  "client")
    ./build_/protocol_testbed_client --protocol $protocol \
      --address $PROTOCOL_TESTBED_ADDRESS \
      --port $PROTOCOL_TESTBED_PORT \
      --stport $PROTOCOL_TESTBED_STPORT \
      --buffer-size $PROTOCOL_TESTBED_BUFSIZE \
      --file $filepath
      ;;
  "server")
    if [ ! -e "cert.pem" -o ! -e "key.pem" ] ; then
      openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365
    fi

    if [ $(eval ps aux | grep iperf3 | grep -v grep | wc -l) -lt 1 ] ; then
      iperf3 -s -p $PROTOCOL_TESTBED_STPORT &
    fi

    ./build_/protocol_testbed_server --protocol $protocol \
      --port $PROTOCOL_TESTBED_PORT \
      --cert ./cert.pem \
      --pkey ./key.pem \
      --path $filepath
      ;;
  *)
    print_usage
    ;;
  esac
}

if [ $# -lt 2 ]; then
  print_usage
  exit 1
fi

mode=$1
protocol=$2

if [ -z "$PROTOCOL_TESTBED_FILE" ] ; then
  filepath="./file/test.txt"
  truncate -s 8499M $filepath
fi

if [ $protocol == "DTLS" ] ; then
  dtls_run
else
  run
fi

exit 0
