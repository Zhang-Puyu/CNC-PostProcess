#include "ToolpathProcesser.h"
#include "qfile.h"
#include "qfileinfo.h"

const unsigned int X = 0;
const unsigned int Y = 1;
const unsigned int Z = 2;
const unsigned int F = 3;
const unsigned int C = 4;

ToolpathProcesser::ToolpathProcesser(QObject* parent)
	: QObject(parent)
{

};

ToolpathProcesser::~ToolpathProcesser()
{

}

void ToolpathProcesser::parseMainProgam()
{
	if(m_clsFile.isEmpty())
		return;

	// read file
	QFile file(m_clsFile);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		qDebug() << "Open failed.";
		return;
	}
	qDebug() << "import apt file: " << m_clsFile;
	QTextStream in(&file);
	
	m_toolpaths.clear();
	m_toolpaths.append(Toolpath());
	parseSubProgam(in, m_toolpaths[m_toolpaths.count() - 1]);

	file.close();

	emit parsed();
}

void ToolpathProcesser::parseSubProgam(QTextStream& in, Toolpath& toolpath)
{
	QString line = in.readLine();
	if (line.contains("TOOL PATH/"))
	{	
		toolpath.tool.name = line.split(",").last();
		toolpath.name = line.split(",").first().remove("TOOL PATH/");
		qDebug() << "Tool Path: " << toolpath.name;
	};

	// second line
	line = in.readLine();
	if (line.contains("TLDATA/MILL,"))
	{
		line = line.remove("TLDATA/MILL,");
		toolpath.tool.D = line.split(",").at(0).toFloat();
		toolpath.tool.R1 = line.split(",").at(1).toFloat();
		toolpath.tool.L = line.split(",").at(2).toFloat();
		toolpath.tool.B = line.split(",").at(3).toFloat();
		toolpath.tool.A = line.split(",").at(4).toFloat();
	};

	// third line
	line = in.readLine();

	// centerline data
	float feedRate = 0;
	float color = 0;
	bool bRapid = false;
	while (!line.isNull())
	{
		line = in.readLine();
		emit progessValueChanged(++m_progressValue);
		if (line.contains("RAPID"))
		{
			bRapid = true;
			continue;
		}
		if (line.contains("GOTO/"))
		{
			line.remove(("GOTO/"));
			if (bRapid)
			{
				toolpath.points.push_back(
					Point{
						line.split(",").at(X).toFloat(),
						line.split(",").at(Y).toFloat(),
						line.split(",").at(Z).toFloat(),
						m_rapidFeed, color });
				bRapid = false;
			}
			else
			{
				toolpath.points.push_back(
					Point{
						line.split(",").at(X).toFloat(),
						line.split(",").at(Y).toFloat(),
						line.split(",").at(Z).toFloat(),
						feedRate, color });
			}
			continue;
		}
		if (line.contains("END-OF-PATH"))
		{
			m_toolpaths.append(Toolpath());
			parseSubProgam(in, m_toolpaths[m_toolpaths.count() - 1]);
			return;
		}
		if (line.contains("FEDRAT/MMPM"))
		{
			feedRate = line.split(",").last().toFloat();
			continue;
		}
		if (line.contains("PAINT/COLOR"))
		{
			color = line.split(",").last().toFloat();
			continue;
		}
		if (line.contains("FEDRAT/"))
		{
			feedRate = line.split("/").last().toFloat();
			continue;
		}
		if (line.contains("CIRCLE/"))
		{
			emit findCircleCodeInClsFile();
			return;
		}
		
	}
}

void ToolpathProcesser::processMainProgram()
{
	m_mpfCode.clear();

	if(m_toolpaths.isEmpty())
		return;

	for(const Toolpath& toolpath : m_toolpaths)
		processSubProgram(toolpath);

	emit processed(m_mpfCode);

	return;
}

