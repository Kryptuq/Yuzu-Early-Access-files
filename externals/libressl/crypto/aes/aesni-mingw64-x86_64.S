#include "x86_arch.h"
.text	
.globl	aesni_encrypt
.def	aesni_encrypt;	.scl 2;	.type 32;	.endef
.p2align	4
aesni_encrypt:
	movups	(%rcx),%xmm2
	movl	240(%r8),%eax
	movups	(%r8),%xmm0
	movups	16(%r8),%xmm1
	leaq	32(%r8),%r8
	xorps	%xmm0,%xmm2
.Loop_enc1_1:
	aesenc	%xmm1,%xmm2
	decl	%eax
	movups	(%r8),%xmm1
	leaq	16(%r8),%r8
	jnz	.Loop_enc1_1	
	aesenclast	%xmm1,%xmm2
	movups	%xmm2,(%rdx)
	retq


.globl	aesni_decrypt
.def	aesni_decrypt;	.scl 2;	.type 32;	.endef
.p2align	4
aesni_decrypt:
	movups	(%rcx),%xmm2
	movl	240(%r8),%eax
	movups	(%r8),%xmm0
	movups	16(%r8),%xmm1
	leaq	32(%r8),%r8
	xorps	%xmm0,%xmm2
.Loop_dec1_2:
	aesdec	%xmm1,%xmm2
	decl	%eax
	movups	(%r8),%xmm1
	leaq	16(%r8),%r8
	jnz	.Loop_dec1_2	
	aesdeclast	%xmm1,%xmm2
	movups	%xmm2,(%rdx)
	retq

.def	_aesni_encrypt3;	.scl 3;	.type 32;	.endef
.p2align	4
_aesni_encrypt3:
	movups	(%rcx),%xmm0
	shrl	$1,%eax
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
	xorps	%xmm0,%xmm3
	xorps	%xmm0,%xmm4
	movups	(%rcx),%xmm0

.Lenc_loop3:
	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	decl	%eax
	aesenc	%xmm1,%xmm4
	movups	16(%rcx),%xmm1
	aesenc	%xmm0,%xmm2
	aesenc	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	aesenc	%xmm0,%xmm4
	movups	(%rcx),%xmm0
	jnz	.Lenc_loop3

	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	aesenc	%xmm1,%xmm4
	aesenclast	%xmm0,%xmm2
	aesenclast	%xmm0,%xmm3
	aesenclast	%xmm0,%xmm4
	retq

.def	_aesni_decrypt3;	.scl 3;	.type 32;	.endef
.p2align	4
_aesni_decrypt3:
	movups	(%rcx),%xmm0
	shrl	$1,%eax
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
	xorps	%xmm0,%xmm3
	xorps	%xmm0,%xmm4
	movups	(%rcx),%xmm0

.Ldec_loop3:
	aesdec	%xmm1,%xmm2
	aesdec	%xmm1,%xmm3
	decl	%eax
	aesdec	%xmm1,%xmm4
	movups	16(%rcx),%xmm1
	aesdec	%xmm0,%xmm2
	aesdec	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	aesdec	%xmm0,%xmm4
	movups	(%rcx),%xmm0
	jnz	.Ldec_loop3

	aesdec	%xmm1,%xmm2
	aesdec	%xmm1,%xmm3
	aesdec	%xmm1,%xmm4
	aesdeclast	%xmm0,%xmm2
	aesdeclast	%xmm0,%xmm3
	aesdeclast	%xmm0,%xmm4
	retq

.def	_aesni_encrypt4;	.scl 3;	.type 32;	.endef
.p2align	4
_aesni_encrypt4:
	movups	(%rcx),%xmm0
	shrl	$1,%eax
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
	xorps	%xmm0,%xmm3
	xorps	%xmm0,%xmm4
	xorps	%xmm0,%xmm5
	movups	(%rcx),%xmm0

.Lenc_loop4:
	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	decl	%eax
	aesenc	%xmm1,%xmm4
	aesenc	%xmm1,%xmm5
	movups	16(%rcx),%xmm1
	aesenc	%xmm0,%xmm2
	aesenc	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	aesenc	%xmm0,%xmm4
	aesenc	%xmm0,%xmm5
	movups	(%rcx),%xmm0
	jnz	.Lenc_loop4

	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	aesenc	%xmm1,%xmm4
	aesenc	%xmm1,%xmm5
	aesenclast	%xmm0,%xmm2
	aesenclast	%xmm0,%xmm3
	aesenclast	%xmm0,%xmm4
	aesenclast	%xmm0,%xmm5
	retq

.def	_aesni_decrypt4;	.scl 3;	.type 32;	.endef
.p2align	4
_aesni_decrypt4:
	movups	(%rcx),%xmm0
	shrl	$1,%eax
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
	xorps	%xmm0,%xmm3
	xorps	%xmm0,%xmm4
	xorps	%xmm0,%xmm5
	movups	(%rcx),%xmm0

.Ldec_loop4:
	aesdec	%xmm1,%xmm2
	aesdec	%xmm1,%xmm3
	decl	%eax
	aesdec	%xmm1,%xmm4
	aesdec	%xmm1,%xmm5
	movups	16(%rcx),%xmm1
	aesdec	%xmm0,%xmm2
	aesdec	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	aesdec	%xmm0,%xmm4
	aesdec	%xmm0,%xmm5
	movups	(%rcx),%xmm0
	jnz	.Ldec_loop4

	aesdec	%xmm1,%xmm2
	aesdec	%xmm1,%xmm3
	aesdec	%xmm1,%xmm4
	aesdec	%xmm1,%xmm5
	aesdeclast	%xmm0,%xmm2
	aesdeclast	%xmm0,%xmm3
	aesdeclast	%xmm0,%xmm4
	aesdeclast	%xmm0,%xmm5
	retq

.def	_aesni_encrypt6;	.scl 3;	.type 32;	.endef
.p2align	4
_aesni_encrypt6:
	movups	(%rcx),%xmm0
	shrl	$1,%eax
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
	pxor	%xmm0,%xmm3
	aesenc	%xmm1,%xmm2
	pxor	%xmm0,%xmm4
	aesenc	%xmm1,%xmm3
	pxor	%xmm0,%xmm5
	aesenc	%xmm1,%xmm4
	pxor	%xmm0,%xmm6
	aesenc	%xmm1,%xmm5
	pxor	%xmm0,%xmm7
	decl	%eax
	aesenc	%xmm1,%xmm6
	movups	(%rcx),%xmm0
	aesenc	%xmm1,%xmm7
	jmp	.Lenc_loop6_enter
.p2align	4
.Lenc_loop6:
	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	decl	%eax
	aesenc	%xmm1,%xmm4
	aesenc	%xmm1,%xmm5
	aesenc	%xmm1,%xmm6
	aesenc	%xmm1,%xmm7
.Lenc_loop6_enter:
	movups	16(%rcx),%xmm1
	aesenc	%xmm0,%xmm2
	aesenc	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	aesenc	%xmm0,%xmm4
	aesenc	%xmm0,%xmm5
	aesenc	%xmm0,%xmm6
	aesenc	%xmm0,%xmm7
	movups	(%rcx),%xmm0
	jnz	.Lenc_loop6

	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	aesenc	%xmm1,%xmm4
	aesenc	%xmm1,%xmm5
	aesenc	%xmm1,%xmm6
	aesenc	%xmm1,%xmm7
	aesenclast	%xmm0,%xmm2
	aesenclast	%xmm0,%xmm3
	aesenclast	%xmm0,%xmm4
	aesenclast	%xmm0,%xmm5
	aesenclast	%xmm0,%xmm6
	aesenclast	%xmm0,%xmm7
	retq

.def	_aesni_decrypt6;	.scl 3;	.type 32;	.endef
.p2align	4
_aesni_decrypt6:
	movups	(%rcx),%xmm0
	shrl	$1,%eax
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
	pxor	%xmm0,%xmm3
	aesdec	%xmm1,%xmm2
	pxor	%xmm0,%xmm4
	aesdec	%xmm1,%xmm3
	pxor	%xmm0,%xmm5
	aesdec	%xmm1,%xmm4
	pxor	%xmm0,%xmm6
	aesdec	%xmm1,%xmm5
	pxor	%xmm0,%xmm7
	decl	%eax
	aesdec	%xmm1,%xmm6
	movups	(%rcx),%xmm0
	aesdec	%xmm1,%xmm7
	jmp	.Ldec_loop6_enter
.p2align	4
.Ldec_loop6:
	aesdec	%xmm1,%xmm2
	aesdec	%xmm1,%xmm3
	decl	%eax
	aesdec	%xmm1,%xmm4
	aesdec	%xmm1,%xmm5
	aesdec	%xmm1,%xmm6
	aesdec	%xmm1,%xmm7
.Ldec_loop6_enter:
	movups	16(%rcx),%xmm1
	aesdec	%xmm0,%xmm2
	aesdec	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	aesdec	%xmm0,%xmm4
	aesdec	%xmm0,%xmm5
	aesdec	%xmm0,%xmm6
	aesdec	%xmm0,%xmm7
	movups	(%rcx),%xmm0
	jnz	.Ldec_loop6

	aesdec	%xmm1,%xmm2
	aesdec	%xmm1,%xmm3
	aesdec	%xmm1,%xmm4
	aesdec	%xmm1,%xmm5
	aesdec	%xmm1,%xmm6
	aesdec	%xmm1,%xmm7
	aesdeclast	%xmm0,%xmm2
	aesdeclast	%xmm0,%xmm3
	aesdeclast	%xmm0,%xmm4
	aesdeclast	%xmm0,%xmm5
	aesdeclast	%xmm0,%xmm6
	aesdeclast	%xmm0,%xmm7
	retq

.def	_aesni_encrypt8;	.scl 3;	.type 32;	.endef
.p2align	4
_aesni_encrypt8:
	movups	(%rcx),%xmm0
	shrl	$1,%eax
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
	xorps	%xmm0,%xmm3
	aesenc	%xmm1,%xmm2
	pxor	%xmm0,%xmm4
	aesenc	%xmm1,%xmm3
	pxor	%xmm0,%xmm5
	aesenc	%xmm1,%xmm4
	pxor	%xmm0,%xmm6
	aesenc	%xmm1,%xmm5
	pxor	%xmm0,%xmm7
	decl	%eax
	aesenc	%xmm1,%xmm6
	pxor	%xmm0,%xmm8
	aesenc	%xmm1,%xmm7
	pxor	%xmm0,%xmm9
	movups	(%rcx),%xmm0
	aesenc	%xmm1,%xmm8
	aesenc	%xmm1,%xmm9
	movups	16(%rcx),%xmm1
	jmp	.Lenc_loop8_enter
.p2align	4
.Lenc_loop8:
	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	decl	%eax
	aesenc	%xmm1,%xmm4
	aesenc	%xmm1,%xmm5
	aesenc	%xmm1,%xmm6
	aesenc	%xmm1,%xmm7
	aesenc	%xmm1,%xmm8
	aesenc	%xmm1,%xmm9
	movups	16(%rcx),%xmm1
.Lenc_loop8_enter:
	aesenc	%xmm0,%xmm2
	aesenc	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	aesenc	%xmm0,%xmm4
	aesenc	%xmm0,%xmm5
	aesenc	%xmm0,%xmm6
	aesenc	%xmm0,%xmm7
	aesenc	%xmm0,%xmm8
	aesenc	%xmm0,%xmm9
	movups	(%rcx),%xmm0
	jnz	.Lenc_loop8

	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	aesenc	%xmm1,%xmm4
	aesenc	%xmm1,%xmm5
	aesenc	%xmm1,%xmm6
	aesenc	%xmm1,%xmm7
	aesenc	%xmm1,%xmm8
	aesenc	%xmm1,%xmm9
	aesenclast	%xmm0,%xmm2
	aesenclast	%xmm0,%xmm3
	aesenclast	%xmm0,%xmm4
	aesenclast	%xmm0,%xmm5
	aesenclast	%xmm0,%xmm6
	aesenclast	%xmm0,%xmm7
	aesenclast	%xmm0,%xmm8
	aesenclast	%xmm0,%xmm9
	retq

