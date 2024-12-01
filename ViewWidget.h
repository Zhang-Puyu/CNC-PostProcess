#pragma once

#include "QVTKOpenGLWidget.h"
#include "qvector.h"
#include "qdebug.h"

#include "vtkSmartPointer.h"
#include "vtkActor.h"
#include "vtkAxesActor.h"
#include "vtkScalarBarActor.h"

#include "vtkAutoInit.h"
#ifndef VTKRENDERINGOPENGL2
#define VTKRENDERINGOPENGL2
VTK_MODULE_INIT(vtkRenderingOpenGL2)
#endif 
#ifndef VTKRENDERINGFREETYPE
#define VTKRENDERINGFREETYPE
VTK_MODULE_INIT(vtkRenderingFreeType)
#endif
#ifndef VTKINTERACTIONSTYLE
#define VTKINTERACTIONSTYLE
VTK_MODULE_INIT(vtkInteractionStyle)
#endif 
#ifndef VTKRENDERINGVOLUMEOPENGL2
#define VTKRENDERINGVOLUMEOPENGL2
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2);
#endif 

#include "qthread.h"

#include "ToolpathProcesser.h"

class ViewWidget : public QVTKOpenGLWidget
{
	Q_OBJECT

public:
	ViewWidget(QWidget* parent = nullptr);
	~ViewWidget();

	void showPointsCloud(const Points&    points);
	void showPointsCloud(const Toolpaths& toolpaths);
	void clear();

private:
	vtkSmartPointer<vtkAxesActor>	   axes      = vtkSmartPointer<vtkAxesActor>::New();
	vtkSmartPointer<vtkScalarBarActor> scalarBar = vtkSmartPointer<vtkScalarBarActor>::New();

	vtkSmartPointer<vtkActor>		   actor	 = vtkSmartPointer<vtkActor>::New();
	vtkSmartPointer<vtkRenderer>       renderer	 = vtkSmartPointer<vtkRenderer>::New();
};

