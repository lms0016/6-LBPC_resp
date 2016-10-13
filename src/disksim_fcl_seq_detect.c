   
/*
* Flash Cache Layer (FCL) (Version 1.0) 
*
* Author		: Yongseok Oh (ysoh@uos.ac.kr)
* Date			: 18/06/2012  
* Description	: The sequential I/O detector program
* File Name		: disksim_fcl_seq_detect.c
*/

#include <stdio.h>  
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "disksim_fcl_seq_detect.h"
#include "disksim_global.h"
#include <math.h> // fox

struct sequential_detector *seq_detector;
rb_node_t* root = NULL;   
int nodeCnt = 0; // fox

int sd_is_seq_io ( int blkno ) { 
	int startblk = seq_detector->sd_startblk;
	int lastblk = startblk + seq_detector->sd_length;
	int seq = 0;

	if ( !seq_detector->sd_enable )  
		return 0;

	if ( seq_detector->sd_length >= seq_detector->sd_seq_size &&
		 startblk <= blkno && lastblk >= blkno ) 
	{
		//printf (" Sequential I/O = %d \n", blkno );
		seq = 1;
	} else {

		//printf (" Non-Sequential I/O = %d \n", blkno );
	}		

	return seq;

}
int sd_seq_detection ( int blkno, int length ) {
	int seq = 0;

	if ( !seq_detector->sd_enable ) 
		return 0;

	if ( seq_detector->sd_startblk + seq_detector->sd_length != blkno ) {

		seq_detector->sd_startblk = blkno;
		seq_detector->sd_length = 0;

	} 

	seq_detector->sd_length += length;

	if ( seq_detector->sd_length >= seq_detector->sd_seq_size ) {
		seq = 1;
		/* 
		   printf ( " %f Seq detection: blkno = %d, length = %d \n", 
				   					simtime,
									seq_detector->sd_startblk,
									seq_detector->sd_length );
		//*/
	}

	return seq;

} 

void sd_init ( int enable, int seq_unit_sectors ){
	seq_detector = (struct sequential_detector *) malloc ( sizeof ( struct sequential_detector) );
	memset ( seq_detector, 0x00, sizeof ( struct sequential_detector ) );
	seq_detector->sd_seq_size = seq_unit_sectors;
	seq_detector->sd_enable = enable;
	printf("prize_alpha = %f\n", prize_alpha);
	printf("blk_mod = %d\n", blk_mod);
}
  
void sd_exit () { 
	free ( seq_detector );
}

static rb_node_t* rb_new_node(key_t key, int length, int flags) {
    rb_node_t *node = (rb_node_t*)malloc(sizeof(struct rb_node_t));
    if (!node) {
		printf("malloc error!\n");
	    exit(-1);
    }
    node->key = key; // blkno
//    node->data = data;
    if (flags == 0) {
        node->rCnt = 0;
        node->wCnt = 1;
    } else {
	node->rCnt = 1;
	node->wCnt = 0;
    }
    node->length = length;
    node->prize = 0.0;
    int i;
    for (i=0; i < page_cnt; i++) {
        node->leng[i] = 0;
    }
    
    return node;
} // end rb_new_node

/*-----------------------------------------------------------
|   node           right
|   / \    ==>     / \
|   a  right     node  y
|       / \           / \
|       b  y         a   b
 -----------------------------------------------------------*/

 static rb_node_t* rb_rotate_left(rb_node_t* node, rb_node_t* root) {
    rb_node_t* right = node->right;
    if ((node->right = right->left)) {
		right->left->parent = node;
    }
    right->left = node;
    if ((right->parent = node->parent)) {
        if (node == node->parent->right) {
            node->parent->right = right;
        } else {
            node->parent->left = right;
        }
    } else {
        root = right;
    }
    node->parent = right;
    return root;
} // rb_rotate_left

/*-----------------------------------------------------------
|       node           left
|       / \            / \
|    left  y   ==>    a   node
|   / \               / \
|  a   b             b   y
-----------------------------------------------------------*/

