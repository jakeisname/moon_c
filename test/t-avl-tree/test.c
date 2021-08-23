
#include <stdio.h>
#include <time.h>
#include "avl.h"

typedef struct foo {
	int grp;
	int v;
} t_foo;


static int cmp_foo(t_foo *item, t_foo *data, void *paramP)
{
    if (item->grp == data->grp) {
	    if (item->v == data->v) 
		    return 0;
	    else if (item->v < data->v)
		    return -1;
	    else
		    return 1;
    } else if (item->grp < data->grp)
        return -1;
    else
        return 1;

}

tAvlTable *foo_table;
tAvlTraverser foo_trav;
extern int rebalanced;

int init_foo()
{
	foo_table = avl_create((avl_comparison_func*) cmp_foo, NULL, NULL);
	if (foo_table == NULL) {
		printf("avl_create() failed()\n");
		return -1;
	}

	foo_table->avl_data_size = 0;

	return 0;
}

extern void show_avl_node(TAVL_nodeptr node, int grp, int v);
static void show_all_avl_node()
{
	t_foo *foo;
	TAVL_nodeptr node;

	printf("\n");
	printf("[show all]------------\n");

	foo = (t_foo *) avl_t_first(&foo_trav, foo_table);
	while (foo != NULL) {

		node = foo_trav.tavl_curP;

		show_avl_node(node, foo->grp, foo->v);

		foo = (t_foo *) avl_t_next(&foo_trav);
	}

	printf("----------------------\n");
	printf("\n");
}

int add_foo(int grp, int v)
{
	t_foo *foo;
	t_foo *added;
	t_foo find_foo;

	find_foo.grp = grp;
	find_foo.v = v;

	foo = (t_foo *) avl_find(foo_table, (void *) &find_foo);
	if (foo) 
		return -1; /* found */

	foo = malloc(sizeof(*foo));

	foo->grp = grp;
	foo->v = v;

	rebalanced = 0;

	added = (t_foo *) avl_insert(foo_table, (void *) foo);
	if (!added) {
		printf("avl_insert failed\n");
		return -1;
	}

	printf("%s(%d): avl_insert() success. grp=%d, v=%d\n", 
		__func__, __LINE__, grp, v);
#if 0
	if (rebalanced) 
		show_all_avl_node();
#endif
	return 0;
}

int remove_foo(int grp, int v)
{
	t_foo *foo;
	t_foo find_foo;
	t_foo *deleted;

	find_foo.grp = grp;
	find_foo.v = v;

	foo = (t_foo *) avl_find(foo_table, (void *) &find_foo);
	if (!foo) 
		return -1; /* not found */

	rebalanced = 0;

	deleted = (t_foo *) avl_delete(foo_table, (void *) foo, NULL);
	if (!deleted) {
		printf("avl_delete() failed\n");
		return -2;
	}
	printf("%s(%d): avl_delete() success. foo=%p, deleted=%p, grp=%d, v=%d, d->grp=%d, d->v=%d\n", 
			__func__, __LINE__, foo, deleted, grp, v, deleted->grp, deleted->v);
	
	if (rebalanced) 
		show_all_avl_node();

	free(deleted);

	return 0;	
}

