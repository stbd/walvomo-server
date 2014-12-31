#include "Application.hpp"
#include "Logger.hpp"
#include "ContentManager.hpp"

#include <Wt/WEnvironment>
#include <stdexcept>
#include <Wt/WServer>

Wt::WApplication * createApplication(const Wt::WEnvironment & env)
{
	W::Application * app = 0;
	std::string	locale;
	Wt::WServer::instance()->readConfigurationProperty("default-locale", locale);

	if (!locale.empty()) {
		W::Log::info() << "Using locales from file: " << locale;
		app = new W::Application(env, locale);
	} else {
		W::Log::info() << "Default locale not found";
		app = new W::Application(env);
	}
	if (app == 0) {
		throw std::runtime_error ("Creating application failed");
	}
	return app;
}

int main(int argc, char **argv)
{
	try {
		Wt::WServer server(argv[0]);
		server.setServerConfiguration(argc, argv);
		server.addEntryPoint(Wt::Application, createApplication);

		W::Application::configure(server);															//Do "once for application" stuff
		W::Log::info() << "Size of preloaded content in memory: "
				<< W::ContentManager::getInstance()->getContentByteSize() << " bytes ("
				<< (float)W::ContentManager::getInstance()->getContentByteSize()/(1024.0f*1024.0f) << " MB)";

		if (server.start()) {
			W::Log::info() << "Server started";
			Wt::WServer::waitForShutdown();
			server.stop();
		}
	} catch (Wt::WServer::Exception & e) {
		W::Log::error() << e.what();
	} catch (std::exception & e) {
		W::Log::error() << e.what();
	}
	delete W::ContentManager::getInstance();
	W::Application::shutdown();															//Do "once for application" stuff
}
