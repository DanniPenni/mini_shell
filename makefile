ARGS = -g -Wall
LIBS = 

%: %.o 
	gcc $(ARGS) $(LIBS) -o $@ 

%.o: %.c
	gcc $(ARGS) -c $^

%.c: %.h # Updates C-files based on their header-files.

%: %.c
	gcc $(ARGS) -o $@ $^

clean:
	rm -rf *.o shell 
