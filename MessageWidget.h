#pragma once

#include <QDialog>
#include "ui_MessageWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MessageWidgetClass; };
QT_END_NAMESPACE

class MessageWidget : public QDialog
{
	Q_OBJECT

public:
	MessageWidget(QWidget *parent = nullptr);
	~MessageWidget();

private:
	Ui::MessageWidgetClass *ui;
};
