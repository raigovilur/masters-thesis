outbasedir=out
outdir=$outbasedir/$(eval date +"%Y-%m-%d-%H:%M:%S")
mkdir $outdir
outdir=`realpath $outdir`

if [ -e "out/current" ] ; then
  rm $outbasedir/current
fi

echo "ln -s $outdir out/current"
ln -s $outdir $outbasedir/current

touch $outdir/powerstat.log
cat /dev/null > $outdir/powerstat.log

while true
do
  echo "powerstat -d 10 1 >> $outdir/powerstat.log"
  powerstat -d 10 1 >> $outdir/powerstat.log
done




