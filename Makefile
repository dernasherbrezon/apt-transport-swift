#Compiler and Linker
CC          := g++

#The Target Binary Program
TARGET      := swift

#The Directories, Source, Includes, Objects, Binary and Resources
SRCDIR      := src
INCDIR      := inc
BUILDDIR    := build
TARGETDIR   := build
SRCEXT      := cc
OBJEXT      := o

#Flags, Libraries and Includes
CFLAGS      := -Wall -O3 -g -std=c++11
LIB         := -lcurl #-lapt-pkg -lapt-inst
INC         := -I$(INCDIR) -I"/Users/dernasherbrezon/apt-includes"
INCDEP      := -I$(INCDIR)

#---------------------------------------------------------------------------------
#DO NOT EDIT BELOW THIS LINE
#---------------------------------------------------------------------------------
SOURCES     := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS     := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/%,$(SOURCES:.$(SRCEXT)=.$(OBJEXT)))

#Defauilt Make
all: $(TARGET)

#Remake
remake: cleaner all

#Make the Directories
directories:
	mkdir -p $(TARGETDIR)

#Clean only Objecst
clean:
	rm -rf $(BUILDDIR)

#Full Clean, Objects and Binaries
cleaner: clean
	rm -rf $(TARGETDIR)

#Link
$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGETDIR)/$(TARGET) $^ $(LIB)

#Compile
$(BUILDDIR)/%.$(OBJEXT): $(SRCDIR)/%.$(SRCEXT)
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

install: swift
	mkdir -p $(DESTDIR)/usr/lib/apt/methods/
	cp $(TARGETDIR)/$(TARGET) $(DESTDIR)/usr/lib/apt/methods/
