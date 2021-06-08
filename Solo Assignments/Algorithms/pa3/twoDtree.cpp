
/**
 *
 * twoDtree (pa3)
 * slight modification of a Kd tree of dimension 2.
 * twoDtree.cpp
 * This file will be used for grading.
 *
 */

#include "twoDtree.h"
#include <stack>

/* given */
twoDtree::Node::Node(pair<int,int> ul, pair<int,int> lr, RGBAPixel a)
	:upLeft(ul),lowRight(lr),avg(a),left(NULL),right(NULL)
	{}

/* given */
twoDtree::~twoDtree(){
	clear();
}

/* given */
twoDtree::twoDtree(const twoDtree & other) {
	copy(other);
}

/* given */
twoDtree & twoDtree::operator=(const twoDtree & rhs){
	if (this != &rhs) {
		clear();
		copy(rhs);
	}
	return *this;
}

twoDtree::twoDtree(PNG & imIn){
	 stats stat = stats(imIn);
	 height = imIn.height();
	 width = imIn.width();
	 pair<int, int> ul (0, 0);
 	 pair<int, int> lr (width - 1, height - 1);
	 root = buildTree(stat, ul, lr);
}

twoDtree::Node * twoDtree::buildTree(stats & s, pair<int,int> ul, pair<int,int> lr) {
	pair<int, int> newUl1;
	pair<int, int> newLr1;
	pair<int, int> newUl2;
	pair<int, int> newLr2;
	if (ul == lr) {
		return new Node(ul, lr, s.getAvg(ul, lr));
	} else if (ul.first == lr.first) {
		pair<int, int> newUl1 (ul.first, ul.second);
		pair<int, int> newLr1 (lr.first, ul.second);
		pair<int, int> newUl2 (ul.first, ul.second + 1);
		pair<int, int> newLr2 (lr.first, lr.second);
		long smallScore = s.getScore(newUl1, newLr1) + s.getScore(newUl2, newLr2);
		for (int i = ul.second + 1; i < lr.second; i++) {
			pair<int, int> curUl1 (ul.first, ul.second);
			pair<int, int> curLr1 (lr.first, i);
			pair<int, int> curUl2 (ul.first, i + 1);
			pair<int, int> curLr2 (lr.first, lr.second);
			long curSmallScore = s.getScore(curUl1, curLr1) + s.getScore(curUl2, curLr2);
			if (curSmallScore <= smallScore) {
				newUl1 = curUl1;
				newLr1 = curLr1;
				newUl2 = curUl2;
				newLr2 = curLr2;
				smallScore = curSmallScore;
			}
		}
		Node * root = new Node(ul, lr, s.getAvg(ul, lr));
		root->left = buildTree(s, newUl1, newLr1);
		root->right = buildTree(s, newUl2, newLr2);
		return root;
	} else if (ul.second == lr.second) {
		pair<int, int> newUl1 (ul.first, ul.second);
		pair<int, int> newLr1 (ul.first, lr.second);
		pair<int, int> newUl2 (ul.first + 1, ul.second);
		pair<int, int> newLr2 (lr.first, lr.second);
		long smallScore = s.getScore(newUl1, newLr1) + s.getScore(newUl2, newLr2);
		for (int i = ul.first + 1; i < lr.first; i++) {
			pair<int, int> curUl1 (ul.first, ul.second);
			pair<int, int> curLr1 (i, lr.second);
			pair<int, int> curUl2 (i + 1, ul.second);
			pair<int, int> curLr2 (lr.first, lr.second);
			long curSmallScore = s.getScore(curUl1, curLr1) + s.getScore(curUl2, curLr2);
			if (curSmallScore <= smallScore) {
				newUl1 = curUl1;
				newLr1 = curLr1;
				newUl2 = curUl2;
				newLr2 = curLr2;
				smallScore = curSmallScore;
			}
		}
		Node * root = new Node(ul, lr, s.getAvg(ul, lr));
		root->left = buildTree(s, newUl1, newLr1);
		root->right = buildTree(s, newUl2, newLr2);
		return root;
	} else {
		pair<int, int> newUl1 (ul.first, ul.second);
		pair<int, int> newLr1 (ul.first, lr.second);
		pair<int, int> newUl2 (ul.first + 1, ul.second);
		pair<int, int> newLr2 (lr.first, lr.second);
		long smallScore = s.getScore(newUl1, newLr1) + s.getScore(newUl2, newLr2);
		for (int i = ul.first + 1; i < lr.first; i++) {
			pair<int, int> curUl1 (ul.first, ul.second);
			pair<int, int> curLr1 (i, lr.second);
			pair<int, int> curUl2 (i + 1, ul.second);
			pair<int, int> curLr2 (lr.first, lr.second);
			long curSmallScore = s.getScore(curUl1, curLr1) + s.getScore(curUl2, curLr2);
			if (curSmallScore <= smallScore) {
				newUl1 = curUl1;
				newLr1 = curLr1;
				newUl2 = curUl2;
				newLr2 = curLr2;
				smallScore = curSmallScore;
			}
		}
		for (int i = ul.second; i < lr.second; i++) {
			pair<int, int> curUl1 (ul.first, ul.second);
			pair<int, int> curLr1 (lr.first, i);
			pair<int, int> curUl2 (ul.first, i + 1);
			pair<int, int> curLr2 (lr.first, lr.second);
			long curSmallScore = s.getScore(curUl1, curLr1) + s.getScore(curUl2, curLr2);
			if (curSmallScore <= smallScore) {
				newUl1 = curUl1;
				newLr1 = curLr1;
				newUl2 = curUl2;
				newLr2 = curLr2;
				smallScore = curSmallScore;
			}
		}
		Node * root = new Node(ul, lr, s.getAvg(ul, lr));
		root->left = buildTree(s, newUl1, newLr1);
		root->right = buildTree(s, newUl2, newLr2);
		return root;
	}


}

