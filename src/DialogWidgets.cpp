#include "DialogWidgets.hpp"
#include "Logger.hpp"
#include "Globals.hpp"

#include <Wt/WContainerWidget>
#include <Wt/WPushButton>
#include <Wt/WText>
#include <Wt/WDialog>
#include <Wt/WLineEdit>
#include <Wt/WTextArea>

using namespace W;

void W::showTermsOfUsage(void)
{
	Wt::WDialog dialog(Wt::WString::tr("dialog.terms-of-usage.title"));
	Wt::WText * text = new Wt::WText(Wt::WString::tr("dialog.terms-of-usage.text"), dialog.contents());
	new Wt::WBreak(dialog.contents());
	Wt::WPushButton ok(Wt::WString::tr("dialog.terms-of-usage.exit_button"), dialog.contents());

	text->setStyleClass("dialog-window-base");

	Wt::WApplication::instance()->globalEscapePressed().connect(&dialog, &Wt::WDialog::accept);
	ok.clicked().connect(&dialog, &Wt::WDialog::accept);
	dialog.exec();
}

void W::showCreateAccountDialog(void)
{
	Wt::WDialog dialog(Wt::WString::tr("dialog.create_account.title"));
	Wt::WText * text = new Wt::WText(Wt::WString::tr("dialog.create_account.text"), dialog.contents());
	new Wt::WBreak(dialog.contents());
	Wt::WPushButton ok(Wt::WString::tr("dialog.create_account.exit_button"), dialog.contents());

	text->setStyleClass("dialog-window-base");

	Wt::WApplication::instance()->globalEscapePressed().connect(&dialog, &Wt::WDialog::accept);
	ok.clicked().connect(&dialog, &Wt::WDialog::accept);
	dialog.exec();
}

void W::showVerifyEmailNotificationDialog(void)
{
	Wt::WDialog dialog(Wt::WString::tr("dialog.verify_email.title"));
	Wt::WText * listName = new Wt::WText(Wt::WString::tr("dialog.verify_email.text"), dialog.contents());
	new Wt::WBreak(dialog.contents());
	Wt::WPushButton ok(Wt::WString::tr("dialog.verify_email.exit_button"), dialog.contents());

	listName->setStyleClass("dialog-window-base");

	Wt::WApplication::instance()->globalEscapePressed().connect(&dialog, &Wt::WDialog::accept);
	ok.clicked().connect(&dialog, &Wt::WDialog::accept);
	dialog.exec();
}

void W::showShareDialog(const Wt::WString & linkTextEncoded, const Wt::WString & linkText, const Wt::WString & shareText, const std::string & locale)
{
	W::Log::info() << "Sharing link: " << linkText;

	Wt::WDialog dialog(Wt::WString::tr("dialog.share.title"));
	Wt::WText * shareListDialogText = new Wt::WText(Wt::WString::tr("dialog.share.text"), dialog.contents());
	new Wt::WBreak(dialog.contents());
	Wt::WLineEdit * linkLine = new Wt::WLineEdit(linkText, dialog.contents());
	new Wt::WBreak(dialog.contents());
	Wt::WText * share = new Wt::WText(dialog.contents());
	new Wt::WBreak(dialog.contents());
	Wt::WPushButton ok(Wt::WString::tr("dialog.share.exit_button"), dialog.contents());

	linkLine->setReadOnly(true);
	linkLine->setStyleClass("dialog-window-base");
	shareListDialogText->setStyleClass("dialog-window-base");
	ok.setStyleClass("left");

	share->setTextFormat(Wt::XHTMLUnsafeText);
	share->setText(Wt::WString("<div id=\"voteShare\" class=\"addthis_toolbox addthis_default_style addthis_32x32_style\"><a class=\"addthis_button_twitter\"></a><a class=\"addthis_button_email\"></a><a class=\"addthis_button_preferred_rank\"></a><a class=\"addthis_button_compact\"></a></div>"));
	Wt::WApplication::instance()->doJavaScript(Wt::WString("renderAddthisShareButton(\"{1}\", \"{2}\");").arg(linkText).arg(shareText).toUTF8());

	Wt::WApplication::instance()->globalEscapePressed().connect(&dialog, &Wt::WDialog::accept);
	ok.clicked().connect(&dialog, &Wt::WDialog::accept);
	dialog.exec();
}

void W::showUserErrorDialog(const Wt::WString & errorText)
{
	Wt::WDialog dialog(Wt::WString::tr("dialog.usererror.title"));
	Wt::WText * errorTextWidget = new Wt::WText(errorText, dialog.contents());
	new Wt::WBreak(dialog.contents());
	Wt::WPushButton ok(Wt::WString::tr("dialog.usererror.exit_button"), dialog.contents());

	errorTextWidget->setStyleClass("dialog-window-base");

	Wt::WApplication::instance()->globalEscapePressed().connect(&dialog, &Wt::WDialog::accept);
	ok.clicked().connect(&dialog, &Wt::WDialog::accept);
	dialog.exec();
}

