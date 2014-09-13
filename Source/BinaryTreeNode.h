#ifndef _BINARYTREENODE_H_
#define _BINARYTREENODE_H_

class IBinaryTreeNode
{
	public:
		IBinaryTreeNode();
		virtual int GetValue() = 0;

	private:
		void Add(IBinaryTreeNode* pNode);	
		void Remove(IBinaryTreeNode** pRoot);		
		void DebugDrawTree(int Indent, char LR);		
		void DebugDrawAscending();		
			
		void RemoveHasBothChildren(IBinaryTreeNode** pRoot);	
		void ReplaceWith(IBinaryTreeNode* pNode);
		void RemoveAtParent(IBinaryTreeNode** pRoot, IBinaryTreeNode* pNewChild);		
		IBinaryTreeNode* FindRightmost();			
		
		IBinaryTreeNode* m_pParent;			
		IBinaryTreeNode* m_pLeft;					
		IBinaryTreeNode* m_pRight;	
		
	friend class CBinaryTree;
};

#endif