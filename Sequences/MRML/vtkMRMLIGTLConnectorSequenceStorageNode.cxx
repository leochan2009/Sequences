/*=auto=========================================================================
 
 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.
 
 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.
 
 =========================================================================auto=*/
#include <algorithm>

#include "vtkMRMLIGTLConnectorSequenceStorageNode.h"
#include "vtkMRMLSequenceNode.h"


#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkStringArray.h"
#include "vtkCollection.h"
// VTK includes
#include <vtkAbstractTransform.h>
#include <vtkCommand.h>
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkMatrix4x4.h>
#include <vtkMRMLCameraNode.h>
#include <vtkMRMLMarkupsFiducialNode.h>
#include <vtkMRMLModelNode.h>
#include <vtkMRMLScalarVolumeDisplayNode.h>
#include <vtkMRMLSliceCompositeNode.h>
#include <vtkMRMLSliceNode.h>
#include <vtkMRMLSegmentationNode.h>
#include <vtkMRMLLinearTransformNode.h>
#include <vtkMRMLVectorVolumeDisplayNode.h>
#include <vtkMRMLViewNode.h>
#include <vtkMRMLVectorVolumeNode.h>
#include <vtkMRMLVolumeNode.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkPolyData.h>
#include <vtkMutexLock.h>


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
  
  vtkMRMLSequenceNode* sequenceNode = dynamic_cast<vtkMRMLSequenceNode*>(refNode);
  if (!sequenceNode)
  {
    vtkErrorMacro("ReadDataInternal: not a Sequence node.");
    return 0;
  }
  
  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string("") || (!(vtksys::SystemTools::FileExists(fullName.c_str()))))
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
        long messageLength = 0;
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
          igtl_uint8* IGTLMessage = new igtl_uint8[messageLength];
          fread(IGTLMessage, messageLength, 1, stream);
          frameNode->SetCurrentIGTLMessage(IGTLMessage, messageLength);
          sequenceNode->SetDataNodeAtValue(frameNode, std::string(timeStamp));
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
  int numberOfFrameConnectors = sequenceNode->GetNumberOfDataNodes();
  for (int frameIndex = 0; frameIndex < numberOfFrameConnectors; frameIndex++)
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

