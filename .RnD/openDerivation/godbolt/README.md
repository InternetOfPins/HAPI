# Banknote wave4 on Compiler Explorer: HAPI vs plain C

This is the trained Banknote cell from `examples/static_net`, built two ways, so their assembly can be compared:

| file | what it is |
|---|---|
| `wave4_apiof.cpp` | the HAPI cell as static_net writes it: `Cell = hapi::APIOf<API,OO...,Bias<k>>`, with the trained parameters as template arguments |
| `wave4_c.c` | the same cell written by hand in plain C: one function, `bool wave4(const uint8_t* x)`, no templates, no HAPI |

A third source, `wave4_od.cpp`, is the HAPI cell written with Open Derivation (`:`) and translated by `../translate.py`. It is the
same program as `wave4_apiof.cpp` after translation, so it only checks the translator (their diff is empty by construction);
it is not a comparison with anything outside HAPI.

## HAPI vs plain C

Both compiled as C++ with AVR gcc 7.3.0, `-std=c++17 -Os -mmcu=atmega328p`. `verify.sh` produced these listings (`verify.log`).

<table>
<tr><th>HAPI (<code>wave4_apiof.cpp</code>): 28 instructions, 56 B</th><th>plain C (<code>wave4_c.c</code>): 27 instructions, 54 B</th></tr>
<tr><td>

```asm
movw r30, r24
ldd  r25, Z+3     ; x[3]
mov  r24, r25     ; <- the extra instruction
adc  r24, r24
eor  r24, r24
adc  r24, r24     ; x[3] >> 7
ldd  r25, Z+2     ; x[2]
lsr  r25
lsr  r25          ; x[2] >> 2
add  r25, r24
subi r25, 0x51    ; + 175
ld   r18, Z       ; x[0]
lsr  r18
lsr  r18
subi r18, 0x92    ; + 110
andi r18, 0xBA
add  r25, r18
ldd  r24, Z+1     ; x[1]
lsr  r24
lsr  r24
subi r24, 0xB0    ; + 80
andi r24, 0xBF
add  r24, r25
com  r24
adc  r24, r24
eor  r24, r24
adc  r24, r24     ; sum < 128
ret
```

</td><td>

```asm
movw r30, r24
ldd  r25, Z+2     ; x[2]
lsr  r25
lsr  r25          ; x[2] >> 2
ldd  r24, Z+3     ; x[3]
adc  r24, r24
eor  r24, r24
adc  r24, r24     ; x[3] >> 7
add  r25, r24
subi r25, 0x51    ; + 175
ld   r18, Z       ; x[0]
lsr  r18
lsr  r18
subi r18, 0x92    ; + 110
andi r18, 0xBA
add  r25, r18
ldd  r24, Z+1     ; x[1]
lsr  r24
lsr  r24
subi r24, 0xB0    ; + 80
andi r24, 0xBF
add  r24, r25
com  r24
adc  r24, r24
eor  r24, r24
adc  r24, r24     ; sum < 128
ret
```

</td></tr>
</table>

What this shows, and what it does not:

- **The same computation.** Both do the same loads, shifts, adds and masks, with the same constants. Neither has a call, a
  table, a loop or a stack frame. The template layers (`Threshold`, four `Wave`, `Bias`, `API`) leave nothing behind but the
  arithmetic they describe.
- **Not identical: HAPI is one instruction longer.** HAPI loads `x[3]` into `r25` and copies it to `r24` (`mov r24, r25`);
  plain C loads it straight into `r24`. That is 2 B of flash and 1 cycle. The rest differs only in order: HAPI starts from
  `x[3]` because its layers nest with `Wave<3>` innermost (next to `Bias`), and plain C adds in the order it is written.
- **It is register allocation, not HAPI and not `:`.** At `-Os` this gcc's register choice is sensitive to how the
  expression is spelled. Code with no HAPI in it gets the same 28 instructions, with the same `mov`: a hand-rolled
  template mixin chain (`Th<Wave<0,..,Wave<3,..,Bias<175,API>>>>>`, plain inheritance), the plain C++ statements with
  `WaveOf`'s casts, plain C++ with a small helper lambda for the four inputs, and `wave4_c.c` with a `(uint8_t)` cast
  before each mask (28 as C++, 27 as C). Only the direct plain C spellings reach 27. Changing `WaveOf`'s casts (dropping
  the inner ones, doing the arithmetic in `int`) leaves the HAPI cell at 28.
- **Reordering does not change it.** Plain C gives 27 instructions in every order of the four `s +=` lines tried
  (0123, 3210, 3012, 2301, 1032, and nested like HAPI). HAPI with the `Wave` layers reversed in `net.h` still gives 28:
  the `mov` just moves to whichever input comes first (`x[2]` instead of `x[3]`).
