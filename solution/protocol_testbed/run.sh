#!/bin/bash

current=current

print_usage() {
  echo "Usage: $0 [OPTIONS] client|server TLS|DTLS|QUIC"
}

print_usage_help() {
  echo "Options:"
  echo "  -p                   Enable power consumption log" 
  echo "  -c                   Enable cpu usage log"
  echo "  -o                   Enable log" 
}

kill_top() {
  if [ `ps aux | grep top | grep -v grep | wc -l` -gt 0 ] ; then
      kill `ps aux | grep top | grep -v grep | awk '{print $2}'`
  fi
}

kill_iperf() {
  if [ `ps aux | grep iperf | grep -v grep | wc -l` -gt 0 ] ; then
      kill `ps aux | grep iperf | grep -v grep | awk '{print $2}'`
  fi
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
    
    kill_iperf
    iperf3 -s -p $PROTOCOL_TESTBED_STPORT &

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

    kill_iperf
    iperf3 -s -p $PROTOCOL_TESTBED_STPORT &

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
    
    kill_iperf
    iperf3 -s -p $PROTOCOL_TESTBED_STPORT >$iperf_logfile &

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

    kill_iperf
    iperf3 -s -p $PROTOCOL_TESTBED_STPORT >$iperf_logfile &

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
while getopts pcoh OPT; do
  case $OPT in
    p)
      powerstat_enabled=1
      ;;
    c)
      top_enabled=1
      ;;
    o)
      log_enabled=1
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

case $mode in
  "client")
      outbasedir="out"
      ;;
  "server")
      outbasedir="out/server"
      ;;
  *)
    print_usage
    exit 1
    ;;
  esac

if [ -z "$PROTOCOL_TESTBED_FILE" ] ; then
  filepath="./file/test.txt"
  truncate -s 8499M $filepath
  truncate -s 300M "./file/test_small.txt"
fi

# TEMPORARY
filepath="./file/test_small.txt"

if [ $log_enabled == 1 -o $top_enabled == 1 ] ; then
  if [ $powerstat_enabled == 1 ] ; then
    echo -n "Run powerstat on host [After running powerstat, enter done]: "
    read is_powerstat_enabled
    if [ $is_powerstat_enabled != "done" ] ; then
      exit 1
    fi
    outdir=`realpath $outbasedir/$current`
    logfile=$outdir/$protocol
  else
    outdir=$(eval date +"%Y-%m-%d-%H:%M:%S")
    mkdir $outbasedir/$outdir
    if [ -e "$outbasedir/$current" ] ; then
      rm $outbasedir/$current
    fi
    ln -s $outdir $outbasedir/$current
    outdir=`realpath $outbasedir/$outdir`
    logfile=$outdir/$protocol
    iperf_logfile=$outdir/iperf.log
  fi
fi

if [ $top_enabled == 1 ] ; then
  kill_top
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

kill_top
kill_iperf


exit 0
