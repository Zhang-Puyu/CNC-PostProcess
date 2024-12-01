#pragma once

#include <QMainWindow>
#include "ui_MainWindow.h"

#include "qlabel.h"
#include "qthread.h"

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

	void openConfig();
	void saveConfig();

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

	/// @brief �����ļ��������
	void onParsed();
	/// @brief ���ô������
	/// @param mpfCode ���ú��NC�������
	void onProcessed(const QString& mpfCode);

signals:
	void doParseSignal();
	void doProcessSignal();
};
