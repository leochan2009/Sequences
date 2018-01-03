project(vtkSlicer${MODULE_NAME}ModuleMRML)

set(KIT ${PROJECT_NAME})

set(${KIT}_EXPORT_DIRECTIVE "VTK_SLICER_${MODULE_NAME_UPPER}_MODULE_MRML_EXPORT")

include_directories(BEFORE ${vtkTeem_INCLUDE_DIRS})
find_package(OpenIGTLinkIO REQUIRED)

set(vtkSlicerOpenIGTLinkIFModuleMRML_SOURCE_DIR "/Users/longquanchen/Desktop/Github/Slicer-build/OpenIGTLinkIF/MRML")
set(vtkSlicerOpenIGTLinkIFModuleMRML_BINARY_DIR "/Users/longquanchen/Desktop/Github/Slicer-build/OpenIGTLinkIF-XcodeBuild/MRML")
set(vtkSlicerOpenIGTLinkIFModuleMRML optimized /Users/longquanchen/Desktop/Github/Slicer-build/OpenIGTLinkIF-XcodeBuild/lib/Slicer-4.9/qt-loadable-modules/Release/libvtkSlicerOpenIGTLinkIFModuleMRML.dylib debug /Users/longquanchen/Desktop/Github/Slicer-build/OpenIGTLinkIF-XcodeBuild/lib/Slicer-4.9/qt-loadable-modules/Debug/libvtkSlicerOpenIGTLinkIFModuleMRML.dylib)

set(${KIT}_INCLUDE_DIRECTORIES
  ${Slicer_Base_INCLUDE_DIRS}
  ${OpenIGTLinkIO_INCLUDE_DIRS}
  ${vtkSlicerOpenIGTLinkIFModuleMRML_SOURCE_DIR}
  ${vtkSlicerOpenIGTLinkIFModuleMRML_BINARY_DIR}
  )

set(${KIT}_SRCS
  vtkMRMLLinearTransformSequenceStorageNode.cxx
  vtkMRMLLinearTransformSequenceStorageNode.h
  vtkMRMLNodeSequencer.cxx
  vtkMRMLNodeSequencer.h
  vtkMRMLSequenceNode.cxx
  vtkMRMLSequenceNode.h
  vtkMRMLSequenceStorageNode.cxx
  vtkMRMLSequenceStorageNode.h
  vtkMRMLVolumeSequenceStorageNode.cxx
  vtkMRMLVolumeSequenceStorageNode.h
  vtkMRMLBitStreamSequenceStorageNode.cxx
  vtkMRMLBitStreamSequenceStorageNode.h
  )

set(${KIT}_TARGET_LIBRARIES
  ${OpenIGTLink_LIBRARIES}
  ${OpenIGTLinkIO_LIBRARIES}
  ${ITK_LIBRARIES}
  ${MRML_LIBRARIES}
  SlicerBaseLogic
  qSlicerBaseQTCLI
  vtkSlicerMarkupsModuleMRML
  ${vtkSlicerOpenIGTLinkIFModuleMRML}
  )

#-----------------------------------------------------------------------------
SlicerMacroBuildModuleMRML(
  NAME ${KIT}
  EXPORT_DIRECTIVE ${${KIT}_EXPORT_DIRECTIVE}
  INCLUDE_DIRECTORIES ${${KIT}_INCLUDE_DIRECTORIES}
  SRCS ${${KIT}_SRCS}
  TARGET_LIBRARIES ${${KIT}_TARGET_LIBRARIES}
  )
