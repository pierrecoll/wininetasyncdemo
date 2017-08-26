!include <win32.mak>

ALL : $(OUTDIR) $(OUTDIR)\AsyncDemo.exe



$(OUTDIR) :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"
    
$(OUTDIR)\asyncdemo.res: asyncdemo.rc 
    $(rc) $(rcflags) $(rcvars) /fo $(OUTDIR)\asyncdemo.res asyncdemo.rc
    
$(OUTDIR)\asyncdemo.obj: asyncdemo.cpp
    $(cc) $(cflags) $(cvarsdll) $(cdebug) /EHsc /Fp"$(OUTDIR)\\" /Fo"$(OUTDIR)\\" /Fd"$(OUTDIR)\\" asyncdemo.cpp
    
$(OUTDIR)\Asyncdemo.exe: $(OUTDIR)\asyncdemo.res $(OUTDIR)\asyncdemo.obj
    $(link) $(guilflags)  $(ldebug) /MACHINE:$(CPU) -out:$(OUTDIR)\$(PROJ).exe $(OUTDIR)\asyncdemo.res $(OUTDIR)\asyncdemo.obj $(baselibs) wininet.lib  user32.lib

clean:
        $(CLEANUP)


