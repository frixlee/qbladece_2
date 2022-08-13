# -------------------------------------------------
# Project created by David Marten using QtCreator
# -------------------------------------------------

# switch here to build either for 32bit or 64bit
CONFIG += build64bit

#the current version numbers
        DEFINES += version_string=\"\\\"2.0.4_alpha\\\"\"
win32:  DEFINES += compiled_string=\"\\\"windows\\\"\"
unix:   DEFINES += compiled_string=\"\\\"unix\\\"\"
        DEFINES += chrono_string=\"\\\"7.0.0\\\"\"

# specify which Qt modules are needed.
QT += core gui widgets opengl xml testlib

# set the name of the executable
TARGET = QBladeCE
# compiler optimizes for debugging
QMAKE_CXXFLAGS += -Og

#The template to use for the project. This determines that the output will be an application.
TEMPLATE = app

# include the resources file into the binary
RESOURCES += qblade.qrc

# set the icon of the executable application file
win32:RC_ICONS = images/qblade.ico

# from gcc 4.8.1 on the c++11 implementation is feature complete
#QMAKE_CXXFLAGS += -std=gnu++11  # usually it would be fine to give c++11 but the OpenCL part apparently needs gnu extensions
#QMAKE_CXXFLAGS += -fpermissive
# fixes the "too many sections" issue when compiling debug build under windows
win32:QMAKE_CXXFLAGS += -Wa,-mbig-ob

#disable all warnings
QMAKE_CXXFLAGS += -w
# activate compiler support for openMP
QMAKE_CXXFLAGS += -fopenmp
LIBS += -fopenmp

# add the proper include path for libraries
win32: LIBS += -L$$PWD/libraries/libs_windows_64bit
unix:  LIBS += -L$$PWD/libraries/libs_unix_64bit

# includes QGLViewer
INCLUDEPATH += include_headers/QGLViewer

win32: LIBS += -lQGLViewer2
unix:  LIBS += -lQGLViewer

# include openGL & glu
win32: LIBS += -lopengl32 -lglu32
unix:  LIBS += -lGLU

# include openCL
win32: INCLUDEPATH += include_headers/OpenCL/win
unix:  INCLUDEPATH += include_headers/OpenCL/unix
       LIBS += -lOpenCL

# include path to Eigen headers
INCLUDEPATH += include_headers

# include Project Chrono
win32: LIBS += -llibChronoEngine
unix:  LIBS += -lChronoEngine

INCLUDEPATH += include_headers/Chrono
INCLUDEPATH += include_headers/Chrono/chrono
INCLUDEPATH += include_headers/Chrono/chrono/physics
INCLUDEPATH += include_headers/Chrono/chrono/collision
INCLUDEPATH += include_headers/Chrono/chrono/collision/bullet

# include clBlast
INCLUDEPATH += include_headers/CLBlast
win32: LIBS += -llibclblast
unix:  LIBS += -lclblast

# include lapack and others required by TurbSim
unix: LIBS += -llapack
unix: LIBS += -lblas
unix: LIBS += -lgfortran

