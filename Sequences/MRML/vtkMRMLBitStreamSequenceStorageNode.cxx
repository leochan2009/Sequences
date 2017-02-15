/*=auto=========================================================================
 
 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.
 
 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.
 
 =========================================================================auto=*/

#include <algorithm>

#include "vtkMRMLBitStreamSequenceStorageNode.h"
#include "vtkMRMLSequenceNode.h"

#include "vtkMRMLBitStreamNode.h"

#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkStringArray.h"

#include "vtksys/SystemTools.hxx"

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLBitStreamSequenceStorageNode);

// Add the helper functions

//-------------------------------------------------------
inline void Trim(std::string &str)
{
  str.erase(str.find_last_not_of(" \t\r\n") + 1);
  str.erase(0, str.find_first_not_of(" \t\r\n"));
}


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
  
  FILE* stream = fopen(fullName.c_str(),"rb");
  
  // Check if this is a NRRD file that we can read
  if (stream == NULL)
  {
    vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode: This is not a text file");
    return 0;
  }
  const int MAX_LINE_LENGTH = 100000000;
  
  char line[MAX_LINE_LENGTH + 1] = { 0 };
  /*
  while (fgets(line, MAX_LINE_LENGTH, stream))
  {
    std::string lineStr = line;
    
    // Split line into name and value
    size_t equalSignFound = 0;
    equalSignFound = lineStr.find_first_of("=");
    if (equalSignFound == std::string::npos)
    {
      vtkGenericWarningMacro("Parsing line failed, equal sign is missing (" << lineStr << ")");
      continue;
    }
    std::string name = lineStr.substr(0, equalSignFound);
    std::string value = lineStr.substr(equalSignFound + 1);
    
    // Trim spaces from the left and right
    Trim(name);
    Trim(value);
    
    // Only consider the Seq_Frame
    if (strcmp(name.c_str(),"Seq_Frame") != 0)
    {
      continue;
    }
    
    
    // frame field
    // name: Seq_Frame0000_CustomTransform
    name.erase(0, SEQMETA_FIELD_FRAME_FIELD_PREFIX.size()); // 0000_CustomTransform
    
    // Split line into name and value
    size_t underscoreFound;
    underscoreFound = name.find_first_of("_");
    if (underscoreFound == std::string::npos)
    {
      vtkGenericWarningMacro("Parsing line failed, underscore is missing from frame field name (" << lineStr << ")");
      continue;
    }
    
    std::string frameNumberStr = name.substr(0, underscoreFound); // 0000
    std::string frameFieldName = name.substr(underscoreFound + 1); // CustomTransform
    
    int frameNumber = 0;
    StringToInt(frameNumberStr.c_str(), frameNumber); // TODO: Removed warning
    if (frameNumber > lastFrameNumber)
    {
      lastFrameNumber = frameNumber;
    }
    
    // Convert the string to transform and add transform to hierarchy
    if (frameFieldName.find("Transform") != std::string::npos && frameFieldName.find("Status") == std::string::npos)
    {
      vtkNew<vtkMatrix4x4> matrix;
      bool success = vtkAddonMathUtilities::FromString(matrix.GetPointer(), value);
      if (!success)
      {
        continue;
      }
      vtkMRMLLinearTransformNode* currentTransform = vtkMRMLLinearTransformNode::New(); // will be deleted when added to the scene
      currentTransform->SetMatrixTransformToParent(matrix.GetPointer());
      // Generating a unique name is important because that will be used to generate the filename by default
      currentTransform->SetName(frameFieldName.c_str());
      importedTransformNodes[frameNumber].push_back(currentTransform);
    }
    
    if (frameFieldName.compare("Timestamp") == 0)
    {
      double timestampSec = atof(value.c_str());
      // round timestamp to 3 decimal digits, as timestamp is included in node names and having lots of decimal digits would
      // sometimes lead to extremely long node names
      std::ostringstream timestampSecStr;
      timestampSecStr << std::fixed << std::setprecision(3) << timestampSec << std::ends;
      frameNumberToIndexValueMap[frameNumber] = timestampSecStr.str();
    }
    
    if (ferror(stream))
    {
      vtkGenericWarningMacro("Error reading the file " << fileName.c_str());
      break;
    }
    if (feof(stream))
    {
      break;
    }
    
  }
  fclose(stream);
  
  // Now add all the nodes to the scene
  
  std::map< std::string, vtkMRMLSequenceNode* > transformSequenceNodes;
  
  for (int currentFrameNumber = 0; currentFrameNumber <= lastFrameNumber; currentFrameNumber++)
  {
    std::map<int, std::vector<vtkMRMLLinearTransformNode*> >::iterator transformsForCurrentFrame = importedTransformNodes.find(currentFrameNumber);
    if (transformsForCurrentFrame == importedTransformNodes.end())
    {
      // no transforms for this frame
      continue;
    }
    std::string paramValueString = frameNumberToIndexValueMap[currentFrameNumber];
    for (std::vector<vtkMRMLLinearTransformNode*>::iterator transformIt = transformsForCurrentFrame->second.begin(); transformIt != transformsForCurrentFrame->second.end(); ++transformIt)
    {
      vtkMRMLLinearTransformNode* transform = (*transformIt);
      vtkMRMLSequenceNode* transformsSequenceNode = NULL;
      if (transformSequenceNodes.find(transform->GetName()) == transformSequenceNodes.end())
      {
        // Setup hierarchy structure
        vtkMRMLSequenceNode* newTransformsSequenceNode = NULL;
        if (numberOfCreatedNodes < createdNodes.size())
        {
          // reuse supplied sequence node
          newTransformsSequenceNode = createdNodes[numberOfCreatedNodes];
          newTransformsSequenceNode->RemoveAllDataNodes();
        }
        else
        {
          // Create new sequence node
          newTransformsSequenceNode = vtkMRMLSequenceNode::New();
          createdNodes.push_back(newTransformsSequenceNode);
        }
        numberOfCreatedNodes++;
        transformsSequenceNode = newTransformsSequenceNode;
        transformsSequenceNode->SetIndexName("time");
        transformsSequenceNode->SetIndexUnit("s");
        std::string transformName = transform->GetName();
        // Strip "Transform" from the end of the transform name
        std::string transformPostfix = "Transform";
        if (transformName.length() > transformPostfix.length() &&
            transformName.compare(transformName.length() - transformPostfix.length(),
                                  transformPostfix.length(), transformPostfix) == 0)
        {
          // ends with "Transform" (SomethingToSomethingElseTransform),
          // remove it (to have SomethingToSomethingElse)
          transformName.erase(transformName.length() - transformPostfix.length(), transformPostfix.length());
        }
        // Save transform name to Sequences.Source attribute so that modules can
        // find a transform by matching the original the transform name.
        transformsSequenceNode->SetAttribute("Sequences.Source", transformName.c_str());
        
        transformSequenceNodes[transform->GetName()] = transformsSequenceNode;
      }
      else
      {
        transformsSequenceNode = transformSequenceNodes[transform->GetName()];
      }
      transform->SetHideFromEditors(false);
      // Generating a unique name is important because that will be used to generate the filename by default
      std::ostringstream nameStr;
      nameStr << transform->GetName() << "_" << std::setw(4) << std::setfill('0') << currentFrameNumber << std::ends;
      transform->SetName(nameStr.str().c_str());
      transformsSequenceNode->SetDataNodeAtValue(transform, paramValueString.c_str());
      transform->Delete(); // ownership transferred to the sequence node
    }
  }
  
  // Add to scene and set name and storage node
  std::string fileNameName = vtksys::SystemTools::GetFilenameName(fileName);
  std::string shortestBaseNodeName;
  int transformNodeIndex = 0;
  for (std::deque< vtkSmartPointer<vtkMRMLSequenceNode> >::iterator createdTransformNodeIt = createdNodes.begin();
       createdTransformNodeIt != createdNodes.end() && transformNodeIndex < numberOfCreatedNodes; ++createdTransformNodeIt, transformNodeIndex++)
  {
    // strip known file extensions from filename to get base name
    std::string transformName = (*createdTransformNodeIt)->GetAttribute("Sequences.Source") ?
    (*createdTransformNodeIt)->GetAttribute("Sequences.Source") : "";
    std::string baseNodeName = vtkMRMLSequenceStorageNode::GetSequenceBaseName(fileNameName, transformName);
    if (shortestBaseNodeName.empty() || baseNodeName.size() < shortestBaseNodeName.size())
    {
      shortestBaseNodeName = baseNodeName;
    }
    std::string transformsSequenceName = vtkMRMLSequenceStorageNode::GetSequenceNodeName(baseNodeName, transformName);
    (*createdTransformNodeIt)->SetName(transformsSequenceName.c_str());
    if (scene && (*createdTransformNodeIt)->GetScene() == NULL)
    {
      scene->AddNode(*createdTransformNodeIt);
    }
    if ((*createdTransformNodeIt)->GetScene())
    {
      // Add/initialize storage node
      if (!(*createdTransformNodeIt)->GetStorageNode())
      {
        (*createdTransformNodeIt)->AddDefaultStorageNode();
      }
      if (numberOfCreatedNodes == 1)
      {
        // Only one transform is stored in this file. Update stored time to mark the file as not modified since read.
        vtkMRMLLinearTransformSequenceStorageNode* storageNode =
        vtkMRMLLinearTransformSequenceStorageNode::SafeDownCast((*createdTransformNodeIt)->GetStorageNode());
        if (storageNode)
        {
          // Only one transform is stored in this file. Update stored time to mark the file as not modified since read.
          storageNode->StoredTime->Modified();
        }
      }
    }
  }
     */
  // success
  return 1;
}

