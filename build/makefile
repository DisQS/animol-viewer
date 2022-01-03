# destination name
NAME=pdbmovie

all: ${NAME}.js decoder_worker.js

COMMON_SRCS = ../src/${NAME}.cpp

MOLCLIBPATH   = ../external/molscript/code/clib
MOLSCRIPTPATH = ../external/molscript/code

MOLAUTO_SRCS   = aa_lookup.c mol3d_init.c dynstring.c mol3d.c named_data.c colour.c element_lookup.c mol3d_secstruc.c mol3d_utils.c vector3.c str_utils.c io_utils.c key_value.c double_hash.c mol3d_io.c quaternion.c matrix3.c mol3d_chain.c hermite_curve.c indent.c extent3d.c args.c fast_atof.c

MOLSCRIPT_SRCS = molscript.tab.c global.c lex.c col.c select.c state.c graphics.c segment.c coord.c xform.c vertex.c molauto.c

MOLSCRIPTOUTPUT = output
MOLCLIBOUTPUT   = output/clib

MOLAUTO_OBJS   = $(patsubst %.c,${MOLCLIBOUTPUT}/%.o,$(MOLAUTO_SRCS))
MOLSCRIPT_OBJS = $(patsubst %.c,${MOLSCRIPTOUTPUT}/%.o,$(MOLSCRIPT_SRCS))

POST_JS = ../external/plate/system/webgl/post.js

# -g -DNDEBUG

CC=emcc
CFLAGS=-std=c++20 -O3 -flto -DNDEBUG -I ../src/ -I ../external/include/ -I../external/plate/ -DPLATE -DPLATE_WEBGL -I${MOLCLIBPATH}/ -I${MOLSCRIPTPATH}/

EFLAGS=-O3 -DNDEBUG -flto -DVERTEX_SUPPORT -DCUSTOM_ATOF=fast_atof

# single threaded, webgl1

OPTIONS=--bind -s MODULARIZE=1 -s EXPORT_NAME="'plate'" -s WASM=1 -s USE_WEBGL2=0 -s FILESYSTEM=1 -s FETCH=1 -s ENVIRONMENT='web,worker' -s DISABLE_EXCEPTION_CATCHING=1 -s ALLOW_MEMORY_GROWTH=1 --closure 1


# compile molauto

$(MOLCLIBOUTPUT)/aa_lookup.o: $(MOLCLIBPATH)/aa_lookup.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/mol3d_init.o: $(MOLCLIBPATH)/mol3d_init.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/dynstring.o: $(MOLCLIBPATH)/dynstring.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/mol3d.o: $(MOLCLIBPATH)/mol3d.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/named_data.o: $(MOLCLIBPATH)/named_data.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/colour.o: $(MOLCLIBPATH)/colour.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/element_lookup.o: $(MOLCLIBPATH)/element_lookup.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/mol3d_secstruc.o: $(MOLCLIBPATH)/mol3d_secstruc.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/mol3d_utils.o: $(MOLCLIBPATH)/mol3d_utils.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/vector3.o: $(MOLCLIBPATH)/vector3.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/str_utils.o: $(MOLCLIBPATH)/str_utils.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/io_utils.o: $(MOLCLIBPATH)/io_utils.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/key_value.o: $(MOLCLIBPATH)/key_value.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/double_hash.o: $(MOLCLIBPATH)/double_hash.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/mol3d_io.o: $(MOLCLIBPATH)/mol3d_io.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/quaternion.o: $(MOLCLIBPATH)/quaternion.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/matrix3.o: $(MOLCLIBPATH)/matrix3.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/mol3d_chain.o: $(MOLCLIBPATH)/mol3d_chain.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/hermite_curve.o: $(MOLCLIBPATH)/hermite_curve.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/indent.o: $(MOLCLIBPATH)/indent.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/extent3d.o: $(MOLCLIBPATH)/extent3d.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/args.o: $(MOLCLIBPATH)/args.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLCLIBOUTPUT)/fast_atof.o: $(MOLCLIBPATH)/fast_atof.c
	mkdir -p $(MOLCLIBOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLCLIBPATH)/ -o $@ -c $<

# molscript

$(MOLSCRIPTOUTPUT)/molscript.tab.o: $(MOLSCRIPTPATH)/molscript.tab.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLSCRIPTOUTPUT)/global.o: $(MOLSCRIPTPATH)/global.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLSCRIPTOUTPUT)/lex.o: $(MOLSCRIPTPATH)/lex.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLSCRIPTOUTPUT)/col.o: $(MOLSCRIPTPATH)/col.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLSCRIPTOUTPUT)/select.o: $(MOLSCRIPTPATH)/select.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLSCRIPTOUTPUT)/state.o: $(MOLSCRIPTPATH)/state.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLSCRIPTOUTPUT)/graphics.o: $(MOLSCRIPTPATH)/graphics.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLSCRIPTOUTPUT)/segment.o: $(MOLSCRIPTPATH)/segment.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLSCRIPTOUTPUT)/coord.o: $(MOLSCRIPTPATH)/coord.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLSCRIPTOUTPUT)/xform.o: $(MOLSCRIPTPATH)/xform.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLSCRIPTOUTPUT)/vertex.o: $(MOLSCRIPTPATH)/vertex.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<
$(MOLSCRIPTOUTPUT)/molauto.o: $(MOLSCRIPTPATH)/molauto.c
	mkdir -p $(MOLSCRIPTOUTPUT)
	$(CC) $(EFLAGS) -I $(MOLSCRIPTPATH)/ -I $(MOLCLIBPATH)/ -o $@ -c $<

.PHONY: default

$(NAME).js: $(COMMON_SRCS)
	$(CC) $(CFLAGS) $(COMMON_SRCS) $(OPTIONS) -o $(NAME).js
	cat ${POST_JS} >> ${NAME}.js

install:
	./install.sh $(server)

decoder_worker.js: ../src/worker/decoder_worker.cpp $(MOLAUTO_OBJS) $(MOLSCRIPT_OBJS)
	$(CC) $(CFLAGS) $(MOLAUTO_OBJS) $(MOLSCRIPT_OBJS) -s EXPORTED_FUNCTIONS="['_end_worker', '_script', '_script_with', '_decode_url_color_interleaved', '_decode_url_color_non_interleaved','_decode_url_no_color','_decode_contents_color_interleaved','_decode_contents_color_non_interleaved','_decode_contents_no_color']" -s BUILD_AS_WORKER=1 -s FILESYSTEM=1 -s FETCH=1 -s DISABLE_EXCEPTION_CATCHING=1 -s ALLOW_MEMORY_GROWTH=1 --bind --closure 1 -o decoder_worker.js ../src/worker/decoder_worker.cpp

clean:
	rm -f $(NAME).js $(NAME).wasm

clean-all:
	rm -f $(NAME).js $(NAME).wasm $(MOLSCRIPTOUTPUT)/*.o $(MOLCLIBOUTPUT)/*.o $(MOLSCRIPTOUTPUT)/other/*.o decoder_worker.*
