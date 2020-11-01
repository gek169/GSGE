#/bin/bash
rm *.o
#mv main*.cpp ./temp/
g++ -O3 -ffast-math -msse4 -c lib/*.c lib/*.cpp -Wl,--no-as-needed -ldl -lGL -lglfw -lpthread -lopenal -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I./include/ -I/usr/include/bullet/ -m64 -Wno-undef -fPIC

#g++ -O1 -g -ffast-math -msse4 -c *.c *.cpp -lGL -lglfw -lpthread -lopenal -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath -I/usr/include/bullet/ -m64 -Wno-undef -fPIC
#mv ./temp/main*.cpp ./
#rm main2.o
#rm main3.o
#g++ -O3 -ffast-math -msse4 -shared -o GSGE.so *.o -m64 -Wno-undef -lGL -lpthread -lglfw -lBulletSoftBody -lBulletDynamics -lBulletCollision -lLinearMath
rm GSGE.a
ar rvs GSGE.a *.o
echo "Compiled Static Library File!"
echo "Now call ./Compile_Mains.sh to compile the demos!"

