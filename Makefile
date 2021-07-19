TARGET=tempsensor

# Standard arm development tools
CC=arm-none-eabi-gcc
LD=arm-none-eabi-gcc
AR=arm-none-eabi-ar
AS=arm-none-eabi-as
CP=arm-none-eabi-objcopy
OD=arm-none-eabi-objdump
SE=arm-none-eabi-size
SF=st-flash

# This needs to point to the roof of your libopencm3 installation
LIBOPENPCMROOT = /home/euler357/libopencm3

CFLAGS  = -std=gnu99 -g -O2 -Wall
CFLAGS += -mlittle-endian -mthumb -mthumb-interwork -mcpu=cortex-m3
CFLAGS += -fsingle-precision-constant -Wdouble-promotion
CFLAGS += -I$(LIBOPENPCMROOT)/include -c -DSTM32F1 
 
LDFLAGS = --static -nostartfiles -mthumb -mcpu=cortex-m3 -msoft-float -mfix-cortex-m3-ldrd -Wl,-Map=tempsensor.map 
LDFLAGS += -Wl,--gc-sections -specs=nosys.specs -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group 
LDFLAGS += -T./stm32f103c8t6.ld -L/home/euler357/libopencm3/lib 


default: $(TARGET).bin 

$(TARGET).bin: $(TARGET).elf
	@echo '***************************'
	@echo '*** Making Bin from Elf ***'
	@echo '***************************'
	$(CP) -O binary $^ $@
	@echo

$(TARGET).o: $(TARGET).c
	@echo '*****************'
	@echo '*** Compiling ***'
	@echo '*****************'
	$(CC) $(INCLUDE) $(CFLAGS) $^ -o $@

$(TARGET).elf: $(TARGET).o 
	
	@echo '***************'
	@echo '*** Linking ***'
	@echo '***************'
	$(LD) $(LDFLAGS) $^ -lopencm3_stm32f1 -o $@

fresh: clean default

clean:
	@echo '****************'
	@echo '*** Cleaning ***'
	@echo '****************'
	rm -f *.o *.elf *.bin *.map

flash: default
	@echo '************************'
	@echo '*** Burning to Flash ***'
	@echo '************************'
	$(SF) write $(TARGET).bin 0x8000000
