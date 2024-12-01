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

	m_processer = new ToolpathProcesser();

	auto valueChanged = qOverload<int>(&QSpinBox::valueChanged);
	connect(ui->spinSpindleSpeed, valueChanged, m_processer, &ToolpathProcesser::setSpindleSpeed);
	connect(ui->spinRapidFeed,    valueChanged, m_processer, &ToolpathProcesser::setRapidFeed);
	m_processer->setSpindleSpeed(ui->spinSpindleSpeed->value());
	m_processer->setRapidFeed(ui->spinRapidFeed->value());
	connect(ui->spinPrecisionCoordinate, valueChanged, m_processer, &ToolpathProcesser::setPrecisionCoordinate);
	connect(ui->spinPrecisionFeedrate,   valueChanged, m_processer, &ToolpathProcesser::setPrecisionFeedrate);
	m_processer->setPrecisionCoordinate(ui->spinPrecisionCoordinate->value());
	m_processer->setPrecisionFeedrate(ui->spinPrecisionFeedrate->value());

	connect(ui->checkLiquidCool, &QCheckBox::stateChanged, m_processer, &ToolpathProcesser::setLiquidtCool);
	connect(ui->checkLiquidCool, &QCheckBox::stateChanged, m_processer, &ToolpathProcesser::setGasCool);
	connect(ui->checkGrooveTurn, &QCheckBox::stateChanged, m_processer, &ToolpathProcesser::setGrooveTurn);	
	connect(ui->checkSmooth,	 &QCheckBox::stateChanged, m_processer, &ToolpathProcesser::setSmooth);
	connect(ui->checkComment,	 &QCheckBox::stateChanged, m_processer, &ToolpathProcesser::setComment);

	// 移至子线程
	m_thread = new QThread();
	m_processer->moveToThread(m_thread);

	connect(this, &MainWindow::doParseSignal,   m_processer, &ToolpathProcesser::parseMainProgam);
	connect(this, &MainWindow::doProcessSignal, m_processer, &ToolpathProcesser::processMainProgram);
	connect(m_processer, &ToolpathProcesser::parsed, this, &MainWindow::onParsed);
	connect(m_processer, &ToolpathProcesser::processed, this, &MainWindow::onProcessed);

	connect(ui->buttonStartProcess, &QPushButton::clicked, [=]() {
		ui->progressBar->setMaximum(m_processer->getPointsCount());
		ui->progressBar->setValue(0);
		ui->textMpfCode->clear();
		emit doProcessSignal();
		});

	// 刀位点三维可视化
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
			QMessageBox::critical(this, tr("错误"), tr("  未后置  "), QMessageBox::Ok);
		}});

	// 当在cls文件中发现圆弧语句时，弹出警告
	connect(m_processer, &ToolpathProcesser::findCircleCodeInClsFile, [=]() {
		QMessageBox::critical(this, tr("导入时出错"),
		tr("格式错误，cls文件中存在圆弧语句\n请在UG导出刀轨时将<机床控制-运动输出类型>设置为<直线>"),
		QMessageBox::Ok);
		});

	// 版权声明
	QLabel* copyright = new QLabel("Copyright (C) 2023 NWPU-张璞玉. a11 rights reserved.");
	ui->statusBar->addPermanentWidget(copyright);
	// 帮助信息
	MessageWidget* messageWidget = new MessageWidget(this);
	connect(ui->buttonMessage, &QPushButton::clicked, messageWidget, &MessageWidget::show);
	// 进度条
	connect(m_processer, &ToolpathProcesser::progessValueChanged, ui->progressBar, &QProgressBar::setValue);

	openConfig();
}

MainWindow::~MainWindow()
{
	saveConfig();
	delete ui;
}

