/*=auto=========================================================================
 
 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.
 
 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.
 
 =========================================================================auto=*/

#include <algorithm>

#include "vtkMRMLBitStreamSequenceStorageNode.h"

#include "vtkMRMLSequenceNode.h"

#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkStringArray.h"

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLBitStreamSequenceStorageNode);

//----------------------------------------------------------------------------
vtkMRMLBitStreamSequenceStorageNode::vtkMRMLBitStreamSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLBitStreamSequenceStorageNode::~vtkMRMLBitStreamSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
bool vtkMRMLBitStreamSequenceStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLSequenceNode");
}

//----------------------------------------------------------------------------
int vtkMRMLBitStreamSequenceStorageNode::ReadDataInternal(vtkMRMLNode* refNode)
{
  if (!this->CanReadInReferenceNode(refNode))
  {
    return 0;
  }
  
  vtkMRMLSequenceNode* volSequenceNode = dynamic_cast<vtkMRMLSequenceNode*>(refNode);
  if (!volSequenceNode)
  {
    vtkErrorMacro("ReadDataInternal: not a Sequence node.");
    return 0;
  }
  
  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
  {
    vtkErrorMacro("ReadData: File name not specified");
    return 0;
  }
  
  FILE* reader = fopen(fullName.c_str(),"rb");
  
  // Check if this is a NRRD file that we can read
  if (reader == NULL)
  {
    vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode: This is not a text file");
    return 0;
  }
  
  // success
  return 1;
}

//----------------------------------------------------------------------------
bool vtkMRMLBitStreamSequenceStorageNode::CanWriteFromReferenceNode(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* volSequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (volSequenceNode == NULL)
  {
    vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode::CanWriteFromReferenceNode: invalid volSequenceNode");
    return false;
  }
  if (volSequenceNode->GetNumberOfDataNodes() == 0)
  {
    vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode::CanWriteFromReferenceNode: no data nodes");
    return false;
  }
  
  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLBitStreamSequenceStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* volSequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (volSequenceNode==NULL)
  {
    vtkErrorMacro(<< "vtkMRMLBitStreamSequenceStorageNode::WriteDataInternal: Do not recognize node type " << refNode->GetClassName());
    return 0;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamSequenceStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedReadFileTypes->InsertNextValue("Text (.txt)");
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamSequenceStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Text (.txt)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLBitStreamSequenceStorageNode::GetDefaultWriteFileExtension()
{
  return ".txt";
}
