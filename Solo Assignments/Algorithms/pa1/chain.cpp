#include "chain.h"
#include "chain_given.cpp"

// PA1 functions

/**
 * Destroys the current Chain. This function should ensure that
 * memory does not leak on destruction of a chain.
 */
Chain::~Chain(){
  clear();
}

/**
 * Inserts a new node in position one of the Chain,
 * after the sentinel node.
 * This function **SHOULD** create a new Node.
 *
 * @param ndata The data to be inserted.
 */
void Chain::insertFront(const Block & ndata){
  Node * curr = head_;
  Node * newN = new Node(ndata);
  newN->next = curr->next;
  curr->next->prev = newN;
  curr->next = newN;
  newN->prev = curr;
  length_ += 1;
}

/**
 * Inserts a new node at the back of the Chain,
 * but before the tail sentinel node.
 * This function **SHOULD** create a new ListNode.
 *
 * @param ndata The data to be inserted.
 */
void Chain::insertBack(const Block & ndata){
  Node * curr = tail_;
  Node * newN = new Node(ndata);
  newN->prev = curr->prev;
  curr->prev->next = newN;
  curr->prev = newN;
  newN->next = curr;
  length_ += 1;
}

/**
 * Modifies the Chain by moving a contiguous subset of Nodes to
 * the end of the chain. The subset of len nodes from (and
 * including) startPos are moved so that they
 * occupy the last len positions of the
 * chain (maintaining the sentinel at the end). Their order is
 * not changed in the move.
 */
void Chain::moveToBack(int startPos, int len){
   Node * curr = head_;
   Node * end = tail_;
   for (int i = startPos; i < startPos + len; i++) {
       Node * move = walk(curr, startPos);
       move->prev->next = move->next;
       move->next->prev = move->prev;
       move->prev = end->prev;
       end->prev->next = move;
       move->next = end;
       end->prev = move;
   }
}

/**
 * Rotates the current Chain by k nodes: removes the first
 * k nodes from the Chain and attaches them, in order, at
 * the end of the chain (maintaining the sentinel at the end).
 */
void Chain::rotate(int k){
   moveToBack(1, k);
}

/**
 * Modifies the current chain by swapping the Node at pos1
 * with the Node at pos2. the positions are 1-based.
 */
void Chain::swap(int pos1, int pos2){
  if (abs(pos1 - pos2) == 1) {
    if (pos2 + 3 > size()) {
      swap(pos2, pos2 - 3);
      swap(pos1, pos2 - 3);
      swap(pos2, pos2 - 3);
    }
    else {
      swap(pos2, pos2 + 3);
      swap(pos1, pos2 + 3);
      swap(pos2, pos2 + 3);
    }
  }
  else {
    Node * curr = head_;
    Node * move1 = walk(curr, pos1);
    Node * move2 = walk(curr, pos2);
    Node * move2prev = walk(curr, pos2 - 1);
    Node * move2next = walk(curr, pos2 + 1);
    move1->prev->next = move2;
    move1->next->prev = move2;
    move2->prev = move1->prev;
    move2->next = move1->next;
    move2prev->next = move1;
    move2next->prev = move1;
    move1->prev = move2prev;
    move1->next = move2next;
  }
}

/*
 *  Modifies both the current and input chains by trading
 * nodes in even positions between the chains. Length
 * of each chain should stay the same. If the block dimensions
 * are NOT the same, the funtion has no effect and a simple
 * error message is output: cout << "Block sizes differ." << endl;
 */

void Chain::twist(Chain & other){
   if (width_ != other.width_ || height_ != other.height_) {
     cout << "Block sizes differ." << endl;
     return;
   }
   else {
     for (int i = 1; i <= (min((size()/2), other.size()/2)); i++) {
       Node * curr1 = head_;
       Node * curr2 = other.head_;
       Node * move1 = walk(curr1, 2*i);
       Node * move2 = walk(curr2, 2*i);
       Node * move2prev = walk(curr2, 2*i - 1);
       Node * move2next = walk(curr2, 2*i + 1);
       move1->prev->next = move2;
       move1->next->prev = move2;
       move2->prev = move1->prev;
       move2->next = move1->next;
       move2prev->next = move1;
       move2next->prev = move1;
       move1->prev = move2prev;
       move1->next = move2next;
     }
   }
}

/**
 * Destroys all dynamically allocated memory associated with the
 * current Chain class.
 */

void Chain::clear() {
   Node * curr = head_;
   Node * end = tail_;
   for (int i = 1; i <= size(); i++) {
     Node * move = walk(curr, 1);
     move->prev->next = move->next;
     move->next->prev = move->prev;
     delete move;
     move = NULL;
   }
   delete curr;
   curr = NULL;
   delete end;
   end = NULL;
}

/* makes the current object into a copy of the parameter:
 * All member variables should have the same value as
 * those of other, but the memory should be completely
 * independent. This function is used in both the copy
 * constructor and the assignment operator for Chains.
 */

void Chain::copy(Chain const& other) {

  width_ = other.width_;
  height_ = other.height_;

  head_ = new Node();
  tail_ = new Node();
  head_->next = tail_;
  tail_->prev = head_;

  length_ = 0;
  for (int i = 1; i <= other.size(); i++) {
    Node * move = walk(other.head_, i);
    insertBack(move->data);
  }
}
