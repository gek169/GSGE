#/bin/bash
# this version doesn't compile an SO file
#~ rm *.o
#~ rm *.bin
#~ rm *.so
#~ g++ -O3 -ffast-math -msse4 -c *.c *.cpp  -lGL -lglfw -lpthread -lopenal -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I/usr/include/bullet/ -m64 -Wno-undef
#~ rm main*.o
#~ g++ -O3 -ffast-math -msse4 -o main.bin *.o main.cpp  -ldl -lGL -lglfw -lopenal -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I/usr/include/bullet/ -m64 -Wno-undef
#~ g++ -O3 -ffast-math -msse4 -o main2.bin *.o main2.cpp  -ldl -lGL -lglfw -lopenal -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I/usr/include/bullet/ -m64 -Wno-undef
#~ g++ -O3 -ffast-math -msse4 -o main3.bin *.o main3.cpp  -ldl -lGL -lglfw -lpthread -lopenal -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I/usr/include/bullet/ -m64 -Wno-undef
#~ g++ -O3 -ffast-math -msse4 -o main4.bin *.o main4.cpp  -ldl -lGL -lglfw -lopenal -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I/usr/include/bullet/ -m64 -Wno-undef
#~ g++ -O3 -ffast-math -msse4 -o main5.bin *.o main5.cpp  -ldl -lGL -lglfw -lopenal -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I/usr/include/bullet/ -m64 -Wno-undef
#~ g++ -O3 -ffast-math -msse4 -o main6.bin *.o main6.cpp  -ldl -lGL -lglfw -lopenal -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I/usr/include/bullet/ -m64 -Wno-undef
rm *.bin
MAINFILE=$(ls ./ |grep 'main.*.cpp')
for filename in $MAINFILE
do
 name=`echo "$filename" | sed -e "s/.cpp$//g"`
 echo "Processing file: $filename"
 #~ echo "Name is $name"
 g++ -O3 -ffast-math -msse4 -Wl,--no-as-needed -ldl -lGL -lglfw -lpthread -lopenal \
 -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I./include/ -I/usr/include/bullet/\
  -m64 -Wno-undef -o $name.bin $filename ./GSGE.a
 #g++ -O1 -g -ffast-math -msse4 -lGL -ldl -lglfw -lpthread -lopenal -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I/usr/include/bullet/ -m64 -Wno-undef -o $name.bin $filename ./GSGE.a 
done 


UTILFILE=$(ls ./ |grep 'main.*util\.bin')
for filename in $UTILFILE
do
 name=`echo "$filename" | sed -e "s/main//g;s/util.bin$//g"`
 echo "Moving util: $filename"
 echo "To ../utils/$name"
 mv $filename ../utils/$name
done 
