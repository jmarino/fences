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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "avl-tree.h"



/*
 * Create a new node
 */
static struct avl_node*
avltree_create_node(avl_value *value, void *data)
{
	struct avl_node *node;

	node= (struct avl_node*)g_malloc(sizeof(struct avl_node));
	memcpy(&node->value, value, sizeof(avl_value));
	node->data= data;
	node->height= 0;
	node->parent= NULL;
	node->left= NULL;
	node->right= NULL;
	return node;
}


/*
 * Return max height of each children
 */
static int
avltree_calculate_height(struct avl_node *node)
{
	int height;

	if (node->left == NULL) height= -1;
	else height= node->left->height;
	if (node->right != NULL && node->right->height > height)
		return node->right->height + 1;

	return height + 1;
}


/*
 * Insert node in tree under node 'parent'.
 *   parent: insert new node under 'parent', creates new tree if NULL
 *   value_cmp: function that compares avl_value's (returns -1,0,+1)
 */
struct avl_node*
avltree_insert_node_at(struct avl_node *parent, avl_value *value, void *data,
					   AvltreeValueCmp value_cmp)
{
	struct avl_node *node;
	struct avl_node *pivot;
	int lh;
	int rh;
	int diff;

	/* create new node */
	node= avltree_create_node(value, data);
	if (parent == NULL) return node;	/* first element of tree */

	/* connect new node */
	node->parent= parent;
	if (value_cmp(value, &parent->value) < 0) parent->left= node;
	else parent->right= node;

	/* unwind up the tree and fix any height balance */
	node= parent;
	while(node != NULL) {
		node->height= avltree_calculate_height(node);
		if (node->left == NULL) lh= -1;
		else lh= node->left->height;
		if (node->right == NULL) rh= -1;
		else rh= node->right->height;
		diff= lh - rh;

		/* rebalance if necessary */
		pivot= node;		// for when no balancing required
		if (diff > 1) {				/* left is more loaded */
			/* rotate nodes to left */
			pivot= node->left;
			/* re-parent node and pivot */
			pivot->parent= node->parent;
			if (node->parent != NULL) {
				if (node->parent->left == node) node->parent->left= pivot;
				else if (node->parent->right == node) node->parent->right= pivot;
			}
			node->parent= pivot;
			/* rotate nodes */
			node->left= pivot->right;
			if (pivot->right != NULL) pivot->right->parent= node;
			pivot->right= node;
			/* adjust heights */
			node->height= avltree_calculate_height(node);
			pivot->height= avltree_calculate_height(pivot);
			/* new node was old pivot */
			node= pivot;
		} else if (diff < -1) {		/* right is more loaded */
			/* rotate nodes to left */
			pivot= node->right;
			/* avoid putting an equal value on the left branch */
			if (value_cmp(&node->value, &pivot->value) == 0) {
				pivot= node;		// need to set pivot if current node is root
				node= node->parent;
				continue;
			}
			/* re-parent node and pivot */
			pivot->parent= node->parent;
			if (node->parent != NULL) {
				if (node->parent->left == node) node->parent->left= pivot;
				else if (node->parent->right == node) node->parent->right= pivot;
			}
			node->parent= pivot;
			/* rotate nodes */
			node->right= pivot->left;
			if (pivot->left != NULL) pivot->left->parent= node;
			pivot->left= node;
			/* adjust heights */
			node->height= avltree_calculate_height(node);
			pivot->height= avltree_calculate_height(pivot);
			/* new node was old pivot */
			node= pivot;
		}
		node= pivot->parent;
	}

	return pivot;
}


/*
 * Insert node in tree.
 * Bottom insertion and rebalance as we backtrack up the tree.
 *   value_cmp: function that compares avl_value's (returns -1,0,+1)
 */
struct avl_node*
avltree_insert_node(struct avl_node *root, avl_value *value, void *data,
					AvltreeValueCmp value_cmp)
{
	struct avl_node *parent=NULL;

	/* traverse tree to find insertion point */
	while(root != NULL) {
		parent= root;
		if (value_cmp(value, &root->value) < 0) {	/* go on left */
			root= root->left;
		} else {								/* go on right */
			root= root->right;
		}
	}
	return avltree_insert_node_at(parent, value, data, value_cmp);
}


/*
 * Destroy AVL tree
 */
void
avltree_destroy(struct avl_node *root)
{
	struct avl_node *node=root;
	struct avl_node *parent;

	while(node != NULL) {
		if (node->left != NULL) {
			node= node->left;
			continue;
		}
		if (node->right != NULL) {
			node= node->right;
			continue;
		}
		parent= node->parent;
		if (parent != NULL) {
			if (parent->left == node) parent->left= NULL;
			else if (parent->right == node) parent->right= NULL;
		}
		g_free(node);
		node= parent;
	}
}


/*
 * Find element in tree
 *  value_cmp: function that compares avl_value's (returns -1,0,+1)
 *  data_cmp: compares the data to make sure they're equal
 *  parent: returns pointer to parent of last travelled node, useful for insert_at
 */
void*
avltree_find(struct avl_node *root, avl_value *value, void *data,
			 AvltreeValueCmp value_cmp, AvltreeDataCmp data_cmp, struct avl_node **parent)
{
	int cmp;

	if (parent) *parent= NULL;
	while(root != NULL) {
		if (parent) *parent= root;
		cmp= value_cmp(value, &root->value);
		if (cmp == 0) { 			/* found? */
			if (data_cmp(data, root->data) == 0) {	/* FOUND! */
				return root->data;
			} else {
				cmp= 1;				/* not this one, continue on the right */
			}
		}
		if (cmp < 0) {		/* go on left */
			root= root->left;
		} else {					/* go on right */
			root= root->right;
		}
	}

	return NULL;
}
