outbasedir=out
outdir=$(eval date +"%Y-%m-%d-%H:%M:%S")
mkdir $outbasedir/$outdir

if [ -e "$outbasedir/current" ] ; then
  rm -f $outbasedir/current
fi

echo "ln -s $outdir $outbasedir/current"
ln -s $outdir $outbasedir/current

if [ `ps aux | grep powerstat | grep -v grep | wc -l` -gt 0 ] ; then
  killall powerstat
fi

# Pre excecution
start=`date +%s`
echo "powerstat -d 10 1 > $outbasedir/$outdir/powerstat.log"
powerstat -d 10 1 > $outbasedir/$outdir/powerstat.log &
echo "Pre excecution starts (120s) "

while true
do
  now=`date +%s`
  runtime=$((now-start))
  if [ $runtime -ge 120 ] ; then
    break
  fi
done
echo "Pre excecution done"

killall powerstat
cat /dev/null > $outbasedir/$outdir/powerstat.log

# Measure power consumption without protocol_testbed
start=`date +%s`
echo "powerstat -d 0 1 > $outbasedir/$outdir/powerstat.log"
powerstat -d 0 1 > $outbasedir/$outdir/powerstat.log &
echo "Measuring power consumption withought protocol_testbed (60s) "

while true
do
  now=`date +%s`
  runtime=$((now-start))
  if [ $runtime -ge 60 ] ; then
    break
  fi
done
echo "Done"
echo "You can run protocol testbed!"

killall powerstat

while true
do
  echo "powerstat -d 0 1 >> $outbasedir/$outdir/powerstat.log"
  powerstat -d 0 1 >> $outbasedir/$outdir/powerstat.log
done




