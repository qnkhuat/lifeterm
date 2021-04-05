export DEBUG=""
clear
F=lifeterm
make $F
#if [ $? -eq 0 ]; then ./$F.o repos/golly-4.0-src/Patterns/HashLife/broken-lines.mc; fi
if [ $? -eq 0 ]; then ./$F.o; fi
