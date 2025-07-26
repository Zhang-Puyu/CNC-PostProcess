#include "MainWindow.h"
#pragma execution_character_set("utf-8")

#include "qsettings.h"
#include "qfiledialog.h"
#include "qdesktopservices.h"
#include "qevent.h"
#include "qmimedata.h"
#include "qmessagebox.h"
#include "qaction.h"

#include "ViewWidget.h"
#include "MessageWidget.h"

MainWindow::MainWindow(QWidget* parent)
	: QMainWindow(parent)
	, ui(new Ui::MainWindowClass())
{
	ui->setupUi(this);

	this->setAcceptDrops(true);

	connect(ui->buttonOpenFile, &QPushButton::clicked, this, &MainWindow::openFile);
	connect(ui->buttonSaveFile, &QPushButton::clicked, this, &MainWindow::saveFile);

	m_setting = new QSettings("./config.ini", QSettings::IniFormat);

	m_processer = new ToolpathProcesser();

	// �������߳�
	m_thread = new QThread();
	m_processer->moveToThread(m_thread);
	m_thread->start();

	auto valueChanged = qOverload<int>(&QSpinBox::valueChanged);
	connect(ui->spinSpindleSpeed, valueChanged, m_processer, &ToolpathProcesser::setSpindleSpeed);
	connect(ui->spinRapidFeed,    valueChanged, m_processer, &ToolpathProcesser::setRapidFeed);

	connect(ui->spinPrecisionCoordinate, valueChanged, m_processer, &ToolpathProcesser::setPrecisionCoordinate);
	connect(ui->spinPrecisionFeedrate,   valueChanged, m_processer, &ToolpathProcesser::setPrecisionFeedrate);

	connect(ui->checkLiquidCool, &QCheckBox::stateChanged, m_processer, &ToolpathProcesser::setLiquidtCool);
	connect(ui->checkLiquidCool, &QCheckBox::stateChanged, m_processer, &ToolpathProcesser::setGasCool);
	connect(ui->checkGrooveTurn, &QCheckBox::stateChanged, m_processer, &ToolpathProcesser::setGrooveTurn);	
	connect(ui->checkSmooth,	 &QCheckBox::stateChanged, m_processer, &ToolpathProcesser::setSmooth);
	connect(ui->checkComment,	 &QCheckBox::stateChanged, m_processer, &ToolpathProcesser::setComment);

	connect(this, &MainWindow::parseSignal,   m_processer, &ToolpathProcesser::parseMainProgam);
	connect(this, &MainWindow::processSignal, m_processer, &ToolpathProcesser::processMainProgram);
	connect(m_processer, &ToolpathProcesser::parsed, this, &MainWindow::parsedHandler);
	connect(m_processer, &ToolpathProcesser::processed, this, &MainWindow::processedHandler);

	connect(ui->buttonStartProcess, &QPushButton::clicked, [=]() {
		ui->progressBar->setMaximum(m_processer->getPointsCount());
		ui->progressBar->setValue(0);
		ui->textMpfCode->clear();
		emit processSignal();
		});

	// ��λ����ά���ӻ�
	connect(ui->visualButton, &QPushButton::clicked, [=]() {
		ViewWidget* viewWidget = new ViewWidget(nullptr);
		if (m_processer->getPointsCount())
		{
			viewWidget->showPointsCloud(m_processer->toolpaths());
			viewWidget->setWindowModality(Qt::ApplicationModal);
			viewWidget->show();
		}
		else
		{
			QMessageBox::critical(this, tr("����"), tr("  δ����  "), QMessageBox::Ok);
		}});

	// ����cls�ļ��з���Բ�����ʱ����������
	connect(m_processer, &ToolpathProcesser::findCircleCodeInClsFile, [=]() {
		QMessageBox::critical(this, tr("����ʱ����"),
		tr("��ʽ����cls�ļ��д���Բ�����\n����UG��������ʱ��<��������-�˶��������>����Ϊ<ֱ��>"),
		QMessageBox::Ok);
		});

	// ��Ȩ����
	QLabel* copyright = new QLabel("Copyright (C) 2023 NWPU-�����. a11 rights reserved.");
	ui->statusBar->addPermanentWidget(copyright);
	// ������Ϣ
	MessageWidget* messageWidget = new MessageWidget(this);
	connect(ui->buttonMessage, &QPushButton::clicked, messageWidget, &MessageWidget::show);
	// ������
	connect(m_processer, &ToolpathProcesser::progessValueChanged, ui->progressBar, &QProgressBar::setValue);

	readSetting();
}

MainWindow::~MainWindow()
{
	writeSetting();
	delete ui;
}

