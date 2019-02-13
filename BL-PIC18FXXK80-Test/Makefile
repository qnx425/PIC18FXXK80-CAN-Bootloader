MAIN   = app
SRC    = $(MAIN).c
OUTDIR = out
CC     = C:\Program Files (x86)\Microchip\xc8\v1.42\bin\xc8.exe
CHIP   = 18F25K80

$(MAIN).hex: $(SRC)
	$(CC) $(SRC) --chip=$(CHIP) --MODE=pro --ROM=default,-0-7ff,-806-807 --CODEOFFSET=800 --OUTDIR=$(OUTDIR) -O$(MAIN)
				 
clean:
	cd $(OUTDIR); mv $(MAIN).hex testapp.hex; rm startup.*; rm $(MAIN).*
