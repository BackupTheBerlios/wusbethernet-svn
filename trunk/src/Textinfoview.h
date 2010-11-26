/*
 * TextInfoView.h
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-26
 */
#ifndef _TEXTINFOVIEW_H
#define _TEXTINFOVIEW_H

#include <QtGui/QDialog>
#include "ui_Textinfoview.h"

class TextInfoView : public QDialog
{
    Q_OBJECT

public:
    static TextInfoView & getInstance();

    TextInfoView(QWidget *parent = 0);
    ~TextInfoView();
    void showText( const QString & text );
private:
    static TextInfoView * singleton;
    Ui::TextInfoViewClass ui;
private slots:
	void closeWindow();
};

#endif // _TEXTINFOVIEW_H