#if 1
int remove_foo_grp(int grp)
{
	t_foo *foo;
	t_foo *deleted;
	int cnt = 0;
	int fg;
	int fv;
	int rc;
	int top;

	rebalanced = 0;

	foo = (t_foo *) avl_t_first(&foo_trav, foo_table);
	while (foo) {

		fg = foo->grp;
		fv = foo->v;

		if (fg == grp) {
			printf("%s(%d): try remove node. grp=%d, v=%d\n", 
					__func__, __LINE__, fg, fv);

			top = avl_t_ptr_to_head(&foo_trav);
			avl_t_prev(&foo_trav);
			top += avl_t_ptr_to_head(&foo_trav);

			deleted = (t_foo *) avl_delete(foo_table, (void *) foo, &foo_trav);
			if (deleted) {
				cnt++;
				free(foo);

				if (top) 
					foo = (t_foo *) avl_t_first(&foo_trav, foo_table);
				else
					foo = (t_foo *) avl_t_curr(&foo_trav);
				continue;
			} else {
				printf("%s(%d): ***) avl_delete() failed.\n", 
						__func__, __LINE__);
				avl_t_next(&foo_trav);
			}
		}

		foo = (t_foo *) avl_t_next(&foo_trav);
	}

	if (rebalanced) 
		show_all_avl_node();

	printf("%s(%d): removed grp=%d, cnt=%d\n", 
			__func__, __LINE__, grp, cnt);
	
	return 0;	
}
#elif 0
int remove_foo_grp(int grp)
{
	t_foo *foo;
	t_foo *deleted;
	int cnt = 0;
	int fg;
	int fv;
	int rc;
	int top;

	rebalanced = 0;

	foo = (t_foo *) avl_t_first(&foo_trav, foo_table);
	while (foo) {

		fg = foo->grp;
		fv = foo->v;

		if (fg == grp) {
			printf("%s(%d): try remove node. grp=%d, v=%d\n", 
					__func__, __LINE__, fg, fv);

			top = avl_t_ptr_to_head(&foo_trav);
			avl_t_prev(&foo_trav);

			top += avl_t_ptr_to_head(&foo_trav);
			avl_t_prev(&foo_trav);
			top += avl_t_ptr_to_head(&foo_trav);

			deleted = (t_foo *) avl_delete(foo_table, (void *) foo, &foo_trav);
			if (deleted) {
				cnt++;
				free(foo);

				if (top) 
					foo = (t_foo *) avl_t_first(&foo_trav, foo_table);
				else
					foo = (t_foo *) avl_t_curr(&foo_trav);
				continue;
			} else {
				printf("%s(%d): ***) avl_delete() failed.\n", 
						__func__, __LINE__);
				avl_t_next(&foo_trav);
				avl_t_next(&foo_trav);
			}
		}

		foo = (t_foo *) avl_t_next(&foo_trav);
	}

	if (rebalanced) 
		show_all_avl_node();

	printf("%s(%d): removed grp=%d, cnt=%d\n", 
			__func__, __LINE__, grp, cnt);
	
	return 0;	
}
#else
int remove_foo_grp(int grp)
{
	t_foo *foo;
	t_foo *deleted;
	int cnt = 0;
	int fg;
	int fv;
	int rc;
	int top;
	t_foo *prev_foo, *find_foo;

	rebalanced = 0;

	foo = (t_foo *) avl_t_first(&foo_trav, foo_table);

	while (foo) {

		fg = foo->grp;
		fv = foo->v;

		if (fg == grp) {
			printf("%s(%d): try remove node. grp=%d, v=%d\n", 
					__func__, __LINE__, fg, fv);

			top = avl_t_ptr_to_head(&foo_trav);
			avl_t_prev(&foo_trav);

			prev_foo = (t_foo *) avl_t_curr(&foo_trav);
			top += avl_t_ptr_to_head(&foo_trav);

			deleted = (t_foo *) avl_delete(foo_table, (void *) foo);
			if (deleted) {
				cnt++;
				free(foo);

				if (top) {
					foo = (t_foo *) avl_t_first(&foo_trav, foo_table);
					continue;
				}

				if (prev_foo) {
					find_foo = avl_t_find(&foo_trav, foo_table, prev_foo);
					if (!find_foo) {
						foo = (t_foo *) avl_t_first(&foo_trav, foo_table);
						continue;
					}
				} else {
					foo = (t_foo *) avl_t_first(&foo_trav, foo_table);
					continue;
				}
			} else {
				printf("%s(%d): ***) avl_delete() failed.\n", 
						__func__, __LINE__);
				foo = (t_foo *) avl_t_next(&foo_trav);
			}
		}

		foo = (t_foo *) avl_t_next(&foo_trav);
	}

	if (rebalanced) 
		show_all_avl_node();

	printf("%s(%d): removed grp=%d, cnt=%d\n", 
			__func__, __LINE__, grp, cnt);
	
	return 0;	
}
#endif

static void test1()
{
	add_foo(2, 42);
	add_foo(2, 42);
	add_foo(2, 42);
	add_foo(2, 42);

	add_foo(2, 12);
	add_foo(2, 32);
	add_foo(2, 52);
	add_foo(2, 62);
	add_foo(2, 22);
	add_foo(2, 72);

	add_foo(1, 71);
	add_foo(1, 41);
	add_foo(1, 11);
	add_foo(1, 31);
	add_foo(1, 51);
	add_foo(1, 61);
	add_foo(1, 21);

	add_foo(3, 73);
	add_foo(3, 43);
	add_foo(3, 13);
	add_foo(3, 33);
	add_foo(3, 53);
	add_foo(3, 63);
	add_foo(3, 23);

	remove_foo(3, 23);
	add_foo(3, 23);

	remove_foo(3, 23);
	add_foo(3, 23);

	remove_foo(3, 23);
	add_foo(3, 23);

	remove_foo(2, 12);

	remove_foo_grp(2);

	remove_foo(1, 11);
	remove_foo(1, 21);
	remove_foo(1, 31);
	remove_foo(1, 41);
	remove_foo(1, 51);
	remove_foo(1, 61);
	remove_foo(1, 71);

	remove_foo_grp(3);

	show_all_avl_node();

	add_foo(2, 72);
	add_foo(2, 42);
	add_foo(2, 12);
	add_foo(2, 32);
	add_foo(2, 52);
	add_foo(2, 62);
	add_foo(2, 22);

	add_foo(1, 71);
	add_foo(1, 41);
	add_foo(1, 11);
	add_foo(1, 31);
	add_foo(1, 51);
	add_foo(1, 61);
	add_foo(1, 21);

	add_foo(3, 73);
	add_foo(3, 43);
	add_foo(3, 13);
	add_foo(3, 33);
	add_foo(3, 53);
	add_foo(3, 63);
	add_foo(3, 23);

	show_all_avl_node();

	remove_foo_grp(2);

	remove_foo(1, 11);
	remove_foo(1, 21);
	remove_foo(1, 31);
	remove_foo(1, 41);
	remove_foo(1, 51);
	remove_foo(1, 61);
	remove_foo(1, 71);

	remove_foo_grp(3);

	show_all_avl_node();
}

