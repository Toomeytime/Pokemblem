.thumb 
.align 4

.macro blh to, reg=r3
  ldr \reg, =\to
  mov lr, \reg
  .short 0xf800
.endm

.include "Definitions.s"

.type AoE_HoverEffect, %function 
AoE_HoverEffect:
push {r4, lr} 

bl AoE_ClearMoveMap

ldr r0, =AoE_SpecificEffectIndex 
ldrb r0, [r0]
ldr r1, =AoE_EntrySize 
ldrb r1, [r1]
mul r1, r0 @ Offset for the entry we want 
ldr r0, =AoE_Table 
add r0, r1 @ Specific entry 
mov r4, r0 

@mov r0, r4 
@ Given r0 = Table Entry, construct range map 
bl AoE_RangeSetup_Hover
mov r0, #2 
blh 0x801da98 @DisplayMoveRangeGraphics


mov r2, r4 

ldr r3, =CurrentUnit
ldr r3, [r3] 
ldrb r0, [r3, #0x10] @ XX 
ldrb r1, [r3, #0x11] @ YY 
@given r0 = xx, r1 = yy, r2= table entry pointer, display movement squares in a template around it 
bl AoE_DisplayDamageArea
@mov r0, #42 
@blh 0x801da98 @DisplayMoveRangeGraphics

pop {r4}
pop {r0} 
bx r0 

.align 4
.ltorg 

AoE_SpecificEffectIndex:
@ WORD AoE_Index
