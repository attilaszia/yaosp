grep -r "Werror" > ../faszom
mv ../faszom ./faszom
for file in $(cat faszom | grep -o '.*xml')
do
    sed '/^.*Werror.*$/d' $file > $file.jo
    mv $file.jo $file
done
