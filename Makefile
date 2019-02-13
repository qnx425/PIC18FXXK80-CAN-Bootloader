MAIN           = BL-PIC18FXXK80
SRC            = config.c idloc.c main.c BootPIC18FXXK80.c can.c putch.c VectorRemap.as
CC             = C:\Program Files (x86)\Microchip\xc8\v1.42\bin\xc8.exe
CHIP           = 18F25K80

all: $(MAIN).hex

$(MAIN).hex: $(SRC)
	$(CC) $(SRC) --chip=$(CHIP) --MODE=pro --OPT=+speed --OUTDIR=out -O$(MAIN) --ROM=default,-800-7fff -D__DEBUG=1
				 
clean:
	rm -f $(MAIN).hex funclist $(MAIN).cof $(MAIN).hxl $(MAIN).p1 $(MAIN).sdb startup.* $(MAIN).lst $(MAIN).pre $(MAIN).sym