void MainWindow::readSetting()
{
	ui->spinSpindleSpeed->setValue(m_setting->value("SpindleSpeed", 3000).toInt());
	ui->spinRapidFeed->setValue(m_setting->value("RapidFeed", 3000).toInt());
	ui->checkLiquidCool->setChecked(m_setting->value("LiquidCool", true).toBool());
	ui->checkGasCool->setChecked(m_setting->value("GasCool", true).toBool());
	ui->checkSmooth->setChecked(m_setting->value("Smooth", true).toBool());
	ui->checkGrooveTurn->setChecked(m_setting->value("GrooveTurn", true).toBool());
	ui->checkComment->setChecked(m_setting->value("Comment", true).toBool());
}
void MainWindow::writeSetting()
{
	m_setting->setValue("SpindleSpeed", ui->spinSpindleSpeed->value());
	m_setting->setValue("RapidFeed",	 ui->spinRapidFeed->value());
	m_setting->setValue("LiquidCool",   ui->checkLiquidCool->isChecked());
	m_setting->setValue("GasCool",		 ui->checkGasCool->isChecked());
	m_setting->setValue("Smooth",		 ui->checkSmooth->isChecked());
	m_setting->setValue("GrooveTurn",   ui->checkGrooveTurn->isChecked());
	m_setting->setValue("Comment",		 ui->checkComment->isChecked());
}

void MainWindow::dragEnterEvent(QDragEnterEvent* ev)
{
	if (ev->mimeData()->hasUrls())
	{
		// �����϶������¼�
		ev->accept();
	}
}

void MainWindow::dropEvent(QDropEvent* ev)
{
	if (ev->mimeData()->hasUrls())
	{
		QList<QUrl> urls = ev->mimeData()->urls();
		QString filePath = urls.last().toLocalFile();
		if (filePath.contains(".cls", Qt::CaseInsensitive))
		{
			loadFile(filePath);
		}
	}
}

void MainWindow::parsedHandler()
{
	ui->visualButton->setEnabled(true);
	ui->buttonStartProcess->setEnabled(true);

	ui->progressBar->setValue(ui->progressBar->maximum());
}

void MainWindow::processedHandler(const QString& mpfCode)
{
	ui->buttonSaveFile->setEnabled(true);

	QString msg = "�����б�\n";
	for (const auto& toolpath : m_processer->toolpaths())
		if(!toolpath.name.isEmpty())
			msg += toolpath.name + "   |   " + toolpath.tool.name + "\n";
	if (!m_bLargeFile)
	{
		ui->textMpfCode->setText(mpfCode);
		QMessageBox::information(this, tr("�������"), msg, QMessageBox::Ok);
	}	
	else
	{
		ui->textMpfCode->setText(msg);
	}

	ui->progressBar->setValue(ui->progressBar->maximum());
}

void MainWindow::openFile()
{
	QString path = m_setting->value("path").toString();
	QString filePath = QFileDialog::getOpenFileName(
		this, tr("�򿪵����ļ�"), path, "CLS file (*.cls)");
	if (!filePath.isEmpty()) 
		loadFile(filePath);
}

void MainWindow::loadFile(const QString& filePath)
{
	QString path = m_setting->value("path").toString();

	ui->lineOpenFile->setText(filePath);

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::critical(this,
			tr("���ļ�ʧ��"),
			tr("�����ļ��Ƿ�ռ��"),
			QMessageBox::Ok);
		return;
	}

	ui->statusBar->showMessage(filePath);

	QTextStream stream(&file);
	ui->textClsCode->clear();
	ui->textMpfCode->clear();

	m_bLargeFile = file.size() > 5 * 1024 * 1024;
	if (m_bLargeFile)
	{
		auto result =
			QMessageBox::warning(this,
				tr("�ļ����󾯸�"),
				tr("�����ļ�������ʾ����ȫ�Ļ���ֿ���\n���Ok����ʾȫ�ģ����Cancel��������ʾ"),
				QMessageBox::Ok, QMessageBox::Cancel);

		m_bLargeFile = (result == QMessageBox::Ok);
	}

	if (!m_bLargeFile)
	{
		ui->textClsCode->append(stream.readAll());
		ui->progressBar->setMaximum(ui->textClsCode->toPlainText().count('\n'));
	}
	else
	{
		ui->textClsCode->append(file.fileName() + "�ѵ���");
		ui->progressBar->setMaximum(stream.readAll().count('\n'));
	}

	ui->progressBar->setValue(0);
	file.close();

	ui->buttonSaveFile->setEnabled(false);

	m_clsFile = filePath;
	path = filePath.left(filePath.lastIndexOf('/'));
	m_setting->setValue("path", path);

	m_processer->setClsFile(m_clsFile);
	emit parseSignal();
};

void MainWindow::saveFile()
{
	QString filePath = QFileDialog::getSaveFileName(this,
		tr("����NC����"), m_clsFile.replace(".cls", ".mpf"), tr("mpf (*.mpf)"));
	if (!filePath.isEmpty())
	{
		ui->lineSaveFile->setText(filePath);

		QFile file(filePath);
		if (!file.open(QIODevice::WriteOnly))
		{
			QMessageBox::critical(this, 
				tr("���ļ�ʧ��"), 
				tr("�����ļ��Ƿ�ռ��"), 
				QMessageBox::Ok);
			return;
		}
			
		QTextStream stream(&file);
		stream << m_processer->getProcessResult();
		file.close();

		QString path = filePath.left(filePath.lastIndexOf('/'));
		// ��ϵͳ�ļ�������
		QDesktopServices::openUrl(QUrl::fromLocalFile(path));
	}
}