int vtkMRMLIGTLConnectorSequenceStorageNode::CheckNodeExist(vtkMRMLScene* scene, const char* classname, igtl::MessageBase::Pointer buffer)
{
  vtkCollection* collection = scene->GetNodesByClassByName(classname, buffer->GetDeviceName());
  //----------------------
  //GetNodesByClassByName() is buggy, vtkMRMLVectorVolumeNode.IsA("vtkMRMLScalarVolumeNode") == 1, so when ask for scalar volume, the collection contains a vector volume
  for (int i = 0; i < collection->GetNumberOfItems(); i ++)
  {
    vtkMRMLNode* node = vtkMRMLNode::SafeDownCast(collection->GetItemAsObject(i));
    if(strcmp(node->GetClassName(),classname)==0)
    {
      return 1;
    }
  }
  //-----------------------
  return 0;
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
  std::string fileName = fullName.substr(0,fullName.find("."));
  std::string bitStreamFileName = fileName.append("_bitstream");
  fileName = fullName.substr(0,fullName.find("."));
  std::string linearTransformFileName = fileName.append("_tracking");
  fileName = fullName.substr(0,fullName.find("."));
  std::string volumeFileName = fileName.append("_volume");
  if (fullName == std::string("") || vtksys::SystemTools::FileExists(fullName.c_str())
      || vtksys::SystemTools::FileExists(bitStreamFileName.c_str()) || vtksys::SystemTools::FileExists(linearTransformFileName.c_str())
      || vtksys::SystemTools::FileExists(volumeFileName.c_str()))
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
  std::ofstream outStream(fullName.c_str(), std::ios_base::binary);
  outStream << defaultHeaderOutStream.str();
  outStream << std::setfill('0');
  outStream << std::endl;
  outStream << std::endl;

  vtkIGTLToMRMLTrackingData* trackingConverter = vtkIGTLToMRMLTrackingData::New();
  vtkMRMLIGTLTrackingDataBundleNode* trackingBundleNode = NULL;
  std::deque<vtkMRMLSequenceNode*> transformSequenceNodes;
  std::deque<std::string> transformNames;
  vtkMRMLLinearTransformSequenceStorageNode* transformStorageNode = vtkMRMLLinearTransformSequenceStorageNode::New();
  
  vtkMRMLSequenceNode* volumeSequence = vtkMRMLSequenceNode::New();
  vtkIGTLToMRMLImage* imageConverter = vtkIGTLToMRMLImage::New();
  vtkMRMLVolumeNode* volumeNode = NULL;
  vtkMRMLVolumeSequenceStorageNode* volumeStorageNode = vtkMRMLVolumeSequenceStorageNode::New();
  
  vtkMRMLSequenceNode* bitStreamSequence = vtkMRMLSequenceNode::New();
  vtkIGTLToMRMLVideo* videoConverter = vtkIGTLToMRMLVideo::New();
  vtkMRMLVectorVolumeNode* vectorVolumeNode = NULL;
  vtkMRMLBitStreamNode* bitStreamNode = NULL;
  vtkMRMLBitStreamSequenceStorageNode* bitStreamStorageNode = vtkMRMLBitStreamSequenceStorageNode::New();
  bool hasBitStream = false, hasVolume = false, hasTransform=false;
  int numberOfFrames = igtlConnectorSequenceNode->GetNumberOfDataNodes();
  for (int frameIndex=0; frameIndex<numberOfFrames; frameIndex++)
  {
    vtkMRMLIGTLConnectorNode* frameIGTLConnector = vtkMRMLIGTLConnectorNode::SafeDownCast(igtlConnectorSequenceNode->GetNthDataNode(frameIndex));
    std::string timeStamp = igtlConnectorSequenceNode->GetNthIndexValue(frameIndex);
    if (igtlConnectorSequenceNode!=NULL && timeStamp.size())
    {
      outStream.write(timeStamp.c_str(), timeStamp.size());
      long messageLen = 0;
      unsigned char* message  = frameIGTLConnector->ExportCurrentMessage(messageLen);
      if(messageLen && message)
      {
        outStream <<": "<<messageLen;
        outStream << std::endl;
        outStream.write((char*)message, messageLen);
        outStream << std::endl;
        igtl::MessageHeader::Pointer headerMsg;
        headerMsg = igtl::MessageHeader::New();
        headerMsg->InitPack();
        memcpy(headerMsg->GetPackPointer(), message, IGTL_HEADER_SIZE);
        igtl::MessageBase::Pointer buffer = igtl::MessageBase::New();
        headerMsg->Unpack();
        buffer->SetMessageHeader(headerMsg);
        buffer->AllocatePack();
        memcpy(buffer->GetPackBodyPointer(), message+IGTL_HEADER_SIZE, buffer->GetPackBodySize());
        std::string CurrentIGTLMSGType = std::string(headerMsg->GetDeviceType());
        if (CurrentIGTLMSGType.compare("TDATA")==0)
        {
          if (!this->CheckNodeExist(igtlConnectorSequenceNode->GetSequenceScene(),"vtkMRMLIGTLTrackingDataBundleNode", buffer))
          {
            vtkMRMLNode* createdNode =  trackingConverter->CreateNewNodeWithMessage(igtlConnectorSequenceNode->GetSequenceScene(),headerMsg->GetDeviceName(),buffer);
            trackingBundleNode = vtkMRMLIGTLTrackingDataBundleNode::SafeDownCast(createdNode);
            trackingConverter->IGTLToMRML(buffer, trackingBundleNode);
            for(int i = 0; i<trackingBundleNode->GetNumberOfTransformNodes();i++)
            {
              vtkMRMLSequenceNode * transformSequence = vtkMRMLSequenceNode::New();
              transformSequenceNodes.push_back(transformSequence);
              transformNames.push_back(trackingBundleNode->GetTransformNode(i)->GetName());
            }
          }
          trackingConverter->IGTLToMRML(buffer, trackingBundleNode);
          for(int i = 0; i<trackingBundleNode->GetNumberOfTransformNodes();i++)
          {
            transformSequenceNodes.at(i)->SetDataNodeAtValue(trackingBundleNode->GetTransformNode(i), timeStamp);
          }
          hasTransform = true;
        }
        if (CurrentIGTLMSGType.compare("IMAGE")==0)
        {
          if (!this->CheckNodeExist(igtlConnectorSequenceNode->GetSequenceScene(),"vtkMRMLVolumeNode", buffer))
          {
            vtkMRMLNode* createdNode =  imageConverter->CreateNewNodeWithMessage(igtlConnectorSequenceNode->GetSequenceScene(),headerMsg->GetDeviceName(), buffer);
            volumeNode = vtkMRMLVolumeNode::SafeDownCast(createdNode);
          }
          imageConverter->IGTLToMRML(buffer, volumeNode);
          volumeSequence->SetDataNodeAtValue(volumeNode, timeStamp);
          hasVolume = true;
        }
        if (CurrentIGTLMSGType.compare("Video")==0)
        {
          if (!this->CheckNodeExist(igtlConnectorSequenceNode->GetSequenceScene(),"vtkMRMLVectorVolumeNode.rel_bitStreamID", buffer))
          {
            vtkMRMLNode* createdNode =  videoConverter->CreateNewNodeWithMessage(igtlConnectorSequenceNode->GetSequenceScene(),headerMsg->GetDeviceName(), buffer);
            vectorVolumeNode = vtkMRMLVectorVolumeNode::SafeDownCast(createdNode);
            const char* nodeID = vectorVolumeNode->GetAttribute("vtkMRMLVectorVolumeNode.rel_bitStreamID");
            bitStreamNode = vtkMRMLBitStreamNode::SafeDownCast((bitStreamSequence->GetSequenceScene())->GetNodeByID(nodeID));
          }
          videoConverter->IGTLToMRML(buffer, vectorVolumeNode);
          bitStreamSequence->SetDataNodeAtValue(bitStreamNode, timeStamp);
          hasBitStream = true;
        }
      }
      delete message;
    }
  }
  if(hasTransform)
  {
    defaultHeaderOutStream << "LinearTransfromFileName: " << linearTransformFileName << std::endl;
    std::string fullName = linearTransformFileName.append(".").append(transformStorageNode->GetDefaultWriteFileExtension());
    //transformStorageNode->SetFileName(fullName.c_str());
    //transformStorageNode->WriteDataInternal(transformSequence);
    transformStorageNode->WriteSequenceMetafileTransforms(fullName.c_str(), transformSequenceNodes, transformNames, igtlConnectorSequenceNode, NULL);
  }
  if(hasVolume)
  {
    defaultHeaderOutStream << "VolumeFileName: " << volumeFileName << std::endl;
    std::string fullName = volumeFileName.append(".").append(volumeStorageNode->GetDefaultWriteFileExtension());
    volumeStorageNode->SetFileName(fullName.c_str());
    volumeStorageNode->WriteDataInternal(volumeSequence);
  }
  if(hasBitStream)
  {
    defaultHeaderOutStream << "BitStreamFileName: " << bitStreamFileName << std::endl;
    std::string fullName = bitStreamFileName.append(".").append(bitStreamStorageNode->GetDefaultWriteFileExtension());
    bitStreamStorageNode->SetFileName(fullName.c_str());
    bitStreamStorageNode->WriteDataInternal(bitStreamSequence);
  }
  
  outStream.close();
  this->StageWriteData(refNode);
  
  return 1;
}


//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorSequenceStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("IGTLConnector Message Binary (.igtlbinary)");
  this->SupportedWriteFileTypes->InsertNextValue("IGTLConnector Message Binary (.seq.igtlbinary)");
}

//----------------------------------------------------------------------------
void vtkMRMLIGTLConnectorSequenceStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("IGTLConnector Message Binary (.igtlbinary)");
  this->SupportedWriteFileTypes->InsertNextValue("IGTLConnector Message Binary (.seq.igtlbinary)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLIGTLConnectorSequenceStorageNode::GetDefaultWriteFileExtension()
{
  return "igtlbinary";
}