.def	_aesni_decrypt8;	.scl 3;	.type 32;	.endef
.p2align	4
_aesni_decrypt8:
	movups	(%rcx),%xmm0
	shrl	$1,%eax
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
	xorps	%xmm0,%xmm3
	aesdec	%xmm1,%xmm2
	pxor	%xmm0,%xmm4
	aesdec	%xmm1,%xmm3
	pxor	%xmm0,%xmm5
	aesdec	%xmm1,%xmm4
	pxor	%xmm0,%xmm6
	aesdec	%xmm1,%xmm5
	pxor	%xmm0,%xmm7
	decl	%eax
	aesdec	%xmm1,%xmm6
	pxor	%xmm0,%xmm8
	aesdec	%xmm1,%xmm7
	pxor	%xmm0,%xmm9
	movups	(%rcx),%xmm0
	aesdec	%xmm1,%xmm8
	aesdec	%xmm1,%xmm9
	movups	16(%rcx),%xmm1
	jmp	.Ldec_loop8_enter
.p2align	4
.Ldec_loop8:
	aesdec	%xmm1,%xmm2
	aesdec	%xmm1,%xmm3
	decl	%eax
	aesdec	%xmm1,%xmm4
	aesdec	%xmm1,%xmm5
	aesdec	%xmm1,%xmm6
	aesdec	%xmm1,%xmm7
	aesdec	%xmm1,%xmm8
	aesdec	%xmm1,%xmm9
	movups	16(%rcx),%xmm1
.Ldec_loop8_enter:
	aesdec	%xmm0,%xmm2
	aesdec	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	aesdec	%xmm0,%xmm4
	aesdec	%xmm0,%xmm5
	aesdec	%xmm0,%xmm6
	aesdec	%xmm0,%xmm7
	aesdec	%xmm0,%xmm8
	aesdec	%xmm0,%xmm9
	movups	(%rcx),%xmm0
	jnz	.Ldec_loop8

	aesdec	%xmm1,%xmm2
	aesdec	%xmm1,%xmm3
	aesdec	%xmm1,%xmm4
	aesdec	%xmm1,%xmm5
	aesdec	%xmm1,%xmm6
	aesdec	%xmm1,%xmm7
	aesdec	%xmm1,%xmm8
	aesdec	%xmm1,%xmm9
	aesdeclast	%xmm0,%xmm2
	aesdeclast	%xmm0,%xmm3
	aesdeclast	%xmm0,%xmm4
	aesdeclast	%xmm0,%xmm5
	aesdeclast	%xmm0,%xmm6
	aesdeclast	%xmm0,%xmm7
	aesdeclast	%xmm0,%xmm8
	aesdeclast	%xmm0,%xmm9
	retq

.globl	aesni_ecb_encrypt
.def	aesni_ecb_encrypt;	.scl 2;	.type 32;	.endef
.p2align	4
aesni_ecb_encrypt:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_aesni_ecb_encrypt:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx
	movq	40(%rsp),%r8

	andq	$-16,%rdx
	jz	.Lecb_ret

	movl	240(%rcx),%eax
	movups	(%rcx),%xmm0
	movq	%rcx,%r11
	movl	%eax,%r10d
	testl	%r8d,%r8d
	jz	.Lecb_decrypt

	cmpq	$128,%rdx
	jb	.Lecb_enc_tail

	movdqu	(%rdi),%xmm2
	movdqu	16(%rdi),%xmm3
	movdqu	32(%rdi),%xmm4
	movdqu	48(%rdi),%xmm5
	movdqu	64(%rdi),%xmm6
	movdqu	80(%rdi),%xmm7
	movdqu	96(%rdi),%xmm8
	movdqu	112(%rdi),%xmm9
	leaq	128(%rdi),%rdi
	subq	$128,%rdx
	jmp	.Lecb_enc_loop8_enter
.p2align	4
.Lecb_enc_loop8:
	movups	%xmm2,(%rsi)
	movq	%r11,%rcx
	movdqu	(%rdi),%xmm2
	movl	%r10d,%eax
	movups	%xmm3,16(%rsi)
	movdqu	16(%rdi),%xmm3
	movups	%xmm4,32(%rsi)
	movdqu	32(%rdi),%xmm4
	movups	%xmm5,48(%rsi)
	movdqu	48(%rdi),%xmm5
	movups	%xmm6,64(%rsi)
	movdqu	64(%rdi),%xmm6
	movups	%xmm7,80(%rsi)
	movdqu	80(%rdi),%xmm7
	movups	%xmm8,96(%rsi)
	movdqu	96(%rdi),%xmm8
	movups	%xmm9,112(%rsi)
	leaq	128(%rsi),%rsi
	movdqu	112(%rdi),%xmm9
	leaq	128(%rdi),%rdi
.Lecb_enc_loop8_enter:

	call	_aesni_encrypt8

	subq	$128,%rdx
	jnc	.Lecb_enc_loop8

	movups	%xmm2,(%rsi)
	movq	%r11,%rcx
	movups	%xmm3,16(%rsi)
	movl	%r10d,%eax
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	movups	%xmm6,64(%rsi)
	movups	%xmm7,80(%rsi)
	movups	%xmm8,96(%rsi)
	movups	%xmm9,112(%rsi)
	leaq	128(%rsi),%rsi
	addq	$128,%rdx
	jz	.Lecb_ret

