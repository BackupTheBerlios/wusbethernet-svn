/*
 * TextInfoView.cpp
 *
 * @author:		Sebastian Kolbe-Nusser &lt;Sebastian DOT Kolbe AT gmail DOT com&gt;
 * @version:	$Id$
 * @created:	2010-10-26
 */

#include "Textinfoview.h"

TextInfoView * TextInfoView::singleton = NULL;

TextInfoView::TextInfoView(QWidget *parent)
    : QDialog(parent)
{
	ui.setupUi(this);
}

TextInfoView::~TextInfoView()
{

}

TextInfoView & TextInfoView::getInstance() {
	TextInfoView * temp = singleton;
	if ( temp == NULL ) {
		temp = new TextInfoView();
		singleton = temp;
	}
	return *singleton;
}

void TextInfoView::showText( const QString & text ) {
	if ( !this->isVisible() )
		this->show();
	else
		this->raise();
	ui.textBrowser->setHtml( text );
}

void TextInfoView::closeWindow() {
	if (this->isVisible() )
		this->hide();
}

