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
    if (LogLevel == L"Info")
    {
        ParsedLogLevel = Info;
    }
    else if (LogLevel == L"Warning")
    {
        ParsedLogLevel = Warning;
    }
    else if (LogLevel == L"Error")
    {
        ParsedLogLevel = Error;
    }
    else
    {
        Log(Error, L"NPrintLogAction, Bad log level specified '{}'", LogLevel);
    }

    Log(ParsedLogLevel, Message.c_str());
}
