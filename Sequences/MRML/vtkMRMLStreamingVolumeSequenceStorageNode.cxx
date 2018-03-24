/*=auto=========================================================================
 
 Portions (c) Copyright 2005 Brigham and Women's Hospital (BWH) All Rights Reserved.
 
 See COPYRIGHT.txt
 or http://www.slicer.org/copyright/copyright.txt for details.
 
 =========================================================================auto=*/
#include <algorithm>

#include "vtkMRMLStreamingVolumeSequenceStorageNode.h"
#include "vtkMRMLSequenceNode.h"
#include "vtkMRMLScene.h"
#include "vtkMRMLStreamingVolumeNode.h"
#include <vtkCollection.h>

#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkStringArray.h"

#include "vtksys/SystemTools.hxx"

//----------------------------------------------------------------------------
vtkMRMLNodeNewMacro(vtkMRMLStreamingVolumeSequenceStorageNode);

// Add the helper functions

//-------------------------------------------------------
inline void Trim(std::string &str)
{
  str.erase(str.find_last_not_of(" \t\r\n") + 1);
  str.erase(0, str.find_first_not_of(" \t\r\n"));
}


//----------------------------------------------------------------------------
vtkMRMLStreamingVolumeSequenceStorageNode::vtkMRMLStreamingVolumeSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
vtkMRMLStreamingVolumeSequenceStorageNode::~vtkMRMLStreamingVolumeSequenceStorageNode()
{
}

//----------------------------------------------------------------------------
bool vtkMRMLStreamingVolumeSequenceStorageNode::CanReadInReferenceNode(vtkMRMLNode *refNode)
{
  return refNode->IsA("vtkMRMLSequenceNode");
}



int vtkMRMLStreamingVolumeSequenceStorageNode::GetTagValue(char* headerString, int headerLenght, const char* tag, int tagLength, std::string &tagValueString, int&tagValueLength)
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

int vtkMRMLStreamingVolumeSequenceStorageNode::ReadElementsInSingleLine(std::string& firstElement, std::string& secondElement, FILE* stream)
{
  int stringLineLength = 0;
  std::string data = std::string(" ");
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
  if (!bCanBeRead) return -1;
  char *lineString = new char[stringLineLength];
  fread(lineString,stringLineLength,1,stream);
  std::string stringOperator(lineString);
  size_t pos = 0;
  if ((pos = stringOperator.find(":")) != std::string::npos)
    {
    firstElement = std::string(lineString,pos);
    std::string stringMsgLength(lineString+pos,stringLineLength-pos);
    stringMsgLength.erase(stringMsgLength.find_last_not_of(" :\t\r\n") + 1);
    stringMsgLength.erase(0, stringMsgLength.find_first_not_of(" :\t\r\n")); // Get rid of ":"
    stringMsgLength.erase(0, stringMsgLength.find_first_not_of(" :\t\r\n")); // Get rid of " "
    secondElement = std::string(stringMsgLength);
    }
  else
    {
    return 0;
    }
  return 1;
}

//----------------------------------------------------------------------------
int vtkMRMLStreamingVolumeSequenceStorageNode::ReadDataInternal(vtkMRMLNode* refNode)
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
  volSequenceNode->RemoveAllDataNodes();
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
    vtkDebugMacro("vtkMRMLStreamingVolumeSequenceStorageNode: This is not a text file");
    return 0;
    }
  std::string data("  ");
  int headerLength = 0;
  while(fread(&data[0],2,1,stream))
    {
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
    if (strcmp(tagValueString.c_str(), "StreamingVolume")==0)
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
    fileValid = true;
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
  fread(headerString, 2,1,stream); // get rid of the following two line breaks
  if(fileValid)
    {
    if(this->GetScene())
      {
      vtkCollection* collection =  NULL;
      vtkMRMLScene* scene = this->GetScene();
      collection = scene->GetNodesByClassByName("vtkMRMLStreamingVolumeNode",volumeName.c_str());
      int nCol = collection->GetNumberOfItems();
      if (nCol > 0)
        {
        vtkMRMLStreamingVolumeNode * frameProxyNode = vtkMRMLStreamingVolumeNode::SafeDownCast(collection->GetItemAsObject(0));
        while(1)
          {
          std::string timeStamp("");
          std::string stringMessageLength("");
          int errorCode = this->ReadElementsInSingleLine(timeStamp, stringMessageLength, stream);
          int stringMessageLenInt = atoi(stringMessageLength.c_str());
          if (errorCode == -1)
            {
            break;
            }
          std::string FrameType("");
          std::string isKeyFrame("");
          errorCode = this->ReadElementsInSingleLine(FrameType, isKeyFrame, stream);
          if (errorCode == -1)
            {
            break;
            }
          else if (errorCode == 1 && stringMessageLenInt > 0)
            {
            char *buffer = new char[stringMessageLenInt];
            fread(buffer, stringMessageLenInt, 1, stream);
            vtkUnsignedCharArray* bufferString = vtkUnsignedCharArray::New();
            bufferString->SetNumberOfTuples(stringMessageLenInt);
            memcpy(bufferString->GetPointer(0), buffer, stringMessageLenInt);
            frameProxyNode->SetFrameUpdated(true);
            if(strncmp(FrameType.c_str(), "PrecedingKeyFrame", 17) == 0)
              {
              frameProxyNode->UpdateKeyFrameByDeepCopy(bufferString);
              }
            else if(strncmp(FrameType.c_str(), "IsKeyFrame", 10) == 0)
              {
              if(strncmp(isKeyFrame.c_str(), "1", 10) == 0)
                {
                frameProxyNode->UpdateKeyFrameByDeepCopy(bufferString);
                }
              else
                {
                if (frameProxyNode->GetKeyFrame() != NULL)
                  {
                  // no update of key frame, just copy the previous key frame.
                  frameProxyNode->UpdateKeyFrameByDeepCopy(frameProxyNode->GetKeyFrame());
                  frameProxyNode->SetKeyFrameUpdated(false);
                  }
                }
              frameProxyNode->UpdateFrameByDeepCopy(bufferString);
              volSequenceNode->SetDataNodeAtValue(frameProxyNode, std::string(timeStamp));
              }
              fread(&data[0],1,1,stream); // get rid of last line break
            }
          }
        }
      }
    }
  return 1;
}