static rb_node_t* rb_rotate_right(rb_node_t* node, rb_node_t* root) {
    rb_node_t* left = node->left;
    if ((node->left = left->right)) {
		left->right->parent = node;
    }
    left->right = node;
    if ((left->parent = node->parent)) {
        if (node == node->parent->right) {
            node->parent->right = left;
        } else {
            node->parent->left = left;
        }
    } else {
        root = left;
    }
    node->parent = left;
    return root;
} // end rb_rotate_right

static rb_node_t* rb_insert_rebalance(rb_node_t *node, rb_node_t *root) {
    rb_node_t *parent, *gparent, *uncle, *tmp;
    while ((parent = node->parent) && parent->color == RED) {
        gparent = parent->parent;
        if (parent == gparent->left) {
            uncle = gparent->right;
            if (uncle && uncle->color == RED) {
                uncle->color = BLACK;
                parent->color = BLACK;
                gparent->color = RED;
                node = gparent;
            } else {
                if (parent->right == node) {
                    root = rb_rotate_left(parent, root);
                    tmp = parent;
                    parent = node;
                    node = tmp;
                }
                parent->color = BLACK;
                gparent->color = RED;
                root = rb_rotate_right(gparent, root);
            }
		} else {
            uncle = gparent->left;
            if (uncle && uncle->color == RED) {
                uncle->color = BLACK;
                parent->color = BLACK;
                gparent->color = RED;
                node = gparent;
            } else {
                if (parent->left == node) {
                    root = rb_rotate_right(parent, root);
                    tmp = parent;
                    parent = node;
                    node = tmp;
                }
                parent->color = BLACK;
                gparent->color = RED;
                root = rb_rotate_left(gparent, root);
            }
        }
    }
    root->color = BLACK;
    return root;
} // end rb_insert_rebalance

static rb_node_t* rb_erase_rebalance(rb_node_t *node, rb_node_t *parent, rb_node_t *root) {
    rb_node_t *other, *o_left, *o_right;
    while ((!node || node->color == BLACK) && node != root) {
        if (parent->left == node) {
            other = parent->right;
            if (other->color == RED) {
                other->color = BLACK;
                parent->color = RED;
                root = rb_rotate_left(parent, root);
                other = parent->right;
            }
            if ((!other->left || other->left->color == BLACK) &&
                (!other->right || other->right->color == BLACK)) {
                other->color = RED;
                node = parent;
                parent = node->parent;
            } else {
                if (!other->right || other->right->color == BLACK) {
                    if ((o_left = other->left)) {
                        o_left->color = BLACK;
                    }
                    other->color = RED;
                    root = rb_rotate_right(other, root);
                    other = parent->right;
                }
                other->color = parent->color;
                parent->color = BLACK;
                if (other->right) {
                    other->right->color = BLACK;
                }
                root = rb_rotate_left(parent, root);
                node = root;
                break;
            }
        } else {
            other = parent->left;
            if (other->color == RED) {
                other->color = BLACK;
                parent->color = RED;
                root = rb_rotate_right(parent, root);
                other = parent->left;
            }
            if ((!other->left || other->left->color == BLACK) &&
                (!other->right || other->right->color == BLACK)) {
                other->color = RED;
                node = parent;
                parent = node->parent;
            } else {
                if (!other->left || other->left->color == BLACK) {
                    if ((o_right = other->right)) {
                        o_right->color = BLACK;
                    }
                    other->color = RED;
                    root = rb_rotate_left(other, root);
                    other = parent->left;
                }
                other->color = parent->color;
                parent->color = BLACK;
                if (other->left) {
                    other->left->color = BLACK;
                }
                root = rb_rotate_right(parent, root);
                node = root;
                break;
            }
        }
    }
    if (node) {
        node->color = BLACK;
    }
    return root;
} // end rb_erase_rebalance

