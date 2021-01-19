clear
F=hashlife
make $F
if [ $? -eq 0 ]; then ./$F.o; fi
