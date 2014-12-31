#ifndef DIALOG_WIDGETS_HPP_
#define DIALOG_WIDGETS_HPP_

#include <string>

namespace Wt
{
	class WString;
}

namespace W
{
	void showTermsOfUsage(void);
	void showCreateAccountDialog(void);
	void showVerifyEmailNotificationDialog(void);
	void showShareDialog(const Wt::WString & linkTextEncoded, const Wt::WString & linkText, const Wt::WString & shareText, const std::string & locale);
	void showUserErrorDialog(const Wt::WString & errorText);
}

#endif
