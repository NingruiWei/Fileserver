CC=g++ -g -Wall -pthread -std=c++17

# List of source files for your file server
FS_SOURCES= listen.cpp fs_server.cpp

# Generate the names of the file server's object files
FS_OBJS=${FS_SOURCES:.cpp=.o}

all: fs app

# Compile the file server and tag this compilation
fs: ${FS_OBJS} libfs_server.o
	./autotag.sh
	${CC} -o $@ $^ -pthread -ldl

# Compile a client program
<<<<<<< HEAD
app: test_spec.cpp libfs_client.o
=======
app: test_depression.cpp libfs_client.o
>>>>>>> 2f1ca3ca964fd92df24701fbb53b0737459a067d
	${CC} -o $@ $^

# Generic rules for compiling a source file to an object file
%.o: %.cpp
	${CC} -c $<
%.o: %.cc
	${CC} -c $<

clean:
	rm -f ${FS_OBJS} fs app
