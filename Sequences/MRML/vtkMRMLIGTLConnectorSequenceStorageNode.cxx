/*=auto=========================================================================
 
 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.
 
 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.
 
 =========================================================================auto=*/
#include <algorithm>

#include "vtkMRMLIGTLConnectorSequenceStorageNode.h"
#include "vtkMRMLSequenceNode.h"

#include "vtkMRMLIGTLConnectorNode.h"

#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkCollection.h"

#include "vtksys/SystemTools.hxx"

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLIGTLConnectorSequenceStorageNode);

// Add the helper functions

//-------------------------------------------------------
inline void Trim(std::string &str)
{
  str.erase(str.find_last_not_of(" \t\r\n") + 1);
  str.erase(0, str.find_first_not_of(" \t\r\n"));
}


//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorSequenceStorageNode::vtkMRMLIGTLConnectorSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLIGTLConnectorSequenceStorageNode::~vtkMRMLIGTLConnectorSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorSequenceStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLSequenceNode");
}



int vtkMRMLIGTLConnectorSequenceStorageNode::GetTagValue(char* headerString, int headerLenght, const char* tag, int tagLength, std::string &tagValueString, int&tagValueLength)
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

//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorSequenceStorageNode::ReadDataInternal(vtkMRMLNode* refNode)
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
    vtkDebugMacro("vtkMRMLIGTLConnectorSequenceStorageNode: This is not a text file");
    return 0;
  }
  std::string data("  ");
  int headerLength = 0;
  int temp = 1;
  while(fread(&data[0],2,1,stream)){
    fseek(stream, -1, SEEK_CUR);
    headerLength ++;
    if (strcmp(data.c_str(),"\n\n")==0)
    {
      fseek(stream, -headerLength, SEEK_CUR);
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
    if (strcmp(tagValueString.c_str(), "IGTLConnector")==0)
    {
      fileValid *= true;
    }
    else
    {
      fileValid = false;
    }
  }
  std::string connectorName = "";
  if(GetTagValue(headerString, headerLength, "ConnectorName", 13, tagValueString, tagValueLength))
  {
    fileValid *= true;
    connectorName = tagValueString;
  }
  else
  {
    fileValid = false;
  }
  fread(headerString, 2,1,stream); // get rid of the following two line breaks
  if(fileValid)
  {
    
    if(this->GetScene())
    {
      vtkMRMLIGTLConnectorNode * frameProxyNode = NULL;
      vtkCollection* collection =  this->GetScene()->GetNodesByClassByName("vtkMRMLIGTLConnectorNode",connectorName.c_str());
      int nCol = collection->GetNumberOfItems();
      if (nCol > 0)
      {
        frameProxyNode = vtkMRMLIGTLConnectorNode::SafeDownCast(collection->GetItemAsObject(0));
      }
      else
      {
        frameProxyNode = vtkMRMLIGTLConnectorNode::New();
        this->GetScene()->AddNode(frameProxyNode);
        frameProxyNode->SetName(connectorName.c_str());
      }
      //frameProxyNode->SetUpMRMLNodeAndConverter(volumeName.c_str());
      bool proxyNodeSet = false;
      while(1)
      {
        temp = 0;
        int stringLineLength = 0;
        data.clear();
        data = std::string(" ");
        bool bCanBeRead = true;
        while(bCanBeRead)
        {
          bCanBeRead = fread(&data[0],1,1,stream);
          stringLineLength++;
          if (strcmp(data.c_str(),"\n")==0)
          {
            fseek(stream, -stringLineLength, SEEK_CUR);
            break;
          }
        }
        if (!bCanBeRead) break;
        char *lineString = new char[stringLineLength];
        fread(lineString,stringLineLength,1,stream);
        std::string stringOperator(lineString);
        size_t pos = 0;
        int messageLength = 0;
        if ((pos = stringOperator.find(":")) != std::string::npos)
        {
          std::string timeStamp(lineString,pos);
          std::string stringMsgLength(lineString+pos,stringLineLength-pos);
          stringMsgLength.erase(stringMsgLength.find_last_not_of(" :\t\r\n") + 1);
          stringMsgLength.erase(0, stringMsgLength.find_first_not_of(" :\t\r\n")); // Get rid of ":"
          stringMsgLength.erase(0, stringMsgLength.find_first_not_of(" :\t\r\n")); // Get rid of " "
          messageLength = atoi(stringMsgLength.c_str());
          vtkMRMLIGTLConnectorNode * frameNode;
          if(!proxyNodeSet)
          {
            frameNode = frameProxyNode;
            proxyNodeSet = true;
          }
          else
          {
            frameNode = vtkMRMLIGTLConnectorNode::New();
          }
          frameNode->CurrentIGTLMessage = new igtl_uint8[messageLength];
          fread(frameNode->CurrentIGTLMessage , messageLength, 1, stream);
          frameNode->messageLength = messageLength;
          volSequenceNode->SetDataNodeAtValue(frameNode, std::string(timeStamp));
        }
      }
    }
  }
  
  return 1;
}