static rb_node_t* rb_search_auxiliary(key_t key, rb_node_t* root, rb_node_t** save) {
    rb_node_t *node = root, *parent = NULL;
    int ret;
    while (node) {
        parent = node;
        ret = node->key - key;
        if (0 < ret) {
            node = node->left;
        } else if (0 > ret) {
            node = node->right;
        } else {
            return node;
        }
    }
    if (save) {
        *save = parent;
    }
    return NULL;
} // end rb_search_auxiliary


/*
        cond_1        cond_2        cond_3        cond_4
      ----            ----             ----     --------
        --------    --------     --------         ----         
*/
static rb_node_t* rb_merge(rb_node_t *parent, rb_node_t *node, rb_node_t *root, int flags, double base){  
	rb_node_t *r_n = NULL, *l_n = NULL, *update_n = NULL;
	int key = node->key;	
	int length = node->length;
	int new_length = node->length, new_key = node->key;
	update_n = node;
	int remove_r_key = -1, remove_l_key = -1;

	if (node->left) { // finding left
		l_n = node->left;
		while (l_n->right) {
			l_n = l_n->right;		
		}

		if ((l_n->key + l_n->length-1) > key) { 
			new_key = l_n->key;
			remove_l_key = node->key;
			update_n = l_n;	
			update_n->wCnt += node->wCnt;
			update_n->rCnt += node->rCnt;		
			if (key+length-1 < l_n->key+l_n->length-1) {
				new_length = l_n->length; // cond_2				
			} else {
				new_length = key + length - l_n->key; // cond_3
			}
		}
	} else {
		if (parent) {
			if ((parent->key < key) && ((parent->key + parent->length-1) > key)) {
				new_key = parent->key;
				remove_l_key = node->key;
				update_n = parent;
				update_n->wCnt += node->wCnt;
				update_n->rCnt += node->rCnt;
				if (flags == 0) 
					update_n->wCnt++;
				else 
					update_n->rCnt++;	
				if (key+length-1 < parent->key+parent->length-1) {
					new_length = parent->length; // cond_2				
				} else {
					new_length = key + length - new_key; // cond_3
				}
			}
		}
	}
	if (node->right) { // finding right 
		r_n = node->right;
		while (r_n->left) {
			r_n = r_n->left;		
		}
		if ((key+length-1) > r_n->key) {
			remove_r_key = r_n->key;
			update_n->wCnt += r_n->wCnt;
			update_n->rCnt += r_n->rCnt;
			if (key+length-1 < r_n->key+r_n->length-1) {
				new_length = r_n->key + r_n->length - new_key; // cond_1
			} else {
				new_length = length; // cond_4
			}
		}
	} else {
		if (parent) {
			if ((parent->key > key) && ((key + length-1) > parent->key)) {
				remove_r_key = parent->key;
				update_n->wCnt += parent->wCnt;
				update_n->rCnt += parent->rCnt;
				if (key+length-1 < parent->key+parent->length-1) {
					new_length = parent->key + parent->length - new_key; // cond_1
				} else {
					new_length = length; // cond_4
				}
			}
		}
	}
	if (update_n) {
		update_n->length = new_length;
		update_n->prize = prize_alpha*(update_n->rCnt+1)/(update_n->wCnt*update_n->length+1) + (1-prize_alpha)*base; 
	} 
	if (remove_r_key != -1) 
		root = rb_erase(remove_r_key, root);
	if (remove_l_key != -1)
		root = rb_erase(remove_l_key, root);
	return root;
} // end rb_merge

rb_node_t* rb_insert(int key, double base, int length, int flags, rb_node_t* root, rb_node_t** save) {
    nodeCnt++; // fox
    rb_node_t *parent = NULL, *node;
    parent = NULL;
    if ((node = rb_search_auxiliary(key, root, &parent))) { 
        return root; 
    }
    node = rb_new_node(key, length, flags); 
    node->parent = parent; 
    node->left = node->right = NULL; 
    node->color = RED; 
    if (parent) {
        if (parent->key > key) { 
            parent->left = node;
        } else {
            parent->right = node;
        }
    } else {
        root = node;
    }
    root = rb_insert_rebalance(node, root);
//  root = rb_merge(parent, node, root, flags, base); // no need to merge
    if (save) {
        *save = node;
    }
    return root;
} // end rb_insert

