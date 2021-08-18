#!/bin/bash

current=current

print_usage() {
  echo "Usage: $0 [OPTIONS] client|server TLS|DTLS|QUIC"
}

print_usage_help() {
  echo "Options:"
  echo "  -o                   Enable log" 
  echo "  -f  directory        Store logfile to the directory (default: directory named with the current datetime)"
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
    exit 1
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
    exit 1
    ;;
  esac
}

dtls_run_with_log() {
  case $mode in
  "client")
    ./build_/protocol_testbed_client --protocol $protocol \
      --address $PROTOCOL_TESTBED_ADDRESS \
      --port $PROTOCOL_TESTBED_PORT \
      --stport $PROTOCOL_TESTBED_STPORT \
      --buffer-size $PROTOCOL_TESTBED_BUFSIZE \
      --udpTarget $PROTOCOL_TESTBED_UDP_TARGET \
      --stportBW $PROTOCOL_TESTBED_STPORTBW \
      --file $filepath >$logfile
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
      --path $filepath >$logfile
    ;;
  *)
    print_usage
    exit 1
    ;;
  esac
}

run_with_log() {

  case $mode in
  "client")
    ./build_/protocol_testbed_client --protocol $protocol \
      --address $PROTOCOL_TESTBED_ADDRESS \
      --port $PROTOCOL_TESTBED_PORT \
      --stport $PROTOCOL_TESTBED_STPORT \
      --buffer-size $PROTOCOL_TESTBED_BUFSIZE \
      --file $filepath >$logfile
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
      --path $filepath >$logfile
      ;;
  *)
    print_usage
    exit 1
    ;;
  esac
}

log_enabled=0
powerstat_enabled=0
top_enabled=0
while getopts pcof:h OPT; do
  case $OPT in
    c)
      top_enabled=1
      ;;
    p)
      powerstat_enabled=1
      ;;
    o)
      log_enabled=1
      ;;
    f)
      outdir=out/$OPTARG
      ;;
    h)
     print_usage_help
      exit 1
      ;;
    \?)
      print_usage
      exit 1
      ;;
  esac
done

shift `expr $OPTIND - 1`


if [ $# -lt 2 ] ; then
  print_usage
  exit 1
fi

mode=$1
protocol=$2


if [ -z "$PROTOCOL_TESTBED_FILE" ] ; then
  filepath="./file/test.txt"
  truncate -s 8499M $filepath
fi

# TEMPORARY
filepath="./file/file_small.txt"

if [ $log_enabled == 1 -o $top_enabled == 1 ] ; then
  if [ $powerstat_enabled == 1 ] ; then
    echo -n "Run powerstat on host [After running powerstat, enter done]: "
    read is_powerstat_enabled
    if [ $is_powerstat_enabled != "done" ] ; then
      exit 1
    fi
    outdir=out/$current
    logfile=$outdir/$protocol
  else
    outdir=out/$(eval date +"%Y-%m-%d-%H:%M:%S")
    echo "Performance results are saved to" $outdir
    mkdir $outdir
    logfile=$outdir/$protocol
    touch logfile
  fi
fi

if [ $top_enabled == 1 ] ; then
  killall top
  top -b -d1 > $outdir/top.log &
fi

if [ $log_enabled == 1 ] ; then  
  if [ $protocol == "DTLS" ] ; then
    dtls_run_with_log
  else
    run_with_log
  fi
else
  if [ $protocol == "DTLS" ] ; then
    dtls_run
  else
    run
  fi
fi


exit 0