SOURCES += src/MainFrame.cpp \
    src/CurveStyleBox.cpp \
    src/DebugDialog.cpp \
    src/FloatDelegate.cpp \
    src/FoilModule/FoilEditDlg.cpp \
    src/PolarModule/OperationalPoint.cpp \
    src/PolarModule/BatchFoilDialog.cpp \
    src/PolarModule/EditPolarDlg.cpp \
    src/PolarModule/PolarCtxMenu.cpp \
    src/PolarModule/PolarDialog.cpp \
    src/PolarModule/PolarDock.cpp \
    src/PolarModule/PolarMenu.cpp \
    src/PolarModule/PolarModule.cpp \
    src/PolarModule/PolarToolBar.cpp \
    src/IceThrowSimulation/IceParticle.cpp \
    src/ImportExport.cpp \
    src/Main.cpp \
    src/Globals.cpp \
    src/GUI/EnvironmentDialog.cpp \
    src/QBEM/BDamage.cpp \
    src/QBEM/BDamageDialog.cpp \
    src/QBEM/OptimizeDlgPROP.cpp \
    src/QControl/TurbineInputs.cpp \
    src/QSimulation/QSimulationDock.cpp \
    src/QSimulation/QSimulationMenu.cpp \
    src/QSimulation/QSimulationModule.cpp \
    src/QSimulation/QSimulationToolBar.cpp \
    src/QSimulation/QSimulationTwoDContextMenu.cpp \
    src/TwoDWidget.cpp \
    src/GLWidget.cpp \
    src/GUI/GLLightSettings.cpp \
    src/VortexObjects/VortexPanel.cpp \
    src/Waves/LinearWave.cpp \
    src/Waves/WaveCreatorDialog.cpp \
    src/Waves/WaveDock.cpp \
    src/Waves/WaveMenu.cpp \
    src/Waves/WaveModule.cpp \
    src/Waves/WaveToolBar.cpp \
    src/Waves/WaveTwoDContextMenu.cpp \
    src/Vec3.cpp \
    src/FoilModule/Airfoil.cpp \
    src/PolarModule/Polar.cpp \
    src/GUI/CurvePickerDlg.cpp \
    src/GUI/CurveDelegate.cpp \
    src/GUI/CurveCbBox.cpp \
    src/GUI/CurveButton.cpp \
    src/GUI/MainSettingsDialog.cpp \
    src/QBEM/TData.cpp \
    src/QBEM/TBEMData.cpp \
    src/QBEM/SimuWidget.cpp \
    src/QBEM/OptimizeDlg.cpp \
    src/QBEM/CreateBEMDlg.cpp \
    src/QBEM/BladeScaleDlg.cpp \
    src/QBEM/BEMData.cpp \
    src/QBEM/BEM.cpp \
    src/QBEM/Edit360PolarDlg.cpp \
    src/QBEM/BData.cpp \
    src/QBEM/Blade.cpp \
    src/QBEM/BladeSurface.cpp \
    src/QBEM/BladeDelegate.cpp \
    src/QBEM/BladeAxisDelegate.cpp \
    src/QBEM/CBEMData.cpp \
    src/QBEM/PrescribedValuesDlg.cpp \
    src/QBEM/CircularFoilDlg.cpp \
    src/QBEM/BEMSimDock.cpp \
    src/QBEM/BEMDock.cpp \
    src/QDMS/DMS.cpp \
    src/QDMS/SimuWidgetDMS.cpp \
    src/QDMS/BladeDelegateVAWT.cpp \
    src/QDMS/OptimizeDlgVAWT.cpp \
    src/QDMS/BladeScaleDlgVAWT.cpp \
    src/QDMS/CreateDMSDlg.cpp \
    src/QDMS/DMSData.cpp \
    src/QDMS/DData.cpp \
    src/QDMS/TDMSData.cpp \
    src/QDMS/DMSSimDock.cpp \
    src/QDMS/DMSDock.cpp \
    src/QDMS/CDMSData.cpp \
    src/Windfield/WindField.cpp \
    src/XWidgets.cpp \
    src/Windfield/WindFieldModule.cpp \
    src/Module.cpp \
    src/Windfield/WindFieldToolBar.cpp \
    src/ScrolledDock.cpp \
    src/Store.cpp \
    src/Windfield/WindFieldMenu.cpp \
    src/QFEMModule/taperedelem.cpp \
    src/QFEMModule/structintegrator.cpp \
    src/QFEMModule/structelem.cpp \
    src/QFEMModule/node.cpp \
    src/QFEMModule/eqnmotion.cpp \
    src/QFEMModule/clipper.cpp \
    src/QFEMModule/unitvector.cpp \
    src/QFEMModule/mode.cpp \
    src/QFEMModule/deformationvector.cpp \
    src/QFEMModule/QFEMDock.cpp \
    src/QFEMModule/QFEMToolBar.cpp \
    src/QFEMModule/QFEMModule.cpp \
    src/QFEMModule/QFEMMenu.cpp \
    src/QFEMModule/StructDelegate.cpp \
    src/QFEMModule/BladeStructure.cpp \
    src/StorableObject.cpp \
    src/QBEM/BEMToolbar.cpp \
    src/QDMS/DMSToolbar.cpp \
    src/QBEM/Polar360.cpp \
    src/GUI/NumberEdit.cpp \
    src/Serializer.cpp \
    src/StoreAssociatedComboBox.cpp \
    src/GlobalFunctions.cpp \
    src/GUI/SignalBlockerInterface.cpp \
    src/Graph/NewGraph.cpp \
    src/Graph/Axis.cpp \
    src/Graph/NewCurve.cpp \
    src/Graph/GraphOptionsDialog.cpp \
    src/Graph/ShowAsGraphInterface.cpp \
    src/QFEMModule/QFEMTwoDContextMenu.cpp \
    src/QFEMModule/forcingterm.cpp \
    src/QFEMModule/staticequation.cpp \
    src/QFEMModule/BladeStructureLoading.cpp \
    src/QBEM/ExportGeomDlg.cpp \
    src/TwoDContextMenu.cpp \
    src/GUI/FixedSizeLabel.cpp \
    src/QBladeApplication.cpp \
    src/VortexObjects/VortexNode.cpp \
    src/VortexObjects/VortexLine.cpp \
    src/VortexObjects/DummyLine.cpp \
    src/QBEM/PolarSelectionDialog.cpp \
    src/CreatorDock.cpp \
    src/ColorManager.cpp \
    src/GUI/LineStyleButton.cpp \
    src/GUI/LineStyleDialog.cpp \
    src/GUI/LineStyleComboBox.cpp \
    src/GUI/NewColorButton.cpp \
    src/TwoDGraphMenu.cpp \
    src/Windfield/WindFieldCreatorDialog.cpp \
    src/ParameterViewer.cpp \
    src/ParameterObject.cpp \
    src/ParameterGrid.cpp \
    src/TwoDWidgetInterface.cpp \
    src/Windfield/WindFieldDock.cpp \
    src/CreatorDialog.cpp \
    src/NoiseModule/NoiseModule.cpp \
    src/NoiseModule/NoiseSimulation.cpp \
    src/NoiseModule/NoiseToolBar.cpp \
    src/NoiseModule/NoiseDock.cpp \
    src/CreatorTwoDDock.cpp \
    src/NoiseModule/NoiseCreatorDialog.cpp \
    src/NoiseModule/NoiseOpPoint.cpp \
    src/NoiseModule/NoiseCalculation.cpp \
    src/NoiseModule/NoiseParameter.cpp \
    src/NoiseModule/NoiseException.cpp \
    src/NoiseModule/NoiseContextMenu.cpp \
    src/NoiseModule/NoiseMenu.cpp \
    src/FoilModule/FoilCtxMenu.cpp \
    src/FoilModule/FoilDock.cpp \
    src/FoilModule/FoilModule.cpp \
    src/FoilModule/FoilToolBar.cpp \
    src/FoilModule/FoilDelegate.cpp \
    src/FoilModule/FoilMenu.cpp \
    src/QDMS/StrutCreatorDialog.cpp \
    src/QDMS/Strut.cpp \
    src/GUI/NumberEditDelegate.cpp \
    src/GUI/ComboBoxDelegate.cpp \
    src/Vec3f.cpp \
    src/StructModel/StrElem.cpp \
    src/StructModel/StrNode.cpp \
    src/StructModel/ChBodyAddedMass.cpp \
    src/StructModel/ChNodeFEAxyzrotAddedMass.cpp \
    src/StructModel/ChVariablesBodyAddedMass.cpp \
    src/QBEM/AFC.cpp \
    src/QBEM/DynPolarSet.cpp \
    src/QBEM/DynPolarSetDialog.cpp \
    src/QBEM/FlapCreatorDialog.cpp \
    src/VortexObjects/VortexParticle.cpp \
    src/StructModel/PID.cpp \
    src/QControl/QControl.cpp \
    src/StructModel/CoordSys.cpp \
    src/IceThrowSimulation/IceThrowSimulation.cpp \
    src/QTurbine/QTurbineModule.cpp \
    src/QTurbine/QTurbineDock.cpp \
    src/QTurbine/QTurbineToolBar.cpp \
    src/QTurbine/QTurbine.cpp \
    src/QTurbine/QTurbineCreatorDialog.cpp \
    src/QTurbine/QTurbineTwoDContextMenu.cpp \
    src/QSimulation/QSimulation.cpp \
    src/QSimulation/QSimulationCreatorDialog.cpp \
    src/StructModel/StrModel.cpp \
    src/StructModel/StrObjects.cpp \
    src/QTurbine/QTurbineSimulationData.cpp \
    src/QTurbine/QTurbineResults.cpp \
    src/QTurbine/QTurbineGlRendering.cpp \
    src/OpenCLSetup.cpp \
    src/QSimulation/QVelocityCutPlane.cpp \
    src/QBEM/Interpolate360PolarsDlg.cpp \
    src/BinaryProgressDialog.cpp \
    src/Windfield/WindFieldTwoDContextMenu.cpp \
    src/VPML/Box_Tree.cpp \
    src/VPML/ML_Box.cpp \
    src/VPML/ML_Euler.cpp \
    src/VPML/Particle_Grid.cpp \
    src/QTurbine/QTurbineMenu.cpp

