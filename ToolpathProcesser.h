#pragma once

#include "qstring"
#include "qvector.h"
#include "qdebug.h"
#include "qtextedit.h"
#include "qspinbox.h"
#include "qthread.h"

struct Tool
{
	QString name;
	float D;  // ֱ��
	float R1; // �°뾶
	float L;  // ����
	float A;  // ���
	float B;  // ׶��
	QString toString()
	{
		QString str;
		str += "name: " + name + "\n";
		str += "D: " + QString::number(D) + "\n";
		str += "R1: " + QString::number(R1) + "\n";
		str += "L: " + QString::number(L) + "\n";
		str += "A: " + QString::number(A) + "\n";
		str += "B: " + QString::number(B) + "\n";
		return str;
	}
};

struct Point
{
	float x = 0, y = 0, z = 0, f = 0, color = 0;
};
typedef QVector<Point> Points;

struct Toolpath
{
	Points points;
	int pointsCount() const { return points.count(); }

	Tool tool;
	QString name;
};
typedef QVector<Toolpath> Toolpaths;

class ToolpathProcesser : public QObject
{
	Q_OBJECT

public:
	ToolpathProcesser(QObject* parent = nullptr);
	~ToolpathProcesser();

	void parseMainProgam();
	void parseSubProgam(QTextStream& in, Toolpath& toolpath);
	void processMainProgram();
	void processSubProgram(const Toolpath& toolpath);

	void setClsFile(const QString& filePath) { m_clsFile = filePath; }
	const QString& getProcessResult()		 { return m_mpfCode; }

	void setSpindleSpeed(int spindleSpeed) { m_spindleSpeed = spindleSpeed;}
	void setRapidFeed(float rapidFeed)	   { m_rapidFeed    = rapidFeed;   }

	void setLiquidtCool(bool bCool)		   { m_bLiquidCool = bCool;       }
	void setGasCool(bool bCool)			   { m_bGasCool    = bCool;       }
	void setSmooth(bool bSmooth)		   { m_bSmooth	   = bSmooth;     }
	void setComment(bool bComment)	       { m_bComment    = bComment;    }
	void setGrooveTurn(bool bGrooveTurn)   { m_bGrooveTurn = bGrooveTurn; }

	void setPrecisionFeedrate(int precisionFeedrate) { 
		m_precisionFeedrate = precisionFeedrate; }
	void setPrecisionCoordinate(int precisionCoordinate) { 
		m_precisionCoordinate = precisionCoordinate; }

	int getPointsCount()    const;
	int getToolpathsCount() const;

	const Toolpaths& toolpaths() { return m_toolpaths; }

	friend class ViewWidget;
		
private:

	QString coor2str(const float& coor) const 
	{ return QString::number(coor, 'f', m_precisionCoordinate); };
	QString feed2str(const float& feed) const 
	{ return QString::number(feed, 'f', m_precisionFeedrate);   };

	/// @brief Դ�ļ�·��
	QString m_clsFile;
	/// @brief ���õ�NC����
	QString m_mpfCode;

	/// @brief �ӳ��򵶹�
	Toolpaths m_toolpaths;

	/// @brief ����ת��
	int m_spindleSpeed = 3000;
	/// @brief ���ٽ���
	float m_rapidFeed  = 3000;

	/// @brief Һ��
	bool m_bLiquidCool = true;
	/// @brief ����
	bool m_bGasCool    = true;
	/// @brief �켣��˳��ָ��
	bool m_bSmooth	   = true;
	/// @brief ע�ʹ���
	bool m_bComment	   = false;
	/// @brief ��м����ת
	bool m_bGrooveTurn = false;

	/// @brief ���ý���
	int m_progressValue = 0;

	/// @brief �����ٶȾ���
	unsigned int m_precisionFeedrate = 0;
	/// @brief ���꾫��
	unsigned int m_precisionCoordinate = 3;

signals:
	/// @brief cls�ļ��������
	void parsed();
	/// @brief ������ô������
	void processed(const QString& mpfCode);

	void progessValueChanged(const int& value);

	void findSecondToolpathInClsFile();
	void findCircleCodeInClsFile();
};

