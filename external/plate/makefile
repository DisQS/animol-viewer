# destination name
NAME=plate

# shaders

SHELL := /bin/bash
.ONESHELL:

# shaders

SHADER_VERT_SRCS := $(wildcard gpu/webgl/shaders/*/*.vert)
SHADER_VERT_DESTS := $(patsubst %.vert,%.vert.h, $(SHADER_VERT_SRCS))

SHADER_FRAG_SRCS := $(wildcard gpu/webgl/shaders/*/*.frag)
SHADER_FRAG_DESTS := $(patsubst %.frag,%.frag.h, $(SHADER_FRAG_SRCS))


%.vert.h : %.vert
	@echo "$< -> $@"
	pushd gpu/webgl/shaders > /dev/null
	xxd -i $$(basename $$(dirname "$<"))/$$(basename "$<") > $$(basename $$(dirname "$@"))/$$(basename "$@")
	popd > /dev/null

%.frag.h : %.frag
	@echo "$< -> $@"
	pushd gpu/webgl/shaders > /dev/null
	xxd -i $$(basename $$(dirname "$<"))/$$(basename "$<") > $$(basename $$(dirname "$@"))/$$(basename "$@")
	popd > /dev/null


all: $(SHADER_VERT_DESTS) $(SHADER_FRAG_DESTS)
	@echo "built shaders"

clean:
	rm -f gpu/webgl/shaders/*/*.vert.h gpu/webgl/shaders/*/*.frag.h
