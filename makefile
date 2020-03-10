CC       = tcc
MODEL    = c
CFLAGS   = -k- -a -b- -wamp -wnod -wpro -wrvi -wuse -wsig -d -f -G -O -N
LIB      = a:\turbo\tc\lib
PROGRAM  = soroban.exe
OBJS     = soroban.obj control.obj mid.obj newsbody.obj smail.obj \
		tempfile.obj errors.obj stdfunc.obj hash.obj netdef.obj \
		wexlib.obj ryujilib.obj derrors.obj sys.obj active.obj \
		dateconv.obj datetok.obj getindat.obj qmktime.obj
SRCPAC   = sorosrc.lzh
PROGPAC  = soro302.lzh
SRCPACKS = README.src cnewssrc.doc *.c *.h  \
		netdef.doc man.ntf Makefile
PROGPACS = soroban.man soroban.ntf cvhist.man cvhist.ntf \
		README.doc $(PROGRAM) cvhist.exe CHANGES.doc \
		q&a.doc CHANGES $(SRCPAC)

all : $(PROGRAM)

$(PROGRAM) : $(OBJS) converter
	$(CC) -e$(PROGRAM) -lx -m$(MODEL) @${$(CFLAGS) $(OBJS)}

converter : cvhist.exe
cvhist.exe : cvhist.c wexlib.h ryujilib.c ryujilib.h wexlib.c netdef.c netdef.h
	tcc -lx -mc $(CFLAGS) cvhist.c wexlib.obj ryujilib.obj netdef.obj

netdef.obj : netdef.c netdef.h
	$(CC) -c -m$(MODEL) $(CFLAGS) -d- netdef.c

.c.obj:
	$(CC) -c -m$(MODEL) $(CFLAGS) $<

pack : all man
	-rm $(SRCPAC)
	-rm $(PROGPAC)
	lha a -i2 -n1 -l1 $(SRCPAC) $(SRCPACKS)
	lha a -i2 -n1 -l1 $(PROGPAC) @${$(PROGPACS)}

man : soroban.man cvhist.man

soroban.man : soroban.ntf
	ntf < soroban.ntf > soroban.man

cvhist.man : cvhist.ntf
	ntf < cvhist.ntf > cvhist.man

# DO NOT DELETE THIS LINE -- mkdep uses it.
# DO NOT PUT ANYTHING AFTER THIS LINE, IT WILL GO AWAY.

control.obj: soroban.h tempfile.h stdfunc.h errors.h
cvhist.obj: wexlib.h netdef.h
datetok.obj: dateconv.h datetok.h
derrors.obj: errors.h
errors.obj: errors.h soroban.h
getindat.obj: dateconv.h datetok.h
hash.obj: soroban.h hash.h
mid.obj: soroban.h wexlib.h hash.h errors.h tempfile.h
netdef.obj: netdef.h
newsbody.obj: soroban.h tempfile.h stdfunc.h errors.h
ryujilib.obj: ryujilib.h
smail.obj: soroban.h tempfile.h errors.h stdfunc.h
soroban.obj: soroban.h tempfile.h errors.h stdfunc.h netdef.h wexlib.h
stdfunc.obj: stdfunc.h errors.h soroban.h
tempfile.obj: errors.h tempfile.h soroban.h
wexlib.obj: wexlib.h ryujilib.h

# IF YOU PUT ANYTHING HERE IT WILL GO AWAY
