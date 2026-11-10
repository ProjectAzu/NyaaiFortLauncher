module CopyFileAction;

void NCopyFileAction::Execute()
{
    From = ResolvePossiblyFortniteBuildRelativePath(From);
    To = ResolvePossiblyFortniteBuildRelativePath(To);

    if (!std::filesystem::exists(From))
    {
        Log(Error, L"Can't copy file \"{}\" as it doesn't exist", From.wstring());

        return;
    }

    if (std::filesystem::exists(To))
    {
        std::filesystem::remove(To);
    }

    std::filesystem::copy(From, To);
    Log(Info, L"Copied file \"{}\" to \"{}\"", From.wstring(), To.wstring());
}
