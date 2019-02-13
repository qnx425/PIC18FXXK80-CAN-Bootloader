PSECT HiVector,class=CODE,delta=1,abs

org  0x08
goto 0x808

PSECT LoVector,class=CODE,delta=1,abs

org  0x18
goto 0x818

end
