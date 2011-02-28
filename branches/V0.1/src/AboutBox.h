#ifndef ABOUTBOX_H
#define ABOUTBOX_H

#include <QtGui/QDialog>
#include "ui_AboutBox.h"

class AboutBox : public QDialog
{
    Q_OBJECT

public:
    AboutBox(QWidget *parent = 0);
    ~AboutBox();

private:
    Ui::AboutBoxClass ui;
private slots:
	void QtAboutButton();

};

#endif // ABOUTBOX_H
