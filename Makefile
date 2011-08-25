TARGETS=	Gen/ntris

PROJECT=	./

#CXXFLAGS=	-g -O2 `sdl-config --cflags`
CXXFLAGS= -g -O2 -I/Library/Frameworks/SDL.framework/Headers

#LDFLAGS=	`sdl-config --libs`
LDFLAGS=  -framework SDL -framework SDL_mixer -framework Cocoa

SOURCES=	Pentris.cpp

define copy-images
cp blockData.dat Gen/blockData.dat
cp multiplayerData.dat Gen/multiplayerData.dat
cp font.bmp Gen/font.bmp
cp numbers.bmp Gen/numbers.bmp
cp gameover.bmp Gen/gameover.bmp
cp paused.bmp Gen/paused.bmp
cp wonlost.bmp Gen/wonlost.bmp
endef

ifneq ($(INCLUDE_DEPENDENCIES),yes)

all:	
	@make --no-print-directory INCLUDE_DEPENDENCIES=yes $(TARGETS)
	$(copy-images)

mac:
	cp MacColor.h Color.h
	@make --no-print-directory INCLUDE_DEPENDENCIES=yes $(TARGETS)
	$(copy-images)
	
linux:
	cp LinuxColor.h Color.h
	@make --no-print-directory INCLUDE_DEPENDENCIES=yes $(TARGETS)
	$(copy-images)
	
.PHONY:	clean
clean:
	rm -rf Gen

else

-include $(addprefix Gen/,$(SOURCES:.cpp=.d))
-include $(addprefix Gen/,$(SOURCES:.cpp=.d))

endif

FLOAT_OBJECTS= $(addprefix Gen/,$(SOURCES:.cpp=.o)) Gen/SDL_prims.o

.INTERMEDIATE: $(FLOAT_OBJECTS)

Gen/%.o:		%.cpp
	mkdir -p $(dir $@)
	c++ $(CXXFLAGS) -c -o $@ $<

Gen/%.o:		%.c
	mkdir -p $(dir $@)
	cc $(CXXFLAGS) -c -o $@ $<

Gen/ntris:	$(FLOAT_OBJECTS)
	g++ -o $@ $^ SDLMain.m $(LDFLAGS)

Gen/%.d:		%.cpp
	@mkdir -p $(dir $@)
	c++ -M -MT $(@:.d=.o) $(CXXFLAGS) -o $@ $<

