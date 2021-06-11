.global _rsqrt_inaccurate
.global rsqrt_inaccurate
.global _rsqrt_full
.global rsqrt_full
.global _rsqrt_full_gpr
.global rsqrt_full_gpr
.global _rsqrt_full_nb
.global rsqrt_full_nb
.global _rsqrt_full_nb2
.global rsqrt_full_nb2
.global _rsqrt_full_nb_gpr
.global rsqrt_full_nb_gpr
.global _rsqrt_newton
.global rsqrt_newton
.global _rsqrt_hack
.global rsqrt_hack
.global _rsqrt_fallback

.text
.intel_syntax noprefix

.align 16
min_pos_denorm:
.long 0x00800000,0,0,0
penultimate_bit:
.long 0x00008000,0,0,0
ultimate_bit:
.long 0x00004000,0,0,0
top_mask:
.long 0xFFFF8000,0,0,0
one:
.long 0x3f800000,0,0,0
half:
.long 0x3f000000,0,0,0
one_point_five:
.long 0x3fc00000,0,0,0
magic1:
.long 0x60000000,0,0,0
magic2:
.long 0x3c000000,0,0,0
magic3:
.long 0x000047ff,0,0,0

_rsqrt_inaccurate:
rsqrt_inaccurate:
    movd xmm0, edi

    rsqrtss xmm0, xmm0

    movd eax, xmm0
    ret

_rsqrt_full:
rsqrt_full:
    movd xmm0, edi

    pand xmm0, [rip + top_mask]
    por xmm0, [rip + penultimate_bit]

    vcmpngt_uqss xmm1, xmm0, [rip + min_pos_denorm]
    ptest xmm1, xmm1
    jnz rsqrt_full_bad

    sqrtss xmm0, xmm0

    movd xmm1, [rip + one]
    divss xmm1, xmm0

    paddd xmm1, [rip + ultimate_bit]
    pand xmm1, [rip + top_mask]

    movd eax, xmm1
    ret

_rsqrt_full_gpr:
rsqrt_full_gpr:
    movd eax, xmm0 # Emulate regalloc mov

    mov eax, edi
    and eax, 0xFFFF8000
    or eax, 0x00008000

    movd xmm0, eax
    vcmpngt_uqss xmm1, xmm0, [rip + min_pos_denorm]
    ptest xmm1, xmm1
    jnz rsqrt_full_bad

    sqrtss xmm0, xmm0

    movd xmm1, [rip + one]
    divss xmm1, xmm0
    movd eax, xmm1

    add eax, 0x00004000
    and eax, 0xffff8000

    movd xmm0, eax # Emulate regalloc mov
    ret

_rsqrt_full_nb2:
rsqrt_full_nb2:
    movd xmm0, edi

    pand xmm0, [rip + top_mask]
    por xmm0, [rip + penultimate_bit]

    ucomiss xmm0, [rip + min_pos_denorm]
    jna rsqrt_full_bad_new1

    sqrtss xmm0, xmm0

    movd xmm1, [rip + one]
    divss xmm1, xmm0

    paddd xmm1, [rip + ultimate_bit]
    pand xmm1, [rip + top_mask]

    movd eax, xmm1
    ret

_rsqrt_full_nb:
rsqrt_full_nb:
    movd xmm0, edi

    pand xmm0, [rip + top_mask]
    por xmm0, [rip + penultimate_bit]

    vcmpngt_uqss xmm1, xmm0, [rip + min_pos_denorm]
    ptest xmm1, xmm1
    jnz rsqrt_full_bad_new1

    sqrtss xmm0, xmm0

    movd xmm1, [rip + one]
    divss xmm1, xmm0

    paddd xmm1, [rip + ultimate_bit]
    pand xmm1, [rip + top_mask]

    movd eax, xmm1
    ret

rsqrt_full_bad_new1:
    cmp edi, 0x00800000
    jb rsqrt_full_bad_new_fallback1

    movd xmm0, edi
    rsqrtss xmm1, xmm0

    ucomiss xmm1, xmm1
    jp rsqrt_full_bad_new1_nan

    movd eax, xmm1
    ret

