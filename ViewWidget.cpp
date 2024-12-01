#include "ViewWidget.h"

#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkPointData.h"
#include "vtkPolyVertex.h"
#include "vtkDoubleArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDataSetMapper.h"
#include "vtkScalarBarActor.h"
#include "vtkLookupTable.h"
#include "vtkTextActor.h"
#include "vtkProperty.h"
#include "vtkTextProperty.h"
#include "vtkCellArray.h"
#include "vtkPolyLine.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include "vtkCellData.h"

const unsigned int X = 0;
const unsigned int Y = 1;
const unsigned int Z = 2;
const unsigned int F = 3;


ViewWidget::ViewWidget(QWidget* parent)
	: QVTKOpenGLWidget(parent)
{
	// 设置坐标轴
	axes->SetTotalLength(20, 20, 20);
	axes->SetTipTypeToCone();
	axes->AxisLabelsOn();

	axes->GetXAxisCaptionActor2D()->GetTextActor()->SetTextScaleModeToNone();
	axes->GetXAxisCaptionActor2D()->GetCaptionTextProperty()->SetFontSize(10);
	axes->GetXAxisCaptionActor2D()->GetCaptionTextProperty()->SetColor(1, 0, 0);

	axes->GetYAxisCaptionActor2D()->GetTextActor()->SetTextScaleModeToNone();
	axes->GetYAxisCaptionActor2D()->GetCaptionTextProperty()->SetFontSize(10);
	axes->GetYAxisCaptionActor2D()->GetCaptionTextProperty()->SetColor(0, 1, 0);

	axes->GetZAxisCaptionActor2D()->GetTextActor()->SetTextScaleModeToNone();
	axes->GetZAxisCaptionActor2D()->GetCaptionTextProperty()->SetFontSize(10);
	axes->GetZAxisCaptionActor2D()->GetCaptionTextProperty()->SetColor(0, 0, 1);

	scalarBar->GetTitleTextProperty()->SetColor(0.1, 0.1, 0.1);
	scalarBar->GetTitleTextProperty()->SetFontSize(3);
	scalarBar->SetNumberOfLabels(5);
	scalarBar->SetHeight(0.7);
	scalarBar->SetWidth(0.1);

	// 设置背景
	renderer->SetBackground(0.1, 0.2, 0.4);
	renderer->SetBackground2(1, 1, 1);
	renderer->SetGradientBackground(1);

	renderer->AddActor(actor);
	renderer->AddActor(axes);
	renderer->AddActor(scalarBar);

	this->GetRenderWindow()->AddRenderer(renderer);
	this->GetRenderWindow()->Render();
}

ViewWidget::~ViewWidget()
{

}

void ViewWidget::showPointsCloud(const Points& points)
{
	float minFeedrate = std::min_element(points.begin(), points.end(),
		[=](Point p, Point q)->bool {return p.f < q.f; })->f;
	float maxFeedrate = std::max_element(points.begin(), points.end(),
		[=](Point p, Point q)->bool {return p.f > q.f; })->f;

	// 几何数据
	vtkSmartPointer<vtkPoints> pointsData = vtkSmartPointer<vtkPoints>::New();
	// 拓扑数据
	vtkSmartPointer<vtkPolyVertex> polyVertex = vtkSmartPointer<vtkPolyVertex>::New();
	// 属性数据
	vtkSmartPointer<vtkDoubleArray> pointsScalars = vtkSmartPointer<vtkDoubleArray>::New();
	for (int i = 0; i < points.count(); i++)
	{
		pointsData->InsertNextPoint(points[i].x, points[i].y, points[i].z);
		// 第一个参数是几何point的ID号，第2个参数是拓扑中的Id号
		polyVertex->GetPointIds()->InsertNextId(polyVertex->GetNumberOfPoints());
		// 第1个参数是points点的Id，第2个参数是该点的属性值
		pointsScalars->InsertNextTuple1(points[i].f);
	}

	// 颜色表
	vtkSmartPointer<vtkLookupTable> lut = vtkSmartPointer<vtkLookupTable>::New();
	lut->SetNumberOfColors(256);
	lut->SetRange(minFeedrate, maxFeedrate);
	lut->Build();

	scalarBar->SetLookupTable(lut);
	scalarBar->SetTitle("Feedrate");

	// 将以上三部分数据组合成一个结构为vtkUnstructureGrid数据集
	vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
	grid->Allocate(1, 1);
	grid->SetPoints(pointsData);
	grid->InsertNextCell(polyVertex->GetCellType(), polyVertex->GetPointIds()); //设置映射器
	grid->GetPointData()->SetScalars(pointsScalars);

	vtkSmartPointer<vtkDataSetMapper> pointsMapper = vtkSmartPointer<vtkDataSetMapper>::New();
	// 根据属性数据的最大、最小值，将颜色表和属性数据关联
	pointsMapper->SetInputData(grid);
	pointsMapper->ScalarVisibilityOn();
	pointsMapper->SetScalarRange(minFeedrate, maxFeedrate);
	pointsMapper->SetLookupTable(lut);
	pointsMapper->SetColorModeToDefault();

	actor->SetMapper(pointsMapper);
	actor->GetProperty()->SetRepresentationToPoints();
	actor->GetProperty()->SetPointSize(3);

	this->GetRenderWindow()->Render();
}

void ViewWidget::showPointsCloud(const Toolpaths& toolpaths)
{
	Points points;
	for (const Toolpath& toolpath : toolpaths)
		points += toolpath.points;
	showPointsCloud(points);
}

void ViewWidget::clear()
{
	renderer->RemoveAllViewProps();
	this->GetRenderWindow()->Render();
}
