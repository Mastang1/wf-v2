# Source only after stopping in bootloader do_boot; avoid ambiguous main/isr_reset.
symbol-file build/image.elf
thbreak isr_reset
echo APP symbols active. Continue to APP isr_reset; inspect MSP/VTOR, then thbreak main.\n
