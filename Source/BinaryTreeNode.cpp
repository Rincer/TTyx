#include "stdafx.h"
#include "Macros.h"

#include "BinaryTreeNode.h"

//--------------------------------------------------------------------------------
IBinaryTreeNode::IBinaryTreeNode()
{
	m_pParent = NULL;			
	m_pLeft = NULL;					
	m_pRight = NULL;							
}

//--------------------------------------------------------------------------------
void IBinaryTreeNode::Add(IBinaryTreeNode *pNode)
{
	if(pNode->GetValue() > GetValue())
	{
		if(m_pRight)
		{
			m_pRight->Add(pNode);
		}
		else
		{
			m_pRight = pNode;
			pNode->m_pParent = this;
		}
	}
	else
	{
		if(m_pLeft)
		{
			m_pLeft->Add(pNode);
		}
		else
		{
			m_pLeft = pNode;
			pNode->m_pParent = this;
		}	
	}
}

//--------------------------------------------------------------------------------
// most complicated case
void IBinaryTreeNode::RemoveHasBothChildren(IBinaryTreeNode** pRoot)	
{
	//  has 2 children, find the rightmost node of the left subtree, remove it from the tree, and replace pNode with it
	IBinaryTreeNode* pRightmost = m_pLeft->FindRightmost();	// rightmost node of the left subtree
	pRightmost->Remove(pRoot);
	ReplaceWith(pRightmost);
	RemoveAtParent(pRoot, pRightmost);		
}		
								
//--------------------------------------------------------------------------------
void IBinaryTreeNode::ReplaceWith(IBinaryTreeNode* pNode)
{
	pNode->m_pParent = m_pParent;
	pNode->m_pRight = m_pRight;	
	if(m_pRight)
		m_pRight->m_pParent = pNode;	
	pNode->m_pLeft = m_pLeft;			
	if(m_pLeft)
		m_pLeft->m_pParent = pNode;		
}

//--------------------------------------------------------------------------------
void IBinaryTreeNode::RemoveAtParent(IBinaryTreeNode** pRoot, IBinaryTreeNode* pNewChild)
{
	if(pNewChild)
		pNewChild->m_pParent = m_pParent;
	if(m_pParent)
	{
		if(m_pParent->m_pLeft == this)
		{
			m_pParent->m_pLeft = pNewChild;
		}
		else
		{
			m_pParent->m_pRight = pNewChild;
		}		
	}
	else // removing a root node
		*pRoot = pNewChild;		
}

//--------------------------------------------------------------------------------
void IBinaryTreeNode::Remove(IBinaryTreeNode** pRoot)
{
	// pNode has no children so remove it from the parents' node
	if((m_pLeft == NULL) && (m_pRight == NULL))
	{	
		RemoveAtParent(pRoot, NULL);				
	}		
	//  has only right child, so propagate the child to this nodes parent	
	else if((m_pLeft == NULL) && (m_pRight != NULL))
	{
		RemoveAtParent(pRoot, m_pRight);					
	}	
	//  has only left child, so propagate the child to this nodes parent		
	else if((m_pLeft != NULL) && (m_pRight == NULL))	
	{
		RemoveAtParent(pRoot, m_pLeft);						
	}	
	else
		RemoveHasBothChildren(pRoot);		
}


//--------------------------------------------------------------------------------
static void Pad(int Indent)
{	
	for( int i = 0; i < Indent; i++)
	{
		printf("-");
	}
}

//--------------------------------------------------------------------------------
void IBinaryTreeNode::DebugDrawTree(int Indent, char LR)
{
	Pad(Indent);
	printf("%c%d\n", LR, GetValue());
	if(m_pRight)
		m_pRight->DebugDrawTree(Indent + 4, 'R');
	if(m_pLeft)
		m_pLeft->DebugDrawTree(Indent + 4, 'L');
}

//--------------------------------------------------------------------------------
void IBinaryTreeNode::DebugDrawAscending()
{
	if(m_pLeft)
		m_pLeft->DebugDrawAscending();
	printf("%d ", GetValue());
	if(m_pRight)
		m_pRight->DebugDrawAscending();
}

//--------------------------------------------------------------------------------
IBinaryTreeNode* IBinaryTreeNode::FindRightmost()
{
	IBinaryTreeNode* pNode = this;
	while(pNode->m_pRight)
		pNode = pNode->m_pRight;
	return pNode;
}

