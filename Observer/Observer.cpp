#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include "..\gen-versioninfo.h"

#include "markets\binance\BinanceTrader.h"
#include "Strategy.h"
#include "Logger.h"
#include "Network.h"

using namespace std;

int main() {
    try
    {
        SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
        Logger::getInstance().addFile(Logger::level::info, "log.txt");
        Logger::getInstance().addFile(Logger::level::debug, "debuglog.txt");
        Logger::getInstance().setConsoleLogLevel(Logger::level::info);

        filesystem::create_directory("data");

        string flagFile = "running.flag";
        ofstream flag(flagFile, fstream::out);
        flag << "running";
        flag.close();

        BinanceTrader::m_bookHits = 0;
        auto api = make_shared<BinanceTrader>("API_KEY", "API_SIGN");
        api->connect();

        auto trader = make_shared<Strategy>(api);
        trader->connect();

        while (filesystem::exists(flagFile))
            Sleep(10 * 1000);

        trader->disconnect();
        trader->showStats();

        Logger::i("test balanse start: " + api->getBalanceSummary(true).to_string());
        Logger::i("test balanse end: " + api->getBalanceSummary().to_string());
        Logger::i("book hits: " + to_string(BinanceTrader::m_bookHits));

        api->disconnect();
        Network::disconnect();
    }
    catch (exception e)
    {
        Logger::e("Exception caught: " + string(e.what()));
    }
    catch (...) {
        Logger::e("Unknown exception caught: ");
    }
         
    return 0;
}
