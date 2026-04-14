gcc src/main.c -o game ^
 -I%RAYLIB_PATH%\include ^
 -L%RAYLIB_PATH%\lib ^
 -lraylib -lopengl32 -lgdi32 -lwinmm
