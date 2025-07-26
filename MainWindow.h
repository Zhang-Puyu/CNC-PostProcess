#pragma once

#include <QMainWindow>
#include "ui_MainWindow.h"

#include "qlabel.h"
#include "qthread.h"
#include "qfile.h"
#include "qsettings.h"

#include "ToolpathProcesser.h"
#include "MessageWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindowClass; };
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();
	
	void openFile();
	void loadFile(const QString& filePath);
	void saveFile();

	void readSetting();
	void writeSetting();

protected:
	void dragEnterEvent(QDragEnterEvent* ev) override;
	void dropEvent(QDropEvent* ev) override;

private:
	Ui::MainWindowClass* ui;

	/// @brief Դ�ļ�·��	
	QString m_clsFile;
	/// @brief ���ô���
	ToolpathProcesser* m_processer;
	/// @brief �����߳�
	QThread* m_thread;
	/// @brief �����ļ�����
	bool m_bLargeFile = true;

	QSettings* m_setting;

	/// @brief �����ļ��������
	void parsedHandler();
	/// @brief ���ô������
	/// @param mpfCode ���ú��NC�������
	void processedHandler(const QString& mpfCode);

signals:
	/// @brief �����ļ�������ʼ�ź�
	void parseSignal();
	/// @brief ������ô���ʼ�ź�
	void processSignal();
};