rsqrt_full_bad_new_fallback1:
    call _rsqrt_fallback
    ret

rsqrt_full_bad_new1_nan:
    ucomiss xmm0, xmm0
    jp rsqrt_full_bad_new1_nan_ret

    mov eax, 0x7FC00000
    ret

rsqrt_full_bad_new1_nan_ret:
    ret

_rsqrt_full_nb_gpr:
rsqrt_full_nb_gpr:
    movd eax, xmm0 # Emulate regalloc mov

    mov eax, edi
    and eax, 0xFFFF8000
    or eax, 0x00008000

    movd xmm0, eax
    vcmpngt_uqss xmm1, xmm0, [rip + min_pos_denorm]
    ptest xmm1, xmm1
    jnz rsqrt_full_bad_new2

    sqrtss xmm0, xmm0

    movd xmm1, [rip + one]
    divss xmm1, xmm0
    movd eax, xmm1

    add eax, 0x00004000
    and eax, 0xffff8000

    movd xmm0, eax # Emulate regalloc mov
    ret

rsqrt_full_bad_new2:
    cmp edi, 0x00800000
    jb rsqrt_full_bad_new_fallback2

    movd xmm0, edi
    rsqrtss xmm1, xmm0

    test edi, edi
    js rsqrt_full_bad_new2_nan

    movd eax, xmm1
    ret

rsqrt_full_bad_new_fallback2:
    call _rsqrt_fallback
    ret

rsqrt_full_bad_new2_nan:
    mov eax, 0x7FC00000
    ret

rsqrt_full_bad:
    xorps xmm1, xmm1
    movd xmm0, edi
    ucomiss xmm0, xmm1
    jp rsqrt_full_nan
    je rsqrt_full_zero
    jc rsqrt_full_neg

    cmp edi, 0x7F800000
    je rsqrt_full_inf

    # TODO: Full Denormal Implementation
    call _rsqrt_fallback
    ret

rsqrt_full_neg:
    mov eax, 0x7FC00000
    ret

rsqrt_full_inf:
    xor eax, eax
    ret

rsqrt_full_nan:
    mov eax, edi
    or eax, 0x00400000
    ret

rsqrt_full_zero:
    mov eax, edi
    or eax, 0x7F800000
    ret

_rsqrt_newton:
rsqrt_newton:
    movd xmm0, edi

    pand xmm0, [rip + top_mask]
    por xmm0, [rip + penultimate_bit]

    vcmpngt_uqss xmm1, xmm0, [rip + min_pos_denorm]
    ptest xmm1, xmm1
    jnz rsqrt_full_bad

    rsqrtps xmm1, xmm0
    mulss xmm0, [rip + half]
    vmulss xmm2, xmm1, xmm1
    mulss xmm2, xmm0
    movaps xmm0, [rip + one_point_five]
    subss xmm0, xmm2
    mulss xmm0, xmm1

    paddd xmm0, [rip + ultimate_bit]
    pand xmm0, [rip + top_mask]

    movd eax, xmm0
    ret

_rsqrt_hack:
rsqrt_hack:
    movd xmm9, edi

    vpand xmm0, xmm9, [rip + top_mask]
    por xmm0, [rip + penultimate_bit]

    # detect NaNs, negatives, zeros, denormals and infinities
    vcmpngt_uqss xmm1, xmm0, [rip + min_pos_denorm]
    ptest xmm1, xmm1
    jnz rsqrt_full_bad

    # calculate x64 estimate
    rsqrtps xmm0, xmm0

    # calculate correction factor
    vpslld xmm1, xmm9, 8
    vpsrad xmm2, xmm1, 31
    paddd xmm1, [rip + magic1]
    pcmpgtd xmm1, [rip + magic2]
    pxor xmm1, xmm2
    movaps xmm2, [rip + magic3]
    psubd xmm2, xmm1

    # correct x64 estimate
    paddd xmm0, xmm2
    pand xmm0, [rip + top_mask]

    movd eax, xmm0
    ret
