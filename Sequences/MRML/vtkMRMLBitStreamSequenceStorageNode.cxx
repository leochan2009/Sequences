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



int vtkMRMLBitStreamSequenceStorageNode::GetTagValue(char* headerString, int headerLenght, const char* tag, int tagLength, std::string &tagValueString, int&tagValueLength)
{
  int beginIndex = -1;
  int endIndex = -1;
  int index = 0;
  for(index = 0; index < headerLenght; index ++ )
  {
    if (index < headerLenght -tagLength)
    {
      std::string stringTemp(&(headerString[index]), &(headerString[index + tagLength]));
      if(strcmp(stringTemp.c_str(),tag)==0)
      {
        beginIndex = index+tagLength+2;
      }
    }
    std::string stringTemp2(&(headerString[index]), &(headerString[index + 1]));
    if(beginIndex>=0 && (strcmp(stringTemp2.c_str(), "\n") == 0))
    {
      endIndex = index;
      break;
    }
  }
  if(beginIndex>=0 &&(endIndex>beginIndex))
  {
    tagValueString = std::string(&(headerString[beginIndex]), &(headerString[endIndex]));
    return 1;
  }
  else
  {
    return 0;
  }
}

std::string vtkMRMLBitStreamSequenceStorageNode::GetValueByDelimiter(std::string &inputString, std::string delimiter, int index)
{
  size_t pos = 0;
  std::string token;
  int tokenIndex = 0;
  while ((pos = inputString.find(delimiter)) != std::string::npos) {
    token = inputString.substr(0, pos);
    //std::cout << token << std::endl;
    inputString.erase(0, pos + delimiter.length());
    tokenIndex++;
    if (tokenIndex == index)
    {
      return token;
    }
  }
  return NULL;
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
  
  // Check if this is a  file that we can read
  if (stream == NULL)
  {
    vtkDebugMacro("vtkMRMLBitStreamSequenceStorageNode: This is not a text file");
    return 0;
  }
  std::string data("  ");
  int headerLength = 0;
  int temp = 1;
  while(fread(&data[0],2,1,stream)){
    fseek(stream, -1, SEEK_CUR);
    temp ++;
    if (strcmp(data.c_str(),"\n\n")==0)
    {
      headerLength = temp;
      break;
    }
  }
  char * headerString = new char[headerLength];
  fread(headerString, headerLength,1,stream);
  bool fileValid = true;
  std::string tagValueString("");
  int tagValueLength;
  if(GetTagValue(headerString, headerLength, "ObjectType", 10, tagValueString, tagValueLength))
  {
    if (strcmp(tagValueString.c_str(), "BitStream")==0)
    {
      fileValid *= true;
    }
    else
    {
      fileValid = false;
    }
  }
  if(GetTagValue(headerString, headerLength, "Codec", 5, tagValueString, tagValueLength))
  {
    if (strcmp(tagValueString.c_str(), "H264")==0)
    {
      fileValid *= true;
    }
    else
    {
      fileValid = false;
    }
  }
  std::string volumeName = "";
  if(GetTagValue(headerString, headerLength, "VolumeName", 10, tagValueString, tagValueLength))
  {
    fileValid *= true;
    volumeName = tagValueString;
  }
  else
  {
    fileValid = false;
  }
  if(fileValid)
  {
    vtkMRMLBitStreamNode * frameProxyNode = vtkMRMLBitStreamNode::New();
    frameProxyNode->SetUpVolumeAndConverter(volumeName.c_str());
    char* data= (char *)" ";
    char* timeStamp = NULL;
    temp = 0;
    int dataLength = 0;
    bool proxyNodeSet = false;
    bool timeStampFound = false;
    bool frameMessageFound = false;
    vtkMRMLBitStreamNode * frameNode;
    while(fread(&data[0],1,1,stream))
    {
      if (strcmp(data,"\n")==0 && dataLength && (!timeStampFound))
      {
        timeStamp = new char[dataLength];
        fseek(stream, -dataLength, SEEK_CUR);
        fread(timeStamp, dataLength, 1, stream);
        timeStampFound = true;
        dataLength = 0;
        continue;
      }
      if (strcmp(data,"\n")==0 && dataLength  && (!frameMessageFound))
      {
        if(!proxyNodeSet)
        {
          frameNode = frameProxyNode;
          proxyNodeSet = true;
        }
        else
        {
          frameNode = vtkMRMLBitStreamNode::New();
          frameNode->SetVectorVolumeNode(frameProxyNode->GetVectorVolumeNode());
        }
        timeStamp = new char[dataLength];
        fseek(stream, -dataLength, SEEK_CUR);
        fread(frameNode->GetMessageStreamBuffer()->GetPackPointer(), dataLength, 1, stream);
        frameMessageFound = true;
        timeStampFound = false; // time stamp should be found first
        dataLength = 0;
        continue;
      }
      if(timeStampFound && frameMessageFound)
      {
        volSequenceNode->SetDataNodeAtValue(frameNode, std::string(timeStamp));
        delete timeStamp;
        timeStamp = NULL;
        timeStampFound = false;
        frameMessageFound = false;
        dataLength = 0;
      }
      dataLength++;
    }
    
  }
  
  
  
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
  char* volumeName = (char*)"";
  if (bitStreamSequenceNode->GetNumberOfDataNodes()>0)
  {
    vtkMRMLBitStreamNode* frameBitStream = vtkMRMLBitStreamNode::SafeDownCast(bitStreamSequenceNode->GetNthDataNode(0));
    if (frameBitStream==NULL)
    {
      vtkErrorMacro(<< "vtkMRMLBitStreamSequenceStorageNode::WriteDataInternal: Data node is not a bit stream");
      return 0;
    }
    volumeName = frameBitStream->GetVectorVolumeNode()->GetName();
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
  << "ObjectType: BitStream" << std::endl
  << "Codec: H264" << std::endl
  << "VolumeName: " << volumeName << std::endl;
  // Append the bit stream to the end of the file
  std::ofstream outStream(fullName.c_str(), std::ios_base::binary);
  outStream << defaultHeaderOutStream.str();
  outStream << std::setfill('0');
  outStream << std::endl;
  outStream << std::endl;
  int numberOfFrameBitStreams = bitStreamSequenceNode->GetNumberOfDataNodes();
  for (int frameIndex=0; frameIndex<numberOfFrameBitStreams; frameIndex++)
  {
    vtkMRMLBitStreamNode* frameBitStream = vtkMRMLBitStreamNode::SafeDownCast(bitStreamSequenceNode->GetNthDataNode(frameIndex));
    std::string timeStamp = bitStreamSequenceNode->GetNthIndexValue(frameIndex);
    if (frameBitStream!=NULL && frameBitStream->GetMessageValid()>0 && timeStamp.size())
    {
      outStream << frameIndex << std::setw(0);
      std::string timeStamp = bitStreamSequenceNode->GetNthIndexValue(frameIndex);
      char* messageStream = (char*)frameBitStream->GetMessageStreamBuffer()->GetPackPointer();
      int messageLength = frameBitStream->GetMessageStreamBuffer()->GetPackSize();
      outStream.write(timeStamp.c_str(), timeStamp.size());
      outStream << std::endl;
      outStream.write(messageStream, messageLength);
      outStream << std::endl;
    }
  }
  
  outStream.close();
  
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
