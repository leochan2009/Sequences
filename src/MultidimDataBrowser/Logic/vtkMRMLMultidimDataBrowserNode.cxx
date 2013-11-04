/*==============================================================================

  Program: 3D Slicer

  Copyright (c) Kitware Inc.

  See COPYRIGHT.txt
  or http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Csaba Pinter, PerkLab, Queen's University
  and was supported through the Applied Cancer Research Unit program of Cancer Care
  Ontario with funds provided by the Ontario Ministry of Health and Long-Term Care

==============================================================================*/

// MRMLMultidimDataBrowser includes
#include "vtkMRMLMultidimDataBrowserNode.h"

// MRML includes
#include <vtkMRMLScene.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkMRMLHierarchyNode.h>

// VTK includes
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

// STD includes
#include <sstream>

static const char* ROOT_NODE_REFERENCE_ROLE = "root";
static const char* ROOT_NODE_REFERENCE_ATTRIBUTE_NAME = "rootNodeRef"; 
static const char* SELECTED_SEQUENCE_NODE_REFERENCE_ROLE = "selectedSequence";
static const char* SELECTED_SEQUENCE_NODE_REFERENCE_ATTRIBUTE_NAME = "selectedSequenceNodeRef"; 
static const char* VIRTUAL_OUTPUT_NODE_REFERENCE_ROLE = "virtualOutput";
static const char* VIRTUAL_OUTPUT_NODE_REFERENCE_ATTRIBUTE_NAME = "virtualOutputNodeRef"; 

//------------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLMultidimDataBrowserNode);

//----------------------------------------------------------------------------
vtkMRMLMultidimDataBrowserNode::vtkMRMLMultidimDataBrowserNode()
{
  this->SetHideFromEditors(false);
  this->AddNodeReferenceRole(ROOT_NODE_REFERENCE_ROLE, ROOT_NODE_REFERENCE_ATTRIBUTE_NAME );
  this->AddNodeReferenceRole(SELECTED_SEQUENCE_NODE_REFERENCE_ROLE, SELECTED_SEQUENCE_NODE_REFERENCE_ATTRIBUTE_NAME ); 
  this->AddNodeReferenceRole(VIRTUAL_OUTPUT_NODE_REFERENCE_ROLE, VIRTUAL_OUTPUT_NODE_REFERENCE_ATTRIBUTE_NAME ); 
  this->PlaybackActive=false;
  this->PlaybackRateFps=5.0;
  this->PlaybackLooped=true;
}

//----------------------------------------------------------------------------
vtkMRMLMultidimDataBrowserNode::~vtkMRMLMultidimDataBrowserNode()
{
 
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::WriteXML(ostream& of, int nIndent)
{
  Superclass::WriteXML(of, nIndent);
  vtkIndent indent(nIndent);
  of << indent << " playbackActive=\"" << (this->PlaybackActive ? "true" : "false") << "\"";
  of << indent << " playbackRateFps=\"" << this->PlaybackRateFps << "\""; 
  of << indent << " playbackLooped=\"" << (this->PlaybackLooped ? "true" : "false") << "\"";  
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::ReadXMLAttributes(const char** atts)
{
  vtkMRMLNode::ReadXMLAttributes(atts);
}

//----------------------------------------------------------------------------
// Copy the node's attributes to this object.
// Does NOT copy: ID, FilePrefix, Name, VolumeID
void vtkMRMLMultidimDataBrowserNode::Copy(vtkMRMLNode *anode)
{
  Superclass::Copy(anode);
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkMRMLNode::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::SetAndObserveRootNodeID(const char *rootNodeID)
{
  this->SetAndObserveNodeReferenceID(ROOT_NODE_REFERENCE_ROLE, rootNodeID);
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::SetAndObserveSelectedSequenceNodeID(const char *selectedSequenceNodeID)
{
  this->SetAndObserveNodeReferenceID(SELECTED_SEQUENCE_NODE_REFERENCE_ROLE, selectedSequenceNodeID);
}

//----------------------------------------------------------------------------
void vtkMRMLMultidimDataBrowserNode::SetAndObserveVirtualOutputNodeID(const char *virtualOutputNodeID)
{
  this->SetAndObserveNodeReferenceID(VIRTUAL_OUTPUT_NODE_REFERENCE_ROLE, virtualOutputNodeID);
}

//----------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkMRMLMultidimDataBrowserNode::GetRootNode()
{
  vtkMRMLHierarchyNode* node=vtkMRMLHierarchyNode::SafeDownCast(this->GetNodeReference(ROOT_NODE_REFERENCE_ROLE));
  return node;
}

//----------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkMRMLMultidimDataBrowserNode::GetSelectedSequenceNode()
{
  vtkMRMLHierarchyNode* node=vtkMRMLHierarchyNode::SafeDownCast(this->GetNodeReference(SELECTED_SEQUENCE_NODE_REFERENCE_ROLE));
  return node;
}
  
//----------------------------------------------------------------------------
vtkMRMLHierarchyNode* vtkMRMLMultidimDataBrowserNode::GetVirtualOutputNode()
{
  vtkMRMLHierarchyNode* node=vtkMRMLHierarchyNode::SafeDownCast(this->GetNodeReference(VIRTUAL_OUTPUT_NODE_REFERENCE_ROLE));
  return node;
}