//----------------------------------------------------------------------------
bool vtkMRMLStreamingVolumeSequenceStorageNode::CanWriteFromReferenceNode(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* sequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (sequenceNode == NULL)
    {
    vtkDebugMacro("vtkMRMLStreamingVolumeSequenceStorageNode::CanWriteFromReferenceNode: input is not a sequence node");
    return false;
    }
  if (sequenceNode->GetNumberOfDataNodes() == 0)
    {
    vtkDebugMacro("vtkMRMLStreamingVolumeSequenceStorageNode::CanWriteFromReferenceNode: no data nodes");
    return false;
    }
  int numberOfFrameVolumes = sequenceNode->GetNumberOfDataNodes();
  for (int frameIndex = 0; frameIndex < numberOfFrameVolumes; frameIndex++)
    {
    vtkMRMLStreamingVolumeNode* bitstream = vtkMRMLStreamingVolumeNode::SafeDownCast(sequenceNode->GetNthDataNode(frameIndex));
    if (bitstream == NULL)
      {
      vtkDebugMacro("vtkMRMLStreamingVolumeSequenceStorageNode::CanWriteFromReferenceNode: stream nodes has not bit stream (frame " << frameIndex << ")");
      return false;
      }
    }
  return true;
}

//----------------------------------------------------------------------------
int vtkMRMLStreamingVolumeSequenceStorageNode::WriteDataInternal(vtkMRMLNode *refNode)
{
  vtkMRMLSequenceNode* bitStreamSequenceNode = vtkMRMLSequenceNode::SafeDownCast(refNode);
  if (bitStreamSequenceNode==NULL)
    {
    vtkErrorMacro(<< "vtkMRMLStreamingVolumeSequenceStorageNode::WriteDataInternal: Do not recognize node type " << refNode->GetClassName());
    return 0;
    }
  char* volumeName = (char*)"";
  if (bitStreamSequenceNode->GetNumberOfDataNodes()>0)
    {
    vtkMRMLStreamingVolumeNode* streamingVol = vtkMRMLStreamingVolumeNode::SafeDownCast(bitStreamSequenceNode->GetNthDataNode(0));
    if (streamingVol==NULL)
      {
      vtkErrorMacro(<< "vtkMRMLStreamingVolumeSequenceStorageNode::WriteDataInternal: Data node is not a bit stream");
      return 0;
      }
    volumeName = streamingVol->GetName();
    }
  std::string fullName = this->GetFullNameFromFileName();
  if (fullName == std::string(""))
    {
    vtkErrorMacro("WriteData: File name not specified");
    return 0;
    }
  // If header file exists then append transform info before element data file line
  // Append the transform information to the end of the file
  vtkCollection* dataCollection = this->GetScene()->GetNodesByClassByName("vtkMRMLStreamingVolumeNode", volumeName);
  vtkMRMLStreamingVolumeNode* streamingVol = vtkMRMLStreamingVolumeNode::SafeDownCast(dataCollection->GetItemAsObject(0));
  if (streamingVol !=NULL)
    {
    std::stringstream defaultHeaderOutStream;
    defaultHeaderOutStream << "ObjectType: StreamingVolume" << std::endl;
    defaultHeaderOutStream<< "Compression Device Node Class: " << streamingVol->GetCodecClassName() <<std::endl;
    defaultHeaderOutStream<< "Codec: " << streamingVol->GetCodecType() <<std::endl
    << "VolumeName: " << volumeName << std::endl;
    // Append the bit stream to the end of the file
    std::ofstream outStream(fullName.c_str(), std::ios_base::binary);
    outStream << defaultHeaderOutStream.str();
    outStream << std::setfill('0');
    outStream << std::endl;
    outStream << std::endl;
    int numberOfFrameBitStreams = bitStreamSequenceNode->GetNumberOfDataNodes();
    std::string timeStamp = bitStreamSequenceNode->GetNthIndexValue(0);
    vtkMRMLStreamingVolumeNode* frameBitStream = vtkMRMLStreamingVolumeNode::SafeDownCast(bitStreamSequenceNode->GetNthDataNode(0));
    if (streamingVol!=NULL && timeStamp.size())
      {
      vtkUnsignedCharArray* keyFrame = frameBitStream->GetKeyFrame();
      vtkUnsignedCharArray* frame = frameBitStream->GetFrame();
      char * keyFramePointer = reinterpret_cast<char*> (keyFrame->GetPointer(0));
      char * framePointer = reinterpret_cast<char*> (frame->GetPointer(0));
      long keyFrameLength = static_cast<long>(keyFrame->GetNumberOfTuples());
      long frameLength = static_cast<long>(frame->GetNumberOfTuples());
      if (strcmp(keyFramePointer, framePointer) == 0)
        {
        outStream.write(timeStamp.c_str(), timeStamp.size());
        outStream <<": "<< keyFrameLength << std::endl;
        outStream<< "IsKeyFrame: " << 1 << std::endl;
        outStream.write(keyFramePointer, keyFrameLength);
        outStream << std::endl;
        }
      else
        {
        outStream.write(timeStamp.c_str(), timeStamp.size());
        outStream <<": "<<keyFrameLength << std::endl;;
        outStream<< "PrecedingKeyFrame: " << 1 << std::endl;
        outStream.write(keyFramePointer, keyFrameLength);
        outStream << std::endl;
        outStream.write(timeStamp.c_str(), timeStamp.size());
        outStream <<": "<<frameLength<< std::endl;
        outStream<< "IsKeyFrame: " << 0 << std::endl;
        outStream.write(framePointer, frameLength);
        outStream << std::endl;
        }
      }
    for (int frameIndex=1; frameIndex<numberOfFrameBitStreams; frameIndex++)
      {
      vtkMRMLStreamingVolumeNode* frameBitStream = vtkMRMLStreamingVolumeNode::SafeDownCast(bitStreamSequenceNode->GetNthDataNode(frameIndex));
      std::string timeStamp = bitStreamSequenceNode->GetNthIndexValue(frameIndex);
      if (frameBitStream!=NULL && timeStamp.size())
        {
        vtkUnsignedCharArray* frame = frameBitStream->GetFrame();
        char * framePointer = reinterpret_cast<char*> (frame->GetPointer(0));
        long frameLength = static_cast<long>(frame->GetNumberOfTuples());
        if (frameLength > 0)
          {
          outStream.write(timeStamp.c_str(), timeStamp.size());
          outStream <<": "<<frameLength << std::endl;
          if(frameBitStream->GetKeyFrameUpdated())
            {
            outStream<< "IsKeyFrame: " << 1 << std::endl;
            }
          else
            {
            outStream<< "IsKeyFrame: " << 0 << std::endl;
            }
          outStream.write(framePointer, frameLength);
          outStream << std::endl;
          }
        }
      }
      outStream.close();
      }
  this->StageWriteData(refNode);
  return 1;
}


//----------------------------------------------------------------------------
void vtkMRMLStreamingVolumeSequenceStorageNode::InitializeSupportedReadFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Video Bit Stream (.bin)");
  this->SupportedWriteFileTypes->InsertNextValue("Video Bit Stream (.seq.bin)");
}

//----------------------------------------------------------------------------
void vtkMRMLStreamingVolumeSequenceStorageNode::InitializeSupportedWriteFileTypes()
{
  this->SupportedWriteFileTypes->InsertNextValue("Video Bit Stream (.bin)");
  this->SupportedWriteFileTypes->InsertNextValue("Video Bit Stream (.seq.bin)");
}

//----------------------------------------------------------------------------
const char* vtkMRMLStreamingVolumeSequenceStorageNode::GetDefaultWriteFileExtension()
{
  return "bin";
}