//----------------------------------------------------------------------------
bool vtkMRMLIGTLConnectorSequenceStorageNode::CanWriteFromReferenceNode(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (sequenceNode == NULL)
  {
    vtkDebugMacro("vtkMRMLIGTLConnectorSequenceStorageNode::CanWriteFromReferenceNode: input is not a sequence node");
    return false;
  }
  if (sequenceNode->GetNumberOfDataNodes() == 0)
  {
    vtkDebugMacro("vtkMRMLIGTLConnectorSequenceStorageNode::CanWriteFromReferenceNode: no data nodes");
    return false;
  }
  int numberOfFrameVolumes = sequenceNode->GetNumberOfDataNodes();
  for (int frameIndex = 0; frameIndex < numberOfFrameVolumes; frameIndex++)
  {
    vtkMRMLIGTLConnectorNode* igtlConnectorNode = vtkMRMLIGTLConnectorNode::SafeDownCast(sequenceNode->GetNthDataNode(frameIndex));
    if (igtlConnectorNode == NULL)
    {
      vtkDebugMacro("vtkMRMLIGTLConnectorSequenceStorageNode::CanWriteFromReferenceNode: stream nodes has not bit stream (frame " << frameIndex << ")");
      return false;
    }
  }
  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLIGTLConnectorSequenceStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* igtlConnectorSequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (igtlConnectorSequenceNode==NULL)
  {
    vtkErrorMacro(<< "vtkMRMLIGTLConnectorSequenceStorageNode::WriteDataInternal: Do not recognize node type " << refNode->GetClassName());
    return 0;
  }
  char* connectorName = (char*)"";
  if (igtlConnectorSequenceNode->GetNumberOfDataNodes()>0)
  {
    vtkMRMLIGTLConnectorNode* frameIGTLConnector = vtkMRMLIGTLConnectorNode::SafeDownCast(igtlConnectorSequenceNode->GetNthDataNode(0));
    if (frameIGTLConnector==NULL)
    {
      vtkErrorMacro(<< "vtkMRMLIGTLConnectorSequenceStorageNode::WriteDataInternal: Data node is not a bit stream");
      return 0;
    }
    connectorName = frameIGTLConnector->GetName();
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
  << "ObjectType: IGTLConnector" << std::endl
  << "ConnectorName: " << connectorName << std::endl;
  // Append the bit stream to the end of the file
  std::ofstream outStream(fullName.c_str(), std::ios_base::binary);
  outStream << defaultHeaderOutStream.str();
  outStream << std::setfill('0');
  outStream << std::endl;
  outStream << std::endl;
  int numberOfFrameBitStreams = igtlConnectorSequenceNode->GetNumberOfDataNodes();
  for (int frameIndex=0; frameIndex<numberOfFrameBitStreams; frameIndex++)
  {
    vtkMRMLIGTLConnectorNode* frameIGTLConnector = vtkMRMLIGTLConnectorNode::SafeDownCast(igtlConnectorSequenceNode->GetNthDataNode(frameIndex));
    std::string timeStamp = igtlConnectorSequenceNode->GetNthIndexValue(frameIndex);
    if (igtlConnectorSequenceNode!=NULL && frameIGTLConnector->CurrentIGTLMessage && timeStamp.size())
    {
      int messageLength = frameIGTLConnector->messageLength;
      //igtl::MessageBase::Pointer messageTarget = igtl::MessageBase::New();
      igtl_uint8* message = frameIGTLConnector->CurrentIGTLMessage;
      //messageTarget->SetMessageHeader(messageBase);
      //messageTarget->SetBitStreamSize(messageBase->GetBodySizeToRead()-sizeof(igtl_extended_header) -messageBase->GetMetaDataHeaderSize() - messageBase->GetMetaDataSize() - IGTL_VIDEO_HEADER_SIZE);
      //messageTarget->AllocateBuffer();
      //memcpy(messageTarget->GetPackBodyPointer(),(unsigned char*) messageBase->GetPackBodyPointer(),messageBase->GetBodySizeToRead());
      //int packStatus = messageTarget->Pack();
      outStream.write(timeStamp.c_str(), timeStamp.size());
      outStream <<": "<<messageLength;
      outStream << std::endl;
      outStream.write((char*)message, messageLength);
      outStream << std::endl;
    }
  }
  
  outStream.close();
  
  this->StageWriteData(refNode);
  
  return 1;
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorSequenceStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("IGTLConnector Message Binary (.bin)");
  this->SupportedWriteFileTypes->InsertNextValue("IGTLConnector Message Binary (.seq.bin)");
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorSequenceStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("IGTLConnector Message Binary (.bin)");
  this->SupportedWriteFileTypes->InsertNextValue("IGTLConnector Message Binary (.seq.bin)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLIGTLConnectorSequenceStorageNode::GetDefaultWriteFileExtension()
{
  return "bin";
}
