for f in `cat listtoreplace`
do
  sed -i -e 's/4\.6\.4/4\.6\.3/g' $f
done
