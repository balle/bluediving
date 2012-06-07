OBJ=src/greenplaque.o
EXE=src/greenplaque

DEPS=src/list.h

LIBS=-lbluetooth -lpthread -D_REENTRANT

all: $(EXE)

$(EXE): $(OBJ) $(DEPS)
	cc -o $(EXE) $(OBJ) $(LIBS)
	rm $(OBJ)
clean:
	rm -f $(EXE) $(OBJ) *~