HEADERS += src/MainFrame.h \
    src/CurveStyleBox.h \
    src/DebugDialog.h \
    src/FloatDelegate.h \
    src/FoilModule/FoilEditDlg.h \
    src/PolarModule/OperationalPoint.h \
    src/PolarModule/BatchFoilDialog.h \
    src/PolarModule/EditPolarDlg.h \
    src/PolarModule/PolarCtxMenu.h \
    src/PolarModule/PolarDialog.h \
    src/PolarModule/PolarDock.h \
    src/PolarModule/PolarMenu.h \
    src/PolarModule/PolarModule.h \
    src/PolarModule/PolarToolBar.h \
    src/IceThrowSimulation/IceParticle.h \
    src/ImportExport.h \
    src/GUI/EnvironmentDialog.h \
    src/Params.h \
    src/Globals.h \
    src/QBEM/BDamage.h \
    src/QBEM/BDamageDialog.h \
    src/QBEM/OptimizeDlgPROP.h \
    src/QControl/TurbineInputs.h \
    src/QSimulation/QSimulationDock.h \
    src/QSimulation/QSimulationMenu.h \
    src/QSimulation/QSimulationModule.h \
    src/QSimulation/QSimulationToolBar.h \
    src/QSimulation/QSimulationTwoDContextMenu.h \
    src/TwoDWidget.h \
    src/GLWidget.h \
    src/GUI/GLLightSettings.h \
    src/VortexObjects/VortexPanel.h \
    src/Waves/LinearWave.h \
    src/Waves/WaveCreatorDialog.h \
    src/Waves/WaveDock.h \
    src/Waves/WaveMenu.h \
    src/Waves/WaveModule.h \
    src/Waves/WaveToolBar.h \
    src/Waves/WaveTwoDContextMenu.h \
    src/QBEM/BladeSurface.h \
    src/Quaternion.h \
    src/PolarModule/Polar.h \
    src/Vec3.h \
    src/FoilModule/Airfoil.h \
    src/GUI/CurvePickerDlg.h \
    src/GUI/CurveDelegate.h \
    src/GUI/MainSettingsDialog.h \
    src/GUI/CurveCbBox.h \
    src/GUI/CurveButton.h \
    src/QBEM/TData.h \
    src/QBEM/TBEMData.h \
    src/QBEM/SimuWidget.h \
    src/QBEM/OptimizeDlg.h \
    src/QBEM/CreateBEMDlg.h \
    src/QBEM/BladeScaleDlg.h \
    src/QBEM/BEMData.h \
    src/QBEM/BEM.h \
    src/QBEM/Edit360PolarDlg.h \
    src/QBEM/BData.h \
    src/QBEM/Blade.h \
    src/QBEM/BladeDelegate.h \
    src/QBEM/BladeAxisDelegate.h \
    src/QBEM/CBEMData.h \
    src/QBEM/PrescribedValuesDlg.h \
    src/QBEM/CircularFoilDlg.h \
    src/QBEM/BEMSimDock.h \
    src/QBEM/BEMDock.h \
    src/QDMS/DMS.h \
    src/QDMS/SimuWidgetDMS.h \
    src/QDMS/BladeDelegateVAWT.h \
    src/QDMS/OptimizeDlgVAWT.h \
    src/QDMS/BladeScaleDlgVAWT.h \
    src/QDMS/CreateDMSDlg.h \
    src/QDMS/DMSData.h \
    src/QDMS/DData.h \
    src/QDMS/TDMSData.h \
    src/QDMS/DMSSimDock.h \
    src/QDMS/DMSDock.h \
    src/QDMS/CDMSData.h \
    src/Windfield/WindField.h \
    src/XWidgets.h \
    src/Windfield/WindFieldModule.h \
    src/Module.h \
    src/Windfield/WindFieldToolBar.h \
    src/ScrolledDock.h \
    src/Store.h \
    src/Windfield/WindFieldMenu.h \
    src/QFEMModule/taperedelem.h \
    src/QFEMModule/structintegrator.h \
    src/QFEMModule/structelem.h \
    src/QFEMModule/node.h \
    src/QFEMModule/eqnmotion.h \
    src/QFEMModule/unitvector.h \
    src/QFEMModule/mode.h \
    src/QFEMModule/deformationvector.h \
    src/QFEMModule/QFEMDock.h \
    src/QFEMModule/QFEMToolBar.h \
    src/QFEMModule/QFEMModule.h \
    src/QFEMModule/QFEMMenu.h \
    src/QFEMModule/BladeStructure.h \
    src/QFEMModule/StructDelegate.h \
    src/StorableObject.h \
    src/QBEM/BEMToolbar.h \
    src/QDMS/DMSToolbar.h \
    src/QBEM/Polar360.h \
    src/GUI/NumberEdit.h \
    src/Serializer.h \
    src/StoreAssociatedComboBox.h \
    src/StoreAssociatedComboBox_include.h \
    src/Store_include.h \
    src/StorableObject_heirs.h \
    src/GlobalFunctions.h \
    src/GUI/SignalBlockerInterface.h \
    src/Graph/NewGraph.h \
    src/Graph/Axis.h \
    src/Graph/NewCurve.h \
    src/Graph/GraphOptionsDialog.h \
    src/Graph/ShowAsGraphInterface.h \
    src/QFEMModule/QFEMTwoDContextMenu.h \
    src/QFEMModule/forcingterm.h \
    src/QFEMModule/staticequation.h \
    src/QFEMModule/BladeStructureLoading.h \
    src/QBEM/ExportGeomDlg.h \
    src/TwoDContextMenu.h \
    src/GUI/FixedSizeLabel.h \
    src/QBladeApplication.h \
    src/VortexObjects/VortexNode.h \
    src/VortexObjects/VortexLine.h \
    src/VortexObjects/DummyLine.h \
    src/QBEM/PolarSelectionDialog.h \
    src/CreatorDock.h \
    src/ColorManager.h \
    src/GUI/LineStyleButton.h \
    src/GUI/LineStyleDialog.h \
    src/GUI/LineStyleComboBox.h \
    src/GUI/NewColorButton.h \
    src/TwoDGraphMenu.h \
    src/Windfield/WindFieldCreatorDialog.h \
    src/ParameterViewer.h \
    src/ParameterObject.h \
    src/ParameterGrid.h \
    src/ParameterKeys.h \
    src/TwoDWidgetInterface.h \
    src/Windfield/WindFieldDock.h \
    src/CreatorDialog.h \
    src/NoiseModule/NoiseModule.h \
    src/NoiseModule/NoiseSimulation.h \
    src/NoiseModule/NoiseToolBar.h \
    src/NoiseModule/NoiseDock.h \
    src/CreatorTwoDDock.h \
    src/NoiseModule/NoiseCreatorDialog.h \
    src/NoiseModule/NoiseOpPoint.h \
    src/NoiseModule/NoiseCalculation.h \
    src/NoiseModule/NoiseParameter.h \
    src/NoiseModule/NoiseException.h \
    src/NoiseModule/NoiseContextMenu.h \
    src/NoiseModule/NoiseMenu.h \
    src/FoilModule/FoilCtxMenu.h \
    src/FoilModule/FoilDock.h \
    src/FoilModule/FoilModule.h \
    src/FoilModule/FoilToolBar.h \
    src/FoilModule/FoilDelegate.h \
    src/FoilModule/FoilMenu.h \
    src/QDMS/StrutCreatorDialog.h \
    src/QDMS/Strut.h \
    src/GUI/NumberEditDelegate.h \
    src/GUI/ComboBoxDelegate.h \
    src/Vec3f.h \
    src/StructModel/StrElem.h \
    src/StructModel/StrNode.h \
    src/StructModel/ChBodyAddedMass.h \
    src/StructModel/ChNodeFEAxyzrotAddedMass.h \
    src/StructModel/ChVariablesBodyAddedMass.h \
    src/StructModel/StrLoads.h \
    src/QBEM/AFC.h \
    src/QBEM/DynPolarSet.h \
    src/QBEM/DynPolarSetDialog.h \
    src/QBEM/FlapCreatorDialog.h \
    src/VortexObjects/VortexParticle.h \
    src/StructModel/PID.h \
    src/QControl/QControl.h \
    src/StructModel/CoordSys.h \
    src/IceThrowSimulation/IceThrowSimulation.h \
    src/QTurbine/QTurbineModule.h \
    src/QTurbine/QTurbineDock.h \
    src/QTurbine/QTurbineToolBar.h \
    src/QTurbine/QTurbine.h \
    src/QTurbine/QTurbineCreatorDialog.h \
    src/QTurbine/QTurbineTwoDContextMenu.h \
    src/QSimulation/QSimulation.h \
    src/QSimulation/QSimulationCreatorDialog.h \
    src/StructModel/StrModel.h \
    src/StructModel/StrObjects.h \
    src/QTurbine/QTurbineSimulationData.h \
    src/QTurbine/QTurbineResults.h \
    src/QTurbine/QTurbineGlRendering.h \
    src/QSimulation/QSimulationThread.h \
    src/OpenCLSetup.h \
    src/QSimulation/QVelocityCutPlane.h \
    src/QBEM/Interpolate360PolarsDlg.h \
    src/BinaryProgressDialog.h \
    src/Windfield/WindFieldTwoDContextMenu.h \
    src/VPML/Box_Tree.h \
    src/VPML/Debug.h \
    src/VPML/ML_Box.h \
    src/VPML/ML_Euler.h \
    src/VPML/Math_Types.h \
    src/VPML/Octtree.h \
    src/VPML/Particle.h \
    src/VPML/Particle_Grid.h \
    src/VPML/Tree_Methods.h \
    src/VPML/VPML_Gobal_Vars.h \
    src/QTurbine/QTurbineMenu.h