void MainWindow::openConfig()
{
	QSettings setting("./config.ini", QSettings::IniFormat);
	ui->spinSpindleSpeed->setValue(setting.value("SpindleSpeed", 3000).toInt());
	m_processer->setSpindleSpeed(ui->spinSpindleSpeed->value());

	ui->spinRapidFeed->setValue(setting.value("RapidFeed", 3000).toInt());
	m_processer->setRapidFeed(ui->spinRapidFeed->value());

	ui->checkLiquidCool->setChecked(setting.value("LiquidCool", true).toBool());
	m_processer->setLiquidtCool(ui->checkLiquidCool->isChecked());
	ui->checkGasCool->setChecked(setting.value("GasCool", true).toBool());
	m_processer->setGasCool(ui->checkGasCool->isChecked());
	ui->checkSmooth->setChecked(setting.value("Smooth", true).toBool());
	m_processer->setSmooth(ui->checkSmooth->isChecked());
	ui->checkGrooveTurn->setChecked(setting.value("GrooveTurn", true).toBool());
	m_processer->setGrooveTurn(ui->checkGrooveTurn->isChecked());
	ui->checkComment->setChecked(setting.value("Comment", true).toBool());
	m_processer->setComment(ui->checkComment->isChecked());
}
void MainWindow::saveConfig()
{
	QSettings setting("./config.ini", QSettings::IniFormat);
	setting.setValue("SpindleSpeed", ui->spinSpindleSpeed->value());
	setting.setValue("RapidFeed",	 ui->spinRapidFeed->value());
	setting.setValue("LiquidCool",   ui->checkLiquidCool->isChecked());
	setting.setValue("GasCool",		 ui->checkGasCool->isChecked());
	setting.setValue("Smooth",		 ui->checkSmooth->isChecked());
	setting.setValue("GrooveTurn",   ui->checkGrooveTurn->isChecked());
	setting.setValue("Comment",		 ui->checkComment->isChecked());
}

void MainWindow::dragEnterEvent(QDragEnterEvent* ev)
{
	if (ev->mimeData()->hasUrls())
	{
		// 接收拖动进入事件
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

void MainWindow::onParsed()
{
	ui->visualButton->setEnabled(true);
	ui->buttonStartProcess->setEnabled(true);

	ui->progressBar->setValue(ui->progressBar->maximum());
}

void MainWindow::onProcessed(const QString& mpfCode)
{
	ui->buttonSaveFile->setEnabled(true);

	QString msg = "刀轨列表：\n";
	for (const auto& toolpath : m_processer->toolpaths())
		if(!toolpath.name.isEmpty())
			msg += toolpath.name + "   |   " + toolpath.tool.name + "\n";
	if (!m_bLargeFile)
	{
		ui->textClsCode->setText(mpfCode);
		QMessageBox::information(this, tr("后置完成"), msg, QMessageBox::Ok);
	}	
	else
	{
		ui->textMpfCode->setText(msg);
	}

	ui->progressBar->setValue(ui->progressBar->maximum());
}

void MainWindow::openFile()
{
	QSettings setting("./config.ini", QSettings::IniFormat);
	QString path = setting.value("path").toString();
	QString filePath = QFileDialog::getOpenFileName(
		this, tr("打开刀轨文件"), path, "CLS file (*.cls)");
	if (!filePath.isEmpty()) 
		loadFile(filePath);
}
void MainWindow::loadFile(const QString& filePath)
{
	QSettings setting("./config.ini", QSettings::IniFormat);
	QString path = setting.value("path").toString();

	ui->lineOpenFile->setText(filePath);

	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::critical(this,
			tr("打开文件失败"),
			tr("请检查文件是否被占用"),
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
				tr("文件过大警告"),
				tr("刀轨文件过大，显示刀轨全文会出现卡顿\n点击Ok则不显示全文，点击Cancel则依旧显示"),
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
		ui->textClsCode->append(file.fileName() + "已导入");
		ui->progressBar->setMaximum(stream.readAll().count('\n'));
	}

	ui->progressBar->setValue(0);
	file.close();

	ui->buttonSaveFile->setEnabled(false);

	m_clsFile = filePath;
	path = filePath.left(filePath.lastIndexOf('/'));
	setting.setValue("path", path);

	m_processer->setClsFile(m_clsFile);
	emit doParseSignal();
};

void MainWindow::saveFile()
{
	QString filePath = QFileDialog::getSaveFileName(this,
		tr("保存NC代码"), m_clsFile.replace(".cls", ".mpf"), tr("mpf (*.mpf)"));
	if (!filePath.isEmpty())
	{
		ui->lineSaveFile->setText(filePath);

		QFile file(filePath);
		if (!file.open(QIODevice::WriteOnly))
		{
			QMessageBox::critical(this, 
				tr("打开文件失败"), 
				tr("请检查文件是否被占用"), 
				QMessageBox::Ok);
			return;
		}
			
		QTextStream stream(&file);
		stream << m_processer->getProcessResult();
		file.close();

		QString path = filePath.left(filePath.lastIndexOf('/'));
		// 打开系统文件管理器
		QDesktopServices::openUrl(QUrl::fromLocalFile(path));
	}
}


