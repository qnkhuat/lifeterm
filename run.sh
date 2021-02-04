clear
F=lifeterm
make $F
if [ $? -eq 0 ]; then ./$F.o; fi