- **Same answers.** `verify.sh` runs both on all 2^32 inputs (host g++) and gets the same class for every one.
- **C or C++ does not matter here.** `wave4_c.c` built as C (`avr-gcc -std=c99`) gives the same 27 instructions.

On Compiler Explorer, both sources side by side: [open the comparison](https://godbolt.org/clientstate/eyJzZXNzaW9ucyI6W3siaWQiOjEsImxhbmd1YWdlIjoiYysrIiwic291cmNlIjoiLy8gQmFua25vdGUgd2F2ZTQgKHN0YXRpY19uZXQpLCB3YXZlQ2VsbC5oIGFzIHdyaXR0ZW4gYnkgaGFuZDogQ2VsbCA9IGhhcGk6OkFQSU9mPEFQSSxPTy4uLixCaWFzPGs-Pi5cbi8vIEdlbmVyYXRlZCBieSAuUm5EL29wZW5EZXJpdmF0aW9uL2dvZGJvbHQvbWFrZS5weSAoSEFQSSAzYjBjNDY2KS4gQ29tcGlsZSBmb3IgQVZSIHdpdGhcbi8vIC1zdGQ9YysrMTcgLU9zIC1tbWN1PWF0bWVnYTMyOHA7IHdhdmU0KCkgaXMgdGhlIGZ1bmN0aW9uIHRvIHJlYWQgKHNlZSBSRUFETUUubWQpLlxuI2luY2x1ZGUgPGh0dHBzOi8vcmF3LmdpdGh1YnVzZXJjb250ZW50LmNvbS9JbnRlcm5ldE9mUGlucy9IQVBJLzNiMGM0NjZjYTVhNjg3MGEwNzQ3YWJjODEzM2ZkNjQ3NDNjMzMzYjcvc2luZ2xlL2hhcGkuaD5cblxuLy8gPT09PT09PT0gZXhhbXBsZXMvc3RhdGljX25ldC9pbmNsdWRlL3N0YXRpY05ldC5oID09PT09PT09XG4vLyBzdGF0aWNOZXQuaCBcdTIwMTQgZW5naW5lLWFnbm9zdGljIHN0YXRpYyBuZXR3b3JrIGNvbXBvc2l0aW9uXG4vLyBOZXQgPSB0eXBlbGlzdCBvZiBjZWxsIHR5cGVzIChubyBpbnN0YW5jZXMpLiBTdGF0ZSBpcyB0aGUgb25seSBydW50aW1lIGRhdGEuXG4vLyBDZWxscyBhcmUgYW55IHR5cGUgd2l0aCBgc3RhdGljIHByb2MoaW4pYCAoYW5kIG9wdGlvbmFsbHkgYHN0YXRpYyB1cGRhdGUoaW4pYCksXG4vLyBwdXJlIG92ZXIgdGhlIHN0YXRlIHZpZXcgdGhleSBhcmUgZ2l2ZW4uIFZhbHVlIHR5cGVzIGZsb3cgZnJvbSBlYWNoIGNlbGwncyBwcm9jLlxuI2luY2x1ZGUgPHN0ZGRlZi5oPlxuXG4jaWZuZGVmIFNORVRfSU5MSU5FXG4gIC8vIGF2ci1nY2MgNy4zIC1PcyBkZWNsaW5lcyB0byBpbmxpbmUgbXVsdGlwbHktY2FsbGVkIHByb2MgY2hhaW5zIChtZWFzdXJlZCwgc2VlIFJFQURNRSk7IGZvcmNlIGl0XG4gICNkZWZpbmUgU05FVF9JTkxJTkUgW1tnbnU6OmFsd2F5c19pbmxpbmVdXVxuI2VuZGlmXG5cbm5hbWVzcGFjZSBzbmV0IHtcbiAgLy8gc3RhdGUgdmlldzogbmV0IGFuZCBjdXJyZW50IGNlbGwgaW5kZXggYXMgcGhhbnRvbSB0eXBlcywgaG9sZHMgb25seSBhIHJlZmVyZW5jZVxuICB0ZW1wbGF0ZTx0eXBlbmFtZSBOLHR5cGVuYW1lIFMsc2l6ZV90IGs-IHN0cnVjdCBDdHgge1xuICAgIHVzaW5nIE5ldD1OO1xuICAgIHN0YXRpYyBjb25zdGV4cHIgc2l6ZV90IGN1cj1rO1xuICAgIFMmIHM7XG4gICAgdGVtcGxhdGU8c2l6ZV90IGo-IFNORVRfSU5MSU5FIGNvbnN0ZXhwciBDdHg8TixTLGo-IGF0KCkgY29uc3Qge3JldHVybiB7c307fVxuICAgIHRlbXBsYXRlPHNpemVfdCBpPiBTTkVUX0lOTElORSBjb25zdGV4cHIgZGVjbHR5cGUoYXV0bykgZ2V0KCkgY29uc3Qge3JldHVybiBzLnRlbXBsYXRlIGdldDxpPigpO31cbiAgICB0ZW1wbGF0ZTxzaXplX3QgaSx0eXBlbmFtZSBWPiBTTkVUX0lOTElORSBjb25zdGV4cHIgdm9pZCBzZXQoViB4KSBjb25zdCB7cy50ZW1wbGF0ZSBzZXQ8aT4oeCk7fVxuICAgIFNORVRfSU5MSU5FIGNvbnN0ZXhwciBhdXRvIGRhdGEoKSBjb25zdCB7cmV0dXJuIHMuZGF0YSgpO30gICAgIC8vIHRoZSByYXcgc2xvdCBieXRlcywgZm9yIHBhcnRzIHRoYXQgaXRlcmF0ZSBvdmVyIHRoZW0gKHJvbGwuaClcbiAgfTtcblxuICAvLyBpbi1wbGFjZSByZWZlcmVuY2UgdG8gY2VsbCBqIG9mIHRoZSBlbmNsb3NpbmcgbmV0IChzYW1lIHBhc3MsIHB1cmUsIENTRS1mb2xkYWJsZSlcbiAgdGVtcGxhdGU8c2l6ZV90IGo-IHN0cnVjdCBSZWYge1xuICAgIHRlbXBsYXRlPHR5cGVuYW1lIEk-IFNORVRfSU5MSU5FIHN0YXRpYyBjb25zdGV4cHIgYXV0byBnZXQoY29uc3QgSSYgaW4pIHtcbiAgICAgIHN0YXRpY19hc3NlcnQoajxJOjpjdXIsXCJzbmV0OjpSZWY8aj46IGluLXBsYWNlIHJlZmVyZW5jZSBtdXN0IHBvaW50IHRvIGEgbG93ZXIgbmV0IGluZGV4OyBhIGN5Y2xlIG5lZWRzIGEgcmVnaXN0ZXIgKFNsb3Q8aT4gKyBTdG9yZTxpPilcIik7XG4gICAgICByZXR1cm4gSTo6TmV0Ojp0ZW1wbGF0ZSBDZWxsPGo-Ojpwcm9jKGluLnRlbXBsYXRlIGF0PGo-KCkpO1xuICAgIH1cbiAgfTtcblxuICAvLyBzdGF0ZSBzbG90IGk6IGEgdHJ1ZSBpbnB1dCBlZGdlLCBvciBhIHJlZ2lzdGVyIHdyaXR0ZW4gYnkgYSBTdG9yZTxpPlxuICB0ZW1wbGF0ZTxzaXplX3QgaT4gc3RydWN0IFNsb3Qge1xuICAgIHRlbXBsYXRlPHR5cGVuYW1lIEk-IFNORVRfSU5MSU5FIHN0YXRpYyBjb25zdGV4cHIgYXV0byBnZXQoY29uc3QgSSYgaW4pIHtyZXR1cm4gaW4udGVtcGxhdGUgZ2V0PGk-KCk7fVxuICB9O1xuXG4gIHRlbXBsYXRlPHR5cGVuYW1lLi4uIENDPiBzdHJ1Y3QgTmV0IHtcbiAgICB1c2luZyBDZWxscz1oYXBpOjpDaGFpbjxDQy4uLj47XG4gICAgdGVtcGxhdGU8c2l6ZV90IGo-IHVzaW5nIENlbGw9dHlwZW5hbWUgQ2VsbHM6OnRlbXBsYXRlIERyb3A8aj46OkhlYWQ7XG4gICAgdGVtcGxhdGU8c2l6ZV90IGosdHlwZW5hbWUgUz4gU05FVF9JTkxJTkUgc3RhdGljIGNvbnN0ZXhwciBhdXRvIHByb2MoUyYgcylcbiAgICAgIHtyZXR1cm4gQ2VsbDxqPjo6cHJvYyhDdHg8TmV0LFMsaj57c30pO31cbiAgICB0ZW1wbGF0ZTxzaXplX3Qgaix0eXBlbmFtZSBTPiBTTkVUX0lOTElORSBzdGF0aWMgY29uc3RleHByIHZvaWQgdXBkYXRlKFMmIHMpXG4gICAgICB7Q3R4PE5ldCxTLGo-IGN7c307IENlbGw8aj46OnVwZGF0ZShjKTt9XG4gIH07XG59XG5cbi8vIFdoYXQgYSBOZXQgaG9sZHMsIGZvciBIQVBJJ3Mgc3RydWN0dXJhbCB3YWxrczogaXRzIGNlbGxzLCBpbiBvcmRlci4gUXVlcmllcywgRmlsdGVyL01hcC9QYXJ0aXRpb24gYW5kIEZpbmRGaXJzdFxuLy8gb3BlbiBpdDsgcnVsZXMoKSBhcmUgbm90IHJ1biBpbnNpZGUgaXQgKG5vdGhpbmcgbmVzdHMgYSBOZXQgdG9kYXkpLiBUaGUgY2VsbHMgc3RheSB3aG9sZSwgYSBjZWxsIGJlaW5nIGFuIEFQSU9mXG4vLyAoYSBsZWFmIGZvciB0aG9zZSB3YWxrcyksIHdoaWNoIGlzIHdoeSByZWZpZC5oIG1hdGNoZXMgdGhlbSB3aXRoIEZyb21UeXBlcy5cbm5hbWVzcGFjZSBoYXBpIHtcbiAgdGVtcGxhdGU8dHlwZW5hbWUuLi4gQ0M-XG4gIHN0cnVjdCBFeHBhbmQ8c25ldDo6TmV0PENDLi4uPj4gOiBFeHBhbnNpb248Q2hhaW48Q0MuLi4-LHRydWUsdHJ1ZSxmYWxzZSx0cnVlPiB7fTtcbn1cblxuLy8gPT09PT09PT0gZXhhbXBsZXMvc3RhdGljX25ldC9pbmNsdWRlL3dhdmVDZWxsLmggPT09PT09PT1cbi8vIHdhdmVDZWxsLmggXHUyMDE0IGNvbmRpdGlvbmFsLWZyZWUsIG11bHRpcGx5LWZyZWUgcGhhc2UgY2VsbCBjb21wb25lbnRzIChIQVBJIFBhcnQ8Tz4gc3R5bGUpXG4vLyBvbmUgZW5naW5lIGZvciBzbmV0OjpOZXQ7IHRoZSB1OCBjYXN0IG9mIGFueSBzb3VyY2UgbGl2ZXMgaGVyZSwgbm90IGluIHRoZSBjb21wb3NpdGlvbiBsYXllclxuI2luY2x1ZGUgPHN0ZGludC5oPlxuXG5uYW1lc3BhY2Ugd2F2ZSB7XG4gIHVzaW5nIHU4PXVpbnQ4X3Q7IC8vIG5vdCBcdTAwYjU6IFVURi04IGlkZW50aWZpZXJzIG5lZWQgR0NDPj0xMCwgYXZyLWdjYyA3LjMgcmVqZWN0cyB0aGVtXG4gIHVzaW5nIHNuZXQ6Ok5ldDsgdXNpbmcgc25ldDo6U2xvdDsgdXNpbmcgc25ldDo6UmVmO1xuXG4gIHN0cnVjdCBBUEkge1xuICAgIHRlbXBsYXRlPHR5cGVuYW1lIEk-IFNORVRfSU5MSU5FIHN0YXRpYyBjb25zdGV4cHIgdTggcHJvYyhjb25zdCBJJikge3JldHVybiAwO31cbiAgICB0ZW1wbGF0ZTx0eXBlbmFtZSBJPiBTTkVUX0lOTElORSBzdGF0aWMgY29uc3RleHByIHZvaWQgdXBkYXRlKEkmKSB7fVxuICB9O1xuXG4gIC8vIGdsb2JhbCBwaGFzZSBvZmZzZXRcbiAgdGVtcGxhdGU8dTggaz5cbiAgc3RydWN0IEJpYXMge3RlbXBsYXRlPHR5cGVuYW1lIE8-IHN0cnVjdCBQYXJ0Ok8ge1xuICAgIHVzaW5nIEJhc2U9TzsgdXNpbmcgQmFzZTo6QmFzZTtcbiAgICB0ZW1wbGF0ZTx0eXBlbmFtZSBJPiBTTkVUX0lOTElORSBzdGF0aWMgY29uc3RleHByIHU4IHByb2MoY29uc3QgSSYgaW4pIHtyZXR1cm4gdTgoaytCYXNlOjpwcm9jKGluKSk7fVxuICB9O307XG5cbiAgLy8gZ3JhZGVkIHNvdXJjZTogb3B0aW9uYWwgTk9ULCBzaGlmdCAoczwwIHJpZ2h0LCBzPjAgbGVmdCksIHBoYXNlIGFkZCwgYml0LW1hc2sgcmVhZG91dFxuICB0ZW1wbGF0ZTx0eXBlbmFtZSBTcmMsYm9vbCBuLGludCBzLHU4IHAsdTggbT5cbiAgc3RydWN0IFdhdmVPZiB7dGVtcGxhdGU8dHlwZW5hbWUgTz4gc3RydWN0IFBhcnQ6TyB7XG4gICAgdXNpbmcgQmFzZT1POyB1c2luZyBCYXNlOjpCYXNlO1xuICAgIHN0YXRpYyBjb25zdGV4cHIgdTggaW52KHU4IHgpIHtyZXR1cm4gbj91OCh-eCk6eDt9XG4gICAgc3RhdGljIGNvbnN0ZXhwciB1OCBzaGYodTggeCkge3JldHVybiBzPj0wP3U4KHg8PHMpOnU4KHg-PigtcykpO31cbiAgICB0ZW1wbGF0ZTx0eXBlbmFtZSBJPiBTTkVUX0lOTElORSBzdGF0aWMgY29uc3RleHByIHU4IHByb2MoY29uc3QgSSYgaW4pXG4gICAgICB7cmV0dXJuIHU4KHU4KHU4KHNoZihpbnYodTgoU3JjOjpnZXQoaW4pKSkpK3ApJm0pK0Jhc2U6OnByb2MoaW4pKTt9XG4gIH07fTtcbiAgdGVtcGxhdGU8c2l6ZV90IGksYm9vbCBuLGludCBzLHU4IHAsdTggbT4gdXNpbmcgV2F2ZT1XYXZlT2Y8U2xvdDxpPixuLHMscCxtPjtcbiAgdGVtcGxhdGU8c2l6ZV90IGosYm9vbCBuLGludCBzLHU4IHAsdTggbT4gdXNpbmcgUmVmV2F2ZT1XYXZlT2Y8UmVmPGo-LG4scyxwLG0-O1xuXG4gIC8vIHJlYWRvdXQgb2YgYml0IDcgKHBoYXNlIDwgaGFsZilcbiAgc3RydWN0IFRocmVzaG9sZCB7dGVtcGxhdGU8dHlwZW5hbWUgTz4gc3RydWN0IFBhcnQ6TyB7XG4gICAgdXNpbmcgQmFzZT1POyB1c2luZyBCYXNlOjpCYXNlO1xuICAgIHRlbXBsYXRlPHR5cGVuYW1lIEk-IFNORVRfSU5MSU5FIHN0YXRpYyBjb25zdGV4cHIgYm9vbCBwcm9jKGNvbnN0IEkmIGluKSB7cmV0dXJuIEJhc2U6OnByb2MoaW4pPDEyODt9XG4gIH07fTtcblxuICAvLyB3cml0ZXMgdGhlIHJ1bm5pbmcgcHJvYygpIHZhbHVlIGF0IHRoaXMgcG9pbnQgaW50byBzdGF0ZSBzbG90IGksIHRoZW4gY2hhaW5zIHRvIHRoZSBuZXh0IFN0b3JlXG4gIHRlbXBsYXRlPHNpemVfdCBpPlxuICBzdHJ1Y3QgU3RvcmUge3RlbXBsYXRlPHR5cGVuYW1lIE8-IHN0cnVjdCBQYXJ0Ok8ge1xuICAgIHVzaW5nIEJhc2U9TzsgdXNpbmcgQmFzZTo6QmFzZTtcbiAgICB0ZW1wbGF0ZTx0eXBlbmFtZSBJPiBTTkVUX0lOTElORSBzdGF0aWMgY29uc3RleHByIHZvaWQgdXBkYXRlKEkmIGluKSB7XG4gICAgICBpbi50ZW1wbGF0ZSBzZXQ8aT4oQmFzZTo6cHJvYyhpbikpO1xuICAgICAgQmFzZTo6dXBkYXRlKGluKTtcbiAgICB9XG4gIH07fTtcblxuICB0ZW1wbGF0ZTx1OCBrLHR5cGVuYW1lLi4uIE9PPiB1c2luZyBDZWxsPWhhcGk6OkFQSU9mPEFQSSxPTy4uLixCaWFzPGs-PjtcblxuICAvLyBpbnB1dCBzdGF0ZSBwYXNzZWQgYnkgdmFsdWUvcmVmZXJlbmNlLCBubyBnbG9iYWwgcmVhZHMgKENTRS1lbGlnaWJsZSlcbiAgdGVtcGxhdGU8c2l6ZV90IE4-IHN0cnVjdCBGZWF0dXJlcyB7XG4gICAgdTggdltOXTtcbiAgICB0ZW1wbGF0ZTxzaXplX3QgaT4gY29uc3RleHByIHU4IGdldCgpIGNvbnN0IHtyZXR1cm4gdltpXTt9XG4gICAgdGVtcGxhdGU8c2l6ZV90IGk-IGNvbnN0ZXhwciB2b2lkIHNldCh1OCB4KSB7dltpXT14O31cbiAgICBjb25zdGV4cHIgY29uc3QgdTgqIGRhdGEoKSBjb25zdCB7cmV0dXJuIHY7fVxuICB9O1xufVxuXG4vLyA9PT09PT09PSBleGFtcGxlcy9zdGF0aWNfbmV0L21vZGVscy9iYW5rbm90ZS93YXZlX3BhcmFtcy5oID09PT09PT09XG4vLyBnZW5lcmF0ZWQgYnkgYm4yLmMsIEJhbmtub3RlIDUtZm9sZCBDViwgZm9sZCAxIChzZWVkIDQyKTogdHJhaW4gOTkuNjQlIHRlc3QgMTAwLjAwJVxuI2RlZmluZSBXQVZFX0sgMTc1XG4jZGVmaW5lIFdBVkVfRjBfTiAwXG4jZGVmaW5lIFdBVkVfRjBfUyAtMlxuI2RlZmluZSBXQVZFX0YwX1AgMTEwXG4jZGVmaW5lIFdBVkVfRjBfTSAweGJhXG4jZGVmaW5lIFdBVkVfRjFfTiAwXG4jZGVmaW5lIFdBVkVfRjFfUyAtMlxuI2RlZmluZSBXQVZFX0YxX1AgODBcbiNkZWZpbmUgV0FWRV9GMV9NIDB4YmZcbiNkZWZpbmUgV0FWRV9GMl9OIDBcbiNkZWZpbmUgV0FWRV9GMl9TIC0yXG4jZGVmaW5lIFdBVkVfRjJfUCAwXG4jZGVmaW5lIFdBVkVfRjJfTSAweDNmXG4jZGVmaW5lIFdBVkVfRjNfTiAwXG4jZGVmaW5lIFdBVkVfRjNfUyAtN1xuI2RlZmluZSBXQVZFX0YzX1AgMFxuI2RlZmluZSBXQVZFX0YzX00gMHgwMVxuLy8gcXVhbnRpemF0aW9uIHE9cm91bmQoKHgtbWluKS8obWF4LW1pbikqMjU1KSwgY2xhbXBlZFxuc3RhdGljIGNvbnN0IGRvdWJsZSBXQVZFX01JTls0XT17LTcuMDQyMDk5OTk5OTk5OTk5NiwtMTMuNzczMDk5OTk5OTk5OTk5LC01LjI4NjEwMDAwMDAwMDAwMDIsLTguNTQ4MTk5OTk5OTk5OTk5Nn07XG5zdGF0aWMgY29uc3QgZG91YmxlIFdBVkVfTUFYWzRdPXs2LjgyNDc5OTk5OTk5OTk5OTgsMTIuNzMwMiwxNy45MjczOTk5OTk5OTk5OTksMi4xNjI1MDAwMDAwMDAwMDAxfTtcblxuLy8gPT09PT09PT0gZXhhbXBsZXMvc3RhdGljX25ldC9tb2RlbHMvYmFua25vdGUvbmV0LmggPT09PT09PT1cbi8vIFRoZSB0cmFpbmVkIEJhbmtub3RlIGNsYXNzaWZpZXIgb2YgdGhlIHJ1bm5pbmcgZXhhbXBsZSwgYXMgYSBUWVBFOiBvbmUgd2F2ZSBjZWxsIG9mIGZvdXIgZ3JhZGVkIGlucHV0cy4gVGhlIHRyYWluZWQgcGFyYW1ldGVyc1xuLy8gKHdhdmVfcGFyYW1zLmgsIGdlbmVyYXRlZCBieSB0cmFpbi9ibjJ4LmM6IGZvbGQgMSkgYXJlIHRlbXBsYXRlIGFyZ3VtZW50czsgdGhlcmUgaXMgbm8gdGFibGUgYW5kIG5vIHN0YXRlLlxudXNpbmcgQmFua25vdGVOZXQ9d2F2ZTo6Q2VsbDxXQVZFX0ssXG4gIHdhdmU6OlRocmVzaG9sZCxcbiAgd2F2ZTo6V2F2ZTwwLFdBVkVfRjBfTixXQVZFX0YwX1MsV0FWRV9GMF9QLFdBVkVfRjBfTT4sXG4gIHdhdmU6OldhdmU8MSxXQVZFX0YxX04sV0FWRV9GMV9TLFdBVkVfRjFfUCxXQVZFX0YxX00-LFxuICB3YXZlOjpXYXZlPDIsV0FWRV9GMl9OLFdBVkVfRjJfUyxXQVZFX0YyX1AsV0FWRV9GMl9NPixcbiAgd2F2ZTo6V2F2ZTwzLFdBVkVfRjNfTixXQVZFX0YzX1MsV0FWRV9GM19QLFdBVkVfRjNfTT5cbj47XG5cbi8vID09PT09PT09IGVudHJ5IHBvaW50IChjb21wYXJlX2VtbGVhcm4vYm5jX3dhdmUuaCdzIGJuY19wcmVkaWN0KSA9PT09PT09PVxuYm9vbCB3YXZlNChjb25zdCB1aW50OF90KiB4KSB7d2F2ZTo6RmVhdHVyZXM8ND4gZnt7eFswXSx4WzFdLHhbMl0seFszXX19OyByZXR1cm4gQmFua25vdGVOZXQ6OnByb2MoZik7fVxuIiwiZmlsZW5hbWUiOiJ3YXZlNF9hcGlvZi5jcHAiLCJjb21waWxlcnMiOlt7ImlkIjoiYXZyZzczMCIsIm9wdGlvbnMiOiItc3RkPWMrKzE3IC1PcyAtbW1jdT1hdG1lZ2EzMjhwIiwiZmlsdGVycyI6eyJiaW5hcnkiOmZhbHNlLCJjb21tZW50T25seSI6dHJ1ZSwiZGVtYW5nbGUiOnRydWUsImRpcmVjdGl2ZXMiOnRydWUsImV4ZWN1dGUiOmZhbHNlLCJpbnRlbCI6ZmFsc2UsImxhYmVscyI6dHJ1ZSwibGlicmFyeUNvZGUiOnRydWUsInRyaW0iOmZhbHNlfSwibGlicyI6W119XX0seyJpZCI6MiwibGFuZ3VhZ2UiOiJjKysiLCJzb3VyY2UiOiIvLyBCYW5rbm90ZSB3YXZlNCAoc3RhdGljX25ldCksIHdyaXR0ZW4gYnkgaGFuZCBpbiBwbGFpbiBDOiBubyB0ZW1wbGF0ZXMsIG5vIEhBUEksIG5vdGhpbmcgYnV0IDxzdGRpbnQuaD4gYW5kIDxzdGRib29sLmg-LlxuLy8gVGhlIHNhbWUgdHJhaW5lZCBjZWxsIGFzIHdhdmU0X2FwaW9mLmNwcCBhbmQgd2F2ZTRfb2QuY3BwIChtb2RlbHMvYmFua25vdGUvd2F2ZV9wYXJhbXMuaCwgZm9sZCAxKSwgc3BlbGxlZCBvdXQ6XG4vLyBlYWNoIGlucHV0IGlzIHNoaWZ0ZWQgcmlnaHQsIG9mZnNldCBieSBhIHBoYXNlIGFuZCBtYXNrZWQ7IHRoZSBmb3VyIHBoYXNlcyBhbmQgdGhlIGJpYXMgYWRkIHVwIG1vZHVsbyAyNTYsIGFuZCB0aGVcbi8vIGNsYXNzIGlzIGJpdCA3IG9mIHRoZSBzdW0gKHN1bSA8IDEyOCkuIEl0IGNvbXBpbGVzIGFzIEMgb3IgYXMgQysrOyBmb3IgdGhlIGNvbXBhcmlzb24sIGNvbXBpbGUgaXQgYXMgdGhlIG90aGVyIHR3b1xuLy8gYXJlLCBhcyBDKysgd2l0aCAtc3RkPWMrKzE3IC1PcyAtbW1jdT1hdG1lZ2EzMjhwIChzZWUgUkVBRE1FLm1kKS5cbiNpbmNsdWRlIDxzdGRpbnQuaD5cbiNpbmNsdWRlIDxzdGRib29sLmg-XG5cbmJvb2wgd2F2ZTQoY29uc3QgdWludDhfdCogeCkge1xuICB1aW50OF90IHMgPSAxNzU7ICAgICAgICAgICAgICAgICAgICAgIC8vIFdBVkVfS1xuICBzICs9ICgoeFswXSA-PiAyKSArIDExMCkgJiAweGJhOyAgICAgIC8vIFdBVkVfRjA6IHNoaWZ0IC0yLCBwaGFzZSAxMTAsIG1hc2sgMHhiYVxuICBzICs9ICgoeFsxXSA-PiAyKSArICA4MCkgJiAweGJmOyAgICAgIC8vIFdBVkVfRjE6IHNoaWZ0IC0yLCBwaGFzZSAgODAsIG1hc2sgMHhiZlxuICBzICs9ICh4WzJdID4-IDIpICYgMHgzZjsgICAgICAgICAgICAgIC8vIFdBVkVfRjI6IHNoaWZ0IC0yLCBwaGFzZSAgIDAsIG1hc2sgMHgzZlxuICBzICs9ICh4WzNdID4-IDcpICYgMHgwMTsgICAgICAgICAgICAgIC8vIFdBVkVfRjM6IHNoaWZ0IC03LCBwaGFzZSAgIDAsIG1hc2sgMHgwMVxuICByZXR1cm4gcyA8IDEyODtcbn1cbiIsImZpbGVuYW1lIjoid2F2ZTRfYy5jIiwiY29tcGlsZXJzIjpbeyJpZCI6ImF2cmc3MzAiLCJvcHRpb25zIjoiLXN0ZD1jKysxNyAtT3MgLW1tY3U9YXRtZWdhMzI4cCIsImZpbHRlcnMiOnsiYmluYXJ5IjpmYWxzZSwiY29tbWVudE9ubHkiOnRydWUsImRlbWFuZ2xlIjp0cnVlLCJkaXJlY3RpdmVzIjp0cnVlLCJleGVjdXRlIjpmYWxzZSwiaW50ZWwiOmZhbHNlLCJsYWJlbHMiOnRydWUsImxpYnJhcnlDb2RlIjp0cnVlLCJ0cmltIjpmYWxzZX0sImxpYnMiOltdfV19XX0=).
It is a full-state link (Compiler Explorer's `/clientstate/` form), with each source in its own pane with AVR gcc 7.3.0
(`avrg730`) and the flags above. Diff the two compiler panes to see the `mov`. It has not been opened from this session
(godbolt.org is unreachable here), so the compiler id is unconfirmed.

The earlier link, <https://godbolt.org/z/TvM334frd>, is the translator check: `wave4_apiof.cpp` against `wave4_od.cpp`,
empty diff, 28 instructions in both.

Each C++ source is self-contained except for one line, which includes HAPI's single header by raw GitHub URL, pinned to the commit that added it:

```c++
#include <https://raw.githubusercontent.com/InternetOfPins/HAPI/3b0c466ca5a6870a0747abc8133fd64743c333b7/single/hapi.h>
```

In the two HAPI sources everything else is inlined, unchanged: static_net's `staticNet.h`, `waveCell.h` (in one of its two forms), `wave_params.h` and `net.h`, and
`wave4()`, which is `compare_emlearn/bnc_wave.h`'s `bnc_predict` under another name.

## Settings

| | |
|---|---|
| language | C++ |
| compiler | **AVR gcc 7.3.0**: the toolchain every static_net figure was measured with (Ubuntu's `gcc-avr`, 7.3.0). If Compiler Explorer's list has no 7.3.0, take the nearest AVR gcc it offers. Other versions should compile it (C++17), but their instruction counts are not the ones below |
| flags | `-std=c++17 -Os -mmcu=atmega328p` |

To compare two of them, open one source pane per file, each with its own compiler pane (same compiler and flags), and use Compiler Explorer's diff view. Paste `wave4_c.c` into a C++ pane too, so all three are compiled the same way.

## What `verify.sh` checks

`verify.sh` compiles every source with avr-gcc 7.3.0 against the exact bytes the URL serves (`verify.log`):

- `wave4_apiof.cpp` and `wave4_od.cpp` give the same `wave4()`, 28 instructions, 56 B (the translator check).
- That `wave4()` is instruction for instruction the `bnc_predict` static_net builds from `include/` for `compare_emlearn`.
  The 44 B figure static_net reports is this function minus the null model's `bnc_predict` (12 B, a single compare), the floor
  `compare_emlearn/run.sh` subtracts: 56 − 12 = 44. For plain C the same subtraction gives 54 − 12 = 42.
- `wave4_c.c` compiles as C++ and as C to the same 27 instructions, 54 B, and differs from the HAPI cell by the instructions
  listed above.
- `wave4_c.c` and the HAPI cell give the same answer on all 2^32 inputs.

The pinned URL was fetched and matches `single/hapi.h` at that commit byte for byte.

## Regenerate

```sh
python3 make.py     # rebuild the two HAPI sources from the current files (wave4_c.c is written by hand)
./verify.sh         # compile all three as Compiler Explorer would; compare wave4() across them and with static_net's bnc_predict
```

`make.py` pins `single/hapi.h` to `HAPI_SHA`. Bump it when `include/hapi/` changes.