//----------------------------------------------------------------------------
bool vtkMRMLBitStreamSequenceStorageNode::CanWriteFromReferenceNode(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (sequenceNode == NULL)
  {
    vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode::CanWriteFromReferenceNode: input is not a sequence node");
    return false;
  }
  if (sequenceNode->GetNumberOfDataNodes() == 0)
  {
    vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode::CanWriteFromReferenceNode: no data nodes");
    return false;
  }
  int numberOfFrameVolumes = sequenceNode->GetNumberOfDataNodes();
  for (int frameIndex = 0; frameIndex < numberOfFrameVolumes; frameIndex++)
  {
    vtkMRMLBitStreamNode* bitstream = vtkMRMLBitStreamNode::SafeDownCast(sequenceNode->GetNthDataNode(frameIndex));
    if (bitstream == NULL || (bitstream->GetMessageValid()<=0))
    {
      vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode::CanWriteFromReferenceNode: stream nodes has not bit stream (frame " << frameIndex << ")");
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLBitStreamSequenceStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* bitStreamSequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (bitStreamSequenceNode==NULL)
  {
    vtkErrorMacro(<< "vtkMRMLBitStreamSequenceStorageNode::WriteDataInternal: Do not recognize node type " << refNode->GetClassName());
    return 0;
  }

  if (bitStreamSequenceNode->GetNumberOfDataNodes()>0)
  {
    vtkMRMLBitStreamNode* frameBitStream = vtkMRMLBitStreamNode::SafeDownCast(bitStreamSequenceNode->GetNthDataNode(0));
    if (frameBitStream==NULL)
    {
      vtkErrorMacro(<< "vtkMRMLBitStreamSequenceStorageNode::WriteDataInternal: Data node is not a bit stream");
      return 0;
    }
  }
  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string("") || vtksys::SystemTools::FileExists(fullName.c_str()))
  {
    vtkErrorMacro("WriteData: File name not specified");
    return 0;
  }
  // If header file exists then append transform info before element data file line
  // Append the transform information to the end of the file
  std::stringstream defaultHeaderOutStream;
  defaultHeaderOutStream
  << "ObjectType = BitStream" << std::endl
  << "Codec = H264" << std::endl;

  // Append the transform information to the end of the file
  std::ofstream headerOutStream(fullName.c_str(), std::ios_base::binary);
  headerOutStream << defaultHeaderOutStream.str();
  headerOutStream << std::setfill('0');
  
  int numberOfFrameBitStreams = bitStreamSequenceNode->GetNumberOfDataNodes();
  for (int frameIndex=0; frameIndex<numberOfFrameBitStreams; frameIndex++)
  {
    vtkMRMLBitStreamNode* frameBitStream = vtkMRMLBitStreamNode::SafeDownCast(bitStreamSequenceNode->GetNthDataNode(frameIndex));
    if (frameBitStream!=NULL || frameBitStream->GetMessageValid()>0)
    {
      headerOutStream << frameIndex << std::setw(0);
      char* messageStream = (char*)frameBitStream->GetMessageStreamBuffer()->GetPackPointer();
      int messageLength = frameBitStream->GetMessageStreamBuffer()->GetPackSize();
      headerOutStream.write(messageStream, messageLength);
      headerOutStream << std::endl;
    }
  }
  headerOutStream.close();
  
  this->StageWriteData(refNode);
  
  return 1;
}


//----------------------------------------------------------------------------
void vtkMRMLBitStreamSequenceStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Linear transform sequence (.seq.mhd)");
  this->SupportedWriteFileTypes->InsertNextValue("Linear transform sequence (.seq.mha)");
  this->SupportedWriteFileTypes->InsertNextValue("Linear transform sequence (.mhd)");
  this->SupportedWriteFileTypes->InsertNextValue("Linear transform sequence (.mha)");
}

//----------------------------------------------------------------------------
void vtkMRMLBitStreamSequenceStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Linear transform sequence (.seq.mhd)");
  this->SupportedWriteFileTypes->InsertNextValue("Linear transform sequence (.seq.mha)");
  this->SupportedWriteFileTypes->InsertNextValue("Linear transform sequence (.mhd)");
  this->SupportedWriteFileTypes->InsertNextValue("Linear transform sequence (.mha)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLBitStreamSequenceStorageNode::GetDefaultWriteFileExtension()
{
  return "seq.mha";
}
