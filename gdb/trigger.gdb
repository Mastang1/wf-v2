# Source ONLY after APP main has run clock/timer/UART setup and is receiving UART.
# Calling target functions changes state. Never call while bootloader is swapping.
call (void)wolfBoot_update_trigger()
monitor reset halt
echo Trigger written. Switch back to build/wolfboot.elf symbols before continuing.\n
