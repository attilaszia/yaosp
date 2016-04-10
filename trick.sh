for f in `cat listtoreplace`
do
  sed -i -e 's/i686-pc-yaosp/i386-linux/g' $f
  sed -i -e 's/4\.3\.3/4\.6\.4/g' $f
done
