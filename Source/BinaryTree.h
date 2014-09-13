#ifndef _BINARYTREE_H_
#define _BINARYTREE_H_

class CBinaryTree
{
	public:		
		CBinaryTree();
		void AddNode(IBinaryTreeNode *pNode);	
		void RemoveNode(IBinaryTreeNode *pNode);		
		
// Debug member functions		
		void DebugDrawTree();
		void DebugDrawAscending();		

	private:
		IBinaryTreeNode* m_pRoot;
};

#endif