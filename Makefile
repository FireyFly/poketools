
.PHONY: all
all: readscript

.PHONY: clean
clean:
	rm obj/formats/*.o
	rmdir obj/formats
	rm obj/*.o
	rmdir obj
	rm readscript


obj:
	mkdir obj

obj/formats:
	mkdir obj/formats

obj/%.o: src/%.c obj
	$(CC) -c -g $< -o $@

obj/formats/%.o: src/formats/%.c obj/formats
	$(CC) -c -g $< -o $@

readscript: obj/readscript.o obj/script_pp.o obj/formats/script.o
	$(CC) $^ -o $@