#here a few usefull files and required binaries are automatically copied to the build directory

CONFIG(debug, debug|release) {
    VARIANT = debug
} else {
    VARIANT = release
}

QMAKE_EXTRA_TARGETS += copyTarget
PRE_TARGETDEPS += copyTarget

win32:copybinaries.commands = $(COPY_DIR) \"$$shell_path($$PWD\\data\\Binaries\\win)\" \"$$shell_path($$OUT_PWD\\$$VARIANT\\Binaries)\"
unix:copybinaries.commands = $(COPY_DIR) \"$$shell_path($$PWD\\data\\Binaries\\unix)\" \"$$shell_path($$OUT_PWD\\Binaries\\)\"
copyTarget.depends += copybinaries
QMAKE_EXTRA_TARGETS += copybinaries

win32:copycontrollers.commands = $(COPY_DIR) \"$$shell_path($$PWD\\data\\ControllerFiles\\win)\" \"$$shell_path($$OUT_PWD\\$$VARIANT\\ControllerFiles)\"
unix:copycontrollers.commands = $(COPY_DIR) \"$$shell_path($$PWD\\data\\ControllerFiles\\unix)\" \"$$shell_path($$OUT_PWD\\ControllerFiles\\)\"
copyTarget.depends += copycontrollers
QMAKE_EXTRA_TARGETS += copycontrollers

win32:copyparameters.commands = $(COPY_DIR) \"$$shell_path($$PWD\\data\\ControllerFiles\\ParameterFiles)\" \"$$shell_path($$OUT_PWD\\$$VARIANT\\ControllerFiles\\ParameterFiles)\"
unix:copyparameters.commands = $(COPY_DIR) \"$$shell_path($$PWD\\data\\ControllerFiles\\ParameterFiles)\" \"$$shell_path($$OUT_PWD\\ControllerFiles\\)\"
copyTarget.depends += copyparameters
QMAKE_EXTRA_TARGETS += copyparameters

win32:copystructural.commands = $(COPY_DIR) \"$$shell_path($$PWD\\data\\StructuralFiles)\" \"$$shell_path($$OUT_PWD\\$$VARIANT\\StructuralFiles)\"
unix:copystructural.commands = $(COPY_DIR) \"$$shell_path($$PWD\\data\\StructuralFiles)\" \"$$shell_path($$OUT_PWD)\"
copyTarget.depends += copystructural
QMAKE_EXTRA_TARGETS += copystructural
