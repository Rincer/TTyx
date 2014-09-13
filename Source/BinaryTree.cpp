#include "stdafx.h"
#include "BinaryTreeNode.h"

#include "BinaryTree.h"

//--------------------------------------------------------------------------------
CBinaryTree::CBinaryTree()
{
	m_pRoot = NULL;
}

//--------------------------------------------------------------------------------
void CBinaryTree::AddNode(IBinaryTreeNode *pNode)
{
	if(m_pRoot == NULL)
		m_pRoot = pNode;
	else
		m_pRoot->Add(pNode);
}

//--------------------------------------------------------------------------------
void CBinaryTree::RemoveNode(IBinaryTreeNode *pNode)			
{
	pNode->Remove(&m_pRoot);
}


// debug member functions
//--------------------------------------------------------------------------------
void CBinaryTree::DebugDrawTree()
{
	if(m_pRoot)
		m_pRoot->DebugDrawTree(0, 'O');
	printf("\n");	
}

//--------------------------------------------------------------------------------
void CBinaryTree::DebugDrawAscending()
{
	if(m_pRoot)
		m_pRoot->DebugDrawAscending();
	printf("\n");
}