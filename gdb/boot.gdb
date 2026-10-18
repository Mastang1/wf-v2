set pagination off
set confirm off
file build/wolfboot.elf
target extended-remote localhost:3333
monitor reset halt
echo CPU is reset-halted. Inspect x/i $pc; step reset code, or thbreak main then continue.\n
# No load: signed factory image must already be flashed.
