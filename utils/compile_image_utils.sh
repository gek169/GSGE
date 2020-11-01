#/bin/bash
MAINFILE=$(ls ./ |grep '.*\.c')
for filename in $MAINFILE
do
 name=`echo "$filename" | sed -e "s/.c$//g;s/.cpp$//g"`
 echo "Processing file: $filename"
 echo "output will be $name"
 rm $name
 gcc -lm $filename -o $name
done 