.Lecb_enc_tail:
	movups	(%rdi),%xmm2
	cmpq	$32,%rdx
	jb	.Lecb_enc_one
	movups	16(%rdi),%xmm3
	je	.Lecb_enc_two
	movups	32(%rdi),%xmm4
	cmpq	$64,%rdx
	jb	.Lecb_enc_three
	movups	48(%rdi),%xmm5
	je	.Lecb_enc_four
	movups	64(%rdi),%xmm6
	cmpq	$96,%rdx
	jb	.Lecb_enc_five
	movups	80(%rdi),%xmm7
	je	.Lecb_enc_six
	movdqu	96(%rdi),%xmm8
	call	_aesni_encrypt8
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	movups	%xmm6,64(%rsi)
	movups	%xmm7,80(%rsi)
	movups	%xmm8,96(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_enc_one:
	movups	(%rcx),%xmm0
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
.Loop_enc1_3:
	aesenc	%xmm1,%xmm2
	decl	%eax
	movups	(%rcx),%xmm1
	leaq	16(%rcx),%rcx
	jnz	.Loop_enc1_3	
	aesenclast	%xmm1,%xmm2
	movups	%xmm2,(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_enc_two:
	xorps	%xmm4,%xmm4
	call	_aesni_encrypt3
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_enc_three:
	call	_aesni_encrypt3
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_enc_four:
	call	_aesni_encrypt4
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_enc_five:
	xorps	%xmm7,%xmm7
	call	_aesni_encrypt6
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	movups	%xmm6,64(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_enc_six:
	call	_aesni_encrypt6
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	movups	%xmm6,64(%rsi)
	movups	%xmm7,80(%rsi)
	jmp	.Lecb_ret

.p2align	4
.Lecb_decrypt:
	cmpq	$128,%rdx
	jb	.Lecb_dec_tail

	movdqu	(%rdi),%xmm2
	movdqu	16(%rdi),%xmm3
	movdqu	32(%rdi),%xmm4
	movdqu	48(%rdi),%xmm5
	movdqu	64(%rdi),%xmm6
	movdqu	80(%rdi),%xmm7
	movdqu	96(%rdi),%xmm8
	movdqu	112(%rdi),%xmm9
	leaq	128(%rdi),%rdi
	subq	$128,%rdx
	jmp	.Lecb_dec_loop8_enter
.p2align	4
.Lecb_dec_loop8:
	movups	%xmm2,(%rsi)
	movq	%r11,%rcx
	movdqu	(%rdi),%xmm2
	movl	%r10d,%eax
	movups	%xmm3,16(%rsi)
	movdqu	16(%rdi),%xmm3
	movups	%xmm4,32(%rsi)
	movdqu	32(%rdi),%xmm4
	movups	%xmm5,48(%rsi)
	movdqu	48(%rdi),%xmm5
	movups	%xmm6,64(%rsi)
	movdqu	64(%rdi),%xmm6
	movups	%xmm7,80(%rsi)
	movdqu	80(%rdi),%xmm7
	movups	%xmm8,96(%rsi)
	movdqu	96(%rdi),%xmm8
	movups	%xmm9,112(%rsi)
	leaq	128(%rsi),%rsi
	movdqu	112(%rdi),%xmm9
	leaq	128(%rdi),%rdi
.Lecb_dec_loop8_enter:

	call	_aesni_decrypt8

	movups	(%r11),%xmm0
	subq	$128,%rdx
	jnc	.Lecb_dec_loop8

	movups	%xmm2,(%rsi)
	movq	%r11,%rcx
	movups	%xmm3,16(%rsi)
	movl	%r10d,%eax
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	movups	%xmm6,64(%rsi)
	movups	%xmm7,80(%rsi)
	movups	%xmm8,96(%rsi)
	movups	%xmm9,112(%rsi)
	leaq	128(%rsi),%rsi
	addq	$128,%rdx
	jz	.Lecb_ret

.Lecb_dec_tail:
	movups	(%rdi),%xmm2
	cmpq	$32,%rdx
	jb	.Lecb_dec_one
	movups	16(%rdi),%xmm3
	je	.Lecb_dec_two
	movups	32(%rdi),%xmm4
	cmpq	$64,%rdx
	jb	.Lecb_dec_three
	movups	48(%rdi),%xmm5
	je	.Lecb_dec_four
	movups	64(%rdi),%xmm6
	cmpq	$96,%rdx
	jb	.Lecb_dec_five
	movups	80(%rdi),%xmm7
	je	.Lecb_dec_six
	movups	96(%rdi),%xmm8
	movups	(%rcx),%xmm0
	call	_aesni_decrypt8
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	movups	%xmm6,64(%rsi)
	movups	%xmm7,80(%rsi)
	movups	%xmm8,96(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_dec_one:
	movups	(%rcx),%xmm0
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
.Loop_dec1_4:
	aesdec	%xmm1,%xmm2
	decl	%eax
	movups	(%rcx),%xmm1
	leaq	16(%rcx),%rcx
	jnz	.Loop_dec1_4	
	aesdeclast	%xmm1,%xmm2
	movups	%xmm2,(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_dec_two:
	xorps	%xmm4,%xmm4
	call	_aesni_decrypt3
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_dec_three:
	call	_aesni_decrypt3
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_dec_four:
	call	_aesni_decrypt4
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_dec_five:
	xorps	%xmm7,%xmm7
	call	_aesni_decrypt6
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	movups	%xmm6,64(%rsi)
	jmp	.Lecb_ret
.p2align	4
.Lecb_dec_six:
	call	_aesni_decrypt6
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	movups	%xmm6,64(%rsi)
	movups	%xmm7,80(%rsi)

.Lecb_ret:
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	retq
.LSEH_end_aesni_ecb_encrypt:
.globl	aesni_ccm64_encrypt_blocks
.def	aesni_ccm64_encrypt_blocks;	.scl 2;	.type 32;	.endef
.p2align	4
aesni_ccm64_encrypt_blocks:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_aesni_ccm64_encrypt_blocks:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx
	movq	40(%rsp),%r8
	movq	48(%rsp),%r9

	leaq	-88(%rsp),%rsp
	movaps	%xmm6,(%rsp)
	movaps	%xmm7,16(%rsp)
	movaps	%xmm8,32(%rsp)
	movaps	%xmm9,48(%rsp)
.Lccm64_enc_body:
	movl	240(%rcx),%eax
	movdqu	(%r8),%xmm9
	movdqa	.Lincrement64(%rip),%xmm6
	movdqa	.Lbswap_mask(%rip),%xmm7

	shrl	$1,%eax
	leaq	0(%rcx),%r11
	movdqu	(%r9),%xmm3
	movdqa	%xmm9,%xmm2
	movl	%eax,%r10d
.byte	102,68,15,56,0,207
	jmp	.Lccm64_enc_outer
.p2align	4
.Lccm64_enc_outer:
	movups	(%r11),%xmm0
	movl	%r10d,%eax
	movups	(%rdi),%xmm8

	xorps	%xmm0,%xmm2
	movups	16(%r11),%xmm1
	xorps	%xmm8,%xmm0
	leaq	32(%r11),%rcx
	xorps	%xmm0,%xmm3
	movups	(%rcx),%xmm0

.Lccm64_enc2_loop:
	aesenc	%xmm1,%xmm2
	decl	%eax
	aesenc	%xmm1,%xmm3
	movups	16(%rcx),%xmm1
	aesenc	%xmm0,%xmm2
	leaq	32(%rcx),%rcx
	aesenc	%xmm0,%xmm3
	movups	0(%rcx),%xmm0
	jnz	.Lccm64_enc2_loop
	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	paddq	%xmm6,%xmm9
	aesenclast	%xmm0,%xmm2
	aesenclast	%xmm0,%xmm3

	decq	%rdx
	leaq	16(%rdi),%rdi
	xorps	%xmm2,%xmm8
	movdqa	%xmm9,%xmm2
	movups	%xmm8,(%rsi)
	leaq	16(%rsi),%rsi
.byte	102,15,56,0,215
	jnz	.Lccm64_enc_outer

	movups	%xmm3,(%r9)
	movaps	(%rsp),%xmm6
	movaps	16(%rsp),%xmm7
	movaps	32(%rsp),%xmm8
	movaps	48(%rsp),%xmm9
	leaq	88(%rsp),%rsp
.Lccm64_enc_ret:
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	retq
.LSEH_end_aesni_ccm64_encrypt_blocks:
.globl	aesni_ccm64_decrypt_blocks
.def	aesni_ccm64_decrypt_blocks;	.scl 2;	.type 32;	.endef
.p2align	4
aesni_ccm64_decrypt_blocks:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_aesni_ccm64_decrypt_blocks:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx
	movq	40(%rsp),%r8
	movq	48(%rsp),%r9

	leaq	-88(%rsp),%rsp
	movaps	%xmm6,(%rsp)
	movaps	%xmm7,16(%rsp)
	movaps	%xmm8,32(%rsp)
	movaps	%xmm9,48(%rsp)
.Lccm64_dec_body:
	movl	240(%rcx),%eax
	movups	(%r8),%xmm9
	movdqu	(%r9),%xmm3
	movdqa	.Lincrement64(%rip),%xmm6
	movdqa	.Lbswap_mask(%rip),%xmm7

	movaps	%xmm9,%xmm2
	movl	%eax,%r10d
	movq	%rcx,%r11
.byte	102,68,15,56,0,207
	movups	(%rcx),%xmm0
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
.Loop_enc1_5:
	aesenc	%xmm1,%xmm2
	decl	%eax
	movups	(%rcx),%xmm1
	leaq	16(%rcx),%rcx
	jnz	.Loop_enc1_5	
	aesenclast	%xmm1,%xmm2
	movups	(%rdi),%xmm8
	paddq	%xmm6,%xmm9
	leaq	16(%rdi),%rdi
	jmp	.Lccm64_dec_outer
.p2align	4
.Lccm64_dec_outer:
	xorps	%xmm2,%xmm8
	movdqa	%xmm9,%xmm2
	movl	%r10d,%eax
	movups	%xmm8,(%rsi)
	leaq	16(%rsi),%rsi
.byte	102,15,56,0,215

	subq	$1,%rdx
	jz	.Lccm64_dec_break

	movups	(%r11),%xmm0
	shrl	$1,%eax
	movups	16(%r11),%xmm1
	xorps	%xmm0,%xmm8
	leaq	32(%r11),%rcx
	xorps	%xmm0,%xmm2
	xorps	%xmm8,%xmm3
	movups	(%rcx),%xmm0

.Lccm64_dec2_loop:
	aesenc	%xmm1,%xmm2
	decl	%eax
	aesenc	%xmm1,%xmm3
	movups	16(%rcx),%xmm1
	aesenc	%xmm0,%xmm2
	leaq	32(%rcx),%rcx
	aesenc	%xmm0,%xmm3
	movups	0(%rcx),%xmm0
	jnz	.Lccm64_dec2_loop
	movups	(%rdi),%xmm8
	paddq	%xmm6,%xmm9
	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	leaq	16(%rdi),%rdi
	aesenclast	%xmm0,%xmm2
	aesenclast	%xmm0,%xmm3
	jmp	.Lccm64_dec_outer

.p2align	4
.Lccm64_dec_break:

	movups	(%r11),%xmm0
	movups	16(%r11),%xmm1
	xorps	%xmm0,%xmm8
	leaq	32(%r11),%r11
	xorps	%xmm8,%xmm3
.Loop_enc1_6:
	aesenc	%xmm1,%xmm3
	decl	%eax
	movups	(%r11),%xmm1
	leaq	16(%r11),%r11
	jnz	.Loop_enc1_6	
	aesenclast	%xmm1,%xmm3
	movups	%xmm3,(%r9)
	movaps	(%rsp),%xmm6
	movaps	16(%rsp),%xmm7
	movaps	32(%rsp),%xmm8
	movaps	48(%rsp),%xmm9
	leaq	88(%rsp),%rsp
.Lccm64_dec_ret:
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	retq
.LSEH_end_aesni_ccm64_decrypt_blocks:
.globl	aesni_ctr32_encrypt_blocks
.def	aesni_ctr32_encrypt_blocks;	.scl 2;	.type 32;	.endef
.p2align	4
aesni_ctr32_encrypt_blocks:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_aesni_ctr32_encrypt_blocks:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx
	movq	40(%rsp),%r8

	leaq	-200(%rsp),%rsp
	movaps	%xmm6,32(%rsp)
	movaps	%xmm7,48(%rsp)
	movaps	%xmm8,64(%rsp)
	movaps	%xmm9,80(%rsp)
	movaps	%xmm10,96(%rsp)
	movaps	%xmm11,112(%rsp)
	movaps	%xmm12,128(%rsp)
	movaps	%xmm13,144(%rsp)
	movaps	%xmm14,160(%rsp)
	movaps	%xmm15,176(%rsp)
.Lctr32_body:
	cmpq	$1,%rdx
	je	.Lctr32_one_shortcut

	movdqu	(%r8),%xmm14
	movdqa	.Lbswap_mask(%rip),%xmm15
	xorl	%eax,%eax
.byte	102,69,15,58,22,242,3
.byte	102,68,15,58,34,240,3

	movl	240(%rcx),%eax
	bswapl	%r10d
	pxor	%xmm12,%xmm12
	pxor	%xmm13,%xmm13
.byte	102,69,15,58,34,226,0
	leaq	3(%r10),%r11
.byte	102,69,15,58,34,235,0
	incl	%r10d
.byte	102,69,15,58,34,226,1
	incq	%r11
.byte	102,69,15,58,34,235,1
	incl	%r10d
.byte	102,69,15,58,34,226,2
	incq	%r11
.byte	102,69,15,58,34,235,2
	movdqa	%xmm12,0(%rsp)
.byte	102,69,15,56,0,231
	movdqa	%xmm13,16(%rsp)
.byte	102,69,15,56,0,239

	pshufd	$192,%xmm12,%xmm2
	pshufd	$128,%xmm12,%xmm3
	pshufd	$64,%xmm12,%xmm4
	cmpq	$6,%rdx
	jb	.Lctr32_tail
	shrl	$1,%eax
	movq	%rcx,%r11
	movl	%eax,%r10d
	subq	$6,%rdx
	jmp	.Lctr32_loop6

.p2align	4
.Lctr32_loop6:
	pshufd	$192,%xmm13,%xmm5
	por	%xmm14,%xmm2
	movups	(%r11),%xmm0
	pshufd	$128,%xmm13,%xmm6
	por	%xmm14,%xmm3
	movups	16(%r11),%xmm1
	pshufd	$64,%xmm13,%xmm7
	por	%xmm14,%xmm4
	por	%xmm14,%xmm5
	xorps	%xmm0,%xmm2
	por	%xmm14,%xmm6
	por	%xmm14,%xmm7




	pxor	%xmm0,%xmm3
	aesenc	%xmm1,%xmm2
	leaq	32(%r11),%rcx
	pxor	%xmm0,%xmm4
	aesenc	%xmm1,%xmm3
	movdqa	.Lincrement32(%rip),%xmm13
	pxor	%xmm0,%xmm5
	aesenc	%xmm1,%xmm4
	movdqa	0(%rsp),%xmm12
	pxor	%xmm0,%xmm6
	aesenc	%xmm1,%xmm5
	pxor	%xmm0,%xmm7
	movups	(%rcx),%xmm0
	decl	%eax
	aesenc	%xmm1,%xmm6
	aesenc	%xmm1,%xmm7
	jmp	.Lctr32_enc_loop6_enter
.p2align	4
.Lctr32_enc_loop6:
	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	decl	%eax
	aesenc	%xmm1,%xmm4
	aesenc	%xmm1,%xmm5
	aesenc	%xmm1,%xmm6
	aesenc	%xmm1,%xmm7
.Lctr32_enc_loop6_enter:
	movups	16(%rcx),%xmm1
	aesenc	%xmm0,%xmm2
	aesenc	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	aesenc	%xmm0,%xmm4
	aesenc	%xmm0,%xmm5
	aesenc	%xmm0,%xmm6
	aesenc	%xmm0,%xmm7
	movups	(%rcx),%xmm0
	jnz	.Lctr32_enc_loop6

	aesenc	%xmm1,%xmm2
	paddd	%xmm13,%xmm12
	aesenc	%xmm1,%xmm3
	paddd	16(%rsp),%xmm13
	aesenc	%xmm1,%xmm4
	movdqa	%xmm12,0(%rsp)
	aesenc	%xmm1,%xmm5
	movdqa	%xmm13,16(%rsp)
	aesenc	%xmm1,%xmm6
.byte	102,69,15,56,0,231
	aesenc	%xmm1,%xmm7
.byte	102,69,15,56,0,239

	aesenclast	%xmm0,%xmm2
	movups	(%rdi),%xmm8
	aesenclast	%xmm0,%xmm3
	movups	16(%rdi),%xmm9
	aesenclast	%xmm0,%xmm4
	movups	32(%rdi),%xmm10
	aesenclast	%xmm0,%xmm5
	movups	48(%rdi),%xmm11
	aesenclast	%xmm0,%xmm6
	movups	64(%rdi),%xmm1
	aesenclast	%xmm0,%xmm7
	movups	80(%rdi),%xmm0
	leaq	96(%rdi),%rdi

	xorps	%xmm2,%xmm8
	pshufd	$192,%xmm12,%xmm2
	xorps	%xmm3,%xmm9
	pshufd	$128,%xmm12,%xmm3
	movups	%xmm8,(%rsi)
	xorps	%xmm4,%xmm10
	pshufd	$64,%xmm12,%xmm4
	movups	%xmm9,16(%rsi)
	xorps	%xmm5,%xmm11
	movups	%xmm10,32(%rsi)
	xorps	%xmm6,%xmm1
	movups	%xmm11,48(%rsi)
	xorps	%xmm7,%xmm0
	movups	%xmm1,64(%rsi)
	movups	%xmm0,80(%rsi)
	leaq	96(%rsi),%rsi
	movl	%r10d,%eax
	subq	$6,%rdx
	jnc	.Lctr32_loop6

	addq	$6,%rdx
	jz	.Lctr32_done
	movq	%r11,%rcx
	leal	1(%rax,%rax,1),%eax

.Lctr32_tail:
	por	%xmm14,%xmm2
	movups	(%rdi),%xmm8
	cmpq	$2,%rdx
	jb	.Lctr32_one

	por	%xmm14,%xmm3
	movups	16(%rdi),%xmm9
	je	.Lctr32_two

	pshufd	$192,%xmm13,%xmm5
	por	%xmm14,%xmm4
	movups	32(%rdi),%xmm10
	cmpq	$4,%rdx
	jb	.Lctr32_three

	pshufd	$128,%xmm13,%xmm6
	por	%xmm14,%xmm5
	movups	48(%rdi),%xmm11
	je	.Lctr32_four

	por	%xmm14,%xmm6
	xorps	%xmm7,%xmm7

	call	_aesni_encrypt6

	movups	64(%rdi),%xmm1
	xorps	%xmm2,%xmm8
	xorps	%xmm3,%xmm9
	movups	%xmm8,(%rsi)
	xorps	%xmm4,%xmm10
	movups	%xmm9,16(%rsi)
	xorps	%xmm5,%xmm11
	movups	%xmm10,32(%rsi)
	xorps	%xmm6,%xmm1
	movups	%xmm11,48(%rsi)
	movups	%xmm1,64(%rsi)
	jmp	.Lctr32_done

.p2align	4
.Lctr32_one_shortcut:
	movups	(%r8),%xmm2
	movups	(%rdi),%xmm8
	movl	240(%rcx),%eax
.Lctr32_one:
	movups	(%rcx),%xmm0
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
.Loop_enc1_7:
	aesenc	%xmm1,%xmm2
	decl	%eax
	movups	(%rcx),%xmm1
	leaq	16(%rcx),%rcx
	jnz	.Loop_enc1_7	
	aesenclast	%xmm1,%xmm2
	xorps	%xmm2,%xmm8
	movups	%xmm8,(%rsi)
	jmp	.Lctr32_done

.p2align	4
.Lctr32_two:
	xorps	%xmm4,%xmm4
	call	_aesni_encrypt3
	xorps	%xmm2,%xmm8
	xorps	%xmm3,%xmm9
	movups	%xmm8,(%rsi)
	movups	%xmm9,16(%rsi)
	jmp	.Lctr32_done

.p2align	4
.Lctr32_three:
	call	_aesni_encrypt3
	xorps	%xmm2,%xmm8
	xorps	%xmm3,%xmm9
	movups	%xmm8,(%rsi)
	xorps	%xmm4,%xmm10
	movups	%xmm9,16(%rsi)
	movups	%xmm10,32(%rsi)
	jmp	.Lctr32_done

.p2align	4
.Lctr32_four:
	call	_aesni_encrypt4
	xorps	%xmm2,%xmm8
	xorps	%xmm3,%xmm9
	movups	%xmm8,(%rsi)
	xorps	%xmm4,%xmm10
	movups	%xmm9,16(%rsi)
	xorps	%xmm5,%xmm11
	movups	%xmm10,32(%rsi)
	movups	%xmm11,48(%rsi)

.Lctr32_done:
	movaps	32(%rsp),%xmm6
	movaps	48(%rsp),%xmm7
	movaps	64(%rsp),%xmm8
	movaps	80(%rsp),%xmm9
	movaps	96(%rsp),%xmm10
	movaps	112(%rsp),%xmm11
	movaps	128(%rsp),%xmm12
	movaps	144(%rsp),%xmm13
	movaps	160(%rsp),%xmm14
	movaps	176(%rsp),%xmm15
	leaq	200(%rsp),%rsp
.Lctr32_ret:
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	retq
.LSEH_end_aesni_ctr32_encrypt_blocks:
.globl	aesni_xts_encrypt
.def	aesni_xts_encrypt;	.scl 2;	.type 32;	.endef
.p2align	4
aesni_xts_encrypt:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_aesni_xts_encrypt:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx
	movq	40(%rsp),%r8
	movq	48(%rsp),%r9

	leaq	-264(%rsp),%rsp
	movaps	%xmm6,96(%rsp)
	movaps	%xmm7,112(%rsp)
	movaps	%xmm8,128(%rsp)
	movaps	%xmm9,144(%rsp)
	movaps	%xmm10,160(%rsp)
	movaps	%xmm11,176(%rsp)
	movaps	%xmm12,192(%rsp)
	movaps	%xmm13,208(%rsp)
	movaps	%xmm14,224(%rsp)
	movaps	%xmm15,240(%rsp)
.Lxts_enc_body:
	movups	(%r9),%xmm15
	movl	240(%r8),%eax
	movl	240(%rcx),%r10d
	movups	(%r8),%xmm0
	movups	16(%r8),%xmm1
	leaq	32(%r8),%r8
	xorps	%xmm0,%xmm15
.Loop_enc1_8:
	aesenc	%xmm1,%xmm15
	decl	%eax
	movups	(%r8),%xmm1
	leaq	16(%r8),%r8
	jnz	.Loop_enc1_8	
	aesenclast	%xmm1,%xmm15
	movq	%rcx,%r11
	movl	%r10d,%eax
	movq	%rdx,%r9
	andq	$-16,%rdx

	movdqa	.Lxts_magic(%rip),%xmm8
	pxor	%xmm14,%xmm14
	pcmpgtd	%xmm15,%xmm14
	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm10
	paddq	%xmm15,%xmm15
	pand	%xmm8,%xmm9
	pcmpgtd	%xmm15,%xmm14
	pxor	%xmm9,%xmm15
	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm11
	paddq	%xmm15,%xmm15
	pand	%xmm8,%xmm9
	pcmpgtd	%xmm15,%xmm14
	pxor	%xmm9,%xmm15
	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm12
	paddq	%xmm15,%xmm15
	pand	%xmm8,%xmm9
	pcmpgtd	%xmm15,%xmm14
	pxor	%xmm9,%xmm15
	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm13
	paddq	%xmm15,%xmm15
	pand	%xmm8,%xmm9
	pcmpgtd	%xmm15,%xmm14
	pxor	%xmm9,%xmm15
	subq	$96,%rdx
	jc	.Lxts_enc_short

	shrl	$1,%eax
	subl	$1,%eax
	movl	%eax,%r10d
	jmp	.Lxts_enc_grandloop

.p2align	4
.Lxts_enc_grandloop:
	pshufd	$19,%xmm14,%xmm9
	movdqa	%xmm15,%xmm14
	paddq	%xmm15,%xmm15
	movdqu	0(%rdi),%xmm2
	pand	%xmm8,%xmm9
	movdqu	16(%rdi),%xmm3
	pxor	%xmm9,%xmm15

	movdqu	32(%rdi),%xmm4
	pxor	%xmm10,%xmm2
	movdqu	48(%rdi),%xmm5
	pxor	%xmm11,%xmm3
	movdqu	64(%rdi),%xmm6
	pxor	%xmm12,%xmm4
	movdqu	80(%rdi),%xmm7
	leaq	96(%rdi),%rdi
	pxor	%xmm13,%xmm5
	movups	(%r11),%xmm0
	pxor	%xmm14,%xmm6
	pxor	%xmm15,%xmm7



	movups	16(%r11),%xmm1
	pxor	%xmm0,%xmm2
	pxor	%xmm0,%xmm3
	movdqa	%xmm10,0(%rsp)
	aesenc	%xmm1,%xmm2
	leaq	32(%r11),%rcx
	pxor	%xmm0,%xmm4
	movdqa	%xmm11,16(%rsp)
	aesenc	%xmm1,%xmm3
	pxor	%xmm0,%xmm5
	movdqa	%xmm12,32(%rsp)
	aesenc	%xmm1,%xmm4
	pxor	%xmm0,%xmm6
	movdqa	%xmm13,48(%rsp)
	aesenc	%xmm1,%xmm5
	pxor	%xmm0,%xmm7
	movups	(%rcx),%xmm0
	decl	%eax
	movdqa	%xmm14,64(%rsp)
	aesenc	%xmm1,%xmm6
	movdqa	%xmm15,80(%rsp)
	aesenc	%xmm1,%xmm7
	pxor	%xmm14,%xmm14
	pcmpgtd	%xmm15,%xmm14
	jmp	.Lxts_enc_loop6_enter

.p2align	4
.Lxts_enc_loop6:
	aesenc	%xmm1,%xmm2
	aesenc	%xmm1,%xmm3
	decl	%eax
	aesenc	%xmm1,%xmm4
	aesenc	%xmm1,%xmm5
	aesenc	%xmm1,%xmm6
	aesenc	%xmm1,%xmm7
.Lxts_enc_loop6_enter:
	movups	16(%rcx),%xmm1
	aesenc	%xmm0,%xmm2
	aesenc	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	aesenc	%xmm0,%xmm4
	aesenc	%xmm0,%xmm5
	aesenc	%xmm0,%xmm6
	aesenc	%xmm0,%xmm7
	movups	(%rcx),%xmm0
	jnz	.Lxts_enc_loop6

	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	paddq	%xmm15,%xmm15
	aesenc	%xmm1,%xmm2
	pand	%xmm8,%xmm9
	aesenc	%xmm1,%xmm3
	pcmpgtd	%xmm15,%xmm14
	aesenc	%xmm1,%xmm4
	pxor	%xmm9,%xmm15
	aesenc	%xmm1,%xmm5
	aesenc	%xmm1,%xmm6
	aesenc	%xmm1,%xmm7
	movups	16(%rcx),%xmm1

	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm10
	paddq	%xmm15,%xmm15
	aesenc	%xmm0,%xmm2
	pand	%xmm8,%xmm9
	aesenc	%xmm0,%xmm3
	pcmpgtd	%xmm15,%xmm14
	aesenc	%xmm0,%xmm4
	pxor	%xmm9,%xmm15
	aesenc	%xmm0,%xmm5
	aesenc	%xmm0,%xmm6
	aesenc	%xmm0,%xmm7
	movups	32(%rcx),%xmm0

	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm11
	paddq	%xmm15,%xmm15
	aesenc	%xmm1,%xmm2
	pand	%xmm8,%xmm9
	aesenc	%xmm1,%xmm3
	pcmpgtd	%xmm15,%xmm14
	aesenc	%xmm1,%xmm4
	pxor	%xmm9,%xmm15
	aesenc	%xmm1,%xmm5
	aesenc	%xmm1,%xmm6
	aesenc	%xmm1,%xmm7

	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm12
	paddq	%xmm15,%xmm15
	aesenclast	%xmm0,%xmm2
	pand	%xmm8,%xmm9
	aesenclast	%xmm0,%xmm3
	pcmpgtd	%xmm15,%xmm14
	aesenclast	%xmm0,%xmm4
	pxor	%xmm9,%xmm15
	aesenclast	%xmm0,%xmm5
	aesenclast	%xmm0,%xmm6
	aesenclast	%xmm0,%xmm7

	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm13
	paddq	%xmm15,%xmm15
	xorps	0(%rsp),%xmm2
	pand	%xmm8,%xmm9
	xorps	16(%rsp),%xmm3
	pcmpgtd	%xmm15,%xmm14
	pxor	%xmm9,%xmm15

	xorps	32(%rsp),%xmm4
	movups	%xmm2,0(%rsi)
	xorps	48(%rsp),%xmm5
	movups	%xmm3,16(%rsi)
	xorps	64(%rsp),%xmm6
	movups	%xmm4,32(%rsi)
	xorps	80(%rsp),%xmm7
	movups	%xmm5,48(%rsi)
	movl	%r10d,%eax
	movups	%xmm6,64(%rsi)
	movups	%xmm7,80(%rsi)
	leaq	96(%rsi),%rsi
	subq	$96,%rdx
	jnc	.Lxts_enc_grandloop

	leal	3(%rax,%rax,1),%eax
	movq	%r11,%rcx
	movl	%eax,%r10d

.Lxts_enc_short:
	addq	$96,%rdx
	jz	.Lxts_enc_done

	cmpq	$32,%rdx
	jb	.Lxts_enc_one
	je	.Lxts_enc_two

	cmpq	$64,%rdx
	jb	.Lxts_enc_three
	je	.Lxts_enc_four

	pshufd	$19,%xmm14,%xmm9
	movdqa	%xmm15,%xmm14
	paddq	%xmm15,%xmm15
	movdqu	(%rdi),%xmm2
	pand	%xmm8,%xmm9
	movdqu	16(%rdi),%xmm3
	pxor	%xmm9,%xmm15

	movdqu	32(%rdi),%xmm4
	pxor	%xmm10,%xmm2
	movdqu	48(%rdi),%xmm5
	pxor	%xmm11,%xmm3
	movdqu	64(%rdi),%xmm6
	leaq	80(%rdi),%rdi
	pxor	%xmm12,%xmm4
	pxor	%xmm13,%xmm5
	pxor	%xmm14,%xmm6

	call	_aesni_encrypt6

	xorps	%xmm10,%xmm2
	movdqa	%xmm15,%xmm10
	xorps	%xmm11,%xmm3
	xorps	%xmm12,%xmm4
	movdqu	%xmm2,(%rsi)
	xorps	%xmm13,%xmm5
	movdqu	%xmm3,16(%rsi)
	xorps	%xmm14,%xmm6
	movdqu	%xmm4,32(%rsi)
	movdqu	%xmm5,48(%rsi)
	movdqu	%xmm6,64(%rsi)
	leaq	80(%rsi),%rsi
	jmp	.Lxts_enc_done

.p2align	4
.Lxts_enc_one:
	movups	(%rdi),%xmm2
	leaq	16(%rdi),%rdi
	xorps	%xmm10,%xmm2
	movups	(%rcx),%xmm0
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
.Loop_enc1_9:
	aesenc	%xmm1,%xmm2
	decl	%eax
	movups	(%rcx),%xmm1
	leaq	16(%rcx),%rcx
	jnz	.Loop_enc1_9	
	aesenclast	%xmm1,%xmm2
	xorps	%xmm10,%xmm2
	movdqa	%xmm11,%xmm10
	movups	%xmm2,(%rsi)
	leaq	16(%rsi),%rsi
	jmp	.Lxts_enc_done

.p2align	4
.Lxts_enc_two:
	movups	(%rdi),%xmm2
	movups	16(%rdi),%xmm3
	leaq	32(%rdi),%rdi
	xorps	%xmm10,%xmm2
	xorps	%xmm11,%xmm3

	call	_aesni_encrypt3

	xorps	%xmm10,%xmm2
	movdqa	%xmm12,%xmm10
	xorps	%xmm11,%xmm3
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	leaq	32(%rsi),%rsi
	jmp	.Lxts_enc_done

.p2align	4
.Lxts_enc_three:
	movups	(%rdi),%xmm2
	movups	16(%rdi),%xmm3
	movups	32(%rdi),%xmm4
	leaq	48(%rdi),%rdi
	xorps	%xmm10,%xmm2
	xorps	%xmm11,%xmm3
	xorps	%xmm12,%xmm4

	call	_aesni_encrypt3

	xorps	%xmm10,%xmm2
	movdqa	%xmm13,%xmm10
	xorps	%xmm11,%xmm3
	xorps	%xmm12,%xmm4
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	leaq	48(%rsi),%rsi
	jmp	.Lxts_enc_done

.p2align	4
.Lxts_enc_four:
	movups	(%rdi),%xmm2
	movups	16(%rdi),%xmm3
	movups	32(%rdi),%xmm4
	xorps	%xmm10,%xmm2
	movups	48(%rdi),%xmm5
	leaq	64(%rdi),%rdi
	xorps	%xmm11,%xmm3
	xorps	%xmm12,%xmm4
	xorps	%xmm13,%xmm5

	call	_aesni_encrypt4

	xorps	%xmm10,%xmm2
	movdqa	%xmm15,%xmm10
	xorps	%xmm11,%xmm3
	xorps	%xmm12,%xmm4
	movups	%xmm2,(%rsi)
	xorps	%xmm13,%xmm5
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	leaq	64(%rsi),%rsi
	jmp	.Lxts_enc_done

.p2align	4
.Lxts_enc_done:
	andq	$15,%r9
	jz	.Lxts_enc_ret
	movq	%r9,%rdx

.Lxts_enc_steal:
	movzbl	(%rdi),%eax
	movzbl	-16(%rsi),%ecx
	leaq	1(%rdi),%rdi
	movb	%al,-16(%rsi)
	movb	%cl,0(%rsi)
	leaq	1(%rsi),%rsi
	subq	$1,%rdx
	jnz	.Lxts_enc_steal

	subq	%r9,%rsi
	movq	%r11,%rcx
	movl	%r10d,%eax

	movups	-16(%rsi),%xmm2
	xorps	%xmm10,%xmm2
	movups	(%rcx),%xmm0
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
.Loop_enc1_10:
	aesenc	%xmm1,%xmm2
	decl	%eax
	movups	(%rcx),%xmm1
	leaq	16(%rcx),%rcx
	jnz	.Loop_enc1_10	
	aesenclast	%xmm1,%xmm2
	xorps	%xmm10,%xmm2
	movups	%xmm2,-16(%rsi)

.Lxts_enc_ret:
	movaps	96(%rsp),%xmm6
	movaps	112(%rsp),%xmm7
	movaps	128(%rsp),%xmm8
	movaps	144(%rsp),%xmm9
	movaps	160(%rsp),%xmm10
	movaps	176(%rsp),%xmm11
	movaps	192(%rsp),%xmm12
	movaps	208(%rsp),%xmm13
	movaps	224(%rsp),%xmm14
	movaps	240(%rsp),%xmm15
	leaq	264(%rsp),%rsp
.Lxts_enc_epilogue:
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	retq
.LSEH_end_aesni_xts_encrypt:
.globl	aesni_xts_decrypt
.def	aesni_xts_decrypt;	.scl 2;	.type 32;	.endef
.p2align	4
aesni_xts_decrypt:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_aesni_xts_decrypt:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx
	movq	40(%rsp),%r8
	movq	48(%rsp),%r9

	leaq	-264(%rsp),%rsp
	movaps	%xmm6,96(%rsp)
	movaps	%xmm7,112(%rsp)
	movaps	%xmm8,128(%rsp)
	movaps	%xmm9,144(%rsp)
	movaps	%xmm10,160(%rsp)
	movaps	%xmm11,176(%rsp)
	movaps	%xmm12,192(%rsp)
	movaps	%xmm13,208(%rsp)
	movaps	%xmm14,224(%rsp)
	movaps	%xmm15,240(%rsp)
.Lxts_dec_body:
	movups	(%r9),%xmm15
	movl	240(%r8),%eax
	movl	240(%rcx),%r10d
	movups	(%r8),%xmm0
	movups	16(%r8),%xmm1
	leaq	32(%r8),%r8
	xorps	%xmm0,%xmm15
.Loop_enc1_11:
	aesenc	%xmm1,%xmm15
	decl	%eax
	movups	(%r8),%xmm1
	leaq	16(%r8),%r8
	jnz	.Loop_enc1_11	
	aesenclast	%xmm1,%xmm15
	xorl	%eax,%eax
	testq	$15,%rdx
	setnz	%al
	shlq	$4,%rax
	subq	%rax,%rdx

	movq	%rcx,%r11
	movl	%r10d,%eax
	movq	%rdx,%r9
	andq	$-16,%rdx

	movdqa	.Lxts_magic(%rip),%xmm8
	pxor	%xmm14,%xmm14
	pcmpgtd	%xmm15,%xmm14
	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm10
	paddq	%xmm15,%xmm15
	pand	%xmm8,%xmm9
	pcmpgtd	%xmm15,%xmm14
	pxor	%xmm9,%xmm15
	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm11
	paddq	%xmm15,%xmm15
	pand	%xmm8,%xmm9
	pcmpgtd	%xmm15,%xmm14
	pxor	%xmm9,%xmm15
	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm12
	paddq	%xmm15,%xmm15
	pand	%xmm8,%xmm9
	pcmpgtd	%xmm15,%xmm14
	pxor	%xmm9,%xmm15
	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm13
	paddq	%xmm15,%xmm15
	pand	%xmm8,%xmm9
	pcmpgtd	%xmm15,%xmm14
	pxor	%xmm9,%xmm15
	subq	$96,%rdx
	jc	.Lxts_dec_short

	shrl	$1,%eax
	subl	$1,%eax
	movl	%eax,%r10d
	jmp	.Lxts_dec_grandloop

.p2align	4
.Lxts_dec_grandloop:
	pshufd	$19,%xmm14,%xmm9
	movdqa	%xmm15,%xmm14
	paddq	%xmm15,%xmm15
	movdqu	0(%rdi),%xmm2
	pand	%xmm8,%xmm9
	movdqu	16(%rdi),%xmm3
	pxor	%xmm9,%xmm15

	movdqu	32(%rdi),%xmm4
	pxor	%xmm10,%xmm2
	movdqu	48(%rdi),%xmm5
	pxor	%xmm11,%xmm3
	movdqu	64(%rdi),%xmm6
	pxor	%xmm12,%xmm4
	movdqu	80(%rdi),%xmm7
	leaq	96(%rdi),%rdi
	pxor	%xmm13,%xmm5
	movups	(%r11),%xmm0
	pxor	%xmm14,%xmm6
	pxor	%xmm15,%xmm7



	movups	16(%r11),%xmm1
	pxor	%xmm0,%xmm2
	pxor	%xmm0,%xmm3
	movdqa	%xmm10,0(%rsp)
	aesdec	%xmm1,%xmm2
	leaq	32(%r11),%rcx
	pxor	%xmm0,%xmm4
	movdqa	%xmm11,16(%rsp)
	aesdec	%xmm1,%xmm3
	pxor	%xmm0,%xmm5
	movdqa	%xmm12,32(%rsp)
	aesdec	%xmm1,%xmm4
	pxor	%xmm0,%xmm6
	movdqa	%xmm13,48(%rsp)
	aesdec	%xmm1,%xmm5
	pxor	%xmm0,%xmm7
	movups	(%rcx),%xmm0
	decl	%eax
	movdqa	%xmm14,64(%rsp)
	aesdec	%xmm1,%xmm6
	movdqa	%xmm15,80(%rsp)
	aesdec	%xmm1,%xmm7
	pxor	%xmm14,%xmm14
	pcmpgtd	%xmm15,%xmm14
	jmp	.Lxts_dec_loop6_enter

.p2align	4
.Lxts_dec_loop6:
	aesdec	%xmm1,%xmm2
	aesdec	%xmm1,%xmm3
	decl	%eax
	aesdec	%xmm1,%xmm4
	aesdec	%xmm1,%xmm5
	aesdec	%xmm1,%xmm6
	aesdec	%xmm1,%xmm7
.Lxts_dec_loop6_enter:
	movups	16(%rcx),%xmm1
	aesdec	%xmm0,%xmm2
	aesdec	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	aesdec	%xmm0,%xmm4
	aesdec	%xmm0,%xmm5
	aesdec	%xmm0,%xmm6
	aesdec	%xmm0,%xmm7
	movups	(%rcx),%xmm0
	jnz	.Lxts_dec_loop6

	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	paddq	%xmm15,%xmm15
	aesdec	%xmm1,%xmm2
	pand	%xmm8,%xmm9
	aesdec	%xmm1,%xmm3
	pcmpgtd	%xmm15,%xmm14
	aesdec	%xmm1,%xmm4
	pxor	%xmm9,%xmm15
	aesdec	%xmm1,%xmm5
	aesdec	%xmm1,%xmm6
	aesdec	%xmm1,%xmm7
	movups	16(%rcx),%xmm1

	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm10
	paddq	%xmm15,%xmm15
	aesdec	%xmm0,%xmm2
	pand	%xmm8,%xmm9
	aesdec	%xmm0,%xmm3
	pcmpgtd	%xmm15,%xmm14
	aesdec	%xmm0,%xmm4
	pxor	%xmm9,%xmm15
	aesdec	%xmm0,%xmm5
	aesdec	%xmm0,%xmm6
	aesdec	%xmm0,%xmm7
	movups	32(%rcx),%xmm0

	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm11
	paddq	%xmm15,%xmm15
	aesdec	%xmm1,%xmm2
	pand	%xmm8,%xmm9
	aesdec	%xmm1,%xmm3
	pcmpgtd	%xmm15,%xmm14
	aesdec	%xmm1,%xmm4
	pxor	%xmm9,%xmm15
	aesdec	%xmm1,%xmm5
	aesdec	%xmm1,%xmm6
	aesdec	%xmm1,%xmm7

	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm12
	paddq	%xmm15,%xmm15
	aesdeclast	%xmm0,%xmm2
	pand	%xmm8,%xmm9
	aesdeclast	%xmm0,%xmm3
	pcmpgtd	%xmm15,%xmm14
	aesdeclast	%xmm0,%xmm4
	pxor	%xmm9,%xmm15
	aesdeclast	%xmm0,%xmm5
	aesdeclast	%xmm0,%xmm6
	aesdeclast	%xmm0,%xmm7

	pshufd	$19,%xmm14,%xmm9
	pxor	%xmm14,%xmm14
	movdqa	%xmm15,%xmm13
	paddq	%xmm15,%xmm15
	xorps	0(%rsp),%xmm2
	pand	%xmm8,%xmm9
	xorps	16(%rsp),%xmm3
	pcmpgtd	%xmm15,%xmm14
	pxor	%xmm9,%xmm15

	xorps	32(%rsp),%xmm4
	movups	%xmm2,0(%rsi)
	xorps	48(%rsp),%xmm5
	movups	%xmm3,16(%rsi)
	xorps	64(%rsp),%xmm6
	movups	%xmm4,32(%rsi)
	xorps	80(%rsp),%xmm7
	movups	%xmm5,48(%rsi)
	movl	%r10d,%eax
	movups	%xmm6,64(%rsi)
	movups	%xmm7,80(%rsi)
	leaq	96(%rsi),%rsi
	subq	$96,%rdx
	jnc	.Lxts_dec_grandloop

	leal	3(%rax,%rax,1),%eax
	movq	%r11,%rcx
	movl	%eax,%r10d

.Lxts_dec_short:
	addq	$96,%rdx
	jz	.Lxts_dec_done

	cmpq	$32,%rdx
	jb	.Lxts_dec_one
	je	.Lxts_dec_two

	cmpq	$64,%rdx
	jb	.Lxts_dec_three
	je	.Lxts_dec_four

	pshufd	$19,%xmm14,%xmm9
	movdqa	%xmm15,%xmm14
	paddq	%xmm15,%xmm15
	movdqu	(%rdi),%xmm2
	pand	%xmm8,%xmm9
	movdqu	16(%rdi),%xmm3
	pxor	%xmm9,%xmm15

	movdqu	32(%rdi),%xmm4
	pxor	%xmm10,%xmm2
	movdqu	48(%rdi),%xmm5
	pxor	%xmm11,%xmm3
	movdqu	64(%rdi),%xmm6
	leaq	80(%rdi),%rdi
	pxor	%xmm12,%xmm4
	pxor	%xmm13,%xmm5
	pxor	%xmm14,%xmm6

	call	_aesni_decrypt6

	xorps	%xmm10,%xmm2
	xorps	%xmm11,%xmm3
	xorps	%xmm12,%xmm4
	movdqu	%xmm2,(%rsi)
	xorps	%xmm13,%xmm5
	movdqu	%xmm3,16(%rsi)
	xorps	%xmm14,%xmm6
	movdqu	%xmm4,32(%rsi)
	pxor	%xmm14,%xmm14
	movdqu	%xmm5,48(%rsi)
	pcmpgtd	%xmm15,%xmm14
	movdqu	%xmm6,64(%rsi)
	leaq	80(%rsi),%rsi
	pshufd	$19,%xmm14,%xmm11
	andq	$15,%r9
	jz	.Lxts_dec_ret

	movdqa	%xmm15,%xmm10
	paddq	%xmm15,%xmm15
	pand	%xmm8,%xmm11
	pxor	%xmm15,%xmm11
	jmp	.Lxts_dec_done2

.p2align	4
.Lxts_dec_one:
	movups	(%rdi),%xmm2
	leaq	16(%rdi),%rdi
	xorps	%xmm10,%xmm2
	movups	(%rcx),%xmm0
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
.Loop_dec1_12:
	aesdec	%xmm1,%xmm2
	decl	%eax
	movups	(%rcx),%xmm1
	leaq	16(%rcx),%rcx
	jnz	.Loop_dec1_12	
	aesdeclast	%xmm1,%xmm2
	xorps	%xmm10,%xmm2
	movdqa	%xmm11,%xmm10
	movups	%xmm2,(%rsi)
	movdqa	%xmm12,%xmm11
	leaq	16(%rsi),%rsi
	jmp	.Lxts_dec_done

.p2align	4
.Lxts_dec_two:
	movups	(%rdi),%xmm2
	movups	16(%rdi),%xmm3
	leaq	32(%rdi),%rdi
	xorps	%xmm10,%xmm2
	xorps	%xmm11,%xmm3

	call	_aesni_decrypt3

	xorps	%xmm10,%xmm2
	movdqa	%xmm12,%xmm10
	xorps	%xmm11,%xmm3
	movdqa	%xmm13,%xmm11
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	leaq	32(%rsi),%rsi
	jmp	.Lxts_dec_done

.p2align	4
.Lxts_dec_three:
	movups	(%rdi),%xmm2
	movups	16(%rdi),%xmm3
	movups	32(%rdi),%xmm4
	leaq	48(%rdi),%rdi
	xorps	%xmm10,%xmm2
	xorps	%xmm11,%xmm3
	xorps	%xmm12,%xmm4

	call	_aesni_decrypt3

	xorps	%xmm10,%xmm2
	movdqa	%xmm13,%xmm10
	xorps	%xmm11,%xmm3
	movdqa	%xmm15,%xmm11
	xorps	%xmm12,%xmm4
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	leaq	48(%rsi),%rsi
	jmp	.Lxts_dec_done

.p2align	4
.Lxts_dec_four:
	pshufd	$19,%xmm14,%xmm9
	movdqa	%xmm15,%xmm14
	paddq	%xmm15,%xmm15
	movups	(%rdi),%xmm2
	pand	%xmm8,%xmm9
	movups	16(%rdi),%xmm3
	pxor	%xmm9,%xmm15

	movups	32(%rdi),%xmm4
	xorps	%xmm10,%xmm2
	movups	48(%rdi),%xmm5
	leaq	64(%rdi),%rdi
	xorps	%xmm11,%xmm3
	xorps	%xmm12,%xmm4
	xorps	%xmm13,%xmm5

	call	_aesni_decrypt4

	xorps	%xmm10,%xmm2
	movdqa	%xmm14,%xmm10
	xorps	%xmm11,%xmm3
	movdqa	%xmm15,%xmm11
	xorps	%xmm12,%xmm4
	movups	%xmm2,(%rsi)
	xorps	%xmm13,%xmm5
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	leaq	64(%rsi),%rsi
	jmp	.Lxts_dec_done

.p2align	4
.Lxts_dec_done:
	andq	$15,%r9
	jz	.Lxts_dec_ret
.Lxts_dec_done2:
	movq	%r9,%rdx
	movq	%r11,%rcx
	movl	%r10d,%eax

	movups	(%rdi),%xmm2
	xorps	%xmm11,%xmm2
	movups	(%rcx),%xmm0
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
.Loop_dec1_13:
	aesdec	%xmm1,%xmm2
	decl	%eax
	movups	(%rcx),%xmm1
	leaq	16(%rcx),%rcx
	jnz	.Loop_dec1_13	
	aesdeclast	%xmm1,%xmm2
	xorps	%xmm11,%xmm2
	movups	%xmm2,(%rsi)

.Lxts_dec_steal:
	movzbl	16(%rdi),%eax
	movzbl	(%rsi),%ecx
	leaq	1(%rdi),%rdi
	movb	%al,(%rsi)
	movb	%cl,16(%rsi)
	leaq	1(%rsi),%rsi
	subq	$1,%rdx
	jnz	.Lxts_dec_steal

	subq	%r9,%rsi
	movq	%r11,%rcx
	movl	%r10d,%eax

	movups	(%rsi),%xmm2
	xorps	%xmm10,%xmm2
	movups	(%rcx),%xmm0
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
.Loop_dec1_14:
	aesdec	%xmm1,%xmm2
	decl	%eax
	movups	(%rcx),%xmm1
	leaq	16(%rcx),%rcx
	jnz	.Loop_dec1_14	
	aesdeclast	%xmm1,%xmm2
	xorps	%xmm10,%xmm2
	movups	%xmm2,(%rsi)

.Lxts_dec_ret:
	movaps	96(%rsp),%xmm6
	movaps	112(%rsp),%xmm7
	movaps	128(%rsp),%xmm8
	movaps	144(%rsp),%xmm9
	movaps	160(%rsp),%xmm10
	movaps	176(%rsp),%xmm11
	movaps	192(%rsp),%xmm12
	movaps	208(%rsp),%xmm13
	movaps	224(%rsp),%xmm14
	movaps	240(%rsp),%xmm15
	leaq	264(%rsp),%rsp
.Lxts_dec_epilogue:
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	retq
.LSEH_end_aesni_xts_decrypt:
.globl	aesni_cbc_encrypt
.def	aesni_cbc_encrypt;	.scl 2;	.type 32;	.endef
.p2align	4
aesni_cbc_encrypt:
	movq	%rdi,8(%rsp)
	movq	%rsi,16(%rsp)
	movq	%rsp,%rax
.LSEH_begin_aesni_cbc_encrypt:
	movq	%rcx,%rdi
	movq	%rdx,%rsi
	movq	%r8,%rdx
	movq	%r9,%rcx
	movq	40(%rsp),%r8
	movq	48(%rsp),%r9

	testq	%rdx,%rdx
	jz	.Lcbc_ret

	movl	240(%rcx),%r10d
	movq	%rcx,%r11
	testl	%r9d,%r9d
	jz	.Lcbc_decrypt

	movups	(%r8),%xmm2
	movl	%r10d,%eax
	cmpq	$16,%rdx
	jb	.Lcbc_enc_tail
	subq	$16,%rdx
	jmp	.Lcbc_enc_loop
.p2align	4
.Lcbc_enc_loop:
	movups	(%rdi),%xmm3
	leaq	16(%rdi),%rdi

	movups	(%rcx),%xmm0
	movups	16(%rcx),%xmm1
	xorps	%xmm0,%xmm3
	leaq	32(%rcx),%rcx
	xorps	%xmm3,%xmm2
.Loop_enc1_15:
	aesenc	%xmm1,%xmm2
	decl	%eax
	movups	(%rcx),%xmm1
	leaq	16(%rcx),%rcx
	jnz	.Loop_enc1_15	
	aesenclast	%xmm1,%xmm2
	movl	%r10d,%eax
	movq	%r11,%rcx
	movups	%xmm2,0(%rsi)
	leaq	16(%rsi),%rsi
	subq	$16,%rdx
	jnc	.Lcbc_enc_loop
	addq	$16,%rdx
	jnz	.Lcbc_enc_tail
	movups	%xmm2,(%r8)
	jmp	.Lcbc_ret

.Lcbc_enc_tail:
	movq	%rdx,%rcx
	xchgq	%rdi,%rsi
.long	0x9066A4F3	
	movl	$16,%ecx
	subq	%rdx,%rcx
	xorl	%eax,%eax
.long	0x9066AAF3	
	leaq	-16(%rdi),%rdi
	movl	%r10d,%eax
	movq	%rdi,%rsi
	movq	%r11,%rcx
	xorq	%rdx,%rdx
	jmp	.Lcbc_enc_loop	

.p2align	4
.Lcbc_decrypt:
	leaq	-88(%rsp),%rsp
	movaps	%xmm6,(%rsp)
	movaps	%xmm7,16(%rsp)
	movaps	%xmm8,32(%rsp)
	movaps	%xmm9,48(%rsp)
.Lcbc_decrypt_body:
	movups	(%r8),%xmm9
	movl	%r10d,%eax
	cmpq	$112,%rdx
	jbe	.Lcbc_dec_tail
	shrl	$1,%r10d
	subq	$112,%rdx
	movl	%r10d,%eax
	movaps	%xmm9,64(%rsp)
	jmp	.Lcbc_dec_loop8_enter
.p2align	4
.Lcbc_dec_loop8:
	movaps	%xmm0,64(%rsp)
	movups	%xmm9,(%rsi)
	leaq	16(%rsi),%rsi
.Lcbc_dec_loop8_enter:
	movups	(%rcx),%xmm0
	movups	(%rdi),%xmm2
	movups	16(%rdi),%xmm3
	movups	16(%rcx),%xmm1

	leaq	32(%rcx),%rcx
	movdqu	32(%rdi),%xmm4
	xorps	%xmm0,%xmm2
	movdqu	48(%rdi),%xmm5
	xorps	%xmm0,%xmm3
	movdqu	64(%rdi),%xmm6
	aesdec	%xmm1,%xmm2
	pxor	%xmm0,%xmm4
	movdqu	80(%rdi),%xmm7
	aesdec	%xmm1,%xmm3
	pxor	%xmm0,%xmm5
	movdqu	96(%rdi),%xmm8
	aesdec	%xmm1,%xmm4
	pxor	%xmm0,%xmm6
	movdqu	112(%rdi),%xmm9
	aesdec	%xmm1,%xmm5
	pxor	%xmm0,%xmm7
	decl	%eax
	aesdec	%xmm1,%xmm6
	pxor	%xmm0,%xmm8
	aesdec	%xmm1,%xmm7
	pxor	%xmm0,%xmm9
	movups	(%rcx),%xmm0
	aesdec	%xmm1,%xmm8
	aesdec	%xmm1,%xmm9
	movups	16(%rcx),%xmm1

	call	.Ldec_loop8_enter

	movups	(%rdi),%xmm1
	movups	16(%rdi),%xmm0
	xorps	64(%rsp),%xmm2
	xorps	%xmm1,%xmm3
	movups	32(%rdi),%xmm1
	xorps	%xmm0,%xmm4
	movups	48(%rdi),%xmm0
	xorps	%xmm1,%xmm5
	movups	64(%rdi),%xmm1
	xorps	%xmm0,%xmm6
	movups	80(%rdi),%xmm0
	xorps	%xmm1,%xmm7
	movups	96(%rdi),%xmm1
	xorps	%xmm0,%xmm8
	movups	112(%rdi),%xmm0
	xorps	%xmm1,%xmm9
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	movl	%r10d,%eax
	movups	%xmm6,64(%rsi)
	movq	%r11,%rcx
	movups	%xmm7,80(%rsi)
	leaq	128(%rdi),%rdi
	movups	%xmm8,96(%rsi)
	leaq	112(%rsi),%rsi
	subq	$128,%rdx
	ja	.Lcbc_dec_loop8

	movaps	%xmm9,%xmm2
	movaps	%xmm0,%xmm9
	addq	$112,%rdx
	jle	.Lcbc_dec_tail_collected
	movups	%xmm2,(%rsi)
	leal	1(%r10,%r10,1),%eax
	leaq	16(%rsi),%rsi
.Lcbc_dec_tail:
	movups	(%rdi),%xmm2
	movaps	%xmm2,%xmm8
	cmpq	$16,%rdx
	jbe	.Lcbc_dec_one

	movups	16(%rdi),%xmm3
	movaps	%xmm3,%xmm7
	cmpq	$32,%rdx
	jbe	.Lcbc_dec_two

	movups	32(%rdi),%xmm4
	movaps	%xmm4,%xmm6
	cmpq	$48,%rdx
	jbe	.Lcbc_dec_three

	movups	48(%rdi),%xmm5
	cmpq	$64,%rdx
	jbe	.Lcbc_dec_four

	movups	64(%rdi),%xmm6
	cmpq	$80,%rdx
	jbe	.Lcbc_dec_five

	movups	80(%rdi),%xmm7
	cmpq	$96,%rdx
	jbe	.Lcbc_dec_six

	movups	96(%rdi),%xmm8
	movaps	%xmm9,64(%rsp)
	call	_aesni_decrypt8
	movups	(%rdi),%xmm1
	movups	16(%rdi),%xmm0
	xorps	64(%rsp),%xmm2
	xorps	%xmm1,%xmm3
	movups	32(%rdi),%xmm1
	xorps	%xmm0,%xmm4
	movups	48(%rdi),%xmm0
	xorps	%xmm1,%xmm5
	movups	64(%rdi),%xmm1
	xorps	%xmm0,%xmm6
	movups	80(%rdi),%xmm0
	xorps	%xmm1,%xmm7
	movups	96(%rdi),%xmm9
	xorps	%xmm0,%xmm8
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	movups	%xmm6,64(%rsi)
	movups	%xmm7,80(%rsi)
	leaq	96(%rsi),%rsi
	movaps	%xmm8,%xmm2
	subq	$112,%rdx
	jmp	.Lcbc_dec_tail_collected
.p2align	4
.Lcbc_dec_one:
	movups	(%rcx),%xmm0
	movups	16(%rcx),%xmm1
	leaq	32(%rcx),%rcx
	xorps	%xmm0,%xmm2
.Loop_dec1_16:
	aesdec	%xmm1,%xmm2
	decl	%eax
	movups	(%rcx),%xmm1
	leaq	16(%rcx),%rcx
	jnz	.Loop_dec1_16	
	aesdeclast	%xmm1,%xmm2
	xorps	%xmm9,%xmm2
	movaps	%xmm8,%xmm9
	subq	$16,%rdx
	jmp	.Lcbc_dec_tail_collected
.p2align	4
.Lcbc_dec_two:
	xorps	%xmm4,%xmm4
	call	_aesni_decrypt3
	xorps	%xmm9,%xmm2
	xorps	%xmm8,%xmm3
	movups	%xmm2,(%rsi)
	movaps	%xmm7,%xmm9
	movaps	%xmm3,%xmm2
	leaq	16(%rsi),%rsi
	subq	$32,%rdx
	jmp	.Lcbc_dec_tail_collected
.p2align	4
.Lcbc_dec_three:
	call	_aesni_decrypt3
	xorps	%xmm9,%xmm2
	xorps	%xmm8,%xmm3
	movups	%xmm2,(%rsi)
	xorps	%xmm7,%xmm4
	movups	%xmm3,16(%rsi)
	movaps	%xmm6,%xmm9
	movaps	%xmm4,%xmm2
	leaq	32(%rsi),%rsi
	subq	$48,%rdx
	jmp	.Lcbc_dec_tail_collected
.p2align	4
.Lcbc_dec_four:
	call	_aesni_decrypt4
	xorps	%xmm9,%xmm2
	movups	48(%rdi),%xmm9
	xorps	%xmm8,%xmm3
	movups	%xmm2,(%rsi)
	xorps	%xmm7,%xmm4
	movups	%xmm3,16(%rsi)
	xorps	%xmm6,%xmm5
	movups	%xmm4,32(%rsi)
	movaps	%xmm5,%xmm2
	leaq	48(%rsi),%rsi
	subq	$64,%rdx
	jmp	.Lcbc_dec_tail_collected
.p2align	4
.Lcbc_dec_five:
	xorps	%xmm7,%xmm7
	call	_aesni_decrypt6
	movups	16(%rdi),%xmm1
	movups	32(%rdi),%xmm0
	xorps	%xmm9,%xmm2
	xorps	%xmm8,%xmm3
	xorps	%xmm1,%xmm4
	movups	48(%rdi),%xmm1
	xorps	%xmm0,%xmm5
	movups	64(%rdi),%xmm9
	xorps	%xmm1,%xmm6
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	leaq	64(%rsi),%rsi
	movaps	%xmm6,%xmm2
	subq	$80,%rdx
	jmp	.Lcbc_dec_tail_collected
.p2align	4
.Lcbc_dec_six:
	call	_aesni_decrypt6
	movups	16(%rdi),%xmm1
	movups	32(%rdi),%xmm0
	xorps	%xmm9,%xmm2
	xorps	%xmm8,%xmm3
	xorps	%xmm1,%xmm4
	movups	48(%rdi),%xmm1
	xorps	%xmm0,%xmm5
	movups	64(%rdi),%xmm0
	xorps	%xmm1,%xmm6
	movups	80(%rdi),%xmm9
	xorps	%xmm0,%xmm7
	movups	%xmm2,(%rsi)
	movups	%xmm3,16(%rsi)
	movups	%xmm4,32(%rsi)
	movups	%xmm5,48(%rsi)
	movups	%xmm6,64(%rsi)
	leaq	80(%rsi),%rsi
	movaps	%xmm7,%xmm2
	subq	$96,%rdx
	jmp	.Lcbc_dec_tail_collected
.p2align	4
.Lcbc_dec_tail_collected:
	andq	$15,%rdx
	movups	%xmm9,(%r8)
	jnz	.Lcbc_dec_tail_partial
	movups	%xmm2,(%rsi)
	jmp	.Lcbc_dec_ret
.p2align	4
.Lcbc_dec_tail_partial:
	movaps	%xmm2,64(%rsp)
	movq	$16,%rcx
	movq	%rsi,%rdi
	subq	%rdx,%rcx
	leaq	64(%rsp),%rsi
.long	0x9066A4F3	

.Lcbc_dec_ret:
	movaps	(%rsp),%xmm6
	movaps	16(%rsp),%xmm7
	movaps	32(%rsp),%xmm8
	movaps	48(%rsp),%xmm9
	leaq	88(%rsp),%rsp
.Lcbc_ret:
	movq	8(%rsp),%rdi
	movq	16(%rsp),%rsi
	retq
.LSEH_end_aesni_cbc_encrypt:
.globl	aesni_set_decrypt_key
.def	aesni_set_decrypt_key;	.scl 2;	.type 32;	.endef
.p2align	4
aesni_set_decrypt_key:
	subq	$8,%rsp
	call	__aesni_set_encrypt_key
	shll	$4,%edx
	testl	%eax,%eax
	jnz	.Ldec_key_ret
	leaq	16(%r8,%rdx,1),%rcx

	movups	(%r8),%xmm0
	movups	(%rcx),%xmm1
	movups	%xmm0,(%rcx)
	movups	%xmm1,(%r8)
	leaq	16(%r8),%r8
	leaq	-16(%rcx),%rcx

.Ldec_key_inverse:
	movups	(%r8),%xmm0
	movups	(%rcx),%xmm1
	aesimc	%xmm0,%xmm0
	aesimc	%xmm1,%xmm1
	leaq	16(%r8),%r8
	leaq	-16(%rcx),%rcx
	movups	%xmm0,16(%rcx)
	movups	%xmm1,-16(%r8)
	cmpq	%r8,%rcx
	ja	.Ldec_key_inverse

	movups	(%r8),%xmm0
	aesimc	%xmm0,%xmm0
	movups	%xmm0,(%rcx)
.Ldec_key_ret:
	addq	$8,%rsp
	retq
.LSEH_end_set_decrypt_key:

.globl	aesni_set_encrypt_key
.def	aesni_set_encrypt_key;	.scl 2;	.type 32;	.endef
.p2align	4
aesni_set_encrypt_key:
__aesni_set_encrypt_key:
	subq	$8,%rsp
	movq	$-1,%rax
	testq	%rcx,%rcx
	jz	.Lenc_key_ret
	testq	%r8,%r8
	jz	.Lenc_key_ret

	movups	(%rcx),%xmm0
	xorps	%xmm4,%xmm4
	leaq	16(%r8),%rax
	cmpl	$256,%edx
	je	.L14rounds
	cmpl	$192,%edx
	je	.L12rounds
	cmpl	$128,%edx
	jne	.Lbad_keybits

.L10rounds:
	movl	$9,%edx
	movups	%xmm0,(%r8)
	aeskeygenassist	$1,%xmm0,%xmm1
	call	.Lkey_expansion_128_cold
	aeskeygenassist	$2,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$4,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$8,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$16,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$32,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$64,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$128,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$27,%xmm0,%xmm1
	call	.Lkey_expansion_128
	aeskeygenassist	$54,%xmm0,%xmm1
	call	.Lkey_expansion_128
	movups	%xmm0,(%rax)
	movl	%edx,80(%rax)
	xorl	%eax,%eax
	jmp	.Lenc_key_ret

.p2align	4
.L12rounds:
	movq	16(%rcx),%xmm2
	movl	$11,%edx
	movups	%xmm0,(%r8)
	aeskeygenassist	$1,%xmm2,%xmm1
	call	.Lkey_expansion_192a_cold
	aeskeygenassist	$2,%xmm2,%xmm1
	call	.Lkey_expansion_192b
	aeskeygenassist	$4,%xmm2,%xmm1
	call	.Lkey_expansion_192a
	aeskeygenassist	$8,%xmm2,%xmm1
	call	.Lkey_expansion_192b
	aeskeygenassist	$16,%xmm2,%xmm1
	call	.Lkey_expansion_192a
	aeskeygenassist	$32,%xmm2,%xmm1
	call	.Lkey_expansion_192b
	aeskeygenassist	$64,%xmm2,%xmm1
	call	.Lkey_expansion_192a
	aeskeygenassist	$128,%xmm2,%xmm1
	call	.Lkey_expansion_192b
	movups	%xmm0,(%rax)
	movl	%edx,48(%rax)
	xorq	%rax,%rax
	jmp	.Lenc_key_ret

.p2align	4
.L14rounds:
	movups	16(%rcx),%xmm2
	movl	$13,%edx
	leaq	16(%rax),%rax
	movups	%xmm0,(%r8)
	movups	%xmm2,16(%r8)
	aeskeygenassist	$1,%xmm2,%xmm1
	call	.Lkey_expansion_256a_cold
	aeskeygenassist	$1,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$2,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	aeskeygenassist	$2,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$4,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	aeskeygenassist	$4,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$8,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	aeskeygenassist	$8,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$16,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	aeskeygenassist	$16,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$32,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	aeskeygenassist	$32,%xmm0,%xmm1
	call	.Lkey_expansion_256b
	aeskeygenassist	$64,%xmm2,%xmm1
	call	.Lkey_expansion_256a
	movups	%xmm0,(%rax)
	movl	%edx,16(%rax)
	xorq	%rax,%rax
	jmp	.Lenc_key_ret

.p2align	4
.Lbad_keybits:
	movq	$-2,%rax
.Lenc_key_ret:
	addq	$8,%rsp
	retq
.LSEH_end_set_encrypt_key:

.p2align	4
.Lkey_expansion_128:
	movups	%xmm0,(%rax)
	leaq	16(%rax),%rax
.Lkey_expansion_128_cold:
	shufps	$16,%xmm0,%xmm4
	xorps	%xmm4,%xmm0
	shufps	$140,%xmm0,%xmm4
	xorps	%xmm4,%xmm0
	shufps	$255,%xmm1,%xmm1
	xorps	%xmm1,%xmm0
	retq

.p2align	4
.Lkey_expansion_192a:
	movups	%xmm0,(%rax)
	leaq	16(%rax),%rax
.Lkey_expansion_192a_cold:
	movaps	%xmm2,%xmm5
.Lkey_expansion_192b_warm:
	shufps	$16,%xmm0,%xmm4
	movdqa	%xmm2,%xmm3
	xorps	%xmm4,%xmm0
	shufps	$140,%xmm0,%xmm4
	pslldq	$4,%xmm3
	xorps	%xmm4,%xmm0
	pshufd	$85,%xmm1,%xmm1
	pxor	%xmm3,%xmm2
	pxor	%xmm1,%xmm0
	pshufd	$255,%xmm0,%xmm3
	pxor	%xmm3,%xmm2
	retq

.p2align	4
.Lkey_expansion_192b:
	movaps	%xmm0,%xmm3
	shufps	$68,%xmm0,%xmm5
	movups	%xmm5,(%rax)
	shufps	$78,%xmm2,%xmm3
	movups	%xmm3,16(%rax)
	leaq	32(%rax),%rax
	jmp	.Lkey_expansion_192b_warm

.p2align	4
.Lkey_expansion_256a:
	movups	%xmm2,(%rax)
	leaq	16(%rax),%rax
.Lkey_expansion_256a_cold:
	shufps	$16,%xmm0,%xmm4
	xorps	%xmm4,%xmm0
	shufps	$140,%xmm0,%xmm4
	xorps	%xmm4,%xmm0
	shufps	$255,%xmm1,%xmm1
	xorps	%xmm1,%xmm0
	retq

.p2align	4
.Lkey_expansion_256b:
	movups	%xmm0,(%rax)
	leaq	16(%rax),%rax

	shufps	$16,%xmm2,%xmm4
	xorps	%xmm4,%xmm2
	shufps	$140,%xmm2,%xmm4
	xorps	%xmm4,%xmm2
	shufps	$170,%xmm1,%xmm1
	xorps	%xmm1,%xmm2
	retq


.p2align	6
.Lbswap_mask:
.byte	15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0
.Lincrement32:
.long	6,6,6,0
.Lincrement64:
.long	1,0,0,0
.Lxts_magic:
.long	0x87,0,1,0

.byte	65,69,83,32,102,111,114,32,73,110,116,101,108,32,65,69,83,45,78,73,44,32,67,82,89,80,84,79,71,65,77,83,32,98,121,32,60,97,112,112,114,111,64,111,112,101,110,115,115,108,46,111,114,103,62,0
.p2align	6

.def	ecb_se_handler;	.scl 3;	.type 32;	.endef
.p2align	4
ecb_se_handler:
	pushq	%rsi
	pushq	%rdi
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	pushfq
	subq	$64,%rsp

	movq	152(%r8),%rax

	jmp	.Lcommon_seh_tail


.def	ccm64_se_handler;	.scl 3;	.type 32;	.endef
.p2align	4
ccm64_se_handler:
	pushq	%rsi
	pushq	%rdi
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	pushfq
	subq	$64,%rsp

	movq	120(%r8),%rax
	movq	248(%r8),%rbx

	movq	8(%r9),%rsi
	movq	56(%r9),%r11

	movl	0(%r11),%r10d
	leaq	(%rsi,%r10,1),%r10
	cmpq	%r10,%rbx
	jb	.Lcommon_seh_tail

	movq	152(%r8),%rax

	movl	4(%r11),%r10d
	leaq	(%rsi,%r10,1),%r10
	cmpq	%r10,%rbx
	jae	.Lcommon_seh_tail

	leaq	0(%rax),%rsi
	leaq	512(%r8),%rdi
	movl	$8,%ecx
.long	0xa548f3fc		
	leaq	88(%rax),%rax

	jmp	.Lcommon_seh_tail


.def	ctr32_se_handler;	.scl 3;	.type 32;	.endef
.p2align	4
ctr32_se_handler:
	pushq	%rsi
	pushq	%rdi
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	pushfq
	subq	$64,%rsp

	movq	120(%r8),%rax
	movq	248(%r8),%rbx

	leaq	.Lctr32_body(%rip),%r10
	cmpq	%r10,%rbx
	jb	.Lcommon_seh_tail

	movq	152(%r8),%rax

	leaq	.Lctr32_ret(%rip),%r10
	cmpq	%r10,%rbx
	jae	.Lcommon_seh_tail

	leaq	32(%rax),%rsi
	leaq	512(%r8),%rdi
	movl	$20,%ecx
.long	0xa548f3fc		
	leaq	200(%rax),%rax

	jmp	.Lcommon_seh_tail


.def	xts_se_handler;	.scl 3;	.type 32;	.endef
.p2align	4
xts_se_handler:
	pushq	%rsi
	pushq	%rdi
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	pushfq
	subq	$64,%rsp

	movq	120(%r8),%rax
	movq	248(%r8),%rbx

	movq	8(%r9),%rsi
	movq	56(%r9),%r11

	movl	0(%r11),%r10d
	leaq	(%rsi,%r10,1),%r10
	cmpq	%r10,%rbx
	jb	.Lcommon_seh_tail

	movq	152(%r8),%rax

	movl	4(%r11),%r10d
	leaq	(%rsi,%r10,1),%r10
	cmpq	%r10,%rbx
	jae	.Lcommon_seh_tail

	leaq	96(%rax),%rsi
	leaq	512(%r8),%rdi
	movl	$20,%ecx
.long	0xa548f3fc		
	leaq	104+160(%rax),%rax

	jmp	.Lcommon_seh_tail

.def	cbc_se_handler;	.scl 3;	.type 32;	.endef
.p2align	4
cbc_se_handler:
	pushq	%rsi
	pushq	%rdi
	pushq	%rbx
	pushq	%rbp
	pushq	%r12
	pushq	%r13
	pushq	%r14
	pushq	%r15
	pushfq
	subq	$64,%rsp

	movq	152(%r8),%rax
	movq	248(%r8),%rbx

	leaq	.Lcbc_decrypt(%rip),%r10
	cmpq	%r10,%rbx
	jb	.Lcommon_seh_tail

	leaq	.Lcbc_decrypt_body(%rip),%r10
	cmpq	%r10,%rbx
	jb	.Lrestore_cbc_rax

	leaq	.Lcbc_ret(%rip),%r10
	cmpq	%r10,%rbx
	jae	.Lcommon_seh_tail

	leaq	0(%rax),%rsi
	leaq	512(%r8),%rdi
	movl	$8,%ecx
.long	0xa548f3fc		
	leaq	88(%rax),%rax
	jmp	.Lcommon_seh_tail

.Lrestore_cbc_rax:
	movq	120(%r8),%rax

.Lcommon_seh_tail:
	movq	8(%rax),%rdi
	movq	16(%rax),%rsi
	movq	%rax,152(%r8)
	movq	%rsi,168(%r8)
	movq	%rdi,176(%r8)

	movq	40(%r9),%rdi
	movq	%r8,%rsi
	movl	$154,%ecx
.long	0xa548f3fc		

	movq	%r9,%rsi
	xorq	%rcx,%rcx
	movq	8(%rsi),%rdx
	movq	0(%rsi),%r8
	movq	16(%rsi),%r9
	movq	40(%rsi),%r10
	leaq	56(%rsi),%r11
	leaq	24(%rsi),%r12
	movq	%r10,32(%rsp)
	movq	%r11,40(%rsp)
	movq	%r12,48(%rsp)
	movq	%rcx,56(%rsp)
	call	*__imp_RtlVirtualUnwind(%rip)

	movl	$1,%eax
	addq	$64,%rsp
	popfq
	popq	%r15
	popq	%r14
	popq	%r13
	popq	%r12
	popq	%rbp
	popq	%rbx
	popq	%rdi
	popq	%rsi
	retq


.section	.pdata
.p2align	2
.rva	.LSEH_begin_aesni_ecb_encrypt
.rva	.LSEH_end_aesni_ecb_encrypt
.rva	.LSEH_info_ecb

.rva	.LSEH_begin_aesni_ccm64_encrypt_blocks
.rva	.LSEH_end_aesni_ccm64_encrypt_blocks
.rva	.LSEH_info_ccm64_enc

.rva	.LSEH_begin_aesni_ccm64_decrypt_blocks
.rva	.LSEH_end_aesni_ccm64_decrypt_blocks
.rva	.LSEH_info_ccm64_dec

.rva	.LSEH_begin_aesni_ctr32_encrypt_blocks
.rva	.LSEH_end_aesni_ctr32_encrypt_blocks
.rva	.LSEH_info_ctr32

.rva	.LSEH_begin_aesni_xts_encrypt
.rva	.LSEH_end_aesni_xts_encrypt
.rva	.LSEH_info_xts_enc

.rva	.LSEH_begin_aesni_xts_decrypt
.rva	.LSEH_end_aesni_xts_decrypt
.rva	.LSEH_info_xts_dec
.rva	.LSEH_begin_aesni_cbc_encrypt
.rva	.LSEH_end_aesni_cbc_encrypt
.rva	.LSEH_info_cbc

.rva	aesni_set_decrypt_key
.rva	.LSEH_end_set_decrypt_key
.rva	.LSEH_info_key

.rva	aesni_set_encrypt_key
.rva	.LSEH_end_set_encrypt_key
.rva	.LSEH_info_key
.section	.xdata
.p2align	3
.LSEH_info_ecb:
.byte	9,0,0,0
.rva	ecb_se_handler
.LSEH_info_ccm64_enc:
.byte	9,0,0,0
.rva	ccm64_se_handler
.rva	.Lccm64_enc_body,.Lccm64_enc_ret	
.LSEH_info_ccm64_dec:
.byte	9,0,0,0
.rva	ccm64_se_handler
.rva	.Lccm64_dec_body,.Lccm64_dec_ret	
.LSEH_info_ctr32:
.byte	9,0,0,0
.rva	ctr32_se_handler
.LSEH_info_xts_enc:
.byte	9,0,0,0
.rva	xts_se_handler
.rva	.Lxts_enc_body,.Lxts_enc_epilogue	
.LSEH_info_xts_dec:
.byte	9,0,0,0
.rva	xts_se_handler
.rva	.Lxts_dec_body,.Lxts_dec_epilogue	
.LSEH_info_cbc:
.byte	9,0,0,0
.rva	cbc_se_handler
.LSEH_info_key:
.byte	0x01,0x04,0x01,0x00
.byte	0x04,0x02,0x00,0x00	