static void test1a()
{
	add_foo(1, 1);
	add_foo(2, 1);

	remove_foo_grp(1);
	remove_foo_grp(2);

	add_foo(1, 1);
	add_foo(2, 1);
	show_all_avl_node();

	remove_foo_grp(2);
	remove_foo_grp(1);

	add_foo(2, 1);
	add_foo(1, 1);
	show_all_avl_node();

	remove_foo_grp(2);
	remove_foo_grp(1);

	add_foo(2, 1);
	add_foo(1, 1);

	remove_foo_grp(1);
	remove_foo_grp(2);
}

static void add_t33()
{
	add_foo(1, 2);
	add_foo(3, 2);
	add_foo(1, 1);
	add_foo(2, 3);
	add_foo(2, 1);
	add_foo(3, 1);
	add_foo(2, 2);
	add_foo(3, 3);
	add_foo(1, 3);
}

static void test1b()
{
	add_t33();
	show_all_avl_node();

	remove_foo_grp(3);
	show_all_avl_node();

	remove_foo_grp(2);
	show_all_avl_node();
}

static void test1c()
{
        add_t33();

        show_all_avl_node();
        remove_foo_grp(2);
        show_all_avl_node();
        remove_foo_grp(3);
        show_all_avl_node();
        remove_foo_grp(1);

        show_all_avl_node();
}

#define NN      5
#define GRP     10

static void test2()
{
	long r;
	int n;
	int add;
	int grp;
	int v;
	long cnt = 0;
	int fill_cnt = 0;
	int fill[GRP][NN] = { 0, };

	while (1) {
		r = random();

		n = r % (NN * GRP);

		grp = (n / NN) + 1;

		if (grp > GRP)
			grp -= GRP;

		v = (n % NN) + 1;

		printf("%s(%d): loop=%ld, %s grp=%d, v=%d\n", __func__, __LINE__, 
				++cnt, 
				add ? "add" : "remove",
				grp, v);

		add_foo(grp, v);

		if (!fill[grp-1][v-1]) {
			fill[grp-1][v-1] = 1;
			fill_cnt++;
			if (fill_cnt >= (GRP * NN)) {
				show_all_avl_node();
				for (grp = GRP; grp > 0; grp--) 
					remove_foo_grp(grp);

				return;
			}
		}
	}
}

static void test3()
{
	long r;
	int n;
	int add;
	int grp;
	int v;
	long cnt = 0;

	while (1) {
		r = random();

		n = r % (NN * GRP * 2);

		add = n / (NN * GRP);
		grp = (n / NN) + 1;

		if (grp > GRP)
			grp -= GRP;

		v = (n % NN) + 1;

		printf("%s(%d): loop=%ld, %s grp=%d, v=%d\n", __func__, __LINE__, 
			++cnt, 
			add ? "add" : "remove",
			grp, v);
		if (add) 
			add_foo(grp, v);
		else
			remove_foo(grp, v);

		if (cnt % (NN * GRP * NN * GRP) == 0) {
			show_all_avl_node();
			for (grp = GRP; grp > 0; grp--) 
				remove_foo_grp(grp);

			if (cnt % (NN * GRP * NN * GRP * NN) == 0) 
				return;
		}
	}
}

static void test4()
{
        int i;
        int grp;
        int v;

        for (i = 1; i <= GRP; i++) {
                for (grp = 1; grp <= GRP; grp++)
                        remove_foo_grp(grp);

                /* add */
                for (grp = 1; grp <= GRP; grp++)
                        for (v = 1; v <= NN; v++)
                                add_foo(grp, v);

                printf("%s(%d): i=%d ****************************************************************************************************\n", __func__, __LINE__, i);
                remove_foo_grp(i);
        }
}


int main()
{
	int rc;
	int seed;
	char buff[100];

	rc = init_foo();
	if (rc)
		return -1;

#if 1
	test1();
	test1a();
	test1b();
	test1c();

	srand(time(NULL));

	test2();

	test3();
#endif
	test4();

	return 0;
}

