#include "MessageWidget.h"

MessageWidget::MessageWidget(QWidget *parent)
	: QDialog(parent)
	, ui(new Ui::MessageWidgetClass())
{
	ui->setupUi(this);
}

MessageWidget::~MessageWidget()
{
	delete ui;
}