void ToolpathProcesser::processSubProgram(const Toolpath& toolpath)
{
	if (toolpath.pointsCount() < 1)
		return;

	if (m_bComment)
	{
		m_mpfCode += "; Tool Path: " + toolpath.name + "\n";
		m_mpfCode += "; Tool: " + toolpath.tool.name + "\n";
	}

	int n = 0;
	// first line
	m_mpfCode += "N" + QString::number(n)
		+ " G90"	// 绝对坐标系
		+ " G54"	// 指定工件坐标系
		+ " G40";   // 取消刀具半径补偿

	// second line
	m_mpfCode += "\nN" + QString::number(++n)
		+ " M3"				// 主轴正转
		+ " S" + QString::number(m_spindleSpeed);// 主轴转速

	if (m_bGasCool)
	{
		if (m_bComment) m_mpfCode += "\n\n;Gas-Cool On";
		m_mpfCode += "\nN" + QString::number(++n) + " M7";  // 气体开
	}
	if (m_bLiquidCool)
	{
		if (m_bComment) m_mpfCode += "\n;Liquid-Cool On";
		m_mpfCode += "\nN" + QString::number(++n) + " " + "M8";  // 冷却液开
	}

	if (m_bGrooveTurn)
	{
		if (m_bComment) m_mpfCode += "\n;Groove Turn";
		m_mpfCode += "\nN" + QString::number(++n) + " M21";  // 排屑槽正转
	}


	if (m_bSmooth)
	{
		if (m_bComment) m_mpfCode += "\n\n;Soft Code";
		m_mpfCode += "\nFFWON";
		m_mpfCode += "\nSOFT";
		m_mpfCode += "\nG641 ADIS=0.1";
	}

	auto it = toolpath.points.begin();
	// 为保证安全，先抬刀
	if (m_bComment) m_mpfCode += "\n\n; go to safe height";
	m_mpfCode += "\nN" + QString::number(++n) + " G01"
		+ " Z" + coor2str(it->z > 200 ? it->z : 200)
		+ " F" + feed2str(m_rapidFeed);

	// 水平面移动
	m_mpfCode += "\nN" + QString::number(++n)
		+ " X" + coor2str(it->x)
		+ " Y" + coor2str(it->y)
		+ " Z" + coor2str(it->z > 200 ? it->z : 200)
		+ "\n";

	// first point
	m_mpfCode += "\n\nN" + QString::number(++n)
		+ " X" + coor2str(it->x)
		+ " Y" + coor2str(it->y)
		+ " Z" + coor2str(it->z);

	float feedRate = it->f;
	float color = it->color;
	float x = it->x;
	float y = it->y;
	float z = it->z;
	while (toolpath.points.end() != ++it)
	{
		emit progessValueChanged(++m_progressValue);

		m_mpfCode += "\nN" + QString::number(++n);
		if (x != it->x)
		{
			m_mpfCode += " X" + coor2str(it->x);
			x = it->x;
		}
		if (y != it->y)
		{
			m_mpfCode += " Y" + coor2str(it->y);
			y = it->y;
		}
		if (z != it->z)
		{
			m_mpfCode += " Z" + coor2str(it->z);
			z = it->z;
		}
		if (feedRate != it->f)
		{
			m_mpfCode += " F" + feed2str(it->f);
			feedRate = it->f;
		}
	}

	// 抬刀
	if (m_bComment)
		m_mpfCode += "\n\n; go to safe height";
	m_mpfCode += "\nN" + QString::number(++n) +
		" Z" + coor2str(toolpath.points.last().z > 200 ? toolpath.points.last().z : 200) + 
		" F" + feed2str(m_rapidFeed);

	// 程序结束
	m_mpfCode += "\n\nN" + QString::number(++n);
	if (m_bLiquidCool || m_bGasCool)
		m_mpfCode += " M9";    // 冷却液关
	if (m_bGrooveTurn)
		m_mpfCode += " M22";   // 排屑槽关
	m_mpfCode += " M30\n";	   // 程序结束

	if (m_bComment)
		m_mpfCode += "\n; End of Program\n\n";
}

int ToolpathProcesser::getPointsCount() const
{
	int i = 0;
	for (const Toolpath& t : m_toolpaths)
		i += t.pointsCount();
	return i;
}

int ToolpathProcesser::getToolpathsCount() const
{
	return m_toolpaths.count();
}