rb_node_t* rb_search(key_t key, rb_node_t* root) { 
    return rb_search_auxiliary(key, root, NULL);
} // end rb_search

rb_node_t* rb_erase(key_t key, rb_node_t *root) {
    nodeCnt--; // fox
    rb_node_t *child, *parent, *old, *left, *node;
    color_t color;
    if (!(node = rb_search_auxiliary(key, root, NULL))) {
        printf("key %d is not exist!\n", key);
        return root;
    }
    old = node;
    if (node->left && node->right) { 
        node = node->right;
        while ((left = node->left) != NULL) { 
            node = left;
        }
        child = node->right; 
        parent = node->parent; 
        color = node->color; 
        if (child) {
            child->parent = parent;
        }
        if (parent) {
            if (parent->left == node) {
                parent->left = child;
            } else {
                parent->right = child;
            }
        } else {
            root = child;
        }
        if (node->parent == old) {
            parent = node;
        }
        node->parent = old->parent;
        node->color = old->color;
        node->right = old->right;
        node->left = old->left;
        if (old->parent) {
            if (old->parent->left == old) {
                old->parent->left = node;
            } else {
                old->parent->right = node;
            }
        } else {
            root = node;
        }
        old->left->parent = node;
        if (old->right) {
            old->right->parent = node;
        }
    } else {
        if (!node->left) {
            child = node->right;
        } else if (!node->right) {
            child = node->left;
        }
        parent = node->parent;
        color = node->color;
        if (child) {
            child->parent = parent;
        }
        if (parent) {
            if (parent->left == node) {
                parent->left = child;   
            }
            else {
                parent->right = child;
            }
        } else {
            root = child;
        }
    }
    free(old);
    if (color == BLACK) {
        root = rb_erase_rebalance(child, parent, root);
    }
    return root;
} // end rb_erase

void record_length(int key, int blkno, int length, rb_node_t *node) {
    int i;
    int start = (blkno - key)/8;
    int end = start + (length/8);
    if ( (blkno-key) < 0) {
	printf("error start<0\n\n\n");
    } 
    for ( i = start; i < end; i++ ) {
        node->leng[i] = 1;
    }
    int cnt = 0;
    for ( i = 0; i < page_cnt; i++ ) {
        cnt = cnt + node->leng[i];
    }
    node->length = cnt*8; // sector
}

double calculate_prize(int key, double base, int length, int flags, int blkno) {  
	rb_node_t *node = NULL; 
	node = NULL;
	if ((node = rb_search_auxiliary(key, root, NULL))) { // blkno exist!
		if (flags == 0) 
			node->wCnt++;
		else 
			node->rCnt++;
		
		//if (length > node->length) 
		//	node->length = length;
		record_length( key, blkno, length, node);
		return node->prize;
	} else { // blkno not exist!
		root = rb_insert(key, base, length, flags, root, &node);
		record_length( key, blkno, length, node);
		node->prize = prize_alpha * (node->rCnt+1)/(node->wCnt*node->length+1) + (1-prize_alpha)*base; 
		return node->prize;
	}
} // end calculate_prize

int get_nodeCnt() {
	return nodeCnt;
}

void set_sequnit( int s, unsigned int add ) { // fox
	int tmp = seq_detector->sd_seq_size; //orig
	if (add) {
		tmp += s;
	} else {
		tmp -= s; 
	}
	if (tmp < 256) { // according to SSD & HDD performance 16G: 256, 4G: 96
		tmp = 256;
	}
	if (tmp > 768)  // 16G: 768, 4G: 296
		tmp = 768;
	seq_detector->sd_seq_size = tmp;
}
