#/bin/bash
# compiles into EXEs no DLLs
rm *.o
rm *.exe
rm *.a

g++ -O3 -ffast-math -msse4 -c lib/*.c lib/*.cpp  -lopengl32 -lpthread -lglfw3 -lopenal32 -m64 -Wno-undef -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I"/mingw64/include/bullet" -I"./include"



MAINFILE=$(ls ./ |grep 'main.*.cpp')
for filename in $MAINFILE
do
 name=`echo "$filename" | sed -e "s/.cpp$//g"`
 echo "\nProcessing file: $filename"
 echo "\nName is $name"
 #g++ -O3 -ffast-math -msse4 -lGL -ldl -lglfw -lpthread -lopenal -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I/usr/include/bullet/ -m64 -Wno-undef -o $name.bin $filename ./GSGE.a
 g++ -O3 -ffast-math -msse4 -o $name.exe *.o $filename  -lopengl32 -lpthread -lglfw3 -lopenal32 -m64 -Wno-undef -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I"/mingw64/include/bullet" -I"./include"
done 
