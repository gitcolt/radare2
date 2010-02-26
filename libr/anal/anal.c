/* radare - LGPL - Copyright 2009-2010 */
/*   nibble<.ds@gmail.com> */
/*   pancake<nopcode.org> */

#include <r_anal.h>
#include <r_util.h>
#include <r_list.h>

R_API RAnalysis *r_anal_new() {
	return r_anal_init (MALLOC_STRUCT (struct r_anal_t));
}

R_API RList *r_anal_bb_list_new() {
	RList *list = r_list_new ();
	list->free = &r_anal_bb_list_free;
	return list;
}

R_API RList *r_anal_aop_list_new() {
	RList *list = r_list_new ();
	list->free = &r_anal_aop_list_free;
	return list;
}

R_API RAnalysis *r_anal_free(struct r_anal_t *a) {
	/* TODO: Free a->anals here */
	/* TODO: Free a->bbs here */
	free (a);
	return NULL;
}

R_API void r_anal_bb_list_free(void *bbs) {
	/* TODO */
}

R_API void r_anal_aop_list_free(void *aops) {
	/* TODO */
}

R_API struct r_anal_t *r_anal_init(struct r_anal_t *anal) {
	if (anal) {
		anal->user = NULL;
		anal->ctx = NULL;
		anal->cur = NULL;
		anal->bbs = r_anal_bb_list_new();
		r_anal_set_bits (anal, 32);
		r_anal_set_big_endian (anal, R_FALSE);
		INIT_LIST_HEAD (&anal->anals);
	}
	return anal;
}

R_API void r_anal_set_user_ptr(struct r_anal_t *anal, void *user) {
	anal->user = user;
}

R_API int r_anal_add(struct r_anal_t *anal, struct r_anal_handle_t *foo) {
	if (foo->init)
		foo->init(anal->user);
	list_add_tail(&(foo->list), &(anal->anals));
	return R_TRUE;
}

// TODO: Must be deprecated
R_API int r_anal_list(struct r_anal_t *anal) {
	struct list_head *pos;
	list_for_each_prev(pos, &anal->anals) {
		struct r_anal_handle_t *h = list_entry(pos, struct r_anal_handle_t, list);
		printf (" %s: %s\n", h->name, h->desc);
	}
	return R_FALSE;
}

R_API int r_anal_use(struct r_anal_t *anal, const char *name) {
	struct list_head *pos;
	list_for_each_prev (pos, &anal->anals) {
		struct r_anal_handle_t *h = list_entry(pos, struct r_anal_handle_t, list);
		if (!strcmp (h->name, name)) {
			anal->cur = h;
			return R_TRUE;
		}
	}
	return R_FALSE;
}

R_API int r_anal_set_bits(struct r_anal_t *anal, int bits) {
	switch (bits) {
	case 8:
	case 16:
	case 32:
	case 64:
		anal->bits = bits;
		return R_TRUE;
	}
	return R_FALSE;
}

R_API int r_anal_set_big_endian(struct r_anal_t *anal, int bigend) {
	anal->big_endian = bigend;
	return R_TRUE;
}

R_API int r_anal_aop(struct r_anal_t *anal, struct r_anal_aop_t *aop, ut64 addr, void *data, int len) { 
	if (anal && anal->cur && anal->cur->aop)
		return anal->cur->aop(anal, aop, addr, data, len);
	return R_FALSE;
}

R_API int r_anal_bbs(struct r_anal_t *anal, ut64 addr, ut8 *buf, ut64 len) {
	/* XXX No working*/
	struct r_anal_bb_t *bb;
	struct r_anal_aop_t *aop;
	ut64 oplen;

	if (!(bb = MALLOC_STRUCT(struct r_anal_bb_t))) {
		eprintf ("Error: malloc (bb)\n");
		return R_FALSE;
	}
	r_list_append (anal->bbs, bb);
	bb->addr = addr;
	for (;;) {
		if (!(aop = MALLOC_STRUCT(struct r_anal_aop_t))) {
			eprintf ("Error: malloc (aop)\n");
			return R_FALSE;
		}
		if (!(oplen = r_anal_aop (anal, aop, addr, buf, len)))
			break;
		r_list_append (bb->aops, aop);
		switch (aop->type) {
		case R_ANAL_OP_TYPE_CJMP:
			bb->fail = aop->fail;
			r_anal_bbs (anal, bb->fail, buf, len);
		case R_ANAL_OP_TYPE_JMP:
			bb->jump = aop->jump;
			r_anal_bbs (anal, bb->jump, buf, len);	
			bb->size = addr + oplen - bb->addr;
			break;
		}
		addr += oplen;
	}
	return R_TRUE;
}
