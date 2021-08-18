outbasedir=out
outdir=$(eval date +"%Y-%m-%d-%H:%M:%S")
mkdir $outbasedir/$outdir

if [ -e "out/current" ] ; then
  rm $outbasedir/current
fi

echo "ln -s $outdir out/current"
ln -s $outdir out/current

touch $outbasedir/$outdir/powerstat.log
cat /dev/null > $outbasedir/$outdir/powerstat.log

while true
do
  echo "powerstat -d 10 1 >> $outbasedir/$outdir/powerstat.log"
  powerstat -d 10 1 >> $outbasedir/$outdir/powerstat.log
done




