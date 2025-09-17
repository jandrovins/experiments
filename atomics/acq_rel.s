	.intel_syntax noprefix
	.file	"acq_rel.cpp"
	.text
	.globl	_Z12send_bad_extR7channelPv     # -- Begin function _Z12send_bad_extR7channelPv
	.p2align	4
	.type	_Z12send_bad_extR7channelPv,@function
_Z12send_bad_extR7channelPv:            # @_Z12send_bad_extR7channelPv
	.cfi_startproc
# %bb.0:
	mov	qword ptr [rdi + 8], rsi
	mov	byte ptr [rdi + 1], 1
	ret
.Lfunc_end0:
	.size	_Z12send_bad_extR7channelPv, .Lfunc_end0-_Z12send_bad_extR7channelPv
	.cfi_endproc
                                        # -- End function
	.globl	_Z12recv_bad_extR7channel       # -- Begin function _Z12recv_bad_extR7channel
	.p2align	4
	.type	_Z12recv_bad_extR7channel,@function
_Z12recv_bad_extR7channel:              # @_Z12recv_bad_extR7channel
	.cfi_startproc
# %bb.0:
	mov	rax, qword ptr [rdi + 8]
	mov	byte ptr [rdi + 1], 0
	ret
.Lfunc_end1:
	.size	_Z12recv_bad_extR7channel, .Lfunc_end1-_Z12recv_bad_extR7channel
	.cfi_endproc
                                        # -- End function
	.ident	"clang version 20.1.8"
	.section	".note.GNU-stack","",@progbits
	.addrsig
