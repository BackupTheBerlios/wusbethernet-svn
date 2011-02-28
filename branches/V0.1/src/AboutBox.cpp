#include "AboutBox.h"
#include "ConfigManager.h"
#include <QMessageBox>

AboutBox::AboutBox(QWidget *parent)
    : QDialog(parent)
{
	ui.setupUi(this);
	ConfigManager & cm = ConfigManager::getInstance();
	ui.label_5->setText( cm.getStringValue("main.version", "0.0"));
	ui.label_7->setText( cm.getStringValue("main.build", QString(__DATE__ " " __TIME__)));
}

AboutBox::~AboutBox()
{

}
void AboutBox::QtAboutButton() {
	QMessageBox::aboutQt( this, "About QT" );
}
