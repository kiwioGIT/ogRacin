cd target
gcc ../main.c -IC:..\SDL\include\SDL2 -LC:..\SDL\lib -w -lmingw32 -lSDL2main -lSDL2 -o ogRacer
./ogRacer.exe
cd ..