PNG twoDtree::render(){
	PNG img = PNG(width, height);
	vector<Node *> leaves;
	findLeaves(root, leaves);
	for (int i = 0; i < leaves.size(); i++) {
		Node * node = leaves[i];
		for (int x = node->upLeft.first; x <= node->lowRight.first; x++) {
			for (int y = node->upLeft.second; y <= node->lowRight.second; y++) {
				RGBAPixel * pixel = img.getPixel(x, y);
				pixel->r = node->avg.r;
				pixel->g = node->avg.g;
				pixel->b = node->avg.b;
			}
		}
	}
	return img;
}

void twoDtree::prune(double pct, int tol){
	stack<Node*> s;

	if (root == NULL)
		return;
	s.push(root);
	while (!(s.empty())) {
		Node* node = s.top();
		s.pop();
		if (suitable(pct, tol, node)) {
			if (node->right != NULL)
				clear(node->right);
				node->right = NULL;
			if (node->left != NULL) {
				clear(node->left);
				node->left = NULL;
			}
		}
		if (node->right != NULL)
			s.push(node->right);
		if (node->left != NULL) {
			s.push(node->left);
		}
	}
}

void twoDtree::clear() {
	clear(root);
	height = 0;
	width = 0;
}


void twoDtree::copy(const twoDtree & orig){
	root = copy(orig.root);
	height = orig.height;
	width = orig.width;
}

void twoDtree::findLeaves(Node * root, vector<Node *> & leaves) {
	if (root->left) {
		findLeaves(root->left, leaves);
	}
	if (root->right) {
		findLeaves(root->right, leaves);
	}
	if (root->left == NULL && root->right == NULL) {
		leaves.push_back(root);
	}
}

bool twoDtree::suitable(double pct, int tol, Node * root) {
	vector<Node *> leaves;
	findLeaves(root, leaves);
	int numSuitable = 0;
	for (int i = 0; i < leaves.size(); i++) {
		if (distance(root, leaves[i]) <= tol) {
			numSuitable++;
		}
	}
	return (((double)numSuitable)/(leaves.size())) >= pct;
}

long twoDtree::distance(Node * n1, Node * n2) {
	RGBAPixel avg1 = n1->avg;
	RGBAPixel avg2 = n2->avg;

	long sqRed = (avg1.r - avg2.r) * (avg1.r - avg2.r);
	long sqGreen = (avg1.g - avg2.g) * (avg1.g - avg2.g);
	long sqBlue = (avg1.b - avg2.b) * (avg1.b - avg2.b);
	return sqRed + sqGreen + sqBlue;
}

void twoDtree::clear(Node * root) {
	if (root == NULL)
			return;
	clear(root->left);
	clear(root->right);
	delete root;
	root = NULL;
}

twoDtree::Node * twoDtree::copy(const Node * root) {
	if (root == NULL)
			return NULL;

	Node* newNode = new Node(root->upLeft, root->lowRight, root->avg);
	newNode->left = copy(root->left);
	newNode->right = copy(root->right);
	return newNode;
}
