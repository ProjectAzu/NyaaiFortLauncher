#include "PrintLogAction.h"

GENERATE_BASE_CPP(NPrintLogAction)

NPrintLogAction::NPrintLogAction()
{
    bPrintExecutingActionMessage = false;
}

void NPrintLogAction::Execute()
{
    Super::Execute();

    ELogLevel ParsedLogLevel = Info;
    if (LogLevel == "Info")
    {
        ParsedLogLevel = Info;
    }
    else if (LogLevel == "Warning")
    {
        ParsedLogLevel = Warning;
    }
    else if (LogLevel == "Error")
    {
        ParsedLogLevel = Error;
    }
    else
    {
        Log(Error, "NPrintLogAction, Bad log level specified '{}'", LogLevel);
    }

    Log(ParsedLogLevel, Message.c_str());
}
