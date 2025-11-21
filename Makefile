EXE=htproxy
SOURCES = main.c socket_component.c request_handler.c response_handler.c cache_handler.c parser.c process_keep_runing.c


$(EXE): $(SOURCES)
	cc -Wall -o $(EXE) $(SOURCES)

format:
	clang-format -style=file -i *.c

clean:
	rm -f $(EXE) *.o
