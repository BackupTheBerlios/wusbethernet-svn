#ifndef PREFERENCESBOX_H
#define PREFERENCESBOX_H

#include <QtGui/QDialog>
#include "ui_preferencesbox.h"

class PreferencesBox : public QDialog
{
    Q_OBJECT

public:
    PreferencesBox(QWidget *parent = 0);
    ~PreferencesBox();

private:
    Ui::PreferencesBoxClass ui;
    /** Sets values from configuration to GUI */
    void setValues();
    /** Sets values from GUI into configuration */
    void saveValues();
};

#endif // PREFERENCESBOX_H
