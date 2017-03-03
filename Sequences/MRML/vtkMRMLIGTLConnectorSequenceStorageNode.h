/*=auto=========================================================================
 
 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.
 
 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.
 
 =========================================================================auto=*/
///  vtkMRMLIGTLConnectorSequenceStorageNode - MRML node that can read/write
///  a Sequence node containing IGTLConnector stream in a text file
///

#ifndef __vtkMRMLIGTLConnectorSequenceStorageNode_h
#define __vtkMRMLIGTLConnectorSequenceStorageNode_h

#include "vtkSlicerSequencesModuleMRMLExport.h"
#include "vtkMRMLBitStreamSequenceStorageNode.h"
#include "vtkMRMLVolumeSequenceStorageNode.h"
#include "vtkMRMLLinearTransformSequenceStorageNode.h"
#include "vtkMRMLStorageNode.h"
#include <string>

// OpenIGTLinkIF node include
#include "vtkIGTLToMRMLBase.h"
#include "vtkMRMLBitStreamNode.h"
#include "vtkMRMLIGTLConnectorNode.h"
#include "vtkIGTLToMRMLLinearTransform.h"
#include "vtkIGTLToMRMLTrackingData.h"
#include "vtkMRMLIGTLTrackingDataBundleNode.h"
#include "vtkIGTLToMRMLImage.h"
#include "vtkIGTLToMRMLVideo.h"

/// \ingroup Slicer_QtModules_Sequences
class VTK_SLICER_SEQUENCES_MODULE_MRML_EXPORT vtkMRMLIGTLConnectorSequenceStorageNode : public vtkMRMLStorageNode
{
public:
  
  static vtkMRMLIGTLConnectorSequenceStorageNode *New();
  vtkTypeMacro(vtkMRMLIGTLConnectorSequenceStorageNode,vtkMRMLStorageNode);
  
  virtual vtkMRMLNode* CreateNodeInstance();
  
  ///
  /// Get node XML tag name (like Storage, Model)
  virtual const char* GetNodeTagName()  {return "IGTLConnectorSequenceStorage";};
  
  /// Return true if the node can be read in.
  virtual bool CanReadInReferenceNode(vtkMRMLNode *refNode);
  
  /// Return true if the node can be written by using thie writer.
  virtual bool CanWriteFromReferenceNode(vtkMRMLNode* refNode);
  
  /// Return a default file extension for writting
  virtual const char* GetDefaultWriteFileExtension();
  
  int CheckNodeExist(vtkMRMLScene* scene, const char* classname, igtl::MessageBase::Pointer buffer);
  
protected:
  vtkMRMLIGTLConnectorSequenceStorageNode();
  ~vtkMRMLIGTLConnectorSequenceStorageNode();
  vtkMRMLIGTLConnectorSequenceStorageNode(const vtkMRMLIGTLConnectorSequenceStorageNode&);
  void operator=(const vtkMRMLIGTLConnectorSequenceStorageNode&);
  
  vtkMRMLLinearTransformSequenceStorageNode* transfromStorageNode;
  
  vtkMRMLVolumeSequenceStorageNode* volumeStorageNode;
  
  vtkMRMLBitStreamSequenceStorageNode* bitStreamStorageNode;
  
  virtual int WriteDataInternal(vtkMRMLNode *refNode);
  
  int WriteTransfromNode();

  /// Does the actual reading. Returns 1 on success, 0 otherwise.
  /// Returns 0 by default (read not supported).
  /// This implementation delegates most everything to the superclass
  /// but it has an early exit if the file to be read is incompatible.
  virtual int ReadDataInternal(vtkMRMLNode* refNode);
  
  /// Initialize all the supported write file types
  virtual void InitializeSupportedReadFileTypes();
  
  /// Initialize all the supported write file types
  virtual void InitializeSupportedWriteFileTypes();
  
  /// String Operation
  int GetTagValue(char* headerString, int headerLenght, const char* tag, int tagLength, std::string &tagValueString, int&tagValueLength);
};

#endif