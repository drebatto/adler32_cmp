adler32_cmp : adler32_cmp.c
	@ echo Compiling $< 
	gcc -g -lz -o adler32_cmp adler32_cmp.c

