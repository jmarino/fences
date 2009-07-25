/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#ifndef __INCLUDED_AVL_TREE_H__
#define __INCLUDED_AVL_TREE_H__


#define AVLTREE_VALUECMP(cmp)	((AvltreeValueCmp)(cmp))
#define AVLTREE_DATACMP(cmp)	((AvltreeDataCmp)(cmp))

/*
 * Generalized value to use in AVL tree
 */
typedef union avl_value {
	int i;
	double d;
} avl_value;

/*
 * node structure of AVL tree
 */
struct avl_node {
	avl_value value;	 			// value used for sorting
	void *data;						// pointer to data tracked by this node
	int height;						// height in tree (used for balancing)
	struct avl_node *parent;
	struct avl_node *left;
	struct avl_node *right;
};


/*
 * define types for cmp functions
 */
typedef int (*AvltreeValueCmp)(avl_value*,avl_value*);
typedef int (*AvltreeDataCmp)(void*,void*);



/* avl-tree.c */
struct avl_node* avltree_insert_node(struct avl_node *root, avl_value *value,
									 void *data, AvltreeValueCmp value_cmp);
void avltree_destroy(struct avl_node *root);
void* avltree_find(struct avl_node *root, avl_value *value, void *data,
				   AvltreeValueCmp value_cmp, AvltreeDataCmp data_cmp);

#endif
